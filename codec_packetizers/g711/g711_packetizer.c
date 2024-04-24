/* Standard includes. */
#include <string.h>

/* API includes. */
#include "g711_packetizer.h"

/*-----------------------------------------------------------*/

G711Result_t G711Packetizer_Init( G711PacketizerContext_t * pCtx,
                                  G711Frame_t * pFrame )
{
    G711Result_t result = G711_RESULT_OK;

    if( ( pCtx == NULL ) ||
        ( pFrame == NULL ) ||
        ( pFrame->frameDataLength == 0 ) )
    {
        result = G711_RESULT_BAD_PARAM;
    }

    if( result == G711_RESULT_OK )
    {
        pCtx->frame.pFrameData = pFrame->pFrameData;
        pCtx->frame.frameDataLength = pFrame->frameDataLength;
        pCtx->curFrameDataIndex = 0;
    }

    return result;
}

/*-----------------------------------------------------------*/

G711Result_t G711Packetizer_GetPacket( G711PacketizerContext_t * pCtx,
                                       G711Packet_t * pPacket )
{
    G711Result_t result = G711_RESULT_OK;
    size_t frameDataLengthToSend;

    if( ( pCtx == NULL ) ||
        ( pPacket == NULL ) ||
        ( pPacket->packetDataLength == 0 ) )
    {
        result = G711_RESULT_BAD_PARAM;
    }

    if( result == G711_RESULT_OK )
    {
        if( pCtx->curFrameDataIndex == pCtx->frame.frameDataLength )
        {
            result = G711_RESULT_NO_MORE_PACKETS;
        }
    }

    if( result == G711_RESULT_OK )
    {
        frameDataLengthToSend = G711_MIN( pPacket->packetDataLength,
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
