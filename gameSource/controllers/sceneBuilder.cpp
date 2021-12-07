//
// Created by olivier on 23/11/2021.
//

#include "sceneBuilder.h"

#include <cstdio>
#include <cstdlib>
#include "minorGems/util/SimpleVector.h"
#include "OneLife/gameSource/procedures/graphics/screens.h"
#include "OneLife/gameSource/dataTypes/signals.h"
#include "OneLife/gameSource/debug.h"
#include "OneLife/gameSource/dataTypes/exception/exception.h"

using signal = OneLife::dataType::Signal;

extern char userReconnect;
extern double frameRateFactor;
extern char *userTwinCode;
extern int userTwinCount;
extern SimpleVector<LiveObject> gameObjects;
extern int ourID;

const char* OneLife::game::SceneBuilder::screenName = "Waiting birth";

OneLife::game::SceneBuilder::SceneBuilder()
{
	this->status.isConnected = false;
	this->status.isPlayerAgentSet = false;
	this->status.isObjectsDownloaded = false;
	this->gameScene = nullptr;
	this->player = nullptr;
	this->socket = nullptr;
	this->screen = {0};
	this->screen.center = {0,0};
	this->screen.component.connectingMessage.color = {1,1,1,1};
	this->screen.component.statusMessage.value = nullptr;
	this->screen.component.connectingMessage.value = nullptr;
	this->screen.component.serverAddressMessage.value = nullptr;
	this->screen.component.cancelMessage.value = nullptr;
	this->isScreenInited = false;
}

OneLife::game::SceneBuilder::~SceneBuilder()
{
	if(this->screen.component.statusMessage.value)free(this->screen.component.statusMessage.value);
	if(this->screen.component.connectingMessage.value)free(this->screen.component.connectingMessage.value);
	if(this->screen.component.serverAddressMessage.value)free(this->screen.component.serverAddressMessage.value);
	if(this->screen.component.cancelMessage.value)free(this->screen.component.cancelMessage.value);
}

void OneLife::game::SceneBuilder::handle(OneLife::dataType::UiComponent* screen)
{
	screen->label = this->screenName;
	screen->draw = OneLife::game::graphic::drawWaitingBirthScreen;
	screen->body = &(this->screen);

	if(!this->status.isConnected)
	{
		this->connect();
	}
	else if(!this->isScreenInited)
	{
		this->initScreen();
		this->isScreenInited = true;
	}
	else if(!this->status.isObjectsDownloaded)
	{
		this->downloadObjects();
	}
	else if(!this->status.isPlayerAgentSet)
	{
		//this->initPlayerAgent();
		this->status.isPlayerAgentSet = true;//TODO remove and uncomment initPlayer after test done
	}
	else this->updateScreen();
}

void OneLife::game::SceneBuilder::handle(LiveObject* player)
{
	this->player = player;
}

void OneLife::game::SceneBuilder::handle(LivingLifePage* gameSceneController)
{
	OneLife::game::Debug::writeMethodInfo("OneLife::game::SceneBuilder::handle(%p)", gameSceneController);
	if(!gameSceneController)
	{
		OneLife::game::Debug::write("Trying to instantiate gameSceneController");
		gameSceneController = new LivingLifePage();
	}
	OneLife::game::Debug::write("instantiate gameSceneController(%p)", gameSceneController);
	this->gameScene = gameSceneController;
}

void OneLife::game::SceneBuilder::handle(OneLife::game::component::Socket* socket)
{
	this->socket = socket;
}

/**********************************************************************************************************************/

void OneLife::game::SceneBuilder::connect()
{
	if(!this->socket)throw new OneLife::game::Exception("Socket object is not set before call OneLife::game::SceneBuilder::connect()");
	OneLife::game::Debug::write("Socket: %s:%i", this->socket->getAddress().ip, this->socket->getAddress().port);
}

void OneLife::game::SceneBuilder::initScreen()
{
	int bufferSize;

	this->screen.component.statusMessage.position = this->screen.center;
	bufferSize = 32*sizeof(char);
	this->screen.component.statusMessage.value = (char*)malloc(bufferSize);

	this->screen.component.connectingMessage.position.x = this->screen.center.x;
	this->screen.component.connectingMessage.position.y = this->screen.center.y + 128;

	if(!this->screen.component.serverAddressMessage.value)
	{
		bufferSize = 32*sizeof(char);
		this->screen.component.serverAddressMessage.value = (char*)malloc(bufferSize);
		memset(this->screen.component.serverAddressMessage.value, 0, bufferSize);
		sprintf(this->screen.component.serverAddressMessage.value, "%s:%i",
				//translate( "customServerMessage" ),
				stringToUpperCase(this->socket->getAddress().ip),
				this->socket->getAddress().port);
		this->screen.component.serverAddressMessage.position.x = this->screen.center.x;
		this->screen.component.serverAddressMessage.position.y = this->screen.center.y + 192;
	}

	this->screen.component.cancelMessage.position.x = this->screen.center.x;
	this->screen.component.cancelMessage.position.y = this->screen.center.y - 200;
}

