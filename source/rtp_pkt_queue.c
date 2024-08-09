/* Standard includes. */
#include <string.h>

/* API includes. */
#include "rtp_pkt_queue.h"

/*----------------------------------------------------------------------------*/

#define WRAP( x, n ) \
    ( ( x ) % ( n ) )

#define INC_READ_INDEX( pQueue )        \
    WRAP( ( pQueue )->readIndex + 1,    \
          ( pQueue )->rtpPacketInfoArrayLength )

#define INC_WRITE_INDEX( pQueue )       \
    WRAP( ( pQueue )->writeIndex + 1,   \
          ( pQueue )->rtpPacketInfoArrayLength )

#define IS_QUEUE_FULL( pQueue ) \
    ( ( pQueue )->packetCount == ( pQueue )->rtpPacketInfoArrayLength )

#define IS_QUEUE_EMPTY( pQueue ) \
    ( ( pQueue )->packetCount == 0 )

/*----------------------------------------------------------------------------*/

RtpPacketQueueResult_t RtpPacketQueue_Init( RtpPacketQueue_t * pQueue,
                                            RtpPacketInfo_t * pRtpPacketInfoArray,
                                            size_t rtpPacketInfoArrayLength )
{
    RtpPacketQueueResult_t result = RTP_PACKET_QUEUE_RESULT_OK;

    if( ( pQueue == NULL ) ||
        ( pRtpPacketInfoArray == NULL ) ||
        ( rtpPacketInfoArrayLength < 2 ) )
    {
        result = RTP_PACKET_QUEUE_RESULT_BAD_PARAM;
    }

    if( result == RTP_PACKET_QUEUE_RESULT_OK )
    {
        pQueue->pRtpPacketInfoArray = pRtpPacketInfoArray;
        pQueue->rtpPacketInfoArrayLength = rtpPacketInfoArrayLength;

        memset( pQueue->pRtpPacketInfoArray,
                0,
                sizeof( RtpPacketInfo_t ) * pQueue->rtpPacketInfoArrayLength );

        pQueue->readIndex = 0;
        pQueue->writeIndex = 0;
        pQueue->packetCount = 0;
    }

    return result;
}

/*----------------------------------------------------------------------------*/

/**
 * @brief Add an RTP packet info into the specified queue.
 */
RtpPacketQueueResult_t RtpPacketQueue_Enqueue( RtpPacketQueue_t * pQueue,
                                               const RtpPacketInfo_t * pRtpPacketInfo )
{
    RtpPacketQueueResult_t result = RTP_PACKET_QUEUE_RESULT_OK;

    if( ( pQueue == NULL ) ||
        ( pRtpPacketInfo == NULL ) )
    {
        result = RTP_PACKET_QUEUE_RESULT_BAD_PARAM;
    }

    if( result == RTP_PACKET_QUEUE_RESULT_OK )
    {
        if( IS_QUEUE_FULL( pQueue ) )
        {
            result = RTP_PACKET_QUEUE_RESULT_FULL;
        }
    }

    if( result == RTP_PACKET_QUEUE_RESULT_OK )
    {
        pQueue->pRtpPacketInfoArray[ pQueue->writeIndex ].seqNum = pRtpPacketInfo->seqNum;
        pQueue->pRtpPacketInfoArray[ pQueue->writeIndex ].pSerializedRtpPacket = pRtpPacketInfo->pSerializedRtpPacket;
        pQueue->pRtpPacketInfoArray[ pQueue->writeIndex ].serializedPacketLength = pRtpPacketInfo->serializedPacketLength;

        pQueue->writeIndex = INC_WRITE_INDEX( pQueue );
        pQueue->packetCount += 1;
    }

    return result;
}

/*----------------------------------------------------------------------------*/

/**
 * @brief Add an RTP packet info into the specified queue.
 *
 * Delete the oldest RTP packet info and return it, if the queue is full.
 */
RtpPacketQueueResult_t RtpPacketQueue_ForceEnqueue( RtpPacketQueue_t * pQueue,
                                                    const RtpPacketInfo_t * pRtpPacketInfo,
                                                    RtpPacketInfo_t * pDeletedRtpPacketInfo )
{
    RtpPacketQueueResult_t result = RTP_PACKET_QUEUE_RESULT_OK;

    if( ( pQueue == NULL ) ||
        ( pRtpPacketInfo == NULL ) ||
        ( pDeletedRtpPacketInfo == NULL ) )
    {
        result = RTP_PACKET_QUEUE_RESULT_BAD_PARAM;
    }

    if( result == RTP_PACKET_QUEUE_RESULT_OK )
    {
        if( IS_QUEUE_FULL( pQueue ) )
        {
            pDeletedRtpPacketInfo->seqNum = pQueue->pRtpPacketInfoArray[ pQueue->readIndex ].seqNum;
            pDeletedRtpPacketInfo->pSerializedRtpPacket = pQueue->pRtpPacketInfoArray[ pQueue->readIndex ].pSerializedRtpPacket;
            pDeletedRtpPacketInfo->serializedPacketLength = pQueue->pRtpPacketInfoArray[ pQueue->readIndex ].serializedPacketLength;

            pQueue->readIndex = INC_READ_INDEX( pQueue );
            pQueue->packetCount -= 1;
            result = RTP_PACKET_QUEUE_RESULT_PACKET_DELETED;
        }

        pQueue->pRtpPacketInfoArray[ pQueue->writeIndex ].seqNum = pRtpPacketInfo->seqNum;
        pQueue->pRtpPacketInfoArray[ pQueue->writeIndex ].pSerializedRtpPacket = pRtpPacketInfo->pSerializedRtpPacket;
        pQueue->pRtpPacketInfoArray[ pQueue->writeIndex ].serializedPacketLength = pRtpPacketInfo->serializedPacketLength;

        pQueue->writeIndex = INC_WRITE_INDEX( pQueue );
        pQueue->packetCount += 1;
    }

    return result;
}

