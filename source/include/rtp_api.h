#ifndef RTP_API_H
#define RTP_API_H

/* Data types includes. */
#include "rtp_data_types.h"

RtpResult_t Rtp_Init( RtpContext_t * pCtx );

RtpResult_t Rtp_Serialize( RtpContext_t * pCtx,
                           const RtpPacket_t * pRtpPacket,
                           uint8_t * pBuffer,
                           size_t * pLength );

RtpResult_t Rtp_DeSerialize( RtpContext_t * pCtx,
                             uint8_t * pSerializedPacket,
                             size_t serializedPacketLength,
                             RtpPacket_t * pRtpPacket );

#endif /* RTP_API_H */
