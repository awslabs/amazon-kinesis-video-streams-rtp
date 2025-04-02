#include <string.h>
#include "h265_depacketizer.h"

/*-------------------------------------------------------------------------------------------------------------*/

static void H265Depacketizer_ProcessSingleNalu( H265DepacketizerContext_t * pCtx,
                                                H265Nalu_t * pNalu );

static H265Result_t H265Depacketizer_ProcessFragmentationUnit( H265DepacketizerContext_t * pCtx,
                                                               H265Nalu_t * pNalu );

static H265Result_t H265Depacketizer_ProcessAggregationPacket( H265DepacketizerContext_t * pCtx,
                                                               H265Nalu_t * pNalu );

/*-------------------------------------------------------------------------------------------------------------*/

static void H265Depacketizer_ProcessSingleNalu( H265DepacketizerContext_t * pCtx,
                                                H265Nalu_t * pNalu )
{
    memcpy( ( void * )&( pNalu->pNaluData[0] ),
            ( const void * )&( pCtx->pPacketsArray[pCtx->tailIndex].pPacketData[0] ),
            pCtx->pPacketsArray[pCtx->tailIndex].packetDataLength );

    pNalu->naluDataLength = pCtx->pPacketsArray[pCtx->tailIndex].packetDataLength;

    pCtx->tailIndex++;
    pCtx->packetCount--;
}

/*-------------------------------------------------------------------------------------------------------------*/

static H265Result_t H265Depacketizer_ProcessFragmentationUnit( H265DepacketizerContext_t * pCtx,
                                                               H265Nalu_t * pNalu )
{
    uint8_t * pCurPacketData;
    uint8_t fuHeader = 0;
    uint8_t firstByte, secondByte;
    size_t curPacketLength;
    size_t curNaluDataIndex = 0;
    H265Result_t result = H265_RESULT_OK;

    while( ( pCtx->packetCount > 0 ) &&
           ( ( fuHeader & FU_HEADER_E_BIT_MASK ) == 0 ) )
    {
        pCurPacketData = pCtx->pPacketsArray[pCtx->tailIndex].pPacketData;
        curPacketLength = pCtx->pPacketsArray[pCtx->tailIndex].packetDataLength;

        /* Check minimum size requirement */
        if( curPacketLength < TOTAL_FU_HEADER_SIZE )
        {
            result = H265_RESULT_MALFORMED_PACKET;
            break;
        }

        fuHeader = pCurPacketData[FU_HEADER_OFFSET];
        firstByte = pCurPacketData[0];
        secondByte = pCurPacketData[1];

        /* Validate packet type */
        if( ( ( firstByte >> 1 ) & FU_HEADER_TYPE_MASK ) != FU_PACKET_TYPE )
        {
            result = H265_RESULT_MALFORMED_PACKET;
            break;
        }

        /* Handle start fragment - write NALU header */
        if( fuHeader & FU_HEADER_S_BIT_MASK )
        {
            if( ( curNaluDataIndex + NALU_HEADER_SIZE ) <= pNalu->naluDataLength )
            {
                uint8_t fu_type = fuHeader & FU_HEADER_TYPE_MASK;
                pNalu->pNaluData[curNaluDataIndex] = ( firstByte & FU_HEADER_S_BIT_MASK ) | ( fu_type << 1 );
                pNalu->pNaluData[curNaluDataIndex + 1] = secondByte;
            }
            else
            {
                result = H265_RESULT_OUT_OF_MEMORY;
                break;
            }
            curNaluDataIndex += NALU_HEADER_SIZE;
        }

        /* Write NALU payload */
        size_t payloadLength = curPacketLength - TOTAL_FU_HEADER_SIZE;
        if( ( curNaluDataIndex + payloadLength ) <= pNalu->naluDataLength )
        {
            memcpy( ( void * ) &( pNalu->pNaluData[curNaluDataIndex] ),
                    ( const void * ) &( pCurPacketData[TOTAL_FU_HEADER_SIZE] ),
                    payloadLength );
        }
        else
        {
            result = H265_RESULT_OUT_OF_MEMORY;
            break;
        }
        curNaluDataIndex += payloadLength;

        /* Move to next packet */
        pCtx->tailIndex++;
        pCtx->packetCount--;
    }

    /* Update final NALU length */
    pNalu->naluDataLength = curNaluDataIndex;

    return result;
}

/*-------------------------------------------------------------------------------------------------------------*/

