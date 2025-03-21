/* Unity includes. */
#include "unity.h"
#include "catch_assert.h"

/* Standard includes. */
#include <string.h>
#include <stdint.h>

/* API includes. */
#include "h265_packetizer.h"
#include "h265_depacketizer.h"

/* Defines */
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

/**
 * @brief Test H265 packetization with single NAL unit
 */
void test_H265_Packetizer_SingleNal( void )
{
    H265PacketizerContext_t ctx;
    H265Result_t result;
    H265Nalu_t naluArray[ MAX_NALUS_IN_A_FRAME ];
    H265Packet_t packet;

    /* Test NAL unit (SPS) */
    uint8_t nalData[] =
    {
        0x42, 0x01,      /* NAL header (Type=32/VPS) */
        0x01, 0x02, 0x03 /* NAL payload */
    };

    /* Initialize context */
    result = H265Packetizer_Init( &ctx, naluArray,  MAX_NALUS_IN_A_FRAME);
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    /* Add NAL unit */
    H265Nalu_t nalu =
    {
        .pNaluData      = nalData,
        .naluDataLength = sizeof( nalData ),
        .nal_unit_type  = 32, /* VPS */
        .nal_layer_id   = 0,
        .temporal_id    = 1
    };
    result = H265Packetizer_AddNalu( &ctx, &nalu );
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );
    TEST_ASSERT_EQUAL( 1, ctx.naluCount );

    /* Get packet */
    packet.pPacketData = packetBuffer;
    packet.packetDataLength = MAX_H265_PACKET_LENGTH;
    result = H265Packetizer_GetPacket( &ctx, &packet );
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );
    TEST_ASSERT_EQUAL( sizeof( nalData ), packet.packetDataLength );

    /* Verify packet data */
    TEST_ASSERT_EQUAL_MEMORY( nalData, packet.pPacketData, packet.packetDataLength );
}

void test_H265_Packetizer_SingleNal_MinimumSize( void )                    /* just the header, no payload */
{
    H265PacketizerContext_t ctx;
    H265Result_t result;
    H265Nalu_t naluArray[ MAX_NALUS_IN_A_FRAME ];
    H265Packet_t packet;

    /* Minimum size NAL unit (just header) */
    uint8_t nalData[] =
    {
        0x40, 0x01  /* NAL header (Type=32/VPS, LayerId=0, TID=1) */
    };

    /* Initialize context */
    result = H265Packetizer_Init( &ctx, naluArray, MAX_NALUS_IN_A_FRAME);
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    /* Add NAL unit */
    H265Nalu_t nalu =
    {
        .pNaluData      = nalData,
        .naluDataLength = sizeof( nalData ),
        .nal_unit_type  = 32,
        .nal_layer_id   = 0,
        .temporal_id    = 1
    };
    result = H265Packetizer_AddNalu( &ctx, &nalu );
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    /* Get packet */
    packet.pPacketData = packetBuffer;
    packet.packetDataLength = MAX_H265_PACKET_LENGTH;
    result = H265Packetizer_GetPacket( &ctx, &packet );
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );
    TEST_ASSERT_EQUAL( sizeof( nalData ), packet.packetDataLength );

    /* Verify packet data */
    TEST_ASSERT_EQUAL_MEMORY( nalData, packet.pPacketData, packet.packetDataLength );
}

/*-----------------------------------------------------------------------------------------------------*/

/**
 * @brief Test H265 packetization with fragmentation
 */
void test_H265_Packetizer_Fragmentation( void )
{
    H265PacketizerContext_t ctx;
    H265Result_t result;
    H265Nalu_t naluArray[ MAX_NALUS_IN_A_FRAME ];
    H265Packet_t packet;

    /* Initialize packet counters */
    uint32_t packetCount = 0;
    uint32_t startPackets = 0;
    uint32_t middlePackets = 0;
    uint32_t endPackets = 0;

    /* Create large NAL unit that needs fragmentation */
    uint8_t nalData[ 2000 ] =
    {
        0x40, 0x01, /* First byte: F=0, Type=32 (VPS), second byte: LayerId=0, TID=1 */
        0x00, 0x00  /* Start of payload */
    };

    /* Fill rest of payload */
    memset( nalData + 4, 0xCC, sizeof( nalData ) - 4 );

    /* Initialize context with small packet size to force fragmentation */
    result = H265Packetizer_Init( &ctx, naluArray, MAX_NALUS_IN_A_FRAME );
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    /* Add large NAL unit */
    H265Nalu_t nalu =
    {
        .pNaluData      = nalData,
        .naluDataLength = sizeof( nalData ),
        .nal_unit_type  = 32, /* VPS */
        .nal_layer_id   = 0,
        .temporal_id    = 1
    };
    result = H265Packetizer_AddNalu( &ctx, &nalu );
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );
    TEST_ASSERT_EQUAL( 1, ctx.naluCount );

    /* Get first fragment */
    packet.pPacketData = packetBuffer;
    packet.packetDataLength = 100;
    result = H265Packetizer_GetPacket( &ctx, &packet );
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    /* Count the first packet */
    if( result == H265_RESULT_OK )
    {
        uint8_t fuHeader = packet.pPacketData[ 2 ];

        if( fuHeader & FU_HEADER_S_BIT_MASK )
        {
            startPackets++;
        }

        packetCount++;
    }

    uint8_t lastFragmentSeen = 0;           /* 0 = false, 1 = true */

    do
    {
        packet.pPacketData = packetBuffer;
        packet.packetDataLength = 100;
        result = H265Packetizer_GetPacket( &ctx, &packet );

        if( result == H265_RESULT_OK )
        {
            uint8_t fuHeader = packet.pPacketData[ 2 ]; /* Get FU header */

            /* Check packet type */
            if( fuHeader & FU_HEADER_S_BIT_MASK )
            {
                startPackets++;
            }
            else if( fuHeader & FU_HEADER_E_BIT_MASK )
            {
                endPackets++;
                lastFragmentSeen = 1;
            }
            else
            {
                middlePackets++;
            }

            packetCount++;
        }
    } while( result == H265_RESULT_OK && !lastFragmentSeen );

    /* Verify packet counts */
    TEST_ASSERT_EQUAL( 1, startPackets );
    TEST_ASSERT_EQUAL( 1, endPackets );
    TEST_ASSERT_EQUAL( 21, packetCount );
    TEST_ASSERT_TRUE( lastFragmentSeen );

    /* Verify final state */
    TEST_ASSERT_EQUAL( H265_PACKET_NONE, ctx.currentlyProcessingPacket );
    TEST_ASSERT_EQUAL( 0, ctx.naluCount );
}

