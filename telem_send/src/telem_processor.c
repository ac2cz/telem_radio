/*
 * telem_processor.c
 *
 *  Created on: Feb 24, 2022
 *      Author: g0kla
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 *
 *
 * The telemetry processor encodes packets and supplies the bits to the
 * audio processor.
 *
 */

#include "../../telem_send/inc/telem_processor.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <sched.h>

#include "debug.h"
#include "config.h"

#include "../../telem_send/inc/telem_thread.h"
#include "../../telem_send/inc/TelemEncoding.h"

/* Forward function definitions */
void encode_duv_telem_packet(unsigned char *packet, uint16_t *encoded_packet);

/* Telemetry modulator variables */
unsigned char parities[DUV_PARITIES_LENGTH];   /* This is the parities calculated by the RS encoder */
uint16_t encoded_packet[2][DUV_PACKET_LENGTH+1]; /* This is the 10b encoded packet with parities. It includes space for SYNC WORD at the end. */
int current_encoded_packet_num; /* We have two encoded packets.  One is being sent and the other is being built. */

/* This test packet is a 101010 sequence of 10b words */
uint16_t encoded_packet_test1[] = {
		0x2aa,0x2aa,0x2aa,0x2aa,0x2aa,0x2aa,0x2aa,0x2aa,
		0x2aa,0x2aa,0x2aa,0x2aa,0x2aa,0x2aa,0x2aa,0x2aa,
		0x2aa,0x2aa,0x2aa,0x2aa,0x2aa,0x2aa,0x2aa,0x2aa,
		0x2aa,0x2aa,0x2aa,0x2aa,0x2aa,0x2aa,0x2aa,0x2aa,
		0x2aa,0x2aa,0x2aa,0x2aa,0x2aa,0x2aa,0x2aa,0x2aa,
		0x2aa,0x2aa,0x2aa,0x2aa,0x2aa,0x2aa,0x2aa,0x2aa,
		0x2aa,0x2aa,0x2aa,0x2aa,0x2aa,0x2aa,0x2aa,0x2aa,
		0x2aa,0x2aa,0x2aa,0x2aa,0x2aa,0x2aa,0x2aa,0x2aa,
		0x2aa,0x2aa,0x2aa,0x2aa,0x2aa,0x2aa,0x2aa,0x2aa,
		0x2aa,0x2aa,0x2aa,0x2aa,0x2aa,0x2aa,0x2aa,0x2aa,
		0x2aa,0x2aa,0x2aa,0x2aa,0x2aa,0x2aa,0x2aa,0x2aa,
		0x2aa,0x2aa,0x2aa,0x2aa,0x2aa,0x2aa,0x2aa,0x2aa,0xfa
};

int packet_length; // initialized at startup to equal the length of a packet.
int first_packet_to_be_sent; // flag that tells us if a sync word is needed at the start of the packet
int bits_sent_for_current_word; // how many bits have we sent for the current 10b word
int words_sent_for_current_packet; // how many 10b words we have sent for the current packet
int rd_state = 0; // 8b10b Encoder state, initialized once at startup

int init_telemetry_processor(int packet_len) {
	packet_length = packet_len;
	first_packet_to_be_sent = true;
	bits_sent_for_current_word = 0;
	words_sent_for_current_packet = 0;
	current_encoded_packet_num = 0;
	init_rd_state();
	return 0;
}

void init_rd_state() {
	rd_state = 0;
}

/**
 * This is called when a packet of data needs to be encoded ready for transmission.  It must
 * go into the non active encoded packet buffer.  It will then be sent when the current
 * packet is finished.  The caller is responsible for working out which buffer to use
 *
 */
int encode_next_packet(duv_packet_t *packet, int encoded_packet_num) {
	int rc = EXIT_SUCCESS;
	//debug_print("DEBUG: Getting next packet\n");
	encode_duv_telem_packet((unsigned char *)packet, encoded_packet[encoded_packet_num]);
	return rc;
}

/**
 * This is called at the end of each bit.  The length of the bit is determined by the
 * audio_processor and depends on the sample rate and decimation.  At the end of each
 * bit the audio_processor calls this routine to get the next bit.
 *
 * This routine knows the length of words and packets so it can move from
 * bit to bit.  If we have run out of words in the current packet then it swaps the
 * buffers.
 *
 */
