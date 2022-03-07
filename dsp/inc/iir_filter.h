//---------------------------------------------------------------------------

#ifndef IIRFilterCodeH
#define IIRFilterCodeH
//---------------------------------------------------------------------------
#define OVERFLOW_LIMIT  1.0E20
#define MAX_POLE_COUNT 20
#define ARRAY_DIM 50      // This MUST be at least 2*MAX_POLE_COUNT because some filter polys are defined in terms of 2 * NumPoles

 enum TIIRPassTypes {iirLPF, iirHPF, iirBPF, iirNOTCH, iirALLPASS};

typedef struct {
	double a0[ARRAY_DIM]; double a1[ARRAY_DIM]; double a2[ARRAY_DIM]; double a3[ARRAY_DIM]; double a4[ARRAY_DIM];
	double b0[ARRAY_DIM]; double b1[ARRAY_DIM]; double b2[ARRAY_DIM]; double b3[ARRAY_DIM]; double b4[ARRAY_DIM];
    int NumSections;
} TIIRCoeff;

typedef struct {
	double RegX1[ARRAY_DIM];
	double RegX2[ARRAY_DIM];
	double RegY1[ARRAY_DIM];
	double RegY2[ARRAY_DIM];
	double MaxRegVal;
} TIIRStorage;

 void iir_filter_array(TIIRCoeff IIRCoeff, double *Signal, double *FilteredSignal, int NumSigPts);
 double iir_filter(TIIRCoeff IIRCoeff, double Signal, TIIRStorage *store);

 int test_iir_filter();

#endif
