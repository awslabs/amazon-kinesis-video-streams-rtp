/* Unity includes. */
#include "unity.h"
#include "catch_assert.h"

/* Standard includes. */
#include <string.h>
#include <stdint.h>

/* API includes. */
#include "h265_packetizer.h"

/* Defines */
#define MAX_NALU_LENGTH 5 * 1024
#define MAX_FRAME_LENGTH 10 * 1024
#define MAX_NALUS_IN_A_FRAME 512
#define MAX_H265_PACKET_LENGTH 1500

/* Global buffers */
uint8_t frameBuffer[MAX_FRAME_LENGTH];
uint8_t packetBuffer[MAX_H265_PACKET_LENGTH];

/* Test setup and teardown */
void setUp(void)
{
    memset(frameBuffer, 0, sizeof(frameBuffer));
    memset(packetBuffer, 0, sizeof(packetBuffer));
}

void tearDown(void)
{
    /* Nothing to clean up */
}

/**
 * @brief Test H265 packetization with single NAL unit
 */
void test_H265_Packetizer_SingleNal(void)
{
    H265PacketizerContext_t ctx;
    H265Result_t result;
    H265Nalu_t naluArray[MAX_NALUS_IN_A_FRAME];
    H265Packet_t packet;

    /* Test NAL unit (SPS) */
    uint8_t nalData[] = {
        0x42, 0x01,      /* NAL header (Type=32/VPS) */
        0x01, 0x02, 0x03 /* NAL payload */
    };

    /* Initialize context */
    result = H265Packetizer_Init(&ctx, naluArray, MAX_NALUS_IN_A_FRAME, 0, MAX_H265_PACKET_LENGTH);
    TEST_ASSERT_EQUAL(H265_RESULT_OK, result);

    /* Add NAL unit */
    H265Nalu_t nalu = {
        .pNaluData = nalData,
        .naluDataLength = sizeof(nalData),
        .nal_unit_type = 32, // VPS
        .nal_layer_id = 0,
        .temporal_id = 1};
    result = H265Packetizer_AddNalu(&ctx, &nalu);
    TEST_ASSERT_EQUAL(H265_RESULT_OK, result);
    TEST_ASSERT_EQUAL(1, ctx.naluCount);

    /* Get packet */
    packet.pPacketData = packetBuffer;
    packet.maxPacketSize = MAX_H265_PACKET_LENGTH;
    result = H265Packetizer_GetPacket(&ctx, &packet);
    TEST_ASSERT_EQUAL(H265_RESULT_OK, result);
    TEST_ASSERT_EQUAL(sizeof(nalData), packet.packetDataLength);

    /* Verify packet data */
    TEST_ASSERT_EQUAL_MEMORY(nalData, packet.pPacketData, packet.packetDataLength);
}


/**
 * @brief Test H265 packetization with fragmentation
 */
