/*
 * audio.c
 *
 *  Created on: Feb 21, 2022
 *      Author: g0kla
 */

#include <stdio.h>
#include <stdlib.h>


 /**
  * Convert a byte buffer from an audio source into a float buffer that we can use for
  * DSP processing.
  * The bytes are first converted to a 16 bit signed int that runs from -32768 to +32767
  */
 int get_floats_from_bytes(char * byte_buffer, float * float_buffer, int buf_len) {
	 for (int i = 0; i< buf_len/2; i++) {
		 signed int int_value = (signed int)((byte_buffer[2*i] ) + (byte_buffer[2*i+1] <<8));
		 float_buffer[i] = (float)int_value / 32768.0f;
	 }
	 return 0;
 }


 /**
  * Convert a float buffer into a byte buffer that we can send to the audio sink
  * This assumes the format is stereo Signed 16 bit Little Endian
  */
 int get_bytes_from_floats(float * float_buffer, char * byte_buffer, int buf_len) {
	 for (int i = 0; i< buf_len/2; i++) {
		 signed int int_value = (signed int) (float_buffer[i] * 32768);
		 byte_buffer[2*i] = int_value & 0xff;
		 byte_buffer[2*i+1] = (int_value >> 8) & 0xff;
	 }
	 return 0;
 }

 /**
  * Test routine to test the functions in this file.  Returns true if all tests pass
  */
 int test_audio_tools() {
	 //const char * TEST = "test_audio_tools";
	 printf("TESTING audio_tools .. ");
	 char byte_buffer[4] = {0x00, 0x40, 0x00, 0xC0 };
	 float float_buffer[2];
	 float expected_result[2] = {0.5f, -0.5f};
	 char out_byte_buffer[4] = {0,0,0,0};
	 int fail = 0;

	 get_floats_from_bytes(byte_buffer, float_buffer, sizeof(byte_buffer));

	 /* Check if we got the right result */
	 for (int i = 0; i< (sizeof(expected_result)/sizeof(expected_result[0])); i++) {
		 // printf("%s: byte to float %d %f",TEST, i,float_buffer[i]);
		 if (expected_result[i] != float_buffer[i])
			 // printf("\t.. pass\n");
			 //else {
			 fail = 1;
		 //printf("\t.. fail\n");
		 //}
	 }

	 get_bytes_from_floats(float_buffer, out_byte_buffer, sizeof(byte_buffer));

	 /* Check if we got the right result */
	 for (int i = 0; i< (sizeof(out_byte_buffer)/sizeof(out_byte_buffer[0])); i++) {
		 // printf("%s: float to byte %d %d",TEST, i,out_byte_buffer[i]);
		 if (byte_buffer[i] != out_byte_buffer[i])
			 //	 printf("\t.. pass\n");
			 //else {
			 fail = 1;
		 //	 printf("\t.. fail\n");
		 // }
	 }
	 if (fail == 0)
		 printf("Pass\n");
	 else
		 printf("Fail\n");
	 return fail;
 }
