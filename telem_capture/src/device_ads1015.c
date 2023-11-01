/*
 * device_ads1015.c
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
 *
 *
 *
 */

#ifdef RASPBERRY_PI

#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include "debug.h"
#include "device_ads1015.h"
#include "gpio_interface.h"

/**
 * Initialize the chip
 *
 */
int init_ads1015() {
    uint8_t reg[1];
    uint8_t buf[3];
    uint8_t rbuf[3];
    int rc = 0;

    /* Reset the chip and perhaps all others */
    reg[0] = 0x06; 
//    rc = i2c_write(0x00, reg, 1);
    if (rc != BCM2835_I2C_REASON_OK) {
        debug_print("Error: %d Could not reset the AtoD\n", rc);
        return rc;
    }

    reg[0] = ADS1015_REG_CONFIG; 
    rc = i2c_write(ADS1015_ADDRESS, reg, 1);
    rc = i2c_read(ADS1015_ADDRESS, rbuf, 2);
    debug_print("ADS1015 CONFIG %02x %02x \n",rbuf[0], rbuf[1] );

    buf[0] = ADS1015_REG_CONFIG;
    buf[1] = 0xD5; // START CONVERSION and AIN1 input and Gain +- 2.048V - default, single shot mode
    buf[2] = 0x83; // Ignore Conversion and Coparator settings.
    rc = i2c_write(ADS1015_ADDRESS, buf, 3);
    if (rc != BCM2835_I2C_REASON_OK) {
        debug_print("Error: %d Could not write config to the ADC\n", rc);
        return rc;
    }

/*
    buf[0] = 0x80;
    int loop_safety_limit = 1000000;
    while ((buf[0] & 0x01) == 0x01) {
        if (--loop_safety_limit <= 0) {
            return BCM2835_I2C_REASON_ERROR_TIMEOUT;
        }
        sched_yield();
        reg[0] = LPS25HB_REG_CTRL_REG2; // Control Register 2
        rc = i2c_read_register_rs(LPS25HB_ADDRESS, reg, buf, 1);
        if (rc != BCM2835_I2C_REASON_OK) {
            debug_print("Error: %d Could not read one shot status from the pressure sensor\n", rc);
            return rc;
        }
    }
*/

    /* Read 2 bytes from register */

    rbuf[0] = 0xAA;
    rbuf[1] = 0xAA;
    rbuf[2] = 0xAA;
    //reg[0] = ADS1015_REG_CONFIG; 
    reg[0] = ADS1015_REG_CONVERSION; 
    rc = i2c_write(ADS1015_ADDRESS, reg, 1);
    if (rc != BCM2835_I2C_REASON_OK) {
        debug_print("Error: %d Could not write read address to the ADC\n", rc);
        return rc;
    }

    rc = i2c_read(ADS1015_ADDRESS, rbuf, 2);
    if (rc != BCM2835_I2C_REASON_OK) {
        debug_print("Error: %d Could not read config from the ADC\n", rc);
        return rc;
    }
    debug_print("ADS1015 ADC %02x %02x \n",rbuf[0], rbuf[1] );

    return rc;
}


#endif /* RASPBERRY_PI */
