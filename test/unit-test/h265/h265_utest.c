/* Unity includes. */
#include "unity.h"
#include "catch_assert.h"

/* Standard includes. */
#include <string.h>
#include <stdint.h>

/* API includes. */
#include "h265_packetizer.h"
#include "h265_depacketizer.h"

/* ===========================  EXTERN VARIABLES  =========================== */

#define MAX_NALU_LENGTH           5 * 1024
#define MAX_FRAME_LENGTH          10 * 1024
#define MAX_NALUS_IN_A_FRAME      512
#define MAX_H265_PACKET_LENGTH    1500

/* Global buffers */
uint8_t frameBuffer[ MAX_FRAME_LENGTH ];
uint8_t packetBuffer[ MAX_H265_PACKET_LENGTH ];

/* Test setup and teardown */
void setUp( void )
{
    memset( frameBuffer, 0, sizeof( frameBuffer ) );
    memset( packetBuffer, 0, sizeof( packetBuffer ) );
}

void tearDown( void )
{
    /* Nothing to clean up */
}

/* ==============================  Test Cases for Packetization ============================== */

/**
 * @brief Test H265 packetization with single NAL unit.
 */
void test_H265_Packetizer_SingleNal( void )
{
    H265PacketizerContext_t ctx;
    H265Result_t result;
    H265Nalu_t naluArray[ MAX_NALUS_IN_A_FRAME ];
    H265Packet_t packet;

    uint8_t nalData[] =
    {
        0x42, 0x01,      /* NAL header (Type=32/VPS) */
        0x01, 0x02, 0x03 /* NAL payload */
    };

    result = H265Packetizer_Init( &ctx, naluArray,  MAX_NALUS_IN_A_FRAME);
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    H265Nalu_t nalu =
    {
        .pNaluData      = nalData,
        .naluDataLength = sizeof( nalData ),
        .nalUnitType  = 32, /* VPS */
        .temporalId    = 1
    };
    result = H265Packetizer_AddNalu( &ctx, &nalu );
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );
    TEST_ASSERT_EQUAL( 1, ctx.naluCount );

    packet.pPacketData = packetBuffer;
    packet.packetDataLength = MAX_H265_PACKET_LENGTH;
    result = H265Packetizer_GetPacket( &ctx, &packet );
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );
}

/*-----------------------------------------------------------------------------------------------------*/

/**
 * @brief Test H265 packetization with fragmentation.
 */
void test_H265_Packetizer_Two_Fragment_Packets(void)
{
    H265PacketizerContext_t ctx;
    H265Result_t result;
    H265Nalu_t naluArray[10];
    H265Packet_t packet = { 0 };
    uint8_t packetBuffer[6];

    result = H265Packetizer_Init(&ctx, naluArray, 10);
    TEST_ASSERT_EQUAL(H265_RESULT_OK, result);

    /* Create test NALU: 2 bytes header + 6 bytes payload = 8 bytes total */
    uint8_t naluData[8] = {
        0x40, 0x01,         /* NAL header */
        0xAA, 0xBB, 0xCC,   /* First 3 bytes of payload */
        0xDD, 0xEE, 0xFF    /* Last 3 bytes of payload */
    };

    H265Nalu_t testNalu = { 0 };
    testNalu.pNaluData = naluData;
    testNalu.naluDataLength = sizeof(naluData);
    result = H265Packetizer_AddNalu(&ctx, &testNalu);
    TEST_ASSERT_EQUAL(H265_RESULT_OK, result);

    /* Get first fragment */
    packet.pPacketData = packetBuffer;
    packet.packetDataLength = 6;  /* 2(PayloadHdr) + 1(FUHeader) + 3(payload) */
    result = H265Packetizer_GetPacket(&ctx, &packet);
    TEST_ASSERT_EQUAL(H265_RESULT_OK, result);

    /* Get second fragment */
    packet.packetDataLength = 6;  /* 2(PayloadHdr) + 1(FUHeader) + 3(payload) */
    result = H265Packetizer_GetPacket(&ctx, &packet);
    TEST_ASSERT_EQUAL(H265_RESULT_OK, result);
}

/*-----------------------------------------------------------------------------------------------------*/

/**
 * @brief Test H265 packetization with fragmentation and zero payload size.
 */
void test_H265_Packetizer_Fragmentation_Zero_PayloadSize( void )
{
    H265PacketizerContext_t ctx;
    H265Result_t result;
    H265Nalu_t naluArray[ 10 ];
    H265Packet_t packet = { 0 };
    uint8_t packetBuffer[ 10 ];  /* Small buffer to force zero payload size */

    result = H265Packetizer_Init( &ctx, naluArray, 10);
    TEST_ASSERT_EQUAL( 0, result );

    uint8_t naluData[ 100 ] = { 0 };
    naluData[ 0 ] = 0x62;             /* NAL header first byte */
    naluData[ 1 ] = 0x01;             /* NAL header second byte */
    memset( naluData + 2, 0xAA, 98 ); /* Fill rest with data */

    H265Nalu_t testNalu = { 0 };
    testNalu.pNaluData = naluData;
    testNalu.naluDataLength = sizeof( naluData );
    result = H265Packetizer_AddNalu( &ctx, &testNalu );
    TEST_ASSERT_EQUAL( 0, result );

    testNalu.pNaluData = naluData;
    testNalu.naluDataLength = 0;
    result = H265Packetizer_AddNalu( &ctx, &testNalu );
    TEST_ASSERT_EQUAL( 1, result );

    testNalu.pNaluData = NULL;
    result = H265Packetizer_AddNalu( &ctx, &testNalu );
    TEST_ASSERT_EQUAL( 1, result );

    result = H265Packetizer_AddNalu( &ctx, NULL );
    TEST_ASSERT_EQUAL( 1, result );

    result = H265Packetizer_AddNalu( NULL, NULL );
    TEST_ASSERT_EQUAL( 1, result );

    /* Setup packet with minimal size to force zero payload */
    packet.pPacketData = packetBuffer;
    packet.packetDataLength = 3;  /* Just enough for headers (2 bytes PayloadHdr + 1 byte FUHeader) */
                               /* This should make maxPayloadSize = 0 */

    result = H265Packetizer_GetPacket( &ctx, &packet );
    TEST_ASSERT_EQUAL( H265_RESULT_OUT_OF_MEMORY, result );
}

/*-----------------------------------------------------------------------------------------------------*/

/**
 * @brief Test H265 packetization with simple aggregation.
 */
void test_H265_Packetizer_SimpleAggregation( void )
{
    H265PacketizerContext_t ctx;
    H265Result_t result;
    H265Nalu_t naluArray[ MAX_NALUS_IN_A_FRAME ];
    H265Packet_t packet;

    uint8_t nalData1[] =
    {
        0x40, 0x01,     /* NAL header */
        0xAA, 0xBB      /* Payload */
    };

    uint8_t nalData2[] =
    {
        0x42, 0x01,     /* NAL header */
        0xCC, 0xDD      /* Payload */
    };

    result = H265Packetizer_Init( &ctx, naluArray, MAX_NALUS_IN_A_FRAME);
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    H265Nalu_t nalu1 =
    {
        .pNaluData      = nalData1,
        .naluDataLength = sizeof( nalData1 ),
        .nalUnitType  = 32,
        .temporalId    = 1
    };
    result = H265Packetizer_AddNalu( &ctx, &nalu1 );
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    H265Nalu_t nalu2 =
    {
        .pNaluData      = nalData2,
        .naluDataLength = sizeof( nalData2 ),
        .nalUnitType  = 33,
        .temporalId    = 1
    };
    result = H265Packetizer_AddNalu( &ctx, &nalu2 );
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    packet.pPacketData = packetBuffer;
    packet.packetDataLength = 100;
    result = H265Packetizer_GetPacket( &ctx, &packet );
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );
}

/*-----------------------------------------------------------------------------------------------------*/

/**
 * @brief Test H265 packetization with aggregation and insufficient NALUs.
 */
