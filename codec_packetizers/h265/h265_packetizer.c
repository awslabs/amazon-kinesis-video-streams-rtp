/* Standard includes. */
#include <string.h>
#include <stdio.h>

/* API includes. */
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

static void PacketizeSingleNaluPacket(H265PacketizerContext_t *pCtx,
                                      H265Packet_t *pPacket);

static void PacketizeFragmentationUnitPacket(H265PacketizerContext_t *pCtx,
                                             H265Packet_t *pPacket);

static void PacketizeAggregationPacket(H265PacketizerContext_t *pCtx,
                                       H265Packet_t *pPacket);

/*-----------------------------------------------------------*/

static void PacketizeSingleNaluPacket(H265PacketizerContext_t *pCtx,
                                      H265Packet_t *pPacket)
{
    uint16_t donl = pCtx->pNaluArray[pCtx->tailIndex].don;

    printf("Processing single NAL with DON: %u\n", donl);  // Debug print
    // 1. Copy NAL header from context to output
    memcpy(&(pPacket->pPacketData[0]),
           &(pCtx->pNaluArray[pCtx->tailIndex].pNaluData[0]),
           NALU_HEADER_SIZE);

    if (pCtx->spropMaxDonDiff > 0)
    {

        pPacket->pPacketData[NALU_HEADER_SIZE] = (donl >> 8) & 0xFF;     // MSB
        pPacket->pPacketData[NALU_HEADER_SIZE + 1] = donl & 0xFF;        // LSB

        printf("Writing DONL bytes: 0x%02X 0x%02X\n",                     // Debug print
            pPacket->pPacketData[NALU_HEADER_SIZE],
            pPacket->pPacketData[NALU_HEADER_SIZE + 1]);

        // 3. Copy payload from context to output (after DONL)
        memcpy(&(pPacket->pPacketData[NALU_HEADER_SIZE + DONL_FIELD_SIZE]),
               &(pCtx->pNaluArray[pCtx->tailIndex].pNaluData[NALU_HEADER_SIZE]),
               pCtx->pNaluArray[pCtx->tailIndex].naluDataLength - NALU_HEADER_SIZE);

        pPacket->packetDataLength = pCtx->pNaluArray[pCtx->tailIndex].naluDataLength + DONL_FIELD_SIZE;
    }
    else
    {
        // Without DONL, copy payload right after header
        memcpy(&(pPacket->pPacketData[NALU_HEADER_SIZE]),
               &(pCtx->pNaluArray[pCtx->tailIndex].pNaluData[NALU_HEADER_SIZE]),
               pCtx->pNaluArray[pCtx->tailIndex].naluDataLength - NALU_HEADER_SIZE);

        pPacket->packetDataLength = pCtx->pNaluArray[pCtx->tailIndex].naluDataLength;
    }

    /* Move to next NAL unit */
    pCtx->tailIndex += 1;
    pCtx->naluCount -= 1;
    pCtx->currentDon += 1;
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

static void PacketizeFragmentationUnitPacket(H265PacketizerContext_t *pCtx,
                                             H265Packet_t *pPacket)
{
    uint8_t fuHeader = 0;
    uint8_t *pNaluData = pCtx->pNaluArray[pCtx->tailIndex].pNaluData;

    char debugBuffer[500]; // Buffer to hold debug strings                                                       // for printf

    /* First fragment? */
    if (pCtx->currentlyProcessingPacket == H265_PACKET_NONE) // True for first packet
    {
        /* Initialize FU state */
        pCtx->currentlyProcessingPacket = H265_FU_PACKET; // Mark we're in FU mode

        /* Create PayloadHdr from original NAL unit */
        uint8_t origF = pNaluData[0] & HEVC_NALU_HEADER_F_MASK;
        uint8_t origLayerId = (pNaluData[0] & 0x01) << 5 |
                              (pNaluData[1] & 0xF8) >> 3;
        uint8_t origTid = pNaluData[1] & HEVC_NALU_HEADER_TID_MASK;

        /* Construct PayloadHdr */
        uint16_t payloadHdr =
            ((origF << 7) |          // F bit in MSB of first byte
             (FU_PACKET_TYPE << 1) | // Type = 49
             ((origLayerId >> 5) & 1)) &
            0xFF; // First bit of LayerId

        sprintf(debugBuffer, "Initial payloadHdr: 0x%02X\n", payloadHdr);
        printf("%s", debugBuffer); // printf

        // Add second byte
        payloadHdr = (payloadHdr << 8) |           // Shift first byte
                     ((origLayerId & 0x1F) << 3) | // Rest of LayerId
                     origTid;                      // TID

        sprintf(debugBuffer, "Final payloadHdr: 0x%04X\n", payloadHdr);
        printf("%s", debugBuffer);

        sprintf(debugBuffer, "First byte: 0x%02X\n", (payloadHdr >> 8) & 0xFF);
        printf("%s", debugBuffer);

        sprintf(debugBuffer, "Second byte: 0x%02X\n", payloadHdr & 0xFF);
        printf("%s", debugBuffer); // printf

        pCtx->fuPacketizationState.payloadHdr = payloadHdr;

        sprintf(debugBuffer, "State payloadHdr: 0x%04X\n", pCtx->fuPacketizationState.payloadHdr);
        printf("%s", debugBuffer); // printf

        /* Skip original NAL header (2 bytes) */
        pCtx->fuPacketizationState.naluDataIndex = NALU_HEADER_SIZE;
        pCtx->fuPacketizationState.remainingNaluLength =
            pCtx->pNaluArray[pCtx->tailIndex].naluDataLength - NALU_HEADER_SIZE;
        pCtx->fuPacketizationState.donl = pCtx->currentDon; // Initialize lastDon
        /* Set S bit for first fragment */

        fuHeader |= FU_HEADER_S_BIT_MASK;
    }

    /* Calculate available payload size */
    size_t headerSize = NALU_HEADER_SIZE + 1; // PayloadHdr(2) + FUHeader(1)

    if (pCtx->spropMaxDonDiff > 0)
    {
        if (fuHeader & FU_HEADER_S_BIT_MASK)
        {
            headerSize += DONL_FIELD_SIZE; // 2 bytes for DONL in first fragment
        }
        else
        {
            headerSize += AP_DOND_SIZE; // 1 byte for DOND in subsequent fragments
        }
    }

    /* Calculate maximum payload size for this packet */
    size_t maxPayloadSize = (pPacket->maxPacketSize > headerSize) ? pPacket->maxPacketSize - headerSize : 0;

    /* Calculate actual payload size for this fragment */
    size_t payloadSize = (pCtx->fuPacketizationState.remainingNaluLength <= maxPayloadSize) ? pCtx->fuPacketizationState.remainingNaluLength : maxPayloadSize;

    /* Add debug prints HERE */ // printf
    sprintf(debugBuffer, "\nFragment Details:\n");
    printf("%s", debugBuffer);
    sprintf(debugBuffer, "maxPayloadSize: %zu\n", maxPayloadSize);
    printf("%s", debugBuffer);
    sprintf(debugBuffer, "remainingNaluLength: %zu\n", pCtx->fuPacketizationState.remainingNaluLength);
    printf("%s", debugBuffer);
    sprintf(debugBuffer, "payloadSize: %zu\n", payloadSize);
    printf("%s", debugBuffer);
    sprintf(debugBuffer, "naluDataIndex: %zu\n", pCtx->fuPacketizationState.naluDataIndex);
    printf("%s", debugBuffer);

    /* Set E bit if this is the last fragment */
    if (pCtx->fuPacketizationState.remainingNaluLength <= maxPayloadSize)
    {
        fuHeader |= FU_HEADER_E_BIT_MASK;
        sprintf(debugBuffer, "\nSetting END bit for last fragment (remaining: %zu bytes)\n",
                pCtx->fuPacketizationState.remainingNaluLength);
        printf("%s", debugBuffer);
    }

    /* Set FU type from original NAL unit */
    fuHeader |= ((pNaluData[0] >> 1) & 0x3F);

    /*
    pPacket->pPacketData:
    [PayloadHdr1][PayloadHdr2][FU Header][Payload...]
    ↑           ↑           ↑          ↑
    offset=0    offset=1    offset=2   offset=3
    */

    /* Build packet */
    size_t offset = 0;

    sprintf(debugBuffer, "Before writing - payloadHdr in state: 0x%04X\n", // printf
            pCtx->fuPacketizationState.payloadHdr);
    printf("%s", debugBuffer);

    pPacket->pPacketData[offset] = (pCtx->fuPacketizationState.payloadHdr >> 8) & 0xFF; // offset shows that in which byte we are currently in our PayloadHdr

    sprintf(debugBuffer, "After writing first byte: 0x%02X\n", pPacket->pPacketData[offset]); // printf
    printf("%s", debugBuffer);
    offset++;

    pPacket->pPacketData[offset] = pCtx->fuPacketizationState.payloadHdr & 0xFF;

    sprintf(debugBuffer, "After writing second byte: 0x%02X\n", pPacket->pPacketData[offset]); // printf
    printf("%s", debugBuffer);
    offset++;

    pPacket->pPacketData[offset] = fuHeader;

    sprintf(debugBuffer, "After writing FU header: 0x%02X\n", pPacket->pPacketData[offset]); // printf
    printf("%s", debugBuffer);
    offset++;


    if (pCtx->spropMaxDonDiff > 0)
    {
        if (fuHeader & FU_HEADER_S_BIT_MASK)
        {
            /* Write DONL for first fragment */
            uint16_t donl = pCtx->currentDon;
            pPacket->pPacketData[offset++] = (donl >> 8) & 0xFF; // MSB
            pPacket->pPacketData[offset++] = donl & 0xFF;        // LSB
            pCtx->fuPacketizationState.donl = pCtx->currentDon;  // Save for DOND calculation
        }
        else
        {
            /* Write DOND for subsequent fragments */
            uint8_t dond = 0; // DOND is always 0 for fragments of the same NAL unit
            pPacket->pPacketData[offset++] = dond;
        }
    }

    /* Copy payload */
    if (payloadSize > 0)
    {
        memcpy(&pPacket->pPacketData[offset],
               &pNaluData[pCtx->fuPacketizationState.naluDataIndex],
               payloadSize);
    }

    /* Update state */
    pCtx->fuPacketizationState.naluDataIndex += payloadSize;
    pCtx->fuPacketizationState.remainingNaluLength -= payloadSize;
    pPacket->packetDataLength = offset + payloadSize;

    /* Print state after updates */
    sprintf(debugBuffer, "After state update:\n");
    printf("%s", debugBuffer);

    sprintf(debugBuffer, "Remaining length: %zu\n", pCtx->fuPacketizationState.remainingNaluLength);
    printf("%s", debugBuffer);

    sprintf(debugBuffer, "NAL data index: %zu\n", pCtx->fuPacketizationState.naluDataIndex);
    printf("%s", debugBuffer);

    /* Check if this was the last fragment */
    if (pCtx->fuPacketizationState.remainingNaluLength == 0)
    {
        printf("Processing complete - resetting state\n");
        pCtx->currentlyProcessingPacket = H265_PACKET_NONE;
        pCtx->tailIndex++;
        pCtx->naluCount--;
        pCtx->currentDon++;
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

static void PacketizeAggregationPacket(H265PacketizerContext_t *pCtx,
                                       H265Packet_t *pPacket)
{
    char debugBuffer[200];
    size_t offset = 0;
    uint8_t naluCount = 0;

    sprintf(debugBuffer, "\nStarting aggregation process\n");
    printf("%s", debugBuffer);

    /* First scan phase: determine how many NALs we can aggregate and find header field values */
    uint8_t f_bit = 0;
    uint8_t min_layer_id = 63;
    uint8_t min_tid = 7;

    /* Track space usage for NALs */
    size_t temp_offset = 2; // Start after PayloadHdr
    size_t availableSize = pPacket->maxPacketSize;
    size_t scan_index = pCtx->tailIndex; // Start with first NAL

    sprintf(debugBuffer, "Initial state - Available size: %zu, Starting index: %zu\n",
            availableSize, scan_index);
    printf("%s", debugBuffer);

    /* Scan NALs to find minimums and check size */
    while (scan_index < pCtx->naluArrayLength && pCtx->naluCount > naluCount)
    {
        /* Calculate space needed for this NAL */
        size_t needed_size = AP_NALU_LENGTH_FIELD_SIZE +                  // Size field (2)
                             pCtx->pNaluArray[scan_index].naluDataLength; // NAL data

        sprintf(debugBuffer, "Scanning NAL %u - Need size: %zu\n", naluCount, needed_size);
        printf("%s", debugBuffer);

        /* Add DONL/DOND size if needed */
        if (pCtx->spropMaxDonDiff > 0)
        {
            needed_size += (naluCount == 0) ? DONL_FIELD_SIZE : AP_DOND_SIZE; // DONL(2) or DOND(1)
            sprintf(debugBuffer, "Added DONL/DOND, new needed size: %zu\n", needed_size);
            printf("%s", debugBuffer);
        }

        /* Check if this NAL would fit */
        if (temp_offset + needed_size > availableSize)
        {
            sprintf(debugBuffer, "NAL won't fit - required: %zu, available: %zu\n",
                    temp_offset + needed_size, availableSize);
            printf("%s", debugBuffer);
            break; // Won't fit, stop scanning
        }

        /* Get header fields from this NAL */
        uint8_t *nalu_data = pCtx->pNaluArray[scan_index].pNaluData;
        f_bit |= (nalu_data[0] & HEVC_NALU_HEADER_F_MASK);

        // uint8_t layer_id = (nalu_data[0] & 0x01) << 5 |
        //                    (nalu_data[1] & 0xF8) >> 3;
        uint8_t layer_id = pCtx->pNaluArray[scan_index].nal_layer_id;
        // uint8_t tid = nalu_data[1] & HEVC_NALU_HEADER_TID_MASK;
        uint8_t tid = pCtx->pNaluArray[scan_index].temporal_id;

        sprintf(debugBuffer, "NAL %u - Layer ID: %u, TID: %u\n", naluCount, layer_id, tid);
        printf("%s", debugBuffer);

        /* Update minimum values */
        min_layer_id = H265_MIN(min_layer_id, layer_id);
        min_tid = H265_MIN(min_tid, tid);

        temp_offset += needed_size;
        scan_index++;
        naluCount++;
    }

    sprintf(debugBuffer, "Scan complete - Will aggregate %u NALs\n", naluCount);
    printf("%s", debugBuffer);

    /* Write PayloadHdr - Important: correct byte order */
    pPacket->pPacketData[0] = (AP_PACKET_TYPE << 1) | (min_layer_id >> 5);
    pPacket->pPacketData[1] = ((min_layer_id & 0x1F) << 3) | min_tid;
    offset = 2;

    /* Write first NAL size and data */
    uint16_t naluSize = pCtx->pNaluArray[pCtx->tailIndex].naluDataLength;
    pPacket->pPacketData[offset++] = (naluSize >> 8) & 0xFF;
    pPacket->pPacketData[offset++] = naluSize & 0xFF;

    /* Process first NAL unit */
    if (pCtx->spropMaxDonDiff > 0)
    {
        /* Add DONL */
        uint16_t donl = pCtx->pNaluArray[pCtx->tailIndex].don;
        pPacket->pPacketData[offset++] = (donl >> 8) & 0xFF;
        pPacket->pPacketData[offset++] = donl & 0xFF;

        sprintf(debugBuffer, "Added DONL: %u\n", donl);
        printf("%s", debugBuffer);
    }


    sprintf(debugBuffer, "Writing first NAL - Size: %u, Offset: %zu\n", naluSize, offset);
    printf("%s", debugBuffer);

    memcpy(&pPacket->pPacketData[offset],
           pCtx->pNaluArray[pCtx->tailIndex].pNaluData,
           naluSize);
    offset += naluSize;

    uint16_t lastDon = pCtx->pNaluArray[pCtx->tailIndex].don;
    pCtx->tailIndex++;
    pCtx->naluCount--;

    /* Process subsequent NAL units */
    for (uint8_t i = 1; i < naluCount; i++)
    {

        sprintf(debugBuffer, "Processing NAL %u\n", i);
        printf("%s", debugBuffer);

        /* Write NAL size and data */
        naluSize = pCtx->pNaluArray[pCtx->tailIndex].naluDataLength;
        pPacket->pPacketData[offset++] = (naluSize >> 8) & 0xFF;
        pPacket->pPacketData[offset++] = naluSize & 0xFF;

        /* Add DOND if needed */
        if (pCtx->spropMaxDonDiff > 0)
        {
            // Calculate DOND as difference between current and last DON
            uint16_t currentDon = pCtx->pNaluArray[pCtx->tailIndex].don;
            uint8_t dond = currentDon - lastDon;
            pPacket->pPacketData[offset++] = dond;
            printf("Added DOND: %u (current DON: %u, last DON: %u)\n", 
                   dond, currentDon, lastDon);
            lastDon = currentDon;

            sprintf(debugBuffer, "Added DOND: %u\n", dond);
            printf("%s", debugBuffer);
        }

        sprintf(debugBuffer, "Writing NAL - Size: %u, Offset: %zu\n", naluSize, offset);
        printf("%s", debugBuffer);

        memcpy(&pPacket->pPacketData[offset],
               pCtx->pNaluArray[pCtx->tailIndex].pNaluData,
               naluSize);
        offset += naluSize;

        
        pCtx->tailIndex++;
        pCtx->naluCount--;
    }

    /* Set final packet length */
    pPacket->packetDataLength = offset;

    sprintf(debugBuffer, "Aggregation complete - Total size: %zu\n", offset);
    printf("%s", debugBuffer);
}

/*-----------------------------------------------------------------------------------------------------*/

H265Result_t H265Packetizer_Init(H265PacketizerContext_t *pCtx,
                                 H265Nalu_t *pNaluArray,
                                 size_t naluArrayLength,
                                 uint8_t spropMaxDonDiff,
                                 uint16_t maxPacketSize)
{
    if (pCtx == NULL || pNaluArray == NULL || maxPacketSize == 0)
    {
        return H265_RESULT_BAD_PARAM;
    }

    H265Result_t result = H265_RESULT_OK;

    /* Parameter validation */
    if (naluArrayLength == 0)
    {
        result = H265_RESULT_BAD_PARAM;
    }

    if (result == H265_RESULT_OK)
    {
        /* Initialize NAL array info */
        pCtx->pNaluArray = pNaluArray;
        pCtx->naluArrayLength = naluArrayLength;

        /* Initialize array indices */
        pCtx->headIndex = 0;
        pCtx->tailIndex = 0;
        pCtx->naluCount = 0;

        /* Initialize configuration */
        pCtx->spropMaxDonDiff = spropMaxDonDiff;
        pCtx->maxPacketSize = maxPacketSize;
        pCtx->donPresent = (spropMaxDonDiff > 0);

        /* Initialize packet state */
        pCtx->currentlyProcessingPacket = H265_PACKET_NONE;

        /* Initialize FU state */
        pCtx->fuPacketizationState.payloadHdr = 0;
        pCtx->fuPacketizationState.fuHeader = 0;
        pCtx->fuPacketizationState.naluDataIndex = 0;
        pCtx->fuPacketizationState.remainingNaluLength = 0;
        pCtx->fuPacketizationState.donl = 0;

        /* Initialize AP state */
        pCtx->apPacketizationState.payloadHdr = 0;
        pCtx->apPacketizationState.totalSize = 0;
        pCtx->apPacketizationState.naluCount = 0;

        /* Initialize DON tracking */
        pCtx->currentDon = 0;
    }

    return result;
}

/*-----------------------------------------------------------------------------------------------------*/

H265Result_t H265Packetizer_AddFrame(H265PacketizerContext_t *pCtx,
                                     H265Frame_t *pFrame)
{
    H265Result_t result = H265_RESULT_OK;
    H265Nalu_t nalu;
    size_t currentIndex = 0, naluStartIndex = 0, remainingFrameLength;
    uint8_t startCode1[] = {0x00, 0x00, 0x00, 0x01}; // 4-byte start code
    uint8_t startCode2[] = {0x00, 0x00, 0x01};       // 3-byte start code
    uint8_t firstStartCode = 1;

    /* Parameter validation */
    if ((pCtx == NULL) ||
        (pFrame == NULL) ||
        (pFrame->pFrameData == NULL) ||
        (pFrame->frameDataLength == 0))
    {
        return H265_RESULT_BAD_PARAM;
    }

    /* Process frame data */
    while ((result == H265_RESULT_OK) &&
           (currentIndex < pFrame->frameDataLength))
    {
        remainingFrameLength = pFrame->frameDataLength - currentIndex;

        /* Check for 4-byte start code */
        if (remainingFrameLength >= sizeof(startCode1))
        {
            if (memcmp(&(pFrame->pFrameData[currentIndex]),
                       &(startCode1[0]),
                       sizeof(startCode1)) == 0)
            {
                if (firstStartCode == 1)
                {
                    firstStartCode = 0;
                }
                else
                {
                    /* Create NAL unit from data between start codes */
                    nalu.pNaluData = &(pFrame->pFrameData[naluStartIndex]);
                    nalu.naluDataLength = currentIndex - naluStartIndex;

                    /* Extract H.265 header fields if NAL unit is big enough */
                    if (nalu.naluDataLength >= NALU_HEADER_SIZE)
                    {
                        /* First byte: F bit and Type */
                        nalu.nal_unit_type = (nalu.pNaluData[0] >> 1) & 0x3F;

                        /* LayerId spans both bytes */
                        nalu.nal_layer_id = ((nalu.pNaluData[0] & 0x01) << 5) |
                                            ((nalu.pNaluData[1] >> 3) & 0x1F);

                        /* Second byte: TID */
                        nalu.temporal_id = nalu.pNaluData[1] & 0x07;

                        result = H265Packetizer_AddNalu(pCtx, &nalu);
                    }
                    else
                    {
                        result = H265_RESULT_MALFORMED_PACKET;
                    }
                }

                naluStartIndex = currentIndex + sizeof(startCode1);
                currentIndex = naluStartIndex;
                continue;
            }
        }

        /* Check for 3-byte start code */
        if (remainingFrameLength >= sizeof(startCode2))
        {
            if (memcmp(&(pFrame->pFrameData[currentIndex]),
                       &(startCode2[0]),
                       sizeof(startCode2)) == 0)
            {
                if (firstStartCode == 1)
                {
                    firstStartCode = 0;
                }
                else
                {
                    /* Create NAL unit from data between start codes */
                    nalu.pNaluData = &(pFrame->pFrameData[naluStartIndex]);
                    nalu.naluDataLength = currentIndex - naluStartIndex;

                    /* Extract H.265 header fields if NAL unit is big enough */
                    if (nalu.naluDataLength >= NALU_HEADER_SIZE)
                    {
                        nalu.nal_unit_type = (nalu.pNaluData[0] >> 1) & 0x3F;
                        nalu.nal_layer_id = ((nalu.pNaluData[0] & 0x01) << 5) |
                                            ((nalu.pNaluData[1] >> 3) & 0x1F);
                        nalu.temporal_id = nalu.pNaluData[1] & 0x07;

                        result = H265Packetizer_AddNalu(pCtx, &nalu);
                    }
                    else
                    {
                        result = H265_RESULT_MALFORMED_PACKET;
                    }
                }

                naluStartIndex = currentIndex + sizeof(startCode2);
                currentIndex = naluStartIndex;
                continue;
            }
        }

        currentIndex++;
    }

    /* Handle last NAL unit in frame */
    if ((result == H265_RESULT_OK) && (naluStartIndex > 0))
    {
        nalu.pNaluData = &(pFrame->pFrameData[naluStartIndex]);
        nalu.naluDataLength = pFrame->frameDataLength - naluStartIndex;

        if (nalu.naluDataLength >= NALU_HEADER_SIZE)
        {
            nalu.nal_unit_type = (nalu.pNaluData[0] >> 1) & 0x3F;
            nalu.nal_layer_id = ((nalu.pNaluData[0] & 0x01) << 5) |
                                ((nalu.pNaluData[1] >> 3) & 0x1F);
            nalu.temporal_id = nalu.pNaluData[1] & 0x07;

            result = H265Packetizer_AddNalu(pCtx, &nalu);
        }
        else
        {
            result = H265_RESULT_MALFORMED_PACKET;
        }
    }

    return result;
}

/*-----------------------------------------------------------------------------------------------------*/

H265Result_t H265Packetizer_AddNalu(H265PacketizerContext_t *pCtx,
                                    H265Nalu_t *pNalu)
{

    H265Result_t result = H265_RESULT_OK;

    /* Check for minimum NAL unit size (2-byte header) */
    if (pNalu->naluDataLength < NALU_HEADER_SIZE)
    {
        return H265_RESULT_MALFORMED_PACKET;
    }

    /* Check for available space in array */
    if (pCtx->naluCount >= pCtx->naluArrayLength)
    {
        return H265_RESULT_OUT_OF_MEMORY;
    }


    /* Store NAL unit */
    pCtx->pNaluArray[pCtx->headIndex].pNaluData = pNalu->pNaluData;
    pCtx->pNaluArray[pCtx->headIndex].naluDataLength = pNalu->naluDataLength;

    /* Store H.265 specific header fields */
    pCtx->pNaluArray[pCtx->headIndex].nal_unit_type = pNalu->nal_unit_type;
    pCtx->pNaluArray[pCtx->headIndex].nal_layer_id = pNalu->nal_layer_id;
    pCtx->pNaluArray[pCtx->headIndex].temporal_id = pNalu->temporal_id;

    /* Add this line to copy the DON value */
    pCtx->pNaluArray[pCtx->headIndex].don = pNalu->don;

    /* Add debug print */
    printf("Adding NAL unit with DON: %u at index: %zu\n", pNalu->don, pCtx->headIndex);

    /* Update indices */
    pCtx->headIndex++;
    pCtx->naluCount++;

    return result;
}

/*-----------------------------------------------------------------------------------------------------*/

H265Result_t H265Packetizer_GetPacket(H265PacketizerContext_t *pCtx,
                                      H265Packet_t *pPacket)
{
    H265Result_t result = H265_RESULT_OK;

    if (pCtx == NULL || pPacket == NULL || pPacket->pPacketData == NULL || pPacket->maxPacketSize == 0)
    {
        return H265_RESULT_BAD_PARAM;
    }

    /* Strict size validation for all packet types */
    size_t minRequiredSize = NALU_HEADER_SIZE + 1;  // Minimum size for any packet
    if (pCtx->spropMaxDonDiff > 0)
    {
        minRequiredSize += DONL_FIELD_SIZE;  // Add DONL size if needed
    }

    // Add size validation here
    if (pPacket->maxPacketSize < minRequiredSize)
    {
        return H265_RESULT_BAD_PARAM;
    }

    /* Check if NALs available */
    if (pCtx->naluCount == 0)
    {
        return H265_RESULT_NO_MORE_NALUS;
    }

    /* Main processing */
    if (pCtx->currentlyProcessingPacket == H265_FU_PACKET)
    {
        PacketizeFragmentationUnitPacket(pCtx, pPacket);
    }
    else
    {
        /* Check if NAL fits in single packet */
        size_t singleNalSize = pCtx->pNaluArray[pCtx->tailIndex].naluDataLength;
        if (pCtx->spropMaxDonDiff > 0)
        {
            singleNalSize += DONL_FIELD_SIZE;
        }

        if (singleNalSize <= pPacket->maxPacketSize)
        {
            /* Could fit as single NAL, but check if aggregation possible */
            if ((pCtx->naluCount >= 2) &&
                (pCtx->tailIndex < pCtx->naluArrayLength - 1))
            {
                /* Calculate maximum NALs we can aggregate */
                size_t totalSize = AP_HEADER_SIZE;
                size_t nalusToAggregate = 0;
                size_t tempIndex = pCtx->tailIndex;

                /* Keep adding NALs until we can't fit more */
                while ((tempIndex < pCtx->naluArrayLength) &&
                       (nalusToAggregate < pCtx->naluCount))
                {
                    size_t nextNalSize = pCtx->pNaluArray[tempIndex].naluDataLength;
                    size_t headerSize = AP_NALU_LENGTH_FIELD_SIZE;

                    /* Add DONL/DOND if needed */
                    if (pCtx->spropMaxDonDiff > 0)
                    {
                        if (nalusToAggregate == 0)
                        {
                            headerSize += DONL_FIELD_SIZE;
                        }
                        else
                        {
                            headerSize += AP_DOND_SIZE;
                        }
                    }

                    /* Check if this NAL would fit */
                    if ((totalSize + nextNalSize + headerSize) <= pPacket->maxPacketSize)
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
                if (nalusToAggregate >= 2)
                {
                    PacketizeAggregationPacket(pCtx, pPacket);
                }
            }
            else
            {
                PacketizeSingleNaluPacket(pCtx, pPacket);
            }
        }
        else
        {
            /* NAL too big, use fragmentation */
            PacketizeFragmentationUnitPacket(pCtx, pPacket);
        }
    }

    return result; // Added return statement at the end
}

/*-----------------------------------------------------------------------------------------------------*/