/* Standard includes. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* API includes. */
#include "h264_packetizer.h"
#include "g711_packetizer.h"
#include "vp8_packetizer.h"
#include "opus_packetizer.h"

#define MAX_NALUS_IN_A_FRAME 512
#define MAX_PACKET_LENGTH   1200

#define VP8_PACKET_LENGTH   10
#define G711_PACKET_LENGTH  10
#define OPUS_PACKET_LENGTH  10
/*-----------------------------------------------------------*/

void DumpPacket( H264Packet_t * pPacket,
                 int packetNumber )
{
    size_t i;

    printf( "Packet No %d - Packet Length: %lu.\r\n",
            packetNumber,
            pPacket->packetDataLength );
    for( i = 0; i < pPacket->packetDataLength; i++ )
    {
        printf( "0x%02x, ",
                pPacket->pPacketData[i] );
    }
    printf( "\n" );
}

void DumpNalu( uint8_t * pNalu,
               int naluLength,
               int naluNumber )
{
    size_t i;

    printf( "Nalu No %d - Nalu Length: %u.\r\n",
            naluNumber,
            naluLength );
    for( i = 0; i < naluLength; i++ )
    {
        printf( "0x%02x, ",
                pNalu[i] );
    }
    printf( "\n" );
}

/*-----------------------------------------------------------*/

void ProcessNalu( H264PacketizerContext_t * pCtx,
                  int naluNumber,
                  uint8_t * pNalu,
                  size_t naluLength )
{
    Nalu_t nalu;
    H264Result_t result;

    nalu.pNaluData = pNalu;
    nalu.naluDataLength = naluLength;

    result = H264Packetizer_AddNalu( pCtx,
                                     &( nalu ) );
    assert( result == H264_RESULT_OK );
}

/*-----------------------------------------------------------*/

void ProcessFrame( uint8_t * pFrame,
                   size_t frameLength )
{
    int naluNumber = 0, packetNumber = 0;
    size_t curIndex = 0, naluStartIndex = 0, remainingLength;
    uint32_t fistStartCode = 1;
    H264PacketizerContext_t ctx;
    H264Result_t result;
    H264Packet_t pkt;
    uint8_t pktBuffer[ MAX_PACKET_LENGTH ];
    Nalu_t nalusArray[ MAX_NALUS_IN_A_FRAME ];
    uint32_t mtuSize = 12, i = 0;
    uint32_t expectedPacketLength[] = { 2, 12, 4, 12, 12, 12, 12 };

    uint8_t startCode1[] = { 0x00, 0x00, 0x00, 0x01 };
    uint8_t startCode2[] = { 0x00, 0x00, 0x01 };

    result = H264Packetizer_Init( &( ctx ),
                                  &( nalusArray[ 0 ] ),
                                  MAX_NALUS_IN_A_FRAME );
    assert( result == H264_RESULT_OK );

    while( curIndex < frameLength )
    {
        remainingLength = frameLength - curIndex;

        /* Check the presence of 4 byte start code. */
        if( remainingLength >= sizeof( startCode1 ) )
        {
            if( memcmp( &( pFrame[ curIndex ] ),
                        &( startCode1[ 0 ] ),
                        sizeof( startCode1 ) ) == 0 )
            {
                if( fistStartCode == 1 )
                {
                    fistStartCode = 0;
                }
                else
                {
                    naluNumber++;
                    ProcessNalu( &( ctx ),
                                 naluNumber,
                                 &( pFrame[ naluStartIndex ] ),
                                 curIndex - naluStartIndex );
                }

                naluStartIndex = curIndex + sizeof( startCode1 );
                curIndex = curIndex + sizeof( startCode1 );
                continue;
            }
        }

        /* Check the presence of 3 byte start code. */
        if( remainingLength >= sizeof( startCode2 ) )
        {
            if( memcmp( &( pFrame[ curIndex ] ),
                        &( startCode2[ 0 ] ),
                        sizeof( startCode2 ) ) == 0 )
            {
                if( fistStartCode == 1 )
                {
                    fistStartCode = 0;
                }
                else
                {
                    naluNumber++;
                    ProcessNalu( &( ctx ),
                                 naluNumber,
                                 &( pFrame[ naluStartIndex ] ),
                                 curIndex - naluStartIndex );
                }

                naluStartIndex = curIndex + sizeof( startCode2 );
                curIndex = curIndex + sizeof( startCode2 );
                continue;
            }
        }

        curIndex += 1;
    }

    if( naluStartIndex > 0 )
    {
        naluNumber++;
        ProcessNalu( &( ctx ),
                     naluNumber,
                     &( pFrame[ naluStartIndex ] ),
                     frameLength - naluStartIndex );
    }

    pkt.pPacketData = &( pktBuffer[ 0 ] );
    pkt.packetDataLength = mtuSize;
    result = H264Packetizer_GetPacket( &( ctx ),
                                       &( pkt ) );


    while( result != H264_RESULT_NO_MORE_PACKETS )
    {
        packetNumber++;
        assert( expectedPacketLength[i++] == pkt.packetDataLength );
        //DumpPacket( &( pkt ), packetNumber );

        pkt.pPacketData = &( pktBuffer[ 0 ] );
        pkt.packetDataLength = mtuSize;
        result = H264Packetizer_GetPacket( &( ctx ),
                                           &( pkt ) );
    }
}

