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
#include <math.h>
#include <time.h>

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
#include "dc_filter.h"
#include "telem_thread.h"

/* Forward function declarations */
double modulate_bit();
jack_default_audio_sample_t * duv_audio_loop(jack_default_audio_sample_t *in,
		jack_default_audio_sample_t *out, jack_nframes_t nframes);
int init_filters(int bit_rate, int decimation_rate);

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

double decimate_filter_coeffs[DECIMATE_FILTER_LEN];
double decimate_filter_xv[DECIMATE_FILTER_LEN];

double interpolate_filter_coeffs[DECIMATE_FILTER_LEN];
double interpolate_filter_xv[DECIMATE_FILTER_LEN];

#define BIT_FILTER_LEN 180 // 60 is one bit.  Filter across 3 bits seems to be a good trade off
double bit_filter_coeffs[BIT_FILTER_LEN];
double bit_filter_xv[BIT_FILTER_LEN];

double filtered_audio_buffer[PERIOD_SIZE]; // the audio samples after they are filtered by the decimation filter
double decimated_audio_buffer[PERIOD_SIZE/4]; // the audio samples after decimation and decimation filter
double hpf_decimated_audio_buffer[PERIOD_SIZE/4]; // the decimated audio samples after high pass filtering
double interpolated_audio_buffer[PERIOD_SIZE]; // the audio samples after interpolation back to 48000 but before interpolation filter

TIIRCoeff Elliptic8Pole300HzHighPassIIRCoeff;
TIIRCoeff Elliptic4Pole300HzHighPassIIRCoeff;
TIIRStorage iir_hpf_store; // storage for the IIR High Pass filter

/* Audio processor variables */
int decimation_rate;

/* Telemetry modulator settings */
int samples_per_bit = 0; // this is calculated in the code.  For example it is 12000/200 = 60
int samples_sent_for_current_bit = 0; // how many samples have we sent for the current bit
int current_bit = 0; // the value of the current bit we are sending
int starting_bit_modulator = true;

/* User settings changeable from cmd console */
int hpf = true; // filter the transponder audio
int send_telem = true;
int send_high_speed_telem = false;
int send_test_telem = false; // send a 10101 test telem sequence
int send_test_tone = false; // output a steady tone for measurement of a sound card
int measure_test_tone = false; // display the peak ampltude of a received tone to measure the sound card
int lpf_bits = true;  // filter the telem bits

/* Setup the test bit pattern.  Send this many bits in a row. */
int TEST_BIT_NUMBER = 5;
int test_bits_sent = 0;

int zero_bits_in_a_row = 0;
int one_bits_in_a_row = 0;
int clipping_reported = 0;

/* Audio loop timing variables */
struct timespec ts_start, ts_end;
double loop_time_microsec = 0.0;
double max_loop_time_microsec = 0.0;
double min_loop_time_microsec = 999999;
int loops_timed = 0;
double total_loop_time_microsec = 0.0;
#define LOOPS_TO_TIME 400.0  // Each loop is circa 10ms.  Time about once per telem frame

double get_loop_time_microsec() { return loop_time_microsec; }
double get_max_loop_time_microsec() { return max_loop_time_microsec; }
double get_min_loop_time_microsec() { return min_loop_time_microsec; }

int get_decimation_rate() { return decimation_rate; }
int get_samples_per_bit() { return samples_per_bit; }
double get_test_tone_freq() { return test_tone_freq; }
int get_hpf() { return hpf; }
int get_lpf_bits() { return lpf_bits; }
int get_send_telem() { return send_telem; }
int get_send_high_speed_telem() { return send_high_speed_telem; }
int get_send_test_telem() { return send_test_telem; }
int get_send_test_tone() { return send_test_tone; }
int get_measure_test_tone() { return measure_test_tone; }

void set_samples_per_bit(int val) { samples_per_bit = val; }
void set_test_tone_freq(double val) { test_tone_freq = val; }
void set_hpf(int val) { hpf = val; }
void set_lpf_bits(int val) { lpf_bits = val; }
void set_send_telem(int val) { send_telem = val; }
void set_send_high_speed_telem(int val) { send_high_speed_telem = val; }
void set_send_test_telem(int val) { send_test_telem = val; }
void set_send_test_tone(int val) { send_test_tone = val; }
void set_measure_test_tone(int val) { measure_test_tone = val; }

/*
 * This initializes the audio processor and should be called when it is first started
 */
