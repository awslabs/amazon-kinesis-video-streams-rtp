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

/* Global buffers. */
uint8_t frameBuffer[ MAX_FRAME_LENGTH ];
uint8_t packetBuffer[ MAX_H265_PACKET_LENGTH ];

/* Test setup and teardown. */
void setUp( void )
{
    memset( &( frameBuffer[ 0 ] ), 0, sizeof( frameBuffer ) );
    memset( &( packetBuffer[ 0 ] ), 0, sizeof( packetBuffer ) );
}

void tearDown( void )
{
    /* Nothing to clean up. */
}

/* ==============================  Test Cases for Packetization ============================== */

/**
 * @brief Test H265 packetizer init for bad parameters.
 */
void test_H265_Packetizer_Init_Bad_Params( void )
{
    H265PacketizerContext_t ctx;
    H265Result_t result;
    H265Nalu_t naluArray[ MAX_NALUS_IN_A_FRAME ];

    result = H265Packetizer_Init( &( ctx ), &( naluArray[ 0 ] ), 0 );

    TEST_ASSERT_EQUAL( H265_RESULT_BAD_PARAM, result );

    result = H265Packetizer_Init( &( ctx ), NULL, MAX_NALUS_IN_A_FRAME );

    TEST_ASSERT_EQUAL( H265_RESULT_BAD_PARAM, result );

    result = H265Packetizer_Init( NULL, &( naluArray[ 0 ] ), MAX_NALUS_IN_A_FRAME );

    TEST_ASSERT_EQUAL( H265_RESULT_BAD_PARAM, result );
}

/*-----------------------------------------------------------*/

/**
 * @brief Test H265 packetization with single NAL unit.
 */
void test_H265_Packetizer_Single_Nalu_Packet( void )
{
    H265PacketizerContext_t ctx;
    H265Result_t result;
    H265Nalu_t naluArray[ MAX_NALUS_IN_A_FRAME ];
    uint8_t naluData[] =
    {
        0x40, 0x01,      /* NALU header: Type=32/VPS, TID=1. */
        0x01, 0x02, 0x03 /* NALU payload. */
    };
    H265Nalu_t nalu =
    {
        .pNaluData      = &( naluData[ 0 ] ),
        .naluDataLength = sizeof( naluData ),
    };
    H265Packet_t packet =
    {
        .pPacketData = &( packetBuffer[ 0 ] ),
        .packetDataLength = MAX_H265_PACKET_LENGTH
    };

    result = H265Packetizer_Init( &( ctx ),
                                  &( naluArray[ 0 ] ),
                                  MAX_NALUS_IN_A_FRAME );

    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );


    result = H265Packetizer_AddNalu( &( ctx ), &( nalu ) );

    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );
    TEST_ASSERT_EQUAL( 1, ctx.naluCount );

    result = H265Packetizer_GetPacket( &( ctx ), &( packet ) );

    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );
    TEST_ASSERT_EQUAL( sizeof( naluData ), packet.packetDataLength );
    TEST_ASSERT_EQUAL_UINT8_ARRAY( &( naluData[ 0 ] ),
                                   packet.pPacketData,
                                   packet.packetDataLength );
}

/*-----------------------------------------------------------*/

/**
 * @brief Test H265 packetization with fragmentation.
 */
void test_H265_Packetizer_Two_Fragment_Packets(void)
{
    H265PacketizerContext_t ctx;
    H265Result_t result;
    H265Nalu_t naluArray[ MAX_NALUS_IN_A_FRAME ];
    /* Test NALU: 2 bytes header + 6 bytes payload = 8 bytes total. */
    uint8_t naluData[ 8 ] =
    {
        0x40, 0x01,         /* NALU header: Type=32, TID=1. */
        0xAA, 0xBB, 0xCC,   /* First 3 bytes of payload. */
        0xDD, 0xEE, 0xFF    /* Last 3 bytes of payload. */
    };
    H265Nalu_t nalu =
    {
        .pNaluData = &( naluData[ 0 ] ),
        .naluDataLength = sizeof( naluData )
    };
    H265Packet_t packet =
    {
        .pPacketData = &( packetBuffer[ 0 ] ),
        .packetDataLength = 6 /* Enforce fragmentation. */
    };
    uint8_t expectedFirstFragment[] =
    {
        0x62, 0x01,         /* Payload header: Type=49, TID=1. */
        0xA0,               /* FU header: S=1, Type=32. */
        0xAA, 0xBB, 0xCC    /* NALU payload. */
    };
    uint8_t expectedSecondFragment[] =
    {
        0x62, 0x01,         /* Payload header: Type=49, TID=1. */
        0x60,               /* FU header: E=1, Type=32. */
        0xDD, 0xEE, 0xFF    /* NALU payload. */
    };

    result = H265Packetizer_Init( &( ctx ),
                                  &( naluArray[ 0 ] ),
                                  MAX_NALUS_IN_A_FRAME );

    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    result = H265Packetizer_AddNalu( &( ctx ), &( nalu ) );

    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    /* Get first fragment. */
    result = H265Packetizer_GetPacket( &( ctx ), &( packet ) );

    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );
    TEST_ASSERT_EQUAL( sizeof( expectedFirstFragment ), packet.packetDataLength );
    TEST_ASSERT_EQUAL_UINT8_ARRAY( &( expectedFirstFragment[ 0 ] ),
                                   packet.pPacketData,
                                   packet.packetDataLength );

    /* Get second fragment. */
    packet.packetDataLength = 6;
    result = H265Packetizer_GetPacket( &( ctx ), &( packet ) );

    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );
    TEST_ASSERT_EQUAL( sizeof( expectedSecondFragment ), packet.packetDataLength );
    TEST_ASSERT_EQUAL_UINT8_ARRAY( &( expectedSecondFragment[ 0 ] ),
                                   packet.pPacketData,
                                   packet.packetDataLength );

    /* Ensure no more fragments. */
    packet.packetDataLength = 6;
    result = H265Packetizer_GetPacket( &( ctx ), &( packet ) );

    TEST_ASSERT_EQUAL( H265_RESULT_NO_MORE_PACKETS, result );
}

