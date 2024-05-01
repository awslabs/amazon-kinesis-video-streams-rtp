/* Standard includes. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* API includes. */
#include "h264_depacketizer.h"
#include "vp8_depacketizer.h"
#include "g711_depacketizer.h"
#include "opus_depacketizer.h"

#define MAX_PACKETS_IN_A_FRAME 512
#define MAX_NALU_LENGTH   5 * 1024
#define MAX_FRAME_LENGTH  10 * 1024
#define MAX_PACKET_IN_A_FRAME 512

#define VP8_PACKETS_ARR_LEN 10
#define VP8_FRAME_BUF_LEN 32
/*-----------------------------------------------------------*/

void Depacketize_Nalus( void )
{
    int i;
    H264Result_t result;
    H264Packet_t pkt;
    H264DepacketizerContext_t ctx;
    Nalu_t nalu;
    uint8_t naluBuffer[ MAX_NALU_LENGTH ];
    H264Packet_t packetsArray[ MAX_PACKETS_IN_A_FRAME ];
    uint8_t packetData1[] = {0x09, 0x10};
    uint8_t packetData2[] = {0x67, 0x42, 0xc0, 0x1f, 0xda, 0x01, 0x40, 0x16, 0xec, 0x05, 0xa8, 0x08 };
    uint8_t packetData3[] = {0x68, 0xce, 0x3c, 0x80 };
    uint8_t packetData4[] = {0x06, 0x05, 0xff, 0xff, 0xb7, 0xdc, 0x45, 0xe9, 0xbd, 0xe6, 0xd9, 0x48 };
    uint8_t packetData5[] = {0x7c, 0x85, 0x88, 0x84, 0x12, 0xff, 0xff, 0xfc, 0x3d, 0x14, 0x0, 0x4 };
    uint8_t packetData6[] = {0x7c, 0x45, 0xba, 0xeb, 0xae, 0xba, 0xeb, 0xae, 0xba, 0xeb, 0xae, 0xba };
    uint8_t packetData7[] = {0x65, 0x00, 0x6e, 0x22, 0x21, 0x04, 0xbf, 0xff, 0xff, 0x0f, 0x45, 0x00};

    result = H264Depacketizer_Init( &( ctx ),
                                    &( packetsArray[ 0 ] ),
                                    MAX_PACKETS_IN_A_FRAME );

    assert( result == H264_RESULT_OK );

    pkt.pPacketData = &( packetData1[ 0 ] );
    pkt.packetDataLength = sizeof( packetData1 );
    result = H264Depacketizer_AddPacket( &( ctx ),
                                         &( pkt ) );
    assert( result == H264_RESULT_OK );

    pkt.pPacketData = &( packetData2[ 0 ] );
    pkt.packetDataLength = sizeof( packetData2 );
    result = H264Depacketizer_AddPacket( &( ctx ),
                                         &( pkt ) );
    assert( result == H264_RESULT_OK );

    pkt.pPacketData = &( packetData3[ 0 ] );
    pkt.packetDataLength = sizeof( packetData3 );
    result = H264Depacketizer_AddPacket( &( ctx ),
                                         &( pkt ) );
    assert( result == H264_RESULT_OK );

    pkt.pPacketData = &( packetData4[ 0 ] );
    pkt.packetDataLength = sizeof( packetData4 );
    result = H264Depacketizer_AddPacket( &( ctx ),
                                         &( pkt ) );
    assert( result == H264_RESULT_OK );

    pkt.pPacketData = &( packetData5[ 0 ] );
    pkt.packetDataLength = sizeof( packetData5 );
    result = H264Depacketizer_AddPacket( &( ctx ),
                                         &( pkt ) );
    assert( result == H264_RESULT_OK );

    pkt.pPacketData = &( packetData6[ 0 ] );
    pkt.packetDataLength = sizeof( packetData6 );
    result = H264Depacketizer_AddPacket( &( ctx ),
                                         &( pkt ) );
    assert( result == H264_RESULT_OK );

    pkt.pPacketData = &( packetData7[ 0 ] );
    pkt.packetDataLength = sizeof( packetData7 );
    result = H264Depacketizer_AddPacket( &( ctx ),
                                         &( pkt ) );
    assert( result == H264_RESULT_OK );

    nalu.pNaluData = &( naluBuffer[ 0 ] );
    nalu.naluDataLength = MAX_NALU_LENGTH;
    result = H264Depacketizer_GetNalu( &( ctx ),
                                       &( nalu ) );
    assert( result == H264_RESULT_OK );

    while( result != H264_RESULT_NO_MORE_NALUS )
    {
        nalu.pNaluData = &( naluBuffer[ 0 ] );
        nalu.naluDataLength = MAX_NALU_LENGTH;
        result = H264Depacketizer_GetNalu( &( ctx ),
                                           &( nalu ) );

    }
    assert( result == H264_RESULT_NO_MORE_NALUS );
}

