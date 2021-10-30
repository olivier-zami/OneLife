//
// Created by olivier on 29/10/2021.
//

#include "application.h"

OneLife::game::Application::Application(
		int inWide, int inHigh, char inFullScreen,
		char inDoNotChangeNativeResolution,
		unsigned int inMaxFrameRate,
		char inRecordEvents,
		const char *inCustomRecordedGameData,
		const char *inHashSalt,
		const char *inWindowName,
		KeyboardHandlerGL *inKeyHandler,
		MouseHandlerGL *inMouseHandler,
		SceneHandlerGL *inSceneHandler):

		mWide( inWide ), mHigh( inHigh ),
		mForceAspectRatio( false ),
		mForceSpecifiedDimensions( false ),
		mDoNotChangeNativeResolution( inDoNotChangeNativeResolution ),
		mImageWide( inWide ), mImageHigh( inHigh ),
		mFullScreen( inFullScreen ),
		mMaxFrameRate( inMaxFrameRate ),
		mUseFrameSleep( true ),
		mFullFrameRate( inMaxFrameRate ),
		m2DMode( false ),
		mViewPosition( new Vector3D( 0, 0, 0 ) ),
		mViewOrientation( new Angle3D( 0, 0, 0 ) ),
		mMouseHandlerVector( new SimpleVector<MouseHandlerGL*>() ),
		mKeyboardHandlerVector( new SimpleVector<KeyboardHandlerGL*>() ),
		mSceneHandlerVector( new SimpleVector<SceneHandlerGL*>() ),
		mRedrawListenerVector( new SimpleVector<RedrawListenerGL*>() )
{
	this->screenGL = new ScreenGL(
		inWide,
		inHigh,
		inFullScreen,
		inDoNotChangeNativeResolution,
		inMaxFrameRate,
		inRecordEvents,
		inCustomRecordedGameData,
		inHashSalt,
		inWindowName,
		inKeyHandler,
		inMouseHandler,
		inSceneHandler);
}

OneLife::game::Application::~Application()
{
	this->screenGL->~ScreenGL();
}

/**
* Gets data read from a recorded game file.
*
* Not destroyed by caller.
*/
const char* OneLife::game::Application::getCustomRecordedGameData()
{
	return this->screenGL->getCustomRecordedGameData();
}

/**
* True if currently in playback mode.
*/
char OneLife::game::Application::isPlayingBack()
{
	return this->screenGL->isPlayingBack();
}

/**
* Returns an estimate of playback fraction complete.
*/
float OneLife::game::Application::getPlaybackDoneFraction()
{
	return this->screenGL->getPlaybackDoneFraction();
}

/**
* Returns whether playback display is on or off.
*/
char OneLife::game::Application::shouldShowPlaybackDisplay()
{
	return this->screenGL->shouldShowPlaybackDisplay();
}

/**
* True if minimized.
*/
char OneLife::game::Application::isMinimized()
{
	return this->screenGL->isMinimized();
}


/**
* Starts the GLUT main loop.
*
* Note that this function call never returns.
*/
void OneLife::game::Application::start()
{
	this->screenGL->start();
}

// can avoid recording/playback during certain "front matter"
// activities like menu navigation
// (has no effect if no recording or playback pending)
void OneLife::game::Application::startRecordingOrPlayback()
{
	this->screenGL->startRecordingOrPlayback();
}

// our current max
// can be changed with speed control keys, if enabled
void OneLife::game::Application::setMaxFrameRate( unsigned int inMaxFrameRate )
{
	this->screenGL->setMaxFrameRate(inMaxFrameRate);
}

// our absolute max
// speed control key adjustments are relative to this full rate
void OneLife::game::Application::setFullFrameRate( unsigned int inFullFrameRate )
{
	this->screenGL->setFullFrameRate(inFullFrameRate);
}


unsigned int OneLife::game::Application::getMaxFramerate()
{
	return this->screenGL->getMaxFramerate();
}

