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
void test_H265_Packetizer_Two_Fragment_Packets( void )
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
void test_H265_Depacketizer_GetFrame_Single_Nalu( void )
{
    H265DepacketizerContext_t ctx;
    H265Result_t result;
    H265Packet_t packetsArray[ 10 ];
    H265Frame_t frame =
    {
        .pFrameData = &( frameBuffer[ 0 ] ),
        .frameDataLength = MAX_FRAME_LENGTH
    };
    uint8_t singleNaluPacketData[] =
    {
        0x26, 0x01,      /* NALU header: Type=19, TID=1. */
        0xAA, 0xBB, 0xCC /* NALU payload. */
    };
    H265Packet_t singleNaluPacket =
    {
        .pPacketData = &( singleNaluPacketData[ 0 ] ),
        .packetDataLength = sizeof( singleNaluPacketData )
    };
    uint8_t expectedFrame[] =
    {
        /* Start code. */
        0x00, 0x00, 0x00, 0x01,
        /* Nalu. */
        0x26, 0x01,
        0xAA, 0xBB, 0xCC
    };

    result = H265Depacketizer_Init( &( ctx ),
                                    &( packetsArray[ 0 ] ),
                                    10 );

    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    result = H265Depacketizer_AddPacket( &( ctx ), &( singleNaluPacket ) );

    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    result = H265Depacketizer_GetFrame( &( ctx ), &( frame ) );

    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );
    TEST_ASSERT_EQUAL( sizeof( expectedFrame ), frame.frameDataLength );
    TEST_ASSERT_EQUAL_UINT8_ARRAY( &( expectedFrame[ 0 ] ),
                                   frame.pFrameData,
                                   frame.frameDataLength );
}

/*-----------------------------------------------------------*/

/**
 * @brief Test H265 depacketization with a single NALU.
 */
void test_H265_Depacketizer_GetFrame_Single_Nalu_Out_Of_Memory( void )
{
    H265DepacketizerContext_t ctx;
    H265Result_t result;
    H265Packet_t packetsArray[ 10 ];
    H265Frame_t frame =
    {
        .pFrameData = &( frameBuffer[ 0 ] ),
        .frameDataLength = 3 /* Cannot fit even start code. */
    };
    uint8_t singleNaluPacketData[] =
    {
        0x26, 0x01,      /* NALU header: Type=19, TID=1. */
        0xAA, 0xBB, 0xCC /* NALU payload. */
    };
    H265Packet_t singleNaluPacket =
    {
        .pPacketData = &( singleNaluPacketData[ 0 ] ),
        .packetDataLength = sizeof( singleNaluPacketData )
    };

    result = H265Depacketizer_Init( &( ctx ),
                                    &( packetsArray[ 0 ] ),
                                    10 );

    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    result = H265Depacketizer_AddPacket( &( ctx ), &( singleNaluPacket ) );

    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    result = H265Depacketizer_GetFrame( &( ctx ), &( frame ) );

    TEST_ASSERT_EQUAL( H265_RESULT_OUT_OF_MEMORY, result );
}

/*-----------------------------------------------------------*/

/**
 * @brief Test H265 depacketization with a single NALU.
 */
void test_H265_Depacketizer_GetFrame_Single_Nalu_Unsupported_Packet( void )
{
    H265DepacketizerContext_t ctx;
    H265Result_t result;
    H265Packet_t packetsArray[ 10 ];
    H265Frame_t frame =
    {
        .pFrameData = &( frameBuffer[ 0 ] ),
        .frameDataLength = MAX_FRAME_LENGTH
    };
    uint8_t singleNaluPacketData[] =
    {
        0x00, 0x01,      /* NALU header: Type=0(unsupported), TID=1. */
        0xAA, 0xBB, 0xCC /* NALU payload. */
    };
    H265Packet_t singleNaluPacket =
    {
        .pPacketData = &( singleNaluPacketData[ 0 ] ),
        .packetDataLength = sizeof( singleNaluPacketData )
    };

    result = H265Depacketizer_Init( &( ctx ),
                                    &( packetsArray[ 0 ] ),
                                    10 );

    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    result = H265Depacketizer_AddPacket( &( ctx ), &( singleNaluPacket ) );

    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    result = H265Depacketizer_GetFrame( &( ctx ), &( frame ) );

    TEST_ASSERT_EQUAL( H265_RESULT_UNSUPPORTED_PACKET, result );
}

/*-----------------------------------------------------------*/

