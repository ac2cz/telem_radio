/*
 * jack_audio.h
 *
 *  Created on: Mar 8, 2022
 *      Author: g0kla
 */

#ifndef JACK_AUDIO_H_
#define JACK_AUDIO_H_

/* Setup and start the jack audio thread */
int start_jack_audio_processor (void);
void stop_jack();
int get_xruns_since_start();

#endif /* JACK_AUDIO_H_ */
