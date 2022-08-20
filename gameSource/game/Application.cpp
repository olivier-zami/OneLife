//
// Created by olivier on 19/08/2022.
//

#include "Application.h"
#include "graphics/GameSceneHandler.h"
#include "minorGems/graphics/openGL/ScreenGL.h"
#include "minorGems/util/SettingsManager.h"
#include "minorGems/util/stringUtils.h"//stringDuplicate?

char countingOnVsync = false;
GameSceneHandler *sceneHandler;
int idealTargetFrameRate = 60;
int targetFrameRate = idealTargetFrameRate;
char loadingDone = false;
char loadingFailedFlag = false;
char *loadingFailedMessage = NULL;

extern ScreenGL *screen;
extern int targetFrameRate;

char getCountingOnVsync()
{
	return countingOnVsync;
}

double getRecentFrameRate()
{
	if( screen->isPlayingBack() )
	{
		return screen->getRecordedFrameRate();
	}
	else
	{
		screen->registerActualFrameRate( sceneHandler->mLastFrameRate );
		return sceneHandler->mLastFrameRate;
	}
}

/**
 *
 * @return
 * @note from minorGems/game/platforms/SDL/gameSDL.cpp
 */
char isPaused()
{
	return sceneHandler->mPaused;
}

char isQuittingBlocked()
{
	return sceneHandler->mBlockQuitting;
}

void loadingComplete()
{
	loadingDone = true;
}

void loadingFailed( const char *inFailureMessage )
{
	loadingFailedFlag = true;

	if( loadingFailedMessage != NULL ) {
		delete [] loadingFailedMessage;
	}
	loadingFailedMessage = stringDuplicate( inFailureMessage );
}

void blockQuitting( char inNoQuitting )
{
	sceneHandler->mBlockQuitting = inNoQuitting;
}

void saveFrameRateSettings()
{
	SettingsManager::setSetting( "targetFrameRate", targetFrameRate );
	int settingValue = 0;
	if( countingOnVsync ) {
		settingValue = 1;
	}
	SettingsManager::setSetting( "countingOnVsync", settingValue );
}

void wakeUpPauseFrameRate()
{
	sceneHandler->mPausedSleepTime = 0;
}