/**
 * @brief Test H265 depacketization of fragmentation units.
 */
void test_H265Depacketizer_GetFrame_Fragmentation_Units( void )
{
    H265DepacketizerContext_t ctx;
    H265Result_t result;
    H265Packet_t packetsArray[ 10 ];
    H265Frame_t frame =
    {
        .pFrameData = &( frameBuffer[ 0 ] ),
        .frameDataLength = MAX_FRAME_LENGTH
    };
    uint8_t fragmentUnitData1[] =
    {
        0x62, 0x01, /* Payload header: Type=49, TID=1. */
        0xA0,       /* FU header: S=1, Type=32. */
        0xAA, 0xBB  /* FU payload. */
    };
    uint8_t fragmentUnitData2[] =
    {
        0x62, 0x01, /* Payload header: Type=49, TID=1. */
        0x60,       /* FU header: E=1, Type=32. */
        0xCC, 0xDD  /* FU payload. */
    };
    H265Packet_t fragment1 =
    {
        .pPacketData = &( fragmentUnitData1[ 0 ] ),
        .packetDataLength = sizeof( fragmentUnitData1 )
    };
    H265Packet_t fragment2 =
    {
        .pPacketData = &( fragmentUnitData2[ 0 ] ),
        .packetDataLength = sizeof( fragmentUnitData2 )
    };
    uint8_t expectedFrame[] =
    {
        /* Start code. */
        0x00, 0x00, 0x00, 0x01,
        /* Nalu. */
        0x40, 0x01,            /* Nalu header: Type=32, TID=1. */
        0xAA, 0xBB, 0xCC, 0xDD /* Nalu payload. */
    };

    result = H265Depacketizer_Init( &( ctx ),
                                    &( packetsArray[ 0 ] ),
                                    10 );

    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    result = H265Depacketizer_AddPacket( &( ctx ), &( fragment1 ) );

    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    result = H265Depacketizer_AddPacket( &( ctx ), &( fragment2 ) );

    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    result = H265Depacketizer_GetFrame( &( ctx ), &( frame ) );

    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );
    TEST_ASSERT_EQUAL( sizeof( expectedFrame ), frame.frameDataLength );
    TEST_ASSERT_EQUAL_UINT8_ARRAY( &( expectedFrame[ 0 ] ),
                                   frame.pFrameData,
                                   frame.frameDataLength );
}

/*-----------------------------------------------------------*/

/**
 * @brief Test H265 depacketization of fragmentation unit with invalid second fragment.
 */
void test_H265Depacketizer_GetFrame_Fragmentation_Units_Invalid_Second_Fragment( void )
{
    H265DepacketizerContext_t ctx;
    H265Result_t result;
    H265Packet_t packetsArray[ 10 ];
    H265Frame_t frame =
    {
        .pFrameData = &( frameBuffer[ 0 ] ),
        .frameDataLength = MAX_FRAME_LENGTH
    };
    uint8_t fragmentUnitData1[] =
    {
        0x62, 0x01, /* Payload header: Type=49, TID=1. */
        0xA0,       /* FU header: S=1, Type=32. */
        0xAA, 0xBB  /* FU payload. */
    };
    uint8_t fragmentUnitData2[] =
    {
        0x40, 0x01, /* Payload header: Type=32 (invalid), TID=1. */
        0x60,       /* FU header: E=1, Type=32. */
        0xCC, 0xDD  /* FU payload. */
    };
    H265Packet_t fragment1 =
    {
        .pPacketData = &( fragmentUnitData1[ 0 ] ),
        .packetDataLength = sizeof( fragmentUnitData1 )
    };
    H265Packet_t fragment2 =
    {
        .pPacketData = &( fragmentUnitData2[ 0 ] ),
        .packetDataLength = sizeof( fragmentUnitData2 )
    };

    result = H265Depacketizer_Init( &( ctx ),
                                    &( packetsArray[ 0 ] ),
                                    10 );

    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    result = H265Depacketizer_AddPacket( &( ctx ), &( fragment1 ) );

    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    result = H265Depacketizer_AddPacket( &( ctx ), &( fragment2 ) );

    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    result = H265Depacketizer_GetFrame( &( ctx ), &( frame ) );

    TEST_ASSERT_EQUAL( H265_RESULT_MALFORMED_PACKET, result );
}

/*-----------------------------------------------------------*/

/**
 * @brief Test H265 depacketization of fragmentation unit out of memory when
 * writing header.
 */