void test_H265_Packetizer_Fragmentation_Zero_PayloadSize( void )
{
    H265PacketizerContext_t ctx;
    H265Result_t result;
    H265Nalu_t naluArray[ 10 ];
    H265Packet_t packet = { 0 };
    uint8_t packetBuffer[ 10 ];  /* Small buffer to force zero payload size */

    /* Initialize context */
    result = H265Packetizer_Init( &ctx,
                                  naluArray,
                                  10);
    TEST_ASSERT_EQUAL( 0, result );

    /* Create a NAL unit that needs fragmentation */
    uint8_t naluData[ 100 ] = { 0 };
    naluData[ 0 ] = 0x62;             /* NAL header first byte */
    naluData[ 1 ] = 0x01;             /* NAL header second byte */
    memset( naluData + 2, 0xAA, 98 ); /* Fill rest with data */

    /* Setup NAL */
    H265Nalu_t testNalu = { 0 };
    testNalu.pNaluData = naluData;
    testNalu.naluDataLength = sizeof( naluData );

    /* Add NAL to context */
    result = H265Packetizer_AddNalu( &ctx, &testNalu );
    TEST_ASSERT_EQUAL( 0, result );

    /* Setup packet with minimal size to force zero payload */
    packet.pPacketData = packetBuffer;
    packet.packetDataLength = 3;  /* Just enough for headers (2 bytes PayloadHdr + 1 byte FUHeader) */
                               /* This should make maxPayloadSize = 0 */

    /* Get packet - should attempt fragmentation with zero payload size */
    result = H265Packetizer_GetPacket( &ctx, &packet );
    TEST_ASSERT_EQUAL( 0, result );

    /* Verify packet size is just headers */
    TEST_ASSERT_EQUAL( 3, packet.packetDataLength );
}



/*-----------------------------------------------------------------------------------------------------*/



void test_H265_Packetizer_SimpleAggregation( void )
{
    H265PacketizerContext_t ctx;
    H265Result_t result;
    H265Nalu_t naluArray[ MAX_NALUS_IN_A_FRAME ];
    H265Packet_t packet;

    /* Create two small NAL units */
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

    /* Initialize context */
    result = H265Packetizer_Init( &ctx, naluArray, MAX_NALUS_IN_A_FRAME);
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    /* Add NAL units */
    H265Nalu_t nalu1 =
    {
        .pNaluData      = nalData1,
        .naluDataLength = sizeof( nalData1 ),
        .nal_unit_type  = 32,
        .nal_layer_id   = 0,
        .temporal_id    = 1
    };
    result = H265Packetizer_AddNalu( &ctx, &nalu1 );
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    H265Nalu_t nalu2 =
    {
        .pNaluData      = nalData2,
        .naluDataLength = sizeof( nalData2 ),
        .nal_unit_type  = 33,
        .nal_layer_id   = 0,
        .temporal_id    = 1
    };
    result = H265Packetizer_AddNalu( &ctx, &nalu2 );
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    /* Calculate expected packet size */
    size_t expectedSize = 2 +                          /* PayloadHdr */
                          ( 2 + sizeof( nalData1 ) ) + /* Length1 + NAL1 */
                          ( 2 + sizeof( nalData2 ) );  /* Length2 + NAL2 */

    /* Get aggregated packet */
    packet.pPacketData = packetBuffer;
    packet.packetDataLength = 100;
    result = H265Packetizer_GetPacket( &ctx, &packet );
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    /* Print first NAL unit details */
    uint16_t firstNaluLength = ( packet.pPacketData[ 2 ] << 8 ) | packet.pPacketData[ 3 ];

    /* Print second NAL unit details */

    size_t secondNalOffset = 2 + 2 + sizeof( nalData1 );
    uint16_t secondNaluLength = ( packet.pPacketData[ secondNalOffset ] << 8 ) |
                                packet.pPacketData[ secondNalOffset + 1 ];

    /* Basic verification */
    TEST_ASSERT_EQUAL( expectedSize, packet.packetDataLength );
    TEST_ASSERT_EQUAL( 48, ( packet.pPacketData[ 0 ] >> 1 ) & 0x3F );  /* AP packet type */
    TEST_ASSERT_EQUAL( sizeof( nalData1 ), firstNaluLength );
    TEST_ASSERT_EQUAL( sizeof( nalData2 ), secondNaluLength );
    TEST_ASSERT_EQUAL( 0, ctx.naluCount );
}

void test_H265_Packetizer_Aggregation_Insufficient_Nalus( void )
{
    H265PacketizerContext_t ctx;
    H265Result_t result;
    H265Nalu_t naluArray[ 10 ];
    H265Packet_t packet = { 0 };
    uint8_t packetBuffer[ 20 ];  /* Small buffer to limit aggregation */

    /* Initialize context */
    result = H265Packetizer_Init( &ctx,
                                  naluArray,
                                  10);    /* No DON handling */
    TEST_ASSERT_EQUAL( 0, result );

    /* Create two NAL units - second one too big to fit */
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

    /* Setup first NAL */
    H265Nalu_t testNalu1 = { 0 };
    testNalu1.pNaluData = naluData1;
    testNalu1.naluDataLength = sizeof( naluData1 );

    /* Setup second NAL */
    H265Nalu_t testNalu2 = { 0 };
    testNalu2.pNaluData = naluData2;
    testNalu2.naluDataLength = sizeof( naluData2 );

    /* Add NALs to context */
    result = H265Packetizer_AddNalu( &ctx, &testNalu1 );
    TEST_ASSERT_EQUAL( 0, result );

    result = H265Packetizer_AddNalu( &ctx, &testNalu2 );
    TEST_ASSERT_EQUAL( 0, result );

    /* Setup packet with small size */
    packet.pPacketData = packetBuffer;
    packet.packetDataLength = 20;  /* Small enough that second NAL won't fit */

    /* Try to get packet - should attempt aggregation but fail */
    result = H265Packetizer_GetPacket( &ctx, &packet );
    TEST_ASSERT_EQUAL( 0, result );  /* Adjust expected result based on your implementation */
}

/*-----------------------------------------------------------------------------------------------------*/

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
    TEST_ASSERT_EQUAL( 32, ctx.pNaluArray[ 0 ].nal_unit_type );
    TEST_ASSERT_EQUAL( 0, ctx.pNaluArray[ 0 ].nal_layer_id );
    TEST_ASSERT_EQUAL( 1, ctx.pNaluArray[ 0 ].temporal_id );
}


void test_H265_Packetizer_AddFrame_Malformed_Three_Byte_StartCode( void )
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

    /* Create frame with 3-byte start codes and malformed NAL */
    uint8_t frameData[] =
    {
        /* First NAL with 3-byte start code */
        0x00, 0x00, 0x01,        /* First start code (3-byte) */
        0x40, 0x01, 0x02, 0x03,  /* Valid first NAL */

        /* Second NAL with 3-byte start code but malformed */
        0x00, 0x00, 0x01, /* Second start code (3-byte) */
        0x41,             /* Single byte NAL (malformed) */

        /* Third start code to trigger processing of malformed NAL */
        0x00, 0x00, 0x01,        /* Third start code (3-byte) */
        0x42, 0x01, 0x04, 0x05   /* Final valid NAL */
    };

    /* Setup frame */
    frame.pFrameData = frameData;
    frame.frameDataLength = sizeof( frameData );

    /* Process frame */
    result = H265Packetizer_AddFrame( &ctx, &frame );

    /* Should return malformed packet error */
    TEST_ASSERT_EQUAL( H265_RESULT_MALFORMED_PACKET, result );
}



