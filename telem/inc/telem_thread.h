/*
 * telem_thread.h
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
 */

#ifndef TELEM_THREAD_H_
#define TELEM_THREAD_H_

/**
 * Start the telem thread and run the main process
 */
void *telem_thread_process( void *arg );

/**
 * Stop the telem thread and exit
 */
void telem_thread_stop();

/**
 * Tell the telem thread that it should gather new telemetry and build the next packet.
 * This returns imediately and the processing happens in the background
 */
void telem_thread_fill_next_packet();

/* Find out which packet is ready to be sent */
int telem_thread_get_packet_num();

/* Test functions */
int test_gather_duv_telemetry();

#endif /* TELEM_THREAD_H_ */
