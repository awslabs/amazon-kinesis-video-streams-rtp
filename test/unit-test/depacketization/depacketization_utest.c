/* Unity includes. */
#include "unity.h"
#include "catch_assert.h"

/* Standard includes. */
#include <string.h>
#include <stdint.h>

/* API includes. */
#include "h264_depacketizer.h"
#include "g711_depacketizer.h"
#include "vp8_depacketizer.h"
#include "opus_depacketizer.h"

/* ===========================  EXTERN VARIABLES  =========================== */

#define MAX_PACKETS_IN_A_FRAME  512
#define MAX_NALU_LENGTH         5 * 1024
#define MAX_FRAME_LENGTH        10 * 1024
#define MAX_PACKET_IN_A_FRAME   512

#define VP8_PACKETS_ARR_LEN     10
#define VP8_FRAME_BUF_LEN       32

uint8_t frameBuffer[ MAX_FRAME_LENGTH ];

void setUp( void )
{
    memset( &( frameBuffer[ 0 ] ),
            0,
            sizeof( frameBuffer ) );
}

void tearDown( void )
{
}

/* ==============================  Test Cases  ============================== */

/**
 * @brief Validate Opus depacketization happy path.
 */
void test_Opus_Depacketizer( void )
{
    OpusResult_t result;
    OpusDepacketizerContext_t ctx;
    OpusPacket_t packetsArray[ MAX_PACKET_IN_A_FRAME ], pkt;
    OpusFrame_t frame;

    uint8_t packetData1[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x10, 0x11, 0x12, 0x13 };
    uint8_t packetData2[] = { 0x14, 0x15, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x30, 0x31 };
    uint8_t packetData3[] = { 0x32, 0x33, 0x34, 0x35, 0x40, 0x41 };

    uint8_t decodedFrame[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
                               0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
                               0x20, 0x21, 0x22, 0x23, 0x24, 0x25,
                               0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
                               0x40, 0x41 };

    result = OpusDepacketizer_Init( &( ctx ),
                                    &( packetsArray[ 0 ] ),
                                    MAX_PACKET_IN_A_FRAME );
    TEST_ASSERT_EQUAL( result,
                       OPUS_RESULT_OK );

    pkt.pPacketData = &( packetData1[ 0 ] );
    pkt.packetDataLength = sizeof( packetData1 );
    result = OpusDepacketizer_AddPacket( &( ctx ),
                                         &( pkt ) );
    TEST_ASSERT_EQUAL( result,
                       OPUS_RESULT_OK );

    pkt.pPacketData = &( packetData2[ 0 ] );
    pkt.packetDataLength = sizeof( packetData2 );
    result = OpusDepacketizer_AddPacket( &( ctx ),
                                         &( pkt ) );
    TEST_ASSERT_EQUAL( result,
                       OPUS_RESULT_OK );

    pkt.pPacketData = &( packetData3[ 0 ] );
    pkt.packetDataLength = sizeof( packetData3 );
    result = OpusDepacketizer_AddPacket( &( ctx ),
                                         &( pkt ) );
    TEST_ASSERT_EQUAL( result,
                       OPUS_RESULT_OK );

    frame.pFrameData = &( frameBuffer[ 0 ] );
    frame.frameDataLength = MAX_FRAME_LENGTH;
    result = OpusDepacketizer_GetFrame( &( ctx ),
                                        &( frame ) );
    TEST_ASSERT_EQUAL( result,
                       OPUS_RESULT_OK );
    TEST_ASSERT_EQUAL( frame.frameDataLength,
                       sizeof( decodedFrame ) );
    TEST_ASSERT_EQUAL_UINT8_ARRAY( &( decodedFrame[ 0 ] ),
                                   &( frame.pFrameData[ 0 ] ),
                                   frame.frameDataLength );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate G711 depacketization happy path.
 */
void test_G711_Depacketizer( void )
{
    G711Result_t result;
    G711DepacketizerContext_t ctx;
    G711Packet_t packetsArray[ MAX_PACKET_IN_A_FRAME ], pkt;
    G711Frame_t frame;

    uint8_t packetData1[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x10, 0x11, 0x12, 0x13 };
    uint8_t packetData2[] = { 0x14, 0x15, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x30, 0x31 };
    uint8_t packetData3[] = { 0x32, 0x33, 0x34, 0x35, 0x40, 0x41 };

    uint8_t decodedFrame[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
                               0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
                               0x20, 0x21, 0x22, 0x23, 0x24, 0x25,
                               0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
                               0x40, 0x41 };

    result = G711Depacketizer_Init( &( ctx ),
                                    &( packetsArray[ 0 ] ),
                                    MAX_PACKET_IN_A_FRAME );
    TEST_ASSERT_EQUAL( result,
                       G711_RESULT_OK );

    pkt.pPacketData = &( packetData1[ 0 ] );
    pkt.packetDataLength = sizeof( packetData1 );
    result = G711Depacketizer_AddPacket( &( ctx ),
                                         &( pkt ) );
    TEST_ASSERT_EQUAL( result,
                       G711_RESULT_OK );

    pkt.pPacketData = &( packetData2[ 0 ] );
    pkt.packetDataLength = sizeof( packetData2 );
    result = G711Depacketizer_AddPacket( &( ctx ),
                                         &( pkt ) );
    TEST_ASSERT_EQUAL( result,
                       G711_RESULT_OK );

    pkt.pPacketData = &( packetData3[ 0 ] );
    pkt.packetDataLength = sizeof( packetData3 );
    result = G711Depacketizer_AddPacket( &( ctx ),
                                         &( pkt ) );
    TEST_ASSERT_EQUAL( result,
                       G711_RESULT_OK );

    frame.pFrameData = &( frameBuffer[ 0 ] );
    frame.frameDataLength = MAX_FRAME_LENGTH;
    result = G711Depacketizer_GetFrame( &( ctx ),
                                        &( frame ) );
    TEST_ASSERT_EQUAL( result,
                       G711_RESULT_OK );
    TEST_ASSERT_EQUAL( frame.frameDataLength,
                       sizeof( decodedFrame ) );
    TEST_ASSERT_EQUAL_UINT8_ARRAY( &( decodedFrame[ 0 ] ),
                                   &( frame.pFrameData[ 0 ] ),
                                   frame.frameDataLength );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate VP8 depacketization happy path.
 */

void test_VP8_Depacketizer_AllProperties( void )
{
    VP8DepacketizerContext_t ctx;
    VP8Result_t result;
    VP8Packet_t pkt;
    VP8Frame_t frame;
    VP8Packet_t packetsArray[ VP8_PACKETS_ARR_LEN ];

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
    TEST_ASSERT_EQUAL( result,
                       VP8_RESULT_OK );

    pkt.pPacketData = &( packetData1[ 0 ] );
    pkt.packetDataLength = sizeof( packetData1 );
    result = VP8Depacketizer_AddPacket( &( ctx ),
                                        &( pkt ) );
    TEST_ASSERT_EQUAL( result,
                       VP8_RESULT_OK );

    pkt.pPacketData = &( packetData2[ 0 ] );
    pkt.packetDataLength = sizeof( packetData2 );
    result = VP8Depacketizer_AddPacket( &( ctx ),
                                        &( pkt ) );
    TEST_ASSERT_EQUAL( result,
                       VP8_RESULT_OK );

    pkt.pPacketData = &( packetData3[ 0 ] );
    pkt.packetDataLength = sizeof( packetData3 );
    result = VP8Depacketizer_AddPacket( &( ctx ),
                                        &( pkt ) );
    TEST_ASSERT_EQUAL( result,
                       VP8_RESULT_OK );

    pkt.pPacketData = &( packetData4[ 0 ] );
    pkt.packetDataLength = sizeof( packetData4 );
    result = VP8Depacketizer_AddPacket( &( ctx ),
                                        &( pkt ) );
    TEST_ASSERT_EQUAL( result,
                       VP8_RESULT_OK );

    pkt.pPacketData = &( packetData5[ 0 ] );
    pkt.packetDataLength = sizeof( packetData5 );
    result = VP8Depacketizer_AddPacket( &( ctx ),
                                        &( pkt ) );
    TEST_ASSERT_EQUAL( result,
                       VP8_RESULT_OK );

    pkt.pPacketData = &( packetData6[ 0 ] );
    pkt.packetDataLength = sizeof( packetData6 );
    result = VP8Depacketizer_AddPacket( &( ctx ),
                                        &( pkt ) );
    TEST_ASSERT_EQUAL( result,
                       VP8_RESULT_OK );

    pkt.pPacketData = &( packetData7[ 0 ] );
    pkt.packetDataLength = sizeof( packetData7 );
    result = VP8Depacketizer_AddPacket( &( ctx ),
                                        &( pkt ) );
    TEST_ASSERT_EQUAL( result,
                       VP8_RESULT_OK );

    frame.pFrameData = &( frameBuffer[ 0 ] );
    frame.frameDataLength = VP8_FRAME_BUF_LEN;
    result = VP8Depacketizer_GetFrame( &( ctx ),
                                       &( frame ) );
    TEST_ASSERT_EQUAL( result,
                       VP8_RESULT_OK );

    TEST_ASSERT_EQUAL( frame.frameDataLength,
                       sizeof( decodedFrameData ) );

    TEST_ASSERT_EQUAL_UINT8_ARRAY( &( decodedFrameData[ 0 ] ),
                                   &( frame.pFrameData[ 0 ] ),
                                   frame.frameDataLength );

    TEST_ASSERT_EQUAL( frame.pictureId,
                       0x7ACD );
    TEST_ASSERT_EQUAL( frame.tl0PicIndex,
                       0xAB );
    TEST_ASSERT_EQUAL( frame.tid,
                       5 );
    TEST_ASSERT_EQUAL( frame.keyIndex,
                       10 );
    TEST_ASSERT_EQUAL( frame.frameProperties,
                       ( VP8_FRAME_PROP_NON_REF_FRAME |
                         VP8_FRAME_PROP_PICTURE_ID_PRESENT |
                         VP8_FRAME_PROP_TL0PICIDX_PRESENT |
                         VP8_FRAME_PROP_TID_PRESENT |
                         VP8_FRAME_PROP_KEYIDX_PRESENT |
                         VP8_FRAME_PROP_DEPENDS_ON_BASE_ONLY ) );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate VP8 depacketization happy path containing picture ID.
 */

void test_VP8_Depacketizer_PictureID( void )
{
    VP8DepacketizerContext_t ctx;
    VP8Result_t result;
    VP8Packet_t pkt;
    VP8Frame_t frame;
    VP8Packet_t packetsArray[ VP8_PACKETS_ARR_LEN ];

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
    TEST_ASSERT_EQUAL( result,
                       VP8_RESULT_OK );

    pkt.pPacketData = &( packetData1[ 0 ] );
    pkt.packetDataLength = sizeof( packetData1 );
    result = VP8Depacketizer_AddPacket( &( ctx ),
                                        &( pkt ) );
    TEST_ASSERT_EQUAL( result,
                       VP8_RESULT_OK );

    pkt.pPacketData = &( packetData2[ 0 ] );
    pkt.packetDataLength = sizeof( packetData2 );
    result = VP8Depacketizer_AddPacket( &( ctx ),
                                        &( pkt ) );
    TEST_ASSERT_EQUAL( result,
                       VP8_RESULT_OK );

    pkt.pPacketData = &( packetData3[ 0 ] );
    pkt.packetDataLength = sizeof( packetData3 );
    result = VP8Depacketizer_AddPacket( &( ctx ),
                                        &( pkt ) );
    TEST_ASSERT_EQUAL( result,
                       VP8_RESULT_OK );

    pkt.pPacketData = &( packetData4[ 0 ] );
    pkt.packetDataLength = sizeof( packetData4 );
    result = VP8Depacketizer_AddPacket( &( ctx ),
                                        &( pkt ) );
    TEST_ASSERT_EQUAL( result,
                       VP8_RESULT_OK );

    frame.pFrameData = &( frameBuffer[ 0 ] );
    frame.frameDataLength = VP8_FRAME_BUF_LEN;
    result = VP8Depacketizer_GetFrame( &( ctx ),
                                       &( frame ) );
    TEST_ASSERT_EQUAL( result,
                       VP8_RESULT_OK );

    TEST_ASSERT_EQUAL( frame.frameDataLength,
                       sizeof( decodedFrameData ) );

    TEST_ASSERT_EQUAL_UINT8_ARRAY( &( decodedFrameData[ 0 ] ),
                                   &( frame.pFrameData[ 0 ] ),
                                   frame.frameDataLength );

    TEST_ASSERT_EQUAL( frame.tl0PicIndex,
                       0xAB );
    TEST_ASSERT_EQUAL( frame.frameProperties,
                       ( VP8_FRAME_PROP_NON_REF_FRAME |
                         VP8_FRAME_PROP_TL0PICIDX_PRESENT ) );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate H264 depacketization happy path to get Nalus.
 */
void test_H264_Depacketizer_GetNalu( void )
{
    H264Result_t result;
    H264Packet_t pkt;
    H264DepacketizerContext_t ctx;
    Nalu_t nalu;
    uint8_t naluBuffer[ MAX_NALU_LENGTH ];
    H264Packet_t packetsArray[ MAX_PACKETS_IN_A_FRAME ];
    uint8_t packetData1[] = { 0x09, 0x10 };
    uint8_t packetData2[] = { 0x67, 0x42, 0xc0, 0x1f, 0xda, 0x01, 0x40, 0x16, 0xec, 0x05, 0xa8, 0x08 };
    uint8_t packetData3[] = { 0x68, 0xce, 0x3c, 0x80 };
    uint8_t packetData4[] = { 0x06, 0x05, 0xff, 0xff, 0xb7, 0xdc, 0x45, 0xe9, 0xbd, 0xe6, 0xd9, 0x48 };
    uint8_t packetData5[] = { 0x7c, 0x85, 0x88, 0x84, 0x12, 0xff, 0xff, 0xfc, 0x3d, 0x14, 0x0, 0x4 };
    uint8_t packetData6[] = { 0x7c, 0x45, 0xba, 0xeb, 0xae, 0xba, 0xeb, 0xae, 0xba, 0xeb, 0xae, 0xba };
    uint8_t packetData7[] = { 0x65, 0x00, 0x6e, 0x22, 0x21, 0x04, 0xbf, 0xff, 0xff, 0x0f, 0x45, 0x00 };

    result = H264Depacketizer_Init( &( ctx ),
                                    &( packetsArray[ 0 ] ),
                                    MAX_PACKETS_IN_A_FRAME );

    TEST_ASSERT_EQUAL( result,
                       H264_RESULT_OK );

    pkt.pPacketData = &( packetData1[ 0 ] );
    pkt.packetDataLength = sizeof( packetData1 );
    result = H264Depacketizer_AddPacket( &( ctx ),
                                         &( pkt ) );
    TEST_ASSERT_EQUAL( result,
                       H264_RESULT_OK );

    pkt.pPacketData = &( packetData2[ 0 ] );
    pkt.packetDataLength = sizeof( packetData2 );
    result = H264Depacketizer_AddPacket( &( ctx ),
                                         &( pkt ) );
    TEST_ASSERT_EQUAL( result,
                       H264_RESULT_OK );

    pkt.pPacketData = &( packetData3[ 0 ] );
    pkt.packetDataLength = sizeof( packetData3 );
    result = H264Depacketizer_AddPacket( &( ctx ),
                                         &( pkt ) );
    TEST_ASSERT_EQUAL( result,
                       H264_RESULT_OK );

    pkt.pPacketData = &( packetData4[ 0 ] );
    pkt.packetDataLength = sizeof( packetData4 );
    result = H264Depacketizer_AddPacket( &( ctx ),
                                         &( pkt ) );
    TEST_ASSERT_EQUAL( result,
                       H264_RESULT_OK );

    pkt.pPacketData = &( packetData5[ 0 ] );
    pkt.packetDataLength = sizeof( packetData5 );
    result = H264Depacketizer_AddPacket( &( ctx ),
                                         &( pkt ) );
    TEST_ASSERT_EQUAL( result,
                       H264_RESULT_OK );

    pkt.pPacketData = &( packetData6[ 0 ] );
    pkt.packetDataLength = sizeof( packetData6 );
    result = H264Depacketizer_AddPacket( &( ctx ),
                                         &( pkt ) );
    TEST_ASSERT_EQUAL( result,
                       H264_RESULT_OK );

    pkt.pPacketData = &( packetData7[ 0 ] );
    pkt.packetDataLength = sizeof( packetData7 );
    result = H264Depacketizer_AddPacket( &( ctx ),
                                         &( pkt ) );
    TEST_ASSERT_EQUAL( result,
                       H264_RESULT_OK );

    nalu.pNaluData = &( naluBuffer[ 0 ] );
    nalu.naluDataLength = MAX_NALU_LENGTH;
    result = H264Depacketizer_GetNalu( &( ctx ),
                                       &( nalu ) );
    TEST_ASSERT_EQUAL( result,
                       H264_RESULT_OK );

    while( result != H264_RESULT_NO_MORE_NALUS )
    {
        nalu.pNaluData = &( naluBuffer[ 0 ] );
        nalu.naluDataLength = MAX_NALU_LENGTH;
        result = H264Depacketizer_GetNalu( &( ctx ),
                                           &( nalu ) );
    }
    TEST_ASSERT_EQUAL( result,
                       H264_RESULT_NO_MORE_NALUS );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate H264 depacketization happy path to get frame.
 */
void test_H264_Depacketizer_GetFrame( void )
{
    H264Result_t result;
    H264Packet_t pkt;
    H264DepacketizerContext_t ctx;
    Frame_t frame;
    H264Packet_t packetsArray[ MAX_PACKETS_IN_A_FRAME ];
    uint8_t packetData1[] = { 0x09, 0x10 };
    uint8_t packetData2[] = { 0x67, 0x42, 0xc0, 0x1f, 0xda, 0x01, 0x40, 0x16, 0xec, 0x05, 0xa8, 0x08 };
    uint8_t packetData3[] = { 0x68, 0xce, 0x3c, 0x80 };
    uint8_t packetData4[] = { 0x06, 0x05, 0xff, 0xff, 0xb7, 0xdc, 0x45, 0xe9, 0xbd, 0xe6, 0xd9, 0x48 };
    uint8_t packetData5[] = { 0x7c, 0x85, 0x88, 0x84, 0x12, 0xff, 0xff, 0xfc, 0x3d, 0x14, 0x0, 0x4 };
    uint8_t packetData6[] = { 0x7c, 0x45, 0xba, 0xeb, 0xae, 0xba, 0xeb, 0xae, 0xba, 0xeb, 0xae, 0xba };
    uint8_t packetData7[] = { 0x65, 0x00, 0x6e, 0x22, 0x21, 0x04, 0xbf, 0xff, 0xff, 0x0f, 0x45, 0x00 };
    uint32_t totalSize = 0;
    result = H264Depacketizer_Init( &( ctx ),
                                    &( packetsArray[ 0 ] ),
                                    MAX_PACKETS_IN_A_FRAME );
    TEST_ASSERT_EQUAL( result,
                       H264_RESULT_OK );

    pkt.pPacketData = &( packetData1[ 0 ] );
    pkt.packetDataLength = sizeof( packetData1 );
    totalSize += pkt.packetDataLength;
    result = H264Depacketizer_AddPacket( &( ctx ),
                                         &( pkt ) );
    TEST_ASSERT_EQUAL( result,
                       H264_RESULT_OK );

    pkt.pPacketData = &( packetData2[ 0 ] );
    pkt.packetDataLength = sizeof( packetData2 );
    totalSize += pkt.packetDataLength;
    result = H264Depacketizer_AddPacket( &( ctx ),
                                         &( pkt ) );
    TEST_ASSERT_EQUAL( result,
                       H264_RESULT_OK );

    pkt.pPacketData = &( packetData3[ 0 ] );
    pkt.packetDataLength = sizeof( packetData3 );
    totalSize += pkt.packetDataLength;
    result = H264Depacketizer_AddPacket( &( ctx ),
                                         &( pkt ) );
    TEST_ASSERT_EQUAL( result,
                       H264_RESULT_OK );

    pkt.pPacketData = &( packetData4[ 0 ] );
    pkt.packetDataLength = sizeof( packetData4 );
    totalSize += pkt.packetDataLength;
    result = H264Depacketizer_AddPacket( &( ctx ),
                                         &( pkt ) );
    TEST_ASSERT_EQUAL( result,
                       H264_RESULT_OK );

    pkt.pPacketData = &( packetData5[ 0 ] );
    pkt.packetDataLength = sizeof( packetData5 );
    totalSize += pkt.packetDataLength;
    result = H264Depacketizer_AddPacket( &( ctx ),
                                         &( pkt ) );
    TEST_ASSERT_EQUAL( result,
                       H264_RESULT_OK );

    pkt.pPacketData = &( packetData6[ 0 ] );
    pkt.packetDataLength = sizeof( packetData6 );
    totalSize += pkt.packetDataLength;
    result = H264Depacketizer_AddPacket( &( ctx ),
                                         &( pkt ) );
    TEST_ASSERT_EQUAL( result,
                       H264_RESULT_OK );

    pkt.pPacketData = &( packetData7[ 0 ] );
    pkt.packetDataLength = sizeof( packetData7 );
    totalSize += pkt.packetDataLength;
    result = H264Depacketizer_AddPacket( &( ctx ),
                                         &( pkt ) );
    TEST_ASSERT_EQUAL( result,
                       H264_RESULT_OK );

    frame.pFrameData = &( frameBuffer[ 0 ] );
    frame.frameDataLength = MAX_FRAME_LENGTH;
    result = H264Depacketizer_GetFrame( &( ctx ),
                                        &( frame ) );
    TEST_ASSERT_EQUAL( result,
                       H264_RESULT_OK );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate H264 depacketization happy path to get properties.
 */
void test_H264_Depacketizer_GetProperties( void )
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
    TEST_ASSERT_EQUAL( result,
                       H264_RESULT_OK );
    TEST_ASSERT_EQUAL( packetProperties,
                       H264_PACKET_PROPERTY_START_PACKET );

    pkt.pPacketData = &( fuAStartPacket[ 0 ] );
    pkt.packetDataLength = sizeof( fuAStartPacket );
    result = H264Depacketizer_GetPacketProperties( pkt.pPacketData,
                                                   pkt.packetDataLength,
                                                   &( packetProperties ) );
    TEST_ASSERT_EQUAL( result,
                       H264_RESULT_OK );
    TEST_ASSERT_EQUAL( packetProperties,
                       H264_PACKET_PROPERTY_START_PACKET );

    pkt.pPacketData = &( fuAEndPacket[ 0 ] );
    pkt.packetDataLength = sizeof( fuAEndPacket );
    result = H264Depacketizer_GetPacketProperties( pkt.pPacketData,
                                                   pkt.packetDataLength,
                                                   &( packetProperties ) );
    TEST_ASSERT_EQUAL( result,
                       H264_RESULT_OK );
    TEST_ASSERT_EQUAL( packetProperties,
                       H264_PACKET_PROPERTY_END_PACKET );
}

/*-----------------------------------------------------------*/