void test_H265Depacketizer_GetFrame_Fragmentation_Units_Out_Of_Memory_Writing_Header( void )
{
    H265DepacketizerContext_t ctx;
    H265Result_t result;
    H265Packet_t packetsArray[ 10 ];
    H265Frame_t frame =
    {
        .pFrameData = &( frameBuffer[ 0 ] ),
        .frameDataLength = 5 /* Can fit start code but not NALU header. */
    };
    uint8_t fragmentUnitData1[] =
    {
        0x62, 0x01, /* Payload header: Type=49, TID=1. */
        0xA0,       /* FU header: S=1, Type=32. */
        0xAA, 0xBB  /* FU payload. */
    };
    uint8_t fragmentUnitData2[] =
    {
        62, 0x01, /* Payload header: Type=49, TID=1. */
        0x60,       /* FU header: E=1, Type=32. */
        0xCC, 0xDD  /* FU payload. */
    };
    H265Packet_t fragment1 =
    {
        .pPacketData = &( fragmentUnitData1[ 0 ] ),
        .packetDataLength = sizeof( fragmentUnitData1 )
    };
    H265Packet_t fragment2 =
    {
        .pPacketData = &( fragmentUnitData2[ 0 ] ),
        .packetDataLength = sizeof( fragmentUnitData2 )
    };

    result = H265Depacketizer_Init( &( ctx ),
                                    &( packetsArray[ 0 ] ),
                                    10 );

    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    result = H265Depacketizer_AddPacket( &( ctx ), &( fragment1 ) );

    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    result = H265Depacketizer_AddPacket( &( ctx ), &( fragment2 ) );

    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    result = H265Depacketizer_GetFrame( &( ctx ), &( frame ) );

    TEST_ASSERT_EQUAL( H265_RESULT_OUT_OF_MEMORY, result );
}

/*-----------------------------------------------------------*/

/**
 * @brief Test H265 depacketization of fragmentation unit out of memory when
 * writing payload.
 */
void test_H265Depacketizer_GetFrame_Fragmentation_Units_Out_Of_Memory_Writing_Payload( void )
{
    H265DepacketizerContext_t ctx;
    H265Result_t result;
    H265Packet_t packetsArray[ 10 ];
    H265Frame_t frame =
    {
        .pFrameData = &( frameBuffer[ 0 ] ),
        .frameDataLength = 7 /* Can fit start code and NALU header but not NALU payload. */
    };
    uint8_t fragmentUnitData1[] =
    {
        0x62, 0x01, /* Payload header: Type=49, TID=1. */
        0xA0,       /* FU header: S=1, Type=32. */
        0xAA, 0xBB  /* FU payload. */
    };
    uint8_t fragmentUnitData2[] =
    {
        62, 0x01, /* Payload header: Type=49, TID=1. */
        0x60,       /* FU header: E=1, Type=32. */
        0xCC, 0xDD  /* FU payload. */
    };
    H265Packet_t fragment1 =
    {
        .pPacketData = &( fragmentUnitData1[ 0 ] ),
        .packetDataLength = sizeof( fragmentUnitData1 )
    };
    H265Packet_t fragment2 =
    {
        .pPacketData = &( fragmentUnitData2[ 0 ] ),
        .packetDataLength = sizeof( fragmentUnitData2 )
    };

    result = H265Depacketizer_Init( &( ctx ),
                                    &( packetsArray[ 0 ] ),
                                    10 );

    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    result = H265Depacketizer_AddPacket( &( ctx ), &( fragment1 ) );

    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    result = H265Depacketizer_AddPacket( &( ctx ), &( fragment2 ) );

    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    result = H265Depacketizer_GetFrame( &( ctx ), &( frame ) );

    TEST_ASSERT_EQUAL( H265_RESULT_OUT_OF_MEMORY, result );
}

/*-----------------------------------------------------------*/

/**
 * @brief Test H265 depacketization of aggregation packet.
 */
