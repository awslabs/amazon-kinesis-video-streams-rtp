/* Standard includes. */
#include <string.h>

/* API includes. */
#include "h265_depacketizer.h"

/*-----------------------------------------------------------*/

static H265Result_t DepacketizeSingleNaluPacket( H265DepacketizerContext_t * pCtx,
                                                 H265Nalu_t * pNalu );

static H265Result_t DepacketizeFragmentationUnitPacket( H265DepacketizerContext_t * pCtx,
                                                        H265Nalu_t * pNalu );

static H265Result_t DepacketizeAggregationPacket( H265DepacketizerContext_t * pCtx,
                                                  H265Nalu_t * pNalu );

/*-----------------------------------------------------------*/

static H265Result_t DepacketizeSingleNaluPacket( H265DepacketizerContext_t * pCtx,
                                                 H265Nalu_t * pNalu )
{
    H265Result_t result = H265_RESULT_OK;

    if( pCtx->pPacketsArray[ pCtx->tailIndex ].packetDataLength <= pNalu->naluDataLength )
    {
        memcpy( ( void * ) &( pNalu->pNaluData[ 0 ] ),
                ( const void * ) &( pCtx->pPacketsArray[ pCtx->tailIndex ].pPacketData[ 0 ] ),
                pCtx->pPacketsArray[ pCtx->tailIndex ].packetDataLength );

        pNalu->naluDataLength = pCtx->pPacketsArray[ pCtx->tailIndex ].packetDataLength;

        /* Move to the next packet in the next call to H265Depacketizer_GetNalu. */
        pCtx->tailIndex += 1;
        pCtx->packetCount -= 1;
    }
    else
    {
        result = H265_RESULT_OUT_OF_MEMORY;
    }

    return result;
}

/*-----------------------------------------------------------*/

static H265Result_t DepacketizeFragmentationUnitPacket( H265DepacketizerContext_t * pCtx,
                                                        H265Nalu_t * pNalu )
{
    uint8_t * pCurPacketData;
    uint8_t fuHeader = 0, fuType = 0;
    size_t curPacketLength, curNaluDataIndex = 0, payloadLength;
    H265Result_t result = H265_RESULT_OK;

    /* While there are more fragments to process and we have not yet processed
     * the last fragment. */
    while( ( pCtx->packetCount > 0 ) &&
           ( ( fuHeader & FU_HEADER_E_BIT_MASK ) == 0 ) )
    {
        pCurPacketData = pCtx->pPacketsArray[ pCtx->tailIndex ].pPacketData;
        curPacketLength = pCtx->pPacketsArray[ pCtx->tailIndex ].packetDataLength;

        if( ( curPacketLength < FU_PAYLOAD_HEADER_SIZE + FU_HEADER_SIZE ) ||
            ( ( ( pCurPacketData[ 0 ] & NALU_HEADER_TYPE_MASK ) >> NALU_HEADER_TYPE_LOCATION ) != FU_PACKET_TYPE ) )
        {
            result = H265_RESULT_MALFORMED_PACKET;
            break;
        }

        fuHeader = pCurPacketData[ FU_HEADER_OFFSET ];

        /* Write NALU header for the first fragment only. */
        if( ( fuHeader & FU_HEADER_S_BIT_MASK ) != 0 )
        {
            if( ( curNaluDataIndex + NALU_HEADER_SIZE ) <= pNalu->naluDataLength )
            {
                fuType = ( fuHeader & FU_HEADER_TYPE_MASK ) >> FU_HEADER_TYPE_LOCATION;

                pNalu->pNaluData[ curNaluDataIndex ] = ( pCurPacketData[ 0 ] & NALU_HEADER_F_MASK ) |
                                                       ( fuType << NALU_HEADER_TYPE_LOCATION );
                pNalu->pNaluData[ curNaluDataIndex + 1 ] = pCurPacketData[ 1 ];
            }
            else
            {
                result = H265_RESULT_OUT_OF_MEMORY;
                break;
            }
            curNaluDataIndex += NALU_HEADER_SIZE;
        }

        /* Write NALU payload. */
        payloadLength = curPacketLength - FU_PAYLOAD_HEADER_SIZE - FU_HEADER_SIZE;
        if( ( curNaluDataIndex + payloadLength ) <= pNalu->naluDataLength )
        {
            memcpy( ( void * ) &( pNalu->pNaluData[ curNaluDataIndex ] ),
                    ( const void * ) &( pCurPacketData[ FU_PAYLOAD_HEADER_SIZE + FU_HEADER_SIZE ] ),
                    payloadLength );
        }
        else
        {
            result = H265_RESULT_OUT_OF_MEMORY;
            break;
        }
        curNaluDataIndex += payloadLength;

        /* Move to the next packet. */
        pCtx->tailIndex += 1;
        pCtx->packetCount -= 1;
    }

    /* Update NALU length. */
    pNalu->naluDataLength = curNaluDataIndex;

    return result;
}

