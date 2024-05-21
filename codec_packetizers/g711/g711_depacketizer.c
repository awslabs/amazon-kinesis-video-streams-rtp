/* Standard includes. */
#include <string.h>

/* API includes. */
#include "g711_depacketizer.h"

G711Result_t G711Depacketizer_Init( G711DepacketizerContext_t * pCtx,
                                    G711Packet_t * pPacketsArray,
                                    size_t packetsArrayLength )
{
    G711Result_t result = G711_RESULT_OK;

    if( ( pCtx == NULL ) ||
        ( pPacketsArray == NULL ) ||
        ( packetsArrayLength == 0 ) )
    {
        result = G711_RESULT_BAD_PARAM;
    }

    if( result == G711_RESULT_OK )
    {
        pCtx->pPacketsArray = pPacketsArray;
        pCtx->packetsArrayLength = packetsArrayLength;
        pCtx->packetCount = 0;
    }

    return result;
}

/*-----------------------------------------------------------*/

G711Result_t G711Depacketizer_AddPacket( G711DepacketizerContext_t * pCtx,
                                         const G711Packet_t * pPacket )
{
    G711Result_t result = G711_RESULT_OK;

    if( ( pCtx == NULL ) ||
        ( pPacket == NULL ) )
    {
        result = G711_RESULT_BAD_PARAM;
    }

    if( result == G711_RESULT_OK )
    {
        if( pCtx->packetCount >= pCtx->packetsArrayLength )
        {
            result = G711_RESULT_OUT_OF_MEMORY;
        }
    }

    if( result == G711_RESULT_OK )
    {
        pCtx->pPacketsArray[ pCtx->packetCount ].pPacketData = pPacket->pPacketData;
        pCtx->pPacketsArray[ pCtx->packetCount ].packetDataLength = pPacket->packetDataLength;
        pCtx->packetCount += 1;
    }

    return result;
}

/*-----------------------------------------------------------*/

G711Result_t G711Depacketizer_GetFrame( G711DepacketizerContext_t * pCtx,
                                        G711Frame_t * pFrame )
{
    G711Result_t result = G711_RESULT_OK;
    G711Packet_t * pPacket;
    size_t i, currentFrameDataIndex = 0;

    if( ( pCtx == NULL ) ||
        ( pFrame == NULL ) ||
        ( pFrame->pFrameData == NULL ) ||
        ( pFrame->frameDataLength == 0 ) )
    {
        result = G711_RESULT_BAD_PARAM;
    }

    for( i = 0; ( i < pCtx->packetCount ) && ( result == G711_RESULT_OK ); i++ )
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
            result = G711_RESULT_OUT_OF_MEMORY;
        }
    }

    if( result == G711_RESULT_OK )
    {
        pFrame->frameDataLength = currentFrameDataIndex;
    }

    return result;
}

/*-----------------------------------------------------------*/

G711Result_t G711Depacketizer_GetPacketProperties( const uint8_t * pPacketData,
                                                   const size_t packetDataLength,
                                                   uint32_t * pProperties )
{
    G711Result_t result = G711_RESULT_OK;

    ( void ) packetDataLength;

    if( ( pPacketData == NULL ) ||
        ( pProperties == NULL ) ||
        ( packetDataLength == 0 ) )
    {
        result = G711_RESULT_BAD_PARAM;
    }

    if( result == G711_RESULT_OK )
    {
        *pProperties = G711_PACKET_PROPERTY_START_PACKET;
    }

    return result;
}

/*-----------------------------------------------------------*/