void test_H265_Packetizer_AddFrame_Malformed_Last_Nalu( void )
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

    /* Create frame with malformed last NAL unit */
    uint8_t frameData[] =
    {
        0x00, 0x00, 0x00, 0x01,  /* First start code */
        0x40, 0x01, 0x02, 0x03,  /* Valid first NAL unit */
        0x00, 0x00, 0x00, 0x01,  /* Second start code */
        0x40                     /* Malformed NAL unit (only 1 byte, less than NALU_HEADER_SIZE) */
    };

    /* Setup frame */
    frame.pFrameData = frameData;
    frame.frameDataLength = sizeof( frameData );

    /* Add frame with malformed last NAL */
    result = H265Packetizer_AddFrame( &ctx, &frame );

    /* Should return malformed packet error */
    TEST_ASSERT_EQUAL( H265_RESULT_MALFORMED_PACKET, result );
}

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

    /* Add frame - should not find any start codes */
    result = H265Packetizer_AddFrame( &ctx, &frame );

    /* Verify result */
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    /* Verify no NALs were added */
    TEST_ASSERT_EQUAL( 0, ctx.naluCount );
}



/*-----------------------------------------------------------------------------------------------------*/



void test_H265_Packetizer_AddNalu_Malformed_Packet( void )
{
    H265PacketizerContext_t ctx;
    H265Result_t result;
    H265Nalu_t naluArray[ 10 ];
    H265Nalu_t testNalu = { 0 };

    /* Initialize context */
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
    testNalu.nal_unit_type = MAX_NAL_UNIT_TYPE + 1;  /* Invalid type */
    testNalu.nal_layer_id = 0;
    testNalu.temporal_id = 0;
    result = H265Packetizer_AddNalu( &ctx, &testNalu );
    TEST_ASSERT_EQUAL( H265_RESULT_BAD_PARAM, result );

    /* Test Case 3: Invalid layer ID */
    testNalu.nal_unit_type = 32;              /* Valid type */
    testNalu.nal_layer_id = MAX_LAYER_ID + 1; /* Invalid layer ID */
    testNalu.temporal_id = 0;
    result = H265Packetizer_AddNalu( &ctx, &testNalu );
    TEST_ASSERT_EQUAL( H265_RESULT_BAD_PARAM, result );

    /* Test Case 4: Invalid temporal ID */
    testNalu.nal_unit_type = 32;                /* Valid type */
    testNalu.nal_layer_id = 0;                  /* Valid layer ID */
    testNalu.temporal_id = MAX_TEMPORAL_ID + 1; /* Invalid temporal ID */
    result = H265Packetizer_AddNalu( &ctx, &testNalu );
    TEST_ASSERT_EQUAL( H265_RESULT_BAD_PARAM, result );
}


void test_H265_Packetizer_AddNalu_Array_Full( void )
{
    H265PacketizerContext_t ctx;
    H265Result_t result;
    H265Nalu_t naluArray[ 2 ];  /* Very small array to trigger overflow */
    H265Nalu_t testNalu = { 0 };

    /* Initialize context with small array length */
    result = H265Packetizer_Init( &ctx,
                                  naluArray,
                                  2 );         /* Small array length to trigger overflow */
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    /* Create a valid NAL unit */
    uint8_t naluData[] =
    {
        0x40, 0x01, 0x02, 0x03  /* Valid NAL unit data (assuming NALU_HEADER_SIZE = 2) */
    };

    /* Setup the test NAL unit */
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



void test_H265_Packetizer_GetPacket_Small_MaxSize( void )
{
    H265PacketizerContext_t ctx;
    H265Result_t result;
    H265Nalu_t naluArray[ 10 ];
    H265Packet_t packet = { 0 };
    uint8_t packetBuffer[ 2 ];  /* Very small buffer, less than NALU_HEADER_SIZE */

    /* Initialize context */
    result = H265Packetizer_Init( &ctx,
                                  naluArray,
                                  10);
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    /* Setup packet with small maxPacketSize */
    packet.pPacketData = packetBuffer;
    packet.packetDataLength = 1;  /* Set size less than NALU_HEADER_SIZE */
    result = H265Packetizer_GetPacket( &ctx, &packet );
    TEST_ASSERT_EQUAL( H265_RESULT_BAD_PARAM, result );
}



void test_H265_Packetizer_GetPacket_Size_Overflow( void )
{
    H265PacketizerContext_t ctx;
    H265Result_t result;
    H265Nalu_t naluArray[ 10 ];
    H265Packet_t packet = { 0 };
    uint8_t packetBuffer[ 100 ];  /* Small buffer to trigger overflow */

    /* Initialize context */
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

    /* Add NALs to context */
    result = H265Packetizer_AddNalu( &ctx, &testNalu1 );
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    result = H265Packetizer_AddNalu( &ctx, &testNalu2 );
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    result = H265Packetizer_AddNalu( &ctx, &testNalu3 );
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    /* Setup packet with small max size to force overflow */
    packet.pPacketData = packetBuffer;
    packet.packetDataLength = 100;  /* Small enough to trigger overflow */

    result = H265Packetizer_GetPacket( &ctx, &packet );

    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );
}

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

    /* Create and add a NAL unit */
    uint8_t naluData[] =
    {
        0x40, 0x01,  /* NAL header */
        0x02, 0x03   /* Some payload data */
    };

    /* Setup NAL */
    H265Nalu_t testNalu = { 0 };
    testNalu.pNaluData = naluData;
    testNalu.naluDataLength = sizeof( naluData );

    /* Add NAL to context */
    result = H265Packetizer_AddNalu( &ctx, &testNalu );
    TEST_ASSERT_EQUAL( 0, result );

    /* Setup packet with zero maxPacketSize */
    packet.pPacketData = packetBuffer;
    packet.packetDataLength = 0;  /* Set to zero to trigger the check */

    /* Try to get packet */
    result = H265Packetizer_GetPacket( &ctx, &packet );

    /* Should return bad parameter due to zero maxPacketSize */
    TEST_ASSERT_EQUAL( H265_RESULT_BAD_PARAM, result );
}