/*-----------------------------------------------------------*/

void ProcessFrame2( uint8_t * pFrame,
                    size_t frameLength )
{
    int packetNumber = 0;
    H264PacketizerContext_t ctx;
    H264Result_t result;
    H264Packet_t pkt;
    Frame_t frame;
    Nalu_t nalusArray[ MAX_NALUS_IN_A_FRAME ];
    uint8_t pktBuffer[ MAX_PACKET_LENGTH ];
    uint32_t mtuSize = 12,i = 0;
    uint32_t expectedPacketLength[] = { 2, 12, 4, 12, 12, 12, 12 };

    result = H264Packetizer_Init( &( ctx ),
                                  &( nalusArray[ 0 ] ),
                                  MAX_NALUS_IN_A_FRAME );
    assert( result == H264_RESULT_OK );

    frame.pFrameData = pFrame;
    frame.frameDataLength = frameLength;
    result = H264Packetizer_AddFrame( &( ctx ),
                                      &( frame ) );
    assert( result == H264_RESULT_OK );

    pkt.pPacketData = &( pktBuffer[ 0 ] );
    pkt.packetDataLength = mtuSize;
    result = H264Packetizer_GetPacket( &( ctx ),
                                       &( pkt ) );

    while( result != H264_RESULT_NO_MORE_PACKETS )
    {
        packetNumber++;
        assert( expectedPacketLength[i++] == pkt.packetDataLength );
        //DumpPacket( &( pkt ), packetNumber );

        pkt.pPacketData = &( pktBuffer[ 0 ] );
        pkt.packetDataLength = mtuSize;
        result = H264Packetizer_GetPacket( &( ctx ),
                                           &( pkt ) );
    }
}

/*-----------------------------------------------------------*/

void GenerateNalus( uint8_t * pFrame,
                    size_t frameLength )
{
    int naluNumber = 0, packetNumber = 0;
    size_t curIndex = 0, naluStartIndex = 0, remainingLength;
    uint32_t fistStartCode = 1, i = 0;
    H264Result_t result;
    size_t naluLength;
    uint8_t startCode[] = { 0x00, 0x00, 0x00, 0x01 };
    uint32_t expectedPacketLength[] = { 2, 12, 4, 12, 21, 12 };

    while( curIndex < frameLength )
    {
        remainingLength = frameLength - curIndex;

        /* Check the presence of 4 byte start code. */
        if( remainingLength >= sizeof( startCode ) )
        {
            if( memcmp( &( pFrame[ curIndex ] ),
                        &( startCode[ 0 ] ),
                        sizeof( startCode ) ) == 0 )
            {
                if( fistStartCode == 1 )
                {
                    fistStartCode = 0;
                }
                else
                {
                    naluNumber++;
                    naluLength = curIndex - naluStartIndex;
                    assert( expectedPacketLength[i++] == naluLength );
                    //DumpNalu( &(pFrame[ naluStartIndex ]), naluLength, naluNumber);
                }

                naluStartIndex = curIndex + sizeof( startCode );
                curIndex = curIndex + sizeof( startCode );
                continue;
            }
        }

        curIndex += 1;
    }

    if( naluStartIndex > 0 )
    {
        naluNumber++;
        naluLength = frameLength - naluStartIndex;
        assert( expectedPacketLength[i++] == naluLength );
        //DumpNalu( &(pFrame[ naluStartIndex ]), naluLength, naluNumber);
    }
}

