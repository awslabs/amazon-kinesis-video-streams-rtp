/* Standard includes. */
#include <string.h>

/* API includes. */
#include "h265_packetizer.h"

/*-----------------------------------------------------------*/

static void PacketizeSingleNaluPacket( H265PacketizerContext_t * pCtx,
                                       H265Packet_t * pPacket );

static H265Result_t PacketizeFragmentationUnitPacket( H265PacketizerContext_t * pCtx,
                                                      H265Packet_t * pPacket );

static void PacketizeAggregationPacket( H265PacketizerContext_t * pCtx,
                                        size_t nalusToAggregate,
                                        H265Packet_t * pPacket );

/*-----------------------------------------------------------*/

/*
 * RTP payload format for single NAL unit packet:
 *
 *  0                   1                   2                   3
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |      NAL Unit Header          |                               |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+                               |
 * |                                                               |
 * |                    NAL unit payload data                      |
 * |                                                               |
 * |                               +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                               :...OPTIONAL RTP padding        |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
static void PacketizeSingleNaluPacket( H265PacketizerContext_t * pCtx,
                                       H265Packet_t * pPacket )
{
    /* Fill packet. */
    memcpy( ( void * ) &( pPacket->pPacketData[ 0 ] ),
            ( const void * ) &( pCtx->pNaluArray[ pCtx->tailIndex ].pNaluData[ 0 ] ),
            pCtx->pNaluArray[ pCtx->tailIndex ].naluDataLength );

    pPacket->packetDataLength = pCtx->pNaluArray[ pCtx->tailIndex ].naluDataLength;

    /* Move to the next NALU in the next call to H265Packetizer_GetPacket. */
    pCtx->tailIndex += 1;
    pCtx->naluCount -= 1;
}

/*-----------------------------------------------------------*/

static H265Result_t PacketizeFragmentationUnitPacket( H265PacketizerContext_t * pCtx,
                                                      H265Packet_t * pPacket )
{
    H265Result_t result = H265_RESULT_OK;
    uint8_t fuHeader = 0;
    size_t maxNaluDataLengthToSend, naluDataLengthToSend;
    uint8_t * pNaluData = pCtx->pNaluArray[ pCtx->tailIndex ].pNaluData;

    if( pPacket->packetDataLength <= FU_PAYLOAD_HEADER_SIZE + FU_HEADER_SIZE )
    {
        result = H265_RESULT_OUT_OF_MEMORY;
    }

    if( result == H265_RESULT_OK )
    {
        /* Is this the first fragment? */
        if( pCtx->currentlyProcessingPacket == H265_PACKET_NONE )
        {
            pCtx->currentlyProcessingPacket = H265_FU_PACKET;

            /* Construct the payload header to be sent in all the fragments. The
             * fields F, and TID in the payload header are equal to the fields
             * F, and TID, respectively, of the fragmented NAL unit.*/
            pCtx->fuPacketizationState.payloadHeader[ 0 ] = ( pNaluData[ 0 ] & NALU_HEADER_F_MASK ) |
                                                            ( FU_PACKET_TYPE << NALU_HEADER_TYPE_LOCATION );
            pCtx->fuPacketizationState.payloadHeader[ 1 ] = pNaluData[ 1 ] & NALU_HEADER_TID_MASK;

            /* Construct the FU header to be sent in all the fragments. We will
             * still need to update S and E bit separately. */
            pCtx->fuPacketizationState.fuHeader = ( pNaluData[ 0 ] & NALU_HEADER_TYPE_MASK ) >> NALU_HEADER_TYPE_LOCATION;

            /* Per RFC https://datatracker.ietf.org/doc/html/rfc7798, we do not
             * need to send NALU header in FU packets as the information can be
             * constructed payload header and FU header. */
            pCtx->fuPacketizationState.naluDataIndex = NALU_HEADER_SIZE;
            pCtx->fuPacketizationState.remainingNaluLength = pCtx->pNaluArray[ pCtx->tailIndex ].naluDataLength - NALU_HEADER_SIZE;

            /* Indicate start fragment in the FU header. */
            fuHeader |= FU_HEADER_S_BIT_MASK;
        }

        /* Set type in FU header. */
        fuHeader |= pCtx->fuPacketizationState.fuHeader;

        /* Maximum NALU data that we can send in this packet. */
        maxNaluDataLengthToSend = pPacket->packetDataLength - FU_PAYLOAD_HEADER_SIZE - FU_HEADER_SIZE;
        /* Actual NALU data what we will send in this packet. */
        naluDataLengthToSend = H265_MIN( maxNaluDataLengthToSend,
                                         pCtx->fuPacketizationState.remainingNaluLength );

        if( pCtx->fuPacketizationState.remainingNaluLength == naluDataLengthToSend )
        {
            /* Indicate end fragment in the FU header. */
            fuHeader |= FU_HEADER_E_BIT_MASK;
        }

        /* Write payload header. */
        memcpy( ( void * ) &( pPacket->pPacketData[ 0 ] ),
                ( const void * ) &( pCtx->fuPacketizationState.payloadHeader[ 0 ] ),
                FU_PAYLOAD_HEADER_SIZE );

        /* Write FU header. */
        pPacket->pPacketData[ FU_HEADER_OFFSET ] = fuHeader;

        /* Write NALU data. */
        memcpy( ( void * ) &( pPacket->pPacketData[ FU_PAYLOAD_HEADER_SIZE + FU_HEADER_SIZE ] ),
                ( const void * ) &( pNaluData[ pCtx->fuPacketizationState.naluDataIndex ] ),
                naluDataLengthToSend );
        pPacket->packetDataLength = naluDataLengthToSend + FU_PAYLOAD_HEADER_SIZE + FU_HEADER_SIZE;

        pCtx->fuPacketizationState.naluDataIndex += naluDataLengthToSend;
        pCtx->fuPacketizationState.remainingNaluLength -= naluDataLengthToSend;

        if( pCtx->fuPacketizationState.remainingNaluLength == 0 )
        {
            /* Reset state. */
            memset( &( pCtx->fuPacketizationState ),
                    0,
                    sizeof( FuPacketizationState_t ) );
            pCtx->currentlyProcessingPacket = H265_PACKET_NONE;

            /* Move to the next NALU in the next call to H265Packetizer_GetPacket. */
            pCtx->tailIndex += 1;
            pCtx->naluCount -= 1;
        }
    }

    return result;
}

