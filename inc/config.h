/*
 * config.h
 *
 *  Created on: Feb 21, 2022
 *      Author: g0kla
 */

#ifndef CONFIG_H_
#define CONFIG_H_

#include "debug.h"

#define VERSION __DATE__ " - Version 0.4"
#define DEBUG 1
#define true 1
#define false 0
#define EPOCH_START_YEAR 2000

#define SPACECRAFT_ID 11      /* This is the id placed in the header of each packet */

/* Global variables declared here. All must start with g_ They are defined in main.c */
extern int g_verbose;          /* set from command line switch or from the cmd console */
extern int g_sample_rate;      /* sample rate used by the audio processor */
extern int g_ramp_bits_to_compensate_hpf; /* Apply a slight ramp to the bits to compensate for high pass filter in the radio */

#endif /* CONFIG_H_ */
