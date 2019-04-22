/*
 * MIDI.h
 *
 *  Created on: Jan 9, 2017
 *      Author: mayo
 *
 *  This file is part of the Gecho Loopsynth Firmware Development Framework.
 *  It can be used within the terms of CC-BY-NC-SA license.
 *  It must not be distributed separately.
 *
 *  Find more information at: http://gechologic.com/gechologists/
 */

#ifndef MIDI_H_
#define MIDI_H_

extern int MIDI_notes_to_freq[128];

#ifdef __cplusplus
 extern "C" {
#endif

void MIDI_out_test();
void MIDI_record_playback_test();
void MIDI_direct_signals_test();

#ifdef __cplusplus
}
#endif

#endif /* MIDI_H_ */
