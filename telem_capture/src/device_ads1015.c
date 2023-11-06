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
 * The ADS1015 is a precision Analog to Digital Converter with 4 channels.
 * It allows the gas sensors to be read over the I2C bus
 * Input impedance is circa 6Mohm
 * Vcc and Heater Voltage of GAS SENSORSs are 5V.  The ouput maximum
 * is also 5V. 
 * Full Scale Range FSR for the ADC needs to be set to maximum which is 
 * +/- 6.144V, but the maximum reading will be 5V because that is the
 * supply voltage for the ADC.
 *
 * The ADC operates in single shot mode.  Bit 8 of the Config register 
 * is set to 1.  This is the default state.
 *
 */

#ifdef RASPBERRY_PI

#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include <unistd.h>
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
    uint8_t rbuf[2];
    int rc = 0;

    /* Reset the chip and perhaps all others */
    reg[0] = 0x06; 
//    rc = i2c_write(0x00, reg, 1);
    if (rc != BCM2835_I2C_REASON_OK) {
        debug_print("Error: %d Could not reset the AtoD\n", rc);
        return rc;
    }

    buf[0] = ADS1015_REG_CONFIG;
    buf[1] = 0x51; // Do not start CONVERSION, AIN1 input, Gain +- 6.144V, single shot mode
    buf[2] = 0x83; // Ignore Conversion and Comparator settings. Defaullt values
    rc = i2c_write(ADS1015_ADDRESS, buf, 3);
    if (rc != BCM2835_I2C_REASON_OK) {
        debug_print("Error: %d Could not write config to the ADC\n", rc);
        return rc;
    }

    reg[0] = ADS1015_REG_CONFIG; 
    rc = i2c_write(ADS1015_ADDRESS, reg, 1);
    rc = i2c_read(ADS1015_ADDRESS, rbuf, 2);
    debug_print("ADS1015 CONFIG %02x %02x \n",rbuf[0], rbuf[1] );

    return rc;
}

/* Read a channel of the ADC.  Use one shot method and wait till reading is done */
int ads1015_read(uint8_t channel_and_gain, int16_t *result) {
    uint8_t reg[1];
    uint8_t buf[3];
    uint8_t rbuf[2];
    int rc = 0;

    buf[0] = ADS1015_REG_CONFIG;
    buf[1] = channel_and_gain; // 0xD1; // CONVERSION, AIN1 input, Gain +- 6.144V, single shot mode
    buf[2] = 0x83; // Ignore Conversion and Comparator settings. Defaullt values
    rc = i2c_write(ADS1015_ADDRESS, buf, 3);
    if (rc != BCM2835_I2C_REASON_OK) {
        debug_print("Error: %d Could not write config to the ADC\n", rc);
        return rc;
    }

    /* now read the config and wait till the conversion is done */
    rc = i2c_read(ADS1015_ADDRESS, buf, 2);
        if (rc != BCM2835_I2C_REASON_OK) {
            debug_print("Error: %d Could not read one shot status from the ADC\n", rc);
            return rc;
        }
    int loop_safety_limit = 1000000;
    while ((buf[0] & 0x80) == 0x00) { // if bit 7 of high byte is zero then performing a conversion
        if (--loop_safety_limit <= 0) {
            return BCM2835_I2C_REASON_ERROR_TIMEOUT;
        }
        sched_yield();
        rc = i2c_read(ADS1015_ADDRESS, buf, 2);
        if (rc != BCM2835_I2C_REASON_OK) {
            debug_print("Error: %d Could not re-read one shot status from the ADC\n", rc);
            return rc;
        }
    }

    /* Read 2 bytes from conversion register */
    reg[0] = ADS1015_REG_CONVERSION; 
    rc = i2c_write(ADS1015_ADDRESS, reg, 1);
    if (rc != BCM2835_I2C_REASON_OK) {
        debug_print("Error: %d Could not write conversion address to the ADC\n", rc);
        return rc;
    }

    rc = i2c_read(ADS1015_ADDRESS, rbuf, 2);
    if (rc != BCM2835_I2C_REASON_OK) {
        debug_print("Error: %d Could not read config from the ADC\n", rc);
        return rc;
    }
    //debug_print("ADS1015 ADC %02x %02x \n",rbuf[0], rbuf[1] );
    *result = (rbuf[0] << 8) + rbuf[1];

    return rc;
}


#endif /* RASPBERRY_PI */
