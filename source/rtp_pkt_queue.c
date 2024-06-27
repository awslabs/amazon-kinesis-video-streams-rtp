/* Standard includes. */
#include <string.h>

/* API includes. */
#include "rtp_pkt_queue.h"

/*----------------------------------------------------------------------------*/

#define WRAP( x, n ) \
    ( ( x ) % ( n ) )

#define INC_READ_INDEX( pQueue ) \
    WRAP( ( pQueue )->readIndex + 1,\
          ( pQueue )->rtpPacketInfoArrayLength )

#define INC_WRITE_INDEX( pQueue ) \
    WRAP( ( pQueue )->writeIndex + 1,\
          ( pQueue )->rtpPacketInfoArrayLength )

#define IS_QUEUE_FULL( pQueue ) \
    ( pQueue->packetCount == ( pQueue )->rtpPacketInfoArrayLength )

#define IS_QUEUE_EMPTY( pQueue ) \
    ( pQueue->packetCount == 0 )

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
 * @brief Add an RTP packet into the specified RTP packet queue..
 */
RtpPacketQueueResult_t RtpPacketQueue_Enqueue( RtpPacketQueue_t * pQueue,
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
 * @brief Add an element to the queue, replacing the oldest element if the queue is full.
 *        Return the deleted packet info if the queue is full.
 */
RtpPacketQueueResult_t RtpPacketQueue_ForceEnqueue( RtpPacketQueue_t * pQueue,
                                                    RtpPacketInfo_t * pRtpPacketInfo,
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
 * @brief Read and remove the oldest elements from the queue.
 */
RtpPacketQueueResult_t RtpPacketQueue_Dequeue( RtpPacketQueue_t * pQueue,
                                               RtpPacketInfo_t * pRtpPacketInfo )
{
    RtpPacketQueueResult_t result = RTP_PACKET_QUEUE_RESULT_OK;

    if( pQueue == NULL )
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
        if( pRtpPacketInfo != NULL )
        {
            pRtpPacketInfo->seqNum = pQueue->pRtpPacketInfoArray[ pQueue->readIndex ].seqNum;
            pRtpPacketInfo->pSerializedRtpPacket = pQueue->pRtpPacketInfoArray[ pQueue->readIndex ].pSerializedRtpPacket;
            pRtpPacketInfo->serializedPacketLength = pQueue->pRtpPacketInfoArray[ pQueue->readIndex ].serializedPacketLength;
        }

        pQueue->readIndex = INC_READ_INDEX( pQueue );
        pQueue->packetCount -= 1;
    }

    return result;
}

/*----------------------------------------------------------------------------*/

/**
 * @brief Read the first element from the queue without removing it.
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
 * @brief Read and remove the element with the matching sequence number from the queue.
 *        Return RTP_PACKET_QUEUE_RESULT_PACKET_NOT_FOUND is the sequence number is
 *        not found.
 */
RtpPacketQueueResult_t RtpPacketQueue_Retrieve( RtpPacketQueue_t * pQueue,
                                                uint16_t seqNum,
                                                RtpPacketInfo_t * pRtpPacketInfo )
{
    size_t i;
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

        for( i = pQueue->readIndex;
             i != pQueue->writeIndex;
             i = WRAP( ( i + 1 ),
                       pQueue->rtpPacketInfoArrayLength ) )
        {
            if( pQueue->pRtpPacketInfoArray[ i ].seqNum == seqNum )
            {
                pRtpPacketInfo->seqNum = pQueue->pRtpPacketInfoArray[ i ].seqNum;
                pRtpPacketInfo->pSerializedRtpPacket = pQueue->pRtpPacketInfoArray[ i ].pSerializedRtpPacket;
                pRtpPacketInfo->serializedPacketLength = pQueue->pRtpPacketInfoArray[ i ].serializedPacketLength;

                result = RTP_PACKET_QUEUE_RESULT_OK;
                break;
            }
        }
    }

    return result;
}

/*----------------------------------------------------------------------------*/
