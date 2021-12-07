//
// Created by olivier on 01/11/2021.
//

#include "screenRenderer.h"

#include <SDL/SDL.h>
#include "minorGems/game/doublePair.h"
#include "minorGems/graphics/openGL/SingleTextureGL.h"
#include "minorGems/util/log/AppLog.h"
#include "minorGems/util/SettingsManager.h"
#include "OneLife/gameSource/components/GamePage.h"
#include "OneLife/gameSource/application.h"
#include "OneLife/gameSource/procedures/graphics/modalObjects/drawPausePanel.h"

extern GamePage *currentGamePage;
extern double viewWidth;
extern doublePair lastScreenViewCenter;
extern char userReconnect;

OneLife::game::ScreenRenderer::ScreenRenderer(OneLife::dataType::ui::Screen screen)
{
	if(screen.settings.grabInput) SDL_WM_GrabInput( SDL_GRAB_ON );
	else SDL_WM_GrabInput( SDL_GRAB_OFF );

}

OneLife::game::ScreenRenderer::~ScreenRenderer() {}

void OneLife::game::ScreenRenderer::setDefault(
		double width,
		double height,
		bool forceAspectRatio,
		bool doNotChangeNativeResolution,
		bool fullScreen,
		bool forceSpecifiedDimensions)
{
	this->width = width;
	this->height = height;
	this->forceAspectRatio = forceAspectRatio;
	this->doNotChangeNativeResolution = doNotChangeNativeResolution;
	this->fullScreen = fullScreen;
	this->forceSpecifiedDimensions = forceSpecifiedDimensions;
}

void OneLife::game::ScreenRenderer::switchFullScreenMode()
{
	this->setupSurface();
	callbackResize(this->width, this->height);
	SingleTextureGL::contextChanged();// reload all textures into OpenGL
}

void OneLife::game::ScreenRenderer::switchMinimizedMode()
{

}

void OneLife::game::ScreenRenderer::render(OneLife::dataType::UiComponent* screen)
{
	if(currentGamePage!= nullptr)
	{
		//printf("\n===>draw Screen ...");
		currentGamePage->base_draw( lastScreenViewCenter, viewWidth );
		if(screen)
		{
			if(screen->draw) screen->draw(screen->body);
		}
	}
	drawPauseScreen();
}

/**********************************************************************************************************************/

//!private

/**
 *
 */
void OneLife::game::ScreenRenderer::setupSurface()
{
	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );

	int flags = 0;

#ifndef RASPBIAN
	// don't have SDL create the GL surface
	// (the SDL one is a software surface on Raspbian)
	flags = SDL_OPENGL;