/*-----------------------------------------------------------*/

static void PacketizeAggregationPacket( H265PacketizerContext_t * pCtx,
                                        size_t nalusToAggregate,
                                        H265Packet_t * pPacket )
{
    size_t i, packetWriteIndex = 0, naluSize;
    uint8_t * pNaluData;
    uint8_t temporalId, minTemporalId = 0xFF;

    /* Write payload header. */
    pPacket->pPacketData[ 0 ] = AP_PACKET_TYPE << NALU_HEADER_TYPE_LOCATION;
    pPacket->pPacketData[ 1 ] = 0;
    packetWriteIndex += 2;

    /* Aggregate all the NAL units in the packet. */
    for( i = 0; i < nalusToAggregate; i++ )
    {
        pNaluData = pCtx->pNaluArray[ pCtx->tailIndex + i ].pNaluData;
        naluSize = pCtx->pNaluArray[ pCtx->tailIndex + i ].naluDataLength;

        temporalId = ( pNaluData[ 1 ] & NALU_HEADER_TID_MASK ) >> NALU_HEADER_TID_LOCATION;
        minTemporalId = H265_MIN( minTemporalId, temporalId );

        /* Update F bit in the payload header. */
        pPacket->pPacketData[ 0 ] |= ( pNaluData[ 0 ] & NALU_HEADER_F_MASK );

        /* Write NAL unit size. */
        pPacket->pPacketData[ packetWriteIndex ] = ( naluSize >> 8 ) & 0xFF;
        pPacket->pPacketData[ packetWriteIndex + 1 ] = naluSize & 0xFF;
        packetWriteIndex += 2;

        /* Write NAL unit data. */
        memcpy( ( void * ) &( pPacket->pPacketData[ packetWriteIndex ] ),
                ( const void * ) pNaluData,
                naluSize );
        packetWriteIndex += naluSize;
    }

    /* Write TID in the payload header. */
    pPacket->pPacketData[ 1 ] |= ( minTemporalId << NALU_HEADER_TID_LOCATION );

    pCtx->tailIndex += nalusToAggregate;
    pCtx->naluCount -= nalusToAggregate;

    pPacket->packetDataLength = packetWriteIndex;
}