int init_audio_processor(int bit_rate, int dec_rate) {
	// Init
	int rc;
	decimation_rate = dec_rate;

	rc = init_bit_modulator(bit_rate, decimation_rate);
	if (rc != 0) {
		error_print("Error initializing bit modulator\n");
		return rc;
	}

	/* now we know the sample rate then setup things that are dependent on that */
	rc = init_filters(bit_rate, decimation_rate);
	if (rc != 0) {
		error_print("Error initializing filters\n");
		return rc;
	}

	/* Initialize a sine table in the oscillator */
	rc = gen_cos_table(osc_sin_table, OSC_TABLE_SIZE);
	if (rc != 0)
		printf("Error generating sin table\n");

	return 0;
}

/*
 * This is called at startup to populate the coefficients for digital filters
 */
int init_filters(int bit_rate, int decimation_rate) {
	verbose_print("Generating filters ..\n");

	/* Decimation filter */
	int decimation_cutoff_freq = g_sample_rate / (2* decimation_rate);
	int rc = gen_raised_cosine_coeffs(decimate_filter_coeffs, g_sample_rate, decimation_cutoff_freq, 0.5f, DECIMATE_FILTER_LEN);
	for (int i=0; i< DECIMATE_FILTER_LEN; i++) decimate_filter_xv[i] = 0;

	if (rc != 0)
		return rc;

	iir_hpf_store.MaxRegVal = 1.0E-12;
	for(int i=0; i<ARRAY_DIM; i++) {
		iir_hpf_store.RegX1[i] = 0.0;
		iir_hpf_store.RegX2[i] = 0.0;
		iir_hpf_store.RegY1[i] = 0.0;
		iir_hpf_store.RegY2[i] = 0.0;
	}

	/*
	 * Note that the IIR filters are tied to a decimated sample rate of 12kHz and need to be redefined if the
	 * decimation rate is different
	 */

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
	int interpolation_cutoff_freq = g_sample_rate / (2* decimation_rate);
	for (int i=0; i< DECIMATE_FILTER_LEN; i++) interpolate_filter_xv[i] = 0;
	rc = gen_raised_cosine_coeffs(interpolate_filter_coeffs, g_sample_rate, interpolation_cutoff_freq, 0.5f, DECIMATE_FILTER_LEN);
	if (rc != 0)
		return rc;

	/* Bit shape filter */
	for (int i=0; i< BIT_FILTER_LEN; i++) bit_filter_xv[i] = 0;
	// TODO HIGH SPEED
//////	rc = gen_raised_cosine_coeffs(bit_filter_coeffs, g_sample_rate, bit_rate, 0.5f, BIT_FILTER_LEN);
	rc = gen_raised_cosine_coeffs(bit_filter_coeffs, g_sample_rate/decimation_rate, bit_rate, 0.5f, BIT_FILTER_LEN);

	return rc;
}
/*
 * Turn the bit stream into samples that can be fed into the audio loop
 */
double modulate_bit() {
	if (starting_bit_modulator ||  // starting a new packet
			(samples_sent_for_current_bit >= samples_per_bit )) { // We are starting a new bit
		samples_sent_for_current_bit = 0;
		starting_bit_modulator = false;
		if (send_test_telem) {
			test_bits_sent++;
			if (test_bits_sent == TEST_BIT_NUMBER) {
				current_bit = !current_bit; // TESTING - just toggle the bit to generate tone
				test_bits_sent = 0;
			}
		} else
			current_bit = get_next_bit();

		if (current_bit) {
			one_bits_in_a_row++;
			zero_bits_in_a_row = 0;
		} else {
			one_bits_in_a_row = 0;
			zero_bits_in_a_row++;
		}
	}
	samples_sent_for_current_bit++;
	double bit_audio_value = current_bit ? g_one_value : g_zero_value;
	if (!send_high_speed_telem && g_ramp_bits_to_compensate_hpf) {
		if (one_bits_in_a_row) bit_audio_value = bit_audio_value + (one_bits_in_a_row-1) * g_ramp_amount;
		if (zero_bits_in_a_row) bit_audio_value = bit_audio_value - (zero_bits_in_a_row-1) * g_ramp_amount;
	}

	if (lpf_bits)
		bit_audio_value = fir_filter(bit_audio_value, bit_filter_coeffs, bit_filter_xv, BIT_FILTER_LEN);

	return bit_audio_value;
}

