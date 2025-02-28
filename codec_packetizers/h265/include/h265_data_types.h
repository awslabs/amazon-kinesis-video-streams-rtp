#ifndef H265_DATA_TYPES_H
#define H265_DATA_TYPES_H

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

#define NALU_HEADER_SIZE                      2
#define MAX_DON_DIFF_VALUE                    32767 /* Maximum DON difference allowed (2^15 - 1, per RFC 7798) */

/* NAL unit header field maximum values (as per H.265/HEVC specification) */
#define MAX_NAL_UNIT_TYPE                     63 /* 6 bits (0-63) */
#define MAX_LAYER_ID                          63 /* 6 bits (0-63) */
#define MAX_TEMPORAL_ID                       7  /* 3 bits (0-7) */

/* First byte */
#define HEVC_NALU_HEADER_TYPE_MASK            0x7E
#define HEVC_NALU_HEADER_TYPE_LOCATION        1

#define HEVC_NALU_HEADER_F_MASK               0x80
#define HEVC_NALU_HEADER_F_LOCATION           7

/* Second byte */
#define HEVC_NALU_HEADER_LAYER_ID_MASK        0xF8
#define HEVC_NALU_HEADER_LAYER_ID_LOCATION    3

#define HEVC_NALU_HEADER_TID_MASK             0x07
#define HEVC_NALU_HEADER_TID_LOCATION         0

/*-----------------------------------------------------------*/

/* For RTP packetization (from RFC 7798) */
/* This means NAL unit types 1-47 can be carried in single NAL unit packets */
#define SINGLE_NALU_PACKET_TYPE_START    1  /* First valid NAL unit type */
#define SINGLE_NALU_PACKET_TYPE_END      47 /* Last valid NAL unit type */

/* Special NAL unit types for packetization */
#define AP_PACKET_TYPE                   48 /* Aggregation Packet type */
#define FU_PACKET_TYPE                   49 /* Fragmentation Unit type */



/*****************************************************************/

/*
 * RTP payload format for FU (Fragmentation Units):
 *
 * +---------------+---------------+
 * |0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |F|   Type    |  LayerId  | TID |
 * +---------------+---------------+
 * |S|E|  FuType   |
 * +---------------+
 */
#define FU_HEADER_OFFSET            2
#define FU_HEADER_SIZE              1
#define TOTAL_FU_HEADER_SIZE        3       /* Total header size (NAL + FU) */
#define DONL_FIELD_SIZE             2       /* Size of DONL field when present */

#define FU_HEADER_S_BIT_MASK        0x80
#define FU_HEADER_S_BIT_LOCATION    7

#define FU_HEADER_E_BIT_MASK        0x40
#define FU_HEADER_E_BIT_LOCATION    6

#define FU_HEADER_TYPE_MASK         0x3F
#define FU_HEADER_TYPE_LOCATION     0

/*-----------------------------------------------------------*/

/*
 * RTP payload format for AP (Aggregation Packets):
 *
 * +---------------+---------------+
 * |0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |F|   Type    |  LayerId  | TID |
 * +---------------+---------------+
 * |                               |
 * |      NALU 1 Size             |
 * +---------------+---------------+
 * |                               |
 * |         NALU 1 Data          |
 * +---------------+---------------+
 * |                               |
 * |      NALU 2 Size             |
 * +---------------+---------------+
 * |                               |
 * |         NALU 2 Data          |
 * +---------------+---------------+
 */
#define AP_HEADER_SIZE               2           /* the size of the big box's label */
#define AP_NALU_LENGTH_FIELD_SIZE    2           /* the size of each smaller box's size tag */
#define AP_DONL_SIZE                 2           /* Size of DONL field (if present) */
#define AP_DOND_SIZE                 1           /* Size of DOND field (if present) */

/*-----------------------------------------------------------*/

/* Packet properties */
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
    H265_RESULT_UNSUPPORTED_PACKET
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
    size_t maxPacketSize;
} H265Packet_t;

typedef struct H265Nalu
{
    uint8_t * pNaluData;       /* pointer to the NAL (header + payload) */
    size_t naluDataLength;     /* size of NAL in bytes */
    uint8_t nal_unit_type;     /* (eg. VPS, SPS, PPS) */
    uint8_t nal_layer_id;
    uint8_t temporal_id;
    uint8_t don;
} H265Nalu_t;

typedef struct H265Frame
{
    uint8_t * pFrameData;
    size_t frameDataLength;
} H265Frame_t;

/*-----------------------------------------------------------*/

#endif /* H265_DATA_TYPES_H */
