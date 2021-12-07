//
// Created by olivier on 29/10/2021.
//

#ifndef ONELIFE_GAME_APPLICATION_H
#define ONELIFE_GAME_APPLICATION_H

//#include "minorGems/graphics/openGL/ScreenGL.h"

#include <vector>
#include "minorGems/graphics/openGL/KeyboardHandlerGL.h"
#include "minorGems/graphics/openGL/MouseHandlerGL.h"
#include "minorGems/graphics/openGL/SceneHandlerGL.h"
#include "minorGems/graphics/openGL/RedrawListenerGL.h"
#include "minorGems/math/geometry/Vector3D.h"
#include "minorGems/math/geometry/Angle3D.h"
#include "minorGems/util/SimpleVector.h"
#include "minorGems/system/Time.h"
#include "OneLife/gameSource/components/engines/deviceListener.h"
#include "OneLife/gameSource/components/engines/screenRenderer.h"
#include "OneLife/gameSource/components/socket.h"
#include "OneLife/gameSource/settings.h"
#include "OneLife/gameSource/controllers/initializationScreen.h"
#include "OneLife/gameSource/controllers/sceneBuilder.h"
#include "OneLife/gameSource/feature.h"
#include "OneLife/gameSource/dataTypes/ui.h"
#include "OneLife/gameSource/dataTypes/socket.h"
#include "OneLife/gameSource/dataTypes/hardware.h"

namespace OneLife::game
{
	class Application
	{
		public:
			Application(
					OneLife::game::Settings settings,
					int inWide, int inHigh, char inFullScreen,
					char inDoNotChangeNativeResolution,
					unsigned int inMaxFrameRate,
					char inRecordEvents,
					const char *inCustomRecordedGameData,
					const char *inHashSalt,
					const char *inWindowName,
					KeyboardHandlerGL *inKeyHandler = NULL,
					MouseHandlerGL *inMouseHandler = NULL,
					SceneHandlerGL *inSceneHandler = NULL  );
			~Application();

			void init(OneLife::game::Settings settings);
			void bindGlobalCurrentController(GamePage **currentGamePage);

			void setConnection(const char* ip, int port);
			OneLife::game::component::Socket* getConnection();

			void storeMessage();
			void putMessageIn();

			void setOption(int option, int value);
			bool isEnable(int option);
			void setUseCustomServerStatus(bool status);
			bool isUsingCustomServer();

			void addFeature(void* feature);

			void start();

			const char *getCustomRecordedGameData();
			char isPlayingBack();
			float getPlaybackDoneFraction();
			char shouldShowPlaybackDisplay();
			char isMinimized();


			// can avoid recording/playback during certain "front matter"
			// activities like menu navigation
			// (has no effect if no recording or playback pending)
			void startRecordingOrPlayback();

			// our current max
			// can be changed with speed control keys, if enabled
			void setMaxFrameRate( unsigned int inMaxFrameRate );

			// our absolute max
			// speed control key adjustments are relative to this full rate
			void setFullFrameRate( unsigned int inFullFrameRate );


			unsigned int getMaxFramerate();

			// set to true to enable frame sleeping to enforce max frame rate
			// (sleeping is default)
			// Otherwise, if false, no max is enforced (though we may be able
			// to depend on vsync to enforce it for us)
			void useFrameSleep( char inUse );



			// should ^ and % be allowed to slowdown and resume normal speed
			// during event playback?
			// setMaxFrameRate also can be used to create a slowdown, but
			// ^ and % keys are not passed through during playback
			void allowSlowdownKeysDuringPlayback( char inAllow );


			// enables rand seed to be recorded and played back with
			// event playback
			unsigned int getRandSeed();


			// enables Time::timeSec() values to be recorded and played back
			timeSec_t getTimeSec();
			double getCurrentTime();


			// to save FPS in recording files
			void registerActualFrameRate( double inFrameRate );

			// these can be played back
			double getRecordedFrameRate();


			// sets mapping so that when inFromKey is pressed, an
			// event for inToKey is generated instead
			void setKeyMapping( unsigned char inFromKey,
								unsigned char inToKey );


			// turns key mapping on (default) or off
			void toggleKeyMapping( char inMappingOn );



			// passes in a web event to be (possibly) added to the current
			// game recording
			// type encodes :
			//    0 = normal request step completed (NULL body)
			//    1 = request done step completed (NULL body)
			//   -1 = error-returning request step completed (NULL body)
			//    2 = result-fetch step completed (non-NULL body)
			//   >2 = web progress event, showing number of bytes fetched
			//        (NULL body)
			void registerWebEvent( int inHandle,
								   int inType,
								   const char *inBodyString = NULL,
					// -1 means use string length
					// provide length for binary bodies
								   int inBodyLength = -1 );


