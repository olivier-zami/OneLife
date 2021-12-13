//
// Created by olivier on 09/12/2021.
//

#include "configurationScreen.h"

#include "OneLife/gameSource/configFunctions.h"
#include "OneLife/gameSource/procedures/graphics/screens.h"
#include "OneLife/gameSource/dataTypes/signals.h"
#include "OneLife/gameSource/debug.h"

using SIGNAL = OneLife::dataType::Signal;

OneLife::game::ConfigurationScreen::ConfigurationScreen()
{
	this->status.isInitDemoMode = false;
	this->status.isEnvironmentChecked = false;
	this->status.isPlaybackMode = false;

	this->global.demoMode = nullptr;
	this->global.writeFailed = nullptr;
}

OneLife::game::ConfigurationScreen::~ConfigurationScreen() {}

/**********************************************************************************************************************/

void OneLife::game::ConfigurationScreen::handle(OneLife::dataType::UiComponent* screen)
{
	screen->label = nullptr;
	screen->draw = OneLife::game::graphic::drawInitializingScreen;
	screen->body = &(this->screen);

	if(!this->status.isInitDemoMode)
	{
		this->initDemoMode();
	}
	else if(!this->status.isEnvironmentChecked)
	{
		this->checkEnvironment();
	}
	else
	{
		this->sendSignal(SIGNAL::DONE);
	}
}

void OneLife::game::ConfigurationScreen::handleDemoMode(char* demoMode)
{
	this->global.demoMode = demoMode;
}

void OneLife::game::ConfigurationScreen::handleWriteFailed(char* writeFailed)
{
	this->global.writeFailed = writeFailed;
}

void OneLife::game::ConfigurationScreen::handleMeasureFrameRate(char* measureFrameRate)
{
	this->global.measureFrameRate = measureFrameRate;
}

/**********************************************************************************************************************/

void OneLife::game::ConfigurationScreen::setPlaybackMode(bool playbackMode)
{
	this->status.isPlaybackMode = playbackMode;
}

/**********************************************************************************************************************/

void OneLife::game::ConfigurationScreen::initDemoMode()
{
	OneLife::game::Debug::writeControllerStepInfo("OneLife::game::ConfigurationScreen::initDemoMode");
	if(!isDemoCodePanelShowing())
	{
		*(this->global.demoMode) = false;// stop demo mode when panel done
		//mScreen->addMouseHandler( this );
		//mScreen->addKeyboardHandler( this );
		//screen->startRecordingOrPlayback();
	}
	this->status.isInitDemoMode = true;
}

void OneLife::game::ConfigurationScreen::checkEnvironment()
{
	OneLife::game::Debug::writeControllerStepInfo("OneLife::game::ConfigurationScreen::checkEnvironment");
	bool isErrorCatch = false;
	if(!isErrorCatch && *(this->global.writeFailed))
	{
		/*
		TODO: send signal for writing errorMessage and create errorMessageScreen controller
		drawString( translate( "writeFailed" ), true );
		*/
		isErrorCatch = true;
	}
	this->status.isEnvironmentChecked = true;
}

