#ifndef OPUS_PACKETIZER_H
#define OPUS_PACKETIZER_H

/* Data types includes. */
#include "opus_data_types.h"

typedef struct OpusPacketizerContext
{
    OpusFrame_t frame;
    size_t curFrameDataIndex;
} OpusPacketizerContext_t;

OpusResult_t OpusPacketizer_Init( OpusPacketizerContext_t * pCtx,
                                  OpusFrame_t * pFrame );

OpusResult_t OpusPacketizer_GetPacket( OpusPacketizerContext_t * pCtx,
                                       OpusPacket_t * pPacket );

#endif /* OPUS_PACKETIZER_H */