static H265Result_t H265Depacketizer_ProcessAggregationPacket( H265DepacketizerContext_t * pCtx,
                                                               H265Nalu_t * pNalu )
{
    uint8_t * pCurPacketData;
    size_t curPacketLength, naluLength;
    H265Result_t result = H265_RESULT_OK;
    
    const size_t minSize = AP_HEADER_SIZE + (2 * AP_NALU_LENGTH_FIELD_SIZE + 2 * NALU_HEADER_SIZE);
    pCurPacketData = pCtx->pPacketsArray[pCtx->tailIndex].pPacketData;
    curPacketLength = pCtx->pPacketsArray[pCtx->tailIndex].packetDataLength;

    /* Check if the packet is large enough to potentially contain valid data */
    if( curPacketLength > minSize )
    {

        /* We are just starting to parse an AP packet. Skip the AP header. */
        if( pCtx->currentlyProcessingPacket != H265_AP_PACKET )
        {
            pCtx->currentlyProcessingPacket = H265_AP_PACKET;
            pCtx->apDepacketizationState.currentOffset = AP_HEADER_SIZE;
        }

        /* Is there enough data left in the packet to read the next NALU size? */
        if( ( pCtx->apDepacketizationState.currentOffset + AP_NALU_LENGTH_FIELD_SIZE ) <= curPacketLength )
        {
            naluLength = ( pCurPacketData[pCtx->apDepacketizationState.currentOffset] << 8 ) |
                         pCurPacketData[pCtx->apDepacketizationState.currentOffset + 1];

            pCtx->apDepacketizationState.currentOffset += AP_NALU_LENGTH_FIELD_SIZE;

            /* Is there enough data left in the packet to read the next NALU? */
            if( ( pCtx->apDepacketizationState.currentOffset + naluLength ) <= curPacketLength )
            {
                /* Is there enough space in the output buffer? */
                if( naluLength <= pNalu->naluDataLength )
                {
                    memcpy( pNalu->pNaluData,
                            &pCurPacketData[pCtx->apDepacketizationState.currentOffset],
                            naluLength );
                }
                else
                {
                    result = H265_RESULT_OUT_OF_MEMORY;
                }
                pNalu->naluDataLength = naluLength;
                pCtx->apDepacketizationState.currentOffset += naluLength;
            }
            else
            {
                result = H265_RESULT_MALFORMED_PACKET;
                /* Still move the currentOffset so that we move to the next packet at the end of this function. */
                pCtx->apDepacketizationState.currentOffset += naluLength;
            }
        }

        /* If we do not have enough data left in this packet, move to the next packet in the next call. */
        if( ( pCtx->apDepacketizationState.currentOffset + AP_NALU_LENGTH_FIELD_SIZE ) > curPacketLength )
        {
            pCtx->currentlyProcessingPacket = H265_PACKET_NONE;
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

H265Result_t H265Depacketizer_Init( H265DepacketizerContext_t * pCtx,
                                    H265Packet_t * pPacketsArray,
                                    size_t packetsArrayLength )
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

        /* Initialize packet processing state */
        pCtx->currentlyProcessingPacket = H265_PACKET_NONE;

        /* Initialize FU state */
        memset( &pCtx->fuDepacketizationState,
                0,
                sizeof( pCtx->fuDepacketizationState ) );

        /* Initialize AP state */
        memset( &pCtx->apDepacketizationState,
                0,
                sizeof( pCtx->apDepacketizationState ) );
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
            result = H265_RESULT_OUT_OF_MEMORY;
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
            H265Depacketizer_ProcessSingleNalu( pCtx,
                                                pNalu );
        }
        else if( nal_unit_type == FU_PACKET_TYPE )
        {
            result = H265Depacketizer_ProcessFragmentationUnit( pCtx,
                                                                pNalu );
        }
        else if( nal_unit_type == AP_PACKET_TYPE )
        {
            result = H265Depacketizer_ProcessAggregationPacket( pCtx,
                                                                pNalu );
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
    const uint8_t startCode[] = {0x00, 0x00, 0x00, 0x01};
    H265Nalu_t nalu;
    size_t currentFrameDataIndex = 0;

    /* Parameter validation */
    if( ( pCtx == NULL ) || ( pFrame == NULL ) ||
        ( pFrame->pFrameData == NULL ) || ( pFrame->frameDataLength == 0 ) )
    {
        result = H265_RESULT_BAD_PARAM;
    }

    /* Check if we have any packets to process */
    if( result == H265_RESULT_OK )
    {
        if( pCtx->packetCount == 0 )
        {
            result = H265_RESULT_NO_MORE_FRAMES;
        }
    }

    while( result == H265_RESULT_OK )
    {
        /* Set up NALU buffer at current position */
        nalu.pNaluData = &( pFrame->pFrameData[currentFrameDataIndex] );
        nalu.naluDataLength = pFrame->frameDataLength - currentFrameDataIndex;
        result = H265Depacketizer_GetNalu( pCtx,
                                           &nalu );

        if( result == H265_RESULT_OK )
        {
            currentFrameDataIndex += nalu.naluDataLength;

            if( ( pFrame->frameDataLength - currentFrameDataIndex ) >= sizeof( startCode ) )
            {
               /* Copy start code after the NALU */
                memcpy( ( void * ) &( pFrame->pFrameData[ currentFrameDataIndex ] ),
                        ( const void * ) &( startCode[ 0 ] ),
                        sizeof( startCode ) );

                currentFrameDataIndex += sizeof( startCode );
            }
            else
            {
                result = H265_RESULT_OUT_OF_MEMORY;
            }
        }
    }

    /* Convert NO_MORE_NALUS to OK if we've processed at least one NALU */
    if( result == H265_RESULT_NO_MORE_NALUS )
    {
        result = H265_RESULT_OK;
        pFrame->frameDataLength = currentFrameDataIndex;
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

    if( ( pPacketData == NULL ) || ( pProperties == NULL ) ||
        ( packetDataLength < NALU_HEADER_SIZE ) )
    {
        result = H265_RESULT_BAD_PARAM;
    }

    if( result == H265_RESULT_OK )
    {
        *pProperties = 0;
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
