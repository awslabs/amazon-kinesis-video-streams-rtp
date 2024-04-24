#ifndef VP8_PACKETIZER_H
#define VP8_PACKETIZER_H

/* Data types includes. */
#include "vp8_data_types.h"

typedef struct VP8PacketizerContext
{
    uint8_t payloadDesc[ VP8_PAYLOAD_DESC_MAX_LENGTH ];
    size_t payloadDescLength;
    uint8_t * pFrameData;
    size_t frameDataLength;
    size_t curFrameDataIndex;
} VP8PacketizerContext_t;

VP8Result_t VP8Packetizer_Init( VP8PacketizerContext_t * pCtx,
                                VP8Frame_t * pFrame );

VP8Result_t VP8Packetizer_GetPacket( VP8PacketizerContext_t * pCtx,
                                     VP8Packet_t * pPacket );

#endif /* VP8_PACKETIZER_H */
