#ifndef RTP_PACKET_QUEUE_H
#define RTP_PACKET_QUEUE_H

/* Standard includes. */
#include <stdint.h>
#include <stddef.h>

typedef enum RtpPacketQueueResult
{
    RTP_PACKET_QUEUE_RESULT_OK,
    RTP_PACKET_QUEUE_RESULT_BAD_PARAM,
    RTP_PACKET_QUEUE_RESULT_FULL,
    RTP_PACKET_QUEUE_RESULT_EMPTY,
    RTP_PACKET_QUEUE_RESULT_PACKET_DELETED,
    RTP_PACKET_QUEUE_RESULT_PACKET_NOT_FOUND
} RtpPacketQueueResult_t;

/*----------------------------------------------------------------------------*/

typedef struct RtpPacketInfo
{
    uint16_t seqNum;
    uint8_t * pSerializedRtpPacket;
    size_t serializedPacketLength;
} RtpPacketInfo_t;

typedef struct RtpPacketQueue
{
    RtpPacketInfo_t * pRtpPacketInfoArray;
    size_t rtpPacketInfoArrayLength;
    size_t writeIndex;
    size_t readIndex;
} RtpPacketQueue_t;

/*----------------------------------------------------------------------------*/

RtpPacketQueueResult_t RtpPacketQueue_Init( RtpPacketQueue_t * pQueue,
                                            RtpPacketInfo_t * pRtpPacketInfoArray,
                                            size_t rtpPacketInfoArrayLength );

RtpPacketQueueResult_t RtpPacketQueue_Enqueue( RtpPacketQueue_t * pQueue,
                                               RtpPacketInfo_t * pRtpPacketInfo );

RtpPacketQueueResult_t RtpPacketQueue_ForceEnqueue( RtpPacketQueue_t * pQueue,
                                                    RtpPacketInfo_t * pRtpPacketInfo,
                                                    RtpPacketInfo_t * pDeletedRtpPacketInfo );

RtpPacketQueueResult_t RtpPacketQueue_Dequeue( RtpPacketQueue_t * pQueue,
                                               RtpPacketInfo_t * pRtpPacketInfo );

RtpPacketQueueResult_t RtpPacketQueue_Peek( RtpPacketQueue_t * pQueue,
                                            RtpPacketInfo_t * pRtpPacketInfo );

RtpPacketQueueResult_t RtpPacketQueue_Retrieve( RtpPacketQueue_t * pQueue,
                                                uint16_t seqNum,
                                                RtpPacketInfo_t * pRtpPacketInfo );

/*----------------------------------------------------------------------------*/

#endif /* RTP_PACKET_QUEUE_H */