#endif

	// NOTE:  flags are also adjusted below if fullscreen resolution not
	// available
	int borderless = 0;

	if( this->fullScreen ) {
#ifdef __mac__
		borderless = 1;
        NSMenu_setMenuBarVisible(0);
#else
		borderless = SettingsManager::getIntSetting( "borderless", 0 );
#endif

		if( borderless ) {
			AppLog::info( "Setting borderless mode for fullscreen" );
			SDL_putenv( (char*)"SDL_VIDEO_WINDOW_POS=0,0" );

			flags = flags | SDL_NOFRAME;
		}
		else {
			AppLog::info( "Setting real (not borderless) fullscreen mode" );
			flags = flags | SDL_FULLSCREEN;
		}
	}

	const SDL_VideoInfo* currentScreenInfo = SDL_GetVideoInfo();

	int currentW = currentScreenInfo->current_w;
	int currentH = currentScreenInfo->current_h;

	// aspect ratio
	int currentAspectRatio = computeAspectRatio( currentW, currentH );

	AppLog::getLog()->logPrintf(
			Log::INFO_LEVEL,
			"Current screen configuration is %dx%d with aspect ratio %.2f",
			currentW, currentH, currentAspectRatio / 100.0f );



	// check for available modes
	SDL_Rect** modes;


	// Get available fullscreen/hardware modes
	modes = SDL_ListModes( NULL, flags);

	// Check if there are any modes available
	if( modes == NULL ) {
		AppLog::criticalError( "ERROR:  No video modes available");
		exit(-1);
	}

	// Check if our resolution is restricted
	if( modes == (SDL_Rect**)-1 ) {
		AppLog::info( "All resolutions available" );

		if( this->fullScreen && this->doNotChangeNativeResolution ) {
			AppLog::info( "Sticking with user's current screen resolution" );

			int borderlessHeightAdjust =
					SettingsManager::getIntSetting( "borderlessHeightAdjust", 0 );

			this->width = currentW;
			this->height = currentH;

			if( borderless && borderlessHeightAdjust != 0 ) {
				AppLog::getLog()->logPrintf(
						Log::INFO_LEVEL,
						"Adding %d to borderless window height of %d "
						"resulting in total height of %d",
						borderlessHeightAdjust, this->height,
						this->height + borderlessHeightAdjust );
				this->height += borderlessHeightAdjust;
			}
		}
	}
	else if( this->forceSpecifiedDimensions && this->fullScreen ) {

		AppLog::info( "Requested video mode is forced (playback?)" );

		// check if specified dimension available in fullscreen

		char match = false;

		for( int i=0; modes[i] && ! match; ++i ) {
			if( this->width == modes[i]->w &&
				this->height == modes[i]->h ) {
				match = true;
			}
		}

		if( !match ) {
			AppLog::getLog()->logPrintf(
					Log::WARNING_LEVEL,
					"  Could not find a full-screen match for the forced screen "
					"dimensions %d x %d\n", this->width, this->height );
			AppLog::warning( "Reverting to windowed mode" );

			this->fullScreen = false;

			flags = SDL_OPENGL;
#ifdef RASPBIAN
			flags = 0;
#endif
		}
	}
	else{

		if( this->fullScreen && this->doNotChangeNativeResolution ) {
			AppLog::info( "Sticking with user's current screen resolution" );

			this->width = currentW;
			this->height = currentH;
		}


		AppLog::getLog()->logPrintf(
				Log::INFO_LEVEL,
				"Checking if requested video mode (%dx%d) is available",
				this->width, this->height );


		// Print valid modes

		// only count a match of BOTH resolution and aspect ratio
		char match = false;

		AppLog::info( "Available video modes:" );
		for( int i=0; modes[i]; ++i ) {
			AppLog::getLog()->logPrintf( Log::DETAIL_LEVEL,
					"   %d x %d\n",
					modes[i]->w,
					modes[i]->h );

			int thisAspectRatio = computeAspectRatio( modes[i]->w,
					modes[i]->h );

			if( !this->forceAspectRatio && thisAspectRatio == currentAspectRatio ) {
				AppLog::info( "   ^^^^ this mode matches current "
							  "aspect ratio" );
			}

			if( this->width == modes[i]->w && this->height == modes[i]->h ) {
				AppLog::info( "   ^^^^ this mode matches requested mode" );

				if( ! this->forceAspectRatio &&
					thisAspectRatio != currentAspectRatio ) {
					AppLog::info( "        but it doesn't match current "
								  "aspect ratio" );
				}
				else {
					match = true;
				}
			}
		}

		if( !match ) {
			AppLog::warning( "Warning:  No match for requested video mode" );
			AppLog::info( "Trying to find the closest match" );

			int bestDistance = 99999999;

			int bestIndex = -1;

			for( int i=0; modes[i]; ++i ) {
				// don't even consider modes that are SMALLER than our
				// requested mode in either dimension
				if( modes[i]->w >= this->width &&
					modes[i]->h >= this->height ) {

					int distance = (int)(
							fabs( modes[i]->w - this->width ) +
							fabs( modes[i]->h - this->height ) );

					int thisAspectRatio = computeAspectRatio( modes[i]->w,
							modes[i]->h );

					if( ( this->forceAspectRatio ||
						  thisAspectRatio == currentAspectRatio )
						&&
						distance < bestDistance ) {

						bestIndex = i;
						bestDistance = distance;
					}
				}

			}


			if( bestIndex != -1 ) {

				if( this->forceAspectRatio ) {
					AppLog::getLog()->logPrintf(
							Log::INFO_LEVEL,
							"Picking closest available large-enough resolution:  "
							"%d x %d\n",
							modes[bestIndex]->w,
							modes[bestIndex]->h );
				}
				else {
					AppLog::getLog()->logPrintf(
							Log::INFO_LEVEL,
							"Picking closest available large-enough resolution "
							"that matches current aspect ratio:  %d x %d\n",
							modes[bestIndex]->w,
							modes[bestIndex]->h );
				}
			}
			else {
				// search again, ignoring aspect match

				if( !this->forceAspectRatio ) {

					AppLog::warning(
							"Warning:  No match for current aspect ratio" );
					AppLog::info(
							"Trying to find the closest off-ratio match" );


					for( int i=0; modes[i]; ++i ) {
						// don't even consider modes that are SMALLER than our
						// requested mode in either dimension
						if( modes[i]->w >= this->width &&
							modes[i]->h >= this->height ) {

							int distance = (int)(
									fabs( modes[i]->w - this->width ) +
									fabs( modes[i]->h - this->height ) );

							if( distance < bestDistance ) {
								bestIndex = i;
								bestDistance = distance;
							}
						}
					}
				}


				if( bestIndex != -1 ) {
					AppLog::getLog()->logPrintf(
							Log::INFO_LEVEL,
							"Picking closest available large-enough resolution:  "
							"%d x %d\n",
							modes[bestIndex]->w,
							modes[bestIndex]->h );
				}
				else {
					AppLog::warning(
							"Warning:  No sufficiently sized resolution found" );
					AppLog::info(
							"Considering closest-match smaller resolution" );

					for( int i=0; modes[i]; ++i ) {
						int distance = (int)(
								fabs( modes[i]->w - this->width ) +
								fabs( modes[i]->h - this->height ) );

						if( distance < bestDistance ) {
							bestIndex = i;
							bestDistance = distance;
						}
					}

					if( bestIndex != -1 ) {
						AppLog::getLog()->logPrintf(
								Log::INFO_LEVEL,
								"Picking closest available resolution:  "
								"%d x %d\n",
								modes[bestIndex]->w,
								modes[bestIndex]->h );
					}
					else {
						AppLog::criticalError(
								"ERROR:  No video modes available");
						exit(-1);
					}
				}

			}


			this->width = modes[bestIndex]->w;
			this->height = modes[bestIndex]->h;
		}

	}


	// 1-bit stencil buffer
	SDL_GL_SetAttribute( SDL_GL_STENCIL_SIZE, 1 );

	// vsync to avoid tearing
	SDL_GL_SetAttribute( SDL_GL_SWAP_CONTROL, 1 );

	// current color depth
	SDL_Surface *screen = SDL_SetVideoMode( this->width, this->height, 0, flags);

