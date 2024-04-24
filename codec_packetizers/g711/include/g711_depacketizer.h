#ifndef G711_DEPACKETIZER_H
#define G711_DEPACKETIZER_H

/* Data types includes. */
#include "g711_data_types.h"

typedef struct G711DePacketizerContext
{
    G711Packet_t * pPacketsArray;
    size_t packetsArrayLength;
    size_t packetCount;
} G711DepacketizerContext_t;

G711Result_t G711Depacketizer_Init( G711DepacketizerContext_t * pCtx,
                                    G711Packet_t * pPacketsArray,
                                    size_t packetsArrayLength );

G711Result_t G711Depacketizer_AddPacket( G711DepacketizerContext_t * pCtx,
                                         const G711Packet_t * pPacket );

G711Result_t G711Depacketizer_GetFrame( G711DepacketizerContext_t * pCtx,
                                        G711Frame_t * pFrame );

G711Result_t G711Depacketizer_GetPacketProperties( const uint8_t * pPacketData,
                                                   const size_t packetDataLength,
                                                   uint32_t * pProperties );

#endif /* G711_DEPACKETIZER_H */
