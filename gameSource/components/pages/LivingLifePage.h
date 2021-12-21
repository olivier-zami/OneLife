#ifndef LIVING_LIFE_PAGE_INCLUDED
#define LIVING_LIFE_PAGE_INCLUDED

#include <vector>
#include "minorGems/ui/event/ActionListener.h"
#include "minorGems/util/SimpleVector.h"
#include "minorGems/util/SettingsManager.h"
#include "OneLife/gameSource/game.h"
#include "OneLife/gameSource/transitionBank.h"
#include "OneLife/gameSource/controller.h"
#include "OneLife/gameSource/Picker.h"
#include "OneLife/gameSource/procedures/ai/pathFind.h"
#include "OneLife/gameSource/animationBank.h"
#include "OneLife/gameSource/emotion.h"
#include "OneLife/gameSource/TextField.h"
#include "../../components/socket.h"
#include "OneLife/gameSource/controllers/feature.h"
#include "OneLife/gameSource/dataTypes/misc.h"
#include "OneLife/gameSource/dataTypes/game.h"
#include "OneLife/gameSource/components/pages/menu/playerStatus.h"
#include "OneLife/gameSource/components/scenes/map.h"
#include "OneLife/gameSource/dataTypes/types/message.h"

class LivingLifePage :
		public Controller,
		public ActionListener
{
    public:
        LivingLifePage();
        ~LivingLifePage();

		void handle(OneLife::dataType::UiComponent* screen);

		void readMessage(const char* message);

	private:
		void readMessage(OneLife::data::type::message::MapChunk mapChunk, const char* message=nullptr);//TODO: put messsage content in mapChunk object

	public:

		void setServerSocket(OneLife::game::component::Socket* socket);
        void runTutorial();// enabled tutorail next time a connection loads
        char isMapBeingPulled();
        char *getDeathReason();// destroyed by caller can be NULL
        void adjustAllFrameCounts( double inOldFrameRateFactor, double inNewFrameRateFactor ); // prevent a jitter when frameRateFactor changes due to fps lag
        virtual void draw( doublePair inViewCenter,double inViewSize );
        virtual void step();
        virtual void makeActive( char inFresh );
        virtual void pointerMove( float inX, float inY );
        virtual void pointerDown( float inX, float inY );
        virtual void pointerDrag( float inX, float inY );
        virtual void pointerUp( float inX, float inY );
        virtual void keyDown( unsigned char inASCII );
        virtual void specialKeyDown( int inKeyCode );
        virtual void keyUp( unsigned char inASCII );
        void sendToServerSocket( char *inMessage );// handles error detection, total byte counting, etc.
        void sendBugReport( int inBugNumber );

		void setNextActionMessage( const char* str, int x, int y );
		int getObjId( int mapX, int mapY );
		bool objIdReverseAction( int objId );
		void pickUpBabyInRange();
		void pickUpBaby( int x, int y );
		void useBackpack( bool replace = false );
		void usePocket( int clothingID );
		void useOnSelf();
		void takeOffBackpack();
		void setOurSendPosXY(int &x, int &y);
		bool isCharKey(unsigned char c, unsigned char key);
		void actionAlphaRelativeToMe( int x, int y );
		void actionBetaRelativeToMe( int x, int y );
		void useTileRelativeToMe( int x, int y ) ;
		void dropTileRelativeToMe( int x, int y ) ;
		void movementStep();
		bool findNextMove(int &x, int &y, int dir);
		int getNextMoveDir(int direction, int add);
		int getMoveDirection();
		bool setMoveDirIfSafe(int &x, int &y, int dir);
		void setMoveDirection(int &x, int &y, int direction);
		bool tileHasClosedDoor(int x, int y);
		bool dirIsSafeToWalk(int x, int y, int dir);
		doublePair minitechGetLastScreenViewCenter();
		char *minitechGetDisplayObjectDescription(int objId);
		bool minitechSayFieldIsFocused() { return mSayField.isFocused(); }
        virtual void actionPerformed( GUIComponent *inTarget );
		int sendX( int inX );
		int sendY( int inY );
		int mMapD;
		int *mMap;

    protected:
		void update(OneLife::game::Feature* feature);

		OneLife::game::Map* localMap;
		std::vector<OneLife::game::Feature*> availableFeature;
        char mForceRunTutorial;
        int mTutorialNumber;
        char mGlobalMessageShowing;
        double mGlobalMessageStartTime;
        SimpleVector<char*>mGlobalMessagesToDestroy;
        int mFirstServerMessagesReceived;
        char mStartedLoadingFirstObjectSet;
        char mDoneLoadingFirstObjectSet;
        double mStartedLoadingFirstObjectSetStartTime;
        float mFirstObjectSetLoadingProgress;

        // an offset that we apply to all server-recieved coordinates
        // before storing them locally, and reverse-apply to all local
        // coordinates before sending them to the server.

        // This keeps our local coordinates in a low range and prevents
        // rounding errors caused by rendering huge integers as 32-bit floats
        // on most graphics cards.
        //
        // We base this on the center of the first map chunk received
        char mMapGlobalOffsetSet;
        GridPos mMapGlobalOffset;

        // conversion function for received coordinates into local coords
        void applyReceiveOffset( int *inX, int *inY );
        // converts local coors for sending back to server

		struct {
			bool debugMessageEnabled;
		}feature;

		OneLife::game::component::Socket* socket;
        int *mMapBiomes;
        int *mMapFloors;
        char *mMapCellDrawnFlags;
        double *mMapAnimationFrameCount;
        double *mMapAnimationLastFrameCount;
        double *mMapAnimationFrozenRotFrameCount;
        char *mMapAnimationFrozenRotFrameCountUsed;
        int *mMapFloorAnimationFrameCount;

        // all tiles on ground work their way toward animation type of
        // "ground" but may have a lingering types after being dropped
        AnimType *mMapCurAnimType;
        AnimType *mMapLastAnimType;
        double *mMapLastAnimFade;
        
        // 0,0 for most, except for a newly-dropped object
        // as it slides back into grid position
        doublePair *mMapDropOffsets;
        double *mMapDropRot;
        SoundUsage *mMapDropSounds;

        // 0, 0 for most, except objects that are moving
        doublePair *mMapMoveOffsets;
        // speed in CELL_D per sec
        double *mMapMoveSpeeds;

        // true if left-right flipped (to match last drop)
        // not tracked on server, so resets when object goes off of screen
        char *mMapTileFlips;
        SimpleVector<int> *mMapContainedStacks;
        SimpleVector< SimpleVector<int> > *mMapSubContainedStacks;

        // true if this map spot was something that our
        // player was responsible for placing
        char *mMapPlayerPlacedFlags;
        SimpleVector<GridPos> mMapExtraMovingObjectsDestWorldPos;
        SimpleVector<int> mMapExtraMovingObjectsDestObjectIDs;
        SimpleVector<ExtraMapObject> mMapExtraMovingObjects;

        public: // minitech
        int mMapOffsetX;
        int mMapOffsetY;
		protected: // minitech

        char mEKeyEnabled;
        char mEKeyDown;
        

        SpriteHandle mGuiPanelSprite;
        SpriteHandle mGuiBloodSprite;
        

        SpriteHandle mHungerBoxSprites[ NUM_HUNGER_BOX_SPRITES ];
        SpriteHandle mHungerBoxFillSprites[ NUM_HUNGER_BOX_SPRITES ];

        SpriteHandle mHungerBoxErasedSprites[ NUM_HUNGER_BOX_SPRITES ];
        SpriteHandle mHungerBoxFillErasedSprites[ NUM_HUNGER_BOX_SPRITES ];
        
        SpriteHandle mTempArrowSprites[ NUM_TEMP_ARROWS ];
        SpriteHandle mTempArrowErasedSprites[ NUM_TEMP_ARROWS ];

        SpriteHandle mHungerDashSprites[ NUM_HUNGER_DASHES ];
        SpriteHandle mHungerDashErasedSprites[ NUM_HUNGER_DASHES ];
        SpriteHandle mHungerBarSprites[ NUM_HUNGER_DASHES ];
        SpriteHandle mHungerBarErasedSprites[ NUM_HUNGER_DASHES ];

        SpriteHandle mNotePaperSprite;

        SpriteHandle mFloorSplitSprite;

        SpriteHandle mCellBorderSprite;
        SpriteHandle mCellFillSprite;
        

        SpriteHandle mHomeSlipSprite;
        SpriteHandle mHomeArrowSprites[ NUM_HOME_ARROWS ];
        SpriteHandle mHomeArrowErasedSprites[ NUM_HOME_ARROWS ];
		
		SpriteHandle sheetSprites[9] = {nullptr};
        
        HomeArrow mHomeArrowStates[ NUM_HOME_ARROWS ];

        SimpleVector<int> mCulvertStoneSpriteIDs;
        
        SimpleVector<char*> mPreviousHomeDistStrings;
        SimpleVector<float> mPreviousHomeDistFades;
        

        // offset from current view center
        doublePair mNotePaperHideOffset;
        doublePair mNotePaperPosOffset;
        doublePair mNotePaperPosTargetOffset;


        doublePair mHomeSlipHideOffset;
        doublePair mHomeSlipPosOffset;
        doublePair mHomeSlipPosTargetOffset;

        
        SimpleVector<char*> mLastKnownNoteLines;
        
        SimpleVector<char> mErasedNoteChars;
        SimpleVector<doublePair> mErasedNoteCharOffsets;
        SimpleVector<float> mErasedNoteCharFades;

        SimpleVector<char> mCurrentNoteChars;
        SimpleVector<doublePair> mCurrentNoteCharOffsets;
        
        SimpleVector<char*> mSentChatPhrases;

        
        SoundSpriteHandle mHungerSound;
        char mPulseHungerSound;

        SoundSpriteHandle mTutorialSound;
        SoundSpriteHandle mCurseSound;

        
        SpriteHandle mHungerSlipSprites[3];

        // offset from current view center
        doublePair mHungerSlipHideOffsets[3];
        doublePair mHungerSlipShowOffsets[3];
        doublePair mHungerSlipPosOffset[3];
        doublePair mHungerSlipPosTargetOffset[3];
        
        double mHungerSlipWiggleTime[3];
        double mHungerSlipWiggleAmp[3];
        double mHungerSlipWiggleSpeed[3];
        
        // for catching dir change at peak of starving slip
        // to time sound
        double mStarvingSlipLastPos[2];
                                    

        // index of visble one, or -1
        int mHungerSlipVisible;
        
        
        SpriteHandle mHintSheetSprites[NUM_HINT_SHEETS];
        
        // offset from current view center
        doublePair mHintHideOffset[NUM_HINT_SHEETS];
        doublePair mHintPosOffset[NUM_HINT_SHEETS];
        doublePair mHintTargetOffset[NUM_HINT_SHEETS];
        
        doublePair mHintExtraOffset[NUM_HINT_SHEETS];

        // # separates lines
        char *mHintMessage[NUM_HINT_SHEETS];
        int mHintMessageIndex[NUM_HINT_SHEETS];
        int mNumTotalHints[ NUM_HINT_SHEETS ];

        int mLiveHintSheetIndex;

        char mForceHintRefresh;
        
        int mCurrentHintObjectID;
        int mCurrentHintIndex;
        
        int mNextHintObjectID;
        int mNextHintIndex;

        SimpleVector<TransRecord *> mLastHintSortedList;
        int mLastHintSortedSourceID;
        char *mLastHintFilterString;
        
        // string that's waiting to be shown on hint-sheet 4
        char *mPendingFilterString;
        

        // table sized to number of possible objects
        int *mHintBookmarks;
        

        int getNumHints( int inObjectID );
        char *getHintMessage( int inObjectID, int inIndex );

        char *mHintFilterString;
        

        
        // offset from current view center
        doublePair mTutorialHideOffset[NUM_HINT_SHEETS];
        doublePair mTutorialPosOffset[NUM_HINT_SHEETS];
        doublePair mTutorialTargetOffset[NUM_HINT_SHEETS];

        doublePair mTutorialExtraOffset[NUM_HINT_SHEETS];

        // # separates lines
        const char *mTutorialMessage[NUM_HINT_SHEETS];

        char mTutorialFlips[NUM_HINT_SHEETS];

        int mLiveTutorialSheetIndex;
        int mLiveTutorialTriggerNumber;



        doublePair mCravingHideOffset[NUM_HINT_SHEETS];
        doublePair mCravingPosOffset[NUM_HINT_SHEETS];
        doublePair mCravingTargetOffset[NUM_HINT_SHEETS];

        doublePair mCravingExtraOffset[NUM_HINT_SHEETS];

        char *mCravingMessage[NUM_HINT_SHEETS];

        int mLiveCravingSheetIndex;
        
        void setNewCraving( int inFoodID, int inYumBonus );

        


        // relative to map corner, but not necessary in bounds
        // of locally stored map
        GridPos getMapPos( int inWorldX, int inWorldY );

        // -1 if outside bounds of locally stored map
        int getMapIndex( int inWorldX, int inWorldY );
        

        int mCurrentArrowI;
        float mCurrentArrowHeat;
        
        SimpleVector<OldArrow> mOldArrows;
        
        char *mCurrentDes;
        SimpleVector<char*> mOldDesStrings;
        SimpleVector<float> mOldDesFades;
        

        char *mCurrentLastAteString;
        int mCurrentLastAteFillMax;
        SimpleVector<char*> mOldLastAteStrings;
        SimpleVector<int> mOldLastAteFillMax;
        SimpleVector<float> mOldLastAteFades;
        SimpleVector<float> mOldLastAteBarFades;

        
        void drawHungerMaxFillLine( doublePair inAteWordsPos,
                                    int inMaxFill,
                                    SpriteHandle *inBarSprites,
                                    SpriteHandle *inDashSprites,
                                    char inSkipBar,
                                    char inSkipDashes );
        
        
        int mYumBonus;
        SimpleVector<int> mOldYumBonus;
        SimpleVector<float> mOldYumBonusFades;

        int mYumMultiplier;

        SpriteHandle mYumSlipSprites[ NUM_YUM_SLIPS ];
        int mYumSlipNumberToShow[ NUM_YUM_SLIPS ];
        doublePair mYumSlipHideOffset[ NUM_YUM_SLIPS ];
        doublePair mYumSlipPosOffset[ NUM_YUM_SLIPS ];
        doublePair mYumSlipPosTargetOffset[ NUM_YUM_SLIPS ];
        

        // the object that we're mousing over
        int mLastMouseOverID;
        int mCurMouseOverID;
        float mCurMouseOverFade;
        
        GridPos mCurMouseOverSpot;
        char mCurMouseOverBehind;

        GridPos mCurMouseOverWorld;

        
        char mCurMouseOverPerson;
        char mCurMouseOverSelf;
        

        SimpleVector<GridPos> mPrevMouseOverSpots;
        SimpleVector<char> mPrevMouseOverSpotsBehind;
        SimpleVector<float> mPrevMouseOverSpotFades;
        

        // the ground cell that we're mousing over
        GridPos mCurMouseOverCell;
        float mCurMouseOverCellFade;
        float mCurMouseOverCellFadeRate;
        
        GridPos mLastClickCell;
        

        SimpleVector<GridPos> mPrevMouseOverCells;
        SimpleVector<float> mPrevMouseOverCellFades;
        

        SimpleVector<GridPos> mPrevMouseClickCells;
        SimpleVector<float> mPrevMouseClickCellFades;
        

        float mLastMouseOverFade;

        SpriteHandle mChalkBlotSprite;
        SpriteHandle mPathMarkSprite;
        SpriteHandle mGroundOverlaySprite[4];
        
        SpriteHandle mTeaserArrowLongSprite;
        SpriteHandle mTeaserArrowMedSprite;
        SpriteHandle mTeaserArrowShortSprite;
        SpriteHandle mTeaserArrowVeryShortSprite;
        SpriteHandle mLineSegmentSprite;
        
        
        // not visible, but used for its text filtering
        // capabilities
        TextField mSayField;
        
        double mPageStartTime;

        void computePathToDest( LiveObject *inObject );
        
        double computePathSpeedMod( LiveObject *inObject, int inPathLength );
        
        // check if same floor is present when we take a step in x or y
        char isSameFloor( int inFloor, GridPos inFloorPos, int inDX, int inDY );
        
        // forces next pointerDown call to avoid everything but ground clicks
        char mForceGroundClick;
        

		public: // minitech
        LiveObject *getOurLiveObject();
        LiveObject *getLiveObject( int inID );
        protected: // minitech
		
		bool tileBlocked( int x, int y );
		void drunkWalk( GridPos *path, int pathLen, bool actionMove );
		bool isTripping();

        void clearLiveObjects();
        
        // inSpeaker can be NULL
        void drawChalkBackgroundString( doublePair inPos, 
                                        const char *inString,
                                        double inFade,
                                        double inMaxWidth,
                                        LiveObject *inSpeaker = NULL,
                                        int inForceMinChalkBlots = -1,
                                        FloatColor *inForceBlotColor = NULL,
                                        FloatColor *inForceTextColor = NULL );
        
        
        void drawOffScreenSounds();
        


        // returns an animation pack that can be used to draw the
        // held object.  The pack's object ID is -1 if nothing is held
        ObjectAnimPack drawLiveObject( 
            LiveObject *inObj,
            SimpleVector<LiveObject *> *inSpeakers,
            SimpleVector<doublePair> *inSpeakersPos );
        

        void drawMapCell( int inMapI, 
                          int inScreenX, int inScreenY,
                          char inHighlightOnly = false,
                          // blocks frame update for cell and animation sounds
                          char inNoTimeEffects = false );
        
        void checkForPointerHit( PointerHitRecord *inRecord,
                                 float inX, float inY );
        


        void handleOurDeath( char inDisconnect = false );
        

        char *mDeathReason;
        

        double mRemapDelay;
        double mRemapPeak;
        double mRemapDirection;
        double mCurrentRemapFraction;
        
        

        ExtraMapObject copyFromMap( int inMapI );
        
        void putInMap( int inMapI, ExtraMapObject *inObj );
        

        char getCellBlocksWalking( int inMapX, int inMapY );
        
        
        char mShowHighlights;


        void handleAnimSound( int inObjectID, double inAge, 
                              AnimType inType,
                              int inOldFrameCount, int inNewFrameCount,
                              double inPosX, double inPosY );
        

        SimpleVector<GraveInfo> mGraveInfo;

        SimpleVector<OwnerInfo> mOwnerInfo;
        
        void clearOwnerInfo();
    

        // end the move of an extra moving object and stick it back
        // in the map at its destination.
        // inExtraIndex is its index in the mMapExtraMovingObjects vectors
        void endExtraObjectMove( int inExtraIndex );
        

        char mUsingSteam;
        char mZKeyDown;

        
        //FOV
        void changeHUDFOV( float newScale = 1.0f );
        void changeFOV( float newScale = 1.0f );
        void calcOffsetHUD();
        void calcFontScale( float newScale, Font* font );

        char mPlayerInFlight;

        Picker mObjectPicker;

	private:
		const char* screenName = "LivingLifePage";
    };


#endif
