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
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include "debug.h"
#include "telem_processor.h"
#include "TelemEncoding.h"

int rd_state = 0; // 8b10b Encoder state, initialized once at startup

unsigned char parities[DUV_PARITIES_LENGTH];

void init_rd_state() {
	rd_state = 0;
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
	int fail = 1;
	verbose_print("Packet header length: %i\n",(int)sizeof(duv_header_t))
	verbose_print("Packet payload length: %i\n",(int)sizeof(duv_payload_t))
	verbose_print("Packet structure length: %i\n",(int)sizeof(duv_packet_t))
	assert(sizeof(duv_header_t) == 8);
	assert(sizeof(duv_payload_t) == 56);
	assert(sizeof(duv_packet_t) == DUV_DATA_LENGTH);
	duv_packet_t *packet = (duv_packet_t*)calloc(sizeof(duv_packet_t),1); // allocate 64 bytes for the packet data
	// This is the same header as the test packet
	packet->header.id = 1;
	packet->header.epoch = 42;
	packet->header.uptime = 6920;
	packet->header.type = 1;

	unsigned char *ptr;
	ptr = (unsigned char*)packet;

	for (int i=0; i<4;i++)
		printf("Byte: %d %x\n",i,ptr[i]);

	return fail;
}