/*-----------------------------------------------------------*/

/**
 * @brief Test H265 get packet for output buffer too small to fit fragmented packet.
 */
void test_H265_Packetizer_GetPacket_Output_Buffer_Cannot_Fit_Fragmented_Packet( void )
{
    H265PacketizerContext_t ctx;
    H265Result_t result;
    H265Nalu_t naluArray[ MAX_NALUS_IN_A_FRAME ];
    uint8_t naluData[ 100 ] = { 0 };
    H265Nalu_t nalu =
    {
        .pNaluData = &( naluData[ 0 ] ),
        .naluDataLength = 100
    };
    H265Packet_t packet =
    {
        .pPacketData = &( packetBuffer[ 0 ] ),
        .packetDataLength = 3 /* Just enough only for headers (2 bytes PayloadHdr + 1 byte FUHeader). */
    };

    result = H265Packetizer_Init( &( ctx ),
                                  &( naluArray[ 0 ] ),
                                  MAX_NALUS_IN_A_FRAME );

    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    /* NALU header: Type=32, TID=1. */
    naluData[ 0 ] = 0x40;
    naluData[ 1 ] = 0x01;
    /* Set NALU payload to 0xAA. */
    memset( &( naluData[ 2 ] ), 0xAA, 98 );
    result = H265Packetizer_AddNalu( &( ctx ), &( nalu ) );

    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    result = H265Packetizer_GetPacket( &( ctx ), &( packet ) );

    TEST_ASSERT_EQUAL( H265_RESULT_OUT_OF_MEMORY, result );
}

/*-----------------------------------------------------------*/

/**
 * @brief Test H265 packetization with simple aggregation.
 */
void test_H265_Packetizer_Aggregation_Packet( void )
{
    H265PacketizerContext_t ctx;
    H265Result_t result;
    H265Nalu_t naluArray[ MAX_NALUS_IN_A_FRAME ];
    uint8_t naluData1[] =
    {
        0x40, 0x01, /* NALU header: Type=32, TID=1. */
        0xAA, 0xBB  /* NALU payload. */
    };
    uint8_t naluData2[] =
    {
        0x42, 0x02, /* NALU header: Type=33, TID=2. */
        0xCC, 0xDD  /* NALU payload. */
    };
    H265Nalu_t nalu1 =
    {
        .pNaluData      = &( naluData1[ 0 ] ),
        .naluDataLength = sizeof( naluData1 ),
        .nalUnitType    = 32,
        .temporalId     = 1
    };
    H265Nalu_t nalu2 =
    {
        .pNaluData      = &( naluData2[ 0 ] ),
        .naluDataLength = sizeof( naluData2 ),
        .nalUnitType    = 33,
        .temporalId     = 2
    };
    H265Packet_t packet =
    {
        .pPacketData = &( packetBuffer[ 0 ] ),
        .packetDataLength = MAX_H265_PACKET_LENGTH
    };
    uint8_t expectedAggregatedPacket[] =
    {
        0x60, 0x01,             /* Payload header: Type=48, TID=1. */
        0x00, 0x04,             /* NALU1 size = 4. */
        0x40, 0x01, 0xAA, 0xBB, /* NALU1 payload. */
        0x00, 0x04,             /* NALU2 size = 4. */
        0x42, 0x02, 0xCC, 0xDD, /* NALU2 payload. */
    };

    result = H265Packetizer_Init( &( ctx ),
                                  &( naluArray[ 0 ] ),
                                  MAX_NALUS_IN_A_FRAME );

    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    result = H265Packetizer_AddNalu( &( ctx ), &( nalu1 ) );

    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    result = H265Packetizer_AddNalu( &( ctx ), &( nalu2 ) );

    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    result = H265Packetizer_GetPacket( &( ctx ), &( packet ) );

    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );
    TEST_ASSERT_EQUAL( sizeof( expectedAggregatedPacket ), packet.packetDataLength );
    TEST_ASSERT_EQUAL_UINT8_ARRAY( &( expectedAggregatedPacket[ 0 ] ),
                                   packet.pPacketData,
                                   packet.packetDataLength );
}

/*-----------------------------------------------------------*/

/**
 * @brief Test H265 packetization with multiple NALUs that cannot be aggregated.
 */
