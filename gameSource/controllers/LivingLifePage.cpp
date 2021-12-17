#include "LivingLifePage.h"

#include "OneLife/gameSource/objectBank.h"
#include "OneLife/gameSource/components/banks/spriteBank.h"
#include "OneLife/gameSource/transitionBank.h"
#include "OneLife/gameSource/categoryBank.h"
#include "OneLife/gameSource/soundBank.h"
#include "OneLife/gameSource/whiteSprites.h"
#include "OneLife/gameSource/procedures/graphics/sprites/drawMessage.h"
#include "OneLife/gameSource/groundSprites.h"
#include "OneLife/gameSource/accountHmac.h"
#include "OneLife/gameSource/liveObjectSet.h"
#include "OneLife/gameSource/ageControl.h"
#include "OneLife/gameSource/musicPlayer.h"
#include "OneLife/gameSource/emotion.h"
#include "OneLife/gameSource/photos.h"
#include "OneLife/gameSource/liveAnimationTriggers.h"
#include "OneLife/gameSource/../commonSource/fractalNoise.h"
#include "OneLife/gameSource/../commonSource/sayLimit.h"
#include "minorGems/util/SimpleVector.h"
#include "minorGems/util/MinPriorityQueue.h"
#include "minorGems/game/Font.h"
#include "minorGems/util/stringUtils.h"
#include "minorGems/util/SettingsManager.h"
#include "minorGems/util/random/JenkinsRandomSource.h"
#include "minorGems/game/drawUtils.h"
#include "minorGems/game/gameGraphics.h"
#include "OneLife/gameSource/zoomView.h"
#include "minorGems/io/file/File.h"
#include "minorGems/formats/encodingUtils.h"
#include "minorGems/system/Thread.h"
#include "minorGems/util/log/AppLog.h"
#include "minorGems/crypto/hashes/sha1.h"
#include <stdlib.h>//#include <math.h>
#include <string>
#include "OneLife/gameSource/application.h"
#include "OneLife/gameSource/procedures/graphics/drawGround.h"
#include "OneLife/gameSource/procedures/graphics/drawAgent.h"
#include "OneLife/gameSource/dataTypes/animation.h"
#include "OneLife/gameSource/dataTypes/hardware.h"
#include "OneLife/gameSource/dataTypes/exception/exception.h"
#include "OneLife/gameSource/components/engines/audioRenderer.h"
#include "OneLife/gameSource/components/engines/GameSceneHandler.h"
#include "OneLife/gameSource/features/family.h"
#include "OneLife/gameSource/features/homeland.h"
#include "OneLife/gameSource/procedures/maths/gridPos.h"
#include "OneLife/gameSource/procedures/maths/misc.h"
#include "OneLife/gameSource/scenes1/agent.h"
#include "OneLife/gameSource/scenes1/intangibles/text.h"
#include "OneLife/gameSource/procedures/graphics/sprites/strings.h"
#include "OneLife/gameSource/procedures/graphics/sprites/grounds.h"
#include "OneLife/gameSource/procedures/graphics/sprites/agents.h"
#include "OneLife/gameSource/procedures/graphics/sprites/misc.h"
#include "OneLife/gameSource/procedures/string.h"
#include "OneLife/gameSource/components/camera.h"
#include "OneLife/gameSource/procedures/graphics/base.h"
#include "OneLife/gameSource/minitech.h"
#include "OneLife/gameSource/procedures/misc.h"
#include "OneLife/gameSource/scenes1/maps/outsideMap.h"
#include "OneLife/gameSource/components/soundPlayer.h"
#include "OneLife/gameSource/procedures/graphics/components.h"
#include "OneLife/gameSource/procedures/graphics/screens.h"
#include "OneLife/gameSource/dataTypes/signals.h"
#include "OneLife/gameSource/debug.h"

using SIGNAL = OneLife::dataValue::Signal;

#define OHOL_NON_EDITOR 1
#include "OneLife/gameSource/ObjectPickable.h"

/**********************************************************************************************************************/

#define MAP_D 64
#define MAP_NUM_CELLS 4096

// base speed for animations that aren't speeded up or slowed down
// when player moving at a different speed, anim speed is modified
#define BASE_SPEED 3.75

/**********************************************************************************************************************/

extern OneLife::game::Application *screen;
extern int versionNumber;
extern int dataVersionNumber;
extern double frameRateFactor;
extern Font *mainFont;
extern Font *numbersFontFixed;
extern Font *mainFontReview;
extern Font *handwritingFont;
extern Font *pencilFont;
extern Font *pencilErasedFont;
extern Font *titleFont;
extern doublePair lastScreenViewCenter;
extern double viewWidth;
extern double viewHeight;
extern int screenW, screenH;
extern char usingCustomServer;
extern char *serverIP;
extern int serverPort;
extern char *userEmail;
extern char *userTwinCode;
extern int userTwinCount;
extern char userReconnect;
extern float musicLoudness;
extern int messagesInCount;
extern char pendingCMData;
extern double lastServerMessageReceiveTime;
extern double largestPendingMessageTimeGap;
extern SimpleVector<char*> readyPendingReceivedMessages;
extern char waitForFrameMessages;
extern SimpleVector<char*> serverFrameMessages;
extern char *lastMessageSentToServer;
extern SimpleVector<HomePos> homePosStack;
extern SimpleVector<HomePos> oldHomePosStack;
extern int pathFindingD;// should match limit on server
extern bool isTrippingEffectOn;
extern SimpleVector<unsigned char> serverSocketBuffer;
extern char *pendingMapChunkMessage;
extern char autoAdjustFramerate;
extern int baseFramesPerSecond;
extern SimpleVector<OffScreenSound> offScreenSounds;

//FOV
extern int gui_hud_mode;
extern float gui_fov_scale;
extern float gui_fov_scale_hud;
extern float gui_fov_target_scale_hud;
extern int gui_fov_offset_x;
extern int gui_fov_offset_y;

/**********************************************************************************************************************/

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
char savingSpeech = false;
char savingSpeechColor = false;
Image *speechMaskImage = NULL;
Image *speechColorImage = NULL;
char savingSpeechNumber = 1;
char savingSpeechMask = false;
SimpleVector<doublePair> trail;
SimpleVector<FloatColor> trailColors;
int playerActionTargetX;
int playerActionTargetY;
int ourID;
SimpleVector<LiveObject> gameObjects;// youngest last
FloatColor trailColor = { 0, 0.5, 0, 0.25 };
int overheadServerBytesSent = 0;
SimpleVector<LocationSpeech> locationSpeech;
int numServerBytesSent = 0;
// for determining our ID when we're not youngest on the server
// (so we're not last in the list after receiving the first PU message)
int recentInsertedGameObjectIndex = -1;
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
SpriteHandle guiPanelTileSprite;
int historyGraphLength = 100;
char drawAdd = true;
char drawMult = true;
double multAmount = 0.15;
double addAmount = 0.25;
char blackBorder = false;
char whiteBorder = true;
int bytesInCount = 0;

/**********************************************************************************************************************/