void test_H265_Packetizer_GetPacket_Aggregation_Array_Limit( void )
{
    H265PacketizerContext_t ctx;
    H265Result_t result;
    H265Nalu_t naluArray[ 3 ];  /* Small array to hit the length limit */
    H265Packet_t packet = { 0 };
    uint8_t packetBuffer[ 1500 ];

    /* Initialize context with small NAL array */
    result = H265Packetizer_Init( &ctx,
                                  naluArray,
                                  3 );      /* Small naluArrayLength */
    TEST_ASSERT_EQUAL( 0, result );

    /* Create three small NAL units that would fit in one packet */
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

    /* Setup NALs */
    H265Nalu_t testNalu1 = { 0 };
    testNalu1.pNaluData = naluData1;
    testNalu1.naluDataLength = sizeof( naluData1 );

    H265Nalu_t testNalu2 = { 0 };
    testNalu2.pNaluData = naluData2;
    testNalu2.naluDataLength = sizeof( naluData2 );

    H265Nalu_t testNalu3 = { 0 };
    testNalu3.pNaluData = naluData3;
    testNalu3.naluDataLength = sizeof( naluData3 );

    /* Add all NALs to context */
    result = H265Packetizer_AddNalu( &ctx, &testNalu1 );
    TEST_ASSERT_EQUAL( 0, result );

    result = H265Packetizer_AddNalu( &ctx, &testNalu2 );
    TEST_ASSERT_EQUAL( 0, result );

    result = H265Packetizer_AddNalu( &ctx, &testNalu3 );
    TEST_ASSERT_EQUAL( 0, result );

    /* Setup packet */
    packet.pPacketData = packetBuffer;
    packet.packetDataLength = 1500;  /* Large enough for all NALs */

    /* Get packet - should try to aggregate all NALs and hit array length limit */
    result = H265Packetizer_GetPacket( &ctx, &packet );
    TEST_ASSERT_EQUAL( 0, result );
}

void test_H265_Packetizer_GetPacket_Tail_At_End( void )
{
    H265PacketizerContext_t ctx;
    H265Result_t result;
    H265Nalu_t naluArray[ 2 ];  /* Small array size of 2 */
    H265Packet_t packet = { 0 };
    uint8_t packetBuffer[ 1500 ];

    /* Initialize context */
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

    /* Setup NAL */
    H265Nalu_t testNalu = { 0 };
    testNalu.pNaluData = naluData;
    testNalu.naluDataLength = sizeof( naluData );

    /* Manually set up context state */
    ctx.tailIndex = 1;              /* Set to last index (naluArrayLength - 1) */
    ctx.naluCount = 2;              /* Set count >= 2 to satisfy first condition */
    ctx.naluArrayLength = 2;        /* Confirm array length */

    /* Add NAL at the last position */
    naluArray[ ctx.tailIndex ] = testNalu;

    /* Setup packet */
    packet.pPacketData = packetBuffer;
    packet.packetDataLength = 1500;    /* Large enough for single NAL */

    /* Try to get packet */
    result = H265Packetizer_GetPacket( &ctx, &packet );
    TEST_ASSERT_EQUAL( 0, result );
}

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

    /* Initialize context - making pCtx valid */
    result = H265Packetizer_Init( &ctx,
                                  naluArray,
                                  10);
    TEST_ASSERT_EQUAL( 0, result );

    TEST_ASSERT_EQUAL(0, ctx.naluCount);
    packet.pPacketData = packetBuffer;
    packet.packetDataLength = MAX_H265_PACKET_LENGTH;
    result = H265Packetizer_GetPacket(&ctx, &packet);
     TEST_ASSERT_EQUAL(H265_RESULT_NO_MORE_NALUS, result);

    /* Add a valid NAL to context */
    H265Nalu_t testNalu = { 0 };
    testNalu.pNaluData = naluData;
    testNalu.naluDataLength = sizeof( naluData );
    result = H265Packetizer_AddNalu( &ctx, &testNalu );
    TEST_ASSERT_EQUAL( 0, result );

    /* Test Case 1: Valid pCtx, NULL packet pointer */
    result = H265Packetizer_GetPacket( &ctx, NULL );
    TEST_ASSERT_EQUAL( H265_RESULT_BAD_PARAM, result );

    result = H265Packetizer_GetPacket( NULL, NULL );
    TEST_ASSERT_EQUAL( H265_RESULT_BAD_PARAM, result );

    packet.pPacketData = NULL;           // Setting packet data to NULL
    packet.packetDataLength = MAX_H265_PACKET_LENGTH;  // Valid size
    result = H265Packetizer_GetPacket(&ctx, &packet);
    TEST_ASSERT_EQUAL(H265_RESULT_BAD_PARAM, result);
}

/*-----------------------------------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------------------------------*/

void test_H265_Depacketizer_ProcessFragmentedNalu( void )
{
    H265DepacketizerContext_t ctx;
    H265Result_t result;
    H265Packet_t packetsArray[ 10 ];
    H265Nalu_t nalu;

    /* Initialize context and arrays */
    memset( &ctx, 0, sizeof( H265DepacketizerContext_t ) );
    memset( packetsArray, 0, sizeof( packetsArray ) );
    memset( &nalu, 0, sizeof( H265Nalu_t ) );

    uint8_t reassemblyBuffer[ 1500 ];
    memset( reassemblyBuffer, 0, sizeof( reassemblyBuffer ) );

    /* Initialize test packets with correct FU indicator */
    uint8_t startFragment[] =
    {
        0x62,   /* NAL unit header byte 1: FU indicator (type 0x31) */
        0x01,   /* NAL unit header byte 2 */
        0x82,   /* FU header: Start bit(0x80) + NAL type(0x02) */
        0x11,   /* Payload */
        0x22    /* Payload */
    };

    uint8_t middleFragment[] =
    {
        0x62,   /* Same FU indicator */
        0x01,   /* Same header byte 2 */
        0x02,   /* FU header: continuation (just NAL type) */
        0x33,   /* Payload */
        0x44    /* Payload */
    };

    uint8_t endFragment[] =
    {
        0x62,   /* Same FU indicator */
        0x01,   /* Same header byte 2 */
        0x42,   /* FU header: End bit(0x40) + NAL type(0x02) */
        0x55,   /* Payload */
        0x66    /* Payload */
    };

    /* Initialize depacketizer */
    result = H265Depacketizer_Init( &ctx, packetsArray, 10);

    /* Set up reassembly buffer */
    ctx.fuDepacketizationState.pReassemblyBuffer = reassemblyBuffer;
    ctx.fuDepacketizationState.reassemblyBufferSize = sizeof( reassemblyBuffer );

    /* Process start fragment */
    packetsArray[ 0 ].pPacketData = startFragment;
    packetsArray[ 0 ].packetDataLength = sizeof( startFragment );
    result = H265Depacketizer_AddPacket( &ctx, &packetsArray[ 0 ] );

    /* Get first NALU to process start fragment */
    result = H265Depacketizer_GetNalu( &ctx, &nalu );

    /* Process middle fragment */
    packetsArray[ 1 ].pPacketData = middleFragment;
    packetsArray[ 1 ].packetDataLength = sizeof( middleFragment );
    result = H265Depacketizer_AddPacket( &ctx, &packetsArray[ 1 ] );

    /* Get NALU after middle fragment */
    result = H265Depacketizer_GetNalu( &ctx, &nalu );

    /* Process end fragment */
    packetsArray[ 2 ].pPacketData = endFragment;
    packetsArray[ 2 ].packetDataLength = sizeof( endFragment );
    result = H265Depacketizer_AddPacket( &ctx, &packetsArray[ 2 ] );

    /* Get final NALU */
    result = H265Depacketizer_GetNalu( &ctx, &nalu );

    TEST_ASSERT_EQUAL_MESSAGE( H265_RESULT_OK, result, "Final GetNalu failed" );

    if( result == H265_RESULT_OK )
    {
        TEST_ASSERT_NOT_NULL_MESSAGE( nalu.pNaluData, "NALU data pointer is NULL" );
        TEST_ASSERT_GREATER_THAN_MESSAGE( 0, nalu.naluDataLength, "NALU length is 0" );
    }
}

