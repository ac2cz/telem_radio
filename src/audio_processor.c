/*
 * telem_modulator.c
 *
 *  Created on: Feb 21, 2022
 *      Author: g0kla
 * This is based on the simple client demo for jackd
 */



#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>

#include <jack/jack.h>

#include "../inc/config.h"
#include "../inc/audio_processor.h"
#include "../inc/audio_tools.h"
#include "../inc/fir_filter.h"
#include "../inc/iir_filter.h"
#include "../inc/telem_processor.h"
#include "../inc/oscillator.h"
//#include "../inc/TelemEncoding.h"

jack_port_t *input_port;
jack_port_t *output_port;
jack_client_t *client;

int sample_rate = 0;
#define DECIMATE_FILTER_LEN 480
#define DECIMATION_RATE 4
float decimate_filter_coeffs[DECIMATE_FILTER_LEN];
float decimate_filter_xv[DECIMATE_FILTER_LEN];

#define FIR_HIGHPASS_FILTER_LEN 480  // this is an experimental alternative to IIR, but does not worK!
float fir_highpass_filter_coeffs[DECIMATE_FILTER_LEN];
float fir_highpass_filter_xv[DECIMATE_FILTER_LEN];

float interpolate_filter_coeffs[DECIMATE_FILTER_LEN];
float interpolate_filter_xv[DECIMATE_FILTER_LEN];

#define DUV_BIT_FILTER_LEN 96  // this length seems to be critical 48 is too short.  128 does not work at all.
float duv_bit_filter_coeffs[DUV_BIT_FILTER_LEN];
float duv_bit_filter_xv[DUV_BIT_FILTER_LEN];

float filtered_audio_buffer[PERIOD_SIZE];
float decimated_audio_buffer[PERIOD_SIZE/4]; // the audio samples after decimation to 9600
float interpolated_audio_buffer[PERIOD_SIZE]; // the audio samples after interpolation back to 48000


/* These are test filters from a lookup table.  We should design and implement optimal filters for final use */
// 4 pole cheb hpf at fc = 0.025 = 1200Kz at 48k or 300Hz at 12000 samples per sec or 240Hz at 9600  Ch 20 Eng and Sci guide to DSP
float a_hpf_025[] = {7.941874E-01, -3.176750E+00, 4.765125E+00, -3.176750E+00, 7.941874E-01};
float b_hpf_025[] = {1, 3.538919E+00, -4.722213E+00,  2.814036E+00,  -6.318300E-01};

// 4 pole cheb lpf at fc = 0.025 = 1200Kz at 48k or 300Hz at 12000 samples per sec 240Hz at 9600  Ch 20 Eng and Sci guide to DSP
float a_lpf_025[] = {1.504626E-05, 6.018503E-05, 9.027754E-05, 6.018503E-05, 1.504626E-05};
float b_lpf_025[] = {1, 3.725385E+00, -5.226004E+00,  3.270902E+00,  -7.705239E-01};

/* Telemetry modulator settings */
int samples_per_duv_bit = 0; // this is calculated in the code.  For example it is 12000/200 = 60
int samples_sent_for_current_bit = 0; // how many samples have we sent for the current bit
int bits_sent_for_current_word = 0; // how many bits have we sent for the current 10b word
int words_sent_for_current_packet; // how many 10b words we have sent for the current packet
int current_bit = 0; // the value of the current bit we are sending

int decimate = true;
int hpf = true; // filter the transponder audio
int send_duv_telem = false;
int send_test_telem = false; // send a 10101 test telem sequence
int send_test_tone = false; // output a steady tone for measurement of a sound card
int measure_test_tone = false; // display the peak ampltude of a received tone to measure the sound card
int lpf = true;  // filter the DUV telem bits

/* Test Type 1 packet with id 1.  This will look like Fox-1A in FoxTelem */
unsigned char test_packet[] = {0x51,0x01,0x40,0xd8,0x00,
		0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x10,
		0x00,0x01,0x20,0x00,0x01,0x10,0x00,0x47,0x8f,0xf4,0x47,0x7f,0xf4,0x48,0x7f,0xf4,0x10,0x08,0x00,0x65,
		0x47,0x76,0xff,0xe7,0x33,0xce,0xe2,0x81,0x29,0x78,0x80,0xf2,0x8e,0x01,0x04,0x01,0x01,0x01,0x17,0x38,
		0xac,0x00,0x00,0x20};