void test_H265Depacketizer_GetFrame_Aggregation_Packet( void )
{
    H265DepacketizerContext_t ctx;
    H265Result_t result;
    H265Packet_t packetsArray[ 10 ];
    H265Frame_t frame =
    {
        .pFrameData = &( frameBuffer[ 0 ] ),
        .frameDataLength = MAX_FRAME_LENGTH
    };
    uint8_t aggregationPacketData[] =
    {
        0x60, 0x01, /* Payload header: Type=48, TID=1. */
        0x00, 0x04, /* NALU1 Size. */
        0x40, 0x01, 0xAA, 0xBB, /* NALU1 payload. */
        0x00, 0x04, /* NALU2 Size. */
        0x42, 0x02, 0xCC, 0xDD, /* NALU2 payload. */
    };
    H265Packet_t aggregationPacket =
    {
        .pPacketData = &( aggregationPacketData[ 0 ] ),
        .packetDataLength = sizeof( aggregationPacketData )
    };
    uint8_t expectedFrame[] =
    {
        /* Start code. */
        0x00, 0x00, 0x00, 0x01,
        /* NALU1. */
        0x40, 0x01, 0xAA, 0xBB,
        /* Start code. */
        0x00, 0x00, 0x00, 0x01,
        /* NALU2. */
        0x42, 0x02, 0xCC, 0xDD,
    };

    result = H265Depacketizer_Init( &( ctx ),
                                    &( packetsArray[ 0 ] ),
                                    10 );

    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    result = H265Depacketizer_AddPacket( &( ctx ), &( aggregationPacket ) );

    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    result = H265Depacketizer_GetFrame( &( ctx ), &( frame ) );

    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );
    TEST_ASSERT_EQUAL( sizeof( expectedFrame ), frame.frameDataLength );
    TEST_ASSERT_EQUAL_UINT8_ARRAY( &( expectedFrame[ 0 ] ),
                                   frame.pFrameData,
                                   frame.frameDataLength );
}

/*-----------------------------------------------------------*/

void test_H265Depacketizer_GetFrame_Aggregation_Packet_Incomplete_Second_Nalu( void )
{
    H265DepacketizerContext_t ctx;
    H265Result_t result;
    H265Packet_t packetsArray[ 10 ];
    H265Frame_t frame =
    {
        .pFrameData = &( frameBuffer[ 0 ] ),
        .frameDataLength = MAX_FRAME_LENGTH
    };
    uint8_t aggregationPacketData[] =
    {
        0x60, 0x01, /* Payload header: Type=48, TID=1. */
        0x00, 0x04, /* NALU1 Size. */
        0x40, 0x01, 0xAA, 0xBB, /* NALU1 payload. */
        0x00, 0x04, /* NALU2 Size. */
        0x42, 0x02, 0xCC /* NALU2 payload is incomplete. */
    };
    H265Packet_t aggregationPacket =
    {
        .pPacketData = &( aggregationPacketData[ 0 ] ),
        .packetDataLength = sizeof( aggregationPacketData )
    };

    result = H265Depacketizer_Init( &( ctx ),
                                    &( packetsArray[ 0 ] ),
                                    10 );

    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    result = H265Depacketizer_AddPacket( &( ctx ), &( aggregationPacket ) );

    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    result = H265Depacketizer_GetFrame( &( ctx ), &( frame ) );

    TEST_ASSERT_EQUAL( H265_RESULT_MALFORMED_PACKET, result );
}

/*-----------------------------------------------------------*/

void test_H265Depacketizer_GetFrame_Aggregation_Packet_Out_Of_Memory( void )
{
    H265DepacketizerContext_t ctx;
    H265Result_t result;
    H265Packet_t packetsArray[ 10 ];
    H265Frame_t frame =
    {
        .pFrameData = &( frameBuffer[ 0 ] ),
        .frameDataLength = 10 /* Cannot fit the entire frame. */
    };
    uint8_t aggregationPacketData[] =
    {
        0x60, 0x01, /* Payload header: Type=48, TID=1. */
        0x00, 0x04, /* NALU1 Size. */
        0x40, 0x01, 0xAA, 0xBB, /* NALU1 payload. */
        0x00, 0x04, /* NALU2 Size. */
        0x42, 0x02, 0xCC /* NALU2 payload is incomplete. */
    };
    H265Packet_t aggregationPacket =
    {
        .pPacketData = &( aggregationPacketData[ 0 ] ),
        .packetDataLength = sizeof( aggregationPacketData )
    };

    result = H265Depacketizer_Init( &( ctx ),
                                    &( packetsArray[ 0 ] ),
                                    10 );

    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    result = H265Depacketizer_AddPacket( &( ctx ), &( aggregationPacket ) );

    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    result = H265Depacketizer_GetFrame( &( ctx ), &( frame ) );

    TEST_ASSERT_EQUAL( H265_RESULT_OUT_OF_MEMORY, result );
}

/*-----------------------------------------------------------*/

/**
 * @brief Test H265 depacketization AddPacket for bad params.
 */
