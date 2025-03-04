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
    result = H265Packetizer_Init( &ctx, naluArray, MAX_NALUS_IN_A_FRAME, 0, MAX_H265_PACKET_LENGTH );
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
    packet.maxPacketSize = MAX_H265_PACKET_LENGTH;
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
    result = H265Packetizer_Init( &ctx, naluArray, MAX_NALUS_IN_A_FRAME, 0, MAX_H265_PACKET_LENGTH );
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
    packet.maxPacketSize = MAX_H265_PACKET_LENGTH;
    result = H265Packetizer_GetPacket( &ctx, &packet );
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );
    TEST_ASSERT_EQUAL( sizeof( nalData ), packet.packetDataLength );

    /* Verify packet data */
    TEST_ASSERT_EQUAL_MEMORY( nalData, packet.pPacketData, packet.packetDataLength );
}


void test_H265_Packetizer_Single_NAL_With_DONL( void )
{
    H265PacketizerContext_t ctx;
    H265Result_t result;
    H265Nalu_t naluArray[ MAX_NALUS_IN_A_FRAME ];
    H265Packet_t packet;

    /* Create a NAL unit */
    uint8_t nalData[ 50 ] =
    {
        0x40, 0x01,      /* NAL header: F=0, Type=32 (VPS), LayerId=0, TID=1 */
        0xAA, 0xBB, 0xCC /* Payload pattern */
    };

    memset( nalData + 5, 0xDD, sizeof( nalData ) - 5 );

    /* Initialize context with DONL enabled */
    result = H265Packetizer_Init( &ctx, naluArray, MAX_NALUS_IN_A_FRAME, 1, 100 );
    ctx.spropMaxDonDiff = 3;  /* Enable DONL by setting spropMaxDonDiff */
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    /* Add NAL unit with specific DON value */
    H265Nalu_t nalu =
    {
        .pNaluData      = nalData,
        .naluDataLength = sizeof( nalData ),
        .nal_unit_type  = 32,
        .nal_layer_id   = 0,
        .temporal_id    = 1,
        .don            = 5
    };
    result = H265Packetizer_AddNalu( &ctx, &nalu );
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    /* Get packet */
    packet.pPacketData = packetBuffer;
    packet.maxPacketSize = 100;
    result = H265Packetizer_GetPacket( &ctx, &packet );
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    /* Verify packet structure */
    /* 1. Verify NAL header (first 2 bytes) */
    TEST_ASSERT_EQUAL( 0x40, packet.pPacketData[ 0 ] );
    TEST_ASSERT_EQUAL( 0x01, packet.pPacketData[ 1 ] );

    /* 2. Verify DONL value (next 2 bytes) */
    uint16_t donl = ( packet.pPacketData[ 2 ] << 8 ) | packet.pPacketData[ 3 ];
    TEST_ASSERT_EQUAL( 5, donl );


    /* Verify original payload pattern */
    TEST_ASSERT_EQUAL( 0xAA, packet.pPacketData[ 4 ] );
    TEST_ASSERT_EQUAL( 0xBB, packet.pPacketData[ 5 ] );
    TEST_ASSERT_EQUAL( 0xCC, packet.pPacketData[ 6 ] );

    /* Verify rest of payload */
    for(size_t i = 7; i < packet.packetDataLength; i++)
    {
        TEST_ASSERT_EQUAL( 0xDD, packet.pPacketData[ i ] );
    }

    /* 4. Verify total packet length */
    size_t expectedLength = sizeof( nalData ) + DONL_FIELD_SIZE;
    TEST_ASSERT_EQUAL( expectedLength, packet.packetDataLength );

    /* Memory comparison tests */
    /* Verify NAL header */
    TEST_ASSERT_EQUAL_MEMORY( nalData, packet.pPacketData, NALU_HEADER_SIZE );

    /* Verify payload (excluding header) */
    TEST_ASSERT_EQUAL_MEMORY(
        &nalData[ NALU_HEADER_SIZE ],
        &packet.pPacketData[ NALU_HEADER_SIZE + DONL_FIELD_SIZE ],
        sizeof( nalData ) - NALU_HEADER_SIZE
        );

    /* Verify context state */
    TEST_ASSERT_EQUAL( 0, ctx.naluCount );
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
    result = H265Packetizer_Init( &ctx, naluArray, MAX_NALUS_IN_A_FRAME, 0, 100 );
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
    packet.maxPacketSize = 100;
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
        packet.maxPacketSize = 100;
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


void test_H265_Packetizer_Multiple_NALs_Fragmentation_With_DONL( void )
{
    H265PacketizerContext_t ctx;
    H265Result_t result;
    H265Nalu_t naluArray[ MAX_NALUS_IN_A_FRAME ];
    H265Packet_t packet;

    /* Create NAL units */
    uint8_t nalData1[ 250 ] =
    {
        0x40, 0x01,  /* NAL header */
        0xCC, 0xCC   /* Start payload with 0xCC */
    };

    memset( nalData1 + 2, 0xCC, sizeof( nalData1 ) - 2 );  /* Fill rest with 0xCC */

    uint8_t nalData2[ 200 ] =
    {
        0x40, 0x01,                                       /* NAL header */
        0xDD, 0xDD                                        /* Start payload with 0xDD */
    };
    memset( nalData2 + 2, 0xDD, sizeof( nalData2 ) - 2 ); /* Fill rest with 0xDD */

    /* Initialize context */
    result = H265Packetizer_Init( &ctx, naluArray, MAX_NALUS_IN_A_FRAME, 1, 100 );
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
        .nal_unit_type  = 32,
        .nal_layer_id   = 0,
        .temporal_id    = 1
    };
    result = H265Packetizer_AddNalu( &ctx, &nalu2 );
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    uint32_t packetCount = 0;
    uint16_t expectedDon = 0;
    uint8_t processingFirstNalu = 1;              /* 0 = false, 1 = true */

    do
    {
        packet.pPacketData = packetBuffer;
        packet.maxPacketSize = 100;
        result = H265Packetizer_GetPacket( &ctx, &packet );
        TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

        packetCount++;
        uint8_t fuHeader = packet.pPacketData[ 2 ];

        /* Common header checks */
        TEST_ASSERT_EQUAL( 0x62, packet.pPacketData[ 0 ] );  /* First byte of PayloadHdr */
        TEST_ASSERT_EQUAL( 0x01, packet.pPacketData[ 1 ] );  /* Second byte of PayloadHdr */

        if( fuHeader & FU_HEADER_S_BIT_MASK )
        {
            /* First fragment */
            uint16_t donl = ( packet.pPacketData[ 3 ] << 8 ) | packet.pPacketData[ 4 ];
            TEST_ASSERT_EQUAL( expectedDon, donl );

            /* Verify payload */
            uint8_t expectedPattern = processingFirstNalu ? 0xCC : 0xDD;

            for(size_t i = 5; i < packet.packetDataLength; i++)
            {
                TEST_ASSERT_EQUAL_HEX8( expectedPattern, packet.pPacketData[ i ] );
            }
        }
        else
        {
            /* Middle or last fragment */
            uint8_t dond = packet.pPacketData[ 3 ];
            TEST_ASSERT_EQUAL( 0, dond );

            /* Verify payload */
            uint8_t expectedPattern = processingFirstNalu ? 0xCC : 0xDD;

            for(size_t i = 4; i < packet.packetDataLength; i++)
            {
                TEST_ASSERT_EQUAL_HEX8( expectedPattern, packet.pPacketData[ i ] );
            }

            if( fuHeader & FU_HEADER_E_BIT_MASK )
            {
                if( processingFirstNalu )
                {
                    processingFirstNalu = 0;
                    expectedDon++;
                }
            }
        }
    } while( result == H265_RESULT_OK && ctx.naluCount > 0 );

    TEST_ASSERT_EQUAL( 6, packetCount );
    TEST_ASSERT_EQUAL( 0, ctx.naluCount );
}


