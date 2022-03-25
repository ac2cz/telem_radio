/*
 * telem_modulator.c
 *
 *  Created on: Feb 21, 2022
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
 *  This is based on the client demo for jackd
 *
 *  This audio loop reads audio from the sound card, processes it and then writes it back to
 *  the sound card.  Internally the audio is stored as doubles for all the calculations.  It
 *  is read and written to the sound card as floats.
 *
 *
 */

/* Standard C lib includes */
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <iir_filter.h>
#include <stdint.h>

/* Libraries */
#include <jack/jack.h>

/* telem_radio includes */
#include "config.h"
#include "debug.h"
#include "audio_processor.h"
#include "audio_tools.h"
#include "cheby_iir_filter.h"
#include "fir_filter.h"
#include "telem_processor.h"
#include "oscillator.h"

/* Forward function declarations */
double modulate_bit();
int init_bit_modulator();
jack_default_audio_sample_t * duv_audio_loop(jack_default_audio_sample_t *in,
		jack_default_audio_sample_t *out, jack_nframes_t nframes);
int init();
int init_filters();
void print_status(char *name, int status);
void print_full_status();
int cmd_console();

/* Test tone parameters */
#define OSC_TABLE_SIZE 9600
double osc_phase = 0;
double test_tone_freq = 5000.0f;
double osc_sin_table[OSC_TABLE_SIZE];

/* Tone measurement parameters */
int measurement_loops = 0;
#define LOOPS_PER_MEASUREMENT 500 // so measure once per 5 sec
int measurements = 0;
double peak_value = 0.0f;

// audio filter variables
#define DECIMATE_FILTER_LEN 480
#define DECIMATION_RATE 4
double decimate_filter_coeffs[DECIMATE_FILTER_LEN];
double decimate_filter_xv[DECIMATE_FILTER_LEN];

double interpolate_filter_coeffs[DECIMATE_FILTER_LEN];
double interpolate_filter_xv[DECIMATE_FILTER_LEN];

#define DUV_BIT_FILTER_LEN 60  // 60 is one bit.  If the filter is too long we get ringing.
double duv_bit_filter_coeffs[DUV_BIT_FILTER_LEN];
double duv_bit_filter_xv[DUV_BIT_FILTER_LEN];

double filtered_audio_buffer[PERIOD_SIZE]; // the audio samples after they are filtered by the decimation filter
double decimated_audio_buffer[PERIOD_SIZE/4]; // the audio samples after decimation and decimation filter
double hpf_decimated_audio_buffer[PERIOD_SIZE/4]; // the decimated audio samples after high pass filtering
double interpolated_audio_buffer[PERIOD_SIZE]; // the audio samples after interpolation back to 48000 but before interpolation filter

TIIRCoeff Elliptic8Pole300HzHighPassIIRCoeff;
TIIRCoeff Elliptic4Pole300HzHighPassIIRCoeff;
TIIRStorage iir_hpf_store; // storage for the IIR High Pass filter

/* Telemetry modulator settings */
int samples_per_duv_bit = 0; // this is calculated in the code.  For example it is 12000/200 = 60
int samples_sent_for_current_bit = 0; // how many samples have we sent for the current bit
int current_bit = 0; // the value of the current bit we are sending
int starting_bit_modulator = true;

/* User settings changeable from cmd console */
int hpf = true; // filter the transponder audio
int send_duv_telem = false;
int send_test_telem = false; // send a 10101 test telem sequence
int send_test_tone = false; // output a steady tone for measurement of a sound card
int measure_test_tone = false; // display the peak ampltude of a received tone to measure the sound card
int lpf_duv_bits = true;  // filter the DUV telem bits


/*
 * Turn the bit stream into samples that can be fed into the audio loop
 */