/*-----------------------------------------------------------*/

void Depacketize_Frames( void )
{
    H264Result_t result;
    H264Packet_t pkt;
    H264DepacketizerContext_t ctx;
    Frame_t frame;
    uint8_t frameBuffer[ MAX_FRAME_LENGTH ];
    H264Packet_t packetsArray[ MAX_PACKETS_IN_A_FRAME ];
    uint8_t packetData1[] = {0x09, 0x10};
    uint8_t packetData2[] = {0x67, 0x42, 0xc0, 0x1f, 0xda, 0x01, 0x40, 0x16, 0xec, 0x05, 0xa8, 0x08 };
    uint8_t packetData3[] = {0x68, 0xce, 0x3c, 0x80 };
    uint8_t packetData4[] = {0x06, 0x05, 0xff, 0xff, 0xb7, 0xdc, 0x45, 0xe9, 0xbd, 0xe6, 0xd9, 0x48 };
    uint8_t packetData5[] = {0x7c, 0x85, 0x88, 0x84, 0x12, 0xff, 0xff, 0xfc, 0x3d, 0x14, 0x0, 0x4 };
    uint8_t packetData6[] = {0x7c, 0x45, 0xba, 0xeb, 0xae, 0xba, 0xeb, 0xae, 0xba, 0xeb, 0xae, 0xba };
    uint8_t packetData7[] = {0x65, 0x00, 0x6e, 0x22, 0x21, 0x04, 0xbf, 0xff, 0xff, 0x0f, 0x45, 0x00};
    uint32_t totalSize = 0;
    result = H264Depacketizer_Init( &( ctx ),
                                    &( packetsArray[ 0 ] ),
                                    MAX_PACKETS_IN_A_FRAME );
    assert( result == H264_RESULT_OK );

    pkt.pPacketData = &( packetData1[ 0 ] );
    pkt.packetDataLength = sizeof( packetData1 );
    totalSize += pkt.packetDataLength;
    result = H264Depacketizer_AddPacket( &( ctx ),
                                         &( pkt ) );
    assert( result == H264_RESULT_OK );

    pkt.pPacketData = &( packetData2[ 0 ] );
    pkt.packetDataLength = sizeof( packetData2 );
    totalSize += pkt.packetDataLength;
    result = H264Depacketizer_AddPacket( &( ctx ),
                                         &( pkt ) );
    assert( result == H264_RESULT_OK );

    pkt.pPacketData = &( packetData3[ 0 ] );
    pkt.packetDataLength = sizeof( packetData3 );
    totalSize += pkt.packetDataLength;
    result = H264Depacketizer_AddPacket( &( ctx ),
                                         &( pkt ) );
    assert( result == H264_RESULT_OK );

    pkt.pPacketData = &( packetData4[ 0 ] );
    pkt.packetDataLength = sizeof( packetData4 );
    totalSize += pkt.packetDataLength;
    result = H264Depacketizer_AddPacket( &( ctx ),
                                         &( pkt ) );
    assert( result == H264_RESULT_OK );

    pkt.pPacketData = &( packetData5[ 0 ] );
    pkt.packetDataLength = sizeof( packetData5 );
    totalSize += pkt.packetDataLength;
    result = H264Depacketizer_AddPacket( &( ctx ),
                                         &( pkt ) );
    assert( result == H264_RESULT_OK );

    pkt.pPacketData = &( packetData6[ 0 ] );
    pkt.packetDataLength = sizeof( packetData6 );
    totalSize += pkt.packetDataLength;
    result = H264Depacketizer_AddPacket( &( ctx ),
                                         &( pkt ) );
    assert( result == H264_RESULT_OK );

    pkt.pPacketData = &( packetData7[ 0 ] );
    pkt.packetDataLength = sizeof( packetData7 );
    totalSize += pkt.packetDataLength;
    result = H264Depacketizer_AddPacket( &( ctx ),
                                         &( pkt ) );
    assert( result == H264_RESULT_OK );

    frame.pFrameData = &( frameBuffer[ 0 ] );
    frame.frameDataLength = MAX_FRAME_LENGTH;
    result = H264Depacketizer_GetFrame( &( ctx ),
                                        &( frame ) );
    assert( result == H264_RESULT_OK );
}