int init_bit_modulator(int bit_rate, int decimation_rate) {
	starting_bit_modulator = true;
	current_bit = 0;
	samples_sent_for_current_bit = 0;
	samples_per_bit = g_sample_rate / decimation_rate / bit_rate;
	return EXIT_SUCCESS;
}

jack_default_audio_sample_t * telem_only_audio_loop(jack_default_audio_sample_t *in,
		jack_default_audio_sample_t *out, jack_nframes_t nframes) {


	for (int i = 0; i< nframes; i++) {
		if (send_telem) {
			float bit_audio_value = modulate_bit();
			out[i] = bit_audio_value; // add the telemetry
		} else {
			out[i] = 0.0;
		}
	}
	return out;
}

jack_default_audio_sample_t * high_speed_telem_audio_loop(jack_default_audio_sample_t *in,
		jack_default_audio_sample_t *out, jack_nframes_t nframes) {


	if (send_telem) {
		for (int i = 0; i< nframes; i++) {
			float bit_audio_value = modulate_bit();
			out[i] = bit_audio_value; // add the telemetry
			if (!clipping_reported)
				if (out[i] > 1.0) {
					error_print("Audio is clipping! %f",out[i]);
					clipping_reported = 1;
				}
		}
	}

	return out;
}

/**
 * Prototype audio loop
 * This has too many loops within the loops, which helps with debugging, but could be optimized
 * Currently this uses FIR decimation filters which could be replaced with a more efficient alternative
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
		if (decimate_count == decimation_rate) {
			decimate_count = 0;
			decimated_audio_buffer[i/decimation_rate] = filtered_audio_buffer[i];
		}
	}

	/**
	 * Now we high pass filter
	 */
	if (hpf) {
	//	iir_filter_array(Elliptic8Pole300HzHighPassIIRCoeff, decimated_audio_buffer, hpf_decimated_audio_buffer, nframes/DECIMATION_RATE);
		for (int i = 0; i< nframes/decimation_rate; i++)
			hpf_decimated_audio_buffer[i] = iir_filter(Elliptic8Pole300HzHighPassIIRCoeff,decimated_audio_buffer[i], &iir_hpf_store);
	//		hpf_decimated_audio_buffer[i] = cheby_iir_filter(decimated_audio_buffer[i], a_hpf_025, b_hpf_025);
	} else {
		for (int i = 0; i< nframes/decimation_rate; i++)
			hpf_decimated_audio_buffer[i] = decimated_audio_buffer[i];
	}

	/**
	 * Insert DUV telemetry.
	 */
	if (send_telem) {
		for (int i = 0; i< nframes/decimation_rate; i++) {
			float bit_audio_value = modulate_bit();
			hpf_decimated_audio_buffer[i] += bit_audio_value; // add the telemetry
		}
	}

	/**
	 * We interpolate by adding samples with zero between each decimated sample.  This creates the same signal
	 * at 48k with duplications of the spectrum every 9600Hz.  So we need to filter out those duplicates
	 * from the final signal.  We apply gain equal to DECIMATION_RATE to compensate for the loss of signal
	 * from the inserted samples.
	 */
	float gain = (float)decimation_rate;
	for (int i = 0; i < nframes; i++) {
		decimate_count++;
		if (decimate_count == decimation_rate) {
			decimate_count = 0;
			interpolated_audio_buffer[i] = gain * hpf_decimated_audio_buffer[i/decimation_rate];
		} else
			interpolated_audio_buffer[i] = 0.0f;

	}
	/* Now filter out the duplications of the spectrum that interpolation introduces */
	for (int i = 0; i< nframes; i++) {
		out[i] = (float)fir_filter(interpolated_audio_buffer[i], interpolate_filter_coeffs, interpolate_filter_xv, DECIMATE_FILTER_LEN);
		if (!clipping_reported)
			if (out[i] > 1.0) {
				error_print("Audio is clipping! %f",out[i]);
				clipping_reported = 1;
			}
	}

	return out;
}

