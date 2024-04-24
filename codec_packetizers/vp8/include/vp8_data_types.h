#ifndef VP8_DATA_TYPES_H
#define VP8_DATA_TYPES_H

/* Standard includes. */
#include <stdint.h>
#include <stddef.h>

/*
 * VP8 Payload Descriptor:
 *
 *        0 1 2 3 4 5 6 7                      0 1 2 3 4 5 6 7
 *       +-+-+-+-+-+-+-+-+                   +-+-+-+-+-+-+-+-+
 *       |X|R|N|S|R| PID | (REQUIRED)        |X|R|N|S|R| PID | (REQUIRED)
 *       +-+-+-+-+-+-+-+-+                   +-+-+-+-+-+-+-+-+
 *  X:   |I|L|T|K| RSV   | (OPTIONAL)   X:   |I|L|T|K| RSV   | (OPTIONAL)
 *       +-+-+-+-+-+-+-+-+                   +-+-+-+-+-+-+-+-+
 *  I:   |M| PictureID   | (OPTIONAL)   I:   |M| PictureID   | (OPTIONAL)
 *       +-+-+-+-+-+-+-+-+                   +-+-+-+-+-+-+-+-+
 *  L:   |   TL0PICIDX   | (OPTIONAL)        |   PictureID   |
 *       +-+-+-+-+-+-+-+-+                   +-+-+-+-+-+-+-+-+
 *  T/K: |TID|Y| KEYIDX  | (OPTIONAL)   L:   |   TL0PICIDX   | (OPTIONAL)
 *       +-+-+-+-+-+-+-+-+                   +-+-+-+-+-+-+-+-+
 *                                      T/K: |TID|Y| KEYIDX  | (OPTIONAL)
 *                                           +-+-+-+-+-+-+-+-+
 */
#define VP8_PAYLOAD_DESC_MAX_LENGTH             6
#define VP8_PAYLOAD_DESC_HEADER_OFFSET          0
#define VP8_PAYLOAD_DESC_EXT_OFFSET             1

#define VP8_PAYLOAD_DESC_X_BITMASK              0x80
#define VP8_PAYLOAD_DESC_X_LOCATION             7

#define VP8_PAYLOAD_DESC_N_BITMASK              0x20
#define VP8_PAYLOAD_DESC_N_LOCATION             5

#define VP8_PAYLOAD_DESC_S_BITMASK              0x10
#define VP8_PAYLOAD_DESC_S_LOCATION             4

#define VP8_PAYLOAD_DESC_PID_BITMASK            0x07
#define VP8_PAYLOAD_DESC_PID_LOCATION           0

#define VP8_PAYLOAD_DESC_EXT_I_BITMASK          0x80
#define VP8_PAYLOAD_DESC_EXT_I_LOCATION         7

#define VP8_PAYLOAD_DESC_EXT_L_BITMASK          0x40
#define VP8_PAYLOAD_DESC_EXT_L_LOCATION         6

#define VP8_PAYLOAD_DESC_EXT_T_BITMASK          0x20
#define VP8_PAYLOAD_DESC_EXT_T_LOCATION         5

#define VP8_PAYLOAD_DESC_EXT_K_BITMASK          0x10
#define VP8_PAYLOAD_DESC_EXT_K_LOCATION         4

#define VP8_PAYLOAD_DESC_EXT_M_BITMASK          0x80
#define VP8_PAYLOAD_DESC_EXT_M_LOCATION         7

#define VP8_PAYLOAD_DESC_EXT_TID_BITMASK        0xE0
#define VP8_PAYLOAD_DESC_EXT_TID_LOCATION       5

#define VP8_PAYLOAD_DESC_EXT_Y_BITMASK          0x10
#define VP8_PAYLOAD_DESC_EXT_Y_LOCATION         4

#define VP8_PAYLOAD_DESC_EXT_KEYIDX_BITMASK     0x0F
#define VP8_PAYLOAD_DESC_EXT_KEYIDX_LOCATION    0

/* Frame Properties. */
#define VP8_FRAME_PROP_NON_REF_FRAME            ( 1 << 0 )
#define VP8_FRAME_PROP_PICTURE_ID_PRESENT       ( 1 << 1 )
#define VP8_FRAME_PROP_TL0PICIDX_PRESENT        ( 1 << 2 )
#define VP8_FRAME_PROP_TID_PRESENT              ( 1 << 3 )
#define VP8_FRAME_PROP_KEYIDX_PRESENT           ( 1 << 4 )
#define VP8_FRAME_PROP_DEPENDS_ON_BASE_ONLY     ( 1 << 5 ) /* Y bit in TID/Y/KEYIDX extension is set. */

/* Packet properties, used in VP8Depacketizer_GetPacketProperties. */
#define VP8_PACKET_PROP_START_PACKET            ( 1 << 0 )

/*-----------------------------------------------------------*/

#define VP8_MIN( a, b ) ( ( a ) < ( b ) ? ( a ) : ( b ) )
#define VP8_MAX( a, b ) ( ( a ) > ( b ) ? ( a ) : ( b ) )

/*-----------------------------------------------------------*/

typedef enum VP8Result
{
    VP8_RESULT_OK,
    VP8_RESULT_BAD_PARAM,
    VP8_RESULT_OUT_OF_MEMORY,
    VP8_RESULT_NO_MORE_PACKETS,
    VP8_MALFORMED_PACKET
} VP8Result_t;

/*-----------------------------------------------------------*/

typedef struct VP8Packet
{
    uint8_t * pPacketData;
    size_t packetDataLength;
} VP8Packet_t;

typedef struct VP8Frame
{
    uint32_t frameProperties;
    uint16_t pictureId;
    uint8_t tl0PicIndex;
    uint8_t tid;
    uint8_t keyIndex;
    uint8_t * pFrameData;
    size_t frameDataLength;
} VP8Frame_t;

/*-----------------------------------------------------------*/

#endif /* VP8_DATA_TYPES_H */
