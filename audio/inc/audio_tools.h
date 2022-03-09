/*
 * filter.h
 *
 *  Created on: Feb 21, 2022
 *      Author: g0kla
 */

#ifndef AUDIO_TOOLS_H_
#define AUDIO_TOOLS_H_

/**
 * Convert a byte buffer from an audio source into a float buffer that we can use for
 * DSP processing. This assumes the format is stereo Signed 16 bit Little Endian.
 */
int get_floats_from_bytes(char * byte_buffer, float * float_buffer, int buf_len);

/**
 * Convert a float buffer into a byte buffer that we can send to the audio sink
 * This assumes the format is stereo Signed 16 bit Little Endian
 */
int get_bytes_from_floats(float * float_buffer, char * byte_buffer, int buf_len);

/* Test functions */
int test_audio_tools();

#endif /* AUDIO_TOOLS_H_ */