double modulate_bit() {
	if (starting_bit_modulator ||  // starting a new packet
			(samples_sent_for_current_bit >= samples_per_duv_bit )) { // We are starting a new bit
		samples_sent_for_current_bit = 0;
		starting_bit_modulator = false;
		if (send_test_telem)
			current_bit = !current_bit; // TESTING - just toggle the bit to generate 200Hz tone
		else
			current_bit = get_next_bit();
	}
	samples_sent_for_current_bit++;
	double bit_audio_value = current_bit ? ONE_VALUE : ZERO_VALUE;

	if (lpf_duv_bits)
		bit_audio_value = fir_filter(bit_audio_value, duv_bit_filter_coeffs, duv_bit_filter_xv, DUV_BIT_FILTER_LEN);

	return bit_audio_value;
}

int init_bit_modulator() {
	starting_bit_modulator = true;
	current_bit = 0;
	samples_sent_for_current_bit = 0;
	samples_per_duv_bit = sample_rate / DECIMATION_RATE / DUV_BPS;
	return EXIT_SUCCESS;
}

/**
 * Prototype audio loop
 * This has too many loops within the loops and will be optimized
 * Currently this uses FIR decimation filters which will be replaced with a more efficient alternative
 *
 */
jack_default_audio_sample_t * duv_audio_loop(jack_default_audio_sample_t *in,
		jack_default_audio_sample_t *out, jack_nframes_t nframes) {

	//	memcpy (out, in, sizeof (jack_default_audio_sample_t) * nframes);

	for (int i = 0; i< nframes; i++) {
		filtered_audio_buffer[i] = fir_filter((double)in[i], decimate_filter_coeffs, decimate_filter_xv, DECIMATE_FILTER_LEN);
	}

	int decimate_count = 0;

	for (int i = 0; i< nframes; i++) {
		decimate_count++;
		if (decimate_count == DECIMATION_RATE) {
			decimate_count = 0;
			decimated_audio_buffer[i/DECIMATION_RATE] = filtered_audio_buffer[i];
		}
	}

	/**
	 * Now we high pass filter
	 */
	if (hpf) {
	//	iir_filter_array(Elliptic8Pole300HzHighPassIIRCoeff, decimated_audio_buffer, hpf_decimated_audio_buffer, nframes/DECIMATION_RATE);
		for (int i = 0; i< nframes/DECIMATION_RATE; i++)
			hpf_decimated_audio_buffer[i] = iir_filter(Elliptic8Pole300HzHighPassIIRCoeff,decimated_audio_buffer[i], &iir_hpf_store);
	//		hpf_decimated_audio_buffer[i] = cheby_iir_filter(decimated_audio_buffer[i], a_hpf_025, b_hpf_025);
	} else {
		for (int i = 0; i< nframes/DECIMATION_RATE; i++)
			hpf_decimated_audio_buffer[i] = decimated_audio_buffer[i];
	}

	/**
	 * Insert DUV telemetry.
	 */
	if (send_duv_telem) {
		for (int i = 0; i< nframes/DECIMATION_RATE; i++) {
			float bit_audio_value = modulate_bit();
			hpf_decimated_audio_buffer[i] += bit_audio_value; // add the telemetry
		}
	}

	/**
	 * We interpolate by adding 3 samples with zero between each sample.  This creates the same signal
	 * at 48k with duplications of the spectrum every 9600Hz.  So we need to filter out those duplicates
	 * from the final signal.  We apply gain equal to DECIMATION_RATE to compensate for the loss of signal
	 * from the inserted samples.
	 */
	float gain = (float)DECIMATION_RATE;
	for (int i = 0; i < nframes; i++) {
		decimate_count++;
		if (decimate_count == DECIMATION_RATE) {
			decimate_count = 0;
			interpolated_audio_buffer[i] = gain * hpf_decimated_audio_buffer[i/DECIMATION_RATE];
		} else
			interpolated_audio_buffer[i] = 0.0f;

	}
	/* Now filter out the duplications of the spectrum that interpolation introduces */
	for (int i = 0; i< nframes; i++) {
		out[i] = (float)fir_filter(interpolated_audio_buffer[i], interpolate_filter_coeffs, interpolate_filter_xv, DECIMATE_FILTER_LEN);
	}

	return out;
}

