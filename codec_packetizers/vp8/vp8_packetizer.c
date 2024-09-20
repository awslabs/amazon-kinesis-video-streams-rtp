/* Standard includes. */
#include <string.h>

/* API includes. */
#include "vp8_packetizer.h"

static size_t GeneratePayloadDescriptor( VP8Frame_t * pFrame,
                                         uint8_t * pBuffer );

/*-----------------------------------------------------------*/

static size_t GeneratePayloadDescriptor( VP8Frame_t * pFrame,
                                         uint8_t * pBuffer )
{
    size_t curIndex = 0;

    /* First required byte. */
    pBuffer[ VP8_PAYLOAD_DESC_HEADER_OFFSET ] = 0;
    if( ( pFrame->frameProperties & VP8_FRAME_PROP_NON_REF_FRAME ) != 0 )
    {
        pBuffer[ VP8_PAYLOAD_DESC_HEADER_OFFSET ] |= VP8_PAYLOAD_DESC_N_BITMASK;
    }

    /* Optional extensions. */
    if( ( ( pFrame->frameProperties & VP8_FRAME_PROP_PICTURE_ID_PRESENT ) != 0 ) ||
        ( ( pFrame->frameProperties & VP8_FRAME_PROP_TL0PICIDX_PRESENT ) != 0 ) ||
        ( ( pFrame->frameProperties & VP8_FRAME_PROP_TID_PRESENT ) != 0 ) ||
        ( ( pFrame->frameProperties & VP8_FRAME_PROP_KEYIDX_PRESENT ) != 0 ) )
    {
        pBuffer[ VP8_PAYLOAD_DESC_HEADER_OFFSET ] |= VP8_PAYLOAD_DESC_X_BITMASK;
        pBuffer[ VP8_PAYLOAD_DESC_EXT_OFFSET ] = 0;

        /* Location to write the next extension. */
        curIndex += 2;

        if( ( pFrame->frameProperties & VP8_FRAME_PROP_PICTURE_ID_PRESENT ) != 0 )
        {
            pBuffer[ VP8_PAYLOAD_DESC_EXT_OFFSET ] |= VP8_PAYLOAD_DESC_EXT_I_BITMASK;

            if( pFrame->pictureId >= VP8_PAYLOAD_DESC_EXT_M_BITMASK )
            {
                pBuffer[ curIndex ] = ( uint8_t )( ( pFrame->pictureId & 0x7F00 ) >> 8 );
                pBuffer[ curIndex ] |= VP8_PAYLOAD_DESC_EXT_M_BITMASK;
                pBuffer[ curIndex + 1 ] = ( uint8_t )( pFrame->pictureId & 0x00FF );
                curIndex += 2;
            }
            else
            {
                pBuffer[ curIndex ] = ( uint8_t )( pFrame->pictureId & ~VP8_PAYLOAD_DESC_EXT_M_BITMASK );
                curIndex += 1;
            }
        }

        if( ( pFrame->frameProperties & VP8_FRAME_PROP_TL0PICIDX_PRESENT ) != 0 )
        {
            pBuffer[ VP8_PAYLOAD_DESC_EXT_OFFSET ] |= VP8_PAYLOAD_DESC_EXT_L_BITMASK;
            pBuffer[ curIndex ] = pFrame->tl0PicIndex;
            curIndex += 1;
        }

        if( ( ( pFrame->frameProperties & VP8_FRAME_PROP_TID_PRESENT ) != 0 ) ||
            ( ( pFrame->frameProperties & VP8_FRAME_PROP_KEYIDX_PRESENT ) != 0 ) )
        {
            pBuffer[ curIndex ] = 0;

            if( ( pFrame->frameProperties & VP8_FRAME_PROP_TID_PRESENT ) != 0 )
            {
                pBuffer[ VP8_PAYLOAD_DESC_EXT_OFFSET ] |= VP8_PAYLOAD_DESC_EXT_T_BITMASK;
                pBuffer[ curIndex ] |= ( ( pFrame->tid << VP8_PAYLOAD_DESC_EXT_TID_LOCATION ) &
                                         VP8_PAYLOAD_DESC_EXT_TID_BITMASK );
            }

            if( ( pFrame->frameProperties & VP8_FRAME_PROP_KEYIDX_PRESENT ) != 0 )
            {
                pBuffer[ VP8_PAYLOAD_DESC_EXT_OFFSET ] |= VP8_PAYLOAD_DESC_EXT_K_BITMASK;
                pBuffer[ curIndex ] |= ( ( pFrame->keyIndex << VP8_PAYLOAD_DESC_EXT_KEYIDX_LOCATION ) &
                                         VP8_PAYLOAD_DESC_EXT_KEYIDX_BITMASK );
            }

            if( ( pFrame->frameProperties & VP8_FRAME_PROP_DEPENDS_ON_BASE_ONLY ) != 0 )
            {
                pBuffer[ curIndex ] |= VP8_PAYLOAD_DESC_EXT_Y_BITMASK;
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

VP8Result_t VP8Packetizer_Init( VP8PacketizerContext_t * pCtx,
                                VP8Frame_t * pFrame )
{
    VP8Result_t result = VP8_RESULT_OK;

    if( ( pCtx == NULL ) ||
        ( pFrame == NULL ) ||
        ( pFrame->pFrameData == NULL ) ||
        ( pFrame->frameDataLength == 0 ) )
    {
        result = VP8_RESULT_BAD_PARAM;
    }

    if( result == VP8_RESULT_OK )
    {
        pCtx->payloadDescLength = GeneratePayloadDescriptor( pFrame,
                                                             &( pCtx->payloadDesc[ 0 ] ) );
        pCtx->pFrameData = pFrame->pFrameData;
        pCtx->frameDataLength = pFrame->frameDataLength;
        pCtx->curFrameDataIndex = 0;
    }

    return result;
}

/*-----------------------------------------------------------*/

VP8Result_t VP8Packetizer_GetPacket( VP8PacketizerContext_t * pCtx,
                                     VP8Packet_t * pPacket )
{
    VP8Result_t result = VP8_RESULT_OK;
    size_t frameDataLengthToSend;

    if( ( pCtx == NULL ) ||
        ( pPacket == NULL ) )
    {
        result = VP8_RESULT_BAD_PARAM;
    }

    if( result == VP8_RESULT_OK )
    {
        if( pCtx->curFrameDataIndex == pCtx->frameDataLength )
        {
            result = VP8_RESULT_NO_MORE_PACKETS;
        }
    }

    if( result == VP8_RESULT_OK )
    {
        if( pPacket->packetDataLength > pCtx->payloadDescLength )
        {
            memcpy( ( void * ) &( pPacket->pPacketData[ 0 ] ),
                    ( const void * ) &( pCtx->payloadDesc[ 0 ] ),
                    pCtx->payloadDescLength );

            /* Mark Start flag for the first packet of the frame. */
            if( pCtx->curFrameDataIndex == 0 )
            {
                pPacket->pPacketData[ VP8_PAYLOAD_DESC_HEADER_OFFSET ] |= VP8_PAYLOAD_DESC_S_BITMASK;
            }
        }
        else
        {
            result = VP8_RESULT_OUT_OF_MEMORY;
        }
    }

    if( result == VP8_RESULT_OK )
    {
        frameDataLengthToSend = VP8_MIN( pPacket->packetDataLength - pCtx->payloadDescLength,
                                         pCtx->frameDataLength - pCtx->curFrameDataIndex );

        memcpy( ( void * ) &( pPacket->pPacketData[ pCtx->payloadDescLength ] ),
                ( const void * ) &( pCtx->pFrameData[ pCtx->curFrameDataIndex ] ),
                frameDataLengthToSend );
        pCtx->curFrameDataIndex += frameDataLengthToSend;

        pPacket->packetDataLength = pCtx->payloadDescLength + frameDataLengthToSend;
    }

    return result;
}

/*-----------------------------------------------------------*/
