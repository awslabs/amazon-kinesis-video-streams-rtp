#ifndef H264_DATA_TYPES_H
#define H264_DATA_TYPES_H

/* Standard includes. */
#include <stdint.h>
#include <stddef.h>

/*
 * NAL Unit (NALU) Header:
 *
 * +---------------+
 * |0|1|2|3|4|5|6|7|
 * +-+-+-+-+-+-+-+-+
 * |F|NRI|  Type   |
 * +---------------+
 */
#define NALU_HEADER_TYPE_MASK       0x1F
#define NALU_HEADER_TYPE_LOCATION   0

#define NALU_HEADER_NRI_MASK        0x60
#define NALU_HEADER_NRI_LOCATION    5

/*-----------------------------------------------------------*/

/*
 * NAL Unit types.
 */
#define SINGLE_NALU_PACKET_TYPE_START   1
#define SINGLE_NALU_PACKET_TYPE_END     23
#define STAP_A_PACKET_TYPE              24
#define FU_A_PACKET_TYPE                28

/*-----------------------------------------------------------*/

/*
 * RTP payload format for FU-A:
 *
 *  0                   1                   2                   3
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * | FU indicator  |   FU header   |                               |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+                               |
 * |                                                               |
 * |                         FU payload                            |
 * |                                                               |
 * |                               +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                               :...OPTIONAL RTP padding        |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 * FU indicator:
 *
 * +---------------+
 * |0|1|2|3|4|5|6|7|
 * +-+-+-+-+-+-+-+-+
 * |F|NRI|  Type   |
 * +---------------+
 *
 * FU header:
 *
 * +---------------+
 * |0|1|2|3|4|5|6|7|
 * +-+-+-+-+-+-+-+-+
 * |S|E|R|  Type   |
 * +---------------+
 */
#define FU_A_HEADER_SIZE                2

#define FU_A_INDICATOR_OFFSET           0
#define FU_A_HEADER_OFFSET              1
#define FU_A_PAYLOAD_OFFSET             2

#define FU_A_INDICATOR_TYPE_MASK        0x1F
#define FU_A_INDICATOR_TYPE_LOCATION    0

#define FU_A_INDICATOR_NRI_MASK         0x60
#define FU_A_INDICATOR_NRI_LOCATION     5

#define FU_A_HEADER_TYPE_MASK           0x1F
#define FU_A_HEADER_TYPE_LOCATION       0

#define FU_A_HEADER_S_BIT_MASK          0x80
#define FU_A_HEADER_S_BIT_LOCATION      7

#define FU_A_HEADER_E_BIT_MASK          0x40
#define FU_A_HEADER_E_BIT_LOCATION      6

/*-----------------------------------------------------------*/

/*
 * RTP payload format for STAP-A:
 *
 *   0                   1                   2                   3
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |F|NRI|  type   |                                               |
 * +-+-+-+-+-+-+-+-+                                               |
 * |                                                               |
 * |             one or more aggregation units                     |
 * |                                                               |
 * |                               +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                               :...OPTIONAL RTP padding        |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 * Structure of each aggregation unit:
 *
 *   0                   1                   2                   3
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *                 :        NAL unit size          |               |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+               |
 * |                                                               |
 * |                           NAL unit                            |
 * |                                                               |
 * |                               +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                               :
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */

#define STAP_A_HEADER_SIZE              1
#define STAP_A_NALU_SIZE                2

/*-----------------------------------------------------------*/

/* Packet properties, used in H264Depacketizer_GetPacketProperties. */
#define H264_PACKET_PROPERTY_START_PACKET   ( 1 << 0 )
#define H264_PACKET_PROPERTY_END_PACKET     ( 1 << 1 )

/*-----------------------------------------------------------*/

#define H264_MIN( a, b ) ( ( a ) < ( b ) ? ( a ) : ( b ) )
#define H264_MAX( a, b ) ( ( a ) > ( b ) ? ( a ) : ( b ) )

/*-----------------------------------------------------------*/

typedef enum H264Result
{
    H264_RESULT_OK,
    H264_RESULT_BAD_PARAM,
    H264_RESULT_OUT_OF_MEMORY,
    H264_RESULT_NO_MORE_PACKETS,
    H264_RESULT_NO_MORE_NALUS,
    H264_RESULT_NO_MORE_FRAMES,
    H264_RESULT_MALFORMED_PACKET,
    H264_RESULT_UNSUPPORTED_PACKET
} H264Result_t;

typedef enum H264PacketType
{
    H264_PACKET_NONE,
    H264_SINGLE_NALU_PACKET,
    H264_STAP_A_PACKET,
    H264_FU_A_PACKET
} H264PacketType_t;

/*-----------------------------------------------------------*/

typedef struct H264Packet
{
    uint8_t * pPacketData;
    size_t packetDataLength;
} H264Packet_t;

typedef struct Nalu
{
    uint8_t * pNaluData;
    size_t naluDataLength;
} Nalu_t;

typedef struct Frame
{
    uint8_t * pFrameData;
    size_t frameDataLength;
} Frame_t;

/*-----------------------------------------------------------*/

#endif /* H264_DATA_TYPES_H */
