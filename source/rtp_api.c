/* Standard includes. */
#include <string.h>

/* API includes. */
#include "rtp_api.h"

/*
 * RTP Packet:
 *
 *  0                   1                   2                   3
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |V=2|P|X|  CC   |M|     PT      |       Sequence Number         |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                           Timestamp                           |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |           Synchronization Source (SSRC) identifier            |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |       Contributing Source (CSRC[0..15]) identifiers           |
 * |                             ....                              |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 * RTP Header extension:
 *
 *  0                   1                   2                   3
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |            Profile            |           Length              |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                        Header Extension                       |
 * |                             ....                              |
 */

#define RTP_HEADER_VERSION                      2
#define RTP_HEADER_VERSION_MASK                 0xC0000000
#define RTP_HEADER_VERSION_LOCATION             30

#define RTP_HEADER_PADDING_MASK                 0x20000000
#define RTP_HEADER_PADDING_LOCATION             29

#define RTP_HEADER_EXTENSION_MASK               0x10000000
#define RTP_HEADER_EXTENSION_LOCATION           28

#define RTP_HEADER_CSRC_COUNT_MASK              0x0F000000
#define RTP_HEADER_CSRC_COUNT_LOCATION          24

#define RTP_HEADER_MARKER_MASK                  0x00800000
#define RTP_HEADER_MARKER_LOCATION              23

#define RTP_HEADER_PAYLOAD_TYPE_MASK            0x007F0000
#define RTP_HEADER_PAYLOAD_TYPE_LOCATION        16

#define RTP_HEADER_SEQUENCE_NUMBER_MASK         0x0000FFFF
#define RTP_HEADER_SEQUENCE_NUMBER_LOCATION     0

#define RTP_EXTENSION_HEADER_PROFILE_MASK       0xFFFF0000
#define RTP_EXTENSION_HEADER_PROFILE_LOCATION   16

#define RTP_EXTENSION_HEADER_LENGTH_MASK        0x0000FFFF
#define RTP_EXTENSION_HEADER_LENGTH_LOCATION    0

#define RTP_HEADER_MIN_LENGTH                   12 /* No CSRC and no extension. */

/* Read, Write macros. */
#define RTP_WRITE_UINT32   ( pCtx->readWriteFunctions.writeUint32Fn )
#define RTP_READ_UINT32    ( pCtx->readWriteFunctions.readUint32Fn )

/*-----------------------------------------------------------*/

static size_t CalculateSerializedPacketLength( const RtpPacket_t * pRtpPacket );

/*-----------------------------------------------------------*/

static size_t CalculateSerializedPacketLength( const RtpPacket_t * pRtpPacket )
{
    size_t headerLength = RTP_HEADER_MIN_LENGTH +
                          ( pRtpPacket->header.csrcCount * sizeof( uint32_t ) );

    if( ( pRtpPacket->header.flags & RTP_HEADER_FLAG_EXTENSION ) != 0 )
    {
        headerLength = headerLength +
                       4 + /* Extension header. */
                       ( pRtpPacket->header.extension.extensionPayloadLength * sizeof( uint32_t ) );
    }

    return headerLength + pRtpPacket->payloadLength;
}

/*-----------------------------------------------------------*/

RtpResult_t Rtp_Init( RtpContext_t * pCtx )
{
    RtpResult_t result = RTP_RESULT_OK;

    if( pCtx == NULL )
    {
        result = RTP_RESULT_BAD_PARAM;
    }

    if( result == RTP_RESULT_OK )
    {
        Rtp_InitReadWriteFunctions( &( pCtx->readWriteFunctions ) );
    }

    return result;
}

/*-----------------------------------------------------------*/