/*-----------------------------------------------------------*/

static H265Result_t DepacketizeAggregationPacket( H265DepacketizerContext_t * pCtx,
                                                  H265Nalu_t * pNalu )
{
    uint8_t * pCurPacketData;
    size_t curPacketLength, naluLength;
    H265Result_t result = H265_RESULT_OK;

    pCurPacketData = pCtx->pPacketsArray[ pCtx->tailIndex ].pPacketData;
    curPacketLength = pCtx->pPacketsArray[ pCtx->tailIndex ].packetDataLength;

    /* We are just starting to parse an AP packet. Skip the AP header. */
    if( pCtx->curPacketIndex == 0 )
    {
        pCtx->curPacketIndex += AP_HEADER_SIZE;
    }

    /* Is there enough data left in the packet to read the next NALU size? */
    if( ( pCtx->curPacketIndex + AP_NALU_LENGTH_FIELD_SIZE ) <= curPacketLength )
    {
        /* Read NALU length. */
        naluLength = pCurPacketData[ pCtx->curPacketIndex ];
        naluLength = ( naluLength << 8 ) |
                     ( pCurPacketData[ pCtx->curPacketIndex + 1 ] );

        pCtx->curPacketIndex += AP_NALU_LENGTH_FIELD_SIZE;

        /* Is there enough data left in the packet to read the next NALU? */
        if( ( pCtx->curPacketIndex + naluLength ) <= curPacketLength )
        {
            /* Is there enough space in the output buffer? */
            if( naluLength <= pNalu->naluDataLength )
            {
                memcpy( ( void * ) pNalu->pNaluData,
                        ( const void * ) &( pCurPacketData[ pCtx->curPacketIndex ] ),
                        naluLength );
            }
            else
            {
                result = H265_RESULT_OUT_OF_MEMORY;
            }

            /* Update NALU length. */
            pNalu->naluDataLength = naluLength;

            /* Move to next Nalu in the next call to H265Depacketizer_GetNalu. */
            pCtx->curPacketIndex += naluLength;
        }
        else
        {
            result = H265_RESULT_MALFORMED_PACKET;

            /* Still move the curPacketIndex so that we move to the next packet
             * at the end of this function. */
            pCtx->curPacketIndex += naluLength;
        }
    }
    else
    {
        result = H265_RESULT_MALFORMED_PACKET;
    }

    /* Move to the next packet once current packet content is completely
     * parsed. */
    if( pCtx->curPacketIndex >= curPacketLength )
    {
        pCtx->curPacketIndex = 0;
        pCtx->tailIndex += 1;
        pCtx->packetCount -= 1;
    }

    return result;
}

/*-----------------------------------------------------------*/

H265Result_t H265Depacketizer_Init( H265DepacketizerContext_t * pCtx,
                                    H265Packet_t * pPacketsArray,
                                    size_t packetsArrayLength )
{
    H265Result_t result = H265_RESULT_OK;

    if( ( pCtx == NULL ) ||
        ( pPacketsArray == NULL ) ||
        ( packetsArrayLength == 0 ) )
    {
        result = H265_RESULT_BAD_PARAM;
    }

    if( result == H265_RESULT_OK )
    {
        pCtx->pPacketsArray = pPacketsArray;
        pCtx->packetsArrayLength = packetsArrayLength;

        pCtx->headIndex = 0;
        pCtx->tailIndex = 0;
        pCtx->packetCount = 0;
        pCtx->curPacketIndex = 0;
    }

    return result;
}

/*-----------------------------------------------------------*/

H265Result_t H265Depacketizer_AddPacket( H265DepacketizerContext_t * pCtx,
                                         const H265Packet_t * pPacket )
{
    H265Result_t result = H265_RESULT_OK;

    if( ( pCtx == NULL ) ||
        ( pPacket == NULL ) )
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

/*-----------------------------------------------------------*/

H265Result_t H265Depacketizer_GetNalu( H265DepacketizerContext_t * pCtx,
                                       H265Nalu_t * pNalu )
{
    H265Result_t result = H265_RESULT_OK;
    uint8_t packetType;

    if( ( pCtx == NULL ) ||
        ( pNalu == NULL ) )
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
        packetType = ( pCtx->pPacketsArray[ pCtx->tailIndex ].pPacketData[ 0 ] &
                       NALU_HEADER_TYPE_MASK ) >> NALU_HEADER_TYPE_LOCATION;

        if( ( packetType >= SINGLE_NALU_PACKET_TYPE_START ) &&
            ( packetType <= SINGLE_NALU_PACKET_TYPE_END ) )
        {
            result = DepacketizeSingleNaluPacket( pCtx,
                                                  pNalu );
        }
        else if( packetType == FU_PACKET_TYPE )
        {
            result = DepacketizeFragmentationUnitPacket( pCtx,
                                                         pNalu );
        }
        else if( packetType == AP_PACKET_TYPE )
        {
            result = DepacketizeAggregationPacket( pCtx,
                                                   pNalu );
        }
        else
        {
            result = H265_RESULT_UNSUPPORTED_PACKET;
        }
    }

    return result;
}