void OneLife::game::SceneBuilder::downloadObjects()
{
	OneLife::game::Debug::writeMethodInfo("OneLife::game::SceneBuilder::downloadObjects()");
	//TODO: check if this->socket is set
	if(!this->socket->isConnected())
	{
		this->socket->connect();
	}

}

void OneLife::game::SceneBuilder::initPlayerAgent()
{
	//TODO: test for this->casting != nullptr
	this->player = gameObjects.getElement(this->gameScene->getIndexRecentlyInsertedGameObject());//LECAGY: recentInsertedGameObjectIndex
	ourID = this->player->id;

	/*
	if( ourID != lastPlayerID )
	{
		minitech::initOnBirth();
		// different ID than last time, delete old home markers
		oldHomePosStack.deleteAll();
	}
	homePosStack.push_back_other( &oldHomePosStack );

	lastPlayerID = ourID;

	// we have no measurement yet
	this->player->lastActionSendStartTime = 0;
	this->player->lastResponseTimeDelta = 0;
	remapRandSource.reseed( ourID );
	mCurrentRemapFraction = 0;
	mRemapPeak = 0;
	setRemapFraction( mCurrentRemapFraction );
	printf( "Got first PLAYER_UPDATE message, our ID = %d\n", ourID );
	this->player->displayChar = 'A';
	//}
	 */
}

void OneLife::game::SceneBuilder::updateScreen()
{
	int bufferSize = 32*sizeof(char);

	if(this->screen.component.connectingMessage.color.alpha > 0)
	{
		if(this->socket->isConnected() )
		{
			this->screen.component.connectingMessage.color.alpha -= 0.05 * frameRateFactor;
			if(this->screen.component.connectingMessage.color.alpha < 0) this->screen.component.connectingMessage.color.alpha = 0;
		}
	}

	if(!this->socket->isConnected())
	{
		if(userReconnect)// don't draw waiting message, not connected yet
		{
			memset(this->screen.component.statusMessage.value, 0, bufferSize);
			strcpy(this->screen.component.statusMessage.value, "waitingReconnect");
		}
	}
	else if(userReconnect)
	{
		memset(this->screen.component.statusMessage.value, 0, bufferSize);
		strcpy(this->screen.component.statusMessage.value, "waitingReconnect");
	}
	/*
	else if(mPlayerInFlight)
	{
		memset(this->screen.component.statusMessage.value, 0, bufferSize);
		strcpy(this->screen.component.statusMessage.value, "waitingArrival");
	}
	*/
	else if(userTwinCode == NULL)
	{
		memset(this->screen.component.statusMessage.value, 0, bufferSize);
		strcpy(this->screen.component.statusMessage.value, "waitingBirth");
	}
	else
	{
		const char *sizeString = translate( "twins" );
		if(userTwinCount == 3)
		{
			sizeString = translate( "triplets" );
		}
		else if(userTwinCount == 4)
		{
			sizeString = translate( "quadruplets" );
		}

		memset(this->screen.component.statusMessage.value, 0, bufferSize);
		strcpy(this->screen.component.statusMessage.value, autoSprintf(translate( "waitingBirthFriends" ), sizeString));

		/*
		if(!mStartedLoadingFirstObjectSet)
		{
			memset(this->screen.component.statusMessage.value, 0, bufferSize);
			strcpy(this->screen.component.statusMessage.value, autoSprintf(translate( "cancelWaitingFriends" ), sizeString));
		}
		*/
	}
	this->sendSignal(signal::DONE);
}

void OneLife::game::SceneBuilder::update0()
{
	/*
	//!
	char stillWaitingBirth = false;
	if( mFirstServerMessagesReceived != 3 )
	{
		// haven't gotten first messages from server yet
		stillWaitingBirth = true;
	}
	else if( mFirstServerMessagesReceived == 3 )
	{
		if( !mDoneLoadingFirstObjectSet )
		{
			stillWaitingBirth = true;
		}
	}


	if( stillWaitingBirth )
	{
		if( getSpriteBankLoadFailure() != NULL || getSoundBankLoadFailure() != NULL )
		{
			this->setSignal( "loadFailure" );
		}

		OneLife::game::graphic::drawWaitingBirthScreen(
				nullptr,
				this->socket,
				lastScreenViewCenter,
				connectionMessageFade,
				frameRateFactor,
				mFirstObjectSetLoadingProgress,
				usingCustomServer,
				serverIP,
				serverPort,
				userReconnect,
				mPlayerInFlight,
				userTwinCode,
				userTwinCount,
				mStartedLoadingFirstObjectSet);
		return;
	}
	*/
}

