/*
 * iir_filter.c
 *
 *  Created on: Feb 23, 2022
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
 *  This is an IIR filter implemented according to Ch 20 of the Engineers and Scientists Guide to DSP
 *  Note that the maximum number of poles in a Chebyshev filter is limited by the cutoff frequency.  For
 *  single precision the limit is aproximately as follows:
 *
 *  Cutoff Freq    0.02    0.05    0.10
 *  Max poles        4       6      10
 *
 *  The storage for the filter is at the file level currently, so you can not run two versions of this
 *  filter at the same time
 *
 */

#include "cheby_iir_filter.h"

#include <stdio.h>
#include <stdlib.h>
#include "oscillator.h"

int pos1=0,pos2=0;
int n1 = 4, n2 = 4;
double buf1[4], buf2[4];

/* These are test filters from a lookup table.  We should design and implement optimal filters for final use */

// 4 pole cheb lpf at fc = 0.025 = 1200Kz at 48k or 240Hz at 9600 samples per sec  Ch 20 Eng and Sci guide to DSP
double a_lpf_tst[] = {1.504626E-05, 6.018503E-05, 9.027754E-05, 6.018503E-05, 1.504626E-05};
double b_lpf_tst[] = {1, 3.725385E+00, -5.226004E+00,  3.270902E+00,  -7.705239E-01};

// 4 pole cheb hpf at fc = 0.025 = 1200Kz at 48k or 240Hz at 9600 samples per sec  Ch 20 Eng and Sci guide to DSP
double a_hpf_tst[] = {7.941874E-01, -3.176750E+00, 4.765125E+00, -3.176750E+00, 7.941874E-01};
double b_hpf_tst[] = {1, 3.538919E+00, -4.722213E+00,  2.814036E+00,  -6.318300E-01};

 /**
  * Processes one input signal value and returns the next output signal
  * value.
  */
double cheby_iir_filter(double in, double *a, double *b) {
	double out = a[0] * in;

     for (int j = 1; j <= n1; j++) {
         int p = (pos1 + n1 - j) % n1;
         out += a[j] * buf1[p];
     }

     for (int j = 1; j <= n2; j++) {
         int p = (pos2 + n2 - j) % n2;
         out += b[j] * buf2[p];
     }
     if (n1 > 0) {
         buf1[pos1] = in;
         pos1 = (pos1 + 1) % n1;
     }
     if (n2 > 0) {
         buf2[pos2] = out;
         pos2 = (pos2 + 1) % n2;
     }
     return out;
 }

 int test_cheby_iir_filter() {
// 	int fs = 48000;
// 	int filter_len = 48;025
// 	float coeffs[filter_len];
// 	float filter_xv[filter_len];
// 	int rc = gen_raised_cosine_coeffs(coeffs, fs, 6000, 0.5, filter_len);
 //	for (int i=0; i < filter_len; i++) {
 //		printf("%f\n",coeffs[i]);
 //	}

 	int table_size = 9600;
 	double phase1 = 0, phase2 = 0;
 	double freq1 = 150.0f, freq2 = 8000.0f;
 	int samples_per_sec = 48000;

 	double sin_tab[table_size];
 	int rc = gen_sin_table(sin_tab, table_size);

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
 		buffer2[n] = cheby_iir_filter(buffer[n], a_hpf_tst, b_hpf_tst);
 		printf("%f\n",buffer2[n]);
 	}


 	return rc;
 }