void test_H265_Packetizer_Aggregation_Insufficient_Nalus( void )
{
    H265PacketizerContext_t ctx;
    H265Result_t result;
    H265Nalu_t naluArray[ 10 ];
    H265Packet_t packet = { 0 };
    uint8_t packetBuffer[ 20 ];  /* Small buffer to limit aggregation */

    result = H265Packetizer_Init( &ctx,
                                  naluArray,
                                  10);    /* No DON handling */
    TEST_ASSERT_EQUAL( 0, result );

    uint8_t naluData1[] =
    {
        0x40, 0x01,  /* NAL header */
        0x02, 0x03   /* Small payload */
    };

    uint8_t naluData2[] =
    {
        0x40, 0x01,                         /* NAL header */
        0x04, 0x05, 0x06, 0x07, 0x08, 0x09, /* Large payload */
        0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F  /* Make it big */
    };

    H265Nalu_t testNalu1 = { 0 };
    testNalu1.pNaluData = naluData1;
    testNalu1.naluDataLength = sizeof( naluData1 );

    H265Nalu_t testNalu2 = { 0 };
    testNalu2.pNaluData = naluData2;
    testNalu2.naluDataLength = sizeof( naluData2 );

    result = H265Packetizer_AddNalu( &ctx, &testNalu1 );
    TEST_ASSERT_EQUAL( 0, result );

    result = H265Packetizer_AddNalu( &ctx, &testNalu2 );
    TEST_ASSERT_EQUAL( 0, result );

    packet.pPacketData = packetBuffer;
    packet.packetDataLength = 20;  /* Small enough that second NAL won't fit */
    result = H265Packetizer_GetPacket( &ctx, &packet );
    TEST_ASSERT_EQUAL( 0, result );
}

/*-----------------------------------------------------------------------------------------------------*/

/**
 * @brief Test H265 packetization AddFrame with multiple NAL units.
 */
void test_H265_Packetizer_AddFrame( void )
{
    H265PacketizerContext_t ctx;
    H265Result_t result;
    H265Nalu_t naluArray[ MAX_NALUS_IN_A_FRAME ];
    H265Frame_t frame;

    /* Test NULL context */
    uint8_t dummyData[] = { 0x00, 0x00, 0x01 };

    frame.pFrameData = dummyData;
    frame.frameDataLength = sizeof( dummyData );
    result = H265Packetizer_AddFrame( NULL, &frame );
    TEST_ASSERT_EQUAL( H265_RESULT_BAD_PARAM, result );

    /* Test NULL frame */
    result = H265Packetizer_Init( &ctx, naluArray, MAX_NALUS_IN_A_FRAME);
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );
    result = H265Packetizer_AddFrame( &ctx, NULL );
    TEST_ASSERT_EQUAL( H265_RESULT_BAD_PARAM, result );

    /* Test NULL frame data */
    frame.pFrameData = NULL;
    frame.frameDataLength = 10;
    result = H265Packetizer_AddFrame( &ctx, &frame );
    TEST_ASSERT_EQUAL( H265_RESULT_BAD_PARAM, result );

    /* Test zero length */
    frame.pFrameData = dummyData;
    frame.frameDataLength = 0;
    result = H265Packetizer_AddFrame( &ctx, &frame );
    TEST_ASSERT_EQUAL( H265_RESULT_BAD_PARAM, result );

    uint8_t frameData4[] =
    {
        0x00, 0x00, 0x00, 0x01,         /* Start code */
        0x40, 0x01, 0xAA, 0xBB,         /* VPS NAL */
        0x00, 0x00, 0x00, 0x01,         /* Start code */
        0x42, 0x01, 0xCC, 0xDD,         /* SPS NAL */
        0x00, 0x00, 0x00, 0x01,         /* Start code */
        0x44, 0x01, 0xEE, 0xFF          /* PPS NAL */
    };

    result = H265Packetizer_Init( &ctx, naluArray, MAX_NALUS_IN_A_FRAME );
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    frame.pFrameData = frameData4;
    frame.frameDataLength = sizeof( frameData4 );
    result = H265Packetizer_AddFrame( &ctx, &frame );
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );
    TEST_ASSERT_EQUAL( 3, ctx.naluCount );

    uint8_t frameData3[] =
    {
        0x00, 0x00, 0x01,               /* Start code */
        0x40, 0x01, 0xAA, 0xBB,         /* VPS NAL */
        0x00, 0x00, 0x01,               /* Start code */
        0x42, 0x01, 0xCC, 0xDD,         /* SPS NAL */
        0x00, 0x00, 0x01,               /* Start code */
        0x44, 0x01, 0xEE, 0xFF          /* PPS NAL */
    };

    result = H265Packetizer_Init( &ctx, naluArray, MAX_NALUS_IN_A_FRAME);
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    frame.pFrameData = frameData3;
    frame.frameDataLength = sizeof( frameData3 );
    result = H265Packetizer_AddFrame( &ctx, &frame );
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );
    TEST_ASSERT_EQUAL( 3, ctx.naluCount );

    uint8_t frameDataMixed[] =
    {
        0x00, 0x00, 0x00, 0x01,         /* 4-byte start code */
        0x40, 0x01, 0xAA, 0xBB,         /* VPS NAL */
        0x00, 0x00, 0x01,               /* 3-byte start code */
        0x42, 0x01, 0xCC, 0xDD,         /* SPS NAL */
        0x00, 0x00, 0x00, 0x01,         /* 4-byte start code */
        0x44, 0x01, 0xEE, 0xFF          /* PPS NAL */
    };

    result = H265Packetizer_Init( &ctx, naluArray, MAX_NALUS_IN_A_FRAME);
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    frame.pFrameData = frameDataMixed;
    frame.frameDataLength = sizeof( frameDataMixed );
    result = H265Packetizer_AddFrame( &ctx, &frame );
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );
    TEST_ASSERT_EQUAL( 3, ctx.naluCount );

    uint8_t malformedFrame[] =
    {
        0x00, 0x00, 0x00, 0x01, /* Start code */
        0x40,                   /* Incomplete NAL header */
        0x00, 0x00, 0x00, 0x01, /* Start code */
        0x42, 0x01, 0xCC, 0xDD  /* Valid NAL */
    };

    result = H265Packetizer_Init( &ctx, naluArray, MAX_NALUS_IN_A_FRAME);
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    frame.pFrameData = malformedFrame;
    frame.frameDataLength = sizeof( malformedFrame );
    result = H265Packetizer_AddFrame( &ctx, &frame );
    TEST_ASSERT_EQUAL( H265_RESULT_MALFORMED_PACKET, result );

    uint8_t frameWithFields[] =
    {
        0x00, 0x00, 0x01,               /* Start code */
        0x40, 0x01,                     /* NAL with type=32, layer_id=0, tid=1 */
        0xAA, 0xBB                      /* Payload */
    };

    result = H265Packetizer_Init( &ctx, naluArray, 0);
    TEST_ASSERT_EQUAL( H265_RESULT_BAD_PARAM, result );

    result = H265Packetizer_Init( &ctx, NULL, 0);
    TEST_ASSERT_EQUAL( H265_RESULT_BAD_PARAM, result );

    result = H265Packetizer_Init( NULL, naluArray, 0);
    TEST_ASSERT_EQUAL( H265_RESULT_BAD_PARAM, result );

    result = H265Packetizer_Init( &ctx, naluArray, MAX_NALUS_IN_A_FRAME );
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    frame.pFrameData = frameWithFields;
    frame.frameDataLength = sizeof( frameWithFields );
    result = H265Packetizer_AddFrame( &ctx, &frame );
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );
    TEST_ASSERT_EQUAL( 1, ctx.naluCount );
    TEST_ASSERT_EQUAL( 32, ctx.pNaluArray[ 0 ].nalUnitType );
    TEST_ASSERT_EQUAL( 1, ctx.pNaluArray[ 0 ].temporalId );
}

/*-----------------------------------------------------------------------------------------------------*/

/**
 * @brief Test H265 packetization AddFrame with malformed start code.
 */
void test_H265_Packetizer_AddFrame_Malformed_Three_Byte_StartCode( void )
{
    H265PacketizerContext_t ctx;
    H265Result_t result;
    H265Nalu_t naluArray[ 10 ];
    H265Frame_t frame = { 0 };

    result = H265Packetizer_Init( &ctx,
                                  naluArray,
                                  10);
    TEST_ASSERT_EQUAL( 0, result );

    uint8_t frameData[] =
    {
        0x00, 0x00, 0x01,        /* First start code (3-byte) */
        0x40, 0x01, 0x02, 0x03,  /* Valid first NAL */

        0x00, 0x00, 0x01, /* Second start code (3-byte) */
        0x41,             /* Single byte NAL (malformed) */

        0x00, 0x00, 0x01,        /* Third start code (3-byte) */
        0x42, 0x01, 0x04, 0x05   /* Final valid NAL */
    };

    frame.pFrameData = frameData;
    frame.frameDataLength = sizeof( frameData );
    result = H265Packetizer_AddFrame( &ctx, &frame );
    TEST_ASSERT_EQUAL( H265_RESULT_MALFORMED_PACKET, result );
}