void OneLife::game::SceneBuilder::update1()
{
	/******************************************************************************************************************
	if( mStartedLoadingFirstObjectSet && ! mDoneLoadingFirstObjectSet )
	{
		mDoneLoadingFirstObjectSet = isLiveObjectSetFullyLoaded( &mFirstObjectSetLoadingProgress );

		if( mDoneLoadingFirstObjectSet &&
			game_getCurrentTime() - mStartedLoadingFirstObjectSetStartTime
			< 1 ) {
			// always show loading progress for at least 1 second
			//mDoneLoadingFirstObjectSet = false;
		}


		if( mDoneLoadingFirstObjectSet ) {
			mPlayerInFlight = false;

			printf( "First map load done\n" );

			int loaded, total;
			countLoadedSprites( &loaded, &total );

			printf( "%d/%d sprites loaded\n", loaded, total );


			restartMusic( computeCurrentAge( ourLiveObject ),
					ourLiveObject->ageRate );
			setSoundLoudness( 1.0 );
			resumePlayingSoundSprites();
			setMusicLoudness( musicLoudness );

			// center view on player's starting position
			lastScreenViewCenter.x = CELL_D * ourLiveObject->xd;
			lastScreenViewCenter.y = CELL_D * ourLiveObject->yd;

			setViewCenterPosition( lastScreenViewCenter.x,
					lastScreenViewCenter.y );

			mapPullMode =
					SettingsManager::getIntSetting( "mapPullMode", 0 );
			mapPullStartX =
					SettingsManager::getIntSetting( "mapPullStartX", -10 );
			mapPullStartY =
					SettingsManager::getIntSetting( "mapPullStartY", -10 );
			mapPullEndX =
					SettingsManager::getIntSetting( "mapPullEndX", 10 );
			mapPullEndY =
					SettingsManager::getIntSetting( "mapPullEndY", 10 );

			mapPullCurrentX = mapPullStartX;
			mapPullCurrentY = mapPullStartY;

			if( mapPullMode ) {
				mMapGlobalOffset.x = mapPullCurrentX;
				mMapGlobalOffset.y = mapPullCurrentY;
				mMapGlobalOffsetSet = true;

				applyReceiveOffset( &mapPullCurrentX, &mapPullCurrentY );
				applyReceiveOffset( &mapPullStartX, &mapPullStartY );
				applyReceiveOffset( &mapPullEndX, &mapPullEndY );


				mapPullCurrentSaved = true;
				mapPullModeFinalImage = false;

				char *message = autoSprintf( "MAP %d %d#",
						sendX( mapPullCurrentX ),
						sendY( mapPullCurrentY ) );

				sendToServerSocket( message );

				mapPullCurrentSent = true;

				delete [] message;

				int screenWidth, screenHeight;
				getScreenDimensions( &screenWidth, &screenHeight );

				double scale = screenWidth / (double)screenW;

				mapPullTotalImage =
						new Image( lrint(
										( 10 + mapPullEndX - mapPullStartX )
										* CELL_D * scale ),
								lrint( ( 6 + mapPullEndY - mapPullStartY )
									   * CELL_D * scale ),
								3, false );
				numScreensWritten = 0;
			}
		}
	}
	else {

		clearLiveObjectSet();


		// push all objects from grid, live players, what they're holding
		// and wearing into live set

		// any direct-from-death graves
		// we want these to pop in instantly whenever someone dies
		SimpleVector<int> *allPossibleDeathMarkerIDs =
				getAllPossibleDeathIDs();

		for( int i=0; i<allPossibleDeathMarkerIDs->size(); i++ ) {
			addBaseObjectToLiveObjectSet(
					allPossibleDeathMarkerIDs->getElementDirect( i ) );
		}



		// next, players
		for( int i=0; i<gameObjects.size(); i++ ) {
			LiveObject *o = gameObjects.getElement( i );

			addBaseObjectToLiveObjectSet( o->displayID );


			if( ! o->allSpritesLoaded ) {
				// check if they're loaded yet

				int numLoaded = 0;

				ObjectRecord *displayObj = getObject( o->displayID );
				for( int s=0; s<displayObj->numSprites; s++ ) {

					if( markSpriteLive( displayObj->sprites[s] ) ) {
						numLoaded ++;
					}
				}

				if( numLoaded == displayObj->numSprites ) {
					o->allSpritesLoaded = true;
				}
			}


			// and what they're holding
			if( o->holdingID > 0 ) {
				addBaseObjectToLiveObjectSet( o->holdingID );

				// and what it contains
				for( int j=0; j<o->numContained; j++ ) {
					addBaseObjectToLiveObjectSet(
							o->containedIDs[j] );

					for( int s=0; s<o->subContainedIDs[j].size(); s++ ) {
						addBaseObjectToLiveObjectSet(
								o->subContainedIDs[j].getElementDirect( s ) );
					}
				}
			}

			// and their clothing
			for( int c=0; c<NUM_CLOTHING_PIECES; c++ ) {
				ObjectRecord *cObj = clothingByIndex( o->clothing, c );

				if( cObj != NULL ) {
					addBaseObjectToLiveObjectSet( cObj->id );

					// and what it containes
					for( int cc=0;
						 cc< o->clothingContained[c].size(); cc++ ) {
						int ccID =
								o->clothingContained[c].getElementDirect( cc );
						addBaseObjectToLiveObjectSet( ccID );
					}
				}
			}
		}


		// next all objects in grid
		int numMapCells = mMapD * mMapD;

		for( int i=0; i<numMapCells; i++ ) {
			if( mMapFloors[i] > 0 ) {
				addBaseObjectToLiveObjectSet( mMapFloors[i] );
			}

			if( mMap[i] > 0 ) {

				addBaseObjectToLiveObjectSet( mMap[i] );

				// and what is contained in each object
				int numCont = mMapContainedStacks[i].size();

				for( int j=0; j<numCont; j++ ) {
					addBaseObjectToLiveObjectSet(
							mMapContainedStacks[i].getElementDirect( j ) );

					SimpleVector<int> *subVec =
							mMapSubContainedStacks[i].getElement( j );

					int numSub = subVec->size();
					for( int s=0; s<numSub; s++ ) {
						addBaseObjectToLiveObjectSet(
								subVec->getElementDirect( s ) );
					}
				}
			}
		}


		markEmotionsLive();

		finalizeLiveObjectSet();

		mStartedLoadingFirstObjectSet = true;
		mStartedLoadingFirstObjectSetStartTime = game_getCurrentTime();
	}
	/******************************************************************************************************************/
}

