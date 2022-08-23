//
// Created by olivier on 19/08/2022.
//

#include "Application.h"
#include "ui/GameSceneHandler.h"
#include "minorGems/graphics/openGL/ScreenGL.h"
#include "minorGems/util/SettingsManager.h"
#include "minorGems/util/stringUtils.h"//stringDuplicate?

char countingOnVsync = false;
int idealTargetFrameRate = 60;
int targetFrameRate = idealTargetFrameRate;
char loadingDone = false;
char loadingFailedFlag = false;
char *loadingFailedMessage = NULL;
GameSceneHandler *sceneHandler;
oneLife::client::game::handler::Socket* socketHandler = nullptr;

extern ScreenGL *screen;
extern int targetFrameRate;

oneLife::game::Application::Application()
{
	this->directory = new std::map<std::string, std::string>();
	socketHandler = new oneLife::client::game::handler::Socket();
	this->socket = socketHandler;
}

oneLife::game::Application::~Application() {}

const char * oneLife::game::Application::getDirectory(std::string name)
{
	return this->directory->at(name).c_str();
}

void oneLife::game::Application::setDirectory(std::string name, std::string path)
{
	std::pair<std::string, std::string> newDirectory = {name, path};
	this->directory->insert(newDirectory);
}

/**********************************************************************************************************************/

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