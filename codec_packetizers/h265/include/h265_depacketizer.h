#ifndef H265_DEPACKETIZER_H
#define H265_DEPACKETIZER_H

/* Data types includes. */
#include "h265_data_types.h"

typedef struct H265DePacketizerContext
{
    H265Packet_t * pPacketsArray;
    size_t packetsArrayLength;
    size_t headIndex;
    size_t tailIndex;
    size_t curPacketIndex;
    size_t packetCount;
} H265DepacketizerContext_t;

/* Function declarations. */
H265Result_t H265Depacketizer_Init( H265DepacketizerContext_t * pCtx,
                                    H265Packet_t * pPacketsArray,
                                    size_t packetsArrayLength);

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
