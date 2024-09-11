/* Unity includes. */
#include "unity.h"
#include "catch_assert.h"

/* Standard includes. */
#include <string.h>
#include <stdint.h>

/* API includes. */
#include "g711_packetizer.h"
#include "g711_depacketizer.h"


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


/* ==============================  Test Cases for Packetization ============================== */

/**
 * @brief Validate G711 packetization happy path.
 */
void test_G711_Packetizer( void )
{
    G711Result_t result;
    uint8_t frameData[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
                            0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
                            0x20, 0x21, 0x22, 0x23, 0x24, 0x25,
                            0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
                            0x40, 0x41 };
    G711PacketizerContext_t ctx = { 0 };
    G711Packet_t pkt;
    G711Frame_t frame;
    size_t curFrameDataIndex = 0;

    frame.pFrameData = &( frameData [ 0 ] );
    frame.frameDataLength = sizeof( frameData );

    result = G711Packetizer_Init( &( ctx ),
                                  &( frame ) );
    TEST_ASSERT_EQUAL( G711_RESULT_OK,
                       result );

    pkt.pPacketData = &( packetizationBuffer[ 0 ] );
    pkt.packetDataLength = PACKETIZATION_BUFFER_LENGTH;
    result = G711Packetizer_GetPacket( &( ctx ),
                                       &( pkt ) );

    while( result != G711_RESULT_NO_MORE_PACKETS )
    {
        TEST_ASSERT_TRUE( pkt.packetDataLength <= PACKETIZATION_BUFFER_LENGTH );

        TEST_ASSERT_EQUAL_UINT8_ARRAY( &( frameData[ curFrameDataIndex ] ),
                                       &( pkt.pPacketData[ 0 ] ),
                                       pkt.packetDataLength );
        curFrameDataIndex += pkt.packetDataLength;

        pkt.pPacketData = &( packetizationBuffer[ 0 ] );
        pkt.packetDataLength = PACKETIZATION_BUFFER_LENGTH;
        result = G711Packetizer_GetPacket( &( ctx ),
                                           &( pkt ) );
    }

    TEST_ASSERT_EQUAL( sizeof( frameData ),
                       curFrameDataIndex );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate G711_Packetizer_Init in case of bad parameters.
 */
void test_G711_Packetizer_Init_BadParams( void )
{
    G711Result_t result;
    uint8_t frameData[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
                            0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
                            0x20, 0x21, 0x22, 0x23, 0x24, 0x25,
                            0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
                            0x40, 0x41 };
    G711PacketizerContext_t ctx = { 0 };
    G711Frame_t frame;

    frame.pFrameData = &( frameData [ 0 ] );
    frame.frameDataLength = sizeof( frameData );

    result = G711Packetizer_Init( NULL,
                                  &( frame ) );
    TEST_ASSERT_EQUAL( G711_RESULT_BAD_PARAM,
                       result );

    result = G711Packetizer_Init( &( ctx ),
                                  NULL );
    TEST_ASSERT_EQUAL( G711_RESULT_BAD_PARAM,
                       result );

    frame.pFrameData = NULL;
    frame.frameDataLength = sizeof( frameData );

    result = G711Packetizer_Init( &( ctx ),
                                  &( frame ) );
    TEST_ASSERT_EQUAL( G711_RESULT_BAD_PARAM,
                       result );


    frame.pFrameData = &( frameData [ 0 ] );
    frame.frameDataLength = 0;

    result = G711Packetizer_Init( &( ctx ),
                                  &( frame ) );
    TEST_ASSERT_EQUAL( G711_RESULT_BAD_PARAM,
                       result );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate G711_Packetizer_GetPacket in case of bad parameters.
 */
void test_G711_Packetizer_GetPacket_BadParams( void )
{
    G711Result_t result;
    G711PacketizerContext_t ctx = { 0 };
    G711Packet_t pkt;

    pkt.pPacketData = &( packetizationBuffer[ 0 ] );
    pkt.packetDataLength = PACKETIZATION_BUFFER_LENGTH;
    result = G711Packetizer_GetPacket( NULL,
                                       &( pkt ) );
    TEST_ASSERT_EQUAL( G711_RESULT_BAD_PARAM,
                       result );

    result = G711Packetizer_GetPacket( &( ctx ),
                                       NULL );
    TEST_ASSERT_EQUAL( G711_RESULT_BAD_PARAM,
                       result );

    pkt.pPacketData = &( packetizationBuffer[ 0 ] );
    pkt.packetDataLength = 0;
    result = G711Packetizer_GetPacket( &( ctx ),
                                       &( pkt ) );
    TEST_ASSERT_EQUAL( G711_RESULT_BAD_PARAM,
                       result );
}

/* ==============================  Test Cases for Depacketization ============================== */

/**
 * @brief Validate G711 depacketization happy path.
 */
void test_G711_Depacketizer( void )
{
    G711Result_t result;
    G711DepacketizerContext_t ctx = { 0 };
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
    TEST_ASSERT_EQUAL( G711_RESULT_OK,
                       result );

    pkt.pPacketData = &( packetData1[ 0 ] );
    pkt.packetDataLength = sizeof( packetData1 );
    result = G711Depacketizer_AddPacket( &( ctx ),
                                         &( pkt ) );
    TEST_ASSERT_EQUAL( G711_RESULT_OK,
                       result );

    pkt.pPacketData = &( packetData2[ 0 ] );
    pkt.packetDataLength = sizeof( packetData2 );
    result = G711Depacketizer_AddPacket( &( ctx ),
                                         &( pkt ) );
    TEST_ASSERT_EQUAL( G711_RESULT_OK,
                       result );

    pkt.pPacketData = &( packetData3[ 0 ] );
    pkt.packetDataLength = sizeof( packetData3 );
    result = G711Depacketizer_AddPacket( &( ctx ),
                                         &( pkt ) );
    TEST_ASSERT_EQUAL( G711_RESULT_OK,
                       result );

    frame.pFrameData = &( frameBuffer[ 0 ] );
    frame.frameDataLength = MAX_FRAME_LENGTH;
    result = G711Depacketizer_GetFrame( &( ctx ),
                                        &( frame ) );
    TEST_ASSERT_EQUAL( G711_RESULT_OK,
                       result );
    TEST_ASSERT_EQUAL( sizeof( decodedFrame ),
                       frame.frameDataLength );
    TEST_ASSERT_EQUAL_UINT8_ARRAY( &( decodedFrame[ 0 ] ),
                                   &( frame.pFrameData[ 0 ] ),
                                   frame.frameDataLength );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate G711_Depacketizer_GetPacketProperties happy path.
 **/
void test_G711_Depacketizer_GetPacketProperties( void )
{
    G711Result_t result;
    G711Packet_t pkt;
    uint32_t properties;

    uint8_t packetData[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x10, 0x11, 0x12, 0x13 };

    pkt.pPacketData = &( packetData[ 0 ] );
    pkt.packetDataLength = sizeof( packetData );
    result = G711Depacketizer_GetPacketProperties( pkt.pPacketData,
                                                   pkt.packetDataLength,
                                                   &( properties ) );
    TEST_ASSERT_EQUAL( G711_RESULT_OK,
                       result );
    TEST_ASSERT_EQUAL( G711_PACKET_PROPERTY_START_PACKET,
                       properties );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate G711Depacketizer_Init in case of bad parameters.
 **/
void test_G711_Depacketizer_Init_BadParams( void )
{
    G711Result_t result;
    G711DepacketizerContext_t ctx = { 0 };
    G711Packet_t packetsArray[ MAX_PACKET_IN_A_FRAME ];

    result = G711Depacketizer_Init( NULL,
                                    &( packetsArray[ 0 ] ),
                                    MAX_PACKET_IN_A_FRAME );
    TEST_ASSERT_EQUAL( G711_RESULT_BAD_PARAM,
                       result );

    result = G711Depacketizer_Init( &( ctx ),
                                    NULL,
                                    MAX_PACKET_IN_A_FRAME );
    TEST_ASSERT_EQUAL( G711_RESULT_BAD_PARAM,
                       result );

    result = G711Depacketizer_Init( &( ctx ),
                                    &( packetsArray[ 0 ] ),
                                    0 );
    TEST_ASSERT_EQUAL( G711_RESULT_BAD_PARAM,
                       result );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate G711Depacketizer_AddPacket in case of bad parameters.
 */
void test_G711_Depacketizer_AddPacket_BadParams( void )
{
    G711Result_t result;
    G711DepacketizerContext_t ctx = { 0 };
    G711Packet_t pkt;
    uint8_t packetData[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x10, 0x11, 0x12, 0x13 };

    pkt.pPacketData = &( packetData[ 0 ] );
    pkt.packetDataLength = sizeof( packetData );
    result = G711Depacketizer_AddPacket( NULL,
                                         &( pkt ) );
    TEST_ASSERT_EQUAL( G711_RESULT_BAD_PARAM,
                       result );

    result = G711Depacketizer_AddPacket( &( ctx ),
                                         NULL );
    TEST_ASSERT_EQUAL( G711_RESULT_BAD_PARAM,
                       result );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate G711Depacketizer_AddPacket in case of out of memory.
 **/
void test_G711_Depacketizer_AddPacket_OutOfMemory( void )
{
    G711Result_t result;
    G711DepacketizerContext_t ctx = { 0 };
    G711Packet_t packetsArray[ 1 ], pkt;

    uint8_t packetData1[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x10, 0x11, 0x12, 0x13 };
    uint8_t packetData2[] = { 0x14, 0x15, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x30, 0x31 };

    result = G711Depacketizer_Init( &( ctx ),
                                    &( packetsArray[ 0 ] ),
                                    1 );
    TEST_ASSERT_EQUAL( G711_RESULT_OK,
                       result );

    pkt.pPacketData = &( packetData1[ 0 ] );
    pkt.packetDataLength = sizeof( packetData1 );
    result = G711Depacketizer_AddPacket( &( ctx ),
                                         &( pkt ) );
    TEST_ASSERT_EQUAL( G711_RESULT_OK,
                       result );

    pkt.pPacketData = &( packetData2[ 0 ] );
    pkt.packetDataLength = sizeof( packetData2 );
    result = G711Depacketizer_AddPacket( &( ctx ),
                                         &( pkt ) );
    TEST_ASSERT_EQUAL( G711_RESULT_OUT_OF_MEMORY,
                       result );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate G711Depacketizer_GetFrame in case of bad parameters.
 **/

void test_G711_Depacketizer_GetFrame_BadParams( void )
{
    G711Result_t result;
    G711DepacketizerContext_t ctx = { 0 };
    G711Frame_t frame;

    frame.pFrameData = &( frameBuffer[ 0 ] );
    frame.frameDataLength = MAX_FRAME_LENGTH;
    result = G711Depacketizer_GetFrame( NULL,
                                        &( frame ) );
    TEST_ASSERT_EQUAL( G711_RESULT_BAD_PARAM,
                       result );

    result = G711Depacketizer_GetFrame( &( ctx ),
                                        NULL );
    TEST_ASSERT_EQUAL( G711_RESULT_BAD_PARAM,
                       result );

    frame.pFrameData = NULL;
    frame.frameDataLength = MAX_FRAME_LENGTH;
    result = G711Depacketizer_GetFrame( &( ctx ),
                                        &( frame ) );
    TEST_ASSERT_EQUAL( G711_RESULT_BAD_PARAM,
                       result );

    frame.pFrameData = &( frameBuffer[ 0 ] );
    frame.frameDataLength = 0;
    result = G711Depacketizer_GetFrame( &( ctx ),
                                        &( frame ) );
    TEST_ASSERT_EQUAL( G711_RESULT_BAD_PARAM,
                       result );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate G711_Depacketizer_GetFrame in case of out of memory.
 **/
void test_G711_Depacketizer_GetFrame_OutOfMemory( void )
{
    G711Result_t result;
    G711DepacketizerContext_t ctx = { 0 };
    G711Packet_t packetsArray[ MAX_PACKET_IN_A_FRAME ], pkt;
    G711Frame_t frame;

    uint8_t packetData1[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x10, 0x11, 0x12, 0x13 };
    uint8_t packetData2[] = { 0x14, 0x15, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x30, 0x31 };

    result = G711Depacketizer_Init( &( ctx ),
                                    &( packetsArray[ 0 ] ),
                                    MAX_PACKET_IN_A_FRAME );
    TEST_ASSERT_EQUAL( G711_RESULT_OK,
                       result );

    pkt.pPacketData = &( packetData1[ 0 ] );
    pkt.packetDataLength = sizeof( packetData1 );
    result = G711Depacketizer_AddPacket( &( ctx ),
                                         &( pkt ) );
    TEST_ASSERT_EQUAL( G711_RESULT_OK ,
    result);

    pkt.pPacketData = &( packetData2[ 0 ] );
    pkt.packetDataLength = sizeof( packetData2 );
    result = G711Depacketizer_AddPacket( &( ctx ),
                                         &( pkt ) );
    TEST_ASSERT_EQUAL( G711_RESULT_OK,
                       result );

    frame.pFrameData = &( frameBuffer[ 0 ] );
    frame.frameDataLength = 10; /* A small buffer to test out of memory case. */
    result = G711Depacketizer_GetFrame( &( ctx ),
                                        &( frame ) );
    TEST_ASSERT_EQUAL( G711_RESULT_OUT_OF_MEMORY,
                       result );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate G711Depacketizer_GetPacketProperties in case of bad parameters.
 **/
void test_G711_Depacketizer_GetPacketProperties_BadParams( void )
{
    G711Result_t result;
    G711Packet_t pkt;
    uint32_t properties;
    uint8_t packetData[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x10, 0x11, 0x12, 0x13 };

    pkt.packetDataLength = sizeof( packetData );
    result = G711Depacketizer_GetPacketProperties( NULL,
                                                   pkt.packetDataLength,
                                                   &( properties ) );
    TEST_ASSERT_EQUAL( G711_RESULT_BAD_PARAM,
                       result );

    pkt.pPacketData = &( packetData[ 0 ] );
    result = G711Depacketizer_GetPacketProperties( pkt.pPacketData,
                                                   0,
                                                   &( properties ) );
    TEST_ASSERT_EQUAL( G711_RESULT_BAD_PARAM,
                       result );

    pkt.pPacketData = &( packetData[ 0 ] );
    pkt.packetDataLength = sizeof( packetData );
    result = G711Depacketizer_GetPacketProperties( pkt.pPacketData,
                                                   pkt.packetDataLength,
                                                   NULL );
    TEST_ASSERT_EQUAL( G711_RESULT_BAD_PARAM,
                       result );
}

/*-----------------------------------------------------------*/
