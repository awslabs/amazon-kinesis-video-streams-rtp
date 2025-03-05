#include <stdio.h>
#include <string.h>
#include "h265_depacketizer.h"

/*-------------------------------------------------------------------------------------------------------------*/

static H265Result_t H265Depacketizer_ProcessSingleNalu( H265DepacketizerContext_t * pCtx,
                                                        H265Nalu_t * pNalu );

static H265Result_t H265Depacketizer_ProcessFragmentationUnit( H265DepacketizerContext_t * pCtx,
                                                               H265Nalu_t * pNalu );

static H265Result_t H265Depacketizer_ProcessAggregationPacket( H265DepacketizerContext_t * pCtx,
                                                               H265Nalu_t * pNalu );

static void H265Depacketizer_SetNalu( const H265Packet_t * pCurrentPacket,
                                      H265Nalu_t * pNalu,
                                      size_t offset,
                                      uint16_t nalu_size );

/*-------------------------------------------------------------------------------------------------------------*/

static void H265Depacketizer_SetNalu( const H265Packet_t * pCurrentPacket,
                                      H265Nalu_t * pNalu,
                                      size_t offset,
                                      uint16_t nalu_size )
{
    pNalu->pNaluData = &pCurrentPacket->pPacketData[ offset ];
    pNalu->naluDataLength = nalu_size;
    pNalu->nal_unit_type = ( pCurrentPacket->pPacketData[ offset ] >> 1 ) & 0x3F;
    pNalu->nal_layer_id = ( ( pCurrentPacket->pPacketData[ offset ] & 0x01 ) << 5 ) |
                          ( ( pCurrentPacket->pPacketData[ offset + 1 ] & 0xF8 ) >> 3 );
    pNalu->temporal_id = pCurrentPacket->pPacketData[ offset + 1 ] & 0x07;
}

/*-------------------------------------------------------------------------------------------------------------*/

static H265Result_t H265Depacketizer_ProcessSingleNalu( H265DepacketizerContext_t * pCtx,
                                                        H265Nalu_t * pNalu )
{
    H265Result_t result = H265_RESULT_OK;
    H265Packet_t * pCurrentPacket = NULL;
    uint16_t donl = 0;

/* Get current packet */
    pCurrentPacket = &pCtx->pPacketsArray[ pCtx->tailIndex ];

    if( pCtx->donPresent )
    {
/* Validate packet has enough bytes for DONL */
        if( pCurrentPacket->packetDataLength < ( NALU_HEADER_SIZE + DONL_FIELD_SIZE ) )
        {
            result = H265_RESULT_MALFORMED_PACKET;
        }

        if( result == H265_RESULT_OK )
        {
/* Extract DONL value */
            donl = ( pCurrentPacket->pPacketData[ NALU_HEADER_SIZE ] << 8 ) |
                   pCurrentPacket->pPacketData[ NALU_HEADER_SIZE + 1 ];
            pCtx->currentDon = donl;

/* Move data to create proper NALU structure */
/* First move payload bytes right after NAL header */
            memmove( pCurrentPacket->pPacketData + NALU_HEADER_SIZE,
                     pCurrentPacket->pPacketData + NALU_HEADER_SIZE + DONL_FIELD_SIZE,
                     pCurrentPacket->packetDataLength - ( NALU_HEADER_SIZE + DONL_FIELD_SIZE ) );

/* Set NALU data */
            pNalu->pNaluData = pCurrentPacket->pPacketData;
            pNalu->naluDataLength = pCurrentPacket->packetDataLength - DONL_FIELD_SIZE;
        }
    }
    else
    {
/* No DONL - use packet data directly */
        pNalu->pNaluData = pCurrentPacket->pPacketData;
        pNalu->naluDataLength = pCurrentPacket->packetDataLength;
    }

    if( result == H265_RESULT_OK )
    {
/* Set other NALU fields */
        H265Depacketizer_SetNalu( pCurrentPacket, pNalu, 0, pNalu->naluDataLength );

/* Update context */
        pCtx->tailIndex++;
        pCtx->packetCount--;
    }

    return result;
}



/*-------------------------------------------------------------------------------------------------------------*/

