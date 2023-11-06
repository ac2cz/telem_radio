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
        debug_print("Could not initialize lib\n");
        return 1;
    }

    if (!bcm2835_i2c_begin()) {
        debug_print("Could not initialize i2c, likely not running with sufficient privs\n");
        return 1;
    } else {
        //bcm2835_i2c_set_baudrate(100000); // 100kHz
        bcm2835_i2c_set_baudrate(200000); // 200kHz
        //bcm2835_i2c_setClockDivider(BCM2835_I2C_CLOCK_DIVIDER_150); // 1.666MHz 
        //bcm2835_i2c_setClockDivider(BCM2835_I2C_CLOCK_DIVIDER_626); // 400kHz
        //bcm2835_i2c_setClockDivider(BCM2835_I2C_CLOCK_DIVIDER_2500); // 100kHz
    }
/*
    if (!bcm2835_spi_begin()) {
        debug_print("Could not initialize spi, likely not running as root\n");
        return 1;
    }
*/

    /* Set Pin 7 on connector J8 as OUTPUT
      * This is GPIO 4 and we could have put 4 instead
      * Use the enumerations that start with RPI_BPLUS for all
      * modern versions of the PI post B+
      * type pinout at the console to list the pins
      */
     bcm2835_gpio_fsel(RPI_BPLUS_GPIO_J8_07, BCM2835_GPIO_FSEL_OUTP);

     /*
      * Set pin 18 on connector J8 as OUTPUT
      * This is the REAL TIME CLOCK Reset line.  Has a pull up resistor defined.
      */
     bcm2835_gpio_fsel(RPI_BPLUS_GPIO_J8_18, BCM2835_GPIO_FSEL_OUTP);
     bcm2835_gpio_set_pud(RPI_BPLUS_GPIO_J8_18, BCM2835_GPIO_PUD_UP);

#endif /* RASPBERRY_PI */

    return rc;
}

int gpio_exit() {

#ifdef RASPBERRY_PI

    bcm2835_i2c_end();
/*
    if (!bcm2835_spi_end()) {
        debug_print("Could not end spi, likely not running as root\n");
    }
*/

	bcm2835_close();
#endif
	return EXIT_SUCCESS;
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

#ifdef RASPBERRY_PI
uint8_t i2c_write(uint8_t slave_address, uint8_t *buf, uint32_t len) {
	bcm2835_i2c_setSlaveAddress(slave_address);
	uint8_t rc = bcm2835_i2c_write((char *)buf, len);
	return rc;
}

uint8_t i2c_read(uint8_t slave_address, uint8_t *buf, uint32_t len) {
	bcm2835_i2c_setSlaveAddress(slave_address);
	uint8_t rc = bcm2835_i2c_read((char *)buf, len);
	return rc;
}

uint8_t i2c_read_register_rs(uint8_t slave_address, uint8_t *reg, uint8_t *buf, uint32_t len) {
	bcm2835_i2c_setSlaveAddress(slave_address);
	uint8_t rc = bcm2835_i2c_read_register_rs((char *)reg, (char *)buf, len);
	return rc;
}

#endif /* RASPBERRY_PI */