/*-----------------------------------------------------------------------------------------------------*/

/**
 * @brief Test H265 packetization AddFrame with malformed last NAL unit.
 */
void test_H265_Packetizer_AddFrame_Malformed_Last_Nalu( void )
{
    H265PacketizerContext_t ctx;
    H265Result_t result;
    H265Nalu_t naluArray[ 10 ];
    H265Frame_t frame = { 0 };

    result = H265Packetizer_Init( &ctx,
                                  naluArray,
                                  10);
    TEST_ASSERT_EQUAL( 0, result );

    uint8_t frameData[] =
    {
        0x00, 0x00, 0x00, 0x01,  /* First start code */
        0x40, 0x01, 0x02, 0x03,  /* Valid first NAL unit */
        0x00, 0x00, 0x00, 0x01,  /* Second start code */
        0x40                     /* Malformed NAL unit (only 1 byte, less than NALU_HEADER_SIZE) */
    };

    frame.pFrameData = frameData;
    frame.frameDataLength = sizeof( frameData );
    result = H265Packetizer_AddFrame( &ctx, &frame );
    TEST_ASSERT_EQUAL( H265_RESULT_MALFORMED_PACKET, result );
}

/*-----------------------------------------------------------------------------------------------------*/

/**
 * @brief Test H265 packetization AddFrame with zero NALU start index.
 */
void test_H265_Packetizer_AddFrame_Zero_NaluStartIndex( void )
{
    H265PacketizerContext_t ctx;
    H265Result_t result;
    H265Nalu_t naluArray[ 10 ];
    H265Frame_t frame = { 0 };

    /* Initialize context */
    result = H265Packetizer_Init( &ctx,
                                  naluArray,
                                  10);
    TEST_ASSERT_EQUAL( 0, result );

    /* Create frame data without any start codes */
    uint8_t frameData[] =
    {
        0x40, 0x01,            /* Just NAL header bytes */
        0x02, 0x03, 0x04, 0x05 /* Some payload data */
    };

    /* Setup frame */
    frame.pFrameData = frameData;
    frame.frameDataLength = sizeof( frameData );
    result = H265Packetizer_AddFrame( &ctx, &frame );
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );
    TEST_ASSERT_EQUAL( 0, ctx.naluCount );
}

/*-----------------------------------------------------------------------------------------------------*/

/**
 * @brief Test H265 packetization add NAL unit with malformed packets.
 */
void test_H265_Packetizer_AddNalu_Malformed_Packet( void )
{
    H265PacketizerContext_t ctx;
    H265Result_t result;
    H265Nalu_t naluArray[ 10 ];
    H265Nalu_t testNalu = { 0 };

    result = H265Packetizer_Init( &ctx,
                                  naluArray,
                                  10);
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    /* Test Case 1: NAL unit smaller than NALU_HEADER_SIZE */
    uint8_t smallNaluData[] = { 0x40 };  /* Just 1 byte */
    testNalu.pNaluData = smallNaluData;
    testNalu.naluDataLength = sizeof( smallNaluData );
    result = H265Packetizer_AddNalu( &ctx, &testNalu );
    TEST_ASSERT_EQUAL( H265_RESULT_MALFORMED_PACKET, result );

    /* Test Case 2: Invalid NAL unit type */
    uint8_t validSizeData[] = { 0x40, 0x01, 0x02, 0x03 };
    testNalu.pNaluData = validSizeData;
    testNalu.naluDataLength = sizeof( validSizeData );
    testNalu.nalUnitType = NALU_HEADER_TYPE_MAX_VALUE + 1;  /* Invalid type */
    testNalu.temporalId = 0;
    result = H265Packetizer_AddNalu( &ctx, &testNalu );
    TEST_ASSERT_EQUAL( H265_RESULT_BAD_PARAM, result );

    /* Test Case 3: Invalid temporal ID */
    testNalu.nalUnitType = 32;                /* Valid type */
    testNalu.temporalId = NALU_HEADER_TID_MAX_VALUE + 1; /* Invalid temporal ID */
    result = H265Packetizer_AddNalu( &ctx, &testNalu );
    TEST_ASSERT_EQUAL( H265_RESULT_BAD_PARAM, result );
}

/*-----------------------------------------------------------------------------------------------------*/

/**
 * @brief Test H265 packetization add NAL unit with array overflow.
 */
void test_H265_Packetizer_AddNalu_Array_Full( void )
{
    H265PacketizerContext_t ctx;
    H265Result_t result;
    H265Nalu_t naluArray[ 2 ];  /* Very small array to trigger overflow */
    H265Nalu_t testNalu = { 0 };

    result = H265Packetizer_Init( &ctx,
                                  naluArray,
                                  2 );         /* Small array length to trigger overflow */
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    uint8_t naluData[] =
    {
        0x40, 0x01, 0x02, 0x03  /* Valid NAL unit data (assuming NALU_HEADER_SIZE = 2) */
    };

    testNalu.pNaluData = naluData;
    testNalu.naluDataLength = sizeof( naluData );

    /* Add NALs until array is full */
    result = H265Packetizer_AddNalu( &ctx, &testNalu );  /* First add */
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    result = H265Packetizer_AddNalu( &ctx, &testNalu );  /* Second add */
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    /* Try to add one more NAL - should fail */
    result = H265Packetizer_AddNalu( &ctx, &testNalu );  /* Third add - should fail */
    TEST_ASSERT_EQUAL( H265_RESULT_OUT_OF_MEMORY, result );
}

/*-----------------------------------------------------------------------------------------------------*/

/**
 * @brief Test H265 packetization GetPacket with small buffer size.
 */
void test_H265_Packetizer_GetPacket_Small_MaxSize( void )
{
    H265PacketizerContext_t ctx;
    H265Result_t result;
    H265Nalu_t naluArray[ 10 ];
    H265Packet_t packet = { 0 };
    uint8_t packetBuffer[ 2 ];  /* Very small buffer, less than NALU_HEADER_SIZE */

    result = H265Packetizer_Init( &ctx,
                                  naluArray,
                                  10);
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    packet.pPacketData = packetBuffer;
    packet.packetDataLength = 1;  /* Set size less than NALU_HEADER_SIZE */
    result = H265Packetizer_GetPacket( &ctx, &packet );
    TEST_ASSERT_EQUAL( H265_RESULT_BAD_PARAM, result );
}

/*-----------------------------------------------------------------------------------------------------*/

/**
 * @brief Test H265 packetization GetPacket with size overflow.
 */
void test_H265_Packetizer_GetPacket_Size_Overflow( void )
{
    H265PacketizerContext_t ctx;
    H265Result_t result;
    H265Nalu_t naluArray[ 10 ];
    H265Packet_t packet = { 0 };
    uint8_t packetBuffer[ 100 ];  /* Small buffer to trigger overflow */

    result = H265Packetizer_Init( &ctx,
                                  naluArray,
                                  10);
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    /* Create multiple NAL units with increasing sizes */
    uint8_t nalu1Data[] = { 0x40, 0x01, 0x02, 0x03 }; /* Small NAL */
    uint8_t nalu2Data[ 50 ] = { 0 };                  /* Medium NAL */
    uint8_t nalu3Data[ 80 ] = { 0 };                  /* Large NAL - should cause overflow */

    /* Setup first NAL */
    H265Nalu_t testNalu1 = { 0 };
    testNalu1.pNaluData = nalu1Data;
    testNalu1.naluDataLength = sizeof( nalu1Data );

    /* Setup second NAL */
    H265Nalu_t testNalu2 = { 0 };
    testNalu2.pNaluData = nalu2Data;
    testNalu2.naluDataLength = sizeof( nalu2Data );

    /* Setup third NAL */
    H265Nalu_t testNalu3 = { 0 };
    testNalu3.pNaluData = nalu3Data;
    testNalu3.naluDataLength = sizeof( nalu3Data );

    result = H265Packetizer_AddNalu( &ctx, &testNalu1 );
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    result = H265Packetizer_AddNalu( &ctx, &testNalu2 );
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    result = H265Packetizer_AddNalu( &ctx, &testNalu3 );
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    packet.pPacketData = packetBuffer;
    packet.packetDataLength = 100;  /* Small enough to trigger overflow */
    result = H265Packetizer_GetPacket( &ctx, &packet );
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );
}