/*-----------------------------------------------------------*/

H265Result_t H265Packetizer_Init( H265PacketizerContext_t * pCtx,
                                  H265Nalu_t * pNaluArray,
                                  size_t naluArrayLength )
{
    H265Result_t result = H265_RESULT_OK;

    if( ( pCtx == NULL ) ||
        ( pNaluArray == NULL ) ||
        ( naluArrayLength == 0 ) )
    {
        result = H265_RESULT_BAD_PARAM;
    }

    if( result == H265_RESULT_OK )
    {
        pCtx->pNaluArray = pNaluArray;
        pCtx->naluArrayLength = naluArrayLength;

        pCtx->headIndex = 0;
        pCtx->tailIndex = 0;
        pCtx->naluCount = 0;

        pCtx->currentlyProcessingPacket = H265_PACKET_NONE;

        memset( &( pCtx->fuPacketizationState ),
                0,
                sizeof( FuPacketizationState_t ) );
    }

    return result;
}

/*-----------------------------------------------------------*/

H265Result_t H265Packetizer_AddFrame( H265PacketizerContext_t * pCtx,
                                      H265Frame_t * pFrame )
{
    H265Result_t result = H265_RESULT_OK;
    H265Nalu_t nalu = { 0 };
    size_t currentIndex = 0, naluStartIndex = 0, remainingFrameLength = 0;
    uint8_t startCode1[] = { 0x00, 0x00, 0x00, 0x01 }; /* 4-byte start code. */
    uint8_t startCode2[] = { 0x00, 0x00, 0x01 }; /* 3-byte start code. */
    uint8_t firstStartCode = 1;

    if( ( pCtx == NULL ) ||
        ( pFrame == NULL ) ||
        ( pFrame->pFrameData == NULL ) ||
        ( pFrame->frameDataLength == 0 ) )
    {
        result = H265_RESULT_BAD_PARAM;
    }

    while( ( result == H265_RESULT_OK ) &&
           ( currentIndex < pFrame->frameDataLength ) )
    {
        remainingFrameLength = pFrame->frameDataLength - currentIndex;

        /* Check the presence of 4 byte start code. */
        if( remainingFrameLength >= sizeof( startCode1 ) )
        {
            if( memcmp( &( pFrame->pFrameData[ currentIndex ] ),
                        &( startCode1[ 0 ] ),
                        sizeof( startCode1 ) ) == 0 )
            {
                if( firstStartCode == 1 )
                {
                    firstStartCode = 0;
                }
                else
                {
                    /* Create NAL unit from data between start codes. */
                    nalu.pNaluData = &( pFrame->pFrameData[ naluStartIndex ] );
                    nalu.naluDataLength = currentIndex - naluStartIndex;

                    result = H265Packetizer_AddNalu( pCtx,
                                                     &( nalu ) );
                }

                naluStartIndex = currentIndex + sizeof( startCode1 );
                currentIndex = naluStartIndex;
                continue;
            }
        }

        /* Check the presence of 3 byte start code. */
        if( remainingFrameLength >= sizeof( startCode2 ) )
        {
            if( memcmp( &( pFrame->pFrameData[ currentIndex ] ),
                        &( startCode2[ 0 ] ),
                        sizeof( startCode2 ) ) == 0 )
            {
                if( firstStartCode == 1 )
                {
                    firstStartCode = 0;
                }
                else
                {
                    /* Create NAL unit from data between start codes. */
                    nalu.pNaluData = &( pFrame->pFrameData[ naluStartIndex ] );
                    nalu.naluDataLength = currentIndex - naluStartIndex;

                    result = H265Packetizer_AddNalu( pCtx,
                                                     &( nalu ) );
                }

                naluStartIndex = currentIndex + sizeof( startCode2 );
                currentIndex = naluStartIndex;
                continue;
            }
        }

        currentIndex += 1;
    }

    /* Handle last NAL unit in frame. */
    if( result == H265_RESULT_OK )
    {
        if( naluStartIndex > 0 )
        {
            nalu.pNaluData = &( pFrame->pFrameData[ naluStartIndex ] );
            nalu.naluDataLength = pFrame->frameDataLength - naluStartIndex;

            result = H265Packetizer_AddNalu( pCtx,
                                             &( nalu ) );
        }
        else
        {
            result = H265_RESULT_MALFORMED_PACKET;
        }
    }

    return result;
}

