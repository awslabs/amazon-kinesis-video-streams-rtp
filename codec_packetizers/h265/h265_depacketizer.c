#include <string.h>
#include "h265_depacketizer.h"

/*-------------------------------------------------------------------------------------------------------------*/

static H265Result_t H265Depacketizer_ProcessSingleNalu( H265DepacketizerContext_t * pCtx,
                                                        H265Nalu_t * pNalu );

static H265Result_t H265Depacketizer_ProcessFragmentationUnit( H265DepacketizerContext_t * pCtx,
                                                               H265Nalu_t * pNalu );

static H265Result_t H265Depacketizer_ProcessAggregationPacket( H265DepacketizerContext_t * pCtx,
                                                               H265Nalu_t * pNalu );

/*-------------------------------------------------------------------------------------------------------------*/

static H265Result_t H265Depacketizer_ProcessSingleNalu( H265DepacketizerContext_t * pCtx,
                                                        H265Nalu_t * pNalu ) /* Changed parameter */
{
/* Parameter validation */
    if( ( pCtx == NULL ) || ( pNalu == NULL ) )
    {
        return H265_RESULT_BAD_PARAM;
    }

/* Check if we have packets to process */
    if( pCtx->packetCount == 0 )
    {
        return H265_RESULT_NO_MORE_NALUS;
    }

/* Get current packet */
    H265Packet_t * pCurrentPacket = &pCtx->pPacketsArray[ pCtx->tailIndex ];

/* Validate NAL unit type */
    uint8_t nal_unit_type = ( pCurrentPacket->pPacketData[ 0 ] >> 1 ) & 0x3F;

    if( ( nal_unit_type < SINGLE_NALU_PACKET_TYPE_START ) ||
        ( nal_unit_type > SINGLE_NALU_PACKET_TYPE_END ) )
    {
        return H265_RESULT_MALFORMED_PACKET;
    }

    if( pCtx->donPresent )
    {
/* Validate DONL presence */
        if( pCurrentPacket->packetDataLength < ( NALU_HEADER_SIZE + DONL_FIELD_SIZE ) )
        {
            return H265_RESULT_MALFORMED_PACKET;
        }

/* Set NALU data excluding DONL */
        pNalu->pNaluData = pCurrentPacket->pPacketData;
        pNalu->naluDataLength = pCurrentPacket->packetDataLength - DONL_FIELD_SIZE;

/* Extract DON if needed */
        pCtx->currentDon = ( pCurrentPacket->pPacketData[ NALU_HEADER_SIZE ] << 8 ) |
                           pCurrentPacket->pPacketData[ NALU_HEADER_SIZE + 1 ];
    }
    else
    {
/* Set NALU data */
        pNalu->pNaluData = pCurrentPacket->pPacketData;
        pNalu->naluDataLength = pCurrentPacket->packetDataLength;
    }

/* Set NALU parameters */
    pNalu->nal_unit_type = nal_unit_type;
    pNalu->nal_layer_id = ( ( pCurrentPacket->pPacketData[ 0 ] & 0x01 ) << 5 ) |
                          ( ( pCurrentPacket->pPacketData[ 1 ] & 0xF8 ) >> 3 );
    pNalu->temporal_id = pCurrentPacket->pPacketData[ 1 ] & 0x07;

/* Update context */
    pCtx->tailIndex++;
    pCtx->packetCount--;

    return H265_RESULT_OK;
}

/*-------------------------------------------------------------------------------------------------------------*/

static H265Result_t H265Depacketizer_ProcessFragmentationUnit( H265DepacketizerContext_t * pCtx,
                                                               H265Nalu_t * pNalu )