/*-----------------------------------------------------------------------------------------------------*/

/**
 * @brief Test H265 packetization GetPacket with zero max size.
 */
void test_H265_Packetizer_GetPacket_Zero_MaxSize( void )
{
    H265PacketizerContext_t ctx;
    H265Result_t result;
    H265Nalu_t naluArray[ 10 ];
    H265Packet_t packet = { 0 };
    uint8_t packetBuffer[ 100 ];

    /* Initialize context */
    result = H265Packetizer_Init( &ctx,
                                  naluArray,
                                  10);
    TEST_ASSERT_EQUAL( 0, result );

    uint8_t naluData[] =
    {
        0x40, 0x01,  /* NAL header */
        0x02, 0x03   /* Some payload data */
    };

    H265Nalu_t testNalu = { 0 };
    testNalu.pNaluData = naluData;
    testNalu.naluDataLength = sizeof( naluData );
    result = H265Packetizer_AddNalu( &ctx, &testNalu );
    TEST_ASSERT_EQUAL( 0, result );

    packet.pPacketData = packetBuffer;
    packet.packetDataLength = 0;
    result = H265Packetizer_GetPacket( &ctx, &packet );
    TEST_ASSERT_EQUAL( H265_RESULT_BAD_PARAM, result );
}

/*-----------------------------------------------------------------------------------------------------*/

/**
 * @brief Test H265 packetization GetPacket with aggregation array length limit.
 */
void test_H265_Packetizer_GetPacket_Aggregation_Array_Limit( void )
{
    H265PacketizerContext_t ctx;
    H265Result_t result;
    H265Nalu_t naluArray[ 3 ];  /* Small array to hit the length limit */
    H265Packet_t packet = { 0 };
    uint8_t packetBuffer[ 1500 ];

    result = H265Packetizer_Init( &ctx,
                                  naluArray,
                                  3 );      /* Small naluArrayLength */
    TEST_ASSERT_EQUAL( 0, result );

    uint8_t naluData1[] =
    {
        0x40, 0x01,  /* NAL header */
        0x02, 0x03   /* Payload */
    };
    uint8_t naluData2[] =
    {
        0x40, 0x01,  /* NAL header */
        0x04, 0x05   /* Payload */
    };
    uint8_t naluData3[] =
    {
        0x40, 0x01,  /* NAL header */
        0x06, 0x07   /* Payload */
    };

    H265Nalu_t testNalu1 = { 0 };
    testNalu1.pNaluData = naluData1;
    testNalu1.naluDataLength = sizeof( naluData1 );

    H265Nalu_t testNalu2 = { 0 };
    testNalu2.pNaluData = naluData2;
    testNalu2.naluDataLength = sizeof( naluData2 );

    H265Nalu_t testNalu3 = { 0 };
    testNalu3.pNaluData = naluData3;
    testNalu3.naluDataLength = sizeof( naluData3 );

    result = H265Packetizer_AddNalu( &ctx, &testNalu1 );
    TEST_ASSERT_EQUAL( 0, result );

    result = H265Packetizer_AddNalu( &ctx, &testNalu2 );
    TEST_ASSERT_EQUAL( 0, result );

    result = H265Packetizer_AddNalu( &ctx, &testNalu3 );
    TEST_ASSERT_EQUAL( 0, result );

    packet.pPacketData = packetBuffer;
    packet.packetDataLength = 1500;  /* Large enough for all NALs */
    result = H265Packetizer_GetPacket( &ctx, &packet );      /* Get packet - should try to aggregate all NALs and hit array length limit */
    TEST_ASSERT_EQUAL( 0, result );
}

/*-----------------------------------------------------------------------------------------------------*/

/**
 * @brief Test H265 packetization GetPacket with tail at the end of the array.
 */
void test_H265_Packetizer_GetPacket_Tail_At_End( void )
{
    H265PacketizerContext_t ctx;
    H265Result_t result;
    H265Nalu_t naluArray[ 2 ];  /* Small array size of 2 */
    H265Packet_t packet = { 0 };
    uint8_t packetBuffer[ 1500 ];

    result = H265Packetizer_Init( &ctx,
                                  naluArray,
                                  2 );
    TEST_ASSERT_EQUAL( 0, result );

    /* Create small NAL units that would fit in single packet */
    uint8_t naluData[] =
    {
        0x40, 0x01,  /* NAL header */
        0x02, 0x03   /* Small payload */
    };

    H265Nalu_t testNalu = { 0 };
    testNalu.pNaluData = naluData;
    testNalu.naluDataLength = sizeof( naluData );

    ctx.tailIndex = 1;              /* Set to last index (naluArrayLength - 1) */
    ctx.naluCount = 2;              /* Set count >= 2 to satisfy first condition */
    ctx.naluArrayLength = 2;        /* Confirm array length */

    /* Add NAL at the last position */
    naluArray[ ctx.tailIndex ] = testNalu;

    packet.pPacketData = packetBuffer;
    packet.packetDataLength = 1500;    /* Large enough for single NAL */
    result = H265Packetizer_GetPacket( &ctx, &packet );
    TEST_ASSERT_EQUAL( 0, result );
}

/*-----------------------------------------------------------------------------------------------------*/

/**
 * @brief Test H265 packetization GetPacket with valid context but invalid packet.
 */
void test_H265_Packetizer_GetPacket_Valid_Context_Invalid_Packet( void )
{
    H265PacketizerContext_t ctx;
    H265Result_t result;
    H265Nalu_t naluArray[ 10 ];
    H265Packet_t packet;
    uint8_t naluData[] =
    {
        0x40, 0x01,  /* NAL header */
        0x02, 0x03   /* Payload */
    };

    result = H265Packetizer_Init( &ctx,
                                  naluArray,
                                  10);
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    TEST_ASSERT_EQUAL(0, ctx.naluCount);
    packet.pPacketData = packetBuffer;
    packet.packetDataLength = MAX_H265_PACKET_LENGTH;
    result = H265Packetizer_GetPacket(&ctx, &packet);
    TEST_ASSERT_EQUAL( H265_RESULT_NO_MORE_PACKETS, result );

    H265Nalu_t testNalu = { 0 };
    testNalu.pNaluData = naluData;
    testNalu.naluDataLength = sizeof( naluData );
    result = H265Packetizer_AddNalu( &ctx, &testNalu );
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    result = H265Packetizer_GetPacket( &ctx, NULL );
    TEST_ASSERT_EQUAL( H265_RESULT_BAD_PARAM, result );

    result = H265Packetizer_GetPacket( NULL, NULL );
    TEST_ASSERT_EQUAL( H265_RESULT_BAD_PARAM, result );

    packet.pPacketData = NULL;           // Setting packet data to NULL
    packet.packetDataLength = MAX_H265_PACKET_LENGTH;  // Valid size
    result = H265Packetizer_GetPacket(&ctx, &packet);
    TEST_ASSERT_EQUAL(H265_RESULT_BAD_PARAM, result);
}

/* ==============================  Test Cases for Depacketization ============================== */

/**
 * @brief Test H265 depacketization with a single NALU.
 */
void test_H265_Depacketizer_SingleNALU_Complete(void)
{
    H265DepacketizerContext_t ctx;
    H265Result_t result;
    H265Packet_t packetsArray[10];
    H265Frame_t frame = {0};
    uint32_t properties;

    /* Initialize test packet - Single NALU */
    uint8_t singleNaluPacket[] =
    {
        0x26, 0x01,           /* NAL header (type 0x13) */
        0xAA, 0xBB, 0xCC      /* Payload */
    };

    /* Step 1: Get packet properties */
    result = H265Depacketizer_GetPacketProperties(singleNaluPacket,
                                                 sizeof(singleNaluPacket),
                                                 &properties);
    TEST_ASSERT_EQUAL(H265_RESULT_OK, result);

    /* Step 2: Initialize depacketizer */
    /* Initialize context and arrays */
    memset(&ctx, 0, sizeof(H265DepacketizerContext_t));
    memset(packetsArray, 0, sizeof(packetsArray));

    uint8_t frameBuffer[1500];
    memset(frameBuffer, 0, sizeof(frameBuffer));

    /* Initialize frame buffer */
    frame.pFrameData = frameBuffer;
    frame.frameDataLength = sizeof(frameBuffer);

    /* Initialize depacketizer */
    result = H265Depacketizer_Init(&ctx, packetsArray, 10);
    TEST_ASSERT_EQUAL(H265_RESULT_OK, result);

    /* Step 3: Add packet */
    packetsArray[0].pPacketData = singleNaluPacket;
    packetsArray[0].packetDataLength = sizeof(singleNaluPacket);
    result = H265Depacketizer_AddPacket(&ctx, &packetsArray[0]);
    TEST_ASSERT_EQUAL(H265_RESULT_OK, result);

    result = H265Depacketizer_GetFrame(&ctx, &frame);
    TEST_ASSERT_EQUAL(H265_RESULT_OK, result);
}

