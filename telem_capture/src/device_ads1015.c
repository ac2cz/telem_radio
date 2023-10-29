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
int init_ads() {
    uint8_t reg[2];
    uint8_t reg2[1];
    uint8_t buf[4];
    reg[0] = ADS1015_REG_CONFIG; 
    int rc = i2c_read_register_rs(ADS1015_ADDRESS, reg, buf, 2);
    debug_print("ADS1015 CONFIG %02x %02x\n",buf[0], buf[1]);

    /* Set the Boot bit 
    reg[0] = LPS25HB_REG_CTRL_REG2; // Control Register 2
    reg[1] = 0x80; // Boot
    rc = i2c_write(LPS25HB_ADDRESS, reg, 2);
    */
    return rc;
}


#endif /* RASPBERRY_PI */
