//
// Created by olivier on 07/11/2021.
//

#ifndef ONELIFE_GAME_ENGINE_AUDIORENDERER_H
#define ONELIFE_GAME_ENGINE_AUDIORENDERER_H

#include <cctype>
#include "OneLife/gameSource/game.h"

namespace OneLife::game
{
	class AudioRenderer
	{
		public:
			AudioRenderer();
			~AudioRenderer();
	};
}

void audioCallback( void *inUserData, Uint8 *inStream, int inLengthToFill );
void setSoundSpriteRateRange( double inMin, double inMax );
void setSoundSpriteVolumeRange( double inMin, double inMax );
void setSoundLoudness( float inLoudness );
double pickRandomRate();

void playSoundSpriteInternal(
		SoundSpriteHandle inHandle, double inVolumeTweak,
		double inStereoPosition,
		double inForceVolume = -1,
		double inForceRate = -1 );

void playSoundSprite(
		SoundSpriteHandle inHandle,
		double inVolumeTweak = 1.0,
		double inStereoPosition  = 0.5 );

void playSoundSprite(
		int inNumSprites,
		SoundSpriteHandle *inHandles,
		double *inVolumeTweaks,
		double *inStereoPositions );

void freeSoundSprite( SoundSpriteHandle inHandle );
int getSampleRate();
void setSoundPlaying( char inPlaying );
void lockAudio();
void unlockAudio();
char isSoundRunning();
void resumePlayingSoundSprites();
SoundSpriteHandle loadSoundSprite( const char *inAIFFFileName );
SoundSpriteHandle loadSoundSprite( const char *inFolderName,
								   const char *inAIFFFileName );
SoundSpriteHandle setSoundSprite( int16_t *inSamples, int inNumSamples );
void toggleVariance( SoundSpriteHandle inHandle, char inNoVariance );
void setMaxTotalSoundSpriteVolume( double inMaxTotal,
								   double inCompressionFraction );
int16_t *load16BitMonoSound( int *outNumSamples, int *outSampleRate );
void fadeSoundSprites( double inFadeSeconds );
void resumePlayingSoundSprites();
void setMaxSimultaneousSoundSprites( int inMaxCount );

#endif //ONELIFE_GAME_ENGINE_AUDIORENDERER_H
