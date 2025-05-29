#ifndef H265_DATA_TYPES_H
#define H265_DATA_TYPES_H

/* Standard includes. */
#include <stdint.h>
#include <stddef.h>

/*
 * NAL Unit (NALU) Header for H.265:
 *
 * +---------------+---------------+
 * |0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |F|   Type    |  LayerId  | TID |
 * +-------------+-----------------+
 */
#define NALU_HEADER_SIZE            2

#define NALU_HEADER_F_MASK          0x80
#define NALU_HEADER_F_LOCATION      7

#define NALU_HEADER_TYPE_MASK       0x7E
#define NALU_HEADER_TYPE_LOCATION   1

#define NALU_HEADER_TID_MASK        0x07
#define NALU_HEADER_TID_LOCATION    0

/*-----------------------------------------------------------*/

/*
 * NAL Unit types.
 */
#define SINGLE_NALU_PACKET_TYPE_START    1
#define SINGLE_NALU_PACKET_TYPE_END      47
#define AP_PACKET_TYPE                   48
#define FU_PACKET_TYPE                   49

/*-----------------------------------------------------------*/

/*
 * RTP payload format for FU (Fragmentation Units):
 *
 *  0                   1                   2                   3
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |    PayloadHdr (Type=49)       |   FU header   |               |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+               |
 * |                                                               |
 * |                                                               |
 * |                         FU payload                            |
 * |                                                               |
 * |                               +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                               :...OPTIONAL RTP padding        |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 * PayloadHdr (Same structure as NAL unit header):
 * +---------------+---------------+
 * |0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |F| Type(49)  |  LayerId  | TID |
 * +---------------+---------------+
 *
 * FU header:
 * +---------------+
 * |0|1|2|3|4|5|6|7|
 * +-+-+-+-+-+-+-+-+
 * |S|E|  FuType   |
 * +---------------+
 */
#define FU_PAYLOAD_HEADER_SIZE      2
#define FU_HEADER_OFFSET            2
#define FU_HEADER_SIZE              1

#define FU_HEADER_S_BIT_MASK        0x80
#define FU_HEADER_S_BIT_LOCATION    7

#define FU_HEADER_E_BIT_MASK        0x40
#define FU_HEADER_E_BIT_LOCATION    6

#define FU_HEADER_TYPE_MASK         0x3F
#define FU_HEADER_TYPE_LOCATION     0

/*-----------------------------------------------------------*/

/*
 * RTP payload format for AP (Aggregation Packet):
 *
 *  0                   1                   2                   3
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                          RTP Header                           |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |F| Type (48) |  LayerId  | TID |         NALU 1 Size           |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |          NALU 1 HDR           |                               |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+         NALU 1 Data           |
 * |                   . . .                                       |
 * |                                                               |
 * +               +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  . . .        | NALU 2 Size                   | NALU 2 HDR    |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * | NALU 2 HDR    |                                               |
 * +-+-+-+-+-+-+-+-+              NALU 2 Data                      |
 * |                   . . .                                       |
 * |                               +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                               :...OPTIONAL RTP padding        |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
#define AP_HEADER_SIZE               2 /* Size of the AP payload header. */
#define AP_NALU_LENGTH_FIELD_SIZE    2 /* Size of the NAL unit in bytes. */

/*-----------------------------------------------------------*/

/* Packet properties, used in H265Depacketizer_GetPacketProperties. */
#define H265_PACKET_PROPERTY_START_PACKET    ( 1 << 0 )
#define H265_PACKET_PROPERTY_END_PACKET      ( 1 << 1 )

/*-----------------------------------------------------------*/

#define H265_MIN( a, b )    ( ( a ) < ( b ) ? ( a ) : ( b ) )
#define H265_MAX( a, b )    ( ( a ) > ( b ) ? ( a ) : ( b ) )

/*-----------------------------------------------------------*/

typedef enum H265Result
{
    H265_RESULT_OK,
    H265_RESULT_BAD_PARAM,
    H265_RESULT_OUT_OF_MEMORY,
    H265_RESULT_NO_MORE_PACKETS,
    H265_RESULT_NO_MORE_NALUS,
    H265_RESULT_NO_MORE_FRAMES,
    H265_RESULT_MALFORMED_PACKET,
    H265_RESULT_UNSUPPORTED_PACKET,
    H265_RESULT_BUFFER_TOO_SMALL
} H265Result_t;

typedef enum H265PacketType
{
    H265_PACKET_NONE,
    H265_SINGLE_NALU_PACKET,
    H265_AP_PACKET,
    H265_FU_PACKET
} H265PacketType_t;

/*-----------------------------------------------------------*/

typedef struct H265Packet
{
    uint8_t * pPacketData;
    size_t packetDataLength;
} H265Packet_t;

typedef struct H265Nalu
{
    uint8_t * pNaluData;       /* Pointer to the NAL (header + payload). */
    size_t naluDataLength;     /* Size of NAL in bytes. */
    uint8_t nalUnitType;       /* Such as VPS, SPS, PPS etc. */
    uint8_t temporalId;
} H265Nalu_t;

typedef struct H265Frame
{
    uint8_t * pFrameData;
    size_t frameDataLength;
} H265Frame_t;

/*-----------------------------------------------------------*/

#endif /* H265_DATA_TYPES_H */