			// gets the type of the next pending web event (from playback)
			// if the event has no result body, this call removes the event
			// from the list.
			// If it has a result body, getWebEventResultBody must be called.
			int getWebEventType( int inHandle );

			// gets a recorded event body text from the current frame matching
			//   inHandle
			// result NULL if there is no event matching inHandle
			// result destroyed by caller.
			//
			// This call removes the web event from the list of pending events.
			char *getWebEventResultBody( int inHandle,
										 int *outSize = NULL );





			// passes in a socket event to be (possibly) added to the current
			// game recording
			// type encodes :
			//    0 = send with number sent (NULL body)
			//    1 = send resulting in error (NULL body)
			//    2 = read with number read (non-NULL body if inNumBodyBytes != 0)
			//    3 = read resulting in error (NULL body)
			void registerSocketEvent( int inHandle,
									  int inType,
									  int inNumBodyBytes,
									  unsigned char *inBodyBytes = NULL );


			// gets the type of the next pending socket event (from playback)
			// if the event has no body bytes
			// (type 0, 1, or 3, OR type 2 with 0-length body),
			// this call removes the event from the list.
			//
			// In case of type 2 with outNumBodyBytes > 0,
			// getWebEventResultBody must be called.
			//
			//
			void getSocketEventTypeAndSize( int inHandle,
											int *outType,
											int *outNumBodyBytes );

			// gets a recorded socket event body bytes from the current
			// frame matching inHandle
			// result NULL if there is no event matching inHandle
			// result destroyed by caller.
			//
			// return value has length as set by last getSocketEventTypeAndSize
			// call
			//
			// This call removes the socket event from the list of pending events.
			unsigned char *getSocketEventBodyBytes( int inHandle );


			void registerAsyncFileDone( int inHandle );

			char getAsyncFileDone( int inHandle );



			/**
			 * Switches to 2D mode, where no view transforms are applied
			 *
			 * Must be called before start();
			 */
			void switchTo2DMode();



			/**
			 * Moves the view position.
			 *
			 * @param inPositionChange directional vector describing motion.
			 *   Must be destroyed by caller.
			 */
			void moveView( Vector3D *inPositionChange );


			/**
			 * Rotates the view.
			 *
			 * @param inOrientationChange angle to rotate view by.
			 *   Must be destroyed by caller.
			 */
			void rotateView( Angle3D *inOrientationChange );


			/**
			 * Gets the angle of the current view direction.
			 *
			 * @return the angle of the current view direction.
			 *   Not a copy, so shouldn't be modified or destroyed by caller.
			 */
			Angle3D *getViewOrientation();


			/**
			 * Gets the current view position.
			 *
			 * @return the position of the current view.
			 *  Must be destroyed by caller.
			 */
			Vector3D *getViewPosition();


			/**
			 * Sets the current view position.
			 *
			 * @param inPosition the new position.
			 *   Must be destroyed by caller.
			 */
			void setViewPosition( Vector3D *inPosition );


			/**
			 * Gets the width of the screen.
			 *
			 * @return the width of the screen, in pixels.
			 */
			int getWidth();



			/**
			 * Gets the height of the screen.
			 *
			 * @return the height of the screen, in pixels.
			 */
			int getHeight();



			/**
			 * Switches into full screen mode.
			 *
			 * Use changeWindowSize to switch back out of full screen mode.
			 */
			void setFullScreen();



			/**
			 * Sets the size of the viewport image in the window.
			 *
			 * Defaults to window size.
			 *
			 * Must be called before screen is started.
			 *
			 * @param inWidth, inHeight the new dimensions, in pixels.
			 */
			void setImageSize( int inWidth, int inHeight );



			/**
			 * Gets the width of the viewport image.
			 *
			 * @return the width of the viewport, in pixels.
			 */
			int getImageWidth();



			/**
			 * Gets the height of the viewport image.
			 *
			 * @return the height of the viewport, in pixels.
			 */
			int getImageHeight();



			/**
			 * Change the window size.
			 *
			 * @param inWidth, inHeight the new dimensions, in pixels.
			 */
			void changeWindowSize( int inWidth, int inHeight );



			/**
			 * Adds a mouse handler.
			 *
			 * @param inHandler the handler to add  Must
			 *   be destroyed by caller.
			 *
			 * Must not be called after calling start().
			 */
			void addMouseHandler( MouseHandlerGL *inHandler );



