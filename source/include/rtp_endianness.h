#ifndef RTP_ENDIANNESS_H
#define RTP_ENDIANNESS_H

/* Standard includes. */
#include <stdint.h>

/* Endianness Function types. */
typedef void ( * WriteUint32_t ) ( uint8_t * pDst,
                                   uint32_t val );
typedef uint32_t ( * ReadUint32_t ) ( const uint8_t * pSrc );

typedef struct RtpReadWriteFunctions
{
    WriteUint32_t writeUint32Fn;
    ReadUint32_t readUint32Fn;
} RtpReadWriteFunctions_t;

void Rtp_InitReadWriteFunctions( RtpReadWriteFunctions_t * pReadWriteFunctions );

#endif /* RTP_ENDIANNESS_H */
