#ifndef H264_DEPACKETIZER_H
#define H264_DEPACKETIZER_H

/* Data types includes. */
#include "h264_data_types.h"

typedef struct H264DePacketizerContext
{
    H264Packet_t * pPacketsArray;
    size_t packetsArrayLength;
    size_t headIndex;
    size_t tailIndex;
    size_t curPacketIndex;
    size_t packetCount;
} H264DepacketizerContext_t;

H264Result_t H264Depacketizer_Init( H264DepacketizerContext_t * pCtx,
                                    H264Packet_t * pPacketsArray,
                                    size_t packetsArrayLength );

H264Result_t H264Depacketizer_AddPacket( H264DepacketizerContext_t * pCtx,
                                         const H264Packet_t * pPacket );

H264Result_t H264Depacketizer_GetNalu( H264DepacketizerContext_t * pCtx,
                                       Nalu_t * pNalu );

/* It assumes that all the packets added using H264Depacketizer_AddPacket are
 * part of the same frame. It also adds start codes to separate NALUs. */
H264Result_t H264Depacketizer_GetFrame( H264DepacketizerContext_t * pCtx,
                                        Frame_t * pFrame );

H264Result_t H264Depacketizer_GetPacketProperties( const uint8_t * pPacketData,
                                                   const size_t packetDataLength,
                                                   uint32_t * pProperties );

#endif /* H264_DEPACKETIZER_H */
