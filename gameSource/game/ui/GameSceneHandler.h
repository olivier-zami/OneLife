//
// Created by olivier on 18/08/2022.
//

#ifndef ONE_LIFE__GAME__GRAPHICS__GAME_SCENE_HANDLER_H
#define ONE_LIFE__GAME__GRAPHICS__GAME_SCENE_HANDLER_H

#include "minorGems/graphics/Color.h"
#include "minorGems/graphics/openGL/KeyboardHandlerGL.h"
#include "minorGems/graphics/openGL/MouseHandlerGL.h"
#include "minorGems/graphics/openGL/RedrawListenerGL.h"
#include "minorGems/graphics/openGL/SceneHandlerGL.h"
#include "minorGems/graphics/openGL/ScreenGL.h"
#include "minorGems/system/Time.h"
#include "minorGems/ui/event/ActionListener.h"

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
	GameSceneHandler( ScreenGL *inScreen );

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

	ScreenGL *mScreen;

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
	virtual void fireRedraw();// implements the RedrawListener interface
	virtual void actionPerformed( GUIComponent *inTarget );// implements the ActionListener interface

	char mPaused;
	char mPausedDuringFrameBatch;
	char mLoadingDuringFrameBatch;
	unsigned int mPausedSleepTime;// reduce sleep time when user hits keys to restore responsiveness
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


#endif //ONE_LIFE__GAME__GRAPHICS__GAME_SCENE_HANDLER_H