void OneLife::game::ConfigurationScreen::initUnknownFeature()
{
	/*TODO: implement this code from OneLife/gameSource/application.cpp
	if( !this->isPlayingBack() && measureFrameRate )
		{
			if(this->idScreen !=3){printf("\n===>!isPlayingBack && measureFrameRate");this->idScreen = 3;}
			if( !measureRecorded )
			{
				this->useFrameSleep( false );
			}

			if( numFramesSkippedBeforeMeasure < numFramesToSkipBeforeMeasure )
			{
				numFramesSkippedBeforeMeasure++;

				drawString( translate( "measuringFPS" ), true );
			}
			else if( ! startMeasureTimeRecorded )
			{
				startMeasureTime = Time::getCurrentTime();
				startMeasureTimeRecorded = true;

				drawString( translate( "measuringFPS" ), true );
			}
			else
			{

				numFramesMeasured++;

				double totalTime = Time::getCurrentTime() - startMeasureTime;

				double timePerFrame = totalTime / ( numFramesMeasured );

				double frameRate = 1 / timePerFrame;


				int closestTargetFrameRate = 0;
				double closestFPSDiff = 9999999;

				for( int i=0; i<possibleFrameRates.size(); i++ ) {

					int v = possibleFrameRates.getElementDirect( i );

					double diff = fabs( frameRate - v );

					if( diff < closestFPSDiff ) {
						closestTargetFrameRate = v;
						closestFPSDiff = diff;
					}
				}

				double overAllowFactor = 1.05;



				if( numFramesMeasured > 10 &&
					frameRate > overAllowFactor * closestTargetFrameRate ) {

					secondsToMeasure = warningSecondsToMeasure;
				}
				else {
					secondsToMeasure = noWarningSecondsToMeasure;
				}

				if( totalTime <= secondsToMeasure ) {
					char *message = autoSprintf( "%s\n%0.2f\nFPS",
							translate( "measuringFPS" ),
							frameRate );


					drawString( message, true );

					delete [] message;
				}

				if( totalTime > secondsToMeasure ) {

					if( ! measureRecorded ) {

						if( targetFrameRate == idealTargetFrameRate ) {
							// not invoking halfFrameRate

							AppLog::infoF( "Measured frame rate = %f fps\n",
									frameRate );
							AppLog::infoF(
									"Closest possible frame rate = %d fps\n",
									closestTargetFrameRate );

							if( frameRate >
								overAllowFactor * closestTargetFrameRate ) {

								AppLog::infoF(
										"Vsync to enforce closested frame rate of "
										"%d fps doesn't seem to be in effect.\n",
										closestTargetFrameRate );

								AppLog::infoF(
										"Will sleep each frame to enforce desired "
										"frame rate of %d fps\n",
										idealTargetFrameRate );

								targetFrameRate = idealTargetFrameRate;

								this->useFrameSleep( true );
								countingOnVsync = false;
							}
							else {
								AppLog::infoF(
										"Vsync seems to be enforcing an allowed frame "
										"rate of %d fps.\n", closestTargetFrameRate );

								targetFrameRate = closestTargetFrameRate;

								this->useFrameSleep( false );
								countingOnVsync = true;
							}
						}
						else {
							// half frame rate must be set

							AppLog::infoF(
									"User has halfFrameRate set, so we're going "
									"to manually sleep to enforce a target "
									"frame rate of %d fps.\n", targetFrameRate );
							this->useFrameSleep( true );
							countingOnVsync = false;
						}


						this->setFullFrameRate( targetFrameRate );
						measureRecorded = true;
					}

					if( !countingOnVsync ) {
						// show warning message
						char *message =
								autoSprintf( "%s\n%s\n\n%s\n\n\n%s",
										translate( "vsyncWarning" ),
										translate( "vsyncWarning2" ),
										translate( "vsyncWarning3" ),
										translate( "vsyncContinueMessage" ) );
						drawString( message, true );

						delete [] message;
					}
					else {
						// auto-save it now
						saveFrameRateSettings();
						this->startRecordingOrPlayback();
						measureFrameRate = false;
					}
				}
			}
			//return;
		}
	*/
}

void OneLife::game::ConfigurationScreen::initUnknownFeature1()
{
	/*TODO: implement this code (from OneLife/gameSource/application.cpp) => send FailedSignal ?
	else if( loadingFailedFlag ) {
			if(this->idScreen !=5){printf("\n===>loadingFailedFlag");this->idScreen = 5;}
			drawString( loadingFailedMessage, true );
		}
	*/
}

void OneLife::game::ConfigurationScreen::initUnknownFeature2()
{
	/*
	if( !writeFailed && !loadingFailedFlag && !frameDrawerInited )
		{
			if(this->idScreen !=6){printf("\n===>!writeFailed && !loadingFailedFlag && !frameDrawerInited");this->idScreen = 6;}
			drawString( translate( "loading" ), true );
			int readCursorMode = SettingsManager::getIntSetting( "cursorMode", -1 );
			if( readCursorMode < 0 ) {
				// never set before

				// check if we are ultrawidescreen
				char ultraWide = false;

				const SDL_VideoInfo* currentScreenInfo = SDL_GetVideoInfo();

				int currentW = currentScreenInfo->current_w;
				int currentH = currentScreenInfo->current_h;

				double aspectRatio = (double)currentW / (double)currentH;

				// give a little wiggle room above 16:9
				// ultrawide starts at 21:9
				if( aspectRatio > 18.0 / 9.0 ) {
					ultraWide = true;
				}

				if( ultraWide ) {
					// drawn cursor, because system native cursor
					// is off-target on ultrawide displays

					setCursorMode( 1 );

					double startingScale = 1.0;

					int forceBigPointer =
							SettingsManager::getIntSetting( "forceBigPointer", 0 );
					if( forceBigPointer ||
						screenWidth > 1920 || screenHeight > 1080 ) {

						startingScale *= 2;
					}
					setEmulatedCursorScale( startingScale );
				}
			}
			else {
				setCursorMode( readCursorMode );

				double readCursorScale =
						SettingsManager::
						getDoubleSetting( "emulatedCursorScale", -1.0 );

				if( readCursorScale >= 1 ) {
					setEmulatedCursorScale( readCursorScale );
				}
			}



			frameDrawerInited = true;

			// this is a good time, a while after launch, to do the post
			// update step
			postUpdate();

		}//step2
	*/
}