jack_default_audio_sample_t * audio_loop(jack_default_audio_sample_t *in, jack_default_audio_sample_t *out, jack_nframes_t nframes) {
	/* Time the loop. Use clock_gettime because gettimeofday() is moved by NTP or other time sync mechanisms */
	clock_gettime(CLOCK_MONOTONIC, &ts_start);

	if (send_test_tone) {
		for (int i=0; i < nframes; i++) {
			double value = nextSample(&osc_phase, test_tone_freq, g_sample_rate, osc_sin_table, OSC_TABLE_SIZE);
			if (value > 0) out[i] = g_one_value;
			else out[i] = g_zero_value;
		}
	} else if (measure_test_tone) {
		for (int i=0; i < nframes; i++) {
			//out[i] = 0; // silence the output, except this does not work if we were already receiving a tone?
			out[i] = in[i];  // play what we are measuring
			//printf("%f\n", in[i]);

			if (in[i] > peak_value)
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
	} else if (send_high_speed_telem) {
		high_speed_telem_audio_loop(in, out, nframes);
		clipping_reported = 0;
	} else {
		/*
		 * Now process the data in out buffer before we sent it to the radio
		 */
		//telem_only_audio_loop(in, out, nframes);
		duv_audio_loop(in, out, nframes);
		clipping_reported = 0;
	}

	clock_gettime(CLOCK_MONOTONIC, &ts_end);
	/* store the CPU time in microseconds */
	loop_time_microsec = ((ts_end.tv_sec * 1000000 + ts_end.tv_nsec/1000) -
			(ts_start.tv_sec * 1000000 + ts_start.tv_nsec/1000));

	if (loop_time_microsec > max_loop_time_microsec)
		max_loop_time_microsec = loop_time_microsec;
	if (loop_time_microsec < min_loop_time_microsec)
		min_loop_time_microsec = loop_time_microsec;
	loops_timed++;
	total_loop_time_microsec += loop_time_microsec;
	if (loops_timed > LOOPS_TO_TIME) {
		//verbose_print("INFO: Audio loop processing time: %f secs\n",total_cpu_time_used/loops_timed);
		if (max_loop_time_microsec > 10000) // // 480 frames is 10ms of audio.  So if we take more than 10ms to process this we have an issue
			error_print("WARNING: Loop ran for: %.2f ms\n",max_loop_time_microsec/1000);
		verbose_print("Loop time: Max %.2fms Min: %.2fms\n",max_loop_time_microsec/1000,min_loop_time_microsec/1000);
		total_loop_time_microsec = 0;
		max_loop_time_microsec = 0;
		min_loop_time_microsec = 99999;
		loops_timed = 0;
	}

	return out;
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
	int lpf = lpf_bits; // store this value to reset after the test
	int ramp = g_ramp_bits_to_compensate_hpf; // store this value to reset after the test
	lpf_bits = false; // turn this off just for the test
	g_ramp_bits_to_compensate_hpf = false;// turn this off just for the test

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
	init_telemetry_processor(DUV_PACKET_LENGTH);

//	samples_sent_for_current_bit = 0;
//	current_bit = 0;

	g_sample_rate = 48000;
	init_audio_processor(DUV_BPS, DUV_DECIMATION_RATE);
//	samples_per_duv_bit = g_sample_rate / DECIMATION_RATE / DUV_BPS;

	unsigned char *test_packet = set_test_packet();

	int j=0;

	// Start of transmission test
	//telem_packet = (duv_packet_t*)calloc(sizeof(duv_packet_t),1); // allocate 64 bytes for the packet data which is used as the second packet

	/* Run for whole first word, the sync word and check that the first and last sample of each bit is correct
	 * This is 10 bits */
	for (int i=0; i < 10*samples_per_bit; i++) {
		double bit_audio_value = modulate_bit();
		if ((i) % samples_per_bit == 0) {
			// first sample of bit
			verbose_print ("%.3f ",bit_audio_value);
			double test_value = expected_result1[j] ? g_one_value : g_zero_value;
			if (test_value != bit_audio_value) {
				verbose_print (" **start err ");
				fail = 1;
			}
		}
		if ((i+1) % samples_per_bit == 0) {
			// last sample of bit
			//printf ("%.3f ",i, bit_audio_value);
			double test_value = expected_result1[j++] ? g_one_value : g_zero_value;
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
		for (int i=0; i < 10*samples_per_bit; i++) { // 600 samples is a whole word
			double bit_audio_value = modulate_bit();

			if ((i) % samples_per_bit == 0) {
				verbose_print ("%.3f ",bit_audio_value);
				if (check_expected_results) {
					double test_value = expected_result2[j++] ? g_one_value : g_zero_value;
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
	lpf_bits = lpf; // reset this after the test
	g_ramp_bits_to_compensate_hpf = ramp; // reset after the test
	if (fail == 0) {
		printf(" Pass\n");
	} else {
		printf(" Fail\n");
	}
	return fail;

}
