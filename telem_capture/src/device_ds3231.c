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
#include <time.h>
#include "debug.h"
#include "device_ds3231.h"
#include "gpio_interface.h"

#define BCD2BIN(val) (((val) & 15) + ((val) >> 4) * 10)

/*!
      @brief  Convert a binary coded decimal value to binary. RTC stores
    time/date values as BCD.
      @param val BCD value
      @return Binary value
  */
  static uint8_t bcd2bin(uint8_t val) { return val - 6 * (val >> 4); }
  /*!
      @brief  Convert a binary value to BCD format for the RTC registers
      @param val Binary value
      @return BCD value
  */
  static uint8_t bin2bcd(uint8_t val) { return val + 6 * (val / 10); }

/**
 * Initialize the chip
 *
 */
int init_ds3231() {
    uint8_t reg[2];
    uint8_t buf[4];
    int rc = 0;
    reg[0] = DS3231_REG_CONTROL;
    reg[1] = 0x1c; // Control reg - enable oscillator, all else default
/*    rc = i2c_write(DS3231_ADDRESS, reg, 2);
    if (rc != BCM2835_I2C_REASON_OK) {
        debug_print("Error: %d Could not write control reg on RTS\n", rc);
        return rc;
    }
*/

    rc = i2c_read_register_rs(DS3231_ADDRESS, reg, buf, 1);
    if (rc != BCM2835_I2C_REASON_OK) {
        debug_print("Error: %d Could not read control register from the RTC\n", rc);
        return rc;
    }
    debug_print("RTC CONTROL: %02x\n", buf[0]);
    if (buf[0] != 0x1c) {
        debug_print("Error: RTC control register in unknown state %02x\n", buf[0]);
        return 1;
    }

    reg[0] = DS3231_REG_STATUS; 
    rc = i2c_read_register_rs(DS3231_ADDRESS, reg, buf, 1);
    if (rc != BCM2835_I2C_REASON_OK) {
        debug_print("Error: %d Could not read status from the RTC\n", rc);
        return rc;
    }
    debug_print("RTC STATUS: %02x\n", buf[0]);

    if ((buf[0] & 0x80) == 0x80) {
        debug_print("RTC oscillator is stopped, starting ..\n");
        reg[1] = buf[0] & ~0x80; // clear the stop flag
        rc = i2c_write(DS3231_ADDRESS, reg, 2);
        if (rc != BCM2835_I2C_REASON_OK) {
            debug_print("Error: %d Could not write to RTS to clear the stop flag\n", rc);
            return rc;
        }
    	rc = i2c_read_register_rs(DS3231_ADDRESS, reg, buf, 1);
        if (rc != BCM2835_I2C_REASON_OK) {
            debug_print("Error: %d Could not re-read status from the RTC\n", rc);
       	    return rc;
        }
        debug_print("RTC STATUS: %02x\n", buf[0]);
        if ((buf[0] & 0x80) == 0x80) {
            debug_print("Error: Could not start the RTC\n");
            return 1;
        }
    }

    return 0;
}

/* Toggle the GPIO line to reset the chip.  This will reset the time to zero */
int reset_ds3231() {
	int rc = EXIT_SUCCESS;

	uint8_t val = LOW;
	debug_print("Writing GPIO PIn 18 to: %i\n", val);
	bcm2835_gpio_write(GPIO_PTT, val);
	bcm2835_delay(50);
	val = HIGH;
	debug_print("Writing GPIO PIn 18 to: %i\n", val);
	bcm2835_gpio_write(GPIO_PTT, val);

	return rc;

}

/* Set the time
   year 00-99 from 2000
   month 1-12
   day 1-31
   dow 1-7
   hour 0 - 23
   min 0 - 59
   sec 0 - 59 */
