/* Unity includes. */
#include "unity.h"
#include "catch_assert.h"

/* Standard includes. */
#include <string.h>
#include <stdint.h>

/* API includes. */
#include "h264_depacketizer.h"
#include "h264_packetizer.h"


/* ===========================  EXTERN VARIABLES  =========================== */

#define MAX_PACKETS_IN_A_FRAME  512
#define MAX_NALU_LENGTH         5 * 1024
#define MAX_FRAME_LENGTH        10 * 1024
#define MAX_PACKET_IN_A_FRAME   512

#define MAX_NALUS_IN_A_FRAME        512
#define MAX_H264_PACKET_LENGTH      12

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


/* ==============================  Test Cases for Packetization ============================== */

/**
 * @brief Validate H264 packetization happy path with adding Nalus.
 */
void test_H264_Packetizer_AddNalu( void )
{
    uint8_t pFrame[] = {0x00, 0x00, 0x00, 0x01, 0x09, 0x10,
                        0x00, 0x00, 0x00, 0x01, 0x67, 0x42, 0xc0, 0x1f, 0xda,
                        0x01, 0x40, 0x16, 0xec, 0x05,
                        0xa8, 0x08,
                        0x00, 0x00, 0x00, 0x01, 0x68, 0xce, 0x3c, 0x80,
                        0x00, 0x00, 0x00, 0x01, 0x06, 0x05, 0xff, 0xff, 0xb7,
                        0xdc, 0x45, 0xe9, 0xbd, 0xe6,
                        0xd9, 0x48,
                        0x00, 0x00, 0x00, 0x01, 0x65, 0x88, 0x84, 0x12, 0xff,
                        0xff, 0xfc, 0x3d, 0x14, 0x00,
                        0x04, 0xba, 0xeb, 0xae, 0xba,
                        0xeb, 0xae, 0xba, 0xeb, 0xae,
                        0xba,
                        0x00, 0x00, 0x00, 0x01, 0x65, 0x00, 0x6e, 0x22, 0x21,
                        0x04, 0xbf, 0xff, 0xff, 0x0f,
                        0x45, 0x00};
    size_t frameLength = sizeof( pFrame );
    size_t curIndex = 0, naluStartIndex = 0, remainingLength;
    uint32_t fistStartCode = 1;
    H264PacketizerContext_t ctx = {0};
    H264Result_t result;
    H264Packet_t pkt;
    uint8_t pktBuffer[MAX_H264_PACKET_LENGTH];
    Nalu_t nalusArray[MAX_NALUS_IN_A_FRAME], nalu;
    uint32_t expectedPacketLength[] = {2, 12, 4, 12, 12, 12, 12};

    uint8_t startCode1[] = {0x00, 0x00, 0x00, 0x01};
    uint8_t startCode2[] = {0x00, 0x00, 0x01};

    result = H264Packetizer_Init( &( ctx ),
                                  &( nalusArray[0] ),
                                  MAX_NALUS_IN_A_FRAME );

    TEST_ASSERT_EQUAL( H264_RESULT_OK,
                       result );

    while( curIndex < frameLength )
    {
        remainingLength = frameLength - curIndex;

        /* Check the presence of 4 byte start code. */
        if( remainingLength >= sizeof( startCode1 ) )
        {
            if( memcmp( &( pFrame[curIndex] ),
                        &( startCode1[0] ),
                        sizeof( startCode1 ) ) == 0 )
            {
                if( fistStartCode == 1 )
                {
                    fistStartCode = 0;
                }
                else
                {
                    nalu.pNaluData = &( pFrame[naluStartIndex] );
                    nalu.naluDataLength = curIndex - naluStartIndex;

                    result = H264Packetizer_AddNalu( &( ctx ),
                                                     &( nalu ) );

                    TEST_ASSERT_EQUAL( H264_RESULT_OK,
                                       result );
                }

                naluStartIndex = curIndex + sizeof( startCode1 );
                curIndex = curIndex + sizeof( startCode1 );
                continue;
            }
        }

        /* Check the presence of 3 byte start code. */
        if( remainingLength >= sizeof( startCode2 ) )
        {
            if( memcmp( &( pFrame[curIndex] ),
                        &( startCode2[0] ),
                        sizeof( startCode2 ) ) == 0 )
            {
                if( fistStartCode == 1 )
                {
                    fistStartCode = 0;
                }
                else
                {
                    nalu.pNaluData = &( pFrame[naluStartIndex] );
                    nalu.naluDataLength = curIndex - naluStartIndex;

                    result = H264Packetizer_AddNalu( &( ctx ),
                                                     &( nalu ) );
                }

                naluStartIndex = curIndex + sizeof( startCode2 );
                curIndex = curIndex + sizeof( startCode2 );
                continue;
            }
        }

        curIndex += 1;
    }

    if( naluStartIndex > 0 )
    {
        nalu.pNaluData = &( pFrame[naluStartIndex] );
        nalu.naluDataLength = frameLength - naluStartIndex;

        result = H264Packetizer_AddNalu( &( ctx ),
                                         &( nalu ) );
    }

    pkt.pPacketData = &( pktBuffer[0] );
    pkt.packetDataLength = MAX_H264_PACKET_LENGTH;

    result = H264Packetizer_GetPacket( &( ctx ),
                                       &( pkt ) );

    size_t expectedPacketCount = sizeof( expectedPacketLength ) / sizeof( uint32_t );
    for(size_t i = 0; i < expectedPacketCount; i++)
    {
        TEST_ASSERT_EQUAL( H264_RESULT_OK,
                           result );
        TEST_ASSERT_EQUAL( expectedPacketLength[i],
                           pkt.packetDataLength );

        pkt.pPacketData = &( pktBuffer[0] );
        pkt.packetDataLength = MAX_H264_PACKET_LENGTH;

        result = H264Packetizer_GetPacket( &( ctx ),
                                           &( pkt ) );
    }

    TEST_ASSERT_EQUAL( H264_RESULT_NO_MORE_PACKETS,
                       result );
}
/*-----------------------------------------------------------*/

