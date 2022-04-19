/*
 * telem_modulator.h
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
 */

#ifndef AUDIO_PROCESSOR_H_
#define AUDIO_PROCESSOR_H_

#include <jack/jack.h>

/* the number of frames in each audio sample period.  This must match the period in ALSA */
#define PERIOD_SIZE 512

/* The reduction from 48000 samples per sec for the audio loop */
#define DECIMATION_RATE 4

/*
 * These values specify the strength of the telemetry.  They should be carefully calculated and set.
 * This should probably be in a configuration file.
 */
#define ONE_VALUE 0.20
#define ZERO_VALUE -0.20

/* When bits have the same value ramp the amount up to compensate for HPF in the radio transmitter */
#define RAMP_AMT 0.1 * ONE_VALUE

double get_loop_time_microsec();
double get_max_loop_time_microsec();
double get_min_loop_time_microsec();
int get_samples_per_bit();
double get_test_tone_freq();
int get_hpf();
int get_lpf_bits();
int get_send_telem();
int get_send_high_speed_telem();
int get_send_test_telem();
int get_send_test_tone();
int get_measure_test_tone();

void set_samples_per_bit(int val);
void set_test_tone_freq(double val);
void set_hpf(int val);
void set_lpf_bits(int val);
void set_send_telem(int val);
void set_send_high_speed_telem(int val);
void set_send_test_telem(int val);
void set_send_test_tone(int val);
void set_measure_test_tone(int val);

/* The audio loop.  This is called from jackd or alsa hardware interface routines */
jack_default_audio_sample_t * audio_loop(jack_default_audio_sample_t *in, jack_default_audio_sample_t *out, jack_nframes_t nframes);

/* Create the filters and initialize ready to process audio.  Call this before jack is started */
int init_audio_processor(int bit_rate, int decimation_rate);

/* Reset the modulator ready to send new telemetry */
int init_bit_modulator(int bit_rate, int decimation_rate);

/*
 * Test functions
 */
int test_modulate_bit();

#endif /* AUDIO_PROCESSOR_H_ */