/*-----------------------------------------------------------------------------------------------------*/

/**
 * @brief Test H265 depacketization of fragmentation unit with end bit set.
 */
void test_H265Depacketizer_ProcessFragmentationUnit_EndBitSet(void)
{
    H265DepacketizerContext_t ctx;
    H265Result_t result;
    H265Packet_t packetsArray[2];  // We'll use 2 packets
    H265Nalu_t nalu;
    uint8_t naluBuffer[100];

    // Initialize context
    memset(&ctx, 0, sizeof(H265DepacketizerContext_t));
    ctx.pPacketsArray = packetsArray;
    ctx.packetCount = 2;  // Set initial packet count to 2
    ctx.tailIndex = 0;

    // Initialize NALU buffer
    memset(naluBuffer, 0, sizeof(naluBuffer));
    nalu.pNaluData = naluBuffer;
    nalu.naluDataLength = sizeof(naluBuffer);

    // Create first packet with End bit set (0x40)
    uint8_t firstPacketData[] = {
        0x62,       // First byte: Type 49 (FU)
        0x01,       // Second byte: LayerId and TID
        0x40,       // FU header with End bit set (0x40)
        0xAA, 0xBB  // Payload
    };
    packetsArray[0].pPacketData = firstPacketData;
    packetsArray[0].packetDataLength = sizeof(firstPacketData);

    // Create second packet (should not be processed due to End bit in first packet)
    uint8_t secondPacketData[] = {
        0x62,       // First byte: Type 49 (FU)
        0x01,       // Second byte: LayerId and TID
        0x00,       // FU header: no special bits set
        0xCC, 0xDD  // Payload
    };
    packetsArray[1].pPacketData = secondPacketData;
    packetsArray[1].packetDataLength = sizeof(secondPacketData);
    result = H265Depacketizer_GetNalu(&ctx, &nalu);
    TEST_ASSERT_EQUAL(H265_RESULT_OK, result);

    // Should only process first packet due to End bit
    TEST_ASSERT_EQUAL(1, ctx.tailIndex);
    TEST_ASSERT_EQUAL(1, ctx.packetCount);
    TEST_ASSERT_EQUAL(2, nalu.naluDataLength);    // TOTAL_FU_HEADER_SIZE is 3, so payload length is 2 (5 - 3)
}

/*-----------------------------------------------------------------------------------------------------*/

/**
 * @brief Test H265 depacketization of fragmentation unit with invalid second fragment.
 */
void test_H265Depacketizer_ProcessFragmentationUnit_InvalidSecondFragment(void)
{
    H265DepacketizerContext_t ctx;
    H265Result_t result;
    H265Packet_t packetsArray[2];
    H265Nalu_t nalu;

    // Initialize context
    memset(&ctx, 0, sizeof(H265DepacketizerContext_t));
    ctx.pPacketsArray = packetsArray;
    ctx.packetCount = 2;
    ctx.tailIndex = 0;

    // Initialize NALU buffer
    uint8_t naluBuffer[100];
    memset(naluBuffer, 0, sizeof(naluBuffer));
    nalu.pNaluData = naluBuffer;
    nalu.naluDataLength = sizeof(naluBuffer);

    // First fragment - valid FU packet (with Start bit set)
    uint8_t firstPacketData[] = {
        0x62,       // First byte: Type 49 (FU)
        0x01,       // Second byte: LayerId and TID
        0x80,       // FU header: Start bit set (0x80), type = 0
        0xAA, 0xBB  // Payload
    };
    packetsArray[0].pPacketData = firstPacketData;
    packetsArray[0].packetDataLength = sizeof(firstPacketData);

    // Second fragment - invalid type (not 49) with End bit set
    uint8_t secondPacketData[] = {
        0x42,       // First byte: Invalid type (not 49)
        0x01,       // Second byte: LayerId and TID
        0x40,       // FU header: End bit set (0x40), type = 0
        0xCC, 0xDD  // Payload
    };
    packetsArray[1].pPacketData = secondPacketData;
    packetsArray[1].packetDataLength = sizeof(secondPacketData);

    // Call the function directly
    result = H265Depacketizer_GetNalu(&ctx, &nalu);
    TEST_ASSERT_EQUAL(H265_RESULT_MALFORMED_PACKET, result);
}

/*-----------------------------------------------------------------------------------------------------*/

/**
 * @brief Test H265 depacketization of fragmentation unit with out of memory cases.
 */
void test_H265Depacketizer_ProcessFragmentation_OutOfMemory_Cases(void)
{
    H265DepacketizerContext_t ctx;
    H265Result_t result;
    H265Packet_t packetsArray[10];
    H265Nalu_t nalu;

    /* Test Case 1: Out of memory during header write */
    {
        memset(&ctx, 0, sizeof(H265DepacketizerContext_t));
        memset(packetsArray, 0, sizeof(packetsArray));

        /* Initialize NALU with very small buffer */
        uint8_t naluBuffer1[1];  // Deliberately small buffer
        nalu.pNaluData = naluBuffer1;
        nalu.naluDataLength = sizeof(naluBuffer1);  // Only 1 byte

        result = H265Depacketizer_Init(&ctx, packetsArray, 1);
        TEST_ASSERT_EQUAL(H265_RESULT_OK, result);

        /* Setup FU start fragment packet */
        uint8_t startFragment1[] = {
            0x62,   // NAL unit header byte 1: FU indicator
            0x01,   // NAL unit header byte 2
            0x82,   // FU header: Start bit (0x80) + NAL type (0x02)
            0xAA    // Payload
        };
        packetsArray[0].pPacketData = startFragment1;
        packetsArray[0].packetDataLength = sizeof(startFragment1);

        /* Set up context */
        ctx.packetCount = 1;
        ctx.tailIndex = 0;
        ctx.currentlyProcessingPacket = H265_PACKET_NONE;

        /* Process the packet */
        result = H265Depacketizer_GetNalu(&ctx, &nalu);
        TEST_ASSERT_EQUAL(H265_RESULT_OUT_OF_MEMORY, result);
    }

    /* Test Case 2: Out of memory during payload write */
    {
        /* Initialize structures for second test */
        memset(&ctx, 0, sizeof(H265DepacketizerContext_t));
        memset(packetsArray, 0, sizeof(packetsArray));

        /* Initialize NALU with small buffer */
        uint8_t naluBuffer2[5];
        nalu.pNaluData = naluBuffer2;
        nalu.naluDataLength = sizeof(naluBuffer2);

        /* Initialize test packets */
        uint8_t startFragment2[] = {
            0x62,   /* NAL unit header byte 1: FU indicator */
            0x01,   /* NAL unit header byte 2 */
            0x82,   /* FU header: Start bit(0x80) + NAL type(0x02) */
            0x11,   /* Small payload that will fit */
            0x22
        };

        uint8_t endFragment[] = {
            0x62,   /* NAL unit header byte 1 */
            0x01,   /* NAL unit header byte 2 */
            0x42,   /* FU header: End bit(0x40) + NAL type(0x02) */
            0x33,   /* Larger payload that will cause overflow */
            0x44,
            0x55,
            0x66,
            0x77
        };

        result = H265Depacketizer_Init(&ctx, packetsArray, 10);
        TEST_ASSERT_EQUAL(H265_RESULT_OK, result);

        /* Process start fragment */
        packetsArray[0].pPacketData = startFragment2;
        packetsArray[0].packetDataLength = sizeof(startFragment2);
        result = H265Depacketizer_AddPacket(&ctx, &packetsArray[0]);
        TEST_ASSERT_EQUAL(H265_RESULT_OK, result);

        ctx.packetCount = 1;

        /* Get first NALU */
        result = H265Depacketizer_GetNalu(&ctx, &nalu);
        TEST_ASSERT_EQUAL(H265_RESULT_OK, result);

        /* Process end fragment */
        packetsArray[0].pPacketData = endFragment;
        packetsArray[0].packetDataLength = sizeof(endFragment);
        result = H265Depacketizer_AddPacket(&ctx, &packetsArray[0]);
        TEST_ASSERT_EQUAL(H265_RESULT_OK, result);

        ctx.packetCount = 1;

        /* Try to get NALU - should return out of memory */
        result = H265Depacketizer_GetNalu(&ctx, &nalu);
        TEST_ASSERT_EQUAL(H265_RESULT_OUT_OF_MEMORY, result);
    }
}

