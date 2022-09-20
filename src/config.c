/*
 * config.c
 *
 *  Created on: May 17, 2022
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
 *  Load user configuration variables from a file.  This should hold all of the values
 *  that might change from one environment to the next.  e.g. different gains may
 *  be needed for different sound cards.
 *
 */

#include <string.h>
#include <stdlib.h>

#include "config.h"

char filename[] = "telem_radio.config";

void load_config() {
	char *key;
	char *value;
	char *search = "=";
	debug_print("Loading config from: %s:\n", filename);
	FILE *file = fopen ( filename, "r" );
	if ( file != NULL ) {
		char line [ MAX_LINE_LENGTH ]; /* or other suitable maximum line size */
		while ( fgets ( line, sizeof line, file ) != NULL ) /* read a line */ {

			/* Token will point to the part before the =
			 * Using strtok safe here because we do not have multiple delimiters and
			 * no other threads started at this time. */
			key = strtok(line, search);

			// Token will point to the part after the =.
			value = strtok(NULL, search);
			if (value != NULL) { /* Ignore line with no key value pair */;

				debug_print(" %s",key);
				debug_print(" = %s",value); // don't need a new line as we have not stripped the new line from the line read in

				if (strcmp(key, SAMPLE_RATE) == 0) {
					int rate = atoi(value);
					g_sample_rate = rate;
				} else if (strcmp(key, ONE_VALUE) == 0) {
					float fval = atof(value);
					g_one_value = fval;
				} else if (strcmp(key, ZERO_VALUE) == 0) {
					float fval = atof(value);
					g_zero_value = fval;
				} else if (strcmp(key, RAMP_AMOUNT) == 0) {
					float fval = atof(value);
					g_ramp_amount = fval;
				} else if (strcmp(key, RAMP_BITS_TO_COMPENSATE_HPF) == 0) {
					int intval = atoi(value);
					g_ramp_bits_to_compensate_hpf = intval;
				} else {
					error_print("Unknown key in %s file: %s\n",filename, key);
				}
			}
		}
		fclose ( file );
	} else {
		error_print("FATAL..  Could not load config file: %s\n", filename);
		exit(1);
	}
}