/* Test tone paramaters */
#define OSC_TABLE_SIZE 9600
float osc_phase = 0;
float test_tone_freq = 5000.0f;
float osc_sin_table[OSC_TABLE_SIZE];

//int position_in_packet = 0;
int first_packet_to_be_sent = true; // flag that tells us if a sync word is needed at the start of the packet

uint16_t test_encoded_packet[DUV_PACKET_LENGTH+1]; // includes space for SYNC WORD at the end.

int get_next_bit() {
	int current_bit = -1; // the first bit is set to be the sync word

	if (bits_sent_for_current_word >= BITS_PER_10b_WORD) { // We are starting a new 10b word
		bits_sent_for_current_word = 0;
		if (first_packet_to_be_sent) {
			first_packet_to_be_sent = false; // we have sent at least one word so this is no longer the start of a transmission
		} else {
			// we sent a word from this packet so increment the counter
			words_sent_for_current_packet++;
		}
		if (words_sent_for_current_packet >= DUV_PACKET_LENGTH+1) { // We are ready for a new packet.  We have the sync word in the final word
			words_sent_for_current_packet = 0;

			// if we do nothing here then we keep sending the test packet in a loop

			// this turns off telemetry so we only send 1 frame
		//	send_duv_telem = false;
		//	first_packet_to_be_sent = true;
		}

	}
	// so get the value
	/* If we are starting to transmit then send the sync word first */
	uint16_t current_word = 0xfa;
	if (!first_packet_to_be_sent) {
		current_word = test_encoded_packet[words_sent_for_current_packet];
	}
	// we send most significant bit first of the 10 bit word
	int shift_amt = 9 - bits_sent_for_current_word;
	current_bit = ((current_word & 0x3ff) >> shift_amt)  & 0x01;

	bits_sent_for_current_word++;
	return current_bit;
}

double modulate_bit() {

	if ((samples_sent_for_current_bit == 0 && bits_sent_for_current_word == 0 && words_sent_for_current_packet == 0 )||  // starting a new packet
			(samples_sent_for_current_bit >= samples_per_duv_bit )) { // We are starting a new bit
		samples_sent_for_current_bit = 0;
		if (send_test_telem)
			current_bit = !current_bit; // TESTING - just toggle the bit to generate 200Hz tone
		else
			current_bit = get_next_bit();
	}
	samples_sent_for_current_bit++;
	double bit_audio_value = current_bit ? ONE_VALUE : ZERO_VALUE;
	//	int bit_audio_value = current_bit ? 1 : 0;

		if (lpf)
			bit_audio_value = fir_filter(bit_audio_value, duv_bit_filter_coeffs, duv_bit_filter_xv, DUV_BIT_FILTER_LEN);
	return bit_audio_value;
}

/**
 * Prototype audio loop
 * This has too many loops within the loops and will be optimized
 * Currently this uses FIR decimation filters which will be replaced with a more efficient alternative
 *
 */
jack_default_audio_sample_t * audio_loop(jack_default_audio_sample_t *in,
		jack_default_audio_sample_t *out, jack_nframes_t nframes) {

	//	memcpy (out, in, sizeof (jack_default_audio_sample_t) * nframes);

	for (int i = 0; i< nframes; i++) {
		if (decimate) {
			filtered_audio_buffer[i] = fir_filter(in[i], decimate_filter_coeffs, decimate_filter_xv, DECIMATE_FILTER_LEN);
		} else {
			filtered_audio_buffer[i] = in[i];
		}
	}

	int decimate_count = 0;

	if (decimate) {
		for (int i = 0; i< nframes; i++) {
			decimate_count++;
			if (decimate_count == DECIMATION_RATE) {
				decimate_count = 0;
				decimated_audio_buffer[i/DECIMATION_RATE] = filtered_audio_buffer[i];
			}
		}
	}

	/**
	 * Now we high pass filter
	 */
	if (hpf) {
		for (int i = 0; i< nframes/DECIMATION_RATE; i++)
			decimated_audio_buffer[i] = iir_filter(decimated_audio_buffer[i], a_hpf_025, b_hpf_025);
			//decimated_audio_buffer[i] = fir_filter(decimated_audio_buffer[i], fir_highpass_filter_coeffs, fir_highpass_filter_xv, FIR_HIGHPASS_FILTER_LEN);
	}

	/**
	 * Insert DUV telemetry.
	 */
	if (send_duv_telem) {
		for (int i = 0; i< nframes/DECIMATION_RATE; i++) {
			float bit_audio_value = modulate_bit();
			decimated_audio_buffer[i] += bit_audio_value; // add the telemetry
		}
	}

	/**
	 * We interpolate by adding 3 samples with zero between each sample.  This creates the same signal
	 * at 48k with duplications of the spectrum every 9600Hz.  So we need to filter out those duplicates
	 * from the final signal.  We apply gain equal to DECIMATION_RATE to compensate for the loss of signal
	 * from the inserted samples.
	 */
	if (decimate) {
		float gain = (float)DECIMATION_RATE;
		for (int i = 0; i < nframes; i++) {
			decimate_count++;
			if (decimate_count == DECIMATION_RATE) {
				decimate_count = 0;
				interpolated_audio_buffer[i] = gain * decimated_audio_buffer[i/DECIMATION_RATE];
			} else
				interpolated_audio_buffer[i] = 0.0f;

		}
		/* Now filter out the duplications of the spectrum that interpolation introduces */
		for (int i = 0; i< nframes; i++) {
			out[i] = fir_filter(interpolated_audio_buffer[i], interpolate_filter_coeffs, interpolate_filter_xv, DECIMATE_FILTER_LEN);
		}
	} else {
		for (int i = 0; i< nframes; i++) {
			out[i] = filtered_audio_buffer[i];
		}
	}

	return out;
}

