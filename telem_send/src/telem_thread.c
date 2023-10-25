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
 * buffer.  This stores a complete frame with RS Check bytes and a sync word.
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
#include "jack_audio.h"

#include "../../telem_send/inc/duv_telem_layout.h"
#include "../../telem_send/inc/telem_processor.h"

/* Forward function definitions */
int gather_duv_telemetry(int type, duv_packet_t *packet);
void print_duv_packet(duv_packet_t *packet);
void print_duv_header(duv_header_t header);
void print_rttelemetry(rttelemetry_t payload);

/* Telem thread variables */
duv_packet_t *telem_packet;  /* This is the raw data before it is RS encoded */
int called = false; /* true if we have already started the thread */
int running = true;
pthread_mutex_t fill_packet_mutex = PTHREAD_MUTEX_INITIALIZER;
int fill_packet = true;
int packet_num = 1; // The buffer that is ready to send.  Initialize to one so that on the first pass through we fill buffer zero

/**
 * Main process of the telem thread.  This is called when the pthread is created.
 * It runs while the boolean running is true
 *
 */
void *telem_thread_process(void * arg) {
	char *name;
	name = (char *) arg;
	if (called) {
		error_print("Telem Thread already started.  Exiting: %s\n", name);
		running = false;
	}
	called++;

	debug_print("Starting Thread: %s\n", name);

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
	debug_print("Exiting Thread: %s\n", name);
	called--;
	free(telem_packet);
	return EXIT_SUCCESS;
}

void telem_thread_fill_next_packet() {
	pthread_mutex_lock( &fill_packet_mutex );
	fill_packet = true;
	pthread_mutex_unlock( &fill_packet_mutex );
}

int telem_thread_get_packet_num() {
	return packet_num;
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
		verbose_print("CPU temperature is %.2f degrees C\n",systemp/10.0);

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
		if (g_verbose)
			print_duv_packet(packet);
#ifdef RASPBERRY_PI
#endif
#ifndef LINUX

#endif

		/* Audio loop time */
		packet->payload.loop_time = get_loop_time_microsec()/100;
		verbose_print("Audio loop time %.2f ms\n",packet->payload.loop_time/10.0);

		/* Xruns since we started running */
		packet->payload.xruns = get_xruns_since_start();
		verbose_print("Xruns since start %i\n",packet->payload.xruns);

#ifdef RASPBERRY_PI

		/* Read the PI sensors */

#endif


	return rc;
}

void print_duv_packet(duv_packet_t *packet) {
	print_duv_header(packet->header);
	if (packet->header.type == 1)
		print_rttelemetry(packet->payload);
	 printf("\n");
}

void print_duv_header(duv_header_t header) {
    printf("DUV HEADER\n");
    if (header.id == 0)
        printf("Id: %d ",8 + header.extended_id);
    else
    	printf("Id: %d ",8 + header.id);
    printf("Epoch: %d ",header.epoch);
    printf("Uptime: %d ",header.uptime);
    printf("Type: %d\n",header.type);
}

/**
 * This function is generated by the AMSAT Spacecraft Editor.  Change the layout in that tool
 * and re-generate this function
 */
void print_rttelemetry(rttelemetry_t payload) {
    printf("RAW PAYLOAD: rttelemetry\n");
    printf("Temperature: %d  ",payload.pi_temperature);
    printf("Xruns: %d  ",payload.xruns);
    printf("Audio Loop: %d  ",payload.loop_time);
    printf("Proc Speed: %d  ",payload.cpu_speed);
    printf("\n");
    printf("Data 2: %d  ",payload.data2);
    printf("Data 3: %d  ",payload.data3);
    printf("None: %d  ",payload.pad1);
    printf("None: %d  ",payload.pad2);
    printf("\n");
    printf("None: %d  ",payload.pad3);
    printf("None: %d  ",payload.pad4);
    printf("None: %d  ",payload.pad5);
    printf("None: %d  ",payload.pad6);
    printf("\n");
    printf("None: %d  ",payload.pad7);
    printf("None: %d  ",payload.pad8);
    printf("None: %d  ",payload.pad9);
    printf("None: %d  ",payload.pad10);
    printf("\n");
    printf("None: %d  ",payload.pad11);
    printf("None: %d  ",payload.pad12);
    printf("None: %d  ",payload.pad13);
    printf("\n");
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
