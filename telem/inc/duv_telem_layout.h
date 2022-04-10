/*
 * duv_telem_layout.h
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

#ifndef DUV_TELEM_LAYOUT_H_
#define DUV_TELEM_LAYOUT_H_

/* Header */
typedef struct __attribute__((__packed__)) {
	unsigned int id :			3;
	unsigned int epoch :		16;
	unsigned int uptime :		25;
	unsigned int type :			4;
	unsigned int extended_id :	5;
	unsigned int safe_mode :	1;
	unsigned int health_mode :	1;
	unsigned int science_mode :	1;
} duv_header_t;

typedef struct __attribute__((__packed__)) {
    unsigned int pi_temperature : 16;
    unsigned int xruns : 16;
    unsigned int loop_time : 8;
    unsigned int cpu_speed : 8;
    unsigned int data2 : 8;
    unsigned int data3 : 8;
    unsigned int pad1 : 32;
    unsigned int pad2 : 32;
    unsigned int pad3 : 32;
    unsigned int pad4 : 32;
    unsigned int pad5 : 32;
    unsigned int pad6 : 32;
    unsigned int pad7 : 32;
    unsigned int pad8 : 32;
    unsigned int pad9 : 32;
    unsigned int pad10 : 32;
    unsigned int pad11 : 32;
    unsigned int pad12 : 32;
    unsigned int pad13 : 8;
} rttelemetry_t;

typedef struct __attribute__((__packed__)) {
	duv_header_t header;
	rttelemetry_t payload;
} duv_packet_t;

#endif /* DUV_TELEM_LAYOUT_H_ */
