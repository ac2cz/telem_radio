/*
 * telem_thread.c
 *
 *  Created on: Apr 9, 2022
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
 * The telem thread runs in the background and gathers new telemetry from the
 * subsystems and sensors.  It stores the gathered telemetry in a packet and
 * asks the telem_processor to encode it into the next available encoded packet
 * buffer.  This stores a complete frame with RS Checkk bytes and a sync word.
 *
 * The telem thread waits until a new buffer is available before gathering the
 * telemetry.  The telemetry processor triggers data collection by setting
 * fill_packet = true.  But this can not be used to check if the data collection
 * is finished because it is set to false as data collection begins.
 *
 * To determine if data collection is complete the telem processor should check
 * which encoded packet buffer will be filled next.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sched.h>

#include "config.h"
#include "debug.h"
#include "telem_processor.h"

/* Forward function definitions */
int gather_duv_telemetry(int type, duv_packet_t *packet);

/* Telem thread variables */
duv_packet_t *telem_packet;  /* This is the raw data before it is RS encoded */
int called = false; /* true if we have already started the thread */
int running = true;
pthread_mutex_t fill_packet_mutex = PTHREAD_MUTEX_INITIALIZER;
int fill_packet = true;
int packet_num = 1; // The buffer that is ready to send.  Initialize to one so that on the first pass through we fill buffer zero

/* telemetry parameters and their MUTEXs */
pthread_mutex_t loop_time_mills_mutex = PTHREAD_MUTEX_INITIALIZER;
int loop_time_mills; /* Average time that the audio loop takes to run */
pthread_mutex_t xruns_mutex = PTHREAD_MUTEX_INITIALIZER;
int xruns; /* Number of Xruns since we started */

void telem_thread_process(void * arg) {
	char *name;
	name = (char *) arg;
	if (called) {
		error_print("Telem Thread already started.  Exiting: %s\n", name);
		return;
	}
	called++;

	/* Initialize */
	telem_packet = (duv_packet_t*)calloc(DUV_DATA_LENGTH,sizeof(char)); // allocate 64 bytes for the packet data

	/* Run until stopped */
	while (running) {
		/* use a mutex to prevent race conditions with fill_buffer */
		pthread_mutex_lock( &fill_packet_mutex );
		if (fill_packet) {
			fill_packet = false;
			pthread_mutex_unlock( &fill_packet_mutex );
			int packet_buffer_to_fill = !packet_num; // toggle the buffer so we fill the other one

			int type = 1;
			int rc = gather_duv_telemetry(type, telem_packet);
			if (rc != 0) {
				error_print("Error creating telemetry packet\n");
			}

			encode_next_packet(telem_packet, packet_buffer_to_fill);

			packet_num = packet_buffer_to_fill; /* This is now the next buffer available */
		}
		pthread_mutex_unlock( &fill_packet_mutex );
		sched_yield();
	}
	debug_print("Exiting %s", name);
	called--;
	free(telem_packet);
}

void telem_thread_fill_next_packet() {
	pthread_mutex_lock( &fill_packet_mutex );
	fill_packet = true;
	pthread_mutex_unlock( &fill_packet_mutex );
}

int telem_thread_get_packet_num() {
	return packet_num;
}

void telem_thread_set_loop_time_10_mills(int value) {
	pthread_mutex_lock( &loop_time_mills_mutex );
	loop_time_mills = value;
	pthread_mutex_unlock( &loop_time_mills_mutex );
}

void telem_thread_set_xruns(int value) {
	pthread_mutex_lock( &xruns_mutex );
	xruns = value;
	pthread_mutex_unlock( &xruns_mutex );
}

