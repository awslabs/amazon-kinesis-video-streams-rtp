/* Unity includes. */
#include "unity.h"
#include "catch_assert.h"

/* Standard includes. */
#include <string.h>
#include <stdint.h>

/* API includes. */
#include "rtp_api.h"
#include "rtp_data_types.h"
#include "rtp_endianness.h"


/* ===========================  EXTERN VARIABLES  =========================== */

#define MAX_FRAME_LENGTH        10 * 1024
#define RTP_HEADER_MIN_LENGTH   12 /* No CSRC and no extension. */

void setUp( void )
{
}

void tearDown( void )
{
}

/* ==============================  Test Cases  ============================== */

/**
 * @brief Validate Rtp_Init in case of valid inputs.
 */
void test_Rtp_Init_Pass( void )
{
    RtpResult_t result;
    RtpContext_t ctx = { 0 };

    result = Rtp_Init( &( ctx ) );

    TEST_ASSERT_EQUAL( RTP_RESULT_OK,
                       result );

    TEST_ASSERT_NOT_NULL( ctx.readWriteFunctions.readUint32Fn );

    TEST_ASSERT_NOT_NULL( ctx.readWriteFunctions.writeUint32Fn );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate RTP_Serialize in case of valid inputs.
 */
void test_Rtp_Serialize_Pass( void )
{
    RtpResult_t result;
    RtpContext_t ctx = { 0 };
    RtpPacket_t packet = { 0 };
    uint8_t buffer[ MAX_FRAME_LENGTH ] = { 0 };
    size_t bufferLength = MAX_FRAME_LENGTH;
    uint8_t expectedSerializedPacket[] = {
        /* RTP header */
        0x80, 0x60, 0x04, 0xD2,
        /* Timestamp */
        0x12, 0x34, 0x56, 0x78,
        /* SSRC */
        0x87, 0x65, 0x43, 0x21,
        /* RTP payload - "hello world!" */
        0x68, 0x65, 0x6C, 0x6C,
        0x6F, 0x20, 0x77, 0x6F,
        0x72, 0x6C, 0x64, 0x21
    };

    result = Rtp_Init( &( ctx ) );

    TEST_ASSERT_EQUAL( RTP_RESULT_OK,
                       result );

    packet.header.payloadType = 96;
    packet.header.sequenceNumber = 1234;
    packet.header.timestamp = 0x12345678;
    packet.header.ssrc = 0x87654321;
    packet.payloadLength = 12;
    packet.pPayload = ( uint8_t * ) "hello world!";

    result = Rtp_Serialize( &( ctx ),
                            &( packet ),
                            &( buffer[0] ),
                            &( bufferLength ) );

    TEST_ASSERT_EQUAL( RTP_RESULT_OK,
                       result );

    TEST_ASSERT_EQUAL_UINT8_ARRAY( expectedSerializedPacket,
                                   buffer,
                                   sizeof( expectedSerializedPacket ) );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate RTP_Serialize in case of valid inputs with CSRC identifiers.
 */
void test_Rtp_Serialize_WithCsrc( void )
{
    RtpResult_t result;
    RtpContext_t ctx = { 0 };
    RtpPacket_t packet = { 0 };
    uint8_t buffer[MAX_FRAME_LENGTH] = { 0 };
    size_t bufferLength = MAX_FRAME_LENGTH;
    uint32_t csrcArray[ 2 ] = { 0x11223344, 0x55667788 };
    uint8_t expectedSerializedPacket[] = {
        /* RTP header (CSRC count = 2) */
        0x82, 0x60, 0x04, 0xD2,
        /* Timestamp */
        0x12, 0x34, 0x56, 0x78,
        /* SSRC */
        0x87, 0x65, 0x43, 0x21,
        /* CSRC identifier 1 */
        0x11, 0x22, 0x33, 0x44,
        /* CSRC identifier 2 */
        0x55, 0x66, 0x77, 0x88,
        /* RTP payload - "hello world!" */
        0x68, 0x65, 0x6C, 0x6C,
        0x6F, 0x20, 0x77, 0x6F,
        0x72, 0x6C, 0x64, 0x21
    };

    result = Rtp_Init( &( ctx ) );

    TEST_ASSERT_EQUAL( RTP_RESULT_OK,
                       result );

    packet.header.payloadType = 96;
    packet.header.sequenceNumber = 1234;
    packet.header.timestamp = 0x12345678;
    packet.header.ssrc = 0x87654321;
    packet.header.csrcCount = 2;
    packet.header.pCsrc = csrcArray;
    packet.payloadLength = 12;
    packet.pPayload = ( uint8_t * ) "hello world!";

    result = Rtp_Serialize( &( ctx ),
                            &( packet ),
                            &( buffer[0] ),
                            &( bufferLength ) );

    TEST_ASSERT_EQUAL( RTP_RESULT_OK,
                       result );

    TEST_ASSERT_EQUAL_UINT8_ARRAY( expectedSerializedPacket,
                                   buffer,
                                   sizeof( expectedSerializedPacket ) );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate RTP_Serialize in case of flag set as RTP_HEADER_FLAG_EXTENSION.
 */
void test_Rtp_Serialize_Pass_FlagExtension( void )
{
    RtpResult_t result;
    RtpContext_t ctx = { 0 };
    RtpPacket_t packet = { 0 };
    uint8_t buffer[MAX_FRAME_LENGTH] = { 0 };
    size_t length = MAX_FRAME_LENGTH;
    uint32_t extensionPayload[ 2 ] = { 0x11223344, 0x55667788 };
    uint8_t expectedSerializedPacket[] = {
        /* RTP header (extension flag set) */
        0x90, 0x60, 0x04, 0xD2,
        /* Timestamp */
        0x12, 0x34, 0x56, 0x78,
        /* SSRC */
        0x87, 0x65, 0x43, 0x21,
        /* Extension header length */
        0x00, 0x00, 0x00, 0x02,
        /* Extension payload (part 1) */
        0x11, 0x22, 0x33, 0x44,
        /* Extension payload (part 2) */
        0x55, 0x66, 0x77, 0x88,
        /* RTP payload - "hello world!" */
        0x68, 0x65, 0x6C, 0x6C,
        0x6F, 0x20, 0x77, 0x6F,
        0x72, 0x6C, 0x64, 0x21
    };

    result = Rtp_Init( &( ctx ) );

    TEST_ASSERT_EQUAL( RTP_RESULT_OK,
                       result );

    packet.header.payloadType = 96;
    packet.header.sequenceNumber = 1234;
    packet.header.timestamp = 0x12345678;
    packet.header.ssrc = 0x87654321;
    packet.payloadLength = 12;
    packet.pPayload = ( uint8_t * )"hello world!";
    packet.header.flags = 0b100;
    packet.header.extension.extensionPayloadLength = 2;
    packet.header.extension.pExtensionPayload = extensionPayload;

    result = Rtp_Serialize( &( ctx ),
                            &( packet ),
                            &( buffer[0] ),
                            &( length ) );

    TEST_ASSERT_EQUAL( RTP_RESULT_OK,
                       result );

    TEST_ASSERT_EQUAL_UINT8_ARRAY( expectedSerializedPacket,
                                   buffer,
                                   sizeof( expectedSerializedPacket ) );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate RTP_Serialize in case of flag set as RTP_HEADER_FLAG_MARKER.
 */
void test_Rtp_Serialize_Pass_FlagMarker( void )
{
    RtpResult_t result;
    RtpContext_t ctx = { 0 };
    RtpPacket_t packet = { 0 };
    uint8_t buffer[ MAX_FRAME_LENGTH ] = { 0 };
    size_t bufferLength = MAX_FRAME_LENGTH;
    uint8_t expectedSerializedPacket[] = {
        /* RTP header (marker flag set) */
        0x80, 0xE0, 0x04, 0xD2,
        /* Timestamp */
        0x12, 0x34, 0x56, 0x78,
        /* SSRC */
        0x87, 0x65, 0x43, 0x21,
        /* RTP payload - "hello world!" */
        0x68, 0x65, 0x6C, 0x6C,
        0x6F, 0x20, 0x77, 0x6F,
        0x72, 0x6C, 0x64, 0x21
    };

    result = Rtp_Init( &( ctx ) );
    TEST_ASSERT_EQUAL( RTP_RESULT_OK,
                       result );

    packet.header.payloadType = 96;
    packet.header.sequenceNumber = 1234;
    packet.header.timestamp = 0x12345678;
    packet.header.ssrc = 0x87654321;
    packet.payloadLength = 12;
    packet.pPayload = ( uint8_t * ) "hello world!";
    packet.header.flags = RTP_HEADER_FLAG_MARKER; /* Set the marker flag */

    result = Rtp_Serialize( &( ctx ),
                            &( packet ),
                            &( buffer[0] ),
                            &( bufferLength ) );

    TEST_ASSERT_EQUAL( RTP_RESULT_OK,
                       result );

    TEST_ASSERT_EQUAL_UINT8_ARRAY( expectedSerializedPacket,
                                   buffer,
                                   sizeof( expectedSerializedPacket ) );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate Rtp_Serialize with padding length 1.
 */
void test_Rtp_Serialize_OnePaddingByte( void )
{
    RtpResult_t result;
    RtpContext_t ctx = { 0 };
    RtpPacket_t packet = { 0 };
    uint8_t buffer[21];
    size_t bufferLength = sizeof( buffer );
    uint8_t expectedSerializedPacket[] = {
        /* RTP header with padding bit set. */
        0xA0, 0x66, 0x00, 0x00,
        /* Timestamp */
        0x12, 0x34, 0x56, 0x78,
        /* SSRC */
        0x9A, 0xBC, 0xDE, 0xFF,
        /* RTP payload with 1 padding byte. */
        0x12, 0x34, 0x56, 0x78,
        0x9A, 0xBC, 0xDE, 0x01
    };

    result = Rtp_Init( &( ctx ) );
    TEST_ASSERT_EQUAL( RTP_RESULT_OK,
                       result );

    packet.header.flags = RTP_HEADER_FLAG_PADDING;
    packet.header.payloadType = 0x66;
    packet.header.timestamp = 0x12345678;
    packet.header.ssrc = 0x9ABCDEFFL;
    packet.pPayload = expectedSerializedPacket + 12;
    packet.payloadLength = sizeof( expectedSerializedPacket ) - 12;

    result = Rtp_Serialize( &( ctx ),
                            &( packet ),
                            &( buffer[0] ),
                            &( bufferLength ) );

    TEST_ASSERT_EQUAL( RTP_RESULT_OK,
                       result );

    TEST_ASSERT_EQUAL_UINT8_ARRAY( expectedSerializedPacket,
                                   buffer,
                                   sizeof( expectedSerializedPacket ) );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate Rtp_Serialize with padding length 2.
 */
void test_Rtp_Serialize_TwoPaddingBytes( void )
{
    RtpResult_t result;
    RtpContext_t ctx = { 0 };
    RtpPacket_t packet = { 0 };
    uint8_t buffer[22];
    size_t bufferLength = sizeof( buffer );
    uint8_t expectedSerializedPacket[] = {
        /* RTP header with padding bit set. */
        0xA0, 0x66, 0x00, 0x00,
        /* Timestamp */
        0x12, 0x34, 0x56, 0x78,
        /* SSRC */
        0x9A, 0xBC, 0xDE, 0xFF,
        /* RTP payload with 2 padding bytes. */
        0x12, 0x34, 0x56, 0x78,
        0x9A, 0xBC, 0x00, 0x02
    };

    result = Rtp_Init( &( ctx ) );
    TEST_ASSERT_EQUAL( RTP_RESULT_OK,
                       result );

    packet.header.flags = RTP_HEADER_FLAG_PADDING;
    packet.header.payloadType = 0x66;
    packet.header.timestamp = 0x12345678;
    packet.header.ssrc = 0x9ABCDEFFL;
    packet.pPayload = expectedSerializedPacket + 12;
    packet.payloadLength = sizeof( expectedSerializedPacket ) - 12;

    result = Rtp_Serialize( &( ctx ),
                            &( packet ),
                            &( buffer[0] ),
                            &( bufferLength ) );

    TEST_ASSERT_EQUAL( RTP_RESULT_OK,
                       result );

    TEST_ASSERT_EQUAL_UINT8_ARRAY( expectedSerializedPacket,
                                   buffer,
                                   sizeof( expectedSerializedPacket ) );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate Rtp_Serialize with padding length 3.
 */
void test_Rtp_Serialize_ThreePaddingBytes( void )
{
    RtpResult_t result;
    RtpContext_t ctx = { 0 };
    RtpPacket_t packet = { 0 };
    uint8_t buffer[23];
    size_t bufferLength = sizeof( buffer );
    uint8_t expectedSerializedPacket[] = {
        /* RTP header with padding bit set. */
        0xA0, 0x66, 0x00, 0x00,
        /* Timestamp */
        0x12, 0x34, 0x56, 0x78,
        /* SSRC */
        0x9A, 0xBC, 0xDE, 0xFF,
        /* RTP payload with 3 padding bytes. */
        0x12, 0x34, 0x56, 0x78,
        0x9A, 0x00, 0x00, 0x03
    };

    result = Rtp_Init( &( ctx ) );
    TEST_ASSERT_EQUAL( RTP_RESULT_OK,
                       result );

    packet.header.flags = RTP_HEADER_FLAG_PADDING;
    packet.header.payloadType = 0x66;
    packet.header.timestamp = 0x12345678;
    packet.header.ssrc = 0x9ABCDEFFL;
    packet.pPayload = expectedSerializedPacket + 12;
    packet.payloadLength = sizeof( expectedSerializedPacket ) - 12;

    result = Rtp_Serialize( &( ctx ),
                            &( packet ),
                            &( buffer[0] ),
                            &( bufferLength ) );

    TEST_ASSERT_EQUAL( RTP_RESULT_OK,
                       result );

    TEST_ASSERT_EQUAL_UINT8_ARRAY( expectedSerializedPacket,
                                   buffer,
                                   sizeof( expectedSerializedPacket ) );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate Rtp_Serialize with an invalid padding length.
 */
void test_Rtp_Serialize_InvalidPaddingLength( void )
{
    RtpResult_t result;
    RtpContext_t ctx = { 0 };
    RtpPacket_t packet = { 0 };
    uint8_t buffer[MAX_FRAME_LENGTH] = { 0 };
    size_t bufferLength = MAX_FRAME_LENGTH;
    uint8_t expectedSerializedPacket[] = {
        /* RTP header with padding bit set. */
        0xA0, 0x66, 0x00, 0x00,
        /* Timestamp */
        0x12, 0x34, 0x56, 0x78,
        /* SSRC */
        0x9A, 0xBC, 0xDE, 0xFF,
        /* RTP payload with 3 padding bytes. */
        0x12, 0x00, 0x00, 0x05
    };

    result = Rtp_Init( &( ctx ) );
    TEST_ASSERT_EQUAL( RTP_RESULT_OK,
                       result );

    packet.header.flags = RTP_HEADER_FLAG_PADDING;
    packet.header.payloadType = 0x66;
    packet.header.timestamp = 0x12345678;
    packet.header.ssrc = 0x9ABCDEFFL;
    packet.pPayload = expectedSerializedPacket + 12;
    packet.payloadLength = sizeof( expectedSerializedPacket ) - 12;

    result = Rtp_Serialize( &( ctx ),
                            &( packet ),
                            &( buffer[0] ),
                            &( bufferLength ) );

    TEST_ASSERT_EQUAL( RTP_RESULT_MALFORMED_PACKET,
                       result );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate RTP_Serialize with not NULL payload but 0 payload length.
 */
void test_Rtp_Serialize_Pass_ZeroPayloadLength( void )
{
    RtpResult_t result;
    RtpContext_t ctx = { 0 };
    RtpPacket_t packet = { 0 };
    uint8_t buffer[ MAX_FRAME_LENGTH ] = { 0 };
    size_t bufferLength = MAX_FRAME_LENGTH;
    uint8_t expectedSerializedPacket[] = {
        /* RTP header */
        0x80, 0x60, 0x04, 0xD2,
        /* Timestamp */
        0x12, 0x34, 0x56, 0x78,
        /* SSRC */
        0x87, 0x65, 0x43, 0x21
    };
    size_t expectedSerializedBufferLength = sizeof( expectedSerializedPacket );

    result = Rtp_Init( &( ctx ) );

    TEST_ASSERT_EQUAL( RTP_RESULT_OK,
                       result );

    packet.header.payloadType = 96;
    packet.header.sequenceNumber = 1234;
    packet.header.timestamp = 0x12345678;
    packet.header.ssrc = 0x87654321;
    packet.payloadLength = 0;
    packet.pPayload = ( uint8_t * ) "hello world!";

    result = Rtp_Serialize( &( ctx ),
                            &( packet ),
                            &( buffer[0] ),
                            &( bufferLength ) );

    TEST_ASSERT_EQUAL( RTP_RESULT_OK,
                       result );

    TEST_ASSERT_EQUAL( expectedSerializedBufferLength,
                       bufferLength );

    TEST_ASSERT_EQUAL_UINT8_ARRAY( expectedSerializedPacket,
                                   buffer,
                                   bufferLength );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate Rtp_DeSerialize with padding length 0.
 */
void test_Rtp_DeSerialize_NoPaddingByte( void )
{
    RtpResult_t result;
    RtpContext_t ctx = { 0 };
    RtpPacket_t packet = { 0 };
    uint8_t serializedPacket[] = {
        /* RTP header without padding bit set. */
        0x80, 0x66, 0x00, 0x00,
        /* Timestamp */
        0x12, 0x34, 0x56, 0x78,
        /* SSRC */
        0x9A, 0xBC, 0xDE, 0xFF,
        /* RTP payload with 0 padding bytes. */
        0x12, 0x34, 0x56, 0x78,
        0x9A, 0xBC, 0xDE, 0xFF
    };
    size_t payloadLength = sizeof( serializedPacket ) - 12; /* 12 bytes for RTP header, 0 bytes for padding. */

    result = Rtp_Init( &( ctx ) );

    TEST_ASSERT_EQUAL( RTP_RESULT_OK,
                       result );

    result = Rtp_DeSerialize( &( ctx ),
                              &( serializedPacket[0] ),
                              sizeof( serializedPacket ),
                              &( packet ) );

    TEST_ASSERT_EQUAL( RTP_RESULT_OK,
                       result );

    TEST_ASSERT_FALSE( packet.header.flags & RTP_HEADER_FLAG_PADDING );

    TEST_ASSERT_EQUAL( payloadLength,
                       packet.payloadLength );

    TEST_ASSERT_EQUAL( serializedPacket + 12,
                       packet.pPayload );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate Rtp_DeSerialize with padding length 1.
 */
void test_Rtp_DeSerialize_OnePaddingByte( void )
{
    RtpResult_t result;
    RtpContext_t ctx = { 0 };
    RtpPacket_t packet = { 0 };
    uint8_t serializedPacket[] = {
        /* RTP header with padding bit set. */
        0xA0, 0x66, 0x00, 0x00,
        /* Timestamp */
        0x12, 0x34, 0x56, 0x78,
        /* SSRC */
        0x9A, 0xBC, 0xDE, 0xFF,
        /* RTP payload with 1 padding bytes. */
        0x12, 0x34, 0x56, 0x78,
        0x9A, 0xBC, 0xDE, 0x01
    };
    size_t payloadLength = sizeof( serializedPacket ) - 12 - 1; /* 12 bytes for RTP header, 1 bytes for padding. */

    result = Rtp_Init( &( ctx ) );

    TEST_ASSERT_EQUAL( RTP_RESULT_OK,
                       result );

    result = Rtp_DeSerialize( &( ctx ),
                              &( serializedPacket[0] ),
                              sizeof( serializedPacket ),
                              &( packet ) );

    TEST_ASSERT_EQUAL( RTP_RESULT_OK,
                       result );

    TEST_ASSERT_TRUE( packet.header.flags & RTP_HEADER_FLAG_PADDING );

    TEST_ASSERT_EQUAL( payloadLength,
                       packet.payloadLength );

    TEST_ASSERT_EQUAL( serializedPacket + 12,
                       packet.pPayload );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate Rtp_DeSerialize with padding length 2.
 */
void test_Rtp_DeSerialize_TwoPaddingBytes( void )
{
    RtpResult_t result;
    RtpContext_t ctx = { 0 };
    RtpPacket_t packet = { 0 };
    uint8_t serializedPacket[] = {
        /* RTP header with padding bit set. */
        0xA0, 0x66, 0x00, 0x00,
        /* Timestamp */
        0x12, 0x34, 0x56, 0x78,
        /* SSRC */
        0x9A, 0xBC, 0xDE, 0xFF,
        /* RTP payload with 2 padding bytes. */
        0x12, 0x34, 0x56, 0x78,
        0x9A, 0xBC, 0x00, 0x02
    };
    size_t payloadLength = sizeof( serializedPacket ) - 12 - 2; /* 12 bytes for RTP header, 2 bytes for padding. */

    result = Rtp_Init( &( ctx ) );

    TEST_ASSERT_EQUAL( RTP_RESULT_OK,
                       result );

    result = Rtp_DeSerialize( &( ctx ),
                              &( serializedPacket[0] ),
                              sizeof( serializedPacket ),
                              &( packet ) );

    TEST_ASSERT_EQUAL( RTP_RESULT_OK,
                       result );

    TEST_ASSERT_TRUE( packet.header.flags & RTP_HEADER_FLAG_PADDING );

    TEST_ASSERT_EQUAL( payloadLength,
                       packet.payloadLength );

    TEST_ASSERT_EQUAL( serializedPacket + 12,
                       packet.pPayload );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate Rtp_DeSerialize with padding length 3.
 */
void test_Rtp_DeSerialize_ThreePaddingBytes( void )
{
    RtpResult_t result;
    RtpContext_t ctx = { 0 };
    RtpPacket_t packet = { 0 };
    uint8_t serializedPacket[] = {
        /* RTP header with padding bit set. */
        0xA0, 0x66, 0x00, 0x00,
        /* Timestamp */
        0x12, 0x34, 0x56, 0x78,
        /* SSRC */
        0x9A, 0xBC, 0xDE, 0xFF,
        /* RTP payload with 3 padding bytes. */
        0x12, 0x34, 0x56, 0x78,
        0x9A, 0x00, 0x00, 0x03
    };
    size_t payloadLength = sizeof( serializedPacket ) - 12 - 3; /* 12 bytes for RTP header, 3 bytes for padding. */

    result = Rtp_Init( &( ctx ) );

    TEST_ASSERT_EQUAL( RTP_RESULT_OK,
                       result );

    result = Rtp_DeSerialize( &( ctx ),
                              &( serializedPacket[0] ),
                              sizeof( serializedPacket ),
                              &( packet ) );

    TEST_ASSERT_EQUAL( RTP_RESULT_OK,
                       result );

    TEST_ASSERT_TRUE( packet.header.flags & RTP_HEADER_FLAG_PADDING );

    TEST_ASSERT_EQUAL( payloadLength,
                       packet.payloadLength );

    TEST_ASSERT_EQUAL( serializedPacket + 12,
                       packet.pPayload );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate Rtp_DeSerialize with an invalid padding length.
 */
void test_Rtp_DeSerialize_InvalidPaddingLength( void )
{
    RtpResult_t result;
    RtpContext_t ctx = { 0 };
    RtpPacket_t packet = { 0 };
    uint8_t serializedPacket[] = {
        /* RTP header with padding bit set. */
        0xA0, 0x66, 0x00, 0x00,
        /* Timestamp */
        0x12, 0x34, 0x56, 0x78,
        /* SSRC */
        0x9A, 0xBC, 0xDE, 0xFF,
        /* RTP payload with 1 padding bytes. */
        0x12, 0x34, 0x56, 0x78,
        0x9A, 0xBC, 0xDE, 0x10
    };

    result = Rtp_Init( &( ctx ) );

    TEST_ASSERT_EQUAL( RTP_RESULT_OK,
                       result );

    result = Rtp_DeSerialize( &( ctx ),
                              &( serializedPacket[0] ),
                              sizeof( serializedPacket ),
                              &( packet ) );

    TEST_ASSERT_EQUAL( RTP_RESULT_MALFORMED_PACKET,
                       result );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate RTP_Init in case of bad parameters.
 */
void test_Rtp_Init_BadParams( void )
{
    RtpResult_t result;

    result = Rtp_Init( NULL );

    TEST_ASSERT_EQUAL( RTP_RESULT_BAD_PARAM,
                       result );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate Rtp_Serialize in case of bad parameters.
 */
void test_Rtp_Serialize_BadParams( void )
{
    RtpResult_t result;
    RtpContext_t ctx = { 0 };
    RtpPacket_t packet = { 0 };
    uint8_t buffer;
    size_t bufferLength = sizeof( buffer );

    result = Rtp_Serialize( NULL,
                            &( packet ),
                            &( buffer ),
                            &( bufferLength ) );

    TEST_ASSERT_EQUAL( RTP_RESULT_BAD_PARAM,
                       result );

    result = Rtp_Serialize( &( ctx ),
                            NULL,
                            &( buffer ),
                            &( bufferLength ) );

    TEST_ASSERT_EQUAL( RTP_RESULT_BAD_PARAM,
                       result );

    result = Rtp_Serialize( &( ctx ),
                            &( packet ),
                            &( buffer ),
                            0 );

    TEST_ASSERT_EQUAL( RTP_RESULT_BAD_PARAM,
                       result );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate Rtp_Deserialize in case of bad parameters.
 */
void test_Rtp_DeSerialize_BadParams( void )
{
    RtpResult_t result;
    RtpContext_t ctx = { 0 };
    RtpPacket_t packet = { 0 };
    uint8_t serializedPacket[] = {
        /* RTP header. */
        0x80, 0x60, 0x04, 0xD2,
        /* Timestamp */
        0x12, 0x34, 0x56, 0x78,
        /* SSRC */
        0x87, 0x65, 0x43, 0x21,
        /* RTP payload */
        0x68, 0x65, 0x6C, 0x6C,
        0x6F, 0x20, 0x77, 0x6F,
        0x72, 0x6C, 0x64
    };
    size_t length = sizeof( serializedPacket );

    result = Rtp_DeSerialize( NULL,
                              &( serializedPacket[0] ),
                              length,
                              &( packet ) );

    TEST_ASSERT_EQUAL( RTP_RESULT_BAD_PARAM,
                       result );

    result = Rtp_DeSerialize( &( ctx ),
                              NULL,
                              length,
                              &( packet ) );

    TEST_ASSERT_EQUAL( RTP_RESULT_BAD_PARAM,
                       result );

    length = RTP_HEADER_MIN_LENGTH - 1;

    result = Rtp_DeSerialize( &( ctx ),
                              &( serializedPacket[0] ),
                              length,
                              &( packet ) );

    TEST_ASSERT_EQUAL( RTP_RESULT_BAD_PARAM,
                       result );

    packet.pPayload = NULL;
    packet.payloadLength = 0;

    result = Rtp_DeSerialize( &( ctx ),
                              &( serializedPacket[0] ),
                              length,
                              &( packet ) );

    TEST_ASSERT_EQUAL( RTP_RESULT_BAD_PARAM,
                       result );

    length = sizeof( serializedPacket );

    result = Rtp_DeSerialize( &( ctx ),
                              &( serializedPacket[0] ),
                              length,
                              NULL );

    TEST_ASSERT_EQUAL( RTP_RESULT_BAD_PARAM,
                       result );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate Rtp_Serialize incase of out of memory - buffer
 * is NULL and the provided length is less than the required length.
 */
void test_Rtp_Serialize_NullBuffer_ShortLength( void )
{
    RtpResult_t result;
    RtpContext_t ctx = { 0 };
    RtpPacket_t packet = { 0 };
    size_t bufferLength = 5;

    result = Rtp_Init( &( ctx ) );
    TEST_ASSERT_EQUAL( RTP_RESULT_OK,
                       result );

    packet.header.payloadType = 96;
    packet.header.sequenceNumber = 1234;
    packet.header.timestamp = 0x12345678;
    packet.header.ssrc = 0x87654321;
    packet.payloadLength = 12;
    packet.pPayload = ( uint8_t * )"hello world!";

    result = Rtp_Serialize( &( ctx ),
                            &( packet ),
                            NULL,
                            &( bufferLength ) );

    TEST_ASSERT_EQUAL( RTP_RESULT_OK,
                       result );

    TEST_ASSERT_EQUAL( 24,
                       bufferLength );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate Rtp_Serialize incase of out of memory - buffer
 * is not NULL and the provided length is less than the required length.
 */
void test_Rtp_Serialize_OutOfMemory( void )
{
    RtpResult_t result;
    RtpContext_t ctx = { 0 };
    RtpPacket_t packet = { 0 };
    uint8_t buffer[ 10 ] = { 0 };
    size_t bufferLength = sizeof( buffer );

    result = Rtp_Init( &( ctx ) );
    TEST_ASSERT_EQUAL( RTP_RESULT_OK,
                       result );

    packet.header.payloadType = 96;
    packet.header.sequenceNumber = 1234;
    packet.header.timestamp = 0x12345678;
    packet.header.ssrc = 0x87654321;
    packet.payloadLength = 12;
    packet.pPayload = ( uint8_t * )"hello world";

    result = Rtp_Serialize( &( ctx ),
                            &( packet ),
                            &( buffer[0] ),
                            &( bufferLength ) );

    TEST_ASSERT_EQUAL( RTP_RESULT_OUT_OF_MEMORY,
                       result );

    TEST_ASSERT_EQUAL( 10,
                       bufferLength );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate Rtp_Serialize with no payload data.
 */
void test_Rtp_Serialize_NoPayload( void )
{
    RtpResult_t result;
    RtpContext_t ctx = { 0 };
    RtpPacket_t packet = { 0 };
    uint8_t buffer[ MAX_FRAME_LENGTH ] = { 0 };
    size_t bufferLength = MAX_FRAME_LENGTH;
    uint8_t expectedSerializedPacket[] = {
        /* RTP header */
        0x80, 0x60, 0x04, 0xD2,
        /* Timestamp */
        0x12, 0x34, 0x56, 0x78,
        /* SSRC */
        0x87, 0x65, 0x43, 0x21
    };
    size_t expectedSerializedBufferLength = sizeof( expectedSerializedPacket );

    result = Rtp_Init( &( ctx ) );

    TEST_ASSERT_EQUAL( RTP_RESULT_OK,
                       result );

    packet.header.payloadType = 96;
    packet.header.sequenceNumber = 1234;
    packet.header.timestamp = 0x12345678;
    packet.header.ssrc = 0x87654321;
    packet.payloadLength = 0;
    packet.pPayload = NULL;

    result = Rtp_Serialize( &( ctx ),
                            &( packet ),
                            &( buffer[0] ),
                            &( bufferLength ) );

    TEST_ASSERT_EQUAL( RTP_RESULT_OK,
                       result );

    TEST_ASSERT_EQUAL( expectedSerializedBufferLength,
                       bufferLength );

    TEST_ASSERT_EQUAL_UINT8_ARRAY( expectedSerializedPacket,
                                   buffer,
                                   bufferLength );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate Rtp_DeSerialize with a valid packet without extension or padding.
 */
void test_Rtp_DeSerialize_Pass_NoExtensionNoPadding( void )
{
    RtpResult_t result;
    RtpContext_t ctx = { 0 };
    RtpPacket_t packet = { 0 };
    uint8_t serializedPacket[] = {
        /* RTP header */
        0x80, 0x60, 0x04, 0xD2,
        /* Timestamp */
        0x12, 0x34, 0x56, 0x78,
        /* SSRC */
        0x87, 0x65, 0x43, 0x21,
        /* RTP payload */
        0x68, 0x65, 0x6C, 0x6C,
        0x6F, 0x20, 0x77, 0x6F,
        0x72, 0x6C, 0x64, 0X21
    };

    size_t length = sizeof( serializedPacket );

    result = Rtp_Init( &( ctx ) );

    TEST_ASSERT_EQUAL( RTP_RESULT_OK,
                       result );

    result = Rtp_DeSerialize( &( ctx ),
                              &( serializedPacket[0] ),
                              length,
                              &( packet ) );

    TEST_ASSERT_EQUAL( RTP_RESULT_OK,
                       result );

    TEST_ASSERT_EQUAL( 0,
                       packet.header.flags );  // No flags set

    TEST_ASSERT_EQUAL( 0,
                       packet.header.csrcCount );

    TEST_ASSERT_NULL( packet.header.pCsrc );

    TEST_ASSERT_EQUAL( 96,
                       packet.header.payloadType );

    TEST_ASSERT_EQUAL( 1234,
                       packet.header.sequenceNumber );

    TEST_ASSERT_EQUAL( 0x12345678,
                       packet.header.timestamp );

    TEST_ASSERT_EQUAL( 0x87654321,
                       packet.header.ssrc );

    TEST_ASSERT_EQUAL_STRING_LEN( "hello world!",
                                  ( char * ) packet.pPayload,
                                  packet.payloadLength );

    TEST_ASSERT_EQUAL( 12,
                       packet.payloadLength );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate Rtp_DeSerialize with a valid packet with CSRC identifiers.
 */
void test_Rtp_DeSerialize_Pass_WithCsrc( void )
{
    RtpResult_t result;
    RtpContext_t ctx = { 0 };
    RtpPacket_t packet = { 0 };
    uint8_t serializedPacket[] = {
        /* RTP header with CSRC count = 2 */
        0x82, 0x60, 0x04, 0xD2,
        /* Timestamp */
        0x12, 0x34, 0x56, 0x78,
        /* SSRC */
        0x87, 0x65, 0x43, 0x21,
        /* CSRC 1 */
        0x11, 0x22, 0x33, 0x44,
        /* CSRC 2*/
        0x55, 0x66, 0x77, 0x88,
        /* RTP payload */
        0x68, 0x65, 0x6C, 0x6C,
        0x6F, 0x20, 0x77, 0x6F,
        0x72, 0x6C, 0x64
    };
    size_t length = sizeof( serializedPacket );

    result = Rtp_Init( &( ctx ) );

    TEST_ASSERT_EQUAL( RTP_RESULT_OK,
                       result );

    result = Rtp_DeSerialize( &( ctx ),
                              &( serializedPacket[0] ),
                              length,
                              &( packet ) );

    TEST_ASSERT_EQUAL( RTP_RESULT_OK,
                       result );

    TEST_ASSERT_EQUAL( 0,
                       packet.header.flags );  // No flags set

    TEST_ASSERT_EQUAL( 2,
                       packet.header.csrcCount );

    TEST_ASSERT_EQUAL_HEX32( 0x11223344,
                             packet.header.pCsrc[0] );

    TEST_ASSERT_EQUAL_HEX32( 0x55667788,
                             packet.header.pCsrc[1] );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate Rtp_DeSerialize with a valid packet with extension header.
 */
void test_Rtp_DeSerialize_Pass_WithExtension( void )
{
    RtpResult_t result;
    RtpContext_t ctx = { 0 };
    RtpPacket_t packet = { 0 };
    uint8_t serializedPacket[] = {
        /* RTP header with extension flag set */
        0x90, 0x60, 0x04, 0xD2,
        /* Timestamp */
        0x12, 0x34, 0x56, 0x78,
        /* SSRC */
        0x87, 0x65, 0x43, 0x21,
        /* Extension header (profile = 0, length = 2) */
        0x00, 0x00, 0x00, 0x02,
        /* Extension payload 1 */
        0x11, 0x22, 0x33, 0x44,
        /* Extension payload 2 */
        0x55, 0x66, 0x77, 0x88,
        /* RTP payload */
        0x68, 0x65, 0x6C, 0x6C,
        0x6F, 0x20, 0x77, 0x6F,
        0x72, 0x6C, 0x64
    };
    size_t length = sizeof( serializedPacket );

    result = Rtp_Init( &( ctx ) );
    TEST_ASSERT_EQUAL( RTP_RESULT_OK,
                       result );

    result = Rtp_DeSerialize( &( ctx ),
                              &( serializedPacket[0] ),
                              length,
                              &( packet ) );

    TEST_ASSERT_EQUAL( RTP_RESULT_OK,
                       result );

    TEST_ASSERT_EQUAL( RTP_HEADER_FLAG_EXTENSION,
                       packet.header.flags );

    TEST_ASSERT_EQUAL( 0,
                       packet.header.csrcCount );

    TEST_ASSERT_EQUAL( 0,
                       packet.header.extension.extensionProfile );

    TEST_ASSERT_EQUAL( 2,
                       packet.header.extension.extensionPayloadLength );

    TEST_ASSERT_EQUAL_HEX32( 0x11223344,
                             packet.header.extension.pExtensionPayload[0] );

    TEST_ASSERT_EQUAL_HEX32( 0x55667788,
                             packet.header.extension.pExtensionPayload[1] );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate Rtp_DeSerialize with a valid packet with padding.
 */
void test_Rtp_DeSerialize_Pass_WithPadding( void )
{
    RtpResult_t result;
    RtpContext_t ctx = { 0 };
    RtpPacket_t packet = { 0 };
    uint8_t serializedPacket[] = {
        /* RTP header with padding flag set */
        0xA0, 0x60, 0x04, 0xD2,
        /* Timestamp */
        0x12, 0x34, 0x56, 0x78,
        /* SSRC */
        0x87, 0x65, 0x43, 0x21,
        /* RTP payload */
        0x68, 0x65, 0x6C, 0x6C,
        0x6F, 0x20, 0x77, 0x6F,
        0x72, 0x6C, 0x64, 0x01
    };
    size_t length = sizeof( serializedPacket );

    result = Rtp_Init( &( ctx ) );

    TEST_ASSERT_EQUAL( RTP_RESULT_OK,
                       result );

    result = Rtp_DeSerialize( &( ctx ),
                              &( serializedPacket[0] ),
                              length,
                              &( packet ) );

    TEST_ASSERT_EQUAL( RTP_RESULT_OK,
                       result );

    TEST_ASSERT_EQUAL( RTP_HEADER_FLAG_PADDING,
                       packet.header.flags );

    TEST_ASSERT_EQUAL( 0,
                       packet.header.csrcCount );

    TEST_ASSERT_EQUAL( 11,
                       packet.payloadLength );

    TEST_ASSERT_EQUAL_STRING_LEN( "hello world",
                                  ( char * )packet.pPayload,
                                  packet.payloadLength );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate Rtp_DeSerialize with a malformed packet (incorrect version).
 */
void test_Rtp_DeSerialize_MalformedPacket_WrongVersion( void )
{
    RtpResult_t result;
    RtpContext_t ctx = { 0 };
    RtpPacket_t packet = { 0 };
    uint8_t serializedPacket[] = {
        /* RTP header with wrong version. */
        0x00, 0x60, 0x04, 0xD2,
        /* Timestamp */
        0x12, 0x34, 0x56, 0x78,
        /* SSRC */
        0x87, 0x65, 0x43, 0x21,
        /* RTP payload */
        0x68, 0x65, 0x6C, 0x6C,
        0x6F, 0x20, 0x77, 0x6F,
        0x72, 0x64
    };
    size_t length = sizeof( serializedPacket );

    result = Rtp_Init( &( ctx ) );

    TEST_ASSERT_EQUAL( RTP_RESULT_OK,
                       result );

    result = Rtp_DeSerialize( &( ctx ),
                              &( serializedPacket[0] ),
                              length,
                              &( packet ) );

    TEST_ASSERT_EQUAL( RTP_RESULT_WRONG_VERSION,
                       result );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate Rtp_DeSerialize with a malformed packet (no extension header specified).
 */
void test_Rtp_DeSerialize_MalformedPacket_NoExtensionHeader( void )
{
    RtpResult_t result;
    RtpContext_t ctx = { 0 };
    RtpPacket_t packet = { 0 };
    uint8_t serializedPacket[] = {
        /* RTP header with extension flag set */
        0x90, 0x60, 0x04, 0xD2,
        /* Timestamp */
        0x12, 0x34, 0x56, 0x78,
        /* SSRC */
        0x87, 0x65, 0x43, 0x21,
        /* Extension header missing */
    };
    size_t length = sizeof( serializedPacket );

    result = Rtp_Init( &( ctx ) );

    TEST_ASSERT_EQUAL( RTP_RESULT_OK,
                       result );

    result = Rtp_DeSerialize( &( ctx ),
                              &( serializedPacket[0] ),
                              length,
                              &( packet ) );

    TEST_ASSERT_EQUAL( RTP_RESULT_MALFORMED_PACKET,
                       result );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate Rtp_DeSerialize with insufficient data to read CSRC identifiers.
 */
void test_Rtp_DeSerialize_InsufficientCsrcData( void )
{
    RtpResult_t result;
    RtpContext_t ctx = { 0 };
    RtpPacket_t packet = { 0 };
    uint8_t serializedPacket[] = {
        /* RTP header with CSRC count = 2 */
        0x82, 0x60, 0x04, 0xD2,
        /* Timestamp */
        0x12, 0x34, 0x56, 0x78,
        /* SSRC */
        0x87, 0x65, 0x43, 0x21,
        /* CSRC 1 */
        0x11, 0x22, 0x33, 0x44,
        // Only 1 CSRC identifier (not enough for count = 2)
    };
    size_t length = sizeof( serializedPacket );

    result = Rtp_Init( &( ctx ) );

    TEST_ASSERT_EQUAL( RTP_RESULT_OK,
                       result );

    result = Rtp_DeSerialize( &( ctx ),
                              &( serializedPacket[0] ),
                              length,
                              &( packet ) );

    TEST_ASSERT_EQUAL( RTP_RESULT_MALFORMED_PACKET,
                       result );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate Rtp_DeSerialize with insufficient data to read extension payload.
 */
void test_Rtp_DeSerialize_InsufficientExtensionPayloadData( void )
{
    RtpResult_t result;
    RtpContext_t ctx = { 0 };
    RtpPacket_t packet = { 0 };
    uint8_t serializedPacket[] = {
        /* RTP header with extension flag set. */
        0x90, 0x60, 0x04, 0xD2,
        /* Timestamp */
        0x12, 0x34, 0x56, 0x78,
        /* SSRC */
        0x87, 0x65, 0x43, 0x21,
        /* Extension header */
        0x00, 0x00, 0x00, 0x02, // Extension header (profile = 0, length = 2)
        /* Extension payload */
        0x11, 0x22, 0x33, 0x44 // Only 1 extension payload word (not enough for length = 2)
    };
    size_t length = sizeof( serializedPacket );

    result = Rtp_Init( &( ctx ) );

    TEST_ASSERT_EQUAL( RTP_RESULT_OK,
                       result );

    result = Rtp_DeSerialize( &( ctx ),
                              &( serializedPacket[0] ),
                              length,
                              &( packet ) );

    TEST_ASSERT_EQUAL( RTP_RESULT_MALFORMED_PACKET,
                       result );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate Rtp_DeSerialize with no payload data.
 */
void test_Rtp_DeSerialize_NoPayload( void )
{
    RtpResult_t result;
    RtpContext_t ctx = { 0 };
    RtpPacket_t packet = { 0 };
    uint8_t serializedPacket[] = {
        /* RTP header with marker bit not set. */
        0x80, 0x60, 0x04, 0xD2,
        /* Timestamp */
        0x12, 0x34, 0x56, 0x78,
        /* SSRC */
        0x87, 0x65, 0x43, 0x21
    };
    size_t length = sizeof( serializedPacket );

    result = Rtp_Init( &( ctx ) );

    TEST_ASSERT_EQUAL( RTP_RESULT_OK,
                       result );

    result = Rtp_DeSerialize( &( ctx ),
                              &( serializedPacket[0] ),
                              length,
                              &( packet ) );

    TEST_ASSERT_EQUAL( RTP_RESULT_OK,
                       result );

    TEST_ASSERT_NULL( packet.pPayload );

    TEST_ASSERT_EQUAL( 0,
                       packet.payloadLength );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate Rtp_DeSerialize with the marker bit not set.
 */
void test_Rtp_DeSerialize_MarkerBitNotSet( void )
{
    RtpResult_t result;
    RtpContext_t ctx = { 0 };
    RtpPacket_t packet = { 0 };
    uint8_t serializedPacket[] = {
        /* RTP header with marker bit not set. */
        0x80, 0x80, 0x04, 0xD2,
        /* Timestamp */
        0x12, 0x34, 0x56, 0x78,
        /* SSRC */
        0x87, 0x65, 0x43, 0x21,
        /* Payload */
        0x68, 0x65, 0x6C, 0x6C,
        0x6F, 0x20, 0x77, 0x6F,
        0x72, 0x6C, 0x64, 0x21
    };
    size_t length = sizeof( serializedPacket );
    size_t payloadLength = length - 12; /* 12 bytes for RTP header, 0 bytes for padding. */

    result = Rtp_Init( &( ctx ) );

    TEST_ASSERT_EQUAL( RTP_RESULT_OK,
                       result );

    result = Rtp_DeSerialize( &( ctx ),
                              &( serializedPacket[0] ),
                              length,
                              &( packet ) );

    TEST_ASSERT_EQUAL( RTP_RESULT_OK,
                       result );

    TEST_ASSERT_EQUAL( payloadLength,
                       packet.payloadLength );

    TEST_ASSERT_EQUAL_STRING_LEN( "hello world!",
                                  ( char * )packet.pPayload,
                                  packet.payloadLength );
}

/*-----------------------------------------------------------*/