int get_next_bit() {
	int current_bit = -1; // the first bit is set to be the sync word

	if (bits_sent_for_current_word == 0 && words_sent_for_current_packet == 0) {
		// This is the start of sending a packets.
		// Tell the telem thread to fill the next one
		telem_thread_fill_next_packet();
	}

	if (bits_sent_for_current_word >= BITS_PER_10b_WORD) { // We are starting a new 10b word
		bits_sent_for_current_word = 0;
		if (first_packet_to_be_sent) {
			first_packet_to_be_sent = false; // we have sent at least one word so this is no longer the start of a transmission
		} else {
			// we sent a word from this packet so increment the counter
			words_sent_for_current_packet++;
		}
		if (words_sent_for_current_packet >= packet_length+1) { // We are ready for a new packet.  We have the sync word in the final word
			words_sent_for_current_packet = 0;

			int next_packet = telem_thread_get_packet_num();
			//debug_print("DEBUG: next packet: %i\n", next_packet);
			if (next_packet == current_encoded_packet_num) {
				error_print("Next packet was not available\n");
				// TODO - we need to reset things here and send the sync word again.
			} else {
				current_encoded_packet_num = next_packet;
			}
			// TODO - call at end as well as start?
			// TODO - is this causing audio clipping somehow.....
			telem_thread_fill_next_packet();
		}

	}
	// so get the value
	/* If we are starting to transmit then send the sync word first */
	uint16_t current_word = 0xfa;
	if (!first_packet_to_be_sent) {
		current_word = encoded_packet[current_encoded_packet_num][words_sent_for_current_packet];
	}
	// we send most significant bit first of the 10 bit word
	int shift_amt = 9 - bits_sent_for_current_word;
	current_bit = ((current_word & 0x3ff) >> shift_amt)  & 0x01;

	bits_sent_for_current_word++;
	return current_bit;
}


/**
 * This takes a telemetry frame and encodes it ready for transmission
 */
void encode_duv_telem_packet(unsigned char *packet, uint16_t *encoded_packet) {

	memset(parities,0,sizeof(parities)); // Do this before every frame

	int j = 0;
	// Encode the data, updating the RS encoder
	for(int i=0; i< DUV_DATA_LENGTH;i++){
		update_rs(parities,packet[i]);
		encoded_packet[j++] = encode_8b10b(&rd_state,packet[i]);
	}

	// get the RS parities
	for(int i=0;i< DUV_PARITIES_LENGTH;i++)
		encoded_packet[j++] = encode_8b10b(&rd_state,parities[i]);
	encoded_packet[j] = encode_8b10b(&rd_state,-1); // Insert end-of-frame flag
}

/*
 * Call this to free any memory allocated by the telemetry processor
 */
void cleanup_telem_processor() {

}

/******************************************************************************
 *
 * TEST FUNCTIONS
 *
 ******************************************************************************/
/* Test Type 1 packet with id 1.  This will look like Fox-1A in FoxTelem
 * Note that this has a different header.  It has the old Fox-1A header and
 * does not use the extended FOX-ID.
 */
unsigned char test_packet[] = {
		0x51,0x01,0x40,0xd8,0x00,0x10,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x01,0x10,0x00,0x01,0x20,0x00,
		0x01,0x10,0x00,0x47,0x8f,0xf4,0x47,0x7f,
		0xf4,0x48,0x7f,0xf4,0x10,0x08,0x00,0x65,
		0x47,0x76,0xff,0xe7,0x33,0xce,0xe2,0x81,
		0x29,0x78,0x80,0xf2,0x8e,0x01,0x04,0x01,
		0x01,0x01,0x17,0x38,0xac,0x00,0x00,0x20};

unsigned char * set_test_packet() {
	encode_duv_telem_packet((unsigned char *)test_packet, encoded_packet[0]);
	return (unsigned char *)test_packet;
}

/**
 * Generate a test packet with RS checkbytes but without 8b10b encoding.  This is
 * useful for testing if the core RS encoder is working
 */
int test_telem_encoder(unsigned char *packet, uint16_t *encoded_packet) {
	int fail = 0;
	memset(parities,0,sizeof(parities)); // Do this before every frame

	int j = 0;
	// Encode the data, updating the RS encoder
	for(int i=0; i< DUV_DATA_LENGTH;i++){
		update_rs(parities,packet[i]);
		encoded_packet[j++] = packet[i];
	}

	// get the RS parities
	for(int i=0;i< DUV_PARITIES_LENGTH;i++)
		encoded_packet[j++] = parities[i];
	return fail;
}

/**
 * Generate a test packet by specifying the data
 */
int test_encode_packet() {
	int fail = EXIT_SUCCESS;
	printf("TESTING encode_packet .. ");
	verbose_print("\n");

	unsigned char expected_result[] = {
			0x51,0x01,0x40,0xd8
	};

	verbose_print("Packet header length: %i\n",(int)sizeof(duv_header_t))
	verbose_print("Packet payload length: %i\n",(int)sizeof(rttelemetry_t))
	verbose_print("Packet structure length: %i\n",(int)sizeof(duv_packet_t))
	assert(sizeof(duv_header_t) == 7);
	assert(sizeof(rttelemetry_t) == 57);
	assert(sizeof(duv_packet_t) == DUV_DATA_LENGTH);
	duv_packet_t *packet = (duv_packet_t*)calloc(DUV_DATA_LENGTH,sizeof(char)); // allocate 64 bytes for the packet data

	// This is the same header as the test packet
	packet->header.id = 1;
	packet->header.epoch = 42;
	packet->header.uptime = 6920;
	packet->header.type = 1;

	unsigned char *ptr;
	ptr = (unsigned char*)packet;

	for (int i=0; i<4;i++) {
		if (ptr[i] != expected_result[i]) {
			fail = EXIT_FAILURE;
			verbose_print(" **err->");
		}
		verbose_print(" byte: %d %x\n",i,ptr[i]);
	}
	free(packet);
	if (fail == EXIT_SUCCESS) {
		printf(" Pass\n");
	} else {
		printf(" Fail\n");
	}
	return fail;
}