static H265Result_t H265Depacketizer_ProcessFragmentationUnit( H265DepacketizerContext_t * pCtx,
                                                               H265Nalu_t * pNalu )
{
    H265Result_t result = H265_RESULT_OK;
    H265Packet_t * pCurrentPacket;
    uint8_t fu_header;
    uint8_t start_bit;
    uint8_t end_bit;
    uint8_t fu_type;
    size_t payload_offset;
    size_t payloadLength;
    uint16_t donl;
    size_t totalLength;
    uint8_t f_bit;
    uint8_t layer_id;
    uint8_t tid;

/* Get current packet */
    pCurrentPacket = &pCtx->pPacketsArray[ pCtx->tailIndex ];

    if( pCurrentPacket->packetDataLength >= TOTAL_FU_HEADER_SIZE )
    {
/* Get FU header */
        fu_header = pCurrentPacket->pPacketData[ FU_HEADER_OFFSET ];
        start_bit = ( fu_header & FU_HEADER_S_BIT_MASK ) >> FU_HEADER_S_BIT_LOCATION;
        end_bit = ( fu_header & FU_HEADER_E_BIT_MASK ) >> FU_HEADER_E_BIT_LOCATION;
        fu_type = fu_header & FU_HEADER_TYPE_MASK;

/* Calculate initial payload offset */
        payload_offset = TOTAL_FU_HEADER_SIZE;

/* Handle DON fields if present */
        if( pCtx->donPresent )
        {
            if( start_bit )
            {
/* First fragment should have DONL */
                if( pCurrentPacket->packetDataLength >= ( payload_offset + DONL_FIELD_SIZE ) )
                {
/* Extract DONL */
                    donl = ( pCurrentPacket->pPacketData[ payload_offset ] << 8 ) |
                           pCurrentPacket->pPacketData[ payload_offset + 1 ];
                    pCtx->fuDepacketizationState.donl = donl;
                    payload_offset += DONL_FIELD_SIZE;
                }
                else
                {
                    result = H265_RESULT_MALFORMED_PACKET;
                }
            }
            else
            {
/* Subsequent fragments should have DOND */
                if( pCurrentPacket->packetDataLength >= ( payload_offset + AP_DOND_SIZE ) )
                {
                    payload_offset += AP_DOND_SIZE;
                }
                else
                {
                    result = H265_RESULT_MALFORMED_PACKET;
                }
            }
        }

/* Handle start of fragmented NAL unit */
        if( ( start_bit ) && ( result == H265_RESULT_OK ) )
        {
/* Initialize FU state */
            pCtx->currentlyProcessingPacket = H265_FU_PACKET;

/* Store payload header from first fragment */
            pCtx->fuDepacketizationState.payloadHdr =
                ( pCurrentPacket->pPacketData[ 0 ] << 8 ) | pCurrentPacket->pPacketData[ 1 ];

/* Store FU header and original NAL type */
            pCtx->fuDepacketizationState.fuHeader = fu_header;
            pCtx->fuDepacketizationState.originalNalType = fu_type;
            pCtx->fuDepacketizationState.reassembledLength = 0;

/* Calculate payload length */
            payloadLength = pCurrentPacket->packetDataLength - payload_offset;

/* Check buffer space */
            if( payloadLength <= pCtx->fuDepacketizationState.reassemblyBufferSize )
            {
/* Copy payload data */
                memcpy( pCtx->fuDepacketizationState.pReassemblyBuffer,
                        pCurrentPacket->pPacketData + payload_offset,
                        payloadLength );

                pCtx->fuDepacketizationState.reassembledLength = payloadLength;
            }
            else
            {
                result = H265_RESULT_MALFORMED_PACKET;
            }
        }
/* If not start bit, verify we're in FU processing state */
        else if( ( pCtx->currentlyProcessingPacket != H265_FU_PACKET ) &&
                 ( result == H265_RESULT_OK ) )
        {
            result = H265_RESULT_MALFORMED_PACKET;
        }
        else if( result == H265_RESULT_OK )
        {
/* Continue reassembly for middle and end fragments */
            payloadLength = pCurrentPacket->packetDataLength - payload_offset;

/* Check buffer space */
            if( ( pCtx->fuDepacketizationState.reassembledLength + payloadLength ) <=
                pCtx->fuDepacketizationState.reassemblyBufferSize )
            {
/* Copy fragment payload */
                memcpy( pCtx->fuDepacketizationState.pReassemblyBuffer +
                        pCtx->fuDepacketizationState.reassembledLength,
                        pCurrentPacket->pPacketData + payload_offset,
                        payloadLength );

                pCtx->fuDepacketizationState.reassembledLength += payloadLength;
            }
            else
            {
                result = H265_RESULT_MALFORMED_PACKET;
            }
        }

/* If this is the end fragment */
        if( ( end_bit ) && ( result == H265_RESULT_OK ) )
        {
/* Calculate total NAL unit size */
            totalLength = NALU_HEADER_SIZE +
                          pCtx->fuDepacketizationState.reassembledLength;

/* Extract header fields */
            fu_type = pCtx->fuDepacketizationState.fuHeader & FU_HEADER_TYPE_MASK;
            f_bit = ( pCtx->fuDepacketizationState.payloadHdr >> 15 ) & 0x01;
            layer_id = ( pCtx->fuDepacketizationState.payloadHdr >> 3 ) & 0x3F;
            tid = pCtx->fuDepacketizationState.payloadHdr & 0x07;

/* Shift payload right by NALU_HEADER_SIZE to make room for header */
            memmove( pCtx->fuDepacketizationState.pReassemblyBuffer + NALU_HEADER_SIZE,
                     pCtx->fuDepacketizationState.pReassemblyBuffer,
                     pCtx->fuDepacketizationState.reassembledLength );

/* Build NAL header at start of buffer */
            pCtx->fuDepacketizationState.pReassemblyBuffer[ 0 ] =
                ( f_bit << 7 ) | ( fu_type << 1 ) | ( ( layer_id >> 5 ) & 0x01 );
            pCtx->fuDepacketizationState.pReassemblyBuffer[ 1 ] =
                ( ( layer_id & 0x1F ) << 3 ) | tid;

/* Create a temporary H265Packet_t structure */
            H265Packet_t tempPacket =
            {
                .pPacketData      = pCtx->fuDepacketizationState.pReassemblyBuffer,
                .packetDataLength = totalLength,
                .maxPacketSize    = pCtx->fuDepacketizationState.reassemblyBufferSize
            };

            H265Depacketizer_SetNalu(
                &tempPacket,
                pNalu,
                0, 
                totalLength
                );

/* Reset processing state */
            pCtx->currentlyProcessingPacket = H265_PACKET_NONE;
            pCtx->fuDepacketizationState.reassembledLength = 0;
        }

        if( result == H265_RESULT_OK )
        {
/* Update context */
            pCtx->tailIndex++;
            pCtx->packetCount--;
        }
    }
    else
    {
        result = H265_RESULT_MALFORMED_PACKET;
    }

    return result;
}

