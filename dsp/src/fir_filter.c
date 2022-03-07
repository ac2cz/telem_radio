/*
 * fir_filter.c
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
#include <stdlib.h>
#include "oscillator.h"
#include "debug.h"

/**
 * Processes one float sample through an FIR filter.  The caller is responsible
 * for passing in the coefficients, their length and a storage array xv for
 * intermediate calculations.  xv is the same length as the coefficients.
 *
 */
double fir_filter(double in, double *coeffs, double *xv, int len) {
	int M = len-1;
	double sum;
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
int gen_raised_cosine_coeffs(double *coeffs, double sampleRate, double freq, double alpha, int len) {
	verbose_print("  Generating Raised Cosine Filter Sample Rate: %d Freq:%d Alpha:%f Len:%d\n", (int)sampleRate, (int)freq, alpha, len);
	int M = len-1;
	double Fc = freq/sampleRate;

	double sumofsquares = 0;
	double tempCoeffs[len];
	int limit = (int)(0.5 / (alpha * Fc));
	for (int i=0; i <= M; i++) {
		double sinc = (sin(2 * (double)M_PI * Fc * (i - M/2)))/ (i - M/2);
		double cos_calc = cos(alpha * (double)M_PI * Fc * (i - M/2)) / ( 1 - (pow((2 * alpha * Fc * (i - M/2)),2)));

		if (i == M/2) {
			tempCoeffs[i] = 2 * (double)M_PI * Fc * cos_calc;
		} else {
			tempCoeffs[i] = sinc * cos_calc;
		}

		// Care because ( 1 - ( 2 * Math.pow((alpha * Fc * (i - M/2)),2))) is zero for
		if ((i-M/2) == limit || (i-M/2) == -limit) {
			tempCoeffs[i] = 0.25 * (double)M_PI * sinc;
		}

		sumofsquares += tempCoeffs[i]*tempCoeffs[i];
	}
	double gain = sqrt(sumofsquares);

	for (int i=0; i < len; i++) {
		coeffs[i] = tempCoeffs[len-i-1]/gain;
	}
	return 0;
}

int test_fir_filter() {
	int fs = 48000;
	int filter_len = 48;
	double coeffs[filter_len];
	double filter_xv[filter_len];
	int rc = gen_raised_cosine_coeffs(coeffs, fs, 6000, 0.5, filter_len);
//	for (int i=0; i < filter_len; i++) {
//		printf("%f\n",coeffs[i]);
//	}

	int table_size = 9600;
	double phase1 = 0, phase2 = 0;
	double freq1 = 1000.0f, freq2 = 8000.0f;
	int samples_per_sec = 48000;

	double sin_tab[table_size];
	rc = gen_sin_table(sin_tab, table_size);

	int len = 600;
	double buffer[len];
	double buffer2[len];

	for (int n=0; n< len; n++) {
		// Fill buffer with the test signal
		double value = nextSample(&phase1, freq1, samples_per_sec, sin_tab, table_size);
		double value2 = nextSample(&phase2, freq2, samples_per_sec, sin_tab, table_size);
		buffer[n] = value + value2;
		//printf("%f\n",buffer[n]);
		// Filter
		buffer2[n] = fir_filter(buffer[n], coeffs, filter_xv, filter_len);
		printf("%f\n",buffer2[n]);
	}

	return rc;
}
