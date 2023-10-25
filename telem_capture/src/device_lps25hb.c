/*
 * device_LPS25HB.c
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
 *
 *
 *
 * The LPS25HB is a high resolution, digital output pressure sensor packaged
 * in an HLGA fullmold package. The complete device includes a sensing element
 * based on a piezoresistive Wheatstone bridge approach, and an IC interface
 * which communicates a digital signal from the sensing element to the application.
 *
 * An ST proprietary process is used to obtain a silicon membrane for MEMS pressure
 * sensors. When pressure is applied, the membrane deflection induces an imbalance
 * in the Wheatstone bridge piezoresistances whose output signal is converted by
 * the IC interface.
 *
 * The LPS25HB features a Data-Ready signal which indicates when a new set of measured
 * pressure and temperature data are available, thus simplifying data synchronization
 * in the digital system that uses the device.
 *
 * The IC interface is factory calibrated at three temperatures and two pressures
 * for sensitivity and accuracy.
 * The trimming values are stored inside the device in a non-volatile structure. When
 * the device is turned on, the trimming parameters are downloaded into the
 * registers to be employed during normal operation which allows the device to be
 * used without requiring any further calibration.
 *
 * The pressure data are stored in 3 registers: PRESS_OUT_H (2Ah), PRESS_OUT_L (29h)
 * and PRESS_OUT_XL (28h). The value is expressed as 2's complement. To obtain the
 * pressure in hPa, take the two's complement of the complete word and then
 * divide by 4096 hPa
 */

#include "device_lps25hb.h"
#include "gpio_interface.h"

/**
 * Initialize the chip
 * Set FIFO to Bypass - default
 *
 */
int init_lp25hb() {
	uint8_t buf[2];
	buf[0] = 0x0F; // WHO AM I
	int rc = i2c_write(LPS25HB_ADDRESS, buf, 2);
	rc = i2c_read(LPS25HB_ADDRESS, buf, 2);

	return rc;
}
