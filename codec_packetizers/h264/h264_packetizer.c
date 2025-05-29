/* Standard includes. */
#include <string.h>

/* API includes. */
#include "h264_packetizer.h"

/*-----------------------------------------------------------*/

static void PacketizeSingleNaluPacket( H264PacketizerContext_t * pCtx,
                                       H264Packet_t * pPacket );

static void PacketizeFragmentationUnitPacket( H264PacketizerContext_t * pCtx,
                                              H264Packet_t * pPacket );

/*-----------------------------------------------------------*/

/*
 * RTP payload format for single NAL unit packet:
 *
 *  0                   1                   2                   3
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |F|NRI|  Type   |                                               |
 * +-+-+-+-+-+-+-+-+                                               |
 * |                                                               |
 * |               Bytes 2..n of a single NAL unit                 |
 * |                                                               |
 * |                               +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                               :...OPTIONAL RTP padding        |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
static void PacketizeSingleNaluPacket( H264PacketizerContext_t * pCtx,
                                       H264Packet_t * pPacket )
{
    /* Fill packet. */
    memcpy( ( void * ) &( pPacket->pPacketData[ 0 ] ),
            ( const void * ) &( pCtx->pNaluArray[ pCtx->tailIndex ].pNaluData[ 0 ] ),
            pCtx->pNaluArray[ pCtx->tailIndex ].naluDataLength );

    pPacket->packetDataLength = pCtx->pNaluArray[ pCtx->tailIndex ].naluDataLength;

    /* Move to the next NALU in the next call to H264Packetizer_GetPacket. */
    pCtx->tailIndex += 1;
    pCtx->naluCount -= 1;
}

/*-----------------------------------------------------------*/

static void PacketizeFragmentationUnitPacket( H264PacketizerContext_t * pCtx,
                                              H264Packet_t * pPacket )
{
    uint8_t fuHeader = 0;
    size_t maxNaluDataLengthToSend, naluDataLengthToSend;
    uint8_t * pNaluData = pCtx->pNaluArray[ pCtx->tailIndex ].pNaluData;

    /* Is this the first fragment? */
    if( pCtx->currentlyProcessingPacket == H264_PACKET_NONE )
    {
        pCtx->currentlyProcessingPacket = H264_FU_A_PACKET;
        pCtx->fuAPacketizationState.naluHeader = pNaluData[ 0 ];

        /* Per RFC https://www.rfc-editor.org/rfc/rfc6184.html, we do not need
         * to send NALU header in FU-A as the information can be constructed
         * using FU indicator and FU header. */
        pCtx->fuAPacketizationState.naluDataIndex = 1;
        pCtx->fuAPacketizationState.remainingNaluLength = pCtx->pNaluArray[ pCtx->tailIndex ].naluDataLength - 1;

        /* Indicate start fragment in the FU header. */
        fuHeader |= FU_A_HEADER_S_BIT_MASK;
    }

    /* Maximum NALU data that we can send in this packet. */
    maxNaluDataLengthToSend = pPacket->packetDataLength - FU_A_HEADER_SIZE;
    /* Actual NALU data what we will send in this packet. */
    naluDataLengthToSend = H264_MIN( maxNaluDataLengthToSend,
                                     pCtx->fuAPacketizationState.remainingNaluLength );

    if( pCtx->fuAPacketizationState.remainingNaluLength == naluDataLengthToSend )
    {
        /* Indicate end fragment in the FU header. */
        fuHeader |= FU_A_HEADER_E_BIT_MASK;
    }

    /* Write FU indicator and header. */
    pPacket->pPacketData[ FU_A_INDICATOR_OFFSET ] = ( FU_A_PACKET_TYPE |
                                                      ( pCtx->fuAPacketizationState.naluHeader &
                                                        NALU_HEADER_NRI_MASK ) );
    pPacket->pPacketData[ FU_A_HEADER_OFFSET ] = ( fuHeader |
                                                   ( pCtx->fuAPacketizationState.naluHeader &
                                                     NALU_HEADER_TYPE_MASK ) );

    /* Write FU payload. */
    memcpy( ( void * ) &( pPacket->pPacketData[ FU_A_PAYLOAD_OFFSET ] ),
            ( const void * ) &( pNaluData[ pCtx->fuAPacketizationState.naluDataIndex ] ),
            naluDataLengthToSend );
    pPacket->packetDataLength = naluDataLengthToSend + FU_A_HEADER_SIZE;

    pCtx->fuAPacketizationState.naluDataIndex += naluDataLengthToSend;
    pCtx->fuAPacketizationState.remainingNaluLength -= naluDataLengthToSend;

    if( pCtx->fuAPacketizationState.remainingNaluLength == 0 )
    {
        /* Reset state. */
        memset( &( pCtx->fuAPacketizationState ),
                0,
                sizeof( FuAPacketizationState_t ) );
        pCtx->currentlyProcessingPacket = H264_PACKET_NONE;

        /* Move to the next NALU in the next call to H264Packetizer_GetPacket. */
        pCtx->tailIndex += 1;
        pCtx->naluCount -= 1;
    }
}

