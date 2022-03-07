/*
 * oscillator.c
 *
 *  Created on: Feb 22, 2022
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
#include <math.h>
#include <stdio.h>

/**
 * Calculate the next sample of the sine wave at the frequency requested.
 * The caller must keep track of the current phase, so it is passed by reference.  The
 * caller should generate a sine table once and pass it in to each call,
 */
double nextSample(double *phase, double frequency, int samples_per_second, double *sin_table, int table_size) {
	double phaseIncrement = 2 * M_PI * frequency / (double)samples_per_second;
	*phase = *phase + phaseIncrement;
	if (frequency > 0 && *phase >= 2 * M_PI)
		*phase = *phase - 2 * M_PI;
	if (frequency < 0 && phase <= 0)
		*phase = *phase + 2 * M_PI;
	int idx = ((int)((*phase * table_size/(2 * M_PI)))%table_size);
	double value = sin_table[idx];
	return value;
}

/**
 * Generate a sine lookup table to reduce the processing required for the oscillator.  The table size
 * is supplied and the lookup table is returned.  The caller is responsible for allocating the needed
 * space.
 */
int gen_sin_table(double * sin_table, int table_size) {
	for (int n=0; n<table_size; n++) {
		sin_table[n] = sin(n*2.0*M_PI/table_size);
	}
	return 0;
}

int gen_cos_table(double * sin_table, int table_size) {
	for (int n=0; n<table_size; n++) {
		sin_table[n] = cos(n*2.0*M_PI/table_size);
	}
	return 0;
}

int test_oscillator() {
	int rc = 0;
	int table_size = 9600;
	double phase = 0;
	double freq = 1200.0f;
	int samples_per_sec = 48000;

	double sin_tab[table_size];
	rc = gen_cos_table(sin_tab, table_size);

	/* Generate test values which can be plotted as a graph to ensure they are correct */
	for (int i=0; i < 100; i++) {
		double value = nextSample(&phase, freq, samples_per_sec, sin_tab, table_size);
		printf("%f\n",value);
	}

	return rc;

}
