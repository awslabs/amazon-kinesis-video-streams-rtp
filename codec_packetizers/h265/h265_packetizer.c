#include <string.h>
#include "h265_packetizer.h"

/*-----------------------------------------------------------*/

/*
 * Original NAL unit header:
 *
 * +---------------+---------------+
 * |0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |F|   Type    | LayerId   |TID  |
 * +---------------+---------------+
 */

/*-----------------------------------------------------------*/

static void PacketizeSingleNaluPacket( H265PacketizerContext_t * pCtx,
                                       H265Packet_t * pPacket );

static void PacketizeFragmentationUnitPacket( H265PacketizerContext_t * pCtx,
                                              H265Packet_t * pPacket );

static void PacketizeAggregationPacket( H265PacketizerContext_t * pCtx,
                                        H265Packet_t * pPacket );

/*-----------------------------------------------------------*/

static void PacketizeSingleNaluPacket( H265PacketizerContext_t * pCtx,
                                       H265Packet_t * pPacket )
{
    /* Copy NAL header from context to output */
    memcpy( ( void * ) &( pPacket->pPacketData[ 0 ] ),
            ( const void * ) &( pCtx->pNaluArray[ pCtx->tailIndex ].pNaluData[ 0 ] ),
            pCtx->pNaluArray[ pCtx->tailIndex ].naluDataLength );

    pPacket->packetDataLength = pCtx->pNaluArray[ pCtx->tailIndex ].naluDataLength;

    pCtx->tailIndex += 1;
    pCtx->naluCount -= 1;
}

/*-------------------------------------------------------------------------------------------------*/

/*
 * RTP payload format for FU:
 *
 * +---------------+---------------+
 * |0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |F|   Type    |  LayerId  | TID |  -> PayloadHdr (Type=49)
 * +---------------+---------------+
 * |S|E|   FuType   |             |  -> FU header
 * +---------------+---------------+
 * |     DONL (conditional)       |  -> Only if S=1 & sprop-max-don-diff > 0
 * +---------------+---------------+
 * |        FU payload            |
 * |                             |
 * +---------------+---------------+
 */

