#ifndef VP8_DEPACKETIZER_H
#define VP8_DEPACKETIZER_H

/* Data types includes. */
#include "vp8_data_types.h"

typedef struct VP8DePacketizerContext
{
    VP8Packet_t * pPacketsArray;
    size_t packetsArrayLength;
    size_t packetCount;
} VP8DepacketizerContext_t;

VP8Result_t VP8Depacketizer_Init( VP8DepacketizerContext_t * pCtx,
                                  VP8Packet_t * pPacketsArray,
                                  size_t packetsArrayLength );

VP8Result_t VP8Depacketizer_AddPacket( VP8DepacketizerContext_t * pCtx,
                                       const VP8Packet_t * pPacket );

VP8Result_t VP8Depacketizer_GetFrame( VP8DepacketizerContext_t * pCtx,
                                      VP8Frame_t * pFrame );

VP8Result_t VP8Depacketizer_GetPacketProperties( const uint8_t * pPacketData,
                                                 const size_t packetDataLength,
                                                 uint32_t * pProperties );

#endif /* VP8_DEPACKETIZER_H */
