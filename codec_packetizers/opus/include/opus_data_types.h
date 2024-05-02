#ifndef OPUS_DATA_TYPES_H
#define OPUS_DATA_TYPES_H

/* Standard includes. */
#include <stdint.h>
#include <stddef.h>

/* Packet properties, used in OPUSDepacketizer_GetPacketProperties. */
#define OPUS_PACKET_PROPERTY_START_PACKET   ( 1 << 0 )

/*-----------------------------------------------------------*/

#define OPUS_MIN( a, b ) ( ( a ) < ( b ) ? ( a ) : ( b ) )
#define OPUS_MAX( a, b ) ( ( a ) > ( b ) ? ( a ) : ( b ) )

/*-----------------------------------------------------------*/

typedef enum OpusResult
{
    OPUS_RESULT_OK,
    OPUS_RESULT_BAD_PARAM,
    OPUS_RESULT_OUT_OF_MEMORY,
    OPUS_RESULT_NO_MORE_PACKETS
} OpusResult_t;

/*-----------------------------------------------------------*/

typedef struct OpusPacket
{
    uint8_t * pPacketData;
    size_t packetDataLength;
} OpusPacket_t;

typedef struct OpusFrame
{
    uint8_t * pFrameData;
    size_t frameDataLength;
} OpusFrame_t;

/*-----------------------------------------------------------*/

#endif /* OPUS_DATA_TYPES_H */
