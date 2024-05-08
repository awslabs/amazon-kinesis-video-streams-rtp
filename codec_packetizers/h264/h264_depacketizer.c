/* Standard includes. */
#include <string.h>

/* API includes. */
#include "h264_depacketizer.h"

static void DepacketizeSingleNaluPacket( H264DepacketizerContext_t * pCtx,
                                         Nalu_t * pNalu );

static H264Result_t DepacketizeFragmentationUnitPacket( H264DepacketizerContext_t * pCtx,
                                                        Nalu_t * pNalu );

/*-----------------------------------------------------------*/

static void DepacketizeSingleNaluPacket( H264DepacketizerContext_t * pCtx,
                                         Nalu_t * pNalu )
{
    memcpy( ( void * ) &( pNalu->pNaluData[ 0 ] ),
            ( const void * ) &( pCtx->pPacketsArray[ pCtx->tailIndex ].pPacketData[ 0 ] ),
            pCtx->pPacketsArray[ pCtx->tailIndex ].packetDataLength );
    pNalu->naluDataLength = pCtx->pPacketsArray[ pCtx->tailIndex ].packetDataLength;

    /* Move to the next packet in the next call to H264Depacketizer_GetNalu. */
    pCtx->tailIndex += 1;
    pCtx->packetCount -= 1;
}

/*-----------------------------------------------------------*/

static H264Result_t DepacketizeFragmentationUnitPacket( H264DepacketizerContext_t * pCtx,
                                                        Nalu_t * pNalu )
{
    uint8_t * pCurPacketData;
    uint8_t fuHeader = 0, fuIndicator = 0;
    size_t curPacketLength, curNaluDataIndex = 0;
    H264Result_t result = H264_RESULT_OK;

    /* While there are more fragments to process and we have not yet processed
     * the last fragment. */
    while( ( pCtx->packetCount > 0 ) &&
           ( ( fuHeader & FU_A_HEADER_E_BIT_MASK ) == 0 ) )
    {
        pCurPacketData = pCtx->pPacketsArray[ pCtx->tailIndex ].pPacketData;
        curPacketLength = pCtx->pPacketsArray[ pCtx->tailIndex ].packetDataLength;

        fuIndicator = pCurPacketData[ FU_A_INDICATOR_OFFSET ];
        fuHeader = pCurPacketData[ FU_A_HEADER_OFFSET ];

        /* Break if this packet is not a fragment of the current packet. */
        if( ( fuIndicator & FU_A_INDICATOR_TYPE_MASK ) != FU_A_PACKET_TYPE )
        {
            break;
        }

        /* Write NALU header for the first fragment only. */
        if( ( fuHeader & FU_A_HEADER_S_BIT_MASK ) != 0 )
        {
            if( ( curNaluDataIndex + 1 ) <= pNalu->naluDataLength )
            {
                pNalu->pNaluData[ curNaluDataIndex ] = ( ( fuIndicator & FU_A_INDICATOR_NRI_MASK ) |
                                                         ( fuHeader & FU_A_HEADER_TYPE_MASK ) );
            }
            else
            {
                result = H264_RESULT_OUT_OF_MEMORY;
            }
            curNaluDataIndex += 1;
        }

        /* Write NALU payload. */
        if( ( curNaluDataIndex + curPacketLength ) <= pNalu->naluDataLength )
        {
            memcpy( ( void * ) &( pNalu->pNaluData[ curNaluDataIndex ] ),
                    ( const void * ) &( pCurPacketData[ FU_A_PAYLOAD_OFFSET ] ),
                    curPacketLength - FU_A_HEADER_SIZE );
        }
        else
        {
            result = H264_RESULT_OUT_OF_MEMORY;
        }
        curNaluDataIndex += ( curPacketLength - FU_A_HEADER_SIZE );

        /* Move to the next packet. */
        pCtx->tailIndex += 1;
        pCtx->packetCount -= 1;
    }

    /* Update NALU Length. */
    pNalu->naluDataLength = curNaluDataIndex;

    return result;
}

/*-----------------------------------------------------------*/

H264Result_t H264Depacketizer_Init( H264DepacketizerContext_t * pCtx,
                                    H264Packet_t * pPacketsArray,
                                    size_t packetsArrayLength )
{
    H264Result_t result = H264_RESULT_OK;

    if( ( pCtx == NULL ) ||
        ( pPacketsArray == NULL ) ||
        ( packetsArrayLength == 0 ) )
    {
        result = H264_RESULT_BAD_PARAM;
    }

    if( result == H264_RESULT_OK )
    {
        pCtx->pPacketsArray = pPacketsArray;
        pCtx->packetsArrayLength = packetsArrayLength;

        pCtx->headIndex = 0;
        pCtx->tailIndex = 0;
        pCtx->packetCount = 0;
    }

    return result;
}

/*-----------------------------------------------------------*/

