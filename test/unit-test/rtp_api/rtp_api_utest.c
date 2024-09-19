/* Unity includes. */
#include "unity.h"
#include "catch_assert.h"

/* Standard includes. */
#include <string.h>
#include <stdint.h>

/* API includes. */
#include "rtp_api.h"

/* ===========================  EXTERN VARIABLES  =========================== */

void setUp( void )
{
}

void tearDown( void )
{
}

/* ==============================  Test Cases  ============================== */

/**
 * @brief Validate Rtp_DeSerialize with padding length 0.
 */
void test_Rtp_DeSerialize_NoPaddingByte( void )
{
    RtpResult_t result;
    RtpContext_t rtpContext;
    RtpPacket_t rtpPacket;
    uint8_t expectedSerializedPacket[] = { 0x80, 0x66, 0x00, 0x00, /* RTP header without padding bit set. */
                                           0x12, 0x34, 0x56, 0x78, /* Timestamp */
                                           0x9A, 0xBC, 0xDE, 0xFF, /* SSRC */
                                           0x12, 0x34, 0x56, 0x78, /* RTP payload with 0 padding bytes. */
                                           0x9A, 0xBC, 0xDE, 0xFF };
    size_t expectedPayloadLength = sizeof( expectedSerializedPacket ) - 12; /* 12 bytes for RTP header, 0 bytes for padding. */

    result = Rtp_Init( &rtpContext );
    TEST_ASSERT_EQUAL( RTP_RESULT_OK, result );

    result = Rtp_DeSerialize( &rtpContext,
                              expectedSerializedPacket,
                              sizeof( expectedSerializedPacket ),
                              &rtpPacket );
    TEST_ASSERT_EQUAL( RTP_RESULT_OK, result );
    TEST_ASSERT_FALSE( rtpPacket.header.flags & RTP_HEADER_FLAG_PADDING );
    TEST_ASSERT_EQUAL( expectedPayloadLength, rtpPacket.payloadLength );
    TEST_ASSERT_EQUAL( expectedSerializedPacket + 12, rtpPacket.pPayload );
}

/**
 * @brief Validate Rtp_DeSerialize with padding length 1.
 */
void test_Rtp_DeSerialize_OnePaddingByte( void )
{
    RtpResult_t result;
    RtpContext_t rtpContext;
    RtpPacket_t rtpPacket;
    uint8_t expectedSerializedPacket[] = { 0xA0, 0x66, 0x00, 0x00, /* RTP header with padding bit set. */
                                           0x12, 0x34, 0x56, 0x78, /* Timestamp */
                                           0x9A, 0xBC, 0xDE, 0xFF, /* SSRC */
                                           0x12, 0x34, 0x56, 0x78, /* RTP payload with 1 padding bytes. */
                                           0x9A, 0xBC, 0xDE, 0x01 };
    size_t expectedPayloadLength = sizeof( expectedSerializedPacket ) - 12 - 1; /* 12 bytes for RTP header, 1 bytes for padding. */

    result = Rtp_Init( &rtpContext );
    TEST_ASSERT_EQUAL( RTP_RESULT_OK, result );

    result = Rtp_DeSerialize( &rtpContext,
                              expectedSerializedPacket,
                              sizeof( expectedSerializedPacket ),
                              &rtpPacket );
    TEST_ASSERT_EQUAL( RTP_RESULT_OK, result );
    TEST_ASSERT_TRUE( rtpPacket.header.flags & RTP_HEADER_FLAG_PADDING );
    TEST_ASSERT_EQUAL( expectedPayloadLength, rtpPacket.payloadLength );
    TEST_ASSERT_EQUAL( expectedSerializedPacket + 12, rtpPacket.pPayload );
}

/**
 * @brief Validate Rtp_DeSerialize with padding length 2.
 */
void test_Rtp_DeSerialize_TwoPaddingBytes( void )
{
    RtpResult_t result;
    RtpContext_t rtpContext;
    RtpPacket_t rtpPacket;
    uint8_t expectedSerializedPacket[] = { 0xA0, 0x66, 0x00, 0x00, /* RTP header with padding bit set. */
                                           0x12, 0x34, 0x56, 0x78, /* Timestamp */
                                           0x9A, 0xBC, 0xDE, 0xFF, /* SSRC */
                                           0x12, 0x34, 0x56, 0x78, /* RTP payload with 2 padding bytes. */
                                           0x9A, 0xBC, 0x00, 0x02 };
    size_t expectedPayloadLength = sizeof( expectedSerializedPacket ) - 12 - 2; /* 12 bytes for RTP header, 2 bytes for padding. */

    result = Rtp_Init( &rtpContext );
    TEST_ASSERT_EQUAL( RTP_RESULT_OK, result );

    result = Rtp_DeSerialize( &rtpContext,
                              expectedSerializedPacket,
                              sizeof( expectedSerializedPacket ),
                              &rtpPacket );
    TEST_ASSERT_EQUAL( RTP_RESULT_OK, result );
    TEST_ASSERT_TRUE( rtpPacket.header.flags & RTP_HEADER_FLAG_PADDING );
    TEST_ASSERT_EQUAL( expectedPayloadLength, rtpPacket.payloadLength );
    TEST_ASSERT_EQUAL( expectedSerializedPacket + 12, rtpPacket.pPayload );
}

/**
 * @brief Validate Rtp_DeSerialize with padding length 3.
 */
void test_Rtp_DeSerialize_ThreePaddingBytes( void )
{
    RtpResult_t result;
    RtpContext_t rtpContext;
    RtpPacket_t rtpPacket;
    uint8_t expectedSerializedPacket[] = { 0xA0, 0x66, 0x00, 0x00, /* RTP header with padding bit set. */
                                           0x12, 0x34, 0x56, 0x78, /* Timestamp */
                                           0x9A, 0xBC, 0xDE, 0xFF, /* SSRC */
                                           0x12, 0x34, 0x56, 0x78, /* RTP payload with 3 padding bytes. */
                                           0x9A, 0x00, 0x00, 0x03 };
    size_t expectedPayloadLength = sizeof( expectedSerializedPacket ) - 12 - 3; /* 12 bytes for RTP header, 3 bytes for padding. */

    result = Rtp_Init( &rtpContext );
    TEST_ASSERT_EQUAL( RTP_RESULT_OK, result );

    result = Rtp_DeSerialize( &rtpContext,
                              expectedSerializedPacket,
                              sizeof( expectedSerializedPacket ),
                              &rtpPacket );
    TEST_ASSERT_EQUAL( RTP_RESULT_OK, result );
    TEST_ASSERT_TRUE( rtpPacket.header.flags & RTP_HEADER_FLAG_PADDING );
    TEST_ASSERT_EQUAL( expectedPayloadLength, rtpPacket.payloadLength );
    TEST_ASSERT_EQUAL( expectedSerializedPacket + 12, rtpPacket.pPayload );
}
