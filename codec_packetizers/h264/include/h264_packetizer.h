#ifndef H264_PACKETIZER_H
#define H264_PACKETIZER_H

/* Data types includes. */
#include "h264_data_types.h"

typedef struct FuAPacketizationState
{
    uint8_t naluHeader;
    size_t naluDataIndex;
    size_t remainingNaluLength;
} FuAPacketizationState_t;

typedef struct H264PacketizerContext
{
    Nalu_t * pNaluArray;
    size_t naluArrayLength;
    size_t headIndex;
    size_t tailIndex;
    size_t naluCount;
    H264PacketType_t currentlyProcessingPacket;
    FuAPacketizationState_t fuAPacketizationState;
} H264PacketizerContext_t;

H264Result_t H264Packetizer_Init( H264PacketizerContext_t * pCtx,
                                  Nalu_t * pNaluArray,
                                  size_t naluArrayLength );

/* A frame comprising of multiple NALUs separated by start codes. */
H264Result_t H264Packetizer_AddFrame( H264PacketizerContext_t * pCtx,
                                      Frame_t * pFrame );

H264Result_t H264Packetizer_AddNalu( H264PacketizerContext_t * pCtx,
                                     Nalu_t * pNalu );

H264Result_t H264Packetizer_GetPacket( H264PacketizerContext_t * pCtx,
                                       H264Packet_t * pPacket );

#endif /* H264_PACKETIZER_H */