void test_H265_Packetizer_Multiple_Nalus_Cannot_Aggregate( void )
{
    H265PacketizerContext_t ctx;
    H265Result_t result;
    H265Nalu_t naluArray[ MAX_NALUS_IN_A_FRAME ];
    uint8_t naluData1[] =
    {
        0x40, 0x01, /* NALU header: Type=32, TID=1. */
        0x02, 0x03  /* NALU payload. */
    };
    uint8_t naluData2[] =
    {
        0x40, 0x01,                         /* NALU header: Type=32, TID=1. */
        0x04, 0x05, 0x06, 0x07, 0x08, 0x09, /* NALU payload. */
        0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
    };
    H265Nalu_t nalu1 =
    {
        .pNaluData = &( naluData1[ 0 ] ),
        .naluDataLength = sizeof( naluData1 )
    };
    H265Nalu_t nalu2 =
    {
        .pNaluData = &( naluData2[ 0 ] ),
        .naluDataLength = sizeof( naluData2 )
    };
    H265Packet_t packet =
    {
        .pPacketData = &( packetBuffer[ 0 ] ),
        .packetDataLength = 20 /* Small packet buffer to prevent aggregation. */
    };

    result = H265Packetizer_Init( &( ctx ),
                                  &( naluArray[ 0 ] ),
                                  MAX_NALUS_IN_A_FRAME );

    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    result = H265Packetizer_AddNalu( &( ctx ), &( nalu1 ) );

    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    result = H265Packetizer_AddNalu( &( ctx ), &( nalu2 ) );

    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    /* Get first packet. */
    result = H265Packetizer_GetPacket( &( ctx ), &( packet ) );

    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );
    TEST_ASSERT_EQUAL( sizeof( naluData1 ), packet.packetDataLength );
    TEST_ASSERT_EQUAL_UINT8_ARRAY( &( naluData1[ 0 ] ),
                                   packet.pPacketData,
                                   packet.packetDataLength );

    /* Get second packet. */
    packet.packetDataLength = 20;
    result = H265Packetizer_GetPacket( &( ctx ), &( packet ) );

    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );
    TEST_ASSERT_EQUAL( sizeof( naluData2 ), packet.packetDataLength );
    TEST_ASSERT_EQUAL_UINT8_ARRAY( &( naluData2[ 0 ] ),
                                   packet.pPacketData,
                                   packet.packetDataLength );

    /* Ensure no more packets. */
    packet.packetDataLength = 20;
    result = H265Packetizer_GetPacket( &( ctx ), &( packet ) );

    TEST_ASSERT_EQUAL( H265_RESULT_NO_MORE_PACKETS, result );
}

/*-----------------------------------------------------------*/

/**
 * @brief Test H265 packetization AddFrame for bad parameters.
 */
void test_H265_Packetizer_AddFrame_Bad_Params( void )
{
    H265PacketizerContext_t ctx;
    H265Result_t result;
    uint8_t frameData[] = { 0x00, 0x00, 0x01 };
    H265Frame_t frame =
    {
        .pFrameData = &( frameData[ 0 ] ),
        .frameDataLength = sizeof( frameData )
    };

    result = H265Packetizer_AddFrame( NULL, &( frame ) );

    TEST_ASSERT_EQUAL( H265_RESULT_BAD_PARAM, result );

    result = H265Packetizer_AddFrame( &( ctx ), NULL );

    TEST_ASSERT_EQUAL( H265_RESULT_BAD_PARAM, result );

    frame.pFrameData = NULL;
    frame.frameDataLength = sizeof( frameData );
    result = H265Packetizer_AddFrame( &( ctx ), NULL );

    TEST_ASSERT_EQUAL( H265_RESULT_BAD_PARAM, result );

    frame.pFrameData = &( frameData[ 0 ] );
    frame.frameDataLength = 0;
    result = H265Packetizer_AddFrame( &( ctx ), NULL );

    TEST_ASSERT_EQUAL( H265_RESULT_BAD_PARAM, result );
}

/*-----------------------------------------------------------*/

/**
 * @brief Test H265 packetization AddFrame with multiple NAL units.
 */
void test_H265_Packetizer_AddFrame_Multiple_Nalus_4_Byte_Start_Code( void )
{
    H265PacketizerContext_t ctx;
    H265Result_t result;
    H265Nalu_t naluArray[ MAX_NALUS_IN_A_FRAME ];
    uint8_t frameData[] =
    {
        0x00, 0x00, 0x00, 0x01, /* Start code. */
        0x40, 0x01, 0xAA, 0xBB, /* NALU1. Header: Type=32, TID=1. */
        0x00, 0x00, 0x00, 0x01, /* Start code. */
        0x42, 0x02, 0xCC, 0xDD, /* NALU2. Header: Type=33, TID=2. */
        0x00, 0x00, 0x00, 0x01, /* Start code. */
        0xC4, 0x03, 0xEE, 0xFF  /* NALU3 NAL. Header: F=1, Type=34, TID=3. */
    };
    H265Frame_t frame =
    {
        .pFrameData         = &( frameData[ 0 ] ),
        .frameDataLength    = sizeof( frameData )
    };
    H265Packet_t packet =
    {
        .pPacketData = &( packetBuffer[ 0 ] ),
        .packetDataLength = MAX_H265_PACKET_LENGTH
    };
    /* Expected aggregate packet. */
    uint8_t expectedPacket[] =
    {
        0xE0, 0x01,                 /* Payload header: F=1, Type=48, TID=1. */
        0x00, 0x04,                 /* NALU1 size. */
        0x40, 0x01, 0xAA, 0xBB,     /* NALU1 payload. */
        0x00, 0x04,                 /* NALU2 size. */
        0x42, 0x02, 0xCC, 0xDD,     /* NALU2 payload. */
        0x00, 0x04,                 /* NALU3 size. */
        0xC4, 0x03, 0xEE, 0xFF      /* NALU3 payload. */
    };

    result = H265Packetizer_Init( &( ctx ),
                                  &( naluArray[ 0 ] ),
                                  MAX_NALUS_IN_A_FRAME );

    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    result = H265Packetizer_AddFrame( &( ctx ), &( frame ) );

    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );
    TEST_ASSERT_EQUAL( 3, ctx.naluCount );

    result = H265Packetizer_GetPacket( &( ctx ), &( packet ) );

    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );
    TEST_ASSERT_EQUAL( sizeof( expectedPacket ), packet.packetDataLength );
    TEST_ASSERT_EQUAL_UINT8_ARRAY( &( expectedPacket[ 0 ] ),
                                   packet.pPacketData,
                                   packet.packetDataLength );
}

