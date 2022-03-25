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
#include <string.h>
#include "oscillator.h"
#include "debug.h"

/*
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

int gen_root_raised_cosine_coeffs(double *coeffs, double sampleRate, double freq, double alpha, int len) {
	verbose_print("  Root Raised Cosine Filter Rate: %d Freq:%d Alpha:%f Len:%d\n", (int)sampleRate, (int)freq, alpha, len);
	int M = len-1;

	double Ts = 1/(freq); // reciprocal of the symbol rate in Hz
	double sum = 0;

	double tempCoeffs[M+1];

	for (int i=0; i <= M; i++) {
		double t = (i - M/2) / sampleRate;

		double sin_calc = sin((1 - alpha) * M_PI * t / Ts);
		double cos_calc = (4 * alpha * t / Ts) * cos((1 + alpha) * M_PI * t / Ts);
		double det = M_PI * t / Ts * (1 - pow(4 * alpha * t / Ts,2));
		//double blackman = 0.42 - 0.5 * cos((2 * M_PI * i) / M) + 0.08 * cos((4 * M_PI * i) / M);
		if (t == 0) {
			tempCoeffs[i] = 1/sqrt(Ts) * (1 - alpha + 4*alpha/M_PI);
		} else 	if (t == Ts/(4*alpha) || t == -Ts/(4*alpha)) {
			tempCoeffs[i] = alpha/sqrt(2 * Ts) * ((1+2/M_PI)* sin(M_PI/(4*alpha)) + (1-2/M_PI)*cos(M_PI/(4*alpha)));
		} else {
			tempCoeffs[i] = 1/sqrt(Ts) * ((sin_calc + cos_calc)/det);
		}
		//xcoeffs[i] = xcoeffs[i] * blackman;
		sum += tempCoeffs[i];
	}

	for (int i=0; i<=M; i++) {
		coeffs[i] = tempCoeffs[i]/sum;
	}
	return 0;
}

/*
 * Generate a raised cosine filter kernel and return the result in coeffs.  The caller is responsible
 * for allocating the needed space for the array.
 */
int gen_raised_cosine_coeffs(double *coeffs, double sampleRate, double freq, double alpha, int len) {
	verbose_print("  Raised Cosine Filter Rate: %d Freq:%d Alpha:%f Len:%d\n", (int)sampleRate, (int)freq, alpha, len);
	int M = len-1;
	double Fc = freq/sampleRate;

	double sum = 0;
	double tempCoeffs[M+1];
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

		sum += tempCoeffs[i];
	}
	//double gain = sqrt(sumofsquares);

	/* Gain for low pass filter is equal to the sum of the coefficients, so normalize to have gain = 1 */
	for (int i=0; i < len; i++) {
		coeffs[i] = tempCoeffs[len-i-1]/sum;
	}
	return 0;
}

/******************************************************************************
 *
 * TEST FUNCTIONS
 *
 ******************************************************************************/

int test_fir_filter() {
	int fs = 12000;
	int filter_len = 60;
	double coeffs[filter_len];
	double filter_xv[filter_len];
	memset(filter_xv, 0, filter_len*sizeof(filter_xv[0]));
	int rc = gen_root_raised_cosine_coeffs(coeffs, fs, 200, 0.5, filter_len);
//	for (int i=0; i < filter_len; i++) {
//		printf("%f\n",coeffs[i]);
//	}

	int table_size = 9600;
//	double phase1 = 0, phase2 = 0;
//	double freq1 = 100.0f, freq2 = 8000.0f;

	double sin_tab[table_size];
	rc = gen_sin_table(sin_tab, table_size);

	int len = 1200;
	int bit_len = fs / 200;
	int bit_pos = 0;
	double buffer[len];
	double buffer2[len];
	double value = 0.2;
	int bits[] = {1,-1,1,-1,-1,1,1};  // this gives bits of 1 1 0 0 1 0 0 0 0 1 1 0 ...
	int p = 0;

	for (int n=0; n< len; n++) {
		// Fill buffer with the test signal
		//double value = nextSample(&phase1, freq1, samples_per_sec, sin_tab, table_size);
		if (bit_pos >= bit_len) {
			bit_pos = 0;
			value = bits[p++] * value;
			if (p == 7) p = 0;
		}
		bit_pos++;
		//double value2 = nextSample(&phase2, freq2, fs, sin_tab, table_size);
		buffer[n] = value;// + value2;
		printf("%f\n",buffer[n]);
		// Filter
		buffer2[n] = fir_filter(buffer[n], coeffs, filter_xv, filter_len);
		//printf("%f\n",buffer2[n]);
	}

	return rc;
}
