/*
 * TelemEncoding.h
 *
 *  Created on: Feb 3, 2014
 *      Author: AMSAT-NA
 */
#ifndef TELEMENCODING_H_
#define TELEMENCODING_H_

#include <stdint.h>


#define CHARACTER_BITS 10
#define CHARACTERS_PER_LONGWORD 3
#define CHARACTER_MASK ((1<<CHARACTER_BITS)-1)
#define SYNC_CHARACTER -1

#define PARITY_BYTES_PER_CODEWORD 32U     // Number of parity symbols in frame
#define NP 32U //For Phil's code
#define DATA_BYTES_PER_CODE_WORD 223

void update_rs(
   unsigned char parity[32], // 32-byte encoder state; zero before each frame
   unsigned char c          // Current data byte to update
);

int encode_8b10b(
    int *state, // pointer to encoder state (run disparity, RD)
    int16_t data);

#endif /* TELEMENCODING_H_ */