/*-----------------------------------------------------------*/

/**
 * @brief Test H265 packetization AddFrame with multiple NAL units.
 */
void test_H265_Packetizer_AddFrame_Multiple_Nalus_3_Byte_Start_Code( void )
{
    H265PacketizerContext_t ctx;
    H265Result_t result;
    H265Nalu_t naluArray[ MAX_NALUS_IN_A_FRAME ];
    uint8_t frameData[] =
    {
        0x00, 0x00, 0x01,       /* Start code. */
        0x40, 0x01, 0xAA, 0xBB, /* NALU1. Header: Type=32, TID=1. */
        0x00, 0x00, 0x01,       /* Start code. */
        0x42, 0x02, 0xCC, 0xDD, /* NALU2. Header: Type=33, TID=2. */
        0x00, 0x00, 0x01,       /* Start code. */
        0xC4, 0x03, 0xEE, 0xFF  /* NALU3 NAL. Header: F=1, Type=34, TID=3. */
    };
    H265Frame_t frame =
    {
        .pFrameData         = &( frameData[ 0 ] ),
        .frameDataLength    = sizeof( frameData )
    };
    H265Packet_t packet =
    {
        .pPacketData = &( packetBuffer[ 0 ] ),
        .packetDataLength = MAX_H265_PACKET_LENGTH
    };
    /* Expected aggregate packet. */
    uint8_t expectedPacket[] =
    {
        0xE0, 0x01,                 /* Payload header: F=1, Type=48, TID=1. */
        0x00, 0x04,                 /* NALU1 size. */
        0x40, 0x01, 0xAA, 0xBB,     /* NALU1 payload. */
        0x00, 0x04,                 /* NALU2 size. */
        0x42, 0x02, 0xCC, 0xDD,     /* NALU2 payload. */
        0x00, 0x04,                 /* NALU3 size. */
        0xC4, 0x03, 0xEE, 0xFF      /* NALU3 payload. */
    };

    result = H265Packetizer_Init( &( ctx ),
                                  &( naluArray[ 0 ] ),
                                  MAX_NALUS_IN_A_FRAME );

    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    result = H265Packetizer_AddFrame( &( ctx ), &( frame ) );

    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );
    TEST_ASSERT_EQUAL( 3, ctx.naluCount );

    result = H265Packetizer_GetPacket( &( ctx ), &( packet ) );

    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );
    TEST_ASSERT_EQUAL( sizeof( expectedPacket ), packet.packetDataLength );
    TEST_ASSERT_EQUAL_UINT8_ARRAY( &( expectedPacket[ 0 ] ),
                                   packet.pPacketData,
                                   packet.packetDataLength );
}

/*-----------------------------------------------------------*/

/**
 * @brief Test H265 packetization AddFrame with multiple NAL units.
 */
void test_H265_Packetizer_AddFrame_Multiple_Nalus_Mixed_Start_Code( void )
{
    H265PacketizerContext_t ctx;
    H265Result_t result;
    H265Nalu_t naluArray[ MAX_NALUS_IN_A_FRAME ];
    uint8_t frameData[] =
    {
        0x00, 0x00, 0x00, 0x01, /* Start code. */
        0x40, 0x01, 0xAA, 0xBB, /* NALU1. Header: Type=32, TID=1. */
        0x00, 0x00, 0x01,       /* Start code. */
        0x42, 0x02, 0xCC, 0xDD, /* NALU2. Header: Type=33, TID=2. */
        0x00, 0x00, 0x00, 0x01, /* Start code. */
        0xC4, 0x03, 0xEE, 0xFF  /* NALU3 NAL. Header: F=1, Type=34, TID=3. */
    };
    H265Frame_t frame =
    {
        .pFrameData         = &( frameData[ 0 ] ),
        .frameDataLength    = sizeof( frameData )
    };
    H265Packet_t packet =
    {
        .pPacketData = &( packetBuffer[ 0 ] ),
        .packetDataLength = MAX_H265_PACKET_LENGTH
    };
    /* Expected aggregate packet. */
    uint8_t expectedPacket[] =
    {
        0xE0, 0x01,                 /* Payload header: F=1, Type=48, TID=1. */
        0x00, 0x04,                 /* NALU1 size. */
        0x40, 0x01, 0xAA, 0xBB,     /* NALU1 payload. */
        0x00, 0x04,                 /* NALU2 size. */
        0x42, 0x02, 0xCC, 0xDD,     /* NALU2 payload. */
        0x00, 0x04,                 /* NALU3 size. */
        0xC4, 0x03, 0xEE, 0xFF      /* NALU3 payload. */
    };

    result = H265Packetizer_Init( &( ctx ),
                                  &( naluArray[ 0 ] ),
                                  MAX_NALUS_IN_A_FRAME );

    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    result = H265Packetizer_AddFrame( &( ctx ), &( frame ) );

    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );
    TEST_ASSERT_EQUAL( 3, ctx.naluCount );

    result = H265Packetizer_GetPacket( &( ctx ), &( packet ) );

    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );
    TEST_ASSERT_EQUAL( sizeof( expectedPacket ), packet.packetDataLength );
    TEST_ASSERT_EQUAL_UINT8_ARRAY( &( expectedPacket[ 0 ] ),
                                   packet.pPacketData,
                                   packet.packetDataLength );
}

