/*
 * oscillator.c
 *
 *  Created on: Feb 22, 2022
 *      Author: g0kla
 */
#include <math.h>
#include <stdio.h>

/**
 * Calculate the next sample of the sine wave at the frequency requested.
 * The caller must keep track of the current phase, so it is passed by reference.  The
 * caller should generate a sine table once and pass it in to each call,
 */
float nextSample(float *phase, float frequency, int samples_per_second, float *sin_table, int table_size) {
	float phaseIncrement = 2 * (float)M_PI * frequency / (float)samples_per_second;
	*phase = *phase + phaseIncrement;
	if (frequency > 0 && *phase >= 2 * (float)M_PI)
		*phase = *phase - 2 * (float)M_PI;
	if (frequency < 0 && phase <= 0)
		*phase = *phase + 2 * (float)M_PI;
	int idx = ((int)((*phase * (float)table_size/(2 * (float)M_PI)))%table_size);
	double value = sin_table[idx];
	return value;
}

/**
 * Generate a sine lookup table to reduce the processing required for the oscillator.  The table size
 * is supplied and the lookup table is returned.  The caller is responsible for allocating the needed
 * space.
 */
int gen_sin_table(float * sin_table, int table_size) {
	for (int n=0; n<table_size; n++) {
		sin_table[n] = sin(n*2.0*(float)M_PI/table_size);
	}
	return 0;
}

int gen_cos_table(float * sin_table, int table_size) {
	for (int n=0; n<table_size; n++) {
		sin_table[n] = cos(n*2.0*(float)M_PI/table_size);
	}
	return 0;
}

int test_oscillator() {
	int rc = 0;
	int table_size = 9600;
	float phase = 0;
	float freq = 1200.0f;
	int samples_per_sec = 48000;

	float sin_tab[table_size];
	rc = gen_cos_table(sin_tab, table_size);

	/* Generate test values which can be plotted as a graph to ensure they are correct */
	for (int i=0; i < 100; i++) {
		float value = nextSample(&phase, freq, samples_per_sec, sin_tab, table_size);
		printf("%f\n",value);
	}

	return rc;

}
