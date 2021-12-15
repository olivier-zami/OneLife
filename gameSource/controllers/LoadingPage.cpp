#include "LoadingPage.h"

#include "OneLife/gameSource/procedures/graphics/sprites/drawMessage.h"
#include "minorGems/game/drawUtils.h"
#include "OneLife/gameSource/animationBank.h"
#include "OneLife/gameSource/categoryBank.h"
#include "OneLife/gameSource/groundSprites.h"
#include "OneLife/gameSource/liveObjectSet.h"
#include "OneLife/gameSource/objectBank.h"
#include "OneLife/gameSource/soundBank.h"
#include "OneLife/gameSource/components/banks/spriteBank.h"
#include "OneLife/gameSource/transitionBank.h"
#include "OneLife/gameSource/procedures/graphics/screens.h"
#include "OneLife/gameSource/dataTypes/signals.h"
#include "OneLife/gameSource/debug.h"

#include "OneLife/gameSource/photos.h"
#include "OneLife/gameSource/lifeTokens.h"
#include "OneLife/gameSource/fitnessScore.h"
#include "OneLife/gameSource/musicPlayer.h"

using SIGNAL = OneLife::dataValue::Signal;

extern int loadingStepBatchSize;
extern int numLoadingSteps;
extern float musicLoudness;
extern char mapPullMode;
extern char autoLogIn;
extern char *userEmail;
extern char *accountKey;

char initDone = false;
int loadingPhase = 0;
double loadingPhaseStartTime;

LoadingPage::LoadingPage()
{
	this->status.isSpritesLoaded = false;//legacy char initDone = false;
	this->status.isSoundsLoaded = false;
	this->status.isAnimationsLoaded = false;
	this->status.isObjectsLoaded = false;
	this->status.isCategoriesLoaded = false;
	this->status.isTransitionsLoaded = false;
	this->status.isGroundTexturesLoaded = false;
	this->status.isSocketSet = false;
	this->status.isInitOther = false;

	this->screen.status.showProgressBar = true;
}

LoadingPage::~LoadingPage() {}

/**********************************************************************************************************************/

void LoadingPage::handle(OneLife::dataType::UiComponent* screen)
{
	screen->label = nullptr;
	screen->draw = OneLife::game::graphic::drawLoadingScreen;
	screen->body = &(this->screen);

	if(!this->status.isSpritesLoaded)
	{
		this->loadSprites();
	}
	else if(!this->status.isSoundsLoaded)
	{
		this->loadSounds();
	}
	else if(!this->status.isAnimationsLoaded)
	{
		this->loadAnimations();
	}
	else if(!this->status.isObjectsLoaded)
	{
		this->loadObjects();
	}
	else if(!this->status.isCategoriesLoaded)
	{
		this->loadCategories();
	}
	else if(!this->status.isTransitionsLoaded)
	{
		this->loadTransitions();
	}
	else if (!this->status.isGroundTexturesLoaded)
	{
		this->loadGroundTextures();
	}
	else if(!this->status.isSocketSet)
	{
		this->loadSocketConfiguration();
	}
	else if(!this->status.isInitOther)
	{
		this->initOther();
	}
	else
	{
		this->sendSignal(SIGNAL::DONE);
	}
}

/**********************************************************************************************************************/

void LoadingPage::showProgressBar(bool showProgressBar)
{
	this->screen.status.showProgressBar = showProgressBar;
}

void LoadingPage::setCurrentPhase( const char *inPhaseName )
{
    this->screen.taskName = inPhaseName;
}

void LoadingPage::setCurrentProgress( float inProgress )
{
    this->screen.progressBar = inProgress;
}

/**********************************************************************************************************************/

void LoadingPage::loadSprites()
{
	OneLife::game::Debug::writeControllerStepInfo("LoadingPage::loadSprites()");
	this->base_step();
	char rebuilding;
	int numSprites = initSpriteBankStart( &rebuilding );
	loadingStepBatchSize = numSprites / numLoadingSteps;
	if( loadingStepBatchSize < 1 ) loadingStepBatchSize = 1;
	if( rebuilding )
	{
		this->setCurrentPhase( translate( "spritesRebuild" ) );
	}
	else this->setCurrentPhase( translate( "sprites" ) );
	this->setCurrentProgress( 0 );
	this->base_makeActive(true);

	initDone = true;
	this->status.isSpritesLoaded = true;
}

