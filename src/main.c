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
 *
 * This is the main program for telem_radio.  It handles command line parameters and starts the
 * telem_radio components:
 * 	telem_processor - The telemetry processor encodes packets and supplies the bits to the
 *                    audio processor.
 * 	telem_thread    - The telem thread runs in the background and gathers new telemetry from the
 *                    subsystems and sensors.
 * 	audio_processor - This is an audio loop that reads audio from the sound card, processes it and
 * 	                  then writes it back to the sound card.
 *
 *
 */

/* System include files */
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>

/* project include files */
#include "config.h"
#include "debug.h"
#include "jack_audio.h"
#include "audio_processor.h"
#include "gpio_interface.h"
#include "cmd_console.h"
#include "serial.h"

#include "device_lps25hb.h"

/* Included for self tests */
#include "iir_filter.h"
#include "audio_tools.h"
#include "cheby_iir_filter.h"
#include "fir_filter.h"
#include "oscillator.h"
#include "../telem_send/inc/telem_processor.h"
#include "../telem_send/inc/telem_thread.h"

/*
 *  GLOBAL VARIABLES defined here.  They are declared in config.h
 *  These are the default values.  Many can be updated with a value
 *  in telem_radio.config or can be over riden on the command line.
 *
 */
int g_verbose = false;
int g_sample_rate = 48000;
double g_one_value = 0.2;
double g_zero_value = -0.2;
double g_ramp_amount = 0.02;
int g_ramp_bits_to_compensate_hpf = true;
int g_ptt_state = 0;
int g_serial_fd = -1;

/* local variables for this file */
pthread_t telem_pthread;
int run_tests = false;
int more_help = false;
int filter_test_num = 0;
int print_filter_test_output = true;

int run_self_test() {
	int rc = EXIT_SUCCESS;
	int fail = EXIT_SUCCESS;
	printf("\nRunning Self Test..\n");
	//rc = test_oscillator(); exit(1);

	rc = test_rs_encoder();    if (rc != EXIT_SUCCESS) fail = EXIT_FAILURE;
	rc = test_sync_word();     if (rc != EXIT_SUCCESS) fail = EXIT_FAILURE;
	rc = test_get_next_bit();  if (rc != EXIT_SUCCESS) fail = EXIT_FAILURE;
	rc = test_modulate_bit();  if (rc != EXIT_SUCCESS) fail = EXIT_FAILURE;  ////////// WHY SOMETIMES FAILS??
	rc = test_encode_packet();  if (rc != EXIT_SUCCESS) fail = EXIT_FAILURE;
	rc = test_gather_duv_telemetry(); if (rc != EXIT_SUCCESS) fail = EXIT_FAILURE;

	//	rc = test_audio_tools();   if (rc != EXIT_SUCCESS) fail = EXIT_FAILURE; // audio tools not currently used


	if (fail == EXIT_SUCCESS)
		printf("All Tests have Passed\n\n");
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
			"use telem_radio/scripts/run_filter_test.sh to display the output in a graph\n"
#endif
	);
	exit(EXIT_SUCCESS);
}

void signal_handler (int sig) {
	debug_print (" Signal received, exiting ...\n");
	closeserial(g_serial_fd);
	telem_thread_stop();
	stop_cmd_console();
	stop_jack_audio_processor();
	sleep(1); // give jack time to close
	cleanup_telem_processor();

	exit (0);
}

int main(int argc, char *argv[]) {

	signal (SIGQUIT, signal_handler);
	signal (SIGTERM, signal_handler);
	signal (SIGHUP, signal_handler);
	signal (SIGINT, signal_handler);

	/* Load configuration from the config file */
	load_config();

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
			g_verbose = true;
			break;
		}
	}

	if (more_help) {
		help();
		return 0;
	}

	if (!filter_test_num) {
		printf("TELEM Radio Platform\n");
	    printf("Build: %s\n", VERSION);
	}
	int rc = EXIT_SUCCESS;



#ifdef RASPBERRY_PI
	if (!filter_test_num)
		debug_print("Running on a Raspberry PI\n");
	gpio_init();

	if(init_lp25hb() != 0) {
		printf("ERROR: Can't connect to pressure sensor\n");
	} else {
		printf("Connected to pressure sensor\n");
	}

#endif

#ifdef LINUX
	if (!filter_test_num)
		debug_print("Running on Linux\n");
#endif

#ifndef PTT_WITH_GPIO
	char *serialdev = "/dev/ttyUSB0";

	g_serial_fd = openserial(serialdev);
	if (!g_serial_fd) {
		fprintf(stderr, "Error while initializing %s.\n", serialdev);
	//	return 1;
	}
	set_rts(g_serial_fd, g_ptt_state);
#endif

#ifdef DEBUG
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

	rc = init_telemetry_processor(DUV_PACKET_LENGTH);
	if (rc != EXIT_SUCCESS) {
		error_print("FATAL. Could not initialize the telemetry processor.\n");
		exit(rc);
	}

	char *name = "Telem Thread";
	rc = pthread_create( &telem_pthread, NULL, telem_thread_process, (void*) name);
	if (rc != EXIT_SUCCESS) {
		error_print("FATAL. Could not start the telemetry thread.\n");
		exit(rc);
	}

	rc = init_audio_processor(DUV_BPS,DUV_DECIMATION_RATE);
//	rc = init_audio_processor(FSK_1200_BPS,1);
	if (rc != EXIT_SUCCESS) {
		error_print("Initialization error with audio processor\n");
		return rc;
	}

    rc = start_jack_audio_processor();
    if (rc != EXIT_SUCCESS) {
    		error_print("FATAL. Could not start the jack audio thread.\n");
    		exit(rc);
    	}
    rc = start_cmd_console();  // this will run until the user exits (or until we receive a signal)
    if (rc != EXIT_SUCCESS) {
    	error_print("FATAL. Error with the command console.\n");
    	exit(rc);
    }

    closeserial(g_serial_fd);
    telem_thread_stop();
    cleanup_telem_processor();

	printf("Exiting TELEM radio platform ..\n");
	stop_jack_audio_processor();
	return rc;
}
