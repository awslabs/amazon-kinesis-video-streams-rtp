/* Unity includes. */
#include "unity.h"
#include "catch_assert.h"

/* Standard includes. */
#include <string.h>
#include <stdint.h>

/* API includes. */
#include "vp8_packetizer.h"
#include "vp8_depacketizer.h"

/* ===========================  EXTERN VARIABLES  =========================== */

#define MAX_FRAME_LENGTH        10 * 1024
#define MAX_PACKET_IN_A_FRAME   512

#define PACKET_BUFFER_LENGTH    10

#define VP8_PACKETS_ARR_LEN     10
#define VP8_FRAME_BUF_LEN       32

uint8_t packetBuffer[ PACKET_BUFFER_LENGTH];
uint8_t frameBuffer[ MAX_FRAME_LENGTH ];
VP8Packet_t packetsArray[ VP8_PACKETS_ARR_LEN ];

void setUp( void )
{
    memset( &( packetBuffer[ 0 ] ),
            0,
            sizeof( packetBuffer ) );

    memset( &( frameBuffer[ 0 ] ),
            0,
            sizeof( frameBuffer ) );

    memset( &( packetsArray[ 0 ] ),
            0,
            VP8_PACKETS_ARR_LEN * sizeof( VP8Packet_t ) );
}

void tearDown( void )
{
}

/* ==============================  Test Cases for Packetization ============================== */

/**
 * @brief Validate VP8 packetization happy path.
 */