void test_H265_Packetizer_Null_Packet_Data( void )
{
    H265PacketizerContext_t ctx = { 0 };
    H265Packet_t packet = { 0 };

    /* Initialize the context if needed */
    H265Packetizer_Init( &ctx,
                         NULL,   /* pNaluArray */
                         0,      /* naluArrayLength */
                         1,      /* spropMaxDonDiff */
                         1500 ); /* maxPacketSize */

    /* Setup the test packet */
    packet.pPacketData = NULL;  /* This is the key test condition */

    /* Call the function that contains the NULL check */
    H265Packetizer_GetPacket( &ctx, &packet );
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
                                  10,
                                  0,
                                  1500 );
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
    packet.maxPacketSize = 3;  /* Just enough for headers (2 bytes PayloadHdr + 1 byte FUHeader) */
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
    result = H265Packetizer_Init( &ctx, naluArray, MAX_NALUS_IN_A_FRAME, 0, 100 );
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
    packet.maxPacketSize = 100;
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

void test_H265_Packetizer_Aggregation_With_DONL( void )
{
    H265PacketizerContext_t ctx;
    H265Result_t result;
    H265Nalu_t naluArray[ MAX_NALUS_IN_A_FRAME ];
    H265Packet_t packet;

    /* Create small NAL units suitable for aggregation */
    uint8_t nalData1[ 20 ] =
    {
        0x40, 0x01,  /* NAL header: F=0, Type=32 (VPS), LayerId=0, TID=1 */
        0xAA, 0xAA   /* Payload pattern */
    };

    memset( nalData1 + 4, 0xAA, sizeof( nalData1 ) - 4 );

    uint8_t nalData2[ 15 ] =
    {
        0x40, 0x01,  /* NAL header */
        0xBB, 0xBB   /* Different payload pattern */
    };
    memset( nalData2 + 4, 0xBB, sizeof( nalData2 ) - 4 );

    uint8_t nalData3[ 25 ] =
    {
        0x40, 0x01,  /* NAL header */
        0xCC, 0xCC   /* Different payload pattern */
    };
    memset( nalData3 + 4, 0xCC, sizeof( nalData3 ) - 4 );

    /* Initialize context with DONL enabled */
    result = H265Packetizer_Init( &ctx, naluArray, MAX_NALUS_IN_A_FRAME, 1, 100 );
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
        .nal_unit_type  = 32,
        .nal_layer_id   = 0,
        .temporal_id    = 1
    };
    result = H265Packetizer_AddNalu( &ctx, &nalu2 );
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    H265Nalu_t nalu3 =
    {
        .pNaluData      = nalData3,
        .naluDataLength = sizeof( nalData3 ),
        .nal_unit_type  = 32,
        .nal_layer_id   = 0,
        .temporal_id    = 1
    };
    result = H265Packetizer_AddNalu( &ctx, &nalu3 );
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    /* Get aggregation packet */
    packet.pPacketData = packetBuffer;
    packet.maxPacketSize = 100;
    result = H265Packetizer_GetPacket( &ctx, &packet );
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    /* Verify AP header */
    TEST_ASSERT_EQUAL( 0x60, packet.pPacketData[ 0 ] & 0xFE ); /* Type = 48 (AP) */
    TEST_ASSERT_EQUAL( 0x01, packet.pPacketData[ 1 ] );        /* TID = 1 */

    size_t offset = 2;                                         /* Skip PayloadHdr */

    /* Process first NAL in aggregation */
    {
        /* Get NALU length */
        uint16_t naluLength = ( packet.pPacketData[ offset ] << 8 ) |
                              packet.pPacketData[ offset + 1 ];
        TEST_ASSERT_EQUAL( sizeof( nalData1 ), naluLength );
        offset += 2;  /* Skip length field */

        /* Check DONL */
        uint16_t donl = ( packet.pPacketData[ offset ] << 8 ) |
                        packet.pPacketData[ offset + 1 ];
        TEST_ASSERT_EQUAL( 0, donl ); /* First NAL, DON = 0 */
        offset += 2;                  /* Skip DONL */

        for(size_t i = 0; i < naluLength; i++)
        {
            TEST_ASSERT_EQUAL( nalData1[ i ], packet.pPacketData[ offset + i ] );
        }

        offset += naluLength;
    }

    /* Process second NAL in aggregation */
    {
        uint16_t naluLength = ( packet.pPacketData[ offset ] << 8 ) |
                              packet.pPacketData[ offset + 1 ];
        TEST_ASSERT_EQUAL( sizeof( nalData2 ), naluLength );
        offset += 2;

        /* Check DOND */
        uint8_t dond = packet.pPacketData[ offset ];
        TEST_ASSERT_EQUAL( 0, dond ); /* Difference from previous DON */
        offset += 1;                  /* Skip DOND */

        for(size_t i = 0; i < naluLength; i++)
        {
            TEST_ASSERT_EQUAL( nalData2[ i ], packet.pPacketData[ offset + i ] );
        }

        offset += naluLength;
    }

    /* Process third NAL in aggregation */
    {
        uint16_t naluLength = ( packet.pPacketData[ offset ] << 8 ) |
                              packet.pPacketData[ offset + 1 ];
        TEST_ASSERT_EQUAL( sizeof( nalData3 ), naluLength );
        offset += 2;

        /* Check DOND */
        uint8_t dond = packet.pPacketData[ offset ];
        TEST_ASSERT_EQUAL( 0, dond );
        offset += 1;

        for(size_t i = 0; i < naluLength; i++)
        {
            TEST_ASSERT_EQUAL( nalData3[ i ], packet.pPacketData[ offset + i ] );
        }

        offset += naluLength;
    }

    /* Verify final packet size */
    TEST_ASSERT_EQUAL( offset, packet.packetDataLength );

    /* Verify context state */
    TEST_ASSERT_EQUAL( 0, ctx.naluCount );
}