{
    /* Parameter validation */
    if( ( pCtx == NULL ) || ( pNalu == NULL ) )
    {
        return H265_RESULT_BAD_PARAM;
    }

    /* Check if packet array is full */
    if( pCtx->packetCount == 0 )
    {
        return H265_RESULT_NO_MORE_PACKETS;
    }

    /* Get current packet */
    H265Packet_t * pCurrentPacket = &pCtx->pPacketsArray[ pCtx->tailIndex ];

    if( ( pCurrentPacket->pPacketData == NULL ) ||
        ( pCurrentPacket->packetDataLength < TOTAL_FU_HEADER_SIZE ) )
    {
        return H265_RESULT_MALFORMED_PACKET;
    }

    /* Get FU header */
    uint8_t fu_header = pCurrentPacket->pPacketData[ 2 ];
    uint8_t start_bit = ( fu_header & FU_HEADER_S_BIT_MASK ) >> FU_HEADER_S_BIT_LOCATION;
    uint8_t end_bit = ( fu_header & FU_HEADER_E_BIT_MASK ) >> FU_HEADER_E_BIT_LOCATION;
    uint8_t fu_type = fu_header & FU_HEADER_TYPE_MASK;

    /* Calculate initial payload offset */
    size_t payload_offset = TOTAL_FU_HEADER_SIZE;

    /* Handle DON fields if present */
    if( pCtx->donPresent )
    {
        if( start_bit )
        {
            /* First fragment should have DONL */
            if( pCurrentPacket->packetDataLength < ( payload_offset + DONL_FIELD_SIZE ) )
            {
                return H265_RESULT_MALFORMED_PACKET;
            }

            /* Extract DONL */
            uint16_t donl = ( pCurrentPacket->pPacketData[ payload_offset ] << 8 ) |
                            pCurrentPacket->pPacketData[ payload_offset + 1 ];
            pCtx->fuDepacketizationState.donl = donl;
            payload_offset += DONL_FIELD_SIZE;
        }
        else
        {
            /* Subsequent fragments should have DOND */
            if( pCurrentPacket->packetDataLength < ( payload_offset + AP_DOND_SIZE ) )
            {
                return H265_RESULT_MALFORMED_PACKET;
            }

            /* Extract DOND */
            uint8_t dond = pCurrentPacket->pPacketData[ payload_offset ];

            payload_offset += AP_DOND_SIZE;
        }
    }

    /* Handle start of fragmented NAL unit */
    if( start_bit )
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

        /* Calculate payload length (accounting for DON fields) */
        size_t payloadLength = pCurrentPacket->packetDataLength - payload_offset;

        /* Check buffer space */
        if( payloadLength > pCtx->fuDepacketizationState.reassemblyBufferSize )
        {
            return H265_RESULT_MALFORMED_PACKET;
        }

        /* Copy payload data */
        memcpy( pCtx->fuDepacketizationState.pReassemblyBuffer,
                pCurrentPacket->pPacketData + payload_offset,
                payloadLength );

        pCtx->fuDepacketizationState.reassembledLength = payloadLength;
    }
    /* If not start bit, verify we're in FU processing state */
    else if( pCtx->currentlyProcessingPacket != H265_FU_PACKET )
    {
        return H265_RESULT_MALFORMED_PACKET;
    }
    else
    {
        /* Continue reassembly for middle and end fragments */
        size_t payloadLength = pCurrentPacket->packetDataLength - payload_offset;

        /* Check buffer space */
        if( ( pCtx->fuDepacketizationState.reassembledLength + payloadLength ) >
            pCtx->fuDepacketizationState.reassemblyBufferSize )
        {
            return H265_RESULT_MALFORMED_PACKET;
        }

        /* Copy fragment payload */
        memcpy( pCtx->fuDepacketizationState.pReassemblyBuffer +
                pCtx->fuDepacketizationState.reassembledLength,
                pCurrentPacket->pPacketData + payload_offset,
                payloadLength );

        pCtx->fuDepacketizationState.reassembledLength += payloadLength;
    }

    /* If this is the end fragment */
    if( end_bit )
    {
        /* Calculate total NAL unit size */
        size_t totalLength = NALU_HEADER_SIZE +
                             pCtx->fuDepacketizationState.reassembledLength;

        /* Extract header fields */
        uint8_t fu_type = pCtx->fuDepacketizationState.fuHeader & FU_HEADER_TYPE_MASK;
        uint8_t f_bit = ( pCtx->fuDepacketizationState.payloadHdr >> 15 ) & 0x01;
        uint8_t layer_id = ( pCtx->fuDepacketizationState.payloadHdr >> 3 ) & 0x3F;
        uint8_t tid = pCtx->fuDepacketizationState.payloadHdr & 0x07;

        /* Shift payload right by NALU_HEADER_SIZE to make room for header */
        memmove( pCtx->fuDepacketizationState.pReassemblyBuffer + NALU_HEADER_SIZE,
                 pCtx->fuDepacketizationState.pReassemblyBuffer,
                 pCtx->fuDepacketizationState.reassembledLength );

        /* Build NAL header at start of buffer */
        pCtx->fuDepacketizationState.pReassemblyBuffer[ 0 ] =
            ( f_bit << 7 ) | ( fu_type << 1 ) | ( ( layer_id >> 5 ) & 0x01 );
        pCtx->fuDepacketizationState.pReassemblyBuffer[ 1 ] =
            ( ( layer_id & 0x1F ) << 3 ) | tid;

        /* Set NALU fields */
        pNalu->pNaluData = pCtx->fuDepacketizationState.pReassemblyBuffer;
        pNalu->naluDataLength = totalLength;
        pNalu->nal_unit_type = fu_type;
        pNalu->nal_layer_id = layer_id;
        pNalu->temporal_id = tid;

        /* Reset processing state */
        pCtx->currentlyProcessingPacket = H265_PACKET_NONE;
        pCtx->fuDepacketizationState.reassembledLength = 0;
    }