void test_VP8_Packetizer( void )
{
    VP8Result_t result;
    VP8PacketizerContext_t ctx;
    VP8Frame_t frame;
    VP8Packet_t pkt;
    size_t frameDataIndex = 0;
    uint8_t frameData[] =
    {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
        0x20, 0x21, 0x22, 0x23, 0x24, 0x25,
        0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
        0x40, 0x41
    };
    uint8_t payloadDescFirstPkt[] = { 0x10 }; /* S: Start of VP8 partition set to 1. */
    uint8_t payloadDescOtherPkt[] = { 0x00 };

    memset( &( frame ),
            0,
            sizeof( VP8Frame_t ) );
    frame.pFrameData = &( frameData[ 0 ] );
    frame.frameDataLength = sizeof( frameData );

    result = VP8Packetizer_Init( &( ctx ),
                                 &( frame ) );

    TEST_ASSERT_EQUAL( VP8_RESULT_OK,
                       result );

    pkt.pPacketData = &( packetBuffer[ 0 ] );
    pkt.packetDataLength = PACKET_BUFFER_LENGTH;

    result = VP8Packetizer_GetPacket( &( ctx ),
                                      &( pkt ) );

    while( result != VP8_RESULT_NO_MORE_PACKETS )
    {
        TEST_ASSERT_EQUAL( VP8_RESULT_OK,
                           result );

        if( frameDataIndex == 0 )
        {
            TEST_ASSERT_EQUAL( payloadDescFirstPkt[ 0 ],
                               pkt.pPacketData[ 0 ] );
        }
        else
        {
            TEST_ASSERT_EQUAL( payloadDescOtherPkt[ 0 ],
                               pkt.pPacketData[ 0 ] );
        }

        TEST_ASSERT_EQUAL_UINT8_ARRAY( &( frameData[ frameDataIndex ] ),
                                       &( pkt.pPacketData[ 1 ] ),
                                       pkt.packetDataLength - 1 );
        frameDataIndex += ( pkt.packetDataLength - 1 );

        pkt.pPacketData = &( packetBuffer[ 0 ] );
        pkt.packetDataLength = PACKET_BUFFER_LENGTH;

        result = VP8Packetizer_GetPacket( &( ctx ),
                                          &( pkt ) );
    }

    TEST_ASSERT_EQUAL( sizeof( frameData ),
                       frameDataIndex );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate VP8 packetization happy path containing picture ID.
 */
void test_VP8_Packetizer_PictureID( void )
{
    VP8Result_t result;
    VP8PacketizerContext_t ctx;
    VP8Frame_t frame;
    VP8Packet_t pkt;
    size_t frameDataIndex = 0;
    uint8_t frameData[] =
    {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
        0x20, 0x21, 0x22, 0x23, 0x24, 0x25,
        0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
        0x40, 0x41
    };
    uint8_t payloadDescFirstPkt[] =
    {
        /* X = 1, N = 1, S = 1. */
        0xB0,
        /* I = 1. */
        0x80,
        /* Picture ID. */
        0x1A
    };
    uint8_t payloadDescOtherPkt[] =
    {
        /* X = 1, N = 1. */
        0xA0,
        /* I = 1. */
        0x80,
        /* Picture ID. */
        0x1A
    };

    memset( &( frame ),
            0,
            sizeof( VP8Frame_t ) );

    frame.frameProperties |= VP8_FRAME_PROP_NON_REF_FRAME;
    frame.frameProperties |= VP8_FRAME_PROP_PICTURE_ID_PRESENT;
    frame.pictureId = 0x1A;
    frame.pFrameData = &( frameData[ 0 ] );
    frame.frameDataLength = sizeof( frameData );

    result = VP8Packetizer_Init( &( ctx ),
                                 &( frame ) );

    TEST_ASSERT_EQUAL( VP8_RESULT_OK,
                       result );

    pkt.pPacketData = &( packetBuffer[ 0 ] );
    pkt.packetDataLength = PACKET_BUFFER_LENGTH;

    result = VP8Packetizer_GetPacket( &( ctx ),
                                      &( pkt ) );

    while( result != VP8_RESULT_NO_MORE_PACKETS )
    {
        TEST_ASSERT_EQUAL( VP8_RESULT_OK,
                           result );

        if( frameDataIndex == 0 )
        {
            TEST_ASSERT_EQUAL_UINT8_ARRAY( &( payloadDescFirstPkt[ 0 ] ),
                                           &( pkt.pPacketData[ 0 ] ),
                                           3 );
        }
        else
        {
            TEST_ASSERT_EQUAL_UINT8_ARRAY( &( payloadDescOtherPkt[ 0 ] ),
                                           &( pkt.pPacketData[ 0 ] ),
                                           3 );
        }

        TEST_ASSERT_EQUAL_UINT8_ARRAY( &( frameData[ frameDataIndex ] ),
                                       &( pkt.pPacketData[ 3 ] ),
                                       pkt.packetDataLength - 3 );
        frameDataIndex += ( pkt.packetDataLength - 3 );

        pkt.pPacketData = &( packetBuffer[ 0 ] );
        pkt.packetDataLength = PACKET_BUFFER_LENGTH;

        result = VP8Packetizer_GetPacket( &( ctx ),
                                          &( pkt ) );
    }

    TEST_ASSERT_EQUAL( sizeof( frameData ),
                       frameDataIndex );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate VP8 packetization happy path containing TID.
 */
void test_VP8_Packetizer_TID( void )
{
    VP8Result_t result;
    VP8PacketizerContext_t ctx;
    VP8Frame_t frame;
    VP8Packet_t pkt;
    size_t frameDataIndex = 0;
    uint8_t frameData[] =
    {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
        0x20, 0x21, 0x22, 0x23, 0x24, 0x25,
        0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
        0x40, 0x41
    };
    uint8_t payloadDescFirstPkt[] =
    {
        /* X = 1, N = 1, S = 1. */
        0xB0,
        /* T = 1. */
        0x20,
        /* TID = 3. */
        0xC0
    };
    uint8_t payloadDescOtherPkt[] =
    {
        /* X = 1, N = 1. */
        0xA0,
        /* T = 1. */
        0x20,
        /* TID = 3. */
        0xC0
    };

    memset( &( frame ),
            0,
            sizeof( VP8Frame_t ) );

    frame.frameProperties |= VP8_FRAME_PROP_NON_REF_FRAME;
    frame.frameProperties |= VP8_FRAME_PROP_TID_PRESENT;
    frame.tid = 3;
    frame.pFrameData = &( frameData[ 0 ] );
    frame.frameDataLength = sizeof( frameData );

    result = VP8Packetizer_Init( &( ctx ),
                                 &( frame ) );

    TEST_ASSERT_EQUAL( VP8_RESULT_OK,
                       result );

    pkt.pPacketData = &( packetBuffer[ 0 ] );
    pkt.packetDataLength = PACKET_BUFFER_LENGTH;

    result = VP8Packetizer_GetPacket( &( ctx ),
                                      &( pkt ) );

    while( result != VP8_RESULT_NO_MORE_PACKETS )
    {
        TEST_ASSERT_EQUAL( VP8_RESULT_OK,
                           result );

        if( frameDataIndex == 0 )
        {
            TEST_ASSERT_EQUAL_UINT8_ARRAY( &( payloadDescFirstPkt[ 0 ] ),
                                           &( pkt.pPacketData[ 0 ] ),
                                           3 );
        }
        else
        {
            TEST_ASSERT_EQUAL_UINT8_ARRAY( &( payloadDescOtherPkt[ 0 ] ),
                                           &( pkt.pPacketData[ 0 ] ),
                                           3 );
        }

        TEST_ASSERT_EQUAL_UINT8_ARRAY( &( frameData[ frameDataIndex ] ),
                                       &( pkt.pPacketData[ 3 ] ),
                                       pkt.packetDataLength - 3 );
        frameDataIndex += ( pkt.packetDataLength - 3 );

        pkt.pPacketData = &( packetBuffer[ 0 ] );
        pkt.packetDataLength = PACKET_BUFFER_LENGTH;

        result = VP8Packetizer_GetPacket( &( ctx ),
                                          &( pkt ) );
    }

    TEST_ASSERT_EQUAL( sizeof( frameData ),
                       frameDataIndex );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate VP8 packetization happy path containing TL0PICIDX.
 */
void test_VP8_Packetizer_TL0PICIDX( void )
{
    VP8Result_t result;
    VP8PacketizerContext_t ctx;
    VP8Frame_t frame;
    VP8Packet_t pkt;
    size_t frameDataIndex = 0;
    uint8_t frameData[] =
    {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
        0x20, 0x21, 0x22, 0x23, 0x24, 0x25,
        0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
        0x40, 0x41
    };
    uint8_t payloadDescFirstPkt[] =
    {
        /* X = 1, N = 1, S = 1. */
        0xB0,
        /* L = 1. */
        0x40,
        /* TL0PICIDX. */
        0xAB
    };
    uint8_t payloadDescOtherPkt[] =
    {
        /* X = 1, N = 1. */
        0xA0,
        /* L = 1. */
        0x40,
        /* TL0PICIDX. */
        0xAB
    };

    memset( &( frame ),
            0,
            sizeof( VP8Frame_t ) );

    frame.frameProperties |= VP8_FRAME_PROP_NON_REF_FRAME;
    frame.frameProperties |= VP8_FRAME_PROP_TL0PICIDX_PRESENT;
    frame.tl0PicIndex = 0xAB;
    frame.pFrameData = &( frameData[ 0 ] );
    frame.frameDataLength = sizeof( frameData );

    result = VP8Packetizer_Init( &( ctx ),
                                 &( frame ) );

    TEST_ASSERT_EQUAL( VP8_RESULT_OK,
                       result );

    pkt.pPacketData = &( packetBuffer[ 0 ] );
    pkt.packetDataLength = PACKET_BUFFER_LENGTH;

    result = VP8Packetizer_GetPacket( &( ctx ),
                                      &( pkt ) );

    while( result != VP8_RESULT_NO_MORE_PACKETS )
    {
        TEST_ASSERT_EQUAL( VP8_RESULT_OK,
                           result );

        if( frameDataIndex == 0 )
        {
            TEST_ASSERT_EQUAL_UINT8_ARRAY( &( payloadDescFirstPkt[ 0 ] ),
                                           &( pkt.pPacketData[ 0 ] ),
                                           3 );
        }
        else
        {
            TEST_ASSERT_EQUAL_UINT8_ARRAY( &( payloadDescOtherPkt[ 0 ] ),
                                           &( pkt.pPacketData[ 0 ] ),
                                           3 );
        }

        TEST_ASSERT_EQUAL_UINT8_ARRAY( &( frameData[ frameDataIndex ] ),
                                       &( pkt.pPacketData[ 3 ] ),
                                       pkt.packetDataLength - 3 );
        frameDataIndex += ( pkt.packetDataLength - 3 );

        pkt.pPacketData = &( packetBuffer[ 0 ] );
        pkt.packetDataLength = PACKET_BUFFER_LENGTH;

        result = VP8Packetizer_GetPacket( &( ctx ),
                                          &( pkt ) );
    }

    TEST_ASSERT_EQUAL( sizeof( frameData ),
                       frameDataIndex );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate VP8 packetization happy path containing KeyIndex.
 */
void test_VP8_Packetizer_KeyIndex( void )
{
    VP8Result_t result;
    VP8PacketizerContext_t ctx;
    VP8Frame_t frame;
    VP8Packet_t pkt;
    size_t frameDataIndex = 0;
    uint8_t frameData[] =
    {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
        0x20, 0x21, 0x22, 0x23, 0x24, 0x25,
        0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
        0x40, 0x41
    };
    uint8_t payloadDescFirstPkt[] =
    {
        /* X = 1, N = 1, S = 1. */
        0xB0,
        /* K = 1. */
        0x10,
        /* KEYIDX = 0xA. */
        0xA
    };
    uint8_t payloadDescOtherPkt[] =
    {
        /* X = 1, N = 1. */
        0xA0,
        /* K = 1. */
        0x10,
        /* KEYIDX = 0xA. */
        0xA
    };

    memset( &( frame ),
            0,
            sizeof( VP8Frame_t ) );

    frame.frameProperties |= VP8_FRAME_PROP_NON_REF_FRAME;
    frame.frameProperties |= VP8_FRAME_PROP_KEYIDX_PRESENT;
    frame.keyIndex = 0xA;
    frame.pFrameData = &( frameData[ 0 ] );
    frame.frameDataLength = sizeof( frameData );

    result = VP8Packetizer_Init( &( ctx ),
                                 &( frame ) );

    TEST_ASSERT_EQUAL( VP8_RESULT_OK,
                       result );

    pkt.pPacketData = &( packetBuffer[ 0 ] );
    pkt.packetDataLength = PACKET_BUFFER_LENGTH;

    result = VP8Packetizer_GetPacket( &( ctx ),
                                      &( pkt ) );

    while( result != VP8_RESULT_NO_MORE_PACKETS )
    {
        TEST_ASSERT_EQUAL( VP8_RESULT_OK,
                           result );

        if( frameDataIndex == 0 )
        {
            TEST_ASSERT_EQUAL_UINT8_ARRAY( &( payloadDescFirstPkt[ 0 ] ),
                                           &( pkt.pPacketData[ 0 ] ),
                                           3 );
        }
        else
        {
            TEST_ASSERT_EQUAL_UINT8_ARRAY( &( payloadDescOtherPkt[ 0 ] ),
                                           &( pkt.pPacketData[ 0 ] ),
                                           3 );
        }

        TEST_ASSERT_EQUAL_UINT8_ARRAY( &( frameData[ frameDataIndex ] ),
                                       &( pkt.pPacketData[ 3 ] ),
                                       pkt.packetDataLength - 3 );
        frameDataIndex += ( pkt.packetDataLength - 3 );

        pkt.pPacketData = &( packetBuffer[ 0 ] );
        pkt.packetDataLength = PACKET_BUFFER_LENGTH;

        result = VP8Packetizer_GetPacket( &( ctx ),
                                          &( pkt ) );
    }

    TEST_ASSERT_EQUAL( sizeof( frameData ),
                       frameDataIndex );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate VP8 packetization happy path with all properties.
 */
void test_VP8_Packetizer_AllProperties( void )
{
    VP8Result_t result;
    VP8PacketizerContext_t ctx;
    VP8Frame_t frame;
    VP8Packet_t pkt;
    size_t frameDataIndex = 0;
    uint8_t frameData[] =
    {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
        0x20, 0x21, 0x22, 0x23, 0x24, 0x25,
        0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
        0x40, 0x41
    };
    uint8_t payloadDescFirstPkt[] =
    {
        /* X = 1, N = 1, S = 1. */
        0xB0,
        /* I = 1. L = 1, T = 1, K = 1. */
        0xF0,
        /* Picture ID = 0x7ACD. */
        0xFA, 0xCD,
        /* TL0PICIDX. */
        0xAB,
        /* TID = 3, Y = 1, KEYIDX = 10. */
        0xEA
    };
    uint8_t payloadDescOtherPkt[] =
    {
        /* X = 1, N = 1. */
        0xA0,
        /* I = 1. L = 1, T = 1, K = 1. */
        0xF0,
        /* Picture ID = 0x7ACD. */
        0xFA, 0xCD,
        /* TL0PICIDX. */
        0xAB,
        /* TID = 3, Y = 1, KEYIDX = 10. */
        0xEA
    };

    memset( &( frame ),
            0,
            sizeof( VP8Frame_t ) );

    frame.frameProperties |= VP8_FRAME_PROP_NON_REF_FRAME;
    frame.frameProperties |= VP8_FRAME_PROP_PICTURE_ID_PRESENT;
    frame.frameProperties |= VP8_FRAME_PROP_TL0PICIDX_PRESENT;
    frame.frameProperties |= VP8_FRAME_PROP_TID_PRESENT;
    frame.frameProperties |= VP8_FRAME_PROP_KEYIDX_PRESENT;
    frame.frameProperties |= VP8_FRAME_PROP_DEPENDS_ON_BASE_ONLY;
    frame.pictureId = 0x7ACD;
    frame.tl0PicIndex = 0xAB;
    frame.tid = 3;
    frame.keyIndex = 10;
    frame.pFrameData = &( frameData[ 0 ] );
    frame.frameDataLength = sizeof( frameData );

    result = VP8Packetizer_Init( &( ctx ),
                                 &( frame ) );

    TEST_ASSERT_EQUAL( VP8_RESULT_OK,
                       result );

    pkt.pPacketData = &( packetBuffer[ 0 ] );
    pkt.packetDataLength = PACKET_BUFFER_LENGTH;

    result = VP8Packetizer_GetPacket( &( ctx ),
                                      &( pkt ) );

    while( result != VP8_RESULT_NO_MORE_PACKETS )
    {
        TEST_ASSERT_EQUAL( VP8_RESULT_OK,
                           result );

        if( frameDataIndex == 0 )
        {
            TEST_ASSERT_EQUAL_UINT8_ARRAY( &( payloadDescFirstPkt[ 0 ] ),
                                           &( pkt.pPacketData[ 0 ] ),
                                           6 );
        }
        else
        {
            TEST_ASSERT_EQUAL_UINT8_ARRAY( &( payloadDescOtherPkt[ 0 ] ),
                                           &( pkt.pPacketData[ 0 ] ),
                                           6 );
        }

        TEST_ASSERT_EQUAL_UINT8_ARRAY( &( frameData[ frameDataIndex ] ),
                                       &( pkt.pPacketData[ 6 ] ),
                                       pkt.packetDataLength - 6 );
        frameDataIndex += ( pkt.packetDataLength - 6 );

        pkt.pPacketData = &( packetBuffer[ 0 ] );
        pkt.packetDataLength = PACKET_BUFFER_LENGTH;

        result = VP8Packetizer_GetPacket( &( ctx ),
                                          &( pkt ) );
    }

    TEST_ASSERT_EQUAL( sizeof( frameData ),
                       frameDataIndex );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate VP8_Packetizer_Init incase of bad parameters.
 */
void test_VP8_Packetizer_Init_BadParams( void )
{
    VP8Result_t result;
    VP8PacketizerContext_t ctx;
    VP8Frame_t frame;
    uint8_t frameData[] =
    {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
        0x20, 0x21, 0x22, 0x23, 0x24, 0x25,
        0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
        0x40, 0x41
    };

    memset( &( frame ),
            0,
            sizeof( VP8Frame_t ) );

    frame.pFrameData = &( frameData[ 0 ] );
    frame.frameDataLength = sizeof( frameData );

    result = VP8Packetizer_Init( NULL,
                                 &( frame ) );

    TEST_ASSERT_EQUAL( VP8_RESULT_BAD_PARAM,
                       result );

    result = VP8Packetizer_Init( &( ctx ),
                                 NULL );

    TEST_ASSERT_EQUAL( VP8_RESULT_BAD_PARAM,
                       result );

    frame.pFrameData = NULL;
    frame.frameDataLength = sizeof( frameData );

    result = VP8Packetizer_Init( &( ctx ),
                                 &( frame ) );

    TEST_ASSERT_EQUAL( VP8_RESULT_BAD_PARAM,
                       result );

    frame.pFrameData = &( frameData[ 0 ] );
    frame.frameDataLength = 0;

    result = VP8Packetizer_Init( &( ctx ),
                                 &( frame ) );

    TEST_ASSERT_EQUAL( VP8_RESULT_BAD_PARAM,
                       result );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate VP8_Packetizer_GetPacket in case of bad parameters.
 */
void test_VP8_Packetizer_GetPacket_BadParams( void )
{
    VP8Result_t result;
    VP8PacketizerContext_t ctx = { 0 };
    VP8Packet_t pkt;

    pkt.pPacketData = &( packetBuffer[ 0 ] );
    pkt.packetDataLength = PACKET_BUFFER_LENGTH;

    result = VP8Packetizer_GetPacket( NULL,
                                      &( pkt ) );

    TEST_ASSERT_EQUAL( VP8_RESULT_BAD_PARAM,
                       result );

    result = VP8Packetizer_GetPacket( &( ctx ),
                                      NULL );

    TEST_ASSERT_EQUAL( VP8_RESULT_BAD_PARAM,
                       result );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate VP8_Packetizer_GetPacket incase of out of memory.
 */
void test_VP8_Packetizer_GetPacket_OutOfMemory( void )
{
    VP8Result_t result;
    VP8PacketizerContext_t ctx;
    VP8Frame_t frame;
    VP8Packet_t pkt;
    uint8_t frameData[] =
    {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
        0x20, 0x21, 0x22, 0x23, 0x24, 0x25,
        0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
        0x40, 0x41
    };

    memset( &( frame ),
            0,
            sizeof( VP8Frame_t ) );

    /* Set the frame properties such that the size of payload descriptor is 3. */
    frame.frameProperties |= VP8_FRAME_PROP_NON_REF_FRAME;
    frame.frameProperties |= VP8_FRAME_PROP_KEYIDX_PRESENT;
    frame.keyIndex = 0xA;
    frame.pFrameData = &( frameData[ 0 ] );
    frame.frameDataLength = sizeof( frameData );

    result = VP8Packetizer_Init( &( ctx ),
                                 &( frame ) );

    TEST_ASSERT_EQUAL( VP8_RESULT_OK,
                       result );

    pkt.pPacketData = &( packetBuffer[ 0 ] );
    pkt.packetDataLength = 2; /* Packet buffer smaller than the payload descriptor. */

    result = VP8Packetizer_GetPacket( &( ctx ),
                                      &( pkt ) );

    TEST_ASSERT_EQUAL( VP8_RESULT_OUT_OF_MEMORY,
                       result );
}

/* ==============================  Test Cases for Depacketization ============================== */

/**
 * @brief Validate VP8 depacketization happy path.
 */
void test_VP8_Depacketizer_AllProperties( void )
{
    VP8DepacketizerContext_t ctx;
    VP8Result_t result;
    VP8Packet_t pkt;
    VP8Frame_t frame;
    uint8_t packetData1[] =
    {
        /* X = 1, N = 1, S = 1. */
        0xB0,
        /* I = 1. L = 1, T = 1, K = 1. */
        0xF0,
        /* Picture ID = 0x7ACD. */
        0xFA, 0xCD,
        /* TL0PICIDX. */
        0xAB,
        /* TID = 3, Y = 1, KEYIDX = 10. */
        0xEA,
        /* Payload. */
        0x00, 0x01, 0x02, 0x03
    };
    uint8_t packetData2[] =
    {
        /* X = 1, N = 1. */
        0xA0,
        /* I = 1. L = 1, T = 1, K = 1. */
        0xF0,
        /* Picture ID = 0x7ACD. */
        0xFA, 0xCD,
        /* TL0PICIDX. */
        0xAB,
        /* TID = 3, Y = 1, KEYIDX = 10. */
        0xEA,
        /* Payload. */
        0x04, 0x05, 0x10, 0x11
    };
    uint8_t packetData3[] =
    {
        /* Payload descriptor same as packetData2. */
        0xA0, 0xF0, 0xFA, 0xCD, 0xAB, 0xEA,
        /* Payload. */
        0x12, 0x13, 0x14, 0x15
    };
    uint8_t packetData4[] =
    {
        /* Payload descriptor same as packetData2. */
        0xA0, 0xF0, 0xFA, 0xCD, 0xAB, 0xEA,
        /* Payload. */
        0x20, 0x21, 0x22, 0x23
    };
    uint8_t packetData5[] =
    {
        /* Payload descriptor same as packetData2. */
        0xA0, 0xF0, 0xFA, 0xCD, 0xAB, 0xEA,
        /* Payload. */
        0x24, 0x25, 0x30, 0x31
    };
    uint8_t packetData6[] =
    {
        /* Payload descriptor same as packetData2. */
        0xA0, 0xF0, 0xFA, 0xCD, 0xAB, 0xEA,
        /* Payload. */
        0x32, 0x33, 0x34, 0x35
    };
    uint8_t packetData7[] =
    {
        /* Payload descriptor same as packetData2. */
        0xA0, 0xF0, 0xFA, 0xCD, 0xAB, 0xEA,
        /* Payload. */
        0x40, 0x41
    };
    uint8_t decodedFrameData[] =
    {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
        0x20, 0x21, 0x22, 0x23, 0x24, 0x25,
        0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
        0x40, 0x41
    };

    result = VP8Depacketizer_Init( &( ctx ),
                                   &( packetsArray[ 0 ] ),
                                   VP8_PACKETS_ARR_LEN );

    TEST_ASSERT_EQUAL( VP8_RESULT_OK,
                       result );

    pkt.pPacketData = &( packetData1[ 0 ] );
    pkt.packetDataLength = sizeof( packetData1 );

    result = VP8Depacketizer_AddPacket( &( ctx ),
                                        &( pkt ) );

    TEST_ASSERT_EQUAL( VP8_RESULT_OK,
                       result );

    pkt.pPacketData = &( packetData2[ 0 ] );
    pkt.packetDataLength = sizeof( packetData2 );

    result = VP8Depacketizer_AddPacket( &( ctx ),
                                        &( pkt ) );

    TEST_ASSERT_EQUAL( VP8_RESULT_OK,
                       result );

    pkt.pPacketData = &( packetData3[ 0 ] );
    pkt.packetDataLength = sizeof( packetData3 );

    result = VP8Depacketizer_AddPacket( &( ctx ),
                                        &( pkt ) );

    TEST_ASSERT_EQUAL( VP8_RESULT_OK,
                       result );

    pkt.pPacketData = &( packetData4[ 0 ] );
    pkt.packetDataLength = sizeof( packetData4 );

    result = VP8Depacketizer_AddPacket( &( ctx ),
                                        &( pkt ) );

    TEST_ASSERT_EQUAL( VP8_RESULT_OK,
                       result );

    pkt.pPacketData = &( packetData5[ 0 ] );
    pkt.packetDataLength = sizeof( packetData5 );

    result = VP8Depacketizer_AddPacket( &( ctx ),
                                        &( pkt ) );

    TEST_ASSERT_EQUAL( VP8_RESULT_OK,
                       result );

    pkt.pPacketData = &( packetData6[ 0 ] );
    pkt.packetDataLength = sizeof( packetData6 );

    result = VP8Depacketizer_AddPacket( &( ctx ),
                                        &( pkt ) );

    TEST_ASSERT_EQUAL( VP8_RESULT_OK,
                       result );

    pkt.pPacketData = &( packetData7[ 0 ] );
    pkt.packetDataLength = sizeof( packetData7 );

    result = VP8Depacketizer_AddPacket( &( ctx ),
                                        &( pkt ) );

    TEST_ASSERT_EQUAL( VP8_RESULT_OK,
                       result );

    frame.pFrameData = &( frameBuffer[ 0 ] );
    frame.frameDataLength = VP8_FRAME_BUF_LEN;

    result = VP8Depacketizer_GetFrame( &( ctx ),
                                       &( frame ) );

    TEST_ASSERT_EQUAL( VP8_RESULT_OK,
                       result );

    TEST_ASSERT_EQUAL( sizeof( decodedFrameData ),
                       frame.frameDataLength );

    TEST_ASSERT_EQUAL_UINT8_ARRAY( &( decodedFrameData[ 0 ] ),
                                   &( frame.pFrameData[ 0 ] ),
                                   frame.frameDataLength );

    TEST_ASSERT_EQUAL( 0x7ACD,
                       frame.pictureId );

    TEST_ASSERT_EQUAL( 0xAB,
                       frame.tl0PicIndex );

    TEST_ASSERT_EQUAL( 3,
                       frame.tid );

    TEST_ASSERT_EQUAL( 10,
                       frame.keyIndex );

    TEST_ASSERT_EQUAL( ( VP8_FRAME_PROP_NON_REF_FRAME |
                         VP8_FRAME_PROP_PICTURE_ID_PRESENT |
                         VP8_FRAME_PROP_TL0PICIDX_PRESENT |
                         VP8_FRAME_PROP_TID_PRESENT |
                         VP8_FRAME_PROP_KEYIDX_PRESENT |
                         VP8_FRAME_PROP_DEPENDS_ON_BASE_ONLY ),
                       frame.frameProperties );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate VP8 depacketization happy path containing TL0PICIDX.
 */
void test_VP8_Depacketizer_TL0PICIDX( void )
{
    VP8DepacketizerContext_t ctx;
    VP8Result_t result;
    VP8Packet_t pkt;
    VP8Frame_t frame;
    uint8_t packetData1[] =
    {
        /* X = 1, N = 1, S = 1. */
        0xB0,
        /* L = 1. */
        0x40,
        /* TL0PICIDX. */
        0xAB,
        /* Payload. */
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x10
    };
    uint8_t packetData2[] =
    {
        /* X = 1, N = 1. */
        0xA0,
        /* L = 1. */
        0x40,
        /* TL0PICIDX. */
        0xAB,
        /* Payload. */
        0x11, 0x12, 0x13, 0x14, 0x15, 0x20, 0x21
    };
    uint8_t packetData3[] =
    {
        /* Payload descriptor same as packetData2. */
        0xA0, 0x40, 0xAB,
        /* Payload. */
        0x22, 0x23, 0x24, 0x25, 0x30, 0x31, 0x32
    };
    uint8_t packetData4[] =
    {
        /* Payload descriptor same as packetData2. */
        0xA0, 0x40, 0xAB,
        /* Payload. */
        0x33, 0x34, 0x35, 0x40, 0x41
    };

    uint8_t decodedFrameData[] =
    {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
        0x20, 0x21, 0x22, 0x23, 0x24, 0x25,
        0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
        0x40, 0x41
    };

    result = VP8Depacketizer_Init( &( ctx ),
                                   &( packetsArray[ 0 ] ),
                                   VP8_PACKETS_ARR_LEN );

    TEST_ASSERT_EQUAL( VP8_RESULT_OK,
                       result );

    pkt.pPacketData = &( packetData1[ 0 ] );
    pkt.packetDataLength = sizeof( packetData1 );

    result = VP8Depacketizer_AddPacket( &( ctx ),
                                        &( pkt ) );

    TEST_ASSERT_EQUAL( VP8_RESULT_OK,
                       result );

    pkt.pPacketData = &( packetData2[ 0 ] );
    pkt.packetDataLength = sizeof( packetData2 );

    result = VP8Depacketizer_AddPacket( &( ctx ),
                                        &( pkt ) );

    TEST_ASSERT_EQUAL( VP8_RESULT_OK,
                       result );

    pkt.pPacketData = &( packetData3[ 0 ] );
    pkt.packetDataLength = sizeof( packetData3 );

    result = VP8Depacketizer_AddPacket( &( ctx ),
                                        &( pkt ) );

    TEST_ASSERT_EQUAL( VP8_RESULT_OK,
                       result );

    pkt.pPacketData = &( packetData4[ 0 ] );
    pkt.packetDataLength = sizeof( packetData4 );

    result = VP8Depacketizer_AddPacket( &( ctx ),
                                        &( pkt ) );

    TEST_ASSERT_EQUAL( VP8_RESULT_OK,
                       result );

    frame.pFrameData = &( frameBuffer[ 0 ] );
    frame.frameDataLength = VP8_FRAME_BUF_LEN;

    result = VP8Depacketizer_GetFrame( &( ctx ),
                                       &( frame ) );

    TEST_ASSERT_EQUAL( VP8_RESULT_OK,
                       result );

    TEST_ASSERT_EQUAL( sizeof( decodedFrameData ),
                       frame.frameDataLength );

    TEST_ASSERT_EQUAL_UINT8_ARRAY( &( decodedFrameData[ 0 ] ),
                                   &( frame.pFrameData[ 0 ] ),
                                   frame.frameDataLength );

    TEST_ASSERT_EQUAL( 0xAB,
                       frame.tl0PicIndex );

    TEST_ASSERT_EQUAL( ( VP8_FRAME_PROP_NON_REF_FRAME |
                         VP8_FRAME_PROP_TL0PICIDX_PRESENT ),
                       frame.frameProperties );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate VP8 depacketization happy path containing Picture ID.
 */
void test_VP8_Depacketizer_PictureID( void )
{
    VP8DepacketizerContext_t ctx;
    VP8Result_t result;
    VP8Packet_t pkt;
    VP8Frame_t frame;
    uint8_t packetData1[] =
    {
        /* X = 1, S = 1. */
        0x90,
        /* I = 1. */
        0x80,
        /* Picture ID. */
        0x0B,
        /* Payload. */
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x10
    };
    uint8_t packetData2[] =
    {
        /* X = 1. */
        0x80,
        /* I = 1. */
        0x80,
        /* Picture ID. */
        0x0B,
        /* Payload. */
        0x11, 0x12
    };

    uint8_t decodedFrameData[] =
    {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
        0x10, 0x11, 0x12
    };

    result = VP8Depacketizer_Init( &( ctx ),
                                   &( packetsArray[ 0 ] ),
                                   VP8_PACKETS_ARR_LEN );

    TEST_ASSERT_EQUAL( VP8_RESULT_OK,
                       result );

    pkt.pPacketData = &( packetData1[ 0 ] );
    pkt.packetDataLength = sizeof( packetData1 );

    result = VP8Depacketizer_AddPacket( &( ctx ),
                                        &( pkt ) );

    TEST_ASSERT_EQUAL( VP8_RESULT_OK,
                       result );

    pkt.pPacketData = &( packetData2[ 0 ] );
    pkt.packetDataLength = sizeof( packetData2 );

    result = VP8Depacketizer_AddPacket( &( ctx ),
                                        &( pkt ) );

    TEST_ASSERT_EQUAL( VP8_RESULT_OK,
                       result );

    frame.pFrameData = &( frameBuffer[ 0 ] );
    frame.frameDataLength = VP8_FRAME_BUF_LEN;

    result = VP8Depacketizer_GetFrame( &( ctx ),
                                       &( frame ) );

    TEST_ASSERT_EQUAL( VP8_RESULT_OK,
                       result );

    TEST_ASSERT_EQUAL( sizeof( decodedFrameData ),
                       frame.frameDataLength );

    TEST_ASSERT_EQUAL_UINT8_ARRAY( &( decodedFrameData[ 0 ] ),
                                   &( frame.pFrameData[ 0 ] ),
                                   frame.frameDataLength );

    TEST_ASSERT_EQUAL( 0x0B,
                       frame.pictureId );

    TEST_ASSERT_EQUAL( VP8_FRAME_PROP_PICTURE_ID_PRESENT,
                       frame.frameProperties );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate VP8 depacketization happy path containing TID.
 */
void test_VP8_Depacketizer_TID( void )
{
    VP8DepacketizerContext_t ctx;
    VP8Result_t result;
    VP8Packet_t pkt;
    VP8Frame_t frame;
    uint8_t packetData1[] =
    {
        /* X = 1, N = 1, S = 1. */
        0xB0,
        /* T = 1. */
        0x20,
        /* TID = 3. */
        0xC0,
        /* Payload. */
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x10
    };
    uint8_t packetData2[] =
    {
        /* X = 1, N = 1. */
        0xA0,
        /* T = 1. */
        0x20,
        /* TID = 3. */
        0xC0,
        /* Payload. */
        0x11, 0x12
    };

    uint8_t decodedFrameData[] =
    {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
        0x10, 0x11, 0x12
    };

    result = VP8Depacketizer_Init( &( ctx ),
                                   &( packetsArray[ 0 ] ),
                                   VP8_PACKETS_ARR_LEN );

    TEST_ASSERT_EQUAL( VP8_RESULT_OK,
                       result );

    pkt.pPacketData = &( packetData1[ 0 ] );
    pkt.packetDataLength = sizeof( packetData1 );

    result = VP8Depacketizer_AddPacket( &( ctx ),
                                        &( pkt ) );

    TEST_ASSERT_EQUAL( VP8_RESULT_OK,
                       result );

    pkt.pPacketData = &( packetData2[ 0 ] );
    pkt.packetDataLength = sizeof( packetData2 );

    result = VP8Depacketizer_AddPacket( &( ctx ),
                                        &( pkt ) );

    TEST_ASSERT_EQUAL( VP8_RESULT_OK,
                       result );

    frame.pFrameData = &( frameBuffer[ 0 ] );
    frame.frameDataLength = VP8_FRAME_BUF_LEN;

    result = VP8Depacketizer_GetFrame( &( ctx ),
                                       &( frame ) );

    TEST_ASSERT_EQUAL( VP8_RESULT_OK,
                       result );

    TEST_ASSERT_EQUAL( sizeof( decodedFrameData ),
                       frame.frameDataLength );

    TEST_ASSERT_EQUAL_UINT8_ARRAY( &( decodedFrameData[ 0 ] ),
                                   &( frame.pFrameData[ 0 ] ),
                                   frame.frameDataLength );

    TEST_ASSERT_EQUAL( 0x3,
                       frame.tid );

    TEST_ASSERT_EQUAL( ( VP8_FRAME_PROP_NON_REF_FRAME |
                         VP8_FRAME_PROP_TID_PRESENT ),
                       frame.frameProperties );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate VP8 depacketization happy path containing KeyIndex.
 */
void test_VP8_Depacketizer_KeyIndex( void )
{
    VP8DepacketizerContext_t ctx;
    VP8Result_t result;
    VP8Packet_t pkt;
    VP8Frame_t frame;
    uint8_t packetData1[] =
    {
        /* X = 1, N = 1, S = 1. */
        0xB0,
        /* K = 1. */
        0x10,
        /* KEYIDX = 0xA. */
        0x0A,
        /* Payload. */
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x10
    };
    uint8_t packetData2[] =
    {
        /* X = 1, N = 1. */
        0xA0,
        /* K = 1. */
        0x10,
        /* KEYIDX = 0xA. */
        0x0A,
        /* Payload. */
        0x11, 0x12
    };

    uint8_t decodedFrameData[] =
    {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
        0x10, 0x11, 0x12
    };

    result = VP8Depacketizer_Init( &( ctx ),
                                   &( packetsArray[ 0 ] ),
                                   VP8_PACKETS_ARR_LEN );

    TEST_ASSERT_EQUAL( VP8_RESULT_OK,
                       result );

    pkt.pPacketData = &( packetData1[ 0 ] );
    pkt.packetDataLength = sizeof( packetData1 );

    result = VP8Depacketizer_AddPacket( &( ctx ),
                                        &( pkt ) );

    TEST_ASSERT_EQUAL( VP8_RESULT_OK,
                       result );

    pkt.pPacketData = &( packetData2[ 0 ] );
    pkt.packetDataLength = sizeof( packetData2 );

    result = VP8Depacketizer_AddPacket( &( ctx ),
                                        &( pkt ) );

    TEST_ASSERT_EQUAL( VP8_RESULT_OK,
                       result );

    frame.pFrameData = &( frameBuffer[ 0 ] );
    frame.frameDataLength = VP8_FRAME_BUF_LEN;

    result = VP8Depacketizer_GetFrame( &( ctx ),
                                       &( frame ) );

    TEST_ASSERT_EQUAL( VP8_RESULT_OK,
                       result );

    TEST_ASSERT_EQUAL( sizeof( decodedFrameData ),
                       frame.frameDataLength );

    TEST_ASSERT_EQUAL_UINT8_ARRAY( &( decodedFrameData[ 0 ] ),
                                   &( frame.pFrameData[ 0 ] ),
                                   frame.frameDataLength );

    TEST_ASSERT_EQUAL( 0xA,
                       frame.keyIndex );

    TEST_ASSERT_EQUAL( ( VP8_FRAME_PROP_NON_REF_FRAME |
                         VP8_FRAME_PROP_KEYIDX_PRESENT ),
                       frame.frameProperties );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate VP8 depacketization GetPacketProperties's happy path.
 */
void test_VP8_Depacketizer_GetPacketProperties( void )
{
    VP8Result_t result;
    VP8Packet_t pkt;
    uint32_t properties;
    uint8_t packetData[] =
    {
        /* X = 1, N = 1, S = 1. */
        0xB0,
        /* L = 1. */
        0x40,
        /* TL0PICIDX. */
        0xAB,
        /* Payload. */
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x10
    };

    pkt.pPacketData = &( packetData[ 0 ] );
    pkt.packetDataLength = sizeof( packetData );

    result = VP8Depacketizer_GetPacketProperties( pkt.pPacketData,
                                                  pkt.packetDataLength,
                                                  &( properties ) );

    TEST_ASSERT_EQUAL( VP8_RESULT_OK,
                       result );
    TEST_ASSERT_EQUAL( VP8_PACKET_PROP_START_PACKET,
                       properties );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate VP8 depacketization happy path with no extensions.
 */
void test_VP8_Depacketizer_NoExtensions( void )
{
    VP8DepacketizerContext_t ctx;
    VP8Result_t result;
    VP8Packet_t pkt;
    VP8Frame_t frame;
    uint8_t packetData[] =
    {
        /* N = 1, S = 1. */
        0x30,
        /* Payload. */
        0x01, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x10
    };
    uint8_t decodedFrameData[] = { 0x01, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x10 };

    result = VP8Depacketizer_Init( &( ctx ),
                                   &( packetsArray[ 0 ] ),
                                   VP8_PACKETS_ARR_LEN );

    TEST_ASSERT_EQUAL( VP8_RESULT_OK,
                       result );

    pkt.pPacketData = &( packetData[ 0 ] );
    pkt.packetDataLength = sizeof( packetData );

    result = VP8Depacketizer_AddPacket( &( ctx ),
                                        &( pkt ) );

    TEST_ASSERT_EQUAL( VP8_RESULT_OK,
                       result );

    frame.pFrameData = &( frameBuffer[ 0 ] );
    frame.frameDataLength = VP8_FRAME_BUF_LEN;

    result = VP8Depacketizer_GetFrame( &( ctx ),
                                       &( frame ) );

    TEST_ASSERT_EQUAL( VP8_RESULT_OK,
                       result );

    TEST_ASSERT_EQUAL( sizeof( decodedFrameData ),
                       frame.frameDataLength );

    TEST_ASSERT_EQUAL_UINT8_ARRAY( &( decodedFrameData[ 0 ] ),
                                   &( frame.pFrameData[ 0 ] ),
                                   frame.frameDataLength );

    TEST_ASSERT_EQUAL( VP8_FRAME_PROP_NON_REF_FRAME,
                       frame.frameProperties );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate VP8_Depacketizer_Init incase of bad parameters.
 */
void test_VP8_Depacketizer_Init_BadParams( void )
{
    VP8DepacketizerContext_t ctx;
    VP8Result_t result;

    result = VP8Depacketizer_Init( NULL,
                                   &( packetsArray[ 0 ] ),
                                   VP8_PACKETS_ARR_LEN );

    TEST_ASSERT_EQUAL( VP8_RESULT_BAD_PARAM,
                       result );

    result = VP8Depacketizer_Init( &( ctx ),
                                   NULL,
                                   VP8_PACKETS_ARR_LEN );

    TEST_ASSERT_EQUAL( VP8_RESULT_BAD_PARAM,
                       result );

    result = VP8Depacketizer_Init( &( ctx ),
                                   &( packetsArray[ 0 ] ),
                                   0 );

    TEST_ASSERT_EQUAL( VP8_RESULT_BAD_PARAM,
                       result );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate VP8_Depacketizer_AddPacket incase of bad parameters.
 */
void test_VP8_Depacketizer_AddPacket_BadParams( void )
{
    VP8DepacketizerContext_t ctx;
    VP8Result_t result;
    VP8Packet_t pkt;
    uint8_t packetData[] = { 0xB0, 0x40, 0xAB,
                             0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x10 };

    pkt.pPacketData = &( packetData[ 0 ] );
    pkt.packetDataLength = sizeof( packetData );

    result = VP8Depacketizer_AddPacket( NULL,
                                        &( pkt ) );

    TEST_ASSERT_EQUAL( VP8_RESULT_BAD_PARAM,
                       result );

    result = VP8Depacketizer_AddPacket( &( ctx ),
                                        NULL );

    TEST_ASSERT_EQUAL( VP8_RESULT_BAD_PARAM,
                       result );

    pkt.pPacketData = &( packetData[ 0 ] );
    pkt.packetDataLength = 0;

    result = VP8Depacketizer_AddPacket( &( ctx ),
                                        &( pkt ) );

    TEST_ASSERT_EQUAL( VP8_RESULT_BAD_PARAM,
                       result );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate VP8_Depacketizer_AddPacket incase of out of memory.
 */
void test_VP8_Depacketizer_AddPacket_OutOfMemory( void )
{
    VP8DepacketizerContext_t ctx;
    VP8Result_t result;
    VP8Packet_t pkt;
    uint8_t packetData1[] = { 0xB0, 0x40, 0xAB,
                              0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x10 };
    uint8_t packetData2[] = { 0xA0, 0x40, 0xAB,
                              0x11, 0x12, 0x13, 0x14, 0x15, 0x20, 0x21 };

    result = VP8Depacketizer_Init( &( ctx ),
                                   &( packetsArray[ 0 ] ),
                                   1 );

    TEST_ASSERT_EQUAL( VP8_RESULT_OK,
                       result );

    pkt.pPacketData = &( packetData1[ 0 ] );
    pkt.packetDataLength = sizeof( packetData1 );

    result = VP8Depacketizer_AddPacket( &( ctx ),
                                        &( pkt ) );

    TEST_ASSERT_EQUAL( VP8_RESULT_OK,
                       result );

    pkt.pPacketData = &( packetData2[ 0 ] );
    pkt.packetDataLength = sizeof( packetData2 );

    result = VP8Depacketizer_AddPacket( &( ctx ),
                                        &( pkt ) );

    TEST_ASSERT_EQUAL( VP8_RESULT_OUT_OF_MEMORY,
                       result );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate VP8_Depacketizer_GetFrame in case of bad parameters.
 */
void test_VP8_Depacketizer_GetFrame_BadParams( void )
{
    VP8DepacketizerContext_t ctx;
    VP8Result_t result;
    VP8Frame_t frame;

    frame.pFrameData = &( frameBuffer[ 0 ] );
    frame.frameDataLength = VP8_FRAME_BUF_LEN;

    result = VP8Depacketizer_GetFrame( NULL,
                                       &( frame ) );

    TEST_ASSERT_EQUAL( VP8_RESULT_BAD_PARAM,
                       result );

    result = VP8Depacketizer_GetFrame( &( ctx ),
                                       NULL );

    TEST_ASSERT_EQUAL( VP8_RESULT_BAD_PARAM,
                       result );

    frame.pFrameData = NULL;
    frame.frameDataLength = VP8_FRAME_BUF_LEN;

    result = VP8Depacketizer_GetFrame( &( ctx ),
                                       &( frame ) );

    TEST_ASSERT_EQUAL( VP8_RESULT_BAD_PARAM,
                       result );

    frame.pFrameData = &( frameBuffer[ 0 ] );
    frame.frameDataLength = 0;

    result = VP8Depacketizer_GetFrame( &( ctx ),
                                       &( frame ) );

    TEST_ASSERT_EQUAL( VP8_RESULT_BAD_PARAM,
                       result );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate VP8_Depacketizer_GetFrame in case of out of memory.
 */
void test_VP8_Depacketizer_GetFrame_OutOfMemory( void )
{
    VP8DepacketizerContext_t ctx;
    VP8Result_t result;
    VP8Packet_t pkt;
    VP8Frame_t frame;
    uint8_t packetData1[] = { 0xB0, 0xF0, 0xFA, 0xCD, 0xAB, 0xEA,
                              0x00, 0x01, 0x02, 0x03 };
    uint8_t packetData2[] = { 0xA0, 0xF0, 0xFA, 0xCD, 0xAB, 0xEA,
                              0x04, 0x05, 0x10, 0x11 };

    result = VP8Depacketizer_Init( &( ctx ),
                                   &( packetsArray[ 0 ] ),
                                   VP8_PACKETS_ARR_LEN );

    TEST_ASSERT_EQUAL( VP8_RESULT_OK,
                       result );

    pkt.pPacketData = &( packetData1[ 0 ] );
    pkt.packetDataLength = sizeof( packetData1 );

    result = VP8Depacketizer_AddPacket( &( ctx ),
                                        &( pkt ) );

    TEST_ASSERT_EQUAL( VP8_RESULT_OK,
                       result );

    pkt.pPacketData = &( packetData2[ 0 ] );
    pkt.packetDataLength = sizeof( packetData2 );

    result = VP8Depacketizer_AddPacket( &( ctx ),
                                        &( pkt ) );

    TEST_ASSERT_EQUAL( VP8_RESULT_OK,
                       result );

    frame.pFrameData = &( frameBuffer[ 0 ] );
    frame.frameDataLength = 1; /* Frame buffer too small. */

    result = VP8Depacketizer_GetFrame( &( ctx ),
                                       &( frame ) );

    TEST_ASSERT_EQUAL( VP8_RESULT_OUT_OF_MEMORY,
                       result );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate VP8_Depacketizer_GetFrame in case of malformed packet.
 */
void test_VP8_Depacketizer_GetFrame_MalformedPacket( void )
{
    VP8DepacketizerContext_t ctx;
    VP8Result_t result;
    VP8Packet_t pkt;
    VP8Frame_t frame;
    uint8_t packetData1[] =
    {
        /* X = 1, N = 1, S = 1. */
        0xB0,
        /* I = 1, L = 1, T = 1, K = 1. */
        0xF0
        /* Incomplete packet. */
    };
    uint8_t packetData2[] =
    {
        /* X = 1, N = 1. */
        0xA0,
        /* I = 1, L = 1, T = 1, K = 1. */
        0xF0,
        /* Picture ID = 0x7ACD. */
        0xFA, 0xCD,
        /* TL0PICIDX. */
        0xAB,
        /* TID = 3, Y = 1, KEYIDX = 10. */
        0xEA,
        /* Payload. */
        0x04, 0x05, 0x10, 0x11
    };

    result = VP8Depacketizer_Init( &( ctx ),
                                   &( packetsArray[ 0 ] ),
                                   VP8_PACKETS_ARR_LEN );

    TEST_ASSERT_EQUAL( VP8_RESULT_OK,
                       result );

    pkt.pPacketData = &( packetData1[ 0 ] );
    pkt.packetDataLength = sizeof( packetData1 );

    result = VP8Depacketizer_AddPacket( &( ctx ),
                                        &( pkt ) );

    TEST_ASSERT_EQUAL( VP8_RESULT_OK,
                       result );

    pkt.pPacketData = &( packetData2[ 0 ] );
    pkt.packetDataLength = sizeof( packetData2 );

    result = VP8Depacketizer_AddPacket( &( ctx ),
                                        &( pkt ) );

    TEST_ASSERT_EQUAL( VP8_RESULT_OK,
                       result );

    frame.pFrameData = &( frameBuffer[ 0 ] );
    frame.frameDataLength = VP8_FRAME_BUF_LEN;

    result = VP8Depacketizer_GetFrame( &( ctx ),
                                       &( frame ) );

    TEST_ASSERT_EQUAL( VP8_MALFORMED_PACKET,
                       result );
}
/*-----------------------------------------------------------*/

/**
 * @brief Validate VP8_Depacketizer_GetPacketProperties incase of bad parameters.
 */

void test_VP8_Depacketizer_GetPacketProperties_BadParams( void )
{
    VP8Result_t result;
    VP8Packet_t pkt;
    uint32_t properties;
    uint8_t packetData[] = { 0xB0, 0x40, 0xAB,
                             0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x10 };

    pkt.pPacketData = NULL;
    pkt.packetDataLength = sizeof( packetData );

    result = VP8Depacketizer_GetPacketProperties( pkt.pPacketData,
                                                  pkt.packetDataLength,
                                                  &( properties ) );

    TEST_ASSERT_EQUAL( VP8_RESULT_BAD_PARAM,
                       result );

    pkt.pPacketData = &( packetData[ 0 ] );
    pkt.packetDataLength = 0;

    result = VP8Depacketizer_GetPacketProperties( pkt.pPacketData,
                                                  pkt.packetDataLength,
                                                  &( properties ) );

    TEST_ASSERT_EQUAL( VP8_RESULT_BAD_PARAM,
                       result );

    pkt.pPacketData = &( packetData[ 0 ] );
    pkt.packetDataLength = sizeof( packetData );

    result = VP8Depacketizer_GetPacketProperties( pkt.pPacketData,
                                                  pkt.packetDataLength,
                                                  NULL );

    TEST_ASSERT_EQUAL( VP8_RESULT_BAD_PARAM,
                       result );
}

/*-----------------------------------------------------------*/