RtpResult_t Rtp_Serialize( RtpContext_t * pCtx,
                           const RtpPacket_t * pRtpPacket,
                           uint8_t * pBuffer,
                           size_t * pLength )
{
    size_t i, serializedPacketLength, currentIndex = 0;
    uint32_t firstWord, extensionHeader;
    RtpResult_t result = RTP_RESULT_OK;
    uint8_t paddingByte;

    if( ( pCtx == NULL ) ||
        ( pRtpPacket == NULL ) ||
        ( pLength == NULL ) )
    {
        result = RTP_RESULT_BAD_PARAM;
    }

    if( result == RTP_RESULT_OK )
    {
        serializedPacketLength = CalculateSerializedPacketLength( pRtpPacket );

        if( ( pBuffer != NULL ) &&
            ( *pLength < serializedPacketLength ) )
        {
            result = RTP_RESULT_OUT_OF_MEMORY;
        }
        else
        {
            *pLength = serializedPacketLength;
        }
    }

    if( ( result == RTP_RESULT_OK ) &&
        ( pBuffer != NULL ) )
    {
        firstWord = ( RTP_HEADER_VERSION << RTP_HEADER_VERSION_LOCATION );

        /* Calculate if padding is needed. */
        if( ( pRtpPacket->header.flags & RTP_HEADER_FLAG_PADDING ) != 0 )
        {
            firstWord |= ( 1 << RTP_HEADER_PADDING_LOCATION );
        }

        if( ( pRtpPacket->header.flags & RTP_HEADER_FLAG_EXTENSION ) != 0 )
        {
            firstWord |= ( 1 << RTP_HEADER_EXTENSION_LOCATION );
        }

        firstWord |= ( ( ( uint32_t ) pRtpPacket->header.csrcCount <<
                         RTP_HEADER_CSRC_COUNT_LOCATION ) &
                       RTP_HEADER_CSRC_COUNT_MASK );

        if( ( pRtpPacket->header.flags & RTP_HEADER_FLAG_MARKER ) != 0 )
        {
            firstWord |= ( 1 << RTP_HEADER_MARKER_LOCATION );
        }

        firstWord |= ( ( ( uint32_t ) pRtpPacket->header.payloadType <<
                         RTP_HEADER_PAYLOAD_TYPE_LOCATION ) &
                       RTP_HEADER_PAYLOAD_TYPE_MASK );

        firstWord |= ( ( ( uint32_t ) pRtpPacket->header.sequenceNumber <<
                         RTP_HEADER_SEQUENCE_NUMBER_LOCATION ) &
                       RTP_HEADER_SEQUENCE_NUMBER_MASK );

        RTP_WRITE_UINT32( &( pBuffer[ currentIndex ] ),
                          firstWord );
        currentIndex += 4;

        RTP_WRITE_UINT32( &( pBuffer[ currentIndex ] ),
                          pRtpPacket->header.timestamp );
        currentIndex += 4;

        RTP_WRITE_UINT32( &( pBuffer[ currentIndex ] ),
                          pRtpPacket->header.ssrc );
        currentIndex += 4;

        for( i = 0; i < pRtpPacket->header.csrcCount; i++ )
        {
            RTP_WRITE_UINT32( &( pBuffer[ currentIndex ] ),
                              pRtpPacket->header.pCsrc[ i ] );
            currentIndex += 4;
        }

        if( ( pRtpPacket->header.flags & RTP_HEADER_FLAG_EXTENSION ) != 0 )
        {
            extensionHeader = ( ( ( uint32_t ) pRtpPacket->header.extension.extensionProfile <<
                                  RTP_EXTENSION_HEADER_PROFILE_LOCATION ) &
                                RTP_EXTENSION_HEADER_PROFILE_MASK );

            extensionHeader |= ( ( ( uint32_t ) pRtpPacket->header.extension.extensionPayloadLength <<
                                   RTP_EXTENSION_HEADER_LENGTH_LOCATION ) &
                                 RTP_EXTENSION_HEADER_LENGTH_MASK );

            RTP_WRITE_UINT32( &( pBuffer[ currentIndex ] ),
                              extensionHeader );
            currentIndex += 4;

            for( i = 0; i < pRtpPacket->header.extension.extensionPayloadLength; i++ )
            {
                RTP_WRITE_UINT32( &( pBuffer[ currentIndex ] ),
                                  pRtpPacket->header.extension.pExtensionPayload[ i ] );
                currentIndex += 4;
            }
        }

        if( ( pRtpPacket->pPayload != NULL ) &&
            ( pRtpPacket->payloadLength > 0 ) )
        {
            /* If padding is enabled, check the padding length at the end of payload. */
            if( ( pRtpPacket->header.flags & RTP_HEADER_FLAG_PADDING ) != 0 )
            {
                paddingByte = pRtpPacket->pPayload[ pRtpPacket->payloadLength - 1 ];
                if( paddingByte > pRtpPacket->payloadLength )
                {
                    /* It doesn't make sense to have a packet that the length padding byte is larger than
                     * the payload length. */
                    result = RTP_RESULT_MALFORMED_PACKET;
                }
            }

            if( result == RTP_RESULT_OK )
            {
                memcpy( ( void * ) &( pBuffer[ currentIndex ] ),
                        ( const void * ) &( pRtpPacket->pPayload[ 0 ] ),
                        pRtpPacket->payloadLength );
            }
        }
    }

    return result;
}

