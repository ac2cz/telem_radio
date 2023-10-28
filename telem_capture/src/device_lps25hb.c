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

#ifdef RASPBERRY_PI

#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include "debug.h"
#include "device_lps25hb.h"
#include "gpio_interface.h"

/**
 * Initialize the chip
 * Set FIFO to Bypass - default
 *
 */
int init_lps25hb() {
    uint8_t reg[2];
    uint8_t reg2[1];
    uint8_t buf[4];
    reg[0] = LPS25HB_REG_WHO_AM_I; // Chip id
    int rc = i2c_read_register_rs(LPS25HB_ADDRESS, reg, buf, 1);
    if (buf[0] != LPS25HB_CHIP_ID) {
        return 1;
    }	

    /* Reset the Chip */
    reg[0] = LPS25HB_REG_CTRL_REG2; // Control Register 2
    reg[1] = 0x04; // Software Reset
    rc = i2c_write(LPS25HB_ADDRESS, reg, 2);

    /* Set the Boot bit */
    reg[0] = LPS25HB_REG_CTRL_REG2; // Control Register 2
    reg[1] = 0x80; // Boot
    rc = i2c_write(LPS25HB_ADDRESS, reg, 2);
    buf[0] = 0x80;
    int loop_safety_limit = 10000;
    while ((buf[0] & 0x80) != 0) {
	if (--loop_safety_limit <= 0) { 
            return 1;
        }
        debug_print("Waiting for boot ..");
        sched_yield();
        reg[0] = LPS25HB_REG_CTRL_REG2; // Control Register 2
        rc = i2c_read_register_rs(LPS25HB_ADDRESS, reg, buf, 1);
    }
    debug_print("booted\n");

    /* Enable the Chip */
    reg[0] = LPS25HB_REG_CTRL_REG1; // Control Register 1
    reg[1] = 0x80; // Enable chip
    rc = i2c_write(LPS25HB_ADDRESS, reg, 2);

    reg2[0] = LPS25HB_REG_CTRL_REG1; // Control Register 1
    rc = i2c_read_register_rs(LPS25HB_ADDRESS, reg2, buf, 1);
    if (buf[0] != 0x80) {
        debug_print("ERROR: Could not start the pressure sensor\n");
        return 1;
    }

    /* Debug - print the control registers out
    debug_print("CTRL_REG1: %0x %0x\n", LPS25HB_REG_CTRL_REG1, buf[0]);
    reg2[0] = LPS25HB_REG_CTRL_REG2; // Control Register 1
    rc = i2c_read_register_rs(LPS25HB_ADDRESS, reg2, buf, 1);
    debug_print("CTRL_REG2: %0x %0x\n", LPS25HB_REG_CTRL_REG2, buf[0]);
    reg2[0] = LPS25HB_REG_CTRL_REG3; // Control Register 1
    rc = i2c_read_register_rs(LPS25HB_ADDRESS, reg2, buf, 1);
    debug_print("CTRL_REG3: %0x %0x\n", LPS25HB_REG_CTRL_REG3, buf[0]);
    reg2[0] = LPS25HB_REG_CTRL_REG4; // Control Register 1
    rc = i2c_read_register_rs(LPS25HB_ADDRESS, reg2, buf, 1);
    debug_print("CTRL_REG4: %0x %0x\n", LPS25HB_REG_CTRL_REG4, buf[0]);
    */
    return rc;
}

/* This performs a one shot read of the pressure sensor.  It
   blocks and waits for the read to finish.  Call the funtions
   below to retrieve the pressure and temperature */
int lps25hb_one_shot_read() {
    uint8_t reg[1];
    uint8_t reg2[2];
    uint8_t buf[3];
    int rc = 0;

    /* Set one shot read */
    reg2[0] = LPS25HB_REG_CTRL_REG2; // Control Register 2
    reg2[1] = 0x01; // One shot
    rc = i2c_write(LPS25HB_ADDRESS, reg2, 2);

    buf[0] = 0x01;
    int loop_safety_limit = 10000;
    while ((buf[0] & 0x01) == 0x01) { 
        if (--loop_safety_limit <= 0) {
            return BCM2835_I2C_REASON_ERROR_TIMEOUT;
        }
        sched_yield();
        reg[0] = LPS25HB_REG_CTRL_REG2; // Control Register 2
        rc = i2c_read_register_rs(LPS25HB_ADDRESS, reg, buf, 1);
    }

    return rc;
}

int lps25hb_read_status(uint8_t * status) {
   // reg[0] = LPS25HB_REG_STATUS_REG; // Status of the reads for P and T
   // rc = i2c_read_register_rs(LPS25HB_ADDRESS, reg, buf, 1);
   //  debug_print("STATUS: %02x \n", buf[0]);
    return 1;
}

/* Read the pressure registers and return as a 24 bit number.
   Call the one shot read before reading the registers */
int get_lps25hb_pressure(uint32_t *raw_pressure) {
    uint8_t reg[1];
    uint8_t buf[3];
    int rc = 0;

    reg[0] = LPS25HB_REG_PRESS_OUT_XL; // Pressure LSB
    rc = i2c_read_register_rs(LPS25HB_ADDRESS, reg, buf, 1);
    reg[0] = LPS25HB_REG_PRESS_OUT_L; // Pressure middle byte
    rc = i2c_read_register_rs(LPS25HB_ADDRESS, reg, &buf[1], 1);
    reg[0] = LPS25HB_REG_PRESS_OUT_H; // Pressure MSB
    rc = i2c_read_register_rs(LPS25HB_ADDRESS, reg, &buf[2], 1);
    /* We dont have to worry about 2s complement because the pressure will not be negative in our case */
    *raw_pressure = (buf[2] << 16) + (buf[1] << 8) + buf[0];

    return rc;
}

int get_lps25hb_temperature(uint16_t *raw_temperature) {
    uint8_t reg[1];
    uint8_t buf[3];
    int rc = 0;

    reg[0] = LPS25HB_REG_TEMP_OUT_L; // temp LSB
    rc = i2c_read_register_rs(LPS25HB_ADDRESS, reg, buf, 1);
    reg[0] = LPS25HB_REG_TEMP_OUT_H; // temp MSB
    rc = i2c_read_register_rs(LPS25HB_ADDRESS, reg, &buf[1], 1);
    *raw_temperature = (buf[1] << 8) + buf[0];

    return rc;
}

#endif /* RASPBERRY_PI */
