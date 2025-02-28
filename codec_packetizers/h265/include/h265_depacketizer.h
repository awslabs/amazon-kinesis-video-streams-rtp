#ifndef H265_DEPACKETIZER_H
#define H265_DEPACKETIZER_H

#include "h265_data_types.h"
#include "h265_packetizer.h"

/* FU Depacketization State */
typedef struct FuDepacketizationState
{
    uint16_t payloadHdr;     /* 2 bytes for HEVC */
    uint8_t fuHeader;        /* S, E, and Type bits */
    uint8_t originalNalType; /* Original NAL unit type */
    size_t reassembledLength;
    uint16_t donl;           /* For DON handling if needed */
    uint8_t * pReassemblyBuffer;
    size_t reassemblyBufferSize;
} FuDepacketizationState_t;

/* AP Depacketization State */
typedef struct ApDepacketizationState
{
    uint16_t payloadHdr;                 /* Type 48 for AP */
    size_t currentUnitIndex;
    uint8_t naluCount;                   /* Number of NALUs in AP */
    H265AggregationUnitHeader_t * units; /* Array of aggregation units */
    size_t currentOffset;            
    uint8_t firstUnit;               
} ApDepacketizationState_t;

/* Main Depacketizer Context */
typedef struct H265DePacketizerContext
{
    H265Packet_t * pPacketsArray;
    size_t packetsArrayLength;
    size_t headIndex;
    size_t tailIndex;
    size_t curPacketIndex;
    size_t packetCount;

    /* Current state */
    H265PacketType_t currentlyProcessingPacket;
    FuDepacketizationState_t fuDepacketizationState;
    ApDepacketizationState_t apDepacketizationState;

    /* DON tracking */
    uint16_t currentDon;
    uint8_t donPresent;        /* If DON fields are present (1=present, 0=not present) */
} H265DepacketizerContext_t;

/* Function declarations */
H265Result_t H265Depacketizer_Init( H265DepacketizerContext_t * pCtx,
                                    H265Packet_t * pPacketsArray,
                                    size_t packetsArrayLength,
                                    uint8_t donPresent );

H265Result_t H265Depacketizer_AddPacket( H265DepacketizerContext_t * pCtx,
                                         const H265Packet_t * pPacket );

H265Result_t H265Depacketizer_GetNalu( H265DepacketizerContext_t * pCtx,
                                       H265Nalu_t * pNalu );

H265Result_t H265Depacketizer_GetFrame( H265DepacketizerContext_t * pCtx,
                                        H265Frame_t * pFrame );

H265Result_t H265Depacketizer_GetPacketProperties( const uint8_t * pPacketData,
                                                   const size_t packetDataLength,
                                                   uint32_t * pProperties );

#endif /* H265_DEPACKETIZER_H */