/*-------------------------------------------------------------------------------------------------------------*/

static H265Result_t H265Depacketizer_ProcessAggregationPacket( H265DepacketizerContext_t * pCtx,
                                                               H265Nalu_t * pNalu )
{
    H265Result_t result = H265_RESULT_OK;
    H265Packet_t * pCurrentPacket;
    size_t minSize;
    size_t offset;
    uint16_t nalu_size;
    uint16_t donl;
    uint8_t dond;

/* Get current packet */
    pCurrentPacket = &pCtx->pPacketsArray[ pCtx->tailIndex ];

/* Calculate minimum required size */
    minSize = AP_HEADER_SIZE + ( 2 * AP_NALU_LENGTH_FIELD_SIZE + 2 * NALU_HEADER_SIZE );

    if( pCtx->donPresent )
    {
        minSize += DONL_FIELD_SIZE + AP_DOND_SIZE;         /* DONL for first, DOND for second */
    }

    if( pCurrentPacket->packetDataLength >= minSize )
    {
/* If this is first NAL unit in AP, initialize state */
        if( pCtx->currentlyProcessingPacket != H265_AP_PACKET )
        {
            pCtx->currentlyProcessingPacket = H265_AP_PACKET;
            pCtx->apDepacketizationState.currentOffset = AP_HEADER_SIZE;

/* Handle DONL if present */
            if( pCtx->donPresent )
            {
                donl = ( pCurrentPacket->pPacketData[ pCtx->apDepacketizationState.currentOffset ] << 8 ) |
                       pCurrentPacket->pPacketData[ pCtx->apDepacketizationState.currentOffset + 1 ];
                pCtx->currentDon = donl;
                pCtx->apDepacketizationState.currentOffset += DONL_FIELD_SIZE;
            }

            pCtx->apDepacketizationState.firstUnit = 1;

/* Get first NAL unit size */
            offset = pCtx->apDepacketizationState.currentOffset;
            nalu_size = ( pCurrentPacket->pPacketData[ offset ] << 8 ) |
                        pCurrentPacket->pPacketData[ offset + 1 ];
            offset += AP_NALU_LENGTH_FIELD_SIZE;

/* Validate NALU size */
            if( offset + nalu_size <= pCurrentPacket->packetDataLength )
            {
                /* Add buffer size check */
                if( nalu_size <= pNalu->naluDataLength )
                {
                    /* Set NALU fields for first unit */
                    H265Depacketizer_SetNalu( pCurrentPacket, pNalu, offset, nalu_size );

                    /* Update offset for next time */
                    offset += nalu_size;
                    pCtx->apDepacketizationState.currentOffset = offset;
                }
                else
                {
                    result = H265_RESULT_OUT_OF_MEMORY;
                }
            }
            else
            {
                result = H265_RESULT_MALFORMED_PACKET;
            }
        }
        else
        {
/* Get next NAL unit from AP */
            offset = pCtx->apDepacketizationState.currentOffset;

/* Handle DOND for non-first units */
            if( pCtx->donPresent )
            {
                dond = pCurrentPacket->pPacketData[ offset ];
                pCtx->currentDon += dond;
                offset += AP_DOND_SIZE;
            }

/* Get NALU size */
            nalu_size = ( pCurrentPacket->pPacketData[ offset ] << 8 ) |
                        pCurrentPacket->pPacketData[ offset + 1 ];
            offset += AP_NALU_LENGTH_FIELD_SIZE;

/* Validate NALU size */
            if( offset + nalu_size <= pCurrentPacket->packetDataLength )
            {
                /* Add buffer size check */
                if( nalu_size <= pNalu->naluDataLength )
                {
                    /* Set NALU fields */
                    H265Depacketizer_SetNalu( pCurrentPacket, pNalu, offset, nalu_size );

                    /* Update state */
                    offset += nalu_size;
                    pCtx->apDepacketizationState.firstUnit = 0;

                    /* Check if we've processed all NAL units in this AP */
                    if( offset >= pCurrentPacket->packetDataLength )
                    {
                        pCtx->currentlyProcessingPacket = H265_PACKET_NONE;
                        pCtx->tailIndex++;
                        pCtx->packetCount--;
                    }
                    else
                    {
                        pCtx->apDepacketizationState.currentOffset = offset;
                    }
                }
                else
                {
                    result = H265_RESULT_OUT_OF_MEMORY;
                }
            }
            else
            {
                result = H265_RESULT_MALFORMED_PACKET;
            }
        }
    }
    else
    {
        result = H265_RESULT_MALFORMED_PACKET;
    }

    return result;
}