jack_default_audio_sample_t * audio_loop(jack_default_audio_sample_t *in, jack_default_audio_sample_t *out, jack_nframes_t nframes) {
	if (send_test_tone) {
			for (int i=0; i < nframes; i++) {
				double value = nextSample(&osc_phase, test_tone_freq, sample_rate, osc_sin_table, OSC_TABLE_SIZE);
				out[i] = 0.2 * value;
			}
		} else if (measure_test_tone) {
			for (int i=0; i < nframes; i++) {
				//out[i] = 0; // silence the output, except this does not work if we were already receiving a tone?
				out[i] = in[i];  // play what we are measuring
				//printf("%f\n", in[i]);

				if (in[i] > peak_value)  //////////// This compare does not WORK!!
					peak_value = in[i];

				measurements++;

			}
			measurement_loops++;
			if (measurement_loops >= LOOPS_PER_MEASUREMENT) {
				printf("Peak over %d measurements: %f\n", measurements, peak_value);
				measurement_loops = 0;
				measurements = 0;
				peak_value = 0;
			}
		} else {
			/*
			 * Now process the data in out buffer before we sent it to the radio
			 * First we apply a high pass filter at 300Hz
			 */
			duv_audio_loop(in, out, nframes);
		}
	return out;
}

/*
 * This initializes the audio processor and should be called when it is first started
 */
int init() {
	// Init
	/* Encode first packet */
	int rc;
	rc = init_bit_modulator();

	/* now we know the sample rate then setup things that are dependent on that */
	rc = init_filters();
	if (rc != 0) {
		error_print("Error initializing filters\n");
		return rc;
	}

	return 0;
}

/*
 * This is called at startup to populate the coefficients for digital filters
 */
