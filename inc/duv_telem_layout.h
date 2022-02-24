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
	unsigned int id :	6;
	unsigned int epoch :	16;
	unsigned int uptime :	25;
	unsigned int type :		4;
} duv_header_t;

#endif /* DUV_TELEM_LAYOUT_H_ */