/*-------------------------------------------------------------------------------------------------------------*/

H265Result_t H265Depacketizer_Init( H265DepacketizerContext_t * pCtx,
                                    H265Packet_t * pPacketsArray,
                                    size_t packetsArrayLength,
                                    uint8_t donPresent )
{
    H265Result_t result = H265_RESULT_OK;

    /* Parameter validation */
    if( ( pCtx == NULL ) || ( pPacketsArray == NULL ) || ( packetsArrayLength == 0 ) )
    {
        result = H265_RESULT_BAD_PARAM;
    }

    if( result == H265_RESULT_OK )
    {
        /* Initialize indices */
        pCtx->headIndex = 0;
        pCtx->tailIndex = 0;
        pCtx->curPacketIndex = 0;
        pCtx->packetCount = 0;

        /* Initialize packet array */
        pCtx->pPacketsArray = pPacketsArray;
        pCtx->packetsArrayLength = packetsArrayLength;

        /* Initialize DON handling */
        pCtx->donPresent = donPresent; /* Only this is needed */
        pCtx->currentDon = 0;          /* For tracking current DON value */

        /* Initialize packet processing state */
        pCtx->currentlyProcessingPacket = H265_PACKET_NONE;

        /* Initialize FU state */
        memset( &pCtx->fuDepacketizationState, 0, sizeof( pCtx->fuDepacketizationState ) );

        /* Initialize AP state */
        memset( &pCtx->apDepacketizationState, 0, sizeof( pCtx->apDepacketizationState ) );
    }

    return result;
}

