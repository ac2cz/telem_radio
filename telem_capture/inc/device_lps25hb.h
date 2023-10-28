/*
 * device_LPS25HB.h
 *
 *  Created on: Oct 25, 2023
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
 */

#ifndef DEVICE_LPS25HB_H_
#define DEVICE_LPS25HB_H_

#ifdef RASPBERRY_PI

#include <stdint.h>

/* Chip constants */
#define LPS25HB_ADDRESS 0x5C
#define LPS25HB_CHIP_ID 0xBD

/* Registers */
#define LPS25HB_REG_REF_P_XL 0x08
#define LPS25HB_REG_REF_P_L 0x09
#define LPS25HB_REG_REF_P_H 0x0A
#define LPS25HB_REG_WHO_AM_I 0x0F
#define LPS25HB_REG_CTRL_REG1 0x20
#define LPS25HB_REG_CTRL_REG2 0x21
#define LPS25HB_REG_CTRL_REG3 0x22
#define LPS25HB_REG_CTRL_REG4 0x23
#define LPS25HB_REG_STATUS_REG 0x27
#define LPS25HB_REG_PRESS_OUT_XL 0x28
#define LPS25HB_REG_PRESS_OUT_L 0x29
#define LPS25HB_REG_PRESS_OUT_H 0x2A
#define LPS25HB_REG_TEMP_OUT_L 0x2B 
#define LPS25HB_REG_TEMP_OUT_H 0x2C

int init_lps25hb();
int lps25hb_one_shot_read();
int get_lps25hb_pressure(uint32_t *raw_pressure);
int get_lps25hb_temperature(uint16_t *raw_temperature);

#endif /* RASPBERRY_PI */

#endif /* DEVICE_LPS25HB_H_ */
