/*
 * telem_processor.c
 *
 *  Created on: Feb 24, 2022
 *      Author: g0kla
 */
#ifndef TELEM_PROCESSOR_C_
#define TELEM_PROCESSOR_C_

#include <stdint.h>

/* Store the values for the telemetry modulator */
#define DUV_BPS 200
#define DUV_PACKET_LENGTH 96  // 4 header bytes, 60 payload bytes, 32 check bytes
#define DUV_DATA_LENGTH 64
#define DUV_PARITIES_LENGTH 32
#define BITS_PER_10b_WORD 10
#define ONE_VALUE 0.01
#define ZERO_VALUE -0.01


/**
 * Takes an array of bytes as input and encodes them as 10 bits words with RS parities
 */
void encode_duv_telem_packet(unsigned char *packet, uint16_t *encoded_packet);

int test_telem_encoder();

#endif /* TELEM_PROCESSOR_C_ */