/*-----------------------------------------------------------*/

/**
 * @brief Test H265 packetization AddFrame with malformed frame.
 */
void test_H265_Packetizer_AddFrame_Malformed_Incomplete_Nalu_Header( void )
{
    H265PacketizerContext_t ctx;
    H265Result_t result;
    H265Nalu_t naluArray[ MAX_NALUS_IN_A_FRAME ];
    uint8_t malformedFrame[] =
    {
        0x00, 0x00, 0x00, 0x01, /* Start code. */
        0x40,                   /* Incomplete NALU header. */
        0x00, 0x00, 0x00, 0x01, /* Start code. */
        0x42, 0x01, 0xCC, 0xDD  /* Valid NALU. */
    };
    H265Frame_t frame =
    {
        .pFrameData = &( malformedFrame[ 0 ] ),
        .frameDataLength = sizeof( malformedFrame )
    };

    result = H265Packetizer_Init( &( ctx ),
                                  &( naluArray[ 0 ] ),
                                  MAX_NALUS_IN_A_FRAME );

    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    result = H265Packetizer_AddFrame( &( ctx ), &( frame ) );

    TEST_ASSERT_EQUAL( H265_RESULT_MALFORMED_PACKET, result );
}

/*-----------------------------------------------------------*/

/**
 * @brief Test H265 packetization AddFrame with a malformed NALU in the middle.
 */
void test_H265_Packetizer_AddFrame_Malformed_In_Between_Nalu( void )
{
    H265PacketizerContext_t ctx;
    H265Result_t result;
    H265Nalu_t naluArray[ MAX_NALUS_IN_A_FRAME ];
    uint8_t frameData[] =
    {
        0x00, 0x00, 0x01,       /* First start code (3-byte). */
        0x40, 0x01, 0x02, 0x03, /* Valid first NALU. */
        0x00, 0x00, 0x01,       /* Second start code (3-byte) */
        0x41,                   /* Malformed NALU. */
        0x00, 0x00, 0x01,       /* Third start code (3-byte). */
        0x42, 0x01, 0x04, 0x05  /* Final valid NALU. */
    };
    H265Frame_t frame =
    {
        .pFrameData = &( frameData[ 0 ] ),
        .frameDataLength = sizeof( frameData )
    };

    result = H265Packetizer_Init( &( ctx ),
                                  &( naluArray[ 0 ] ),
                                  MAX_NALUS_IN_A_FRAME );

    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    result = H265Packetizer_AddFrame( &( ctx ), &( frame ) );

    TEST_ASSERT_EQUAL( H265_RESULT_MALFORMED_PACKET, result );
}

/*-----------------------------------------------------------*/

/**
 * @brief Test H265 packetization AddFrame with malformed last NALU.
 */
void test_H265_Packetizer_AddFrame_Malformed_Last_Nalu( void )
{
    H265PacketizerContext_t ctx;
    H265Result_t result;
    H265Nalu_t naluArray[ MAX_NALUS_IN_A_FRAME ];
    uint8_t frameData[] =
    {
        0x00, 0x00, 0x00, 0x01,  /* First start code. */
        0x40, 0x01, 0x02, 0x03,  /* Valid first NAL unit. */
        0x00, 0x00, 0x00, 0x01,  /* Second start code. */
        0x40                     /* Malformed NAL unit (only 1 byte, less than NALU_HEADER_SIZE). */
    };
    H265Frame_t frame =
    {
        .pFrameData = &( frameData[ 0 ] ),
        .frameDataLength = sizeof( frameData )
    };

    result = H265Packetizer_Init( &( ctx ),
                                  &( naluArray[ 0 ] ),
                                  MAX_NALUS_IN_A_FRAME );

    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    result = H265Packetizer_AddFrame( &( ctx ), &( frame ) );

    TEST_ASSERT_EQUAL( H265_RESULT_MALFORMED_PACKET, result );
}

/*-----------------------------------------------------------*/

/**
 * @brief Test H265 packetization AddFrame with zero NALU start index.
 */
