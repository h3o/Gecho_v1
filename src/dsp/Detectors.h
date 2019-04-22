/*
 * Detectors.h
 *
 *  Created on: Jun 30, 2016
 *      Author: mayo
 */

#ifndef DETECTORS_H_
#define DETECTORS_H_

#include <dsp/Goertzel.h>

//detection of two octaves
#define GORS 25
#define GORS_FREQS_345 "c3,c#3,d3,d#3,e3,f3,f#3,g3,g#3,a3,a#3,b3,c4,c#4,d4,d#4,e4,f4,f#4,g4,g#4,a4,a#4,b4,c5"
#define GORS_FREQS_456 "c4,c#4,d4,d#4,e4,f4,f#4,g4,g#4,a4,a#4,b4,c5,c#5,d5,d#5,e5,f5,f#5,g5,g#5,a5,a#5,b5,c6"
#define GORS_FREQS_567 "c5,c#5,d5,d#5,e5,f5,f#5,g5,g#5,a5,a#5,b5,c6,c#6,d6,d#6,e6,f6,f#6,g6,g#6,a6,a#6,b6,c7"
#define GORS_FREQS_678 "c6,c#6,d6,d#6,e6,f6,f#6,g6,g#6,a6,a#6,b6,c7,c#7,d7,d#7,e7,f7,f#7,g7,g#7,a7,a#7,b7,c8"

#define GOERTZEL_OCTAVES_345 GORS_FREQS_345
#define GOERTZEL_OCTAVES_456 GORS_FREQS_456
#define GOERTZEL_OCTAVES_567 GORS_FREQS_567
#define GOERTZEL_OCTAVES_678 GORS_FREQS_678

class Detectors
{
	public:

		Goertzel *gor;

		float *gors_freqs;
		char *gors_freq_notes;

		Detectors(char *goertzel_octaves);
		~Detectors(void);

		void detectors_setup(int buffer_length);
		void process_sample(float sample);
		int find_max_freq();
		void reset_all();
		//void test_Goertzel_detectors();

	private:

};

#endif /* DETECTORS_H_ */