/*-----------------------------------------------------------*/

void h264Packetizer()
{
    int i;
    FILE * fp;
    uint8_t pFrame[] = { 0x00, 0x00, 0x00, 0x01, 0x09, 0x10,
                         0x00, 0x00, 0x00, 0x01, 0x67, 0x42, 0xc0, 0x1f, 0xda, 0x01, 0x40, 0x16, 0xec, 0x05, 0xa8, 0x08,
                         0x00, 0x00, 0x00, 0x01, 0x68, 0xce, 0x3c, 0x80,
                         0x00, 0x00, 0x00, 0x01, 0x06, 0x05, 0xff, 0xff, 0xb7, 0xdc, 0x45, 0xe9, 0xbd, 0xe6, 0xd9, 0x48,
                         0x00, 0x00, 0x00, 0x01, 0x65, 0x88, 0x84, 0x12, 0xff, 0xff, 0xfc, 0x3d, 0x14, 0x00, 0x04, 0xba, 0xeb, 0xae, 0xba, 0xeb, 0xae, 0xba, 0xeb, 0xae, 0xba,
                         0x00, 0x00, 0x00, 0x01, 0x65, 0x00, 0x6e, 0x22, 0x21, 0x04, 0xbf, 0xff, 0xff, 0x0f, 0x45, 0x00 };
    size_t frameLength = sizeof( pFrame );

    ProcessFrame( &( pFrame[ 0 ] ),
                  frameLength );
}

void h264Packetizer_2()
{
    int i;
    FILE * fp;
    uint8_t pFrame[] = { 0x00, 0x00, 0x00, 0x01, 0x09, 0x10,
                         0x00, 0x00, 0x00, 0x01, 0x67, 0x42, 0xc0, 0x1f, 0xda, 0x01, 0x40, 0x16, 0xec, 0x05, 0xa8, 0x08,
                         0x00, 0x00, 0x00, 0x01, 0x68, 0xce, 0x3c, 0x80,
                         0x00, 0x00, 0x00, 0x01, 0x06, 0x05, 0xff, 0xff, 0xb7, 0xdc, 0x45, 0xe9, 0xbd, 0xe6, 0xd9, 0x48,
                         0x00, 0x00, 0x00, 0x01, 0x65, 0x88, 0x84, 0x12, 0xff, 0xff, 0xfc, 0x3d, 0x14, 0x00, 0x04, 0xba, 0xeb, 0xae, 0xba, 0xeb, 0xae, 0xba, 0xeb, 0xae, 0xba,
                         0x00, 0x00, 0x00, 0x01, 0x65, 0x00, 0x6e, 0x22, 0x21, 0x04, 0xbf, 0xff, 0xff, 0x0f, 0x45, 0x00 };
    size_t frameLength = sizeof( pFrame );

    ProcessFrame2( &( pFrame[ 0 ] ),
                   frameLength );
}