/*-----------------------------------------------------------*/

H265Result_t H265Packetizer_AddNalu( H265PacketizerContext_t * pCtx,
                                     H265Nalu_t * pNalu )
{
    H265Result_t result = H265_RESULT_OK;

    if( ( pCtx == NULL ) ||
        ( pNalu == NULL ) ||
        ( pNalu->pNaluData == NULL ) )
    {
        result = H265_RESULT_BAD_PARAM;
    }

    if( result == H265_RESULT_OK )
    {
        if( pNalu->naluDataLength < NALU_HEADER_SIZE )
        {
            result = H265_RESULT_MALFORMED_PACKET;
        }
    }

    if( result == H265_RESULT_OK )
    {
        if( pCtx->naluCount >= pCtx->naluArrayLength )
        {
            result = H265_RESULT_OUT_OF_MEMORY;
        }
    }

    if( result == H265_RESULT_OK )
    {
        pCtx->pNaluArray[ pCtx->headIndex ].pNaluData = pNalu->pNaluData;
        pCtx->pNaluArray[ pCtx->headIndex ].naluDataLength = pNalu->naluDataLength;

        pCtx->headIndex += 1;
        pCtx->naluCount += 1;
    }

    return result;
}

/*-----------------------------------------------------------*/

H265Result_t H265Packetizer_GetPacket( H265PacketizerContext_t * pCtx,
                                       H265Packet_t * pPacket )
{
    H265Result_t result = H265_RESULT_OK;
    size_t aggregatePacketSize = 0, nalusToAggregate = 0;
    size_t i, naluSize;

    if( ( pCtx == NULL ) ||
        ( pPacket == NULL ) ||
        ( pPacket->pPacketData == NULL ) ||
        ( pPacket->packetDataLength == 0 ) ||
        ( pPacket->packetDataLength < NALU_HEADER_SIZE + 1 ) ) /* Minimum size for any packet. */
    {
        result = H265_RESULT_BAD_PARAM;
    }

    if( result == H265_RESULT_OK )
    {
        if( pCtx->naluCount == 0 )
        {
            result = H265_RESULT_NO_MORE_PACKETS;
        }
    }

    if( result == H265_RESULT_OK )
    {
        /* Are we in the middle of packetizing fragments of a NALU? */
        if( pCtx->currentlyProcessingPacket == H265_FU_PACKET )
        {
            result = PacketizeFragmentationUnitPacket( pCtx,
                                                       pPacket );
        }
        else
        {
            /* If a NAL Unit can fit in one packet, use Single NAL Unit packet
             * or Aggregation Packet if more than one NAL units can fit in the
             * same packet. */
            if( pCtx->pNaluArray[ pCtx->tailIndex ].naluDataLength <= pPacket->packetDataLength )
            {
                /* Can we aggregate more than one NAL units? */
                aggregatePacketSize = AP_HEADER_SIZE;
                for( i = 0; i < pCtx->naluCount; i++ )
                {
                    naluSize = pCtx->pNaluArray[ pCtx->tailIndex + i ].naluDataLength;

                    /* Can we fit in this NAL unit? */
                    if( ( aggregatePacketSize + AP_NALU_LENGTH_FIELD_SIZE + naluSize ) <= pPacket->packetDataLength )
                    {
                        aggregatePacketSize += ( AP_NALU_LENGTH_FIELD_SIZE + naluSize );
                        nalusToAggregate += 1;
                    }
                    else
                    {
                        break;
                    }
                }

                /* If we can aggregate more than one NAL units, use Aggregation Packet. */
                if( nalusToAggregate > 1 )
                {
                    PacketizeAggregationPacket( pCtx,
                                                nalusToAggregate,
                                                pPacket );
                }
                else
                {
                    /* Otherwise, use Single NAL Unit Packet. */
                    PacketizeSingleNaluPacket( pCtx,
                                               pPacket );
                }
            }
            else
            {
                /* Otherwise, fragment the NAL Unit in more than one packets. */
                result = PacketizeFragmentationUnitPacket( pCtx,
                                                           pPacket );
            }
        }
    }

    return result;
}

/*-----------------------------------------------------------*/