// set to true to enable frame sleeping to enforce max frame rate
// (sleeping is default)
// Otherwise, if false, no max is enforced (though we may be able
// to depend on vsync to enforce it for us)
void OneLife::game::Application::useFrameSleep( char inUse )
{
	this->screenGL->useFrameSleep(inUse);
}



// should ^ and % be allowed to slowdown and resume normal speed
// during event playback?
// setMaxFrameRate also can be used to create a slowdown, but
// ^ and % keys are not passed through during playback
void OneLife::game::Application::allowSlowdownKeysDuringPlayback( char inAllow )
{
	this->screenGL->allowSlowdownKeysDuringPlayback(inAllow);
}


// enables rand seed to be recorded and played back with
// event playback
unsigned int OneLife::game::Application::getRandSeed()
{
	return this->screenGL->getRandSeed();
}


// enables Time::timeSec() values to be recorded and played back
timeSec_t OneLife::game::Application::getTimeSec()
{
	return this->screenGL->getTimeSec();
}

double OneLife::game::Application::getCurrentTime()
{
	return this->screenGL->getCurrentTime();
}


// to save FPS in recording files
void OneLife::game::Application::registerActualFrameRate( double inFrameRate )
{
	this->screenGL->registerActualFrameRate(inFrameRate);
}

// these can be played back
double OneLife::game::Application::getRecordedFrameRate()
{
	return this->screenGL->getRecordedFrameRate();
}

// sets mapping so that when inFromKey is pressed, an
// event for inToKey is generated instead
void OneLife::game::Application::setKeyMapping( unsigned char setKeyMapping,
					unsigned char inToKey )
{
	this->screenGL->setKeyMapping(setKeyMapping, inToKey);
}


// turns key mapping on (default) or off
void OneLife::game::Application::toggleKeyMapping( char inMappingOn )
{
	this->screenGL->toggleKeyMapping(inMappingOn);
}



// passes in a web event to be (possibly) added to the current
// game recording
// type encodes :
//    0 = normal request step completed (NULL body)
//    1 = request done step completed (NULL body)
//   -1 = error-returning request step completed (NULL body)
//    2 = result-fetch step completed (non-NULL body)
//   >2 = web progress event, showing number of bytes fetched
//        (NULL body)
void OneLife::game::Application::registerWebEvent( int inHandle,
					   int inType,
					   const char *inBodyString,
		// -1 means use string length
		// provide length for binary bodies
					   int inBodyLength)
{
	this->screenGL->registerWebEvent(inHandle, inType, inBodyString, inBodyLength);
}


// gets the type of the next pending web event (from playback)
// if the event has no result body, this call removes the event
// from the list.
// If it has a result body, getWebEventResultBody must be called.
int OneLife::game::Application::getWebEventType( int inHandle )
{
	return this->screenGL->getWebEventType(inHandle);
}

// gets a recorded event body text from the current frame matching
//   inHandle
// result NULL if there is no event matching inHandle
// result destroyed by caller.
//
// This call removes the web event from the list of pending events.
char* OneLife::game::Application::getWebEventResultBody( int inHandle,
							 int *outSize)
{
	return this->screenGL->getWebEventResultBody(inHandle, outSize);
}

// passes in a socket event to be (possibly) added to the current
// game recording
// type encodes :
//    0 = send with number sent (NULL body)
//    1 = send resulting in error (NULL body)
//    2 = read with number read (non-NULL body if inNumBodyBytes != 0)
//    3 = read resulting in error (NULL body)
void OneLife::game::Application::registerSocketEvent( int inHandle,
						  int inType,
						  int inNumBodyBytes,
						  unsigned char *inBodyBytes)
{
	this->screenGL->registerSocketEvent(inHandle, inType, inNumBodyBytes, inBodyBytes);
}


