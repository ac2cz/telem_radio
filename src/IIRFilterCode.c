/*
 By Daniel Klostermann
 Iowa Hills Software, LLC  IowaHills.com
 If you find a problem, please leave a note at:
 http://www.iowahills.com/feedbackcomments.html
 May 1, 2016

 ShowMessage is a C++ Builder function, and it usage has been commented out.
 If you are using C++ Builder, include vcl.h for ShowMessage.
 Otherwise replace ShowMessage with something appropriate for your compiler.

 See the FilterKitMain.cpp file for an example on how to use this code.
*/

#include <math.h>
#include <stdio.h>
#include "../inc/IIRFilterCode.h"
#include "../inc/oscillator.h"
#include "../inc/config.h"


// This code implements an IIR filter as a Form 1 Biquad.
// It uses 2 sets of shift registers, RegX on the input side and RegY on the output side.
// There are many ways to implement an IIR filter, some very good, and some extremely bad.
// For numerical reasons, a Form 1 Biquad implementation is among the best.
void FilterWithIIR(TIIRCoeff IIRCoeff, double *Signal, double *FilteredSignal, int NumSigPts)
{
 double y;
 int j, k;

 for(j=0; j<NumSigPts; j++)
  {
   k = 0;
   y = SectCalc(j, k, Signal[j], IIRCoeff);
   for(k=1; k<IIRCoeff.NumSections; k++)
    {
     y = SectCalc(j, k, y, IIRCoeff);
    }
   FilteredSignal[j] = y;
  }

}
//---------------------------------------------------------------------------

// This gets used with the function above, FilterWithIIR()
// Note the use of MaxRegVal to avoid a math overflow condition.
double SectCalc(int j, int k, double x, TIIRCoeff IIRCoeff)
{
 double y, CenterTap;
 static double RegX1[ARRAY_DIM], RegX2[ARRAY_DIM], RegY1[ARRAY_DIM], RegY2[ARRAY_DIM], MaxRegVal;
 static int MessageShown = false;

 // Zero the registers on the 1st call or on an overflow condition. The overflow limit used
 // here is small for double variables, but a filter that reaches this threshold is broken.
 if( (j == 0 && k == 0) || MaxRegVal > OVERFLOW_LIMIT)
  {
   if(MaxRegVal > OVERFLOW_LIMIT && !MessageShown)
    {
     printf("ERROR: Math Over Flow in IIR Section Calc. \nThe register values exceeded 1.0E20 \n");
     MessageShown = true; // So this message doesn't get shown thousands of times.
    }

   MaxRegVal = 1.0E-12;
   for(int i=0; i<ARRAY_DIM; i++)
    {
     RegX1[i] = 0.0;
     RegX2[i] = 0.0;
     RegY1[i] = 0.0;
     RegY2[i] = 0.0;
    }
  }

 CenterTap = x * IIRCoeff.b0[k] + IIRCoeff.b1[k] * RegX1[k] + IIRCoeff.b2[k] * RegX2[k];
 y = IIRCoeff.a0[k] * CenterTap - IIRCoeff.a1[k] * RegY1[k] - IIRCoeff.a2[k] * RegY2[k];

 RegX2[k] = RegX1[k];
 RegX1[k] = x;
 RegY2[k] = RegY1[k];
 RegY1[k] = y;

 // MaxRegVal is used to prevent overflow.  Overflow seldom occurs, but will
 // if the filter has faulty coefficients. MaxRegVal is usually less than 100.0
 if( fabs(CenterTap)  > MaxRegVal ) MaxRegVal = fabs(CenterTap);
 if( fabs(y)  > MaxRegVal ) MaxRegVal = fabs(y);
 return(y);
}

int test_iir_filter() {
	printf("Testing Elliptic IIR Filter\n");
	int samples_per_sec = 12000;

	// Cutoff 0.05 - 300Hz at 12k or 1200Hz at 48k
	TIIRCoeff Elliptic4Pole300HzHighPassIIRCoeff, Elliptic8Pole300HzHighPassIIRCoeff;

	Elliptic4Pole300HzHighPassIIRCoeff = (TIIRCoeff) {
		.a0 = {1.0,1.0},
		.a1 = {-1.632749182559936950,-1.902361314288061540},
		.a2 = {0.680318914944733955,0.928685994848813867},
		.a3 = {0.0,0.0},
		.a4 = {0.0,0.0},

		.b0 = {0.828466075118585832,0.957801757196423798},
		.b1 = {-1.656135947267499240,-1.915443794744027710},
		.b2 = {0.828466075118585832,0.957801757196423798},
		.b3 = {0.0,0.0},
		.b4 = {0.0,0.0},
		.NumSections = 2
	};

	Elliptic8Pole300HzHighPassIIRCoeff = (TIIRCoeff) {
			.a0 = {1.0,1.0,1.0,1.0},
			.a1 = {-1.457958640999101440,-1.801882953872335770,-1.918405877608232670,-1.961807844116467030},
			.a2 = { 0.553994469886055829, 0.847908435354810197, 0.948117893349852192, 0.986885245659999910},
			.a3 = {0.0,0.0,0.0,0.0},
			.a4 = {0.0,0.0,0.0,0.0},

			.b0 = { 0.755468172841911700, 0.914802148903627210, 0.967898257208821722, 0.987349171838800999},
			.b1 = {-1.501016765201333980,-1.820187091419891430,-1.930727256540441420,-1.973994746098864940},
			.b2 = { 0.755468172841911700, 0.914802148903627210, 0.967898257208821722, 0.987349171838800999},
			.b3 = {0.0,0.0,0.0,0.0},
			.b4 = {0.0,0.0,0.0,0.0},
			.NumSections = 4
		};

	int table_size = 9600;
	float phase1 = 0, phase2 = 0, phase3 = 0;
	float freq1 = 50.0f, freq2 = 2000.0f, freq3 = 200.0f;


	float sin_tab[table_size];
	int rc = gen_sin_table(sin_tab, table_size);

	int len = 600;
	double buffer[len];
	double buffer2[len];

	for (int n=0; n< len; n++) {
		// Fill buffer with the test signal
		double value = nextSample(&phase1, freq1, samples_per_sec, sin_tab, table_size);
		double value2 = nextSample(&phase2, freq2, samples_per_sec, sin_tab, table_size);
		double value3 = nextSample(&phase3, freq3, samples_per_sec, sin_tab, table_size);
		buffer[n] = value/3.0 + value2/3.0 + value3/3.0;
		//printf("%f\n",buffer[n]);
	}

	// Filter
	FilterWithIIR(Elliptic4Pole300HzHighPassIIRCoeff, buffer, buffer2, len);

	for (int n=0; n< len; n++) {
		printf("%f\n",buffer2[n]);
	}


	return rc;
}