void test_H265_Packetizer_Aggregation_DON_Sequence( void )
{
    H265PacketizerContext_t ctx;
    H265Result_t result;
    H265Nalu_t naluArray[ MAX_NALUS_IN_A_FRAME ];
    H265Packet_t packet;

    /* Initialize context */
    result = H265Packetizer_Init( &ctx, naluArray, MAX_NALUS_IN_A_FRAME, 1, 100 );
    ctx.spropMaxDonDiff = 3;  /* Set maximum DON difference */
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    /* Create NAL units with different DON values */
    uint8_t nalData1[ 20 ] = { 0x40, 0x01 };  /* NAL header */
    uint8_t nalData2[ 15 ] = { 0x40, 0x01 };  /* NAL header */
    uint8_t nalData3[ 25 ] = { 0x40, 0x01 };  /* NAL header */

    memset( nalData1 + 2, 0xAA, sizeof( nalData1 ) - 2 );
    memset( nalData2 + 2, 0xBB, sizeof( nalData2 ) - 2 );
    memset( nalData3 + 2, 0xCC, sizeof( nalData3 ) - 2 );

    /* Add NAL units with specific DON values */
    H265Nalu_t nalu1 =
    {
        .pNaluData      = nalData1,
        .naluDataLength = sizeof( nalData1 ),
        .nal_unit_type  = 32,
        .nal_layer_id   = 0,
        .temporal_id    = 1,
        .don            = 5
    };
    result = H265Packetizer_AddNalu( &ctx, &nalu1 );
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    H265Nalu_t nalu2 =
    {
        .pNaluData      = nalData2,
        .naluDataLength = sizeof( nalData2 ),
        .nal_unit_type  = 32,
        .nal_layer_id   = 0,
        .temporal_id    = 1,
        .don            = 6
    };
    result = H265Packetizer_AddNalu( &ctx, &nalu2 );
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    H265Nalu_t nalu3 =
    {
        .pNaluData      = nalData3,
        .naluDataLength = sizeof( nalData3 ),
        .nal_unit_type  = 32,
        .nal_layer_id   = 0,
        .temporal_id    = 1,
        .don            = 7
    };
    result = H265Packetizer_AddNalu( &ctx, &nalu3 );
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    /* Verify context state before getting packet */
    TEST_ASSERT_EQUAL( 3, ctx.naluCount );
    TEST_ASSERT_EQUAL( 5, ctx.pNaluArray[ 0 ].don );
    TEST_ASSERT_EQUAL( 6, ctx.pNaluArray[ 1 ].don );
    TEST_ASSERT_EQUAL( 7, ctx.pNaluArray[ 2 ].don );

    /* Get aggregation packet */
    packet.pPacketData = packetBuffer;
    packet.maxPacketSize = 100;
    result = H265Packetizer_GetPacket( &ctx, &packet );
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    /* Verify AP header */
    TEST_ASSERT_EQUAL( 0x60, packet.pPacketData[ 0 ] & 0xFE ); /* Type = 48 (AP) */
    TEST_ASSERT_EQUAL( 0x01, packet.pPacketData[ 1 ] );        /* TID = 1 */

    size_t offset = 2;                                         /* Skip PayloadHdr */

    /* Process first NAL in aggregation */
    {
        uint16_t naluLength = ( packet.pPacketData[ offset ] << 8 ) |
                              packet.pPacketData[ offset + 1 ];
        TEST_ASSERT_EQUAL( sizeof( nalData1 ), naluLength );
        offset += 2;

        uint16_t donl = ( packet.pPacketData[ offset ] << 8 ) |
                        packet.pPacketData[ offset + 1 ];
        TEST_ASSERT_EQUAL( 5, donl );
        offset += 2;

        TEST_ASSERT_EQUAL_MEMORY( nalData1, &packet.pPacketData[ offset ], naluLength );
        offset += naluLength;
    }

    /* Process second NAL */
    {
        uint16_t naluLength = ( packet.pPacketData[ offset ] << 8 ) |
                              packet.pPacketData[ offset + 1 ];
        TEST_ASSERT_EQUAL( sizeof( nalData2 ), naluLength );
        offset += 2;

        uint8_t dond = packet.pPacketData[ offset ];
        TEST_ASSERT_EQUAL( 1, dond );  /* Difference between DON 6 and 5 */
        offset += 1;

        TEST_ASSERT_EQUAL_MEMORY( nalData2, &packet.pPacketData[ offset ], naluLength );
        offset += naluLength;
    }

    /* Process third NAL */
    {
        uint16_t naluLength = ( packet.pPacketData[ offset ] << 8 ) |
                              packet.pPacketData[ offset + 1 ];
        TEST_ASSERT_EQUAL( sizeof( nalData3 ), naluLength );
        offset += 2;

        uint8_t dond = packet.pPacketData[ offset ];
        TEST_ASSERT_EQUAL( 1, dond );  /* Difference between DON 7 and 6 */
        offset += 1;

        TEST_ASSERT_EQUAL_MEMORY( nalData3, &packet.pPacketData[ offset ], naluLength );
        offset += naluLength;
    }

    /* Verify final packet size */
    TEST_ASSERT_EQUAL( offset, packet.packetDataLength );

    /* Verify DON difference is within spropMaxDonDiff */
    uint16_t maxDonDiff = nalu3.don - nalu1.don;
    TEST_ASSERT_TRUE( maxDonDiff <= ctx.spropMaxDonDiff );

    /* Verify context state after processing */
    TEST_ASSERT_EQUAL( 0, ctx.naluCount );
}