void test_H265_Depacketizer_FragmentationUnit_ErrorCases( void )
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
    result = H265Depacketizer_Init( &ctx, packetsArray, 10 );
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    /* Test small packet */
    uint8_t smallPacket[] =
    {
        0x62, 0x01  /* FU indicator but too small */
    };
    packetsArray[ 0 ].pPacketData = smallPacket;
    packetsArray[ 0 ].packetDataLength = sizeof( smallPacket );
    ctx.packetCount = 1;
    result = H265Depacketizer_GetNalu( &ctx, &nalu );
    TEST_ASSERT_EQUAL( H265_RESULT_MALFORMED_PACKET, result );

    /* Test reassembly buffer overflow on start fragment */
    uint8_t largeFuStart[] =
    {
        0x62, 0x01,                                      /* FU indicator */
        0x82,                                            /* FU header with start bit */
        0xAA, 0xBB                                       /* Payload */
    };
    ctx.fuDepacketizationState.reassemblyBufferSize = 1; /* Too small buffer */
    packetsArray[ 0 ].pPacketData = largeFuStart;
    packetsArray[ 0 ].packetDataLength = sizeof( largeFuStart );
    ctx.packetCount = 1;
    result = H265Depacketizer_GetNalu( &ctx, &nalu );
    TEST_ASSERT_EQUAL( H265_RESULT_MALFORMED_PACKET, result );

    /* Test continuation fragment without start */
    uint8_t continuationFu[] =
    {
        0x62, 0x01,                                         /* FU indicator */
        0x02,                                               /* FU header continuation */
        0xAA, 0xBB                                          /* Payload */
    };
    ctx.fuDepacketizationState.reassemblyBufferSize = 1500; /* Reset buffer size */
    ctx.currentlyProcessingPacket = H265_PACKET_NONE;       /* Not processing FU */
    packetsArray[ 0 ].pPacketData = continuationFu;
    packetsArray[ 0 ].packetDataLength = sizeof( continuationFu );
    ctx.packetCount = 1;
    result = H265Depacketizer_GetNalu( &ctx, &nalu );
    TEST_ASSERT_EQUAL( H265_RESULT_MALFORMED_PACKET, result );
}

void test_H265_Depacketizer_ProcessFragmentedNalu_BufferOverflow( void )
{
    H265DepacketizerContext_t ctx;
    H265Result_t result;
    H265Packet_t packetsArray[ 10 ];
    H265Nalu_t nalu;

    /* Initialize context and arrays */
    memset( &ctx, 0, sizeof( H265DepacketizerContext_t ) );
    memset( packetsArray, 0, sizeof( packetsArray ) );
    memset( &nalu, 0, sizeof( H265Nalu_t ) );

    /* Create small reassembly buffer to force overflow */
    uint8_t reassemblyBuffer[ 5 ];  /* Deliberately small buffer */
    memset( reassemblyBuffer, 0, sizeof( reassemblyBuffer ) );

    /* Initialize test packets */
    uint8_t startFragment[] =
    {
        0x62,   /* NAL unit header byte 1: FU indicator */
        0x01,   /* NAL unit header byte 2 */
        0x82,   /* FU header: Start bit(0x80) + NAL type(0x02) */
        0x11,   /* Small payload that will fit */
        0x22
    };

    uint8_t endFragment[] =
    {
        0x62,   /* NAL unit header byte 1 */
        0x01,   /* NAL unit header byte 2 */
        0x42,   /* FU header: End bit(0x40) + NAL type(0x02) */
        0x33,   /* Larger payload that will cause overflow */
        0x44,
        0x55,
        0x66,
        0x77
    };

    /* Initialize depacketizer */
    result = H265Depacketizer_Init( &ctx, packetsArray, 10);
    TEST_ASSERT_EQUAL_MESSAGE( H265_RESULT_OK, result, "Depacketizer init failed" );

    /* Set up small reassembly buffer */
    ctx.fuDepacketizationState.pReassemblyBuffer = reassemblyBuffer;
    ctx.fuDepacketizationState.reassemblyBufferSize = sizeof( reassemblyBuffer );

    /* Process start fragment - should succeed */
    packetsArray[ 0 ].pPacketData = startFragment;
    packetsArray[ 0 ].packetDataLength = sizeof( startFragment );
    result = H265Depacketizer_AddPacket( &ctx, &packetsArray[ 0 ] );
    TEST_ASSERT_EQUAL_MESSAGE( H265_RESULT_OK, result, "Adding start fragment failed" );

    /* Get first NALU */
    result = H265Depacketizer_GetNalu( &ctx, &nalu );
    TEST_ASSERT_EQUAL_MESSAGE( H265_RESULT_OK, result, "Getting start fragment NALU failed" );

    /* Process end fragment - should fail due to buffer overflow */
    packetsArray[ 1 ].pPacketData = endFragment;
    packetsArray[ 1 ].packetDataLength = sizeof( endFragment );
    result = H265Depacketizer_AddPacket( &ctx, &packetsArray[ 1 ] );
    TEST_ASSERT_EQUAL_MESSAGE( H265_RESULT_OK, result, "Adding end fragment failed" );

    /* Try to get NALU - should return malformed packet */
    result = H265Depacketizer_GetNalu( &ctx, &nalu );
    TEST_ASSERT_EQUAL_MESSAGE( H265_RESULT_MALFORMED_PACKET, result,
                               "Expected malformed packet error due to buffer overflow" );
}



/*-----------------------------------------------------------------------------------------------------*/

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

void test_H265_Depacketizer_AP_NALUSize_TooLarge( void )
{
    H265DepacketizerContext_t ctx;
    H265Result_t result;
    H265Packet_t packetsArray[ 10 ];
    H265Frame_t frame = { 0 };

    /* Initialize everything */
    memset( &ctx, 0, sizeof( H265DepacketizerContext_t ) );
    memset( packetsArray, 0, sizeof( packetsArray ) );

    /* Initialize frame buffer */
    static uint8_t frameBuffer[ 3000 ];
    memset( frameBuffer, 0, sizeof( frameBuffer ) );
    frame.pFrameData = frameBuffer;
    frame.frameDataLength = sizeof( frameBuffer );

    /* Initialize depacketizer */
    result = H265Depacketizer_Init( &ctx, packetsArray, 10 );
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    /* Create AP packet with invalid NALU size */
    static const uint8_t apPacket[] =
    {
        0x61, 0x01,             /* AP indicator (type = 48) and layer/TID */
        0x00, 0xFF,             /* First NALU size (255 bytes - too large) */
        0x26, 0x01,             /* First NALU header */
        0xAA, 0xBB, 0xCC, 0xDD, /* First NALU payload */
        0x00, 0x04,             /* Second NALU length */
        0x40, 0x01              /* Second NALU header */
    };

    /* Add packet */
    packetsArray[ 0 ].pPacketData = ( uint8_t * ) apPacket;
    packetsArray[ 0 ].packetDataLength = sizeof( apPacket );
    ctx.packetCount = 1;
    ctx.tailIndex = 0;

    result = H265Depacketizer_AddPacket( &ctx, &packetsArray[ 0 ] );
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    /* Try to get frame - should fail */
    result = H265Depacketizer_GetFrame( &ctx, &frame );

    /* Verify results */
    TEST_ASSERT_EQUAL( H265_RESULT_MALFORMED_PACKET, result );
}

