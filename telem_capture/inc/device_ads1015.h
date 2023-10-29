/*
 * device_ads1015.h
 *
 *  Created on: Oct 29, 2023
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

#ifndef DEVICE_ADS1015_H_
#define DEVICE_ADS1015_H_

#ifdef RASPBERRY_PI

#include <stdint.h>

/* Chip constants */
#define ADS1015_ADDRESS 0x48

/* Registers */
#define ADS1015_REG_CONVERSION 0x00
#define ADS1015_REG_CONFIG 0x01
#define ADS1015_REG_LOW_THRESH 0x02
#define ADS1015_REG_HIGH_THRESH 0x03

int init_ads1015();

#endif /* RASPBERRY_PI */

#endif /* DEVICE_ADS1015_H_ */