/*-----------------------------------------------------------*/

void GetPropertiesTest( void )
{
    uint8_t singleNaluPacket[] = { 0x09, 0x10 };
    uint8_t fuAStartPacket[] = { 0x7C, 0x89, 0xAB, 0xCD };
    uint8_t fuAEndPacket[] = { 0x7C, 0x49, 0xAB, 0xCD };
    uint32_t packetProperties;
    H264Result_t result;
    H264Packet_t pkt;

    pkt.pPacketData = &( singleNaluPacket[ 0 ] );
    pkt.packetDataLength = sizeof( singleNaluPacket );
    result = H264Depacketizer_GetPacketProperties( pkt.pPacketData,
                                                   pkt.packetDataLength,
                                                   &( packetProperties ) );
    assert( result == H264_RESULT_OK );
    assert( packetProperties == H264_PACKET_PROPERTY_START_PACKET );

    pkt.pPacketData = &( fuAStartPacket[ 0 ] );
    pkt.packetDataLength = sizeof( fuAStartPacket );
    result = H264Depacketizer_GetPacketProperties( pkt.pPacketData,
                                                   pkt.packetDataLength,
                                                   &( packetProperties ) );
    assert( result == H264_RESULT_OK );
    assert( packetProperties == H264_PACKET_PROPERTY_START_PACKET );

    pkt.pPacketData = &( fuAEndPacket[ 0 ] );
    pkt.packetDataLength = sizeof( fuAEndPacket );
    result = H264Depacketizer_GetPacketProperties( pkt.pPacketData,
                                                   pkt.packetDataLength,
                                                   &( packetProperties ) );
    assert( result == H264_RESULT_OK );
    assert( packetProperties == H264_PACKET_PROPERTY_END_PACKET );
}

/*-----------------------------------------------------------*/