int init_filters() {
	verbose_print("Generating filters ..\n");

	/* Decimation filter */
	int decimation_cutoff_freq = sample_rate / (2* DECIMATION_RATE);
	int rc = gen_raised_cosine_coeffs(decimate_filter_coeffs, sample_rate, decimation_cutoff_freq, 0.5f, DECIMATE_FILTER_LEN);
	memset(decimate_filter_xv, 0, DECIMATE_FILTER_LEN*sizeof(decimate_filter_xv[0]));

	if (rc != 0)
		return rc;

	iir_hpf_store.MaxRegVal = 1.0E-12;
	for(int i=0; i<ARRAY_DIM; i++) {
		iir_hpf_store.RegX1[i] = 0.0;
		iir_hpf_store.RegX2[i] = 0.0;
		iir_hpf_store.RegY1[i] = 0.0;
		iir_hpf_store.RegY2[i] = 0.0;
	}

	/* High pass filter Cutoff 0.05 - 300Hz at 12k, 8 poles, 0.1dB ripple, 80dB stop band */
	Elliptic8Pole300HzHighPassIIRCoeff = (TIIRCoeff) {
				.a0 = {1.0,1.0,1.0,1.0},
				.a1 = {-1.457958640999101440,-1.801882953872335770,-1.918405877608232670,-1.961807844116467030},
				.a2 = { 0.553994469886055829, 0.847908435354810197, 0.948117893349852192, 0.986885245659999910},
				.a3 = {0.0,0.0,0.0,0.0},
				.a4 = {0.0,0.0,0.0,0.0},

				.b0 = { 0.755468172841911700, 0.914802148903627210, 0.967898257208821722, 0.987349171838800999},
				.b1 = {-1.501016765201333980,-1.820187091419891430,-1.930727256540441420,-1.973994746098864940},
				.b2 = { 0.755468172841911700, 0.914802148903627210, 0.967898257208821722, 0.987349171838800999},
				.b3 = {0.0,0.0,0.0,0.0},
				.b4 = {0.0,0.0,0.0,0.0},
				.NumSections = 4
			};

//	/* High pass filter Cutoff 0.05 - 300Hz at 12k, 4 poles, 0.1dB ripple, 80dB stop band */
//	Elliptic4Pole300HzHighPassIIRCoeff = (TIIRCoeff) {
//		.a0 = {1.0,1.0},
//				.a1 = {-1.632749182559936950,-1.902361314288061540},
//				.a2 = {0.680318914944733955,0.928685994848813867},
//				.a3 = {0.0,0.0},
//				.a4 = {0.0,0.0},
//
//				.b0 = {0.828466075118585832,0.957801757196423798},
//				.b1 = {-1.656135947267499240,-1.915443794744027710},
//				.b2 = {0.828466075118585832,0.957801757196423798},
//				.b3 = {0.0,0.0},
//				.b4 = {0.0,0.0},
//				.NumSections = 2
//	};

	/* High pass filter Cutoff 0.05 - 300Hz at 12k, 4 poles, 0.02dB ripple, 60dB stop band */
	Elliptic4Pole300HzHighPassIIRCoeff = (TIIRCoeff) {
		.a0 = {1.0,1.0},
		.a1 = {-1.672069386975465260,-1.893899543708855940},
		.a2 = {0.707257251879312099,0.919220780682049821},
		.a3 = {0.0,0.0},
		.a4 = {0.0,0.0},

		.b0 = {0.845362532264354760,0.953385271211727114,},
		.b1 = {-1.688601574326067830,-1.906349781967451530},
		.b2 = {0.845362532264354760,0.953385271211727114},
		.b3 = {0.0,0.0},
		.b4 = {0.0,0.0},
		.NumSections = 2
	};

	/* Interpolation filter */
	int interpolation_cutoff_freq = sample_rate / (2* DECIMATION_RATE);
	memset(interpolate_filter_xv, 0, DECIMATE_FILTER_LEN*sizeof(interpolate_filter_xv[0]));
	rc = gen_raised_cosine_coeffs(interpolate_filter_coeffs, sample_rate, interpolation_cutoff_freq, 0.5f, DECIMATE_FILTER_LEN);
	if (rc != 0)
		return rc;

	/* Bit shape filter - SHOULD BE UPDATED TO ROOT RAISED COSINE TO MATCH FOXTELEM */
	int duv_bit_cutoff_freq = 200;
	memset(duv_bit_filter_xv, 0, DUV_BIT_FILTER_LEN*sizeof(duv_bit_filter_xv[0]));
	rc = gen_root_raised_cosine_coeffs(duv_bit_filter_coeffs, sample_rate/DECIMATION_RATE, duv_bit_cutoff_freq, 0.5f, DUV_BIT_FILTER_LEN);

	return rc;
}

char *help_str =
		"TELEM Radio Platform Console Commands:\n"
		" (s)tatus   - display settings and status\n"
		" (f)ilter   - Toggle high pass filter on/off\n"
		" (l)ow pass filter   - Toggle bit low high pass filter on/off\n"
		" (t)elem    - Toggle DUV telemetry on/off\n"
		" test       - DUV telem contains only 101010 on/off\n"
		" tone       - Generate test tone\n"
		" freq <Hz>  - Set freq of test tone\n"
		" measure    - Display measurement for input tone\n"
		" (h)help    - show this help\n"
		" (q)uit     - Shutdown and exit\n\n";


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
	printf(" audio engine sample rate: %" PRIu32 "\n", sample_rate);
	printf(" samples per DUV bit: %d\n", samples_per_duv_bit);
	int rate = sample_rate/DECIMATION_RATE;
	printf(" decimation factor: %d with audio loop sample rate %d\n",DECIMATION_RATE, rate);
	printf(" test tone freq %d Hz\n",(int)test_tone_freq);
	print_status("High Pass Filter", hpf);
	print_status("Bit Low Pass Filter", lpf_duv_bits);
	print_status("DUV Telemetry", send_duv_telem);
	print_status("Test Telem", send_test_telem);

	print_status("Generate test tone", send_test_tone);
	print_status("Measure input test tone", measure_test_tone);
	print_status("Verbose Output", verbose);
}