/*-----------------------------------------------------------------------------------------------------*/

/**
 * @brief Test H265 depacketization of fragmentation unit with error cases.
 */
void test_H265_Depacketizer_FragmentationUnit_ErrorCases(void)
{
    H265DepacketizerContext_t ctx;
    H265Result_t result;
    H265Packet_t packetsArray[10];
    H265Nalu_t nalu;
    uint8_t naluBuffer[1500];

    memset(&ctx, 0, sizeof(H265DepacketizerContext_t));
    memset(packetsArray, 0, sizeof(packetsArray));

    nalu.pNaluData = naluBuffer;
    nalu.naluDataLength = sizeof(naluBuffer);

    result = H265Depacketizer_Init(&ctx, packetsArray, 10);
    TEST_ASSERT_EQUAL(H265_RESULT_OK, result);

    /* Test Case 1: Small packet (first malformed condition) */
    uint8_t smallPacket[] = {0x62, 0x01};  // Too small for FU
    packetsArray[0].pPacketData = smallPacket;
    packetsArray[0].packetDataLength = sizeof(smallPacket);

    ctx.tailIndex = 0;
    ctx.packetCount = 1;
    ctx.currentlyProcessingPacket = H265_PACKET_NONE;

    result = H265Depacketizer_GetNalu(&ctx, &nalu);
    TEST_ASSERT_EQUAL(H265_RESULT_MALFORMED_PACKET, result);
}

/*-----------------------------------------------------------------------------------------------------*/

/**
 * @brief Test H265 depacketization of aggregation packet with insufficient data for Nalu size.
 */
void test_H265Depacketizer_ProcessAggregationPacket_InsufficientDataForNaluSize(void)
{
    H265DepacketizerContext_t ctx;
    H265Result_t result;
    H265Packet_t packetsArray[1];
    H265Nalu_t nalu;
    uint8_t naluBuffer[100];

    memset(&ctx, 0, sizeof(H265DepacketizerContext_t));
    ctx.pPacketsArray = packetsArray;
    ctx.packetCount = 1;
    ctx.tailIndex = 0;

    memset(naluBuffer, 0, sizeof(naluBuffer));
    nalu.pNaluData = naluBuffer;
    nalu.naluDataLength = sizeof(naluBuffer);

    // Create a packet that's just big enough to be valid (minSize + 1)
    // but with currentOffset near the end to trigger the condition
    uint8_t packetData[] = {
        // AP header (2 bytes)
        0x60, 0x01,  // Type 48 (AP)

        // First NALU length + data (to make packet valid)
        0x00, 0x03,  // NALU length = 3
        0x01, 0x02, 0x03,  // NALU data

        // Second NALU length + data (to make packet valid)
        0x00, 0x03,  // NALU length = 3
        0x04, 0x05, 0x06   // NALU data
    };
    packetsArray[0].pPacketData = packetData;
    packetsArray[0].packetDataLength = sizeof(packetData);


    ctx.currentlyProcessingPacket = H265_AP_PACKET;
    ctx.apDepacketizationState.currentOffset = sizeof(packetData) - 1;
    result = H265Depacketizer_GetNalu(&ctx, &nalu);
    TEST_ASSERT_EQUAL(H265_RESULT_OK, result);

    TEST_ASSERT_EQUAL(H265_PACKET_NONE, ctx.currentlyProcessingPacket);
    TEST_ASSERT_EQUAL(1, ctx.tailIndex);
    TEST_ASSERT_EQUAL(0, ctx.packetCount);
}

/*-----------------------------------------------------------------------------------------------------*/

/**
 * @brief Test H265 depacketization of aggregation packet with error cases.
 */
void test_H265_Depacketizer_AggregationPacket_ErrorCases( void )
{
    H265DepacketizerContext_t ctx;
    H265Result_t result;
    H265Packet_t packetsArray[ 10 ];
    H265Nalu_t nalu;

    /* Initialize for remaining tests */
    memset( &ctx, 0, sizeof( H265DepacketizerContext_t ) );
    memset( packetsArray, 0, sizeof( packetsArray ) );
    memset( &nalu, 0, sizeof( H265Nalu_t ) );
    result = H265Depacketizer_Init( &ctx, packetsArray, 10);
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    /* Test 1: Malformed Packet - Invalid Second NALU Size */
    uint8_t invalidSecondSize[] =
    {
        0x61, 0x01,        /* AP header */
        0x00, 0x04,        /* First NALU length */
        0x26, 0x01,        /* First NALU header */
        0xAA, 0xBB,        /* First NALU payload */
        0xFF, 0xFF,        /* Invalid second NALU size */
        0x40, 0x01         /* Incomplete second NALU */
    };
    packetsArray[ 0 ].pPacketData = invalidSecondSize;
    packetsArray[ 0 ].packetDataLength = sizeof( invalidSecondSize );
    ctx.packetCount = 1;
    ctx.currentlyProcessingPacket = H265_AP_PACKET;
    ctx.apDepacketizationState.currentOffset = 8;  /* Point to second NALU */
    ctx.apDepacketizationState.firstUnit = 0;
    result = H265Depacketizer_GetNalu( &ctx, &nalu );
    TEST_ASSERT_EQUAL( H265_RESULT_MALFORMED_PACKET, result );
}

/*-----------------------------------------------------------------------------------------------------*/

/**
 * @brief Test H265 depacketization of aggregation packet with update offset.
 */
void test_H265_Depacketizer_AP_UpdateOffset_MoreNalus( void )
{
    H265DepacketizerContext_t ctx;
    H265Result_t result;
    H265Packet_t packetsArray[ 10 ];
    H265Nalu_t nalu = { 0 };

    /* Initialize everything */
    memset( &ctx, 0, sizeof( H265DepacketizerContext_t ) );
    memset( packetsArray, 0, sizeof( packetsArray ) );

    /* Initialize NALU buffer */
    uint8_t naluBuffer[ 20 ];
    memset( naluBuffer, 0, sizeof( naluBuffer ) );
    nalu.pNaluData = naluBuffer;
    nalu.naluDataLength = sizeof( naluBuffer );

    /* Initialize depacketizer */
    result = H265Depacketizer_Init( &ctx, packetsArray, 10 );
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    const uint8_t apPacket[] =
    {
        0x60, 0x00,             /* AP indicator (type = 48) and layer/TID */

        0x00, 0x04,             /* First NALU size (4 bytes) */
        0x26, 0x01,             /* First NALU header */
        0xAA, 0xBB,             /* First NALU payload */

        0x00, 0x04,             /* Second NALU size (4 bytes) */
        0x40, 0x01,             /* Second NALU header */
        0xCC, 0xDD,             /* Second NALU payload */

        0x00, 0x04,             /* Third NALU size (4 bytes) */
        0x44, 0x01,             /* Third NALU header */
        0xEE, 0xFF              /* Third NALU payload */
    };

    /* Add packet */
    H265Packet_t inputPacket =
    {
        .pPacketData      = ( uint8_t * ) apPacket,
        .packetDataLength = sizeof( apPacket )
    };

    result = H265Depacketizer_AddPacket( &ctx, &inputPacket );
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    /* Get first NALU */
    result = H265Depacketizer_GetNalu( &ctx, &nalu );
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );
    TEST_ASSERT_EQUAL( 8, ctx.apDepacketizationState.currentOffset );    /* Verify currentOffset is set to start of second NALU */

    /* Get second NALU */
    result = H265Depacketizer_GetNalu( &ctx, &nalu );
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );
    TEST_ASSERT_EQUAL( 14, ctx.apDepacketizationState.currentOffset );     /* Verify currentOffset is set to start of third NALU */

    /* Get third NALU */
    result = H265Depacketizer_GetNalu( &ctx, &nalu );
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );
    TEST_ASSERT_EQUAL( H265_PACKET_NONE, ctx.currentlyProcessingPacket );    /* Verify packet is completed */
}

