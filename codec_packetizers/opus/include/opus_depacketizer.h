#ifndef OPUS_DEPACKETIZER_H
#define OPUS_DEPACKETIZER_H

/* Data types includes. */
#include "opus_data_types.h"

typedef struct OpusDePacketizerContext
{
    OpusPacket_t * pPacketsArray;
    size_t packetsArrayLength;
    size_t packetCount;
} OpusDepacketizerContext_t;

OpusResult_t OpusDepacketizer_Init( OpusDepacketizerContext_t * pCtx,
                                    OpusPacket_t * pPacketsArray,
                                    size_t packetsArrayLength );

OpusResult_t OpusDepacketizer_AddPacket( OpusDepacketizerContext_t * pCtx,
                                         const OpusPacket_t * pPacket );

OpusResult_t OpusDepacketizer_GetFrame( OpusDepacketizerContext_t * pCtx,
                                        OpusFrame_t * pFrame );

OpusResult_t OpusDepacketizer_GetPacketProperties( const uint8_t * pPacketData,
                                                   const size_t packetDataLength,
                                                   uint32_t * pProperties );

#endif /* OPUS_DEPACKETIZER_H */