void h264Packetizer_3()
{
    int i;
    FILE * fp;
    uint8_t pFrame[] = { 0x00, 0x00, 0x00, 0x01, 0x09, 0x10,
                         0x00, 0x00, 0x00, 0x01, 0x67, 0x42, 0xc0, 0x1f, 0xda, 0x01, 0x40, 0x16, 0xec, 0x05, 0xa8, 0x08,
                         0x00, 0x00, 0x00, 0x01, 0x68, 0xce, 0x3c, 0x80,
                         0x00, 0x00, 0x00, 0x01, 0x06, 0x05, 0xff, 0xff, 0xb7, 0xdc, 0x45, 0xe9, 0xbd, 0xe6, 0xd9, 0x48,
                         0x00, 0x00, 0x00, 0x01, 0x65, 0x88, 0x84, 0x12, 0xff, 0xff, 0xfc, 0x3d, 0x14, 0x00, 0x04, 0xba, 0xeb, 0xae, 0xba, 0xeb, 0xae, 0xba, 0xeb, 0xae, 0xba,
                         0x00, 0x00, 0x00, 0x01, 0x65, 0x00, 0x6e, 0x22, 0x21, 0x04, 0xbf, 0xff, 0xff, 0x0f, 0x45, 0x00 };
    size_t frameLength = sizeof( pFrame );

    GenerateNalus( &( pFrame[ 0 ] ),
                   frameLength );
}

/*-----------------------------------------------------------*/

void g711Packetizer()
{
    G711Result_t result;
    uint8_t frameData[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
                            0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
                            0x20, 0x21, 0x22, 0x23, 0x24, 0x25,
                            0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
                            0x40, 0x41 };
    G711PacketizerContext_t ctx;
    G711Packet_t pkt;
    G711Frame_t frame;
    uint8_t pktBuffer[ G711_PACKET_LENGTH ];
    size_t i, curFrameDataIndex = 0;

    frame.pFrameData = &( frameData [ 0 ] );
    frame.frameDataLength = sizeof( frameData );

    result = G711Packetizer_Init( &( ctx ),
                                  &( frame ) );
    assert( result == G711_RESULT_OK );

    pkt.pPacketData = &( pktBuffer[ 0 ] );
    pkt.packetDataLength = G711_PACKET_LENGTH;
    result = G711Packetizer_GetPacket( &( ctx ),
                                       &( pkt ) );

    while( result != VP8_RESULT_NO_MORE_PACKETS )
    {
        assert( pkt.packetDataLength <= G711_PACKET_LENGTH );
        for( i = 0; i < pkt.packetDataLength; i++ )
        {
            assert( pkt.pPacketData[ i ] == frameData[ curFrameDataIndex ] );
            curFrameDataIndex++;
        }

        pkt.pPacketData = &( pktBuffer[ 0 ] );
        pkt.packetDataLength = G711_PACKET_LENGTH;
        result = G711Packetizer_GetPacket( &( ctx ),
                                           &( pkt ) );
    }

    assert( curFrameDataIndex == sizeof( frameData ) );
}

/*-----------------------------------------------------------*/

void OpusPacketizer()
{
    OpusResult_t result;
    uint8_t frameData[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
                            0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
                            0x20, 0x21, 0x22, 0x23, 0x24, 0x25,
                            0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
                            0x40, 0x41 };
    OpusPacketizerContext_t ctx;
    OpusPacket_t pkt;
    OpusFrame_t frame;
    uint8_t pktBuffer[ OPUS_PACKET_LENGTH ];
    size_t i, curFrameDataIndex = 0;

    frame.pFrameData = &( frameData [ 0 ] );
    frame.frameDataLength = sizeof( frameData );

    result = OpusPacketizer_Init( &( ctx ),
                                  &( frame ) );
    assert( result == OPUS_RESULT_OK );

    pkt.pPacketData = &( pktBuffer[ 0 ] );
    pkt.packetDataLength = OPUS_PACKET_LENGTH;
    result = OpusPacketizer_GetPacket( &( ctx ),
                                       &( pkt ) );

    while( result != OPUS_RESULT_NO_MORE_PACKETS )
    {
        assert( pkt.packetDataLength <= OPUS_PACKET_LENGTH );
        for( i = 0; i < pkt.packetDataLength; i++ )
        {
            assert( pkt.pPacketData[ i ] == frameData[ curFrameDataIndex ] );
            curFrameDataIndex++;
        }

        pkt.pPacketData = &( pktBuffer[ 0 ] );
        pkt.packetDataLength = OPUS_PACKET_LENGTH;
        result = OpusPacketizer_GetPacket( &( ctx ),
                                           &( pkt ) );
    }

    assert( curFrameDataIndex == sizeof( frameData ) );
}

