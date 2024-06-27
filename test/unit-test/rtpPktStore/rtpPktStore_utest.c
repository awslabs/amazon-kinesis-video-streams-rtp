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

RtpPacketInfo_t rtpPacketInfoArr[ MAX_IN_FLIGHT_PKTS ];
RtpPacketQueue_t rtpPacketQueue;

void setUp( void )
{
    memset( &( rtpPacketInfoArr[ 0 ] ),
            0,
            sizeof( sizeof( RtpPacketInfo_t ) * MAX_IN_FLIGHT_PKTS ) );
}

void tearDown( void )
{
}

/* ==============================  Test Cases  ============================== */

/**
 * @brief Validate RtpPktQueueInit functionality.
 */
void test_RtpPacketQueue_Init( void )
{
    RtpPacketQueueResult_t result;

    result = RtpPacketQueue_Init( &( rtpPacketQueue ),
                                  &( rtpPacketInfoArr[ 0 ] ),
                                  MAX_IN_FLIGHT_PKTS );
    TEST_ASSERT_EQUAL( result,
                       RTP_PACKET_QUEUE_RESULT_OK );
}

/*-----------------------------------------------------------*/

/**
 * @brief Add packets in the RTP packet Queue.
 */
void test_RtpPacketQueue_ForceEnqueue( void )
{
    RtpPacketQueueResult_t result;
    RtpPacketInfo_t deletedRtpPacketInfo, rtpPacketInfo;
    uint8_t expectedSerializedPacket[] = { 0x80, 0x66, 0xAB, 0x12,
                                           0x12, 0x34, 0x43, 0x21,
                                           0xAB, 0xCD, 0xDB, 0xCA };

    result = RtpPacketQueue_Init( &( rtpPacketQueue ),
                                  &( rtpPacketInfoArr[ 0 ] ),
                                  MAX_IN_FLIGHT_PKTS );
    TEST_ASSERT_EQUAL( result,
                       RTP_PACKET_QUEUE_RESULT_OK );


    for( int i = 0; i < 20; i++ )
    {
        rtpPacketInfo.seqNum = i;
        rtpPacketInfo.pSerializedRtpPacket = expectedSerializedPacket;
        rtpPacketInfo.serializedPacketLength = sizeof( expectedSerializedPacket );
        result = RtpPacketQueue_ForceEnqueue( &( rtpPacketQueue ),
                                              &rtpPacketInfo,
                                              &deletedRtpPacketInfo );
        TEST_ASSERT_EQUAL( result,
                           RTP_PACKET_QUEUE_RESULT_OK );
    }
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate Queue full functionality.
 */
void test_RtpPacketQueue_ForceEnqueue_Full( void )
{
    RtpPacketQueueResult_t result;
    RtpPacketInfo_t deletedRtpPacketInfo, rtpPacketInfo;
    uint8_t expectedSerializedPacket[] = { 0x80, 0x66, 0xAB, 0x12,
                                           0x12, 0x34, 0x43, 0x21,
                                           0xAB, 0xCD, 0xDB, 0xCA };

    result = RtpPacketQueue_Init( &( rtpPacketQueue ),
                                  &( rtpPacketInfoArr[ 0 ] ),
                                  4 );
    TEST_ASSERT_EQUAL( result,
                       RTP_PACKET_QUEUE_RESULT_OK );

    for( int i = 0; i < 4; i++ )
    {
        rtpPacketInfo.seqNum = i;
        rtpPacketInfo.pSerializedRtpPacket = expectedSerializedPacket;
        rtpPacketInfo.serializedPacketLength = sizeof( expectedSerializedPacket );
        result = RtpPacketQueue_ForceEnqueue( &( rtpPacketQueue ),
                                              &rtpPacketInfo,
                                              &deletedRtpPacketInfo );
        TEST_ASSERT_EQUAL( result,
                       RTP_PACKET_QUEUE_RESULT_OK );
    }

    result = RtpPacketQueue_ForceEnqueue( &( rtpPacketQueue ),
                                              &rtpPacketInfo,
                                              &deletedRtpPacketInfo );
    TEST_ASSERT_EQUAL( result,
                       RTP_PACKET_QUEUE_RESULT_PACKET_DELETED );
    TEST_ASSERT_EQUAL( deletedRtpPacketInfo.seqNum,
                       0 ); // First packet deleted

    result = RtpPacketQueue_ForceEnqueue( &( rtpPacketQueue ),
                                              &rtpPacketInfo,
                                              &deletedRtpPacketInfo );
    TEST_ASSERT_EQUAL( result,
                       RTP_PACKET_QUEUE_RESULT_PACKET_DELETED );
    TEST_ASSERT_EQUAL( deletedRtpPacketInfo.seqNum,
                       1 ); // Second packet deleted
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate Queue deletion when queue is empty.
 */
void test_RtpPacketQueue_Dequeue_EmptyQueue( void )
{
    RtpPacketQueueResult_t result;
    RtpPacketInfo_t rtpPacketInfo;

    result = RtpPacketQueue_Init( &( rtpPacketQueue ),
                                  &( rtpPacketInfoArr[ 0 ] ),
                                  MAX_IN_FLIGHT_PKTS );
    TEST_ASSERT_EQUAL( result,
                       RTP_PACKET_QUEUE_RESULT_OK );

    result = RtpPacketQueue_Dequeue( &( rtpPacketQueue ),
                                     &rtpPacketInfo );
    TEST_ASSERT_EQUAL( result,
                       RTP_PACKET_QUEUE_RESULT_EMPTY );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate Queue deletion functionality.
 */
void test_RtpPacketQueue_Dequeue( void )
{
    RtpPacketQueueResult_t result;
    RtpPacketInfo_t deletedRtpPacketInfo, rtpPacketInfo, rtpDeletedPacketInfo;
    uint8_t expectedSerializedPacket[] = { 0x80, 0x66, 0xAB, 0x12,
                                           0x12, 0x34, 0x43, 0x21,
                                           0xAB, 0xCD, 0xDB, 0xCA };

    result = RtpPacketQueue_Init( &( rtpPacketQueue ),
                                  &( rtpPacketInfoArr[ 0 ] ),
                                  MAX_IN_FLIGHT_PKTS );
    TEST_ASSERT_EQUAL( result,
                       RTP_PACKET_QUEUE_RESULT_OK );

    rtpPacketInfo.seqNum = 10;
    rtpPacketInfo.pSerializedRtpPacket = expectedSerializedPacket;
    rtpPacketInfo.serializedPacketLength = sizeof( expectedSerializedPacket );
    result = RtpPacketQueue_ForceEnqueue( &( rtpPacketQueue ),
                                          &rtpPacketInfo,
                                          &deletedRtpPacketInfo );
    TEST_ASSERT_EQUAL( result,
                       RTP_PACKET_QUEUE_RESULT_OK );

    result = RtpPacketQueue_Dequeue( &( rtpPacketQueue ),
                                     &rtpDeletedPacketInfo );
    TEST_ASSERT_EQUAL( rtpDeletedPacketInfo.seqNum,
                       10 );
    TEST_ASSERT_EQUAL( rtpDeletedPacketInfo.serializedPacketLength,
                       sizeof( expectedSerializedPacket ) );
    TEST_ASSERT_EQUAL_UINT8_ARRAY( &( rtpDeletedPacketInfo.pSerializedRtpPacket[ 0 ] ),
                                   &( expectedSerializedPacket[ 0 ] ),
                                   sizeof( expectedSerializedPacket ) );

}

/*-----------------------------------------------------------*/

/**
 * @brief Validate Queue peek functionality when queue is empty.
 */
void test_RtpPacketQueue_Peek_EmptyQueue( void )
{
    RtpPacketQueueResult_t result;
    RtpPacketInfo_t rtpPacketInfo;

    result = RtpPacketQueue_Init( &( rtpPacketQueue ),
                                  &( rtpPacketInfoArr[ 0 ] ),
                                  MAX_IN_FLIGHT_PKTS );
    TEST_ASSERT_EQUAL( result,
                       RTP_PACKET_QUEUE_RESULT_OK );

    result = RtpPacketQueue_Peek( &( rtpPacketQueue ),
                                  &rtpPacketInfo );
    TEST_ASSERT_EQUAL( result,
                       RTP_PACKET_QUEUE_RESULT_EMPTY );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate Queue peek functionality.
 */
void test_RtpPacketQueue_Peek( void )
{
    RtpPacketQueueResult_t result;
    RtpPacketInfo_t deletedRtpPacketInfo, rtpPacketInfo, rtpDeletedPacketInfo;
    uint8_t expectedSerializedPacket[] = { 0x80, 0x66, 0xAB, 0x12,
                                           0x12, 0x34, 0x43, 0x21,
                                           0xAB, 0xCD, 0xDB, 0xCA };

    result = RtpPacketQueue_Init( &( rtpPacketQueue ),
                                  &( rtpPacketInfoArr[ 0 ] ),
                                  MAX_IN_FLIGHT_PKTS );
    TEST_ASSERT_EQUAL( result,
                       RTP_PACKET_QUEUE_RESULT_OK );

    result = RtpPacketQueue_Dequeue( &( rtpPacketQueue ),
                                     &rtpPacketInfo );
    TEST_ASSERT_EQUAL( result,
                       RTP_PACKET_QUEUE_RESULT_EMPTY );

    rtpPacketInfo.seqNum = 20;
    rtpPacketInfo.pSerializedRtpPacket = expectedSerializedPacket;
    rtpPacketInfo.serializedPacketLength = sizeof( expectedSerializedPacket );
    result = RtpPacketQueue_ForceEnqueue( &( rtpPacketQueue ),
                                          &rtpPacketInfo,
                                          &deletedRtpPacketInfo );
    TEST_ASSERT_EQUAL( result,
                       RTP_PACKET_QUEUE_RESULT_OK );

    result = RtpPacketQueue_Peek( &( rtpPacketQueue ),
                                  &rtpDeletedPacketInfo );
    TEST_ASSERT_EQUAL( rtpDeletedPacketInfo.seqNum,
                       20 );
    TEST_ASSERT_EQUAL( rtpDeletedPacketInfo.serializedPacketLength,
                       sizeof( expectedSerializedPacket ) );
    TEST_ASSERT_EQUAL_UINT8_ARRAY( &( rtpDeletedPacketInfo.pSerializedRtpPacket[ 0 ] ),
                                   &( expectedSerializedPacket[ 0 ] ),
                                   sizeof( expectedSerializedPacket ) );

}

/*-----------------------------------------------------------*/

/**
 * @brief Validate Queue Retrieve functionality when queue is empty.
 */
void test_RtpPacketQueue_Retrieve_Empty( void )
{
    RtpPacketQueueResult_t result;
    RtpPacketInfo_t retrieveRtpPacketInfo;

    result = RtpPacketQueue_Init( &( rtpPacketQueue ),
                                  &( rtpPacketInfoArr[ 0 ] ),
                                  MAX_IN_FLIGHT_PKTS );
    TEST_ASSERT_EQUAL( result,
                       RTP_PACKET_QUEUE_RESULT_OK );

    result = RtpPacketQueue_Retrieve( &( rtpPacketQueue ),
                                      0,
                                      &retrieveRtpPacketInfo );
    TEST_ASSERT_EQUAL( result,
                       RTP_PACKET_QUEUE_RESULT_EMPTY );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate Queue Retrieve functionality when element is not found.
 */
void test_RtpPacketQueue_Retrieve_NotFound( void )
{
    RtpPacketQueueResult_t result;
    RtpPacketInfo_t deletedRtpPacketInfo, rtpPacketInfo, rtpDeletedPacketInfo;
    uint8_t expectedSerializedPacket[] = { 0x80, 0x66, 0xAB, 0x12,
                                           0x12, 0x34, 0x43, 0x21,
                                           0xAB, 0xCD, 0xDB, 0xCA };

    result = RtpPacketQueue_Init( &( rtpPacketQueue ),
                                  &( rtpPacketInfoArr[ 0 ] ),
                                  MAX_IN_FLIGHT_PKTS );
    TEST_ASSERT_EQUAL( result,
                       RTP_PACKET_QUEUE_RESULT_OK );

    for( int i = 0; i < 20; i++ )
    {
        rtpPacketInfo.seqNum = i;
        rtpPacketInfo.pSerializedRtpPacket = expectedSerializedPacket;
        rtpPacketInfo.serializedPacketLength = sizeof( expectedSerializedPacket );
        result = RtpPacketQueue_ForceEnqueue( &( rtpPacketQueue ),
                                              &rtpPacketInfo,
                                              &deletedRtpPacketInfo );
        TEST_ASSERT_EQUAL( result,
                           RTP_PACKET_QUEUE_RESULT_OK );
    }

    result = RtpPacketQueue_Retrieve( &( rtpPacketQueue ),
                                      100,
                                      &rtpDeletedPacketInfo );
    TEST_ASSERT_EQUAL( result,
                       RTP_PACKET_QUEUE_RESULT_PACKET_NOT_FOUND );

}

/*-----------------------------------------------------------*/

/**
 * @brief Validate Queue Retrieve functionality when element is found.
 */
void test_RtpPacketQueue_Retrieve( void )
{
    RtpPacketQueueResult_t result;
    RtpPacketInfo_t deletedRtpPacketInfo, rtpPacketInfo, rtpDeletedPacketInfo;
    uint8_t expectedSerializedPacket[] = { 0x80, 0x66, 0xAB, 0x12,
                                           0x12, 0x34, 0x43, 0x21,
                                           0xAB, 0xCD, 0xDB, 0xCA };

    result = RtpPacketQueue_Init( &( rtpPacketQueue ),
                                  &( rtpPacketInfoArr[ 0 ] ),
                                  MAX_IN_FLIGHT_PKTS );
    TEST_ASSERT_EQUAL( result,
                       RTP_PACKET_QUEUE_RESULT_OK );

    for( int i = 0; i < 20; i++ )
    {
        rtpPacketInfo.seqNum = i;
        rtpPacketInfo.pSerializedRtpPacket = expectedSerializedPacket;
        rtpPacketInfo.serializedPacketLength = sizeof( expectedSerializedPacket );
        result = RtpPacketQueue_ForceEnqueue( &( rtpPacketQueue ),
                                              &rtpPacketInfo,
                                              &deletedRtpPacketInfo );
        TEST_ASSERT_EQUAL( result,
                           RTP_PACKET_QUEUE_RESULT_OK );
    }

    result = RtpPacketQueue_Retrieve( &( rtpPacketQueue ),
                                      8,
                                      &rtpDeletedPacketInfo );
    TEST_ASSERT_EQUAL( result,
                       RTP_PACKET_QUEUE_RESULT_OK );

}