void Depacketize_VP8_Frames_1()
{
    VP8DepacketizerContext_t ctx;
    VP8Result_t result;
    VP8Packet_t pkt;
    VP8Frame_t frame;
    VP8Packet_t packetsArray[ VP8_PACKETS_ARR_LEN ];
    uint8_t frameBuffer[ VP8_FRAME_BUF_LEN ];
    size_t i;

    uint8_t packetData1[] = { 0xB0, 0xF0, 0xFA, 0xCD, 0xAB, 0xBA, 0x00, 0x01, 0x02, 0x03 };
    uint8_t packetData2[] = { 0xA0, 0xF0, 0xFA, 0xCD, 0xAB, 0xBA, 0x04, 0x05, 0x10, 0x11 };
    uint8_t packetData3[] = { 0xA0, 0xF0, 0xFA, 0xCD, 0xAB, 0xBA, 0x12, 0x13, 0x14, 0x15 };
    uint8_t packetData4[] = { 0xA0, 0xF0, 0xFA, 0xCD, 0xAB, 0xBA, 0x20, 0x21, 0x22, 0x23 };
    uint8_t packetData5[] = { 0xA0, 0xF0, 0xFA, 0xCD, 0xAB, 0xBA, 0x24, 0x25, 0x30, 0x31 };
    uint8_t packetData6[] = { 0xA0, 0xF0, 0xFA, 0xCD, 0xAB, 0xBA, 0x32, 0x33, 0x34, 0x35 };
    uint8_t packetData7[] = { 0xA0, 0xF0, 0xFA, 0xCD, 0xAB, 0xBA, 0x40, 0x41 };

    uint8_t decodedFrameData[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
                                   0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
                                   0x20, 0x21, 0x22, 0x23, 0x24, 0x25,
                                   0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
                                   0x40, 0x41 };

    result = VP8Depacketizer_Init( &( ctx ),
                                   &( packetsArray[ 0 ] ),
                                   VP8_PACKETS_ARR_LEN );
    assert( result == VP8_RESULT_OK );

    pkt.pPacketData = &( packetData1[ 0 ] );
    pkt.packetDataLength = sizeof( packetData1 );
    result = VP8Depacketizer_AddPacket( &( ctx ),
                                        &( pkt ) );
    assert( result == VP8_RESULT_OK );

    pkt.pPacketData = &( packetData2[ 0 ] );
    pkt.packetDataLength = sizeof( packetData2 );
    result = VP8Depacketizer_AddPacket( &( ctx ),
                                        &( pkt ) );
    assert( result == VP8_RESULT_OK );

    pkt.pPacketData = &( packetData3[ 0 ] );
    pkt.packetDataLength = sizeof( packetData3 );
    result = VP8Depacketizer_AddPacket( &( ctx ),
                                        &( pkt ) );
    assert( result == VP8_RESULT_OK );

    pkt.pPacketData = &( packetData4[ 0 ] );
    pkt.packetDataLength = sizeof( packetData4 );
    result = VP8Depacketizer_AddPacket( &( ctx ),
                                        &( pkt ) );
    assert( result == VP8_RESULT_OK );

    pkt.pPacketData = &( packetData5[ 0 ] );
    pkt.packetDataLength = sizeof( packetData5 );
    result = VP8Depacketizer_AddPacket( &( ctx ),
                                        &( pkt ) );
    assert( result == VP8_RESULT_OK );

    pkt.pPacketData = &( packetData6[ 0 ] );
    pkt.packetDataLength = sizeof( packetData6 );
    result = VP8Depacketizer_AddPacket( &( ctx ),
                                        &( pkt ) );
    assert( result == VP8_RESULT_OK );

    pkt.pPacketData = &( packetData7[ 0 ] );
    pkt.packetDataLength = sizeof( packetData7 );
    result = VP8Depacketizer_AddPacket( &( ctx ),
                                        &( pkt ) );
    assert( result == VP8_RESULT_OK );

    frame.pFrameData = &( frameBuffer[ 0 ] );
    frame.frameDataLength = VP8_FRAME_BUF_LEN;
    result = VP8Depacketizer_GetFrame( &( ctx ),
                                       &( frame ) );
    assert( result == VP8_RESULT_OK );

    assert( frame.frameDataLength == sizeof( decodedFrameData ) );
    for( i = 0; i < frame.frameDataLength; i++ )
    {
        assert( frame.pFrameData[ i ] == decodedFrameData[ i ] );
    }
    assert( frame.pictureId == 0x7ACD );
    assert( frame.tl0PicIndex == 0xAB );
    assert( frame.tid == 5 );
    assert( frame.keyIndex == 10 );
    assert( frame.frameProperties == ( VP8_FRAME_PROP_NON_REF_FRAME |
                                       VP8_FRAME_PROP_PICTURE_ID_PRESENT |
                                       VP8_FRAME_PROP_TL0PICIDX_PRESENT |
                                       VP8_FRAME_PROP_TID_PRESENT |
                                       VP8_FRAME_PROP_KEYIDX_PRESENT |
                                       VP8_FRAME_PROP_DEPENDS_ON_BASE_ONLY ) );
}

/*-----------------------------------------------------------*/