// gets the type of the next pending socket event (from playback)
// if the event has no body bytes
// (type 0, 1, or 3, OR type 2 with 0-length body),
// this call removes the event from the list.
//
// In case of type 2 with outNumBodyBytes > 0,
// getWebEventResultBody must be called.
//
//
void OneLife::game::Application::getSocketEventTypeAndSize( int inHandle,
								int *outType,
								int *outNumBodyBytes )
{
	this->screenGL->getSocketEventTypeAndSize(inHandle, outType, outNumBodyBytes);
}

// gets a recorded socket event body bytes from the current
// frame matching inHandle
// result NULL if there is no event matching inHandle
// result destroyed by caller.
//
// return value has length as set by last getSocketEventTypeAndSize
// call
//
// This call removes the socket event from the list of pending events.
unsigned char* OneLife::game::Application::getSocketEventBodyBytes( int inHandle )
{
	return this->screenGL->getSocketEventBodyBytes(inHandle);
}


void OneLife::game::Application::registerAsyncFileDone( int inHandle )
{
	this->screenGL->registerAsyncFileDone(inHandle);
}

char OneLife::game::Application::getAsyncFileDone( int inHandle )
{
	return this->screenGL->getAsyncFileDone(inHandle);
}



/**
 * Switches to 2D mode, where no view transforms are applied
 *
 * Must be called before start();
 */
void OneLife::game::Application::switchTo2DMode()
{
	this->screenGL->switchTo2DMode();
}



/**
 * Moves the view position.
 *
 * @param inPositionChange directional vector describing motion.
 *   Must be destroyed by caller.
 */
void OneLife::game::Application::moveView( Vector3D *inPositionChange )
{
	this->screenGL->moveView(inPositionChange);
}


/**
 * Rotates the view.
 *
 * @param inOrientationChange angle to rotate view by.
 *   Must be destroyed by caller.
 */
void OneLife::game::Application::rotateView( Angle3D *inOrientationChange )
{
	this->screenGL->rotateView(inOrientationChange);
}


/**
 * Gets the angle of the current view direction.
 *
 * @return the angle of the current view direction.
 *   Not a copy, so shouldn't be modified or destroyed by caller.
 */
Angle3D* OneLife::game::Application::getViewOrientation()
{
	return this->screenGL->getViewOrientation();
}


/**
 * Gets the current view position.
 *
 * @return the position of the current view.
 *  Must be destroyed by caller.
 */
Vector3D* OneLife::game::Application::getViewPosition()
{
	return this->screenGL->getViewPosition();
}


/**
 * Sets the current view position.
 *
 * @param inPosition the new position.
 *   Must be destroyed by caller.
 */
void OneLife::game::Application::setViewPosition( Vector3D *inPosition )
{
	this->screenGL->setViewPosition(inPosition);
}


/**
 * Gets the width of the screen.
 *
 * @return the width of the screen, in pixels.
 */
int OneLife::game::Application::getWidth()
{
	return this->screenGL->getWidth();
}



/**
 * Gets the height of the screen.
 *
 * @return the height of the screen, in pixels.
 */
int OneLife::game::Application::getHeight()
{
	return this->screenGL->getHeight();
}



/**
 * Switches into full screen mode.
 *
 * Use changeWindowSize to switch back out of full screen mode.
 */
void OneLife::game::Application::setFullScreen()
{
	this->screenGL->setFullScreen();
}



/**
 * Sets the size of the viewport image in the window.
 *
 * Defaults to window size.
 *
 * Must be called before screen is started.
 *
 * @param inWidth, inHeight the new dimensions, in pixels.
 */
void OneLife::game::Application::setImageSize( int inWidth, int inHeight )
{
	this->screenGL->setImageSize(inWidth, inHeight);
}



/**
 * Gets the width of the viewport image.
 *
 * @return the width of the viewport, in pixels.
 */
int OneLife::game::Application::getImageWidth()
{
	return this->screenGL->getImageWidth();
}



/**
 * Gets the height of the viewport image.
 *
 * @return the height of the viewport, in pixels.
 */
int OneLife::game::Application::getImageHeight()
{
	return this->screenGL->getImageHeight();
}



