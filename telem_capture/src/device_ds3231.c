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
 *
 *
 * The DS3231 is a low-cost, extremely accurate I2C real-time clock (RTC)
 * with an integrated temperaturecompensated crystal oscillator (TCXO) and
 * crystal. The device incorporates a battery input, and maintains accurate
 * timekeeping when main power to the device is interrupted.
 *
 *
 *
 */

#ifdef RASPBERRY_PI

#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include "debug.h"
#include "device_lps25hb.h"
#include "gpio_interface.h"

/**
 * Initialize the chip
 *
 */
int init_ds3231() {

	return 0;
}

int reset_ds3231() {
	int rc = EXIT_SUCCESS;

	uint8_t val = LOW;
	debug_print("Writing GPIO PIn 18 to: %i\n", val);
	bcm2835_gpio_write(GPIO_PTT, val);
	bcm2835_delay(500);
	val = HIGH;
	debug_print("Writing GPIO PIn 18 to: %i\n", val);
	bcm2835_gpio_write(GPIO_PTT, val);

	return rc;

}

#endif /* RASPBERRY_PI */
