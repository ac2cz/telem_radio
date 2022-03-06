/*
 * main.c
 *
 *  Created on: Feb 21, 2022
 *      Author: g0kla
 */

#include <stdio.h>
#include <stdlib.h>

#include "../inc/audio_processor.h"
#include "../inc/config.h"
#include "../inc/audio_tools.h"
#include "../inc/cheby_iir_filter.h"
#include "../inc/fir_filter.h"
#include "../inc/IIRFilterCode.h"
#include "../inc/oscillator.h"

int run_tests = false;

int run_self_test() {
	int rc = 0;
	int fail = 0;
	printf("\nRunning Self Test..\n");
	//rc = test_oscillator();
	rc = test_iir_filter();
	exit(1);
	rc = test_rs_encoder();
	if (rc != 0) fail = 1;
	rc = test_sync_word();
	if (rc != 0) fail = 1;
	rc = test_get_next_bit();
	if (rc != 0) fail = 1;
	rc = test_audio_tools();
	if (rc != 0) fail = 1;

	rc = test_modulate_bit();
	if (fail == 0)
		printf("All Tests Passed");
	else
		printf("Some Tests Failed\n");
	return rc;
}

int main(int argc, char *argv[]) {
	printf("TELEM Radio Platform\n");
	printf("Build: %s\n", VERSION);

	int rc = 0;
	if (run_tests == false) {
		rc = start_audio_processor();
	} else {
		rc = run_self_test();
		return rc;
	}

	printf("Exiting TELEM radio platform ..");
	return rc;
}