/*-----------------------------------------------------------------------------------------------------*/

/**
 * @brief Test H265 depacketization of aggregation packet incase of out of memory.
 */
void test_H265Depacketizer_ProcessAggregationPacket_OutOfMemory(void)
{
    H265DepacketizerContext_t ctx;
    H265Result_t result;
    H265Packet_t packetsArray[1];
    H265Nalu_t nalu;
    uint8_t naluBuffer[10];  // Small buffer to force OUT_OF_MEMORY

    /* Initialize context and arrays */
    memset(&ctx, 0, sizeof(H265DepacketizerContext_t));
    memset(packetsArray, 0, sizeof(packetsArray));

    /* Initialize NALU with a small buffer */
    nalu.pNaluData = naluBuffer;
    nalu.naluDataLength = sizeof(naluBuffer);

    /* Initialize depacketizer */
    result = H265Depacketizer_Init(&ctx, packetsArray, 1);
    TEST_ASSERT_EQUAL(H265_RESULT_OK, result);

    /* Create an AP packet with a NALU larger than the output buffer */
    uint8_t apPacket[] = {
        0x60, 0x00,             // AP header (type 48)
        0x00, 0x0F,             // NALU length (15 bytes)
        0x01, 0x02, 0x03, 0x04, // NALU data (15 bytes)
        0x05, 0x06, 0x07, 0x08,
        0x09, 0x0A, 0x0B, 0x0C,
        0x0D, 0x0E, 0x0F
    };

    packetsArray[0].pPacketData = apPacket;
    packetsArray[0].packetDataLength = sizeof(apPacket);

    ctx.tailIndex = 0;
    ctx.packetCount = 1;
    ctx.currentlyProcessingPacket = H265_PACKET_NONE;

    /* Process the packet */
    result = H265Depacketizer_GetNalu(&ctx, &nalu);
    TEST_ASSERT_EQUAL(H265_RESULT_OUT_OF_MEMORY, result);
}

/*-----------------------------------------------------------------------------------------------------*/

/**
 * @brief Test H265 depacketization of aggregation packet with small packet.
 */
void test_H265Depacketizer_ProcessAggregationPacket_SmallPacket(void)
{
    H265DepacketizerContext_t ctx;
    H265Result_t result;
    H265Packet_t packetsArray[1];
    H265Nalu_t nalu;
    uint8_t naluBuffer[100];

    /* Initialize context and arrays */
    memset(&ctx, 0, sizeof(H265DepacketizerContext_t));
    memset(packetsArray, 0, sizeof(packetsArray));

    /* Initialize NALU */
    nalu.pNaluData = naluBuffer;
    nalu.naluDataLength = sizeof(naluBuffer);

    /* Initialize depacketizer */
    result = H265Depacketizer_Init(&ctx, packetsArray, 1);
    TEST_ASSERT_EQUAL(H265_RESULT_OK, result);

    /* Create an AP packet that's too small (smaller than minSize) */
    uint8_t smallApPacket[] = {
        0x60, 0x00,             // AP header (type 48)
        0x00, 0x01              // Just a small length field
    };

    packetsArray[0].pPacketData = smallApPacket;
    packetsArray[0].packetDataLength = sizeof(smallApPacket);  // Too small for valid AP packet

    ctx.tailIndex = 0;
    ctx.packetCount = 1;
    ctx.currentlyProcessingPacket = H265_PACKET_NONE;

    /* Process the packet */
    result = H265Depacketizer_GetNalu(&ctx, &nalu);
    TEST_ASSERT_EQUAL(H265_RESULT_MALFORMED_PACKET, result);
}

/*-------------------------------------------------------------------------------------------------------------*/

/**
 * @brief Test H265 depacketization AddPacket with out of memory cases.
 */
void test_H265Depacketizer_AddPacket_OutOfMemory(void)
{
    H265DepacketizerContext_t ctx;
    H265Packet_t packet;
    H265Packet_t packetsArray[2];  // Small array to trigger out of memory
    H265Result_t result;
    uint8_t packetData[] = {0x01, 0x02, 0x03, 0x04};  // Sample packet data

    /* Initialize context */
    memset(&ctx, 0, sizeof(ctx));
    memset(&packetsArray, 0, sizeof(packetsArray));

    /* Setup context with small array */
    ctx.pPacketsArray = packetsArray;
    ctx.packetsArrayLength = 2;  // Set small array length
    ctx.headIndex = 0;
    ctx.packetCount = 2;  // Set count equal to array length to trigger OOM

    /* Setup packet to add */
    packet.pPacketData = packetData;
    packet.packetDataLength = sizeof(packetData);

    /* Try to add packet when array is full */
    result = H265Depacketizer_AddPacket(&ctx, &packet);

    /* Verify results */
    TEST_ASSERT_EQUAL(H265_RESULT_OUT_OF_MEMORY, result);
    TEST_ASSERT_EQUAL(2, ctx.packetCount);  // Count shouldn't change
    TEST_ASSERT_EQUAL(0, ctx.headIndex);    // Index shouldn't change

    result = H265Depacketizer_AddPacket(&ctx, NULL);
    TEST_ASSERT_EQUAL(H265_RESULT_BAD_PARAM, result);

    result = H265Depacketizer_AddPacket(NULL, NULL);
    TEST_ASSERT_EQUAL(H265_RESULT_BAD_PARAM, result);
}

/*-------------------------------------------------------------------------------------------------------------*/

/**
 * @brief Test H265 depacketization GetFrame with NULL context and invalid parameters.
 */
void test_H265Depacketizer_GetFrame_InvalidParams(void)
{
    H265Frame_t frame = {0};
    uint8_t frameBuffer[100];
    H265Result_t result;
    H265DepacketizerContext_t ctx = {0};

    //Test:1
    {
        /* Initialize frame with valid data */
        frame.pFrameData = frameBuffer;
        frame.frameDataLength = sizeof(frameBuffer);
        result = H265Depacketizer_GetFrame(NULL, &frame);
        TEST_ASSERT_EQUAL(H265_RESULT_BAD_PARAM, result);
    }

    //Test:2
    {
        ctx.packetsArrayLength = 1;
        frame.pFrameData = NULL;
        frame.frameDataLength = 100;
        result = H265Depacketizer_GetFrame(&ctx, &frame);
        TEST_ASSERT_EQUAL(H265_RESULT_BAD_PARAM, result);
    }

    //Test:3
    {
        result = H265Depacketizer_GetFrame(&ctx, NULL);
        TEST_ASSERT_EQUAL(H265_RESULT_BAD_PARAM, result);
    }

    //Test:4
    {
        frame.pFrameData = frameBuffer;
        frame.frameDataLength = 0;
        result = H265Depacketizer_GetFrame(&ctx, &frame);
        TEST_ASSERT_EQUAL(H265_RESULT_BAD_PARAM, result);
    }

    //Test:5
    {
        frame.pFrameData = frameBuffer;
        frame.frameDataLength = sizeof(frameBuffer);
        ctx.packetCount = 0;
        result = H265Depacketizer_GetFrame(&ctx, &frame);
        TEST_ASSERT_EQUAL(H265_RESULT_NO_MORE_FRAMES, result);
    }
}

/*-----------------------------------------------------------------------------------------------------*/

/**
 * @brief Test H265 depacketization GetFrame with out of memory case.
 */
void test_H265Depacketizer_GetFrame_OutOfMemory(void)
{
    H265DepacketizerContext_t ctx;
    H265Frame_t frame;
    H265Packet_t packet;
    uint8_t frameBuffer[8];  // Small buffer
    // Create a valid single NALU packet
    uint8_t packetData[] = {
        0x26, 0x01,         // NALU header
        0x11, 0x22, 0x33    // NALU payload
    };
    H265Result_t result;

    /* Initialize structures */
    memset(&ctx, 0, sizeof(H265DepacketizerContext_t));
    memset(&frame, 0, sizeof(H265Frame_t));
    memset(&packet, 0, sizeof(H265Packet_t));

    /* Set up frame with small buffer */
    frame.pFrameData = frameBuffer;
    frame.frameDataLength = sizeof(frameBuffer);

    /* Set up packet */
    packet.pPacketData = packetData;
    packet.packetDataLength = sizeof(packetData);

    /* Set up context */
    result = H265Depacketizer_Init(&ctx, &packet, 1);
    TEST_ASSERT_EQUAL(H265_RESULT_OK, result);

    /* Ensure we have a packet to process */
    ctx.packetCount = 1;
    ctx.tailIndex = 0;
    ctx.currentlyProcessingPacket = H265_PACKET_NONE;

    /* Call GetFrame - should fail due to insufficient space for start code */
    result = H265Depacketizer_GetFrame(&ctx, &frame);
    TEST_ASSERT_EQUAL(H265_RESULT_OUT_OF_MEMORY, result);
}

