/*
 * device_ds3231.h
 *
 *  Created on: Oct 30, 2023
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

#ifndef DEVICE_DS3231_H_
#define DEVICE_DS3231_H_


#ifdef RASPBERRY_PI

#include <stdint.h>

/* Chip constants */
#define DS3231_ADDRESS 0x68

/* Registers */
#define DS3231_REG_SECONDS 0x00
#define DS3231_REG_CONTROL 0x0E
#define DS3231_REG_STATUS 0x0F

int init_ds3231();
int reset_ds3231();
int ds3231_get_time();
int ds3231_set_time(uint16_t year, uint8_t month, uint8_t day, uint8_t dow, uint8_t hour, uint8_t min, uint8_t sec);

#endif /* RASPBERRY_PI */

#endif /* DEVICE_DS3231_H_ */