void test_H265_Packetizer_AddFrame_Malformed_No_Start_Code( void )
{
    H265PacketizerContext_t ctx;
    H265Result_t result;
    H265Nalu_t naluArray[ MAX_NALUS_IN_A_FRAME ];
    /* Create frame data without any start codes. */
    uint8_t frameData[] =
    {
        0x40, 0x01,            /* NALU header. */
        0x02, 0x03, 0x04, 0x05 /* NALU payload. */
    };
    H265Frame_t frame =
    {
        .pFrameData = &( frameData[ 0 ] ),
        .frameDataLength = sizeof( frameData )
    };

    result = H265Packetizer_Init( &( ctx ),
                                  &( naluArray[ 0 ] ),
                                  MAX_NALUS_IN_A_FRAME );

    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    result = H265Packetizer_AddFrame( &( ctx ), &( frame ) );

    TEST_ASSERT_EQUAL( H265_RESULT_MALFORMED_PACKET, result );
}

/*-----------------------------------------------------------*/

/**
 * @brief Test H265 packetization add NAL unit with malformed packet.
 */
void test_H265_Packetizer_AddNalu_Malformed_Too_Short_Nalu( void )
{
    H265PacketizerContext_t ctx;
    H265Result_t result;
    H265Nalu_t naluArray[ MAX_NALUS_IN_A_FRAME ];
    uint8_t malformedNaluData[] = { 0x40 }; /* Too short. */
    H265Nalu_t malformedNalu =
    {
        .pNaluData = &( malformedNaluData[ 0 ] ),
        .naluDataLength = sizeof( malformedNaluData )
    };

    result = H265Packetizer_Init( &( ctx ),
                                  &( naluArray[ 0 ] ),
                                  MAX_NALUS_IN_A_FRAME );

    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    result = H265Packetizer_AddNalu( &( ctx ), &( malformedNalu ) );

    TEST_ASSERT_EQUAL( H265_RESULT_MALFORMED_PACKET, result );
}

/*-----------------------------------------------------------*/

/**
 * @brief Test H265 packetization add NAL unit with array overflow.
 */
void test_H265_Packetizer_AddNalu_Array_Full( void )
{
    H265PacketizerContext_t ctx;
    H265Result_t result;
    H265Nalu_t naluArray[ 2 ];
    uint8_t naluData[] =
    {
        0x40, 0x01, /* NALU header. */
        0x02, 0x03  /* NALU payload. */
    };
    H265Nalu_t nalu =
    {
        .pNaluData = &( naluData[ 0 ] ),
        .naluDataLength = sizeof( naluData )
    };

    result = H265Packetizer_Init( &( ctx ),
                                  &( naluArray[ 0 ] ),
                                  2 ); /* Small array length to trigger out of memory. */

    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    /* First add should be successful. */
    result = H265Packetizer_AddNalu( &( ctx ), &( nalu ) );

    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    /* Second add should be successful. */
    result = H265Packetizer_AddNalu( &( ctx ), &( nalu ) );

    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    /* Third add should fail as we supplied naluArray of size 2 during init. */
    result = H265Packetizer_AddNalu( &( ctx ), &( nalu ) );

    TEST_ASSERT_EQUAL( H265_RESULT_OUT_OF_MEMORY, result );
}

/*-----------------------------------------------------------*/

/**
 * @brief Test H265 packetization GetPacket with small buffer size.
 */
void test_H265_Packetizer_GetPacket_Output_Buffer_Too_Small( void )
{
    H265PacketizerContext_t ctx;
    H265Result_t result;
    H265Nalu_t naluArray[ MAX_NALUS_IN_A_FRAME ];
    H265Packet_t packet =
    {
        .pPacketData = &( packetBuffer[ 0 ] ),
        .packetDataLength = 2 /* Too small to fit a packet. */
    };

    result = H265Packetizer_Init( &( ctx ),
                                  &( naluArray[ 0 ] ),
                                  MAX_NALUS_IN_A_FRAME );

    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    result = H265Packetizer_GetPacket( &( ctx ), &( packet ) );

    TEST_ASSERT_EQUAL( H265_RESULT_BAD_PARAM, result );
}

/*-----------------------------------------------------------*/

/**
 * @brief Test H265 packetization GetPacket with one aggregate and one single
 * NALU packet.
 */