/**
 * @brief Validate H264 packetization happy path with adding Frame.
 */
void test_H264_Packetizer_AddFrame( void )
{
    uint8_t pFrame[] = { 0x00, 0x00, 0x00, 0x01, 0x09, 0x10,
                         0x00, 0x00, 0x00, 0x01, 0x67, 0x42, 0xc0, 0x1f, 0xda,
                         0x01, 0x40, 0x16, 0xec, 0x05,
                         0xa8, 0x08,
                         0x00, 0x00, 0x00, 0x01, 0x68, 0xce, 0x3c, 0x80,
                         0x00, 0x00, 0x00, 0x01, 0x06, 0x05, 0xff, 0xff, 0xb7,
                         0xdc, 0x45, 0xe9, 0xbd, 0xe6,
                         0xd9, 0x48,
                         0x00, 0x00, 0x00, 0x01, 0x65, 0x88, 0x84, 0x12, 0xff,
                         0xff, 0xfc, 0x3d, 0x14, 0x00,
                         0x04, 0xba, 0xeb, 0xae, 0xba,
                         0xeb, 0xae, 0xba, 0xeb, 0xae,
                         0xba,
                         0x00, 0x00, 0x00, 0x01, 0x65, 0x00, 0x6e, 0x22, 0x21,
                         0x04, 0xbf, 0xff, 0xff, 0x0f,
                         0x45, 0x00 };
    size_t frameLength = sizeof( pFrame );
    H264PacketizerContext_t ctx = { 0 };
    H264Result_t result;
    H264Packet_t pkt;
    Frame_t frame;
    Nalu_t nalusArray[ MAX_NALUS_IN_A_FRAME ];
    uint8_t pktBuffer[ MAX_H264_PACKET_LENGTH ];
    uint32_t expectedPacketLength[] = { 2, 12, 4, 12, 12, 12, 12 };

    result = H264Packetizer_Init( &( ctx ),
                                  &( nalusArray[ 0 ] ),
                                  MAX_NALUS_IN_A_FRAME );

    TEST_ASSERT_EQUAL( H264_RESULT_OK,
                       result );

    frame.pFrameData = pFrame;
    frame.frameDataLength = frameLength;

    result = H264Packetizer_AddFrame( &( ctx ),
                                      &( frame ) );

    TEST_ASSERT_EQUAL( H264_RESULT_OK,
                       result );

    pkt.pPacketData = &( pktBuffer[ 0 ] );
    pkt.packetDataLength = MAX_H264_PACKET_LENGTH;

    result = H264Packetizer_GetPacket( &( ctx ),
                                       &( pkt ) );

    size_t expectedPacketCount = sizeof( expectedPacketLength ) / sizeof( uint32_t );
    for(size_t i = 0; i < expectedPacketCount; i++)
    {
        TEST_ASSERT_EQUAL( H264_RESULT_OK,
                           result );
        TEST_ASSERT_EQUAL( expectedPacketLength[i],
                           pkt.packetDataLength );

        pkt.pPacketData = &( pktBuffer[0] );
        pkt.packetDataLength = MAX_H264_PACKET_LENGTH;

        result = H264Packetizer_GetPacket( &( ctx ),
                                           &( pkt ) );
    }

    TEST_ASSERT_EQUAL( H264_RESULT_NO_MORE_PACKETS,
                       result );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate H264 packetization happy path with adding Frame (3 byte start code).
 */
void test_H264_Packetizer_AddFrame_ThreeByte( void )
{
    uint8_t pFrame[] = { 0x00, 0x00, 0x01, 0x09, 0x10,
                         0x00, 0x00, 0x01, 0x67, 0x42, 0xc0, 0x1f, 0xda,
                         0x01, 0x40, 0x16, 0xec, 0x05,
                         0xa8, 0x08,
                         0x00, 0x00, 0x01, 0x68, 0xce, 0x3c, 0x80,
                         0x00, 0x00, 0x01, 0x06, 0x05, 0xff, 0xff, 0xb7,
                         0xdc, 0x45, 0xe9, 0xbd, 0xe6,
                         0xd9, 0x48,
                         0x00, 0x00, 0x01, 0x65, 0x88, 0x84, 0x12, 0xff,
                         0xff, 0xfc, 0x3d, 0x14, 0x00,
                         0x04, 0xba, 0xeb, 0xae, 0xba,
                         0xeb, 0xae, 0xba, 0xeb, 0xae,
                         0xba,
                         0x00, 0x00, 0x01, 0x65, 0x00, 0x6e, 0x22, 0x21,
                         0x04, 0xbf, 0xff, 0xff, 0x0f,
                         0x45, 0x00 };
    size_t frameLength = sizeof( pFrame );
    H264PacketizerContext_t ctx = { 0 };
    H264Result_t result;
    H264Packet_t pkt;
    Frame_t frame;
    Nalu_t nalusArray[ MAX_NALUS_IN_A_FRAME ];
    uint8_t pktBuffer[ MAX_H264_PACKET_LENGTH ];
    uint32_t expectedPacketLength[] = { 2, 12, 4, 12, 12, 12, 12 };

    result = H264Packetizer_Init( &( ctx ),
                                  &( nalusArray[ 0 ] ),
                                  MAX_NALUS_IN_A_FRAME );

    TEST_ASSERT_EQUAL( H264_RESULT_OK,
                       result );

    frame.pFrameData = pFrame;
    frame.frameDataLength = frameLength;

    result = H264Packetizer_AddFrame( &( ctx ),
                                      &( frame ) );

    TEST_ASSERT_EQUAL( H264_RESULT_OK,
                       result );

    pkt.pPacketData = &( pktBuffer[ 0 ] );
    pkt.packetDataLength = MAX_H264_PACKET_LENGTH;

    result = H264Packetizer_GetPacket( &( ctx ),
                                       &( pkt ) );

    size_t expectedPacketCount = sizeof( expectedPacketLength ) / sizeof( uint32_t );
    for(size_t i = 0; i < expectedPacketCount; i++)
    {
        TEST_ASSERT_EQUAL( H264_RESULT_OK,
                           result );
        TEST_ASSERT_EQUAL( expectedPacketLength[i],
                           pkt.packetDataLength );

        pkt.pPacketData = &( pktBuffer[0] );
        pkt.packetDataLength = MAX_H264_PACKET_LENGTH;

        result = H264Packetizer_GetPacket( &( ctx ),
                                           &( pkt ) );
    }

    TEST_ASSERT_EQUAL( H264_RESULT_NO_MORE_PACKETS,
                       result );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate H264_Packetizer_Init incase of bad parameters.
 */
void test_H264_Packetizer_Init_BadParams( void )
{
    H264PacketizerContext_t ctx = {0};
    H264Result_t result;
    Nalu_t nalusArray[ MAX_NALUS_IN_A_FRAME ];

    result = H264Packetizer_Init( NULL,
                                  &( nalusArray[ 0 ] ),
                                  MAX_NALUS_IN_A_FRAME );

    TEST_ASSERT_EQUAL( H264_RESULT_BAD_PARAM,
                       result );

    result = H264Packetizer_Init( &( ctx ),
                                  NULL,
                                  MAX_NALUS_IN_A_FRAME );

    TEST_ASSERT_EQUAL( H264_RESULT_BAD_PARAM,
                       result );

    ctx.naluArrayLength = 0;

    result = H264Packetizer_Init( &( ctx ),
                                  &( nalusArray[ 0 ] ),
                                  0 );

    TEST_ASSERT_EQUAL( H264_RESULT_BAD_PARAM,
                       result );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate H264_Packetizer_AddFrame incase of bad parameters.
 */
void test_H264_Packetizer_AddFrame_BadParams( void )
{
    uint8_t pFrame[] = { 0x00, 0x00, 0x00, 0x01, 0x09, 0x10,
                         0x00, 0x00, 0x00, 0x01, 0x67, 0x42, 0xc0, 0x1f, 0xda,
                         0x01, 0x40, 0x16, 0xec, 0x05,
                         0xa8, 0x08,
                         0x00, 0x00, 0x00, 0x01, 0x68, 0xce, 0x3c, 0x80,
                         0x00, 0x00, 0x00, 0x01, 0x06, 0x05, 0xff, 0xff, 0xb7,
                         0xdc, 0x45, 0xe9, 0xbd, 0xe6,
                         0xd9, 0x48,
                         0x00, 0x00, 0x00, 0x01, 0x65, 0x88, 0x84, 0x12, 0xff,
                         0xff, 0xfc, 0x3d, 0x14, 0x00,
                         0x04, 0xba, 0xeb, 0xae, 0xba,
                         0xeb, 0xae, 0xba, 0xeb, 0xae,
                         0xba,
                         0x00, 0x00, 0x00, 0x01, 0x65, 0x00, 0x6e, 0x22, 0x21,
                         0x04, 0xbf, 0xff, 0xff, 0x0f,
                         0x45, 0x00 };
    size_t frameLength = sizeof( pFrame );
    H264PacketizerContext_t ctx = {0};
    H264Result_t result;
    Frame_t frame;

    frame.pFrameData = pFrame;
    frame.frameDataLength = frameLength;

    result = H264Packetizer_AddFrame( NULL,
                                      &( frame ) );

    TEST_ASSERT_EQUAL( H264_RESULT_BAD_PARAM,
                       result );

    result = H264Packetizer_AddFrame( &( ctx ),
                                      NULL );

    TEST_ASSERT_EQUAL( H264_RESULT_BAD_PARAM,
                       result );

    frame.pFrameData = NULL;
    frame.frameDataLength = frameLength;

    result = H264Packetizer_AddFrame( &( ctx ),
                                      &( frame ) );

    TEST_ASSERT_EQUAL( H264_RESULT_BAD_PARAM,
                       result );

    frame.pFrameData = pFrame;
    frame.frameDataLength = 0;

    result = H264Packetizer_AddFrame( &( ctx ),
                                      &( frame ) );

    TEST_ASSERT_EQUAL( H264_RESULT_BAD_PARAM,
                       result );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate H264_Packetizer_AddNalu incase of bad parameters.
 */
void test_H264_Packetizer_AddNalu_BadParams( void )
{
    H264PacketizerContext_t ctx = {0};
    H264Result_t result;
    Nalu_t nalusArray[ MAX_NALUS_IN_A_FRAME ];

    result = H264Packetizer_AddNalu( NULL,
                                     &( nalusArray[ 0 ] ) );

    TEST_ASSERT_EQUAL( H264_RESULT_BAD_PARAM,
                       result );

    result = H264Packetizer_AddNalu( &( ctx ),
                                     NULL );

    TEST_ASSERT_EQUAL( H264_RESULT_BAD_PARAM,
                       result );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate H264_Packetizer_AddNalu incase of out of memory.
 */
void test_H264_Packetizer_AddNalu_OutOfMemory( void )
{
    H264PacketizerContext_t ctx = {0};
    H264Result_t result;
    Nalu_t nalusArray[ MAX_NALUS_IN_A_FRAME ];

    ctx.naluArrayLength = 0;

    result = H264Packetizer_AddNalu( &( ctx ),
                                     &( nalusArray[ 0 ] ) );

    TEST_ASSERT_EQUAL( H264_RESULT_OUT_OF_MEMORY,
                       result );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate H264_Packetizer_GetPacket incase of bad parameters.
 */
void test_H264_Packetizer_GetPacket_BadParams( void )
{
    H264PacketizerContext_t ctx = {0};
    H264Result_t result;
    H264Packet_t pkt;
    uint8_t pktBuffer[ MAX_H264_PACKET_LENGTH ];

    pkt.pPacketData = &( pktBuffer[ 0 ] );
    pkt.packetDataLength = MAX_H264_PACKET_LENGTH;

    result = H264Packetizer_GetPacket( NULL,
                                       &( pkt ) );

    TEST_ASSERT_EQUAL( H264_RESULT_BAD_PARAM,
                       result );

    result = H264Packetizer_GetPacket( &( ctx ),
                                       NULL );

    TEST_ASSERT_EQUAL( H264_RESULT_BAD_PARAM,
                       result );
}

/* ==============================  Test Cases for Depacketization ============================== */

/**
 * @brief Validate H264 depacketization happy path to get Nalus.
 */
void test_H264_Depacketizer_GetNalu( void )
{
    H264Result_t result;
    H264Packet_t pkt;
    H264DepacketizerContext_t ctx = { 0 };
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

    TEST_ASSERT_EQUAL( H264_RESULT_OK,
                       result );

    pkt.pPacketData = &( packetData1[ 0 ] );
    pkt.packetDataLength = sizeof( packetData1 );

    result = H264Depacketizer_AddPacket( &( ctx ),
                                         &( pkt ) );

    TEST_ASSERT_EQUAL( H264_RESULT_OK,
                       result );

    pkt.pPacketData = &( packetData2[ 0 ] );
    pkt.packetDataLength = sizeof( packetData2 );

    result = H264Depacketizer_AddPacket( &( ctx ),
                                         &( pkt ) );

    TEST_ASSERT_EQUAL( H264_RESULT_OK,
                       result );

    pkt.pPacketData = &( packetData3[ 0 ] );
    pkt.packetDataLength = sizeof( packetData3 );

    result = H264Depacketizer_AddPacket( &( ctx ),
                                         &( pkt ) );

    TEST_ASSERT_EQUAL( H264_RESULT_OK,
                       result );

    pkt.pPacketData = &( packetData4[ 0 ] );
    pkt.packetDataLength = sizeof( packetData4 );

    result = H264Depacketizer_AddPacket( &( ctx ),
                                         &( pkt ) );

    TEST_ASSERT_EQUAL( H264_RESULT_OK,
                       result );

    pkt.pPacketData = &( packetData5[ 0 ] );
    pkt.packetDataLength = sizeof( packetData5 );

    result = H264Depacketizer_AddPacket( &( ctx ),
                                         &( pkt ) );

    TEST_ASSERT_EQUAL( H264_RESULT_OK,
                       result );

    pkt.pPacketData = &( packetData6[ 0 ] );
    pkt.packetDataLength = sizeof( packetData6 );

    result = H264Depacketizer_AddPacket( &( ctx ),
                                         &( pkt ) );

    TEST_ASSERT_EQUAL( H264_RESULT_OK,
                       result );

    pkt.pPacketData = &( packetData7[ 0 ] );
    pkt.packetDataLength = sizeof( packetData7 );

    result = H264Depacketizer_AddPacket( &( ctx ),
                                         &( pkt ) );

    TEST_ASSERT_EQUAL( H264_RESULT_OK,
                       result );

    nalu.pNaluData = &( naluBuffer[ 0 ] );
    nalu.naluDataLength = MAX_NALU_LENGTH;

    result = H264Depacketizer_GetNalu( &( ctx ),
                                       &( nalu ) );

    TEST_ASSERT_EQUAL( H264_RESULT_OK,
                       result );

    while( result != H264_RESULT_NO_MORE_NALUS )
    {
        nalu.pNaluData = &( naluBuffer[ 0 ] );
        nalu.naluDataLength = MAX_NALU_LENGTH;

        result = H264Depacketizer_GetNalu( &( ctx ),
                                           &( nalu ) );
    }

    TEST_ASSERT_EQUAL( H264_RESULT_NO_MORE_NALUS,
                       result );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate H264 depacketization happy path to get frame.
 */
void test_H264_Depacketizer_GetFrame( void )
{
    H264Result_t result;
    H264Packet_t pkt;
    H264DepacketizerContext_t ctx = { 0 };
    Frame_t frame;
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

    TEST_ASSERT_EQUAL( H264_RESULT_OK,
                       result );

    pkt.pPacketData = &( packetData1[ 0 ] );
    pkt.packetDataLength = sizeof( packetData1 );

    result = H264Depacketizer_AddPacket( &( ctx ),
                                         &( pkt ) );

    TEST_ASSERT_EQUAL( H264_RESULT_OK,
                       result );

    pkt.pPacketData = &( packetData2[ 0 ] );
    pkt.packetDataLength = sizeof( packetData2 );

    result = H264Depacketizer_AddPacket( &( ctx ),
                                         &( pkt ) );

    TEST_ASSERT_EQUAL( H264_RESULT_OK,
                       result );

    pkt.pPacketData = &( packetData3[ 0 ] );
    pkt.packetDataLength = sizeof( packetData3 );

    result = H264Depacketizer_AddPacket( &( ctx ),
                                         &( pkt ) );

    TEST_ASSERT_EQUAL( H264_RESULT_OK,
                       result );

    pkt.pPacketData = &( packetData4[ 0 ] );
    pkt.packetDataLength = sizeof( packetData4 );

    result = H264Depacketizer_AddPacket( &( ctx ),
                                         &( pkt ) );

    TEST_ASSERT_EQUAL( H264_RESULT_OK,
                       result );

    pkt.pPacketData = &( packetData5[ 0 ] );
    pkt.packetDataLength = sizeof( packetData5 );

    result = H264Depacketizer_AddPacket( &( ctx ),
                                         &( pkt ) );

    TEST_ASSERT_EQUAL( H264_RESULT_OK,
                       result );

    pkt.pPacketData = &( packetData6[ 0 ] );
    pkt.packetDataLength = sizeof( packetData6 );

    result = H264Depacketizer_AddPacket( &( ctx ),
                                         &( pkt ) );

    TEST_ASSERT_EQUAL( H264_RESULT_OK,
                       result );

    pkt.pPacketData = &( packetData7[ 0 ] );
    pkt.packetDataLength = sizeof( packetData7 );

    result = H264Depacketizer_AddPacket( &( ctx ),
                                         &( pkt ) );

    TEST_ASSERT_EQUAL( H264_RESULT_OK,
                       result );

    frame.pFrameData = &( frameBuffer[ 0 ] );
    frame.frameDataLength = MAX_FRAME_LENGTH;

    result = H264Depacketizer_GetFrame( &( ctx ),
                                        &( frame ) );

    TEST_ASSERT_EQUAL( H264_RESULT_OK,
                       result );
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

    TEST_ASSERT_EQUAL( H264_RESULT_OK,
                       result );

    TEST_ASSERT_EQUAL( H264_PACKET_PROPERTY_START_PACKET,
                       packetProperties );

    pkt.pPacketData = &( fuAStartPacket[ 0 ] );
    pkt.packetDataLength = sizeof( fuAStartPacket );

    result = H264Depacketizer_GetPacketProperties( pkt.pPacketData,
                                                   pkt.packetDataLength,
                                                   &( packetProperties ) );

    TEST_ASSERT_EQUAL( H264_RESULT_OK,
                       result );

    TEST_ASSERT_EQUAL( H264_PACKET_PROPERTY_START_PACKET,
                       packetProperties );

    pkt.pPacketData = &( fuAEndPacket[ 0 ] );
    pkt.packetDataLength = sizeof( fuAEndPacket );

    result = H264Depacketizer_GetPacketProperties( pkt.pPacketData,
                                                   pkt.packetDataLength,
                                                   &( packetProperties ) );

    TEST_ASSERT_EQUAL( H264_RESULT_OK,
                       result );

    TEST_ASSERT_EQUAL( H264_PACKET_PROPERTY_END_PACKET,
                       packetProperties );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate H264_Depacketizer_GetProperties incase of bad parameters.
 */
void test_H264_Depacketizer_GetProperties_BadParams( void )
{
    uint8_t singleNaluPacket[] = { 0x09, 0x10 };
    uint8_t fuAStartPacket[] = { 0x7C, 0x89, 0xAB, 0xCD };
    uint8_t fuAEndPacket[] = { 0x7C, 0x49, 0xAB, 0xCD };
    H264Result_t result;
    H264Packet_t pkt;

    pkt.pPacketData = &( singleNaluPacket[ 0 ] );
    pkt.packetDataLength = sizeof( singleNaluPacket );

    result = H264Depacketizer_GetPacketProperties( pkt.pPacketData,
                                                   pkt.packetDataLength,
                                                   NULL );

    TEST_ASSERT_EQUAL( H264_RESULT_BAD_PARAM,
                       result );

    pkt.pPacketData = &( fuAStartPacket[ 0 ] );
    pkt.packetDataLength = sizeof( fuAStartPacket );

    result = H264Depacketizer_GetPacketProperties( pkt.pPacketData,
                                                   pkt.packetDataLength,
                                                   NULL );

    TEST_ASSERT_EQUAL( H264_RESULT_BAD_PARAM,
                       result );

    pkt.pPacketData = &( fuAEndPacket[ 0 ] );
    pkt.packetDataLength = sizeof( fuAEndPacket );

    result = H264Depacketizer_GetPacketProperties( pkt.pPacketData,
                                                   pkt.packetDataLength,
                                                   NULL );

    TEST_ASSERT_EQUAL( H264_RESULT_BAD_PARAM,
                       result );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate H264_Depacketizer_GetProperties incase of unsupported packet type.
 */
void test_H264_Depacketizer_GetProperties_UnsupportedPacket( void )
{
    uint8_t packet[] = {0x00, 0x10}; // Packet type 0x00 is not supported
    H264Result_t result;
    uint32_t packetProperties;

    result = H264Depacketizer_GetPacketProperties( packet,
                                                   sizeof( packet ),
                                                   &packetProperties );

    TEST_ASSERT_EQUAL( H264_RESULT_UNSUPPORTED_PACKET,
                       result );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate H264_Depacketizer_Init incase of bad parameters.
 */
void test_H264_Depacketizer_Init_BadParams( void )
{
    H264Result_t result;
    H264DepacketizerContext_t ctx = { 0 };
    H264Packet_t packetsArray[ MAX_PACKETS_IN_A_FRAME ];

    result = H264Depacketizer_Init( NULL,
                                    &( packetsArray[ 0 ] ),
                                    MAX_PACKETS_IN_A_FRAME );

    TEST_ASSERT_EQUAL( H264_RESULT_BAD_PARAM,
                       result );

    result = H264Depacketizer_Init( &( ctx ),
                                    NULL,
                                    MAX_PACKETS_IN_A_FRAME );

    TEST_ASSERT_EQUAL( H264_RESULT_BAD_PARAM,
                       result );

    ctx.packetsArrayLength = 0;

    result = H264Depacketizer_Init( &( ctx ),
                                    &( packetsArray[ 0 ] ),
                                    0 );

    TEST_ASSERT_EQUAL( H264_RESULT_BAD_PARAM,
                       result );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate H264_Depacketizer_AddPacket incase of bad parameters.
 */
void test_H264_Depacketizer_AddPacket_BadParams( void )
{
    H264Result_t result;
    H264Packet_t pkt;
    H264DepacketizerContext_t ctx = { 0 };
    uint8_t packetData1[] = { 0x09, 0x10 };

    pkt.pPacketData = &( packetData1[ 0 ] );
    pkt.packetDataLength = sizeof( packetData1 );

    result = H264Depacketizer_AddPacket( NULL,
                                         &( pkt ) );

    TEST_ASSERT_EQUAL( H264_RESULT_BAD_PARAM,
                       result );

    result = H264Depacketizer_AddPacket( &( ctx ),
                                         NULL );

    TEST_ASSERT_EQUAL( H264_RESULT_BAD_PARAM,
                       result );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate H264_Depacketizer_AddPacket incase of out of memory.
 */
void test_H264_Depacketizer_AddPacket_OutOfMemory( void )
{
    H264Result_t result;
    H264Packet_t pkt;
    H264DepacketizerContext_t ctx = { 0 };
    uint8_t packetData1[] = { 0x09, 0x10 };

    pkt.pPacketData = &( packetData1[ 0 ] );
    pkt.packetDataLength = sizeof( packetData1 );
    ctx.packetsArrayLength = 0;

    result = H264Depacketizer_AddPacket( &( ctx ),
                                         &( pkt ) );

    TEST_ASSERT_EQUAL( H264_RESULT_OUT_OF_MEMORY,
                       result );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate H264_Depacketizer_GetNalu incase of bad parameters.
 */
void test_H264_Depacketizer_GetNalu_BadParams( void )
{
    H264Result_t result;
    H264DepacketizerContext_t ctx = { 0 };
    Nalu_t nalu;
    uint8_t naluBuffer[ MAX_NALU_LENGTH ];

    nalu.pNaluData = &( naluBuffer[ 0 ] );
    nalu.naluDataLength = MAX_NALU_LENGTH;

    result = H264Depacketizer_GetNalu( NULL,
                                       &( nalu ) );

    TEST_ASSERT_EQUAL( H264_RESULT_BAD_PARAM,
                       result );

    result = H264Depacketizer_GetNalu( &( ctx ),
                                       NULL );

    TEST_ASSERT_EQUAL( H264_RESULT_BAD_PARAM,
                       result );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate H264_Depacketizer_GetNalu incase of unsupported packet type.
 */
void test_H264_Depacketizer_GetNalu_UnsupportedPacket( void )
{
    H264Result_t result;
    H264DepacketizerContext_t ctx = { 0 };
    Nalu_t nalu;
    uint8_t naluBuffer[MAX_NALU_LENGTH];
    H264Packet_t packetData;

    nalu.pNaluData = &( naluBuffer[ 0 ] );
    nalu.naluDataLength = MAX_NALU_LENGTH;

    packetData.pPacketData = ( uint8_t[] ) {0x00, 0x10}; // Unsupported packet type
    packetData.packetDataLength = sizeof( packetData.pPacketData );
    ctx.pPacketsArray = &packetData;
    ctx.packetCount = 1;
    ctx.tailIndex = 0;

    result = H264Depacketizer_GetNalu( &ctx,
                                       &nalu );

    TEST_ASSERT_EQUAL( H264_RESULT_UNSUPPORTED_PACKET,
                       result );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate H264_Depacketizer_GetNalu incase of payload out of memory.
 */
void test_H264_Depacketizer_GetNalu_PayloadOutOfMemory( void )
{
    H264Result_t result;
    H264DepacketizerContext_t ctx = { 0 };
    Nalu_t nalu;
    uint8_t naluBuffer[MAX_NALU_LENGTH];
    H264Packet_t packetData;

    nalu.pNaluData = &( naluBuffer[ 0 ] );
    nalu.naluDataLength = 0;

    packetData.pPacketData = ( uint8_t[] ) {0x1C, 0x00};
    packetData.packetDataLength = sizeof( packetData.pPacketData );
    ctx.pPacketsArray = &packetData;
    ctx.packetCount = 1;
    ctx.tailIndex = 0;

    result = H264Depacketizer_GetNalu( &ctx,
                                       &nalu );

    TEST_ASSERT_EQUAL( H264_RESULT_OUT_OF_MEMORY,
                       result );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate H264_Depacketizer_GetNalu incase of header out of memory.
 */
void test_H264_Depacketizer_GetNalu_HeaderOutOfMemory( void )
{
    H264Result_t result;
    H264DepacketizerContext_t ctx = { 0 };
    Nalu_t nalu;
    uint8_t naluBuffer[MAX_NALU_LENGTH];
    H264Packet_t packetData;

    nalu.pNaluData = &( naluBuffer[ 0 ] );
    nalu.naluDataLength = 0;

    packetData.pPacketData = ( uint8_t[] ) {0x1C, 0x80};
    packetData.packetDataLength = sizeof( packetData.pPacketData );
    ctx.pPacketsArray = &packetData;
    ctx.packetCount = 1;
    ctx.tailIndex = 0;

    result = H264Depacketizer_GetNalu( &ctx,
                                       &nalu );

    TEST_ASSERT_EQUAL( H264_RESULT_OUT_OF_MEMORY,
                       result );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate H264_Depacketizer_GetFrame in case of bad parameters.
 */
void test_H264_Depacketizer_GetFrame_BadParams( void )
{
    H264Result_t result;
    H264DepacketizerContext_t ctx = { 0 };
    Frame_t frame;

    frame.pFrameData = &( frameBuffer[ 0 ] );
    frame.frameDataLength = MAX_FRAME_LENGTH;

    result = H264Depacketizer_GetFrame( NULL,
                                        &( frame ) );

    TEST_ASSERT_EQUAL( H264_RESULT_BAD_PARAM,
                       result );

    result = H264Depacketizer_GetFrame( &( ctx ),
                                        NULL );

    TEST_ASSERT_EQUAL( H264_RESULT_BAD_PARAM,
                       result );

    frame.pFrameData = NULL;
    frame.frameDataLength = MAX_FRAME_LENGTH;

    result = H264Depacketizer_GetFrame( &( ctx ),
                                        &( frame ) );

    TEST_ASSERT_EQUAL( H264_RESULT_BAD_PARAM,
                       result );

    frame.pFrameData = &( frameBuffer[ 0 ] );
    frame.frameDataLength = 0;

    result = H264Depacketizer_GetFrame( &( ctx ),
                                        &( frame ) );

    TEST_ASSERT_EQUAL( H264_RESULT_BAD_PARAM,
                       result );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate H264_Depacketizer_GetFrame in case of no more frames.
 */
void test_H264_Depacketizer_GetFrame_NoMoreFrames( void )
{
    H264Result_t result;
    H264DepacketizerContext_t ctx = { 0 };
    Frame_t frame;

    ctx.packetCount = 0;
    frame.pFrameData = &( frameBuffer[ 0 ] );
    frame.frameDataLength = MAX_FRAME_LENGTH;

    result = H264Depacketizer_GetFrame( &( ctx ),
                                        &( frame ) );

    TEST_ASSERT_EQUAL( H264_RESULT_NO_MORE_FRAMES,
                       result );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate H264_Depacketizer_GetFrame in case of out of memory.
 */
void test_H264_Depacketizer_GetFrame_OutOfMemory( void )
{
    H264Result_t result;
    H264Packet_t pkt;
    H264DepacketizerContext_t ctx = { 0 };
    Frame_t frame;
    H264Packet_t packetsArray[ MAX_PACKETS_IN_A_FRAME ];
    uint8_t packetData1[] = { 0x09, 0x10 };
    uint8_t packetData2[] = { 0x67, 0x42, 0xc0, 0x1f, 0xda, 0x01, 0x40, 0x16, 0xec, 0x05, 0xa8, 0x08 };
    uint8_t packetData3[] = { 0x68, 0xce, 0x3c, 0x80 };

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

    frame.pFrameData = &( frameBuffer[ 0 ] );
    frame.frameDataLength = 2;

    result = H264Depacketizer_GetFrame( &( ctx ),
                                        &( frame ) );

    TEST_ASSERT_EQUAL( H264_RESULT_OUT_OF_MEMORY,
                       result );
}

/*-----------------------------------------------------------*/