void test_H265_Packetizer_Aggregation_Size_Overflow( void )
{
    H265PacketizerContext_t ctx;
    H265Result_t result;
    H265Nalu_t naluArray[ 10 ];
    H265Packet_t packet = { 0 };

    uint8_t largeData1[ 80 ] = { 0 };
    uint8_t largeData2[ 80 ] = { 0 };
    uint8_t packetBuffer[ 100 ] = { 0 };

    /* Fill test data */
    memset( largeData1, 0xAA, sizeof( largeData1 ) );
    memset( largeData2, 0xBB, sizeof( largeData2 ) );

    /* Initialize the context with small max packet size */
    result = H265Packetizer_Init( &ctx,
                                  naluArray,
                                  10,
                                  1,
                                  100 ); /* Small max packet size to force overflow */
    TEST_ASSERT_EQUAL( 0, result );

    /* Setup NAL units */
    H265Nalu_t testNalu1 = { 0 };
    H265Nalu_t testNalu2 = { 0 };

    testNalu1.pNaluData = largeData1;
    testNalu2.pNaluData = largeData2;
    /* Set other necessary NAL fields here */

    /* Add NAL units to context */
    H265Packetizer_AddNalu( &ctx, &testNalu1 );
    H265Packetizer_AddNalu( &ctx, &testNalu2 );

    /* Setup packet */
    packet.pPacketData = packetBuffer;
    packet.maxPacketSize = 100;

    /* Try to get packet - should trigger size overflow */
    result = H265Packetizer_GetPacket( &ctx, &packet );
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
                                  10,
                                  0,   /* No DON handling */
                                  1500 );
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
    packet.maxPacketSize = 20;  /* Small enough that second NAL won't fit */

    /* Try to get packet - should attempt aggregation but fail */
    result = H265Packetizer_GetPacket( &ctx, &packet );
    TEST_ASSERT_EQUAL( 0, result );  /* Adjust expected result based on your implementation */
}



/*-----------------------------------------------------------------------------------------------------*/



void test_H265_Packetizer_Init_Null_Params( void )
{
    H265PacketizerContext_t ctx;
    H265Result_t result;
    H265Nalu_t naluArray[ 10 ];

    /* Test Case 1: NULL context */
    result = H265Packetizer_Init( NULL,  /* NULL context */
                                  naluArray,
                                  10,
                                  1,
                                  1500 );
    TEST_ASSERT_EQUAL( H265_RESULT_BAD_PARAM, result );

    /* Test Case 2: NULL NAL array */
    result = H265Packetizer_Init( &ctx,
                                  NULL, /* NULL NAL array */
                                  10,
                                  1,
                                  1500 );
    TEST_ASSERT_EQUAL( H265_RESULT_BAD_PARAM, result );

    /* Test Case 3: Zero array length */
    result = H265Packetizer_Init( &ctx,
                                  naluArray,
                                  0,    /* Zero length */
                                  1,
                                  1500 );
    TEST_ASSERT_EQUAL( H265_RESULT_BAD_PARAM, result );

    /* Test Case 4: Zero maxPacketSize */
    result = H265Packetizer_Init( &ctx,
                                  naluArray,
                                  10,
                                  1,
                                  0 ); /* Setting maxPacketSize to 0 */
    TEST_ASSERT_EQUAL( H265_RESULT_BAD_PARAM, result );

    /* Test Case 5: spropMaxDonDiff exceeding maximum */
    result = H265Packetizer_Init( &ctx,
                                  naluArray,
                                  10,
                                  MAX_DON_DIFF_VALUE + 1, /* Exceeding maximum */
                                  1500 );
    TEST_ASSERT_EQUAL( H265_RESULT_BAD_PARAM, result );
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
    result = H265Packetizer_Init( &ctx, naluArray, MAX_NALUS_IN_A_FRAME, 1, 100 );
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

    result = H265Packetizer_Init( &ctx, naluArray, MAX_NALUS_IN_A_FRAME, 1, 100 );
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

    result = H265Packetizer_Init( &ctx, naluArray, MAX_NALUS_IN_A_FRAME, 1, 100 );
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

    result = H265Packetizer_Init( &ctx, naluArray, MAX_NALUS_IN_A_FRAME, 1, 100 );
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

    result = H265Packetizer_Init( &ctx, naluArray, MAX_NALUS_IN_A_FRAME, 1, 100 );
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

    result = H265Packetizer_Init( &ctx, naluArray, MAX_NALUS_IN_A_FRAME, 1, 100 );
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
                                  10,
                                  0,
                                  1500 );
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
                                  10,
                                  0,
                                  1500 );
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
                                  10,
                                  0,
                                  1500 );
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
                                  10,
                                  1,
                                  1500 );
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
                                  2,    /* Small array length to trigger overflow */
                                  1,
                                  1500 );
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
                                  10,
                                  1,
                                  1500 );
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    /* Setup packet with small maxPacketSize */
    packet.pPacketData = packetBuffer;
    packet.maxPacketSize = 1;  /* Set size less than NALU_HEADER_SIZE */

    /* Try to get packet */
    result = H265Packetizer_GetPacket( &ctx, &packet );

    /* Verify that we get bad parameter error */
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
                                  10,
                                  1,
                                  1500 );
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
    packet.maxPacketSize = 100;  /* Small enough to trigger overflow */

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
                                  10,
                                  0,
                                  1500 );
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
    packet.maxPacketSize = 0;  /* Set to zero to trigger the check */

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
                                  3,   /* Small naluArrayLength */
                                  1,   /* Set spropMaxDonDiff > 0 to test DONL/DOND paths */
                                  1500 );
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
    packet.maxPacketSize = 1500;  /* Large enough for all NALs */

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
                                  2,   /* naluArrayLength = 2 */
                                  0,
                                  1500 );
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
    packet.maxPacketSize = 1500;    /* Large enough for single NAL */

    /* Try to get packet */
    result = H265Packetizer_GetPacket( &ctx, &packet );
    TEST_ASSERT_EQUAL( 0, result );
}