static bool waitForDoorToOpen;
static SpriteHandle guiPanelLeftSprite;
static SpriteHandle guiPanelRightSprite;
/*static */JenkinsRandomSource randSource2( 340403 );
static JenkinsRandomSource remapRandSource( 340403 );
static int lastScreenMouseX, lastScreenMouseY;
static char mouseDown = false;
static int mouseDownFrames = 0;
static int minMouseDownFrames = 30;
static int screenCenterPlayerOffsetX, screenCenterPlayerOffsetY;
static float lastMouseX = 0;
static float lastMouseY = 0;
static char teaserVideo = false;// set to true to render for teaser video
static int showBugMessage = 0;
static const char *bugEmail = "jason" "rohrer" "@" "fastmail.fm";
static char savingSpeechEnabled = false;// if true, pressing S key (capital S)// causes current speech and mask to be saved to the screenShots folder
static char takingPhoto = false;
static GridPos takingPhotoGlobalPos;
static char takingPhotoFlip = false;
static int photoSequenceNumber = -1;
static char waitingForPhotoSig = false;
static char *photoSig = NULL;
static double emotDuration = 10;
static int drunkEmotionIndex = -1;
static int trippingEmotionIndex = -1;
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
static int messagesOutCount = 0;
static int bytesOutCount = 0;
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
static ObjectPickable objectPickable;
static float pencilErasedFontExtraFade = 0.75;// to make all erased pencil fonts lighter
static char shouldMoveCamera = true;
static char vogMode = false;
static doublePair vogPos = { 0, 0 };
static char vogPickerOn = false;
static int lastPlayerID = -1;// used on reconnect to decide whether to delete old home positions
static char hideGuiPanel = false;
static float connectionMessageFade = 1.0f;
// if user clicks to initiate an action while still moving, we
// queue it here
static char *nextActionMessageToSend = NULL;
static char nextActionEating = false;
static char nextActionDropping = false;
// block move until next PLAYER_UPDATE received after action sent
static char playerActionPending = false;
static char playerActionTargetNotAdjacent = false;
static char waitingForPong = false;
static int lastPingSent = 0;
static int lastPongReceived = 0;
static int valleySpacing = 40;
static int valleyOffset = 0;
static int apocalypseInProgress = false;
static double apocalypseDisplayProgress = 0;
static double apocalypseDisplaySeconds = 6;
static double remapPeakSeconds = 60;
static double remapDelaySeconds = 30;

/**********************************************************************************************************************/

extern void setTrippingColor( double x, double y );

/**********************************************************************************************************************/

LivingLifePage::LivingLifePage()
        : mForceRunTutorial( false ),
          mTutorialNumber( 0 ),
          mGlobalMessageShowing( false ),
          mGlobalMessageStartTime( 0 ),
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
	this->mRemapPeak = 0;
	this->mRemapDelay = 0;
	this->mPageStartTime = 0;

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
    drunkEmotionIndex = SettingsManager::getIntSetting( "drunkEmotionIndex", 2 );
    trippingEmotionIndex = SettingsManager::getIntSetting( "trippingEmotionIndex", 2 );
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
    mMapSubContainedStacks = new SimpleVector< SimpleVector<int> >[ mMapD * mMapD ];

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


    splitAndExpandSprites( "hungerBoxes.tga", NUM_HUNGER_BOX_SPRITES, mHungerBoxSprites );
    splitAndExpandSprites( "hungerBoxFills.tga", NUM_HUNGER_BOX_SPRITES, mHungerBoxFillSprites );
    splitAndExpandSprites( "hungerBoxesErased.tga", NUM_HUNGER_BOX_SPRITES, mHungerBoxErasedSprites );
    splitAndExpandSprites( "hungerBoxFillsErased.tga", NUM_HUNGER_BOX_SPRITES, mHungerBoxFillErasedSprites );
    splitAndExpandSprites( "tempArrows.tga", NUM_TEMP_ARROWS, mTempArrowSprites );
    splitAndExpandSprites( "tempArrowsErased.tga", NUM_TEMP_ARROWS, mTempArrowErasedSprites );
    splitAndExpandSprites( "hungerDashes.tga", NUM_HUNGER_DASHES, mHungerDashSprites );
    splitAndExpandSprites( "hungerDashesErased.tga", NUM_HUNGER_DASHES, mHungerDashErasedSprites );
    splitAndExpandSprites( "hungerBars.tga", NUM_HUNGER_DASHES, mHungerBarSprites );
    splitAndExpandSprites( "hungerBarsErased.tga", NUM_HUNGER_DASHES, mHungerBarErasedSprites );
    splitAndExpandSprites( "homeArrows.tga", NUM_HOME_ARROWS, mHomeArrowSprites );
    splitAndExpandSprites( "homeArrowsErased.tga", NUM_HOME_ARROWS, mHomeArrowErasedSprites );


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

	this->feature.debugMessageEnabled = false;

	/******************************************************************************************************************/
}

