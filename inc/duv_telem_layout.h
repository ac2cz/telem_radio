/*
 * duv_telem_layout.h
 *
 *  Created on: Feb 24, 2022
 *      Author: g0kla
 */

#ifndef DUV_TELEM_LAYOUT_H_
#define DUV_TELEM_LAYOUT_H_

/* Header */
typedef struct __attribute__((__packed__)) {
	unsigned int id :		3;
	unsigned int epoch :	16;
	unsigned int uptime :	25;
	unsigned int type :		4;
} duv_header_t;

typedef struct __attribute__((__packed__)) {
	unsigned int pi_temperature :	8;
	unsigned int xruns :	16;
	unsigned int dummy :	8;
	unsigned int data0 :	8;
	unsigned int data1 :	8;
	unsigned int data2 :	8;
	unsigned int data3 :	8;
	unsigned int pad1 :		32;
	unsigned int pad2 :		32;
	unsigned int pad3 :		32;
	unsigned int pad4 :		32;
	unsigned int pad5 :		32;
	unsigned int pad6 :		32;
	unsigned int pad7 :		32;
	unsigned int pad8 :		32;
	unsigned int pad9 :		32;
	unsigned int pad10 :	32;
	unsigned int pad11 :	32;
	unsigned int pad12 :	32;
	unsigned int pad13 :	16;
} duv_payload_t;

typedef struct __attribute__((__packed__)) {
	duv_header_t header;
	duv_payload_t payload;
} duv_packet_t;

#endif /* DUV_TELEM_LAYOUT_H_ */