/* Update context */
    pCtx->tailIndex++;
    pCtx->packetCount--;

    return H265_RESULT_OK;
}

/*-------------------------------------------------------------------------------------------------------------*/

static H265Result_t H265Depacketizer_ProcessAggregationPacket( H265DepacketizerContext_t * pCtx,
                                                               H265Nalu_t * pNalu )
{
    /* Parameter validation */
    if( ( pCtx == NULL ) || ( pNalu == NULL ) )
    {
        return H265_RESULT_BAD_PARAM;
    }

    /* Check if we have packets to process */
    if( pCtx->packetCount == 0 )
    {
        return H265_RESULT_NO_MORE_NALUS;
    }

    /* Get current packet */
    H265Packet_t * pCurrentPacket = &pCtx->pPacketsArray[ pCtx->tailIndex ];

    size_t minSize = AP_HEADER_SIZE + ( 2 * AP_NALU_LENGTH_FIELD_SIZE + 2 * NALU_HEADER_SIZE );

    if( pCtx->donPresent )
    {
        minSize += DONL_FIELD_SIZE + AP_DOND_SIZE;  /* DONL for first, DOND for second */
    }

    if( pCurrentPacket->packetDataLength < minSize )
    {
        return H265_RESULT_MALFORMED_PACKET;
    }

    /* If this is first NAL unit in AP, initialize state */
    if( pCtx->currentlyProcessingPacket != H265_AP_PACKET )
    {
        pCtx->currentlyProcessingPacket = H265_AP_PACKET;
        pCtx->apDepacketizationState.currentOffset = NALU_HEADER_SIZE;

        /* Handle DONL if present */
        if( pCtx->donPresent )
        {
            uint16_t donl = ( pCurrentPacket->pPacketData[ pCtx->apDepacketizationState.currentOffset ] << 8 ) |
                            pCurrentPacket->pPacketData[ pCtx->apDepacketizationState.currentOffset + 1 ];
            pCtx->currentDon = donl;
            pCtx->apDepacketizationState.currentOffset += DONL_FIELD_SIZE;
        }

        pCtx->apDepacketizationState.firstUnit = 1;

        /* Get first NAL unit size */
        size_t offset = pCtx->apDepacketizationState.currentOffset;
        uint16_t nalu_size = ( pCurrentPacket->pPacketData[ offset ] << 8 ) |
                             pCurrentPacket->pPacketData[ offset + 1 ];
        offset += AP_NALU_LENGTH_FIELD_SIZE;

        /* Validate NALU size */
        if( offset + nalu_size > pCurrentPacket->packetDataLength )
        {
            return H265_RESULT_MALFORMED_PACKET;
        }

        /* Set NALU fields for first unit */
        pNalu->pNaluData = &pCurrentPacket->pPacketData[ offset ];
        pNalu->naluDataLength = nalu_size;
        pNalu->nal_unit_type = ( pCurrentPacket->pPacketData[ offset ] >> 1 ) & 0x3F;
        pNalu->nal_layer_id = ( ( pCurrentPacket->pPacketData[ offset ] & 0x01 ) << 5 ) |
                              ( ( pCurrentPacket->pPacketData[ offset + 1 ] & 0xF8 ) >> 3 );
        pNalu->temporal_id = pCurrentPacket->pPacketData[ offset + 1 ] & 0x07;

        /* Update offset for next time */
        offset += nalu_size;
        pCtx->apDepacketizationState.currentOffset = offset;
        return H265_RESULT_OK;
    }

    /* Get next NAL unit from AP */
    size_t offset = pCtx->apDepacketizationState.currentOffset;

    /* Handle DOND for non-first units */
    if( pCtx->donPresent && ( pCtx->apDepacketizationState.firstUnit == 0 ) )
    {
        if( offset + AP_DOND_SIZE > pCurrentPacket->packetDataLength )
        {
            return H265_RESULT_MALFORMED_PACKET;
        }

        uint8_t dond = pCurrentPacket->pPacketData[ offset ];
        pCtx->currentDon += dond;
        offset += AP_DOND_SIZE;
    }

    /* Get NALU size */
    uint16_t nalu_size = ( pCurrentPacket->pPacketData[ offset ] << 8 ) |
                         pCurrentPacket->pPacketData[ offset + 1 ];
    offset += AP_NALU_LENGTH_FIELD_SIZE;


    /* Validate NALU size */
    if( offset + nalu_size > pCurrentPacket->packetDataLength )
    {
        return H265_RESULT_MALFORMED_PACKET;
    }

    /* Set NALU fields */
    pNalu->pNaluData = &pCurrentPacket->pPacketData[ offset ];
    pNalu->naluDataLength = nalu_size;
    pNalu->nal_unit_type = ( pCurrentPacket->pPacketData[ offset ] >> 1 ) & 0x3F;
    pNalu->nal_layer_id = ( ( pCurrentPacket->pPacketData[ offset ] & 0x01 ) << 5 ) |
                          ( ( pCurrentPacket->pPacketData[ offset + 1 ] & 0xF8 ) >> 3 );
    pNalu->temporal_id = pCurrentPacket->pPacketData[ offset + 1 ] & 0x07;

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

    return H265_RESULT_OK;
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
        pCtx->fuDepacketizationState.payloadHdr = 0;
        pCtx->fuDepacketizationState.fuHeader = 0;
        pCtx->fuDepacketizationState.reassembledLength = 0;
        pCtx->fuDepacketizationState.donl = 0;
        pCtx->fuDepacketizationState.pReassemblyBuffer = NULL;
        pCtx->fuDepacketizationState.reassemblyBufferSize = 0;

        /* Initialize AP state */
        pCtx->apDepacketizationState.payloadHdr = 0;
        pCtx->apDepacketizationState.currentUnitIndex = 0;
        pCtx->apDepacketizationState.naluCount = 0;
        pCtx->apDepacketizationState.units = NULL;

        /* Validate packet array entries */
        for(size_t i = 0; i < packetsArrayLength; i++)
        {
            if( ( pPacketsArray[ i ].pPacketData == NULL ) ||
                ( pPacketsArray[ i ].maxPacketSize == 0 ) )
            {
                result = H265_RESULT_BAD_PARAM;
                break;
            }
        }
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
        nal_unit_type = ( pCtx->pPacketsArray[ pCtx->tailIndex ].pPacketData[ 0 ] >> 1 ) & 0x3F;

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
    const size_t startCodeSize = 4;

/* Parameter validation */
    if( ( pCtx == NULL ) || ( pFrame == NULL ) ||
        ( pFrame->pFrameData == NULL ) )
    {
        return H265_RESULT_BAD_PARAM;
    }

/* Get first NAL unit */
    result = H265Depacketizer_GetNalu( pCtx, &nalu );

    if( result == H265_RESULT_NO_MORE_NALUS )
    {
        return H265_RESULT_NO_MORE_FRAMES;
    }

/* Process NAL units until we hit an error or run out */
    while( result == H265_RESULT_OK )
    {
/* Add start code */
        memcpy( pFrame->pFrameData + currentOffset,
                startCode,
                startCodeSize );
        currentOffset += startCodeSize;

/* Add NAL unit */
        memcpy( pFrame->pFrameData + currentOffset,
                nalu.pNaluData,
                nalu.naluDataLength );
        currentOffset += nalu.naluDataLength;

/* Try to get next NAL unit */
        result = H265Depacketizer_GetNalu( pCtx, &nalu );

        if( result == H265_RESULT_NO_MORE_NALUS )
        {
/* Normal end of NALUs - not an error */
            result = H265_RESULT_OK;
            break;
        }
    }

/* Set frame length if we successfully built a frame */
    if( result == H265_RESULT_OK )
    {
        if( currentOffset > 0 )
        {
            pFrame->frameDataLength = currentOffset;
        }
        else
        {
            result = H265_RESULT_NO_MORE_FRAMES;
        }
    }

    return result;
}

