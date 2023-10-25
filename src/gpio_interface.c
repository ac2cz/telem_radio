/*
 * gpio_interface.c
 *
 *  Created on: Apr 13, 2022
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
 * Here are the GPIO routines for telem_radio.  Call the init() function before
 * any others
 *
 */

/* System */
#include <stdlib.h>
#include <stdint.h>

/* Program */
#include "gpio_interface.h"
#include "config.h"
#include "debug.h"



/**
 * Initialize the Raspberry PI GPIO.  Do nothing on Linux
 */
int gpio_init() {
	int rc = EXIT_SUCCESS;

    #ifdef RASPBERRY_PI

	if (!bcm2835_init()) {
        printf("Could not initialize lib\n");
        return 1;
    }

    if (!bcm2835_i2c_begin()) {
        printf("Could not initialize i2c, likely not running with sufficient privs\n");
    } else {
        printf("Initialized i2c\n");
        bcm2835_i2c_setClockDivider(BCM2835_I2C_CLOCK_DIVIDER_148);
    }
/*
    if (!bcm2835_spi_begin()) {
        printf("Could not initialize spi, likely not running as root\n");
    }
*/

    /* Set Pin 7 on connector J8 as OUTPUT
      * This is GPIO 4 and we could have put 4 instead
      * Use the enumerations that start with RPI_BPLUS for all
      * modern versions of the PI post B+
      * type pinout at the console to list the pins
      */
     bcm2835_gpio_fsel(RPI_BPLUS_GPIO_J8_07, BCM2835_GPIO_FSEL_OUTP);

    #endif /* RASPBERRY_PI */

    return rc;
}

int gpio_set_ptt(int state) {
	int rc = EXIT_SUCCESS;


	#ifdef RASPBERRY_PI
	uint8_t val = LOW;
		if (state) val = HIGH;
	debug_print("Writing GPIO PTT to: %i\n", val);
	bcm2835_gpio_write(GPIO_PTT, val);
	#endif /* RASPBERRY_PI */

	return rc;
}