#ifdef RASPBIAN
	raspbianCreateSurface();
#endif


	if ( screen == NULL ) {
		printf( "Couldn't set %fx%f GL video mode: %s\n",
				this->width,
				this->height,
				SDL_GetError() );
	}

#ifdef RASPBIAN
	//screenGLStencilBufferSupported = true;//TODO: check if not used somewhere =>delete
#else
	int setStencilSize;
	SDL_GL_GetAttribute( SDL_GL_STENCIL_SIZE, &setStencilSize );
	if( setStencilSize > 0 ) {
		// we have a stencil buffer
		//screenGLStencilBufferSupported = true; //TODO: check if not used somewhere =>delete
	}
#endif


	glEnable( GL_DEPTH_TEST );
	glEnable( GL_CULL_FACE );
	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	glCullFace( GL_BACK );
	glFrontFace( GL_CCW );
}

/**********************************************************************************************************************/

/**
 *
 * @param inGrabOn
 * @note : confines mouse pointer to window and prevents window manager from intercepting key presses default off
 */
void grabInput( char inGrabOn ) {
	if( inGrabOn ) {
		SDL_WM_GrabInput( SDL_GRAB_ON );
	}
	else {
		SDL_WM_GrabInput( SDL_GRAB_OFF );
	}
}

// draw code separated from updates
// some updates are still embedded in draw code, so pass a switch to
// turn them off
void drawFrameNoUpdate( char inUpdate ) {
	if( currentGamePage != NULL ) {
		currentGamePage->base_draw( lastScreenViewCenter, viewWidth );
	}
}

/**********************************************************************************************************************/

#include "OneLife/gameSource/components/engines/GameSceneHandler.h"
#include "OneLife/gameSource/controllers/ExtendedMessagePage.h"
extern ExtendedMessagePage *extendedMessagePage;
#include "OneLife/gameSource/controllers/LivingLifePage.h"
extern LivingLifePage *livingLifePage;
#include "OneLife/gameSource/components/engines/GameSceneHandler.h"
extern char userReconnect;
extern char *userTwinCode;

void showDiedPage() {
	userReconnect = false;

	lastScreenViewCenter.x = 0;
	lastScreenViewCenter.y = 0;

	setViewCenterPosition( lastScreenViewCenter.x,
			lastScreenViewCenter.y );

	currentGamePage = extendedMessagePage;

	extendedMessagePage->setMessageKey( "youDied" );

	char *reason = livingLifePage->getDeathReason();

	if( reason == NULL ) {
		extendedMessagePage->setSubMessage( "" );
	}
	else {
		extendedMessagePage->setSubMessage( reason );

		delete [] reason;
	}


	currentGamePage->base_makeActive( true );
}

void showReconnectPage() {
	lastScreenViewCenter.x = 0;
	lastScreenViewCenter.y = 0;

	setViewCenterPosition( lastScreenViewCenter.x,
			lastScreenViewCenter.y );

	currentGamePage = extendedMessagePage;

	extendedMessagePage->setMessageKey( "connectionLost" );

	extendedMessagePage->setSubMessage( translate( "willTryReconnect" ) );

	userReconnect = true;

	// don't reconnect as twin
	// that will cause them to wait for their party again.
	if( userTwinCode != NULL ) {
		delete [] userTwinCode;
		userTwinCode = NULL;
	}

	currentGamePage->base_makeActive( true );
}