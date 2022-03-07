/*
 * main.c
 *
 *  Created on: Feb 21, 2022
 *      Author: g0kla
 */

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

#include "audio_processor.h"
#include "config.h"
#include "audio_tools.h"
#include "cheby_iir_filter.h"
#include "fir_filter.h"
#include "IIRFilterCode.h"
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
	printf("Build: %s\n", VERSION);

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