static void PacketizeFragmentationUnitPacket( H265PacketizerContext_t * pCtx,
                                              H265Packet_t * pPacket )
{
    uint8_t fuHeader = 0;
    uint8_t * pNaluData = pCtx->pNaluArray[ pCtx->tailIndex ].pNaluData;

    uint8_t origF = 0;
    uint8_t origLayerId = 0;
    uint8_t origTid = 0;
    uint16_t payloadHdr = 0;

    size_t headerSize = 0;
    size_t maxPayloadSize = 0;
    size_t payloadSize = 0;
    size_t offset = 0;

    if( pCtx->currentlyProcessingPacket == H265_PACKET_NONE )
    {
        pCtx->currentlyProcessingPacket = H265_FU_PACKET;

        origF = pNaluData[ 0 ] & HEVC_NALU_HEADER_F_MASK;
        origLayerId = ( pNaluData[ 0 ] & HEVC_NALU_HEADER_LAYER_ID_FIRST_BYTE_MASK ) << 5 |
                      ( pNaluData[ 1 ] & HEVC_NALU_HEADER_LAYER_ID_SECOND_BYTE_MASK ) >> 3;
        origTid = pNaluData[ 1 ] & HEVC_NALU_HEADER_TID_MASK;

        /* Construct PayloadHdr, the first byte */
        payloadHdr =
            ( ( origF << 7 ) |          /* F bit in MSB of first byte */
              ( FU_PACKET_TYPE << 1 ) | /* Type = 49 */
              ( ( origLayerId >> 5 ) & 1 ) );

        /* Add second byte */
        payloadHdr = ( payloadHdr << 8 ) |             /* Shift first byte */
                     ( ( origLayerId & HEVC_RTP_LAYER_ID_MASK ) << 3 ) | /* Rest of LayerId */
                     origTid;                          /* TID */

        pCtx->fuPacketizationState.payloadHdr = payloadHdr;

        /* Skip original NAL header (2 bytes) */
        pCtx->fuPacketizationState.naluDataIndex = NALU_HEADER_SIZE;
        pCtx->fuPacketizationState.remainingNaluLength =
            pCtx->pNaluArray[ pCtx->tailIndex ].naluDataLength - NALU_HEADER_SIZE;

        /* Set S bit for first fragment */
        fuHeader |= FU_HEADER_S_BIT_MASK;
    }

    headerSize = TOTAL_FU_HEADER_SIZE;     /* PayloadHdr(2) + FUHeader(1) */

    /* Calculate maximum payload size for this packet */
    maxPayloadSize = ( pPacket->packetDataLength > headerSize ) ? pPacket->packetDataLength - headerSize : 0;

    /* Calculate actual payload size for this fragment */
    payloadSize = H265_MIN( pCtx->fuPacketizationState.remainingNaluLength,
                            maxPayloadSize );

    /* Set E bit if this is the last fragment */
    if( pCtx->fuPacketizationState.remainingNaluLength <= maxPayloadSize )
    {
        fuHeader |= FU_HEADER_E_BIT_MASK;
    }

    /* Set FU type from original NAL unit */
    fuHeader |= ( ( pNaluData[ 0 ] >> 1 ) & FU_HEADER_TYPE_MASK );

    /*
     * pPacket->pPacketData:
     * [PayloadHdr1][PayloadHdr2][FU Header][Payload...]
     * ↑            ↑            ↑          ↑
     * offset=0    offset=1    offset=2   offset=3
     */

    /* Build packet */
    pPacket->pPacketData[ offset ] = ( pCtx->fuPacketizationState.payloadHdr >> 8 ) & HEVC_BYTE_MASK ;
    offset++;

    pPacket->pPacketData[ offset ] = pCtx->fuPacketizationState.payloadHdr & HEVC_BYTE_MASK ;
    offset++;

    pPacket->pPacketData[ offset ] = fuHeader;
    offset++;
    if( payloadSize > 0 )
    {
        memcpy( ( void * ) &( pPacket->pPacketData[ offset ] ),
                ( const void * ) &( pNaluData[ pCtx->fuPacketizationState.naluDataIndex ] ),
                payloadSize );
    }

    pCtx->fuPacketizationState.naluDataIndex += payloadSize;
    pCtx->fuPacketizationState.remainingNaluLength -= payloadSize;
    pPacket->packetDataLength = offset + payloadSize;

    /* Check if this was the last fragment */
    if( pCtx->fuPacketizationState.remainingNaluLength == 0 )
    {
        pCtx->fuPacketizationState.naluDataIndex = 0;
        pCtx->fuPacketizationState.payloadHdr = 0;

        pCtx->currentlyProcessingPacket = H265_PACKET_NONE;
        pCtx->tailIndex++;
        pCtx->naluCount--;
    }
}

/*-----------------------------------------------------------------------------------------------------*/

/*
 * AP structure:
 *
 * [PayloadHdr (Type=48)]
 * First Unit:
 *   [DONL (2 bytes, conditional)]
 *   [NAL Size (2 bytes)]
 *   [NAL unit]
 * Subsequent Units:
 *   [DOND (1 byte, conditional)]
 *   [NAL Size (2 bytes)]
 *   [NAL unit]
 */

static void PacketizeAggregationPacket( H265PacketizerContext_t * pCtx,
                                        H265Packet_t * pPacket )
{
    size_t offset = 0;
    uint8_t naluCount = 0;
    size_t needed_size = 0;
    uint16_t naluSize = 0;
    uint8_t * nalu_data = NULL;


    uint8_t f_bit = 0;
    uint8_t layer_id = 0;
    uint8_t tid = 0;
    uint8_t min_layer_id = MAX_LAYER_ID;
    uint8_t min_tid = MAX_TEMPORAL_ID;

    size_t temp_offset = 2;              /* Start after PayloadHdr */
    size_t availableSize = pPacket->packetDataLength;
    size_t scan_index = pCtx->tailIndex; /* Start with first NAL */

    /* Scan NALs to find minimums and check size */
    while( scan_index < pCtx->naluArrayLength && pCtx->naluCount > naluCount )
    {
        /* Calculate space needed for this NAL */
        needed_size = AP_NALU_LENGTH_FIELD_SIZE +
                      pCtx->pNaluArray[ scan_index ].naluDataLength;        /* NAL data */

        /* Check if this NAL would fit */
        if( temp_offset + needed_size > availableSize )
        {
            break; /* Won't fit, stop scanning */
        }

        /* Get header fields from this NAL */
        nalu_data = pCtx->pNaluArray[ scan_index ].pNaluData;
        f_bit |= ( nalu_data[ 0 ] & HEVC_NALU_HEADER_F_MASK );

        layer_id = pCtx->pNaluArray[ scan_index ].nal_layer_id;
        tid = pCtx->pNaluArray[ scan_index ].temporal_id;

        /* Update minimum values */
        min_layer_id = H265_MIN( min_layer_id,
                                 layer_id );
        min_tid = H265_MIN( min_tid,
                            tid );

        temp_offset += needed_size;
        scan_index++;
        naluCount++;
    }

    /* Write PayloadHdr */
    pPacket->pPacketData[ 0 ] = ( AP_PACKET_TYPE << 1 ) | ( min_layer_id >> 5 );
    pPacket->pPacketData[ 1 ] = ( ( min_layer_id & HEVC_RTP_LAYER_ID_MASK ) << 3 ) | min_tid;
    offset = 2;

    /* Process subsequent NAL units */
    for(uint8_t i = 0; i < naluCount; i++)
    {
        /* Write NAL size and data */
        naluSize = pCtx->pNaluArray[ pCtx->tailIndex ].naluDataLength;
        pPacket->pPacketData[ offset++ ] = ( naluSize >> 8 ) & HEVC_BYTE_MASK ;
        pPacket->pPacketData[ offset++ ] = naluSize & HEVC_BYTE_MASK ;

        memcpy( ( void * ) &( pPacket->pPacketData[ offset ] ),
                ( const void * ) &( pCtx->pNaluArray[ pCtx->tailIndex ].pNaluData[ 0 ] ),
                naluSize );
        offset += naluSize;

        pCtx->tailIndex++;
        pCtx->naluCount--;
    }

    pPacket->packetDataLength = offset;
}