void test_H265_Packetizer_GetPacket_One_Aggregate_And_One_Single_Nalu_Packet( void )
{
    H265PacketizerContext_t ctx;
    H265Result_t result;
    H265Nalu_t naluArray[ MAX_NALUS_IN_A_FRAME ];
    uint8_t naluData1[] =
    {
        0x40, 0x01, /* NALU header: Type=32, TID=1. */
        0xAA, 0xBB  /* NALU payload. */
    };
    uint8_t naluData2[] =
    {
        0x42, 0x02, /* NALU header: Type=33, TID=2. */
        0xCC, 0xDD  /* NALU payload. */
    };
    uint8_t naluData3[] =
    {
        0x40, 0x01, /* NALU header: Type=32, TID=1. */
        0xEE, 0xFF  /* NALU payload. */
    };
    H265Nalu_t nalu1 =
    {
        .pNaluData = &( naluData1[ 0 ] ),
        .naluDataLength = sizeof( naluData1 )
    };
    H265Nalu_t nalu2 =
    {
        .pNaluData = &( naluData2[ 0 ] ),
        .naluDataLength = sizeof( naluData2 )
    };
    H265Nalu_t nalu3 =
    {
        .pNaluData = &( naluData3[ 0 ] ),
        .naluDataLength = sizeof( naluData3 )
    };
    H265Packet_t packet =
    {
        .pPacketData = &( packetBuffer[ 0 ] ),
        .packetDataLength = 14 /* Enough to aggregate 2 but not the third NALU. */
    };
    uint8_t expectedAggregatePacket[] =
    {
        0x60, 0x01,             /* Payload header: Type=48, TID=1. */
        0x00, 0x04,             /* NALU1 size. */
        0x40, 0x01, 0xAA, 0xBB, /* NALU1 payload. */
        0x00, 0x04,             /* NALU2 size. */
        0x42, 0x02, 0xCC, 0xDD  /* NALU2 payload. */
    };
    result = H265Packetizer_Init( &( ctx ),
                                  &( naluArray[ 0 ] ),
                                  MAX_NALUS_IN_A_FRAME );

    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    result = H265Packetizer_AddNalu( &( ctx ), &( nalu1 ) );

    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    result = H265Packetizer_AddNalu( &( ctx ), &( nalu2 ) );

    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    result = H265Packetizer_AddNalu( &( ctx ), &( nalu3 ) );

    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    /* Should get aggregate packet. */
    result = H265Packetizer_GetPacket( &( ctx ), &( packet ) );
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );
    TEST_ASSERT_EQUAL( sizeof( expectedAggregatePacket ), packet.packetDataLength );
    TEST_ASSERT_EQUAL_UINT8_ARRAY( &( expectedAggregatePacket[ 0 ] ),
                                   packet.pPacketData,
                                   packet.packetDataLength );

    /* Should get single NALU packet. */
    packet.packetDataLength = 14;
    result = H265Packetizer_GetPacket( &( ctx ), &( packet ) );
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );
    TEST_ASSERT_EQUAL( sizeof( naluData3 ), packet.packetDataLength );
    TEST_ASSERT_EQUAL_UINT8_ARRAY( &( naluData3[ 0 ] ),
                                   packet.pPacketData,
                                   packet.packetDataLength );

    /* No more packets. */
    result = H265Packetizer_GetPacket( &( ctx ), &( packet ) );
    TEST_ASSERT_EQUAL( H265_RESULT_NO_MORE_PACKETS, result );
}

/*-----------------------------------------------------------*/

/**
 * @brief Test H265 packetization GetPacket for bad parameters.
 */
void test_H265_Packetizer_GetPacket_Bad_Params( void )
{
    H265PacketizerContext_t ctx;
    H265Result_t result;
    H265Packet_t packet =
    {
        .pPacketData = &( packetBuffer[ 0 ] ),
        .packetDataLength = MAX_H265_PACKET_LENGTH
    };

    result = H265Packetizer_GetPacket( NULL, &( packet ) );

    TEST_ASSERT_EQUAL( H265_RESULT_BAD_PARAM, result );

    result = H265Packetizer_GetPacket( &( ctx ), NULL );

    TEST_ASSERT_EQUAL( H265_RESULT_BAD_PARAM, result );

    packet.pPacketData = NULL;
    result = H265Packetizer_GetPacket( &( ctx ), &( packet ) );

    TEST_ASSERT_EQUAL( H265_RESULT_BAD_PARAM, result );

    packet.pPacketData = &( packetBuffer[ 0 ] );
    packet.packetDataLength = 0;
    result = H265Packetizer_GetPacket( &( ctx ), &( packet ) );

    TEST_ASSERT_EQUAL( H265_RESULT_BAD_PARAM, result );

    packet.packetDataLength = NALU_HEADER_SIZE;
    result = H265Packetizer_GetPacket( &( ctx ), &( packet ) );

    TEST_ASSERT_EQUAL( H265_RESULT_BAD_PARAM, result );
}

/*-----------------------------------------------------------*/

/**
 * @brief Test H265 packetization GetPacket with aggregation array length limit.
 */
