/*
 * TELEM RADIO
 * main.c
 *
 *  Created on: Feb 21, 2022
 *      Author: g0kla
 *
 *  Copyright (C) 2022
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
 */

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

#include "config.h"
#include "debug.h"
#include "iir_filter.h"
#include "audio_processor.h"
#include "audio_tools.h"
#include "cheby_iir_filter.h"
#include "fir_filter.h"
#include "oscillator.h"

int run_tests = false;
int more_help = false;

int run_self_test() {
	int rc = 0;
	int fail = 0;
	printf("\nRunning Self Test..\n");
	//rc = test_oscillator(); exit(1);
	//rc = test_iir_filter(); exit(1);
	rc = test_rs_encoder();    if (rc != 0) fail = 1;
	rc = test_sync_word();     if (rc != 0) fail = 1;
	rc = test_get_next_bit();  if (rc != 0) fail = 1;
	rc = test_audio_tools();   if (rc != 0) fail = 1;
	rc = test_modulate_bit();  if (rc != 0) fail = 1;

	if (fail == 0)
		printf("All Tests Passed\n\n");
	else
		printf("Some Tests Failed\n\n");
	return rc;
}

/**
 * Print this help if the -h or --help command line options are used
 */
void help(void) {
	printf(
			"Usage: telem_radio [OPTION]... \n"
			"-h,--help      help\n"
			"-t,--test      run self tests before starting the audio\n"
	);
	exit(0);
}

int main(int argc, char *argv[]) {

	struct option long_option[] =
	{
			{"help", 0, NULL, 'h'},
			{"test", 0, NULL, 't'},
			{NULL, 0, NULL, 0},
	};

	more_help = 0;
	while (1) {
		int c;
		if ((c = getopt_long(argc, argv, "ht", long_option, NULL)) < 0)
			break;
		switch (c) {
		case 'h':
			more_help = true;
			break;
		case 't':
			run_tests = true;
			break;
		}
	}

	if (more_help) {
		help();
		return 0;
	}
	printf("TELEM Radio Platform\n");
	verbose_print("Build: %s\n", VERSION);

	int rc = 0;
	if (run_tests) {
		rc = run_self_test();
		if (rc != 0)
    		exit(rc);
    }
    rc = start_audio_processor();

	printf("Exiting TELEM radio platform ..");
	return rc;
}