/*----------------------------------------------------------------------------*/

/**
 * @brief Read and remove the oldest RTP packet info from the queue.
 */
RtpPacketQueueResult_t RtpPacketQueue_Dequeue( RtpPacketQueue_t * pQueue,
                                               RtpPacketInfo_t * pRtpPacketInfo )
{
    RtpPacketQueueResult_t result;

    result = RtpPacketQueue_Peek( pQueue, pRtpPacketInfo );

    if( result == RTP_PACKET_QUEUE_RESULT_OK )
    {
        pQueue->readIndex = INC_READ_INDEX( pQueue );
        pQueue->packetCount -= 1;
    }

    return result;
}

/*----------------------------------------------------------------------------*/

/**
 * @brief Read the oldest RTP packet info from the queue without removing it.
 */
RtpPacketQueueResult_t RtpPacketQueue_Peek( RtpPacketQueue_t * pQueue,
                                            RtpPacketInfo_t * pRtpPacketInfo )
{
    RtpPacketQueueResult_t result = RTP_PACKET_QUEUE_RESULT_OK;

    if( ( pQueue == NULL ) ||
        ( pRtpPacketInfo == NULL ) )
    {
        result = RTP_PACKET_QUEUE_RESULT_BAD_PARAM;
    }

    if( result == RTP_PACKET_QUEUE_RESULT_OK )
    {
        if( IS_QUEUE_EMPTY( pQueue ) )
        {
            result = RTP_PACKET_QUEUE_RESULT_EMPTY;
        }
    }

    if( result == RTP_PACKET_QUEUE_RESULT_OK )
    {
        pRtpPacketInfo->seqNum = pQueue->pRtpPacketInfoArray[ pQueue->readIndex ].seqNum;
        pRtpPacketInfo->pSerializedRtpPacket = pQueue->pRtpPacketInfoArray[ pQueue->readIndex ].pSerializedRtpPacket;
        pRtpPacketInfo->serializedPacketLength = pQueue->pRtpPacketInfoArray[ pQueue->readIndex ].serializedPacketLength;
    }

    return result;
}

/*----------------------------------------------------------------------------*/

/**
 * @brief Read and remove the RTP packet info with the matching sequence number.
 *
 * Return RTP_PACKET_QUEUE_RESULT_PACKET_NOT_FOUND if an RTP packet info with
 * matching sequence number is not found.
 */
RtpPacketQueueResult_t RtpPacketQueue_Retrieve( RtpPacketQueue_t * pQueue,
                                                uint16_t seqNum,
                                                RtpPacketInfo_t * pRtpPacketInfo )
{
    size_t i, readIndex;
    RtpPacketQueueResult_t result = RTP_PACKET_QUEUE_RESULT_OK;

    if( ( pQueue == NULL ) ||
        ( pRtpPacketInfo == NULL ) )
    {
        result = RTP_PACKET_QUEUE_RESULT_BAD_PARAM;
    }

    if( result == RTP_PACKET_QUEUE_RESULT_OK )
    {
        if( IS_QUEUE_EMPTY( pQueue ) )
        {
            result = RTP_PACKET_QUEUE_RESULT_EMPTY;
        }
    }

    if( result == RTP_PACKET_QUEUE_RESULT_OK )
    {
        result = RTP_PACKET_QUEUE_RESULT_PACKET_NOT_FOUND;

        for( i = 0; i < pQueue->packetCount; i++ )
        {
            readIndex = WRAP( ( pQueue->readIndex + i ),
                              pQueue->rtpPacketInfoArrayLength );

            if( pQueue->pRtpPacketInfoArray[ readIndex ].seqNum == seqNum )
            {
                pRtpPacketInfo->seqNum = pQueue->pRtpPacketInfoArray[ readIndex ].seqNum;
                pRtpPacketInfo->pSerializedRtpPacket = pQueue->pRtpPacketInfoArray[ readIndex ].pSerializedRtpPacket;
                pRtpPacketInfo->serializedPacketLength = pQueue->pRtpPacketInfoArray[ readIndex ].serializedPacketLength;

                result = RTP_PACKET_QUEUE_RESULT_OK;
                break;
            }
        }
    }

    return result;
}

/*----------------------------------------------------------------------------*/