void test_H265Depacketizer_AddPacket_Bad_Params( void )
{
    H265DepacketizerContext_t ctx;
    H265Packet_t packet;
    H265Result_t result;

    result = H265Depacketizer_AddPacket( &( ctx ), NULL );

    TEST_ASSERT_EQUAL( H265_RESULT_BAD_PARAM, result );

    result = H265Depacketizer_AddPacket( NULL, &( packet ) );

    TEST_ASSERT_EQUAL( H265_RESULT_BAD_PARAM, result );
}

/*-----------------------------------------------------------*/

/**
 * @brief Test H265 depacketization AddPacket for out of memory.
 */
void test_H265Depacketizer_AddPacket_Out_Of_Memory( void )
{
    H265DepacketizerContext_t ctx;
    H265Result_t result;
    H265Packet_t packetsArray[ 2 ];
    uint8_t packetData[] =
    {
        0x01, 0x02, 0x03, 0x04
    };
    H265Packet_t packet =
    {
        .pPacketData = &( packetData[ 0 ] ),
        .packetDataLength = sizeof( packetData )
    };

    result = H265Depacketizer_Init( &( ctx ),
                                    &( packetsArray[ 0 ] ),
                                    2 );

    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    result = H265Depacketizer_AddPacket( &( ctx ), &( packet ) );

    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    result = H265Depacketizer_AddPacket( &( ctx ), &( packet ) );

    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );

    result = H265Depacketizer_AddPacket( &( ctx ), &( packet ) );

    TEST_ASSERT_EQUAL( H265_RESULT_OUT_OF_MEMORY, result );
}

/*-----------------------------------------------------------*/

/**
 * @brief Test H265 depacketization GetFrame for bad parameters.
 */
void test_H265Depacketizer_GetFrame_Bad_Params( void )
{
    H265DepacketizerContext_t ctx;
    H265Result_t result;
    H265Frame_t frame =
    {
        .pFrameData = &( frameBuffer[ 0 ] ),
        .frameDataLength = MAX_FRAME_LENGTH
    };

    result = H265Depacketizer_GetFrame( NULL, &( frame ) );

    TEST_ASSERT_EQUAL( H265_RESULT_BAD_PARAM, result );

    result = H265Depacketizer_GetFrame( &( ctx ), NULL );

    TEST_ASSERT_EQUAL( H265_RESULT_BAD_PARAM, result );

    frame.pFrameData = NULL;
    frame.frameDataLength = MAX_FRAME_LENGTH;
    result = H265Depacketizer_GetFrame( &( ctx ), &( frame ) );

    TEST_ASSERT_EQUAL( H265_RESULT_BAD_PARAM, result );

    frame.pFrameData = &( frameBuffer[ 0 ] );
    frame.frameDataLength = 0;
    result = H265Depacketizer_GetFrame( &( ctx ), &( frame ) );

    TEST_ASSERT_EQUAL( H265_RESULT_BAD_PARAM, result );
}

/*-----------------------------------------------------------*/

/**
 * @brief Test H265 depacketization GetPacketProperties for single nalu packet.
 */
void test_H265_Depacketizer_GetPacketProperties_Single_Nalu( void )
{
    H265Result_t result;
    uint32_t properties;
    uint8_t singleNaluPacketData[] =
    {
        0x26, 0x01,      /* NALU header: Type=19, TID=1. */
        0xAA, 0xBB, 0xCC /* NALU payload. */
    };

    result = H265Depacketizer_GetPacketProperties( &( singleNaluPacketData[ 0 ] ),
                                                   sizeof( singleNaluPacketData ),
                                                   &( properties ) );

    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );
    TEST_ASSERT_EQUAL( H265_PACKET_PROPERTY_START_PACKET | H265_PACKET_PROPERTY_END_PACKET,
                       properties );
}

/*-----------------------------------------------------------*/

/**
 * @brief Test H265 depacketization GetPacketProperties for fragmentation unit with S bit.
 */
void test_H265_Depacketizer_GetPacketProperties_Fragmentation_Unit_With_S_Bit( void )
{
    H265Result_t result;
    uint32_t properties;
    uint8_t fragmentUnitData[] =
    {
        0x62, 0x01, /* Payload header: Type=49, TID=1. */
        0xA0,       /* FU header: S=1, Type=32. */
        0xAA, 0xBB  /* FU payload. */
    };

    result = H265Depacketizer_GetPacketProperties( &( fragmentUnitData[ 0 ] ),
                                                   sizeof( fragmentUnitData ),
                                                   &( properties ) );

    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );
    TEST_ASSERT_EQUAL( H265_PACKET_PROPERTY_START_PACKET,
                       properties );
}

