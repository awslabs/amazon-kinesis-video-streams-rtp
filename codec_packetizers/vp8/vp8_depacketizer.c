/* Standard includes. */
#include <string.h>

/* API includes. */
#include "vp8_depacketizer.h"

static size_t ReadPayloadDescriptor( const VP8Packet_t * pPacket,
                                     VP8Frame_t * pFrame );

/*-----------------------------------------------------------*/

static size_t ReadPayloadDescriptor( const VP8Packet_t * pPacket,
                                     VP8Frame_t * pFrame )
{
    size_t curIndex = 0;
    uint8_t extensions;

    pFrame->frameProperties = 0;
    if( ( pPacket->pPacketData[ VP8_PAYLOAD_DESC_HEADER_OFFSET ] & VP8_PAYLOAD_DESC_N_BITMASK ) != 0 )
    {
        pFrame->frameProperties |= VP8_FRAME_PROP_NON_REF_FRAME;
    }

    if( ( pPacket->pPacketData[ VP8_PAYLOAD_DESC_HEADER_OFFSET ] & VP8_PAYLOAD_DESC_X_BITMASK ) != 0 )
    {
        extensions = pPacket->pPacketData[ VP8_PAYLOAD_DESC_EXT_OFFSET ];

        /* Location to read the next extension. */
        curIndex += 2;

        if( ( extensions & VP8_PAYLOAD_DESC_EXT_I_BITMASK ) != 0 )
        {
            pFrame->frameProperties |= VP8_FRAME_PROP_PICTURE_ID_PRESENT;

            if( ( pPacket->pPacketData[ curIndex ] & VP8_PAYLOAD_DESC_EXT_M_BITMASK ) != 0 )
            {
                pFrame->pictureId = ( ( pPacket->pPacketData[ curIndex ] & ~VP8_PAYLOAD_DESC_EXT_M_BITMASK ) << 8 );
                pFrame->pictureId |= pPacket->pPacketData[ curIndex + 1 ];
                curIndex += 2;
            }
            else
            {
                pFrame->pictureId = pPacket->pPacketData[ curIndex ];
                curIndex += 1;
            }
        }

        if( ( extensions & VP8_PAYLOAD_DESC_EXT_L_BITMASK ) != 0 )
        {
            pFrame->frameProperties |= VP8_FRAME_PROP_TL0PICIDX_PRESENT;

            pFrame->tl0PicIndex = pPacket->pPacketData[ curIndex ];
            curIndex += 1;
        }

        if( ( ( extensions & VP8_PAYLOAD_DESC_EXT_T_BITMASK ) != 0 ) ||
            ( ( extensions & VP8_PAYLOAD_DESC_EXT_K_BITMASK ) != 0 ) )
        {
            if( ( extensions & VP8_PAYLOAD_DESC_EXT_T_BITMASK ) != 0 )
            {
                pFrame->frameProperties |= VP8_FRAME_PROP_TID_PRESENT;

                pFrame->tid = ( pPacket->pPacketData[ curIndex ] &
                                VP8_PAYLOAD_DESC_EXT_TID_BITMASK ) >>
                              VP8_PAYLOAD_DESC_EXT_TID_LOCATION;
            }

            if( ( extensions & VP8_PAYLOAD_DESC_EXT_K_BITMASK ) != 0 )
            {
                pFrame->frameProperties |= VP8_FRAME_PROP_KEYIDX_PRESENT;

                pFrame->keyIndex = ( pPacket->pPacketData[ curIndex ] &
                                     VP8_PAYLOAD_DESC_EXT_KEYIDX_BITMASK ) >>
                                   VP8_PAYLOAD_DESC_EXT_KEYIDX_LOCATION;
            }

            if( ( pPacket->pPacketData[ curIndex ] & VP8_PAYLOAD_DESC_EXT_Y_BITMASK ) != 0 )
            {
                pFrame->frameProperties |= VP8_FRAME_PROP_DEPENDS_ON_BASE_ONLY;
            }

            curIndex += 1;
        }
    }
    else
    {
        curIndex += 1;
    }

    return curIndex;
}

/*-----------------------------------------------------------*/

