/*
 * cmd_console.c
 *
 *  Created on: Apr 11, 2022
 *      Author: g0kla
 */

/* System */
#include <stdlib.h>
#include <string.h>

/* Program */
#include "config.h"
#include "debug.h"
#include "audio_processor.h"
#include "telem_processor.h"
#include "oscillator.h"
#include "gpio_interface.h"
#include "duv_telem_layout.h"

/* Forward function declarations */
void print_status(char *name, int status);
void print_full_status();

int cmd_console_running = true;

char *help_str =
		"TELEM Radio Platform Console Commands:\n"
		" (s)tatus      - display settings and status\n"
		" (f)ilter      - Toggle high pass filter on/off\n"
		" (l)ow pass filter   - Toggle bit low high pass filter on/off\n"
		" (t)elem       - Toggle DUV telemetry on/off\n"
		" (hs)highspeed - Toggle High Speed telemetry on/off\n"
		" (p)tt         - Toggle the radio on/off\n"
		" test          - DUV telem contains only 101010 on/off\n"
		" tone          - Generate test tone\n"
		" freq <Hz>     - Set freq of test tone\n"
		" measure       - Display measurement for input tone\n"
		" (h)help       - show this help\n"
		" (q)uit        - Shutdown and exit\n\n";


/*
 * Print a status line to the console for a boolean variable
 */
void print_status(char *name, int status) {
	char *val = status ? " ON " : " OFF";
	printf(" %s : %s\n",val, name);
}

/*
 * Print status for all paramaters to the console
 */
void print_full_status() {
	printf("TELEM Radio status:\n");
	int rate = g_sample_rate/get_decimation_rate();
	printf(" audio engine sample rate: %" PRIu32 " with decimation by %d to %d\n", g_sample_rate,get_decimation_rate(), rate);
	printf(" samples per bit: %d. Ramp to compensate HPF: %d", get_samples_per_bit(), g_ramp_bits_to_compensate_hpf);
	if (g_ramp_bits_to_compensate_hpf)
		printf(" amount %.2f", g_ramp_amount);
	printf("\n");
	printf(" test tone freq %d Hz\n",(int)get_test_tone_freq());
	print_status("High Pass Filter", get_hpf());
	print_status("Bit Low Pass Filter", get_lpf_bits());
	print_status("DUV Telemetry", get_send_telem());
	print_status("High Speed Telemetry", get_send_high_speed_telem());
	print_status("Test Telem", get_send_test_telem());
	print_status("Transmitter", gpio_get_ptt());

	print_status("Generate test tone", get_send_test_tone());
	print_status("Measure input test tone", get_measure_test_tone());
	print_status("Verbose Output", g_verbose);
}

void stop_cmd_console() { cmd_console_running = false; }

/*
 * Start an interactive command console that allows the user to issue commands to the
 * telem_radio platform.  This should only be attached for lab testing.
 *
 */
int start_cmd_console() {

	printf("Type (q)uit to exit, or (h)help..\n\n");
	size_t buffer_size = 32;
	char *line;
	line = (char *)malloc(buffer_size * sizeof(char));
	if( line == NULL)
	{
		error_print("Unable to allocate line buffer");
		exit(1);
	}

	while (cmd_console_running) {
		int rc = getline(&line, &buffer_size, stdin);
		if (rc > 1) {
			char * token;
			line[strcspn(line, "\n")] = '\0';
			token = strsep(&line, " ");

			if (strcmp(token, "filter") == 0|| strcmp(token, "f") == 0) {
				set_hpf(!get_hpf());
				print_status("High Pass Filter", get_hpf());
			} else if (strcmp(token, "low") == 0 || strcmp(token, "l") == 0) {
				set_lpf_bits(!get_lpf_bits());
				print_status("Bit Low Pass Filter", get_lpf_bits());
			} else if (strcmp(token, "telem") == 0 || strcmp(token, "t") == 0) {
				set_send_telem(!get_send_telem());
				set_send_high_speed_telem(false);
				if (get_send_telem() == true) { // reset the modulator
					rc = init_bit_modulator(DUV_BPS, DUV_DECIMATION_RATE);
					if (rc != EXIT_SUCCESS) {
						error_print("Issue initializing the modulator.  It may not work correctly..");
					}
				}
				print_status("Telemetry", get_send_telem());
			} else if (strcmp(token, "highspeed") == 0 || strcmp(token, "hs") == 0) {
				set_send_high_speed_telem(!get_send_high_speed_telem());
				set_send_telem(get_send_high_speed_telem());
				if (get_send_high_speed_telem() == true) { // reset the modulator
					rc = init_bit_modulator(FSK_1200_BPS, 1);
					if (rc != EXIT_SUCCESS) {
						error_print("Issue initializing the high speed bit modulator.  It may not work correctly..");
					}
				}
				print_status("Telemetry", get_send_telem());
			} else if (strcmp(token, "ptt") == 0 || strcmp(token, "p") == 0) {
				gpio_set_ptt(!gpio_get_ptt());
				print_status("Transmitter", gpio_get_ptt());
			} else if (strcmp(token, "test") == 0) {
				set_send_test_telem(!get_send_test_telem());
				print_status("Test Telem", get_send_test_telem());
			} else if (strcmp(token, "verbose") == 0|| strcmp(token, "v") == 0) {
				g_verbose = !g_verbose;
				print_status("Verbose Output", g_verbose);
			} else if (strcmp(token, "measure") == 0) {
				set_measure_test_tone(!get_measure_test_tone());
				print_status("Measure test tone", get_measure_test_tone());
			} else if (strcmp(token, "freq") == 0) {
				token = strsep(&line, " ");
				float freq = atof(token);
				if (freq == 0.0)
					printf("Invalid frequency: %s\n", token);
				else {
					set_test_tone_freq(freq);
					printf("Test tone frequency now: %d Hz\n", (int)get_test_tone_freq());
				}
			} else if (strcmp(token, "tone") == 0) {
				set_send_test_tone(!get_send_test_tone());
				print_status("Send Test Tone", get_send_test_tone());
			} else if (strcmp(token, "status") == 0 || strcmp(token, "s") == 0) {
				print_full_status();
			} else if (strcmp(token, "help") == 0 || strcmp(token, "h") == 0) {
				printf("%s\n",help_str);
			} else if (strcmp(token, "quit") == 0 || strcmp(token, "q") == 0) {
				break;
			} else {
				printf("Unknown command: %s\n", line);
			}
		}
	}
	free(line);
	printf("Stopping audio processor ..\n");
	/* Make sure the transmitter is off as we exit */
	gpio_set_ptt(LOW);
	print_status("Transmitter", gpio_get_ptt());

	return 0;
}