/*-------------------------------------------------------------------------------------------------------------*/

H265Result_t H265Depacketizer_AddPacket( H265DepacketizerContext_t * pCtx,
                                         const H265Packet_t * pPacket )
{
    H265Result_t result = H265_RESULT_OK;

    if( ( pCtx == NULL ) || ( pPacket == NULL ) )
    {
        result = H265_RESULT_BAD_PARAM;
    }

    if( result == H265_RESULT_OK )
    {
        if( pCtx->packetCount >= pCtx->packetsArrayLength )
        {
            result = H265_RESULT_NO_MORE_PACKETS;
        }
    }

    if( result == H265_RESULT_OK )
    {
        pCtx->pPacketsArray[ pCtx->headIndex ].pPacketData = pPacket->pPacketData;
        pCtx->pPacketsArray[ pCtx->headIndex ].packetDataLength = pPacket->packetDataLength;
        pCtx->headIndex += 1;
        pCtx->packetCount += 1;
    }

    return result;
}

/*-------------------------------------------------------------------------------------------------------------*/

H265Result_t H265Depacketizer_GetNalu( H265DepacketizerContext_t * pCtx,
                                       H265Nalu_t * pNalu )
{
    H265Result_t result = H265_RESULT_OK;
    uint8_t nal_unit_type;

    if( ( pCtx == NULL ) || ( pNalu == NULL ) )
    {
        result = H265_RESULT_BAD_PARAM;
    }

    if( result == H265_RESULT_OK )
    {
        if( pCtx->packetCount == 0 )
        {
            result = H265_RESULT_NO_MORE_NALUS;
        }
    }

    if( result == H265_RESULT_OK )
    {
        nal_unit_type = ( pCtx->pPacketsArray[ pCtx->tailIndex ].pPacketData[ 0 ] >> 1 ) & FU_HEADER_TYPE_MASK;

        if( ( nal_unit_type >= SINGLE_NALU_PACKET_TYPE_START ) &&
            ( nal_unit_type <= SINGLE_NALU_PACKET_TYPE_END ) )
        {
            result = H265Depacketizer_ProcessSingleNalu( pCtx, pNalu );
        }
        else if( nal_unit_type == FU_PACKET_TYPE )
        {
            result = H265Depacketizer_ProcessFragmentationUnit( pCtx, pNalu );
        }
        else if( nal_unit_type == AP_PACKET_TYPE )
        {
            result = H265Depacketizer_ProcessAggregationPacket( pCtx, pNalu );
        }
        else
        {
            result = H265_RESULT_UNSUPPORTED_PACKET;
        }
    }

    return result;
}

/*-------------------------------------------------------------------------------------------------------------*/

