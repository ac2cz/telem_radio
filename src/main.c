/*
 * main.c
 *
 *  Created on: Feb 21, 2022
 *      Author: g0kla
 */

#include <stdio.h>

#include "../inc/audio_processor.h"
#include "../inc/config.h"
#include "../inc/audio_tools.h"
#include "../inc/fir_filter.h"
#include "../inc/iir_filter.h"
#include "../inc/oscillator.h"

int run_tests = false;

int main(int argc, char *argv[]) {
	printf("TELEM Radio Platform\n");
	printf("Build: %s\n", VERSION);

	int rc = 0;
	if (run_tests == false) {
		rc = start_audio_processor();
	} else {
		//rc = test_oscillator();
		rc = test_iir_filter();

//		printf("Running Self Test..\n");
//		rc = test_audio_tools();
//		if (rc == 0)
//			printf("Test Passed");
//		else
//			printf("Test Failed");
	}

	printf("Shutting down TELEM radio platform ..");
	return rc;
}