/*-----------------------------------------------------------------------------------------------------*/

H265Result_t H265Packetizer_Init( H265PacketizerContext_t * pCtx,
                                  H265Nalu_t * pNaluArray,
                                  size_t naluArrayLength )
{
    H265Result_t result = H265_RESULT_OK;

    if( ( pCtx == NULL ) || ( pNaluArray == NULL ) ||
        ( naluArrayLength == 0 ) )
    {
        result = H265_RESULT_BAD_PARAM;
    }
    else
    {
        pCtx->pNaluArray = pNaluArray;
        pCtx->naluArrayLength = naluArrayLength;

        pCtx->headIndex = 0;
        pCtx->tailIndex = 0;
        pCtx->naluCount = 0;

        pCtx->currentlyProcessingPacket = H265_PACKET_NONE;

        memset( &pCtx->fuPacketizationState,
                0,
                sizeof( FuPacketizationState_t ) );
    }

    return result;
}

/*-----------------------------------------------------------------------------------------------------*/

H265Result_t H265Packetizer_AddFrame( H265PacketizerContext_t * pCtx,
                                      H265Frame_t * pFrame )
{
    H265Result_t result = H265_RESULT_OK;
    H265Nalu_t nalu = {0};
    size_t currentIndex = 0, naluStartIndex = 0, remainingFrameLength = 0;
    uint8_t startCode1[] = {0x00, 0x00, 0x00, 0x01}; /* 4-byte start code */
    uint8_t startCode2[] = {0x00, 0x00, 0x01};   /* 3-byte start code */
    uint8_t firstStartCode = 1;

    if( ( pCtx == NULL ) ||
        ( pFrame == NULL ) ||
        ( pFrame->pFrameData == NULL ) ||
        ( pFrame->frameDataLength == 0 ) )
    {
        result = H265_RESULT_BAD_PARAM;
    }
    else
    {
        while( ( result == H265_RESULT_OK ) &&
               ( currentIndex < pFrame->frameDataLength ) )
        {
            remainingFrameLength = pFrame->frameDataLength - currentIndex;

            /* Check for 4-byte start code */
            if( remainingFrameLength >= sizeof( startCode1 ) )
            {
                if( memcmp( &( pFrame->pFrameData[currentIndex] ),
                            &( startCode1[0] ),
                            sizeof( startCode1 ) ) == 0 )
                {
                    if( firstStartCode == 1 )
                    {
                        firstStartCode = 0;
                    }
                    else
                    {
                        /* Create NAL unit from data between start codes */
                        nalu.pNaluData = &( pFrame->pFrameData[naluStartIndex] );
                        nalu.naluDataLength = currentIndex - naluStartIndex;

                        /* Extract H.265 header fields if NAL unit is big enough */
                        if( nalu.naluDataLength >= NALU_HEADER_SIZE )
                        {
                            /* First byte: F bit and Type */
                            nalu.nal_unit_type = ( nalu.pNaluData[0] >> 1 ) & HEVC_NALU_HEADER_TYPE_MASK ;

                            /* LayerId spans both bytes */
                            nalu.nal_layer_id = ( ( nalu.pNaluData[0] & HEVC_NALU_HEADER_LAYER_ID_FIRST_BYTE_MASK ) << 5 ) |
                                                ( ( nalu.pNaluData[ 1 ] & HEVC_NALU_HEADER_LAYER_ID_SECOND_BYTE_MASK ) >> 3 );

                            /* Second byte: TID */
                            nalu.temporal_id = nalu.pNaluData[1] & HEVC_NALU_HEADER_TID_MASK;

                            result = H265Packetizer_AddNalu( pCtx,
                                                             &nalu );
                        }
                        else
                        {
                            result = H265_RESULT_MALFORMED_PACKET;
                        }
                    }

                    naluStartIndex = currentIndex + sizeof( startCode1 );
                    currentIndex = naluStartIndex;
                    continue;
                }
            }

            /* Check for 3-byte start code */
            if( remainingFrameLength >= sizeof( startCode2 ) )
            {
                if( memcmp( &( pFrame->pFrameData[currentIndex] ),
                            &( startCode2[0] ),
                            sizeof( startCode2 ) ) == 0 )
                {
                    if( firstStartCode == 1 )
                    {
                        firstStartCode = 0;
                    }
                    else
                    {
                        /* Create NAL unit from data between start codes */
                        nalu.pNaluData = &( pFrame->pFrameData[naluStartIndex] );
                        nalu.naluDataLength = currentIndex - naluStartIndex;

                        /* Extract H.265 header fields if NAL unit is big enough */
                        if( nalu.naluDataLength >= NALU_HEADER_SIZE )
                        {
                            nalu.nal_unit_type = ( nalu.pNaluData[0] >> 1 ) & HEVC_NALU_HEADER_TYPE_MASK ;
                            nalu.nal_layer_id = ( ( nalu.pNaluData[0] & HEVC_NALU_HEADER_LAYER_ID_FIRST_BYTE_MASK ) << 5 ) |
                                                ( ( nalu.pNaluData[ 1 ] & HEVC_NALU_HEADER_LAYER_ID_SECOND_BYTE_MASK ) >> 3 );
                            nalu.temporal_id = nalu.pNaluData[1] & HEVC_NALU_HEADER_TID_MASK;

                            result = H265Packetizer_AddNalu( pCtx,
                                                             &nalu );
                        }
                        else
                        {
                            result = H265_RESULT_MALFORMED_PACKET;
                        }
                    }

                    naluStartIndex = currentIndex + sizeof( startCode2 );
                    currentIndex = naluStartIndex;
                    continue;
                }
            }

            currentIndex++;
        }

        /* Handle last NAL unit in frame */
        if( ( result == H265_RESULT_OK ) && ( naluStartIndex > 0 ) )
        {
            nalu.pNaluData = &( pFrame->pFrameData[naluStartIndex] );
            nalu.naluDataLength = pFrame->frameDataLength - naluStartIndex;

            if( nalu.naluDataLength >= NALU_HEADER_SIZE )
            {
                nalu.nal_unit_type = ( nalu.pNaluData[0] >> 1 ) & HEVC_NALU_HEADER_TYPE_MASK ;
                nalu.nal_layer_id = ( ( nalu.pNaluData[0] & HEVC_NALU_HEADER_LAYER_ID_FIRST_BYTE_MASK ) << 5 ) |
                                    ( ( nalu.pNaluData[ 1 ] & HEVC_NALU_HEADER_LAYER_ID_SECOND_BYTE_MASK ) >> 3 );
                nalu.temporal_id = nalu.pNaluData[1] & HEVC_NALU_HEADER_TID_MASK;

                result = H265Packetizer_AddNalu( pCtx,
                                                 &nalu );
            }
            else
            {
                result = H265_RESULT_MALFORMED_PACKET;
            }
        }
    }

    return result;
}