LivingLifePage::~LivingLifePage()
{
	printf( "Total received = %d bytes (+%d in headers), "
			"total sent = %d bytes (+%d in headers)\n",
			this->socket->getTotalServerBytesRead(),
			this->socket->getTotalServerOverheadBytesRead(),
			numServerBytesSent,
			this->socket->getTotalServerOverheadBytesSent() );

	mGlobalMessagesToDestroy.deallocateStringElements();

	freeLiveTriggers();

	readyPendingReceivedMessages.deallocateStringElements();

	serverFrameMessages.deallocateStringElements();

	if( pendingMapChunkMessage != NULL )
	{
		delete [] pendingMapChunkMessage;
		pendingMapChunkMessage = NULL;
	}

	clearLiveObjects();

	mOldDesStrings.deallocateStringElements();
	if( mCurrentDes != NULL )
	{
		delete [] mCurrentDes;
	}

	mOldLastAteStrings.deallocateStringElements();
	if( mCurrentLastAteString != NULL )
	{
		delete [] mCurrentLastAteString;
	}

	mSentChatPhrases.deallocateStringElements();

	if( !this->socket->isClosed()) this->socket->close();

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

/**********************************************************************************************************************/

void LivingLifePage::handle(OneLife::dataType::UiComponent* screen)
{
	screen->label = this->screenName;
	screen->draw = OneLife::game::graphic::drawOutdoorSceneScreen;
	screen->body = &(this->screen);
}

/**********************************************************************************************************************/


int LivingLifePage::getIndexRecentlyInsertedGameObject()
{
	return recentInsertedGameObjectIndex;
}

void LivingLifePage::setSocket(OneLife::game::component::Socket *socket)
{
	this->socket = socket;
}

/**********************************************************************************************************************/

void LivingLifePage::sendToServerSocket( char *inMessage )
{
	OneLife::game::Debug::write("#######>sendToServer : %s", inMessage);
	OneLife::dataType::socket::Message message;
	message.body = inMessage;
	try
	{
		this->socket->sendMessage(message);
	}
	catch(OneLife::game::Exception* e)
	{
		if(true)//if map has been loaded
		{
			this->sendDeathMessage(translate( "reasonDisconnected" ));
			handleOurDeath( true );
		}
		else
		{
			setWaiting( false );
			setSignal( "loginFailed" );
		}
	}
}

doublePair LivingLifePage::minitechGetLastScreenViewCenter() { return lastScreenViewCenter; }

char *LivingLifePage::minitechGetDisplayObjectDescription( int objId )
{
    ObjectRecord *o = getObject( objId );
    if( o == NULL )
	{
		return NULL;
    }
	return getDisplayObjectDescription(objId);
}

void LivingLifePage::computePathToDest( LiveObject *inObject )
{
	OneLife::game::computePathToDest(
		inObject,
		mMapD,
		mMapOffsetX,
		mMapOffsetY,
		mMap);
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

//EXTENDED FUNCTIONALITY
void LivingLifePage::setNextActionMessage(const char* msg, int x, int y)
{
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

int LivingLifePage::getObjId( int tileX, int tileY )
{
	int mapX = tileX - mMapOffsetX + mMapD / 2;
	int mapY = tileY - mMapOffsetY + mMapD / 2;
	int i = mapY * mMapD + mapX;
	if (i < 0 || i >= mMapD*mMapD) return -1;
	return mMap[i];
}

bool LivingLifePage::objIdReverseAction( int objId )
{
	LiveObject *ourLiveObject = getOurLiveObject();
	if (objId <= 0) return false;
	bool r = false;
	if ( ourLiveObject->holdingID <= 0 )
	{
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

void LivingLifePage::pickUpBabyInRange()
{
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

void LivingLifePage::useBackpack(bool replace)
{
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

void LivingLifePage::usePocket(int clothingID)
{
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

void LivingLifePage::useOnSelf()
{
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

void LivingLifePage::takeOffBackpack()
{
	LiveObject *ourLiveObject = getOurLiveObject();
	
	char message[32];
	sprintf(message, "SELF %i %i 5#", ourLiveObject->xd, ourLiveObject->yd);
	sendToServerSocket( message );
}

void LivingLifePage::setOurSendPosXY(int &x, int &y)
{
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

void LivingLifePage::clearMap()
{
    for( int i=0; i<mMapD *mMapD; i++ ) {
        // -1 represents unknown
        // 0 represents known empty
        mMap[i] = -1;
        mMapBiomes[i] = -1;
        mMapFloors[i] = -1;
        
        mMapAnimationFrameCount[i] = randSource2.getRandomBoundedInt( 0, 10000 );
        mMapAnimationLastFrameCount[i] = 
            randSource2.getRandomBoundedInt( 0, 10000 );
        
        mMapAnimationFrozenRotFrameCount[i] = 0;
        mMapAnimationFrozenRotFrameCountUsed[i] = false;
        
        mMapFloorAnimationFrameCount[i] = 
            randSource2.getRandomBoundedInt( 0, 10000 );

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

void LivingLifePage::runTutorial()
{
    mForceRunTutorial = true;
}

void LivingLifePage::clearLiveObjects()
{
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


void LivingLifePage::clearOwnerInfo()
{
    for( int i=0; i<mOwnerInfo.size(); i++ ) {
        delete mOwnerInfo.getElement( i )->ownerList;
        }
    mOwnerInfo.deleteAll();
}

char LivingLifePage::isMapBeingPulled()
{
    return mapPullMode;
}

void LivingLifePage::adjustAllFrameCounts( double inOldFrameRateFactor,
                                           double inNewFrameRateFactor )
{
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

LiveObject *LivingLifePage::getOurLiveObject()
{
    return getLiveObject( ourID );
}

LiveObject *LivingLifePage::getLiveObject( int inID )
{
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
				
					if( abs(xDis) == 0 ) newXDis = randSource2.getRandomBoundedInt( -1, 1 );
					if( abs(xDis) == 1 ) newXDis = signX * randSource2.getRandomBoundedInt( 0, 1 );
					if( abs(yDis) == 0 ) newYDis = randSource2.getRandomBoundedInt( -1, 1 );
					if( abs(yDis) == 1 ) newYDis = signY * randSource2.getRandomBoundedInt( 0, 1 );
					
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
		
		if( abs(xDis) == 0 ) newXDis = randSource2.getRandomBoundedInt( -1, 1 );
		if( abs(yDis) == 0 ) newYDis = randSource2.getRandomBoundedInt( -1, 1 );
		
		int newX = path[ 1 ].x;
		int newY = path[ 1 ].y;
		if( newXDis != 2 ) newX = path[ 0 ].x + newXDis;
		if( newYDis != 2 ) newY = path[ 0 ].y + newYDis;
		
		if( !tileBlocked( newX, newY ) ) path[ 1 ] = { newX, newY };
		
		}
	
	}
	

bool LivingLifePage::isTripping()
{
	LiveObject *ourLiveObject = getOurLiveObject();
	if( ourLiveObject == NULL ) return false;
	return 
		trippingEmotionIndex != -1 &&
		ourLiveObject->currentEmot != NULL &&
		strcmp( ourLiveObject->currentEmot->triggerWord, 
		getEmotion( trippingEmotionIndex )->triggerWord ) == 0;
}

// forces uppercase
void LivingLifePage::drawChalkBackgroundString( doublePair inPos, 
                                                const char *inString,
                                                double inFade,
                                                double inMaxWidth,
                                                LiveObject *inSpeaker,
                                                int inForceMinChalkBlots,
                                                FloatColor *inForceBlotColor,
                                                FloatColor *inForceTextColor )
{
	OneLife::graphic::drawChalkBackgroundString(
		inPos,
		inString,
		inFade,
		inMaxWidth,
		inSpeaker,
		inForceMinChalkBlots,
		inForceBlotColor,
		inForceTextColor,
		mChalkBlotSprite);
}

void LivingLifePage::drawOffScreenSounds()
{
    OneLife::graphic::drawOffScreenSounds(mChalkBlotSprite);
}

void LivingLifePage::handleAnimSound( int inObjectID, double inAge, 
                                      AnimType inType,
                                      int inOldFrameCount, int inNewFrameCount,
                                      double inPosX, double inPosY )
{
    double oldTimeVal = frameRateFactor * inOldFrameCount / 60.0;
    double newTimeVal = frameRateFactor * inNewFrameCount / 60.0;
    if( inType == ground2 )
	{
        inType = ground;
	}

    AnimationRecord *anim = getAnimation( inObjectID, inType );
    if( anim != NULL )
	{
        for( int s=0; s<anim->numSounds; s++ )
		{
            if( anim->soundAnim[s].sound.numSubSounds == 0 )
			{
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
                                  char inNoTimeEffects ) //TODO: rename drawMapAgent()
{
	OneLife::graphic::drawMapCell(
		inMapI,
		inScreenX,
		inScreenY,
		inHighlightOnly,
		inNoTimeEffects,
		mMapD,
		mMap,
		mMapAnimationFrameCount,
		mMapAnimationLastFrameCount,
		mMapAnimationFrozenRotFrameCount,
		mMapAnimationFrozenRotFrameCountUsed,
		mMapLastAnimFade,
		mMapMoveSpeeds,
		mMapCurAnimType,
		mMapLastAnimType,
		mMapDropOffsets,
		mMapMoveOffsets,
		mMapDropSounds,
		mMapContainedStacks,
		mMapSubContainedStacks,
		mShowHighlights,
		mPrevMouseOverSpotFades,
		mPrevMouseOverSpotsBehind,
		mPrevMouseOverSpots,
		mCurMouseOverFade,
		mCurMouseOverBehind,
		mCurMouseOverSpot,
		mCurMouseOverSelf,
		mCurMouseOverID,
		mMapTileFlips,
		mMapDropRot,
		mMapFloors,
		mMapOffsetX,
		mMapOffsetY);
}

double pathStepDistFactor = 0.2;

GridPos LivingLifePage::getMapPos( int inWorldX, int inWorldY )
{
    GridPos p =
        { inWorldX - mMapOffsetX + mMapD / 2,
          inWorldY - mMapOffsetY + mMapD / 2 };
    
    return p;
}

int LivingLifePage::getMapIndex( int inWorldX, int inWorldY )
{
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
    SimpleVector<doublePair> *inSpeakersPos )
{
	return OneLife::game::drawLiveObject(
		inObj,
		inSpeakers,
		inSpeakersPos );
}

void LivingLifePage::drawHungerMaxFillLine( doublePair inAteWordsPos,
                                            int inMaxFill,
                                            SpriteHandle *inBarSprites,
                                            SpriteHandle *inDashSprites,
                                            char inSkipBar,
                                            char inSkipDashes )
{
	OneLife::game::drawHungerMaxFillLine(
		inAteWordsPos,
		inMaxFill,
		inBarSprites,
		inDashSprites,
		inSkipBar,
		inSkipDashes);
}

void LivingLifePage::draw(
	doublePair inViewCenter,
	double inViewSize )
{
	//#include "OneLife/gameSource/procedures/graphics/drawTutorialSheet.cpp"
	/*******************************************************************************************************************
	OneLife::game::graphic::drawGameScreen(
			mStartedLoadingFirstObjectSet,
			mFirstObjectSetLoadingProgress,
			mDoneLoadingFirstObjectSet);//TODO: replace with OneLife::game::graphic::drawGameScreen();
	*******************************************************************************************************************/
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

char *LivingLifePage::getDeathReason() {
    if( mDeathReason != NULL ) {
        return stringDuplicate( mDeathReason );
        }
    else {
        return NULL;
        }
    }

void LivingLifePage::handleOurDeath( char inDisconnect ) {
    
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

    if( inDisconnect ) {
        setSignal( "disconnect" );
        }
    else {				
		//reset fov on death
		changeFOV( SettingsManager::getFloatSetting( "fovDefault", 1.25f ) );
        
		setSignal( "died" );
        }
    
    instantStopMusic();
    // so sound tails are not still playing when we we get reborn
    fadeSoundSprites( 0.1 );
    setSoundLoudness( 0 );
    }

int LivingLifePage::getNumHints( int inObjectID ) {
    

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

char *LivingLifePage::getHintMessage( int inObjectID, int inIndex ) {

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

void LivingLifePage::sendBugReport( int inBugNumber ) {
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

void LivingLifePage::endExtraObjectMove( int inExtraIndex ) {
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

void LivingLifePage::setNewCraving( int inFoodID, int inYumBonus ) {
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

void LivingLifePage::step()
{
	OneLife::game::Debug::writeMethodInfo("LivingLifePage::step()");
    
    if(isAnySignalSet()) return;

    if( apocalypseInProgress )
	{
        double stepSize = frameRateFactor / ( apocalypseDisplaySeconds * 60.0 );
        apocalypseDisplayProgress += stepSize;
	}
    
    if( mRemapPeak > 0 )
	{
        if( mRemapDelay < 1 )
		{
            double stepSize = frameRateFactor / ( remapDelaySeconds * 60.0 );
            mRemapDelay += stepSize;
		}
        else
		{
            double stepSize = mRemapDirection * frameRateFactor / ( remapPeakSeconds * 60.0 );
            mCurrentRemapFraction += stepSize;
            if( stepSize > 0 && mCurrentRemapFraction >= mRemapPeak )
			{
                mCurrentRemapFraction = mRemapPeak;
                mRemapDirection *= -1;
			}
            if( stepSize < 0 && mCurrentRemapFraction <= 0 )
			{
                mCurrentRemapFraction = 0;
                mRemapPeak = 0;
			}
            if( takingPhoto )
			{
                // stop remapping briefly during photo
                setRemapFraction( 0 );
			}
            else
			{
                setRemapFraction( mCurrentRemapFraction );
			}
		}
	}

	if( mouseDown )
	{
        mouseDownFrames++;
	}
    
    if( !this->socket->isConnected() )
	{
		this->socket->connect();
		connectionMessageFade = 1.0f;
        return;
	}

    double pageLifeTime = game_getCurrentTime() - mPageStartTime;
    
    if( pageLifeTime < 1 ) {
        // let them see CONNECTING message for a bit
        return;
        }

    if( this->socket->isConnected() )
	{
        // we've heard from server, not waiting to connect anymore
        setWaiting( false );
	}
    else
	{
        if( pageLifeTime > 10 )
		{
            // having trouble connecting.
            this->socket->close();
            setWaiting( false );
            setSignal( "connectionFailed" );
            return;
		}
	}

    // first, read all available data from server
    char readSuccess = this->socket->readMessage();

    if( ! readSuccess )
	{
        if( this->socket->isConnected() )
		{
            if( this->socket->getLastQueryLifeTime() < 1 )
			{
                // let player at least see waiting page
                // avoid flicker
                return;
			}
		}
		this->socket->close();
		if(true)//scene has ben loaded
		{
			this->sendDeathMessage(translate( "reasonDisconnected" ));
			handleOurDeath( true );
		}
		else
		{
			setWaiting( false );
            setSignal( "loginFailed" );
		}
        return;
	}
    
    if( mLastMouseOverID != 0 )
	{
        mLastMouseOverFade -= 0.01 * frameRateFactor;
        
        if( mLastMouseOverFade < 0 ) {
            mLastMouseOverID = 0;
            }
	}

    if( mGlobalMessageShowing )
	{
        
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
    for( int i=0; i<mMapExtraMovingObjects.size(); i++ )
	{
        
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

    if( mCurMouseOverID > 0 && ! mCurMouseOverSelf )
	{
        mCurMouseOverFade += 0.2 * frameRateFactor;
        if( mCurMouseOverFade >= 1 ) {
            mCurMouseOverFade = 1.0;
            }
        }
    
    for( int i=0; i<mPrevMouseOverSpotFades.size(); i++ )
	{
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

    if( mCurMouseOverCell.x != -1 )
	{
        
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
    
    OneLife::game::Debug::write("(%i,%i) (%i,%i)", mNotePaperPosOffset.x, mNotePaperPosOffset.y, mNotePaperPosTargetOffset.x, mNotePaperPosTargetOffset.y);
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
            
            mHomeSlipPosOffset = add( mHomeSlipPosOffset, mult( dir, speed ) );
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
    if( ( mTutorialNumber > 0 || mGlobalMessageShowing ) 
        && ourObject != NULL ) {
        
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
    for( int i=0; i<NUM_HINT_SHEETS; i++ ) {
        
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
    for( int i=0; i<NUM_HINT_SHEETS; i++ ) {
        
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
    for( int i=0; i<3; i++ ) {
        if( i != mHungerSlipVisible &&
            ! equal( mHungerSlipPosOffset[i], mHungerSlipHideOffsets[i] ) ) {
            // visible when it shouldn't be
            mHungerSlipPosTargetOffset[i] = mHungerSlipHideOffsets[i];
            anySlipsMovingDown = true;
            }
        }
    
    if( !anySlipsMovingDown ) {
        if( mHungerSlipVisible != -1 ) {
            // send one up
            mHungerSlipPosTargetOffset[ mHungerSlipVisible ] =
                mHungerSlipShowOffsets[ mHungerSlipVisible ];
            }
        }


    // move all toward their targets
    for( int i=0; i<3; i++ ) {
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
        ! waitingForPong ) {

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
             ! waitingForPong ) {
        // we're waiting for a response from the server, and
        // we haven't heard ANYTHING from the server in a long time
        // a full, two-way network connection break
        printf( "Been waiting for response to our action request "
                "from server for %.2f seconds, and last server message "
                "received %.2f sec ago.  Declaring connection broken.\n",
                curTime - ourObject->pendingActionAnimationStartTime,
                curTime - lastServerMessageReceiveTime );

		this->socket->close();

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
        10 + largestPendingMessageTimeGap ) {

        // been bouncing for five seconds with no answer from server
        // in the mean time, we have seen other messages arrive from server
        // (so any network outage is over)

        if( waitingForPong &&
            lastPingSent == lastPongReceived ) {
            
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
        else {
            printf( 
                "Been waiting for response to our action request "
                "from server for %.2f seconds, and no response received "
                "for our PING.  Declaring connection broken.\n",
                curTime - ourObject->pendingActionAnimationStartTime );
            
            this->socket->close();

            if( mDeathReason != NULL ) {
                delete [] mDeathReason;
                }
            mDeathReason = stringDuplicate( translate( "reasonDisconnected" ) );
            
            handleOurDeath( true );
            }
        }
	if ( SettingsManager::getIntSetting( "keyboardActions", 1 ) ) movementStep();

	minitech::livingLifeStep();

	#include "OneLife/gameSource/procedures/socket/do_socket_stuff_0.cpp"
        
    // check if we're about to move off the screen
    LiveObject *ourLiveObject = getOurLiveObject();


    
    if( !mapPullMode && mDoneLoadingFirstObjectSet && ourLiveObject != NULL )
	{
        // current age
        double age = computeCurrentAge( ourLiveObject );
        int sayCap = (int)( floor( age ) + 1 );
        if( ourLiveObject->lineage.size() == 0  && sayCap < 30 )
		{
            // eve has a larger say limit
            sayCap = 30;
		}
        if( vogMode )
		{
            sayCap = 200;
		}
        char *currentText = mSayField.getText();
        if( strlen( currentText ) > 0 && currentText[0] == '/' )
		{
            // typing a filter
            // hard cap at 25, regardless of age
            // don't want them typing long filters that overflow the display
            sayCap = 23;
            }
        delete [] currentText;
        mSayField.setMaxLength( sayCap );
        LiveObject *cameraFollowsObject = ourLiveObject;
        if( ourLiveObject->heldByAdultID != -1 )
		{
            cameraFollowsObject = getGameObject( ourLiveObject->heldByAdultID );
            if( cameraFollowsObject == NULL )
			{
                ourLiveObject->heldByAdultID = -1;
                cameraFollowsObject = ourLiveObject;
			}
		}
        doublePair targetObjectPos = cameraFollowsObject->currentPos;
        if( vogMode )
		{
            targetObjectPos = vogPos;
		}
        doublePair screenTargetPos = mult( targetObjectPos, CELL_D );
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
    for( int i=0; i<gameObjects.size(); i++ ) {
        
        LiveObject *o = gameObjects.getElement( i );
        
        if( o->currentSpeech != NULL ) {
            if( game_getCurrentTime() > o->speechFadeETATime ) {
                
                o->speechFade -= 0.05 * frameRateFactor;

                if( o->speechFade <= 0 ) {
                    delete [] o->currentSpeech;
                    o->currentSpeech = NULL;
                    o->speechFade = 1.0;
                    o->speechIsSuccessfulCurse = false;
                    }
                }
            }

        
        if( o->currentEmot != NULL ) {
            if( game_getCurrentTime() > o->emotClearETATime ) {
                
                // play decay sounds for this emot

                if( !o->outOfRange ) {
                    for( int s=0; s<getEmotionNumObjectSlots(); s++ ) {
                                    
                        int id = getEmotionObjectByIndex( o->currentEmot, s );
                                    
                        if( id > 0 ) {
                            ObjectRecord *obj = getObject( id );
                                        
                            if( obj->decaySound.numSubSounds > 0 ) {    
                                    
                                playSound( 
                                    obj->decaySound,
                                    getVectorFromCamera( 
                                        o->currentPos.x,
                                        o->currentPos.y ) );
                                }
                            }
                        }
                    }
                
                o->currentEmot = NULL;
                }
            }

        
        double animSpeed = o->lastSpeed;
        

        if( o->holdingID > 0 ) {
            ObjectRecord *heldObj = getObject( o->holdingID );
            
            if( heldObj->speedMult > 1.0 ) {
                // don't speed up animations just because movement
                // speed has increased
                // but DO slow animations down
                animSpeed /= heldObj->speedMult;
                }
            }

        double oldFrameCount = o->animationFrameCount;
        o->animationFrameCount += animSpeed / BASE_SPEED;
        o->lastAnimationFrameCount += animSpeed / BASE_SPEED;
        

        if( o->curAnim == moving ) {
            o->frozenRotFrameCount += animSpeed / BASE_SPEED;
            
            }
            
        
        char holdingRideable = false;
            
        if( o->holdingID > 0 &&
            getObject( o->holdingID )->rideable ) {
            holdingRideable = true;
            }
            
        
        if( o->curAnim != moving || !holdingRideable ) {
            // don't play player moving sound if riding something

            AnimType t = o->curAnim;
            doublePair pos = o->currentPos;
            
            if( o->heldByAdultID != -1 ) {
                t = held;
                

                for( int j=0; j<gameObjects.size(); j++ ) {
                    
                    LiveObject *parent = gameObjects.getElement( j );
                    
                    if( parent->id == o->heldByAdultID ) {
                        
                        pos = parent->currentPos;
                        }
                    }
                }
            
            
            handleAnimSound( o->displayID,
                             computeCurrentAge( o ),
                             t,
                             oldFrameCount, o->animationFrameCount,
                             pos.x,
                             pos.y );    

            if( o->currentEmot != NULL ) {
                int numSlots = getEmotionNumObjectSlots();
                
                for( int e=0; e<numSlots; e++ ) {
                    int oID =
                        getEmotionObjectByIndex( o->currentEmot, e );
                    
                    if( oID != 0 ) {
                        
                        handleAnimSound( oID,
                                         0,
                                         t,
                                         oldFrameCount, o->animationFrameCount,
                                         pos.x,
                                         pos.y ); 
                        }
                    }
                }
            }
            
        
        
        
        if( o->lastAnimFade > 0 ) {
            
            if( o->lastAnimFade == 1 ) {
                // fade just started
                // check if it's necessary
                
                if( isAnimFadeNeeded( o->displayID, 
                                      o->lastAnim, o->curAnim ) ) {
                    // fade needed, do nothing
                    }
                else {
                    // fade not needed
                    // jump to end of it
                    o->lastAnimFade = 0;
                    }
                }
            

            o->lastAnimFade -= 0.05 * frameRateFactor;
            if( o->lastAnimFade < 0 ) {
                o->lastAnimFade = 0;

                if( o->futureAnimStack->size() > 0 ) {
                    // move on to next in stack
                    
                    addNewAnimDirect( 
                        o, 
                        o->futureAnimStack->getElementDirect( 0 ) );

                    // pop from stack                    
                    o->futureAnimStack->deleteElement( 0 );
                    }
                
                }
            }

        oldFrameCount = o->heldAnimationFrameCount;
        o->heldAnimationFrameCount += animSpeed / BASE_SPEED;
        o->lastHeldAnimationFrameCount += animSpeed / BASE_SPEED;
        
        if( o->curHeldAnim == moving ) {
            o->heldFrozenRotFrameCount += animSpeed / BASE_SPEED;
            }
        
        if( o->holdingID > 0 ) {
            handleAnimSound( o->holdingID, 0, o->curHeldAnim,
                             oldFrameCount, o->heldAnimationFrameCount,
                             o->currentPos.x,
                             o->currentPos.y );
            }
        

        if( o->lastHeldAnimFade > 0 ) {
            
            if( o->lastHeldAnimFade == 1 ) {
                // fade just started
                // check if it's necessary
                
                int heldDisplayID = o->holdingID;
                

                if( o->holdingID < 0 ) {
                    LiveObject *babyO = getGameObject( - o->holdingID );
            
                    if( babyO != NULL ) {    
                        heldDisplayID = babyO->displayID;
                        }
                    else {
                        heldDisplayID = 0;
                        }
                    }
                

                if( heldDisplayID > 0 &&
                    isAnimFadeNeeded( heldDisplayID, 
                                      o->lastHeldAnim, o->curHeldAnim ) ) {
                    // fade needed, do nothing
                    }
                else {
                    // fade not needed
                    // jump to end of it
                    o->lastHeldAnimFade = 0;
                    }
                }
            

            o->lastHeldAnimFade -= 0.05 * frameRateFactor;
            if( o->lastHeldAnimFade < 0 ) {
                o->lastHeldAnimFade = 0;
                
                if( o->futureHeldAnimStack->size() > 0 ) {
                    // move on to next in stack
                    
                    addNewHeldAnimDirect( 
                        o,
                        o->futureHeldAnimStack->getElementDirect( 0 ) );
                    
                    // pop from stack
                    o->futureHeldAnimStack->deleteElement( 0 );
                    }
                
                }
            }


        if( o->currentSpeed != 0 && o->pathToDest != NULL ) {

            GridPos curStepDest = o->pathToDest[ o->currentPathStep ];
            GridPos nextStepDest = o->pathToDest[ o->currentPathStep + 1 ];
            
            doublePair startPos = { (double)curStepDest.x, 
                                    (double)curStepDest.y };

            doublePair endPos = { (double)nextStepDest.x, 
                                  (double)nextStepDest.y };
            
            while( distance( o->currentPos, endPos ) <= o->currentSpeed &&
                   o->currentPathStep < o->pathLength - 2 ) {
                
                // speed too great, overshooting next step
                
                o->currentPathStep ++;
                o->numFramesOnCurrentStep = 0;
                
                nextStepDest = o->pathToDest[ o->currentPathStep + 1 ];
            
                endPos.x = nextStepDest.x;
                endPos.y = nextStepDest.y;
                }


            doublePair dir = normalize( sub( endPos, o->currentPos ) );
            

            double turnFactor = 0.35;
            
            if( o->currentPathStep == o->pathLength - 2 ) {
                // last segment
                // speed up turn toward final spot so that we
                // don't miss it and circle around it forever
                turnFactor = 0.5;
                }

            if( o->currentGridSpeed > 4 ) {
                // tighten turns as speed increases to avoid
                // circling around forever
                turnFactor *= o->currentGridSpeed / 4;
                }
            
            
            if( dot( dir, o->currentMoveDirection ) >= 0 ) {
                // a right angle turn or smaller

            
                o->currentMoveDirection = 
                    normalize( add( o->currentMoveDirection, 
                                    mult( dir, 
                                          turnFactor * frameRateFactor ) ) );
                }
            else {
                // a double-back in the path
                // don't tot smooth turn through this, because
                // it doesn't resolve
                // instead, just turn sharply
                o->currentMoveDirection = dir;
                }
            
            if( o->numFramesOnCurrentStep * o->currentSpeed  * frameRateFactor
                > 2 ) {
                // spent twice as much time reaching this tile as we should
                // may be circling
                // go directly there instead
                o->currentMoveDirection = dir;
                }

            if( o->currentGridSpeed * frameRateFactor > 12 ) {
                // at high speed, can't round turns at all without circling
                o->currentMoveDirection = dir;
                }
            
            
            

            // don't change flip unless moving substantially in x
            if( fabs( o->currentMoveDirection.x ) > 0.5 ) {
                if( o->currentMoveDirection.x > 0 ) {
                    o->holdingFlip = false;
                    }
                else {
                    o->holdingFlip = true;
                    }         
                }
            

            if( o->currentPathStep < o->pathLength - 2 ) {

                o->currentPos = add( o->currentPos,
                                     mult( o->currentMoveDirection,
                                           o->currentSpeed ) );
                
                addNewAnim( o, moving );
                
                
                if( pathStepDistFactor * distance( o->currentPos,
                                    startPos )
                    >
                    distance( o->currentPos,
                              endPos ) ) {
                    
                    o->currentPathStep ++;
                    o->numFramesOnCurrentStep = 0;
                    }
                }
            else {

                if( o->id == ourID && mouseDown && shouldMoveCamera &&
                    o->pathLength > 2 ) {
                    
                    float worldMouseX, worldMouseY;
                    
                    screenToWorld( lastScreenMouseX,
                                   lastScreenMouseY,
                                   &worldMouseX,
                                   &worldMouseY );

                    doublePair worldMouse = { worldMouseX, worldMouseY };
                    
                    doublePair worldCurrent = mult( o->currentPos,
                                                    CELL_D );
                    doublePair delta = sub( worldMouse, worldCurrent );

                    // if player started by clicking on nothing
                    // allow continued movement right away
                    // however, if they started by clicking on something
                    // make sure they are really holding the mouse down
                    // (give them time to unpress the mouse)
                    if( nextActionMessageToSend == NULL ||
                        mouseDownFrames >  
                        minMouseDownFrames / frameRateFactor ) {
                        
                        double absX = fabs( delta.x );
                        double absY = fabs( delta.y );
                        

                        if( absX > CELL_D * 1 
                            ||
                            absY > CELL_D * 1 ) {
                            
                            if( absX < CELL_D * 4 
                                &&
                                absY < CELL_D * 4 
                                &&
                                mouseDownFrames >  
                                minMouseDownFrames / frameRateFactor ) {
                                
                                // they're holding mouse down very close
                                // to to their character
                                
                                // throw mouse way out, further in the same
                                // direction
                                
                                // we don't want to repeatedly find a bunch
                                // of short-path moves when mouse
                                // is held down
                            
                                doublePair mouseVector =
                                    mult( 
                                        normalize( 
                                            sub( worldMouse, worldCurrent ) ),
                                        CELL_D * 4 );
                                
                                doublePair fakeClick = add( worldCurrent,
                                                            mouseVector );
                                
                                o->useWaypoint = true;
                                // leave some wiggle room here
                                // path through waypoint might get extended
                                // if it involves obstacles
                                o->maxWaypointPathLength = 10;
                                
                                o->waypointX = lrint( worldMouseX / CELL_D );
                                o->waypointY = lrint( worldMouseY / CELL_D );

                                pointerDown( fakeClick.x, fakeClick.y );
                               
                                o->useWaypoint = false;
                                }
                            else {
                                pointerDown( worldMouseX, worldMouseY );
                                }
                            }
                        }
                    }
                else if( o->id == ourID && o->pathLength >= 2 &&
                         nextActionMessageToSend == NULL &&
                         distance( endPos, o->currentPos )
                         < o->currentSpeed ) {

                    // reached destination of bare-ground click

                    // check for auto-walk on road

                    GridPos prevStep = o->pathToDest[ o->pathLength - 2 ];
                    GridPos finalStep = o->pathToDest[ o->pathLength - 1 ];
                    
                    int mapIP = getMapIndex( prevStep.x, prevStep.y );
                    int mapIF = getMapIndex( finalStep.x, finalStep.y );
                    
                    if( mapIF != -1 && mapIP != -1 ) {
                        int floor = mMapFloors[ mapIF ];
                        
                        if( floor > 0 && mMapFloors[ mapIP ] == floor && 
                            getObject( floor )->rideable ) {
                            
                            // rideable floor is a road!
                            
                            int xDir = finalStep.x - prevStep.x;
                            int yDir = finalStep.y - prevStep.y;
                            
                            GridPos nextStep = finalStep;
                            nextStep.x += xDir;
                            nextStep.y += yDir;
                            
                            int len = 0;

                            if( isSameFloor( floor, finalStep, xDir, yDir ) ) {
                                // floor continues in same direction
                                // go as far as possible in that direction
                                // with next click
                                while( len < 5 && isSameFloor( floor, nextStep,
                                                               xDir, yDir ) ) {
                                    nextStep.x += xDir;
                                    nextStep.y += yDir;
                                    len ++;
                                    }
                                }
                            else {
                                nextStep = finalStep;
                                char foundPerp = false;
                                
                                // first step in same dir goes off floor
                                // try a perp move instead
                                if( xDir != 0 && yDir == 0 ) {
                                    xDir = 0;
                                    yDir = 1;
                                    
                                    if( isSameFloor( floor, finalStep, xDir,
                                                     yDir ) ) {
                                        foundPerp = true;
                                        }
                                    else {
                                        yDir = -1;
                                        if( isSameFloor( floor, finalStep, xDir,
                                                         yDir ) ) {
                                            foundPerp = true;
                                            }
                                        }
                                    }
                                else if( xDir == 0 && yDir != 0 ) {
                                    xDir = 1;
                                    yDir = 0;
                                    
                                    if( isSameFloor( floor, finalStep, xDir,
                                                     yDir ) ) {
                                        foundPerp = true;
                                        }
                                    else {
                                        xDir = -1;
                                        if( isSameFloor( floor, finalStep, xDir,
                                                         yDir ) ) {
                                            foundPerp = true;
                                            }
                                        }
                                    }

                                if( foundPerp ) {
                                    nextStep.x += xDir;
                                    nextStep.y += yDir;
                                    
                                    while( len < 5 &&
                                           isSameFloor( floor, nextStep,
                                                        xDir, yDir ) ) {
                                        nextStep.x += xDir;
                                        nextStep.y += yDir;
                                        len++;
                                        }
                                    }
                                }

                            if( ! equal( nextStep, finalStep ) ) {
                                // found straight-line continue of road
                                // auto-click there (but don't hold
                                
                                // avoid clicks on self and objects
                                // when walking on road
                                mForceGroundClick = true;
                                pointerDown( nextStep.x * CELL_D, 
                                             nextStep.y * CELL_D );
                                
                                pointerUp( nextStep.x * CELL_D, 
                                           nextStep.y * CELL_D );
                                
                                mForceGroundClick = false;
                                
                                endPos.x = (double)( nextStep.x );
                                endPos.y = (double)( nextStep.y );
                                }
                            }
                        }
                    }
                

                if( distance( endPos, o->currentPos )
                    < o->currentSpeed ) {

                    // reached destination
                    o->currentPos = endPos;
                    o->currentSpeed = 0;
                    o->currentGridSpeed = 0;
                    
                    playPendingReceivedMessages( o );
                    
                    //trailColor.r = randSource2.getRandomBoundedDouble( 0, .5 );
                    //trailColor.g = randSource2.getRandomBoundedDouble( 0, .5 );
                    //trailColor.b = randSource2.getRandomBoundedDouble( 0, .5 );
                    

                    if( ( o->id != ourID && 
                          ! o->somePendingMessageIsMoreMovement ) 
                        ||
                        ( o->id == ourID && 
                          nextActionMessageToSend == NULL ) ) {
                        
                        // simply stop walking
                        if( o->holdingID != 0 ) {
                            addNewAnim( o, ground2 );
                            }
                        else {
                            addNewAnim( o, ground );
                            }
                        }

                    printf( "Reached dest (%.0f,%.0f) %f seconds early\n",
                            endPos.x, endPos.y,
                            o->moveEtaTime - game_getCurrentTime() );
                    }
                else {

                    addNewAnim( o, moving );
                    
                    o->currentPos = add( o->currentPos,
                                         mult( o->currentMoveDirection,
                                               o->currentSpeed ) );

                    if( 1.5 * distance( o->currentPos,
                                        startPos )
                        >
                        distance( o->currentPos,
                                  endPos ) ) {
                        
                        o->onFinalPathStep = true;
                        }
                    }
                }
            
            // correct move speed based on how far we have left to go
            // and eta wall-clock time
            
            // make this correction once per second
            if( game_getCurrentTime() - o->timeOfLastSpeedUpdate
                > .25 ) {
    
                //updateMoveSpeed( o );
                }
            
            o->numFramesOnCurrentStep++;
            }
        
        
        
        double progressInc = 0.025 * frameRateFactor;

        if( o->id == ourID &&
            ( o->pendingAction || o->pendingActionAnimationProgress != 0 ) ) {
            
            o->pendingActionAnimationProgress += progressInc;
            
            if( o->pendingActionAnimationProgress > 1 ) {
                if( o->pendingAction ) {
                    // still pending, wrap around smoothly
                    o->pendingActionAnimationProgress -= 1;
                    }
                else {
                    // no longer pending, finish last cycle by snapping
                    // back to 0
                    o->pendingActionAnimationProgress = 0;
                    o->actionTargetTweakX = 0;
                    o->actionTargetTweakY = 0;
                    }
                }
            }
        else if( o->id != ourID && o->pendingActionAnimationProgress != 0 ) {
            
            o->pendingActionAnimationProgress += progressInc;
            
            if( o->pendingActionAnimationProgress > 1 ) {
                // no longer pending, finish last cycle by snapping
                // back to 0
                o->pendingActionAnimationProgress = 0;
                o->actionTargetTweakX = 0;
                o->actionTargetTweakY = 0;
                }
            }
        }
    
    
    // step fades on location-based speech
    if( !mapPullMode )
    for( int i=0; i<locationSpeech.size(); i++ ) {
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
            playerActionTargetNotAdjacent ) ) {
        
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



    if( ourLiveObject != NULL ) {
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
}

char LivingLifePage::isSameFloor( int inFloor, GridPos inFloorPos, 
                                  int inDX, int inDY ) {    
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
	OneLife::game::Debug::writeMethodInfo("void LivingLifePage::makeActive(...)");
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

    

    if( !inFresh ) {
        return;
        }

    mGlobalMessageShowing = false;
    mGlobalMessageStartTime = 0;
    mGlobalMessagesToDestroy.deallocateStringElements();
    
    
    offScreenSounds.deleteAll();
    
    oldHomePosStack.deleteAll();
    
    oldHomePosStack.push_back_other( &homePosStack );
    

    takingPhoto = false;
    photoSequenceNumber = -1;
    waitingForPhotoSig = false;
    if( photoSig != NULL ) {
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

	this->sendSignal(SIGNAL::DISCONNECT);
    //if(this->socket->isConnected())this->socket->disconnect();
    connectionMessageFade = 1.0f;

    
    mPreviousHomeDistStrings.deallocateStringElements();
    mPreviousHomeDistFades.deleteAll();

    mForceHintRefresh = false;
    mLastHintSortedSourceID = 0;
    mLastHintSortedList.deleteAll();

    for( int i=0; i<NUM_HINT_SHEETS; i++ ) {
        mHintTargetOffset[i] = mHintHideOffset[i];
        mHintPosOffset[i] = mHintHideOffset[i];
        }

    mCurrentHintObjectID = 0;
    mCurrentHintIndex = 0;
    
    mNextHintObjectID = 0;
    mNextHintIndex = 0;
    
    
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


    int tutorialDone = SettingsManager::getIntSetting( "tutorialDone", 0 );
    
    if( ! tutorialDone ) {
        mTutorialNumber = 1;
        }
    else {
        mTutorialNumber = 0;
        }
    
    if( mForceRunTutorial ) {
        mTutorialNumber = 1;
        mForceRunTutorial = false;
        }

    mLiveTutorialSheetIndex = -1;
    mLiveCravingSheetIndex = -1;
    
    for( int i=0; i<NUM_HINT_SHEETS; i++ ) {    
        mTutorialTargetOffset[i] = mTutorialHideOffset[i];
        mTutorialPosOffset[i] = mTutorialHideOffset[i];
        mTutorialMessage[i] = "";

        mCravingTargetOffset[i] = mCravingHideOffset[i];
        mCravingPosOffset[i] = mCravingHideOffset[i];
        if( mCravingMessage[i] != NULL ) {
            delete [] mCravingMessage[i];
            mCravingMessage[i] = NULL;
            }
        }
    
    

    savingSpeechEnabled = SettingsManager::getIntSetting( "allowSavingSpeech",
                                                          0 );


    for( int i=0; i<mGraveInfo.size(); i++ ) {
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

    for( int i=0; i<3; i++ ) {    
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
    
    
    for( int i=0; i<NUM_YUM_SLIPS; i++ ) {
        mYumSlipPosOffset[i] = mYumSlipHideOffset[i];
        mYumSlipPosTargetOffset[i] = mYumSlipHideOffset[i];
        mYumSlipNumberToShow[i] = 0;
        }
    

    mCurrentArrowI = 0;
    mCurrentArrowHeat = -1;
    if( mCurrentDes != NULL ) {
        delete [] mCurrentDes;
        }
    mCurrentDes = NULL;

    if( mCurrentLastAteString != NULL ) {
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
    mStartedLoadingFirstObjectSet = false;
    mDoneLoadingFirstObjectSet = false;
    mFirstObjectSetLoadingProgress = 0;

    playerActionPending = false;
    
    waitingForPong = false;
    lastPingSent = 0;
    lastPongReceived = 0;
    

    serverSocketBuffer.deleteAll();

    if( nextActionMessageToSend != NULL ) {    
        delete [] nextActionMessageToSend;
        nextActionMessageToSend = NULL;
        }
    

    for( int i=0; i<NUM_HOME_ARROWS; i++ ) {
        mHomeArrowStates[i].solid = false;
        mHomeArrowStates[i].fade = 0;
        }
    }

void LivingLifePage::checkForPointerHit(
		PointerHitRecord *inRecord,
		float inX,
		float inY )
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
    if( mForceGroundClick ) return;
    
    int clickDestMapX = clickDestX - mMapOffsetX + mMapD / 2;
    int clickDestMapY = clickDestY - mMapOffsetY + mMapD / 2;
    int clickDestMapI = clickDestMapY * mMapD + clickDestMapX;
    if( clickDestMapY >= 0 && clickDestMapY < mMapD && clickDestMapX >= 0 && clickDestMapX < mMapD )
	{
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
            
            for( int i=0; i<mPrevMouseOverCells.size(); i++ )
			{
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

void LivingLifePage::pointerMove( float inX, float inY ) {
    lastMouseX = inX;
    lastMouseY = inY;
        
    getLastMouseScreenPos( &lastScreenMouseX, &lastScreenMouseY );

    if( showBugMessage ) {
        return;
        }

    if( this->socket->isClosed() ) {//TODO: call connectPage if socket closed
        // dead
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

char LivingLifePage::getCellBlocksWalking( int inMapX, int inMapY ) {
    
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

void LivingLifePage::pointerDown( float inX, float inY ) {
	if (minitech::livingLifePageMouseDown( inX, inY )) return;
	
    lastMouseX = inX;
    lastMouseY = inY;

    if( showBugMessage ) {
        return;
        }
    
    if( this->socket->isClosed() ) {
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

void LivingLifePage::pointerDrag( float inX, float inY ) {
    lastMouseX = inX;
    lastMouseY = inY;
    getLastMouseScreenPos( &lastScreenMouseX, &lastScreenMouseY );
    
    if( showBugMessage ) {
        return;
        }
    }

void LivingLifePage::pointerUp( float inX, float inY ) {
    lastMouseX = inX;
    lastMouseY = inY;

    if( showBugMessage ) {
        return;
        }
    

    if( this->socket->isClosed() ) {//TODO: call connectPage if socket closed
        // dead
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

void LivingLifePage::keyDown( unsigned char inASCII ) {
    
    registerTriggerKeyCommand( inASCII, this );


    if( this->socket->isClosed() ) {
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
                this->socket->isConnected() &&
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
                this->socket->isConnected() &&
                vogMode ) {
                sendToServerSocket( (char*)"VOGP 0 0#" );
                }
            break;
        case 'M':
            if( ! mSayField.isFocused() &&
                this->socket->isConnected() &&
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
                
                this->socket->close();
                
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
                            
                            //LiveObject *ourLiveObject = getOurLiveObject();//TODO: uncomment if needed (cause warning otherwise)
                            
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
                                this->socket->close();
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

void LivingLifePage::specialKeyDown( int inKeyCode ) {
    if( showBugMessage ) {
        return;
        }
    
    if( this->socket->isClosed() ) {
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

void LivingLifePage::actionPerformed( GUIComponent *inTarget ) {
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

ExtraMapObject LivingLifePage::copyFromMap( int inMapI ) {
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

void LivingLifePage::putInMap( int inMapI, ExtraMapObject *inObj ) {
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