VP8Result_t VP8Depacketizer_Init( VP8DepacketizerContext_t * pCtx,
                                  VP8Packet_t * pPacketsArray,
                                  size_t packetsArrayLength )
{
    VP8Result_t result = VP8_RESULT_OK;

    if( ( pCtx == NULL ) ||
        ( pPacketsArray == NULL ) ||
        ( packetsArrayLength == 0 ) )
    {
        result = VP8_RESULT_BAD_PARAM;
    }

    if( result == VP8_RESULT_OK )
    {
        pCtx->pPacketsArray = pPacketsArray;
        pCtx->packetsArrayLength = packetsArrayLength;
        pCtx->packetCount = 0;
    }

    return result;
}

/*-----------------------------------------------------------*/

VP8Result_t VP8Depacketizer_AddPacket( VP8DepacketizerContext_t * pCtx,
                                       const VP8Packet_t * pPacket )
{
    VP8Result_t result = VP8_RESULT_OK;

    if( ( pCtx == NULL ) ||
        ( pPacket == NULL ) ||
        ( pPacket->packetDataLength == 0 ) )
    {
        result = VP8_RESULT_BAD_PARAM;
    }

    if( result == VP8_RESULT_OK )
    {
        if( pCtx->packetCount >= pCtx->packetsArrayLength )
        {
            result = VP8_RESULT_OUT_OF_MEMORY;
        }
    }

    if( result == VP8_RESULT_OK )
    {
        pCtx->pPacketsArray[ pCtx->packetCount ].pPacketData = pPacket->pPacketData;
        pCtx->pPacketsArray[ pCtx->packetCount ].packetDataLength = pPacket->packetDataLength;
        pCtx->packetCount += 1;
    }

    return result;
}

/*-----------------------------------------------------------*/

VP8Result_t VP8Depacketizer_GetFrame( VP8DepacketizerContext_t * pCtx,
                                      VP8Frame_t * pFrame )
{
    VP8Result_t result = VP8_RESULT_OK;
    VP8Packet_t * pPacket;
    size_t i, payloadDescLength, curFrameDataIndex = 0;

    if( ( pCtx == NULL ) ||
        ( pFrame == NULL ) ||
        ( pFrame->pFrameData == NULL ) ||
        ( pFrame->frameDataLength == 0 ) )
    {
        result = VP8_RESULT_BAD_PARAM;
    }

    for( i = 0; ( i < pCtx->packetCount ) && ( result == VP8_RESULT_OK ); i++ )
    {
        pPacket = &( pCtx->pPacketsArray[ i ] );

        payloadDescLength = ReadPayloadDescriptor( pPacket,
                                                   pFrame );

        if( pPacket->packetDataLength > payloadDescLength )
        {
            if( ( pFrame->frameDataLength - curFrameDataIndex ) >= ( pPacket->packetDataLength - payloadDescLength ) )
            {
                memcpy( ( void * ) &( pFrame->pFrameData[ curFrameDataIndex ] ),
                        ( const void * ) &( pPacket->pPacketData[ payloadDescLength ] ),
                        pPacket->packetDataLength - payloadDescLength );

                curFrameDataIndex += ( pPacket->packetDataLength - payloadDescLength );
            }
            else
            {
                result = VP8_RESULT_OUT_OF_MEMORY;
            }
        }
        else
        {
            result = VP8_MALFORMED_PACKET;
        }
    }

    if( result == VP8_RESULT_OK )
    {
        pFrame->frameDataLength = curFrameDataIndex;
    }

    return result;
}

/*-----------------------------------------------------------*/

VP8Result_t VP8Depacketizer_GetPacketProperties( const uint8_t * pPacketData,
                                                 const size_t packetDataLength,
                                                 uint32_t * pProperties )
{
    VP8Result_t result = VP8_RESULT_OK;

    if( ( pPacketData == NULL ) ||
        ( packetDataLength == 0 ) ||
        ( pProperties == NULL ) )
    {
        result = VP8_RESULT_BAD_PARAM;
    }

    if( result == VP8_RESULT_OK )
    {
        *pProperties = VP8_PACKET_PROP_START_PACKET;
    }

    return result;
}

/*-----------------------------------------------------------*/
