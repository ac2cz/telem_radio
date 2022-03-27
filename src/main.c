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

/* System include files */
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

/* project include files */
#include "config.h"
#include "debug.h"
#include "jack_audio.h"
#include "audio_processor.h"

/* Included for self tests */
#include "iir_filter.h"
#include "telem_processor.h"
#include "audio_tools.h"
#include "cheby_iir_filter.h"
#include "fir_filter.h"
#include "oscillator.h"

/*
 *  GLOBAL VARIABLES defined here.  They are declared in config.h
 */
int verbose = false;
int sample_rate = 0;
unsigned short epoch = 0;


/* local variables for this file */
int run_tests = false;
int more_help = false;
int filter_test_num = 0;
int print_filter_test_output = true;

int run_self_test() {
	int rc = EXIT_SUCCESS;
	int fail = EXIT_SUCCESS;
	printf("\nRunning Self Test..\n");
	//rc = test_oscillator(); exit(1);
	rc = test_gather_duv_telemetry(); if (rc != EXIT_SUCCESS) fail = EXIT_FAILURE;
	rc = test_rs_encoder();    if (rc != EXIT_SUCCESS) fail = EXIT_FAILURE;
	rc = test_sync_word();     if (rc != EXIT_SUCCESS) fail = EXIT_FAILURE;
	rc = test_get_next_bit();  if (rc != EXIT_SUCCESS) fail = EXIT_FAILURE;
	rc = test_modulate_bit();  if (rc != EXIT_SUCCESS) fail = EXIT_FAILURE;  ////////// WHY SOMETIMES FAILS??
	//	rc = test_audio_tools();   if (rc != EXIT_SUCCESS) fail = EXIT_FAILURE; // audio tools not currently used

	if (fail == EXIT_SUCCESS)
		printf("All Tests Passed\n\n");
	else
		printf("Some Tests Failed\n\n");
	return fail;
}

int run_filter_test(int test_num, int print_filter_test_output) {

	int rc = EXIT_FAILURE;

	fprintf(stderr,"Running Test: %i ", test_num);
	if (print_filter_test_output == -1)
		fprintf(stderr," printing filter kernel\n");
	else
		fprintf(stderr,"Output printed = %i\n", print_filter_test_output);

	if (test_num == 1)
		rc = test_iir_filter(print_filter_test_output);
	else if (test_num == 2)
		rc = test_fir_filter(print_filter_test_output);
	else {
		error_print("Filter test %d does not exist.  Exiting", test_num);
		exit(EXIT_FAILURE);
	}

	return rc;
}

/**
 * Print this help if the -h or --help command line options are used
 */
void help(void) {
	printf(
			"Usage: telem_radio [OPTION]... \n"
			"-h,--help                        help\n"
			"-v,--verbose                     print additional status and progress messages\n"
#ifdef DEBUG
			"-t,--test                        run self tests before starting the audio\n"
			"-f,--filter-test <num>           Run a test on filter <num>\n"
			"-i, --print-filter-test-input    Print the input test wave instead of the filter output\n"
			"Valid filter tests are:\n"
			"    1 - high pass filter\n"
			"    2 - FIR bit filter\n"
			"use telem_radio/scripts/run_filter_test.sh to display the output in a graph"
#endif
	);
	exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]) {

	struct option long_option[] =
	{
			{"help", 0, NULL, 'h'},
			{"test", 0, NULL, 't'},
			{"verbose", 0, NULL, 'v'},
			{"filter-test", 1, NULL, 'f'},
			{"print-filter-test-input", 0, NULL, 'i'},
			{"print_filter_test_kernel", 0, NULL, 'k'},
			{NULL, 0, NULL, 0},
	};

	int err = 0;
	while (1) {
		int c;
		if ((c = getopt_long(argc, argv, "htvfi:", long_option, NULL)) < 0)
			break;
		switch (c) {
		case 'h': // help
			more_help = true;
			break;
		case 'f': // filter test
			err = atoi(optarg);
			filter_test_num = err;
			break;
		case 'i': // filter test input
			print_filter_test_output = 0;
			break;
		case 'k': // filter test kernel
			print_filter_test_output = -1;
			break;
		case 't': // self test
			run_tests = true;
			break;
		case 'v': // verbose
			verbose = true;
			break;
		}
	}

	if (more_help) {
		help();
		return 0;
	}

	if (!filter_test_num)
		printf("TELEM Radio Platform\n");
	verbose_print("Build: %s\n", VERSION);
	int rc = EXIT_SUCCESS;

#ifdef DEBUG

#ifdef RASPBERRY_PI
	if (!filter_test_num)
		debug_print("Running on a Raspberry PI");
#endif
#ifdef LINUX
	if (!filter_test_num)
		debug_print("Running on Linux");
#endif


	if (filter_test_num) {
		rc = run_filter_test(filter_test_num, print_filter_test_output);
		exit(rc);
	}
	if (run_tests) {
		rc = run_self_test();
		if (rc != EXIT_SUCCESS)
    		exit(rc);
    }
#endif

	rc = init_telemetry_processor();
	if (rc != EXIT_SUCCESS) {
		error_print("FATAL. Could not initialize the telemetry processor.\n");
		exit(rc);
	}
    rc = start_jack_audio_processor();

    cleanup_telem_processor();

	printf("Exiting TELEM radio platform ..\n");
	return rc;
}