/*-----------------------------------------------------------*/

void vp8Packetizer_1()
{
    VP8Result_t result;
    uint8_t frameData[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
                            0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
                            0x20, 0x21, 0x22, 0x23, 0x24, 0x25,
                            0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
                            0x40, 0x41 };
    VP8PacketizerContext_t ctx;
    VP8Frame_t frame;
    uint8_t pktBuffer[ VP8_PACKET_LENGTH ];
    VP8Packet_t pkt;
    uint8_t payloadDescFirstPkt[] = { 0x10 };
    uint8_t payloadDesc[] = { 0x00 };
    size_t i, frameDataIndex = 0;

    memset( &( frame ),
            0,
            sizeof( VP8Frame_t ) );

    frame.pFrameData = &( frameData[ 0 ] );
    frame.frameDataLength = sizeof( frameData );

    result = VP8Packetizer_Init( &( ctx ),
                                 &( frame ) );
    assert( result == VP8_RESULT_OK );

    pkt.pPacketData = &( pktBuffer[ 0 ] );
    pkt.packetDataLength = VP8_PACKET_LENGTH;

    result = VP8Packetizer_GetPacket( &( ctx ),
                                      &( pkt ) );

    while( result != VP8_RESULT_NO_MORE_PACKETS )
    {
        //printf( "Packet Length: %lu.\n", pkt.packetDataLength );
        for( i = 0; i < pkt.packetDataLength; i++ )
        {
            if( i < sizeof( payloadDesc ) )
            {
                if( frameDataIndex == 0 )
                {
                    assert( pkt.pPacketData[ i ] == payloadDescFirstPkt[ i ] );
                }
                else
                {
                    assert( pkt.pPacketData[ i ] == payloadDesc[ i ] );
                }
            }
            else
            {
                assert( pkt.pPacketData[ i ] == frameData[ frameDataIndex ] );
                frameDataIndex++;
            }
        }

        pkt.pPacketData = &( pktBuffer[ 0 ] );
        pkt.packetDataLength = VP8_PACKET_LENGTH;

        result = VP8Packetizer_GetPacket( &( ctx ),
                                          &( pkt ) );

    }

    assert( frameDataIndex == sizeof( frameData ) );
}

/*-----------------------------------------------------------*/

void vp8Packetizer_2()
{
    VP8Result_t result;
    uint8_t frameData[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
                            0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
                            0x20, 0x21, 0x22, 0x23, 0x24, 0x25,
                            0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
                            0x40, 0x41 };
    VP8PacketizerContext_t ctx;
    VP8Frame_t frame;
    uint8_t pktBuffer[ VP8_PACKET_LENGTH ];
    VP8Packet_t pkt;
    uint8_t payloadDescFirstPkt[] = { 0xB0, 0x80, 0x1A };
    uint8_t payloadDesc[] = { 0xA0, 0x80, 0x1A };
    size_t i, frameDataIndex = 0;

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
    assert( result == VP8_RESULT_OK );

    pkt.pPacketData = &( pktBuffer[ 0 ] );
    pkt.packetDataLength = VP8_PACKET_LENGTH;

    result = VP8Packetizer_GetPacket( &( ctx ),
                                      &( pkt ) );

    while( result != VP8_RESULT_NO_MORE_PACKETS )
    {
        //printf( "Packet Length: %lu.\n", pkt.packetDataLength );
        for( i = 0; i < pkt.packetDataLength; i++ )
        {
            if( i < sizeof( payloadDesc ) )
            {
                if( frameDataIndex == 0 )
                {
                    assert( pkt.pPacketData[ i ] == payloadDescFirstPkt[ i ] );
                }
                else
                {
                    assert( pkt.pPacketData[ i ] == payloadDesc[ i ] );
                }
            }
            else
            {
                assert( pkt.pPacketData[ i ] == frameData[ frameDataIndex ] );
                frameDataIndex++;
            }
        }

        pkt.pPacketData = &( pktBuffer[ 0 ] );
        pkt.packetDataLength = VP8_PACKET_LENGTH;

        result = VP8Packetizer_GetPacket( &( ctx ),
                                          &( pkt ) );

    }

    assert( frameDataIndex == sizeof( frameData ) );
}