void Depacketize_VP8_Frames_2()
{
    VP8DepacketizerContext_t ctx;
    VP8Result_t result;
    VP8Packet_t pkt;
    VP8Frame_t frame;
    VP8Packet_t packetsArray[ VP8_PACKETS_ARR_LEN ];
    uint8_t frameBuffer[ VP8_FRAME_BUF_LEN ];
    size_t i;

    uint8_t packetData1[] = { 0xB0, 0x40, 0xAB, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x10 };
    uint8_t packetData2[] = { 0xA0, 0x40, 0xAB, 0x11, 0x12, 0x13, 0x14, 0x15, 0x20, 0x21 };
    uint8_t packetData3[] = { 0xA0, 0x40, 0xAB, 0x22, 0x23, 0x24, 0x25, 0x30, 0x31, 0x32 };
    uint8_t packetData4[] = { 0xA0, 0x40, 0xAB, 0x33, 0x34, 0x35, 0x40, 0x41 };

    uint8_t decodedFrameData[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
                                   0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
                                   0x20, 0x21, 0x22, 0x23, 0x24, 0x25,
                                   0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
                                   0x40, 0x41 };

    result = VP8Depacketizer_Init( &( ctx ),
                                   &( packetsArray[ 0 ] ),
                                   VP8_PACKETS_ARR_LEN );
    assert( result == VP8_RESULT_OK );

    pkt.pPacketData = &( packetData1[ 0 ] );
    pkt.packetDataLength = sizeof( packetData1 );
    result = VP8Depacketizer_AddPacket( &( ctx ),
                                        &( pkt ) );
    assert( result == VP8_RESULT_OK );

    pkt.pPacketData = &( packetData2[ 0 ] );
    pkt.packetDataLength = sizeof( packetData2 );
    result = VP8Depacketizer_AddPacket( &( ctx ),
                                        &( pkt ) );
    assert( result == VP8_RESULT_OK );

    pkt.pPacketData = &( packetData3[ 0 ] );
    pkt.packetDataLength = sizeof( packetData3 );
    result = VP8Depacketizer_AddPacket( &( ctx ),
                                        &( pkt ) );
    assert( result == VP8_RESULT_OK );

    pkt.pPacketData = &( packetData4[ 0 ] );
    pkt.packetDataLength = sizeof( packetData4 );
    result = VP8Depacketizer_AddPacket( &( ctx ),
                                        &( pkt ) );
    assert( result == VP8_RESULT_OK );

    frame.pFrameData = &( frameBuffer[ 0 ] );
    frame.frameDataLength = VP8_FRAME_BUF_LEN;
    result = VP8Depacketizer_GetFrame( &( ctx ),
                                       &( frame ) );
    assert( result == VP8_RESULT_OK );

    assert( frame.frameDataLength == sizeof( decodedFrameData ) );
    for( i = 0; i < frame.frameDataLength; i++ )
    {
        assert( frame.pFrameData[ i ] == decodedFrameData[ i ] );
    }
    assert( frame.tl0PicIndex == 0xAB );
    assert( frame.frameProperties == ( VP8_FRAME_PROP_NON_REF_FRAME |
                                       VP8_FRAME_PROP_TL0PICIDX_PRESENT ) );
}

/*-----------------------------------------------------------*/

void Depacketize_G711_Frames()
{
    G711Result_t result;
    G711DepacketizerContext_t ctx;
    G711Packet_t packetsArray[ MAX_PACKET_IN_A_FRAME ], pkt;
    G711Frame_t frame;
    uint8_t frameBuffer[ MAX_FRAME_LENGTH ];
    size_t i;

    uint8_t packetData1[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x10, 0x11, 0x12, 0x13};
    uint8_t packetData2[] = {0x14, 0x15, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x30, 0x31};
    uint8_t packetData3[] = {0x32, 0x33, 0x34, 0x35, 0x40, 0x41};

    uint8_t decodedFrame[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
                               0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
                               0x20, 0x21, 0x22, 0x23, 0x24, 0x25,
                               0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
                               0x40, 0x41 };

    result = G711Depacketizer_Init( &( ctx ),
                                    &( packetsArray[ 0 ] ),
                                    MAX_PACKET_IN_A_FRAME );
    assert( result == G711_RESULT_OK );

    pkt.pPacketData = &( packetData1[ 0 ] );
    pkt.packetDataLength = sizeof( packetData1 );
    result = G711Depacketizer_AddPacket( &( ctx ),
                                         &( pkt ) );
    assert( result == G711_RESULT_OK );

    pkt.pPacketData = &( packetData2[ 0 ] );
    pkt.packetDataLength = sizeof( packetData2 );
    result = G711Depacketizer_AddPacket( &( ctx ),
                                         &( pkt ) );
    assert( result == G711_RESULT_OK );

    pkt.pPacketData = &( packetData3[ 0 ] );
    pkt.packetDataLength = sizeof( packetData3 );
    result = G711Depacketizer_AddPacket( &( ctx ),
                                         &( pkt ) );
    assert( result == G711_RESULT_OK );

    frame.pFrameData = &( frameBuffer[ 0 ] );
    frame.frameDataLength = MAX_FRAME_LENGTH;
    result = G711Depacketizer_GetFrame( &( ctx ),
                                        &( frame ) );
    assert( result == G711_RESULT_OK );
    assert( frame.frameDataLength == sizeof( decodedFrame ) );
    for( i = 0; i < frame.frameDataLength; i++ )
    {
        assert( frame.pFrameData[ i ] == decodedFrame[ i ] );
    }
}