void test_H265_Packetizer_GetPacket_Valid_Context_Invalid_Packet( void )
{
    H265PacketizerContext_t ctx;
    H265Result_t result;
    H265Nalu_t naluArray[ 10 ];
    uint8_t naluData[] =
    {
        0x40, 0x01,  /* NAL header */
        0x02, 0x03   /* Payload */
    };

    /* Initialize context - making pCtx valid */
    result = H265Packetizer_Init( &ctx,
                                  naluArray,
                                  10,
                                  0,
                                  1500 );
    TEST_ASSERT_EQUAL( 0, result );

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
}

/*-----------------------------------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------------------------------*/

void test_H265_Depacketizer_ProcessSingleNalu( void )                             /* Without Donl */
{
    H265DepacketizerContext_t ctx;
    H265Result_t result;
    H265Packet_t packetsArray[ 10 ];
    H265Nalu_t nalu;
    uint8_t packetData[] =
    {
        0x02, 0x01,  /* NAL header */
        0x02, 0x03   /* Payload */
    };

    /* Initialize context */
    for(size_t i = 0; i < 10; i++)
    {
        packetsArray[ i ].pPacketData = packetData;  /* All point to same buffer initially */
        packetsArray[ i ].packetDataLength = 0;      /* No data yet */
        packetsArray[ i ].maxPacketSize = 1500;      /* Standard MTU size */
    }

    result = H265Depacketizer_Init( &ctx,
                                    packetsArray,
                                    10,
                                    0 );      /* donPresent = 0 */
    TEST_ASSERT_EQUAL_MESSAGE( H265_RESULT_OK, result, "Depacketizer Init failed" );

    /* Initialize and add packet */
    packetsArray[ 0 ].pPacketData = packetData;
    packetsArray[ 0 ].packetDataLength = sizeof( packetData );
    packetsArray[ 0 ].maxPacketSize = 1500;

    result = H265Depacketizer_AddPacket( &ctx, &packetsArray[ 0 ] );
    TEST_ASSERT_EQUAL_MESSAGE( H265_RESULT_OK, result, "AddPacket failed" );

    /* Get NALU and verify */
    result = H265Depacketizer_GetNalu( &ctx, &nalu );
    TEST_ASSERT_EQUAL_MESSAGE( H265_RESULT_OK, result, "GetNalu failed" );

    /* Verify NALU fields */
    TEST_ASSERT_EQUAL_MESSAGE( SINGLE_NALU_PACKET_TYPE_START, nalu.nal_unit_type, "Wrong NAL unit type" );
    TEST_ASSERT_EQUAL_MESSAGE( sizeof( packetData ), nalu.naluDataLength, "Wrong NALU length" );
    TEST_ASSERT_EQUAL_PTR_MESSAGE( packetData, nalu.pNaluData, "Wrong NALU data pointer" );

    TEST_ASSERT_EQUAL_MESSAGE( 0, ctx.packetCount, "Wrong packet count" );

    /* Try getting another NALU */
    result = H265Depacketizer_GetNalu( &ctx, &nalu );
    TEST_ASSERT_EQUAL_MESSAGE( H265_RESULT_NO_MORE_NALUS, result, "Should have no more NALUs" );
}

void test_H265_Depacketizer_ProcessSingleNalu_With_Donl( void )
{
    H265DepacketizerContext_t ctx;
    H265Result_t result;
    H265Packet_t packetsArray[ 10 ];
    H265Nalu_t nalu;
    uint8_t packetData[] =
    {
        0x02, 0x01,     /* NAL header (type 1) */
        0x00, 0x01,     /* DONL field (value = 1) */
        0x02, 0x03      /* Payload */
    };

    result = H265Depacketizer_Init( &ctx,
                                    packetsArray,
                                    10,
                                    1 ); /* donPresent = 1 */
    TEST_ASSERT_EQUAL_MESSAGE( H265_RESULT_OK, result, "Depacketizer Init failed" );

    packetsArray[ 0 ].pPacketData = packetData;
    packetsArray[ 0 ].packetDataLength = sizeof( packetData );
    packetsArray[ 0 ].maxPacketSize = 1500;

    result = H265Depacketizer_AddPacket( &ctx, &packetsArray[ 0 ] );
    TEST_ASSERT_EQUAL_MESSAGE( H265_RESULT_OK, result, "AddPacket failed" );

    /* Get NALU and verify */
    result = H265Depacketizer_GetNalu( &ctx, &nalu );
    TEST_ASSERT_EQUAL_MESSAGE( H265_RESULT_OK, result, "GetNalu failed" );

    /* Verify correct NAL unit type */
    TEST_ASSERT_EQUAL_MESSAGE( SINGLE_NALU_PACKET_TYPE_START, nalu.nal_unit_type, "Wrong NAL unit type" );

    /* Verify NALU data - should exclude DONL field */
    TEST_ASSERT_EQUAL_MESSAGE( 4, nalu.naluDataLength, "Wrong NALU length" );  /* 2 bytes header + 2 bytes payload */

    /* First two bytes should be NAL header */
    TEST_ASSERT_EQUAL_HEX8_MESSAGE( 0x02, nalu.pNaluData[ 0 ], "Wrong first NAL header byte" );
    TEST_ASSERT_EQUAL_HEX8_MESSAGE( 0x01, nalu.pNaluData[ 1 ], "Wrong second NAL header byte" );

    /* Next two bytes should be payload (skipping DONL) */
    TEST_ASSERT_EQUAL_HEX8_MESSAGE( 0x02, nalu.pNaluData[ 2 ], "Wrong first payload byte" );
    TEST_ASSERT_EQUAL_HEX8_MESSAGE( 0x03, nalu.pNaluData[ 3 ], "Wrong second payload byte" );

    /* Verify DON value */
    TEST_ASSERT_EQUAL_MESSAGE( 1, ctx.currentDon, "Wrong DON value" );
    TEST_ASSERT_EQUAL_MESSAGE( 0, ctx.packetCount, "Wrong packet count" );

    /* Try getting another NALU */
    result = H265Depacketizer_GetNalu( &ctx, &nalu );
    TEST_ASSERT_EQUAL_MESSAGE( H265_RESULT_NO_MORE_NALUS, result, "Should have no more NALUs" );
}



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
    result = H265Depacketizer_Init( &ctx, packetsArray, 10, 0 );

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