int measurement_loops = 0;
#define LOOPS_PER_MEASUREMENT 500 // so measure once per 5 sec
int measurements = 0;
float peak_value = 0.0f;
/**
 * The process callback for this JACK application is called in a
 * special realtime thread once for each audio cycle.
 *
 * Each buffer contains 480 ALSA frames, specified when jackd was started
 *
 */
int process (jack_nframes_t nframes, void *arg) {
	assert(nframes == PERIOD_SIZE);
	jack_default_audio_sample_t *in, *out;

	in = jack_port_get_buffer (input_port, nframes);
	out = jack_port_get_buffer (output_port, nframes);

	if (send_test_tone) {
		for (int i=0; i < nframes; i++) {
			float value = nextSample(&osc_phase, test_tone_freq, sample_rate, osc_sin_table, OSC_TABLE_SIZE);
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
		audio_loop(in, out, nframes);
	}

	return 0;
}

/**
 * JACK calls this shutdown_callback if the server ever shuts down or
 * decides to disconnect the client.
 */
void jack_shutdown (void *arg) {
	exit (1);
}

int  init_filters() {
	printf("Generating filters ..\n");

	/* Decimation filter */
	int decimation_cutoff_freq = sample_rate / (2* DECIMATION_RATE);
	int rc = gen_raised_cosine_coeffs(decimate_filter_coeffs, sample_rate, decimation_cutoff_freq, 0.5f, DECIMATE_FILTER_LEN);
	if (rc != 0)
		return rc;

	/* FOR TESTING - DOES NOT WORK - FIR High Pass filter */
	int fir_highpass_cutoff_freq = 300;
	rc = gen_raised_cosine_coeffs(fir_highpass_filter_coeffs, sample_rate/DECIMATION_RATE, fir_highpass_cutoff_freq, 0.5f, FIR_HIGHPASS_FILTER_LEN);
	if (rc != 0)
		return rc;

	/* Interpolation filter */
	int interpolation_cutoff_freq = sample_rate / (2* DECIMATION_RATE);
	rc = gen_raised_cosine_coeffs(interpolate_filter_coeffs, sample_rate, interpolation_cutoff_freq, 0.5f, DECIMATE_FILTER_LEN);
	if (rc != 0)
		return rc;

	/* Bit shape filter - SHOULD BE UPDATED TO ROOT RAISED COSINE TO MATCH FOXTELEM */
	int duv_bit_cutoff_freq = 200;
	rc = gen_raised_cosine_coeffs(duv_bit_filter_coeffs, sample_rate/DECIMATION_RATE, duv_bit_cutoff_freq, 0.5f, DUV_BIT_FILTER_LEN);

	return rc;
}

char *help_str =
		"TELEM Radio Platform Console Commands:\n"
		" (s)tatus   - display settings and status\n"
		" (f)ilter   - Toggle high pass filter on/off\n"
		" (l)ow pass filter   - Toggle bit low high pass filter on/off\n"
		" (d)ecimate - Toggle decimation on/off\n"
		" (t)elem    - Toggle DUV telemetry on/off\n"
		" test       - DUV telem contains only 101010 on/off\n"
		" tone       - Generate test tone\n"
		" freq <Hz>  - Set freq of test tone\n"
		" measure    - Display measurement for input tone\n"
		" (h)help    - show this help\n"
		" (q)uit     - Shutdown and exit\n\n";



void print_status(char *name, int status) {
	char *val = status ? " ON " : " OFF";
	printf(" %s : %s\n",val, name);
}

void get_status() {
	printf("TELEM Radio status:\n");
	printf(" audio engine sample rate: %" PRIu32 "\n", sample_rate);
	printf(" samples per DUV bit: %d\n", samples_per_duv_bit);
	int rate = sample_rate/DECIMATION_RATE;
	printf(" decimation factor: %d with audio loop sample rate %d\n",DECIMATION_RATE, rate);
	print_status("Decimate", decimate);
	print_status("High Pass Filter", hpf);
	print_status("Bit Low Pass Filter", lpf);
	print_status("DUV Telemetry", send_duv_telem);
	print_status("Test Telem", send_test_telem);
	printf("Test tone freq %d Hz\n",(int)test_tone_freq);
	print_status("Generate test tone", send_test_tone);
	print_status("Measure input test tone", measure_test_tone);
}

int cmd_console() {
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
				lpf = !lpf;
				print_status("Bit Low Pass Filter", lpf);
			} else if (strcmp(token, "decimate") == 0 || strcmp(token, "d") == 0) {
				decimate = !decimate;
				print_status("Decimate", decimate);
			} else if (strcmp(token, "telem") == 0 || strcmp(token, "t") == 0) {
				send_duv_telem = !send_duv_telem;
				if (send_duv_telem == false) { // reset the modulator ready for next time
					send_duv_telem = false;
					first_packet_to_be_sent = true;
					samples_sent_for_current_bit = 0;
					bits_sent_for_current_word = 0;
					words_sent_for_current_packet = 0;
					init_rd_state();
				}
				print_status("Telemetry", send_duv_telem);
			} else if (strcmp(token, "test") == 0) {
				send_test_telem = !send_test_telem;
				print_status("Test Telem", send_test_telem);
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
				get_status();
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

int start_audio_processor (void) {
	const char **ports;
	const char *client_name = "simple";
	const char *server_name = NULL;
	jack_options_t options = JackNullOption;
	jack_status_t status;

	/* Encode one test packet */
	encode_duv_telem_packet(test_packet, test_encoded_packet);

	/* open a client connection to the JACK server */
	client = jack_client_open (client_name, options, &status, server_name);
	if (client == NULL) {
		fprintf (stderr, "jack_client_open() failed, "
				"status = 0x%2.0x\n", status);
		if (status & JackServerFailed) {
			fprintf (stderr, "Unable to connect to JACK server\n");
		}
		exit (1);
	}
	if (status & JackServerStarted) {
		fprintf (stderr, "JACK server started\n");
	}
	if (status & JackNameNotUnique) {
		client_name = jack_get_client_name(client);
		fprintf (stderr, "unique name `%s' assigned\n", client_name);
	}

	/* tell the JACK server to call `process()' whenever
	   there is work to be done.
	 */
	jack_set_process_callback (client, process, 0);

	/* tell the JACK server to call `jack_shutdown()' if
	   it ever shuts down, either entirely, or if it
	   just decides to stop calling us.
	 */
	jack_on_shutdown (client, jack_shutdown, 0);

	/* display the current sample rate */
	sample_rate = jack_get_sample_rate (client);
	samples_per_duv_bit = sample_rate / DECIMATION_RATE / DUV_BPS;

	/* now we know the sample rate then setup things that are dependent on that */
	int rc = init_filters();
	if (rc != 0) {
		fprintf(stderr,"Error initializing filters");
		return rc;
	}


	/* create two ports */
	input_port = jack_port_register (client, "input",
			JACK_DEFAULT_AUDIO_TYPE,
			JackPortIsInput, 0);
	output_port = jack_port_register (client, "output",
			JACK_DEFAULT_AUDIO_TYPE,
			JackPortIsOutput, 0);

	if ((input_port == NULL) || (output_port == NULL)) {
		fprintf(stderr, "no more JACK ports available\n");
		exit (1);
	}

	/* Tell the JACK server that we are ready to roll.  Our
	 * process() callback will start running now. */
	if (jack_activate (client)) {
		fprintf (stderr, "cannot activate client");
		exit (1);
	}

	/* Connect the ports.  You can't do this before the client is
	 * activated, because we can't make connections to clients
	 * that aren't running.  Note the confusing (but necessary)
	 * orientation of the driver backend ports: playback ports are
	 * "input" to the backend, and capture ports are "output" from
	 * it.
	 */

	ports = jack_get_ports (client, NULL, NULL,
			JackPortIsPhysical|JackPortIsOutput);
	if (ports == NULL) {
		fprintf(stderr, "no physical capture ports\n");
		exit (1);
	}

	if (jack_connect (client, ports[0], jack_port_name (input_port))) {
		fprintf (stderr, "cannot connect input ports\n");
	}

	free (ports);

	ports = jack_get_ports (client, NULL, NULL,
			JackPortIsPhysical|JackPortIsInput);
	if (ports == NULL) {
		fprintf(stderr, "no physical playback ports\n");
		exit (1);
	}

	if (jack_connect (client, jack_port_name (output_port), ports[0])) {
		fprintf (stderr, "cannot connect output ports\n");
	}

	free (ports);

	/* keep running until stopped by the user */
	rc = cmd_console();

	jack_client_close (client);
	return rc;
}

unsigned char test_parities_check[] = {0x19,0xa0,0x2c,0x20,0x59,0xf6,0x7c,0x12,0x84,0x27,0x77,0x98,0xb5,0xf3,0x89,0xf1,
		0xa4,0x84,0xba,0x50,0x3a,0x0f,0x16,0x01,0x62,0x1c,0xcd,0x9a,0x11,0x1a,0xf2,0xa7};

int test_rs_encoder() {
	int fail = 0;
	printf("TESTING Rs Encoder .. ");
	init_rd_state();
	// Call the RS encoder without 8b10b encoding
	test_telem_encoder(test_packet, test_encoded_packet);

	// Now check the first and last parity bytes
	// First should be 0x19 -> 25
	// Last is 0xa7 -> 167
	//printf("First parity: %i \n",test_encoded_packet[DUV_DATA_LENGTH]);
	//printf("Last Parity: %i \n",test_encoded_packet[DUV_DATA_LENGTH+DUV_PARITIES_LENGTH-1]);
	for (int i=0; i < DUV_PARITIES_LENGTH; i++) {
		if (test_encoded_packet[DUV_DATA_LENGTH+i] != test_parities_check[i]) {
			printf(" failed with parity %d\n", i);
			fail = 1;
		}
	}
	if (fail == 0)
		printf("Pass\n");
	else
		printf("Fail\n");
	return fail;
}

int test_sync_word() {
	// test that the sync word is the last word in the packet
	int fail = 0;
	printf("TESTING Sync word .. %x %x\n",0xfa, (~0xfa) & 0x3ff);
	init_rd_state();
	encode_duv_telem_packet(test_packet, test_encoded_packet);

	uint16_t word = test_encoded_packet[DUV_DATA_LENGTH+DUV_PARITIES_LENGTH];
	printf(" Sync Word is %x \n",word );

	if (word != 0x0fa && word != 0x305) // 0x305 is ~0xfa
		fail = 1;

	if (fail == 0)
		printf(" Sync word .. Pass\n");
	else
		printf(" Sync word .. Fail\n");
	return fail;
}

int test_get_next_bit() {
	printf("TESTING get_next_bit .. \n");
	/* reset the state of the modulator */
	samples_sent_for_current_bit = 0;
	bits_sent_for_current_word = 0;
	words_sent_for_current_packet = 0;
	init_rd_state();
	// Generate the first 40 bits of the test packet - the header
	// First four bytes are: 0x51,0x01, 0x40, 0xd8
	// These should encode as 10b words:
	//   0x235    10 0011 0101    565
	//   0x1d4    01 1101 0100
	//   0x675   110 0111 0101 - note this is 11 bit because we use bit 11 as the rd state change indicator.
	//   0x0c6    00 1100 0110 - this is encoded with rd state 1
	// The RD state changes when a word does not have the same number of 1s and 0s (+/-1)

	int fail = 0;
	int expected_result[] = {
			0,0,1,1,1,1,1,0,1,0,  // the sync word 0xfa
			1,0,0,0,1,1,0,1,0,1,
			0,1,1,1,0,1,0,1,0,0,
			1,0,0,1,1,1,0,1,0,1,
			0,0,1,1,0,0,0,1,1,0};

	encode_duv_telem_packet(test_packet, test_encoded_packet);
	//	printf("%x %x %x\n",test_packet[0], test_packet[1], test_packet[2]);
	//	printf("%x %x %x\n",test_encoded_packet[0], test_encoded_packet[1], test_encoded_packet[2]);

	for (int i=0; i < 50; i++) {
		int b = get_next_bit();
		printf(" %i",b);

		if (b != expected_result[i]) {
			fail = 1;
			printf("<-err ");
		}
		if ((i+1) % 10 == 0)
			printf("\n");
	}

	// Now check the first and last parity bytes
	// First should be 0x19 -> encoded as 0x264
	// Last is 0xa7 -> encoded as 0x7a
	//	printf("First parity: %x \n",test_encoded_packet[DUV_DATA_LENGTH] );
	//	printf("Last Parity: %x \n",test_encoded_packet[DUV_DATA_LENGTH+DUV_PARITIES_LENGTH-1] );

	if (test_encoded_packet[DUV_DATA_LENGTH] != 0x264) {
		fail = 1;
	}
	if (test_encoded_packet[DUV_DATA_LENGTH+DUV_PARITIES_LENGTH-1] != 0x7a) {
		fail = 1;
	}

	if (fail == 0)
		printf(" get_next_bit .. Pass\n");
	else
		printf(" get_next_bit .. Fail\n");
	return fail;
}

int test_modulate_bit() {
	printf("TESTING modulate_bit .. \n");
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
	first_packet_to_be_sent = true;
	samples_sent_for_current_bit = 0;
	bits_sent_for_current_word = 0;
	words_sent_for_current_packet = 0;
	current_bit = 0;
	init_rd_state();

	sample_rate = 48000;
	samples_per_duv_bit = sample_rate / DECIMATION_RATE / DUV_BPS;


	encode_duv_telem_packet(test_packet, test_encoded_packet);

	int j=0;


	// Start of transmission test
	//double bit_audio_value;
	/* Run for whole first word, the sync word and check that the first and last sample of each bit is correct */
	for (int i=0; i < 600; i++) {
		double bit_audio_value = modulate_bit();
		if ((i) % samples_per_duv_bit == 0) {
			// first sample of bit
			printf ("%.3f ",bit_audio_value);
			double test_value = expected_result1[j] ? ONE_VALUE : ZERO_VALUE;
			if (test_value != bit_audio_value) {
				printf (" **start err ");
				fail = 1;
			}
		}
		if ((i+1) % samples_per_duv_bit == 0) {
			// last sample of bit
			//printf ("%.3f ",i, bit_audio_value);
			double test_value = expected_result1[j++] ? ONE_VALUE : ZERO_VALUE;
			if (test_value != bit_audio_value) {
				printf (" **end err, got '%.3f' ",bit_audio_value);
				fail = 1;
			}
		}

	}
	printf("\n");

	uint16_t decode = 0;
	int b = 9; // bit position in word
	int check_expected_results = false;
	int bit = 0;
	j = 0;
	// test end of packet and start of next
	for (int w=0; w < DUV_PACKET_LENGTH+5; w++) { // +5 so that we check first 5 words of next packet
		if (w == DUV_PACKET_LENGTH) {
			printf("end of packet\n\n");
			check_expected_results = true; // now we check the first 40 bits of the second packet, inc sync word
		}
		printf ("w:%d ",w);
		for (int i=0; i < 600; i++) { // 600 samples is a whole word
			double bit_audio_value = modulate_bit();

			if ((i) % samples_per_duv_bit == 0) {
				printf ("%.3f ",bit_audio_value);
				if (check_expected_results) {
					double test_value = expected_result2[j++] ? ONE_VALUE : ZERO_VALUE;
					if (test_value != bit_audio_value) {
						printf (" **start err ");
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
		printf(" 10b: %x 8b: %x",decode, c);
		if (w < DUV_DATA_LENGTH) {
			if (c != test_packet[w]) {
				printf(" fail \n");
				fail = 1;
			} else {
				printf(" pass \n");
			}
		} else if (w < DUV_PACKET_LENGTH) {
			if (c != test_parities_check[w-DUV_DATA_LENGTH]) {
				printf(" fail \n");
				fail = 1;
			} else {
				printf(" pass \n");
			}
		} else {
			printf("\n"); // these are checked bit by bit above
		}
		decode = 0;
		b = 9;
	}

	if (fail == 0)
		printf(" modulate_bit .. Pass\n");
	else
		printf(" modulate_bit .. Fail\n");
	return fail;

}
