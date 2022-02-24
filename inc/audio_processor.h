/*
 * telem_modulator.h
 *
 *  Created on: Feb 21, 2022
 *      Author: g0kla
 */

#ifndef AUDIO_PROCESSOR_H_
#define AUDIO_PROCESSOR_H_

/* the number of frames in each audio sample period.  This must match the period in ALSA */
#define PERIOD_SIZE 480

/* Start the modulator audio thread */
int start_audio_processor (void);

#endif /* AUDIO_PROCESSOR_H_ */