/*-----------------------------------------------------------*/

void vp8Packetizer_3()
{
    VP8Result_t result;
    uint8_t frameData[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
                            0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
                            0x20, 0x21, 0x22, 0x23, 0x24, 0x25,
                            0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
                            0x40, 0x41 };
    VP8PacketizerContext_t ctx;
    VP8Frame_t frame;
    uint8_t pktBuffer[ VP8_PACKET_LENGTH ];
    VP8Packet_t pkt;
    uint8_t payloadDescFirstPkt[] = { 0xB0, 0x20, 0x60 };
    uint8_t payloadDesc[] = { 0xA0, 0x20, 0x60 };
    size_t i, frameDataIndex = 0;

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
    assert( result == VP8_RESULT_OK );

    pkt.pPacketData = &( pktBuffer[ 0 ] );
    pkt.packetDataLength = VP8_PACKET_LENGTH;

    result = VP8Packetizer_GetPacket( &( ctx ),
                                      &( pkt ) );

    while( result != VP8_RESULT_NO_MORE_PACKETS )
    {
        //printf( "Packet Length: %lu.\n", pkt.packetDataLength );
        for( i = 0; i < pkt.packetDataLength; i++ )
        {
            if( i < sizeof( payloadDesc ) )
            {
                if( frameDataIndex == 0 )
                {
                    assert( pkt.pPacketData[ i ] == payloadDescFirstPkt[ i ] );
                }
                else
                {
                    assert( pkt.pPacketData[ i ] == payloadDesc[ i ] );
                }
            }
            else
            {
                assert( pkt.pPacketData[ i ] == frameData[ frameDataIndex ] );
                frameDataIndex++;
            }
        }

        pkt.pPacketData = &( pktBuffer[ 0 ] );
        pkt.packetDataLength = VP8_PACKET_LENGTH;

        result = VP8Packetizer_GetPacket( &( ctx ),
                                          &( pkt ) );

    }

    assert( frameDataIndex == sizeof( frameData ) );
}

/*-----------------------------------------------------------*/

void vp8Packetizer_4()
{
    VP8Result_t result;
    uint8_t frameData[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
                            0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
                            0x20, 0x21, 0x22, 0x23, 0x24, 0x25,
                            0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
                            0x40, 0x41 };
    VP8PacketizerContext_t ctx;
    VP8Frame_t frame;
    uint8_t pktBuffer[ VP8_PACKET_LENGTH ];
    VP8Packet_t pkt;
    uint8_t payloadDescFirstPkt[] = { 0xB0, 0x40, 0xAB };
    uint8_t payloadDesc[] = { 0xA0, 0x40, 0xAB };
    size_t i, frameDataIndex = 0;

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
    assert( result == VP8_RESULT_OK );

    pkt.pPacketData = &( pktBuffer[ 0 ] );
    pkt.packetDataLength = VP8_PACKET_LENGTH;

    result = VP8Packetizer_GetPacket( &( ctx ),
                                      &( pkt ) );

    while( result != VP8_RESULT_NO_MORE_PACKETS )
    {
        //printf( "Packet Length: %lu.\n", pkt.packetDataLength );
        for( i = 0; i < pkt.packetDataLength; i++ )
        {
            if( i < sizeof( payloadDesc ) )
            {
                if( frameDataIndex == 0 )
                {
                    assert( pkt.pPacketData[ i ] == payloadDescFirstPkt[ i ] );
                }
                else
                {
                    assert( pkt.pPacketData[ i ] == payloadDesc[ i ] );
                }
            }
            else
            {
                assert( pkt.pPacketData[ i ] == frameData[ frameDataIndex ] );
                frameDataIndex++;
            }
        }

        pkt.pPacketData = &( pktBuffer[ 0 ] );
        pkt.packetDataLength = VP8_PACKET_LENGTH;

        result = VP8Packetizer_GetPacket( &( ctx ),
                                          &( pkt ) );

    }

    assert( frameDataIndex == sizeof( frameData ) );
}