/*
 * Start an interactive command console that allows the user to issue commands to the
 * telem_radio platform.  This should only be attached for lab testing.
 *
 */
int cmd_console() {

	int rc = init();
	if (rc != 0) {
		error_print("Initialization error\n");
		return rc;
	}

	printf("Type (q)uit to exit, or (h)help..\n\n");
	size_t buffer_size = 32;
	char *line;
	line = (char *)malloc(buffer_size * sizeof(char));
	if( line == NULL)
	{
		perror("Unable to allocate line buffer");
		exit(1);
	}
	int running = 1;
	while (running) {
		int rc = getline(&line, &buffer_size, stdin);
		if (rc > 1) {
			char * token;
			line[strcspn(line, "\n")] = '\0';
			token = strsep(&line, " ");

			if (strcmp(token, "filter") == 0|| strcmp(token, "f") == 0) {
				hpf = !hpf;
				print_status("High Pass Filter", hpf);
			} else if (strcmp(token, "low") == 0 || strcmp(token, "l") == 0) {
				lpf_duv_bits = !lpf_duv_bits;
				print_status("Bit Low Pass Filter", lpf_duv_bits);
			} else if (strcmp(token, "telem") == 0 || strcmp(token, "t") == 0) {
				send_duv_telem = !send_duv_telem;
				if (send_duv_telem == false) { // reset the modulator ready for next time
					samples_sent_for_current_bit = 0;
				}
				print_status("Telemetry", send_duv_telem);
			} else if (strcmp(token, "test") == 0) {
				send_test_telem = !send_test_telem;
				print_status("Test Telem", send_test_telem);
			} else if (strcmp(token, "verbose") == 0|| strcmp(token, "v") == 0) {
				verbose = !verbose;
				print_status("Verbose Output", verbose);
			} else if (strcmp(token, "measure") == 0) {
				measure_test_tone = !measure_test_tone;
				print_status("Measure test tone", measure_test_tone);
			} else if (strcmp(token, "freq") == 0) {
				token = strsep(&line, " ");
				float freq = atof(token);
				if (freq == 0.0)
					printf("Invalid frequency: %s\n", token);
				else {
					test_tone_freq = freq;
					printf("Test tone frequency now: %d Hz\n", (int)test_tone_freq);
				}
			} else if (strcmp(token, "tone") == 0) {
				send_test_tone = !send_test_tone;
				print_status("Send Test Tone", send_test_tone);
				if (send_test_tone) {
					rc = gen_cos_table(osc_sin_table, OSC_TABLE_SIZE);
					if (rc != 0)
						printf("Error generating sin table\n");
				}

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

	return 0;
}

/******************************************************************************
 *
 * TEST FUNCTIONS
 *
 ******************************************************************************/

unsigned char test_parities_check[] = {0x19,0xa0,0x2c,0x20,0x59,0xf6,0x7c,0x12,0x84,0x27,0x77,0x98,0xb5,0xf3,0x89,0xf1,
		0xa4,0x84,0xba,0x50,0x3a,0x0f,0x16,0x01,0x62,0x1c,0xcd,0x9a,0x11,0x1a,0xf2,0xa7};


int test_modulate_bit() {
	printf("TESTING modulate_bit .. ");
	verbose_print("\n");
	int lpf = lpf_duv_bits;
	lpf_duv_bits = false; // turn this off just for the test
	int fail = 0;
	int expected_result1[] = {
			0,0,1,1,1,1,1,0,1,0,  // the sync word 0xfa
			1,0,0,0,1,1,0,1,0,1,
			0,1,1,1,0,1,0,1,0,0,
			1,0,0,1,1,1,0,1,0,1,
			0,0,1,1,0,0,0,1,1,0};

	int expected_result2[] = {
			1,1,0,0,0,0,0,1,0,1,  // the sync word 0xfa
			1,0,0,0,1,1,0,1,0,1,
			0,1,1,1,0,1,0,1,0,0,
			1,0,0,1,1,1,0,1,0,1,
			0,0,1,1,0,0,0,1,1,0};

	/* reset the state of the modulator */
	init_telemetry_processor();
	samples_sent_for_current_bit = 0;
	current_bit = 0;
	init_rd_state();

	sample_rate = 48000;
	samples_per_duv_bit = sample_rate / DECIMATION_RATE / DUV_BPS;

	unsigned char *test_packet = set_test_packet();

	int j=0;

	// Start of transmission test
	//telem_packet = (duv_packet_t*)calloc(sizeof(duv_packet_t),1); // allocate 64 bytes for the packet data which is used as the second packet

	/* Run for whole first word, the sync word and check that the first and last sample of each bit is correct
	 * This is 600 bits */
	for (int i=0; i < 600; i++) {
		double bit_audio_value = modulate_bit();
		if ((i) % samples_per_duv_bit == 0) {
			// first sample of bit
			verbose_print ("%.3f ",bit_audio_value);
			double test_value = expected_result1[j] ? ONE_VALUE : ZERO_VALUE;
			if (test_value != bit_audio_value) {
				verbose_print (" **start err ");
				fail = 1;
			}
		}
		if ((i+1) % samples_per_duv_bit == 0) {
			// last sample of bit
			//printf ("%.3f ",i, bit_audio_value);
			double test_value = expected_result1[j++] ? ONE_VALUE : ZERO_VALUE;
			if (test_value != bit_audio_value) {
				verbose_print (" **end err, got '%.3f' ",bit_audio_value);
				fail = 1;
			}
		}

	}
	verbose_print("\n");

	uint16_t decode = 0;
	int b = 9; // bit position in word
	int check_expected_results = false;
	int bit = 0;
	j = 0;
	// test end of packet and start of next
	for (int w=0; w < DUV_PACKET_LENGTH+1; w++) { // +1 so that we check sync word of packet
		if (w == DUV_PACKET_LENGTH) {
			verbose_print("end of packet\n\n");
			check_expected_results = true; // now we check the first 40 bits of the second packet, inc sync word
		}
		verbose_print ("w:%d ",w);
		for (int i=0; i < 600; i++) { // 600 samples is a whole word
			double bit_audio_value = modulate_bit();

			if ((i) % samples_per_duv_bit == 0) {
				verbose_print ("%.3f ",bit_audio_value);
				if (check_expected_results) {
					double test_value = expected_result2[j++] ? ONE_VALUE : ZERO_VALUE;
					if (test_value != bit_audio_value) {
						verbose_print (" **start err ");
						fail = 1;
					}

				}
				bit = 0;
				if (bit_audio_value > 0)
					bit = 1;
				decode += (bit << b--);
			}
		}
		unsigned char c = reverse_8b10b_lookup(decode);
		verbose_print(" 10b: %x 8b: %x",decode, c);
		if (w < DUV_DATA_LENGTH) {
			if (c != test_packet[w]) {
				verbose_print(" fail \n");
				fail = 1;
			} else {
				verbose_print(" pass \n");
			}
		} else if (w < DUV_PACKET_LENGTH) {
			if (c != test_parities_check[w-DUV_DATA_LENGTH]) {
				verbose_print(" fail \n");
				fail = 1;
			} else {
				verbose_print(" pass \n");
			}
		} else {
			verbose_print("\n"); // these are checked bit by bit above
		}
		decode = 0;
		b = 9;
	}
	lpf_duv_bits = lpf; // reset this after the test

	if (fail == 0) {
		printf(" Pass\n");
	} else {
		printf(" Fail\n");
	}
	return fail;

}
