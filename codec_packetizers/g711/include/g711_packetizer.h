#ifndef G711_PACKETIZER_H
#define G711_PACKETIZER_H

/* Data types includes. */
#include "g711_data_types.h"

typedef struct G711PacketizerContext
{
    G711Frame_t frame;
    size_t curFrameDataIndex;
} G711PacketizerContext_t;

G711Result_t G711Packetizer_Init( G711PacketizerContext_t * pCtx,
                                  G711Frame_t * pFrame );

G711Result_t G711Packetizer_GetPacket( G711PacketizerContext_t * pCtx,
                                       G711Packet_t * pPacket );

#endif /* G711_PACKETIZER_H */