/*-----------------------------------------------------------------------------------------------------*/

H265Result_t H265Packetizer_AddNalu( H265PacketizerContext_t * pCtx,
                                     H265Nalu_t * pNalu )
{
    H265Result_t result = H265_RESULT_OK;

    if( ( pCtx == NULL ) || ( pNalu == NULL ) ||
        ( pNalu->pNaluData == NULL ) || ( pNalu->naluDataLength == 0 ) )
    {
        result = H265_RESULT_BAD_PARAM;
    }

    else if( pNalu->naluDataLength < NALU_HEADER_SIZE )
    {
        result = H265_RESULT_MALFORMED_PACKET;
    }

    else if( ( pNalu->nal_unit_type > MAX_NAL_UNIT_TYPE ) ||
             ( pNalu->nal_layer_id > MAX_LAYER_ID ) ||
             ( pNalu->temporal_id > MAX_TEMPORAL_ID ) )
    {
        result = H265_RESULT_BAD_PARAM;
    }

    else if( pCtx->naluCount >= pCtx->naluArrayLength )
    {
        result = H265_RESULT_OUT_OF_MEMORY;
    }

    else
    {
        pCtx->pNaluArray[ pCtx->headIndex ].pNaluData = pNalu->pNaluData;
        pCtx->pNaluArray[ pCtx->headIndex ].naluDataLength = pNalu->naluDataLength;

        pCtx->pNaluArray[ pCtx->headIndex ].nal_unit_type = pNalu->nal_unit_type;
        pCtx->pNaluArray[ pCtx->headIndex ].nal_layer_id = pNalu->nal_layer_id;
        pCtx->pNaluArray[ pCtx->headIndex ].temporal_id = pNalu->temporal_id;

        pCtx->headIndex++;
        pCtx->naluCount++;
    }

    return result;
}