/*-----------------------------------------------------------*/

H264Result_t H264Packetizer_Init( H264PacketizerContext_t * pCtx,
                                  Nalu_t * pNaluArray,
                                  size_t naluArrayLength )
{
    H264Result_t result = H264_RESULT_OK;

    if( ( pCtx == NULL ) ||
        ( pNaluArray == NULL ) ||
        ( naluArrayLength == 0 ) )
    {
        result = H264_RESULT_BAD_PARAM;
    }

    if( result == H264_RESULT_OK )
    {
        pCtx->pNaluArray = pNaluArray;
        pCtx->naluArrayLength = naluArrayLength;

        pCtx->headIndex = 0;
        pCtx->tailIndex = 0;
        pCtx->naluCount = 0;

        pCtx->currentlyProcessingPacket = H264_PACKET_NONE;

        memset( &( pCtx->fuAPacketizationState ),
                0,
                sizeof( FuAPacketizationState_t ) );
    }

    return result;
}

/*-----------------------------------------------------------*/

H264Result_t H264Packetizer_AddFrame( H264PacketizerContext_t * pCtx,
                                      Frame_t * pFrame )
{
    H264Result_t result = H264_RESULT_OK;
    Nalu_t nalu;
    size_t currentIndex = 0, naluStartIndex = 0, remainingFrameLength;
    uint8_t startCode1[] = { 0x00, 0x00, 0x00, 0x01 }; /* 4-byte start code. */
    uint8_t startCode2[] = { 0x00, 0x00, 0x01 }; /* 3-byte start code. */
    uint8_t firstStartCode = 1;

    if( ( pCtx == NULL ) ||
        ( pFrame == NULL ) ||
        ( pFrame->pFrameData == NULL ) ||
        ( pFrame->frameDataLength == 0 ) )
    {
        result = H264_RESULT_BAD_PARAM;
    }

    while( ( result == H264_RESULT_OK ) &&
           ( currentIndex < pFrame->frameDataLength ) )
    {
        remainingFrameLength = pFrame->frameDataLength - currentIndex;

        /* Check the presence of 4 byte start code. */
        if( remainingFrameLength >= sizeof( startCode1 ) )
        {
            if( memcmp( &( pFrame->pFrameData[ currentIndex ] ),
                        &( startCode1[ 0 ] ),
                        sizeof( startCode1 ) ) == 0 )
            {
                if( firstStartCode == 1 )
                {
                    firstStartCode = 0;
                }
                else
                {
                    /* Create NAL unit from data between start codes. */
                    nalu.pNaluData = &( pFrame->pFrameData[ naluStartIndex ] );
                    nalu.naluDataLength = currentIndex - naluStartIndex;

                    result = H264Packetizer_AddNalu( pCtx,
                                                     &( nalu ) );
                }

                naluStartIndex = currentIndex + sizeof( startCode1 );
                currentIndex = naluStartIndex;
                continue;
            }
        }

        /* Check the presence of 3 byte start code. */
        if( remainingFrameLength >= sizeof( startCode2 ) )
        {
            if( memcmp( &( pFrame->pFrameData[ currentIndex ] ),
                        &( startCode2[ 0 ] ),
                        sizeof( startCode2 ) ) == 0 )
            {
                if( firstStartCode == 1 )
                {
                    firstStartCode = 0;
                }
                else
                {
                    /* Create NAL unit from data between start codes. */
                    nalu.pNaluData = &( pFrame->pFrameData[ naluStartIndex ] );
                    nalu.naluDataLength = currentIndex - naluStartIndex;

                    result = H264Packetizer_AddNalu( pCtx,
                                                     &( nalu ) );
                }

                naluStartIndex = currentIndex + sizeof( startCode2 );
                currentIndex = naluStartIndex;
                continue;
            }
        }

        currentIndex += 1;
    }

    /* Handle last NAL unit in frame. */
    if( result == H264_RESULT_OK )
    {
        if( naluStartIndex > 0 )
        {
            nalu.pNaluData = &( pFrame->pFrameData[ naluStartIndex ] );
            nalu.naluDataLength = pFrame->frameDataLength - naluStartIndex;

            result = H264Packetizer_AddNalu( pCtx,
                                             &( nalu ) );
        }
        else
        {
            result = H264_RESULT_MALFORMED_PACKET;
        }
    }

    return result;
}

