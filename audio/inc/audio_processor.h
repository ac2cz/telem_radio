/*
 * telem_modulator.h
 *
 *  Created on: Feb 21, 2022
 *      Author: g0kla
 */

#ifndef AUDIO_PROCESSOR_H_
#define AUDIO_PROCESSOR_H_

#include <jack/jack.h>

/* the number of frames in each audio sample period.  This must match the period in ALSA */
#define PERIOD_SIZE 480

jack_default_audio_sample_t * audio_loop(jack_default_audio_sample_t *in, jack_default_audio_sample_t *out, jack_nframes_t nframes);
int cmd_console();

int test_rs_encoder();
int test_sync_word();
int test_get_next_bit();
int test_modulate_bit();

#endif /* AUDIO_PROCESSOR_H_ */
