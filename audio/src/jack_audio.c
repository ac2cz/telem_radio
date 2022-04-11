/*
 * jack_audio.c
 *
 *  Created on: Mar 8, 2022
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
 */

/* System */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>

/* libraries */
#include <jack/jack.h>

/* Program */
#include "config.h"
#include "debug.h"
#include "audio_processor.h"
#include "telem_thread.h"

jack_port_t *input_port;
jack_port_t *output_port;
jack_client_t *client;

// Performance timing variables
struct timeval start, end;
double cpu_time_used;
double max_cpu_time_used;
double min_cpu_time_used = 999999;
int loops_timed = 0;
double total_cpu_time_used;
#define LOOPS_TO_TIME 400.0  // Each loop is circa 10ms.  Time about once per telem frame


/**
 * JACK calls this shutdown_callback if the server ever shuts down or
 * decides to disconnect the client.
 */
void jack_shutdown (void *arg) {
	exit (1);
}

int jack_xrun_callback(void *arg)  {
	/* count xruns */
	static int xruns = 0;
	xruns += 1;
	fprintf (stderr, "xrun %i \n", xruns);
	telem_thread_set_xruns(xruns);
	return 0;
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
	gettimeofday(&start, NULL);
	assert(nframes == PERIOD_SIZE);
	jack_default_audio_sample_t *in, *out;

	in = jack_port_get_buffer (input_port, nframes);
	out = jack_port_get_buffer (output_port, nframes);

	audio_loop(in, out, nframes);

	gettimeofday(&end, NULL);
	/* store the CPU time in microseconds */
	cpu_time_used = ((end.tv_sec * 1000000 + end.tv_usec) -
		    (start.tv_sec * 1000000 + start.tv_usec));
	//cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
	if (cpu_time_used > max_cpu_time_used)
		max_cpu_time_used = cpu_time_used;
	if (cpu_time_used < min_cpu_time_used)
		min_cpu_time_used = cpu_time_used;
	loops_timed++;
	total_cpu_time_used += cpu_time_used;
	if (loops_timed > LOOPS_TO_TIME) {
		telem_thread_set_loop_time_10_mills(round(total_cpu_time_used/loops_timed/1000.0));
		//verbose_print("INFO: Audio loop processing time: %f secs\n",total_cpu_time_used/loops_timed);
		if (max_cpu_time_used > 10000) // // 480 frames is 10ms of audio.  So if we take more than 10ms to process this we have an issue
			debug_print("WARNING: Loop ran for: %f ms\n",max_cpu_time_used/1000);
		debug_print("Max loop time: %f ms\n",max_cpu_time_used/1000);
		debug_print("Min loop time: %f ms\n",min_cpu_time_used/1000);
		total_cpu_time_used = 0;
		max_cpu_time_used = 0;
		min_cpu_time_used = 99999;
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
	const char *client_name = "telem_radio";
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

	/* Setup a callback to track XRUNS */
	jack_set_xrun_callback(client, jack_xrun_callback, 0);

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