void test_H265_Packetizer_Fragmentation(void)
{
    H265PacketizerContext_t ctx;
    H265Result_t result;
    H265Nalu_t naluArray[MAX_NALUS_IN_A_FRAME];
    H265Packet_t packet;
    uint8_t packetBuffer[1500];  // Added packet buffer


    // Initialize packet counters
    uint32_t packetCount = 0;
    uint32_t startPackets = 0;
    uint32_t middlePackets = 0;
    uint32_t endPackets = 0;


    /* Create large NAL unit that needs fragmentation */
    uint8_t nalData[2000] = {
        0x40, 0x01,     /* First byte: F=0, Type=32 (VPS), second byte: LayerId=0, TID=1 */
        0x00, 0x00      /* Start of payload */
    };
    /* Fill rest of payload */
    memset(nalData + 4, 0xCC, sizeof(nalData) - 4);

     /* Fill rest of payload */
     memset(nalData + 4, 0xCC, sizeof(nalData) - 4);

     printf("\nInitial NAL size: %zu bytes\n", sizeof(nalData));

    /* Initialize context with small packet size to force fragmentation */
    result = H265Packetizer_Init(&ctx, naluArray, MAX_NALUS_IN_A_FRAME, 0, 100);
    TEST_ASSERT_EQUAL(H265_RESULT_OK, result);

    /* Add large NAL unit */
    H265Nalu_t nalu = {
        .pNaluData = nalData,
        .naluDataLength = sizeof(nalData),
        .nal_unit_type = 32,    // VPS
        .nal_layer_id = 0,
        .temporal_id = 1
    };
    result = H265Packetizer_AddNalu(&ctx, &nalu);
    TEST_ASSERT_EQUAL(H265_RESULT_OK, result);
    TEST_ASSERT_EQUAL(1, ctx.naluCount);

    /* Get first fragment */
    packet.pPacketData = packetBuffer;
    packet.maxPacketSize = 100;
    result = H265Packetizer_GetPacket(&ctx, &packet);
    TEST_ASSERT_EQUAL(H265_RESULT_OK, result);

    if (packet.pPacketData[2] & 0x80) {  // Start bit set
        startPackets++;
        printf("\nFirst packet detected (Start bit set)\n");
    }
    packetCount++;  // Count first packet
    
    /* Verify first fragment */
    TEST_ASSERT_EQUAL(49, (packet.pPacketData[0] >> 1) & 0x3F);    // Type = 49 for FU
    TEST_ASSERT_EQUAL(0, packet.pPacketData[0] >> 7);              // F bit = 0
    TEST_ASSERT_EQUAL(0, (packet.pPacketData[0] & 0x01));          // LayerId[5] = 0
    TEST_ASSERT_EQUAL(0, (packet.pPacketData[1] >> 3));            // LayerId[4:0] = 0
    TEST_ASSERT_EQUAL(1, packet.pPacketData[1] & 0x07);            // TID = 1
    TEST_ASSERT_EQUAL(0x80, packet.pPacketData[2] & 0x80);         // Start bit set
    TEST_ASSERT_EQUAL(0x00, packet.pPacketData[2] & 0x40);         // End bit not set
    TEST_ASSERT_EQUAL(32, packet.pPacketData[2] & 0x3F);           // Original NAL type

    /* Get middle fragments */
    bool lastFragmentSeen = false;
    do {
        packet.pPacketData = packetBuffer;
        packet.maxPacketSize = 100;
        result = H265Packetizer_GetPacket(&ctx, &packet);
        
        if (result == H265_RESULT_OK && ctx.currentlyProcessingPacket == H265_FU_PACKET) {
            packetCount++;
            
            /* Check packet type */
            if (packet.pPacketData[2] & 0x40) {  // End bit set
                endPackets++;
                lastFragmentSeen = true;
                printf("\nLast packet detected (End bit set)\n");
                /* Verify last fragment */
                TEST_ASSERT_EQUAL(49, (packet.pPacketData[0] >> 1) & 0x3F);  // Type = 49
                TEST_ASSERT_EQUAL(0x40, packet.pPacketData[2] & 0x40);       // End bit set
                TEST_ASSERT_EQUAL(0x00, packet.pPacketData[2] & 0x80);       // Start bit not set
                TEST_ASSERT_EQUAL(32, packet.pPacketData[2] & 0x3F);         // Original NAL type
            } else if (!(packet.pPacketData[2] & 0x80)) {
                middlePackets++;
                /* Verify middle fragment */
                TEST_ASSERT_EQUAL(49, (packet.pPacketData[0] >> 1) & 0x3F);  // Type = 49
                TEST_ASSERT_EQUAL(0x00, packet.pPacketData[2] & 0x80);       // Start bit not set
                TEST_ASSERT_EQUAL(0x00, packet.pPacketData[2] & 0x40);       // End bit not set
                TEST_ASSERT_EQUAL(32, packet.pPacketData[2] & 0x3F);         // Original NAL type
            }

            /* Verify common header fields for all fragments */
            TEST_ASSERT_EQUAL(0, packet.pPacketData[0] >> 7);              // F bit = 0
            TEST_ASSERT_EQUAL(0, (packet.pPacketData[0] & 0x01));          // LayerId[5] = 0
            TEST_ASSERT_EQUAL(0, (packet.pPacketData[1] >> 3));            // LayerId[4:0] = 0
            TEST_ASSERT_EQUAL(1, packet.pPacketData[1] & 0x07);            // TID = 1

            printf("After packet %u:\n", packetCount);
            printf("Remaining length: %zu\n", ctx.fuPacketizationState.remainingNaluLength);
            printf("NAL data index: %zu\n", ctx.fuPacketizationState.naluDataIndex);
        }
    } while (result == H265_RESULT_OK && ctx.currentlyProcessingPacket == H265_FU_PACKET);

    /* Print final packet statistics */
    printf("\nPacket breakdown:\n");
    printf("Start packets:  %u\n", startPackets);
    printf("Middle packets: %u\n", middlePackets);
    printf("End packets:    %u\n", endPackets);
    printf("Total packets:  %u\n", packetCount);

    /* Verify packet counts */
    TEST_ASSERT_EQUAL(1, startPackets);
    TEST_ASSERT_EQUAL(1, endPackets);
    TEST_ASSERT_EQUAL(21, packetCount);
    TEST_ASSERT_TRUE(lastFragmentSeen);

    /* Verify final state */
    TEST_ASSERT_EQUAL(H265_PACKET_NONE, ctx.currentlyProcessingPacket);
    TEST_ASSERT_EQUAL(0, ctx.naluCount);
}