int gather_duv_telemetry(int type, duv_packet_t *packet) {

	int rc = EXIT_SUCCESS;

	/* Assign the spacecraft id */
	packet->header.id = 0;
	packet->header.extended_id = 3; /* SPACECRAFT_ID id 11 is 8 + 3 */

	/* Get the time stamps
	 * We calculate an epoch as the number of years since 2020.  i.e. 2 indicates 2022
	 * We calculate a time stamp as the number of seconds since the start of the year
	 */
	time_t rawtime;

	time_t trc = time ( &rawtime );
	if (trc == (time_t)-1) {
		error_print("Could not read the current time\n");
		return EXIT_FAILURE;
	}

	/* Call localtime_r to make sure this works with multiple threads */
	struct tm timeinfo;
	struct tm * timeptr;
	timeptr = localtime_r ( &rawtime, &timeinfo );
	if (timeptr != &timeinfo) {
		error_print("Could not convert time to broken down format\n");
		return EXIT_FAILURE;
	}

	char buffer[26];

	unsigned short epoch = (timeptr->tm_year - 120); /* This is the year - 1900 - 120 */

	/* CRUDE VALUE IN SECONDS FOR TESTING.  MUST implement a difference calculation that will take into account DST etc */
	unsigned int uptime = timeptr->tm_sec + timeptr->tm_min*60 + timeptr->tm_hour*60*60 + timeptr->tm_yday*24*60*60;

	verbose_print("\nEpoch: %d Uptime: %d Type: %d\n",epoch, uptime, type);
	verbose_print("Storing Type %d Telemetry time and date: %s", type, asctime_r (timeptr, buffer) );

	/* Build the header */
	packet->header.epoch = epoch;
	packet->header.uptime = uptime;
	packet->header.type = type;
	packet->header.safe_mode = false;
	packet->header.health_mode = true;
	packet->header.science_mode = false;

	sched_yield();

	/* Read the sensors and populate the payload */

	int millideg;
	unsigned short systemp;
		FILE *sys_file;
		int n;

		/* Temperature of the CPU */
		sys_file = fopen("/sys/class/thermal/thermal_zone0/temp","r");
		n = fscanf(sys_file,"%d",&millideg);
		fclose(sys_file);
		if (n == 0) {
			error_print("Failed to read the CPU temperature\n");
			systemp = 0;
		} else {
			systemp = millideg / 100;
		}
		sched_yield();

		packet->payload.pi_temperature = systemp; // pass this as tenths of a degree
		verbose_print("CPU temperature is %f degrees C\n",systemp/10.0);

#ifdef RASPBERRY_PI
		/* Frequency of the CPU - reading from this file sometimes causes an XRUN */
		int value;
		sys_file = fopen("/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq","r");
		n = fscanf(sys_file,"%d",&value);
		fclose(sys_file);
		if (n == 0) {
			error_print("Failed to read the CPU frequency\n");
			packet->payload.cpu_speed = 0;
		} else {
			/* Value is in Hz.  We just want to know if it has dropped from 1.8MHz, so we just need 2 digits. */
			packet->payload.cpu_speed = value / 100000;
		}
		verbose_print("CPU Speed is %f MHz\n",packet->payload.cpu_speed/10.0);
#endif
#ifndef RASPBERRY_PI
		packet->payload.cpu_speed = 10;
		verbose_print("CPU Speed is %f MHz\n",packet->payload.cpu_speed/10.0);
#endif

		/* Audio loop time */
		pthread_mutex_lock( &loop_time_mills_mutex );
		packet->payload.loop_time = loop_time_mills;
		verbose_print("Audio loop time %f ms\n",loop_time_mills/10.0);
		pthread_mutex_unlock( &loop_time_mills_mutex );

		/* Xruns since we started running */
		pthread_mutex_lock( &xruns_mutex );
		packet->payload.xruns = xruns;
		verbose_print("Xruns since start %i\n",xruns);
		pthread_mutex_unlock( &xruns_mutex );


#ifdef RASPBERRY_PI

		/* Read the PI sensors */

#endif


	return rc;
}


void telem_thread_stop() {
	running = false;
}

int test_gather_duv_telemetry() {
	int fail = EXIT_SUCCESS;
	printf("TESTING gather_duv_telemetry .. ");
	verbose_print("\n");

	int type = 1;
	duv_packet_t *packet = (duv_packet_t*)calloc(DUV_DATA_LENGTH,sizeof(char)); // allocate 64 bytes for the packet data
	fail = gather_duv_telemetry(type, packet);
	free(packet);

	if (fail == EXIT_SUCCESS) {
		printf(" Pass\n");
	} else {
		printf(" Fail\n");
	}
	return fail;
}