void test_H265_Depacketizer_AP_OutOfMemory( void )
{
    H265DepacketizerContext_t ctx;
    H265Result_t result;
    H265Packet_t packetsArray[ 10 ];
    H265Nalu_t nalu = { 0 };

    /* Initialize everything */
    memset( &ctx, 0, sizeof( H265DepacketizerContext_t ) );
    memset( packetsArray, 0, sizeof( packetsArray ) );

    /* Initialize NALU buffer - intentionally small */
    uint8_t naluBuffer[ 10 ];
    memset( naluBuffer, 0, sizeof( naluBuffer ) );
    nalu.pNaluData = naluBuffer;
    nalu.naluDataLength = sizeof( naluBuffer );

    /* Initialize depacketizer */
    result = H265Depacketizer_Init( &ctx, packetsArray, 10 );
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    /* Create AP packet:
     * - Valid AP header
     * - Valid packet length
     * - NALU size larger than buffer
     */
    static const uint8_t apPacket[] =
    {
        0x60, 0x00,             /* AP indicator (type = 48) and layer/TID */
        0x00, 0x15,             /* First NALU size (21 bytes - larger than buffer) */
        0x26, 0x01,             /* NALU header */
        0x00, 0x00, 0x00, 0x00, /* Payload */
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00       /* 21 bytes total for first NALU */
    };

    /* Add packet */
    H265Packet_t inputPacket =
    {
        .pPacketData      = ( uint8_t * ) apPacket,
        .packetDataLength = sizeof( apPacket )
    };

    result = H265Depacketizer_AddPacket( &ctx, &inputPacket );
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    /* Try to get NALU - should fail with OUT_OF_MEMORY */
    result = H265Depacketizer_GetNalu( &ctx, &nalu );

    /* Verify results */
    TEST_ASSERT_EQUAL( H265_RESULT_OUT_OF_MEMORY, result );
}

void test_H265_Depacketizer_AP_OutOfMemory_SecondNalu( void )
{
    H265DepacketizerContext_t ctx;
    H265Result_t result;
    H265Packet_t packetsArray[ 10 ];
    H265Nalu_t nalu = { 0 };

    /* Initialize everything */
    memset( &ctx, 0, sizeof( H265DepacketizerContext_t ) );
    memset( packetsArray, 0, sizeof( packetsArray ) );

    /* Initialize NALU buffer - intentionally small */
    static uint8_t naluBuffer[ 10 ];
    memset( naluBuffer, 0, sizeof( naluBuffer ) );
    nalu.pNaluData = naluBuffer;
    nalu.naluDataLength = sizeof( naluBuffer );

    /* Initialize depacketizer */
    result = H265Depacketizer_Init( &ctx, packetsArray, 10);
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    /* Create AP packet:
     * - First NALU small enough to fit
     * - Second NALU too large for buffer
     */
    static const uint8_t apPacket[] =
    {
        0x60, 0x00,             /* AP indicator (type = 48) and layer/TID */
        0x00, 0x04,             /* First NALU size (4 bytes) */
        0x26, 0x01,             /* First NALU header */
        0xAA, 0xBB,             /* First NALU payload */
        0x00, 0x15,             /* Second NALU size (21 bytes - larger than buffer) */
        0x40, 0x01,             /* Second NALU header */
        0x00, 0x00, 0x00, 0x00, /* Second NALU payload */
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00       /* 21 bytes total for second NALU */
    };

    /* Add packet */
    H265Packet_t inputPacket =
    {
        .pPacketData      = ( uint8_t * ) apPacket,
        .packetDataLength = sizeof( apPacket )
    };

    result = H265Depacketizer_AddPacket( &ctx, &inputPacket );
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    /* Get first NALU - should succeed */
    result = H265Depacketizer_GetNalu( &ctx, &nalu );
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    /* Try to get second NALU - should fail with OUT_OF_MEMORY */
    result = H265Depacketizer_GetNalu( &ctx, &nalu );

    /* Verify results */
    TEST_ASSERT_EQUAL( H265_RESULT_OUT_OF_MEMORY, result );
}

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
    static uint8_t naluBuffer[ 20 ];
    memset( naluBuffer, 0, sizeof( naluBuffer ) );
    nalu.pNaluData = naluBuffer;
    nalu.naluDataLength = sizeof( naluBuffer );

    /* Initialize depacketizer */
    result = H265Depacketizer_Init( &ctx, packetsArray, 10 );
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    /* Create AP packet with three NALUs:
     * Bytes 0-1:   AP header (2 bytes)
     * Bytes 2-7:   First NALU (2 bytes size + 4 bytes data)
     * Bytes 8-13:  Second NALU (2 bytes size + 4 bytes data)
     * Bytes 14-19: Third NALU (2 bytes size + 4 bytes data)
     */
    static const uint8_t apPacket[] =
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

    /* Verify currentOffset is set to start of second NALU */
    TEST_ASSERT_EQUAL( 8, ctx.apDepacketizationState.currentOffset );

    /* Get second NALU */
    result = H265Depacketizer_GetNalu( &ctx, &nalu );
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    /* Verify currentOffset is set to start of third NALU */
    TEST_ASSERT_EQUAL( 14, ctx.apDepacketizationState.currentOffset );

    /* Get third NALU */
    result = H265Depacketizer_GetNalu( &ctx, &nalu );
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    /* Verify packet is completed */
    TEST_ASSERT_EQUAL( H265_PACKET_NONE, ctx.currentlyProcessingPacket );
}

/*-------------------------------------------------------------------------------------------------------------*/

void test_H265_Depacketizer_GetPacketProperties( void )
{
    H265Result_t result;
    uint32_t properties;

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
}

/*-------------------------------------------------------------------------------------------------------------*/