			/**
			 * Removes a mouse handler.
			 *
			 * @param inHandler the handler to remove.  Must
			 *   be destroyed by caller.
			 *
			 * Must not be called after calling start().
			 */
			void removeMouseHandler( MouseHandlerGL *inHandler );


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
			void addKeyboardHandler( KeyboardHandlerGL *inHandler,
									 char inFirstHandler = false );


			/**
			 * Removes a keyboard handler.
			 *
			 * @param inHandler the handler to remove.  Must
			 *   be destroyed by caller.
			 *
			 * Must not be called after calling start().
			 */
			void removeKeyboardHandler( KeyboardHandlerGL *inHandler );

			/**
			 * Removes a scene handler.
			 *
			 * @param inHandler the handler to remove.  Must
			 *   be destroyed by caller.
			 *
			 * Must not be called after calling start().
			 */
			//void removeSceneHandler( SceneHandlerGL *inHandler );//TODO: remove this since unique sceneHandler system

			/**
			 * Adds a redraw listener.
			 *
			 * @param inListener the listener to add.  Must
			 *   be destroyed by caller.
			 *
			 * Must not be called after calling start().
			 */
			void addRedrawListener( RedrawListenerGL *inListener );

			/**
			 * Removes a redraw listener.
			 *
			 * @param inListener the listener to remove.  Must
			 *   be destroyed by caller.
			 *
			 * Must not be called after calling start().
			 */
			void removeRedrawListener( RedrawListenerGL *inListener );

			/**
			 * Applies the current view matrix transformation
			 * to the matrix at the top of the GL_PROJECTION stack.
			 */
			void applyViewTransform();

			void obscureRecordedNumericTyping( char inObscure,char inCharToRecordInstead );

			/**
			 * Access the various handlers.
			 */
			//KeyboardHandlerGL *getKeyHandler();
			//MouseHandlerGL *getMouseHandler();
			//SceneHandlerGL *getSceneHandler();
			char isLastMouseButtonRight();
			// FOVMOD NOTE:  Change 2/3 - Take these lines during the merge process
			int getLastMouseButton();
			void setupSurface();

			// used by various implementations
			// callbacks (external C functions that can access private members)
			friend void callbackResize( int inW, int inH );
			friend void callbackKeyboard( unsigned char inKey, int inX, int inY );
			friend void callbackKeyboardUp( unsigned char inKey, int inX, int inY );
			friend void callbackSpecialKeyboard( int inKey, int inX, int inY );
			friend void callbackSpecialKeyboardUp( int inKey, int inX, int inY );
			friend void callbackMotion( int inX, int inY );
			friend void callbackPassiveMotion( int inX, int inY );
			friend void callbackMouse( int inButton, int inState, int inX, int inY );
			friend void callbackPreDisplay();
			friend void callbackDisplay();
			friend void callbackIdle();

			// our private members
			int mWide;
			int mHigh;
			// forces requested aspect ratio, if it's available, even
			// if it doesn't match screen's current ratio
			char mForceAspectRatio;
			// goes beyond just forcing the aspect ratio
			char mForceSpecifiedDimensions;
			// if true, when resolution not forced, we don't
			// change the user's resolution at all
			char mDoNotChangeNativeResolution;
			char mImageSizeSet;// for an viewport image that can be smaller than our screen
			int mImageWide;
			int mImageHigh;
			char mWantToMimimize;
			char mMinimized;
			char mWasFullScreenBeforeMinimize;
			char mWasInputGrabbedBeforeMinimize;
			char mStartedFullScreen;// only allow ALT-Enter to toggle fullscreen if it started there
			unsigned int mMaxFrameRate;// current target framerate, may involve slowdown mode (for testing)
			char mUseFrameSleep;
			unsigned int mFullFrameRate;// full frame rate when not in slowdown mode
			char mAllowSlowdownKeysDuringPlayback;
			char m2DMode;
			Vector3D *mViewPosition;

			// orientation of 0,0,0 means looking in the direction (0,0,1)
			// with an up direction of (0,1,0)
			Angle3D *mViewOrientation;

			// vectors of handlers and listeners
			SimpleVector<MouseHandlerGL*> *mMouseHandlerVector;
			SimpleVector<KeyboardHandlerGL*> *mKeyboardHandlerVector;
			//SimpleVector<SceneHandlerGL*> *mSceneHandlerVector;
			SimpleVector<RedrawListenerGL*> *mRedrawListenerVector;



			/**
			 * Gets whether at least one of our keyboard handlers is focused.
			 *
			 * @return true iff at least one keyboard handler is focused.
			 */
			char isKeyboardHandlerFocused();