/*-------------------------------------------------------------------------------------------------------------*/

/**
 * @brief Test H265 depacketization GetPacketProperties with various packet properties.
 */
void test_H265_Depacketizer_GetPacketProperties( void )
{
    H265Result_t result;
    uint32_t properties = 0;

    /* Test 1: Single NALU packet */
    uint8_t singleNaluPacket[] =
    {
        0x26, 0x01,  /* NAL header type 0x13 (within 1-47 range) */
        0xAA, 0xBB   /* Payload */
    };

    result = H265Depacketizer_GetPacketProperties( singleNaluPacket,
                                                   sizeof( singleNaluPacket ),
                                                   &properties );
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );
    TEST_ASSERT_EQUAL( H265_PACKET_PROPERTY_START_PACKET | H265_PACKET_PROPERTY_END_PACKET,
                       properties );

    /* Test 2: Fragmentation Unit - Start packet */
    uint8_t fuStartPacket[] =
    {
        0x62, 0x01,  /* NAL header type 0x31 (FU) */
        0x82,        /* FU header - Start bit set */
        0xCC, 0xDD   /* Payload */
    };
    result = H265Depacketizer_GetPacketProperties( fuStartPacket,
                                                   sizeof( fuStartPacket ),
                                                   &properties );
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );
    TEST_ASSERT_EQUAL( H265_PACKET_PROPERTY_START_PACKET, properties );

    /* Test 3: Fragmentation Unit - End packet */
    uint8_t fuEndPacket[] =
    {
        0x62, 0x01,  /* NAL header type 0x31 (FU) */
        0x42,        /* FU header - End bit set */
        0xEE, 0xFF   /* Payload */
    };
    result = H265Depacketizer_GetPacketProperties( fuEndPacket,
                                                   sizeof( fuEndPacket ),
                                                   &properties );
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );
    TEST_ASSERT_EQUAL( H265_PACKET_PROPERTY_END_PACKET, properties );

    /* Test 4: Aggregation packet */
    uint8_t apPacket[] =
    {
        0x61, 0x01,  /* NAL header type 0x30 (AP) */
        0x00, 0x04,  /* First NALU length */
        0xAA, 0xBB   /* Payload */
    };
    result = H265Depacketizer_GetPacketProperties( apPacket,
                                                   sizeof( apPacket ),
                                                   &properties );
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );
    TEST_ASSERT_EQUAL( H265_PACKET_PROPERTY_START_PACKET | H265_PACKET_PROPERTY_END_PACKET,
                       properties );

    /* Test 5: Invalid packet (too small) */
    uint8_t smallPacket[] = { 0x26 };  /* Just one byte */
    result = H265Depacketizer_GetPacketProperties( smallPacket,
                                                   sizeof( smallPacket ),
                                                   &properties );
    TEST_ASSERT_EQUAL( H265_RESULT_BAD_PARAM, result );

    /* Test 6: NULL parameters */
    result = H265Depacketizer_GetPacketProperties( NULL, 4, &properties );
    TEST_ASSERT_EQUAL( H265_RESULT_BAD_PARAM, result );

    result = H265Depacketizer_GetPacketProperties( singleNaluPacket, 4, NULL );
    TEST_ASSERT_EQUAL( H265_RESULT_BAD_PARAM, result );

    /* Test 7: Create a packet with NAL unit type 0 (which is below SINGLE_NALU_PACKET_TYPE_START) */
    uint8_t packet[] = {
        0x00,  // NAL unit header with type 0 (0 << 1)
        0x01,  // Second byte
        0x00,  // Additional payload
        0x00   // Additional payload
    };

    result = H265Depacketizer_GetPacketProperties(packet, sizeof(packet), &properties);
    TEST_ASSERT_EQUAL(H265_RESULT_UNSUPPORTED_PACKET, result);
    TEST_ASSERT_EQUAL(0, properties);
}

/*-----------------------------------------------------------------------------------------------------*/

/**
 * @brief Test H265 depacketization GetPacketProperties with malformed packets.
 */
void test_H265Depacketizer_GetPacketProperties_MalformedPacket(void)
{
    H265Result_t result;
    uint32_t properties = 0;

    /* Test 1: Malformed FU packet (too short) */
    uint8_t malformedFUPacket[] = {
        0x62,  // NAL unit header (FU indicator)
        0x01   // Second byte
        // Missing FU header
    };
    result = H265Depacketizer_GetPacketProperties(malformedFUPacket, sizeof(malformedFUPacket), &properties);
    TEST_ASSERT_EQUAL(H265_RESULT_MALFORMED_PACKET, result);

    /* Test 2: Malformed AP packet (too short) */
    uint8_t malformedAPPacket[] = {
        0x60,  // NAL unit header (AP indicator)
        0x01   // Second byte
        // Missing NALU length field
    };
    result = H265Depacketizer_GetPacketProperties(malformedAPPacket, sizeof(malformedAPPacket), &properties);
    TEST_ASSERT_EQUAL(H265_RESULT_MALFORMED_PACKET, result);

    /* Test 3: Unsupported packet type */
    uint8_t unsupportedPacket[] = {
        0xFE,  // NAL unit header with unsupported type (0x7F)
        0x01,  // Second byte
        0x00,  // Additional byte to meet minimum size
        0x00   // Additional byte to meet minimum size
    };
    result = H265Depacketizer_GetPacketProperties(unsupportedPacket, sizeof(unsupportedPacket), &properties);
    TEST_ASSERT_EQUAL(H265_RESULT_UNSUPPORTED_PACKET, result);
}

/*-------------------------------------------------------------------------------------------------------------*/

/**
 * @brief Test H265 depacketization Get Nalu with unsupported packet type.
 */
void test_H265_Depacketizer_GetNalu_UnsupportedPacketType( void )
{
    H265DepacketizerContext_t ctx;
    H265Result_t result;
    H265Packet_t packetsArray[ 10 ];
    H265Nalu_t nalu;

    /* Initialize context */
    memset( &ctx, 0, sizeof( H265DepacketizerContext_t ) );
    memset( packetsArray, 0, sizeof( packetsArray ) );
    memset( &nalu, 0, sizeof( H265Nalu_t ) );

    /* Initialize depacketizer */
    result = H265Depacketizer_Init( &ctx, packetsArray, 10);
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    result = H265Depacketizer_Init( &ctx, packetsArray, 0);
    TEST_ASSERT_EQUAL( H265_RESULT_BAD_PARAM, result );

    result = H265Depacketizer_Init( &ctx, NULL, 0);
    TEST_ASSERT_EQUAL( H265_RESULT_BAD_PARAM, result );

    result = H265Depacketizer_Init( NULL, NULL, 0);
    TEST_ASSERT_EQUAL( H265_RESULT_BAD_PARAM, result );

    /* Setup packet with unsupported NAL type */
    uint8_t unsupportedPacket[] =
    {
        0x00, 0x01,  /* NAL type 0x00 (below minimum of 0x01) */
        0xAA, 0xBB   /* Payload */
    };

    /* Setup context */
    packetsArray[ 0 ].pPacketData = unsupportedPacket;
    packetsArray[ 0 ].packetDataLength = sizeof( unsupportedPacket );
    ctx.packetCount = 1;
    ctx.tailIndex = 0;

    /* Call GetNalu with unsupported packet */
    result = H265Depacketizer_GetNalu( &ctx, &nalu );
    TEST_ASSERT_EQUAL( H265_RESULT_UNSUPPORTED_PACKET, result );

    result = H265Depacketizer_GetNalu( NULL, NULL );
    TEST_ASSERT_EQUAL( H265_RESULT_BAD_PARAM, result );

    result = H265Depacketizer_GetNalu( &ctx, NULL );
    TEST_ASSERT_EQUAL( H265_RESULT_BAD_PARAM, result );
}

/*-------------------------------------------------------------------------------------------------------------*/
