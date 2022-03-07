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
#include <stdint.h>
#include <string.h>

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