void test_H265_Depacketizer_ProcessFragmentedNalu_WithDONL( void )
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

    /* Initialize test packets with DONL */
    uint8_t startFragment[] =
    {
        0x62,       /* NAL unit header byte 1: FU indicator (type 0x31) */
        0x01,       /* NAL unit header byte 2 */
        0x82,       /* FU header: Start bit(0x80) + NAL type(0x02) */
        0x00, 0x01, /* DONL (Decoding Order Number Layer) */
        0x11,       /* Payload */
        0x22        /* Payload */
    };

    uint8_t middleFragment[] =
    {
        0x62,   /* Same FU indicator */
        0x01,   /* Same header byte 2 */
        0x02,   /* FU header: continuation (just NAL type) */
        0x00,   /* DOND (Decoding Order Number Difference) */
        0x33,   /* Payload */
        0x44    /* Payload */
    };

    uint8_t endFragment[] =
    {
        0x62,   /* Same FU indicator */
        0x01,   /* Same header byte 2 */
        0x42,   /* FU header: End bit(0x40) + NAL type(0x02) */
        0x00,   /* DOND */
        0x55,   /* Payload */
        0x66    /* Payload */
    };

    /* Initialize depacketizer with DONL present */
    result = H265Depacketizer_Init( &ctx, packetsArray, 10, 1 );  /* 1 indicates DONL present */

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

        /* Verify NALU content */
        TEST_ASSERT_EQUAL_UINT8_MESSAGE( 0x02, ( nalu.pNaluData[ 0 ] >> 1 ) & 0x3F, "Incorrect NAL unit type" );
        TEST_ASSERT_EQUAL_UINT8_MESSAGE( 0x11, nalu.pNaluData[ 2 ], "Incorrect first payload byte" );
        TEST_ASSERT_EQUAL_UINT8_MESSAGE( 0x66, nalu.pNaluData[ 7 ], "Incorrect last payload byte" );
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
    result = H265Depacketizer_Init( &ctx, packetsArray, 10, 0 );
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

    /* Test DONL size error for start fragment */
    uint8_t smallDonlPacket[] =
    {
        0x62, 0x01,     /* FU indicator */
        0x82            /* FU header with start bit but no DONL */
    };
    ctx.donPresent = 1; /* Enable DONL */
    packetsArray[ 0 ].pPacketData = smallDonlPacket;
    packetsArray[ 0 ].packetDataLength = sizeof( smallDonlPacket );
    ctx.packetCount = 1;
    result = H265Depacketizer_GetNalu( &ctx, &nalu );
    TEST_ASSERT_EQUAL( H265_RESULT_MALFORMED_PACKET, result );

    /* Test DOND size error for continuation fragment */
    uint8_t smallDondPacket[] =
    {
        0x62, 0x01,     /* FU indicator */
        0x02            /* FU header without start bit but no DOND */
    };
    packetsArray[ 0 ].pPacketData = smallDondPacket;
    packetsArray[ 0 ].packetDataLength = sizeof( smallDondPacket );
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
    ctx.donPresent = 0;                                  /* Disable DONL */
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
    result = H265Depacketizer_Init( &ctx, packetsArray, 10, 0 );
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
    result = H265Depacketizer_Init( &ctx, packetsArray, 10, 0 );
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