/*-----------------------------------------------------------*/

void Depacketize_Opus_Frames()
{
    OpusResult_t result;
    OpusDepacketizerContext_t ctx;
    OpusPacket_t packetsArray[ MAX_PACKET_IN_A_FRAME ], pkt;
    OpusFrame_t frame;
    uint8_t frameBuffer[ MAX_FRAME_LENGTH ];
    size_t i;

    uint8_t packetData1[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x10, 0x11, 0x12, 0x13};
    uint8_t packetData2[] = {0x14, 0x15, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x30, 0x31};
    uint8_t packetData3[] = {0x32, 0x33, 0x34, 0x35, 0x40, 0x41};

    uint8_t decodedFrame[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
                               0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
                               0x20, 0x21, 0x22, 0x23, 0x24, 0x25,
                               0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
                               0x40, 0x41 };

    result = OpusDepacketizer_Init( &( ctx ),
                                    &( packetsArray[ 0 ] ),
                                    MAX_PACKET_IN_A_FRAME );
    assert( result == OPUS_RESULT_OK );

    pkt.pPacketData = &( packetData1[ 0 ] );
    pkt.packetDataLength = sizeof( packetData1 );
    result = OpusDepacketizer_AddPacket( &( ctx ),
                                         &( pkt ) );
    assert( result == OPUS_RESULT_OK );

    pkt.pPacketData = &( packetData2[ 0 ] );
    pkt.packetDataLength = sizeof( packetData2 );
    result = OpusDepacketizer_AddPacket( &( ctx ),
                                         &( pkt ) );
    assert( result == OPUS_RESULT_OK );

    pkt.pPacketData = &( packetData3[ 0 ] );
    pkt.packetDataLength = sizeof( packetData3 );
    result = OpusDepacketizer_AddPacket( &( ctx ),
                                         &( pkt ) );
    assert( result == OPUS_RESULT_OK );

    frame.pFrameData = &( frameBuffer[ 0 ] );
    frame.frameDataLength = MAX_FRAME_LENGTH;
    result = OpusDepacketizer_GetFrame( &( ctx ),
                                        &( frame ) );
    assert( result == OPUS_RESULT_OK );
    assert( frame.frameDataLength == sizeof( decodedFrame ) );
    for( i = 0; i < frame.frameDataLength; i++ )
    {
        assert( frame.pFrameData[ i ] == decodedFrame[ i ] );
    }
}

/*-----------------------------------------------------------*/
void H264_test_case()
{
    printf( "\nRunning H264 Test Cases\n" );
    Depacketize_Nalus();
    printf( "\tTest 1 - H264 depacketize Nalus Test passed\n" );
    Depacketize_Frames();
    printf( "\tTest 2 - H264 depacketize Frames Test passed\n" );
    GetPropertiesTest();
    printf( "\tTest 3 - H264 Get Properties Passed\n" );

}

void VP8_test_case()
{
    printf( "\nRunning VP8 Test Cases\n" );
    Depacketize_VP8_Frames_1();
    printf( "\tTest 1 - VP8 depacketize Test passed\n" );
    Depacketize_VP8_Frames_2();
    printf( "\tTest 2 - VP8 depacketize Test passed\n" );
}

void G711_test_case()
{
    printf( "\nRunning G711 Test Cases\n" );
    Depacketize_G711_Frames();
    printf( "\tTest 1 - G711 depacketize Test passed\n" );
}

void Opus_test_case()
{
    printf( "\nRunning OPUS Test Cases\n" );
    Depacketize_Opus_Frames();
    printf( "\tTest 1 - Opus depacketize Test passed\n" );
}

int main( void )
{
    H264_test_case();

    VP8_test_case();

    G711_test_case();

    Opus_test_case();

    return 0;
}

/*-----------------------------------------------------------*/