H265Result_t H265Depacketizer_GetFrame( H265DepacketizerContext_t * pCtx,
                                        H265Frame_t * pFrame )
{
    H265Result_t result = H265_RESULT_OK;
    size_t currentOffset = 0;
    H265Nalu_t nalu;
    const uint8_t startCode[] = { 0x00, 0x00, 0x00, 0x01 };

/* Parameter validation */
    if( ( pCtx == NULL ) || ( pFrame == NULL ) ||
        ( pFrame->pFrameData == NULL ) || ( pFrame->frameDataLength == 0 ) )
    {
        result = H265_RESULT_BAD_PARAM;
    }

/* Process NAL units until we hit an error or run out */
    while( result == H265_RESULT_OK )
    {
        result = H265Depacketizer_GetNalu( pCtx, &nalu );

        if( result == H265_RESULT_NO_MORE_NALUS )
        {
            if( currentOffset > 0 )      /* If we've processed at least one NALU */
            {
                result = H265_RESULT_OK;
            }
            else
            {
                result = H265_RESULT_NO_MORE_FRAMES;
            }

            break;
        }
        else if( result == H265_RESULT_OK )
        {
            if( ( pFrame->frameDataLength - currentOffset ) < ( sizeof( startCode ) + nalu.naluDataLength ) )
            {
                result = H265_RESULT_BUFFER_TOO_SMALL;
                break;
            }

/* Add start code */
            memcpy( pFrame->pFrameData + currentOffset,
                    startCode,
                    sizeof( startCode ) );
            currentOffset += sizeof( startCode );

/* Add NAL unit */
            memcpy( pFrame->pFrameData + currentOffset,
                    nalu.pNaluData,
                    nalu.naluDataLength );
            currentOffset += nalu.naluDataLength;
        }
    }

/* Set frame length if we successfully built a frame */
    if( result == H265_RESULT_OK )
    {
        pFrame->frameDataLength = currentOffset;
    }

    return result;
}

/*-------------------------------------------------------------------------------------------------------------*/

H265Result_t H265Depacketizer_GetPacketProperties( const uint8_t * pPacketData,
                                                   const size_t packetDataLength,
                                                   uint32_t * pProperties )
{
    H265Result_t result = H265_RESULT_OK;
    uint8_t nal_unit_type;
    uint8_t fu_header;

/* Parameter validation */
    if( ( pPacketData == NULL ) || ( pProperties == NULL ) ||
        ( packetDataLength < NALU_HEADER_SIZE ) )
    {
        result = H265_RESULT_BAD_PARAM;
    }

    if( result == H265_RESULT_OK )
    {
/* Initialize properties */
        *pProperties = 0;

/* Get NAL unit type from first byte */
        nal_unit_type = ( pPacketData[ 0 ] >> 1 ) & FU_HEADER_TYPE_MASK;

/* Check packet type and set properties */
        if( nal_unit_type == FU_PACKET_TYPE )
        {
/* Validate FU packet has enough bytes for FU header */
            if( packetDataLength < TOTAL_FU_HEADER_SIZE )
            {
                result = H265_RESULT_MALFORMED_PACKET;
            }
            else
            {
/* Get FU header */
                fu_header = pPacketData[ 2 ];

/* Check start and end bits */
                if( fu_header & FU_HEADER_S_BIT_MASK )
                {
                    *pProperties |= H265_PACKET_PROPERTY_START_PACKET;
                }

                if( fu_header & FU_HEADER_E_BIT_MASK )
                {
                    *pProperties |= H265_PACKET_PROPERTY_END_PACKET;
                }
            }
        }
        else if( nal_unit_type == AP_PACKET_TYPE ) /* Type 48 for AP */
        {
/* Validate minimum AP packet size */
            if( packetDataLength < ( NALU_HEADER_SIZE + AP_NALU_LENGTH_FIELD_SIZE ) )
            {
                result = H265_RESULT_MALFORMED_PACKET;
            }
            else
            {
/* AP packets are always complete units */
                *pProperties |= ( H265_PACKET_PROPERTY_START_PACKET |
                                  H265_PACKET_PROPERTY_END_PACKET );
            }
        }
        else if( ( nal_unit_type >= SINGLE_NALU_PACKET_TYPE_START ) && /* Type 1-47 */
                 ( nal_unit_type <= SINGLE_NALU_PACKET_TYPE_END ) )
        {
/* Single NAL unit packet - both start and end */
            *pProperties |= ( H265_PACKET_PROPERTY_START_PACKET |
                              H265_PACKET_PROPERTY_END_PACKET );
        }
        else
        {
            result = H265_RESULT_UNSUPPORTED_PACKET;
        }
    }

    return result;
}


/*-------------------------------------------------------------------------------------------------------------*/