void test_H265_Packetizer_GetPacket_Add_Maximum_Nalus_And_Aggregate_All( void )
{
    H265PacketizerContext_t ctx;
    H265Result_t result;
    H265Nalu_t naluArray[ 3 ];
    uint8_t naluData1[] =
    {
        0x40, 0x01, /* NALU header: Type=32, TID=1. */
        0x02, 0x03  /* NALU payload. */
    };
    uint8_t naluData2[] =
    {
        0x40, 0x01, /* NALU header: Type=32, TID=1. */
        0x04, 0x05  /* NALU payload. */
    };
    uint8_t naluData3[] =
    {
        0x40, 0x01, /* NALU header: Type=32, TID=1. */
        0x06, 0x07  /* NALU payload. */
    };
    H265Nalu_t nalu1 =
    {
        .pNaluData = &( naluData1[ 0 ] ),
        .naluDataLength = sizeof( naluData1 )
    };
    H265Nalu_t nalu2 =
    {
        .pNaluData = &( naluData2[ 0 ] ),
        .naluDataLength = sizeof( naluData2 )
    };
    H265Nalu_t nalu3 =
    {
        .pNaluData = &( naluData3[ 0 ] ),
        .naluDataLength = sizeof( naluData3 )
    };
    H265Packet_t packet =
    {
        .pPacketData = &( packetBuffer[ 0 ] ),
        .packetDataLength = MAX_H265_PACKET_LENGTH
    };
    uint8_t expectedAggregatePacket[] =
    {
        0x60, 0x01,             /* Payload header: Type=48, TID=1. */
        0x00, 0x04,             /* NALU1 size. */
        0x40, 0x01, 0x02, 0x03, /* NALU1 payload. */
        0x00, 0x04,             /* NALU2 size. */
        0x40, 0x01, 0x04, 0x05 ,/* NALU2 payload. */
        0x00, 0x04,             /* NALU3 size. */
        0x40, 0x01, 0x06, 0x07 ,/* NALU3 payload. */
    };

    result = H265Packetizer_Init( &( ctx ),
                                  &( naluArray[ 0 ] ),
                                  3 );
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    result = H265Packetizer_AddNalu( &( ctx ), &( nalu1 ) );

    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    result = H265Packetizer_AddNalu( &( ctx ), &( nalu2 ) );

    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    result = H265Packetizer_AddNalu( &( ctx ), &( nalu3 ) );

    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    /* Ensure that we cannot add any more NALUs. */
    result = H265Packetizer_AddNalu( &( ctx ), &( nalu3 ) );

    TEST_ASSERT_EQUAL( H265_RESULT_OUT_OF_MEMORY, result );

    result = H265Packetizer_GetPacket( &( ctx ), &( packet ) );

    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );
    TEST_ASSERT_EQUAL( sizeof( expectedAggregatePacket ), packet.packetDataLength );
    TEST_ASSERT_EQUAL_UINT8_ARRAY( &( expectedAggregatePacket[ 0 ] ),
                                   packet.pPacketData,
                                   packet.packetDataLength );

    /* Ensure no more packets. */
    packet.packetDataLength = MAX_H265_PACKET_LENGTH;
    result = H265Packetizer_GetPacket( &( ctx ), &( packet ) );

    TEST_ASSERT_EQUAL( H265_RESULT_NO_MORE_PACKETS, result );
}

/*-----------------------------------------------------------*/

/**
 * @brief Test H265 packetization add nalu for bad params.
 */
void test_H265_Packetizer_AddNalu_Bad_Params( void )
{
    H265PacketizerContext_t ctx;
    H265Result_t result;
    uint8_t naluData[] =
    {
        0x40, 0x01, /* NALU header. */
        0x02, 0x03  /* NALU payload. */
    };
    H265Nalu_t nalu =
    {
        .pNaluData = &( naluData[ 0 ] ),
        .naluDataLength = sizeof( naluData )
    };

    result = H265Packetizer_AddNalu( &( ctx ), NULL );

    TEST_ASSERT_EQUAL( H265_RESULT_BAD_PARAM, result );

    result = H265Packetizer_AddNalu( NULL, &( nalu ) );

    TEST_ASSERT_EQUAL( H265_RESULT_BAD_PARAM, result );

    nalu.pNaluData = NULL;
    result = H265Packetizer_AddNalu( &( ctx ), &( nalu ) );

    TEST_ASSERT_EQUAL( H265_RESULT_BAD_PARAM, result );
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

/*-----------------------------------------------------------*/

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

/*-----------------------------------------------------------*/

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

/*-----------------------------------------------------------*/

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

/*-----------------------------------------------------------*/

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

    result = H265Depacketizer_GetNalu(&ctx, &nalu);
    TEST_ASSERT_EQUAL(H265_RESULT_MALFORMED_PACKET, result);
}

/*-----------------------------------------------------------*/

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


    ctx.curPacketIndex = sizeof(packetData) - 1;
    result = H265Depacketizer_GetNalu(&ctx, &nalu);
    TEST_ASSERT_EQUAL(H265_RESULT_OK, result);

    TEST_ASSERT_EQUAL(1, ctx.tailIndex);
    TEST_ASSERT_EQUAL(0, ctx.packetCount);
}

/*-----------------------------------------------------------*/

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
    ctx.curPacketIndex = 8; /* Point to second NALU */
    result = H265Depacketizer_GetNalu( &ctx, &nalu );
    TEST_ASSERT_EQUAL( H265_RESULT_MALFORMED_PACKET, result );
}

/*-----------------------------------------------------------*/

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
    TEST_ASSERT_EQUAL( 8, ctx.curPacketIndex );    /* Verify currentOffset is set to start of second NALU */

    /* Get second NALU */
    result = H265Depacketizer_GetNalu( &ctx, &nalu );
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );
    TEST_ASSERT_EQUAL( 14, ctx.curPacketIndex );     /* Verify currentOffset is set to start of third NALU */

    /* Get third NALU */
    result = H265Depacketizer_GetNalu( &ctx, &nalu );
    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    /* Verify packet is completed */
    result = H265Depacketizer_GetNalu( &ctx, &nalu );
    TEST_ASSERT_EQUAL( H265_RESULT_NO_MORE_NALUS, result );
}

/*-----------------------------------------------------------*/

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

    /* Process the packet */
    result = H265Depacketizer_GetNalu(&ctx, &nalu);
    TEST_ASSERT_EQUAL(H265_RESULT_OUT_OF_MEMORY, result);
}

/*-----------------------------------------------------------*/

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

/*-----------------------------------------------------------*/

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
}

/*-----------------------------------------------------------*/

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

    /* Test 2: Unsupported packet type */
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
