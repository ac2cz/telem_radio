/*
 * oscillator.c
 *
 *  Created on: Feb 22, 2022
 *      Author: g0kla
 */

#ifndef OSCILLATOR_C_
#define OSCILLATOR_C_

/**
 * Calculate the next sample of the sine wave at the frequency requested.
 * The caller must keep track of the current phase, so it is passed by reference.  The
 * caller should generate a sine table once and pass it in to each call,
 */
double nextSample(double *phase, double frequency, int samples_per_second, double *sin_table, int table_size);

/**
 * Generate a sine lookup table to reduce the processing required for the oscillator.  The table size
 * is supplied and the lookup table is returned.  The caller is responsible for allocating the needed
 * space.
 */
int gen_sin_table(double * sin_table, int table_size);
int gen_cos_table(double * cos_table, int table_size);

int test_oscillator();

#endif /* OSCILLATOR_C_ */
