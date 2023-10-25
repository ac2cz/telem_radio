/*
 * TelemEncoding.h
 *
 *  Created on: Feb 3, 2014
 *      Author: AMSAT-NA
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