void test_H265_Depacketizer_ProcessAggregationPacket_WithDON(void)
{
    H265DepacketizerContext_t ctx;
    H265Result_t result;
    H265Packet_t packetsArray[20];
    H265Frame_t frame = { 0 };
    H265Nalu_t nalu = { 0 };

    /* Initialize structures */
    memset(&ctx, 0, sizeof(H265DepacketizerContext_t));
    memset(packetsArray, 0, sizeof(packetsArray));
    memset(&nalu, 0, sizeof(H265Nalu_t));

    /* Create buffers */
    uint8_t frameBuffer[1500];
    uint8_t naluBuffer[1500];
    uint8_t reassemblyBuffer[1500];  // Added reassembly buffer
    memset(frameBuffer, 0, sizeof(frameBuffer));
    memset(naluBuffer, 0, sizeof(naluBuffer));
    memset(reassemblyBuffer, 0, sizeof(reassemblyBuffer));

    /* Setup frame and buffers */
    frame.pFrameData = frameBuffer;
    frame.frameDataLength = sizeof(frameBuffer);
    nalu.pNaluData = naluBuffer;
    nalu.naluDataLength = sizeof(naluBuffer);
    
    /* Setup reassembly buffer */
    ctx.fuDepacketizationState.pReassemblyBuffer = reassemblyBuffer;
    ctx.fuDepacketizationState.reassemblyBufferSize = sizeof(reassemblyBuffer);

    /* Create aggregation packet */
    static uint8_t aggrPacket[] = {
        0x60, 0x01,           /* First byte: 011|00000, Second byte: 00000|001 */
        0x00, 0x0A,           /* DONL = 10 */
        
        /* First NALU */
        0x00, 0x04,           /* First NALU length (4 bytes) */
        0x26, 0x01,           /* NALU1 header */
        0xAA, 0xBB,           /* NALU1 payload */
        
        /* Second NALU with DOND */
        0x02,                 /* DOND = 2 */
        0x00, 0x04,           /* Second NALU length (4 bytes) */
        0x26, 0x01,           /* NALU2 header */
        0xCC, 0xDD            /* NALU2 payload */
    };

    /* Initialize depacketizer */
    result = H265Depacketizer_Init(&ctx, packetsArray, 20, 1);
    TEST_ASSERT_EQUAL_MESSAGE(H265_RESULT_OK, result, "Init failed");

    /* Setup packet in context */
    packetsArray[0].pPacketData = aggrPacket;
    packetsArray[0].packetDataLength = sizeof(aggrPacket);
    ctx.pPacketsArray = packetsArray;
    ctx.tailIndex = 0;
    ctx.donPresent = 1;

    /* Add packet */
    result = H265Depacketizer_AddPacket(&ctx, &packetsArray[0]);
    TEST_ASSERT_EQUAL_MESSAGE(H265_RESULT_OK, result, "AddPacket failed");

    /* Get Frame and verify */
    result = H265Depacketizer_GetFrame(&ctx, &frame);
    TEST_ASSERT_EQUAL_MESSAGE(H265_RESULT_OK, result, "GetFrame failed");
    
    /* Verify frame contents */
    TEST_ASSERT_EQUAL_HEX8_MESSAGE(0x00, frame.pFrameData[0], "First start code byte 1");
    TEST_ASSERT_EQUAL_HEX8_MESSAGE(0x00, frame.pFrameData[1], "First start code byte 2");
    TEST_ASSERT_EQUAL_HEX8_MESSAGE(0x00, frame.pFrameData[2], "First start code byte 3");
    TEST_ASSERT_EQUAL_HEX8_MESSAGE(0x01, frame.pFrameData[3], "First start code byte 4");
    TEST_ASSERT_EQUAL_HEX8_MESSAGE(0x26, frame.pFrameData[4], "First NALU header");
    TEST_ASSERT_EQUAL_HEX8_MESSAGE(0x01, frame.pFrameData[5], "First NALU second byte");
    TEST_ASSERT_EQUAL_HEX8_MESSAGE(0xAA, frame.pFrameData[6], "First NALU payload 1");
    TEST_ASSERT_EQUAL_HEX8_MESSAGE(0xBB, frame.pFrameData[7], "First NALU payload 2");

    TEST_ASSERT_EQUAL_MESSAGE(16, frame.frameDataLength, "Frame length incorrect");
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
    result = H265Depacketizer_Init( &ctx, packetsArray, 10, 0 );
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
    result = H265Depacketizer_Init( &ctx, packetsArray, 10, 0 );
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
    H265Packet_t inputPacket = {
        .pPacketData = (uint8_t *)apPacket,
        .packetDataLength = sizeof(apPacket)
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

    printf( "\n=== Starting AP Out Of Memory Test for Second NALU ===\n" );

    /* Initialize everything */
    memset( &ctx, 0, sizeof( H265DepacketizerContext_t ) );
    memset( packetsArray, 0, sizeof( packetsArray ) );

    /* Initialize NALU buffer - intentionally small */
    static uint8_t naluBuffer[ 10 ];
    memset( naluBuffer, 0, sizeof( naluBuffer ) );
    nalu.pNaluData = naluBuffer;
    nalu.naluDataLength = sizeof( naluBuffer );

    /* Initialize depacketizer */
    result = H265Depacketizer_Init( &ctx, packetsArray, 10, 0 );
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
    H265Packet_t inputPacket = {
        .pPacketData = (uint8_t *)apPacket,
        .packetDataLength = sizeof(apPacket)
    };

    result = H265Depacketizer_AddPacket( &ctx, &inputPacket );
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    /* Get first NALU - should succeed */
    printf( "Getting First NALU (expecting OK)...\n" );
    result = H265Depacketizer_GetNalu( &ctx, &nalu );
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );
    printf( "First GetNalu result: %d (Expected: %d)\n", result, H265_RESULT_OK );

    /* Try to get second NALU - should fail with OUT_OF_MEMORY */
    printf( "Getting Second NALU (expecting OUT_OF_MEMORY)...\n" );
    result = H265Depacketizer_GetNalu( &ctx, &nalu );
    printf( "Second GetNalu result: %d (Expected: %d)\n", result, H265_RESULT_OUT_OF_MEMORY );

    /* Verify results */
    TEST_ASSERT_EQUAL( H265_RESULT_OUT_OF_MEMORY, result );

    printf( "=== Test Complete ===\n" );
}

void test_H265_Depacketizer_AP_UpdateOffset_MoreNalus( void )
{
    H265DepacketizerContext_t ctx;
    H265Result_t result;
    H265Packet_t packetsArray[ 10 ];
    H265Nalu_t nalu = { 0 };

    printf( "\n=== Starting AP Update Offset Test ===\n" );

    /* Initialize everything */
    memset( &ctx, 0, sizeof( H265DepacketizerContext_t ) );
    memset( packetsArray, 0, sizeof( packetsArray ) );

    /* Initialize NALU buffer */
    static uint8_t naluBuffer[ 20 ];
    memset( naluBuffer, 0, sizeof( naluBuffer ) );
    nalu.pNaluData = naluBuffer;
    nalu.naluDataLength = sizeof( naluBuffer );

    /* Initialize depacketizer */
    result = H265Depacketizer_Init( &ctx, packetsArray, 10, 0 );
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
    H265Packet_t inputPacket = {
        .pPacketData = (uint8_t *)apPacket,
        .packetDataLength = sizeof(apPacket)
    };

    result = H265Depacketizer_AddPacket( &ctx, &inputPacket );
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    /* Get first NALU */
    printf( "Getting First NALU...\n" );
    result = H265Depacketizer_GetNalu( &ctx, &nalu );
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );
    
    /* Verify currentOffset is set to start of second NALU */
    TEST_ASSERT_EQUAL( 8, ctx.apDepacketizationState.currentOffset );
    printf( "After first NALU - currentOffset: %zu (Expected: 8)\n", 
            ctx.apDepacketizationState.currentOffset );

    /* Get second NALU */
    printf( "Getting Second NALU...\n" );
    result = H265Depacketizer_GetNalu( &ctx, &nalu );
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );
    
    /* Verify currentOffset is set to start of third NALU */
    TEST_ASSERT_EQUAL( 14, ctx.apDepacketizationState.currentOffset );
    printf( "After second NALU - currentOffset: %zu (Expected: 14)\n", 
            ctx.apDepacketizationState.currentOffset );

    /* Get third NALU */
    printf( "Getting Third NALU...\n" );
    result = H265Depacketizer_GetNalu( &ctx, &nalu );
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );
    
    /* Verify packet is completed */
    TEST_ASSERT_EQUAL( H265_PACKET_NONE, ctx.currentlyProcessingPacket );
    printf( "After third NALU - Processing complete\n" );

    printf( "=== Test Complete ===\n" );
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

    /* Verify properties */
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
    result = H265Depacketizer_Init( &ctx, packetsArray, 10, 0 );  /* No DONL */
    TEST_ASSERT_EQUAL_MESSAGE( H265_RESULT_OK, result, "Init failed" );

    /* Set up reassembly buffer */
    ctx.fuDepacketizationState.pReassemblyBuffer = reassemblyBuffer;
    ctx.fuDepacketizationState.reassemblyBufferSize = sizeof( reassemblyBuffer );

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