void LoadingPage::loadSounds()
{
	OneLife::game::Debug::writeControllerStepInfo("LoadingPage::loadSounds()");
	this->base_step();
	float progress;
	for( int i=0; i<loadingStepBatchSize; i++ ) {
		progress = initSpriteBankStep();
		this->setCurrentProgress( progress );
	}

	if( progress == 1.0 )
	{
		initSpriteBankFinish();
		loadingPhaseStartTime = Time::getCurrentTime();
		char rebuilding;
		int numSounds = initSoundBankStart( &rebuilding );
		if( rebuilding )
		{
			this->setCurrentPhase(translate( "soundsRebuild" ) );
		}
		else this->setCurrentPhase(translate( "sounds" ) );
		this->setCurrentProgress( 0 );
		loadingStepBatchSize = numSounds / numLoadingSteps;
		if( loadingStepBatchSize < 1 ) loadingStepBatchSize = 1;
		this->status.isSoundsLoaded = true;
		loadingPhase ++;
	}
}

void LoadingPage::loadAnimations()
{
	OneLife::game::Debug::writeControllerStepInfo("LoadingPage::loadAnimations()");
	this->base_step();
	float progress;
	for( int i=0; i<loadingStepBatchSize; i++ )
	{
		progress = initSoundBankStep();
		this->setCurrentProgress( progress );
	}

	if( progress == 1.0 )
	{
		initSoundBankFinish();
		loadingPhaseStartTime = Time::getCurrentTime();
		char rebuilding;
		int numAnimations = initAnimationBankStart( &rebuilding );
		if( rebuilding )
		{
			this->setCurrentPhase(translate( "animationsRebuild" ) );
		}
		else this->setCurrentPhase(translate("animations" ));
		this->setCurrentProgress( 0 );
		loadingStepBatchSize = numAnimations / numLoadingSteps;
		if(loadingStepBatchSize < 1) loadingStepBatchSize = 1;
		this->status.isAnimationsLoaded = true;
		loadingPhase ++;
	}

}

void LoadingPage::loadObjects()
{
	OneLife::game::Debug::writeControllerStepInfo("LoadingPage::loadObjects()");
	this->base_step();
	float progress;
	for( int i=0; i<loadingStepBatchSize; i++ )
	{
		progress = initAnimationBankStep();
		this->setCurrentProgress( progress );
	}

	if( progress == 1.0 )
	{
		initAnimationBankFinish();
		//printf( "Finished loading animation bank in %f sec\n", Time::getCurrentTime() - loadingPhaseStartTime );
		loadingPhaseStartTime = Time::getCurrentTime();
		char rebuilding;
		int numObjects = initObjectBankStart(&rebuilding, true, true);
		if (rebuilding) {
			this->setCurrentPhase(translate("objectsRebuild"));
		} else this->setCurrentPhase(translate("objects"));
		this->setCurrentProgress(0);
		loadingStepBatchSize = numObjects / numLoadingSteps;
		if (loadingStepBatchSize < 1) loadingStepBatchSize = 1;
		this->status.isObjectsLoaded = true;
		loadingPhase++;
	}
}

void LoadingPage::loadCategories()
{
	OneLife::game::Debug::writeControllerStepInfo("LoadingPage::loadCategories()");
	this->base_step();
	float progress;
	for( int i=0; i<loadingStepBatchSize; i++ )
	{
		progress = initObjectBankStep();
		this->setCurrentProgress( progress );
	}

	if( progress == 1.0 )
	{
		initObjectBankFinish();
		//printf( "Finished loading object bank in %f sec\n", Time::getCurrentTime() - loadingPhaseStartTime );
		loadingPhaseStartTime = Time::getCurrentTime();
		char rebuilding;
		int numCats = initCategoryBankStart( &rebuilding );
		if( rebuilding )
		{
			this->setCurrentPhase(translate( "categoriesRebuild" ) );
		}
		else this->setCurrentPhase(translate( "categories" ) );
		this->setCurrentProgress( 0 );
		loadingStepBatchSize = numCats / numLoadingSteps;
		if( loadingStepBatchSize < 1 ) loadingStepBatchSize = 1;
		this->status.isCategoriesLoaded = true;
		loadingPhase ++;
	}

}

