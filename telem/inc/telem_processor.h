/*
 * telem_processor.h
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
 */
#ifndef TELEM_PROCESSOR_H_
#define TELEM_PROCESSOR_H_

#include <stdint.h>
#include <duv_telem_layout.h>

/* Store the values for the telemetry modulator */
#define DUV_BPS 200
#define DUV_PACKET_LENGTH 96  /* 4 header bytes, 60 payload bytes, 32 check bytes */
#define DUV_DATA_LENGTH 64
#define DUV_PARITIES_LENGTH 32
#define BITS_PER_10b_WORD 10

/* Getters for variables */
double get_loop_time_microsec();
double get_max_loop_time_microsec();
double get_min_loop_time_microsec();
/*
 * Set the running disparity to zero.  This is called just once at startup,
 * but we may call it more than once if we are running test routines.
 */
void init_rd_state();

/*
 * Ask the telem processor to set the a packet ready for transmission.  it
 * is encoded into the specified packet buffer.  The caller needs to know
 * which buffer is available
 *
 */
int encode_next_packet(duv_packet_t *packet, int encoded_packet_num);

/*
 * Get the next bit for the encoded packet.
 */
int get_next_bit();

/*
 * Initialize the telemetry processor ready to send telemetry.  This should be called
 * whenever the telemetry is stopped and restarted.  Cleanup should be called when
 * it is no longer needed
 */
int init_telemetry_processor();
void cleanup_telem_processor();

/*
 * Self test functions
 */
unsigned char * set_test_packet();
int test_telem_encoder(unsigned char *packet, uint16_t *encoded_packet);
int test_encode_packet();
unsigned char reverse_8b10b_lookup(uint16_t word);
int test_rs_encoder();
int test_sync_word();
int test_get_next_bit();

#endif /* TELEM_PROCESSOR_H_ */
