#ifndef H265_PACKETIZER_H
#define H265_PACKETIZER_H

#include "h265_data_types.h"

/* FU Packetization State */
typedef struct FuPacketizationState
{
    uint16_t payloadHdr;     /* 2 bytes for HEVC */
    uint8_t fuHeader;        /* S, E, and Type bits */
    size_t naluDataIndex;
    size_t remainingNaluLength;
} FuPacketizationState_t;


/* AP Packetization State */
/* Aggregation Unit structure */
typedef struct H265AggregationUnitHeader
{
    uint16_t nalu_size;
    uint16_t nalu_header;
    uint8_t * pPayload;
    size_t payloadLength;
} H265AggregationUnitHeader_t;


/* Main Packetizer Context */
typedef struct H265PacketizerContext
{
    H265Nalu_t * pNaluArray;
    size_t naluArrayLength;
    size_t headIndex;
    size_t tailIndex;
    size_t naluCount;

    /* Current state */
    H265PacketType_t currentlyProcessingPacket;
    FuPacketizationState_t fuPacketizationState;

} H265PacketizerContext_t;

/* Function declarations */
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