unsigned char test_rs_parities_check[] = {0x19,0xa0,0x2c,0x20,0x59,0xf6,0x7c,0x12,0x84,0x27,0x77,0x98,0xb5,0xf3,0x89,0xf1,
		0xa4,0x84,0xba,0x50,0x3a,0x0f,0x16,0x01,0x62,0x1c,0xcd,0x9a,0x11,0x1a,0xf2,0xa7};

int test_rs_encoder() {
	int fail = 0;
	printf("TESTING Rs Encoder .. ");
	init_rd_state();
	// Call the RS encoder without 8b10b encoding
	test_telem_encoder(test_packet, encoded_packet[0]);

	// Now check the parity bytes
	// First should be 0x19 -> 25
	// Last is 0xa7 -> 167
	//printf("First parity: %i \n",test_encoded_packet[DUV_DATA_LENGTH]);
	//printf("Last Parity: %i \n",test_encoded_packet[DUV_DATA_LENGTH+DUV_PARITIES_LENGTH-1]);
	for (int i=0; i < DUV_PARITIES_LENGTH; i++) {
		if (encoded_packet[0][DUV_DATA_LENGTH+i] != test_rs_parities_check[i]) {
			verbose_print(" failed with parity %d\n", i);
			fail = 1;
		}
	}
	if (fail == 0) {
		printf(" Pass\n");
	} else {
		printf(" Fail\n");
	}
	return fail;
}

int test_sync_word() {
	// test that the sync word is the last word in the packet
	int fail = 0;
	printf("TESTING Sync word %x %x .. ",0xfa, (~0xfa) & 0x3ff);
	init_rd_state();
	encode_duv_telem_packet(test_packet, encoded_packet[0]);

	uint16_t word = encoded_packet[0][DUV_DATA_LENGTH+DUV_PARITIES_LENGTH];
	verbose_print(" Sync Word is %x \n",word );

	if (word != 0x0fa && word != 0x305) // 0x305 is ~0xfa
		fail = 1;

	if (fail == 0) {
		printf(" Pass\n");
	} else {
		printf(" Fail\n");
	}

	return fail;
}

int test_get_next_bit() {
	printf("TESTING get_next_bit .. ");
	verbose_print("\n");

	/* reset the state of the modulator */
	init_telemetry_processor(DUV_PACKET_LENGTH);
	// but then set the test packet rather than the real telemetry that was captured
	encode_duv_telem_packet(test_packet, encoded_packet[0]);

	// Generate the first 40 bits of the test packet - the header
	// First four bytes are: 0x51,0x01, 0x40, 0xd8
	// These should encode as 10b words:
	//   0x235    10 0011 0101    565
	//   0x1d4    01 1101 0100
	//   0x675   110 0111 0101 - note this is 11 bit because we use bit 11 as the rd state change indicator.
	//   0x0c6    00 1100 0110 - this is encoded with rd state 1
	// The RD state changes when a word does not have the same number of 1s and 0s (+/-1)

	int fail = 0;
	int expected_result[] = {
			0,0,1,1,1,1,1,0,1,0,  // the sync word 0xfa
			1,0,0,0,1,1,0,1,0,1,
			0,1,1,1,0,1,0,1,0,0,
			1,0,0,1,1,1,0,1,0,1,
			0,0,1,1,0,0,0,1,1,0};


	verbose_print("%x %x %x %x\n",test_packet[0], test_packet[1], test_packet[2],test_packet[3]);
	verbose_print("%x %x %x %x\n",encoded_packet[0][0], encoded_packet[0][1], encoded_packet[0][2],encoded_packet[0][3]);

	for (int i=0; i < 50; i++) {
		int b = get_next_bit();
		verbose_print(" %i",b);

		if (b != expected_result[i]) {
			fail = 1;
			verbose_print("<-err ");
		}
		if ((i+1) % 10 == 0)
			verbose_print("\n");
	}

	// Now check the first and last parity bytes
	// First should be 0x19 -> encoded as 0x264
	// Last is 0xa7 -> encoded as 0x7a
	//	printf("First parity: %x \n",test_encoded_packet[DUV_DATA_LENGTH] );
	//	printf("Last Parity: %x \n",test_encoded_packet[DUV_DATA_LENGTH+DUV_PARITIES_LENGTH-1] );

	if (encoded_packet[0][DUV_DATA_LENGTH] != 0x264) {
		fail = 1;
	}
	if (encoded_packet[0][DUV_DATA_LENGTH+DUV_PARITIES_LENGTH-1] != 0x7a) {
		fail = 1;
	}

	cleanup_telem_processor();
	if (fail == 0) {
		printf(" Pass\n");
	} else {
		printf(" Fail\n");
	}
	return fail;
}