void test_H265_Depacketizer_BAD_PARAMS( void )
{
    H265DepacketizerContext_t ctx;
    H265Result_t result;
    H265Packet_t packetsArray[ 10 ];
    H265Nalu_t nalu;
    H265Frame_t frame = { 0 };
    uint32_t properties;

    // /* Initialize buffers */
    uint8_t reassemblyBuffer[ 1500 ];
    // uint8_t frameBuffer[ 3000 ];

    /* Test 1: Parameter Validation */
    /* Init NULL checks */
    result = H265Depacketizer_Init( NULL, packetsArray, 10);
    TEST_ASSERT_EQUAL( H265_RESULT_BAD_PARAM, result );

    result = H265Depacketizer_Init( &ctx, NULL, 10);
    TEST_ASSERT_EQUAL( H265_RESULT_BAD_PARAM, result );

    result = H265Depacketizer_Init( &ctx, packetsArray, 0);
    TEST_ASSERT_EQUAL( H265_RESULT_BAD_PARAM, result );

    /* GetFrame NULL checks */
    result = H265Depacketizer_GetFrame( NULL, &frame );
    TEST_ASSERT_EQUAL( H265_RESULT_BAD_PARAM, result );

    result = H265Depacketizer_GetFrame( &ctx, NULL );
    TEST_ASSERT_EQUAL( H265_RESULT_BAD_PARAM, result );

    frame.pFrameData = NULL;
    frame.frameDataLength = sizeof( frameBuffer );
    result = H265Depacketizer_GetFrame( &ctx, &frame );
    TEST_ASSERT_EQUAL( H265_RESULT_BAD_PARAM, result );

    /* Test zero frame length */
    frame.pFrameData = frameBuffer;
    frame.frameDataLength = 0;
    result = H265Depacketizer_GetFrame( &ctx, &frame );
    TEST_ASSERT_EQUAL( H265_RESULT_BAD_PARAM, result );

    result = H265Depacketizer_GetNalu( &ctx, NULL );
    TEST_ASSERT_EQUAL( H265_RESULT_BAD_PARAM, result );

    result = H265Depacketizer_AddPacket( NULL, NULL );
    TEST_ASSERT_EQUAL( H265_RESULT_BAD_PARAM, result );

    result = H265Depacketizer_AddPacket( &ctx, NULL );
    TEST_ASSERT_EQUAL( H265_RESULT_BAD_PARAM, result );

    /* Test 2: Malformed Packets */
    /* Initialize context properly */
    memset( &ctx, 0, sizeof( ctx ) );
    ctx.fuDepacketizationState.pReassemblyBuffer = reassemblyBuffer;
    ctx.fuDepacketizationState.reassemblyBufferSize = sizeof( reassemblyBuffer );
    frame.pFrameData = frameBuffer;
    frame.frameDataLength = sizeof( frameBuffer );
    result = H265Depacketizer_Init( &ctx, packetsArray, 10);
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );
    
    /* FU packet too small */
    uint8_t smallFuPacket[] =
    {
        0x62, 0x01  /* Too small for FU packet */
    };
    result = H265Depacketizer_GetPacketProperties( smallFuPacket,
                                                   sizeof( smallFuPacket ),
                                                   &properties );
    TEST_ASSERT_EQUAL( H265_RESULT_MALFORMED_PACKET, result );

    /* AP packet too small */
    uint8_t smallApPacket[] =
    {
        0x61, 0x01  /* Too small for AP packet */
    };
    result = H265Depacketizer_GetPacketProperties( smallApPacket,
                                                   sizeof( smallApPacket ),
                                                   &properties );
    TEST_ASSERT_EQUAL( H265_RESULT_MALFORMED_PACKET, result );

    /* Test 3: Malformed AP packets with DONL */         
    result = H265Depacketizer_Init( &ctx, packetsArray, 10);
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );
    uint8_t invalidApPacket[] =
    {
        0x61, 0x01,           /* AP header */
        0x00, 0x04            /* Incomplete packet */
    };
    packetsArray[ 0 ].pPacketData = invalidApPacket;
    packetsArray[ 0 ].packetDataLength = sizeof( invalidApPacket );
    ctx.packetCount = 1;
    result = H265Depacketizer_GetNalu( &ctx, &nalu );
    TEST_ASSERT_EQUAL( H265_RESULT_MALFORMED_PACKET, result );

    /* Test 4: Unsupported packet types */
    uint8_t unsupportedPacket[] =
    {
        0xFF, 0x01,  /* Invalid NAL unit type */
        0x00, 0x00
    };
    result = H265Depacketizer_GetPacketProperties( unsupportedPacket,
                                                   sizeof( unsupportedPacket ),
                                                   &properties );
    TEST_ASSERT_EQUAL( H265_RESULT_UNSUPPORTED_PACKET, result );
    uint8_t belowMinPacket[] =
    {
        0x00, 0x01,  /* NAL type 0 (below minimum) */
        0xAA, 0xBB
    };
    result = H265Depacketizer_GetPacketProperties( belowMinPacket,
                                                   sizeof( belowMinPacket ),
                                                   &properties );
    TEST_ASSERT_EQUAL( H265_RESULT_UNSUPPORTED_PACKET, result );

    /* Test 5: No more NALUs/Frames */
    ctx.packetCount = 0;
    frame.frameDataLength = sizeof( frameBuffer );
    result = H265Depacketizer_GetFrame( &ctx, &frame );
    TEST_ASSERT_EQUAL( H265_RESULT_NO_MORE_FRAMES, result );
}

/*-------------------------------------------------------------------------------------------------------------*/

