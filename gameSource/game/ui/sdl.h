//
// Created by olivier on 18/08/2022.
//

//TODO: move in openLife/graphics/sdl

#ifndef ONE_LIFE__GAME__GRAPHICS__SDL_H
#define ONE_LIFE__GAME__GRAPHICS__SDL_H

#include "minorGems/game/game.h"
#include "minorGems/graphics/Image.h"
#include "minorGems/network/web/WebRequest.h"

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

typedef struct WebRequestRecord {
	int handle;
	WebRequest *request;
} WebRequestRecord;

void audioCallback( void *inUserData, Uint8 *inStream, int inLengthToFill );
void cleanUpAtExit();
void freeFrameDrawer();
void freeHintedBuffers();
Image *getScreenRegion( double inX, double inY,double inWidth, double inHeight );
Image *getScreenRegionRaw(int inStartX, int inStartY, int inWidth, int inHeight );
void pauseGame();
void redoDrawMatrix();
void setViewCenterPosition( float inX, float inY );
void setViewSize( float inSize );
void takeScreenShot();

#endif //ONE_LIFE__GAME__GRAPHICS__SDL_H
