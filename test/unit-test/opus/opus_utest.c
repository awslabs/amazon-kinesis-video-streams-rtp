/* Unity includes. */
#include "unity.h"
#include "catch_assert.h"

/* Standard includes. */
#include <string.h>
#include <stdint.h>

/* API includes. */
#include "opus_packetizer.h"
#include "opus_depacketizer.h"

/* ===========================  EXTERN VARIABLES  =========================== */

#define MAX_FRAME_LENGTH        10 * 1024
#define MAX_PACKET_IN_A_FRAME   512

#define PACKETIZATION_BUFFER_LENGTH 10

uint8_t packetizationBuffer[ PACKETIZATION_BUFFER_LENGTH];
uint8_t frameBuffer[ MAX_FRAME_LENGTH ];

void setUp( void )
{
    memset( &( frameBuffer[ 0 ] ),
            0,
            sizeof( frameBuffer ) );
    memset( &( packetizationBuffer[ 0 ] ),
            0,
            sizeof( packetizationBuffer ) );
}

void tearDown( void )
{
}


/* ==============================  Test Cases  ============================== */

/**
 * @brief Validate Opus packetization happy path.
 */
void test_Opus_Packetizer( void )
{
    OpusResult_t result;
    uint8_t frameData[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
                            0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
                            0x20, 0x21, 0x22, 0x23, 0x24, 0x25,
                            0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
                            0x40, 0x41 };
    OpusPacketizerContext_t ctx = { 0 };
    OpusPacket_t pkt;
    OpusFrame_t frame;
    size_t curFrameDataIndex = 0;

    frame.pFrameData = &( frameData [ 0 ] );
    frame.frameDataLength = sizeof( frameData );

    result = OpusPacketizer_Init( &( ctx ),
                                  &( frame ) );
    TEST_ASSERT_EQUAL( OPUS_RESULT_OK,
                       result );

    pkt.pPacketData = &( packetizationBuffer[ 0 ] );
    pkt.packetDataLength = PACKETIZATION_BUFFER_LENGTH;
    result = OpusPacketizer_GetPacket( &( ctx ),
                                       &( pkt ) );

    while( result != OPUS_RESULT_NO_MORE_PACKETS )
    {
        TEST_ASSERT_TRUE( pkt.packetDataLength <= PACKETIZATION_BUFFER_LENGTH );

        TEST_ASSERT_EQUAL_UINT8_ARRAY( &( frameData[ curFrameDataIndex ] ),
                                       &( pkt.pPacketData[ 0 ] ),
                                       pkt.packetDataLength );
        curFrameDataIndex += pkt.packetDataLength;

        pkt.pPacketData = &( packetizationBuffer[ 0 ] );
        pkt.packetDataLength = PACKETIZATION_BUFFER_LENGTH;
        result = OpusPacketizer_GetPacket( &( ctx ),
                                           &( pkt ) );
    }

    TEST_ASSERT_EQUAL( sizeof( frameData ),
                       curFrameDataIndex );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate Opus_Packetizer_Init in case of bad parameters.
 */
void test_Opus_Packetizer_Init_BadParams( void )
{
    OpusResult_t result;
    uint8_t frameData[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
                            0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
                            0x20, 0x21, 0x22, 0x23, 0x24, 0x25,
                            0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
                            0x40, 0x41 };
    OpusPacketizerContext_t ctx = { 0 };
    OpusFrame_t frame;

    frame.pFrameData = &( frameData [ 0 ] );
    frame.frameDataLength = sizeof( frameData );

    result = OpusPacketizer_Init( NULL,
                                  &( frame ) );
    TEST_ASSERT_EQUAL( OPUS_RESULT_BAD_PARAM,
                       result );

    result = OpusPacketizer_Init( &( ctx ),
                                  NULL );
    TEST_ASSERT_EQUAL( OPUS_RESULT_BAD_PARAM,
                       result );

    frame.pFrameData = NULL;
    frame.frameDataLength = sizeof( frameData );

    result = OpusPacketizer_Init( &( ctx ),
                                  &( frame ) );
    TEST_ASSERT_EQUAL( OPUS_RESULT_BAD_PARAM,
                       result );

    frame.pFrameData = &( frameData [ 0 ] );
    frame.frameDataLength = 0;

    result = OpusPacketizer_Init( &( ctx ),
                                  &( frame ) );
    TEST_ASSERT_EQUAL( OPUS_RESULT_BAD_PARAM,
                       result );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate Opus_Packetizer_GetPacket in case of bad parameters.
 */
void test_Opus_Packetizer_GetPacket_BadParams( void )
{
    OpusResult_t result;
    OpusPacketizerContext_t ctx = { 0 };
    OpusPacket_t pkt;

    pkt.pPacketData = &( packetizationBuffer[ 0 ] );
    pkt.packetDataLength = PACKETIZATION_BUFFER_LENGTH;
    result = OpusPacketizer_GetPacket( NULL,
                                       &( pkt ) );
    TEST_ASSERT_EQUAL( result,
                       OPUS_RESULT_BAD_PARAM );

    result = OpusPacketizer_GetPacket( &( ctx ),
                                       NULL );
    TEST_ASSERT_EQUAL( result,
                       OPUS_RESULT_BAD_PARAM );

    pkt.pPacketData = &( packetizationBuffer[ 0 ] );
    pkt.packetDataLength = 0;
    result = OpusPacketizer_GetPacket( &( ctx ),
                                       &( pkt ) );
    TEST_ASSERT_EQUAL( OPUS_RESULT_BAD_PARAM,
                       result );
}

/* ==============================  Test Cases for Depacketization ============================== */

/**
 * @brief Validate Opus depacketization happy path.
 */
void test_Opus_Depacketizer( void )
{
    OpusResult_t result;
    OpusDepacketizerContext_t ctx = { 0 };
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
    TEST_ASSERT_EQUAL( OPUS_RESULT_OK,
                       result );

    pkt.pPacketData = &( packetData1[ 0 ] );
    pkt.packetDataLength = sizeof( packetData1 );
    result = OpusDepacketizer_AddPacket( &( ctx ),
                                         &( pkt ) );
    TEST_ASSERT_EQUAL( OPUS_RESULT_OK,
                       result );

    pkt.pPacketData = &( packetData2[ 0 ] );
    pkt.packetDataLength = sizeof( packetData2 );
    result = OpusDepacketizer_AddPacket( &( ctx ),
                                         &( pkt ) );
    TEST_ASSERT_EQUAL( OPUS_RESULT_OK,
                       result );

    pkt.pPacketData = &( packetData3[ 0 ] );
    pkt.packetDataLength = sizeof( packetData3 );
    result = OpusDepacketizer_AddPacket( &( ctx ),
                                         &( pkt ) );
    TEST_ASSERT_EQUAL( OPUS_RESULT_OK,
                       result );

    frame.pFrameData = &( frameBuffer[ 0 ] );
    frame.frameDataLength = MAX_FRAME_LENGTH;
    result = OpusDepacketizer_GetFrame( &( ctx ),
                                        &( frame ) );
    TEST_ASSERT_EQUAL( OPUS_RESULT_OK,
                       result );
    TEST_ASSERT_EQUAL( frame.frameDataLength,
                       sizeof( decodedFrame ) );
    TEST_ASSERT_EQUAL_UINT8_ARRAY( &( decodedFrame[ 0 ] ),
                                   &( frame.pFrameData[ 0 ] ),
                                   frame.frameDataLength );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate Opus_Depacketizer_GetPacketProperties happy path.
 */
void test_Opus_Depacketizer_GetPacketProperties( void )
{
    OpusResult_t result;
    OpusPacket_t pkt;
    uint32_t properties;

    uint8_t packetData1[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x10, 0x11, 0x12, 0x13 };

    pkt.pPacketData = &( packetData1[ 0 ] );
    pkt.packetDataLength = sizeof( packetData1 );
    result = OpusDepacketizer_GetPacketProperties( pkt.pPacketData,
                                                   pkt.packetDataLength,
                                                   &properties );
    TEST_ASSERT_EQUAL( OPUS_RESULT_OK,
                       result );
    TEST_ASSERT_EQUAL( OPUS_PACKET_PROPERTY_START_PACKET,
                       properties );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate Opus_Depacketizer_Init in case of bad parameters.
 */
void test_Opus_Depacketizer_Init_BadParams( void )
{
    OpusResult_t result;
    OpusDepacketizerContext_t ctx = { 0 };
    OpusPacket_t packetsArray[ MAX_PACKET_IN_A_FRAME ];

    result = OpusDepacketizer_Init( NULL,
                                    &( packetsArray[ 0 ] ),
                                    MAX_PACKET_IN_A_FRAME );
    TEST_ASSERT_EQUAL( OPUS_RESULT_BAD_PARAM,
                       result );

    result = OpusDepacketizer_Init( &( ctx ),
                                    NULL,
                                    MAX_PACKET_IN_A_FRAME );
    TEST_ASSERT_EQUAL( OPUS_RESULT_BAD_PARAM,
                       result );

    result = OpusDepacketizer_Init( &( ctx ),
                                    &( packetsArray[ 0 ] ),
                                    0 );
    TEST_ASSERT_EQUAL( OPUS_RESULT_BAD_PARAM,
                       result );

}

/*-----------------------------------------------------------*/

/**
 * @brief Validate Opus_Depacketizer_AddPacket in case of bad parameters.
 */
void test_Opus_Depacketizer_AddPacket_BadParams( void )
{
    OpusResult_t result;
    OpusDepacketizerContext_t ctx = { 0 };
    OpusPacket_t pkt;

    uint8_t packetData1[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x10, 0x11, 0x12, 0x13 };

    pkt.pPacketData = &( packetData1[ 0 ] );
    pkt.packetDataLength = sizeof( packetData1 );
    result = OpusDepacketizer_AddPacket( NULL,
                                         &( pkt ) );
    TEST_ASSERT_EQUAL( result,
                       OPUS_RESULT_BAD_PARAM );

    pkt.pPacketData = &( packetData1[ 0 ] );
    pkt.packetDataLength = sizeof( packetData1 );
    result = OpusDepacketizer_AddPacket( &( ctx ),
                                         NULL );
    TEST_ASSERT_EQUAL( result,
                       OPUS_RESULT_BAD_PARAM );


    pkt.pPacketData = &( packetData1[ 0 ] );
    pkt.packetDataLength = 0;
    result = OpusDepacketizer_AddPacket( &( ctx ),
                                         &( pkt ) );
    TEST_ASSERT_EQUAL( result,
                       OPUS_RESULT_BAD_PARAM );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate Opus_Depacketizer_AddPacket in case of out of memory.
 */
void test_Opus_Depacketizer_AddPacket_OutOfMemory( void )
{
    OpusResult_t result;
    OpusDepacketizerContext_t ctx = { 0 };
    OpusPacket_t packetsArray[1], pkt;

    uint8_t packetData1[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x10, 0x11, 0x12, 0x13 };
    uint8_t packetData2[] = { 0x14, 0x15, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x30, 0x31 };

    result = OpusDepacketizer_Init( &( ctx ),
                                    &( packetsArray[ 0 ] ),
                                    1 );
    TEST_ASSERT_EQUAL( OPUS_RESULT_OK,
                       result );

    pkt.pPacketData = &( packetData1[ 0 ] );
    pkt.packetDataLength = sizeof( packetData1 );
    result = OpusDepacketizer_AddPacket( &( ctx ),
                                         &( pkt ) );
    TEST_ASSERT_EQUAL( OPUS_RESULT_OK,
                       result );

    pkt.pPacketData = &( packetData2[ 0 ] );
    pkt.packetDataLength = sizeof( packetData1 );
    result = OpusDepacketizer_AddPacket( &( ctx ),
                                         &( pkt ) );
    TEST_ASSERT_EQUAL( OPUS_RESULT_OUT_OF_MEMORY,
                       result );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate Opus_Depacketizer_GetFrame in case of bad parameters.
 */
void test_Opus_Depacketizer_GetFrame_BadParams( void )
{
    OpusResult_t result;
    OpusDepacketizerContext_t ctx = { 0 };
    OpusFrame_t frame;

    frame.pFrameData = &( frameBuffer[ 0 ] );
    frame.frameDataLength = MAX_FRAME_LENGTH;
    result = OpusDepacketizer_GetFrame( NULL,
                                        &( frame ) );
    TEST_ASSERT_EQUAL( OPUS_RESULT_BAD_PARAM,
                       result );

    result = OpusDepacketizer_GetFrame( &( ctx ),
                                        NULL );
    TEST_ASSERT_EQUAL( OPUS_RESULT_BAD_PARAM,
                       result );

    frame.pFrameData = NULL;
    frame.frameDataLength = MAX_FRAME_LENGTH;
    result = OpusDepacketizer_GetFrame( &( ctx ),
                                        &( frame ) );
    TEST_ASSERT_EQUAL( OPUS_RESULT_BAD_PARAM,
                       result );

    frame.pFrameData = &( frameBuffer[ 0 ] );
    frame.frameDataLength = 0;
    result = OpusDepacketizer_GetFrame( &( ctx ),
                                        &( frame ) );
    TEST_ASSERT_EQUAL( OPUS_RESULT_BAD_PARAM,
                       result );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate Opus_Depacketizer_GetFrame in case of out of memory.
 */
void test_Opus_Depacketizer_GetFrame_OutOfMemory( void )
{
    OpusResult_t result;
    OpusDepacketizerContext_t ctx = { 0 };
    OpusPacket_t packetsArray[ MAX_PACKET_IN_A_FRAME ], pkt;
    OpusFrame_t frame;

    uint8_t packetData1[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x10, 0x11, 0x12, 0x13 };
    uint8_t packetData2[] = { 0x14, 0x15, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x30, 0x31 };

    result = OpusDepacketizer_Init( &( ctx ),
                                    &( packetsArray[ 0 ] ),
                                    MAX_PACKET_IN_A_FRAME );
    TEST_ASSERT_EQUAL( OPUS_RESULT_OK,
                       result );

    pkt.pPacketData = &( packetData1[ 0 ] );
    pkt.packetDataLength = sizeof( packetData1 );
    result = OpusDepacketizer_AddPacket( &( ctx ),
                                         &( pkt ) );
    TEST_ASSERT_EQUAL( OPUS_RESULT_OK,
                       result );

    pkt.pPacketData = &( packetData2[ 0 ] );
    pkt.packetDataLength = sizeof( packetData2 );
    result = OpusDepacketizer_AddPacket( &( ctx ),
                                         &( pkt ) );
    TEST_ASSERT_EQUAL( OPUS_RESULT_OK,
                       result );

    frame.pFrameData = &( frameBuffer[ 0 ] );
    frame.frameDataLength = 10; /* A small buffer to test out of memory case. */
    result = OpusDepacketizer_GetFrame( &( ctx ),
                                        &( frame ) );
    TEST_ASSERT_EQUAL( OPUS_RESULT_OUT_OF_MEMORY,
                       result );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate Opus_Depacketizer_GetFrame in case of no more packets.
 */
void test_Opus_Depacketizer_GetFrame_NoMorePackets( void )
{
    OpusResult_t result;
    OpusDepacketizerContext_t ctx = { 0 };
    OpusPacket_t packetsArray[ MAX_PACKET_IN_A_FRAME ];
    OpusFrame_t frame;

    result = OpusDepacketizer_Init( &( ctx ),
                                    &( packetsArray[ 0 ] ),
                                    MAX_PACKET_IN_A_FRAME );
    TEST_ASSERT_EQUAL( OPUS_RESULT_OK,
                       result );

    // Without adding any packet we try to invoke GetFrame with ctx.packetCount as 0

    frame.pFrameData = &( frameBuffer[ 0 ] );
    frame.frameDataLength = MAX_FRAME_LENGTH;
    result = OpusDepacketizer_GetFrame( &( ctx ),
                                        &( frame ) );
    TEST_ASSERT_EQUAL( OPUS_RESULT_NO_MORE_PACKETS,
                       result );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate Opus_Depacketizer_GetPacketProperties in case of bad parameters.
 */
void test_Opus_Depacketizer_GetPacketProperties_BadParams( void )
{
    OpusResult_t result;
    OpusPacket_t pkt;
    uint32_t properties;
    uint8_t packetData[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x10, 0x11, 0x12, 0x13 };

    pkt.packetDataLength = sizeof( packetData );
    result = OpusDepacketizer_GetPacketProperties( NULL,
                                                   pkt.packetDataLength,
                                                   &properties );
    TEST_ASSERT_EQUAL( OPUS_RESULT_BAD_PARAM,
                       result );

    pkt.pPacketData = &( packetData[ 0 ] );
    result = OpusDepacketizer_GetPacketProperties( pkt.pPacketData,
                                                   0,
                                                   &properties );
    TEST_ASSERT_EQUAL( OPUS_RESULT_BAD_PARAM,
                       result );

    pkt.pPacketData = &( packetData[ 0 ] );
    pkt.packetDataLength = sizeof( packetData );
    result = OpusDepacketizer_GetPacketProperties( pkt.pPacketData,
                                                   pkt.packetDataLength,
                                                   NULL );
    TEST_ASSERT_EQUAL( OPUS_RESULT_BAD_PARAM,
                       result );
}

/*-----------------------------------------------------------*/
