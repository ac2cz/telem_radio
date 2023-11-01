/*
 * config.h
 *
 *  Created on: Feb 21, 2022
 *      Author: g0kla
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
 */

#ifndef CONFIG_H_
#define CONFIG_H_

#include "debug.h"

#define VERSION __DATE__ " - Version 0.5"
#define DEBUG 1
//#define PTT_WITH_GPIO


/* These defines control if the sensors are enabled */
//#define PRESSURE_SENSOR
//#define GAS_SENSOR_ADC
#define REAL_TIME_CLOCK

#define true 1
#define false 0
#define EPOCH_START_YEAR 2000

#define SPACECRAFT_ID 11      /* This is the id placed in the header of each packet */

#define MAX_LINE_LENGTH 128

#define SAMPLE_RATE "sample_rate"
#define ONE_VALUE "one_value"
#define ZERO_VALUE "zero_value"
#define RAMP_AMOUNT "ramp_amount"
#define RAMP_BITS_TO_COMPENSATE_HPF "ramp_bits_to_compensate_hpf"

/* Global variables declared here. All must start with g_ They are defined in main.c */
extern int g_verbose;          /* set from command line switch or from the cmd console */
extern int g_sample_rate;      /* sample rate used by the audio processor */

/* These values specify the strength of the telemetry. */
extern double g_one_value;
extern double g_zero_value;
extern double g_ramp_amount; /* When bits have the same value ramp the amount up to compensate for HPF in the radio transmitter */
extern int g_ramp_bits_to_compensate_hpf; /* Apply a slight ramp to the bits to compensate for high pass filter in the radio */

extern int g_ptt_state; /* PTT state for RTS or GPIO control */
extern int g_serial_fd; /* the file descriptor for the serial port */

void load_config();

#endif /* CONFIG_H_ */
