/*
 * iir_filter.h
 *
 *  Created on: Feb 23, 2022
 *      Author: g0kla
 */

#ifndef CHEBY_IIR_FILTER_H_
#define CHEBY_IIR_FILTER_H_

/**
 * Take a float as part of a continuous stream of floats in a buffer and filter it.
 */
double iir_filter(double in, double *a, double *b);

int test_cheby_iir_filter();


#endif /* CHEBY_IIR_FILTER_H_ */