/*-------------------------------------------------------------------------------------------------------------*/

H265Result_t H265Depacketizer_GetPacketProperties( const uint8_t * pPacketData,
                                                   const size_t packetDataLength,
                                                   uint32_t * pProperties )
{
    H265Result_t result = H265_RESULT_OK;

/* Parameter validation  */
    if( ( pPacketData == NULL ) || ( pProperties == NULL ) || ( packetDataLength < NALU_HEADER_SIZE ) )
    {
        return H265_RESULT_BAD_PARAM;
    }

    /* Initialize properties */
    *pProperties = 0;


    /* Get NAL unit type from first byte  */
    uint8_t nal_unit_type = ( pPacketData[ 0 ] >> 1 ) & 0x3F;

/* / * Check packet type and set properties * / */
    if( nal_unit_type == FU_PACKET_TYPE )
    {
/* Validate FU packet has enough bytes for FU header */
        if( packetDataLength < TOTAL_FU_HEADER_SIZE )
        {
            return H265_RESULT_MALFORMED_PACKET;
        }

/*  Get FU header  */
        uint8_t fu_header = pPacketData[ 2 ];

/* Check start and end bits  */
        if( fu_header & FU_HEADER_S_BIT_MASK )
        {
            *pProperties |= H265_PACKET_PROPERTY_START_PACKET;
        }

        if( fu_header & FU_HEADER_E_BIT_MASK )
        {
            *pProperties |= H265_PACKET_PROPERTY_END_PACKET;
        }
    }
    else if( nal_unit_type == AP_PACKET_TYPE )   /* Type 48 for AP */
    {
        /* Validate minimum AP packet size */
        if( packetDataLength < ( NALU_HEADER_SIZE + AP_NALU_LENGTH_FIELD_SIZE ) )
        {
            return H265_RESULT_MALFORMED_PACKET;
        }

        /* AP packets are always complete units  */
        *pProperties |= ( H265_PACKET_PROPERTY_START_PACKET |
                          H265_PACKET_PROPERTY_END_PACKET );
    }
    else if( ( nal_unit_type >= SINGLE_NALU_PACKET_TYPE_START ) &&  /* Type 1-47 */
             ( nal_unit_type <= SINGLE_NALU_PACKET_TYPE_END ) )
    {
/*  Single NAL unit packet - both start and end  */
        *pProperties |= ( H265_PACKET_PROPERTY_START_PACKET |
                          H265_PACKET_PROPERTY_END_PACKET );
    }

    else
    {
        result = H265_RESULT_UNSUPPORTED_PACKET;
    }

    return result;
}
