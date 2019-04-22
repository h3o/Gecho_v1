/*
 * songs.h
 *
 *  Created on: May 30, 2016
 *      Author: mayo
 *
 *  This file is part of the Gecho Loopsynth Firmware Development Framework.
 *  It can be used within the terms of CC-BY-NC-SA license.
 *  It must not be distributed separately.
 *
 *  Find more information at: http://gechologic.com/gechologists/
 */

#ifndef SONGS_H_
#define SONGS_H_

//array of const strings, where every even entry is a chord progression and every odd entry is an accompanying melody (or empty string if none)
static const char* MusicBox_SONGS[] = {

		//Song 1: intro ad
		"a2c5e3,a3c5e4,a3c4e3,a3c5e4,d4f#4a4,e4g#4b4,a4c#5e5,a4c#5e6",

		//melody
		"a3. c4. e4. a3. c4. e4. a3. c4."
		"e4. a4. c4. e4. a4. c5. e5. a5."
		"d4 f#4 a4 d5...."
		"e4 g#4 b4 e5...."
		"a5 e5 c#5. e5 c#5. a4. c#5 a4 e4 a4 e4 c#4.",

		//Song 2: epic ad
		"c#4e3g#2,c#4e4g#3,b3e4g#3,a2c#4e4,c#4e3g#4,c#3e4g#5,b2e4g#4,a3c#4e5,"
		"a3c#4e4,a3c#3e4,b2d#4f#5,b3d#4f#4,f#2a4c#5,f#3a4c#5,f#3a4c#5,f#3a4c#4," //1
		"a3c#3e4,a3c#4e4,b3d#3f#4,b3d#4f#4,g#3c4d#3,g#4c4d#4,g#4c5d#5,g#4c4d#4," //2
		"c#2e3g#3,c#2e3g#4,a2c#3e3,a2c#4e4,b2d#3f#5,b3d#4f#5,f#3a4c#4,f#3a4c#5," //repeats
		"c#2e4g#4,c#3e3g#4,b3d#3f#4,b3d#4f#4,c#3e4g#5,c#3e4g#5,b4d#5f#5,b4d#4f#3," //last
		"a3c#4e4,a2c#3e4,b2d#4f#5,b3d#4f#4,f#2a4c#5,f#3a4c#5,f#3a4c#4,f#3a3c#4," //1
        "a3c#3e4,a3c#4e4,b3d#3f#4,b3d#4f#4,g#3c4d#4,g#2c4d#4,g#2c4d#5,g#2c3d#4", //2

		//melody
		"b4 g#4 f#4 e4 b4 g#4 f#4 e4 b4 g#4 f#4 e4 b4 g#4 f#4 e4 "
        "b3 g#3 f#3 e3 b3 g#3 f#3 e3 b5 g#5 f#5 e5 b5 g#5 f#5 e5 "
		"b4 g#4 f#4 e4 b4 g#4 f#4 e4 b4 g#4 f#4 e4 b4 g#4 f#4 e4 "
		"b3 g#3 f#3 e3 b3 g#3 f#3 e3 b5 g#5 f#5 e5 b5 g#5 f#5 e5 "
		"c#5.. e5.... e5.. d#5.. b4. d#4.. e4............"
        "c#5.. e5.... e5.. d#5.. b4. g#4..............."
        "c#4.. e4........ d#4. b3. c#4.. e4........ d#4. b3."
        "c#5.. e5........ d#5. b4. c#5.. e5........ d#5. b4.",

		//Songs 3-10: empty
		"", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",

		//Song 11: main demo song
		"a2c5e3,a3c5e4,a3c4e3,a3c5e4,d4f#4a4,e4g#4b4,a4c#5e5,a4c#5e6,"
		"a2c5e3,a3c5e4,a3c4e3,a3c5e4,d4f#4a4,e4g#4b4,a4c#5e5,a4c#5e6,"
		"a3c4e4,b3d4g4,c4e4g4,d4f4a4,e4g#4b4,f4a4c4,g4b4d5,g3b4d6,"
		"a2c5e3,b3d5g4,c4e4g4,d3f5a4,e4g#4b4,f4a4c4,g4b5d5,g4b4d4,"
		"g#4c5d#5,f4g#4c5,d4f4g#4,b3d4f4,g4b4d5,f4g#4c4,d#4g4a#4,d3g4b4,"
		"a3c4e4,b3d4g4,c4e4g4,d4f4a4,e4g#4b4,f4a4c4,g4b4d5,g3b4d6,"
		"a2c5e3,b3d5g4,c3e4g3,d3f5a4,e4g#4b4,f4a4c4,g4b5d5,g4b5d3,"
		"a3c6e4,a4c6e5,a4c3e4,a4c5e4,d4f#5a4,e4g#5b4,a4c#5e5,a4c#5e4",
		"", //no melody

		//Song 12: Johann Pachelbel - Canon in D
		"d3f#3a3,a3c#4e4,b3d4f#4,f#3b3c#4,g3b3d4,d4f#4a4,e4g4b3,a3c#4e4", //lower octave
		//"d4f#4a4,a4c#5e5,b4d5f#5,f#4b4c#5,g4b4d5,d5f#5a5,e5g5b4,a4c#5e5", //higher octave
		"", //no melody

		//Song 13: Mario's composition
		"f3g#3c4,f3g#4c4,c#3f3g#3,c#4f3g#4,c4e#4g4,c4e#4g3,a#3c#3f3,a#3c#3f4",
		"", //no melody

		//Song 14: xmas song
		"c4e3g2,c4e2g3,c4e3g3,c4e2g3,c4f4a3,c4e3g4,d4f4a4,g3b3d4,"
		"c4e3g4,c4e3g3,c3e4g3,c4e3g4,c3f2a4,c3e4g3,g3b3d4,c4e3g3,"
		"c3e3g4,c4e4g4,c3e3g4,a3c4f4,a4c3f4,g3b3d4,g3b4d4,c2e3g4,"
		"c4e3g4,c4e3g2,c3e2g4,a3c4f3,a4c3f4,g3b2d4,g3b2d4,c2e3g3",

		//melody
		"e5e5e5. e5e5e5. e5g5c5d5 e5... f5f5f5f5 f5e5e5. e5d5d5e5 d5..."
		"e4e4e4.e4e4e4.e4g4c4d4e4...f4f4f4f4f4e4e4.g4g4f4d4c4..."
		"g4e5d5c5g4...g4e5d5c5a4...a4f5e5d5g5g5g5.a5g5f5d5e5..."
		"g3e4d4c4g3...g3e4d4c4a3...a3f4e4d4g4g4g4.a4g4f4d4c4...",

		//Songs 15-20: empty
		"", "", "", "", "", "", "", "", "", "", "", "",

		//Song 21: GITS
		"a3c4e4,a3c4e4,a3e4c4,e3a3c4,a3c4e4,a3e4c4,a3c4e4,g3d4b3," //main
		"f3a3c4,f3c4a3,f3a3c4,f3a3c4,a3f3c4,a3f4c3,a3e4c4,e4a3c4,"
		"e3g3b3,e3b3g3,f3a3c4,f3c4a3,a3c4e4,b3d4g4,c4e4a4,d4g4b4,"
		"f4a4c5,f4a4c5,e4g#4b4,e4g#4b4,"

		"a3c4e4,b3d4f#4,e3g3b3,e4g4b3,a3c4e4,b3f#4d4,e4f#3d4,e4f#4b3," //int 1
		"a3c4e4,b3d4f#4,e3g3b3,e4g4b3,a3c4e4,b3f#4d4,e4f#4b3,e4g#4b3,"
		"a3c4e4,a4c4e4,b3d4f#4,b4d4f#4,c4e4g4,c4g4e4,b3e4f#4,b3d#4f#4,"

		"a3c4e4,a3c4e4,a3e4c4,e3a3c4,a3c4e4,a3e4c4,a3c4e4,g3d4b3," //main
		"f3a3c4,f3c4a3,f3a3c4,f3a3c4,a3f3c4,a3f4c3,a3e4c4,e4a3c4,"
		"e3g3b3,e3b3g3,f3a3c4,f3c4a3,a3c4e4,b3d4g4,c4e4a4,d4g4b4,"
		"f4a4c5,f4a4c5,e4g#4b4,e4g#4b4,"

		"d3f4a4,e3g#4b4,a4c4e3,a4c4e3,"
		"f3a4c4,e4g#4b4,a3c4e4,a4c4e4,"
		"d3f4a4,e3g#4b4,a4c4e3,a4c4e3,"
		"f3a4c4,f3a4c4,d3f4a4,a#3d4f4,"
		"a3c4e4,a4c4e4,a3c4e4,a4c4e4",

		//melody
		"a4.....b4c5 d5......d#5 d5....... ......b4. c5....... ....c5b4a4g4 a4....... ......g4." //main
		"a4.....b4c5 d5......d#5 d5....... ......b4. c5....... ....c5b4a4g4 a4....... ......g4."
		"f#4...e4... f#4.....g4. a4...g4... a4.b4.c5.d5. e5.....e5. d5...b4... e5.....e5. d5...b4..."
		"e5....... ....d5... e5....... ........"

		"e4...d4e4.b4 f#4...b3... g4....f#4g4a4 b4..e5g5f#5.b4" //int 1
		"a4...e5... d5...f#5.d5b4 a4....... ........"
		"e4...d4e4.b4 f#4...b3... g4....f#4g4a4 b4..e5g5f#5.b4"
		"a4...e5... d5...b4... e5....... ........"
		"e5.....d5e5 a4.....e5. f#5.....e5f#5 b4...f#5..."
		"e5.....d5e5 g4...a4... b4....... e5.e5...g4."

		"a4.....b4c5 d5......d#5 d5....... ......b4. c5....... ....c5b4a4g4 a4....... ......g4." //main
		"a4.....b4c5 d5......d#5 d5....... ......b4. c5....... ....c5b4a4g4 a4....... ......g4."
		"f#4...e4... f#4.....g4. a4...g4... a4.b4.c5.d5. e5.....e5. d5...b4... e5.....e5. d5...b4..."
		"e5....... ....d5... e5....... ........"

		"d5..d5d5.d5. e5.....e5. e5....... ........" //int 2
		"d5..d5d5.d5. e5.....e5. e5....... ........"
		"d5..d5d5.d5. e5.....e5. e5....... ..a4.b4.c5."
		"g5...e5... ..g4.a4.e5. c5...a4... d5......."
		"e5....... ........ ........ ........",

		//Song 22: Rachel song
		"g3a#3d3,g3a3d4,g3a#2d3,g3a2d3,g3a#4d4,g3a3d4,g3a#3d4,g4a4d3,g3a#3d3,g3a3d4,g3a#2d3,g3a2d3,g3a#4d4,g3a3d4,g3a#3d4,g4a4d3,g3a#3d4,g3a#4d4,c4e4g3,c4e4g4,d4f3a3,d4e4g3,d4e4a3,d4f4a4,g3a#3d4,g3a#4d4,c4e4g3,c4e4g4,d4f3a3,d4e4g3,d4f4a4,f4d4a4,d3f4a3,d3f3a3,d#3g4a#3,d#3g3a#3,g3a#3d4,d3f3a4,g3a#3d4,g3a#3d4,d3f4a3,d4f3a3,d#3g4a#3,d#4g3a#4,g3a#3d4,f3a3c4,g3a#3d4,g4a#3d4",
		"", //no melody

		//Song 23: Eastern European composition
		"a3c4e4,d4f4a4,g4d4b3,c4e4g4,a4c5e5,d5a4f4,g4d4b3,a3c#4e4",
		"", //no melody

		//Song 24
		"f3g#3c4,f3g#4c4,c#3f3g#3,c#4f3g#4,a#d#3g3,a#d#3g2,a#2c#3f3,a#3c#3f4", //broken syntax - weirldy cool one
		"", //no melody

		//Songs 25-30: empty
		"", "", "", "", "", "", "", "", "", "", "", "",

		//Song 31: Notjustmoreidlechatter by Paul Lansky
		"g3a#3d4,d#4g3a#3,g3a#4d4,f3a4c4,g3a#3d4,c4e4g3,g4a#3d4,f4a#3d4,"
		"g3a#3d4,d#3g3a#3,g3a#4d4,f4a4c4,g3a#3d4,c3e3g3,g3a#3d4,a#3d4g4,"
		"f3a3c4,g3a#3d4,f4a3c4,d#4g4a#3,d4f#4a3,c4d#4g3,a#4d4f4,a4d4f#3",
		"", //no melody

		//Songs 32-34: empty
		"", "", "", "", "", "",

		//Songs 35-40: empty
		"", "", "", "", "", "", "", "", "", "", "", "",

		//Song 41
		"a1a2a3,a1a2a3,a3c4e4,a3c4e4,a3c#4e4,a3c#4e4,a3c4e4,a3c4e4,a3c#4e4,a3c#4e4,a3a4a5,a2a3a4,a3a4a5,a2a3a4,a1a2a3,a1a2a3",
		"", //no melody

		//Song 42
		"a3c4e4",	//a flat (minor) chord
		"", //no melody

		//Song 43
		"a3c#4e4",	//A (major) chord
		"", //no melody

		//Song 44
		"a3a4a5", //one note per 3 octaves
		"", //no melody

		//"g3a#3d4,d#3g3a#3,g3a#d4,f3a3c4,g3a#3d4,c3e3g3,g3a#3d4,a#3d4f4,"
		//"f3a3c4,g3a#3d4,d#4g4a#3,f4a4c4,d4f#4a3,c4d#4g3,a#4d#4f#3,a4d4f#3"

}; //end songs collection


#define SONG_BASES_MAJOR_34 "c3e3g3,d3f#3a3,e3g#3b3,f3a3c4,g3b3d4,a3c#4e4,b3d#4f#4,c4e4g4"
#define SONG_BASES_MAJOR_45 "c4e4g4,d4f#4a4,e4g#4b4,f4a4c5,g4b4d5,a4c#5e5,b4d#5f#5,c5e5g5"
#define SONG_BASES_MINOR_45 "c4d#4g4,d4f4a4,e4g4b4,f4g#4c5,g4a#4d5,a4c5e5,b4d5f#5,c5d#5g5"

#define SONG_BASES_MAJOR_MINOR_45 "c2d#2g2,c4e4g4,d4f#4a4,e4g#4b4,f4a4c5,g4b4d5,a4c#5e5,b4d#5f#5,c5e5g5,c4d#4g4,d4f4a4,e4g4b4,f4g#4c5,g4a#4d5,a4c5e5,b4d5f#5,c5d#5g5"


#ifdef __cplusplus
 extern "C" {
#endif

int channel_to_song_and_melody(int channel);

#ifdef __cplusplus
}
#endif

#endif /* SONGS_H_ */
