/* Standard includes. */
#include <string.h>

/* API includes. */
#include "opus_depacketizer.h"

OpusResult_t OpusDepacketizer_Init( OpusDepacketizerContext_t * pCtx,
                                    OpusPacket_t * pPacketsArray,
                                    size_t packetsArrayLength )
{
    OpusResult_t result = OPUS_RESULT_OK;

    if( ( pCtx == NULL ) ||
        ( pPacketsArray == NULL ) ||
        ( packetsArrayLength == 0 ) )
    {
        result = OPUS_RESULT_BAD_PARAM;
    }

    if( result == OPUS_RESULT_OK )
    {
        pCtx->pPacketsArray = pPacketsArray;
        pCtx->packetsArrayLength = packetsArrayLength;
        pCtx->packetCount = 0;
    }

    return result;
}

/*-----------------------------------------------------------*/

OpusResult_t OpusDepacketizer_AddPacket( OpusDepacketizerContext_t * pCtx,
                                         const OpusPacket_t * pPacket )
{
    OpusResult_t result = OPUS_RESULT_OK;

    if( ( pCtx == NULL ) ||
        ( pPacket == NULL ) ||
        ( pPacket->packetDataLength == 0 ) )
    {
        result = OPUS_RESULT_BAD_PARAM;
    }

    if( result == OPUS_RESULT_OK )
    {
        if( pCtx->packetCount >= pCtx->packetsArrayLength )
        {
            result = OPUS_RESULT_OUT_OF_MEMORY;
        }
    }

    if( result == OPUS_RESULT_OK )
    {
        pCtx->pPacketsArray[ pCtx->packetCount ].pPacketData = pPacket->pPacketData;
        pCtx->pPacketsArray[ pCtx->packetCount ].packetDataLength = pPacket->packetDataLength;
        pCtx->packetCount += 1;
    }

    return result;
}

/*-----------------------------------------------------------*/

OpusResult_t OpusDepacketizer_GetFrame( OpusDepacketizerContext_t * pCtx,
                                        OpusFrame_t * pFrame )
{
    OpusResult_t result = OPUS_RESULT_OK;
    size_t i, currentFrameDataIndex = 0;
    OpusPacket_t * pPacket;

    if( ( pCtx == NULL ) ||
        ( pFrame == NULL ) ||
        ( pFrame->pFrameData == NULL ) ||
        ( pFrame->frameDataLength == 0 ) )
    {
        result = OPUS_RESULT_BAD_PARAM;
    }

    if( result == OPUS_RESULT_OK )
    {
        if( pCtx->packetCount == 0 )
        {
            result = OPUS_RESULT_NO_MORE_PACKETS;
        }
    }

    for( i = 0; ( i < pCtx->packetCount ) && ( result == OPUS_RESULT_OK ); i++ )
    {
        pPacket = &( pCtx->pPacketsArray[ i ] );

        if( ( pFrame->frameDataLength - currentFrameDataIndex ) >= pPacket->packetDataLength )
        {
            memcpy( ( void * ) &( pFrame->pFrameData[ currentFrameDataIndex ] ),
                    ( const void * ) &( pPacket->pPacketData[ 0 ] ),
                    pPacket->packetDataLength );

            currentFrameDataIndex += pPacket->packetDataLength;
        }
        else
        {
            result = OPUS_RESULT_OUT_OF_MEMORY;
        }
    }

    if( result == OPUS_RESULT_OK )
    {
        pFrame->frameDataLength = currentFrameDataIndex;
    }

    return result;
}

/*-----------------------------------------------------------*/

OpusResult_t OpusDepacketizer_GetPacketProperties( const uint8_t * pPacketData,
                                                   const size_t packetDataLength,
                                                   uint32_t * pProperties )
{
    OpusResult_t result = OPUS_RESULT_OK;

    if( ( pPacketData == NULL ) ||
        ( pProperties == NULL ) ||
        ( packetDataLength == 0 ) )
    {
        result = OPUS_RESULT_BAD_PARAM;
    }

    if( result == OPUS_RESULT_OK )
    {
        *pProperties = OPUS_PACKET_PROPERTY_START_PACKET;
    }

    return result;
}

/*-----------------------------------------------------------*/