void OneLife::game::SceneBuilder::update3()
{
	/******************************************************************************************************************
	if( !( mFirstServerMessagesReceived & 1 ) )
	{
		// first map chunk just recieved

		minitech::initOnBirth();

		//reset fov on birth
		if ( SettingsManager::getIntSetting( "fovEnabled", 1 ) ) {
			changeFOV( SettingsManager::getFloatSetting( "fovDefault", 1.25f ) );
		}
		else {
			changeFOV( 1.0f );
		}
		changeHUDFOV( SettingsManager::getFloatSetting( "fovScaleHUD", 1.25f ) );

		char found = false;
		int closestX = 0;
		int closestY = 0;

		// only if marker starts on birth map chunk

		// use distance squared here, no need for sqrt

		// rough estimate of radius of birth map chunk
		// this allows markers way off the screen, but so what?
		double closestDist = 16 * 16;

		int mapCenterY = y + sizeY / 2;
		int mapCenterX = x + sizeX / 2;
		printf( "Map center = %d,%d\n", mapCenterX, mapCenterY );

		for( int mapY=0; mapY < mMapD; mapY++ ) {
			for( int mapX=0; mapX < mMapD; mapX++ ) {

				int i = mMapD * mapY + mapX;

				int id = mMap[ i ];

				if( id > 0 ) {

					// check for home marker
					if( getObject( id )->homeMarker ) {
						int worldY = mapY + mMapOffsetY - mMapD / 2;

						int worldX = mapX + mMapOffsetX - mMapD / 2;

						double dist =
								pow( worldY - mapCenterY, 2 )
								+
								pow( worldX - mapCenterX, 2 );

						if( dist < closestDist ) {
							closestDist = dist;
							closestX = worldX;
							closestY = worldY;
							found = true;
						}
					}
				}
			}
		}

		if( found ) {
			printf( "Found starting home marker at %d,%d\n",
					closestX, closestY );
			addHomeLocation( closestX, closestY );
		}
	}
	mFirstServerMessagesReceived |= 1;
	/******************************************************************************************************************/
}