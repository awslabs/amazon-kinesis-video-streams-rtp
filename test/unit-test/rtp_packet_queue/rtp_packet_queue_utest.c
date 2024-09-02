/* Unity includes. */
#include "unity.h"
#include "catch_assert.h"

/* Standard includes. */
#include <string.h>
#include <stdint.h>

/* API includes. */
#include "rtp_pkt_queue.h"

/* ===========================  EXTERN VARIABLES  =========================== */

#define MAX_IN_FLIGHT_PKTS 128

RtpPacketInfo_t rtpPacketInfoArray[ MAX_IN_FLIGHT_PKTS ];
RtpPacketQueue_t rtpPacketQueue;

void setUp( void )
{
    memset( &( rtpPacketQueue ),
            0,
            sizeof( rtpPacketQueue ) );
    memset( &( rtpPacketInfoArray[ 0 ] ),
            0,
            sizeof( sizeof( RtpPacketInfo_t ) * MAX_IN_FLIGHT_PKTS ) );
}

void tearDown( void )
{
}

/* ==============================  Test Cases  ============================== */

/**
 * @brief Validate RtpPacketQueue_Init functionality.
 */
void test_RtpPacketQueue_Init( void )
{
    RtpPacketQueueResult_t result;

    result = RtpPacketQueue_Init( &( rtpPacketQueue ),
                                  &( rtpPacketInfoArray[ 0 ] ),
                                  MAX_IN_FLIGHT_PKTS );

    TEST_ASSERT_EQUAL( RTP_PACKET_QUEUE_RESULT_OK, result );
    TEST_ASSERT_EQUAL( &( rtpPacketInfoArray[ 0 ] ), rtpPacketQueue.pRtpPacketInfoArray );
    TEST_ASSERT_EQUAL( MAX_IN_FLIGHT_PKTS, rtpPacketQueue.rtpPacketInfoArrayLength );
    TEST_ASSERT_EQUAL( 0, rtpPacketQueue.readIndex );
    TEST_ASSERT_EQUAL( 0, rtpPacketQueue.writeIndex );
    TEST_ASSERT_EQUAL( 0, rtpPacketQueue.packetCount );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate RtpPacketQueue_Init functionality in case of bad parameters.
 */
void test_RtpPacketQueue_Init_BadParams( void )
{
    RtpPacketQueueResult_t result;

    result = RtpPacketQueue_Init( NULL,
                                  &( rtpPacketInfoArray[ 0 ] ),
                                  MAX_IN_FLIGHT_PKTS );

    TEST_ASSERT_EQUAL( RTP_PACKET_QUEUE_RESULT_BAD_PARAM, result );

    result = RtpPacketQueue_Init( &( rtpPacketQueue ),
                                  NULL,
                                  MAX_IN_FLIGHT_PKTS );

    TEST_ASSERT_EQUAL( RTP_PACKET_QUEUE_RESULT_BAD_PARAM, result );

    result = RtpPacketQueue_Init( &( rtpPacketQueue ),
                                  &( rtpPacketInfoArray[ 0 ] ),
                                  1 );

    TEST_ASSERT_EQUAL( RTP_PACKET_QUEUE_RESULT_BAD_PARAM, result );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate Enqueue functionality in case of bad parameters.
 */
void test_RtpPacketQueue_Enqueue_BadParams( void )
{
    RtpPacketQueueResult_t result;
    RtpPacketInfo_t rtpPacketInfo = { 0 };

    result = RtpPacketQueue_Enqueue( NULL, &( rtpPacketInfo ) );
    TEST_ASSERT_EQUAL( RTP_PACKET_QUEUE_RESULT_BAD_PARAM, result );

    result = RtpPacketQueue_Enqueue( &( rtpPacketQueue ), NULL );
    TEST_ASSERT_EQUAL( RTP_PACKET_QUEUE_RESULT_BAD_PARAM, result );
}

/*-----------------------------------------------------------*/

/**
 * @brief Add packets in the RTP packet Queue.
 */
void test_RtpPacketQueue_ForceEnqueue( void )
{
    uint16_t i;
    RtpPacketQueueResult_t result;
    RtpPacketInfo_t deletedRtpPacketInfo, rtpPacketInfo;
    uint8_t expectedSerializedPacket[] = { 0x80, 0x66, 0xAB, 0x12,
                                           0x12, 0x34, 0x43, 0x21,
                                           0xAB, 0xCD, 0xDB, 0xCA };

    result = RtpPacketQueue_Init( &( rtpPacketQueue ),
                                  &( rtpPacketInfoArray[ 0 ] ),
                                  MAX_IN_FLIGHT_PKTS );

    TEST_ASSERT_EQUAL( RTP_PACKET_QUEUE_RESULT_OK, result );

    for( i = 0; i < 20; i++ )
    {
        rtpPacketInfo.seqNum = i;
        rtpPacketInfo.pSerializedRtpPacket = &( expectedSerializedPacket[ 0 ] );
        rtpPacketInfo.serializedPacketLength = sizeof( expectedSerializedPacket );

        result = RtpPacketQueue_ForceEnqueue( &( rtpPacketQueue ),
                                              &( rtpPacketInfo ),
                                              &( deletedRtpPacketInfo ) );

        TEST_ASSERT_EQUAL( RTP_PACKET_QUEUE_RESULT_OK, result );
    }

    for( i = 0; i < 20; i++ )
    {
        result = RtpPacketQueue_Dequeue( &( rtpPacketQueue ),
                                         &( rtpPacketInfo ) );

        TEST_ASSERT_EQUAL( RTP_PACKET_QUEUE_RESULT_OK, result );
        TEST_ASSERT_EQUAL( i, rtpPacketInfo.seqNum );
        TEST_ASSERT_EQUAL( &( expectedSerializedPacket[ 0 ] ),
                           rtpPacketInfo.pSerializedRtpPacket );
        TEST_ASSERT_EQUAL( sizeof( expectedSerializedPacket ),
                           rtpPacketInfo.serializedPacketLength );
    }
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate Queue full functionality.
 */
void test_RtpPacketQueue_ForceEnqueue_Full( void )
{
    uint16_t i;
    RtpPacketQueueResult_t result;
    RtpPacketInfo_t deletedRtpPacketInfo, rtpPacketInfo;
    uint8_t expectedSerializedPacket[] = { 0x80, 0x66, 0xAB, 0x12,
                                           0x12, 0x34, 0x43, 0x21,
                                           0xAB, 0xCD, 0xDB, 0xCA };

    result = RtpPacketQueue_Init( &( rtpPacketQueue ),
                                  &( rtpPacketInfoArray[ 0 ] ),
                                  4 );

    TEST_ASSERT_EQUAL( RTP_PACKET_QUEUE_RESULT_OK, result );

    for( i = 0; i < 4; i++ )
    {
        rtpPacketInfo.seqNum = i;
        rtpPacketInfo.pSerializedRtpPacket = expectedSerializedPacket;
        rtpPacketInfo.serializedPacketLength = sizeof( expectedSerializedPacket );

        result = RtpPacketQueue_ForceEnqueue( &( rtpPacketQueue ),
                                              &( rtpPacketInfo ),
                                              &( deletedRtpPacketInfo ) );

        TEST_ASSERT_EQUAL( RTP_PACKET_QUEUE_RESULT_OK, result );
    }

    result = RtpPacketQueue_ForceEnqueue( &( rtpPacketQueue ),
                                          &( rtpPacketInfo ),
                                          &( deletedRtpPacketInfo ) );

    TEST_ASSERT_EQUAL( RTP_PACKET_QUEUE_RESULT_PACKET_DELETED, result );
    TEST_ASSERT_EQUAL( 0, deletedRtpPacketInfo.seqNum ); /* First packet deleted. */

    result = RtpPacketQueue_ForceEnqueue( &( rtpPacketQueue ),
                                          &( rtpPacketInfo ),
                                          &( deletedRtpPacketInfo ) );

    TEST_ASSERT_EQUAL( RTP_PACKET_QUEUE_RESULT_PACKET_DELETED, result );
    TEST_ASSERT_EQUAL( 1, deletedRtpPacketInfo.seqNum ); /* Second packet deleted. */
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate Forced Enqueue in case of bad parameters.
 */
void test_RtpPacketQueue_ForceEnqueue_BadParams( void )
{
    RtpPacketQueueResult_t result;
    RtpPacketInfo_t deletedRtpPacketInfo = { 0 };
    RtpPacketInfo_t rtpPacketInfo = { 0 };

    result = RtpPacketQueue_ForceEnqueue( NULL, &( rtpPacketInfo ), &( deletedRtpPacketInfo ) );
    TEST_ASSERT_EQUAL(RTP_PACKET_QUEUE_RESULT_BAD_PARAM, result);

    result = RtpPacketQueue_ForceEnqueue( &( rtpPacketQueue ), NULL, &( deletedRtpPacketInfo ) );
    TEST_ASSERT_EQUAL (RTP_PACKET_QUEUE_RESULT_BAD_PARAM, result );

    result = RtpPacketQueue_ForceEnqueue( &( rtpPacketQueue ), &( rtpPacketInfo ), NULL );
    TEST_ASSERT_EQUAL( RTP_PACKET_QUEUE_RESULT_BAD_PARAM, result );
}
/*-----------------------------------------------------------*/

/**
 * @brief Validate dequeue when queue is empty.
 */
void test_RtpPacketQueue_Dequeue_EmptyQueue( void )
{
    RtpPacketQueueResult_t result;
    RtpPacketInfo_t rtpPacketInfo;

    result = RtpPacketQueue_Init( &( rtpPacketQueue ),
                                  &( rtpPacketInfoArray[ 0 ] ),
                                  MAX_IN_FLIGHT_PKTS );

    TEST_ASSERT_EQUAL( RTP_PACKET_QUEUE_RESULT_OK, result );

    result = RtpPacketQueue_Dequeue( &( rtpPacketQueue ),
                                     &( rtpPacketInfo ) );

    TEST_ASSERT_EQUAL( RTP_PACKET_QUEUE_RESULT_EMPTY, result );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate dequeue functionality.
 */
void test_RtpPacketQueue_Dequeue( void )
{
    RtpPacketQueueResult_t result;
    RtpPacketInfo_t deletedRtpPacketInfo, rtpPacketInfo;
    uint8_t expectedSerializedPacket[] = { 0x80, 0x66, 0xAB, 0x12,
                                           0x12, 0x34, 0x43, 0x21,
                                           0xAB, 0xCD, 0xDB, 0xCA };

    result = RtpPacketQueue_Init( &( rtpPacketQueue ),
                                  &( rtpPacketInfoArray[ 0 ] ),
                                  MAX_IN_FLIGHT_PKTS );

    TEST_ASSERT_EQUAL( RTP_PACKET_QUEUE_RESULT_OK, result );

    rtpPacketInfo.seqNum = 10;
    rtpPacketInfo.pSerializedRtpPacket = &( expectedSerializedPacket[ 0 ] );
    rtpPacketInfo.serializedPacketLength = sizeof( expectedSerializedPacket );

    result = RtpPacketQueue_ForceEnqueue( &( rtpPacketQueue ),
                                          &( rtpPacketInfo ),
                                          &( deletedRtpPacketInfo ) );

    TEST_ASSERT_EQUAL( RTP_PACKET_QUEUE_RESULT_OK, result );

    result = RtpPacketQueue_Dequeue( &( rtpPacketQueue ),
                                     &( rtpPacketInfo ) );

    TEST_ASSERT_EQUAL( RTP_PACKET_QUEUE_RESULT_OK, result );
    TEST_ASSERT_EQUAL( 10, rtpPacketInfo.seqNum );
    TEST_ASSERT_EQUAL( sizeof( expectedSerializedPacket ),
                       rtpPacketInfo.serializedPacketLength );
    TEST_ASSERT_EQUAL( &( expectedSerializedPacket[ 0 ] ),
                       rtpPacketInfo.pSerializedRtpPacket );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate peek functionality when the queue is empty.
 */
void test_RtpPacketQueue_Peek_EmptyQueue( void )
{
    RtpPacketQueueResult_t result;
    RtpPacketInfo_t rtpPacketInfo;

    result = RtpPacketQueue_Init( &( rtpPacketQueue ),
                                  &( rtpPacketInfoArray[ 0 ] ),
                                  MAX_IN_FLIGHT_PKTS );

    TEST_ASSERT_EQUAL( RTP_PACKET_QUEUE_RESULT_OK, result );

    result = RtpPacketQueue_Peek( &( rtpPacketQueue ),
                                  &( rtpPacketInfo ) );

    TEST_ASSERT_EQUAL( RTP_PACKET_QUEUE_RESULT_EMPTY, result );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate peek functionality.
 */
void test_RtpPacketQueue_Peek( void )
{
    RtpPacketQueueResult_t result;
    RtpPacketInfo_t deletedRtpPacketInfo, rtpPacketInfo;
    uint8_t expectedSerializedPacket[] = { 0x80, 0x66, 0xAB, 0x12,
                                           0x12, 0x34, 0x43, 0x21,
                                           0xAB, 0xCD, 0xDB, 0xCA };

    result = RtpPacketQueue_Init( &( rtpPacketQueue ),
                                  &( rtpPacketInfoArray[ 0 ] ),
                                  MAX_IN_FLIGHT_PKTS );

    TEST_ASSERT_EQUAL( RTP_PACKET_QUEUE_RESULT_OK, result );

    result = RtpPacketQueue_Dequeue( &( rtpPacketQueue ),
                                     &( rtpPacketInfo ) );

    TEST_ASSERT_EQUAL( RTP_PACKET_QUEUE_RESULT_EMPTY, result );

    rtpPacketInfo.seqNum = 20;
    rtpPacketInfo.pSerializedRtpPacket = &( expectedSerializedPacket[ 0 ] );
    rtpPacketInfo.serializedPacketLength = sizeof( expectedSerializedPacket );

    result = RtpPacketQueue_ForceEnqueue( &( rtpPacketQueue ),
                                          &( rtpPacketInfo ),
                                          &( deletedRtpPacketInfo ) );

    TEST_ASSERT_EQUAL( RTP_PACKET_QUEUE_RESULT_OK, result );

    result = RtpPacketQueue_Peek( &( rtpPacketQueue ),
                                  &( rtpPacketInfo ) );

    TEST_ASSERT_EQUAL( RTP_PACKET_QUEUE_RESULT_OK, result );
    TEST_ASSERT_EQUAL( 20, rtpPacketInfo.seqNum );
    TEST_ASSERT_EQUAL( sizeof( expectedSerializedPacket ),
                       rtpPacketInfo.serializedPacketLength );
    TEST_ASSERT_EQUAL( &( expectedSerializedPacket[ 0 ] ),
                       &( rtpPacketInfo.pSerializedRtpPacket[ 0 ] ) );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate Peek functionality in case of bad parameters.
 */
void test_RtpPacketQueue_Peek_BadParams( void )
{
    RtpPacketQueueResult_t result;
    RtpPacketInfo_t rtpPacketInfo;

    result = RtpPacketQueue_Peek( NULL, &( rtpPacketInfo ) );
    TEST_ASSERT_EQUAL( RTP_PACKET_QUEUE_RESULT_BAD_PARAM, result );

    result = RtpPacketQueue_Peek( &( rtpPacketQueue ), NULL);
    TEST_ASSERT_EQUAL( RTP_PACKET_QUEUE_RESULT_BAD_PARAM, result );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate retrieve functionality when the queue is empty.
 */
void test_RtpPacketQueue_Retrieve_Empty( void )
{
    RtpPacketQueueResult_t result;
    RtpPacketInfo_t retrievedRtpPacketInfo;

    result = RtpPacketQueue_Init( &( rtpPacketQueue ),
                                  &( rtpPacketInfoArray[ 0 ] ),
                                  MAX_IN_FLIGHT_PKTS );

    TEST_ASSERT_EQUAL( RTP_PACKET_QUEUE_RESULT_OK, result );

    result = RtpPacketQueue_Retrieve( &( rtpPacketQueue ),
                                      0,
                                      &( retrievedRtpPacketInfo ) );

    TEST_ASSERT_EQUAL( RTP_PACKET_QUEUE_RESULT_EMPTY, result );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate retrieve functionality when the queue is full.
 */
void test_RtpPacketQueue_Retrieve_Full( void )
{
    uint16_t i;
    RtpPacketQueueResult_t result;
    RtpPacketInfo_t rtpPacketInfo;
    uint8_t expectedSerializedPacket[] = { 0x80, 0x66, 0xAB, 0x12,
                                           0x12, 0x34, 0x43, 0x21,
                                           0xAB, 0xCD, 0xDB, 0xCA };

    result = RtpPacketQueue_Init( &( rtpPacketQueue ),
                                  &( rtpPacketInfoArray[ 0 ] ),
                                  4 );

    TEST_ASSERT_EQUAL( RTP_PACKET_QUEUE_RESULT_OK, result );

    for( i = 0; i < 4; i++ )
    {
        rtpPacketInfo.seqNum = i;
        rtpPacketInfo.pSerializedRtpPacket = &( expectedSerializedPacket[ 0 ] );
        rtpPacketInfo.serializedPacketLength = sizeof( expectedSerializedPacket );

        result = RtpPacketQueue_Enqueue( &( rtpPacketQueue ),
                                         &( rtpPacketInfo ) );

        TEST_ASSERT_EQUAL( RTP_PACKET_QUEUE_RESULT_OK, result );
    }

    rtpPacketInfo.seqNum = 4;
    rtpPacketInfo.pSerializedRtpPacket = &( expectedSerializedPacket[ 0 ] );
    rtpPacketInfo.serializedPacketLength = sizeof( expectedSerializedPacket );

    result = RtpPacketQueue_Enqueue( &( rtpPacketQueue ),
                                     &( rtpPacketInfo ) );

    TEST_ASSERT_EQUAL( RTP_PACKET_QUEUE_RESULT_FULL, result );

    result = RtpPacketQueue_Retrieve( &( rtpPacketQueue ),
                                      3,
                                      &( rtpPacketInfo ) );

    TEST_ASSERT_EQUAL( RTP_PACKET_QUEUE_RESULT_OK, result );
    TEST_ASSERT_EQUAL( 3, rtpPacketInfo.seqNum );
    TEST_ASSERT_EQUAL( &( expectedSerializedPacket[ 0 ] ),
                       rtpPacketInfo.pSerializedRtpPacket );
    TEST_ASSERT_EQUAL( sizeof( expectedSerializedPacket ),
                       rtpPacketInfo.serializedPacketLength );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate retrieve functionality when RTP packet info is not found.
 */
void test_RtpPacketQueue_Retrieve_NotFound( void )
{
    uint16_t i;
    RtpPacketQueueResult_t result;
    RtpPacketInfo_t deletedRtpPacketInfo, rtpPacketInfo;
    uint8_t expectedSerializedPacket[] = { 0x80, 0x66, 0xAB, 0x12,
                                           0x12, 0x34, 0x43, 0x21,
                                           0xAB, 0xCD, 0xDB, 0xCA };

    result = RtpPacketQueue_Init( &( rtpPacketQueue ),
                                  &( rtpPacketInfoArray[ 0 ] ),
                                  MAX_IN_FLIGHT_PKTS );

    TEST_ASSERT_EQUAL( RTP_PACKET_QUEUE_RESULT_OK, result );

    for( i = 0; i < 20; i++ )
    {
        rtpPacketInfo.seqNum = i;
        rtpPacketInfo.pSerializedRtpPacket = &( expectedSerializedPacket[ 0 ] );
        rtpPacketInfo.serializedPacketLength = sizeof( expectedSerializedPacket );

        result = RtpPacketQueue_ForceEnqueue( &( rtpPacketQueue ),
                                              &( rtpPacketInfo ),
                                              &( deletedRtpPacketInfo ) );

        TEST_ASSERT_EQUAL( RTP_PACKET_QUEUE_RESULT_OK, result );
    }

    result = RtpPacketQueue_Retrieve( &( rtpPacketQueue ),
                                      100,
                                      &( rtpPacketInfo ) );

    TEST_ASSERT_EQUAL( RTP_PACKET_QUEUE_RESULT_PACKET_NOT_FOUND, result );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate retrieve functionality when RTP packet info is found.
 */
void test_RtpPacketQueue_Retrieve( void )
{
    uint16_t i;
    RtpPacketQueueResult_t result;
    RtpPacketInfo_t deletedRtpPacketInfo, rtpPacketInfo;
    uint8_t expectedSerializedPacket[] = { 0x80, 0x66, 0xAB, 0x12,
                                           0x12, 0x34, 0x43, 0x21,
                                           0xAB, 0xCD, 0xDB, 0xCA };

    result = RtpPacketQueue_Init( &( rtpPacketQueue ),
                                  &( rtpPacketInfoArray[ 0 ] ),
                                  MAX_IN_FLIGHT_PKTS );

    TEST_ASSERT_EQUAL( RTP_PACKET_QUEUE_RESULT_OK, result );

    for( i = 0; i < 20; i++ )
    {
        rtpPacketInfo.seqNum = i;
        rtpPacketInfo.pSerializedRtpPacket = &( expectedSerializedPacket[ 0 ] );
        rtpPacketInfo.serializedPacketLength = sizeof( expectedSerializedPacket );

        result = RtpPacketQueue_ForceEnqueue( &( rtpPacketQueue ),
                                              &( rtpPacketInfo ),
                                              &( deletedRtpPacketInfo ) );

        TEST_ASSERT_EQUAL( RTP_PACKET_QUEUE_RESULT_OK, result );
    }

    result = RtpPacketQueue_Retrieve( &( rtpPacketQueue ),
                                      8,
                                      &( rtpPacketInfo ) );

    TEST_ASSERT_EQUAL( RTP_PACKET_QUEUE_RESULT_OK, result );
    TEST_ASSERT_EQUAL( 8, rtpPacketInfo.seqNum );
    TEST_ASSERT_EQUAL( &( expectedSerializedPacket[ 0 ] ),
                       rtpPacketInfo.pSerializedRtpPacket );
    TEST_ASSERT_EQUAL( sizeof( expectedSerializedPacket ),
                       rtpPacketInfo.serializedPacketLength );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate retrieve functionality in case of bad parameters.
 */
void test_RtpPacketQueue_Retrieve_BadParams( void )
{
    RtpPacketQueueResult_t result;
    RtpPacketInfo_t rtpPacketInfo;

    result = RtpPacketQueue_Retrieve( NULL, 8, &( rtpPacketInfo ) );
    TEST_ASSERT_EQUAL( RTP_PACKET_QUEUE_RESULT_BAD_PARAM, result );

    result = RtpPacketQueue_Retrieve( &( rtpPacketQueue ), 8, NULL );
    TEST_ASSERT_EQUAL( RTP_PACKET_QUEUE_RESULT_BAD_PARAM, result );
}

/*-----------------------------------------------------------*/

