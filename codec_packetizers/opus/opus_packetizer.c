/* Standard includes. */
#include <string.h>

/* API includes. */
#include "opus_packetizer.h"

/*-----------------------------------------------------------*/

OpusResult_t OpusPacketizer_Init( OpusPacketizerContext_t * pCtx,
                                  OpusFrame_t * pFrame )
{
    OpusResult_t result = OPUS_RESULT_OK;

    if( ( pCtx == NULL ) ||
        ( pFrame == NULL ) ||
        ( pFrame->pFrameData == NULL ) ||
        ( pFrame->frameDataLength == 0 ) )
    {
        result = OPUS_RESULT_BAD_PARAM;
    }

    if( result == OPUS_RESULT_OK )
    {
        pCtx->frame.pFrameData = pFrame->pFrameData;
        pCtx->frame.frameDataLength = pFrame->frameDataLength;
        pCtx->curFrameDataIndex = 0;
    }

    return result;
}

/*-----------------------------------------------------------*/

OpusResult_t OpusPacketizer_GetPacket( OpusPacketizerContext_t * pCtx,
                                       OpusPacket_t * pPacket )
{
    OpusResult_t result = OPUS_RESULT_OK;
    size_t frameDataLengthToSend;

    if( ( pCtx == NULL ) ||
        ( pPacket == NULL ) ||
        ( pPacket->packetDataLength == 0 ) )
    {
        result = OPUS_RESULT_BAD_PARAM;
    }

    if( result == OPUS_RESULT_OK )
    {
        if( pCtx->frame.frameDataLength == pCtx->curFrameDataIndex )
        {
            result = OPUS_RESULT_NO_MORE_PACKETS;
        }
    }

    if( result == OPUS_RESULT_OK )
    {
        frameDataLengthToSend = OPUS_MIN( pPacket->packetDataLength,
                                          pCtx->frame.frameDataLength - pCtx->curFrameDataIndex );

        memcpy( ( void * ) &( pPacket->pPacketData[ 0 ] ),
                ( const void * ) &( pCtx->frame.pFrameData[ pCtx->curFrameDataIndex ] ),
                frameDataLengthToSend );

        pPacket->packetDataLength = frameDataLengthToSend;
        pCtx->curFrameDataIndex += frameDataLengthToSend;
    }

    return result;
}

/*-----------------------------------------------------------*/
