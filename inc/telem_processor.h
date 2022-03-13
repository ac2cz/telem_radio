/*
 * telem_processor.c
 *
 *  Created on: Feb 24, 2022
 *      Author: g0kla
 */
#ifndef TELEM_PROCESSOR_C_
#define TELEM_PROCESSOR_C_

#include <stdint.h>
#include <duv_telem_layout.h>

/* Store the values for the telemetry modulator */
#define DUV_BPS 200
#define DUV_PACKET_LENGTH 96  // 4 header bytes, 60 payload bytes, 32 check bytes
#define DUV_DATA_LENGTH 64
#define DUV_PARITIES_LENGTH 32
#define BITS_PER_10b_WORD 10

/*
 * Set the running disparity to zero.  This is called just once at startup,
 * but we may call it more than once if we are running test routines.
 */
void init_rd_state();

/*
 * Given a frame type, gather and populate the duv telemetry packet
 */
int gather_duv_telemetry(int type, duv_packet_t *packet);

/*
 * Takes an array of bytes as input and encodes them as 10 bits words with RS parities
 */
void encode_duv_telem_packet(unsigned char *packet, uint16_t *encoded_packet);

/*
 * Get the next bit for the encoded packet.
 */
int get_next_bit();

/*
 * Initialize the telemetry processor ready to send telemetry.  This should be called
 * whenever the telemetry is stopped and restarted.
 */
int init_telemetry_processor();

/*
 * Self test functions
 */
unsigned char * set_test_packet();
int test_gather_duv_telemetry();
int test_telem_encoder(unsigned char *packet, uint16_t *encoded_packet);
int test_encode_packet();
unsigned char reverse_8b10b_lookup(uint16_t word);
int test_rs_encoder();
int test_sync_word();
int test_get_next_bit();

#endif /* TELEM_PROCESSOR_C_ */