/*-----------------------------------------------------------*/

RtpResult_t Rtp_DeSerialize( RtpContext_t * pCtx,
                             uint8_t * pSerializedPacket,
                             size_t serializedPacketLength,
                             RtpPacket_t * pRtpPacket )
{
    size_t i, currentIndex = 0;
    uint32_t firstWord, extensionHeader, word;
    RtpResult_t result = RTP_RESULT_OK;
    uint8_t readByte;

    if( ( pCtx == NULL ) ||
        ( pSerializedPacket == NULL ) ||
        ( serializedPacketLength < RTP_HEADER_MIN_LENGTH ) ||
        ( pRtpPacket == NULL ) )
    {
        result = RTP_RESULT_BAD_PARAM;
    }

    if( result == RTP_RESULT_OK )
    {
        firstWord = RTP_READ_UINT32( &( pSerializedPacket[ currentIndex ] ) );
        currentIndex += 4;

        if( ( ( firstWord & RTP_HEADER_VERSION_MASK ) >>
              RTP_HEADER_VERSION_LOCATION ) != RTP_HEADER_VERSION )
        {
            result = RTP_RESULT_WRONG_VERSION;
        }
    }

    if( result == RTP_RESULT_OK )
    {
        pRtpPacket->header.flags = 0;

        if( ( firstWord & RTP_HEADER_PADDING_MASK ) != 0 )
        {
            pRtpPacket->header.flags |= RTP_HEADER_FLAG_PADDING;
        }

        pRtpPacket->header.csrcCount = ( firstWord & RTP_HEADER_CSRC_COUNT_MASK ) >>
                                       RTP_HEADER_CSRC_COUNT_LOCATION;

        if( ( firstWord & RTP_HEADER_MARKER_MASK ) != 0 )
        {
            pRtpPacket->header.flags |= RTP_HEADER_FLAG_MARKER;
        }

        pRtpPacket->header.payloadType = ( firstWord & RTP_HEADER_PAYLOAD_TYPE_MASK ) >>
                                         RTP_HEADER_PAYLOAD_TYPE_LOCATION;
        pRtpPacket->header.sequenceNumber = ( firstWord & RTP_HEADER_SEQUENCE_NUMBER_MASK ) >>
                                            RTP_HEADER_SEQUENCE_NUMBER_LOCATION;

        pRtpPacket->header.timestamp = RTP_READ_UINT32( &( pSerializedPacket[ currentIndex ] ) );
        currentIndex += 4;

        pRtpPacket->header.ssrc = RTP_READ_UINT32( &( pSerializedPacket[ currentIndex ] ) );
        currentIndex += 4;

        if( pRtpPacket->header.csrcCount > 0 )
        {
            /* Is there enough data to read CSRCs? */
            if( ( currentIndex + ( pRtpPacket->header.csrcCount * sizeof( uint32_t ) ) ) <= serializedPacketLength )
            {
                pRtpPacket->header.pCsrc = ( uint32_t * ) &( pSerializedPacket[ currentIndex ] );

                for( i = 0; i < pRtpPacket->header.csrcCount; i++ )
                {
                    word = RTP_READ_UINT32( &( pSerializedPacket[ currentIndex ] ) );
                    currentIndex += 4;

                    pRtpPacket->header.pCsrc[ i ] = word;
                }
            }
            else
            {
                result = RTP_RESULT_MALFORMED_PACKET;
            }
        }
        else
        {
            pRtpPacket->header.pCsrc = NULL;
        }
    }

    if( result == RTP_RESULT_OK )
    {
        if( ( firstWord & RTP_HEADER_EXTENSION_MASK ) != 0 )
        {
            pRtpPacket->header.flags |= RTP_HEADER_FLAG_EXTENSION;

            /* Is there enough data to read extension header? */
            if( ( currentIndex + sizeof( uint32_t ) ) <= serializedPacketLength )
            {
                extensionHeader = RTP_READ_UINT32( &( pSerializedPacket[ currentIndex ] ) );
                currentIndex += 4;

                pRtpPacket->header.extension.extensionProfile = ( extensionHeader &
                                                                  RTP_EXTENSION_HEADER_PROFILE_MASK ) >>
                                                                RTP_EXTENSION_HEADER_PROFILE_LOCATION;
                pRtpPacket->header.extension.extensionPayloadLength = ( extensionHeader &
                                                                        RTP_EXTENSION_HEADER_LENGTH_MASK ) >>
                                                                      RTP_EXTENSION_HEADER_LENGTH_LOCATION;
            }
            else
            {
                result = RTP_RESULT_MALFORMED_PACKET;
            }

            if( result == RTP_RESULT_OK )
            {
                /* Is there enough data to read extension payload? */
                if( ( currentIndex +
                      ( pRtpPacket->header.extension.extensionPayloadLength *
                        sizeof( uint32_t ) ) ) <= serializedPacketLength )
                {
                    pRtpPacket->header.extension.pExtensionPayload = ( uint32_t * ) &( pSerializedPacket[ currentIndex ] );

                    for( i = 0; i < pRtpPacket->header.extension.extensionPayloadLength; i++ )
                    {
                        word = RTP_READ_UINT32( &( pSerializedPacket[ currentIndex ] ) );
                        currentIndex += 4;

                        pRtpPacket->header.extension.pExtensionPayload[ i ] = word;
                    }
                }
                else
                {
                    result = RTP_RESULT_MALFORMED_PACKET;
                }
            }
        }
    }

    if( result == RTP_RESULT_OK )
    {
        if( currentIndex < serializedPacketLength )
        {
            pRtpPacket->pPayload = &( pSerializedPacket[ currentIndex ] );
            pRtpPacket->payloadLength = ( serializedPacketLength - currentIndex );

            if( ( pRtpPacket->header.flags & RTP_HEADER_FLAG_PADDING ) != 0 )
            {
                /* From RFC3550, section 5.1: The last octet of the padding contains a count of how
                 * many padding octets should be ignored, including itself. */
                readByte = pRtpPacket->pPayload[ pRtpPacket->payloadLength - 1 ];
                if( readByte <= pRtpPacket->payloadLength )
                {
                    pRtpPacket->payloadLength -= readByte;
                }
                else
                {
                    /* It doesn't make sense to have a packet that the length padding byte is larger than
                     * the payload length. */
                    result = RTP_RESULT_MALFORMED_PACKET;
                }
            }
        }
        else
        {
            pRtpPacket->pPayload = NULL;
            pRtpPacket->payloadLength = 0;
        }
    }

    return result;
}

/*-----------------------------------------------------------*/