/*-----------------------------------------------------------*/

H264Result_t H264Packetizer_AddNalu( H264PacketizerContext_t * pCtx,
                                     Nalu_t * pNalu )
{
    H264Result_t result = H264_RESULT_OK;

    if( ( pCtx == NULL ) ||
        ( pNalu == NULL ) ||
        ( pNalu->pNaluData == NULL ) )
    {
        result = H264_RESULT_BAD_PARAM;
    }

    if( result == H264_RESULT_OK )
    {
        if( pNalu->naluDataLength < NALU_HEADER_SIZE )
        {
            result = H264_RESULT_MALFORMED_PACKET;
        }
    }

    if( result == H264_RESULT_OK )
    {
        if( pCtx->naluCount >= pCtx->naluArrayLength )
        {
            result = H264_RESULT_OUT_OF_MEMORY;
        }
    }

    if( result == H264_RESULT_OK )
    {
        pCtx->pNaluArray[ pCtx->headIndex ].pNaluData = pNalu->pNaluData;
        pCtx->pNaluArray[ pCtx->headIndex ].naluDataLength = pNalu->naluDataLength;
        pCtx->headIndex += 1;
        pCtx->naluCount += 1;
    }

    return result;
}

/*-----------------------------------------------------------*/

H264Result_t H264Packetizer_GetPacket( H264PacketizerContext_t * pCtx,
                                       H264Packet_t * pPacket )
{
    H264Result_t result = H264_RESULT_OK;

    if( ( pCtx == NULL ) ||
        ( pPacket == NULL ) ||
        ( pPacket->pPacketData == NULL ) ||
        ( pPacket->packetDataLength == 0 ) )
    {
        result = H264_RESULT_BAD_PARAM;
    }

    if( result == H264_RESULT_OK )
    {
        if( pCtx->naluCount == 0 )
        {
            result = H264_RESULT_NO_MORE_PACKETS;
        }
    }

    if( result == H264_RESULT_OK )
    {
        /* Are we in the middle of packetizing fragments of a NALU? */
        if( pCtx->currentlyProcessingPacket == H264_FU_A_PACKET )
        {
            /* Continue packetizing fragments. */
            PacketizeFragmentationUnitPacket( pCtx,
                                              pPacket );
        }
        else
        {
            /* If a NAL Unit can fit in one packet, use Single NAL Unit packet. */
            if( pCtx->pNaluArray[ pCtx->tailIndex ].naluDataLength <= pPacket->packetDataLength )
            {
                PacketizeSingleNaluPacket( pCtx,
                                           pPacket );
            }
            else
            {
                /* Otherwise, fragment the NAL Unit in more than one packets. */
                PacketizeFragmentationUnitPacket( pCtx,
                                                  pPacket );
            }
        }
    }

    return result;
}

/*-----------------------------------------------------------*/