int ds3231_set_time(uint16_t year, uint8_t month, uint8_t day, uint8_t dow, uint8_t hour,
                   uint8_t min, uint8_t sec) {
    if (year < 0 || year > 99) return 1;
    if (month < 1 || month > 12) return 1;
    if (day < 1 || day > 31) return 1;
    if (dow < 1 || dow > 7) return 1;
    if (hour < 0 || hour > 23) return 1;
    if (min < 0 || min > 59) return 1;
    if (sec < 0 || sec > 59) return 1;
    uint8_t reg[2];
    int rc = 0;

    uint8_t buf[8] = {DS3231_REG_SECONDS,
                       bin2bcd(sec),
                       bin2bcd(min),
                       bin2bcd(hour),
                       bin2bcd(dow),
                       bin2bcd(day),
                       bin2bcd(month),
                       bin2bcd(year)};
    rc = i2c_write(DS3231_ADDRESS, buf, 8);
    if (rc != BCM2835_I2C_REASON_OK) {
        debug_print("Error: %d Could not set RTC time registers\n", rc);
        return rc;
    }

    reg[0] = DS3231_REG_STATUS;
    rc = i2c_read_register_rs(DS3231_ADDRESS, reg, buf, 1);
    if (rc != BCM2835_I2C_REASON_OK) {
        debug_print("Error: %d Could not read status from the RTC\n", rc);
        return rc;
    }

    if ((buf[0] & 0x80) == 0x80) { /* Osc is stopped, so start it */
        debug_print("Osc was stopped, starting it\n");
        reg[1] = buf[0] &  ~0x80; // flip OSF bit
        rc = i2c_write(DS3231_ADDRESS, reg, 2);
        if (rc != BCM2835_I2C_REASON_OK) {
            debug_print("Error: %d Could not flip RTC OSF bit\n", rc);
            return rc;
        }
    }

    return 0;
}

/* Convert the time */
void rtc_regs_to_time(struct tm *time, uint8_t *regs) {
    /* tm_sec seconds [0,61] */
    time->tm_sec = bcd2bin(regs[0] & 0x7F);

    /* tm_min minutes [0,59] */
    time->tm_min = bcd2bin(regs[1]);

    /* tm_hour hour [0,23] */
    time->tm_hour = bcd2bin(regs[2]);
    if (regs[2] & 0x40) { /* if bit 6 set then in 12 hour mode */
        if (regs[2] & 0x20) { /* Bit 5 is the am/pm bit */
            time->tm_hour = time->tm_hour + 12;
        }
    }

    /* tm_wday day of week [0,6] (Sunday = 0) */
    time->tm_wday = bcd2bin(regs[3]) - 1;

    /* tm_mday day of month [1,31] */
    time->tm_mday = bcd2bin(regs[4]);

    /* tm_mon month of year [0,11] */
    time->tm_mon = bcd2bin(regs[5] & 0x7f) - 1;

    /* tm_year years since 2000 */
    if (regs[5] & 0x80)
        time->tm_year = bcd2bin(regs[6]) + 200;
    else
        time->tm_year = bcd2bin(regs[6]) + 100;
}

int ds3231_get_time() {
    uint8_t reg[2];
    uint8_t buf[7];
    int rc = 0;
    reg[0] = DS3231_REG_SECONDS;
    rc = i2c_read_register_rs(DS3231_ADDRESS, reg, buf, 7);
    if (rc != BCM2835_I2C_REASON_OK) {
        debug_print("Error: %d Could not write control reg on RTS\n", rc);
        return rc;
    }
//    debug_print("%02x %02x %02x\n",buf[2], buf[1], buf[0]);
//   debug_print("%02x %02x %02x %02x\n",buf[6], buf[5], buf[4], buf[3]);

    struct tm tm_time;
    rtc_regs_to_time(&tm_time, buf);

    debug_print("Date: %d-%d-%d %02d:%02d:%02d UTC\n",(tm_time.tm_year+1900), tm_time.tm_mon+1, tm_time.tm_mday, tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);


    return 0;
 
}

#endif /* RASPBERRY_PI */
