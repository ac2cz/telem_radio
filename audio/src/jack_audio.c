/*
 * jack_audio.c
 *
 *  Created on: Mar 8, 2022
 *      Author: g0kla
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

/* libraries */
#include <jack/jack.h>
#include <config.h>
#include <debug.h>
#include <audio_processor.h>

jack_port_t *input_port;
jack_port_t *output_port;
jack_client_t *client;

// Performance timing variables
clock_t start, end;
double cpu_time_used;
int loops_timed = 0;
double total_cpu_time_used;
#define LOOPS_TO_TIME 5.0  // every few seconds




/**
 * JACK calls this shutdown_callback if the server ever shuts down or
 * decides to disconnect the client.
 */
void jack_shutdown (void *arg) {
	exit (1);
}



/**
 * The process callback for this JACK application is called in a
 * special realtime thread once for each audio cycle.
 *
 * Each buffer contains 480 ALSA frames, specified when jackd was started
 *
 * This calls the core audio_loop()
 *
 */
int process_audio (jack_nframes_t nframes, void *arg) {
	start = clock();
	assert(nframes == PERIOD_SIZE);
	jack_default_audio_sample_t *in, *out;

	in = jack_port_get_buffer (input_port, nframes);
	out = jack_port_get_buffer (output_port, nframes);

	audio_loop(in, out, nframes);

	end = clock();
	cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
	if (cpu_time_used > 0.01) { // 480 frames is 10ms of audio.  So if we take more than 10ms to process this we have an issue
		debug_print("WARNING: Loop ran for: %f secs\n",cpu_time_used);
	}
	loops_timed++;
	total_cpu_time_used += cpu_time_used;
	if (total_cpu_time_used > LOOPS_TO_TIME) {
		verbose_print("INFO: Audio loop processing time: %f secs\n",total_cpu_time_used/loops_timed);
		total_cpu_time_used = 0;
		loops_timed = 0;
	}
	return 0;
}

/**
 * Setup and initialize the connection to the jack server
 * Register the call backs for process_audio() and jack_shutdown()
 * Call the command interpreter if we are in interactive mode
 *
 */
int start_jack_audio_processor (void) {
	const char **ports;
	const char *client_name = "simple";
	const char *server_name = NULL;
	jack_options_t options = JackNullOption;
	jack_status_t status;

	/* open a client connection to the JACK server */
	client = jack_client_open (client_name, options, &status, server_name);
	if (client == NULL) {
		error_print ("jack_client_open() failed, "
				"status = 0x%2.0x\n", status);
		if (status & JackServerFailed) {
			error_print ("Unable to connect to JACK server\n");
		}
		exit (1);
	}
	if (status & JackServerStarted) {
		error_print ("JACK server started\n");
	}
	if (status & JackNameNotUnique) {
		client_name = jack_get_client_name(client);
		error_print ("unique name `%s' assigned\n", client_name);
		error_print ("Exiting because we require a unique connection to the server\n");
		exit (1);
	}

	/* tell the JACK server to call `process()' whenever
	   there is work to be done.
	 */
	jack_set_process_callback (client, process_audio, 0);

	/* tell the JACK server to call `jack_shutdown()' if
	   it ever shuts down, either entirely, or if it
	   just decides to stop calling us.
	 */
	jack_on_shutdown (client, jack_shutdown, 0);

	/* display the current sample rate */
	sample_rate = jack_get_sample_rate (client);

	/* create two ports */
	input_port = jack_port_register (client, "input",
			JACK_DEFAULT_AUDIO_TYPE,
			JackPortIsInput, 0);
	output_port = jack_port_register (client, "output",
			JACK_DEFAULT_AUDIO_TYPE,
			JackPortIsOutput, 0);

	if ((input_port == NULL) || (output_port == NULL)) {
		error_print("no more JACK ports available\n");
		exit (1);
	}

	/* Tell the JACK server that we are ready to roll.  Our
	 * process() callback will start running now. */
	if (jack_activate (client)) {
		error_print ("cannot activate client");
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
		error_print("no physical capture ports\n");
		exit (1);
	}

	if (jack_connect (client, ports[0], jack_port_name (input_port))) {
		error_print ("cannot connect input ports\n");
	}

	free (ports);

	ports = jack_get_ports (client, NULL, NULL,
			JackPortIsPhysical|JackPortIsInput);
	if (ports == NULL) {
		error_print("no physical playback ports\n");
		exit (1);
	}

	if (jack_connect (client, jack_port_name (output_port), ports[0])) {
		error_print ("cannot connect output ports\n");
	}

	free (ports);

	/* keep running until stopped by the user, if we are in interactive mode.  Otherwise run until jack shutdown called. */
	int rc = cmd_console();

	jack_client_close (client);
	return rc;
}

