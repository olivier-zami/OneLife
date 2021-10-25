//
// Created by olivier on 24/10/2021.
//

#ifndef ONELIFE_DATATYPE_SOUND_H
#define ONELIFE_DATATYPE_SOUND_H
typedef struct SoundSprite {
	int handle;
	int numSamples;
	int samplesPlayed;

	// true for sound sprites that are marked to never use
	// pitch and volume variance
	char noVariance;

	// floating point position of next interpolated sameple to play
	// (for variable rate playback)
	double samplesPlayedF;


	Sint16 *samples;
} SoundSprite;
#endif //ONELIFE_DATATYPE_SOUND_H