void test_H265_Depacketizer_BAD_PARAMS( void )
{
    H265DepacketizerContext_t ctx;
    H265Result_t result;
    H265Packet_t packetsArray[ 10 ];
    H265Nalu_t nalu;
    H265Frame_t frame = { 0 };
    uint32_t properties;

    /* Initialize buffers */
    uint8_t reassemblyBuffer[ 1500 ];
    uint8_t frameBuffer[ 3000 ];

    /* Test 1: Parameter Validation */
    /* Init NULL checks */
    result = H265Depacketizer_Init( NULL, packetsArray, 10, 0 );
    TEST_ASSERT_EQUAL( H265_RESULT_BAD_PARAM, result );

    result = H265Depacketizer_Init( &ctx, NULL, 10, 0 );
    TEST_ASSERT_EQUAL( H265_RESULT_BAD_PARAM, result );

    result = H265Depacketizer_Init( &ctx, packetsArray, 0, 0 );
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

    /* GetPacketProperties NULL checks */
    result = H265Depacketizer_GetPacketProperties( NULL, 4, &properties );
    TEST_ASSERT_EQUAL( H265_RESULT_BAD_PARAM, result );

    uint8_t dummyData[] = { 0x00, 0x01 };
    result = H265Depacketizer_GetPacketProperties( dummyData, 4, NULL );
    TEST_ASSERT_EQUAL( H265_RESULT_BAD_PARAM, result );

    /* Size too small */
    result = H265Depacketizer_GetPacketProperties( dummyData, 1, &properties );
    TEST_ASSERT_EQUAL( H265_RESULT_BAD_PARAM, result );

    /* Test 2: Malformed Packets */
    /* Initialize context properly */
    memset( &ctx, 0, sizeof( ctx ) );
    ctx.fuDepacketizationState.pReassemblyBuffer = reassemblyBuffer;
    ctx.fuDepacketizationState.reassemblyBufferSize = sizeof( reassemblyBuffer );
    frame.pFrameData = frameBuffer;
    frame.frameDataLength = sizeof( frameBuffer );
    result = H265Depacketizer_Init( &ctx, packetsArray, 10, 0 );
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

    /* Test 3: Malformed FU packets */
    /* FU packet with invalid size */
    uint8_t invalidFuPacket[] =
    {
        0x62, 0x01,  /* NAL header type 0x31 (FU) */
        0x82,        /* FU header - Start bit set */
        0x11         /* Incomplete payload */
    };
    packetsArray[ 0 ].pPacketData = invalidFuPacket;
    packetsArray[ 0 ].packetDataLength = sizeof( invalidFuPacket );
    ctx.packetCount = 1;
    result = H265Depacketizer_GetNalu( &ctx, &nalu );
    TEST_ASSERT_EQUAL( H265_RESULT_MALFORMED_PACKET, result );

    /* Test 4: Malformed AP packets with DONL */
    /* Initialize with DONL */
    result = H265Depacketizer_Init( &ctx, packetsArray, 10, 1 );
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    /* AP packet with missing DONL */
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

    /* Test 5: Single NALU with DONL malformed */
    uint8_t invalidSingleNalu[] =
    {
        0x26, 0x01  /* Too small for DONL */
    };
    packetsArray[ 0 ].pPacketData = invalidSingleNalu;
    packetsArray[ 0 ].packetDataLength = sizeof( invalidSingleNalu );
    ctx.packetCount = 1;
    result = H265Depacketizer_GetNalu( &ctx, &nalu );
    TEST_ASSERT_EQUAL( H265_RESULT_MALFORMED_PACKET, result );

    /* Test 6: Unsupported packet types */
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


    /* Test 7: No more NALUs/Frames */
    ctx.packetCount = 0;
    frame.frameDataLength = sizeof( frameBuffer );
    result = H265Depacketizer_GetFrame( &ctx, &frame );
    TEST_ASSERT_EQUAL( H265_RESULT_NO_MORE_FRAMES, result );
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
    result = H265Depacketizer_Init( &ctx, packetsArray, 10, 0 );
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
    result = H265Depacketizer_Init( &ctx, packetsArray, 2, 0 );  /* Array size = 2 */
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

void test_H265Depacketizer_GetFrame_BufferTooSmall(void)
{
    H265DepacketizerContext_t ctx;
    H265Result_t result;
    H265Packet_t packetsArray[10];
    H265Frame_t frame;
    
    /* Initialize all structures */
    memset(&ctx, 0, sizeof(H265DepacketizerContext_t));
    memset(packetsArray, 0, sizeof(packetsArray));
    
    /* Setup reassembly buffer */
    uint8_t reassemblyBuffer[1500];
    ctx.fuDepacketizationState.pReassemblyBuffer = reassemblyBuffer;
    ctx.fuDepacketizationState.reassemblyBufferSize = sizeof(reassemblyBuffer);
    
    /* Create test packet with single NAL unit */
    uint8_t packetData[] = {
        0x26, 0x01,           /* NAL unit header (type within valid single NAL unit range) */
        0xAA, 0xBB, 0xCC, 0xDD, 0xEE  /* Payload */
    };
    
    /* Create deliberately small frame buffer */
    uint8_t frameBuffer[6];   /* Only 6 bytes: not enough for start code (4) + NAL unit (7) */
    
    /* Initialize frame */
    frame.pFrameData = frameBuffer;
    frame.frameDataLength = sizeof(frameBuffer);
    
    /* Setup context */
    packetsArray[0].pPacketData = packetData;
    packetsArray[0].packetDataLength = sizeof(packetData);
    ctx.pPacketsArray = packetsArray;
    ctx.packetCount = 1;
    ctx.tailIndex = 0;
    
    /* Initialize depacketizer */
    result = H265Depacketizer_Init(&ctx, packetsArray, 1, 0);
    TEST_ASSERT_EQUAL_MESSAGE(H265_RESULT_OK, result, "Depacketizer init failed");
    
    /* Add packet */
    result = H265Depacketizer_AddPacket(&ctx, &packetsArray[0]);
    TEST_ASSERT_EQUAL_MESSAGE(H265_RESULT_OK, result, "Failed to add packet");
    
    /* Try to get frame - should fail due to small buffer */
    result = H265Depacketizer_GetFrame(&ctx, &frame);
    
    TEST_ASSERT_EQUAL_MESSAGE(H265_RESULT_BUFFER_TOO_SMALL, result, 
                            "Expected buffer too small error when frame buffer cannot hold start code + NALU");
}

/*-------------------------------------------------------------------------------------------------------------*/