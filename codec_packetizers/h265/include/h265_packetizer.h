#ifndef H265_PACKETIZER_H
#define H265_PACKETIZER_H

/* Data types includes. */
#include "h265_data_types.h"

typedef struct FuPacketizationState
{
    uint8_t payloadHeader[ FU_PAYLOAD_HEADER_SIZE ];
    uint8_t fuHeader;
    size_t naluDataIndex;
    size_t remainingNaluLength;
} FuPacketizationState_t;

typedef struct H265PacketizerContext
{
    H265Nalu_t * pNaluArray;
    size_t naluArrayLength;
    size_t headIndex;
    size_t tailIndex;
    size_t naluCount;

    H265PacketType_t currentlyProcessingPacket;
    FuPacketizationState_t fuPacketizationState;
} H265PacketizerContext_t;

/* Function declarations. */
H265Result_t H265Packetizer_Init( H265PacketizerContext_t * pCtx,
                                  H265Nalu_t * pNaluArray,
                                  size_t naluArrayLength);

H265Result_t H265Packetizer_AddFrame( H265PacketizerContext_t * pCtx,
                                      H265Frame_t * pFrame );

H265Result_t H265Packetizer_AddNalu( H265PacketizerContext_t * pCtx,
                                     H265Nalu_t * pNalu );

H265Result_t H265Packetizer_GetPacket( H265PacketizerContext_t * pCtx,
                                       H265Packet_t * pPacket );

#endif /* H265_PACKETIZER_H */
