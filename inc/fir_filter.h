/*
 * fir_filter.h
 *
 *  Created on: Feb 22, 2022
 *      Author: g0kla
 */

#ifndef FIR_FILTER_H_
#define FIR_FILTER_H_

/**
 * Processes one float sample through an FIR filter.  The caller is responsible
 * for passing in the coefficients, their length and a storage array xv for
 * intermediate calculations.  xv is the same length as the coefficients.
 *
 */
float fir_filter(float in, float *coeffs, float *xv, int len);

/**
 * Generate a raised cosine filter kernel and return the result in coeffs.  The caller is responsible
 * for allocating the needed space for the array.
 */
int gen_raised_cosine_coeffs(float * coeffs, float sampleRate, float freq, float alpha, int len);

int test_fir_filter();

#endif /* FIR_FILTER_H_ */