/*-----------------------------------------------------------*/

/**
 * @brief Test H265 depacketization GetPacketProperties for fragmentation unit with E bit.
 */
void test_H265_Depacketizer_GetPacketProperties_Fragmentation_Unit_With_E_Bit( void )
{
    H265Result_t result;
    uint32_t properties;
    uint8_t fragmentUnitData[] =
    {
        0x62, 0x01, /* Payload header: Type=49, TID=1. */
        0x60,       /* FU header: E=1, Type=32. */
        0xCC, 0xDD  /* FU payload. */
    };

    result = H265Depacketizer_GetPacketProperties( &( fragmentUnitData[ 0 ] ),
                                                   sizeof( fragmentUnitData ),
                                                   &( properties ) );

    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );
    TEST_ASSERT_EQUAL( H265_PACKET_PROPERTY_END_PACKET,
                       properties );
}

/*-----------------------------------------------------------*/

/**
 * @brief Test H265 depacketization GetPacketProperties for aggregation packet.
 */
void test_H265_Depacketizer_GetPacketProperties_Aggregation_Packet( void )
{
    H265Result_t result;
    uint32_t properties;
    uint8_t aggregationPacketData[] =
    {
        0x60, 0x01, /* Payload header: Type=48, TID=1. */
        0x00, 0x04, /* NALU1 Size. */
        0x40, 0x01, 0xAA, 0xBB, /* NALU1 payload. */
        0x00, 0x04, /* NALU2 Size. */
        0x42, 0x02, 0xCC, 0xDD, /* NALU2 payload. */
    };

    result = H265Depacketizer_GetPacketProperties( &( aggregationPacketData[ 0 ] ),
                                                   sizeof( aggregationPacketData ),
                                                   &( properties ) );

    TEST_ASSERT_EQUAL( H265_RESULT_OK, result );
    TEST_ASSERT_EQUAL( H265_PACKET_PROPERTY_START_PACKET | H265_PACKET_PROPERTY_END_PACKET,
                       properties );
}

/*-----------------------------------------------------------*/

/**
 * @brief Test H265 depacketization GetPacketProperties for unsupported packet.
 */
void test_H265_Depacketizer_GetPacketProperties_Unsupported_Packet( void )
{
    H265Result_t result;
    uint32_t properties;
    uint8_t unsupportedPacketData[] =
    {
        0x00, 0x01, /* Payload header: Type=0, TID=1. */
        0xAA, 0xBB  /* Payload. */
    };

    result = H265Depacketizer_GetPacketProperties( &( unsupportedPacketData[ 0 ] ),
                                                   sizeof( unsupportedPacketData ),
                                                   &( properties ) );

    TEST_ASSERT_EQUAL( H265_RESULT_UNSUPPORTED_PACKET, result );
}

/*-----------------------------------------------------------*/

/**
 * @brief Test H265 depacketization GetPacketProperties for bad params.
 */
void test_H265_Depacketizer_GetPacketProperties_Bad_Params( void )
{
    H265Result_t result;
    uint32_t properties;
    uint8_t singleNaluPacketData[] =
    {
        0x26, 0x01,      /* NALU header: Type=19, TID=1. */
        0xAA, 0xBB, 0xCC /* NALU payload. */
    };

    result = H265Depacketizer_GetPacketProperties( NULL,
                                                   sizeof( singleNaluPacketData ),
                                                   &( properties ) );

    TEST_ASSERT_EQUAL( H265_RESULT_BAD_PARAM, result );

    result = H265Depacketizer_GetPacketProperties( &( singleNaluPacketData[ 0 ] ),
                                                   sizeof( singleNaluPacketData ),
                                                   NULL );

    TEST_ASSERT_EQUAL( H265_RESULT_BAD_PARAM, result );

    result = H265Depacketizer_GetPacketProperties( &( singleNaluPacketData[ 0 ] ),
                                                   0,
                                                   &( properties ) );

    TEST_ASSERT_EQUAL( H265_RESULT_BAD_PARAM, result );

    result = H265Depacketizer_GetPacketProperties( &( singleNaluPacketData[ 0 ] ),
                                                   1,
                                                   &( properties ) );

    TEST_ASSERT_EQUAL( H265_RESULT_BAD_PARAM, result );
}

/*-----------------------------------------------------------*/