void LoadingPage::loadTransitions()
{
	OneLife::game::Debug::writeControllerStepInfo("LoadingPage::loadTransitions()");
	this->base_step();
	float progress;
	for( int i=0; i<loadingStepBatchSize; i++ ) {
		progress = initCategoryBankStep();
		this->setCurrentProgress( progress );
	}

	if( progress == 1.0 )
	{
		initCategoryBankFinish();
		//printf( "Finished loading category bank in %f sec\n", Time::getCurrentTime() - loadingPhaseStartTime );
		loadingPhaseStartTime = Time::getCurrentTime();
		char rebuilding;

		// true to auto-generate concrete transitions
		// for all abstract category transitions
		int numTrans = initTransBankStart(
				&rebuilding,
				true,
				true,
				true,
				true );

		if( rebuilding )
		{
			this->setCurrentPhase(translate( "transitionsRebuild" ) );
		}
		else this->setCurrentPhase(translate( "transitions" ) );
		this->setCurrentProgress( 0 );
		loadingStepBatchSize = numTrans / numLoadingSteps;
		if( loadingStepBatchSize < 1 ) loadingStepBatchSize = 1;
		this->status.isTransitionsLoaded = true;
		loadingPhase ++;
	}

}

void LoadingPage::loadGroundTextures()
{
	OneLife::game::Debug::writeControllerStepInfo("LoadingPage::loadGroundTextures()");
	this->base_step();
	float progress;
	for( int i=0; i<loadingStepBatchSize; i++ )
	{
		progress = initTransBankStep();
		this->setCurrentProgress( progress );
	}

	if( progress == 1.0 )
	{
		initTransBankFinish();
		//printf( "Finished loading transition bank in %f sec\n", Time::getCurrentTime() - loadingPhaseStartTime );
		loadingPhaseStartTime = Time::getCurrentTime();
		this->setCurrentPhase(translate( "groundTextures" ) );
		this->setCurrentProgress( 0 );
		initGroundSpritesStart();
		loadingStepBatchSize = 1;
		this->status.isGroundTexturesLoaded = true;
		loadingPhase ++;
	}

}

void LoadingPage::loadSocketConfiguration()
{
	OneLife::game::Debug::writeControllerStepInfo("LoadingPage::loadSocketConfiguration()");
	this->base_step();
	float progress;
	for( int i=0; i<loadingStepBatchSize; i++ )
	{
		progress = initGroundSpritesStep();
		this->setCurrentProgress( progress );
	}

	if( progress == 1.0 )
	{
		initGroundSpritesFinish();
		//printf( "Finished loading ground sprites in %f sec\n", Time::getCurrentTime() - loadingPhaseStartTime );
		loadingPhaseStartTime = Time::getCurrentTime();
		initLiveObjectSet();
		//serverIP = SettingsManager::getStringSetting("customServerAddress" );
		//if( serverIP == NULL ) { serverIP = stringDuplicate( "127.0.0.1" ); }
		//serverPort = SettingsManager::getIntSetting("customServerPort", 8005 );
		this->status.isSocketSet = true;
		loadingPhase ++;
	}

}

void LoadingPage::initOther()
{
	OneLife::game::Debug::writeControllerStepInfo("LoadingPage::initOther()");
	this->base_step();
	// NOW game engine can start measuring frame rate
	loadingComplete();
	initEmotion();
	initPhotos();
	initLifeTokens();
	initFitnessScore();
	initMusicPlayer();
	setMusicLoudness( musicLoudness );
	mapPullMode = SettingsManager::getIntSetting( "mapPullMode", 0 );
	autoLogIn = SettingsManager::getIntSetting( "autoLogIn", 0 );
	if( userEmail == NULL || accountKey == NULL ){autoLogIn = false;}
	this->status.isInitOther = true;
}

/**********************************************************************************************************************/

void LoadingPage::draw( doublePair inViewCenter, double inViewSize ){}

void LoadingPage::step(){}