/*-----------------------------------------------------------*/

H265Result_t H265Depacketizer_GetFrame( H265DepacketizerContext_t * pCtx,
                                        H265Frame_t * pFrame )
{
    H265Result_t result = H265_RESULT_OK;
    H265Nalu_t nalu;
    size_t currentFrameDataIndex = 0, naluDataIndex = 0, startCodeIndex = 0;
    const uint8_t startCode[] = { 0x00, 0x00, 0x00, 0x01 };

    if( ( pCtx == NULL ) ||
        ( pFrame == NULL ) ||
        ( pFrame->pFrameData == NULL ) ||
        ( pFrame->frameDataLength == 0 ) )
    {
        result = H265_RESULT_BAD_PARAM;
    }

    if( result == H265_RESULT_OK )
    {
        if( pCtx->packetCount == 0 )
        {
            result = H265_RESULT_NO_MORE_FRAMES;
        }
    }

    while( result == H265_RESULT_OK )
    {
        if( ( pFrame->frameDataLength - currentFrameDataIndex ) > sizeof( startCode ) )
        {
            /* NALU data starts after the start code. */
            startCodeIndex = currentFrameDataIndex;
            naluDataIndex = currentFrameDataIndex + sizeof( startCode );

            nalu.pNaluData = &( pFrame->pFrameData[ naluDataIndex ] );
            nalu.naluDataLength = pFrame->frameDataLength - naluDataIndex;

            result = H265Depacketizer_GetNalu( pCtx,
                                               &( nalu ) );

            if( result == H265_RESULT_OK )
            {
                memcpy( ( void * ) &( pFrame->pFrameData[ startCodeIndex ] ),
                        ( const void * ) &( startCode[ 0 ] ),
                        sizeof( startCode ) );

                currentFrameDataIndex += sizeof( startCode );
                currentFrameDataIndex += nalu.naluDataLength;
            }
        }
        else
        {
            result = H265_RESULT_OUT_OF_MEMORY;
        }
    }

    if( result == H265_RESULT_NO_MORE_NALUS )
    {
        result = H265_RESULT_OK;
        pFrame->frameDataLength = currentFrameDataIndex;
    }

    return result;
}

/*-----------------------------------------------------------*/

H265Result_t H265Depacketizer_GetPacketProperties( const uint8_t * pPacketData,
                                                   const size_t packetDataLength,
                                                   uint32_t * pProperties )
{
    H265Result_t result = H265_RESULT_OK;
    uint8_t packetType, fuHeader;

    if( ( pPacketData == NULL ) ||
        ( pProperties == NULL ) ||
        ( packetDataLength < NALU_HEADER_SIZE ) )
    {
        result = H265_RESULT_BAD_PARAM;
    }

    if( result == H265_RESULT_OK )
    {
        packetType = ( pPacketData[ 0 ] & NALU_HEADER_TYPE_MASK ) >> NALU_HEADER_TYPE_LOCATION;

        if( ( packetType >= SINGLE_NALU_PACKET_TYPE_START ) &&
            ( packetType <= SINGLE_NALU_PACKET_TYPE_END ) )
        {
            /* Single NAL unit packet - both start and end. */
            *pProperties = H265_PACKET_PROPERTY_START_PACKET |
                           H265_PACKET_PROPERTY_END_PACKET;
        }
        else if( packetType == FU_PACKET_TYPE )
        {
            /* Validate FU packet has enough bytes to read FU header. */
            if( packetDataLength < FU_PAYLOAD_HEADER_SIZE + FU_HEADER_SIZE )
            {
                result = H265_RESULT_MALFORMED_PACKET;
            }
            else
            {
                *pProperties = 0;
                fuHeader = pPacketData[ FU_HEADER_OFFSET ];

                if( ( fuHeader & FU_HEADER_S_BIT_MASK ) != 0 )
                {
                    *pProperties |= H265_PACKET_PROPERTY_START_PACKET;
                }

                if( ( fuHeader & FU_HEADER_E_BIT_MASK ) != 0 )
                {
                    *pProperties |= H265_PACKET_PROPERTY_END_PACKET;
                }
            }
        }
        else if( packetType == AP_PACKET_TYPE )
        {
            /* AP packets are always complete units, so mark both start and
             * end. */
            *pProperties = H265_PACKET_PROPERTY_START_PACKET |
                           H265_PACKET_PROPERTY_END_PACKET;
         }
        else
        {
            result = H265_RESULT_UNSUPPORTED_PACKET;
        }
    }

    return result;
}

/*-----------------------------------------------------------*/
