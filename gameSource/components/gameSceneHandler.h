//
// Created by olivier on 24/10/2021.
//

#ifndef ONELIFE_GAMESCENEHANDLER_H
#define ONELIFE_GAMESCENEHANDLER_H

#include "minorGems/graphics/openGL/SceneHandlerGL.h"
#include "minorGems/graphics/openGL/MouseHandlerGL.h"
#include "minorGems/graphics/openGL/KeyboardHandlerGL.h"
#include "minorGems/graphics/openGL/RedrawListenerGL.h"
#include "minorGems/graphics/openGL/ScreenGL.h"
#include "minorGems/ui/event/ActionListener.h"
#include "minorGems/ui/GUIComponent.h"
#include "minorGems/graphics/Color.h"
#include "minorGems/system/Time.h"
#include "minorGems/game/doublePair.h"
#include "minorGems/game/drawUtils.h"
#include "minorGems/game/gameGraphics.h"
#include "minorGems/game/game.h"//Uint8
#include "OneLife/gameSource/application.h"


class GameSceneHandler :
		public SceneHandlerGL, public MouseHandlerGL, public KeyboardHandlerGL,
		public RedrawListenerGL, public ActionListener  {

public:

	/**
	 * Constructs a sceen handler.
	 *
	 * @param inScreen the screen to interact with.
	 *   Must be destroyed by caller after this class is destroyed.
	 */
	GameSceneHandler( OneLife::game::Application *inScreen );

	virtual ~GameSceneHandler();



	/**
	 * Executes necessary init code that reads from files.
	 *
	 * Must be called before using a newly-constructed GameSceneHandler.
	 *
	 * This call assumes that the needed files are in the current working
	 * directory.
	 */
	void initFromFiles();



	OneLife::game::Application *mScreen;








	// implements the SceneHandlerGL interface
	virtual void drawScene();

	// implements the MouseHandlerGL interface
	virtual void mouseMoved( int inX, int inY );
	virtual void mouseDragged( int inX, int inY );
	virtual void mousePressed( int inX, int inY );
	virtual void mouseReleased( int inX, int inY );

	// implements the KeyboardHandlerGL interface
	virtual char isFocused() {
		// always focused
		return true;
	}
	virtual void keyPressed( unsigned char inKey, int inX, int inY );
	virtual void specialKeyPressed( int inKey, int inX, int inY );
	virtual void keyReleased( unsigned char inKey, int inX, int inY );
	virtual void specialKeyReleased( int inKey, int inX, int inY );

	// implements the RedrawListener interface
	virtual void fireRedraw();


	// implements the ActionListener interface
	virtual void actionPerformed( GUIComponent *inTarget );


	char mPaused;
	char mPausedDuringFrameBatch;
	char mLoadingDuringFrameBatch;

	// reduce sleep time when user hits keys to restore responsiveness
	unsigned int mPausedSleepTime;


	char mBlockQuitting;

	double mLastFrameRate;


protected:

	timeSec_t mStartTimeSeconds;


	char mPrintFrameRate;
	unsigned long mNumFrames;
	unsigned long mFrameBatchSize;
	double mFrameBatchStartTimeSeconds;



	Color mBackgroundColor;


};

void takeScreenShot();
void warpMouseToScreenPos( int inX, int inY );
void warpMouseToCenter( int *outNewMouseX, int *outNewMouseY );
void cleanUpAtExit();
void warpMouseToWorldPos( float inX, float inY );
char isCommandKeyDown();
void specialKeyUp( int inKey );
void freeDemoCodePanel();
void worldToScreen( float inX, float inY, int *outX, int *outY );
void screenToWorld( int inX, int inY, float *outX, float *outY );
doublePair getViewCenterPosition();
void setViewCenterPosition( float inX, float inY );
void redoDrawMatrix();
void getScreenDimensions( int *outWidth, int *outHeight );
void warpMouseToCenter( int *outNewMouseX, int *outNewMouseY );
double game_getCurrentTime();//defined in game.cpp
char isPaused();//defined in game.cpp
void freeDrawString();//defined in gme.cpp
char isDemoCodePanelShowing();//defined in game.cpp
void freeFrameDrawer();
Image *getScreenRegion( double inX, double inY, double inWidth, double inHeight );
Image *getScreenRegionRaw(int inStartX, int inStartY, int inWidth, int inHeight );
Image *getScreenRegionInternal(
		int inStartX, int inStartY, int inWidth, int inHeight,
		char inForceManual = false );
 const char *translate( const char *inTranslationKey );//defined in game.cpp
void saveFrameRateSettings();
void audioCallback( void *inUserData, Uint8 *inStream, int inLengthToFill );
#endif //ONELIFE_GAMESCENEHANDLER_H
