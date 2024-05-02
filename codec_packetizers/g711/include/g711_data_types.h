#ifndef G711_DATA_TYPES_H
#define G711_DATA_TYPES_H

/* Standard includes. */
#include <stdint.h>
#include <stddef.h>

/* Packet properties, used in G711Depacketizer_GetPacketProperties. */
#define G711_PACKET_PROPERTY_START_PACKET   ( 1 << 0 )

/*-----------------------------------------------------------*/

#define G711_MIN( a, b ) ( ( a ) < ( b ) ? ( a ) : ( b ) )
#define G711_MAX( a, b ) ( ( a ) > ( b ) ? ( a ) : ( b ) )

/*-----------------------------------------------------------*/

typedef enum G711Result
{
    G711_RESULT_OK,
    G711_RESULT_BAD_PARAM,
    G711_RESULT_OUT_OF_MEMORY,
    G711_RESULT_NO_MORE_PACKETS
} G711Result_t;

/*-----------------------------------------------------------*/

typedef struct G711Packet
{
    uint8_t * pPacketData;
    size_t packetDataLength;
} G711Packet_t;

typedef struct G711Frame
{
    uint8_t * pFrameData;
    size_t frameDataLength;
} G711Frame_t;

/*-----------------------------------------------------------*/

#endif /* G711_DATA_TYPES_H */
