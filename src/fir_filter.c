/*
 * fir_filter.c
 *
 *  Created on: Feb 22, 2022
 *      Author: g0kla
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "../inc/oscillator.h"

/**
 * Processes one float sample through an FIR filter.  The caller is responsible
 * for passing in the coefficients, their length and a storage array xv for
 * intermediate calculations.  xv is the same length as the coefficients.
 *
 */
float fir_filter(float in, float *coeffs, float *xv, int len) {
	int M = len-1;
	float sum;
	for (int i = 0; i < M; i++)
		xv[i] = xv[i+1];
	xv[M] = in;
	sum = 0.0;
	for (int i = 0; i <= M; i++) {
		sum += (coeffs[i] * xv[i]);
	}
	return sum;
}

/**
 * Generate a raised cosine filter kernel and return the result in coeffs.  The caller is responsible
 * for allocating the needed space for the array.
 */
int gen_raised_cosine_coeffs(float *coeffs, float sampleRate, float freq, float alpha, int len) {
	int M = len-1;
	float Fc = freq/sampleRate;

	float sumofsquares = 0;
	float tempCoeffs[len];
	int limit = (int)(0.5 / (alpha * Fc));
	for (int i=0; i <= M; i++) {
		float sinc = (sin(2 * (float)M_PI * Fc * (i - M/2)))/ (i - M/2);
		float cos_calc = cos(alpha * (float)M_PI * Fc * (i - M/2)) / ( 1 - (pow((2 * alpha * Fc * (i - M/2)),2)));

		if (i == M/2) {
			tempCoeffs[i] = 2 * (float)M_PI * Fc * cos_calc;
		} else {
			tempCoeffs[i] = sinc * cos_calc;
		}

		// Care because ( 1 - ( 2 * Math.pow((alpha * Fc * (i - M/2)),2))) is zero for
		if ((i-M/2) == limit || (i-M/2) == -limit) {
			tempCoeffs[i] = 0.25 * (float)M_PI * sinc;
		}

		sumofsquares += tempCoeffs[i]*tempCoeffs[i];
	}
	float gain = sqrt(sumofsquares);

	for (int i=0; i < len; i++) {
		coeffs[i] = tempCoeffs[len-i-1]/gain;
	}
	return 0;
}

int test_fir_filter() {
	int fs = 48000;
	int filter_len = 48;
	float coeffs[filter_len];
	float filter_xv[filter_len];
	int rc = gen_raised_cosine_coeffs(coeffs, fs, 6000, 0.5, filter_len);
//	for (int i=0; i < filter_len; i++) {
//		printf("%f\n",coeffs[i]);
//	}

	int table_size = 9600;
	float phase1 = 0, phase2 = 0;
	float freq1 = 1000.0f, freq2 = 8000.0f;
	int samples_per_sec = 48000;

	float sin_tab[table_size];
	rc = gen_sin_table(sin_tab, table_size);

	int len = 600;
	float buffer[len];
	float buffer2[len];

	for (int n=0; n< len; n++) {
		// Fill buffer with the test signal
		float value = nextSample(&phase1, freq1, samples_per_sec, sin_tab, table_size);
		float value2 = nextSample(&phase2, freq2, samples_per_sec, sin_tab, table_size);
		buffer[n] = value + value2;
		//printf("%f\n",buffer[n]);
		// Filter
		buffer2[n] = fir_filter(buffer[n], coeffs, filter_xv, filter_len);
		printf("%f\n",buffer2[n]);
	}


	return rc;
}
