/*
 * dc_filter.c
 *
 *  Created on: Mar 27, 2022
 *      Author: g0kla
 *
 * This is an IIR single-pole DC removal filter, as described by J M de Freitas in
 * 29Jan2007 paper at:
 *
 * http://www.mathworks.com/matlabcentral/fileexchange/downloads/72134/download
 *
 * @param alpha 0.0 - 1.0 float - the closer alpha is to unity, the closer
 * the cutoff frequency is to DC.
 *
 */

/* TODO - Filter params need to be passed in a structure.  This only allows us to run one copy */
double previous_input;
double previous_output;
double alpha = 0.999;

double dc_filter( double current_input ) {

	double currentOutput = ( current_input - previous_input ) +
							  ( alpha * previous_output );

	previous_input = current_input;
	previous_output = currentOutput;

		return currentOutput;

}
