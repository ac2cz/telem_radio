/*
 * iir_filter.h
 *
 *  Created on: Feb 23, 2022
 *      Author: g0kla
 */

#ifndef IIR_FILTER_H_
#define IIR_FILTER_H_

/**
 * Take a float as part of a continuous stream of floats in a buffer and filter it.
 */
float iir_filter(float in, float *a, float *b);

int test_iir_filter();


#endif /* IIR_FILTER_H_ */
