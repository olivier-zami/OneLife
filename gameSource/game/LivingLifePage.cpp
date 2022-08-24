#include "LivingLifePage.h"

#include "minorGems/util/SimpleVector.h"
#include "minorGems/util/MinPriorityQueue.h"
#include "minorGems/game/Font.h"
#include "minorGems/util/stringUtils.h"
#include "minorGems/util/SettingsManager.h"
#include "minorGems/util/random/JenkinsRandomSource.h"
#include "minorGems/game/drawUtils.h"
#include "minorGems/game/gameGraphics.h"
#include "minorGems/io/file/File.h"
#include "minorGems/formats/encodingUtils.h"
#include "minorGems/system/Thread.h"
#include "minorGems/util/log/AppLog.h"
#include "minorGems/crypto/hashes/sha1.h"
#include "handler/Socket.h"
#include "../objectBank.h"
#include "../spriteBank.h"
#include "../transitionBank.h"
#include "../categoryBank.h"
#include "../soundBank.h"
#include "../whiteSprites.h"
#include "../message.h"
#include "../groundSprites.h"
#include "../accountHmac.h"
#include "../liveObjectSet.h"
#include "../ageControl.h"
#include "../musicPlayer.h"
#include "../emotion.h"
#include "../photos.h"
#include "../liveAnimationTriggers.h"
#include "../../commonSource/fractalNoise.h"
#include "../../commonSource/sayLimit.h"
#include "../zoomView.h"
#include "../../commonSource/handler/Message.h"

#include <stdlib.h>//#include <math.h>
#include <string>


#define OHOL_NON_EDITOR 1
#include "../ObjectPickable.h"

static ObjectPickable objectPickable;

#include "../minitech.h"

#define MAP_D 64
#define MAP_NUM_CELLS 4096

extern int bytesOutCount;
extern int bytesInCount;
extern double connectedTime;
extern int dataVersionNumber;
extern char forceDisconnect;
extern double frameRateFactor;
extern char *lastMessageSentToServer;
extern double lastServerMessageReceiveTime;
extern double largestPendingMessageTimeGap;
extern int messagesInCount;
extern int messagesOutCount;
extern int numServerBytesRead;
extern int numServerBytesSent;
extern int overheadServerBytesSent;
extern char pendingCMData;
extern char *pendingMapChunkMessage;
extern SimpleVector<char*> serverFrameMessages;
extern char serverSocketConnected;
extern SimpleVector<unsigned char> serverSocketBuffer;
extern SimpleVector<char*> readyPendingReceivedMessages;
extern double timeLastMessageSent;
extern int versionNumber;
extern char waitForFrameMessages;

extern Font *mainFont;
extern Font *numbersFontFixed;
extern Font *mainFontReview;
extern Font *handwritingFont;
extern Font *pencilFont;
extern Font *pencilErasedFont;

extern Font *titleFont;

// to make all erased pencil fonts lighter
static float pencilErasedFontExtraFade = 0.75;


extern doublePair lastScreenViewCenter;
doublePair LivingLifePage::minitechGetLastScreenViewCenter() { return lastScreenViewCenter; }

static char shouldMoveCamera = true;


extern double viewWidth;
extern double viewHeight;
extern int screenW, screenH;
extern char usingCustomServer;
extern char *serverIP;
extern int serverPort;
extern oneLife::client::game::handler::Socket* socketHandler;
extern char useSpawnSeed;
extern char *userEmail;
extern char *userTwinCode;
extern int userTwinCount;
extern char userReconnect;
static char vogMode = false;
static doublePair vogPos = { 0, 0 };
static char vogPickerOn = false;
extern float musicLoudness;

//KB
bool blockMouseScaling = false;

bool firstMovementStep = true;
int magnetMoveDir = -1;
int magnetWrongMoveDir = -1;
int magnetMoveCount = 0;

bool upKeyDown;
bool downKeyDown;
bool leftKeyDown;
bool rightKeyDown;

int lastDoorToOpenX;
int lastDoorToOpenY;

float lastPosX;
float lastPosY;

unsigned char charKey_Up = 'w';
unsigned char charKey_Down = 's';
unsigned char charKey_Left = 'a';
unsigned char charKey_Right = 'd';
unsigned char charKey_TileStandingOn = ' ';

unsigned char charKey_Backpack = 'q';
unsigned char charKey_TakeOffBackpack = 'b';
unsigned char charKey_Pocket = 't';
unsigned char charKey_Eat = 'e';
unsigned char charKey_Baby = 'c';

static bool waitForDoorToOpen;
//FOV
extern int gui_hud_mode;
extern float gui_fov_scale;
extern float gui_fov_scale_hud;
extern float gui_fov_target_scale_hud;
extern int gui_fov_offset_x;
extern int gui_fov_offset_y;
static SpriteHandle guiPanelLeftSprite;
static SpriteHandle guiPanelTileSprite;
static SpriteHandle guiPanelRightSprite;
static JenkinsRandomSource randSource( 340403 );
static JenkinsRandomSource remapRandSource( 340403 );
static int lastScreenMouseX, lastScreenMouseY;
static char mouseDown = false;
static int mouseDownFrames = 0;
static int minMouseDownFrames = 30;
static int screenCenterPlayerOffsetX, screenCenterPlayerOffsetY;
static float lastMouseX = 0;
static float lastMouseY = 0;

// set to true to render for teaser video
static char teaserVideo = false;
static int showBugMessage = 0;
static const char *bugEmail = "jason" "rohrer" "@" "fastmail.fm";

// if true, pressing S key (capital S)
// causes current speech and mask to be saved to the screenShots folder
static char savingSpeechEnabled = false;
static char savingSpeech = false;
static char savingSpeechColor = false;
static char savingSpeechMask = false;
static char savingSpeechNumber = 1;
static char takingPhoto = false;
static GridPos takingPhotoGlobalPos;
static char takingPhotoFlip = false;
static int photoSequenceNumber = -1;
static char waitingForPhotoSig = false;
static char *photoSig = NULL;
static double emotDuration = 10;
static int drunkEmotionIndex = -1;
static int trippingEmotionIndex = -1;
static int historyGraphLength = 100;
static char showFPS = false;
static double frameBatchMeasureStartTime = -1;
static int framesInBatch = 0;
static double fpsToDraw = -1;
static SimpleVector<double> fpsHistoryGraph;


static char showNet = false;
static double netBatchMeasureStartTime = -1;
static int messagesInPerSec = -1;
static int messagesOutPerSec = -1;
static int bytesInPerSec = -1;
static int bytesOutPerSec = -1;

static SimpleVector<double> messagesInHistoryGraph;
static SimpleVector<double> messagesOutHistoryGraph;
static SimpleVector<double> bytesInHistoryGraph;
static SimpleVector<double> bytesOutHistoryGraph;


static SimpleVector<GridPos> graveRequestPos;
static SimpleVector<GridPos> ownerRequestPos;



static char showPing = false;
static char showHelp = false;

static char *closeMessage = NULL;

static double pingSentTime = -1;
static double pongDeltaTime = -1;
static double pingDisplayStartTime = -1;


static double culvertFractalScale = 20;
static double culvertFractalRoughness = 0.62;
static double culvertFractalAmp = 98;


typedef struct LocationSpeech {
        doublePair pos;
        char *speech;
        double fade;
        // wall clock time when speech should start fading
        double fadeETATime;
    } LocationSpeech;



SimpleVector<LocationSpeech> locationSpeech;


static void clearLocationSpeech() {
    for( int i=0; i<locationSpeech.size(); i++ ) {
        delete [] locationSpeech.getElementDirect( i ).speech;
        }
    locationSpeech.deleteAll();
    }

//FOV
static double recalcOffsetX( double x, bool force = false ) {
    double res;
    if( gui_hud_mode == 0 || force ) {
        res = x + ( ( x > 0. ) ? gui_fov_offset_x : -gui_fov_offset_x );
        res /= 640. * gui_fov_target_scale_hud;
        res *= 640.;
        }
    else {
        res = x / gui_fov_target_scale_hud;
        }
    return res;
    }

static double recalcOffsetY( double y ) {
    double res;
    res = y + ( ( y > 0. ) ? gui_fov_offset_y : -gui_fov_offset_y );
    res /= 360. * gui_fov_target_scale_hud;
    res *= 360.;
    return res;
    }

static doublePair recalcOffset( doublePair ofs, bool force = false ) {
    ofs.x = recalcOffsetX( ofs.x, force );
    ofs.y = recalcOffsetY( ofs.y );
    return ofs;
    }


// most recent home at end

typedef struct {
        GridPos pos;
        char ancient;
    } HomePos;

static SimpleVector<HomePos> homePosStack;
static SimpleVector<HomePos> oldHomePosStack;
static int lastPlayerID = -1;// used on reconnect to decide whether to delete old home positions
static bool isTrippingEffectOn;

// returns pointer to record, NOT destroyed by caller, or NULL if 
// home unknown
static  GridPos *getHomeLocation()
{
    int num = homePosStack.size();
    if( num > 0 ) {
        return &( homePosStack.getElement( num - 1 )->pos );
        }
    else {
        return NULL;
        }
}

static void removeHomeLocation( int inX, int inY )
{
    for( int i=0; i<homePosStack.size(); i++ ) {
        GridPos p = homePosStack.getElementDirect( i ).pos;
        
        if( p.x == inX && p.y == inY ) {
            homePosStack.deleteElement( i );
            break;
            }
        }
}

static void addHomeLocation( int inX, int inY )
{
    removeHomeLocation( inX, inY );
    GridPos newPos = { inX, inY };
    HomePos p;
    p.pos = newPos;
    p.ancient = false;
    
    homePosStack.push_back( p );
}

static void addAncientHomeLocation( int inX, int inY )
{
    removeHomeLocation( inX, inY );

    // remove all ancient pos
    // there can be only one ancient
    for( int i=0; i<homePosStack.size(); i++ ) {
        if( homePosStack.getElementDirect( i ).ancient ) {
            homePosStack.deleteElement( i );
            i--;
            }
        }

    GridPos newPos = { inX, inY };
    HomePos p;
    p.pos = newPos;
    p.ancient = true;
    
    homePosStack.push_front( p );
}


// returns if -1 no home needs to be shown (home unknown)
// otherwise, returns 0..7 index of arrow
static int getHomeDir( doublePair inCurrentPlayerPos, 
                       double *outTileDistance = NULL,
                       char *outTooClose = NULL )
{
    GridPos *p = getHomeLocation();
    if( p == NULL )
    {
        return -1;
	}

    if( outTooClose != NULL )
    {
        *outTooClose = false;
    }

    doublePair homePos = { (double)p->x, (double)p->y };
    doublePair vector = sub( homePos, inCurrentPlayerPos );
    double dist = length( vector );

    if( outTileDistance != NULL )
    {
        *outTileDistance = dist;
	}

    if( dist < 5 )
    {
        // too close
        if( outTooClose != NULL )
        {
            *outTooClose = true;
        }
        
        if( dist == 0 )
        {
            // can't compute angle
            return -1;
		}
	}

    double a = angle( vector );

    // north is 0
    a -= M_PI / 2; 

    if( a <  - M_PI / 8 )
    {
        a += 2 * M_PI;
	}
    
    int index = lrint( 8 * a / ( 2 * M_PI ) );
    return index;
}

char *getRelationName( SimpleVector<int> *ourLin, 
                       SimpleVector<int> *theirLin, 
                       int ourID, int theirID,
                       int ourDisplayID, int theirDisplayID,
                       double ourAge, double theirAge,
                       int ourEveID, int theirEveID )
{
    

    ObjectRecord *theirDisplayO = getObject( theirDisplayID );
    
    char theyMale = false;
    
    if( theirDisplayO != NULL ) {
        theyMale = theirDisplayO->male;
        }
    

    if( ourLin->size() == 0 && theirLin->size() == 0 ) {
        // both eve, no relation
        return NULL;
        }

    const char *main = "";
    char grand = false;
    int numGreats = 0;
    int cousinNum = 0;
    int cousinRemovedNum = 0;
    
    char found = false;

    for( int i=0; i<theirLin->size(); i++ ) {
        if( theirLin->getElementDirect( i ) == ourID ) {
            found = true;
            
            if( theyMale ) {
                main = translate( "son" );
                }
            else {
                main = translate( "daughter" );
                }
            if( i > 0  ) {
                grand = true;
                }
            numGreats = i - 1;
            }
        }

    if( ! found ) {
        for( int i=0; i<ourLin->size(); i++ ) {
            if( ourLin->getElementDirect( i ) == theirID ) {
                found = true;
                main = translate( "mother" );
                if( i > 0  ) {
                    grand = true;
                    }
                numGreats = i - 1;
                }
            }
        }
    
    char big = false;
    char little = false;
    char twin = false;
    char identical = false;

    if( ! found ) {
        // not a direct descendent or ancestor

        // look for shared relation
        int ourMatchIndex = -1;
        int theirMatchIndex = -1;
        
        for( int i=0; i<ourLin->size(); i++ ) {
            for( int j=0; j<theirLin->size(); j++ ) {
                
                if( ourLin->getElementDirect( i ) == 
                    theirLin->getElementDirect( j ) ) {
                    ourMatchIndex = i;
                    theirMatchIndex = j;
                    break;
                    }
                }
            if( ourMatchIndex != -1 ) {
                break;
                }
            }
        
        if( ourMatchIndex == -1 ) {
            
            if( ourEveID != -1 && theirEveID != -1 &&
                ourEveID == theirEveID ) {
                // no shared lineage, but same eve beyond lineage cuttoff
                return stringDuplicate( translate( "distantRelative" ) );
                }

            return NULL;
            }
        
        found = true;

        if( theirMatchIndex == 0 && ourMatchIndex == 0 ) {
            if( theyMale ) {
                main = translate( "brother" );
                }
            else {
                main = translate( "sister" );
                }
            
            if( ourAge < theirAge - 0.1 ) {
                big = true;
                }
            else if( ourAge > theirAge + 0.1 ) {
                little = true;
                }
            else {
                // close enough together in age
                twin = true;
                
                if( ourDisplayID == theirDisplayID ) {
                    identical = true;
                    }
                }
            }
        else if( theirMatchIndex == 0 ) {
            if( theyMale ) {
                main = translate( "uncle" );
                }
            else {
                main = translate( "aunt" );
                }
            numGreats = ourMatchIndex - 1;
            }
        else if( ourMatchIndex == 0 ) {
            if( theyMale ) {
                main = translate( "nephew" );
                }
            else {
                main = translate( "niece" );
                }
            numGreats = theirMatchIndex - 1;
            }
        else {
            // cousin of some kind
            
            main = translate( "cousin" );
            
            // shallowest determines cousin number
            // diff determines removed number
            if( ourMatchIndex <= theirMatchIndex ) {
                cousinNum = ourMatchIndex;
                cousinRemovedNum = theirMatchIndex - ourMatchIndex;
                }
            else {
                cousinNum = theirMatchIndex;
                cousinRemovedNum = ourMatchIndex - theirMatchIndex;
                }
            }
        }


    SimpleVector<char> buffer;
    
    buffer.appendElementString( translate( "your" ) );
    buffer.appendElementString( " " );


    if( numGreats <= 4 ) {    
        for( int i=0; i<numGreats; i++ ) {
            buffer.appendElementString( translate( "great" ) );
            }
        }
    else {
        char *greatCount = autoSprintf( "%dX %s", 
                                        numGreats, translate( "great") );
        buffer.appendElementString( greatCount );
        delete [] greatCount;
        }
    
    
    if( grand ) {
        buffer.appendElementString( translate( "grand" ) );
        }
    
    if( cousinNum > 0 ) {
        int remainingCousinNum = cousinNum;

        if( cousinNum >= 30 ) {
            buffer.appendElementString( translate( "distant" ) );
            remainingCousinNum = 0;
            }
        
        if( cousinNum > 20 && cousinNum < 30 ) {
            buffer.appendElementString( translate( "twenty" ) );
            remainingCousinNum = cousinNum - 20;
            }

        if( remainingCousinNum > 0  ) {
            char *numth = autoSprintf( "%dth", remainingCousinNum );
            buffer.appendElementString( translate( numth ) );
            delete [] numth;
            }
        buffer.appendElementString( " " );
        }

    if( little ) {
        buffer.appendElementString( translate( "little" ) );
        buffer.appendElementString( " " );
        }
    else if( big ) {
        buffer.appendElementString( translate( "big" ) );
        buffer.appendElementString( " " );
        }
    else if( twin ) {
        if( identical ) {
            buffer.appendElementString( translate( "identical" ) );
            buffer.appendElementString( " " );
            }
        
        buffer.appendElementString( translate( "twin" ) );
        buffer.appendElementString( " " );
        }
    
    
    buffer.appendElementString( main );
    
    if( cousinRemovedNum > 0 ) {
        buffer.appendElementString( " " );
        
        if( cousinRemovedNum > 9 ) {
            buffer.appendElementString( translate( "manyTimes" ) );
            }
        else {
            char *numTimes = autoSprintf( "%dTimes", cousinRemovedNum );
            buffer.appendElementString( translate( numTimes ) );
            delete [] numTimes;
            }
        buffer.appendElementString( " " );
        buffer.appendElementString( translate( "removed" ) );
        }

    return buffer.getElementString();
    }



char *getRelationName( LiveObject *inOurObject, LiveObject *inTheirObject )
{
    SimpleVector<int> *ourLin = &( inOurObject->lineage );
    SimpleVector<int> *theirLin = &( inTheirObject->lineage );
    
    int ourID = inOurObject->id;
    int theirID = inTheirObject->id;

    
    return getRelationName( ourLin, theirLin, ourID, theirID,
                            inOurObject->displayID, inTheirObject->displayID,
                            inOurObject->age,
                            inTheirObject->age,
                            inOurObject->lineageEveID,
                            inTheirObject->lineageEveID );
}


// base speed for animations that aren't speeded up or slowed down
// when player moving at a different speed, anim speed is modified
#define BASE_SPEED 3.75

int overheadServerBytesRead = 0;
static char hideGuiPanel = false;
static float connectionMessageFade = 1.0f;

void LivingLifePage::sendToServerSocket( char *inMessage )
{
	socketHandler->sendMessage(inMessage);
	if(!socketHandler->isLastSendSucceed())
	{
		socketHandler->close();
		if(this->mFirstServerMessagesReceived)
		{
			if(this->mDeathReason != NULL)
			{
				delete [] this->mDeathReason;
			}
			this->mDeathReason = stringDuplicate(translate("reasonDisconnected"));
			handleOurDeath( true );
		}
		else
		{
			setWaiting( false );
			setSignal( "loginFailed" );
		}
	}
}

static char equal( GridPos inA, GridPos inB ) {
    if( inA.x == inB.x && inA.y == inB.y ) {
        return true;
        }
    return false;
    }


static double distance2( GridPos inA, GridPos inB ) {
    int dX = inA.x - inB.x;
    int dY = inA.y - inB.y;
    
    return dX * dX + dY * dY;
    }



static void printPath( GridPos *inPath, int inLength ) {
    for( int i=0; i<inLength; i++ ) {
        printf( "(%d,%d) ", inPath[i].x, inPath[i].y );
        }
    printf( "\n" );
    }


static void removeDoubleBacksFromPath( GridPos **inPath, int *inLength ) {
    
    SimpleVector<GridPos> filteredPath;
    
    int dbA = -1;
    int dbB = -1;
    
    int longestDB = 0;

    GridPos *path = *inPath;
    int length = *inLength;

    for( int e=0; e<length; e++ ) {
                                    
        for( int f=e; f<length; f++ ) {
            
            if( equal( path[e],
                       path[f] ) ) {
                                            
                int dist = f - e;
                                            
                if( dist > longestDB ) {
                                                
                    dbA = e;
                    dbB = f;
                    longestDB = dist;
                    }
                }
            }
        }
                                
    if( longestDB > 0 ) {
                                    
        printf( "Removing loop with %d elements\n",
                longestDB );

        for( int e=0; e<=dbA; e++ ) {
            filteredPath.push_back( 
                path[e] );
            }
                                    
        // skip loop between

        for( int e=dbB + 1; e<length; e++ ) {
            filteredPath.push_back( 
                path[e] );
            }
                                    
        *inLength = filteredPath.size();
                                    
        delete [] path;
                                    
        *inPath = 
            filteredPath.getElementArray();
        }
    }



static double computeCurrentAgeNoOverride( LiveObject *inObj ) {
    if( inObj->finalAgeSet ) {
        return inObj->age;
        }
    else {
        return inObj->age +
            inObj->ageRate * ( game_getCurrentTime() - inObj->lastAgeSetTime );
        }
    }




static double computeCurrentAge( LiveObject *inObj ) {
    if( inObj->finalAgeSet ) {
        return inObj->age;
        }
    else {
        if( inObj->tempAgeOverrideSet ) {
            double curTime = game_getCurrentTime();
            
            if( curTime - inObj->tempAgeOverrideSetTime < 5 ) {
                // baby cries for 5 seconds each time they speak
            
                // update age using clock
                return computeDisplayAge( inObj->tempAgeOverride +
                    inObj->ageRate * 
                    ( curTime - inObj->tempAgeOverrideSetTime ) );
                }
            else {
                // temp override over
                inObj->tempAgeOverrideSet = false;
                }
            }
        
        // update age using clock
        return computeDisplayAge( inObj->age +
            inObj->ageRate * ( game_getCurrentTime() - inObj->lastAgeSetTime ) );
        }
    
    }




static void stripDescriptionComment( char *inString ) {
    // pound sign is used for trailing developer comments
    // that aren't show to end user, cut them off if they exist
    char *firstPound = strstr( inString, "#" );
            
    if( firstPound != NULL ) {
        firstPound[0] = '\0';
        }
    }



static char *getDisplayObjectDescription( int inID ) {
    ObjectRecord *o = getObject( inID );
    if( o == NULL ) {
        return NULL;
        }
    char *upper = stringToUpperCase( o->description );
    stripDescriptionComment( upper );
    return upper;
    }

char *LivingLifePage::minitechGetDisplayObjectDescription( int objId ) { 
    ObjectRecord *o = getObject( objId );
    if( o == NULL ) {
		return NULL;
    }
	return getDisplayObjectDescription(objId);
}

doublePair getVectorFromCamera( int inMapX, int inMapY ) {
    doublePair vector = 
        { inMapX - lastScreenViewCenter.x / CELL_D, 
          inMapY - lastScreenViewCenter.y / CELL_D };
    
    return vector;
    }

doublePair gridToDouble( GridPos inGridPos ) {
    doublePair d = { (double) inGridPos.x, (double) inGridPos.y };
    
    return d;
    }

static char isGridAdjacent( int inXA, int inYA, int inXB, int inYB ) {
    if( ( abs( inXA - inXB ) == 1 && inYA == inYB ) 
        ||
        ( abs( inYA - inYB ) == 1 && inXA == inXB ) ) {
        
        return true;
        }

    return false;
    }



static GridPos sub( GridPos inA, GridPos inB ) {
    GridPos result = { inA.x - inB.x, inA.y - inB.y };
    
    return result;
    }






// measure a possibly truncated path, compensating for diagonals
static double measurePathLength( LiveObject *inObject,
                                 int inPathLength ) {
    // diags are square root of 2 in length
    double diagLength = 1.4142356237;
    

    double totalLength = 0;
    
    if( inPathLength < 2 ) {
        return totalLength;
        }
    

    GridPos lastPos = inObject->pathToDest[0];
    
    for( int i=1; i<inPathLength; i++ ) {
        
        GridPos thisPos = inObject->pathToDest[i];
        
        if( thisPos.x != lastPos.x &&
            thisPos.y != lastPos.y ) {
            totalLength += diagLength;
            }
        else {
            // not diag
            totalLength += 1;
            }
        lastPos = thisPos;
        }
    
    return totalLength;
    }




double LivingLifePage::computePathSpeedMod( LiveObject *inObject,
                                            int inPathLength ) {
    
    if( inPathLength < 1 ) {
        return 1;
        }    

    GridPos lastPos = inObject->pathToDest[0];
    
    int mapI = getMapIndex( lastPos.x, lastPos.y );

    if( mapI == -1 ) {
        return 1;
        }
    int floor = mMapFloors[ mapI ];
    
    if( floor <= 0 ) {
        return 1;
        }
    double speedMult = getObject( floor )->speedMult;
    
    if( speedMult == 1 ) {
        return 1;
        }

    for( int i=1; i<inPathLength; i++ ) {
        
        GridPos thisPos = inObject->pathToDest[i];

        mapI = getMapIndex( thisPos.x, thisPos.y );

        if( mapI == -1 ) {
            return 1;
            }
        int thisFloor = mMapFloors[ mapI ];
    
        if( floor != thisFloor ) {
            return 1;
            }
        }
    return speedMult;
    }





// youngest last
SimpleVector<LiveObject> gameObjects;

// for determining our ID when we're not youngest on the server
// (so we're not last in the list after receiving the first PU message)
int recentInsertedGameObjectIndex = -1;



static LiveObject *getGameObject( int inID ) {
    for( int i=0; i<gameObjects.size(); i++ ) {
        
        LiveObject *o = gameObjects.getElement( i );
        
        if( o->id == inID ) {
            return o;
            }
        }
    return NULL;
    }



extern char autoAdjustFramerate;
extern int baseFramesPerSecond;


void updateMoveSpeed( LiveObject *inObject ) {
    double etaSec = inObject->moveEtaTime - game_getCurrentTime();
    
    double moveLeft = measurePathLength( inObject, inObject->pathLength ) -
        measurePathLength( inObject, inObject->currentPathStep + 1 );
    

    // count number of turns, which we execute faster than we should
    // because of path smoothing,
    // and use them to reduce overall speed to compensate
    int numTurns = 0;

    if( inObject->currentPathStep < inObject->pathLength - 1 ) {
        GridPos lastDir = sub( 
            inObject->pathToDest[inObject->currentPathStep + 1],
            inObject->pathToDest[inObject->currentPathStep] );
        
        for( int p=inObject->currentPathStep+1; 
             p<inObject->pathLength -1; p++ ) {
            
            GridPos dir = sub( 
                inObject->pathToDest[p+1],
                inObject->pathToDest[p] );

            if( !equal( dir, lastDir ) ) {
                numTurns++;
                lastDir = dir;
                }
            }

        }
    
    double turnTimeBoost = 0.08 * numTurns;

    etaSec += turnTimeBoost;
    
    if( etaSec < 0.1 ) {
        // less than 1/10 of a second
        // this includes 0 values and negative values
        // we DO NOT want infinite or negative move speeds

        printf( "updateMoveSpeed sees etaSec of %f, too low, "
                "upping to 0.1 sec\n", etaSec );
        
        etaSec = 0.1;
        }
    

    double speedPerSec = moveLeft / etaSec;


    // pretend that frame rate is constant
    double fps = baseFramesPerSecond / frameRateFactor;
    
    inObject->currentSpeed = speedPerSec / fps;
    printf( "fixed speed = %f\n", inObject->currentSpeed );
    
    inObject->currentGridSpeed = speedPerSec;
    
    // slow move speed for testing
    //inObject->currentSpeed *= 0.25;

    inObject->timeOfLastSpeedUpdate = game_getCurrentTime();
    }



static void fixSingleStepPath( LiveObject *inObject ) {
    
    printf( "Fix for overtruncated, single-step path for player %d\n", 
            inObject->id );
    
    // trimmed path too short
    // needs to have at least
    // a start and end pos
    
    // give it an artificial
    // start pos
    

    doublePair nextWorld =
        gridToDouble( 
         inObject->pathToDest[0] );

    
    doublePair vectorAway;

    if( ! equal( 
            inObject->currentPos,
            nextWorld ) ) {
            
        vectorAway = normalize(
            sub( 
                inObject->
                currentPos,
                nextWorld ) );
        }
    else {
        vectorAway.x = 1;
        vectorAway.y = 0;
        }
    
    GridPos oldPos =
        inObject->pathToDest[0];
    
    delete [] inObject->pathToDest;
    inObject->pathLength = 2;
    
    inObject->pathToDest =
        new GridPos[2];
    inObject->pathToDest[0].x =
        oldPos.x + vectorAway.x;
    inObject->pathToDest[0].y =
        oldPos.y + vectorAway.y;
    
    inObject->pathToDest[1] =
        oldPos;
    }



// should match limit on server
static int pathFindingD = 32;


void LivingLifePage::computePathToDest( LiveObject *inObject ) {
    
    GridPos start;
    start.x = lrint( inObject->currentPos.x );
    start.y = lrint( inObject->currentPos.y );

    // make sure start is on our last path, if we have one
    if( inObject->pathToDest != NULL ) {
        char startOnPath = false;

        for( int i=0; i<inObject->pathLength; i++ ) {
            if( inObject->pathToDest[i].x == start.x &&
                inObject->pathToDest[i].y == start.y ) {
                startOnPath = true;
                break;
                }
            }
        
        if( ! startOnPath ) {
            // find closest spot on old path
            int closestI = -1;
            int closestD2 = 9999999;
            
            for( int i=0; i<inObject->pathLength; i++ ) {
                int dx = inObject->pathToDest[i].x - start.x;
                int dy = inObject->pathToDest[i].y - start.y;
                
                int d2 = dx * dx + dy * dy;
                
                if( d2 < closestD2 ) {
                    closestD2 = d2;
                    closestI = i;
                    }
                }

            start = inObject->pathToDest[ closestI ];
            }
        }
    
                
            

    if( inObject->pathToDest != NULL ) {
        delete [] inObject->pathToDest;
        inObject->pathToDest = NULL;
        }


    GridPos end = { inObject->xd, inObject->yd };
        
    // window around player's start position
    int numPathMapCells = pathFindingD * pathFindingD;
    char *blockedMap = new char[ numPathMapCells ];

    // assume all blocked
    memset( blockedMap, true, numPathMapCells );

    int pathOffsetX = pathFindingD/2 - start.x;
    int pathOffsetY = pathFindingD/2 - start.y;


    for( int y=0; y<pathFindingD; y++ ) {
        int mapY = ( y - pathOffsetY ) + mMapD / 2 - mMapOffsetY;
        
        for( int x=0; x<pathFindingD; x++ ) {
            int mapX = ( x - pathOffsetX ) + mMapD / 2 - mMapOffsetX;
            
            if( mapY >= 0 && mapY < mMapD &&
                mapX >= 0 && mapX < mMapD ) { 

                int mapI = mapY * mMapD + mapX;
            
                // note that unknowns (-1) count as blocked too
                if( mMap[ mapI ] == 0
                    ||
                    ( mMap[ mapI ] != -1 && 
                      ! getObject( mMap[ mapI ] )->blocksWalking ) ) {
                    
                    blockedMap[ y * pathFindingD + x ] = false;
                    }
                }
            }
        }

    // now add extra blocked spots for wide objects
    for( int y=0; y<pathFindingD; y++ ) {
        int mapY = ( y - pathOffsetY ) + mMapD / 2 - mMapOffsetY;
        
        for( int x=0; x<pathFindingD; x++ ) {
            int mapX = ( x - pathOffsetX ) + mMapD / 2 - mMapOffsetX;
            
            if( mapY >= 0 && mapY < mMapD &&
                mapX >= 0 && mapX < mMapD ) { 

                int mapI = mapY * mMapD + mapX;
                
                if( mMap[ mapI ] > 0 ) {
                    ObjectRecord *o = getObject( mMap[ mapI ] );
                    
                    if( o->wide ) {
                        
                        for( int dx = - o->leftBlockingRadius;
                             dx <= o->rightBlockingRadius; dx++ ) {
                            
                            int newX = x + dx;
                            
                            if( newX >=0 && newX < pathFindingD ) {
                                blockedMap[ y * pathFindingD + newX ] = true;
                                }
                            }
                        }
                    }
                }
            }
        }
    
    
    start.x += pathOffsetX;
    start.y += pathOffsetY;
    
    end.x += pathOffsetX;
    end.y += pathOffsetY;
    
    double startTime = game_getCurrentTime();

    GridPos closestFound;
    
    char pathFound = false;
    
    if( inObject->useWaypoint ) {
        GridPos waypoint = { inObject->waypointX, inObject->waypointY };
        waypoint.x += pathOffsetX;
        waypoint.y += pathOffsetY;
        
        pathFound = pathFind( pathFindingD, pathFindingD,
                              blockedMap, 
                              start, waypoint, end, 
                              &( inObject->pathLength ),
                              &( inObject->pathToDest ),
                              &closestFound );
        if( pathFound && inObject->pathToDest != NULL &&
            inObject->pathLength > inObject->maxWaypointPathLength ) {
            
            // path through waypoint too long, use waypoint as dest
            // instead
            delete [] inObject->pathToDest;
            pathFound = pathFind( pathFindingD, pathFindingD,
                                  blockedMap, 
                                  start, waypoint, 
                                  &( inObject->pathLength ),
                                  &( inObject->pathToDest ),
                                  &closestFound );
            inObject->xd = inObject->waypointX;
            inObject->yd = inObject->waypointY;
            inObject->destTruncated = false;
            }
        }
    else {
        pathFound = pathFind( pathFindingD, pathFindingD,
                              blockedMap, 
                              start, end, 
                              &( inObject->pathLength ),
                              &( inObject->pathToDest ),
                              &closestFound );
        }
        

    if( pathFound && inObject->pathToDest != NULL ) {
        printf( "Path found in %f ms\n", 
                1000 * ( game_getCurrentTime() - startTime ) );

        // move into world coordinates
        for( int i=0; i<inObject->pathLength; i++ ) {
            inObject->pathToDest[i].x -= pathOffsetX;
            inObject->pathToDest[i].y -= pathOffsetY;
            }

        inObject->shouldDrawPathMarks = false;
        
        // up, down, left, right
        int dirsInPath[4] = { 0, 0, 0, 0 };
        
        for( int i=1; i<inObject->pathLength; i++ ) {
            if( inObject->pathToDest[i].x > inObject->pathToDest[i-1].x ) {
                dirsInPath[3]++;
                }
            if( inObject->pathToDest[i].x < inObject->pathToDest[i-1].x ) {
                dirsInPath[2]++;
                }
            if( inObject->pathToDest[i].y > inObject->pathToDest[i-1].y ) {
                dirsInPath[1]++;
                }
            if( inObject->pathToDest[i].y < inObject->pathToDest[i-1].y ) {
                dirsInPath[0]++;
                }
            }
        
        if( ( dirsInPath[0] > 1 && dirsInPath[1] > 1 )
            ||
            ( dirsInPath[2] > 1 && dirsInPath[3] > 1 ) ) {

            // path contains switchbacks, making in confusing without
            // path marks
            inObject->shouldDrawPathMarks = true;
            }
        
        GridPos aGridPos = inObject->pathToDest[0];
        GridPos bGridPos = inObject->pathToDest[1];
        
        doublePair aPos = { (double)aGridPos.x, (double)aGridPos.y };
        doublePair bPos = { (double)bGridPos.x, (double)bGridPos.y };
        
        inObject->currentMoveDirection =
            normalize( sub( bPos, aPos ) );
        }
    else {
        printf( "Path not found in %f ms\n", 
                1000 * ( game_getCurrentTime() - startTime ) );
        
        if( !pathFound ) {
            
            inObject->closestDestIfPathFailedX = 
                closestFound.x - pathOffsetX;
            
            inObject->closestDestIfPathFailedY = 
                closestFound.y - pathOffsetY;
            }
        else {
            // degen case where start == end?
            inObject->closestDestIfPathFailedX = inObject->xd;
            inObject->closestDestIfPathFailedY = inObject->yd;
            }
        
        
        }
    
    inObject->currentPathStep = 0;
    inObject->numFramesOnCurrentStep = 0;
    inObject->onFinalPathStep = false;
    
    delete [] blockedMap;
    }



static void addNewAnimDirect( LiveObject *inObject, AnimType inNewAnim ) {
    inObject->lastAnim = inObject->curAnim;
    inObject->curAnim = inNewAnim;
    inObject->lastAnimFade = 1;
            
    inObject->lastAnimationFrameCount = inObject->animationFrameCount;

    if( inObject->lastAnim == moving ) {
        inObject->frozenRotFrameCount = inObject->lastAnimationFrameCount;
        inObject->frozenRotFrameCountUsed = false;
        }
    else if( inObject->curAnim == moving &&
             inObject->lastAnim != moving &&
             inObject->frozenRotFrameCountUsed ) {
        // switching back to moving
        // resume from where frozen
        inObject->animationFrameCount = inObject->frozenRotFrameCount;
        }
    else if( ( inObject->curAnim == ground || inObject->curAnim == ground2 )
             &&
             inObject->lastAnim == held ) {
        // keep old frozen frame count as we transition away
        // from held
        }
    }



static void addNewHeldAnimDirect( LiveObject *inObject, AnimType inNewAnim ) {
    inObject->lastHeldAnim = inObject->curHeldAnim;
    inObject->curHeldAnim = inNewAnim;
    inObject->lastHeldAnimFade = 1;
    
    inObject->lastHeldAnimationFrameCount = 
        inObject->heldAnimationFrameCount;

    if( inObject->lastHeldAnim == moving ) {
        inObject->heldFrozenRotFrameCount = 
            inObject->lastHeldAnimationFrameCount;
        inObject->heldFrozenRotFrameCountUsed = false;
        }
    else if( inObject->curHeldAnim == moving &&
             inObject->lastHeldAnim != moving &&
             inObject->heldFrozenRotFrameCountUsed ) {
        // switching back to moving
        // resume from where frozen
        inObject->heldAnimationFrameCount = inObject->heldFrozenRotFrameCount;
        }
    else if( ( inObject->curHeldAnim == ground || 
               inObject->curHeldAnim == ground2 ) 
             &&
             inObject->lastHeldAnim == held ) {
        // keep old frozen frame count as we transition away
        // from held
        }
    }


static void addNewAnimPlayerOnly( LiveObject *inObject, AnimType inNewAnim ) {
    
    if( inObject->curAnim != inNewAnim || 
        inObject->futureAnimStack->size() > 0 ) {
        

        // if we're still in the middle of fading, finish the fade,
        // by pushing this animation on the stack...
        // ...but NOT if we're fading TO ground.
        // Cut that off, and start our next animation right away.
        if( inObject->lastAnimFade != 0 && 
            inObject->curAnim != ground &&
            inObject->curAnim != ground2 ) {
                        
            // don't double stack
            if( inObject->futureAnimStack->size() == 0 ||
                inObject->futureAnimStack->getElementDirect(
                    inObject->futureAnimStack->size() - 1 ) 
                != inNewAnim ) {
                
                // don't push another animation after ground
                // that animation will replace ground (no need to go to
                // ground between animations.... can just go straight 
                // to the next animation
                char foundNonGround = false;
                
                while( ! foundNonGround &&
                       inObject->futureAnimStack->size() > 0 ) {
                    
                    int prevAnim =
                        inObject->futureAnimStack->getElementDirect(
                            inObject->futureAnimStack->size() - 1 );
                    
                    if( prevAnim == ground || prevAnim == ground2 ) {
                        inObject->futureAnimStack->deleteElement(
                            inObject->futureAnimStack->size() - 1 );
                        }
                    else {
                        foundNonGround = true;
                        }
                    }
                
                inObject->futureAnimStack->push_back( inNewAnim );
                }
            }
        else {
            addNewAnimDirect( inObject, inNewAnim );
            }
        }
    }


static void addNewAnim( LiveObject *inObject, AnimType inNewAnim ) {
    addNewAnimPlayerOnly( inObject, inNewAnim );

    AnimType newHeldAnim = inNewAnim;
    
    if( inObject->holdingID < 0 ) {
        // holding a baby
        // never show baby's moving animation
        // baby always stuck in held animation when being held

        newHeldAnim = held;
        }
    else if( inObject->holdingID > 0 && 
             ( newHeldAnim == ground || newHeldAnim == ground2 || 
               newHeldAnim == doing || newHeldAnim == eating ) ) {
        // ground is used when person comes to a hault,
        // but for the held item, we should still show the held animation
        // same if person is starting a doing or eating animation
        newHeldAnim = held;
        }

    if( inObject->curHeldAnim != newHeldAnim ) {

        if( inObject->lastHeldAnimFade != 0 ) {
            
            // don't double stack
            if( inObject->futureHeldAnimStack->size() == 0 ||
                inObject->futureHeldAnimStack->getElementDirect(
                    inObject->futureHeldAnimStack->size() - 1 ) 
                != newHeldAnim ) {
                
                inObject->futureHeldAnimStack->push_back( newHeldAnim );
                }
            }
        else {
            addNewHeldAnimDirect( inObject, newHeldAnim );
            }
        }    
        
    }






// if user clicks to initiate an action while still moving, we
// queue it here
static char *nextActionMessageToSend = NULL;
static char nextActionEating = false;
static char nextActionDropping = false;


// block move until next PLAYER_UPDATE received after action sent
static char playerActionPending = false;
static int playerActionTargetX, playerActionTargetY;
static char playerActionTargetNotAdjacent = false;


static char waitingForPong = false;
static int lastPingSent = 0;
static int lastPongReceived = 0;


int ourID;

static int valleySpacing = 40;
static int valleyOffset = 0;


char lastCharUsed = 'A';

char mapPullMode = false;
int mapPullStartX = -10;
int mapPullEndX = 10;
int mapPullStartY = -10;
int mapPullEndY = 10;

int mapPullCurrentX;
int mapPullCurrentY;
char mapPullCurrentSaved = false;
char mapPullCurrentSent = false;
char mapPullModeFinalImage = false;

Image *mapPullTotalImage = NULL;
int numScreensWritten = 0;


static int apocalypseInProgress = false;
static double apocalypseDisplayProgress = 0;
static double apocalypseDisplaySeconds = 6;

static double remapPeakSeconds = 60;
static double remapDelaySeconds = 30;


//EXTENDED FUNCTIONALITY
void LivingLifePage::setNextActionMessage(const char* msg, int x, int y) {
	if( nextActionMessageToSend != NULL ) {
		delete [] nextActionMessageToSend;
		nextActionMessageToSend = NULL;
	}

	playerActionTargetX = x;
	playerActionTargetY = y;
	playerActionTargetNotAdjacent = true;
	nextActionDropping = false;
	nextActionEating = false;
	nextActionMessageToSend = autoSprintf( "%s", msg );
}

int LivingLifePage::getObjId( int tileX, int tileY ) {
	int mapX = tileX - mMapOffsetX + mMapD / 2;
	int mapY = tileY - mMapOffsetY + mMapD / 2;
	int i = mapY * mMapD + mapX;
	if (i < 0 || i >= mMapD*mMapD) return -1;
	return mMap[i];
}

bool LivingLifePage::objIdReverseAction( int objId ) {
	LiveObject *ourLiveObject = getOurLiveObject();
	
	if (objId <= 0) return false;

	bool r = false;
	if ( ourLiveObject->holdingID <= 0 ) {
		switch (objId) {
			case 253: // full berry clay bowl
			case 225: // wheat bundle
				return true;
				break;
		}
		if ( getObject(objId) ) {
			char* descr	= getObject(objId)->description;
			if ( strstr(descr, "Bowl of") != NULL ) {
				return true;
			}
		}
	}
	return r;
}

void LivingLifePage::pickUpBabyInRange() {
	LiveObject *ourLiveObject = getOurLiveObject();
	
	if ( computeCurrentAge( ourLiveObject ) < 13 ) return;

	if ( ourLiveObject->holdingID != 0 ) {
		dropTileRelativeToMe( 0, 0 );
		return;
	}

	// find new baby to pick up - prefer babies further away
	int babyFound = false;
	int babyX = 0;
	int babyY = 0;
	for(int i=0; i<gameObjects.size(); i++) {
		LiveObject *o = gameObjects.getElement( i );
			
		if ( computeCurrentAge( o ) > 5 ) continue;

		if ( o->xd != ourLiveObject->xd && o->yd != ourLiveObject->yd ) continue; 
		if ( !babyFound ) {
			if ( o->xd == ourLiveObject->xd && o->yd == ourLiveObject->yd ) {
				babyFound = true;
				babyX = o->xd;
				babyY = o->yd;
				continue;
			}
		}
		int posDiff = 0;
		if ( o->xd == ourLiveObject->xd) posDiff = o->yd - ourLiveObject->yd;
		else if ( o->yd == ourLiveObject->yd) posDiff = o->xd - ourLiveObject->xd;
		if (posDiff > 1 || posDiff < -1) continue;

		pickUpBaby( o->xd, o->yd );
		return;
	}
	if ( !babyFound ) return;
	pickUpBaby( babyX, babyY );
}

void LivingLifePage::pickUpBaby( int x, int y ) {
	char msg[32];
	sprintf( msg, "BABY %d %d#", x, y );
	setNextActionMessage( msg, x, y );
}

void LivingLifePage::useBackpack(bool replace) {
	LiveObject *ourLiveObject = getOurLiveObject();
	
	int clothingSlot = 5; // backpack clothing slot

	int x, y;
	setOurSendPosXY(x, y);

	char msg[32];
	if( ourLiveObject->holdingID > 0 ) {
		if (replace) {
			sprintf( msg, "DROP %d %d %d#", x, y, clothingSlot );
		} else {
			sprintf( msg, "SELF %d %d %d#", x, y, clothingSlot );
		}
		setNextActionMessage( msg, x, y );
		nextActionDropping = true;
	} else {
		sprintf( msg, "SREMV %d %d %d %d#", x, y, clothingSlot, -1 );
		setNextActionMessage( msg, x, y );
	}
}

void LivingLifePage::usePocket(int clothingID) {
	LiveObject *ourLiveObject = getOurLiveObject();
	
	int x, y;
	setOurSendPosXY(x, y);

	char msg[32];
	if( ourLiveObject->holdingID > 0 ) {
		sprintf( msg, "DROP %d %d %d#", x, y, clothingID );
		setNextActionMessage( msg, x, y );
		nextActionDropping = true;
	} else {
		sprintf( msg, "SREMV %d %d %d %d#", x, y, clothingID, -1 );
		setNextActionMessage( msg, x, y );
	}
}

void LivingLifePage::useOnSelf() {
	LiveObject *ourLiveObject = getOurLiveObject();
	
	int x, y;
	setOurSendPosXY(x, y);

	if( ourLiveObject->holdingID <= 0 ) return;

	char msg[32];
	sprintf( msg, "SELF %d %d %d#", x, y, -1 );
	setNextActionMessage( msg, x, y );

	if( getObject( ourLiveObject->holdingID )->foodValue > 0)
		nextActionEating = true;
}

void LivingLifePage::takeOffBackpack() {
	LiveObject *ourLiveObject = getOurLiveObject();
	
	char message[32];
	sprintf(message, "SELF %i %i 5#", ourLiveObject->xd, ourLiveObject->yd);
	sendToServerSocket( message );
}

void LivingLifePage::setOurSendPosXY(int &x, int &y) {
	LiveObject *ourLiveObject = getOurLiveObject();
	
	x = round( ourLiveObject->xd );
	y = round( ourLiveObject->yd );
	x = sendX(x);
	y = sendY(y);
}

bool LivingLifePage::isCharKey(unsigned char c, unsigned char key) {
	char tKey = key;
	return (c == key || c == toupper(tKey));
}

static Image *expandToPowersOfTwoWhite( Image *inImage ) {
    
    int w = 1;
    int h = 1;
                    
    while( w < inImage->getWidth() ) {
        w *= 2;
        }
    while( h < inImage->getHeight() ) {
        h *= 2;
        }
    
    return inImage->expandImage( w, h, true );
    }

static void splitAndExpandSprites( const char *inTgaFileName, int inNumSprites,
                                   SpriteHandle *inDestArray ) {
    
    Image *full = readTGAFile( inTgaFileName );
    if( full != NULL ) {
        
        int spriteW = full->getWidth() / inNumSprites;
        
        int spriteH = full->getHeight();
        
        for( int i=0; i<inNumSprites; i++ ) {
            
            Image *part = full->getSubImage( i * spriteW, 0, 
                                          spriteW, spriteH );
            Image *partExpanded = expandToPowersOfTwoWhite( part );
            
            
            delete part;

            inDestArray[i] = fillSprite( partExpanded, false );
            
            delete partExpanded;
            }

        delete full;
        }

    }

void LivingLifePage::clearMap() {
    for( int i=0; i<mMapD *mMapD; i++ ) {
        // -1 represents unknown
        // 0 represents known empty
        mMap[i] = -1;
        mMapBiomes[i] = -1;
        mMapFloors[i] = -1;
        
        mMapAnimationFrameCount[i] = randSource.getRandomBoundedInt( 0, 10000 );
        mMapAnimationLastFrameCount[i] = 
            randSource.getRandomBoundedInt( 0, 10000 );
        
        mMapAnimationFrozenRotFrameCount[i] = 0;
        mMapAnimationFrozenRotFrameCountUsed[i] = false;
        
        mMapFloorAnimationFrameCount[i] = 
            randSource.getRandomBoundedInt( 0, 10000 );

        mMapCurAnimType[i] = ground;
        mMapLastAnimType[i] = ground;
        mMapLastAnimFade[i] = 0;

        mMapDropOffsets[i].x = 0;
        mMapDropOffsets[i].y = 0;
        mMapDropRot[i] = 0;
        mMapDropSounds[i] = blankSoundUsage;

        mMapMoveOffsets[i].x = 0;
        mMapMoveOffsets[i].y = 0;
        mMapMoveSpeeds[i] = 0;
        
        mMapTileFlips[i] = false;
        
        mMapPlayerPlacedFlags[i] = false;
        }
    }



LivingLifePage::LivingLifePage() 
        : mServerSocket( -1 ), 
          mForceRunTutorial( false ),
          mTutorialNumber( 0 ),
          mGlobalMessageShowing( false ),
          mGlobalMessageStartTime( 0 ),
          mFirstServerMessagesReceived( 0 ),
          mMapGlobalOffsetSet( false ),
          mMapD( MAP_D ),
          mMapOffsetX( 0 ),
          mMapOffsetY( 0 ),
          mEKeyEnabled( false ),
          mEKeyDown( false ),
          mGuiPanelSprite( loadSprite( "guiPanel.tga", false ) ),
          mGuiBloodSprite( loadSprite( "guiBlood.tga", false ) ),
          mNotePaperSprite( loadSprite( "notePaper.tga", false ) ),
          mFloorSplitSprite( loadSprite( "floorSplit.tga", false ) ),
          mCellBorderSprite( loadWhiteSprite( "cellBorder.tga" ) ),
          mCellFillSprite( loadWhiteSprite( "cellFill.tga" ) ),
          mHomeSlipSprite( loadSprite( "homeSlip.tga", false ) ),
          mLastMouseOverID( 0 ),
          mCurMouseOverID( 0 ),
          mChalkBlotSprite( loadWhiteSprite( "chalkBlot.tga" ) ),
          mPathMarkSprite( loadWhiteSprite( "pathMark.tga" ) ),
          mSayField( handwritingFont, 0, 1000, 10, true, NULL,
                     "ABCDEFGHIJKLMNOPQRSTUVWXYZ.-,'?!/ " ),
          mDeathReason( NULL ),
          mShowHighlights( true ),
          mUsingSteam( false ),
          mZKeyDown( false ),
          mObjectPicker( &objectPickable, +510, 90 )
{
	socketHandler->mServerSocket = &mServerSocket;

    if( SettingsManager::getIntSetting( "useSteamUpdate", 0 ) ) {
        mUsingSteam = true;
        }

    mForceGroundClick = false;
    
    mYumSlipSprites[0] = loadSprite( "yumSlip1.tga", false );
    mYumSlipSprites[1] = loadSprite( "yumSlip2.tga", false );
    mYumSlipSprites[2] = loadSprite( "yumSlip3.tga", false );
    mYumSlipSprites[3] = loadSprite( "yumSlip4.tga", false );

    
    mCurMouseOverCell.x = -1;
    mCurMouseOverCell.y = -1;
    mCurMouseOverCellFade = 0.0f;
    mCurMouseOverCellFadeRate = 0.04;
    mLastClickCell.x = -1;
    mLastClickCell.y = -1;

    // we're not showing a cursor on note paper, so arrow key behavior
    // is confusing.
    mSayField.setIgnoreArrowKeys( true );
    // drawn under world at (0,1000), don't allow click to focus
    mSayField.setIgnoreMouse( true );
    
    initLiveTriggers();

    for( int i=0; i<4; i++ ) {
        char *name = autoSprintf( "ground_t%d.tga", i );    
        mGroundOverlaySprite[i] = loadSprite( name, false );
        delete [] name;
        }
    

    mMapGlobalOffset.x = 0;
    mMapGlobalOffset.y = 0;

    emotDuration = SettingsManager::getFloatSetting( "emotDuration", 10 );
	
    drunkEmotionIndex =
        SettingsManager::getIntSetting( "drunkEmotionIndex", 2 );
	
    trippingEmotionIndex =
        SettingsManager::getIntSetting( "trippingEmotionIndex", 2 );
          
    hideGuiPanel = SettingsManager::getIntSetting( "hideGameUI", 0 );

    mHungerSound = loadSoundSprite( "otherSounds", "hunger.aiff" );
    mPulseHungerSound = false;
    

    if( mHungerSound != NULL ) {
        toggleVariance( mHungerSound, true );
        }


    mTutorialSound = loadSoundSprite( "otherSounds", "tutorialChime.aiff" );

    if( mTutorialSound != NULL ) {
        toggleVariance( mTutorialSound, true );
        }

    mCurseSound = loadSoundSprite( "otherSounds", "curseChime.aiff" );

    if( mCurseSound != NULL ) {
        toggleVariance( mCurseSound, true );
        }
    

    mHungerSlipSprites[0] = loadSprite( "fullSlip.tga", false );
    mHungerSlipSprites[1] = loadSprite( "hungrySlip.tga", false );
    mHungerSlipSprites[2] = loadSprite( "starvingSlip.tga", false );
    

    // not visible, drawn under world at 0, 0, and doesn't move with camera
    // still, we can use it to receive/process/filter typing events
    addComponent( &mSayField );
    
    mSayField.unfocus();
    
    
    mNotePaperHideOffset.x = -242;
    mNotePaperHideOffset.y = -420;


    mHomeSlipHideOffset.x = 0;
    mHomeSlipHideOffset.y = -360;


    for( int i=0; i<NUM_YUM_SLIPS; i++ ) {    
        mYumSlipHideOffset[i].x = -140;
        mYumSlipHideOffset[i].y = -330;
        }
    
    mYumSlipHideOffset[2].x += 60;
    mYumSlipHideOffset[3].x += 70;

    for( int i=0; i<NUM_YUM_SLIPS; i++ ) {    
        mYumSlipPosOffset[i] = mYumSlipHideOffset[i];
        mYumSlipPosTargetOffset[i] = mYumSlipHideOffset[i];
        }
    

    for( int i=0; i<3; i++ ) {    
        mHungerSlipShowOffsets[i].x = -540;
        mHungerSlipShowOffsets[i].y = -250;
    
        mHungerSlipHideOffsets[i].x = -540;
        mHungerSlipHideOffsets[i].y = -370;
        
        mHungerSlipWiggleTime[i] = 0;
        mHungerSlipWiggleAmp[i] = 0;
        mHungerSlipWiggleSpeed[i] = 0.05;
        }
    mHungerSlipShowOffsets[2].y += 20;
    mHungerSlipHideOffsets[2].y -= 20;

    mHungerSlipShowOffsets[2].y -= 50;
    mHungerSlipShowOffsets[1].y -= 30;
    mHungerSlipShowOffsets[0].y += 18;


    mHungerSlipWiggleAmp[1] = 0.5;
    mHungerSlipWiggleAmp[2] = 0.5;

    mHungerSlipWiggleSpeed[2] = 0.075;

    mStarvingSlipLastPos[0] = 0;
    mStarvingSlipLastPos[1] = 0;
    

    for( int i=0; i<3; i++ ) {    
        mHungerSlipPosOffset[i] = mHungerSlipHideOffsets[i];
        mHungerSlipPosTargetOffset[i] = mHungerSlipPosOffset[i];
        }
    
    mHungerSlipVisible = -1;

    
    
    for( int i=0; i<NUM_HINT_SHEETS; i++ ) {
        char *name = autoSprintf( "hintSheet%d.tga", i + 1 );    
        mHintSheetSprites[i] = loadSprite( name, false );
        delete [] name;
        
        mHintHideOffset[i].x = 900;
        mHintHideOffset[i].y = -370;
        
        mHintTargetOffset[i] = mHintHideOffset[i];
        mHintPosOffset[i] = mHintHideOffset[i];
        
        mHintExtraOffset[i].x = 0;
        mHintExtraOffset[i].y = 0;

        mHintMessage[i] = NULL;
        mHintMessageIndex[i] = 0;
        
        mNumTotalHints[i] = 0;
        }
    
    mLiveHintSheetIndex = -1;

    mForceHintRefresh = false;
    mCurrentHintObjectID = 0;
    mCurrentHintIndex = 0;
    
    mNextHintObjectID = 0;
    mNextHintIndex = 0;
    
    mLastHintSortedSourceID = 0;
    
    int maxObjectID = getMaxObjectID();
    
    mHintBookmarks = new int[ maxObjectID + 1 ];

    for( int i=0; i<=maxObjectID; i++ ) {
        mHintBookmarks[i] = 0;
        }
    
    mHintFilterString = NULL;
    mLastHintFilterString = NULL;
    mPendingFilterString = NULL;
    
    

    for( int i=0; i<NUM_HINT_SHEETS; i++ ) {
        
        mTutorialHideOffset[i].x = -914;
        mTutorialFlips[i] = false;
        
        if( i % 2 == 1 ) {
            // odd on right side of screen
            mTutorialHideOffset[i].x = 914;
            mTutorialFlips[i] = true;
            }
        
        mTutorialHideOffset[i].y = 430;
        
        mTutorialTargetOffset[i] = mTutorialHideOffset[i];
        mTutorialPosOffset[i] = mTutorialHideOffset[i];

        mTutorialExtraOffset[i].x = 0;
        mTutorialExtraOffset[i].y = 0;
        
        mTutorialMessage[i] = "";


        mCravingHideOffset[i].x = -932;
        
        mCravingHideOffset[i].y = -370;
        
        mCravingTargetOffset[i] = mCravingHideOffset[i];
        mCravingPosOffset[i] = mCravingHideOffset[i];

        mCravingExtraOffset[i].x = 0;
        mCravingExtraOffset[i].y = 0;
        
        mCravingMessage[i] = NULL;        
        }
    
    mLiveTutorialSheetIndex = -1;
    mLiveTutorialTriggerNumber = -1;
    
    mLiveCravingSheetIndex = -1;

    //FOV
	calcOffsetHUD();

	Image *tempImage = readTGAFile( "guiPanel.tga" );
	Image *tempImage2;

	tempImage2 = tempImage->getSubImage( tempImage->getWidth() / 2 - 640, 0, 512, tempImage->getHeight() );
	guiPanelLeftSprite = fillSprite( tempImage2 );
	delete tempImage2;

	tempImage2 = tempImage->getSubImage( tempImage->getWidth() / 2 - 128, 0, 256, tempImage->getHeight() );
	guiPanelTileSprite = fillSprite( tempImage2 );
	setSpriteWrapping( guiPanelTileSprite, true, false );
	delete tempImage2;

	tempImage2 = tempImage->getSubImage( tempImage->getWidth() / 2 + 640 - 512, 0, 512, tempImage->getHeight() );
	guiPanelRightSprite = fillSprite( tempImage2 );
	delete tempImage2;

	delete tempImage;
	//

    mMap = new int[ mMapD * mMapD ];
    mMapBiomes = new int[ mMapD * mMapD ];
    mMapFloors = new int[ mMapD * mMapD ];
    
    mMapCellDrawnFlags = new char[ mMapD * mMapD ];

    mMapContainedStacks = new SimpleVector<int>[ mMapD * mMapD ];
    mMapSubContainedStacks = 
        new SimpleVector< SimpleVector<int> >[ mMapD * mMapD ];
    
    mMapAnimationFrameCount =  new double[ mMapD * mMapD ];
    mMapAnimationLastFrameCount =  new double[ mMapD * mMapD ];
    mMapAnimationFrozenRotFrameCount =  new double[ mMapD * mMapD ];

    mMapAnimationFrozenRotFrameCountUsed =  new char[ mMapD * mMapD ];
    
    mMapFloorAnimationFrameCount =  new int[ mMapD * mMapD ];

    mMapCurAnimType =  new AnimType[ mMapD * mMapD ];
    mMapLastAnimType =  new AnimType[ mMapD * mMapD ];
    mMapLastAnimFade =  new double[ mMapD * mMapD ];
    
    mMapDropOffsets = new doublePair[ mMapD * mMapD ];
    mMapDropRot = new double[ mMapD * mMapD ];
    mMapDropSounds = new SoundUsage[ mMapD * mMapD ];

    mMapMoveOffsets = new doublePair[ mMapD * mMapD ];
    mMapMoveSpeeds = new double[ mMapD * mMapD ];

    mMapTileFlips = new char[ mMapD * mMapD ];
    
    mMapPlayerPlacedFlags = new char[ mMapD * mMapD ];
    

    clearMap();


    splitAndExpandSprites( "hungerBoxes.tga", NUM_HUNGER_BOX_SPRITES, 
                           mHungerBoxSprites );
    splitAndExpandSprites( "hungerBoxFills.tga", NUM_HUNGER_BOX_SPRITES, 
                           mHungerBoxFillSprites );

    splitAndExpandSprites( "hungerBoxesErased.tga", NUM_HUNGER_BOX_SPRITES, 
                           mHungerBoxErasedSprites );
    splitAndExpandSprites( "hungerBoxFillsErased.tga", NUM_HUNGER_BOX_SPRITES, 
                           mHungerBoxFillErasedSprites );

    splitAndExpandSprites( "tempArrows.tga", NUM_TEMP_ARROWS, 
                           mTempArrowSprites );
    splitAndExpandSprites( "tempArrowsErased.tga", NUM_TEMP_ARROWS, 
                           mTempArrowErasedSprites );

    splitAndExpandSprites( "hungerDashes.tga", NUM_HUNGER_DASHES, 
                           mHungerDashSprites );
    splitAndExpandSprites( "hungerDashesErased.tga", NUM_HUNGER_DASHES, 
                           mHungerDashErasedSprites );

    splitAndExpandSprites( "hungerBars.tga", NUM_HUNGER_DASHES, 
                           mHungerBarSprites );
    splitAndExpandSprites( "hungerBarsErased.tga", NUM_HUNGER_DASHES, 
                           mHungerBarErasedSprites );

    
    splitAndExpandSprites( "homeArrows.tga", NUM_HOME_ARROWS, 
                           mHomeArrowSprites );
    splitAndExpandSprites( "homeArrowsErased.tga", NUM_HOME_ARROWS, 
                           mHomeArrowErasedSprites );

    
    SimpleVector<int> *culvertStoneIDs = 
        SettingsManager::getIntSettingMulti( "culvertStoneSprites" );
    
    for( int i=0; i<culvertStoneIDs->size(); i++ ) {
        int id = culvertStoneIDs->getElementDirect( i );
        
        if( getSprite( id ) != NULL ) {
            mCulvertStoneSpriteIDs.push_back( id );
            }
        }
    delete culvertStoneIDs;


    mCurrentArrowI = 0;
    mCurrentArrowHeat = -1;
    mCurrentDes = NULL;
    mCurrentLastAteString = NULL;

    mShowHighlights = 
        SettingsManager::getIntSetting( "showMouseOverHighlights", 1 );

    mEKeyEnabled = 
        SettingsManager::getIntSetting( "eKeyForRightClick", 0 );


    if( teaserVideo ) {
        mTeaserArrowLongSprite = loadWhiteSprite( "teaserArrowLong.tga" );
        mTeaserArrowMedSprite = loadWhiteSprite( "teaserArrowMed.tga" );
        mTeaserArrowShortSprite = loadWhiteSprite( "teaserArrowShort.tga" );
        mTeaserArrowVeryShortSprite = 
            loadWhiteSprite( "teaserArrowVeryShort.tga" );
        
        mLineSegmentSprite = loadWhiteSprite( "lineSegment.tga" );
        }
          
    
    int tutorialDone = SettingsManager::getIntSetting( "tutorialDone", 0 );
    
    if( ! tutorialDone ) {
        mTutorialNumber = 1;
        }
		
	minitech::setLivingLifePage(
		this, 
		&gameObjects, 
		mMapD, 
		pathFindingD, 
		mMapContainedStacks, 
		mMapSubContainedStacks);
    }

void LivingLifePage::runTutorial() {
    mForceRunTutorial = true;
    }

void LivingLifePage::clearLiveObjects() {
    for( int i=0; i<gameObjects.size(); i++ ) {
        
        LiveObject *nextObject =
            gameObjects.getElement( i );
        
        nextObject->pendingReceivedMessages.deallocateStringElements();

        if( nextObject->containedIDs != NULL ) {
            delete [] nextObject->containedIDs;
            }

        if( nextObject->subContainedIDs != NULL ) {
            delete [] nextObject->subContainedIDs;
            }
        
        if( nextObject->pathToDest != NULL ) {
            delete [] nextObject->pathToDest;
            }

        if( nextObject->currentSpeech != NULL ) {
            delete [] nextObject->currentSpeech;
            }

        if( nextObject->relationName != NULL ) {
            delete [] nextObject->relationName;
            }

        if( nextObject->name != NULL ) {
            delete [] nextObject->name;
            }

        delete nextObject->futureAnimStack;
        delete nextObject->futureHeldAnimStack;
        }
    
    gameObjects.deleteAll();
    }

LivingLifePage::~LivingLifePage() {
    printf( "Total received = %d bytes (+%d in headers), "
            "total sent = %d bytes (+%d in headers)\n",
            numServerBytesRead, overheadServerBytesRead,
            numServerBytesSent, overheadServerBytesSent );
    
    mGlobalMessagesToDestroy.deallocateStringElements();

    freeLiveTriggers();

    readyPendingReceivedMessages.deallocateStringElements();

    serverFrameMessages.deallocateStringElements();
    
    if( pendingMapChunkMessage != NULL ) {
        delete [] pendingMapChunkMessage;
        pendingMapChunkMessage = NULL;
        }
    

    clearLiveObjects();

    mOldDesStrings.deallocateStringElements();
    if( mCurrentDes != NULL ) {
        delete [] mCurrentDes;
        }

    mOldLastAteStrings.deallocateStringElements();
    if( mCurrentLastAteString != NULL ) {
        delete [] mCurrentLastAteString;
        }

    mSentChatPhrases.deallocateStringElements();
    
    if( mServerSocket != -1 ) {
        closeSocket( mServerSocket );
        }
    
    mPreviousHomeDistStrings.deallocateStringElements();
    mPreviousHomeDistFades.deleteAll();

    
    delete [] mMapAnimationFrameCount;
    delete [] mMapAnimationLastFrameCount;
    delete [] mMapAnimationFrozenRotFrameCount;
    
    delete [] mMapAnimationFrozenRotFrameCountUsed;

    delete [] mMapFloorAnimationFrameCount;

    delete [] mMapCurAnimType;
    delete [] mMapLastAnimType;
    delete [] mMapLastAnimFade;
    
    delete [] mMapDropOffsets;
    delete [] mMapDropRot;
    delete [] mMapDropSounds;

    delete [] mMapMoveOffsets;
    delete [] mMapMoveSpeeds;

    delete [] mMapTileFlips;
    
    delete [] mMapContainedStacks;
    delete [] mMapSubContainedStacks;
    
    delete [] mMap;
    delete [] mMapBiomes;
    delete [] mMapFloors;

    delete [] mMapCellDrawnFlags;

    delete [] mMapPlayerPlacedFlags;

    if( nextActionMessageToSend != NULL ) {
        delete [] nextActionMessageToSend;
        nextActionMessageToSend = NULL;
        }

    if( lastMessageSentToServer != NULL ) {
        delete [] lastMessageSentToServer;
        lastMessageSentToServer = NULL;
        }
    
    if( mHungerSound != NULL ) {    
        freeSoundSprite( mHungerSound );
        }

    if( mTutorialSound != NULL ) {    
        freeSoundSprite( mTutorialSound );
        }

    if( mCurseSound != NULL ) {    
        freeSoundSprite( mCurseSound );
        }
    
    for( int i=0; i<3; i++ ) {
        freeSprite( mHungerSlipSprites[i] );
        }

    for( int i=0; i<NUM_YUM_SLIPS; i++ ) {
        freeSprite( mYumSlipSprites[i] );
        }

    freeSprite( mGuiPanelSprite );
    freeSprite( mGuiBloodSprite );

	//FOV
	freeSprite( guiPanelLeftSprite );
	freeSprite( guiPanelTileSprite );
	freeSprite( guiPanelRightSprite );
    
    freeSprite( mFloorSplitSprite );
    
    freeSprite( mCellBorderSprite );
    freeSprite( mCellFillSprite );
    
    freeSprite( mNotePaperSprite );
    freeSprite( mChalkBlotSprite );
    freeSprite( mPathMarkSprite );
    freeSprite( mHomeSlipSprite );
    
    if( teaserVideo ) {
        freeSprite( mTeaserArrowLongSprite );
        freeSprite( mTeaserArrowMedSprite );
        freeSprite( mTeaserArrowShortSprite );
        freeSprite( mTeaserArrowVeryShortSprite );
        freeSprite( mLineSegmentSprite );
        }
    
    for( int i=0; i<4; i++ ) {
        freeSprite( mGroundOverlaySprite[i] );
        }
    

    for( int i=0; i<NUM_HUNGER_BOX_SPRITES; i++ ) {
        freeSprite( mHungerBoxSprites[i] );
        freeSprite( mHungerBoxFillSprites[i] );
        
        freeSprite( mHungerBoxErasedSprites[i] );
        freeSprite( mHungerBoxFillErasedSprites[i] );
        }

    for( int i=0; i<NUM_TEMP_ARROWS; i++ ) {
        freeSprite( mTempArrowSprites[i] );
        freeSprite( mTempArrowErasedSprites[i] );
        }

    for( int i=0; i<NUM_HUNGER_DASHES; i++ ) {
        freeSprite( mHungerDashSprites[i] );
        freeSprite( mHungerDashErasedSprites[i] );
        freeSprite( mHungerBarSprites[i] );
        freeSprite( mHungerBarErasedSprites[i] );
        }
    
    for( int i=0; i<NUM_HOME_ARROWS; i++ ) {
        freeSprite( mHomeArrowSprites[i] );
        freeSprite( mHomeArrowErasedSprites[i] );
        }



    for( int i=0; i<NUM_HINT_SHEETS; i++ ) {
        freeSprite( mHintSheetSprites[i] );
        
        if( mHintMessage[i] != NULL ) {
            delete [] mHintMessage[i];
            }
        if( mCravingMessage[i] != NULL ) {
            delete [] mCravingMessage[i];
            }
        }
    
    delete [] mHintBookmarks;

    if( mHintFilterString != NULL ) {
        delete [] mHintFilterString;
        mHintFilterString = NULL;
        }

    if( mLastHintFilterString != NULL ) {
        delete [] mLastHintFilterString;
        mLastHintFilterString = NULL;
        }

    if( mPendingFilterString != NULL ) {
        delete [] mPendingFilterString;
        mPendingFilterString = NULL;
        }

    if( mDeathReason != NULL ) {
        delete [] mDeathReason;
        }

    for( int i=0; i<mGraveInfo.size(); i++ ) {
        delete [] mGraveInfo.getElement(i)->relationName;
        }
    mGraveInfo.deleteAll();

    clearOwnerInfo();

    clearLocationSpeech();

    if( photoSig != NULL ) {
        delete [] photoSig;
        photoSig = NULL;
        }
    }

void LivingLifePage::clearOwnerInfo() {
    
    for( int i=0; i<mOwnerInfo.size(); i++ ) {
        delete mOwnerInfo.getElement( i )->ownerList;
        }
    mOwnerInfo.deleteAll();
    }

char LivingLifePage::isMapBeingPulled() {
    return mapPullMode;
    }

void LivingLifePage::adjustAllFrameCounts( double inOldFrameRateFactor,
                                           double inNewFrameRateFactor ) {
    int numMapCells = mMapD * mMapD;
    
    for( int i=0; i<numMapCells; i++ ) {
        
        double timeVal = inOldFrameRateFactor * mMapAnimationFrameCount[ i ];
        mMapAnimationFrameCount[i] = lrint( timeVal / inNewFrameRateFactor );
        
        timeVal = inOldFrameRateFactor * mMapAnimationFrozenRotFrameCount[ i ];
        mMapAnimationFrozenRotFrameCount[i] = 
            lrint( timeVal / inNewFrameRateFactor );

        timeVal = inOldFrameRateFactor * mMapAnimationLastFrameCount[ i ];
        mMapAnimationLastFrameCount[i] = 
            lrint( timeVal / inNewFrameRateFactor );
        
        timeVal = 
            inOldFrameRateFactor * mMapFloorAnimationFrameCount[ i ];
        mMapFloorAnimationFrameCount[i] = 
            lrint( timeVal / inNewFrameRateFactor );
        }

    for( int i=0; i<gameObjects.size(); i++ ) {
        
        LiveObject *o = gameObjects.getElement( i );

        double timeVal = inOldFrameRateFactor * o->animationFrameCount;
        o->animationFrameCount = timeVal / inNewFrameRateFactor;
        
        timeVal = inOldFrameRateFactor * o->heldAnimationFrameCount;
        o->heldAnimationFrameCount = timeVal / inNewFrameRateFactor;
        
        timeVal = inOldFrameRateFactor * o->lastAnimationFrameCount;
        o->lastAnimationFrameCount = timeVal / inNewFrameRateFactor;

        timeVal = inOldFrameRateFactor * o->lastHeldAnimationFrameCount;
        o->lastHeldAnimationFrameCount = timeVal / inNewFrameRateFactor;

        timeVal = inOldFrameRateFactor * o->frozenRotFrameCount;
        o->frozenRotFrameCount = timeVal / inNewFrameRateFactor;

        timeVal = inOldFrameRateFactor * o->heldFrozenRotFrameCount;
        o->heldFrozenRotFrameCount = timeVal / inNewFrameRateFactor;
        }
    }

LiveObject *LivingLifePage::getOurLiveObject() {
    
    return getLiveObject( ourID );
    }

LiveObject *LivingLifePage::getLiveObject( int inID ) {
    
    LiveObject *obj = NULL;

    for( int i=0; i<gameObjects.size(); i++ ) {
        
        LiveObject *o = gameObjects.getElement( i );
        
        if( o->id == inID ) {
            obj = o;
            break;
            }
        }
    return obj;
    }

bool LivingLifePage::tileBlocked( int x, int y ) {
	int oid = getObjId( x, y );
	int oid_right = getObjId( x + 1, y );
	int oid_left = getObjId( x - 1, y );
	ObjectRecord *o = NULL;
	ObjectRecord *o_right = NULL;
	ObjectRecord *o_left = NULL;
	if( oid > 0 ) o = getObject( oid );
	if( oid_right > 0 ) o_right = getObject( oid_right );
	if( oid_left > 0 ) o_left = getObject( oid_left );
	bool blocked_center = true;
	bool blocked_from_right = true;
	bool blocked_from_left = true;
	if( oid == 0 || ( o != NULL && !o->blocksWalking ) ) blocked_center = false;
	if( oid_right == 0 || ( o_right != NULL && o_right->leftBlockingRadius == 0 ) ) blocked_from_right = false;
	if( oid_left == 0 || ( o_left != NULL && o_left->rightBlockingRadius == 0 ) ) blocked_from_left = false;
	if( blocked_center || blocked_from_right || blocked_from_left ) return true;
	return false;
	}

void LivingLifePage::drunkWalk( GridPos *path, int pathLen, bool actionMove ) {
	
	if( path == NULL ) return;	
	if( pathLen >= 3 ) {
		bool changeThis = true;
		
		for( int i = 1; i < pathLen - 1; i++ ) {
			if( changeThis ) {
				int xDis = path[ i + 1 ].x - path[ i - 1 ].x;
				int yDis = path[ i + 1 ].y - path[ i - 1 ].y;
				
				if( abs(xDis) + abs(yDis) < 4 ) {
					int newXDis = 2;
					int newYDis = 2;
					int signX = 0;
					int signY = 0;
					if( xDis != 0 ) signX = xDis / abs(xDis);
					if( yDis != 0 ) signY = yDis / abs(yDis);
				
					if( abs(xDis) == 0 ) newXDis = randSource.getRandomBoundedInt( -1, 1 );
					if( abs(xDis) == 1 ) newXDis = signX * randSource.getRandomBoundedInt( 0, 1 );
					if( abs(yDis) == 0 ) newYDis = randSource.getRandomBoundedInt( -1, 1 );
					if( abs(yDis) == 1 ) newYDis = signY * randSource.getRandomBoundedInt( 0, 1 );
					
					int newX = path[ i ].x;
					int newY = path[ i ].y;
					if( newXDis != 2 ) newX = path[ i - 1 ].x + newXDis;
					if( newYDis != 2 ) newY = path[ i - 1 ].y + newYDis;
					
					if( !tileBlocked( newX, newY ) ) path[ i ] = { newX, newY };
					}
				}
			changeThis = !changeThis;
			}
		}
	else if( pathLen == 2 && !actionMove ) {
		
		int xDis = path[ 1 ].x - path[ 0 ].x;
		int yDis = path[ 1 ].y - path[ 0 ].y;
		int newXDis = 2;
		int newYDis = 2;
		
		if( abs(xDis) == 0 ) newXDis = randSource.getRandomBoundedInt( -1, 1 );
		if( abs(yDis) == 0 ) newYDis = randSource.getRandomBoundedInt( -1, 1 );
		
		int newX = path[ 1 ].x;
		int newY = path[ 1 ].y;
		if( newXDis != 2 ) newX = path[ 0 ].x + newXDis;
		if( newYDis != 2 ) newY = path[ 0 ].y + newYDis;
		
		if( !tileBlocked( newX, newY ) ) path[ 1 ] = { newX, newY };
		
		}
	
	}

bool LivingLifePage::isTripping() {
	LiveObject *ourLiveObject = getOurLiveObject();
	if( ourLiveObject == NULL ) return false;
	return 
		trippingEmotionIndex != -1 &&
		ourLiveObject->currentEmot != NULL &&
		strcmp( ourLiveObject->currentEmot->triggerWord, 
		getEmotion( trippingEmotionIndex )->triggerWord ) == 0;
	}

SimpleVector<char*> *splitLines( const char *inString,
                                 double inMaxWidth ) {
    
    // break into lines
    SimpleVector<char *> *tokens = 
        tokenizeString( inString );
    
    
    // collect all lines before drawing them
    SimpleVector<char *> *lines = new SimpleVector<char*>();
    
    
    if( tokens->size() > 0 ) {
        // start with firt token
        char *firstToken = tokens->getElementDirect( 0 );
        
        lines->push_back( firstToken );
        
        tokens->deleteElement( 0 );
        }
    
    
    while( tokens->size() > 0 ) {
        char *nextToken = tokens->getElementDirect( 0 );
        
        char *currentLine = lines->getElementDirect( lines->size() - 1 );
         
        char *expandedLine = autoSprintf( "%s %s", currentLine, nextToken );
         
        if( handwritingFont->measureString( expandedLine ) <= inMaxWidth ) {
            // replace current line
            delete [] currentLine;
            lines->deleteElement(  lines->size() - 1 );
             
            lines->push_back( expandedLine );
            }
        else {
            // expanded is too long
            // put token at start of next line
            delete [] expandedLine;
             
            lines->push_back( stringDuplicate( nextToken ) );
            }
         

        delete [] nextToken;
        tokens->deleteElement( 0 );
        }
    
    delete tokens;
    
    return lines;
    }


static Image *speechColorImage = NULL;
static Image *speechMaskImage = NULL;



// forces uppercase
void LivingLifePage::drawChalkBackgroundString( doublePair inPos, 
                                                const char *inString,
                                                double inFade,
                                                double inMaxWidth,
                                                LiveObject *inSpeaker,
                                                int inForceMinChalkBlots,
                                                FloatColor *inForceBlotColor,
                                                FloatColor *inForceTextColor ) {
    
    char *stringUpper = stringToUpperCase( inString );

    SimpleVector<char*> *lines = splitLines( inString, inMaxWidth );
    
    delete [] stringUpper;

    
    if( lines->size() == 0 ) {
        delete lines;
        return;
        }

    double lineSpacing = handwritingFont->getFontHeight() / 2 + ( 5 * gui_fov_scale_hud );
    
    double firstLineY =  inPos.y + ( lines->size() - 1 ) * lineSpacing;
    
    if( firstLineY > lastScreenViewCenter.y + recalcOffsetY( 330 ) * gui_fov_scale ) {
        firstLineY = lastScreenViewCenter.y + recalcOffsetY( 330 ) * gui_fov_scale;
        }

    
    if( inForceBlotColor != NULL ) {
        setDrawColor( *inForceBlotColor );
        setDrawFade( inFade );
        }
    else if( inSpeaker != NULL && inSpeaker->dying ) {
        if( inSpeaker->sick ) {
            // sick-ish yellow
            setDrawColor( 0.874510, 0.658824, 0.168627, inFade );
            }
        else {
            // wounded, blood red
            setDrawColor( .65, 0, 0, inFade );
            }
        }
    else if( inSpeaker != NULL && inSpeaker->curseLevel > 0 ) {
        setDrawColor( 0, 0, 0, inFade );
        }
    else {
        setDrawColor( 1, 1, 1, inFade );
        }



    char maskOnly = false;
    char colorOnly = false;
    
    if( savingSpeech && savingSpeechColor && inFade == 1.0 ) {
        drawSquare( inPos, 1024 * gui_fov_scale );
        colorOnly = true;
        }
    else if( savingSpeech && savingSpeechMask && inFade == 1.0 ) {
        setDrawColor( 0, 0, 0, 1.0 );
        drawSquare( inPos, 1024 * gui_fov_scale );
        setDrawColor( 1, 1, 1, 1 );
        maskOnly = true;
        }



    // with a fixed seed
    JenkinsRandomSource blotRandSource( 0 );
        
    for( int i=0; i<lines->size(); i++ ) {
        char *line = lines->getElementDirect( i );
        

        double length = handwritingFont->measureString( line );

        //FOV
        /*int numBlots = lrint( 0.25 + length / 20 ) + 1;
        
        if( inForceMinChalkBlots != -1 && numBlots < inForceMinChalkBlots ) {
            numBlots = inForceMinChalkBlots;
            }
    
        doublePair blotSpacing = { 20, 0 };*/
    
        doublePair firstBlot = 
            { inPos.x, firstLineY - i * lineSpacing};

        
        for( doublePair blotPos = firstBlot; blotPos.x < inPos.x + ( length + 20 * gui_fov_scale_hud ); blotPos.x += 20 * gui_fov_scale_hud ) {
            //doublePair blotPos = add( firstBlot, mult( blotSpacing, b ) );
			blotPos.y = firstBlot.y;
        
            double rot = blotRandSource.getRandomDouble();
            drawSprite( mChalkBlotSprite, blotPos, gui_fov_scale_hud, rot );
            drawSprite( mChalkBlotSprite, blotPos, gui_fov_scale_hud, rot );
            
            // double hit vertically
            blotPos.y += ( 5 * gui_fov_scale_hud );
            rot = blotRandSource.getRandomDouble();
            drawSprite( mChalkBlotSprite, blotPos, gui_fov_scale_hud, rot );
            
            blotPos.y -= ( 10 * gui_fov_scale_hud );
            rot = blotRandSource.getRandomDouble();
            drawSprite( mChalkBlotSprite, blotPos, gui_fov_scale_hud, rot );
            }
        }
    
    
    if( inForceTextColor != NULL ) {
        setDrawColor( *inForceTextColor );
        setDrawFade( inFade );
        }
    else if( inSpeaker != NULL && inSpeaker->dying && ! inSpeaker->sick ) {
        setDrawColor( 1, 1, 1, inFade );
        }
    else if( inSpeaker != NULL && inSpeaker->curseLevel > 0 ) {
        setDrawColor( 1, 1, 1, inFade );
        if( inSpeaker->speechIsSuccessfulCurse ) {
            setDrawColor( 0.875, 0, 0.875, inFade );
            }
        }
    else if( inSpeaker != NULL && inSpeaker->speechIsSuccessfulCurse ) {
        setDrawColor( 0.5, 0, 0.5, inFade );
        }
    else {
        setDrawColor( 0, 0, 0, inFade );
        }
    

    if( maskOnly ) {
        // font should add to opacity of mask too
        setDrawColor( 1, 1, 1, 1 );
        }

    
    for( int i=0; i<lines->size(); i++ ) {
        char *line = lines->getElementDirect( i );
        
        doublePair lineStart = 
            { inPos.x, firstLineY - i * lineSpacing};
        
        handwritingFont->drawString( line, lineStart, alignLeft );
        delete [] line;
        }

    delete lines;


    if( colorOnly ) {
        saveScreenShot( "speechColor", &speechColorImage );
        savingSpeechColor = false;
        savingSpeechMask = true;
        }
    else if( maskOnly ) {
        saveScreenShot( "speechMask", &speechMaskImage );
        savingSpeechMask = false;
        savingSpeech = false;
        }
    
    if( speechColorImage != NULL && speechMaskImage != NULL ) {
        // both screen shot requests are done

        Image *subColor = speechColorImage->getSubImage( 0, 0, 1280, 500 );
        Image *subMask = speechMaskImage->getSubImage( 0, 0, 1280, 500 );
        
        int w = subColor->getWidth();
        int h = subColor->getHeight();
        
        Image blend( w, h, 4, true );
        blend.paste( subColor );
        double *alpha = blend.getChannel( 3 );
        
        memcpy( alpha, subMask->getChannel( 0 ),
                w * h * sizeof( double ) );
        
        int minX = w -1;
        int maxX = 0;
        int minY = h -1;
        int maxY = 0;
        
        for( int y=0; y<h; y++ ) {
            for( int x=0; x<w; x++ ) {
                if( alpha[ y * w + x ] > 0 ) {
                    
                    if( x < minX ) {
                        minX = x;
                        }
                    if( x > maxX ) {
                        maxX = x;
                        }

                    if( y < minY ) {
                        minY = y;
                        }
                    if( y > maxY ) {
                        maxY = y;
                        }
                    }
                }
            }
        
        // expand 1 pixel to be safe
        if( minX > 0 ) {
            minX --;
            }
        if( minY > 0 ) {
            minY --;
            }
        if( maxX < w - 1 ) {
            maxX ++;
            }
        if( maxY < h - 1 ) {
            maxY ++;
            }
        

        Image *trimmed = blend.getSubImage( minX, minY,
                                            maxX - minX,
                                            maxY - minY );
                
        File screenShots( NULL, "screenShots" );
        
        char *fileName = autoSprintf( "speechBlend%04d.tga", 
                                      savingSpeechNumber );
        savingSpeechNumber++;
        
        File *tgaFile = screenShots.getChildFile( fileName );
        
        delete [] fileName;

        char *tgaPath = tgaFile->getFullFileName();

        delete tgaFile;

        writeTGAFile( tgaPath, trimmed );
        
        delete [] tgaPath;
        
        delete trimmed;
        

        delete subColor;
        delete subMask;


        delete speechColorImage;
        speechColorImage = NULL;
        delete speechMaskImage;
        speechMaskImage = NULL;
        }
    }




typedef struct OffScreenSound {
        doublePair pos;

        double fade;
        // wall clock time when should start fading
        double fadeETATime;

        char red;
    } OffScreenSound;

SimpleVector<OffScreenSound> offScreenSounds;
    



static void addOffScreenSound( double inPosX, double inPosY,
                               char *inDescription ) {

    char red = false;
    
    char *stringPos = strstr( inDescription, "offScreenSound" );
    
    if( stringPos != NULL ) {
        stringPos = &( stringPos[ strlen( "offScreenSound" ) ] );
        
        if( strstr( stringPos, "_red" ) == stringPos ) {
            // _red flag next
            red = true;
            }
        }
    
    double fadeETATime = game_getCurrentTime() + 4;
    
    doublePair pos = { inPosX, inPosY };
    
    OffScreenSound s = { pos, 1.0, fadeETATime, red };
    
    offScreenSounds.push_back( s );
    }



void LivingLifePage::drawOffScreenSounds() {
    
    if( offScreenSounds.size() == 0 ) {
        return;
        }
    
    double xRadius = viewWidth / 2 - 32;
    double yRadius = viewHeight / 2 - 32;
    
    FloatColor red = { 0.65, 0, 0, 1 };
    FloatColor white = { 1, 1, 1, 1 };
    FloatColor black = { 0, 0, 0, 1 };
    

    double curTime = game_getCurrentTime();
    
    for( int i=0; i<offScreenSounds.size(); i++ ) {
        OffScreenSound *s = offScreenSounds.getElement( i );
        
        if( s->fadeETATime <= curTime ) {
            s->fade -= 0.05 * frameRateFactor;

            if( s->fade <= 0 ) {
                offScreenSounds.deleteElement( i );
                i--;
                continue;
                }
            }

        
        if( fabs( s->pos.x - lastScreenViewCenter.x ) > xRadius
            ||
            fabs( s->pos.y - lastScreenViewCenter.y ) > yRadius ) {
            
            // off screen
            
            // relative vector
            doublePair v = sub( s->pos, lastScreenViewCenter );
            
            doublePair normV = normalize( v );
            
            // first extend in x dir to edge
            double xScale = fabs( xRadius / normV.x );
            
            doublePair edgeV = mult( normV, xScale );
            

            if( fabs( edgeV.y ) > yRadius ) {
                // off top/bottom
                
                // extend in y dir instead
                double yScale = fabs( yRadius / normV.y );
            
                edgeV = mult( normV, yScale );
                }
            
            if( edgeV.y < -270 ) {
                edgeV.y = -270;
                }
            

            doublePair drawPos = add( edgeV, lastScreenViewCenter );
            
            FloatColor *textColor = &black;
            FloatColor *bgColor = &white;
            
            if( s->red ) {
                textColor = &white;
                bgColor = &red;
                }

            drawChalkBackgroundString( drawPos,
                                       "!",
                                       s->fade,
                                       100,
                                       NULL,
                                       -1,
                                       bgColor, textColor );
            }    
        }
    }






void LivingLifePage::handleAnimSound( int inObjectID, double inAge, 
                                      AnimType inType,
                                      int inOldFrameCount, int inNewFrameCount,
                                      double inPosX, double inPosY ) {    
    
            
    double oldTimeVal = frameRateFactor * inOldFrameCount / 60.0;
            
    double newTimeVal = frameRateFactor * inNewFrameCount / 60.0;
                
    if( inType == ground2 ) {
        inType = ground;
        }


    AnimationRecord *anim = getAnimation( inObjectID, inType );
    if( anim != NULL ) {
                    
        for( int s=0; s<anim->numSounds; s++ ) {
            
            if( anim->soundAnim[s].sound.numSubSounds == 0 ) {
                continue;
                }
            
            
            if( ( anim->soundAnim[s].ageStart != -1 &&
                  inAge < anim->soundAnim[s].ageStart )
                ||
                ( anim->soundAnim[s].ageEnd != -1 &&
                  inAge >= anim->soundAnim[s].ageEnd ) ) {
                
                continue;
                }
            
            
            double hz = anim->soundAnim[s].repeatPerSec;
                        
            double phase = anim->soundAnim[s].repeatPhase;
            
            if( hz != 0 ) {
                double period = 1 / hz;
                
                double startOffsetSec = phase * period;
                
                int oldPeriods = 
                    lrint( 
                        floor( ( oldTimeVal - startOffsetSec ) / 
                               period ) );
                
                int newPeriods = 
                    lrint( 
                        floor( ( newTimeVal - startOffsetSec ) / 
                               period ) );
                
                if( newPeriods > oldPeriods ) {
                    SoundUsage u = anim->soundAnim[s].sound;
                    
                    
                    if( anim->soundAnim[s].footstep ) {
                        
                        // check if we're on a floor

                        int x = lrint( inPosX );
                        int y = lrint( inPosY );

                        int i = getMapIndex( x, y );
                        
                        if( i != -1 && mMapFloors[i] > 0 ) {
                            
                            ObjectRecord *f = getObject( mMapFloors[i] );
                            
                            if( f->usingSound.numSubSounds > 0 ) {
                                u = f->usingSound;
                                }
                            }
                        }
                    
                    
                    
                    playSound( u,
                               getVectorFromCamera( inPosX, inPosY ) );
                    
                    }
                }
            }
        }
    }




void LivingLifePage::drawMapCell( int inMapI, 
                                  int inScreenX, int inScreenY,
                                  char inHighlightOnly,
                                  char inNoTimeEffects ) {
            
    int oID = mMap[ inMapI ];

    int objectHeight = 0;
    
    if( oID > 0 ) {
        
        objectHeight = getObjectHeight( oID );
        
        double oldFrameCount = mMapAnimationFrameCount[ inMapI ];

        if( !mapPullMode && !inHighlightOnly && !inNoTimeEffects ) {
            
            if( mMapCurAnimType[ inMapI ] == moving ) {
                double animSpeed = 1.0;
                ObjectRecord *movingObj = getObject( oID );
            
                if( movingObj->speedMult < 1.0 ) {
                    // only slow anims down don't speed them up
                    animSpeed *= movingObj->speedMult;
                    }

                mMapAnimationFrameCount[ inMapI ] += animSpeed;
                mMapAnimationLastFrameCount[ inMapI ] += animSpeed;
                mMapAnimationFrozenRotFrameCount[ inMapI ] += animSpeed;
                mMapAnimationFrozenRotFrameCountUsed[ inMapI ] = false;
                }
            else {
                mMapAnimationFrameCount[ inMapI ] ++;
                mMapAnimationLastFrameCount[ inMapI ] ++;
                }

            
            if( mMapLastAnimFade[ inMapI ] > 0 ) {
                mMapLastAnimFade[ inMapI ] -= 0.05 * frameRateFactor;
                if( mMapLastAnimFade[ inMapI ] < 0 ) {
                    mMapLastAnimFade[ inMapI ] = 0;
                    
                    AnimType newType;

                    if( mMapMoveSpeeds[ inMapI ] == 0 ) {
                        newType = ground;
                        }
                    else if( mMapMoveSpeeds[ inMapI ] > 0 ) {
                        // transition to moving now
                        newType = moving;
                        }

                    if( mMapCurAnimType[ inMapI ] != newType ) {
                        
                        mMapLastAnimType[ inMapI ] = mMapCurAnimType[ inMapI ];
                        mMapCurAnimType[ inMapI ] = newType;
                        mMapLastAnimFade[ inMapI ] = 1;
                        
                        mMapAnimationLastFrameCount[ inMapI ] =
                            mMapAnimationFrameCount[ inMapI ];
                        
                        
                        if( newType == moving &&
                            mMapAnimationFrozenRotFrameCountUsed[ inMapI ] ) {
                            mMapAnimationFrameCount[ inMapI ] = 
                                mMapAnimationFrozenRotFrameCount[ inMapI ];
                            }
                        }
                    
                    }
                }
            }

        doublePair pos = { (double)inScreenX, (double)inScreenY };
        double rot = 0;
        
        if( mMapDropOffsets[ inMapI ].x != 0 ||
            mMapDropOffsets[ inMapI ].y != 0 ) {
                    
            doublePair nullOffset = { 0, 0 };
                    

            doublePair delta = sub( nullOffset, 
                                    mMapDropOffsets[ inMapI ] );
                    
            double step = frameRateFactor * 0.0625;
            double rotStep = frameRateFactor * 0.03125;
                    
            if( length( delta ) < step ) {
                        
                mMapDropOffsets[ inMapI ].x = 0;
                mMapDropOffsets[ inMapI ].y = 0;
                }
            else {
                mMapDropOffsets[ inMapI ] =
                    add( mMapDropOffsets[ inMapI ],
                         mult( normalize( delta ), step ) );
                }
            
            if( mMapDropOffsets[ inMapI ].x == 0 &&
                mMapDropOffsets[ inMapI ].y == 0 ) {
                // done dropping into place
                if( mMapDropSounds[ inMapI ].numSubSounds > 0 ) {
                    
                    playSound( mMapDropSounds[ inMapI ],
                               getVectorFromCamera( 
                                   (double)inScreenX / CELL_D,
                                   (double)inScreenY / CELL_D ) );
                    mMapDropSounds[ inMapI ] = blankSoundUsage;
                    }
                
                }
            
                
            
            double rotDelta = 0 - mMapDropRot[ inMapI ];
            
            if( rotDelta > 0.5 ) {
                rotDelta = rotDelta - 1;
                }
            else if( rotDelta < -0.5 ) {
                rotDelta = 1 + rotDelta;
                }

            if( fabs( rotDelta ) < rotStep ) {
                mMapDropRot[ inMapI ] = 0;
                }
            else {
                double rotSign = 1;
                if( rotDelta < 0 ) {
                    rotSign = -1;
                    }
                
                mMapDropRot[ inMapI ] = 
                    mMapDropRot[ inMapI ] + rotSign * rotStep;
                }
            

                                        
            // step offset BEFORE applying it
            // (so we don't repeat starting position)
            pos = add( pos, mult( mMapDropOffsets[ inMapI ], CELL_D ) );
            
            rot = mMapDropRot[ inMapI ];
            }
        
        if( mMapMoveSpeeds[inMapI] > 0 &&
            ( mMapMoveOffsets[ inMapI ].x != 0 ||
              mMapMoveOffsets[ inMapI ].y != 0  ) ) {

            pos = add( pos, mult( mMapMoveOffsets[ inMapI ], CELL_D ) );
            }
        


        setDrawColor( 1, 1, 1, 1 );
                
        AnimType curType = ground;
        AnimType fadeTargetType = ground;
        double animFade = 1;

        if( mMapMoveSpeeds[ inMapI ] > 0 ) {
            curType = moving;
            fadeTargetType = moving;
            animFade = 1;
            }
        


        double timeVal = frameRateFactor * 
            mMapAnimationFrameCount[ inMapI ] / 60.0;
                
        double frozenRotTimeVal = frameRateFactor *
            mMapAnimationFrozenRotFrameCount[ inMapI ] / 60.0;

        double targetTimeVal = timeVal;

        if( mMapLastAnimFade[ inMapI ] != 0 ) {
            animFade = mMapLastAnimFade[ inMapI ];
            curType = mMapLastAnimType[ inMapI ];
            fadeTargetType = mMapCurAnimType[ inMapI ];
            
            timeVal = frameRateFactor * 
                mMapAnimationLastFrameCount[ inMapI ] / 60.0;
            }
                
                
            
        char flip = mMapTileFlips[ inMapI ];
        
        ObjectRecord *obj = getObject( oID );
        if( obj->noFlip ||
            ( obj->permanent && 
              ( obj->blocksWalking || obj->drawBehindPlayer || 
                obj->anySpritesBehindPlayer) ) ) {
            // permanent, blocking objects (e.g., walls) 
            // or permanent behind-player objects (e.g., roads) 
            // are never drawn flipped
            flip = false;
            // remember that this tile is NOT flipped, so that it
            // won't flip back strangely if it changes to something
            // that doesn't have a noFlip status
            mMapTileFlips[ inMapI ] = false;
            }
        
        char highlight = false;
        float highlightFade = 1.0f;
        
        if( mCurMouseOverID > 0 &&
            ! mCurMouseOverSelf &&
            mCurMouseOverSpot.y * mMapD + mCurMouseOverSpot.x == inMapI ) {
            
            if( mCurMouseOverBehind ) {
                highlight = inHighlightOnly;
                }
            else {
                highlight = true;
                }
        
            highlightFade = mCurMouseOverFade;
            }
        else {
            for( int i=0; i<mPrevMouseOverSpots.size(); i++ ) {
                GridPos prev = mPrevMouseOverSpots.getElementDirect( i );
                
                if( prev.y * mMapD + prev.x == inMapI ) {
                    if( mPrevMouseOverSpotsBehind.getElementDirect( i ) ) {
                        highlight = inHighlightOnly;
                        }
                    else {
                        highlight = true;
                        }
                    highlightFade = 
                        mPrevMouseOverSpotFades.getElementDirect(i);
                    }    
                }
            }
        
        if( ! mShowHighlights ) {
            if( inHighlightOnly ) {
                return;
                }
            highlight = false;
            }
        

        if( !mapPullMode && !inHighlightOnly && !inNoTimeEffects ) {
            handleAnimSound( oID, 0, mMapCurAnimType[ inMapI ], oldFrameCount, 
                             mMapAnimationFrameCount[ inMapI ],
                             pos.x / CELL_D,
                             pos.y / CELL_D );
            }
        
        
        if( highlight && obj->noHighlight ) {
            if( inHighlightOnly ) {
                return;
                }
            highlight = false;
            }

        
        int numPasses = 1;
        int startPass = 0;
        
        if( highlight ) {
            
            // first pass normal draw
            // then three stencil passes (second and third with a subtraction)
            numPasses = 6;
            
            if( highlightFade != 1.0f ) {
                //fadeHandle = addGlobalFade( highlightFade );
                }

            if( inHighlightOnly ) {
                startPass = 1;
                }
            }
        
        for( int i=startPass; i<numPasses; i++ ) {
            
            doublePair passPos = pos;
            
            if( highlight ) {
                
                switch( i ) {
                    case 0:
                        // normal color draw
                        break;
                    case 1:
                        // opaque portion
                        startAddingToStencil( false, true, .99f );
                        break;
                    case 2:
                        // first fringe
                        startAddingToStencil( false, true, .07f );
                        break;
                    case 3:
                        // subtract opaque from fringe to get just first fringe
                        startAddingToStencil( false, false, .99f );
                        break;
                    case 4:
                        // second fringe
                        // ignore totally transparent stuff
                        // like invisible animation layers
                        startAddingToStencil( false, true, 0.01f );
                        break;
                    case 5:
                        // subtract first fringe from fringe to get 
                        // just secon fringe
                        startAddingToStencil( false, false, .07f );
                        break;
                    default:
                        break;
                    }

                }

        if( mMapContainedStacks[ inMapI ].size() > 0 ) {
            int *stackArray = 
                mMapContainedStacks[ inMapI ].getElementArray();
            SimpleVector<int> *subStackArray =
                mMapSubContainedStacks[ inMapI ].getElementArray();
            
            drawObjectAnim( oID, 
                            curType, timeVal,
                            animFade,
                            fadeTargetType,
                            targetTimeVal,
                            frozenRotTimeVal,
                            &( mMapAnimationFrozenRotFrameCountUsed[ inMapI ] ),
                            endAnimType,
                            endAnimType,
                            passPos, rot, false, flip,
                            -1,
                            false, false, false,
                            getEmptyClothingSet(),
                            NULL,
                            mMapContainedStacks[ inMapI ].size(),
                            stackArray, subStackArray );
            delete [] stackArray;
            delete [] subStackArray;
            }
        else {
            drawObjectAnim( oID, 2, 
                            curType, timeVal,
                            animFade,
                            fadeTargetType, 
                            targetTimeVal,
                            frozenRotTimeVal,
                            &( mMapAnimationFrozenRotFrameCountUsed[ inMapI ] ),
                            endAnimType,
                            endAnimType,
                            passPos, rot,
                            false,
                            flip, -1,
                            false, false, false,
                            getEmptyClothingSet(), NULL );
            }
        

        if( highlight ) {
            
            
            float mainFade = .35f;
        
            toggleAdditiveBlend( true );
            
            doublePair squarePos = passPos;
            
            if( objectHeight > 1.5 * CELL_D ) {
                squarePos.y += 192;
                }
            
            int squareRad = 306;
            
            switch( i ) {
                case 0:
                    // normal color draw
                    break;
                case 1:
                    // opaque portion
                    startDrawingThroughStencil( false );

                    setDrawColor( 1, 1, 1, highlightFade * mainFade );
                    
                    drawSquare( squarePos, squareRad );
                    
                    stopStencil();
                    break;
                case 2:
                    // first fringe
                    // wait until next pass to isolate fringe
                    break;
                case 3:
                    // now first fringe is isolated in stencil
                    startDrawingThroughStencil( false );

                    setDrawColor( 1, 1, 1, highlightFade * mainFade * .5 );

                    drawSquare( squarePos, squareRad );

                    stopStencil();                    
                    break;
                case 4:
                    // second fringe
                    // wait until next pass to isolate fringe
                    break;
                case 5:
                    // now second fringe is isolated in stencil
                    startDrawingThroughStencil( false );
                    
                    setDrawColor( 1, 1, 1, highlightFade * mainFade *.25 );
                    
                    drawSquare( squarePos, squareRad );
                    
                    stopStencil();
                    break;
                default:
                    break;
                }
            toggleAdditiveBlend( false );
            }

        
            }
        
        
        
        }
    else if( oID == -1 ) {
        // unknown
        doublePair pos = { (double)inScreenX, (double)inScreenY };
                
        setDrawColor( 0, 0, 0, 0.5 );
        drawSquare( pos, 14 );
        }

    }


SimpleVector<doublePair> trail;
SimpleVector<FloatColor> trailColors;
double pathStepDistFactor = 0.2;

FloatColor trailColor = { 0, 0.5, 0, 0.25 };



GridPos LivingLifePage::getMapPos( int inWorldX, int inWorldY ) {
    GridPos p =
        { inWorldX - mMapOffsetX + mMapD / 2,
          inWorldY - mMapOffsetY + mMapD / 2 };
    
    return p;
    }



int LivingLifePage::getMapIndex( int inWorldX, int inWorldY ) {
    GridPos mapTarget = getMapPos( inWorldX, inWorldY );
    
    if( mapTarget.y >= 0 && mapTarget.y < mMapD &&
        mapTarget.x >= 0 && mapTarget.x < mMapD ) {
                    
        return mapTarget.y * mMapD + mapTarget.x;
        }
    return -1;
    }




ObjectAnimPack LivingLifePage::drawLiveObject( 
    LiveObject *inObj,
    SimpleVector<LiveObject *> *inSpeakers,
    SimpleVector<doublePair> *inSpeakersPos ) {    


    ObjectAnimPack returnPack;
    returnPack.inObjectID = -1;


    if( inObj->hide || inObj->outOfRange ) {
        return returnPack;
        }


    inObj->onScreen = true;



    if( ! inObj->allSpritesLoaded ) {
        // skip drawing until fully loaded
        return returnPack;
        }

    // current pos
                    
    doublePair pos = mult( inObj->currentPos, CELL_D );
    

    if( inObj->heldByDropOffset.x != 0 ||
        inObj->heldByDropOffset.y != 0 ) {
                    
        doublePair nullOffset = { 0, 0 };
                    
        
        doublePair delta = sub( nullOffset, 
                                inObj->heldByDropOffset );
                    
        double step = frameRateFactor * 0.0625;

        if( length( delta ) < step ) {
            
            inObj->heldByDropOffset.x = 0;
            inObj->heldByDropOffset.y = 0;
            
            ObjectRecord *displayObj = 
                getObject( inObj->displayID );
            
            if( displayObj->usingSound.numSubSounds > 0 ) {
                // play baby's using sound as they are put down
                // we no longer have access to parent's using sound
                playSound( displayObj->usingSound,
                           getVectorFromCamera(
                               inObj->currentPos.x, inObj->currentPos.y ) );
                }
            }
        else {
            inObj->heldByDropOffset =
                add( inObj->heldByDropOffset,
                     mult( normalize( delta ), step ) );
            }
                            
        // step offset BEFORE applying it
        // (so we don't repeat starting position)
        pos = add( pos, mult( inObj->heldByDropOffset, CELL_D ) );
        }
    

    doublePair actionOffset = { 0, 0 };
    
    if( false )
    if( inObj->curAnim == moving ) {
        trail.push_back( pos );
        trailColors.push_back( trailColor );

        while( trail.size() > 1000 ) {
            trail.deleteElement( 0 );
            trailColors.deleteElement( 0 );
            }
        }
    

    int targetX = playerActionTargetX;
    int targetY = playerActionTargetY;
    
    if( inObj->id != ourID ) {
        targetX = inObj->actionTargetX;
        targetY = inObj->actionTargetY;
        }
    else {
        setClothingHighlightFades( inObj->clothingHighlightFades );
        }
    
                
    if( inObj->curAnim != eating &&
        inObj->lastAnim != eating &&
        inObj->pendingActionAnimationProgress != 0 ) {
                    
        // wiggle toward target

        
        int trueTargetX = targetX + inObj->actionTargetTweakX;
        int trueTargetY = targetY + inObj->actionTargetTweakY;
        
                    
        float xDir = 0;
        float yDir = 0;
                
        doublePair dir = { trueTargetX - inObj->currentPos.x,
                           trueTargetY - inObj->currentPos.y };
        
        if( dir.x != 0 || dir.y != 0 ) {    
            dir = normalize( dir );
            }
        
        xDir = dir.x;
        yDir = dir.y;
    
        if( inObj->currentPos.x < trueTargetX ) {
            if( inObj->currentSpeed == 0 ) {
                inObj->holdingFlip = false;
                }
            }
        if( inObj->currentPos.x > trueTargetX ) {
            if( inObj->currentSpeed == 0 ) {
                inObj->holdingFlip = true;
                }
            }

        double wiggleMax = CELL_D *.5 *.90;
        
        double halfWiggleMax = wiggleMax * 0.5;

        if( xDir == 0 && yDir == 0 ) {
            // target where we're standing
            // wiggle tiny bit down
            yDir = -1;
            
            halfWiggleMax *= 0.25;
            }
        else if( xDir == 0 && yDir == -1 ) {
            // moving down causes feet to cross object in our same tile
            // move less
            halfWiggleMax *= 0.5;
            }
        

        
        double offset =
            halfWiggleMax - 
            halfWiggleMax * 
            cos( 2 * M_PI * inObj->pendingActionAnimationProgress );
                    
                    
        actionOffset.x += xDir * offset;
        actionOffset.y += yDir * offset;
        }
                
                
    // bare hands action OR holding something
    // character wiggle
    if( inObj->pendingActionAnimationProgress != 0 ) {
                    
        pos = add( pos, actionOffset );
        }                

                
    AnimType curType = inObj->curAnim;
    AnimType fadeTargetType = inObj->curAnim;
                
    double animFade = 1.0;
                
    double timeVal = frameRateFactor * 
        inObj->animationFrameCount / 60.0;
    
    double targetTimeVal = timeVal;

    double frozenRotTimeVal = frameRateFactor * 
        inObj->frozenRotFrameCount / 60.0;

    if( inObj->lastAnimFade > 0 ) {
        curType = inObj->lastAnim;
        fadeTargetType = inObj->curAnim;
        animFade = inObj->lastAnimFade;
        
        timeVal = frameRateFactor * 
            inObj->lastAnimationFrameCount / 60.0;
        }
                

    setDrawColor( 1, 1, 1, 1 );
    //setDrawColor( red, 0, blue, 1 );
    //mainFont->drawString( string, 
    //                      pos, alignCenter );
                
    double age = computeCurrentAge( inObj );


    ObjectRecord *heldObject = NULL;

    int hideClosestArm = 0;
    char hideAllLimbs = false;


    if( inObj->holdingID != 0 ) { 
        if( inObj->holdingID > 0 ) {
            heldObject = getObject( inObj->holdingID );
            }
        else if( inObj->holdingID < 0 ) {
            // held baby
            LiveObject *babyO = getGameObject( - inObj->holdingID );
            
            if( babyO != NULL ) {    
                heldObject = getObject( babyO->displayID );
                }
            }
        }
    
    
    getArmHoldingParameters( heldObject, &hideClosestArm, &hideAllLimbs );
    

    // override animation types for people who are riding in something
    // animationBank will freeze the time on their arms whenever they are
    // in a moving animation.
    AnimType frozenArmType = endAnimType;
    AnimType frozenArmFadeTargetType = endAnimType;
    
    if( hideClosestArm == -2 
        ||
        ( inObj->holdingID > 0 && getObject( inObj->holdingID )->rideable )
        ||
        ( inObj->lastHoldingID > 0 && 
          getObject( inObj->lastHoldingID )->rideable ) ) {
    
        if( curType == ground2 || curType == moving ) {
            frozenArmType = moving;
            }
        if( fadeTargetType == ground2 || fadeTargetType == moving ) {
            frozenArmFadeTargetType = moving;
            }
        }
    
    char alreadyDrawnPerson = false;
    
    HoldingPos holdingPos;
    holdingPos.valid = false;
    

    if( inObj->holdingID > 0 &&
        heldObject->rideable ) {
        // don't draw now,
        // wait until we know rideable's offset
        }
    else {
        alreadyDrawnPerson = true;
        doublePair personPos = pos;
        
        // decay away from riding offset, if any
        if( inObj->ridingOffset.x != 0 ||
            inObj->ridingOffset.y != 0 ) {
            
            doublePair nullOffset = { 0, 0 };
            
            
            doublePair delta = sub( nullOffset, inObj->ridingOffset );
            
            double step = frameRateFactor * 8;

            if( length( delta ) < step ) {
            
                inObj->ridingOffset.x = 0;
                inObj->ridingOffset.y = 0;
                }
            else {
                inObj->ridingOffset =
                    add( inObj->ridingOffset,
                         mult( normalize( delta ), step ) );
                }
                            
            // step offset BEFORE applying it
            // (so we don't repeat starting position)
            personPos = add( personPos, inObj->ridingOffset );
            }
        

        setAnimationEmotion( inObj->currentEmot );
        addExtraAnimationEmotions( &( inObj->permanentEmots ) );
        
        holdingPos =
            drawObjectAnim( inObj->displayID, 2, curType, 
                            timeVal,
                            animFade,
                            fadeTargetType,
                            targetTimeVal,
                            frozenRotTimeVal,
                            &( inObj->frozenRotFrameCountUsed ),
                            frozenArmType,
                            frozenArmFadeTargetType,
                            personPos,
                            0,
                            false,
                            inObj->holdingFlip,
                            age,
                            // don't actually hide body parts until
                            // held object is done sliding into place
                            hideClosestArm,
                            hideAllLimbs,
                            inObj->heldPosOverride && 
                            ! inObj->heldPosOverrideAlmostOver,
                            inObj->clothing,
                            inObj->clothingContained );
        
        setAnimationEmotion( NULL );
        }
    
        
    if( inObj->holdingID != 0 ) { 
        doublePair holdPos;
        
        double holdRot = 0;
        
        computeHeldDrawPos( holdingPos, pos,
                            heldObject,
                            inObj->holdingFlip,
                            &holdPos, &holdRot );

                
        doublePair heldObjectDrawPos = holdPos;
        
        if( heldObject != NULL && heldObject->rideable ) {
            heldObjectDrawPos = pos;
            }
        

        heldObjectDrawPos = mult( heldObjectDrawPos, 1.0 / CELL_D );
        
        if( inObj->currentSpeed == 0 &&
            inObj->heldPosOverride && 
            ! inObj->heldPosOverrideAlmostOver &&
            ! equal( heldObjectDrawPos, inObj->heldObjectPos ) ) {
                        
            doublePair delta = sub( heldObjectDrawPos, inObj->heldObjectPos );
            double rotDelta = holdRot - inObj->heldObjectRot;

            if( rotDelta > 0.5 ) {
                rotDelta = rotDelta - 1;
                }
            else if( rotDelta < -0.5 ) {
                rotDelta = 1 + rotDelta;
                }

            // as slide gets longer, we speed up
            double longSlideModifier = 1;
            
            double slideTime = inObj->heldPosSlideStepCount * frameRateFactor;
            
            if( slideTime > 30 ) {
                // more than a half second
                longSlideModifier = pow( slideTime / 30, 2 );
                }

            double step = frameRateFactor * 0.0625 * longSlideModifier;
            double rotStep = frameRateFactor * 0.03125;
            
            if( length( delta ) < step ) {
                inObj->heldObjectPos = heldObjectDrawPos;
                inObj->heldPosOverrideAlmostOver = true;
                }
            else {
                inObj->heldObjectPos =
                    add( inObj->heldObjectPos,
                         mult( normalize( delta ),
                               step ) );
                
                heldObjectDrawPos = inObj->heldObjectPos;
                }

            if( fabs( rotDelta ) < rotStep ) {
                inObj->heldObjectRot = holdRot;
                }
            else {

                double rotDir = 1;
                if( rotDelta < 0 ) {
                    rotDir = -1;
                    }

                inObj->heldObjectRot =
                    inObj->heldObjectRot + rotDir * rotStep;
                
                holdRot = inObj->heldObjectRot;
                }

            inObj->heldPosSlideStepCount ++;
            }
        else {
            inObj->heldPosOverride = false;
            inObj->heldPosOverrideAlmostOver = false;
            // track it every frame so we have a good
            // base point for smooth move when the object
            // is dropped
            inObj->heldObjectPos = heldObjectDrawPos;
            inObj->heldObjectRot = holdRot;
            }
          
        doublePair worldHoldPos = heldObjectDrawPos;
          
        heldObjectDrawPos = mult( heldObjectDrawPos, CELL_D );
        
        if( heldObject == NULL || 
            ! heldObject->rideable ) {
            
            holdPos = heldObjectDrawPos;
            }

        setDrawColor( 1, 1, 1, 1 );
                    
                    
        AnimType curHeldType = inObj->curHeldAnim;
        AnimType fadeTargetHeldType = inObj->curHeldAnim;
                
        double heldAnimFade = 1.0;
                    
        double heldTimeVal = frameRateFactor * 
            inObj->heldAnimationFrameCount / 60.0;
        
        double targetHeldTimeVal = heldTimeVal;
        
        double frozenRotHeldTimeVal = frameRateFactor * 
            inObj->heldFrozenRotFrameCount / 60.0;

        
        char heldFlip = inObj->holdingFlip;

        if( heldObject != NULL &&
            heldObject->noFlip ) {
            heldFlip = false;
            }


        if( !alreadyDrawnPerson ) {
            doublePair personPos = pos;
            
            doublePair targetRidingOffset = sub( personPos, holdPos );

            
            ObjectRecord *personObj = getObject( inObj->displayID );

            targetRidingOffset = 
                sub( targetRidingOffset, 
                     getAgeBodyOffset( 
                         age,
                         personObj->spritePos[ 
                             getBodyIndex( personObj, age ) ] ) );
            
            // step toward target to smooth
            doublePair delta = sub( targetRidingOffset, 
                                    inObj->ridingOffset );
            
            double step = frameRateFactor * 8;

            if( length( delta ) < step ) {            
                inObj->ridingOffset = targetRidingOffset;
                }
            else {
                inObj->ridingOffset =
                    add( inObj->ridingOffset,
                         mult( normalize( delta ), step ) );
                }

            personPos = add( personPos, inObj->ridingOffset );

            setAnimationEmotion( inObj->currentEmot );
            addExtraAnimationEmotions( &( inObj->permanentEmots ) );
            
            if( heldObject->anySpritesBehindPlayer ) {
                // draw part that is behind player
                prepareToSkipSprites( heldObject, true );
                
                if( inObj->numContained == 0 ) {
                    drawObjectAnim(
                        inObj->holdingID, curHeldType, 
                        heldTimeVal,
                        heldAnimFade,
                        fadeTargetHeldType,
                        targetHeldTimeVal,
                        frozenRotHeldTimeVal,
                        &( inObj->heldFrozenRotFrameCountUsed ),
                        endAnimType,
                        endAnimType,
                        heldObjectDrawPos,
                        holdRot,
                        false,
                        heldFlip, -1, false, false, false,
                        getEmptyClothingSet(), NULL,
                        0, NULL, NULL );
                    }
                else {
                    drawObjectAnim( 
                        inObj->holdingID, curHeldType, 
                        heldTimeVal,
                        heldAnimFade,
                        fadeTargetHeldType,
                        targetHeldTimeVal,
                        frozenRotHeldTimeVal,
                        &( inObj->heldFrozenRotFrameCountUsed ),
                        endAnimType,
                        endAnimType,
                        heldObjectDrawPos,
                        holdRot,
                        false,
                        heldFlip,
                        -1, false, false, false,
                        getEmptyClothingSet(),
                        NULL,
                        inObj->numContained,
                        inObj->containedIDs,
                        inObj->subContainedIDs );
                    }
                
                restoreSkipDrawing( heldObject );
                }
            

            // rideable object
            holdingPos =
                drawObjectAnim( inObj->displayID, 2, curType, 
                                timeVal,
                                animFade,
                                fadeTargetType,
                                targetTimeVal,
                                frozenRotTimeVal,
                                &( inObj->frozenRotFrameCountUsed ),
                                frozenArmType,
                                frozenArmFadeTargetType,
                                personPos,
                                0,
                                false,
                                inObj->holdingFlip,
                                age,
                                // don't actually hide body parts until
                                // held object is done sliding into place
                                hideClosestArm,
                                hideAllLimbs,
                                inObj->heldPosOverride && 
                                ! inObj->heldPosOverrideAlmostOver,
                                inObj->clothing,
                                inObj->clothingContained );
            
            setAnimationEmotion( NULL );
            }
        


        // animate baby with held anim just like any other held object
        if( inObj->lastHeldAnimFade > 0 ) {
            curHeldType = inObj->lastHeldAnim;
            fadeTargetHeldType = inObj->curHeldAnim;
            heldAnimFade = inObj->lastHeldAnimFade;
            
            heldTimeVal = frameRateFactor * 
                inObj->lastHeldAnimationFrameCount / 60.0;
            }
        
                    
        if( inObj->holdingID < 0 ) {
            // draw baby here
            int babyID = - inObj->holdingID;
            
            LiveObject *babyO = getGameObject( babyID );
            
            if( babyO != NULL ) {
                
                // save flip so that it sticks when baby set down
                babyO->holdingFlip = inObj->holdingFlip;
                
                // save world hold pos for smooth set-down of baby
                babyO->lastHeldByRawPosSet = true;
                babyO->lastHeldByRawPos = worldHoldPos;

                int hideClosestArmBaby = 0;
                char hideAllLimbsBaby = false;

                if( babyO->holdingID > 0 ) {
                    ObjectRecord *babyHoldingObj = 
                        getObject( babyO->holdingID );
                    
                    getArmHoldingParameters( babyHoldingObj, 
                                             &hideClosestArmBaby,
                                             &hideAllLimbsBaby );
                    }
                
                
                setAnimationEmotion( babyO->currentEmot );
                addExtraAnimationEmotions( &( babyO->permanentEmots ) );

                doublePair babyHeldPos = holdPos;
                
                if( babyO->babyWiggle ) {
                    
                    babyO->babyWiggleProgress += 0.04 * frameRateFactor;
                    
                    if( babyO->babyWiggleProgress > 1 ) {
                        babyO->babyWiggle = false;
                        }
                    else {

                        // cosine from pi to 3 pi has smooth start and finish
                        int wiggleDir = 1;
                        if( heldFlip ) {
                            wiggleDir = -1;
                            }
                        babyHeldPos.x += wiggleDir * 8 *
                            ( cos( babyO->babyWiggleProgress * 2 * M_PI +
                                   M_PI ) * 0.5 + 0.5 );
                        }
                    }

                returnPack =
                    drawObjectAnimPacked( 
                                babyO->displayID, curHeldType, 
                                heldTimeVal,
                                heldAnimFade,
                                fadeTargetHeldType,
                                targetHeldTimeVal,
                                frozenRotHeldTimeVal,
                                &( inObj->heldFrozenRotFrameCountUsed ),
                                endAnimType,
                                endAnimType,
                                babyHeldPos,
                                // never apply held rot to baby
                                0,
                                false,
                                heldFlip,
                                computeCurrentAge( babyO ),
                                hideClosestArmBaby,
                                hideAllLimbsBaby,
                                false,
                                babyO->clothing,
                                babyO->clothingContained,
                                0, NULL, NULL );
                
                setAnimationEmotion( NULL );

                if( babyO->currentSpeech != NULL ) {
                    
                    inSpeakers->push_back( babyO );
                    inSpeakersPos->push_back( holdPos );
                    }
                }
            }
        else if( inObj->numContained == 0 ) {
                        
            returnPack = 
                drawObjectAnimPacked(
                            inObj->holdingID, curHeldType, 
                            heldTimeVal,
                            heldAnimFade,
                            fadeTargetHeldType,
                            targetHeldTimeVal,
                            frozenRotHeldTimeVal,
                            &( inObj->heldFrozenRotFrameCountUsed ),
                            endAnimType,
                            endAnimType,
                            heldObjectDrawPos,
                            holdRot,
                            false,
                            heldFlip, -1, false, false, false,
                            getEmptyClothingSet(), NULL,
                            0, NULL, NULL );
            }
        else {
            returnPack =
                drawObjectAnimPacked( 
                            inObj->holdingID, curHeldType, 
                            heldTimeVal,
                            heldAnimFade,
                            fadeTargetHeldType,
                            targetHeldTimeVal,
                            frozenRotHeldTimeVal,
                            &( inObj->heldFrozenRotFrameCountUsed ),
                            endAnimType,
                            endAnimType,
                            heldObjectDrawPos,
                            holdRot,
                            false,
                            heldFlip,
                            -1, false, false, false,
                            getEmptyClothingSet(),
                            NULL,
                            inObj->numContained,
                            inObj->containedIDs,
                            inObj->subContainedIDs );
            }
        }
                
    if( inObj->currentSpeech != NULL ) {
                    
        inSpeakers->push_back( inObj );
        inSpeakersPos->push_back( pos );
        }

    if( inObj->id == ourID ) {
        setClothingHighlightFades( NULL );
        }
    
    return returnPack;
    }



void LivingLifePage::drawHungerMaxFillLine( doublePair inAteWordsPos,
                                            int inMaxFill,
                                            SpriteHandle *inBarSprites,
                                            SpriteHandle *inDashSprites,
                                            char inSkipBar,
                                            char inSkipDashes ) {
    
    
    
    //FOV
	doublePair barPos = { lastScreenViewCenter.x - ( recalcOffsetX( 590 ) * gui_fov_scale ), 
						  lastScreenViewCenter.y - ( recalcOffsetY( 334 ) * gui_fov_scale )};

    barPos.x -= 12 * gui_fov_scale_hud;
    barPos.y -= 10 * gui_fov_scale_hud;
    
    
    barPos.x += ( 30 * gui_fov_scale_hud ) * inMaxFill;

    if( ! inSkipBar ) {
        drawSprite( inBarSprites[ inMaxFill %
                                  NUM_HUNGER_DASHES ], 
                    barPos, gui_fov_scale_hud );
        }
    

    if( inSkipDashes ) {
        return;
        }

    doublePair dashPos = inAteWordsPos;
            
    dashPos.y -= 6 * gui_fov_scale_hud;
    dashPos.x -= 5 * gui_fov_scale_hud;

    int numDashes = 0;
            
    JenkinsRandomSource dashRandSource( 0 );

    while( dashPos.x > barPos.x + 9 ) {
        
        doublePair drawPos = dashPos;
        
        //drawPos.x += dashRandSource.getRandomBoundedInt( -2, 2 );
        //drawPos.y += dashRandSource.getRandomBoundedInt( -1, 1 );
        
        drawSprite( inDashSprites[ numDashes %
                                   NUM_HUNGER_DASHES ], 
                    drawPos, gui_fov_scale_hud );
        dashPos.x -= 15 * gui_fov_scale_hud;
        //numDashes += dashRandSource.getRandomBoundedInt( 1, 10 );
        numDashes += 1;
        
        // correct shortness of last one
        if( numDashes % NUM_HUNGER_DASHES == 0 ) {
            dashPos.x += 3 * gui_fov_scale_hud;
            }
        }
            
    // draw one more to connect to bar
    dashPos.x = barPos.x + ( 6 * gui_fov_scale_hud );
    drawSprite( inDashSprites[ numDashes %
                               NUM_HUNGER_DASHES ], 
                dashPos, gui_fov_scale_hud );
    }




static void drawLine( SpriteHandle inSegmentSprite,
                      doublePair inStart, doublePair inEnd,
                      FloatColor inColor ) {
    
    doublePair dir = normalize( sub( inEnd, inStart ) );
    
    doublePair perpDir = { -dir.y, dir.x };
    
    perpDir = mult( perpDir, 2 );
    

    doublePair spriteVerts[4] = 
        { { inStart.x - perpDir.x,
            inStart.y - perpDir.y },
          { inEnd.x - perpDir.x,
            inEnd.y - perpDir.y },
          { inEnd.x + perpDir.x,
            inEnd.y + perpDir.y },
          { inStart.x + perpDir.x,
            inStart.y + perpDir.y } };
    
    FloatColor spriteColors[4] = 
        { inColor, inColor, inColor, inColor };
    
                                
    drawSprite( inSegmentSprite,
                spriteVerts, spriteColors );
    }



static double getBoundedRandom( int inX, int inY,
                                double inUpper, double inLower ) {
    double val = getXYRandom( inX, inY );
    
    return val * ( inUpper - inLower ) + inLower;
    }




static char isInBounds( int inX, int inY, int inMapD ) {
    if( inX < 0 || inY < 0 || inX > inMapD - 1 || inY > inMapD - 1 ) {
        return false;
        }
    return true;
    }



static void drawFixedShadowString( const char *inString, doublePair inPos ) {
    
    setDrawColor( 0, 0, 0, 1 );
    numbersFontFixed->drawString( inString, inPos, alignLeft );
            
    setDrawColor( 1, 1, 1, 1 );
            
    inPos.x += 2;
    inPos.y -= 2;
    numbersFontFixed->drawString( inString, inPos, alignLeft );
    }


static void addToGraph( SimpleVector<double> *inHistory, double inValue ) {
    inHistory->push_back( inValue );
                
    while( inHistory->size() > historyGraphLength ) {
        inHistory->deleteElement( 0 );
        }
    }



static void drawGraph( SimpleVector<double> *inHistory, doublePair inPos,
                       FloatColor inColor ) {
    double max = 0;
    for( int i=0; i<inHistory->size(); i++ ) {
        double val = inHistory->getElementDirect( i );
        if( val > max ) {
            max = val;
            }
        }

    setDrawColor( 0, 0, 0, 0.5 );

    double graphHeight = 40;

    drawRect( inPos.x - 2, 
              inPos.y - 2,
              inPos.x + historyGraphLength + 2,
              inPos.y + graphHeight + 2 );
        
    

    setDrawColor( inColor.r, inColor.g, inColor.b, 0.75 );
    for( int i=0; i<inHistory->size(); i++ ) {
        double val = inHistory->getElementDirect( i );

        double scaledVal = val / max;
        
        drawRect( inPos.x + i, 
                  inPos.y,
                  inPos.x + i + 1,
                  inPos.y + scaledVal * graphHeight );
        }
    }



typedef struct DrawOrderRecord {
        char person;
        // if person
        LiveObject *personO;
        
        // if cell
        int mapI;
        int screenX, screenY;

        char extraMovingObj;
        // if extra moving obj
        int extraMovingIndex;
        
    } DrawOrderRecord;
        


char drawAdd = true;
char drawMult = true;

double multAmount = 0.15;
double addAmount = 0.25;


char blackBorder = false;
                                
char whiteBorder = true;

//FOV
static void drawHUDBarPart( double x, double y, double width, double height ) {
    doublePair barPos[4] = {
        { x, y + height },
        { x + width, y + height },
        { x + width, y },
        { x, y }
        };
    double gapLength = abs( barPos[0].x - barPos[1].x ) / ( 256. * gui_fov_scale_hud );
    doublePair barTexCoords[4] = {
        { 0.f, 0.f },
        { gapLength, 0.f },
        { gapLength, 1.f },
        { 0.f , 1.f },
        };
    drawSprite( guiPanelTileSprite, barPos, barTexCoords );
    }

#include "GameScreen_Draw.cpp"

char nearEndOfMovement( LiveObject *inPlayer )
{
    if( inPlayer->currentSpeed == 0  ) {
        return true;
        }
    else if( inPlayer->currentPathStep >= inPlayer->pathLength - 2 ) {
        return true;
        }
    return false;
    }



void playPendingReceivedMessages( LiveObject *inPlayer ) {
    printf( "Playing %d pending received messages for %d\n", 
            inPlayer->pendingReceivedMessages.size(), inPlayer->id );
    
    for( int i=0; i<inPlayer->pendingReceivedMessages.size(); i++ ) {
        readyPendingReceivedMessages.push_back( 
            inPlayer->pendingReceivedMessages.getElementDirect( i ) );
        }
    inPlayer->pendingReceivedMessages.deleteAll();
    }



void playPendingReceivedMessagesRegardingOthers( LiveObject *inPlayer ) {
    printf( "Playing %d pending received messages for %d "
            "    (skipping those that don't affect other players or map)\n", 
            inPlayer->pendingReceivedMessages.size(), inPlayer->id );

    for( int i=0; i<inPlayer->pendingReceivedMessages.size(); i++ ) {
        char *message = inPlayer->pendingReceivedMessages.getElementDirect( i );
        
        if( strstr( message, "PU" ) == message ) {
            // only keep PU's not about this player
            
            int messageID = -1;
            
            sscanf( message, "PU\n%d", &messageID );
            
            if( messageID != inPlayer->id ) {
                readyPendingReceivedMessages.push_back( message );
                }
            else {
                delete [] message;
                }
            }
        else if( strstr( message, "PM" ) == message ) {
            // only keep PM's not about this player
            
            int messageID = -1;
            
            sscanf( message, "PM\n%d", &messageID );
            
            if( messageID != inPlayer->id ) {
                readyPendingReceivedMessages.push_back( message );
                }
            else {
                delete [] message;
                }
            }
        else {
            // not a PU, keep it no matter what (map change, etc.
            readyPendingReceivedMessages.push_back( message );
            }
        }
    inPlayer->pendingReceivedMessages.deleteAll();
    }



void dropPendingReceivedMessagesRegardingID( LiveObject *inPlayer,
                                             int inIDToDrop ) {
    for( int i=0; i<inPlayer->pendingReceivedMessages.size(); i++ ) {
        char *message = inPlayer->pendingReceivedMessages.getElementDirect( i );
        char match = false;
        
        if( strstr( message, "PU" ) == message ) {
            // only keep PU's not about this player
            
            int messageID = -1;
            
            sscanf( message, "PU\n%d", &messageID );
            
            if( messageID == inIDToDrop ) {
                match = true;
                }
            }
        else if( strstr( message, "PM" ) == message ) {
            // only keep PM's not about this player
            
            int messageID = -1;
            
            sscanf( message, "PM\n%d", &messageID );
            
            if( messageID == inIDToDrop ) {
                match = true;
                }
            }
        
        if( match ) {
            delete [] message;
            inPlayer->pendingReceivedMessages.deleteElement( i );
            i--;
            }
        }
    }




void LivingLifePage::applyReceiveOffset( int *inX, int *inY ) {
    if( mMapGlobalOffsetSet ) {
        *inX -= mMapGlobalOffset.x;
        *inY -= mMapGlobalOffset.y;
        }
    }



int LivingLifePage::sendX( int inX ) {
    if( mMapGlobalOffsetSet ) {
        return inX + mMapGlobalOffset.x;
        }
    return inX;
    }



int LivingLifePage::sendY( int inY ) {
    if( mMapGlobalOffsetSet ) {
        return inY + mMapGlobalOffset.y;
        }
    return inY;
    }



char *LivingLifePage::getDeathReason()
{
    if( mDeathReason != NULL ) {
        return stringDuplicate( mDeathReason );
        }
    else {
        return NULL;
        }
}



void LivingLifePage::handleOurDeath( char inDisconnect )
{
    
    if( mDeathReason == NULL ) {
        mDeathReason = stringDuplicate( "" );
        }
    
    int years = (int)floor( computeCurrentAge( getOurLiveObject() ) );

    char *ageString;
    
    if( years == 1 ) {
        ageString = stringDuplicate( translate( "deathAgeOne" ) );
        }
    else {
        ageString = autoSprintf( translate( "deathAge" ), years );
        }
    
    char *partialReason = stringDuplicate( mDeathReason );
    delete [] mDeathReason;
    
    mDeathReason = autoSprintf( "%s####%s", ageString, partialReason );
    
    delete [] ageString;
    delete [] partialReason;
    

    setWaiting( false );

    //reset fov on death or disconnect
		changeFOV( 1.0 );

    if( inDisconnect ) {
        setSignal( "disconnect" );
        }
    else {    
        setSignal( "died" );
        }
    
    instantStopMusic();
    // so sound tails are not still playing when we we get reborn
    fadeSoundSprites( 0.1 );
    setSoundLoudness( 0 );
}



static char isCategory( int inID )
{
    if( inID <= 0 ) {
        return false;
        }
    
    CategoryRecord *c = getCategory( inID );
    
    if( c == NULL ) {
        return false;
        }
    if( ! c->isPattern && c->objectIDSet.size() > 0 ) {
        return true;
        }
    return false;
}


static char isFood( int inID )
{
    if( inID <= 0 ) {
        return false;
        }
    
    ObjectRecord *o = getObject( inID );
    
    if( o->foodValue > 0 ) {
        return true;
        }
    else {
        return false;
        }
}

static char getTransHintable( TransRecord *inTrans )
{
    if( inTrans->lastUseActor ) {
        return false;
        }
    if( inTrans->lastUseTarget ) {
        return false;
        }
    
    if( inTrans->actor >= 0 && 
        ( inTrans->target > 0 || inTrans->target == -1 ) &&
        ( inTrans->newActor > 0 || inTrans->newTarget > 0 ) ) {


        if( isCategory( inTrans->actor ) ) {
            return false;
            }
        if( isCategory( inTrans->target ) ) {
            return false;
            }
        
        if( inTrans->target == -1 && inTrans->newTarget == 0 &&
            ! isFood( inTrans->actor ) ) {
            // generic one-time-use transition
            return false;
            }

        return true;
        }
    else {
        return false;
        }
}

static int findMainObjectID( int inObjectID )
{
    if( inObjectID <= 0 ) {
        return inObjectID;
        }
    
    ObjectRecord *o = getObject( inObjectID );
    
    if( o == NULL ) {
        return inObjectID;
        }
    
    if( o->isUseDummy ) {
        return o->useDummyParent;
        }
    else {
        return inObjectID;
        }
}


static int getTransMostImportantResult( TransRecord *inTrans )
{
    int actor = findMainObjectID( inTrans->actor );
    int target = findMainObjectID( inTrans->target );
    int newActor = findMainObjectID( inTrans->newActor );
    int newTarget = findMainObjectID( inTrans->newTarget );

    int result = 0;
        

    if( target != newTarget &&
        newTarget > 0 &&
        actor != newActor &&
        newActor > 0 ) {
        // both actor and target change
        // we need to decide which is the most important result
        // to hint
            
        if( actor == 0 && newActor > 0 ) {
            // something new ends in your hand, that's more important
            result = newActor;
            }
        else {
            // if the trans takes one of the elements to a deeper
            // state, that's more important, starting with actor
            if( actor > 0 && 
                getObjectDepth( newActor ) > getObjectDepth( actor ) ) {
                result = newActor;
                }
            else if( target > 0 && 
                     getObjectDepth( newTarget ) > 
                     getObjectDepth( target ) ) {
                result = newTarget;
                }
            // else neither actor or target becomes deeper
            // which result is deeper?
            else if( getObjectDepth( newActor ) > 
                     getObjectDepth( newTarget ) ) {
                result = newActor;
                }
            else {
                result = newTarget;
                }
            }
        }
    else if( target != newTarget && 
             newTarget > 0 ) {
            
        result = newTarget;
        }
    else if( actor != newActor && 
             newActor > 0 ) {
            
        result = newActor;
        }
    else if( newTarget > 0 ) {
        // don't show NOTHING as a result
        result = newTarget;
        }

    return result;
}

int LivingLifePage::getNumHints( int inObjectID )
{
    char sameFilter = false;
    
    if( mLastHintFilterString == NULL &&
        mHintFilterString == NULL ) {
        sameFilter = true;
        }
    else if( mLastHintFilterString != NULL &&
             mHintFilterString != NULL &&
             strcmp( mLastHintFilterString, mHintFilterString ) == 0 ) {
        sameFilter = true;
        }
    


    if( mLastHintSortedSourceID == inObjectID && sameFilter ) {
        return mLastHintSortedList.size();
        }
    

    // else need to regenerated sorted list

    mLastHintSortedSourceID = inObjectID;
    mLastHintSortedList.deleteAll();

    if( mLastHintFilterString != NULL ) {
        delete [] mLastHintFilterString;
        mLastHintFilterString = NULL;
        }

    if( mHintFilterString != NULL ) {
        mLastHintFilterString = stringDuplicate( mHintFilterString );
        }
    
    if( ! sameFilter ) {
        // new filter, clear all bookmarks
        int maxObjectID = getMaxObjectID();
        for( int i=0; i<=maxObjectID; i++ ) {
            mHintBookmarks[i] = 0;
            }
        }
    

    // heap sort
    MinPriorityQueue<TransRecord*> queue;
    
    SimpleVector<TransRecord*> *trans = getAllUses( inObjectID );
    
    
    SimpleVector<TransRecord *> unfilteredTrans;
    SimpleVector<TransRecord *> filteredTrans;
    
    if( trans != NULL )
    for( int i = 0; i<trans->size(); i++ ) {
        TransRecord *t = trans->getElementDirect( i );
        
        if( getTransHintable( t ) ) {
            unfilteredTrans.push_back( t );
            filteredTrans.push_back( t );
            }
        }


    int numFilterHits = 0;


    if( mLastHintFilterString != NULL && filteredTrans.size() > 0 ) {        
        unsigned int filterLength = strlen( mLastHintFilterString );
        
        int numHits = 0;
        int numRemain = 0;
        ObjectRecord **hits = searchObjects( mLastHintFilterString,
                                             0,
                                             200,
                                             &numHits, &numRemain );
        
        SimpleVector<int> hitMatchIDs;

        numFilterHits = numHits;

        SimpleVector<int> exactHitMatchIDs;
        

        for( int i=0; i<numHits; i++ ) {
            if( hits[i]->id == inObjectID ) {
                // don't count the object itself as a hit
                continue;
                }
            char *des = stringToUpperCase( hits[i]->description );
            
            stripDescriptionComment( des );
        
            if( strcmp( des, mLastHintFilterString ) == 0 ) {
                exactHitMatchIDs.push_back( hits[i]->id );
                }

            char *searchPos = strstr( des, mLastHintFilterString );
            
            // only count if occurrence of filter string matches whole words
            // not partial words
            if( searchPos != NULL && strlen( searchPos ) >= filterLength ) {
                
                unsigned int remainLen = strlen( searchPos );

                char frontOK = false;
                char backOK = false;
                
                // space or start of string in front of search phrase
                if( searchPos == des ||
                    searchPos[-1] == ' ' ) {
                    frontOK = true;
                    }
                
                // space or end of string after search phrase
                if( remainLen == filterLength ||
                    searchPos[filterLength] == ' ' ) {
                    backOK = true;
                    }

                if( frontOK && backOK ) {
                    hitMatchIDs.push_back( hits[i]->id );
                    }
                }
            
            delete [] des;
            }
        
        
        // now find shallowest matching objects

        numHits = hitMatchIDs.size();
        
        int startDepth = getObjectDepth( inObjectID );
        
        ObjectRecord *startObject = getObject( inObjectID );
        if( startObject->isUseDummy ) {
            startDepth = getObjectDepth( startObject->useDummyParent );
            }
        

        int shallowestDepth = UNREACHABLE;
       
        for( int i=0; i<numHits; i++ ) {
            
            int depth = getObjectDepth( hitMatchIDs.getElementDirect( i ) );
            
            if( depth >= startDepth && depth < shallowestDepth ) {
                shallowestDepth = depth;
                }
            }

        SimpleVector<int> hitIDs;

        for( int i=0; i<numHits; i++ ) {
            int id = hitMatchIDs.getElementDirect( i );
            
            int depth = getObjectDepth( id );
            
            if( depth == shallowestDepth ) {
                hitIDs.push_back( id );
                }
            }
        

        if( hits != NULL ) {    
            delete [] hits;
            }
        
        // there are exact matches
        // use those instead
        if( exactHitMatchIDs.size() > 0 ) {
            hitIDs.deleteAll();
            hitIDs.push_back_other( &exactHitMatchIDs );
            }
        


        numHits = hitIDs.size();

        // list of IDs that are used to make hit objects
        SimpleVector<int> precursorIDs;
        
        if( numHits > 0 ) {
            
            if( numHits < 10 ) {
                
                for( int i=0; i<numHits; i++ ) {
                    precursorIDs.push_back( hitIDs.getElementDirect( i ) );
                    }
                // go limited number of steps back
                
                SimpleVector<int> lastStep = precursorIDs;

                for( int s=0; s<10; s++ ) {
                    SimpleVector<int> oldLastStep = lastStep;
                    
                    lastStep.deleteAll();
                    
                    for( int i=0; i<oldLastStep.size(); i++ ) {
                        int oldStepID = oldLastStep.getElementDirect( i );
                        if( oldStepID == inObjectID ) {
                            // don't follow precursor chains back through
                            // our object
                            // don't care about things BEFORE out object
                            // that lead to filter target
                            continue;
                            }
                        
                        int oldStepDepth = getObjectDepth( oldStepID );


                        int numResults = 0;
                        int numRemain = 0;
                        TransRecord **prodTrans =
                            searchProduces( oldStepID, 
                                            0,
                                            200,
                                            &numResults, &numRemain );
                        
                        if( prodTrans != NULL ) {
                            
                            int shallowestTransDepth = UNREACHABLE;
                            int shallowestTransIndex = -1;

                            for( int t=0; t<numResults; t++ ) {
                                
                                int actor = prodTrans[t]->actor;
                                int target = prodTrans[t]->target;
                                
                                int transDepth = UNREACHABLE;
                                
                                if( actor > 0 ) {
                                    transDepth = getObjectDepth( actor );
                                    if( transDepth == UNREACHABLE ) {
                                        // is this a category?
                                        CategoryRecord *cat = 
                                            getCategory( actor );
                                        
                                        if( cat != NULL &&
                                            cat->objectIDSet.size() > 0 &&
                                            ! cat->isPattern ) {
                                            continue;
                                            }
                                        }
                                    }
                                if( target > 0 ) {
                                    int targetDepth = getObjectDepth( target );
                                    if( targetDepth == UNREACHABLE ) {
                                        // must be category
                                        // is this a category?
                                        CategoryRecord *cat = 
                                            getCategory( target );
                                        
                                        if( cat != NULL && 
                                            cat->objectIDSet.size() > 0 &&
                                            ! cat->isPattern ) {
                                            continue;
                                            }
                                        }
                                    if( targetDepth > transDepth ||
                                        transDepth == UNREACHABLE ) {
                                        transDepth = targetDepth;
                                        }
                                    }
                                
                                if( transDepth < shallowestTransDepth ) {
                                    shallowestTransDepth = transDepth;
                                    shallowestTransIndex = t;
                                    }
                                }
                            
                            
                            if( shallowestTransIndex != -1 &&
                                shallowestTransDepth < oldStepDepth ) {
                                int actor = 
                                    prodTrans[shallowestTransIndex]->actor;
                                int target = 
                                    prodTrans[shallowestTransIndex]->target;
                                
                                if( actor > 0 &&
                                    precursorIDs.
                                    getElementIndex( actor ) == -1 ) {

                                    precursorIDs.push_back( actor );
                                    lastStep.push_back( actor );
                                    }

                                if( target > 0 && 
                                    precursorIDs.
                                    getElementIndex( target ) == -1 ) {

                                    precursorIDs.push_back( target );
                                    lastStep.push_back( target );
                                    }
                                }
                            
                            delete [] prodTrans;
                            }
                        }
                    }
                }
            
            int numPrecursors = precursorIDs.size();

            for( int i = 0; i<filteredTrans.size(); i++ ) {
                char matchesFilter = false;
                TransRecord *t = filteredTrans.getElementDirect( i );
                
                // don't show trans that result in a hit or a precursor of a
                // hit if the trans doesn't display that hit or precursor
                // as a result when shown to user (will be confusing if
                // it only produces the precursor as a "side-effect"
                // example:  bone needle produced from stitching shoes
                //           and bone needle is a precursor of bellows
                //           but it's very odd to show the shoe-producing
                //           transition when filtering by bellows and
                //           holding two rabbit furs.
                int resultOfTrans = getTransMostImportantResult( t );
                
                for( int h=0; h<numHits; h++ ) {
                    int id = hitIDs.getElementDirect( h );    
                    
                    if( t->actor != id && t->target != id 
                        &&
                        ( resultOfTrans == id ) ) {
                        matchesFilter = true;
                        break;
                        }    
                    }
                if( matchesFilter == false ) {
                    for( int p=0; p<numPrecursors; p++ ) {
                        int id = precursorIDs.getElementDirect( p );
                        
                        if( t->actor != id && t->target != id 
                            &&
                            ( resultOfTrans == id ) ) {
                            // precursors only count if they actually
                            // make id, not just if they use it

                            // but make sure it doesn't use
                            // one of our main hits as an ingredient
                            char hitIsIngredient = false;
                            
                            int actor = t->actor;
                            int target = t->target;
                            
                            if( actor > 0 ) {
                                ObjectRecord *actorO = 
                                    getObject( actor );
                                if( actorO->isUseDummy ) {
                                    actor = actorO->useDummyParent;
                                    }
                                }
                            if( target > 0 ) {
                                ObjectRecord *targetO = 
                                    getObject( target );
                                if( targetO->isUseDummy ) {
                                    target = targetO->useDummyParent;
                                    }
                                }
                            
                            for( int h=0; h<numHits; h++ ) {
                                int hitID = hitIDs.getElementDirect( h ); 
                                
                                if( actor == hitID || 
                                    target == hitID ) {
                                    hitIsIngredient = true;
                                    break;
                                    }
                                }
                            if( ! hitIsIngredient ) {
                                matchesFilter = true;
                                break;
                                }
                            }
                        }
                    }
                
                if( ! matchesFilter ) {
                    filteredTrans.deleteElement( i );
                    i--;
                    }
                }
            }

        }

    
    int numTrans = filteredTrans.size();

    int numRelevant = numTrans;

      // for now, just leave it empty
    if( false &&
        numTrans == 0 && unfilteredTrans.size() > 0 ) {
        
        // after filtering, no transititions are left
        // show all trans instead
        for( int i = 0; i<unfilteredTrans.size(); i++ ) {
            filteredTrans.push_back( unfilteredTrans.getElementDirect( i ) );
            }
        numTrans = filteredTrans.size();
        }
    
    

    int lastSheet = NUM_HINT_SHEETS - 1;
    
    if( mPendingFilterString != NULL ) {
        delete [] mPendingFilterString;
        mPendingFilterString = NULL;
        }

    if( mLastHintFilterString != NULL ) {

        if( mPendingFilterString != NULL ) {
            delete [] mPendingFilterString;
            mPendingFilterString = NULL;
            }
        
        if( numRelevant == 0 || numFilterHits == 0 ) {
            const char *key = "notRelevant";
            char *reasonString = NULL;
            if( numFilterHits == 0 && unfilteredTrans.size() > 0 ) {
                // no match because object named in filter does not
                // exist
                key = "noMatch";
                reasonString = stringDuplicate( translate( key ) );
                }
            else {
                const char *formatString = translate( key );
                
                
                char *objString = getDisplayObjectDescription( inObjectID );
                reasonString = autoSprintf( formatString, objString );
                
                delete [] objString;
                }
            
            

            mPendingFilterString = autoSprintf( "%s %s %s",
                                                translate( "making" ),
                                                mLastHintFilterString,
                                                reasonString );
            delete [] reasonString;
            }
        else {    
            mPendingFilterString = autoSprintf( "%s %s",
                                                translate( "making" ),
                                                mLastHintFilterString );
            }
        
        }
    else {
        mHintTargetOffset[ lastSheet ] = mHintHideOffset[ lastSheet ];
        }




    // skip any that repeat exactly the same string
    // (example:  goose pond in different states)
    SimpleVector<char *> otherActorStrings;
    SimpleVector<char *> otherTargetStrings;
    

    for( int i=0; i<numTrans; i++ ) {
        TransRecord *tr = filteredTrans.getElementDirect( i );
        
        int depth = 0;
            
        if( tr->actor > 0 && tr->actor != inObjectID ) {
            depth = getObjectDepth( tr->actor );
            }
        else if( tr->target > 0 && tr->target != inObjectID ) {
            depth = getObjectDepth( tr->target );
            }
            
            
        char stringAlreadyPresent = false;
            
        if( tr->actor > 0 && tr->actor != inObjectID ) {
            ObjectRecord *otherObj = getObject( tr->actor );
                
            char *trimmedDesc = stringDuplicate( otherObj->description );
            stripDescriptionComment( trimmedDesc );

            for( int s=0; s<otherActorStrings.size(); s++ ) {
                    
                if( strcmp( trimmedDesc, 
                            otherActorStrings.getElementDirect( s ) )
                    == 0 ) {
                        
                    stringAlreadyPresent = true;
                    break;
                    }    
                }

            if( !stringAlreadyPresent ) {
                otherActorStrings.push_back( trimmedDesc );
                }
            else {
                delete [] trimmedDesc;
                }
            }
            
        if( tr->target > 0 && tr->target != inObjectID ) {
            ObjectRecord *otherObj = getObject( tr->target );
                
            char *trimmedDesc = stringDuplicate( otherObj->description );
            stripDescriptionComment( trimmedDesc );

            for( int s=0; s<otherTargetStrings.size(); s++ ) {
                    
                if( strcmp( trimmedDesc, 
                            otherTargetStrings.getElementDirect( s ) )
                    == 0 ) {
                        
                    stringAlreadyPresent = true;
                    break;
                    }    
                }

            if( !stringAlreadyPresent ) {
                otherTargetStrings.push_back( trimmedDesc );
                }
            else {
                delete [] trimmedDesc;
                }
            }

            
        if( !stringAlreadyPresent ) {
            queue.insert( tr, depth );
            }
        }
    
    otherActorStrings.deallocateStringElements();
    otherTargetStrings.deallocateStringElements();

    int numInQueue = queue.size();
    
    for( int i=0; i<numInQueue; i++ ) {
        mLastHintSortedList.push_back( queue.removeMin() );
        }
    
    
    return mLastHintSortedList.size();
}



static double getLongestLine( char *inMessage ) {
    
    double longestLine = 0;
    
    int numLines;
    char **lines = split( inMessage, 
                          "#", &numLines );
    
    for( int l=0; l<numLines; l++ ) {
        double len = handwritingFont->measureString( lines[l] ) / gui_fov_scale_hud;
        
        if( len > longestLine ) {
            longestLine = len;
                    }
        delete [] lines[l];
        }
    delete [] lines;
    
    return longestLine;
}

char *LivingLifePage::getHintMessage( int inObjectID, int inIndex )
{

    if( inObjectID != mLastHintSortedSourceID ) {
        getNumHints( inObjectID );
        }

    TransRecord *found = NULL;

    if( inIndex < mLastHintSortedList.size() ) {    
        found = mLastHintSortedList.getElementDirect( inIndex );
        }
    

    
    if( found != NULL ) {
        int actor = findMainObjectID( found->actor );
        int target = findMainObjectID( found->target );
        int newActor = findMainObjectID( found->newActor );

        int result = getTransMostImportantResult( found );
        
        
        char *actorString;
        
        if( actor > 0 ) {
            actorString = stringToUpperCase( getObject( actor )->description );
            stripDescriptionComment( actorString );
            }
        else if( actor == 0 ) {
            actorString = stringDuplicate( translate( "bareHandHint" ) );
            }
        else {
            actorString = stringDuplicate( "" );
            }

        
        char eventually = false;
        
        
        char *targetString;
        
        if( target > 0 ) {
            targetString = 
                stringToUpperCase( getObject( target )->description );
            stripDescriptionComment( targetString );
            }
        else if( target == -1 && actor > 0 ) {
            ObjectRecord *actorObj = getObject( actor );
            
            if( actorObj->foodValue ) {
                targetString = stringDuplicate( translate( "eatHint" ) );
                
                if( result == 0 && 
                    actor == newActor &&
                    actorObj->numUses > 1 ) {
                    // see if there's a last-use transition and give hint
                    // about what it produces, in the end

                    int lastDummy = 
                        actorObj->useDummyIDs[ 0 ];
                    
                    TransRecord *lastUseTrans = getTrans( lastDummy, -1 );
                    
                    if( lastUseTrans != NULL ) {
                        if( lastUseTrans->newActor > 0 ) {
                            result = findMainObjectID( lastUseTrans->newActor );
                            eventually = true;
                            }
                        else if( lastUseTrans->newTarget > 0 ) {
                            result = 
                                findMainObjectID( lastUseTrans->newTarget );
                            eventually = true;
                            }
                        }    
                    }
                }
            else {
                targetString = stringDuplicate( translate( "bareGroundHint" ) );
                }
            }
        else {
            targetString = stringDuplicate( "" );
            }
        

        char *resultString;
        
        if( result > 0 ) {
            resultString = 
                stringToUpperCase( getObject( result )->description );
            }
        else {
            resultString = stringDuplicate( translate( "nothingHint" ) );
            }
        
        stripDescriptionComment( resultString );
        
        if( eventually ) {
            char *old = resultString;
            resultString = autoSprintf( "%s %s", old, 
                                        translate( "eventuallyHint" ) );
            delete [] old;
            }

        
        char *fullString =
            autoSprintf( "%s#%s#%s", actorString,
                         targetString, 
                         resultString );
        
        delete [] actorString;
        delete [] targetString;
        delete [] resultString;
        
        return fullString;
        }
    else {
        return stringDuplicate( translate( "noHint" ) );
        }
}

// inNewID > 0
static char shouldCreationSoundPlay( int inOldID, int inNewID )
{
    if( inOldID == inNewID ) {
        // no change
        return false;
        }
    

    // make sure this is really a fresh creation
    // of newID, and not a cycling back around
    // for a reusable object
    
    // also not useDummies that have the same
    // parent
    char sameParent = false;
    
    ObjectRecord *obj = getObject( inNewID );

    if( obj->creationSound.numSubSounds == 0 ) {
        return false;
        }

    if( inOldID > 0 && inNewID > 0 ) {
        ObjectRecord *oldObj = getObject( inOldID );
        
        if( obj->isUseDummy &&
            oldObj->isUseDummy &&
            obj->useDummyParent ==
            oldObj->useDummyParent ) {
            sameParent = true;
            }
        else if( obj->numUses > 1 
                 &&
                 oldObj->isUseDummy 
                 &&
                 oldObj->useDummyParent
                 == inNewID ) {
            sameParent = true;
            }
        else if( oldObj->numUses > 1 
                 &&
                 obj->isUseDummy 
                 &&
                 obj->useDummyParent
                 == inOldID ) {
            sameParent = true;
            }
        }
    
    if( ! sameParent 
        &&
        ( ! obj->creationSoundInitialOnly
          ||
          inOldID <= 0
          ||
          ( ! isSpriteSubset( inOldID, inNewID ) ) ) ) {
        return true;
        }

    return false;
}

static char checkIfHeldContChanged( LiveObject *inOld, LiveObject *inNew )
{
                    
    if( inOld->numContained != inNew->numContained ) {
        return true;
        }
    else {
        for( int c=0; c<inOld->numContained; c++ ) {
            if( inOld->containedIDs[c] != 
                inNew->containedIDs[c] ) {
                return true;
                }
            if( inOld->subContainedIDs[c].size() != 
                inNew->subContainedIDs[c].size() ) {
                return true;
                }
            for( int s=0; s<inOld->subContainedIDs[c].size();
                 s++ ) {
                if( inOld->subContainedIDs[c].
                    getElementDirect( s ) !=
                    inNew->subContainedIDs[c].
                    getElementDirect( s ) ) {
                    return true;
                    }
                }
            }
        }
    return false;
}

void LivingLifePage::sendBugReport( int inBugNumber )
{
    char *bugString = stringDuplicate( "" );

    if( lastMessageSentToServer != NULL ) {
        char *temp = bugString;
        bugString = autoSprintf( "%s   Just sent: [%s]",
                                 temp, lastMessageSentToServer );
        delete [] temp;
        }
    if( nextActionMessageToSend != NULL ) {
        char *temp = bugString;
        bugString = autoSprintf( "%s   Waiting to send: [%s]",
                                 temp, nextActionMessageToSend );
        delete [] temp;
        }
    
    // clear # terminators from message
    char *spot = strstr( bugString, "#" );
    
    while( spot != NULL ) {
        spot[0] = ' ';
        spot = strstr( bugString, "#" );
        }
    
    
    char *bugMessage = autoSprintf( "BUG %d %s#", inBugNumber, bugString );
    
    delete [] bugString;

    sendToServerSocket( bugMessage );
    delete [] bugMessage;
    
    if( ! SettingsManager::getIntSetting( "reportWildBugToUser", 1 ) ) {
        return;
        }

    FILE *f = fopen( "stdout.txt", "r" );

    int recordGame = SettingsManager::getIntSetting( "recordGame", 0 );
    
    if( f != NULL ) {
        // stdout.txt exists
        
        printf( "Bug report sent, telling user to email files to us.\n" );
        
        fclose( f );

        showBugMessage = 3;
        
        if( recordGame ) {
            showBugMessage = 2;
            }
        }
    else if( recordGame ) {
        printf( "Bug report sent, telling user to email files to us.\n" );
        showBugMessage = 1;
        }
    
}

void LivingLifePage::endExtraObjectMove( int inExtraIndex )
{
    int i = inExtraIndex;

    ExtraMapObject *o = mMapExtraMovingObjects.getElement( i );
    o->moveOffset.x = 0;
    o->moveOffset.y = 0;
    o->moveSpeed = 0;
                
    if( o->curAnimType != ground ) {
                        
        o->lastAnimType = o->curAnimType;
        o->curAnimType = ground;
        o->lastAnimFade = 1;
                    
        o->animationLastFrameCount = o->animationFrameCount;
                    
        o->animationFrameCount = 0;
        }

    GridPos worldPos = 
        mMapExtraMovingObjectsDestWorldPos.getElementDirect( i );
            
    int mapI = getMapIndex( worldPos.x, worldPos.y );
            
    if( mapI != -1 ) {
        // put it in dest
        putInMap( mapI, o );
        mMap[ mapI ] = 
            mMapExtraMovingObjectsDestObjectIDs.getElementDirect( i );
        }
            
    mMapExtraMovingObjects.deleteElement( i );
    mMapExtraMovingObjectsDestWorldPos.deleteElement( i );
    mMapExtraMovingObjectsDestObjectIDs.deleteElement( i );
}

void LivingLifePage::setNewCraving( int inFoodID, int inYumBonus )
{
    char *foodDescription = 
        stringToUpperCase( getObject( inFoodID )->description );
                
    stripDescriptionComment( foodDescription );

    char *message = 
        autoSprintf( "%s: %s (+%d)", translate( "craving"), 
                     foodDescription, inYumBonus );
    
    delete [] foodDescription;
    
    
    if( mLiveCravingSheetIndex > -1 ) {
        // hide old craving sheet
        mCravingTargetOffset[ mLiveCravingSheetIndex ] =
            mCravingHideOffset[ mLiveCravingSheetIndex ];
        }
    mLiveCravingSheetIndex ++;
    
    if( mLiveCravingSheetIndex >= NUM_HINT_SHEETS ) {
        mLiveCravingSheetIndex -= NUM_HINT_SHEETS;
        }
    
    if( mCravingMessage[ mLiveCravingSheetIndex ] != NULL ) {
        delete [] mCravingMessage[ mLiveCravingSheetIndex ];
        mCravingMessage[ mLiveCravingSheetIndex ] = NULL;
        }

    mCravingMessage[ mLiveCravingSheetIndex ] = message;
    
    mCravingTargetOffset[ mLiveCravingSheetIndex ] =
        mCravingHideOffset[ mLiveCravingSheetIndex ];
    
    mCravingTargetOffset[ mLiveCravingSheetIndex ].y += 64;
    
    double longestLine = getLongestLine( 
        (char*)( mCravingMessage[ mLiveCravingSheetIndex ] ) );
    
    mCravingExtraOffset[ mLiveCravingSheetIndex ].x = longestLine;
}

        
void LivingLifePage::step() {
    
    if( isAnySignalSet() ) {
        return;
        }

    if( apocalypseInProgress ) {
        double stepSize = frameRateFactor / ( apocalypseDisplaySeconds * 60.0 );
        apocalypseDisplayProgress += stepSize;
        }
    
    if( mRemapPeak > 0 ) {
        if( mRemapDelay < 1 ) {
            double stepSize = 
                frameRateFactor / ( remapDelaySeconds * 60.0 );
            mRemapDelay += stepSize;
            }
        else {

            double stepSize = 
                mRemapDirection * frameRateFactor / ( remapPeakSeconds * 60.0 );
        
            mCurrentRemapFraction += stepSize;
            
            if( stepSize > 0 && mCurrentRemapFraction >= mRemapPeak ) {
                mCurrentRemapFraction = mRemapPeak;
                mRemapDirection *= -1;
                }
            if( stepSize < 0 && mCurrentRemapFraction <= 0 ) {
                mCurrentRemapFraction = 0;
                mRemapPeak = 0;
                }
            if( takingPhoto ) {
                // stop remapping briefly during photo
                setRemapFraction( 0 );
                }
            else {
                setRemapFraction( mCurrentRemapFraction );
                }
            }
        }

    if( mouseDown ) {
        mouseDownFrames++;
        }
    
    if(!socketHandler->isConnected())
    {
        serverSocketConnected = false;
        connectionMessageFade = 1.0f;
        socketHandler->connect(serverIP, serverPort);
        //mServerSocket = openSocketConnection( serverIP, serverPort );
        timeLastMessageSent = game_getCurrentTime();
        return;
    }

    double pageLifeTime = game_getCurrentTime() - mPageStartTime;
    
    if( pageLifeTime < 1 ) {
        // let them see CONNECTING message for a bit
        return;
        }

    if( serverSocketConnected )
    {
        // we've heard from server, not waiting to connect anymore
        setWaiting( false );
	}
    else
	{
        
        if( pageLifeTime > 10 ) {
            // having trouble connecting.
            closeSocket( mServerSocket );
            mServerSocket = -1;

            setWaiting( false );
            setSignal( "connectionFailed" );
            
            return;
            }
	}

    // first, read all available data from server
    char readSuccess = readServerSocketFull( mServerSocket );

    if( ! readSuccess ) {
        
        if( serverSocketConnected ) {    
            double connLifeTime = game_getCurrentTime() - connectedTime;
            
            if( connLifeTime < 1 ) {
                // let player at least see waiting page
                // avoid flicker
                return;
                }
            }
        

        closeSocket( mServerSocket );
        mServerSocket = -1;

        if( mFirstServerMessagesReceived  ) {
            
            if( mDeathReason != NULL ) {
                delete [] mDeathReason;
                }
            mDeathReason = stringDuplicate( translate( "reasonDisconnected" ) );
            
            handleOurDeath( true );
            }
        else {
            setWaiting( false );
            setSignal( "loginFailed" );
            }
        return;
        }
    
    if( mLastMouseOverID != 0 ) {
        mLastMouseOverFade -= 0.01 * frameRateFactor;
        
        if( mLastMouseOverFade < 0 ) {
            mLastMouseOverID = 0;
            }
        }

    if( mGlobalMessageShowing ) {
        
        if( game_getCurrentTime() - mGlobalMessageStartTime > 10 ) {
            mTutorialTargetOffset[ mLiveTutorialSheetIndex ] =
                mTutorialHideOffset[ mLiveTutorialSheetIndex ];

            if( mTutorialPosOffset[ mLiveTutorialSheetIndex ].y ==
                mTutorialHideOffset[ mLiveTutorialSheetIndex ].y ) {
                // done hiding
                mGlobalMessageShowing = false;
                mGlobalMessagesToDestroy.deallocateStringElements();
                }
            }
        }

    // move moving objects
    int numCells = mMapD * mMapD;
    
    for( int i=0; i<numCells; i++ ) {
                
        if( mMapMoveSpeeds[i] > 0 &&
            ( mMapMoveOffsets[ i ].x != 0 ||
              mMapMoveOffsets[ i ].y != 0  ) ) {

            
            doublePair nullOffset = { 0, 0 };
                    

            doublePair delta = sub( nullOffset, 
                                    mMapMoveOffsets[ i ] );
                    
            double step = frameRateFactor * mMapMoveSpeeds[ i ] / 60.0;
                    
            if( length( delta ) < step ) {
                        
                mMapMoveOffsets[ i ].x = 0;
                mMapMoveOffsets[ i ].y = 0;
                mMapMoveSpeeds[ i ] = 0;
                
                if( mMapCurAnimType[ i ] != ground ) {
                        
                    mMapLastAnimType[ i ] = mMapCurAnimType[ i ];
                    mMapCurAnimType[ i ] = ground;
                    mMapLastAnimFade[ i ] = 1;
                    
                    mMapAnimationLastFrameCount[ i ] =
                        mMapAnimationFrameCount[ i ];
                    
                    mMapAnimationFrozenRotFrameCount[ i ] = 
                        mMapAnimationLastFrameCount[ i ];
                    
                    }
                }
            else {
                mMapMoveOffsets[ i ] =
                    add( mMapMoveOffsets[ i ],
                         mult( normalize( delta ), step ) );
                }
            }
        }

    // step extra moving objects
    for( int i=0; i<mMapExtraMovingObjects.size(); i++ ) {
        
        ExtraMapObject *o = mMapExtraMovingObjects.getElement( i );
        
        doublePair nullOffset = { 0, 0 };
                    

        doublePair delta = sub( nullOffset, o->moveOffset );
        
        double step = frameRateFactor * o->moveSpeed / 60.0;
                    
        if( length( delta ) < step ) {
            // reached dest
            
            endExtraObjectMove( i );
            i--;
            }
        else {
            o->moveOffset =
                add( o->moveOffset,
                     mult( normalize( delta ), step ) );
            }
        }

    if( mCurMouseOverID > 0 && ! mCurMouseOverSelf ) {
        mCurMouseOverFade += 0.2 * frameRateFactor;
        if( mCurMouseOverFade >= 1 ) {
            mCurMouseOverFade = 1.0;
            }
        }
    
    for( int i=0; i<mPrevMouseOverSpotFades.size(); i++ ) {
        float f = mPrevMouseOverSpotFades.getElementDirect( i );
        
        f -= 0.1 * frameRateFactor;
        
        
        if( f <= 0 ) {
            mPrevMouseOverSpotFades.deleteElement( i );
            mPrevMouseOverSpots.deleteElement( i );
            mPrevMouseOverSpotsBehind.deleteElement( i );
            i--;
            }
        else {
            *( mPrevMouseOverSpotFades.getElement( i ) ) = f;
            }
        }
    
    if( mCurMouseOverCell.x != -1 ) {
        
        LiveObject *ourObject = getOurLiveObject();

        // we're not mousing over any object or person
        if( mCurMouseOverID == 0 &&
            ! mCurMouseOverPerson &&
            // AND we're not moving AND we're holding something
            ourObject != NULL &&
            ! ourObject->inMotion &&
            ourObject->holdingID > 0 ) {
            // fade cell in
            mCurMouseOverCellFade += 
                mCurMouseOverCellFadeRate * frameRateFactor;
            
            if( mCurMouseOverCellFade >= 1 ) {
                mCurMouseOverCellFade = 1.0;
                }
            }
        else {
            // fade cell out
            mCurMouseOverCellFade -= 0.1 * frameRateFactor;
            if( mCurMouseOverCellFade < 0 ) {
                mCurMouseOverCellFade = 0;
                }
            }
        }
    
    for( int i=0; i<mPrevMouseOverCellFades.size(); i++ ) {
        float f = mPrevMouseOverCellFades.getElementDirect( i );
        
        f -= 0.1 * frameRateFactor;
        
        
        if( f <= 0 ) {
            mPrevMouseOverCellFades.deleteElement( i );
            mPrevMouseOverCells.deleteElement( i );
            i--;
            }
        else {
            *( mPrevMouseOverCellFades.getElement( i ) ) = f;
            }
        }

    for( int i=0; i<mPrevMouseClickCellFades.size(); i++ ) {
        float f = mPrevMouseClickCellFades.getElementDirect( i );
        
        f -= 0.02 * frameRateFactor;
        
        
        if( f <= 0 ) {
            mPrevMouseClickCellFades.deleteElement( i );
            mPrevMouseClickCells.deleteElement( i );
            i--;
            }
        else {
            *( mPrevMouseClickCellFades.getElement( i ) ) = f;
            }
        }
    
    if( ! equal( mNotePaperPosOffset, mNotePaperPosTargetOffset ) ) {
        doublePair delta = 
            sub( mNotePaperPosTargetOffset, mNotePaperPosOffset );
        
        double d = distance( mNotePaperPosTargetOffset, mNotePaperPosOffset );
        
        
        if( d <= 1 ) {
            mNotePaperPosOffset = mNotePaperPosTargetOffset;
            }
        else {
            int speed = frameRateFactor * 4;

            if( d < 8 ) {
                speed = lrint( frameRateFactor * d / 2 );
                }

            if( speed > d ) {
                speed = floor( d );
                }

            if( speed < 1 ) {
                speed = 1;
                }
            
            doublePair dir = normalize( delta );
            
            mNotePaperPosOffset = 
                add( mNotePaperPosOffset,
                     mult( dir, speed ) );
            }
        
        if( equal( mNotePaperPosTargetOffset, mNotePaperHideOffset ) ) {
            // fully hidden, clear erased stuff
            mLastKnownNoteLines.deallocateStringElements();
            mErasedNoteChars.deleteAll();
            mErasedNoteCharOffsets.deleteAll();
            mErasedNoteCharFades.deleteAll();
            }
        }

    LiveObject *ourObject = getOurLiveObject();

    if( ourObject != NULL ) {    
        char tooClose = false;
        double homeDist = 0;
        
        int homeArrow = getHomeDir( ourObject->currentPos, &homeDist,
                                    &tooClose );
        
        if( homeArrow != -1 && ! tooClose ) {
            mHomeSlipPosTargetOffset.y = mHomeSlipHideOffset.y + 68;
            
            if( homeDist > 1000 ) {
                mHomeSlipPosTargetOffset.y += 20;
                }
            }
        else {
            mHomeSlipPosTargetOffset.y = mHomeSlipHideOffset.y;
            }

        int cm = ourObject->currentMouseOverClothingIndex;
        if( cm != -1 ) {
            ourObject->clothingHighlightFades[ cm ] 
                += 0.2 * frameRateFactor;
            if( ourObject->clothingHighlightFades[ cm ] >= 1 ) {
                ourObject->clothingHighlightFades[ cm ] = 1.0;
                }
            }
        for( int c=0; c<NUM_CLOTHING_PIECES; c++ ) {
            if( c != cm ) {
                ourObject->clothingHighlightFades[ c ]
                    -= 0.1 * frameRateFactor;
                if( ourObject->clothingHighlightFades[ c ] < 0 ) {
                    ourObject->clothingHighlightFades[ c ] = 0;
                    }
                }
            }
        }

    // update yum slip positions
    for( int i=0; i<NUM_YUM_SLIPS; i++ ) {
        
        if( ! equal( mYumSlipPosOffset[i], mYumSlipPosTargetOffset[i] ) ) {
            doublePair delta = 
                sub( mYumSlipPosTargetOffset[i], mYumSlipPosOffset[i] );
            
            double d = 
                distance( mYumSlipPosTargetOffset[i], mYumSlipPosOffset[i] );
            
            
            if( d <= 1 ) {
                mYumSlipPosOffset[i] = mYumSlipPosTargetOffset[i];
                }
            else {
                int speed = frameRateFactor * 4;
                
                if( d < 8 ) {
                    speed = lrint( frameRateFactor * d / 2 );
                    }
                
                if( speed > d ) {
                    speed = floor( d );
                    }
                
                if( speed < 1 ) {
                    speed = 1;
                    }
                
                doublePair dir = normalize( delta );
                
                mYumSlipPosOffset[i] = 
                    add( mYumSlipPosOffset[i],
                         mult( dir, speed ) );
                }        
            }
        }

    // update home slip positions
    if( ! equal( mHomeSlipPosOffset, mHomeSlipPosTargetOffset ) ) {
        doublePair delta = 
            sub( mHomeSlipPosTargetOffset, mHomeSlipPosOffset );
        
        double d = distance( mHomeSlipPosTargetOffset, mHomeSlipPosOffset );
        
        
        if( d <= 1 ) {
            mHomeSlipPosOffset = mHomeSlipPosTargetOffset;
            }
        else {
            int speed = frameRateFactor * 4;

            if( d < 8 ) {
                speed = lrint( frameRateFactor * d / 2 );
                }

            if( speed > d ) {
                speed = floor( d );
                }

            if( speed < 1 ) {
                speed = 1;
                }
            
            doublePair dir = normalize( delta );
            
            mHomeSlipPosOffset = 
                add( mHomeSlipPosOffset,
                     mult( dir, speed ) );
            }        
        if( equal( mHomeSlipPosTargetOffset, mHomeSlipHideOffset ) ) {
            // fully hidden
            // clear all arrow states
            for( int i=0; i<NUM_HOME_ARROWS; i++ ) {
                mHomeArrowStates[i].solid = false;
                    mHomeArrowStates[i].fade = 0;
                }
            }
        }

    if( ourObject != NULL ) {
        char newTrigger = false;
        
        AnimType anim = stepLiveTriggers( &newTrigger );
        
        if( anim != endAnimType && newTrigger ) {
            addNewAnim( ourObject, anim );
            }
        else if( anim == endAnimType && ourObject->curAnim > endAnimType ) {
            // trigger is over, back to ground
            addNewAnim( ourObject, ground );
            }
        }

    if( ourObject != NULL && mNextHintObjectID != 0 &&
        getNumHints( mNextHintObjectID ) > 0 ) {
        
        if( mCurrentHintObjectID != mNextHintObjectID ||
            mCurrentHintIndex != mNextHintIndex ||
            mForceHintRefresh ) {
            
            mForceHintRefresh = false;

            int newLiveSheetIndex = 0;

            if( mLiveHintSheetIndex != -1 ) {
                mHintTargetOffset[mLiveHintSheetIndex] = mHintHideOffset[0];
                
                // save last hint sheet for filter
                newLiveSheetIndex = 
                    (mLiveHintSheetIndex + 1 ) % ( NUM_HINT_SHEETS - 1 );
                
                }
            
            mLiveHintSheetIndex = newLiveSheetIndex;
            
            int i = mLiveHintSheetIndex;

            mHintTargetOffset[i] = mHintHideOffset[0];
            mHintTargetOffset[i].y += 100;
            
            if( mLastHintFilterString != NULL ) {
                mHintTargetOffset[i].y += 30;
                }
            

            mHintMessageIndex[ i ] = mNextHintIndex;
            
            mCurrentHintObjectID = mNextHintObjectID;
            mCurrentHintIndex = mNextHintIndex;
            
            mHintBookmarks[ mCurrentHintObjectID ] = mCurrentHintIndex;

            mNumTotalHints[ i ] = 
                getNumHints( mCurrentHintObjectID );

            if( mHintMessage[ i ] != NULL ) {
                delete [] mHintMessage[ i ];
                }
            
            mHintMessage[ i ] = getHintMessage( mCurrentHintObjectID, 
                                                mHintMessageIndex[i] );
            


            mHintExtraOffset[ i ].x = - getLongestLine( mHintMessage[i] ) / gui_fov_scale_hud;
            }
        }
    else if( ourObject != NULL && mNextHintObjectID != 0 &&
             getNumHints( mNextHintObjectID ) == 0 ) {
        // holding something with no hints, hide last sheet
        int newLiveSheetIndex = 0;
        
        if( mLiveHintSheetIndex != -1 ) {
            mHintTargetOffset[mLiveHintSheetIndex] = mHintHideOffset[0];
            
            // save last hint sheet for filter
            newLiveSheetIndex = 
                (mLiveHintSheetIndex + 1 ) % ( NUM_HINT_SHEETS - 1 );
            
            }
        
        mLiveHintSheetIndex = newLiveSheetIndex;
        
        mCurrentHintObjectID = mNextHintObjectID;
        }

    int lastSheet = NUM_HINT_SHEETS - 1;
    if( mPendingFilterString != NULL &&
        ( mHintMessage[ lastSheet ] == NULL ||
          strcmp( mHintMessage[ lastSheet ], mPendingFilterString ) != 0 ) ) {
        
        mHintTargetOffset[ lastSheet ] = mHintHideOffset[ lastSheet ];
        
        if( equal( mHintPosOffset[ lastSheet ], 
                   mHintHideOffset[ lastSheet ] ) ) {
            
            mNumTotalHints[ lastSheet ] = 1;
            mHintMessageIndex[ lastSheet ] = 0;
            
            mHintTargetOffset[ lastSheet ].y += 53;

            if( mHintMessage[ lastSheet ] != NULL ) {
                delete [] mHintMessage[ lastSheet ];
                }
            
            mHintMessage[ lastSheet ] = 
                stringDuplicate( mPendingFilterString );

            double len = 
                handwritingFont->measureString( mHintMessage[ lastSheet ] );
            
            mHintExtraOffset[ lastSheet ].x = - len / gui_fov_scale_hud;
            }
        
        }
    else if( mPendingFilterString == NULL &&
             mHintMessage[ lastSheet ] != NULL ) {
        mHintTargetOffset[ lastSheet ] = mHintHideOffset[ lastSheet ];
        
        if( equal( mHintPosOffset[ lastSheet ], 
                   mHintHideOffset[ lastSheet ] ) ) {
            
            // done hiding
            delete [] mHintMessage[ lastSheet ];
            mHintMessage[ lastSheet ] = NULL;
            }
        }

    for( int i=0; i<NUM_HINT_SHEETS; i++ ) {
        
        if( ! equal( mHintPosOffset[i], mHintTargetOffset[i] ) ) {
            doublePair delta = 
                sub( mHintTargetOffset[i], mHintPosOffset[i] );
            
            double d = distance( mHintTargetOffset[i], mHintPosOffset[i] );
            
            
            if( d <= 1 ) {
                mHintPosOffset[i] = mHintTargetOffset[i];                
                }
            else {
                int speed = frameRateFactor * 4;
                
                if( d < 8 ) {
                    speed = lrint( frameRateFactor * d / 2 );
                    }
                
                if( speed > d ) {
                    speed = floor( d );
                    }
                
                if( speed < 1 ) {
                    speed = 1;
                    }
                
                doublePair dir = normalize( delta );
                
                mHintPosOffset[i] = 
                    add( mHintPosOffset[i],
                         mult( dir, speed ) );
                }
            
            }
        }

    // should new tutorial sheet be shown?
    if( ( mTutorialNumber > 0 || mGlobalMessageShowing ) && ourObject != NULL )
    {
        
        // search map for closest tutorial trigger

        double closeDist = 999999;
        int closestNumber = -1;

        char closestIsFinal = false;
        

        for( int y=0; y<mMapD; y++ ) {
        
            int worldY = y + mMapOffsetY - mMapD / 2;
            
            for( int x=0; x<mMapD; x++ ) {
                
                int worldX = x + mMapOffsetX - mMapD / 2;
                
                doublePair worldPos  = { (double)worldX, (double)worldY };
                
                double dist = distance( worldPos, ourObject->currentPos );
                
                if( dist < closeDist ) {
                    
                    int mapI = y * mMapD + x;
                    
                    int mapID = mMap[ mapI ];
                    
                    if( mapID > 0 ) {
                        
                        ObjectRecord *mapO = getObject( mapID );
                        
                        char *tutLoc = strstr( mapO->description, "tutorial" );
                        if( tutLoc != NULL ) {
                            
                            int tutPage = -1;
                            
                            sscanf( tutLoc, "tutorial %d", &tutPage );
                            

                            if( tutPage != -1 ) {
                                
                                closeDist = dist;
                                closestNumber = tutPage;
                                
                                if( strstr( mapO->description, "done" ) 
                                    != NULL ) {
                                    closestIsFinal = true;
                                    }
                                else {
                                    closestIsFinal = false;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        

        if( closeDist > 4 &&
            mLiveTutorialTriggerNumber != -1 ) {
            // don't set a new one unless we get close enough to it
            // OR we haven't even set the first one yet.
            closestNumber = -1;
            }
        

        
        if( closestNumber > -1 && 
            closestNumber != mLiveTutorialTriggerNumber ) {
            
            // different tutorial stone that what is showing
            
            if( closestIsFinal ) {
                // done with totorial for good, unless they request it
                SettingsManager::setSetting( "tutorialDone", 1 );
                }
            

            if( mLiveTutorialSheetIndex >= 0 ) {
                mTutorialTargetOffset[ mLiveTutorialSheetIndex ] =
                    mTutorialHideOffset[ mLiveTutorialSheetIndex ];
                }
            mLiveTutorialSheetIndex ++;
            
            if( mLiveTutorialSheetIndex >= NUM_HINT_SHEETS ) {
                mLiveTutorialSheetIndex -= NUM_HINT_SHEETS;
                }

            mLiveTutorialTriggerNumber = closestNumber;
            

            char *transString;

            if( mUsingSteam && mLiveTutorialTriggerNumber == 8 ) {
                transString = autoSprintf( "tutorial_%d_steam", 
                                           mLiveTutorialTriggerNumber );
                }
            else {    
                transString = autoSprintf( "tutorial_%d", 
                                           mLiveTutorialTriggerNumber );
                }
            
            mTutorialMessage[ mLiveTutorialSheetIndex ] = 
                translate( transString );


            mTutorialTargetOffset[ mLiveTutorialSheetIndex ] =
                mTutorialHideOffset[ mLiveTutorialSheetIndex ];
            
            mTutorialTargetOffset[ mLiveTutorialSheetIndex ].y -= 100;
            
            delete [] transString;


            double longestLine = getLongestLine( 
                (char*)( mTutorialMessage[ mLiveTutorialSheetIndex ] ) );
            
            int numLines;
            char **lines = split( mTutorialMessage[ mLiveTutorialSheetIndex ], 
                                  "#", &numLines );
                
            for( int l=0; l<numLines; l++ ) {
                double len = handwritingFont->measureString( lines[l] );
                
                if( len > longestLine ) {
                    longestLine = len;
                    }
                delete [] lines[l];
                }
            delete [] lines;

            mTutorialExtraOffset[ mLiveTutorialSheetIndex ].x = longestLine / gui_fov_scale_hud;
            }
	}

    // pos for tutorial sheets
    // don't start sliding first sheet until map loaded
    if( ( mTutorialNumber > 0 || mGlobalMessageShowing )
        && mDoneLoadingFirstObjectSet )
    for( int i=0; i<NUM_HINT_SHEETS; i++ )
    {
        
        if( ! equal( mTutorialPosOffset[i], mTutorialTargetOffset[i] ) ) {
            doublePair delta = 
                sub( mTutorialTargetOffset[i], mTutorialPosOffset[i] );
            
            double d = distance( mTutorialTargetOffset[i], 
                                 mTutorialPosOffset[i] );
            
            
            if( d <= 1 ) {
                mTutorialPosOffset[i] = mTutorialTargetOffset[i];
                }
            else {
                int speed = frameRateFactor * 4;
                
                if( d < 8 ) {
                    speed = lrint( frameRateFactor * d / 2 );
                    }
                
                if( speed > d ) {
                    speed = floor( d );
                    }
                
                if( speed < 1 ) {
                    speed = 1;
                    }
                
                doublePair dir = normalize( delta );
                
                mTutorialPosOffset[i] = 
                    add( mTutorialPosOffset[i],
                         mult( dir, speed ) );
                }
            
            if( equal( mTutorialTargetOffset[i], 
                       mTutorialHideOffset[i] ) ) {
                // fully hidden
                }
            else if( equal( mTutorialPosOffset[i],
                            mTutorialTargetOffset[i] ) ) {
                // fully visible, play chime
                double stereoPos = 0.25;
                
                if( i % 2 != 0 ) {
                    stereoPos = 0.75;
                    }
                
                if( mTutorialSound != NULL ) {
                        playSoundSprite( mTutorialSound, 
                                         0.18 * getSoundEffectsLoudness(), 
                                         stereoPos );
                    }
                }
            }
	}

    // pos for craving sheets
    // don't start sliding first sheet until map loaded
    if( mLiveCravingSheetIndex >= 0 && mDoneLoadingFirstObjectSet )
    for( int i=0; i<NUM_HINT_SHEETS; i++ )
    {
        
        if( ! equal( mCravingPosOffset[i], mCravingTargetOffset[i] ) ) {
            doublePair delta = 
                sub( mCravingTargetOffset[i], mCravingPosOffset[i] );
            
            double d = distance( mCravingTargetOffset[i], 
                                 mCravingPosOffset[i] );
            
            
            if( d <= 1 ) {
                mCravingPosOffset[i] = mCravingTargetOffset[i];
                }
            else {
                int speed = frameRateFactor * 4;
                
                if( d < 8 ) {
                    speed = lrint( frameRateFactor * d / 2 );
                    }
                
                if( speed > d ) {
                    speed = floor( d );
                    }
                
                if( speed < 1 ) {
                    speed = 1;
                    }
                
                doublePair dir = normalize( delta );
                
                mCravingPosOffset[i] = 
                    add( mCravingPosOffset[i],
                         mult( dir, speed ) );
                }
            }
	}

    char anySlipsMovingDown = false;
    for( int i=0; i<3; i++ )
    {
        if( i != mHungerSlipVisible &&
            ! equal( mHungerSlipPosOffset[i], mHungerSlipHideOffsets[i] ) ) {
            // visible when it shouldn't be
            mHungerSlipPosTargetOffset[i] = mHungerSlipHideOffsets[i];
            anySlipsMovingDown = true;
            }
	}
    
    if( !anySlipsMovingDown )
    {
        if( mHungerSlipVisible != -1 ) {
            // send one up
            mHungerSlipPosTargetOffset[ mHungerSlipVisible ] =
                mHungerSlipShowOffsets[ mHungerSlipVisible ];
            }
	}

    // move all toward their targets
    for( int i=0; i<3; i++ )
    {
        if( ! equal( mHungerSlipPosOffset[i], 
                     mHungerSlipPosTargetOffset[i] ) ) {
            
            doublePair delta = 
                sub( mHungerSlipPosTargetOffset[i], mHungerSlipPosOffset[i] );
        
            double d = distance( mHungerSlipPosTargetOffset[i], 
                                 mHungerSlipPosOffset[i] );
        
        
            if( d <= 1 ) {
                mHungerSlipPosOffset[i] = mHungerSlipPosTargetOffset[i];
                }
            else {
                int speed = frameRateFactor * 4;
                
                if( d < 8 ) {
                    speed = lrint( frameRateFactor * d / 2 );
                    }
                
                if( speed > d ) {
                    speed = floor( d );
                    }

                if( speed < 1 ) {
                    speed = 1;
                    }
                
                doublePair dir = normalize( delta );
                
                mHungerSlipPosOffset[i] = 
                    add( mHungerSlipPosOffset[i],
                         mult( dir, speed ) );
                }
            
            if( equal( mHungerSlipPosTargetOffset[i],
                       mHungerSlipHideOffsets[i] ) ) {
                // fully hidden    
                // reset wiggle time
                mHungerSlipWiggleTime[i] = 0;
                }
                
            }
        
        if( ! equal( mHungerSlipPosOffset[i],
                     mHungerSlipHideOffsets[i] ) ) {
            
            // advance wiggle time
            mHungerSlipWiggleTime[i] += 
                frameRateFactor * mHungerSlipWiggleSpeed[i];
            }
	}

    double curTime = game_getCurrentTime();

    // after 5 seconds of waiting, send PING
    if( playerActionPending && 
        ourObject != NULL && 
        curTime - lastServerMessageReceiveTime < 1 &&
        curTime - ourObject->pendingActionAnimationStartTime > 
        5 + largestPendingMessageTimeGap &&
        ! waitingForPong )
    {

        printf( "Been waiting for response to our action request "
                "from server for %.2f seconds, and last server message "
                "received %.2f sec ago, saw a message time gap along the way "
                "of %.2f, sending PING request to server as sanity check.\n",
                curTime - ourObject->pendingActionAnimationStartTime,
                curTime - lastServerMessageReceiveTime,
                largestPendingMessageTimeGap );

        waitingForPong = true;
        lastPingSent ++;
        char *pingMessage = autoSprintf( "PING 0 0 %d#", lastPingSent );
        
        sendToServerSocket( pingMessage );
        delete [] pingMessage;
	}
    else if( playerActionPending &&
             ourObject != NULL &&
             curTime - ourObject->pendingActionAnimationStartTime > 5 &&
             curTime - lastServerMessageReceiveTime > 10 &&
             ! waitingForPong )
    {
        // we're waiting for a response from the server, and
        // we haven't heard ANYTHING from the server in a long time
        // a full, two-way network connection break
        printf( "Been waiting for response to our action request "
                "from server for %.2f seconds, and last server message "
                "received %.2f sec ago.  Declaring connection broken.\n",
                curTime - ourObject->pendingActionAnimationStartTime,
                curTime - lastServerMessageReceiveTime );

        closeSocket( mServerSocket );
        mServerSocket = -1;
        
        if( mDeathReason != NULL ) {
            delete [] mDeathReason;
            }
        mDeathReason = stringDuplicate( translate( "reasonDisconnected" ) );
            
        handleOurDeath( true );
	}

    // after 10 seconds of waiting, if we HAVE received our PONG back
    if( playerActionPending && 
        ourObject != NULL && 
        curTime - lastServerMessageReceiveTime < 1 &&
        curTime - ourObject->pendingActionAnimationStartTime > 
        10 + largestPendingMessageTimeGap )
    {

        // been bouncing for five seconds with no answer from server
        // in the mean time, we have seen other messages arrive from server
        // (so any network outage is over)

        if( waitingForPong && lastPingSent == lastPongReceived )
        {
            
            // and got PONG response, so server is hearing us
            // this is a real bug


            printf( 
                "Been waiting for response to our action request "
                "from server for %.2f seconds, and last server message "
                "received %.2f sec ago, saw a message time gap along the way "
                "of %.2f, AND got PONG response to our PING, giving up\n",
                curTime - ourObject->pendingActionAnimationStartTime,
                curTime - lastServerMessageReceiveTime,
                largestPendingMessageTimeGap );

            sendBugReport( 1 );

            // end it
            ourObject->pendingActionAnimationProgress = 0;
            ourObject->pendingAction = false;
            
            playerActionPending = false;
            waitingForPong = false;
            playerActionTargetNotAdjacent = false;
            
            if( nextActionMessageToSend != NULL ) {
                delete [] nextActionMessageToSend;
                nextActionMessageToSend = NULL;
                }
            
            int goodX = ourObject->xServer;
            int goodY = ourObject->yServer;
            
            printf( "   Jumping back to last-known server position of %d,%d\n",
                    goodX, goodY );
            
            // jump to wherever server said we were before
            ourObject->inMotion = false;
            
            ourObject->moveTotalTime = 0;
            ourObject->currentSpeed = 0;
            ourObject->currentGridSpeed = 0;
            
            
            ourObject->currentPos.x = goodX;
            ourObject->currentPos.y = goodY;
            
            ourObject->xd = goodX;
            ourObject->yd = goodY;
            ourObject->destTruncated = false;
		}
        else
		{
            printf( 
                "Been waiting for response to our action request "
                "from server for %.2f seconds, and no response received "
                "for our PING.  Declaring connection broken.\n",
                curTime - ourObject->pendingActionAnimationStartTime );
            
            closeSocket( mServerSocket );
            mServerSocket = -1;

            if( mDeathReason != NULL ) {
                delete [] mDeathReason;
                }
            mDeathReason = stringDuplicate( translate( "reasonDisconnected" ) );
            
            handleOurDeath( true );
		}
	}
    
    
    if( serverSocketConnected && game_getCurrentTime() - timeLastMessageSent > 15 ) {
        // more than 15 seconds without client making a move
        // send KA to keep connection open
        sendToServerSocket( (char*)"KA 0 0#" );
        }
    
	if ( SettingsManager::getIntSetting( "keyboardActions", 1 ) ) movementStep();
	
	minitech::livingLifeStep();

    char *message = getNextServerMessage();
    while( message != NULL )
    {
        overheadServerBytesRead += 52;
        
        printf( "Got length %d message\n%s\n", 
                (int)strlen( message ), message );

        messageType type = getMessageType( message );
        
        if( mapPullMode && type != MAP_CHUNK )
        {
            // ignore it---map is a frozen snapshot in time
            // or as close as we can get to it
            type = UNKNOWN;
		}
        if( type == SHUTDOWN  || type == FORCED_SHUTDOWN )
        {
            closeSocket( mServerSocket );
            mServerSocket = -1;
            
            setWaiting( false );
            setSignal( "serverShutdown" );
            
            delete [] message;
            return;
		}
        else if( type == SERVER_FULL )
        {
            closeSocket( mServerSocket );
            mServerSocket = -1;
            
            setWaiting( false );
            setSignal( "serverFull" );
            
            delete [] message;
            return;
		}
        else if( type == GLOBAL_MESSAGE )
        {
            if( mTutorialNumber <= 0 ) {
                // not in tutorial
                // display this message
                
                char messageFromServer[200];
                sscanf( message, "MS\n%199s", messageFromServer );            
                
                char *upper = stringToUpperCase( messageFromServer );
                
                char found;

                char *lines = replaceAll( upper, "**", "##", &found );
                delete [] upper;
                
                char *spaces = replaceAll( lines, "_", " ", &found );
                
                delete [] lines;
                

                mGlobalMessageShowing = true;
                mGlobalMessageStartTime = game_getCurrentTime();
                
                if( mLiveTutorialSheetIndex >= 0 ) {
                    mTutorialTargetOffset[ mLiveTutorialSheetIndex ] =
                    mTutorialHideOffset[ mLiveTutorialSheetIndex ];
                    }
                mLiveTutorialSheetIndex ++;
                
                if( mLiveTutorialSheetIndex >= NUM_HINT_SHEETS ) {
                    mLiveTutorialSheetIndex -= NUM_HINT_SHEETS;
                    }
                mTutorialMessage[ mLiveTutorialSheetIndex ] = 
                    stringDuplicate( spaces );
                
                // other tutorial messages don't need to be destroyed
                mGlobalMessagesToDestroy.push_back( 
                    (char*)( mTutorialMessage[ mLiveTutorialSheetIndex ] ) );

                mTutorialTargetOffset[ mLiveTutorialSheetIndex ] =
                    mTutorialHideOffset[ mLiveTutorialSheetIndex ];
                
                mTutorialTargetOffset[ mLiveTutorialSheetIndex ].y -= 100;

                double longestLine = getLongestLine( 
                    (char*)( mTutorialMessage[ mLiveTutorialSheetIndex ] ) );
            
                mTutorialExtraOffset[ mLiveTutorialSheetIndex ].x = longestLine;

                
                delete [] spaces;
                }
		}
        else if( type == FLIP )
        {
            int numLines;
            char **lines = split( message, "\n", &numLines );

            if( numLines > 0 ) {
                delete [] lines[0];
                }
            
            for( int i=1; i<numLines; i++ ) {
                int id = 0;
                int facingLeft = 0;
                
                int numRead = 
                    sscanf( lines[i], "%d %d", &id, &facingLeft );
            
                if( numRead == 2 ) {
                    LiveObject *o = getLiveObject( id );
                    
                    if( o != NULL && ! o->inMotion ) {
                        char flip = false;
                        
                        if( facingLeft && ! o->holdingFlip ) {
                            o->holdingFlip = true;
                            flip = true;
                            }
                        else if( ! facingLeft && o->holdingFlip ) {
                            o->holdingFlip = false;
                            flip = true;
                            }
                        if( flip ) {
                            o->lastAnim = moving;
                            o->curAnim = ground2;
                            o->lastAnimFade = 1;

                            o->lastHeldAnim = moving;
                            o->curHeldAnim = held;
                            o->lastHeldAnimFade = 1;
                            }
                        }
                    }
                delete [] lines[i];
                }
            
            delete [] lines;
		}
        else if( type == CRAVING )
        {
            int foodID = -1;
            int bonus = 0;
            
            int numRead = 
                sscanf( message, "CR\n%d %d", &foodID, &bonus );
            
            if( numRead == 2 ) {
                setNewCraving( foodID, bonus );
                }
		}
        else if( type == SEQUENCE_NUMBER )
        {
			#include "GameScreen_SequenceNumber.cpp"
		}
        else if( type == ACCEPTED )
        {
            // logged in successfully, wait for next message
            
            // subsequent messages should all be part of FRAME batches
            waitForFrameMessages = true;

            SettingsManager::setSetting( "loginSuccess", 1 );

            delete [] message;
            return;
		}
        else if( type == REJECTED )
        {
            closeSocket( mServerSocket );
            mServerSocket = -1;
            
            setWaiting( false );
            setSignal( "loginFailed" );
            
            delete [] message;
            return;
		}
        else if( type == NO_LIFE_TOKENS )
        {
            closeSocket( mServerSocket );
            mServerSocket = -1;
            
            setWaiting( false );
            setSignal( "noLifeTokens" );
            
            delete [] message;
            return;
		}
        else if( type == APOCALYPSE )
        {
            apocalypseDisplayProgress = 0;
            apocalypseInProgress = true;
		}
        else if( type == APOCALYPSE_DONE )
        {
            apocalypseDisplayProgress = 0;
            apocalypseInProgress = false;
            homePosStack.deleteAll();
            
            // cancel all emots
            for( int i=0; i<gameObjects.size(); i++ ) {
                LiveObject *p = gameObjects.getElement( i );
                p->currentEmot = NULL;
                }
		}
        else if( type == MONUMENT_CALL )
        {
            int posX, posY, monumentID;
            
            int numRead = sscanf( message, "MN\n%d %d %d",
                                  &posX, &posY, &monumentID );
            if( numRead == 3 ) {
                applyReceiveOffset( &posX, &posY );

                doublePair pos;
                pos.x = posX;
                pos.y = posY;
                
                LiveObject *ourLiveObject = getOurLiveObject();

                if( ourLiveObject != NULL ) {
                    double d = distance( pos, ourLiveObject->currentPos );
                    
                    if( d > 32 ) {
                        addAncientHomeLocation( posX, posY );
                        
                        // play sound in distance
                        ObjectRecord *monObj = getObject( monumentID );
                        
                        if( monObj != NULL && 
                            monObj->creationSound.numSubSounds > 0 ) {    
                             
                            doublePair realVector = 
                                getVectorFromCamera( lrint( posX ),
                                                     lrint( posY ) );
                            // position off camera in that direction
                            // but fake distance
                            realVector = mult( normalize( realVector ), 4 );
                            
                            playSound( monObj->creationSound, realVector );
                            }
                        }
                    }
                }            
		}
        else if( type == GRAVE )
        {
            int posX, posY, playerID;
            
            int numRead = sscanf( message, "GV\n%d %d %d",
                                  &posX, &posY, &playerID );
            if( numRead == 3 ) {
                applyReceiveOffset( &posX, &posY );
                
                LiveObject *gravePerson = getLiveObject( playerID );
                
                if( gravePerson != NULL && 
                    ( gravePerson->relationName || 
                      gravePerson->name != NULL ) ) {
                    
                    GraveInfo g;
                    g.worldPos.x = posX;
                    g.worldPos.y = posY;
                    g.creationTime = game_getCurrentTime();
                    g.creationTimeUnknown = false;
                    g.lastMouseOverYears = -1;
                    g.lastMouseOverTime = g.creationTime;
                    
                    char *des = gravePerson->relationName;
                    char *desToDelete = NULL;
                    
                    if( des == NULL ) {
                        des = (char*)translate( "unrelated" );

                        if( gravePerson->name == NULL ) {
                            // call them nameless instead
                            des = (char*)translate( "namelessPerson" );
                            }
                        }
                    if( gravePerson->name != NULL ) {
                        des = autoSprintf( "%s - %s",
                                           gravePerson->name, des );
                        desToDelete = des;
                        }

                    g.relationName = stringDuplicate( des );
                    
                    if( desToDelete != NULL ) {
                        delete [] desToDelete;
                        }
                    
                    // this grave replaces any in same location
                    for( int i=0; i< mGraveInfo.size(); i++ ) {
                        GraveInfo *otherG = mGraveInfo.getElement( i );
                        
                        if( otherG->worldPos.x == posX &&
                            otherG->worldPos.y == posY ) {
                            
                            delete [] otherG->relationName;
                            mGraveInfo.deleteElement( i );
                            i--;
                            }
                        }
                    

                    mGraveInfo.push_back( g );
                    }
                }            
		}
        else if( type == GRAVE_MOVE )
        {
            int posX, posY, posXNew, posYNew;

            int swapDest = 0;
            
            int numRead = sscanf( message, "GM\n%d %d %d %d %d",
                                  &posX, &posY, &posXNew, &posYNew,
                                  &swapDest );
            if( numRead == 4 || numRead == 5 ) {
                applyReceiveOffset( &posX, &posY );
                applyReceiveOffset( &posXNew, &posYNew );


                // handle case where two graves are "in the air"
                // at the same time, and one gets put down where the
                // other was picked up from before the other gets put down.
                // (graves swapping position)

                // When showing label to player, these are walked through
                // in order until first match found

                // So, we should walk through in reverse order until
                // LAST match found, and then move that one to the top
                // of the list.

                // it will "cover up" the label of the still-matching
                // grave further down on the list, which we will find
                // and fix later when it fininall finishes moving.
                char found = false;
                for( int i=mGraveInfo.size() - 1; i >= 0; i-- ) {
                    GraveInfo *g = mGraveInfo.getElement( i );
                    
                    if( g->worldPos.x == posX &&
                        g->worldPos.y == posY ) {
                        
                        g->worldPos.x = posXNew;
                        g->worldPos.y = posYNew;
                        
                        GraveInfo gStruct = *g;
                        mGraveInfo.deleteElement( i );
                        mGraveInfo.push_front( gStruct );
                        found = true;
                        break;
                        }    
                    }
                
                if( found && ! swapDest ) {
                    // do NOT need to keep any extra ones around
                    // this fixes cases where old grave info is left
                    // behind, due to decay
                    for( int i=1; i < mGraveInfo.size(); i++ ) {
                        GraveInfo *g = mGraveInfo.getElement( i );
                        
                        if( g->worldPos.x == posXNew &&
                            g->worldPos.y == posYNew ) {
                            
                            // a stale match
                            mGraveInfo.deleteElement( i );
                            i--;
                            }
                        }
                    }
                }            
		}
        else if( type == GRAVE_OLD )
        {
			#include "GameScreen_GraveOldMessage.cpp"
        }
        else if( type == OWNER )
        {
            SimpleVector<char*> *tokens = tokenizeString( message );
            
            if( tokens->size() >= 3 ) {
                int x = 0;
                int y = 0;
                
                sscanf( tokens->getElementDirect( 1 ), "%d", &x );
                sscanf( tokens->getElementDirect( 2 ), "%d", &y );
                
                GridPos thisPos = { x, y };
                
                for( int i=0; i<ownerRequestPos.size(); i++ ) {
                    if( equal( ownerRequestPos.getElementDirect( i ),
                               thisPos ) ) {
                        ownerRequestPos.deleteElement( i );
                        break;
                        }
                    }

                OwnerInfo *o = NULL;
                
                for( int i=0; i<mOwnerInfo.size(); i++ ) {
                    OwnerInfo *thisInfo = mOwnerInfo.getElement( i );
                    if( thisInfo->worldPos.x == x &&
                        thisInfo->worldPos.y == y ) {
                        
                        o = thisInfo;
                        break;
                        }    
                    }
                if( o == NULL ) {
                    // not found, create new
                    GridPos p = { x, y };
                    
                    OwnerInfo newO = { p, new SimpleVector<int>() };
                    
                    mOwnerInfo.push_back( newO );
                    o = mOwnerInfo.getElement( mOwnerInfo.size() - 1 );
                    }

                o->ownerList->deleteAll();
                for( int t=3; t < tokens->size(); t++ ) {
                    int ownerID = 0;
                    sscanf( tokens->getElementDirect( t ), "%d", &ownerID );
                    if( ownerID > 0 ) {
                        o->ownerList->push_back( ownerID );
                        }
                    }
                }
            tokens->deallocateStringElements();
            delete tokens;
		}
        else if( type == VALLEY_SPACING )
        {
            sscanf( message, "VS\n%d %d",
                    &valleySpacing, &valleyOffset );
		}
        else if( type == FLIGHT_DEST )
        {
            int posX, posY, playerID;
            
            int numRead = sscanf( message, "FD\n%d %d %d",
                                  &playerID, &posX, &posY );
            if( numRead == 3 ) {
                applyReceiveOffset( &posX, &posY );
                
                LiveObject *flyingPerson = getLiveObject( playerID );
                
                if( flyingPerson != NULL ) {
                    // move them there instantly
                    flyingPerson->xd = posX;
                    flyingPerson->yd = posY;
                    
                    flyingPerson->xServer = posX;
                    flyingPerson->yServer = posY;
                    
                    flyingPerson->currentPos.x = posX;
                    flyingPerson->currentPos.y = posY;
                    
                    flyingPerson->currentSpeed = 0;
                    flyingPerson->currentGridSpeed = 0;
                    flyingPerson->destTruncated = false;
                    
                    flyingPerson->currentMoveDirection.x = 0;
                    flyingPerson->currentMoveDirection.y = 0;
                    
                    if( flyingPerson->pathToDest != NULL ) {
                        delete [] flyingPerson->pathToDest;
                        flyingPerson->pathToDest = NULL;
                        }

                    flyingPerson->inMotion = false;
                        

                    if( flyingPerson->id == ourID ) {
                        // special case for self
                        
                        // jump camera there instantly
                        lastScreenViewCenter.x = posX * CELL_D;
                        lastScreenViewCenter.y = posY * CELL_D;
                        setViewCenterPosition( lastScreenViewCenter.x,
                                               lastScreenViewCenter.y );
                        
                        // show loading screen again
                        mFirstServerMessagesReceived = 2;
                        mStartedLoadingFirstObjectSet = false;
                        mDoneLoadingFirstObjectSet = false;
                        mFirstObjectSetLoadingProgress = 0;
                        mPlayerInFlight = true;

                        // home markers invalid now
                        homePosStack.deleteAll();
                        }
                    }
                }            
		}
        else if( type == VOG_UPDATE )
        {
            int posX, posY;
            
            int numRead = sscanf( message, "VU\n%d %d",
                                  &posX, &posY );
            if( numRead == 2 ) {
                vogPos.x = posX;
                vogPos.y = posY;

                mObjectPicker.setPosition( vogPos.x * CELL_D + 510,
                                           vogPos.y * CELL_D + 90 );

                // jump camp instantly
                lastScreenViewCenter.x = posX * CELL_D;
                lastScreenViewCenter.y = posY * CELL_D;
                setViewCenterPosition( lastScreenViewCenter.x,
                                       lastScreenViewCenter.y );

                mCurMouseOverCellFade = 1.0;
                
                mCurMouseOverCell.x = vogPos.x - mMapOffsetX + mMapD / 2;
                mCurMouseOverCell.y = vogPos.y - mMapOffsetY + mMapD / 2;
                }
		}
        else if( type == PHOTO_SIGNATURE )
        {
            int posX, posY;

            char sig[100];
            
            int numRead = sscanf( message, "PH\n%d %d %99s",
                                  &posX, &posY, sig );
            if( numRead == 3 ) {
                photoSig = stringDuplicate( sig );
                }
            else {
                photoSig = stringDuplicate( "NO_SIG" );
                }
		}
        else if( type == MAP_CHUNK )
        {
			#include "GameScreen_MapChunkMessage.cpp"
		}
        else if( type == MAP_CHANGE )
        {
			#include "GameScreen_MapChangeMessage.cpp"
		}
        else if( type == PLAYER_UPDATE )
        {
			#include "GameScreen_PlayerUpdateMessage.cpp"
		}
        else if( type == PLAYER_MOVES_START )
        {
			#include "GameScreen_PlayerMovesStartMessage.cpp"
		}
        else if( type == PLAYER_SAYS )
        {
            int numLines;
            char **lines = split( message, "\n", &numLines );
            
            if( numLines > 0 ) {
                // skip first
                delete [] lines[0];
                }
            
            
            for( int i=1; i<numLines; i++ ) {

                int id = -1;
                int curseFlag = 0;

                int numRead = 0;
                
                if( strstr( lines[i], "/" ) != NULL ) {
                    // new id/curse_flag format
                    numRead = sscanf( lines[i], "%d/%d ", &id, &curseFlag );
                    }
                else {
                    // old straight ID format
                    numRead = sscanf( lines[i], "%d ", &id );
                    }
                
                if( numRead >= 1 ) {
                    for( int j=0; j<gameObjects.size(); j++ ) {
                        if( gameObjects.getElement(j)->id == id ) {
                            
                            LiveObject *existing = gameObjects.getElement(j);
                            
                            if( existing->currentSpeech != NULL ) {
                                delete [] existing->currentSpeech;
                                existing->currentSpeech = NULL;
                                }
                            
                            char *firstSpace = strstr( lines[i], " " );
        
                            if( firstSpace != NULL ) {
                                existing->currentSpeech = 
                                    stringDuplicate( &( firstSpace[1] ) );
                                
                                existing->speechFade = 1.0;
                                
                                existing->speechIsSuccessfulCurse = curseFlag;

                                // longer time for longer speech
                                existing->speechFadeETATime = 
                                    game_getCurrentTime() + 3 +
                                    strlen( existing->currentSpeech ) / 5;

                                if( existing->age < 1 && 
                                    existing->heldByAdultID == -1 ) {
                                    // make 0-y-old unheld baby revert to 
                                    // crying age every time they speak
                                    existing->tempAgeOverrideSet = true;
                                    existing->tempAgeOverride = 0;
                                    existing->tempAgeOverrideSetTime = 
                                        game_getCurrentTime();
                                    }
                                
                                if( curseFlag && mCurseSound != NULL ) {
                                    playSound( 
                                        mCurseSound,
                                        0.5, // a little loud, tweak it
                                        getVectorFromCamera( 
                                            existing->currentPos.x, 
                                            existing->currentPos.y ) );
                                    }
                                }
                            
                            break;
                            }
                        }
                    
                    }
                
                delete [] lines[i];
                }
            delete [] lines;
		}
        else if( type == LOCATION_SAYS )
        {
            int numLines;
            char **lines = split( message, "\n", &numLines );
            
            if( numLines > 0 ) {
                // skip first
                delete [] lines[0];
                }
            
            for( int i=1; i<numLines; i++ ) {
                int x = 0;
                int y = 0;
                
                int numRead = sscanf( lines[i], "%d %d", &x, &y );
                
                
                if( numRead == 2 ) {
                    
                    char *firstSpace = strstr( lines[i], " " );

                    if( firstSpace != NULL ) {
                        char *secondSpace = strstr( &( firstSpace[1] ), " " );
                        

                        if( secondSpace != NULL ) {
                            
                            char *speech = &( secondSpace[1] );
                            
                            LocationSpeech ls;
                            
                            ls.pos.x = x * CELL_D;
                            ls.pos.y = y * CELL_D;
                            
                            ls.speech = stringDuplicate( speech );
                            
                            ls.fade = 1.0;
                            
                            // longer time for longer speech
                            ls.fadeETATime = 
                                game_getCurrentTime() + 3 +
                                strlen( ls.speech ) / 5;

                            locationSpeech.push_back( ls );
                            }
                        }
                    }
                
                delete [] lines[i];
                }
            delete [] lines;
		}
        else if( type == PLAYER_EMOT )
        {
            int numLines;
            char **lines = split( message, "\n", &numLines );
            
            if( numLines > 0 ) {
                // skip first
                delete [] lines[0];
                }            
            
            for( int i=1; i<numLines; i++ ) {
                int pid, emotIndex;
                int ttlSec = 0;
                
                int numRead = sscanf( lines[i], "%d %d %d",
                                      &pid, &emotIndex, &ttlSec );

                if( numRead >= 2 ) {
                    for( int j=0; j<gameObjects.size(); j++ ) {
                        if( gameObjects.getElement(j)->id == pid ) {
                                
                            LiveObject *existing = gameObjects.getElement(j);
                            Emotion *newEmotPlaySound = NULL;
                            
                            if( ttlSec < 0 ) {
                                // new permanent emot layer
                                newEmotPlaySound = getEmotion( emotIndex );

                                if( newEmotPlaySound != NULL ) {
                                    if( existing->permanentEmots.
                                        getElementIndex( 
                                            newEmotPlaySound ) == -1 ) {
                                    
                                        existing->permanentEmots.push_back(
                                            newEmotPlaySound );
                                        }
                                    }
                                if( ttlSec == -2 ) {
                                    // old emot that we're just learning about
                                    // skip sound
                                    newEmotPlaySound = NULL;
                                    }
                                }
                            else {
                                
                                Emotion *oldEmot = existing->currentEmot;
                            
                                existing->currentEmot = getEmotion( emotIndex );
                            
                                if( numRead == 3 && ttlSec > 0 ) {
                                    existing->emotClearETATime = 
                                        game_getCurrentTime() + ttlSec;
                                    }
                                else {
                                    // no ttl provided by server, use default
                                    existing->emotClearETATime = 
                                        game_getCurrentTime() + emotDuration;
                                    }
                                
                                if( oldEmot != existing->currentEmot &&
                                    existing->currentEmot != NULL ) {
                                    newEmotPlaySound = existing->currentEmot;
                                    }
                                }
                            
                            if( newEmotPlaySound != NULL ) {
                                doublePair playerPos = existing->currentPos;
                                
                                for( int i=0; 
                                     i<getEmotionNumObjectSlots(); i++ ) {
                                    
                                    int id =
                                        getEmotionObjectByIndex(
                                            newEmotPlaySound, i );
                                    
                                    if( id > 0 ) {
                                        ObjectRecord *obj = getObject( id );
                                        
                                        if( obj->creationSound.numSubSounds 
                                            > 0 ) {    
                                    
                                            playSound( 
                                                obj->creationSound,
                                                getVectorFromCamera( 
                                                    playerPos.x,
                                                    playerPos.y ) );
                                            // stop after first sound played
                                            break;
                                            }
                                        }
                                    }
                                }
                            // found matching player, done
                            break;
                            }
                        }
                    }
                delete [] lines[i];
                }
            delete [] lines;
		}
        else if( type == LINEAGE )
        {
            int numLines;
            char **lines = split( message, "\n", &numLines );
            
            if( numLines > 0 ) {
                // skip first
                delete [] lines[0];
                }
            
            
            for( int i=1; i<numLines; i++ ) {

                int id;
                int numRead = sscanf( lines[i], "%d ",
                                      &( id ) );

                if( numRead == 1 ) {
                    for( int j=0; j<gameObjects.size(); j++ ) {
                        if( gameObjects.getElement(j)->id == id ) {
                            
                            LiveObject *existing = gameObjects.getElement(j);
                            
                            existing->lineage.deleteAll();

                            char *firstSpace = strstr( lines[i], " " );
        
                            if( firstSpace != NULL ) {

                                char *linStart = &( firstSpace[1] );
                                
                                SimpleVector<char *> *tokens = 
                                    tokenizeString( linStart );

                                int numNormalTokens = tokens->size();
                                
                                if( tokens->size() > 0 ) {
                                    char *lastToken =
                                        tokens->getElementDirect( 
                                            tokens->size() - 1 );
                                    
                                    if( strstr( lastToken, "eve=" ) ) {   
                                        // eve tag at end
                                        numNormalTokens--;

                                        sscanf( lastToken, "eve=%d",
                                                &( existing->lineageEveID ) );
                                        }
                                    }

                                for( int t=0; t<numNormalTokens; t++ ) {
                                    char *tok = tokens->getElementDirect( t );
                                    
                                    int mID = 0;
                                    sscanf( tok, "%d", &mID );
                                    
                                    if( mID != 0 ) {
                                        existing->lineage.push_back( mID );
                                        }
                                    }
                                

                                
                                tokens->deallocateStringElements();
                                delete tokens;
                                }
                            
                            break;
                            }
                        }
                    
                    }
                
                delete [] lines[i];
                }
            delete [] lines;

            // after a LINEAGE message, we should have lineage for all
            // players
            
            // now process each one and generate relation string
            LiveObject *ourObject = getOurLiveObject();
            
            for( int j=0; j<gameObjects.size(); j++ ) {
                if( gameObjects.getElement(j)->id != ourID ) {
                            
                    LiveObject *other = gameObjects.getElement(j);
                 
                    if( other->relationName == NULL ) {
                        
                        /*
                        // test
                        ourObject->lineage.deleteAll();
                        other->lineage.deleteAll();
                        
                        int cousinNum = 25;
                        int removeNum = 5;
                        
                        for( int i=0; i<=cousinNum; i++ ) {    
                            ourObject->lineage.push_back( i );
                            }

                        for( int i=0; i<=cousinNum - removeNum; i++ ) {    
                            other->lineage.push_back( 100 + i );
                            }
                        other->lineage.push_back( cousinNum );
                        */
                        other->relationName = getRelationName( ourObject,
                                                               other );
                        }
                    }
                }
		}
        else if( type == CURSED )
        {
            int numLines;
            char **lines = split( message, "\n", &numLines );
            
            if( numLines > 0 ) {
                // skip first
                delete [] lines[0];
                }
            
            
            for( int i=1; i<numLines; i++ ) {

                int id, level;
                int numRead = sscanf( lines[i], "%d %d",
                                      &id, &level );

                if( numRead == 2 ) {
                    for( int j=0; j<gameObjects.size(); j++ ) {
                        if( gameObjects.getElement(j)->id == id ) {
                            
                            LiveObject *existing = gameObjects.getElement(j);
                            
                            existing->curseLevel = level;
                            break;
                            }
                        }
                    
                    }
                
                delete [] lines[i];
                }
            delete [] lines;
		}
        else if( type == CURSE_TOKEN_CHANGE )
        {
            LiveObject *ourLiveObject = getOurLiveObject();
            
            if( ourLiveObject != NULL ) {
                
                sscanf( message, "CX\n%d", 
                        &( ourLiveObject->curseTokenCount ) );
                }
		}
        else if( type == CURSE_SCORE )
        {
            LiveObject *ourLiveObject = getOurLiveObject();
            
            if( ourLiveObject != NULL ) {
                
                sscanf( message, "CS\n%d", 
                        &( ourLiveObject->excessCursePoints ) );
                }
		}
        else if( type == PONG )
        {
            sscanf( message, "PONG\n%d", 
                    &( lastPongReceived ) );
            if( lastPongReceived == lastPingSent ) {
                pongDeltaTime = game_getCurrentTime() - pingSentTime;
                }
		}
        else if( type == NAMES )
        {
            int numLines;
            char **lines = split( message, "\n", &numLines );
            
            if( numLines > 0 ) {
                // skip first
                delete [] lines[0];
                }
            
            
            for( int i=1; i<numLines; i++ ) {

                int id;
                int numRead = sscanf( lines[i], "%d ",
                                      &( id ) );

                if( numRead == 1 ) {
                    for( int j=0; j<gameObjects.size(); j++ ) {
                        if( gameObjects.getElement(j)->id == id ) {
                            
                            LiveObject *existing = gameObjects.getElement(j);
                            
                            if( existing->name != NULL ) {
                                delete [] existing->name;
                                }
                            
                            char *firstSpace = strstr( lines[i], " " );
        
                            if( firstSpace != NULL ) {

                                char *nameStart = &( firstSpace[1] );
                                
                                existing->name = stringDuplicate( nameStart );
								
								LiveObject *ourLiveObject = getOurLiveObject();
								if ( id == ourLiveObject->id && 
									//Little hack here to not have the ding
									//when we are just reconnected
									//instead of a real name change
									ourLiveObject->foodCapacity > 0 && 
									mTutorialSound != NULL ) {
									playSound( 
										mTutorialSound,
										0.18 * getSoundEffectsLoudness(), 
										getVectorFromCamera( 
											ourLiveObject->currentPos.x, 
											ourLiveObject->currentPos.y ) );
									}
                                }
                            
                            break;
                            }
                        }
                    
                    }
                
                delete [] lines[i];
                }
            delete [] lines;
		}
        else if( type == DYING )
        {
            int numLines;
            char **lines = split( message, "\n", &numLines );
            
            if( numLines > 0 ) {
                // skip first
                delete [] lines[0];
                }
            
            
            for( int i=1; i<numLines; i++ ) {

                int id;
                int sickFlag = 0;
                
                int numRead = sscanf( lines[i], "%d %d",
                                      &( id ), &sickFlag );

                if( numRead >= 1 ) {
                    for( int j=0; j<gameObjects.size(); j++ ) {
                        if( gameObjects.getElement(j)->id == id ) {
                            
                            LiveObject *existing = gameObjects.getElement(j);
                            
                            existing->dying = true;
                            if( sickFlag ) {
                                existing->sick = true;
                                }
                            break;
                            }
                        }
                    }
                delete [] lines[i];
                }
            delete [] lines;
		}
        else if( type == HEALED )
        {
            int numLines;
            char **lines = split( message, "\n", &numLines );
            
            if( numLines > 0 ) {
                // skip first
                delete [] lines[0];
                }
            
            
            for( int i=1; i<numLines; i++ ) {

                int id;
                int numRead = sscanf( lines[i], "%d ",
                                      &( id ) );

                if( numRead == 1 ) {
                    for( int j=0; j<gameObjects.size(); j++ ) {
                        if( gameObjects.getElement(j)->id == id ) {
                            
                            LiveObject *existing = gameObjects.getElement(j);
                            
                            existing->dying = false;
                            existing->sick = false;
                            
                            // their wound will be gone after this
                            // play decay sound, if any, for their final
                            // wound state
                            if( existing->holdingID > 0 ) {
                                ObjectRecord *held = 
                                    getObject( existing->holdingID );
                                
                                if( held->decaySound.numSubSounds > 0 ) {    
                                    
                                    playSound( 
                                        held->decaySound,
                                        getVectorFromCamera( 
                                            existing->currentPos.x, 
                                            existing->currentPos.y ) );
                                    }
                                }
                            break;
                            }
                        }
                    }
                delete [] lines[i];
                }
            delete [] lines;
		}
        else if( type == PLAYER_OUT_OF_RANGE )
        {
            int numLines;
            char **lines = split( message, "\n", &numLines );
            
            if( numLines > 0 ) {
                // skip first
                delete [] lines[0];
                }
            
            
            for( int i=1; i<numLines; i++ ) {

                int id;
                int numRead = sscanf( lines[i], "%d ",
                                      &( id ) );

                if( numRead == 1 ) {
                    for( int j=0; j<gameObjects.size(); j++ ) {
                        if( gameObjects.getElement(j)->id == id ) {
                            
                            LiveObject *existing = gameObjects.getElement(j);
                            
                            existing->outOfRange = true;
                            
                            if( existing->pendingReceivedMessages.size() > 0 ) {
                                // don't let pending messages for out-of-range
                                // players linger
                                playPendingReceivedMessages( existing );
                                }

                            break;
                            }
                        }
                    }
                delete [] lines[i];
                }
            delete [] lines;
		}
        else if( type == FOOD_CHANGE )
        {
			#include "GameScreen_FoodChangeMessage.cpp"
		}
        else if( type == HEAT_CHANGE )
        {
            LiveObject *ourLiveObject = getOurLiveObject();
            if( ourLiveObject != NULL )
            {
                sscanf( message, "HX\n%f", &( ourLiveObject->heat ) );
			}
		}
        delete [] message;

        // process next message if there is one
        message = getNextServerMessage();
	}

    if( mapPullMode && mapPullCurrentSaved && ! mapPullCurrentSent )
    {
        char *message = autoSprintf( "MAP %d %d#",
                                     sendX( mapPullCurrentX ),
                                     sendY( mapPullCurrentY ) );
        sendToServerSocket( message );
        delete [] message;
        mapPullCurrentSent = true;
	}

    // check if we're about to move off the screen
    LiveObject *ourLiveObject = getOurLiveObject();
    
    if( !mapPullMode && mDoneLoadingFirstObjectSet && ourLiveObject != NULL )
    {
        

        // current age
        double age = computeCurrentAge( ourLiveObject );

        int sayCap = (int)( floor( age ) + 1 );
        
        if( ourLiveObject->lineage.size() == 0  && sayCap < 30 ) {
            // eve has a larger say limit
            sayCap = 30;
            }
        if( vogMode ) {
            sayCap = 200;
            }
        
        char *currentText = mSayField.getText();
        
        if( strlen( currentText ) > 0 && currentText[0] == '/' ) {
            // typing a filter
            // hard cap at 25, regardless of age
            // don't want them typing long filters that overflow the display
            sayCap = 23;
            }
        delete [] currentText;

        mSayField.setMaxLength( sayCap );



        LiveObject *cameraFollowsObject = ourLiveObject;
        
        if( ourLiveObject->heldByAdultID != -1 ) {
            cameraFollowsObject = 
                getGameObject( ourLiveObject->heldByAdultID );
            
            if( cameraFollowsObject == NULL ) {
                ourLiveObject->heldByAdultID = -1;
                cameraFollowsObject = ourLiveObject;
                }
            }
        
        doublePair targetObjectPos = cameraFollowsObject->currentPos;
        
        if( vogMode ) {
            targetObjectPos = vogPos;
            }
        


        doublePair screenTargetPos = 
            mult( targetObjectPos, CELL_D );
        
        if( vogMode || SettingsManager::getIntSetting( "centerCamera", 0 ) ) {
            // don't adjust camera
            }
        else if( 
            cameraFollowsObject->currentPos.x != cameraFollowsObject->xd
            ||
            cameraFollowsObject->currentPos.y != cameraFollowsObject->yd ) {
            
            // moving

            // push camera out in front
            

            double moveScale = 40 * cameraFollowsObject->currentSpeed * gui_fov_scale;
            if( ( screenCenterPlayerOffsetX < 0 &&
                  cameraFollowsObject->currentMoveDirection.x < 0 )
                ||
                ( screenCenterPlayerOffsetX > 0 &&
                  cameraFollowsObject->currentMoveDirection.x > 0 ) ) {
                
                moveScale += fabs( screenCenterPlayerOffsetX );
                }
            
            screenCenterPlayerOffsetX -= 
                lrint( moveScale * 
                       cameraFollowsObject->currentMoveDirection.x );
            
            moveScale = 40 * cameraFollowsObject->currentSpeed;
            if( ( screenCenterPlayerOffsetY < 0 &&
                  cameraFollowsObject->currentMoveDirection.y < 0 )
                ||
                ( screenCenterPlayerOffsetY > 0 &&
                  cameraFollowsObject->currentMoveDirection.y > 0 ) ) {
                
                moveScale += fabs( screenCenterPlayerOffsetY );
                }
            

            screenCenterPlayerOffsetY -= 
                lrint( moveScale * 
                       cameraFollowsObject->currentMoveDirection.y );
 
            if( screenCenterPlayerOffsetX < -viewWidth / 3 ) {
                screenCenterPlayerOffsetX =  -viewWidth / 3;
                }
            if( screenCenterPlayerOffsetX >  viewWidth / 3 ) {
                screenCenterPlayerOffsetX =  viewWidth / 3;
                }
            if( screenCenterPlayerOffsetY < -viewHeight / 5 ) {
                screenCenterPlayerOffsetY =  -viewHeight / 5;
                }
            if( screenCenterPlayerOffsetY >  viewHeight / 6 ) {
                screenCenterPlayerOffsetY =  viewHeight / 6;
                }
            }
        else if( false ) { // skip for now
            // stopped moving
            
            if( screenCenterPlayerOffsetX > 0 ) {
                int speed = screenCenterPlayerOffsetX / 10;
                
                if( speed == 0 || speed > 2 ) {
                    speed = 1;
                    }
                screenCenterPlayerOffsetX -= speed;
                }
            if( screenCenterPlayerOffsetX < 0 ) {
                int speed = screenCenterPlayerOffsetX / 10;
                
                if( speed == 0 || speed < -2 ) {
                    speed = -1;
                    }
                screenCenterPlayerOffsetX -= speed;
                }
            
            if( screenCenterPlayerOffsetY > 0 ) {
                int speed = screenCenterPlayerOffsetY / 10;
                
                if( speed == 0 || speed > 2 ) {
                    speed = 1;
                    }
                screenCenterPlayerOffsetY -= speed;
                }
            if( screenCenterPlayerOffsetY < 0 ) {
                int speed = screenCenterPlayerOffsetY / 10;
                
                if( speed == 0 || speed < -2 ) {
                    speed = -1;
                    }
                screenCenterPlayerOffsetY -= speed;
                }
            
            }

        if( ! vogMode ) {
            
            screenTargetPos.x = 
                CELL_D * targetObjectPos.x - 
                screenCenterPlayerOffsetX;
            
            screenTargetPos.y = 
                CELL_D * targetObjectPos.y - 
                screenCenterPlayerOffsetY;
            }
        

        // whole pixels
        screenTargetPos.x = round( screenTargetPos.x );
        screenTargetPos.y = round( screenTargetPos.y );

        if( !shouldMoveCamera ) {
            screenTargetPos = lastScreenViewCenter;
            }
        

        doublePair dir = sub( screenTargetPos, lastScreenViewCenter );
        
        char viewChange = false;
        
        int maxRX = viewWidth / 15;
        int maxRY = viewHeight / 15;
        int maxR = 0;
        double moveSpeedFactor = 20 * cameraFollowsObject->currentSpeed;
        
        if( moveSpeedFactor < 1 ) {
            moveSpeedFactor = 1 * frameRateFactor;
            }

        if( fabs( dir.x ) > maxRX ) {
            double moveScale = moveSpeedFactor * sqrt( fabs(dir.x) - maxRX );

            doublePair moveStep = mult( normalize( dir ), moveScale );
            
            // whole pixels

            moveStep.x = lrint( moveStep.x );
                        
            if( fabs( moveStep.x ) > 0 ) {
                lastScreenViewCenter.x += moveStep.x;
                viewChange = true;
                }
            }
        if( fabs( dir.y ) > maxRY ) {
            double moveScale = moveSpeedFactor * sqrt( fabs(dir.y) - maxRY );

            doublePair moveStep = mult( normalize( dir ), moveScale );
            
            // whole pixels

            moveStep.y = lrint( moveStep.y );
                        
            if( fabs( moveStep.y ) > 0 ) {
                lastScreenViewCenter.y += moveStep.y;
                viewChange = true;
                }
            }
        

        if( false && length( dir ) > maxR ) {
            double moveScale = moveSpeedFactor * sqrt( length( dir ) - maxR );

            doublePair moveStep = mult( normalize( dir ), moveScale );
            
            // whole pixels

            moveStep.x = lrint( moveStep.x );
            moveStep.y = lrint( moveStep.y );
                        
            if( length( moveStep ) > 0 ) {
                lastScreenViewCenter = add( lastScreenViewCenter, 
                                            moveStep );
                viewChange = true;
                }
            }
        

        if( viewChange ) {
            //lastScreenViewCenter.x = screenTargetPos.x;
            //lastScreenViewCenter.y = screenTargetPos.y;
            
            setViewCenterPosition( lastScreenViewCenter.x, 
                                   lastScreenViewCenter.y );
            
            }
	}

    // update all positions for moving objects
    if( !mapPullMode )
    for( int i=0; i<gameObjects.size(); i++ )
    {
		#include "GameScreen_UpdateMovingObjectPosition.cpp"
	}
    
    
    // step fades on location-based speech
    if( !mapPullMode )
    for( int i=0; i<locationSpeech.size(); i++ )
    {
        LocationSpeech *ls = locationSpeech.getElement( i );
        if( game_getCurrentTime() > ls->fadeETATime ) {
            ls->fade -= 0.05 * frameRateFactor;
            
            if( ls->fade <= 0 ) {
                delete [] ls->speech;
                locationSpeech.deleteElement( i );
                i --;
                }
            }
	}

    double currentTime = game_getCurrentTime();

    if( nextActionMessageToSend != NULL
        && ourLiveObject != NULL
        && ourLiveObject->currentSpeed == 0 
        && (
            isGridAdjacent( ourLiveObject->xd, ourLiveObject->yd,
                            playerActionTargetX, playerActionTargetY )
            ||
            ( ourLiveObject->xd == playerActionTargetX &&
              ourLiveObject->yd == playerActionTargetY ) 
            ||
            playerActionTargetNotAdjacent ) )
    {
        
        // done moving on client end
        // can start showing pending action animation, even if 
        // end of motion not received from server yet

        if( !playerActionPending ) {
            playerActionPending = true;
            ourLiveObject->pendingAction = true;
            
            // start measuring again to detect network outages 
            // during this pending action
            largestPendingMessageTimeGap = 0;
            

            // start on first frame to force at least one cycle no
            // matter how fast the server responds
            ourLiveObject->pendingActionAnimationProgress = 
                0.025 * frameRateFactor;
            
            ourLiveObject->pendingActionAnimationStartTime = 
                currentTime;
            
            if( nextActionEating ) {
                addNewAnim( ourLiveObject, eating );
                }
            else {
                addNewAnim( ourLiveObject, doing );
                }
            }
        
        
        // wait until 
        // we've stopped moving locally
        // AND animation has played for a bit
        // (or we know that recent ping has been long enough so that
        //  animation will play long enough without waiting ahead of time)
        // AND server agrees with our position
        if( ! ourLiveObject->inMotion && 
            currentTime - ourLiveObject->pendingActionAnimationStartTime > 
            0.166 - ourLiveObject->lastResponseTimeDelta &&
            ourLiveObject->xd == ourLiveObject->xServer &&
            ourLiveObject->yd == ourLiveObject->yServer ) {
            
 
            // move end acked by server AND action animation in progress

            // queued action waiting for our move to end
            
            ourLiveObject->lastActionSendStartTime = currentTime;
            sendToServerSocket( nextActionMessageToSend );
            
            // reset the timer, because we've gotten some information
            // back from the server about our action
            // so it doesn't seem like we're experiencing a dropped-message
            // bug just yet.
            ourLiveObject->pendingActionAnimationStartTime = 
                game_getCurrentTime();


            if( nextActionEating ) {
                // don't play eating sound here until 
                // we hear from server that we actually ate    
                if( false && ourLiveObject->holdingID > 0 ) {
                    ObjectRecord *held = getObject( ourLiveObject->holdingID );
                    
                    if( held->eatingSound.numSubSounds > 0 ) {
                        playSound( held->eatingSound,
                                   getVectorFromCamera( 
                                       ourLiveObject->currentPos.x, 
                                       ourLiveObject->currentPos.y ) );
                        }       
                    }
                }
            

            delete [] nextActionMessageToSend;
            nextActionMessageToSend = NULL;
            }
	}

    if( ourLiveObject != NULL )
    {
        if( ourLiveObject->holdingFlip != ourLiveObject->lastFlipSent &&
            currentTime - ourLiveObject->lastFlipSendTime > 2 ) {
            
            // been 2 seconds since last sent FLIP to server
            // avoid spamming
            int offset = 1;
            
            if( ourLiveObject->holdingFlip ) {
                offset = -1;
                }
            
            char *message = autoSprintf( 
                "FLIP %d %d#",
                ourLiveObject->xd + offset,
                ourLiveObject->yd );
            
            sendToServerSocket( message );
            
            delete [] message;
            ourLiveObject->lastFlipSendTime = currentTime;
            ourLiveObject->lastFlipSent = ourLiveObject->holdingFlip;
            }
	}
    if( mFirstServerMessagesReceived == 3 )
    {

        if( mStartedLoadingFirstObjectSet && ! mDoneLoadingFirstObjectSet ) {
            mDoneLoadingFirstObjectSet = 
                isLiveObjectSetFullyLoaded( &mFirstObjectSetLoadingProgress );
            
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
        }
    
}




// previous function (step) is so long that it's slowin down Emacs
// on the following function
// put a dummy function here to help emacs.
static void dummyFunctionA() {
    // call self to avoid compiler warning for unused function
    if( false ) {
        dummyFunctionA();
        }
    }



char LivingLifePage::isSameFloor( int inFloor, GridPos inFloorPos, 
                                  int inDX, int inDY )
{
    GridPos nextStep = inFloorPos;
    nextStep.x += inDX;
    nextStep.y += inDY;
                            
    int nextMapI = getMapIndex( nextStep.x, nextStep.y );
                          
    GridPos nextMap = getMapPos( nextStep.x, nextStep.y );
  
    if( nextMapI != -1 
        &&
        mMapFloors[ nextMapI ] == inFloor 
        && 
        ! getCellBlocksWalking( nextMap.x, nextMap.y ) ) {
        return true;
        }
    return false;
}
  
void LivingLifePage::makeActive( char inFresh )
{
    // unhold E key
    mEKeyDown = false;
    mZKeyDown = false;
    mouseDown = false;
    shouldMoveCamera = true;
    screenCenterPlayerOffsetX = 0;
    screenCenterPlayerOffsetY = 0;
    mLastMouseOverID = 0;
    mCurMouseOverID = 0;
    mCurMouseOverFade = 0;
    mCurMouseOverBehind = false;
    mCurMouseOverPerson = false;
    mCurMouseOverSelf = false;
    mPrevMouseOverSpots.deleteAll();
    mPrevMouseOverSpotFades.deleteAll();
    mPrevMouseOverSpotsBehind.deleteAll();
    mCurMouseOverCell.x = -1;
    mCurMouseOverCell.y = -1;
    mCurMouseOverCellFade = 0;
    mLastClickCell.x = -1;
    mLastClickCell.y = -1;
    mPrevMouseOverCells.deleteAll();
    mPrevMouseOverCellFades.deleteAll();
    mPrevMouseClickCells.deleteAll();
    mPrevMouseClickCellFades.deleteAll();

    if( !inFresh ) return;

    mGlobalMessageShowing = false;
    mGlobalMessageStartTime = 0;
    mGlobalMessagesToDestroy.deallocateStringElements();
    offScreenSounds.deleteAll();
    oldHomePosStack.deleteAll();
    oldHomePosStack.push_back_other( &homePosStack );
    takingPhoto = false;
    photoSequenceNumber = -1;
    waitingForPhotoSig = false;
    if( photoSig != NULL )
    {
        delete [] photoSig;
        photoSig = NULL;
	}
    graveRequestPos.deleteAll();
    ownerRequestPos.deleteAll();
    clearOwnerInfo();
    clearLocationSpeech();
    mPlayerInFlight = false;
    vogMode = false;
    showFPS = false;
    showNet = false;
    showPing = false;
	showHelp = false;
    waitForFrameMessages = false;
    serverSocketConnected = false;
    connectionMessageFade = 1.0f;
    connectedTime = 0;
    mPreviousHomeDistStrings.deallocateStringElements();
    mPreviousHomeDistFades.deleteAll();
    mForceHintRefresh = false;
    mLastHintSortedSourceID = 0;
    mLastHintSortedList.deleteAll();

    for( int i=0; i<NUM_HINT_SHEETS; i++ )
    {
        mHintTargetOffset[i] = mHintHideOffset[i];
        mHintPosOffset[i] = mHintHideOffset[i];
	}

    mCurrentHintObjectID = 0;
    mCurrentHintIndex = 0;
    mNextHintObjectID = 0;
    mNextHintIndex = 0;
    
    if( mHintFilterString != NULL )
    {
        delete [] mHintFilterString;
        mHintFilterString = NULL;
	}

    if( mLastHintFilterString != NULL )
    {
        delete [] mLastHintFilterString;
        mLastHintFilterString = NULL;
	}

    if( mPendingFilterString != NULL )
    {
        delete [] mPendingFilterString;
        mPendingFilterString = NULL;
	}

    int tutorialDone = SettingsManager::getIntSetting( "tutorialDone", 0 );
    
    if( ! tutorialDone )
    {
        mTutorialNumber = 1;
	}
    else
	{
        mTutorialNumber = 0;
	}
    
    if( mForceRunTutorial )
    {
        mTutorialNumber = 1;
        mForceRunTutorial = false;
    }

    mLiveTutorialSheetIndex = -1;
    mLiveCravingSheetIndex = -1;
    
    for( int i=0; i<NUM_HINT_SHEETS; i++ )
    {
        mTutorialTargetOffset[i] = mTutorialHideOffset[i];
        mTutorialPosOffset[i] = mTutorialHideOffset[i];
        mTutorialMessage[i] = "";

        mCravingTargetOffset[i] = mCravingHideOffset[i];
        mCravingPosOffset[i] = mCravingHideOffset[i];
        if( mCravingMessage[i] != NULL )
        {
            delete [] mCravingMessage[i];
            mCravingMessage[i] = NULL;
		}
	}

    savingSpeechEnabled = SettingsManager::getIntSetting( "allowSavingSpeech", 0 );

    for( int i=0; i<mGraveInfo.size(); i++ )
    {
        delete [] mGraveInfo.getElement(i)->relationName;
	}
    mGraveInfo.deleteAll();

    clearOwnerInfo();
    

    mRemapDelay = 0;
    mRemapPeak = 0;
    mRemapDirection = 1.0;
    mCurrentRemapFraction = 0;
    
    apocalypseInProgress = false;

    clearMap();
    
    mMapExtraMovingObjects.deleteAll();
    mMapExtraMovingObjectsDestWorldPos.deleteAll();
    mMapExtraMovingObjectsDestObjectIDs.deleteAll();

    mMapGlobalOffsetSet = false;
    mMapGlobalOffset.x = 0;
    mMapGlobalOffset.y = 0;
    
    mNotePaperPosOffset = mNotePaperHideOffset;
    mNotePaperPosTargetOffset = mNotePaperPosOffset;
    mHomeSlipPosOffset = mHomeSlipHideOffset;
    mHomeSlipPosTargetOffset = mHomeSlipPosOffset;

    mLastKnownNoteLines.deallocateStringElements();
    mErasedNoteChars.deleteAll();
    mErasedNoteCharOffsets.deleteAll();
    mErasedNoteCharFades.deleteAll();
    
    mSentChatPhrases.deallocateStringElements();

    for( int i=0; i<3; i++ )
    {
        mHungerSlipPosOffset[i] = mHungerSlipHideOffsets[i];
        mHungerSlipPosTargetOffset[i] = mHungerSlipPosOffset[i];
        mHungerSlipWiggleTime[i] = 0;
	}
    mHungerSlipVisible = -1;

    mSayField.setText( "" );
    mSayField.unfocus();
    TextField::unfocusAll();
    
    mOldArrows.deleteAll();
    mOldDesStrings.deallocateStringElements();
    mOldDesFades.deleteAll();

    mOldLastAteStrings.deallocateStringElements();
    mOldLastAteFillMax.deleteAll();
    mOldLastAteFades.deleteAll();
    mOldLastAteBarFades.deleteAll();
    
    mYumBonus = 0;
    mOldYumBonus.deleteAll();
    mOldYumBonusFades.deleteAll();
    
    mYumMultiplier = 0;

    for( int i=0; i<NUM_YUM_SLIPS; i++ )
    {
        mYumSlipPosOffset[i] = mYumSlipHideOffset[i];
        mYumSlipPosTargetOffset[i] = mYumSlipHideOffset[i];
        mYumSlipNumberToShow[i] = 0;
    }
    

    mCurrentArrowI = 0;
    mCurrentArrowHeat = -1;
    if( mCurrentDes != NULL )
    {
        delete [] mCurrentDes;
    }
    mCurrentDes = NULL;

    if( mCurrentLastAteString != NULL )
    {
        delete [] mCurrentLastAteString;
    }
    mCurrentLastAteString = NULL;


    lastScreenViewCenter.x = 0;
    lastScreenViewCenter.y = 0;
    
    setViewCenterPosition( lastScreenViewCenter.x, lastScreenViewCenter.y );
    
    mPageStartTime = game_getCurrentTime();
    
    setWaiting( true, false );

    readyPendingReceivedMessages.deallocateStringElements();
    serverFrameMessages.deallocateStringElements();

    if( pendingMapChunkMessage != NULL ) {
        delete [] pendingMapChunkMessage;
        pendingMapChunkMessage = NULL;
        }
    pendingCMData = false;
    

    clearLiveObjects();
    mFirstServerMessagesReceived = 0;
    mStartedLoadingFirstObjectSet = false;
    mDoneLoadingFirstObjectSet = false;
    mFirstObjectSetLoadingProgress = 0;

    playerActionPending = false;
    waitingForPong = false;
    lastPingSent = 0;
    lastPongReceived = 0;
    serverSocketBuffer.deleteAll();
    if( nextActionMessageToSend != NULL )
    {
        delete [] nextActionMessageToSend;
        nextActionMessageToSend = NULL;
    }
    

    for( int i=0; i<NUM_HOME_ARROWS; i++ )
    {
        mHomeArrowStates[i].solid = false;
        mHomeArrowStates[i].fade = 0;
    }
}

void LivingLifePage::checkForPointerHit( PointerHitRecord *inRecord, float inX, float inY )
{
    
    LiveObject *ourLiveObject = getOurLiveObject();

    double minDistThatHits = 2.0;

    int clickDestX = lrintf( ( inX ) / CELL_D );
    
    int clickDestY = lrintf( ( inY ) / CELL_D );
    
    float clickExtraX = inX - clickDestX * CELL_D;
    float clickExtraY = inY - clickDestY * CELL_D;

    PointerHitRecord *p = inRecord;
    
    p->closestCellX = clickDestX;
    p->closestCellY = clickDestY;

    if( mForceGroundClick ) {
        return;
        }
    
    int clickDestMapX = clickDestX - mMapOffsetX + mMapD / 2;
    int clickDestMapY = clickDestY - mMapOffsetY + mMapD / 2;
    
    int clickDestMapI = clickDestMapY * mMapD + clickDestMapX;
    
    if( clickDestMapY >= 0 && clickDestMapY < mMapD &&
        clickDestMapX >= 0 && clickDestMapX < mMapD ) {
                
        int oID = mMap[ clickDestMapI ];

        if( ! ourLiveObject->inMotion 
            &&
            // don't allow them to mouse over clicked cell after click
            // until they mouse out and back in again
            ( clickDestMapX != mLastClickCell.x ||
              clickDestMapY != mLastClickCell.y )  
            &&
            ( clickDestMapX != mCurMouseOverCell.x ||
              clickDestMapY != mCurMouseOverCell.y ) ) {

            GridPos newPos = { clickDestMapX, clickDestMapY };

            float oldFade = -0.8f;
            
            for( int i=0; i<mPrevMouseOverCells.size(); i++ ) {
                GridPos old = mPrevMouseOverCells.getElementDirect( i );
            
                if( equal( old, mCurMouseOverCell ) ) {
                    
                    mPrevMouseOverCells.deleteElement( i );
                    mPrevMouseOverCellFades.deleteElement( i );
                    }
                else if( equal( old, newPos ) ) {
                    oldFade = mPrevMouseOverCellFades.getElementDirect( i );

                    mPrevMouseOverCells.deleteElement( i );
                    mPrevMouseOverCellFades.deleteElement( i );
                    }
                }

            mPrevMouseOverCells.push_back( mCurMouseOverCell );
            mPrevMouseOverCellFades.push_back( mCurMouseOverCellFade );

            mCurMouseOverCell.x = clickDestMapX;
            mCurMouseOverCell.y = clickDestMapY;
            
            // moused somewhere new, clear last click
            mLastClickCell.x = -1;
            mLastClickCell.y = -1;
            

            mCurMouseOverCellFadeRate = 0.04;
            
            if( oldFade < 0 && oID > 0 ) {
                // show cell instantly when mousing over and occupied space
                oldFade = 0;
                mCurMouseOverCellFadeRate = 0.2;
                }
            
            mCurMouseOverCellFade = oldFade;
            }
        
        
        // check this cell first

        // all short objects are mouse-through-able
        
        if( oID > 0 && 
            getObjectHeight( oID ) < CELL_D ) {
            ObjectRecord *obj = getObject( oID );
            

            float thisObjClickExtraX = clickExtraX;
            float thisObjClickExtraY = clickExtraY;
            
            if( mMapMoveSpeeds[ clickDestMapI ] > 0 ) {
                thisObjClickExtraX -= 
                    mMapMoveOffsets[ clickDestMapI ].x * CELL_D;
                thisObjClickExtraY -= 
                    mMapMoveOffsets[ clickDestMapI ].y * CELL_D;
                }
            
            int sp, cl, sl;
                
            double dist = getClosestObjectPart( 
                obj,
                NULL,
                &( mMapContainedStacks[clickDestMapI] ),
                NULL,
                false,
                -1,
                -1,
                mMapTileFlips[ clickDestMapI ],
                thisObjClickExtraX,
                thisObjClickExtraY,
                &sp, &cl, &sl,
                // ignore transparent parts
                // allow objects behind smoke to be picked up
                false );
            
            if( dist < minDistThatHits ) {
                p->hitOurPlacement = true;
                p->closestCellX = clickDestX;
                p->closestCellY = clickDestY;
                
                p->hitSlotIndex = sl;
                
                p->hitAnObject = true;
                }
            }
        }
    

    // need to check 3 around cell in all directions because
    // of moving object offsets

    // start in front row
    // right to left
    // (check things that are in front first
    for( int y=clickDestY-3; y<=clickDestY+3 && ! p->hit; y++ ) {
        float clickOffsetY = ( clickDestY  - y ) * CELL_D + clickExtraY;

        // first all non drawn-behind map objects in this row
        // (in front of people)
        for( int x=clickDestX+3; x>=clickDestX-3  && 
                 ! p->hit; x-- ) {
            float clickOffsetX = ( clickDestX  - x ) * CELL_D + clickExtraX;

            int mapX = x - mMapOffsetX + mMapD / 2;
            int mapY = y - mMapOffsetY + mMapD / 2;
            
            if( mapY < 0 || mapY >= mMapD || 
                mapX < 0 || mapX >= mMapD ) { 
                // skip any that are out of map bounds
                continue;
                }
            

            int mapI = mapY * mMapD + mapX;

            int oID = mMap[ mapI ];
            
            

            if( oID > 0 &&
                ! getObject( oID )->drawBehindPlayer ) {
                ObjectRecord *obj = getObject( oID );
                
                float thisObjClickOffsetX = clickOffsetX;
                float thisObjClickOffsetY = clickOffsetY;
            
                if( mMapMoveSpeeds[ mapI ] > 0 ) {
                    thisObjClickOffsetX -= 
                        mMapMoveOffsets[ mapI ].x * CELL_D;
                    thisObjClickOffsetY -= 
                        mMapMoveOffsets[ mapI ].y * CELL_D;
                    }


                int sp, cl, sl;
                
                double dist = getClosestObjectPart( 
                    obj,
                    NULL,
                    &( mMapContainedStacks[mapI] ),
                    NULL,
                    false,
                    -1,
                    -1,
                    mMapTileFlips[ mapI ],
                    thisObjClickOffsetX,
                    thisObjClickOffsetY,
                    &sp, &cl, &sl,
                    // ignore transparent parts
                    // allow objects behind smoke to be picked up
                    false );
                
                if( dist < minDistThatHits ) {
                    p->hit = true;
                    
                    // already hit a short object
                    // AND this object is tall
                    // (don't click through short behind short)
                    if( p->hitOurPlacement &&
                        getObjectHeight( oID ) > .75 * CELL_D ) {
                        
                        if( p->closestCellY > y ) {
                            p->hitOurPlacementBehind = true;
                            }
                        }
                    else {
                        p->closestCellX = x;
                        p->closestCellY = y;
                        
                        p->hitSlotIndex = sl;

                        p->hitAnObject = true;
                        }
                    }
                }
            }

        // don't worry about p->hitOurPlacement when checking them
        // next, people in this row
        // recently dropped babies are in front and tested first
        for( int d=0; d<2 && ! p->hit; d++ )
        for( int x=clickDestX+1; x>=clickDestX-1 && ! p->hit; x-- ) {
            float clickOffsetX = ( clickDestX  - x ) * CELL_D + clickExtraX;
            
            for( int i=gameObjects.size()-1; i>=0 && ! p->hit; i-- ) {
        
                LiveObject *o = gameObjects.getElement( i );

                if( o->outOfRange ) {
                    // out of range, but this was their last known position
                    // don't draw now
                    continue;
                    }
                
                if( o->heldByAdultID != -1 ) {
                    // held by someone else, don't draw now
                    continue;
                    }

                if( d == 1 &&
                    ( o->heldByDropOffset.x != 0 ||
                      o->heldByDropOffset.y != 0 ) ) {
                    // recently dropped baby, skip
                    continue;
                    }
                else if( d == 0 &&
                         o->heldByDropOffset.x == 0 &&
                         o->heldByDropOffset.y == 0 ) {
                    // not a recently-dropped baby, skip
                    continue;
                    }
                
                
                double oX = o->xd;
                double oY = o->yd;
                
                if( o->currentSpeed != 0 && o->pathToDest != NULL ) {
                    oX = o->currentPos.x;
                    oY = o->currentPos.y;
                    }
                
                
                if( round( oX ) == x &&
                    round( oY ) == y ) {
                                        
                    // here!

                    double personClickOffsetX = ( oX - x ) * CELL_D;
                    double personClickOffsetY = ( oY - y ) * CELL_D;
                    
                    personClickOffsetX = clickOffsetX - personClickOffsetX;
                    personClickOffsetY = clickOffsetY - personClickOffsetY;

                    ObjectRecord *obj = getObject( o->displayID );
                    
                    int sp, cl, sl;
                    
                    double dist = getClosestObjectPart( 
                        obj,
                        &( o->clothing ),
                        NULL,
                        o->clothingContained,
                        false,
                        computeCurrentAge( o ),
                        -1,
                        o->holdingFlip,
                        personClickOffsetX,
                        personClickOffsetY,
                        &sp, &cl, &sl );
                    
                    if( dist < minDistThatHits ) {
                        p->hit = true;
                        p->closestCellX = x;
                        p->closestCellY = y;
                        if( o == ourLiveObject ) {
                            p->hitSelf = true;

                            }
                        else {
                            p->hitOtherPerson = true;
                            p->hitOtherPersonID = o->id;
                            }
                        if( cl != -1 ) {
                            p->hitClothingIndex = cl;
                            p->hitSlotIndex = sl;
                            }
                        }
                    }
                }
            
            }
        
        // now drawn-behind objects in this row

        for( int x=clickDestX+1; x>=clickDestX-1  
                 && ! p->hit; x-- ) {
            float clickOffsetX = ( clickDestX  - x ) * CELL_D + clickExtraX;

            int mapX = x - mMapOffsetX + mMapD / 2;
            int mapY = y - mMapOffsetY + mMapD / 2;

            if( mapY < 0 || mapY >= mMapD || 
                mapX < 0 || mapX >= mMapD ) { 
                // skip any that are out of map bounds
                continue;
                }

            int mapI = mapY * mMapD + mapX;

            int oID = mMap[ mapI ];
            
            

            if( oID > 0 &&
                getObject( oID )->drawBehindPlayer ) {
                ObjectRecord *obj = getObject( oID );
                

                float thisObjClickOffsetX = clickOffsetX;
                float thisObjClickOffsetY = clickOffsetY;
            
                if( mMapMoveSpeeds[ mapI ] > 0 ) {
                    thisObjClickOffsetX -= 
                        mMapMoveOffsets[ mapI ].x * CELL_D;
                    thisObjClickOffsetY -= 
                        mMapMoveOffsets[ mapI ].y * CELL_D;
                    }


                int sp, cl, sl;
                
                double dist = getClosestObjectPart( 
                    obj,
                    NULL,
                    &( mMapContainedStacks[mapI] ),
                    NULL,
                    false,
                    -1,
                    -1,
                    mMapTileFlips[ mapI ],
                    thisObjClickOffsetX,
                    thisObjClickOffsetY,
                    &sp, &cl, &sl,
                    // ignore transparent parts
                    // allow objects behind smoke to be picked up
                    false );
                
                if( dist < minDistThatHits ) {
                    p->hit = true;
                    
                                        
                    // already hit a short object
                    // AND this object is tall
                    // (don't click through short behind short)
                    if( p->hitOurPlacement &&
                        getObjectHeight( oID ) > .75 * CELL_D ) {
                        
                        if( p->closestCellY > y ) {
                            p->hitOurPlacementBehind = true;
                            }
                        }
                    else {
                        p->closestCellX = x;
                        p->closestCellY = y;
                        
                        p->hitSlotIndex = sl;
                        
                        p->hitAnObject = true;
                        }
                    }
                }
            }
        }

    if( p->hitOurPlacement &&
        ( p->hitOtherPerson || p->hitSelf ) ) {
        p->hitAnObject = false;
        }
    else {
        // count it now, if we didn't hit a person that blocks it
        p->hit = true;
        }




    
    if( p->hit && p->hitAnObject && ! p->hitOtherPerson && ! p->hitSelf ) {
        // hit an object
        
        // what if someone is standing behind it?
        
        // look at two cells above
        for( int y= p->closestCellY + 1; 
             y < p->closestCellY + 3 && ! p->hitOtherPerson; y++ ) {
            
            float clickOffsetY = ( clickDestY  - y ) * CELL_D + clickExtraY;
        
            // look one cell to left/right
            for( int x= p->closestCellX - 1; 
                 x < p->closestCellX + 2 && ! p->hitOtherPerson; x++ ) {

                float clickOffsetX = ( clickDestX  - x ) * CELL_D + clickExtraX;

                for( int i=gameObjects.size()-1; 
                     i>=0 && ! p->hitOtherPerson; i-- ) {
        
                    LiveObject *o = gameObjects.getElement( i );
                    
                    if( o->outOfRange ) {
                        // out of range, but this was their last known position
                        // don't draw now
                        continue;
                        }
                    
                    if( o->heldByAdultID != -1 ) {
                        // held by someone else, don't draw now
                        continue;
                        }
                    
                    if( o->heldByDropOffset.x != 0 ||
                        o->heldByDropOffset.y != 0 ) {
                        // recently dropped baby, skip
                        continue;
                        }
                
                    if( o == ourLiveObject ) {
                        // ignore clicks on self behind tree
                        continue;
                        }
                    
                    
                    int oX = o->xd;
                    int oY = o->yd;
                
                    if( o->currentSpeed != 0 && o->pathToDest != NULL ) {
                        if( o->onFinalPathStep ) {
                            oX = o->pathToDest[ o->pathLength - 1 ].x;
                            oY = o->pathToDest[ o->pathLength - 1 ].y;
                            }
                        else {
                            oX = o->pathToDest[ o->currentPathStep ].x;
                            oY = o->pathToDest[ o->currentPathStep ].y;
                            }
                        }
            
                    
                    if( oY == y && oX == x ) {
                        // here!
                        ObjectRecord *obj = getObject( o->displayID );
                    
                        int sp, cl, sl;
                        
                        double dist = getClosestObjectPart( 
                            obj,
                            &( o->clothing ),
                            NULL,
                            o->clothingContained,
                            false,
                            computeCurrentAge( o ),
                            -1,
                            o->holdingFlip,
                            clickOffsetX,
                            clickOffsetY,
                            &sp, &cl, &sl );
                    
                        if( dist < minDistThatHits ) {
                            p->hit = true;
                            p->closestCellX = x;
                            p->closestCellY = y;
                            
                            p->hitAnObject = false;
                            p->hitOtherPerson = true;
                            p->hitOtherPersonID = o->id;
                            
                            if( cl != -1 ) {
                                p->hitClothingIndex = cl;
                                p->hitSlotIndex = sl;
                                }
                            }
                        }
                    }
                }
            }
        }
}

void LivingLifePage::pointerMove( float inX, float inY )
{
    lastMouseX = inX;
    lastMouseY = inY;
        
    getLastMouseScreenPos( &lastScreenMouseX, &lastScreenMouseY );

    if( showBugMessage ) {
        return;
        }

    if( mServerSocket == -1 ) {
        // dead
        return;
        }

    if( mFirstServerMessagesReceived != 3 || ! mDoneLoadingFirstObjectSet ) {
        return;
        }

    PointerHitRecord p;
    
    p.hitSlotIndex = -1;
    

    p.hit = false;
    p.hitSelf = false;
    p.hitOurPlacement = false;
    p.hitOurPlacementBehind = false;
    
    p.hitClothingIndex = -1;
    

    // when we click in a square, only count as hitting something
    // if we actually clicked the object there.  Else, we can walk
    // there if unblocked.
    p.hitAnObject = false;

    p.hitOtherPerson = false;

    checkForPointerHit( &p, inX, inY );

    mCurMouseOverPerson = p.hitOtherPerson || p.hitSelf;
    
    
    int clickDestX = p.closestCellX;
    int clickDestY = p.closestCellY;
    

    int destID = 0;
        
    int mapX = clickDestX - mMapOffsetX + mMapD / 2;
    int mapY = clickDestY - mMapOffsetY + mMapD / 2;
    if( p.hitAnObject && mapY >= 0 && mapY < mMapD &&
        mapX >= 0 && mapX < mMapD ) {
        
        destID = mMap[ mapY * mMapD + mapX ];
        }



    if( mCurMouseOverID > 0 
        && 
        ! mCurMouseOverSelf
        &&
        ( mCurMouseOverID != destID
          ||
          mCurMouseOverSpot.x != mapX ||
          mCurMouseOverSpot.y != mapY ) ) {
        
        GridPos prev = { mCurMouseOverSpot.x, mCurMouseOverSpot.y };
        
        for( int i=0; i<mPrevMouseOverSpots.size(); i++ ) {
            GridPos old = mPrevMouseOverSpots.getElementDirect( i );
            
            if( equal( old, prev ) ) {
                mPrevMouseOverSpots.deleteElement( i );
                mPrevMouseOverSpotFades.deleteElement( i );
                mPrevMouseOverSpotsBehind.deleteElement( i );
                break;
                }
            }
        
        mPrevMouseOverSpots.push_back( prev );
        mPrevMouseOverSpotFades.push_back( mCurMouseOverFade );
        mPrevMouseOverSpotsBehind.push_back( mCurMouseOverBehind );
        }

    char overNothing = true;

    LiveObject *ourLiveObject = getOurLiveObject();

    ourLiveObject->currentMouseOverClothingIndex = -1;
    
    if( destID == 0 ) {
        if( p.hitSelf ) {
            mCurMouseOverSelf = true;
            
            // clear when mousing over bare parts of body
            // show YOU
            mCurMouseOverID = -99;
            
            overNothing = false;
            
            
            if( p.hitClothingIndex != -1 ) {
                if( p.hitSlotIndex != -1 ) {
                    mCurMouseOverID = 
                        ourLiveObject->clothingContained[ p.hitClothingIndex ].
                        getElementDirect( p.hitSlotIndex );
                    }
                else {
                    ObjectRecord *c = 
                        clothingByIndex( ourLiveObject->clothing, 
                                         p.hitClothingIndex );
                    mCurMouseOverID = c->id;
                    }
                
                ourLiveObject->currentMouseOverClothingIndex =
                    p.hitClothingIndex;
                }
            }
        if( p.hitOtherPerson ) {
            // store negative in place so that we can show their relation
            // string
            mCurMouseOverID = - p.hitOtherPersonID;
            }
        }
    

    if( destID > 0 ) {
        mCurMouseOverSelf = false;
        
        if( ( destID != mCurMouseOverID ||
              mCurMouseOverSpot.x != mapX ||
              mCurMouseOverSpot.y != mapY  ) ) {
            
            // start new fade-in
            mCurMouseOverFade = 0;
            }

        mCurMouseOverID = destID;
        overNothing = false;
        
        if( p.hitSlotIndex != -1 ) {
            mCurMouseOverID = 
                mMapContainedStacks[ mapY * mMapD + mapX ].
                getElementDirect( p.hitSlotIndex );
            }
        
        
        mCurMouseOverSpot.x = mapX;
        mCurMouseOverSpot.y = mapY;

        mCurMouseOverWorld.x = clickDestX;
        mCurMouseOverWorld.y = clickDestY;
        
        mCurMouseOverBehind = p.hitOurPlacementBehind;
        

        // check if we already have a partial-fade-out for this cell
        // from previous mouse-overs
        for( int i=0; i<mPrevMouseOverSpots.size(); i++ ) {
            GridPos old = mPrevMouseOverSpots.getElementDirect( i );
            
            if( equal( old, mCurMouseOverSpot ) ) {
                mCurMouseOverFade = 
                    mPrevMouseOverSpotFades.getElementDirect( i );
                
                mPrevMouseOverSpots.deleteElement( i );
                mPrevMouseOverSpotFades.deleteElement( i );
                mPrevMouseOverSpotsBehind.deleteElement( i );
                break;
                }
            }
        }
    
    
    if( overNothing && mCurMouseOverID != 0 ) {
        mLastMouseOverID = mCurMouseOverID;
        
        if( mCurMouseOverSelf ) {
            // don't keep YOU or EAT or clothing tips around after we mouse 
            // outside of them
            mLastMouseOverID = 0;
            }
        mCurMouseOverSelf = false;
        
        mCurMouseOverID = 0;
        mCurMouseOverBehind = false;
        mLastMouseOverFade = 1.0f;
        }
    
    
    double worldX = inX / (double)CELL_D;
    

    if( ! ourLiveObject->inMotion &&
        // watch for being stuck with no-move object in hand, like fishing
        // pole
        ( ourLiveObject->holdingID <= 0 ||
          getObject( ourLiveObject->holdingID )->speedMult > 0 ) ) {
        char flip = false;
        
        if( ourLiveObject->holdingFlip &&
            worldX > ourLiveObject->currentPos.x + 0.5 ) {
            ourLiveObject->holdingFlip = false;
            flip = true;
            }
        else if( ! ourLiveObject->holdingFlip &&
                 worldX < ourLiveObject->currentPos.x - 0.5 ) {
            ourLiveObject->holdingFlip = true;
            flip = true;
            }

        if( flip ) {
            ourLiveObject->lastAnim = moving;
            ourLiveObject->curAnim = ground2;
            ourLiveObject->lastAnimFade = 1;
            
            ourLiveObject->lastHeldAnim = moving;
            ourLiveObject->curHeldAnim = held;
            ourLiveObject->lastHeldAnimFade = 1;
            }
        }
}


char LivingLifePage::getCellBlocksWalking( int inMapX, int inMapY )
{
    
    if( inMapY >= 0 && inMapY < mMapD &&
        inMapX >= 0 && inMapX < mMapD ) {
        
        int destID = mMap[ inMapY * mMapD + inMapX ];
        
        
        if( destID > 0 && getObject( destID )->blocksWalking ) {
            return true;
            }
        else {
            // check for wide neighbors
            
            int r = getMaxWideRadius();
            
            int startX = inMapX - r;
            int endX = inMapX + r;
            
            if( startX < 0 ) {
                startX = 0;
                }
            if( endX >= mMapD ) {
                endX = mMapD - 1;
                }
            for( int x=startX; x<=endX; x++ ) {
                int nID = mMap[ inMapY * mMapD + x ];

                if( nID > 0 ) {
                    ObjectRecord *nO = getObject( nID );
                    
                    if( nO->blocksWalking && nO->wide &&
                        ( x - inMapX <= nO->rightBlockingRadius || 
                          inMapX - x >= nO->leftBlockingRadius ) ) {
                        return true;
                        }
                    }
                }
            }
        
        return false;
        }
    else {
        // off map blocks
        return true;
        }
}


void LivingLifePage::pointerDown( float inX, float inY )
{
	if (minitech::livingLifePageMouseDown( inX, inY )) return;
	
    lastMouseX = inX;
    lastMouseY = inY;

    if( showBugMessage ) {
        return;
        }
    
    if( mServerSocket == -1 ) {
        // dead
        return;
        }
	
	int mouseButton = getLastMouseButton();
	bool scaling = false;
	if ( mouseButton == MouseButton::WHEELUP || mouseButton == MouseButton::WHEELDOWN ) { scaling = true; }
	if ( blockMouseScaling ) { scaling = false; }
	
    if( vogMode ) {
        return;
        }

    char modClick = false;
    
    if( ( mEKeyDown && mEKeyEnabled ) || ( isLastMouseButtonRight() && !mForceGroundClick ) ) {
        modClick = true;
        }
   
	//FOV
	if( scaling ) {
		float currentScale = SettingsManager::getFloatSetting( "fovScale", 1.0f );
		float newScale = ( mouseButton == MouseButton::WHEELUP ) ? currentScale -= 0.25f : currentScale += 0.25f;
		if ( isShiftKeyDown() ) {
			newScale = ( mouseButton == MouseButton::WHEELUP ) ? SettingsManager::getFloatSetting( "fovDefault", 1.25f ) : SettingsManager::getFloatSetting( "fovMax", 2.25f );
            }
        if ( isCommandKeyDown() ) {
            float currentHUDScale = SettingsManager::getFloatSetting( "fovScaleHUD", 1.25f );
            newScale = ( mouseButton == MouseButton::WHEELUP ) ? currentHUDScale -= 0.25f : currentHUDScale += 0.25f;
            changeHUDFOV( newScale );
        } else {
            	if ( SettingsManager::getIntSetting( "fovEnabled", 1 ) ) changeFOV( newScale );
            }
		return;
	}
    
    mLastMouseOverID = 0;
    
    // detect cases where mouse is held down already
    // this is for long-distance player motion, and we don't want
    // this to result in an action by accident if they mouse over
    // something actionable along the way
    char mouseAlreadyDown = mouseDown;
    
    mouseDown = true;
    if( !mouseAlreadyDown ) {
        mouseDownFrames = 0;
        }
    
    getLastMouseScreenPos( &lastScreenMouseX, &lastScreenMouseY );
    
    if( mFirstServerMessagesReceived != 3 || ! mDoneLoadingFirstObjectSet ) {
        return;
        }

    if( playerActionPending ) {
        printf( "Skipping click, action pending\n" );
        
        // block further actions until update received to confirm last
        // action
        return;
        }
    

    LiveObject *ourLiveObject = getOurLiveObject();
    
    if( ourLiveObject->holdingID > 0 &&
        getObject( ourLiveObject->holdingID )->speedMult == 0 ) {
        // holding something that stops movement entirely, ignore click
        
        TransRecord *groundTrans = getTrans( ourLiveObject->holdingID, -1 );
        
        if( groundTrans == NULL ) {

            printf( "Skipping click, holding 0-speed object "
                    "that can't be used on ground\n" );
            return;
            }
        }
    

    if( ourLiveObject->heldByAdultID != -1 ) {
        // click from a held baby

        // only send once every 5 seconds, even on multiple clicks
        double curTime = game_getCurrentTime();
        
        if( ourLiveObject->jumpOutOfArmsSentTime < curTime - 5 ) {
            // send new JUMP message instead of ambigous MOVE message
            sendToServerSocket( (char*)"JUMP 0 0#" );
            
            ourLiveObject->jumpOutOfArmsSentTime = curTime;
            }
        
        if( ! ourLiveObject->babyWiggle ) {
            // start new wiggle
            ourLiveObject->babyWiggle = true;
            ourLiveObject->babyWiggleProgress = 0;
            }
        
        
        return;
        }
    

    // consider 3x4 area around click and test true object pixel
    // collisions in that area
    PointerHitRecord p;
    
    p.hitSlotIndex = -1;
    
    p.hit = false;
    p.hitSelf = false;
    p.hitOurPlacement = false;
    p.hitOurPlacementBehind = false;
    
    p.hitClothingIndex = -1;
    

    // when we click in a square, only count as hitting something
    // if we actually clicked the object there.  Else, we can walk
    // there if unblocked.
    p.hitAnObject = false;
    
    p.hitOtherPerson = false;
    

    checkForPointerHit( &p, inX, inY );



    // new semantics
    // as soon as we trigger a kill attempt, we go into kill mode
    // by sending server a KILL message right away
    char killMode = false;
 

    // don't allow weapon-drop on kill-click unless there's really
    // no one around
    if( ! mouseAlreadyDown &&
        modClick && ourLiveObject->holdingID > 0 &&
        getObject( ourLiveObject->holdingID )->deadlyDistance > 0 &&
        isShiftKeyDown() &&
        ! p.hitOtherPerson ) {
        
        // everything good to go for a kill-click, but they missed
        // hitting someone (and maybe they clicked on an object instead)

        // find closest person for them to hit
        
        doublePair clickPos = { inX, inY };
        
        
        int closePersonID = -1;
        double closeDistance = DBL_MAX;
        
        for( int i=gameObjects.size()-1; i>=0; i-- ) {
        
            LiveObject *o = gameObjects.getElement( i );

            if( o->id == ourID ) {
                // don't consider ourself as a kill target
                continue;
                }
            
            if( o->outOfRange ) {
                // out of range, but this was their last known position
                // don't draw now
                continue;
                }
            
            if( o->heldByAdultID != -1 ) {
                // held by someone else, can't click on them
                continue;
                }
            
            if( o->heldByDropOffset.x != 0 ||
                o->heldByDropOffset.y != 0 ) {
                // recently dropped baby, skip
                continue;
                }
                
                
            double oX = o->xd;
            double oY = o->yd;
                
            if( o->currentSpeed != 0 && o->pathToDest != NULL ) {
                oX = o->currentPos.x;
                oY = o->currentPos.y;
                }

            oY *= CELL_D;
            oX *= CELL_D;
            

            // center of body up from feet position in tile
            oY += CELL_D / 2;
            
            doublePair oPos = { oX, oY };
            

            double thisDistance = distance( clickPos, oPos );
            
            if( thisDistance < closeDistance ) {
                closeDistance = thisDistance;
                closePersonID = o->id;
                }
            }

        if( closePersonID != -1 && closeDistance < 4 * CELL_D ) {
            // somewhat close to clicking on someone
            p.hitOtherPerson = true;
            p.hitOtherPersonID = closePersonID;
            p.hitAnObject = false;
            p.hit = true;
            killMode = true;
            }
        
        }






    
    mCurMouseOverPerson = p.hitOtherPerson || p.hitSelf;

    // don't allow clicking on object during continued motion
    if( mouseAlreadyDown ) {
        p.hit = false;
        p.hitSelf = false;
        p.hitOurPlacement = false;
        p.hitAnObject = false;
        p.hitOtherPerson = false;
        
        
        // also, use direct tile clicking
        // (don't remap clicks on the top of tall objects down to the base tile)
        p.closestCellX = lrintf( ( inX ) / CELL_D );
    
        p.closestCellY = lrintf( ( inY ) / CELL_D );
        }
    
    // clear mouse over cell
    mPrevMouseOverCells.push_back( mCurMouseOverCell );
    mPrevMouseOverCellFades.push_back( mCurMouseOverCellFade );
    
    if( mCurMouseOverCell.x != -1 ) {    
        mLastClickCell = mCurMouseOverCell;
        }

    mCurMouseOverCell.x = -1;
    mCurMouseOverCell.y = -1;

    
    int clickDestX = p.closestCellX;
    int clickDestY = p.closestCellY;

    int destID = 0;
    int floorDestID = 0;
    
    int destObjInClickedTile = 0;
    char destObjInClickedTilePermanent = false;
    
    int destNumContained = 0;
    
    int mapX = clickDestX - mMapOffsetX + mMapD / 2;
    int mapY = clickDestY - mMapOffsetY + mMapD / 2;    
    
    
    if( mouseAlreadyDown && 
        mouseDownFrames >  
        minMouseDownFrames / frameRateFactor ) {
        
        // continuous movement mode

        // watch out for case where they mouse over a blocked spot by accident
        
        if( getCellBlocksWalking( mapX, mapY ) ) {
            printf( "Blocked at cont move click dest %d,%d\n",
                    clickDestX, clickDestY );
            
            double xDelta = clickDestX - ourLiveObject->currentPos.x;
            double yDelta = clickDestY - ourLiveObject->currentPos.y;

            int step = 1;
            int limit = 10;
                
            if( fabs( xDelta ) > fabs( yDelta ) ) {
                // push out further in x direction
                if( xDelta < 0 ) {
                    step = -step;
                    limit = -limit;
                    }

                for( int xd=step; xd != limit; xd += step ) {
                    if( ! getCellBlocksWalking( mapX + xd, mapY ) ) {
                        // found
                        clickDestX += xd;
                        break;
                        }
                    }
                }
            else {
                // push out further in y direction
                if( yDelta < 0 ) {
                    step = -step;
                    limit = -limit;
                    }

                for( int yd=step; yd != limit; yd += step ) {
                    if( ! getCellBlocksWalking( mapX, mapY + yd ) ) {
                        // found
                        clickDestY += yd;
                        break;
                        }
                    }
                }
            
            printf( "Pushing out to click dest %d,%d\n",
                    clickDestX, clickDestY );
            
            // recompute
            mapX = clickDestX - mMapOffsetX + mMapD / 2;
            mapY = clickDestY - mMapOffsetY + mMapD / 2;
            }
        }
    

    printf( "clickDestX,Y = %d, %d,  mapX,Y = %d, %d, curX,Y = %d, %d\n", 
            clickDestX, clickDestY, 
            mapX, mapY,
            ourLiveObject->xd, ourLiveObject->yd );
    if( ! killMode && 
        mapY >= 0 && mapY < mMapD &&
        mapX >= 0 && mapX < mMapD ) {
        
        destID = mMap[ mapY * mMapD + mapX ];
        floorDestID = mMapFloors[ mapY * mMapD + mapX ];
        
        destObjInClickedTile = destID;

        if( destObjInClickedTile > 0 ) {
            destObjInClickedTilePermanent =
                getObject( destObjInClickedTile )->permanent;
            }
    
        destNumContained = mMapContainedStacks[ mapY * mMapD + mapX ].size();
        

        // if holding something, and this is a set-down action
        // show click reaction
        if( modClick &&
            ourLiveObject->holdingID != 0 ) {
        
            int mapI = mapY * mMapD + mapX;
            
            int id = mMap[mapI];
            
            if( id == 0 || ! getObject( id )->permanent ) {
                
                // empty cell, or something we can swap held with
                
                GridPos clickPos = { mapX, mapY };
                
                mPrevMouseClickCells.push_back( clickPos );
                mPrevMouseClickCellFades.push_back( 1 );
                
                // instantly fade current cell to get it out of the way
                // of our click indicator
                mCurMouseOverCellFade = 0;
                }
            
            }
        }



    nextActionEating = false;
    nextActionDropping = false;
    


    if( p.hitSelf ) {
        // click on self

        // ignore unless it's a use-on-self action and standing still

        if( ! ourLiveObject->inMotion ) {
            
            if( nextActionMessageToSend != NULL ) {
                delete [] nextActionMessageToSend;
                nextActionMessageToSend = NULL;
                }

            if( !modClick || p.hitClothingIndex == -1 ) {
                
                if( ourLiveObject->holdingID > 0 &&
                    getObject( ourLiveObject->holdingID )->foodValue > 0 ) {
                    nextActionEating = true;
                    }
    
                nextActionMessageToSend = 
                    autoSprintf( "SELF %d %d %d#",
                                 sendX( clickDestX ), sendY( clickDestY ), 
                                 p.hitClothingIndex );
                printf( "Use on self\n" );
                }
            else {
                // modclick on hit clothing

                if( ourLiveObject->holdingID > 0 ) {
                    nextActionMessageToSend = 
                        autoSprintf( "DROP %d %d %d#",
                                     sendX( clickDestX ), sendY( clickDestY ), 
                                     p.hitClothingIndex  );
                    nextActionDropping = true;
                    printf( "Add to own clothing container\n" );
                    }
                else {
                    nextActionMessageToSend = 
                        autoSprintf( "SREMV %d %d %d %d#",
                                     sendX( clickDestX ), sendY( clickDestY ), 
                                     p.hitClothingIndex,
                                     p.hitSlotIndex );
                    printf( "Remove from own clothing container\n" );
                    }
                }

            playerActionTargetX = clickDestX;
            playerActionTargetY = clickDestY;
            
            }
        

        return;
        }
    
    
    // may change to empty adjacent spot to click
    int moveDestX = clickDestX;
    int moveDestY = clickDestY;
    
    char mustMove = false;


    


    if( destID > 0 && p.hitAnObject ) {
        
        if( ourLiveObject->holdingID > 0 ) {
            TransRecord *tr = getTrans( ourLiveObject->holdingID, destID );
            
            if( tr == NULL ) {
                // try defaul transition
                // tr = getTrans( -2, destID );
                
                // for now, DO NOT consider default transition
                // no main transition for this held object applies
                // so we should probably give a hint about what CAN apply
                // to the target object.

                // Default transitions are currently just used to make
                // something react to the player (usually for animals getting
                // startled), not to actually accomplish
                // something

                // so we should show hints about the target object BEFORE it
                // went into its (temporary) reaction state. 
                }
            
            if( tr == NULL || tr->newTarget == destID ) {
                // give hint about dest object which will be unchanged 
                mNextHintObjectID = destID;
                mNextHintIndex = mHintBookmarks[ destID ];
				if (minitech::changeHintObjOnTouch) minitech::currentHintObjId = destID;
                }
            else if( tr->newActor > 0 && 
                     ourLiveObject->holdingID != tr->newActor ) {
                // give hint about how what we're holding will change
                mNextHintObjectID = tr->newActor;
                mNextHintIndex = mHintBookmarks[ tr->newTarget ];
				if (minitech::changeHintObjOnTouch) minitech::currentHintObjId = tr->newActor;
                }
            else if( tr->newTarget > 0 ) {
                // give hint about changed target after we act on it
                mNextHintObjectID = tr->newTarget;
                mNextHintIndex = mHintBookmarks[ tr->newTarget ];
				if (minitech::changeHintObjOnTouch) minitech::currentHintObjId = tr->newTarget;
                }
            }
        else {
            // bare hand
            // only give hint about hit if no bare-hand action applies

            if( getTrans( 0, destID ) == NULL ) {
                mNextHintObjectID = destID;
                mNextHintIndex = mHintBookmarks[ destID ];
				if (minitech::changeHintObjOnTouch) minitech::currentHintObjId = destID;
                }
            }
        }

    
    if( destID > 0 && ! p.hitAnObject ) {
        
        // clicked on empty space near an object
        
        if( ! getObject( destID )->blocksWalking ) {
            
            // object in this space not blocking
            
            // count as an attempt to walk to the spot where the object is
            destID = 0;
            }
        else {
            // clicked on grid cell around blocking object
            destID = 0;
            
            // search for closest non-blocking and walk there
            
            GridPos closestP = { -1, -1 };
            
            double startDist = 99999;
            
            double closestDist = startDist;
            GridPos clickP = { clickDestX, clickDestY };
            GridPos ourP = { ourLiveObject->xd, ourLiveObject->yd };
            
            for( int y=-5; y<5; y++ ) {
                for( int x=-5; x<5; x++ ) {
                    
                    GridPos p = { clickDestX + x, clickDestY + y };
                    
                    int mapPX = p.x - mMapOffsetX + mMapD / 2;
                    int mapPY = p.y - mMapOffsetY + mMapD / 2;

                    if( mapPY >= 0 && mapPY < mMapD &&
                        mapPX >= 0 && mapPX < mMapD ) {
                        
                        int oID = mMap[ mapPY * mMapD + mapPX ];

                        if( oID == 0 
                            ||
                            ( oID > 0 && ! getObject( oID )->blocksWalking ) ) {


                            double d2 = distance2( p, clickP );
                            
                            if( d2 < closestDist ) {
                                closestDist = d2;
                                closestP = p;
                                }
                            else if( d2 == closestDist ) {
                                // break tie by whats closest to player
                                
                                double dPlayerOld = distance2( closestP,
                                                               ourP );
                                double dPlayerNew = distance2( p, ourP );
                                
                                if( dPlayerNew < dPlayerOld ) {
                                    closestDist = d2;
                                    closestP = p;
                                    }
                                }
                            
                            }
                        }
                    
                    }
                }
            
            if( closestDist < startDist ) {
                // found one
                // walk there instead
                moveDestX = closestP.x;
                moveDestY = closestP.y;
                
                clickDestX = moveDestX;
                clickDestY = moveDestY;
                
                mapX = clickDestX - mMapOffsetX + mMapD / 2;
                mapY = clickDestY - mMapOffsetY + mMapD / 2;
                }
            }
        }

    printf( "DestID = %d\n", destID );
    
    if( destID < 0 ) {
        return;
        }

    if( nextActionMessageToSend != NULL ) {
        delete [] nextActionMessageToSend;
        nextActionMessageToSend = NULL;
        }
    


   
    
    


    if( destID == 0 &&
        p.hitOtherPerson &&
        modClick && ourLiveObject->holdingID > 0 &&
        getObject( ourLiveObject->holdingID )->deadlyDistance > 0 &&
        isShiftKeyDown() ) {
        
        // special case

        // check for possible kill attempt at a distance

        LiveObject *o = getLiveObject( p.hitOtherPersonID );
        
        if( o->id != ourID &&            
            o->heldByAdultID == -1 ) {
                
            // can't kill by clicking on ghost-location of held baby
            
            // clicked on someone
                    
            // new semantics:
            // send KILL to server right away to
            // tell server of our intentions
            // (whether or not we are close enough)
                
            // then walk there
                
            if( nextActionMessageToSend != NULL ) {
                delete [] nextActionMessageToSend;
                nextActionMessageToSend = NULL;
                }
                        
                        
            char *killMessage = 
                autoSprintf( "KILL %d %d %d#",
                             sendX( clickDestX ), 
                             sendY( clickDestY ),
                             p.hitOtherPersonID );
            printf( "KILL with direct-click target player "
                    "id=%d\n", p.hitOtherPersonID );
                    
            sendToServerSocket( killMessage );

            delete [] killMessage;
            

            // try to walk near victim right away
            killMode = true;
                    
            ourLiveObject->killMode = true;
            ourLiveObject->killWithID = ourLiveObject->holdingID;

            // ignore mod-click from here on out, to avoid
            // force-dropping weapon
            modClick = false;
            }
        }
    

    if( destID != 0 &&
        ! modClick &&
        ourLiveObject->holdingID > 0 &&
        getObject( ourLiveObject->holdingID )->useDistance > 1 &&
        getTrans( ourLiveObject->holdingID, destID ) != NULL ) {
        // check if we're close enough to use this from here 
        
        double d = sqrt( ( clickDestX - ourLiveObject->xd ) * 
                         ( clickDestX - ourLiveObject->xd )
                         +
                         ( clickDestY - ourLiveObject->yd ) * 
                         ( clickDestY - ourLiveObject->yd ) );

        if( getObject( ourLiveObject->holdingID )->useDistance >= d ) {
            // close enough to use object right now

                        
            if( nextActionMessageToSend != NULL ) {
                delete [] nextActionMessageToSend;
                nextActionMessageToSend = NULL;
                }
            
            nextActionMessageToSend = 
                autoSprintf( "USE %d %d#",
                             sendX( clickDestX ), 
                             sendY( clickDestY ) );
            
                        
            playerActionTargetX = clickDestX;
            playerActionTargetY = clickDestY;
            
            playerActionTargetNotAdjacent = true;
                        
            printf( "USE from a distance, src=(%d,%d), dest=(%d,%d)\n",
                    ourLiveObject->xd, ourLiveObject->yd,
                    clickDestX, clickDestY );
            
            return;
            }
        }
    

    char tryingToHeal = false;
    

    if( ourLiveObject->holdingID >= 0 &&
        p.hitOtherPerson &&
        getLiveObject( p.hitOtherPersonID )->dying ) {
        
        LiveObject *targetPlayer = getLiveObject( p.hitOtherPersonID );
        
        if( targetPlayer->holdingID > 0 ) {
            
            TransRecord *r = getTrans( ourLiveObject->holdingID,
                                       targetPlayer->holdingID );
            
            if( r != NULL ) {
                // a transition applies between what we're holding and their
                // wound
                tryingToHeal = true;
                }
            }
        }
    
    
    char canClickOnOtherForNonKill = false;
    
    if( tryingToHeal ) {
        canClickOnOtherForNonKill = true;
        }
    
    if( ourLiveObject->holdingID > 0 &&
        getObject( ourLiveObject->holdingID )->deadlyDistance == 0 &&
        ( getObject( ourLiveObject->holdingID )->clothing != 'n' ||
          getObject( ourLiveObject->holdingID )->foodValue > 0 ) ) {
        canClickOnOtherForNonKill = true;
        }
    

    
    // true if we're too far away to use on baby BUT we should execute
    // UBABY once we get to destination

    // if we're close enough to use on baby, we'll use on baby from where 
    // we're standing
    // and return
    char useOnBabyLater = false;
    
    if( p.hitOtherPerson &&
        ! modClick && 
        destID == 0 &&
        canClickOnOtherForNonKill ) {


        doublePair targetPos = { (double)clickDestX, (double)clickDestY };

        for( int i=0; i<gameObjects.size(); i++ ) {
        
            LiveObject *o = gameObjects.getElement( i );
            
            if( o->id != ourID &&
                o->heldByAdultID == -1 ) {

                if( distance( targetPos, o->currentPos ) < 1 ) {
                    // clicked on someone
                    
                    if( ! ourLiveObject->inMotion && 
                        ( isGridAdjacent( clickDestX, clickDestY,
                                        ourLiveObject->xd, 
                                        ourLiveObject->yd ) ||
                          ( clickDestX == ourLiveObject->xd && 
                            clickDestY == ourLiveObject->yd ) ) ) {

                        
                        if( nextActionMessageToSend != NULL ) {
                            delete [] nextActionMessageToSend;
                            nextActionMessageToSend = NULL;
                            }
            
                        nextActionMessageToSend = 
                            autoSprintf( "UBABY %d %d %d %d#",
                                         sendX( clickDestX ), 
                                         sendY( clickDestY ), 
                                         p.hitClothingIndex, 
                                         p.hitOtherPersonID );
                        
                        
                        playerActionTargetX = clickDestX;
                        playerActionTargetY = clickDestY;
                        
                        playerActionTargetNotAdjacent = true;
                        
                        printf( "UBABY with target player %d\n", 
                                p.hitOtherPersonID );

                        return;
                        }
                    else {
                        // too far away, but try to use on baby later,
                        // once we walk there, using standard path-to-adjacent
                        // code below
                        useOnBabyLater = true;
                        
                        break;
                        }
                    }
                }
            }
    
        }




    char tryingToPickUpBaby = false;
    
    double ourAge = computeCurrentAgeNoOverride( ourLiveObject );

    if( destID == 0 &&
        p.hit &&
        p.hitOtherPerson &&
        ! p.hitAnObject &&
        ! modClick && ourLiveObject->holdingID == 0 &&
        // only adults can pick up babies
        ourAge > 13 ) {
        

        doublePair targetPos = { (double)clickDestX, (double)clickDestY };

        for( int i=0; i<gameObjects.size(); i++ ) {
        
            LiveObject *o = gameObjects.getElement( i );
            
            if( o->id != ourID &&
                o->heldByAdultID == -1 ) {

                if( distance( targetPos, o->currentPos ) < 1 ) {
                    // clicked on someone

                    if( computeCurrentAgeNoOverride( o ) < 5 ) {

                        // they're a baby
                        
                        tryingToPickUpBaby = true;
                        break;
                        }
                    }
                }
            }
        }

    
    // for USE actions that specify a slot number
    int useExtraIParam = -1;
	
	// whether this move is short and 
	// an action is gonna be sent shortly
    bool actionMove = false;

    if( !killMode && 
        destID == 0 && !modClick && !tryingToPickUpBaby && !useOnBabyLater && 
        ! ( clickDestX == ourLiveObject->xd && 
            clickDestY == ourLiveObject->yd ) ) {
        // a move to an empty spot where we're not already standing
        // can interrupt current move
        
        mustMove = true;
        }
    else if( ( modClick && 
               // we can right click on an empty tile or full tile if
               // we're holding something
               // we can also right click with empty hand to pick something
               // up
               ( ourLiveObject->holdingID != 0 || 
                 ( destObjInClickedTile > 0 && 
                   ! destObjInClickedTilePermanent ) ) )
             || killMode
             || tryingToPickUpBaby
             || useOnBabyLater
             || destID != 0
             || ( modClick && ourLiveObject->holdingID == 0 &&
                  destNumContained > 0 ) ) {
        // use/drop modifier
        // OR pick up action
            
        
        char canExecute = false;
        char sideAccess = false;
		char noBackAccess = false;
        
        if( destID > 0 && getObject( destID )->sideAccess ) {
            sideAccess = true;
            }
			
        if( destID > 0 && getObject( destID )->noBackAccess ) {
            noBackAccess = true;
            }
        

        // direct click on adjacent cells or self cell?
        if( isGridAdjacent( clickDestX, clickDestY,
                            ourLiveObject->xd, ourLiveObject->yd )
            || 
            ( clickDestX == ourLiveObject->xd && 
              clickDestY == ourLiveObject->yd ) ) {
            
            canExecute = true;
            
            if( sideAccess &&
                ( clickDestY > ourLiveObject->yd ||
                  clickDestY < ourLiveObject->yd ) ) {
                // trying to access side-access object from N or S
                canExecute = false;
                }
            if( noBackAccess &&
                ( clickDestY < ourLiveObject->yd ) ) {
                // trying to access noBackAccess object from N
                canExecute = false;
                }
            }


        if( ! canExecute ) {
            // need to move to empty adjacent first, if it exists
            
            // also consider spot itself in some cases
            
            int nDX[5] = { 0, -1, +1, 0, 0 };
            int nDY[5] = { 0, 0, 0, -1, +1 };
            
            char foundEmpty = false;
            
            int closestDist = 9999999;

            char oldPathExists = ( ourLiveObject->pathToDest != NULL );
            
            // don't consider dest spot itself generally
            int nStart = 1;
            
            int nLimit = 5;
            
            if( sideAccess ) {
                // don't consider N or S neighbors
                nLimit = 3;
                }
            else if( noBackAccess ) {
                // don't consider N neighbor
                nLimit = 4;
                }
            else if( destID > 0 &&
                     ourLiveObject->holdingID == 0 && 
                     getObject( destID )->permanent &&
                     ! getObject( destID )->blocksWalking ) {
                
                TransRecord *handTrans = getTrans( 0, destID );
                
                if( handTrans == NULL ||
                    ( handTrans->newActor != 0 &&
                      getObject( handTrans->newActor )->foodValue > 0 &&
                        handTrans->newTarget != 0 &&
                      ! getObject( handTrans->newTarget )->blocksWalking ) ) {
                    // walk to tile itself if target is permanent
                    // and not blocking, and hand is empty
                    // AND this will result in something still
                    // on the ground (so it's not a transforming pick-up,
                    // like pulling an onion).
                    // and the new thing on the ground is not blocking
                    // (so we're not closing a door)
                    // and what you get in the hand is edible
                    // (example:  picking berries from behind the bush)
                    //
                    // this is the main situation where you'd want to
                    // click the same target and yourself
                    // multiple times in a row, so having yourself
                    // as close as possible to the target matters
                    nStart = 0;
                    }
                }



            for( int n=nStart; n<nLimit; n++ ) {
                int x = mapX + nDX[n];
                int y = mapY + nDY[n];

                if( y >= 0 && y < mMapD &&
                    x >= 0 && x < mMapD ) {
                 
                    
                    int mapI = y * mMapD + x;
                    
                    if( mMap[ mapI ] == 0
                        ||
                        ( mMap[ mapI ] != -1 && 
                          ! getObject( mMap[ mapI ] )->blocksWalking ) ) {
                        
                        int emptyX = clickDestX + nDX[n];
                        int emptyY = clickDestY + nDY[n];

                        // check if there's a path there
                        int oldXD = ourLiveObject->xd;
                        int oldYD = ourLiveObject->yd;
                        
                        // set this temporarily for pathfinding
                        ourLiveObject->xd = emptyX;
                        ourLiveObject->yd = emptyY;
                        
                        computePathToDest( ourLiveObject );
                        
                        if( ourLiveObject->pathToDest != NULL &&
                            ourLiveObject->pathLength < closestDist ) {
                            
                            // can get there
                            
                            moveDestX = emptyX;
                            moveDestY = emptyY;
                            
                            closestDist = ourLiveObject->pathLength;
                            
                            foundEmpty = true;
                            }
                        
                        // restore our old dest
                        ourLiveObject->xd = oldXD;
                        ourLiveObject->yd = oldYD;    

                        if( n == 0 && foundEmpty ) {
                            // always prefer tile itself, if that's an option
                            // based on logic above, even if further
                            break;
                            }
                        }
                    
                    }
                }


            if( !foundEmpty && 
                ! sideAccess &&
                nStart > 0 &&
                destID > 0 &&
                ! getObject( destID )->blocksWalking ) {
                
                // all neighbors blocked
                // we didn't consider tile itself before
                // but now we will, as last resort.

                // consider tile itself as dest
                int oldXD = ourLiveObject->xd;
                int oldYD = ourLiveObject->yd;
                        
                // set this temporarily for pathfinding
                ourLiveObject->xd = clickDestX;
                ourLiveObject->yd = clickDestY;
                        
                computePathToDest( ourLiveObject );
                        
                if( ourLiveObject->pathToDest != NULL  ) {
                            
                    // can get there
                    
                    moveDestX = clickDestX;
                    moveDestY = clickDestY;
                            
                    foundEmpty = true;
                    }
                        
                // restore our old dest
                ourLiveObject->xd = oldXD;
                ourLiveObject->yd = oldYD; 
                }
            
            
            if( oldPathExists ) {
                // restore it
                computePathToDest( ourLiveObject );
                }

            if( foundEmpty ) {
                canExecute = true;
                }
            // always try to move as close as possible, even
            // if we can't actually get close enough to execute action
            mustMove = true;
            }
        

        if( canExecute && ! killMode ) {
			
            actionMove = true;
			
            const char *action = "";
            char *extra = stringDuplicate( "" );
            
            char send = false;
            
            if( tryingToPickUpBaby ) {
                action = "BABY";
                if( p.hitOtherPerson ) {
                    delete [] extra;
                    extra = autoSprintf( " %d", p.hitOtherPersonID );
                    }
                send = true;
                }
            else if( useOnBabyLater ) {
                action = "UBABY";
                delete [] extra;
                extra = autoSprintf( " %d %d", p.hitClothingIndex,
                                     p.hitOtherPersonID );
                send = true;
                }
            else if( modClick && destID == 0 ) {
                
                if( ourLiveObject->holdingID != 0 ) {
                    action = "DROP";
                    nextActionDropping = true;
                    }
                else {
                    action = "USE";
                    nextActionDropping = false;
                    }
                
                send = true;

                // check for other special case
                // a use-on-ground transition or use-on-floor transition

                if( ourLiveObject->holdingID > 0 ) {
                        
                    ObjectRecord *held = 
                        getObject( ourLiveObject->holdingID );
                        
                    char foundAlt = false;
                        
                    if( held->foodValue == 0 &&
                        destObjInClickedTile == 0 ) {
                        // a truly empty spot where use-on-bare-ground
                        // can happen

                        TransRecord *r = 
                            getTrans( ourLiveObject->holdingID,
                                      -1 );
                            
                        if( r != NULL &&
                            r->newTarget != 0 ) {
                                
                            // a use-on-ground transition exists!
                                
                            // override the drop action
                            action = "USE";
                            foundAlt = true;
                            }
                        }

                    if( !foundAlt && floorDestID > 0 ) {
                        // check if use on floor exists
                        TransRecord *r = 
                            getTrans( ourLiveObject->holdingID, 
                                      floorDestID );
                                
                        if( r != NULL ) {
                            // a use-on-floor transition exists!
                                
                            // override the drop action
                            action = "USE";
                                
                            }
                        }
                    }
                }
            else if( destID != 0 &&
                     ( modClick || 
                       ( getObject( destID )->permanent &&
                         getTrans( 0, destID ) == NULL
                         ) 
                       )
                     && ourLiveObject->holdingID == 0 &&
                     getNumContainerSlots( destID ) > 0 ) {
                
                // for permanent container objects that have no bare-hand
                // transition , we shouldn't make
                // distinction between left and right click

                action = "REMV";

                if( ! modClick ) {
                    // no bare-hand action
                    // but check if this object decays in 1 second
                    // and if so, a bare-hand action applies after that
                    TransRecord *decayTrans = getTrans( -1, destID );
                    if( decayTrans != NULL &&
                        decayTrans->newTarget > 0 &&
                        decayTrans->autoDecaySeconds == 1 ) {
                        
                        if( getTrans( 0, decayTrans->newTarget ) != NULL ) {
                            // switch to USE in this case
                            // because server will force object to decay
                            // so a transition can go through
                            action = "USE";
                            }
                        }
                    }
                else {
                    // in case of mod-click, if we clicked a contained item
                    // directly, and it has a bare hand transition,
                    // consider doing that as a USE
                    ObjectRecord *destObj = getObject( destID );
                    
                    if( destObj->numSlots > p.hitSlotIndex &&
                        strstr( destObj->description, "+useOnContained" )
                        != NULL ) {
                        action = "USE";
                        useExtraIParam = p.hitSlotIndex;
                        }
                    }
                

                send = true;
                delete [] extra;
                extra = autoSprintf( " %d", p.hitSlotIndex );
                }
            else if( modClick && ourLiveObject->holdingID != 0 &&
                     destID != 0 &&
                     getNumContainerSlots( destID ) > 0 &&
                     destNumContained <= getNumContainerSlots( destID ) ) {
                action = "DROP";
                nextActionDropping = true;
                send = true;
                }
            else if( modClick && ourLiveObject->holdingID != 0 &&
                     destID != 0 &&
                     getNumContainerSlots( destID ) == 0 &&
                     ! getObject( destID )->permanent ) {
                action = "DROP";
                nextActionDropping = true;
                send = true;
                }
            else if( destID != 0 ) {
                // allow right click and default to USE if nothing
                // else applies
                action = "USE";

                // check for case where both bare-hand transition
                // AND pickup applies
                // use mod-click to differentiate between two possibilities
                if( ourLiveObject->holdingID == 0 &&
                    getNumContainerSlots( destID ) == 0 &&
                    ! getObject( destID )->permanent ) {
                    
                    TransRecord *bareHandTrans = getTrans( 0, destID );
                    
                    if( bareHandTrans != NULL &&
                        bareHandTrans->newTarget != 0 ) {
                        
                        // bare hand trans exists, and it's NOT just a
                        // direct "pickup" trans that should always be applied
                        // (from target to new actor)
                        // The bare hand trans leaves something on the ground
                        // meaning that it is transformational (removing
                        // a plate from a stack, tweaking something, etc.)

                        if( modClick ) {
                            action = "USE";
                            }
                        else {
                            action = "REMV";

                             delete [] extra;
                             extra = stringDuplicate( " 0" );
                            }
                        }
                    }
                else if( ourLiveObject->holdingID > 0 &&
                         p.hitSlotIndex != -1 &&
                         getNumContainerSlots( destID ) > p.hitSlotIndex ) {
                    
                    // USE on a slot?  Only if allowed by container

                    ObjectRecord *destObj = getObject( destID );
                    
                    if( strstr( destObj->description, "+useOnContained" )
                        != NULL ) {
                        useExtraIParam = p.hitSlotIndex;
                        }
                    }
                
                send = true;
                }
            
            if( strcmp( action, "DROP" ) == 0 ) {
                delete [] extra;
                nextActionDropping = true;
                extra = stringDuplicate( " -1" );
                }

            if( strcmp( action, "USE" ) == 0 &&
                destID > 0 ) {
                // optional ID param for USE, specifying that we clicked
                // on something
                delete [] extra;
                
                if( useExtraIParam != -1 ) {
                    extra = autoSprintf( " %d %d", destID, useExtraIParam );
                    }
                else {
                    extra = autoSprintf( " %d", destID );
                    }
                }
            
            
            if( send ) {
                // queue this until after we are done moving, if we are
                nextActionMessageToSend = 
                    autoSprintf( "%s %d %d%s#", action,
                                 sendX( clickDestX ), 
                                 sendY( clickDestY ), extra );
                
                int trueClickDestX = clickDestX;
                int trueClickDestY = clickDestY;
                
                if( mapY >= 0 && mapY < mMapD &&
                    mapX >= 0 && mapX < mMapD ) {
                    
                    int mapI = mapY * mMapD + mapX;
                    
                    if( mMapMoveSpeeds[ mapI ] > 0 ) {        
                        
                        trueClickDestX = 
                            lrint( clickDestX + mMapMoveOffsets[mapI].x );
                        trueClickDestY = 
                            lrint( clickDestY + mMapMoveOffsets[mapI].y );
                        }
                    }

                ourLiveObject->actionTargetX = clickDestX;
                ourLiveObject->actionTargetY = clickDestY;

                ourLiveObject->actionTargetTweakX = trueClickDestX - clickDestX;
                ourLiveObject->actionTargetTweakY = trueClickDestY - clickDestY;
                

                playerActionTargetX = clickDestX;
                playerActionTargetY = clickDestY;
                }

            delete [] extra;
            }
        }
    


    
    if( mustMove ) {
        
        int oldXD = ourLiveObject->xd;
        int oldYD = ourLiveObject->yd;
        
        ourLiveObject->xd = moveDestX;
        ourLiveObject->yd = moveDestY;
        ourLiveObject->destTruncated = false;

        ourLiveObject->inMotion = true;


        GridPos *oldPathToDest = NULL;
        int oldPathLength = 0;
        int oldCurrentPathStep = 0;
        
        if( ourLiveObject->pathToDest != NULL ) {
            oldPathLength = ourLiveObject->pathLength;
            oldPathToDest = new GridPos[ oldPathLength ];

            memcpy( oldPathToDest, ourLiveObject->pathToDest,
                    sizeof( GridPos ) * ourLiveObject->pathLength );
            oldCurrentPathStep = ourLiveObject->currentPathStep;
            }
        

        computePathToDest( ourLiveObject );

        
        if( ourLiveObject->pathToDest == NULL ) {
            // adjust move to closest possible
            ourLiveObject->xd = ourLiveObject->closestDestIfPathFailedX;
            ourLiveObject->yd = ourLiveObject->closestDestIfPathFailedY;
            ourLiveObject->destTruncated = false;
            
            if( ourLiveObject->xd == oldXD && ourLiveObject->yd == oldYD ) {
                // completely blocked in, no path at all toward dest
                
                if( oldPathToDest != NULL ) {
                    // restore it
                    ourLiveObject->pathToDest = oldPathToDest;
                    ourLiveObject->pathLength = oldPathLength;
                    ourLiveObject->currentPathStep = oldCurrentPathStep;
                    oldPathToDest = NULL;
                    }
                
                    

                // ignore click
                
                if( nextActionMessageToSend != NULL ) {
                    delete [] nextActionMessageToSend;
                    nextActionMessageToSend = NULL;
                    }
                ourLiveObject->inMotion = false;
                return;
                }

            if( oldPathToDest != NULL ) {
                delete [] oldPathToDest;
                oldPathToDest = NULL;
                }
            

            computePathToDest( ourLiveObject );
            
            if( ourLiveObject->pathToDest == NULL &&
                ourLiveObject->useWaypoint ) {
                // waypoint itself may be blocked
                // try again with no waypoint at all
                ourLiveObject->useWaypoint = false;
                computePathToDest( ourLiveObject );
                }

            if( ourLiveObject->pathToDest == NULL ) {
                // this happens when our curPos is slightly off of xd,yd
                // but not a full cell away

                // make a fake path
                doublePair dest = { (double) ourLiveObject->xd,
                                    (double) ourLiveObject->yd };
                
                doublePair dir = normalize( sub( dest, 
                                                 ourLiveObject->currentPos ) );

                // fake start, one grid step away
                doublePair fakeStart = dest;

                if( fabs( dir.x ) > fabs( dir.y ) ) {
                    
                    if( dir.x < 0 ) {
                        fakeStart.x += 1;
                        }
                    else {
                        fakeStart.x -= 1;
                        }
                    }
                else {
                    if( dir.y < 0 ) {
                        fakeStart.y += 1;
                        }
                    else {
                        fakeStart.y -= 1;
                        }
                    
                    }
                
                
                ourLiveObject->pathToDest = new GridPos[2];
                
                ourLiveObject->pathToDest[0].x = (int)fakeStart.x;
                ourLiveObject->pathToDest[0].y = (int)fakeStart.y;
                
                ourLiveObject->pathToDest[1].x = ourLiveObject->xd;
                ourLiveObject->pathToDest[1].y = ourLiveObject->yd;
                
                ourLiveObject->pathLength = 2;
                ourLiveObject->currentPathStep = 0;
                ourLiveObject->numFramesOnCurrentStep = 0;
                ourLiveObject->onFinalPathStep = false;
                
                ourLiveObject->currentMoveDirection =
                    normalize( 
                        sub( gridToDouble( ourLiveObject->pathToDest[1] ), 
                             gridToDouble( ourLiveObject->pathToDest[0] ) ) );
                }

            if( ourLiveObject->xd == oldXD 
                &&
                ourLiveObject->yd == oldYD ) {
                
                // truncated path is where we're already going
                // don't send new move message
                return;
                }


            moveDestX = ourLiveObject->xd;
            moveDestY = ourLiveObject->yd;
            
            if( nextActionMessageToSend != NULL ) {
                // abort the action, because we can't reach the spot we
                // want to reach
                delete [] nextActionMessageToSend;
                nextActionMessageToSend = NULL;
                }
            
            }
        
        
        if( oldPathToDest != NULL ) {
            delete [] oldPathToDest;
            oldPathToDest = NULL;
            }
            
		if( drunkEmotionIndex != -1 &&
			ourLiveObject->currentEmot != NULL &&
			strcmp( ourLiveObject->currentEmot->triggerWord, 
			getEmotion( drunkEmotionIndex )->triggerWord ) == 0 ) {
			drunkWalk( (ourLiveObject->pathToDest), ourLiveObject->pathLength, actionMove );
			ourLiveObject->xd = ourLiveObject->pathToDest[ ourLiveObject->pathLength - 1 ].x;
			ourLiveObject->yd = ourLiveObject->pathToDest[ ourLiveObject->pathLength - 1 ].y;
			}

        // send move right away
        //Thread::staticSleep( 2000 );
        SimpleVector<char> moveMessageBuffer;
        
        moveMessageBuffer.appendElementString( "MOVE" );
        ourLiveObject->lastMoveSequenceNumber ++;
        
        // start is absolute
        char *startString = 
            autoSprintf( " %d %d @%d", 
                         sendX( ourLiveObject->pathToDest[0].x ),
                         sendY( ourLiveObject->pathToDest[0].y ),
                         ourLiveObject->lastMoveSequenceNumber );
        moveMessageBuffer.appendElementString( startString );
        delete [] startString;
        
        for( int i=1; i<ourLiveObject->pathLength; i++ ) {
            // rest are relative to start
            char *stepString = autoSprintf( " %d %d", 
                                         ourLiveObject->pathToDest[i].x
                                            - ourLiveObject->pathToDest[0].x,
                                         ourLiveObject->pathToDest[i].y
                                            - ourLiveObject->pathToDest[0].y );
            
            moveMessageBuffer.appendElementString( stepString );
            delete [] stepString;
            }
        moveMessageBuffer.appendElementString( "#" );
        

        char *message = moveMessageBuffer.getElementString();

        sendToServerSocket( message );


        delete [] message;

        // start moving before we hear back from server



        double floorSpeedMod = computePathSpeedMod( ourLiveObject,
                                                    ourLiveObject->pathLength );
        

        ourLiveObject->moveTotalTime = 
            measurePathLength( ourLiveObject, ourLiveObject->pathLength ) / 
            ( ourLiveObject->lastSpeed * floorSpeedMod );

        ourLiveObject->moveEtaTime = game_getCurrentTime() +
            ourLiveObject->moveTotalTime;

            
        updateMoveSpeed( ourLiveObject );
        }    
}


void LivingLifePage::pointerDrag( float inX, float inY )
{
    lastMouseX = inX;
    lastMouseY = inY;
    getLastMouseScreenPos( &lastScreenMouseX, &lastScreenMouseY );
    
    if( showBugMessage ) {
        return;
        }
}


void LivingLifePage::pointerUp( float inX, float inY )
{
    lastMouseX = inX;
    lastMouseY = inY;

    if( showBugMessage ) {
        return;
        }
    

    if( mServerSocket == -1 ) {
        // dead
        return;
        }

    if( mFirstServerMessagesReceived != 3 || ! mDoneLoadingFirstObjectSet ) {
        return;
        }

    if( playerActionPending ) {
        // block further actions until update received to confirm last
        // action
        mouseDown = false;
        return;
        }

    if( mouseDown && 
        getOurLiveObject()->inMotion 
        &&
        mouseDownFrames >  
        minMouseDownFrames / frameRateFactor ) {
        
        // treat the up as one final click (at closest next path position
        // to where they currently are)
        
        LiveObject *o = getOurLiveObject();

        if( o->pathToDest != NULL &&
            o->currentPathStep < o->pathLength - 2 ) {
            GridPos p = o->pathToDest[ o->currentPathStep + 1 ];
        
            pointerDown( p.x * CELL_D, p.y * CELL_D );
            }
        

        // don't do this for now, because it's confusing
        // pointerDown( inX, inY );
        }

    mouseDown = false;


    // clear mouse over cell
    mPrevMouseOverCells.push_back( mCurMouseOverCell );
    mPrevMouseOverCellFades.push_back( mCurMouseOverCellFade );
    mCurMouseOverCell.x = -1;
    mCurMouseOverCell.y = -1;
}


void LivingLifePage::keyDown( unsigned char inASCII )
{
    
    registerTriggerKeyCommand( inASCII, this );


    if( mServerSocket == -1 ) {
        // dead
        return;
        }

    if( showBugMessage ) {
        if( inASCII == '%' ) {
            showBugMessage = false;
            }
        return;
        }

	bool commandKey = isCommandKeyDown();
	bool shiftKey = isShiftKeyDown();

	if ( SettingsManager::getIntSetting( "keyboardActions", 1 ) ) {
		if (! mSayField.isFocused()) {
			if (!commandKey && !shiftKey && inASCII == 27) { // ESCAPE KEY
				upKeyDown = false;
				leftKeyDown = false;
				downKeyDown = false;
				rightKeyDown = false;
				lastPosX = 9999;
				lastPosY = 9999;
			}

			if (commandKey) {
				if (isCharKey(inASCII, charKey_TileStandingOn)) {
					actionBetaRelativeToMe( 0, 0 );
					return;
				}
			} else {
				if (isCharKey(inASCII, charKey_TileStandingOn)) {
					actionAlphaRelativeToMe( 0, 0 );
					return;
				}
			}
			
			if (!shiftKey && !commandKey) {
				if (inASCII == charKey_Up || inASCII == toupper(charKey_Up)) {
					upKeyDown = true;
					//stopAutoRoadRun = true;
					return;
				}
				if (inASCII == charKey_Left || inASCII == toupper(charKey_Left)) {
					leftKeyDown = true;
					//stopAutoRoadRun = true;
					return;
				}
				if (inASCII == charKey_Down || inASCII == toupper(charKey_Down)) {
					downKeyDown = true;
					//stopAutoRoadRun = true;
					return;
				}
				if (inASCII == charKey_Right || inASCII == toupper(charKey_Right)) {
					rightKeyDown = true;
					//stopAutoRoadRun = true;
					return;
				}
			} else if (commandKey) {
				if (inASCII+64 == toupper(charKey_Up)) {
					actionBetaRelativeToMe( 0, 1 );
					return;
				}
				if (inASCII+64 == toupper(charKey_Left)) {
					actionBetaRelativeToMe( -1, 0 );
					return;
				}
				if (inASCII+64 == toupper(charKey_Down)) {
					actionBetaRelativeToMe( 0, -1 );
					return;
				}
				if (inASCII+64 == toupper(charKey_Right)) {
					actionBetaRelativeToMe( 1, 0 );
					return;
				}
			} else if (shiftKey) {
				if (inASCII == charKey_Up || inASCII == toupper(charKey_Up)) {
					actionAlphaRelativeToMe( 0, 1 );
					return;
				}
				if (inASCII == charKey_Left || inASCII == toupper(charKey_Left)) {
					actionAlphaRelativeToMe( -1, 0 );
					return;
				}
				if (inASCII == charKey_Down || inASCII == toupper(charKey_Down)) {
					actionAlphaRelativeToMe( 0, -1 );
					return;
				}
				if (inASCII == charKey_Right || inASCII == toupper(charKey_Right)) {
					actionAlphaRelativeToMe( 1, 0 );
					return;
				}
			}

			if (!shiftKey && isCharKey(inASCII, charKey_Backpack)) {
				useBackpack();
				return;
			}
			if ((shiftKey || commandKey) && isCharKey(inASCII, charKey_Backpack)) {
				useBackpack(true);
				return;
			}
			if (isCharKey(inASCII, charKey_Eat)) {
				useOnSelf();
				return;
			}
			if (isCharKey(inASCII, charKey_Baby)) {
				pickUpBabyInRange();
				return;
			}
			if (!commandKey && !shiftKey && isCharKey(inASCII, charKey_TakeOffBackpack)) {
				takeOffBackpack();
				return;
			}
			if (shiftKey && isCharKey(inASCII, charKey_Pocket)) {
				usePocket(1);
				return;
			}
			if (!shiftKey && isCharKey(inASCII, charKey_Pocket)) {
				usePocket(4);
				return;
			}
		}
	}

	if (minitech::livingLifeKeyDown(inASCII)) return;
    
    switch( inASCII ) {
        /*
        // useful for testing
        case '+':
            getOurLiveObject()->displayID = getRandomPersonObject();
            break;
        case '_':
            getOurLiveObject()->age += 10;
            break;
        case '-':
            getOurLiveObject()->age -= 5;
            break;
        case '~':
            getOurLiveObject()->age -= 1;
            break;
        */
        case 'V':
            if( ! mSayField.isFocused() &&
                serverSocketConnected &&
                SettingsManager::getIntSetting( "vogModeOn", 0 ) ) {
                
                if( ! vogMode ) {
                    sendToServerSocket( (char*)"VOGS 0 0#" );
                    vogMode = true;
                    vogPos = getOurLiveObject()->currentPos;
                    vogPickerOn = false;
                    mObjectPicker.setPosition( vogPos.x * CELL_D + 510,
                                               vogPos.y * CELL_D + 90 );
                    
                    // jump camp instantly
                    lastScreenViewCenter.x = vogPos.x * CELL_D;
                    lastScreenViewCenter.y = vogPos.y * CELL_D;
                    setViewCenterPosition( lastScreenViewCenter.x,
                                           lastScreenViewCenter.y );
                    }
                else {
                    sendToServerSocket( (char*)"VOGX 0 0#" );
                    vogMode = false;
                    if( vogPickerOn ) {
                        removeComponent( &mObjectPicker );
                        mObjectPicker.removeActionListener( this );
                        }
                    vogPickerOn = false;
                    }
                }
            break;
        case 'I':
            if( ! mSayField.isFocused() &&
                vogMode ) {
                
                if( ! vogPickerOn ) {
                    addComponent( &mObjectPicker );
                    mObjectPicker.addActionListener( this );
                    }
                else {
                    removeComponent( &mObjectPicker );
                    mObjectPicker.removeActionListener( this );    
                    }
                vogPickerOn = ! vogPickerOn;
                }
            break;
        case 'N':
            if( ! mSayField.isFocused() &&
                serverSocketConnected &&
                vogMode ) {
                sendToServerSocket( (char*)"VOGP 0 0#" );
                }
            break;
        case 'M':
            if( ! mSayField.isFocused() &&
                serverSocketConnected &&
                vogMode ) {
                sendToServerSocket( (char*)"VOGN 0 0#" );
                }
            break;
        case 'S':
            if( savingSpeechEnabled && 
                ! mSayField.isFocused() ) {
                savingSpeechColor = true;
                savingSpeechMask = false;
                savingSpeech = true;
                }
            break;
        case 'x':
            if( userTwinCode != NULL &&
                ! mStartedLoadingFirstObjectSet ) {
                
                closeSocket( mServerSocket );
                mServerSocket = -1;
                
                setWaiting( false );
                setSignal( "twinCancel" );
                }
            break;
        /*
        case 'b':
            blackBorder = true;
            whiteBorder = false;
            break;
        case 'B':
            blackBorder = false;
            whiteBorder = true;
            break;
        */
        /*
        case 'a':
            drawAdd = ! drawAdd;
            break;
        case 'm':
            drawMult = ! drawMult;
            break;
        case 'n':
            multAmount += 0.05;
            printf( "Mult amount = %f\n", multAmount );
            break;
        case 'b':
            multAmount -= 0.05;
            printf( "Mult amount = %f\n", multAmount );
            break;
        case 's':
            addAmount += 0.05;
            printf( "Add amount = %f\n", addAmount );
            break;
        case 'd':
            addAmount -= 0.05;
            printf( "Add amount = %f\n", addAmount );
            break;
        */
        /*
        case 'd':
            pathStepDistFactor += .1;
            printf( "Path step dist factor = %f\n", pathStepDistFactor );
            break;
        case 'D':
            pathStepDistFactor += .5;
            printf( "Path step dist factor = %f\n", pathStepDistFactor );
            break;
        case 's':
            pathStepDistFactor -= .1;
            printf( "Path step dist factor = %f\n", pathStepDistFactor );
            break;
        case 'S':
            pathStepDistFactor -= .5;
            printf( "Path step dist factor = %f\n", pathStepDistFactor );
            break;
        case 'f':
            trail.deleteAll();
            trailColors.deleteAll();
            break;
        */
        case 'e':
        case 'E':
            if( ! mSayField.isFocused() ) {
                mEKeyDown = true;
                }
            break;
        case 'h':
        case 'H':
            if( ! mSayField.isFocused() && ! vogMode ) {
                showHelp = ! showHelp;
                }
            break;
        case 'z':
        case 'Z':
            if( mUsingSteam && ! mSayField.isFocused() ) {
                mZKeyDown = true;
                }
            break;
        case ' ':
		if( ! mSayField.isFocused() && ! SettingsManager::getIntSetting( "keyboardActions", 1 ) ) {
                shouldMoveCamera = false;
                }
            break;
        case 'f':
        case 'F':
            if( ! mSayField.isFocused() && SettingsManager::getIntSetting( "keyboardActions", 1 ) ) {
                shouldMoveCamera = false;
                }
            break;
        case 96: { // grave
            gui_hud_mode = SettingsManager::getIntSetting( "hudDrawMode", 0 );
            gui_hud_mode = abs( ( gui_hud_mode + 1 ) % 3 );
            SettingsManager::setSetting( "hudDrawMode", gui_hud_mode );
			calcOffsetHUD();
            }
            break;
        case 9: // tab
            if( mCurrentHintObjectID != 0 ) {
                
                int num = getNumHints( mCurrentHintObjectID );
                
                int skip = 1;
                
                if( !mUsingSteam && isShiftKeyDown() ) {
                    skip = -1;
                    }
                else if( mUsingSteam && mZKeyDown ) {
                    skip = -1;
                    }
                if( isCommandKeyDown() ) {
                    if( num > 5 ) {
                        skip *= 5;
                        }
                    }

                if( num > 0 ) {
                    mNextHintIndex += skip;
                    if( mNextHintIndex >= num ) {
                        mNextHintIndex -= num;
                        }
                    if( mNextHintIndex < 0 ) {
                        mNextHintIndex += num;
                        }
                    }
                }
            break;
        case '/':
            if( ! mSayField.isFocused() ) {
                // start typing a filter
                mSayField.setText( "/" );
                mSayField.focus();
                }
            break;
        case 13:  // enter
            // speak
            if( ! TextField::isAnyFocused() ) {
                
                mSayField.setText( "" );
                mSayField.focus();
                }
            else if( mSayField.isFocused() ) {
                char *typedText = mSayField.getText();
                
                
                // tokenize and then recombine with single space
                // between each.  This will get rid of any hidden
                // lead/trailing spaces, which may confuse server
                // for baby naming, etc.
                // also eliminates all-space strings, which 
                // end up in speech history, but are invisible
                SimpleVector<char *> *tokens = 
                    tokenizeString( typedText );
                
                if( tokens->size() > 0 ) {
                    char *oldTypedText = typedText;
                    
                    char **strings = tokens->getElementArray();
                    
                    typedText = join( strings, tokens->size(), " " );
                    
                    delete [] strings;
                    
                    delete [] oldTypedText;
                    }
                else {
                    // string with nothing but spaces, or empty
                    // force it empty
                    delete [] typedText;
                    typedText = stringDuplicate( "" );
                    }
                
                tokens->deallocateStringElements();
                delete tokens;
                    
                

                if( strcmp( typedText, "" ) == 0 ) {
                    mSayField.unfocus();
                    }
                else {
                    
                    if( strlen( typedText ) > 0 &&
                         typedText[0] == '/' ) {
                    
                        // a command, don't send to server
                        
                        const char *filterCommand = "/";
                        
                        if( strstr( typedText, filterCommand ) == typedText ) {
                            // starts with filter command
                            
                            // LiveObject *ourLiveObject = getOurLiveObject();
                            
                            int emotIndex = getEmotionIndex( typedText );
                            
                            if( emotIndex != -1 ) {
                                char *message = 
                                    autoSprintf( "EMOT 0 0 %d#", emotIndex );
                                
                                sendToServerSocket( message );
                                delete [] message;
                                }
                            else if( strstr( typedText,
                                             translate( "dieCommand" ) ) 
                                     == typedText ) {
                                // die command issued from baby
                                char *message = 
                                    autoSprintf( "DIE 0 0#" );
                                
                                sendToServerSocket( message );
                                delete [] message;
                                }
                            else if( strstr( typedText,
                                             translate( "fpsCommand" ) ) 
                                     == typedText ) {
                                showFPS = !showFPS;
                                frameBatchMeasureStartTime = -1;
                                framesInBatch = 0;
                                fpsToDraw = -1;
                                }
                            else if( strstr( typedText,
                                             translate( "netCommand" ) ) 
                                     == typedText ) {
                                showNet = !showNet;
                                netBatchMeasureStartTime = -1;
                                messagesInPerSec = -1;
                                messagesOutPerSec = -1;
                                bytesInPerSec = -1;
                                bytesOutPerSec = -1;
                                messagesInCount = 0;
                                messagesOutCount = 0;
                                bytesInCount = 0;
                                bytesOutCount = 0;
                                }
                            else if( strstr( typedText,
                                             translate( "pingCommand" ) ) 
                                     == typedText ) {

                                waitingForPong = true;
                                lastPingSent ++;
                                char *pingMessage = 
                                    autoSprintf( "PING 0 0 %d#", lastPingSent );
                                
                                sendToServerSocket( pingMessage );
                                delete [] pingMessage;

                                showPing = true;
                                pingSentTime = game_getCurrentTime();
                                pongDeltaTime = -1;
                                pingDisplayStartTime = -1;
                                }
                            else if( strstr( typedText,
                                             translate( "disconnectCommand" ) ) 
                                     == typedText ) {
                                forceDisconnect = true;
                                }
                            else if( strstr( typedText,
                                             translate( "helpCommand" ) ) 
                                     == typedText ) {
                                showHelp = ! showHelp;
                                }
                            else {
                                // filter hints
                                char *filterString = 
                                    &( typedText[ strlen( filterCommand ) ] );
                                
                                
                                if( mHintFilterString != NULL ) {
                                    delete [] mHintFilterString;
                                    mHintFilterString = NULL;
                                    }
                                
                                char *trimmedFilterString = 
                                    trimWhitespace( filterString );
                                
                                int filterStringLen = 
                                    strlen( trimmedFilterString );
							
                                if( filterStringLen > 0 ) {
                                    // not blank
                                    mHintFilterString = 
                                        stringDuplicate( trimmedFilterString );
										
									minitech::inputHintStrToSearch( mHintFilterString );
                                    }
								else {
									minitech::inputHintStrToSearch( "" );;
								}
                            
                                delete [] trimmedFilterString;
                            
                                mForceHintRefresh = true;
                                mNextHintIndex = 0;
                                }
                            }
                        }
                    else {
                        // send text to server

                        const char *sayCommand = "SAY";
                        
                        if( vogMode ) {
                            sayCommand = "VOGT";
                            }
                        
                        char *message = 
                            autoSprintf( "%s 0 0 %s#",
                                         sayCommand, typedText );
                        sendToServerSocket( message );
                        delete [] message;
                        }
                    
                    for( int i=0; i<mSentChatPhrases.size(); i++ ) {
                        if( strcmp( 
                                typedText, 
                                mSentChatPhrases.getElementDirect(i) ) 
                            == 0 ) {
                            
                            delete [] mSentChatPhrases.getElementDirect(i);
                            mSentChatPhrases.deleteElement(i);
                            i--;
                            }
                        }

                    mSentChatPhrases.push_back( stringDuplicate( typedText ) );

                    mSayField.setText( "" );
                    mSayField.unfocus();
                    }
                
                delete [] typedText;
                }
            else if( vogMode ) {
                // return to cursor control
                TextField::unfocusAll();
                }
            break;
        }
}



void LivingLifePage::specialKeyDown( int inKeyCode )
{
    if( showBugMessage ) {
        return;
        }
    
    if( mServerSocket == -1 ) {
        // dead
        return;
        }
		
    //FOV
    if( inKeyCode == MG_KEY_F1) {
        sendToServerSocket( (char*)"EMOT 0 0 0#" );
        return;
        }
    if( inKeyCode == MG_KEY_F2) {
        sendToServerSocket( (char*)"EMOT 0 0 1#" );
        return;
        }
    if( inKeyCode == MG_KEY_F3) {
        sendToServerSocket( (char*)"EMOT 0 0 2#" );
        return;
        }
    if( inKeyCode == MG_KEY_F4) {
        sendToServerSocket( (char*)"EMOT 0 0 3#" );
        return;
        }
    if( inKeyCode == MG_KEY_F5) {
        sendToServerSocket( (char*)"EMOT 0 0 4#" );
        return;
        }
    if( inKeyCode == MG_KEY_F6) {
        sendToServerSocket( (char*)"EMOT 0 0 5#" );
        return;
        }
    if( inKeyCode == MG_KEY_F7) {
        sendToServerSocket( (char*)"EMOT 0 0 6#" );
        return;
        }
	if( ( inKeyCode == MG_KEY_LEFT || 
		inKeyCode == MG_KEY_RIGHT ) && ! vogMode ) {
		float currentScale = SettingsManager::getFloatSetting( "fovScale", 1.0f );
		float newScale = ( inKeyCode == MG_KEY_LEFT ) ? currentScale -= 0.25f : currentScale += 0.25f;
        if ( isShiftKeyDown() ) {
            newScale = ( inKeyCode == MG_KEY_LEFT ) ? SettingsManager::getFloatSetting( "fovDefault", 1.25f ) : SettingsManager::getFloatSetting( "fovMax", 2.25f );
            }
        if ( isCommandKeyDown() ) {
            float currentHUDScale = SettingsManager::getFloatSetting( "fovScaleHUD", 1.25f );
            newScale = ( inKeyCode == MG_KEY_LEFT ) ? currentHUDScale -= 0.25f : currentHUDScale += 0.25f;
            changeHUDFOV( newScale );
        } else {
		    if ( SettingsManager::getIntSetting( "fovEnabled", 1 ) ) changeFOV( newScale );
            }
		return;
	    }

    if( vogMode && ! TextField::isAnyFocused() ) {
        GridPos posOffset = { 0, 0 };
        
        int jump = 1;
        
        if( isCommandKeyDown() ) {
            jump *= 2;
            }
        if( isShiftKeyDown() ) {
            jump *= 5;
            }

        if( inKeyCode == MG_KEY_UP ) {
            posOffset.y = jump;
            }
        else if( inKeyCode == MG_KEY_DOWN ) {
            posOffset.y = -jump;
            }
        else if( inKeyCode == MG_KEY_LEFT ) {
            posOffset.x = -jump;
            }
        else if( inKeyCode == MG_KEY_RIGHT ) {
            posOffset.x = jump;
            }

        if( posOffset.x != 0 || posOffset.y != 0 ) {
            
            GridPos newPos;
            newPos.x = vogPos.x;
            newPos.y = vogPos.y;
            
            newPos.x += posOffset.x;
            newPos.y += posOffset.y;
            
            char *message = autoSprintf( "VOGM %d %d#",
                                         newPos.x, newPos.y );
            sendToServerSocket( message );
            delete [] message;
            }
        }
    else if( inKeyCode == MG_KEY_UP ||
        inKeyCode == MG_KEY_DOWN ) {
        if( ! mSayField.isFocused() && inKeyCode == MG_KEY_UP ) {
            if( mSentChatPhrases.size() > 0 ) {
                mSayField.setText( 
                    mSentChatPhrases.getElementDirect( 
                        mSentChatPhrases.size() - 1 ) );
                }
            else {
                mSayField.setText( "" );
                }
            mSayField.focus();
            }
        else {
            char *curText = mSayField.getText();
                
            int curBufferIndex = -1;
            
            if( strcmp( curText, "" ) != 0 ) {
                for( int i=mSentChatPhrases.size()-1; i>=0; i-- ) {
                    if( strcmp( curText, mSentChatPhrases.getElementDirect(i) )
                        == 0 ) {
                        curBufferIndex = i;
                        break;
                        }
                    }
                }
            delete [] curText;
            
            int newIndex = mSentChatPhrases.size();
            
            if( curBufferIndex != -1 ) {
                newIndex = curBufferIndex;
                
                }

            if( inKeyCode == MG_KEY_UP ) {
                newIndex --;
                }
            else {
                newIndex ++;
                }

            
            if( newIndex >= 0 ) {
                
                if( mCurrentNoteChars.size() > 0 ) {
                    // fade older erased chars 

                    for( int i=0; i<mErasedNoteCharFades.size(); i++ ) {
                        if( mErasedNoteCharFades.getElementDirect( i ) > 0.5 ) {
                            *( mErasedNoteCharFades.getElement( i ) ) -= 0.2;
                            }
                        else {
                            *( mErasedNoteCharFades.getElement( i ) ) -= 0.1;
                            }
                        
                        if( mErasedNoteCharFades.getElementDirect( i ) <= 0 ) {
                            mErasedNoteCharFades.deleteElement( i );
                            mErasedNoteChars.deleteElement( i );
                            mErasedNoteCharOffsets.deleteElement( i );
                            i--;
                            }
                        }
                    }
                
                // first, remove exact duplicates from erased
                for( int i=0; i<mCurrentNoteChars.size(); i++ ) {
                    char c = mCurrentNoteChars.getElementDirect( i );
                    doublePair pos = 
                        mCurrentNoteCharOffsets.getElementDirect( i );
                    
                    for( int j=0; j<mErasedNoteChars.size(); j++ ) {
                        if( mErasedNoteChars.getElementDirect(j) == c 
                            &&
                            equal( mErasedNoteCharOffsets.getElementDirect(j),
                                   pos ) ) {
                            
                            mErasedNoteChars.deleteElement( j );
                            mErasedNoteCharOffsets.deleteElement( j );
                            mErasedNoteCharFades.deleteElement( j );
                            j--;
                            }
                        }
                    }
                

                for( int i=0; i<mCurrentNoteChars.size(); i++ ) {
                    mErasedNoteChars.push_back( 
                        mCurrentNoteChars.getElementDirect( i ) );
                    
                    mErasedNoteCharOffsets.push_back(
                        mCurrentNoteCharOffsets.getElementDirect( i ) );
                    
                    mErasedNoteCharFades.push_back( 1.0f );
                    }
                
                if( newIndex >= mSentChatPhrases.size() ) {
                    mSayField.setText( "" );
                    }
                else {
                    mSayField.setText( 
                        mSentChatPhrases.getElementDirect( newIndex ) );
                    }
                }
            }
        }
}

            
void LivingLifePage::keyUp( unsigned char inASCII ) {

	bool commandKey = isCommandKeyDown();

	if (inASCII == charKey_Up || inASCII == toupper(charKey_Up)) {
		upKeyDown = false;
	}
	if (inASCII == charKey_Left || inASCII == toupper(charKey_Left)) {
		leftKeyDown = false;
	}
	if (inASCII == charKey_Down || inASCII == toupper(charKey_Down)) {
		downKeyDown = false;
	}
	if (inASCII == charKey_Right || inASCII == toupper(charKey_Right)) {
		rightKeyDown = false;
	}
	if (commandKey) {
		if (inASCII+64 == toupper(charKey_Up)) {
			upKeyDown = false;
		}
		if (inASCII+64 == toupper(charKey_Left)) {
			leftKeyDown = false;
		}
		if (inASCII+64 == toupper(charKey_Down)) {
			downKeyDown = false;
		}
		if (inASCII+64 == toupper(charKey_Right)) {
			rightKeyDown = false;
		}
	}

	if (!upKeyDown && !leftKeyDown && !downKeyDown && !rightKeyDown) {
		lastPosX = 9999;
		lastPosY = 9999;
		magnetMoveDir = -1;
		magnetWrongMoveDir = -1;
		magnetMoveCount = 0;
	}

    switch( inASCII ) {
        case 'e':
        case 'E':
            mEKeyDown = false;
            break;
        case 'z':
        case 'Z':
            mZKeyDown = false;
            break;
        case ' ':
            if (! SettingsManager::getIntSetting( "keyboardActions", 1 )) shouldMoveCamera = true;
            break;
        case 'f':
        case 'F':
            if (SettingsManager::getIntSetting( "keyboardActions", 1 )) shouldMoveCamera = true;
            break;
        }
}


void LivingLifePage::actionPerformed( GUIComponent *inTarget )
{
    if( vogMode && inTarget == &mObjectPicker ) {

        char rightClick;
        int objectID = mObjectPicker.getSelectedObject( &rightClick );
        
        if( objectID != -1 ) {
            char *message = autoSprintf( "VOGI %d %d %d#",
                                         lrint( vogPos.x ), 
                                         lrint( vogPos.y ), objectID );
            
            sendToServerSocket( message );
            
            delete [] message;
            }
        }
}


ExtraMapObject LivingLifePage::copyFromMap( int inMapI )
{
    ExtraMapObject o = {
        mMap[ inMapI ],
        mMapMoveSpeeds[ inMapI ],
        mMapMoveOffsets[ inMapI ],
        mMapAnimationFrameCount[ inMapI ],
        mMapAnimationLastFrameCount[ inMapI ],
        mMapAnimationFrozenRotFrameCount[ inMapI ],
        mMapCurAnimType[ inMapI ],
        mMapLastAnimType[ inMapI ],
        mMapLastAnimFade[ inMapI ],
        mMapTileFlips[ inMapI ],
        mMapContainedStacks[ inMapI ],
        mMapSubContainedStacks[ inMapI ] };
    
    return o;
}

        
void LivingLifePage::putInMap( int inMapI, ExtraMapObject *inObj )
{
    mMap[ inMapI ] = inObj->objectID;
    
    mMapMoveSpeeds[ inMapI ] = inObj->moveSpeed;
    mMapMoveOffsets[ inMapI ] = inObj->moveOffset;
    mMapAnimationFrameCount[ inMapI ] = inObj->animationFrameCount;
    mMapAnimationLastFrameCount[ inMapI ] = inObj->animationLastFrameCount;
    mMapAnimationFrozenRotFrameCount[ inMapI ] = 
        inObj->animationFrozenRotFrameCount;
    
    mMapCurAnimType[ inMapI ] = inObj->curAnimType;
    mMapLastAnimType[ inMapI ] = inObj->lastAnimType;
    mMapLastAnimFade[ inMapI ] = inObj->lastAnimFade;
    mMapTileFlips[ inMapI ] = inObj->flip;
    
    mMapContainedStacks[ inMapI ] = inObj->containedStack;
    mMapSubContainedStacks[ inMapI ] = inObj->subContainedStack;
}



//FIELD OF VIEW
void LivingLifePage::calcFontScale( float newScale, Font *font ) {
	float scale = font->getScaleFactor();
	scale /= gui_fov_scale;
	scale *= newScale;
	font->setScaleFactor( scale );
    }

void LivingLifePage::changeFOV( float newScale ) {
	float fov_max = SettingsManager::getFloatSetting( "fovMax", 2.25f );
	
	if( newScale < 1.0f )
		newScale = 1.0f;
	else if( newScale > fov_max )
		newScale = fov_max;
	SettingsManager::setSetting( "fovScale", newScale );

	LiveObject *ourLiveObject = getOurLiveObject();
	if( ourLiveObject != NULL ) {
		if( ourLiveObject->heldByAdultID != -1 ) {
			ourLiveObject = getGameObject( ourLiveObject->heldByAdultID );
			if( ourLiveObject == NULL ) {
				ourLiveObject = getOurLiveObject();
                }
            }
		screenCenterPlayerOffsetX = int( double( screenCenterPlayerOffsetX ) / gui_fov_scale * newScale );
		screenCenterPlayerOffsetY = int( double( screenCenterPlayerOffsetY ) / gui_fov_scale * newScale );

		doublePair centerOffset = sub( lastScreenViewCenter, mult( ourLiveObject->currentPos, CELL_D ) );
		centerOffset = mult( centerOffset, 1. / gui_fov_scale );
		centerOffset = mult( centerOffset, newScale );
		centerOffset = add( mult( ourLiveObject->currentPos, CELL_D ), centerOffset );
		lastScreenViewCenter.x = round( centerOffset.x );
		lastScreenViewCenter.y = round( centerOffset.y );
        }

	calcFontScale( newScale, handwritingFont );
	calcFontScale( newScale, pencilFont );
	calcFontScale( newScale, pencilErasedFont );
	
	calcFontScale( newScale, mainFont );
	calcFontScale( newScale, titleFont );
	
	gui_fov_scale = newScale;
	gui_fov_scale_hud = gui_fov_scale / gui_fov_target_scale_hud;

	minitech::viewWidth = 1280 * newScale;
	minitech::viewHeight = 720 * newScale;
	minitech::guiScale = 1.25 * gui_fov_scale_hud;

	if(minitech::handwritingFont != NULL) minitech::handwritingFont->setScaleFactor( 16*minitech::guiScale );
	if(minitech::mainFont != NULL) minitech::mainFont->setScaleFactor( 16*minitech::guiScale );
	if(minitech::tinyHandwritingFont != NULL) minitech::tinyHandwritingFont->setScaleFactor( 16/2*minitech::guiScale );
	if(minitech::tinyMainFont != NULL) minitech::tinyMainFont->setScaleFactor( 16/2*minitech::guiScale );

	calcOffsetHUD();

	viewWidth = 1280 * newScale;
	viewHeight = 720 * newScale;
	setLetterbox( 1280 * newScale, 720 * newScale );
	setViewSize( 1280 * newScale );
    }

void LivingLifePage::changeHUDFOV( float newScale ) {
	if( newScale < 1 ) {
		newScale = 1.0f;
	} else if ( newScale > 1.75f ) {
		newScale = 1.75f;
	}

	gui_fov_target_scale_hud = newScale;
    SettingsManager::setSetting( "fovScaleHUD", gui_fov_target_scale_hud );
    gui_fov_scale_hud = gui_fov_scale / gui_fov_target_scale_hud;

	calcOffsetHUD();

	handwritingFont = new Font( "font_handwriting_32_32.tga", 3, 6, false, 16 * gui_fov_scale_hud );
	pencilFont->copySpacing( handwritingFont );
	pencilErasedFont->copySpacing( handwritingFont );
	
	changeFOV( SettingsManager::getFloatSetting( "fovScale", 1.0f ) );
    }

void LivingLifePage::calcOffsetHUD() {
    gui_fov_offset_x = (int)(((1280 * gui_fov_target_scale_hud) - 1280)/2);
    gui_fov_offset_y = (int)(((720 * gui_fov_target_scale_hud) - 720)/2);
    }



//KEYBOARD ACTIONS
void LivingLifePage::actionAlphaRelativeToMe( int x, int y ) {
	LiveObject *ourLiveObject = getOurLiveObject();
	
	x += ourLiveObject->xd;
	y += ourLiveObject->yd;

	int objId = getObjId( x, y );
	bool use = false;

	if (objId > 0) use = true;
	else use = false;

	if( ourLiveObject->holdingID > 0 ) {
		ObjectRecord *held = getObject( ourLiveObject->holdingID );

		if( held->foodValue == 0 ) {
			TransRecord *r = getTrans( ourLiveObject->holdingID, -1 );
			if( r != NULL && r->newTarget != 0 ) { // a use-on-ground transition exists!
                use = true;	// override the drop action
			}
		}
	}

	bool remove = false;
	if (objIdReverseAction(objId)) remove = true;
	
	if ( ourLiveObject->holdingID < 0 ) {
		remove = false;
		use = false;
	}

	x = sendX(x);
	y = sendY(y);
	char msg[32];
	if (remove) sprintf( msg, "REMV %d %d -1#", x, y);
	else if (use) sprintf( msg, "USE %d %d#", x, y);
	else sprintf( msg, "DROP %d %d -1#", x, y);
	setNextActionMessage( msg, x, y );
}

void LivingLifePage::actionBetaRelativeToMe( int x, int y ) {
	LiveObject *ourLiveObject = getOurLiveObject();
	
	x += ourLiveObject->xd;
	y += ourLiveObject->yd;

	bool remove = false;
	if (ourLiveObject->holdingID <= 0) {
		remove = true;
	}
	bool use = false;
	int objId = getObjId( x, y );
	if (objId > 0) {
		ObjectRecord* obj = getObject(objId);
		if (obj->numSlots == 0 && obj->blocksWalking) {
			TransRecord *r = getTrans( ourLiveObject->holdingID, objId );
			if ( r != NULL && r->newTarget != 0 ) {
				use = true;
			}
		}
	}

	if ( objIdReverseAction( objId ) ) use = true;

	if ( ourLiveObject->holdingID < 0 ) { // holding babay
		remove = false;
		use = false;
	}

	x = sendX(x);
	y = sendY(y);
	char msg[32];
	if (use) sprintf( msg, "USE %d %d#", x, y);
	else if (remove) sprintf( msg, "REMV %d %d -1#", x, y);
	else sprintf( msg, "DROP %d %d -1#", x, y);
	setNextActionMessage( msg, x, y );
	if (!remove) nextActionDropping = true;
}

void LivingLifePage::useTileRelativeToMe( int x, int y ) {
	LiveObject *ourLiveObject = getOurLiveObject();
	
	x += ourLiveObject->xd;
	y += ourLiveObject->yd;
	x = sendX(x);
	y = sendY(y);
	char msg[32];
	sprintf( msg, "USE %d %d#", x, y);
	setNextActionMessage( msg, x, y );
}

void LivingLifePage::dropTileRelativeToMe( int x, int y ) {
	LiveObject *ourLiveObject = getOurLiveObject();
	
	x += ourLiveObject->xd;
	y += ourLiveObject->yd;
	x = sendX(x);
	y = sendY(y);
	char msg[32];
	sprintf( msg, "DROP %d %d -1#", x, y);
	setNextActionMessage( msg, x, y );
}

//KEYBOARD MOVEMENT
void LivingLifePage::movementStep() {
	LiveObject *ourLiveObject = getOurLiveObject();
	
	if (!upKeyDown && !leftKeyDown && !downKeyDown && !rightKeyDown) return;

	int x = round(ourLiveObject->currentPos.x);
	int y = round(ourLiveObject->currentPos.y);

	if (x == lastPosX && y == lastPosY && ourLiveObject->inMotion) return;

	int objId = getObjId(x, y);
	if (objId > 0 && getObject(objId)->blocksWalking && ourLiveObject->inMotion) return;

	int sX = x;
	int sY = y;

	int dir = getMoveDirection();
	if (dir <= 0) return;

	if (!findNextMove(x, y, dir)) return; // sets x and y

	lastPosX = sX;
	lastPosY = sY;

	if (waitForDoorToOpen && (lastDoorToOpenX != x || lastDoorToOpenY != y)) {
		waitForDoorToOpen = false;
	} else if (waitForDoorToOpen) {
		if (tileHasClosedDoor( lastDoorToOpenX, lastDoorToOpenY ))
			return;
		waitForDoorToOpen = false;
	} else if (tileHasClosedDoor( x, y )) {
		char msg[32];
		sprintf( msg, "USE %d %d#", sendX(x), sendY(y));
		setNextActionMessage( msg, x, y );
		waitForDoorToOpen = true;
		lastDoorToOpenX = (int)x;
		lastDoorToOpenY = (int)y;
		return;
	}

	x *= CELL_D;
	y *= CELL_D;

	blockMouseScaling = true;

	float tMouseX = lastMouseX;
	float tMouseY = lastMouseY;
	mForceGroundClick = true;
	pointerDown( x, y );
	pointerUp( x, y );
	mForceGroundClick = false;
	lastMouseX = tMouseX;
	lastMouseY = tMouseY;
	
	magnetMoveCount++;
	
	blockMouseScaling = false;
}

bool LivingLifePage::findNextMove(int &x, int &y, int dir) {
	if (dir <= 0) return false;
	
	if (magnetMoveDir > 0) {
		if (magnetWrongMoveDir != dir || magnetMoveCount > 2) {
			magnetWrongMoveDir = -1;
			magnetMoveDir = -1;
		} else {
			if (setMoveDirIfSafe(x, y, magnetMoveDir)) {
				magnetWrongMoveDir = -1;
				magnetMoveDir = -1;
				return true;
			}
		}
	}

	if (setMoveDirIfSafe(x, y, dir)) return true;

	int nextMoveDir = getNextMoveDir(dir, 1);
	if (dirIsSafeToWalk(x, y, nextMoveDir)) {
		setMoveDirection(x, y, nextMoveDir);
		if (dir % 2 == 0) {
			magnetWrongMoveDir = dir;
			magnetMoveDir = getNextMoveDir(dir, -1);
			magnetMoveCount = 0;
		}
		return true;
	}
	nextMoveDir = getNextMoveDir(dir, -1);
	if (dirIsSafeToWalk(x, y, nextMoveDir)) {
		setMoveDirection(x, y, nextMoveDir);
		if (dir % 2 == 0) {
			magnetWrongMoveDir = dir;
			magnetMoveDir = getNextMoveDir(dir, 1);
			magnetMoveCount = 0;
		}
		return true;
	}

	return false;
}

int LivingLifePage::getNextMoveDir(int direction, int add) {
	direction += add;
	while (direction < 1) direction += 8;
	while (direction > 8) direction -= 8;
	return direction;
}

int LivingLifePage::getMoveDirection() {
	if (!upKeyDown && !leftKeyDown && !downKeyDown && !rightKeyDown) return 0;
	if (upKeyDown && leftKeyDown && !downKeyDown && !rightKeyDown) return 1;
	if (upKeyDown && !leftKeyDown && !downKeyDown && !rightKeyDown) return 2;
	if (upKeyDown && !leftKeyDown && !downKeyDown && rightKeyDown) return 3;
	if (!upKeyDown && !leftKeyDown && !downKeyDown && rightKeyDown) return 4;
	if (!upKeyDown && !leftKeyDown && downKeyDown && rightKeyDown) return 5;
	if (!upKeyDown && !leftKeyDown && downKeyDown && !rightKeyDown) return 6;
	if (!upKeyDown && leftKeyDown && downKeyDown && !rightKeyDown) return 7;
	if (!upKeyDown && leftKeyDown && !downKeyDown && !rightKeyDown) return 8;
	return 0;
}

bool LivingLifePage::setMoveDirIfSafe(int &x, int &y, int dir) {
	if (!dirIsSafeToWalk(x, y, dir)) return false;
	setMoveDirection(x, y, dir);
	return true;
}

void LivingLifePage::setMoveDirection(int &x, int &y, int direction) {
	switch (direction) {
		case 1: x--; y++; break;
		case 2: y++; break;
		case 3: x++; y++; break;
		case 4: x++; break;
		case 5: x++; y--; break;
		case 6: y--; break;
		case 7: x--; y--; break;
		case 8: x--; break;
	}
}

bool LivingLifePage::tileHasClosedDoor(int x, int y) {
	int closedDoorIDs [10] = { 116, 2759, 876, 1930, 2757, 877, 115, 1851, 2984, 2962 }; 
	
	int objId = getObjId( x, y);
	if (objId > 0) {
		for (int i = 0; i < 10; i++) {
			if (objId == closedDoorIDs[i]) return true;
		}
	}
	return false;
}

bool LivingLifePage::dirIsSafeToWalk(int x, int y, int dir) {
	LiveObject *ourLiveObject = getOurLiveObject();
	
	int tX, tY;

	tX = x; tY = y; setMoveDirection(tX, tY, dir);
	
	int objId = getObjId( tX, tY);
	if (objId > 0) {

		ObjectRecord* obj = getObject(objId);
		if (obj && obj->blocksWalking) {
			if (ourLiveObject->xd == x || ourLiveObject->yd == y)
				if (!tileHasClosedDoor( x, y )) return false;
		}
	}

	if (dir % 2 == 0) return true; // is not a corner dir

	int nextDir = getNextMoveDir(dir, 1);
	tX = x; tY = y; setMoveDirection(tX, tY, nextDir);

	nextDir = getNextMoveDir(dir, -1);
	tX = x; tY = y; setMoveDirection(tX, tY, nextDir);

	return true;
}
