//
// Created by olivier on 19/08/2022.
//

#include "Application.h"

#include "../../third_party/minorGems/graphics/openGL/ScreenGL.h"
#include "../../third_party/minorGems/util/SettingsManager.h"
#include "../../third_party/minorGems/util/stringUtils.h"//stringDuplicate?

#include "../FinalMessagePage.h"
#include "../LoadingPage.h"
#include "../AutoUpdatePage.h"
#include "../ExistingAccountPage.h"
#include "../ExtendedMessagePage.h"
#include "../RebirthChoicePage.h"
#include "../SettingsPage.h"
#include "../ReviewPage.h"
#include "../TwinPage.h"
#include "../PollPage.h"
#include "../GeneticHistoryPage.h"
//#include "TestPage.h"
#include "ui/GameSceneHandler.h"
#include "LivingLifePage.h"

char countingOnVsync = false;
int idealTargetFrameRate = 60;
int targetFrameRate = idealTargetFrameRate;
char loadingDone = false;
char loadingFailedFlag = false;
char *loadingFailedMessage = NULL;
GameSceneHandler *sceneHandler;
oneLife::client::game::handler::Socket* socketHandler = nullptr;
char usingCustomServer = false;

FinalMessagePage *finalMessagePage;
ServerActionPage *getServerAddressPage;
LoadingPage *loadingPage;
AutoUpdatePage *autoUpdatePage;
LivingLifePage *livingLifePage;
ExistingAccountPage *existingAccountPage;
ExtendedMessagePage *extendedMessagePage;
RebirthChoicePage *rebirthChoicePage;
SettingsPage *settingsPage;
ReviewPage *reviewPage;
TwinPage *twinPage;
PollPage *pollPage;
GeneticHistoryPage *geneticHistoryPage;
//TestPage *testPage = NULL;
GamePage *currentGamePage = NULL;

extern ScreenGL *screen;
extern int targetFrameRate;
extern char *serverIP;

oneLife::game::Application::Application(::oneLife::game::dataType::Settings settings)
{
	this->settings = settings;
	if(!this->settings.server.ip) this->settings.server.ip = stringDuplicate( "127.0.0.1" );

	this->directory = new std::map<std::string, std::string>();
	socketHandler = new oneLife::client::game::handler::Socket();
	this->socket = socketHandler;
}

oneLife::game::Application::~Application()
{
	if(this->settings.server.ip) delete [] this->settings.server.ip;
}

const char * oneLife::game::Application::getDirectory(std::string name)
{
	return this->directory->at(name).c_str();
}

::oneLife::game::dataType::Settings oneLife::game::Application::getSettings()
{
	return this->settings;
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