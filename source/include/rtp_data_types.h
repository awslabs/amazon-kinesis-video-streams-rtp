#ifndef RTP_DATA_TYPES_H
#define RTP_DATA_TYPES_H

/* Standard includes. */
#include <stdint.h>
#include <stddef.h>

/* Endianness includes. */
#include "rtp_endianness.h"

#define RTP_HEADER_FLAG_PADDING     ( 1 << 0 )
#define RTP_HEADER_FLAG_MARKER      ( 1 << 1 )
#define RTP_HEADER_FLAG_EXTENSION   ( 1 << 2 )

/*
 * Transport Wide Congestion Control (TWCC) extension:
 *
 * 0                   1                   2                   3
 * 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |       0xBE    |    0xDE       |           Length=1            |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  ID   | L=1   |Transport Wide Sequence Number | Zero Padding  |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 * RFC - https://datatracker.ietf.org/doc/html/draft-holmer-rmcat-transport-wide-cc-extensions-01
 */
#define RTP_EXTENSION_TWCC_PROFILE                  0xBEDE
#define RTP_EXTENSION_TWCC_LENGTH                   1
#define RTP_EXTENSION_TWCC_ID_MASK                  0xF0000000
#define RTP_EXTENSION_TWCC_ID_LOCATION              28
#define RTP_EXTENSION_TWCC_SEQUENCE_NUMBER_MASK     0x00FFFF00
#define RTP_EXTENSION_TWCC_SEQUENCE_NUMBER_LOCATION 8

#define RTP_EXTENSION_TWCC_CREATE_PAYLOAD( id, seq )   \
    ( ( ( id ) << RTP_EXTENSION_TWCC_ID_LOCATION ) & RTP_EXTENSION_TWCC_ID_MASK ) | \
    ( ( 1 << 24 ) ) | \
    ( ( ( seq ) << RTP_EXTENSION_TWCC_SEQUENCE_NUMBER_LOCATION ) & RTP_EXTENSION_TWCC_SEQUENCE_NUMBER_MASK )

#define RTP_EXTENSION_TWCC_GET_ID_FROM_PAYLOAD( payload )   \
    ( ( ( payload ) & RTP_EXTENSION_TWCC_ID_MASK ) >> RTP_EXTENSION_TWCC_ID_LOCATION )

#define RTP_EXTENSION_TWCC_GET_SEQUENCE_NUMBER_FROM_PAYLOAD( payload )   \
    ( ( ( payload ) & RTP_EXTENSION_TWCC_SEQUENCE_NUMBER_MASK ) >> RTP_EXTENSION_TWCC_SEQUENCE_NUMBER_LOCATION )

/*-----------------------------------------------------------*/

typedef enum RtpResult
{
    RTP_RESULT_OK,
    RTP_RESULT_BAD_PARAM,
    RTP_RESULT_OUT_OF_MEMORY,
    RTP_RESULT_WRONG_VERSION,
    RTP_RESULT_MALFORMED_PACKET
} RtpResult_t;

/*-----------------------------------------------------------*/

typedef struct RtpContext
{
    RtpReadWriteFunctions_t readWriteFunctions;
} RtpContext_t;

typedef struct RtpHeaderExtension
{
    uint16_t extensionProfile;
    uint16_t extensionPayloadLength; /* In words. */
    uint32_t * pExtensionPayload;
} RtpHeaderExtension_t;

typedef struct RtpHeader
{
    uint32_t flags;
    uint8_t csrcCount;  /* Contributing Source (CSRC) count. */
    uint8_t payloadType;
    uint16_t sequenceNumber;
    uint32_t timestamp;
    uint32_t ssrc;      /* Synchronization Source (SSRC) identifier. */
    uint32_t * pCsrc;   /* Contributing Source (CSRC) identifiers. */
    RtpHeaderExtension_t extension;
} RtpHeader_t;

typedef struct RtpPacket
{
    RtpHeader_t header;
    uint8_t * pPayload;
    size_t payloadLength;
} RtpPacket_t;

/*-----------------------------------------------------------*/

#endif /* RTP_DATA_TYPES_H */