H264Result_t H264Depacketizer_AddPacket( H264DepacketizerContext_t * pCtx,
                                         const H264Packet_t * pPacket )
{
    H264Result_t result = H264_RESULT_OK;

    if( ( pCtx == NULL ) ||
        ( pPacket == NULL ) )
    {
        result = H264_RESULT_BAD_PARAM;
    }

    if( result == H264_RESULT_OK )
    {
        if( pCtx->packetCount >= pCtx->packetsArrayLength )
        {
            result = H264_RESULT_OUT_OF_MEMORY;
        }
    }

    if( result == H264_RESULT_OK )
    {
        pCtx->pPacketsArray[ pCtx->headIndex ].pPacketData = pPacket->pPacketData;
        pCtx->pPacketsArray[ pCtx->headIndex ].packetDataLength = pPacket->packetDataLength;
        pCtx->headIndex += 1;
        pCtx->packetCount += 1;
    }

    return result;
}

/*-----------------------------------------------------------*/

H264Result_t H264Depacketizer_GetNalu( H264DepacketizerContext_t * pCtx,
                                       Nalu_t * pNalu )
{
    H264Result_t result = H264_RESULT_OK;
    uint8_t packetType;

    if( ( pCtx == NULL ) ||
        ( pNalu == NULL ) )
    {
        result = H264_RESULT_BAD_PARAM;
    }

    if( result == H264_RESULT_OK )
    {
        if( pCtx->packetCount == 0 )
        {
            result = H264_RESULT_NO_MORE_NALUS;
        }
    }

    if( result == H264_RESULT_OK )
    {
        packetType = ( pCtx->pPacketsArray[ pCtx->tailIndex ].pPacketData[ 0 ] &
                       NALU_HEADER_TYPE_MASK );

        if( ( packetType >= SINGLE_NALU_PACKET_TYPE_START ) &&
            ( packetType <= SINGLE_NALU_PACKET_TYPE_END ) )
        {
            DepacketizeSingleNaluPacket( pCtx,
                                         pNalu );
        }
        else if( packetType == FU_A_PACKET_TYPE )
        {
            result = DepacketizeFragmentationUnitPacket( pCtx,
                                                         pNalu );
        }
        else
        {
            result = H264_RESULT_UNSUPPORTED_PACKET;
        }
    }

    return result;
}

/*-----------------------------------------------------------*/

H264Result_t H264Depacketizer_GetFrame( H264DepacketizerContext_t * pCtx,
                                        Frame_t * pFrame )
{
    H264Result_t result = H264_RESULT_OK;
    Nalu_t nalu;
    size_t currentFrameDataIndex = 0, naluDataIndex = 0, startCodeIndex = 0;
    uint8_t startCode[] = { 0x00, 0x00, 0x00, 0x01 };

    if( ( pCtx == NULL ) ||
        ( pFrame == NULL ) ||
        ( pFrame->pFrameData == NULL ) ||
        ( pFrame->frameDataLength == 0 ) )
    {
        result = H264_RESULT_BAD_PARAM;
    }

    if( result == H264_RESULT_OK )
    {
        if( pCtx->packetCount == 0 )
        {
            result = H264_RESULT_NO_MORE_FRAMES;
        }
    }

    while( result == H264_RESULT_OK )
    {
        if( ( pFrame->frameDataLength - currentFrameDataIndex ) > sizeof( startCode ) )
        {
            /* NALU data starts after the start code. */
            startCodeIndex = currentFrameDataIndex;
            naluDataIndex = currentFrameDataIndex + sizeof( startCode );

            nalu.pNaluData = &( pFrame->pFrameData[ naluDataIndex ] );
            nalu.naluDataLength = pFrame->frameDataLength - naluDataIndex;

            result = H264Depacketizer_GetNalu( pCtx,
                                               &( nalu ) );

            if( result == H264_RESULT_OK )
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
            result = H264_RESULT_OUT_OF_MEMORY;
        }
    }

    pFrame->frameDataLength = currentFrameDataIndex;

    if( result == H264_RESULT_NO_MORE_NALUS )
    {
        result = H264_RESULT_OK;
    }

    return result;
}

/*-----------------------------------------------------------*/

H264Result_t H264Depacketizer_GetPacketProperties( const uint8_t * pPacketData,
                                                   const size_t packetDataLength,
                                                   uint32_t * pProperties )
{
    H264Result_t result = H264_RESULT_OK;
    uint8_t packetType, fuHeader;

    (void) packetDataLength;

    if( pProperties == NULL )
    {
        result = H264_RESULT_BAD_PARAM;
    }

    if( result == H264_RESULT_OK )
    {
        packetType = pPacketData[ 0 ] & NALU_HEADER_TYPE_MASK;

        if( ( packetType >= SINGLE_NALU_PACKET_TYPE_START ) &&
            ( packetType <= SINGLE_NALU_PACKET_TYPE_END ) )
        {
            *pProperties = H264_PACKET_PROPERTY_START_PACKET;
        }
        else if( packetType == FU_A_PACKET_TYPE )
        {
            *pProperties = 0;
            fuHeader = pPacketData[ FU_A_HEADER_OFFSET ];

            if( ( fuHeader & FU_A_HEADER_S_BIT_MASK ) != 0 )
            {
                *pProperties |= H264_PACKET_PROPERTY_START_PACKET;
            }

            if( ( fuHeader & FU_A_HEADER_E_BIT_MASK ) != 0 )
            {
                *pProperties |= H264_PACKET_PROPERTY_END_PACKET;
            }
        }
        else
        {
            result = H264_RESULT_UNSUPPORTED_PACKET;
        }
    }

    return result;
}

/*-----------------------------------------------------------*/