/*-----------------------------------------------------------*/

void vp8Packetizer_5()
{
    VP8Result_t result;
    uint8_t frameData[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
                            0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
                            0x20, 0x21, 0x22, 0x23, 0x24, 0x25,
                            0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
                            0x40, 0x41 };
    VP8PacketizerContext_t ctx;
    VP8Frame_t frame;
    uint8_t pktBuffer[ VP8_PACKET_LENGTH ];
    VP8Packet_t pkt;
    uint8_t payloadDescFirstPkt[] = { 0xB0, 0xF0, 0xFA, 0xCD, 0xAB, 0xBA };
    uint8_t payloadDesc[] = { 0xA0, 0xF0, 0xFA, 0xCD, 0xAB, 0xBA };
    size_t i, frameDataIndex = 0;

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
    assert( result == VP8_RESULT_OK );

    pkt.pPacketData = &( pktBuffer[ 0 ] );
    pkt.packetDataLength = VP8_PACKET_LENGTH;

    result = VP8Packetizer_GetPacket( &( ctx ),
                                      &( pkt ) );

    while( result != VP8_RESULT_NO_MORE_PACKETS )
    {
        //printf( "Packet Length: %lu.\n", pkt.packetDataLength );
        for( i = 0; i < pkt.packetDataLength; i++ )
        {
            if( i < sizeof( payloadDesc ) )
            {
                if( frameDataIndex == 0 )
                {
                    assert( pkt.pPacketData[ i ] == payloadDescFirstPkt[ i ] );
                }
                else
                {
                    assert( pkt.pPacketData[ i ] == payloadDesc[ i ] );
                }
            }
            else
            {
                assert( pkt.pPacketData[ i ] == frameData[ frameDataIndex ] );
                frameDataIndex++;
            }
        }

        pkt.pPacketData = &( pktBuffer[ 0 ] );
        pkt.packetDataLength = VP8_PACKET_LENGTH;

        result = VP8Packetizer_GetPacket( &( ctx ),
                                          &( pkt ) );

    }

    assert( frameDataIndex == sizeof( frameData ) );
}

/*-----------------------------------------------------------*/

void H264_test_case()
{
    printf( "\nRunning H264 Test Cases\n" );
    h264Packetizer();
    printf( "\tTest 1 - H264 packetize Test to generate packet1 passed\n" );
    h264Packetizer_2();
    printf( "\tTest 2 - H264 packetize Test to generate packet2 passed\n" );
    h264Packetizer_3();
    printf( "\tTest 3 - H264 packetize Test to generate Nalus passed\n" );
}

void VP8_test_case()
{
    printf( "\nRunning VP8 Test Cases\n" );
    vp8Packetizer_1();
    printf( "\tTest 1 - VP8 packetize Test passed\n" );
    vp8Packetizer_2();
    printf( "\tTest 2 - VP8 packetize Test passed\n" );
    vp8Packetizer_3();
    printf( "\tTest 3 - VP8 packetize Test passed\n" );
    vp8Packetizer_4();
    printf( "\tTest 4 - VP8 packetize Test passed\n" );
    vp8Packetizer_5();
    printf( "\tTest 5 - VP8 packetize Test passed\n" );
}

void G711_test_case()
{
    printf( "\nRunning G711 Test Cases\n" );
    g711Packetizer();
    printf( "\tTest 1 - G711 packetize Test passed\n" );
}

void Opus_test_case()
{
    printf( "\nRunning Opus Test Cases\n" );
    OpusPacketizer();
    printf( "\tTest 1 - Opus packetize Test passed\n" );
}

int main( void )
{
    H264_test_case();

    VP8_test_case();

    G711_test_case();

    Opus_test_case();

}

/*-----------------------------------------------------------*/