void test_H265_Depacketizer_SingleNALU_Complete( void )
{
    H265DepacketizerContext_t ctx;
    H265Result_t result;
    H265Packet_t packetsArray[ 10 ];
    H265Frame_t frame = { 0 };
    uint32_t properties;

    /* Initialize test packet - Single NALU */
    uint8_t singleNaluPacket[] =
    {
        0x26, 0x01,           /* NAL header (type 0x13) */
        0xAA, 0xBB, 0xCC      /* Payload */
    };

    /* Step 1: Get packet properties */
    result = H265Depacketizer_GetPacketProperties( singleNaluPacket,
                                                   sizeof( singleNaluPacket ),
                                                   &properties );
    TEST_ASSERT_EQUAL_MESSAGE( H265_RESULT_OK, result, "GetPacketProperties failed" );
    TEST_ASSERT_EQUAL_MESSAGE( H265_PACKET_PROPERTY_START_PACKET | H265_PACKET_PROPERTY_END_PACKET,
                               properties, "Incorrect packet properties" );

    /* Step 2: Initialize depacketizer */
    /* Initialize context and arrays */
    memset( &ctx, 0, sizeof( H265DepacketizerContext_t ) );
    memset( packetsArray, 0, sizeof( packetsArray ) );

    uint8_t reassemblyBuffer[ 1500 ];
    uint8_t frameBuffer[ 1500 ];
    memset( reassemblyBuffer, 0, sizeof( reassemblyBuffer ) );
    memset( frameBuffer, 0, sizeof( frameBuffer ) );

    /* Initialize frame buffer */
    frame.pFrameData = frameBuffer;
    frame.frameDataLength = sizeof( frameBuffer );

    /* Initialize depacketizer */
    result = H265Depacketizer_Init( &ctx, packetsArray, 10 );  /* No DONL */
    TEST_ASSERT_EQUAL_MESSAGE( H265_RESULT_OK, result, "Init failed" );

    /* Step 3: Add packet */
    packetsArray[ 0 ].pPacketData = singleNaluPacket;
    packetsArray[ 0 ].packetDataLength = sizeof( singleNaluPacket );
    result = H265Depacketizer_AddPacket( &ctx, &packetsArray[ 0 ] );
    TEST_ASSERT_EQUAL_MESSAGE( H265_RESULT_OK, result, "AddPacket failed" );
    TEST_ASSERT_EQUAL_MESSAGE( 1, ctx.packetCount, "Incorrect packet count" );

    /* Step 4: Get Frame */
    result = H265Depacketizer_GetFrame( &ctx, &frame );

    /* Verify frame */
    TEST_ASSERT_EQUAL_MESSAGE( H265_RESULT_OK, result, "GetFrame failed" );
    TEST_ASSERT_NOT_NULL_MESSAGE( frame.pFrameData, "Frame data is NULL" );
    TEST_ASSERT_GREATER_THAN_MESSAGE( 0, frame.frameDataLength, "Frame length is 0" );

    /* Verify frame contents */
    const size_t expectedFrameLength = 9;  /* 4 bytes start code + 5 bytes NALU */
    TEST_ASSERT_EQUAL_MESSAGE( expectedFrameLength, frame.frameDataLength,
                               "Incorrect frame length" );

    /* Verify start code */
    TEST_ASSERT_EQUAL_HEX8_MESSAGE( 0x00, frame.pFrameData[ 0 ], "Incorrect start code byte 1" );
    TEST_ASSERT_EQUAL_HEX8_MESSAGE( 0x00, frame.pFrameData[ 1 ], "Incorrect start code byte 2" );
    TEST_ASSERT_EQUAL_HEX8_MESSAGE( 0x00, frame.pFrameData[ 2 ], "Incorrect start code byte 3" );
    TEST_ASSERT_EQUAL_HEX8_MESSAGE( 0x01, frame.pFrameData[ 3 ], "Incorrect start code byte 4" );

    /* Verify NALU contents */
    TEST_ASSERT_EQUAL_HEX8_MESSAGE( 0x26, frame.pFrameData[ 4 ], "Incorrect NAL header byte 1" );
    TEST_ASSERT_EQUAL_HEX8_MESSAGE( 0x01, frame.pFrameData[ 5 ], "Incorrect NAL header byte 2" );
    TEST_ASSERT_EQUAL_HEX8_MESSAGE( 0xAA, frame.pFrameData[ 6 ], "Incorrect payload byte 1" );
    TEST_ASSERT_EQUAL_HEX8_MESSAGE( 0xBB, frame.pFrameData[ 7 ], "Incorrect payload byte 2" );
    TEST_ASSERT_EQUAL_HEX8_MESSAGE( 0xCC, frame.pFrameData[ 8 ], "Incorrect payload byte 3" );

    /* Verify final state */
    TEST_ASSERT_EQUAL_MESSAGE( 0, ctx.packetCount, "Packet count not zero after GetFrame" );
}

/*-------------------------------------------------------------------------------------------------------------*/

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
}

/*-------------------------------------------------------------------------------------------------------------*/

void test_H265_Depacketizer_AddPacket_NoMoreSpace( void )
{
    H265DepacketizerContext_t ctx;
    H265Result_t result;
    H265Packet_t packetsArray[ 2 ];  /* Small array to make it easier to fill */
    H265Packet_t testPacket;

    /* Initialize context */
    memset( &ctx, 0, sizeof( H265DepacketizerContext_t ) );
    memset( packetsArray, 0, sizeof( packetsArray ) );

    /* Initialize depacketizer with small array size */
    result = H265Depacketizer_Init( &ctx, packetsArray, 2);  /* Array size = 2 */
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    /* Setup test packet */
    uint8_t packetData[] =
    {
        0x26, 0x01,  /* NAL header */
        0xAA, 0xBB   /* Payload */
    };
    testPacket.pPacketData = packetData;
    testPacket.packetDataLength = sizeof( packetData );

    /* Fill up the array */
    for(size_t i = 0; i < ctx.packetsArrayLength; i++)
    {
        result = H265Depacketizer_AddPacket( &ctx, &testPacket );
        TEST_ASSERT_EQUAL( H265_RESULT_OK, result );
    }

    result = H265Depacketizer_AddPacket( &ctx, &testPacket );

    /* Verify result */
    TEST_ASSERT_EQUAL( H265_RESULT_NO_MORE_PACKETS, result );
    TEST_ASSERT_EQUAL( ctx.packetsArrayLength, ctx.packetCount );
}

/*-------------------------------------------------------------------------------------------------------------*/

void test_H265Depacketizer_GetFrame_BufferTooSmall( void )
{
    H265DepacketizerContext_t ctx;
    H265Result_t result;
    H265Packet_t packetsArray[ 10 ];
    H265Frame_t frame;

    /* Initialize all structures */
    memset( &ctx, 0, sizeof( H265DepacketizerContext_t ) );
    memset( packetsArray, 0, sizeof( packetsArray ) );

    /* Setup reassembly buffer */
    uint8_t reassemblyBuffer[ 1500 ];
    ctx.fuDepacketizationState.pReassemblyBuffer = reassemblyBuffer;
    ctx.fuDepacketizationState.reassemblyBufferSize = sizeof( reassemblyBuffer );

    /* Create test packet with single NAL unit */
    uint8_t packetData[] =
    {
        0x26, 0x01,                  /* NAL unit header (type within valid single NAL unit range) */
        0xAA, 0xBB, 0xCC, 0xDD, 0xEE /* Payload */
    };

    /* Create deliberately small frame buffer */
    uint8_t frameBuffer[ 6 ];   /* Only 6 bytes: not enough for start code (4) + NAL unit (7) */
    frame.pFrameData = frameBuffer;
    frame.frameDataLength = sizeof( frameBuffer );

    /* Setup context */
    packetsArray[ 0 ].pPacketData = packetData;
    packetsArray[ 0 ].packetDataLength = sizeof( packetData );
    ctx.pPacketsArray = packetsArray;
    ctx.packetCount = 1;
    ctx.tailIndex = 0;

    /* Initialize depacketizer */
    result = H265Depacketizer_Init( &ctx, packetsArray, 1);
    TEST_ASSERT_EQUAL_MESSAGE( H265_RESULT_OK, result, "Depacketizer init failed" );

    /* Add packet */
    result = H265Depacketizer_AddPacket( &ctx, &packetsArray[ 0 ] );
    TEST_ASSERT_EQUAL_MESSAGE( H265_RESULT_OK, result, "Failed to add packet" );

    /* Try to get frame - should fail due to small buffer */
    result = H265Depacketizer_GetFrame( &ctx, &frame );
    TEST_ASSERT_EQUAL_MESSAGE( H265_RESULT_BUFFER_TOO_SMALL, result,
                               "Expected buffer too small error when frame buffer cannot hold start code + NALU" );
}

/*-------------------------------------------------------------------------------------------------------------*/