/**
 * Change the window size.
 *
 * @param inWidth, inHeight the new dimensions, in pixels.
 */
void OneLife::game::Application::changeWindowSize( int inWidth, int inHeight )
{
	this->screenGL->changeWindowSize(inWidth, inHeight);
}



/**
 * Adds a mouse handler.
 *
 * @param inHandler the handler to add  Must
 *   be destroyed by caller.
 *
 * Must not be called after calling start().
 */
void OneLife::game::Application::addMouseHandler( MouseHandlerGL *inHandler )
{
	this->screenGL->addMouseHandler(inHandler);
}



/**
 * Removes a mouse handler.
 *
 * @param inHandler the handler to remove.  Must
 *   be destroyed by caller.
 *
 * Must not be called after calling start().
 */
void OneLife::game::Application::removeMouseHandler( MouseHandlerGL *inHandler )
{
	this->screenGL->removeMouseHandler(inHandler);
}


/**
 * Adds a keyboard handler.
 *
 * @param inHandler the handler to add  Must
 *   be destroyed by caller.
 * @param inFirstHandler true to put this handler ahead of
 *   existing handlers in the list.
 *
 * Must not be called after calling start().
 */
void OneLife::game::Application::addKeyboardHandler( KeyboardHandlerGL *inHandler,
						 char inFirstHandler)
{
	this->screenGL->addKeyboardHandler(inHandler, inFirstHandler);
}



/**
 * Removes a keyboard handler.
 *
 * @param inHandler the handler to remove.  Must
 *   be destroyed by caller.
 *
 * Must not be called after calling start().
 */
void OneLife::game::Application::removeKeyboardHandler( KeyboardHandlerGL *inHandler )
{
	this->screenGL->removeKeyboardHandler(inHandler);
}


/**
 * Adds a scene handler.
 *
 * @param inHandler the handler to add  Must
 *   be destroyed by caller.
 *
 * Must not be called after calling start().
 */
void OneLife::game::Application::addSceneHandler( SceneHandlerGL *inHandler )
{
	this->screenGL->addSceneHandler(inHandler);
}



/**
 * Removes a scene handler.
 *
 * @param inHandler the handler to remove.  Must
 *   be destroyed by caller.
 *
 * Must not be called after calling start().
 */
void OneLife::game::Application::removeSceneHandler( SceneHandlerGL *inHandler )
{
	this->screenGL->removeSceneHandler(inHandler);
}



/**
 * Adds a redraw listener.
 *
 * @param inListener the listener to add.  Must
 *   be destroyed by caller.
 *
 * Must not be called after calling start().
 */
void OneLife::game::Application::addRedrawListener( RedrawListenerGL *inListener )
{
	this->screenGL->addRedrawListener(inListener);
}


/**
 * Removes a redraw listener.
 *
 * @param inListener the listener to remove.  Must
 *   be destroyed by caller.
 *
 * Must not be called after calling start().
 */
void OneLife::game::Application::removeRedrawListener( RedrawListenerGL *inListener )
{
	this->screenGL->removeRedrawListener(inListener);
}



/**
 * Applies the current view matrix transformation
 * to the matrix at the top of the GL_PROJECTION stack.
 */
void OneLife::game::Application::applyViewTransform()
{
	return this->screenGL->applyViewTransform();
}



void OneLife::game::Application::obscureRecordedNumericTyping( char inObscure,
								   char inCharToRecordInstead ) {
	mObscureRecordedNumericTyping = inObscure;
	mCharToRecordInstead = inCharToRecordInstead;
}


/**
 * Access the various handlers.
 */
//KeyboardHandlerGL *getKeyHandler();
//MouseHandlerGL *getMouseHandler();
//SceneHandlerGL *getSceneHandler();


char OneLife::game::Application::isLastMouseButtonRight() {
	return mLastMouseButtonRight;
}

// FOVMOD NOTE:  Change 2/3 - Take these lines during the merge process
int OneLife::game::Application::getLastMouseButton() {
	return mLastMouseButton;
}