			// for event recording
			SimpleVector<char*> mUserEventBatch;
			// these are written to file before user events
			// so that they can be played back first
			SimpleVector<char*> mEventBatch;
			char mRecordingEvents;
			char mPlaybackEvents;
			FILE *mEventFile;
			char mObscureRecordedNumericTyping;
			char mCharToRecordInstead;
			// length of open playback file
			int mEventFileNumBatches;
			int mNumBatchesPlayed;
			char mShouldShowPlaybackDisplay;
			char mRecordingOrPlaybackStarted;
			char *mCustomRecordedGameData;
			char *mHashSalt;


			void writeEventBatchToFile();
			void writeEventBatchToFile( SimpleVector<char*> *inBatch );
			void playNextEventBatch();

			// recording file may contain gaps between web event sequence
			// numbers (if recording was trimmed by hand)
			// adjust these so that they don't have gaps
			int mLastReadWebEventHandle;
			int mCurrentWebEventHandle;
			int mNextUnusedWebEventHandle;

			SimpleVector<WebEvent> mPendingWebEvents;
			SimpleVector<SocketEvent> mPendingSocketEvents;


			// since async file reads happen to handles in order
			// only need to track largest handle done so far
			int mLastAsyncFileHandleDone;
			unsigned int mRandSeed;
			timeSec_t mLastTimeValue;
			SimpleVector<timeSec_t> mLastTimeValueStack;
			double mLastCurrentTimeValue;
			SimpleVector<double> mLastCurrentTimeValueStack;
			double mLastActualFrameRate;
			timeSec_t mLastRecordedTimeValue;
			double mLastRecordedCurrentTimeValue;

			// for emulating time() values during playback where
			// time values were not recorded

			// detect whether any time values have been played back so far
			// if so, we assume that more will come and that we should stick
			// to those values.  If none have been played back yet, we simulate
			// played back time() values locked to the playback-specified
			// frame rate
			char mTimeValuePlayedBack;
			unsigned int mFramesSinceLastTimeTick;
			char mLastMouseButtonRight;

			// FOVMOD NOTE:  Change 3/3 - Take these lines during the merge process
			int mLastMouseButton;

			// for playing back minimized window state
			char mLastMinimizedStatus;

		private:
			void readDevicesStatus();
			void _oldReadDevicesStatus();
			void readMessages();
			void selectScreen();
			void update(OneLife::dataType::UiComponent* dataScreen);
			void render(OneLife::dataType::UiComponent* dataScreen);
			void sendClientMessage();

			void setController(void* controller);

			//!
			struct{
				bool connectedMode;
			}status;

			//!
			struct{
				bool pauseOnMinimize;
			}option;

			//!
			struct
			{
				OneLife::dataType::socket::Address socket;
			}data;

			//!
			struct{

			}component;
			OneLife::game::component::Socket* connection;//TODO rename component.socket

			//!
			struct{
				OneLife::game::InitializationScreen* initializationScreen;
				OneLife::game::SceneBuilder* mapGenerationScreen;
				LivingLifePage* gameSceneController;
			}controller;



			//!
			GamePage **currentController;
			unsigned lastSignalValue;
			bool isNewSystemEnable;//TODO: delete this after new system implementation done ...
			bool useCustomServer;
			bool isControllerRecentlySet;

			LiveObject* player;
			char* serverMessage;

			unsigned int idScreen;

			OneLife::dataType::ui::Screen currentScreen;//TODO: rename this screenData and make var GameScreen* currentScreen
			OneLife::game::DeviceListener* deviceListener;
			OneLife::game::ScreenRenderer* screenRenderer;


			OneLife::game::component::MessageChannel* messageChannel;

			std::vector<OneLife::game::Feature*> registeredFeature;

			bool quit;
	};
}

// prototypes
void callbackResize( int inW, int inH );
void callbackKeyboard( unsigned char  inKey, int inX, int inY );
void callbackKeyboardUp( unsigned char  inKey, int inX, int inY );
void callbackSpecialKeyboard( int inKey, int inX, int inY );
void callbackSpecialKeyboardUp( int inKey, int inX, int inY );
void callbackMotion( int inX, int inY );
void callbackPassiveMotion( int inX, int inY );
void callbackMouse( int inButton, int inState, int inX, int inY );
void callbackPreDisplay();
void callbackDisplay();
void callbackIdle();

//!Screen
int computeAspectRatio( int inW, int inH );

#endif //ONELIFE_GAME_APPLICATION_H
