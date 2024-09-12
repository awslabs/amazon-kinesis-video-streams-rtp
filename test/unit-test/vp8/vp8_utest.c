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

#define PACKET_BUFFER_LENGTH 10

#define VP8_PACKETS_ARR_LEN     10
#define VP8_FRAME_BUF_LEN       32

uint8_t packetBuffer[ PACKET_BUFFER_LENGTH];
uint8_t frameBuffer[ MAX_FRAME_LENGTH ];

void setUp( void )
{
    memset( &( packetBuffer[ 0 ] ),
            0,
            sizeof( packetBuffer ) );
    memset( &( frameBuffer[ 0 ] ),
            0,
            sizeof( frameBuffer ) );
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
    uint8_t frameData[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
                           0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
                           0x20, 0x21, 0x22, 0x23, 0x24, 0x25,
                           0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
                           0x40, 0x41};
    VP8PacketizerContext_t ctx = {0};
    VP8Frame_t frame;
    VP8Packet_t pkt;
    uint8_t payloadDescFirstPkt[] = {0x10};
    uint8_t payloadDesc[] = {0x00};
    size_t i, frameDataIndex = 0;
    size_t expectedPacketCount = ( sizeof( frameData ) + PACKET_BUFFER_LENGTH - 1 ) / PACKET_BUFFER_LENGTH;

    memset( &( frame ),
            0,
            sizeof( VP8Frame_t ) );

    frame.pFrameData = &( frameData[0] );
    frame.frameDataLength = sizeof( frameData );

    result = VP8Packetizer_Init( &( ctx ),
                                 &( frame ) );

    TEST_ASSERT_EQUAL( VP8_RESULT_OK,
                       result );

    pkt.pPacketData = &( packetBuffer[0] );
    pkt.packetDataLength = PACKET_BUFFER_LENGTH;

    for(size_t packetIndex = 0; packetIndex < expectedPacketCount; packetIndex++)
    {
        result = VP8Packetizer_GetPacket( &( ctx ),
                                          &( pkt ) );

        TEST_ASSERT_EQUAL( VP8_RESULT_OK,
                           result );

        for(i = 0; i < pkt.packetDataLength; i++)
        {
            if( i < sizeof( payloadDesc ) )
            {
                if( frameDataIndex == 0 )
                {
                    TEST_ASSERT_EQUAL( payloadDescFirstPkt[i],
                                       pkt.pPacketData[i] );
                }
                else
                {
                    TEST_ASSERT_EQUAL( payloadDesc[i],
                                       pkt.pPacketData[i] );
                }
            }
            else
            {
                TEST_ASSERT_EQUAL( frameData[frameDataIndex],
                                   pkt.pPacketData[i] );
                frameDataIndex++;
            }
        }

        pkt.pPacketData = &( packetBuffer[0] );
        pkt.packetDataLength = PACKET_BUFFER_LENGTH;
    }

    result = VP8Packetizer_GetPacket( &( ctx ),
                                      &( pkt ) );
    TEST_ASSERT_EQUAL( VP8_RESULT_NO_MORE_PACKETS,
                       result );

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
    uint8_t frameData[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
                           0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
                           0x20, 0x21, 0x22, 0x23, 0x24, 0x25,
                           0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
                           0x40, 0x41};
    VP8PacketizerContext_t ctx = {0};
    VP8Frame_t frame;
    VP8Packet_t pkt;
    uint8_t payloadDescFirstPkt[] = {0xB0, 0x80, 0x1A};
    uint8_t payloadDesc[] = {0xA0, 0x80, 0x1A};
    size_t i, frameDataIndex = 0;
    size_t expectedPacketCount = sizeof( payloadDescFirstPkt );

    memset( &( frame ),
            0,
            sizeof( VP8Frame_t ) );

    frame.frameProperties |= VP8_FRAME_PROP_NON_REF_FRAME;
    frame.frameProperties |= VP8_FRAME_PROP_PICTURE_ID_PRESENT;
    frame.pictureId = 0x1A;
    frame.pFrameData = &( frameData[0] );
    frame.frameDataLength = sizeof( frameData );

    result = VP8Packetizer_Init( &( ctx ),
                                 &( frame ) );

    TEST_ASSERT_EQUAL( VP8_RESULT_OK,
                       result );

    pkt.pPacketData = &( packetBuffer[0] );
    pkt.packetDataLength = PACKET_BUFFER_LENGTH;

    for(size_t packetIndex = 0; packetIndex <= expectedPacketCount; packetIndex++)
    {
        result = VP8Packetizer_GetPacket( &( ctx ),
                                          &( pkt ) );

        TEST_ASSERT_EQUAL( VP8_RESULT_OK,
                           result );

        for(i = 0; i < pkt.packetDataLength; i++)
        {
            if( i < sizeof( payloadDesc ) )
            {
                if( frameDataIndex == 0 )
                {
                    TEST_ASSERT_EQUAL( payloadDescFirstPkt[i],
                                       pkt.pPacketData[i] );
                }
                else
                {
                    TEST_ASSERT_EQUAL( payloadDesc[i],
                                       pkt.pPacketData[i] );
                }
            }
            else
            {
                TEST_ASSERT_EQUAL( frameData[frameDataIndex],
                                   pkt.pPacketData[i] );
                frameDataIndex++;
            }
        }

        pkt.pPacketData = &( packetBuffer[0] );
        pkt.packetDataLength = PACKET_BUFFER_LENGTH;
    }

    result = VP8Packetizer_GetPacket( &( ctx ),
                                      &( pkt ) );

    TEST_ASSERT_EQUAL( VP8_RESULT_NO_MORE_PACKETS,
                       result );

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
    uint8_t frameData[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
                            0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
                            0x20, 0x21, 0x22, 0x23, 0x24, 0x25,
                            0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
                            0x40, 0x41 };
    VP8PacketizerContext_t ctx = { 0 };
    VP8Frame_t frame;
    VP8Packet_t pkt;
    uint8_t payloadDescFirstPkt[] = { 0xB0, 0x20, 0x60 };
    uint8_t payloadDesc[] = { 0xA0, 0x20, 0x60 };
    size_t i, frameDataIndex = 0;
    size_t expectedPacketCount = sizeof( payloadDescFirstPkt );

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

    pkt.pPacketData = &( packetBuffer[0] );
    pkt.packetDataLength = PACKET_BUFFER_LENGTH;

    for(size_t packetIndex = 0; packetIndex <= expectedPacketCount; packetIndex++)
    {
        result = VP8Packetizer_GetPacket( &( ctx ),
                                          &( pkt ) );

        TEST_ASSERT_EQUAL( VP8_RESULT_OK,
                           result );

        for(i = 0; i < pkt.packetDataLength; i++)
        {
            if( i < sizeof( payloadDesc ) )
            {
                if( frameDataIndex == 0 )
                {
                    TEST_ASSERT_EQUAL( payloadDescFirstPkt[i],
                                       pkt.pPacketData[i] );
                }
                else
                {
                    TEST_ASSERT_EQUAL( payloadDesc[i],
                                       pkt.pPacketData[i] );
                }
            }
            else
            {
                TEST_ASSERT_EQUAL( frameData[frameDataIndex],
                                   pkt.pPacketData[i] );
                frameDataIndex++;
            }
        }

        pkt.pPacketData = &( packetBuffer[0] );
        pkt.packetDataLength = PACKET_BUFFER_LENGTH;
    }

    result = VP8Packetizer_GetPacket( &( ctx ),
                                      &( pkt ) );

    TEST_ASSERT_EQUAL( VP8_RESULT_NO_MORE_PACKETS,
                       result );

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
    uint8_t frameData[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
                            0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
                            0x20, 0x21, 0x22, 0x23, 0x24, 0x25,
                            0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
                            0x40, 0x41 };
    VP8PacketizerContext_t ctx = { 0 };
    VP8Frame_t frame;
    VP8Packet_t pkt;
    uint8_t payloadDescFirstPkt[] = { 0xB0, 0x40, 0xAB };
    uint8_t payloadDesc[] = { 0xA0, 0x40, 0xAB };
    size_t i, frameDataIndex = 0;
    size_t expectedPacketCount = sizeof( payloadDescFirstPkt );

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

    pkt.pPacketData = &( packetBuffer[0] );
    pkt.packetDataLength = PACKET_BUFFER_LENGTH;

    for(size_t packetIndex = 0; packetIndex <= expectedPacketCount; packetIndex++)
    {
        result = VP8Packetizer_GetPacket( &( ctx ),
                                          &( pkt ) );

        TEST_ASSERT_EQUAL( VP8_RESULT_OK,
                           result );

        for(i = 0; i < pkt.packetDataLength; i++)
        {
            if( i < sizeof( payloadDesc ) )
            {
                if( frameDataIndex == 0 )
                {
                    TEST_ASSERT_EQUAL( payloadDescFirstPkt[i],
                                       pkt.pPacketData[i] );
                }
                else
                {
                    TEST_ASSERT_EQUAL( payloadDesc[i],
                                       pkt.pPacketData[i] );
                }
            }
            else
            {
                TEST_ASSERT_EQUAL( frameData[frameDataIndex],
                                   pkt.pPacketData[i] );
                frameDataIndex++;
            }
        }

        pkt.pPacketData = &( packetBuffer[0] );
        pkt.packetDataLength = PACKET_BUFFER_LENGTH;
    }

    result = VP8Packetizer_GetPacket( &( ctx ),
                                      &( pkt ) );

    TEST_ASSERT_EQUAL( VP8_RESULT_NO_MORE_PACKETS,
                       result );

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
    uint8_t frameData[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
                           0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
                           0x20, 0x21, 0x22, 0x23, 0x24, 0x25,
                           0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
                           0x40, 0x41};
    VP8PacketizerContext_t ctx = {0};
    VP8Frame_t frame;
    VP8Packet_t pkt;
    uint8_t payloadDescFirstPkt[] = { 0xB0, 0x10, 0xA};
    uint8_t payloadDesc[] = { 0xA0, 0x10, 0xA };
    size_t i, frameDataIndex = 0;
    size_t expectedPacketCount = sizeof( payloadDescFirstPkt );

    memset( &frame,
            0,
            sizeof( VP8Frame_t ) );

    frame.frameProperties |= VP8_FRAME_PROP_NON_REF_FRAME;
    frame.frameProperties |= VP8_FRAME_PROP_KEYIDX_PRESENT;
    frame.keyIndex = 10;
    frame.pFrameData = &( frameData[0] );
    frame.frameDataLength = sizeof( frameData );

    result = VP8Packetizer_Init( &ctx,
                                 &frame );

    TEST_ASSERT_EQUAL( VP8_RESULT_OK,
                       result );

    pkt.pPacketData = &( packetBuffer[0] );
    pkt.packetDataLength = PACKET_BUFFER_LENGTH;

    for(size_t packetIndex = 0; packetIndex <= expectedPacketCount; packetIndex++)
    {
        result = VP8Packetizer_GetPacket( &( ctx ),
                                          &( pkt ) );

        TEST_ASSERT_EQUAL( VP8_RESULT_OK,
                           result );

        for(i = 0; i < pkt.packetDataLength; i++)
        {
            if( i < sizeof( payloadDesc ) )
            {
                if( frameDataIndex == 0 )
                {
                    TEST_ASSERT_EQUAL( payloadDescFirstPkt[i],
                                       pkt.pPacketData[i] );
                }
                else
                {
                    TEST_ASSERT_EQUAL( payloadDesc[i],
                                       pkt.pPacketData[i] );
                }
            }
            else
            {
                TEST_ASSERT_EQUAL( frameData[frameDataIndex],
                                   pkt.pPacketData[i] );
                frameDataIndex++;
            }
        }

        pkt.pPacketData = &( packetBuffer[0] );
        pkt.packetDataLength = PACKET_BUFFER_LENGTH;

    }

    result = VP8Packetizer_GetPacket( &( ctx ),
                                      &( pkt ) );

    TEST_ASSERT_EQUAL( VP8_RESULT_NO_MORE_PACKETS,
                       result );

    TEST_ASSERT_EQUAL( sizeof( frameData ),
                       frameDataIndex );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate VP8 packetization happy path containing base only.
 */
void test_VP8_Packetizer_BaseOnly( void )
{
    VP8Result_t result;
    uint8_t frameData[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
                            0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
                            0x20, 0x21, 0x22, 0x23, 0x24, 0x25,
                            0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
                            0x40, 0x41 };
    VP8PacketizerContext_t ctx = { 0 };
    VP8Frame_t frame;
    VP8Packet_t pkt;
    uint8_t payloadDescFirstPkt[] = { 0xB0, 0x00, 0x10 };
    uint8_t payloadDesc[] = { 0xA0, 0x00, 0x10 };
    size_t i, frameDataIndex = 0;
    size_t expectedPacketCount = sizeof( payloadDescFirstPkt );

    memset( &( frame ),
            0,
            sizeof( VP8Frame_t ) );

    frame.frameProperties |= VP8_FRAME_PROP_NON_REF_FRAME;
    frame.frameProperties |= VP8_FRAME_PROP_DEPENDS_ON_BASE_ONLY;
    frame.pFrameData = &( frameData[ 0 ] );
    frame.frameDataLength = sizeof( frameData );

    result = VP8Packetizer_Init( &( ctx ),
                                 &( frame ) );

    TEST_ASSERT_EQUAL( VP8_RESULT_OK,
                       result );


    pkt.pPacketData = &( packetBuffer[0] );
    pkt.packetDataLength = PACKET_BUFFER_LENGTH;

    for(size_t packetIndex = 0; packetIndex <= expectedPacketCount; packetIndex++)
    {
        result = VP8Packetizer_GetPacket( &( ctx ),
                                          &( pkt ) );

        TEST_ASSERT_EQUAL( VP8_RESULT_OK,
                           result );

        for(i = 0; i < pkt.packetDataLength; i++)
        {
            if( i < sizeof( payloadDesc ) )
            {
                if( frameDataIndex == 0 )
                {
                    TEST_ASSERT_EQUAL( payloadDescFirstPkt[i],
                                       pkt.pPacketData[i] );
                }
                else
                {
                    TEST_ASSERT_EQUAL( payloadDesc[i],
                                       pkt.pPacketData[i] );
                }
            }
            else
            {
                TEST_ASSERT_EQUAL( frameData[frameDataIndex],
                                   pkt.pPacketData[i] );
                frameDataIndex++;
            }
        }

        pkt.pPacketData = &( packetBuffer[0] );
        pkt.packetDataLength = PACKET_BUFFER_LENGTH;
    }

    result = VP8Packetizer_GetPacket( &( ctx ),
                                      &( pkt ) );

    TEST_ASSERT_EQUAL( VP8_RESULT_NO_MORE_PACKETS,
                       result );

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
    uint8_t frameData[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
                            0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
                            0x20, 0x21, 0x22, 0x23, 0x24, 0x25,
                            0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
                            0x40, 0x41 };
    VP8PacketizerContext_t ctx = { 0 };
    VP8Frame_t frame;
    VP8Packet_t pkt;
    uint8_t payloadDescFirstPkt[] = { 0xB0, 0xF0, 0xFA, 0xCD, 0xAB, 0xBA };
    uint8_t payloadDesc[] = { 0xA0, 0xF0, 0xFA, 0xCD, 0xAB, 0xBA };
    size_t i, frameDataIndex = 0;
    size_t expectedPacketCount = sizeof( payloadDescFirstPkt );

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
    frame.tid = 5;
    frame.keyIndex = 10;
    frame.pFrameData = &( frameData[ 0 ] );
    frame.frameDataLength = sizeof( frameData );

    result = VP8Packetizer_Init( &( ctx ),
                                 &( frame ) );

    TEST_ASSERT_EQUAL( VP8_RESULT_OK,
                       result );

    pkt.pPacketData = &( packetBuffer[0] );
    pkt.packetDataLength = PACKET_BUFFER_LENGTH;

    for(size_t packetIndex = 0; packetIndex <= expectedPacketCount; packetIndex++)
    {
        result = VP8Packetizer_GetPacket( &( ctx ),
                                          &( pkt ) );

        TEST_ASSERT_EQUAL( VP8_RESULT_OK,
                           result );

        for(i = 0; i < pkt.packetDataLength; i++)
        {
            if( i < sizeof( payloadDesc ) )
            {
                if( frameDataIndex == 0 )
                {
                    TEST_ASSERT_EQUAL( payloadDescFirstPkt[i],
                                       pkt.pPacketData[i] );
                }
                else
                {
                    TEST_ASSERT_EQUAL( payloadDesc[i],
                                       pkt.pPacketData[i] );
                }
            }
            else
            {
                TEST_ASSERT_EQUAL( frameData[frameDataIndex],
                                   pkt.pPacketData[i] );
                frameDataIndex++;
            }
        }

        pkt.pPacketData = &( packetBuffer[0] );
        pkt.packetDataLength = PACKET_BUFFER_LENGTH;
    }

    result = VP8Packetizer_GetPacket( &( ctx ),
                                      &( pkt ) );

    TEST_ASSERT_EQUAL( VP8_RESULT_NO_MORE_PACKETS,
                       result );

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
    uint8_t frameData[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
                            0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
                            0x20, 0x21, 0x22, 0x23, 0x24, 0x25,
                            0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
                            0x40, 0x41 };
    VP8PacketizerContext_t ctx = { 0 };
    VP8Frame_t frame;

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
    uint8_t frameData[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
                            0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
                            0x20, 0x21, 0x22, 0x23, 0x24, 0x25,
                            0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
                            0x40, 0x41 };
    VP8PacketizerContext_t ctx = { 0 };
    VP8Frame_t frame;
    VP8Packet_t pkt;

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
    uint8_t frameData[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
                            0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
                            0x20, 0x21, 0x22, 0x23, 0x24, 0x25,
                            0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
                            0x40, 0x41 };
    VP8PacketizerContext_t ctx = { 0 };
    VP8Frame_t frame;
    VP8Packet_t pkt;

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
    ctx.payloadDescLength = PACKET_BUFFER_LENGTH + 1;

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
    VP8DepacketizerContext_t ctx = { 0 };
    VP8Result_t result;
    VP8Packet_t pkt;
    VP8Frame_t frame;
    VP8Packet_t packetsArray[ VP8_PACKETS_ARR_LEN ];

    uint8_t packetData1[] = { 0xB0, 0xF0, 0xFA, 0xCD, 0xAB, 0xBA, 0x00, 0x01, 0x02, 0x03 };
    uint8_t packetData2[] = { 0xA0, 0xF0, 0xFA, 0xCD, 0xAB, 0xBA, 0x04, 0x05, 0x10, 0x11 };
    uint8_t packetData3[] = { 0xA0, 0xF0, 0xFA, 0xCD, 0xAB, 0xBA, 0x12, 0x13, 0x14, 0x15 };
    uint8_t packetData4[] = { 0xA0, 0xF0, 0xFA, 0xCD, 0xAB, 0xBA, 0x20, 0x21, 0x22, 0x23 };
    uint8_t packetData5[] = { 0xA0, 0xF0, 0xFA, 0xCD, 0xAB, 0xBA, 0x24, 0x25, 0x30, 0x31 };
    uint8_t packetData6[] = { 0xA0, 0xF0, 0xFA, 0xCD, 0xAB, 0xBA, 0x32, 0x33, 0x34, 0x35 };
    uint8_t packetData7[] = { 0xA0, 0xF0, 0xFA, 0xCD, 0xAB, 0xBA, 0x40, 0x41 };

    uint8_t decodedFrameData[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
                                   0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
                                   0x20, 0x21, 0x22, 0x23, 0x24, 0x25,
                                   0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
                                   0x40, 0x41 };

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

    TEST_ASSERT_EQUAL( 5,
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
 * @brief Validate VP8 depacketization happy path containing picture ID.
 */
void test_VP8_Depacketizer_PictureID( void )
{
    VP8DepacketizerContext_t ctx = { 0 };
    VP8Result_t result;
    VP8Packet_t pkt;
    VP8Frame_t frame;
    VP8Packet_t packetsArray[ VP8_PACKETS_ARR_LEN ];

    uint8_t packetData1[] = { 0xB0, 0x40, 0xAB, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x10 };
    uint8_t packetData2[] = { 0xA0, 0x40, 0xAB, 0x11, 0x12, 0x13, 0x14, 0x15, 0x20, 0x21 };
    uint8_t packetData3[] = { 0xA0, 0x40, 0xAB, 0x22, 0x23, 0x24, 0x25, 0x30, 0x31, 0x32 };
    uint8_t packetData4[] = { 0xA0, 0x40, 0xAB, 0x33, 0x34, 0x35, 0x40, 0x41 };

    uint8_t decodedFrameData[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
                                   0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
                                   0x20, 0x21, 0x22, 0x23, 0x24, 0x25,
                                   0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
                                   0x40, 0x41 };

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
 * @brief Validate VP8 depacketization GetPacketProperties's happy path.
 */
void test_VP8_Depacketizer_GetPacketProperties( void )
{
    VP8Result_t result;
    VP8Packet_t pkt;

    uint8_t packetData1[] = { 0xB0, 0x40, 0xAB, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x10 };

    uint32_t properties;

    pkt.pPacketData = &( packetData1[ 0 ] );
    pkt.packetDataLength = sizeof( packetData1 );

    result = VP8Depacketizer_GetPacketProperties( pkt.pPacketData,
                                                  pkt.packetDataLength,
                                                  &properties );

    TEST_ASSERT_EQUAL( VP8_RESULT_OK,
                       result );
}

/*-----------------------------------------------------------*/

// /**
//  * @brief Validate VP8 depacketization happy path with no X bit set (no extensions).
//  */
void test_VP8_Depacketizer_NoExtensions( void )
{
    VP8DepacketizerContext_t ctx = { 0 };
    VP8Result_t result;
    VP8Packet_t pkt;
    VP8Frame_t frame;
    VP8Packet_t packetsArray[VP8_PACKETS_ARR_LEN];

    uint8_t packetData1[] = {0xB9, 0x00, 0x01, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x10};
    uint8_t decodedFrameData[] = {0x01, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x10};

    result = VP8Depacketizer_Init( &( ctx ),
                                   &( packetsArray[0] ),
                                   VP8_PACKETS_ARR_LEN );

    TEST_ASSERT_EQUAL( VP8_RESULT_OK,
                       result );

    pkt.pPacketData = &( packetData1[0] );
    pkt.packetDataLength = sizeof( packetData1 );

    result = VP8Depacketizer_AddPacket( &( ctx ),
                                        &( pkt ) );

    TEST_ASSERT_EQUAL( VP8_RESULT_OK,
                       result );

    frame.pFrameData = &( frameBuffer[0] );
    frame.frameDataLength = VP8_FRAME_BUF_LEN;

    result = VP8Depacketizer_GetFrame( &( ctx ),
                                       &( frame ) );

    TEST_ASSERT_EQUAL( VP8_RESULT_OK,
                       result );

    TEST_ASSERT_EQUAL( sizeof( decodedFrameData ),
                       frame.frameDataLength );

    TEST_ASSERT_EQUAL_UINT8_ARRAY( &( decodedFrameData[0] ),
                                   &( frame.pFrameData[0] ),
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
    VP8DepacketizerContext_t ctx = { 0 };
    VP8Result_t result;
    VP8Packet_t packetsArray[ VP8_PACKETS_ARR_LEN ];

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
    VP8DepacketizerContext_t ctx = { 0 };
    VP8Result_t result;
    VP8Packet_t pkt;
    VP8Packet_t packetsArray[ VP8_PACKETS_ARR_LEN ];

    uint8_t packetData1[] = { 0xB0, 0x40, 0xAB, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x10 };

    result = VP8Depacketizer_Init( &( ctx ),
                                   &( packetsArray[ 0 ] ),
                                   VP8_PACKETS_ARR_LEN );

    TEST_ASSERT_EQUAL( VP8_RESULT_OK,
                       result );

    pkt.pPacketData = &( packetData1[ 0 ] );
    pkt.packetDataLength = sizeof( packetData1 );

    result = VP8Depacketizer_AddPacket( NULL,
                                        &( pkt ) );

    TEST_ASSERT_EQUAL( VP8_RESULT_BAD_PARAM,
                       result );

    result = VP8Depacketizer_AddPacket( &( ctx ),
                                        NULL );

    TEST_ASSERT_EQUAL( VP8_RESULT_BAD_PARAM,
                       result );

    pkt.pPacketData = &( packetData1[ 0 ] );
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
void test_VP8_Depacketizer_AddPacket_OutOfMem( void )
{
    VP8DepacketizerContext_t ctx = { 0 };
    VP8Result_t result;
    VP8Packet_t pkt;
    VP8Packet_t packetsArray[ 1];

    uint8_t packetData1[] = { 0xB0, 0x40, 0xAB, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x10 };
    uint8_t packetData2[] = { 0xA0, 0x40, 0xAB, 0x11, 0x12, 0x13, 0x14, 0x15, 0x20, 0x21 };

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
    VP8DepacketizerContext_t ctx = { 0 };
    VP8Result_t result;
    VP8Packet_t pkt;
    VP8Frame_t frame;
    VP8Packet_t packetsArray[ VP8_PACKETS_ARR_LEN ];

    uint8_t packetData1[] = { 0xB0, 0xF0, 0xFA, 0xCD, 0xAB, 0xBA, 0x00, 0x01, 0x02, 0x03 };
    uint8_t packetData2[] = { 0xA0, 0xF0, 0xFA, 0xCD, 0xAB, 0xBA, 0x04, 0x05, 0x10, 0x11 };
    uint8_t packetData3[] = { 0xA0, 0xF0, 0xFA, 0xCD, 0xAB, 0xBA, 0x12, 0x13, 0x14, 0x15 };

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
    VP8DepacketizerContext_t ctx = { 0 };
    VP8Result_t result;
    VP8Packet_t pkt;
    VP8Frame_t frame1;
    VP8Packet_t packetsArray[ VP8_PACKETS_ARR_LEN ];

    uint8_t packetData1[] = { 0xB0, 0xF0, 0xFA, 0xCD, 0xAB, 0xBA, 0x00, 0x01, 0x02, 0x03 };
    uint8_t packetData2[] = { 0xA0, 0xF0, 0xFA, 0xCD, 0xAB, 0xBA, 0x04, 0x05, 0x10, 0x11 };

    uint8_t frameBuffer[1];

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

    frame1.pFrameData = &( frameBuffer[ 0 ] );
    frame1.frameDataLength = sizeof( frameBuffer );

    result = VP8Depacketizer_GetFrame( &( ctx ),
                                       &( frame1 ) );

    TEST_ASSERT_EQUAL( VP8_RESULT_OUT_OF_MEMORY,
                       result );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate VP8_Depacketizer_GetFrame in case of malformed packet.
 */
void test_VP8_Depacketizer_GetFrame_MalPacket( void )
{
    VP8DepacketizerContext_t ctx = { 0 };
    VP8Result_t result;
    VP8Packet_t pkt;
    VP8Frame_t frame1;
    VP8Packet_t packetsArray[ VP8_PACKETS_ARR_LEN ];

    uint8_t packetData1[] = { 0xB9, 0xF7};
    uint8_t packetData2[] = { 0x50, 0xF0, 0xFA, 0xCD, 0xAB, 0xBA, 0x04, 0x05, 0x10, 0x11 };

    uint8_t frameBuffer[1];

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

    frame1.pFrameData = &( frameBuffer[ 0 ] );
    frame1.frameDataLength = sizeof( frameBuffer );

    result = VP8Depacketizer_GetFrame( &( ctx ),
                                       &( frame1 ) );

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

    uint8_t packetData1[] = { 0xB0, 0x40, 0xAB, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x10 };

    uint32_t properties;

    pkt.pPacketData = NULL;
    pkt.packetDataLength = sizeof( packetData1 );

    result = VP8Depacketizer_GetPacketProperties( pkt.pPacketData,
                                                  pkt.packetDataLength,
                                                  &properties );

    TEST_ASSERT_EQUAL( VP8_RESULT_BAD_PARAM,
                       result );

    pkt.pPacketData = &( packetData1[ 0 ] );
    pkt.packetDataLength = 0;

    result = VP8Depacketizer_GetPacketProperties( pkt.pPacketData,
                                                  pkt.packetDataLength,
                                                  &properties );

    TEST_ASSERT_EQUAL( VP8_RESULT_BAD_PARAM,
                       result );

    pkt.pPacketData = &( packetData1[ 0 ] );
    pkt.packetDataLength = sizeof( packetData1 );

    result = VP8Depacketizer_GetPacketProperties( pkt.pPacketData,
                                                  pkt.packetDataLength,
                                                  NULL );

    TEST_ASSERT_EQUAL( VP8_RESULT_BAD_PARAM,
                       result );
}

/*-----------------------------------------------------------*/

/**
 * @brief Validate VP8_Depacketizer_GetFrame with X bit not set.
 */
void test_VP8_Depacketizer_GetFrame_XBitNotSet( void )
{
    VP8DepacketizerContext_t ctx = { 0 };
    VP8Result_t result;
    VP8Packet_t pkt;
    VP8Frame_t frame;
    VP8Packet_t packetsArray[ VP8_PACKETS_ARR_LEN ];

    uint8_t packetData1[] = { 0x50, 0xE0, 0xC0, 0xCD, 0xAB, 0xBA, 0x00, 0x01, 0x02, 0x03 };
    uint8_t packetData2[] = { 0xB0, 0xE0, 0xC0, 0xCD, 0xAB, 0xBA, 0x04, 0x05, 0x10, 0x11 };
    uint8_t packetData3[] = { 0XB0, 0xD0, 0xFA, 0xCD, 0xAB, 0xBA, 0x12, 0x13, 0x14, 0x15 };

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

    frame.pFrameData = &( frameBuffer[ 0 ] );
    frame.frameDataLength = VP8_FRAME_BUF_LEN;

    result = VP8Depacketizer_GetFrame( &( ctx ),
                                       &( frame ) );

    TEST_ASSERT_EQUAL( VP8_RESULT_OK,
                       result );
}

/*-----------------------------------------------------------*/