/*-----------------------------------------------------------------------------------------------------*/

H265Result_t H265Packetizer_GetPacket( H265PacketizerContext_t * pCtx,
                                       H265Packet_t * pPacket )
{
    H265Result_t result = H265_RESULT_OK;
    size_t minRequiredSize = 0;
    size_t singleNalSize = 0;
    size_t totalSize = 0;
    size_t nalusToAggregate = 0;
    size_t tempIndex = 0;

    if( ( pCtx == NULL ) || ( pPacket == NULL ) ||
        ( pPacket->pPacketData == NULL ) || ( pPacket->packetDataLength == 0 ) )
    {
        result = H265_RESULT_BAD_PARAM;
    }
    else
    {
        minRequiredSize = NALU_HEADER_SIZE + 1; /* Minimum size for any packet */

        if( pPacket->packetDataLength < minRequiredSize )
        {
            result = H265_RESULT_BAD_PARAM;
        }
        else if( pCtx->naluCount == 0 )
        {
            result = H265_RESULT_NO_MORE_NALUS;
        }
        else
        {
            if( pCtx->currentlyProcessingPacket == H265_FU_PACKET )
            {
                PacketizeFragmentationUnitPacket( pCtx,
                                                  pPacket );
            }
            else
            {
                /* Check if NAL fits in single packet */
                singleNalSize = pCtx->pNaluArray[pCtx->tailIndex].naluDataLength;

                if( singleNalSize <= pPacket->packetDataLength )
                {
                    /* Could fit as single NAL, but check if aggregation possible */
                    if( ( pCtx->naluCount >= 2 ) &&
                        ( pCtx->tailIndex < pCtx->naluArrayLength - 1 ) )
                    {
                        /* Calculate maximum NALs we can aggregate */
                        totalSize = AP_HEADER_SIZE;
                        nalusToAggregate = 0;
                        tempIndex = pCtx->tailIndex;

                        /* Keep adding NALs until we can't fit more */
                        while( ( tempIndex < pCtx->naluArrayLength ) &&
                               ( nalusToAggregate < pCtx->naluCount ) )
                        {
                            size_t nextNalSize = pCtx->pNaluArray[tempIndex].naluDataLength;
                            size_t headerSize = AP_NALU_LENGTH_FIELD_SIZE;

                            /* Check if this NAL would fit */
                            if( ( totalSize + nextNalSize + headerSize ) <= pPacket->packetDataLength )
                            {
                                totalSize += nextNalSize + headerSize;
                                nalusToAggregate++;
                                tempIndex++;
                            }
                            else
                            {
                                break;
                            }
                        }

                        /* If we can aggregate at least 2 NALs */
                        if( nalusToAggregate >= 2 )
                        {
                            PacketizeAggregationPacket( pCtx,
                                                        pPacket );
                        }
                        else
                        {
                            PacketizeSingleNaluPacket( pCtx,
                                                       pPacket );
                        }
                    }
                    else
                    {
                        PacketizeSingleNaluPacket( pCtx,
                                                   pPacket );
                    }
                }
                else
                {
                    /* NAL too big, use fragmentation */
                    PacketizeFragmentationUnitPacket( pCtx,
                                                      pPacket );
                }
            }
        }
    }

    return result;
}

/*-----------------------------------------------------------------------------------------------------*/
