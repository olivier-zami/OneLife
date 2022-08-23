//
// Created by olivier on 23/08/2022.
//

LiveObject *ourLiveObject = getOurLiveObject();

if( ourLiveObject != NULL ) {


int lastAteID, lastAteFillMax;

int responsiblePlayerID = -1;

int foodStore;
int foodCapacity;
double lastSpeed;

int oldYumBonus = mYumBonus;
mYumBonus = 0;

int oldYumMultiplier = mYumMultiplier;
mYumMultiplier = 0;


sscanf( message, "FX\n%d %d %d %d %lf %d %d %d",
&( foodStore ),
&( foodCapacity ),
&( lastAteID ),
&( lastAteFillMax ),
&( lastSpeed ),
&responsiblePlayerID,
&mYumBonus, &mYumMultiplier );



if( oldYumBonus != mYumBonus ) {
// pull out of old stack, if present
for( int i=0; i<mOldYumBonus.size(); i++ ) {
if( mOldYumBonus.getElementDirect( i ) == mYumBonus ) {
mOldYumBonus.deleteElement( i );
i--;
}
}

// fade existing
for( int i=0; i<mOldYumBonus.size(); i++ ) {
float fade =
		mOldYumBonusFades.getElementDirect( i );

if( fade > 0.5 ) {
fade -= 0.20;
}
else {
fade -= 0.1;
}

*( mOldYumBonusFades.getElement( i ) ) = fade;
if( fade <= 0 ) {
mOldYumBonus.deleteElement( i );
mOldYumBonusFades.deleteElement( i );
i--;
}
}

if( oldYumBonus != 0 ) {
// push on top of stack
mOldYumBonus.push_back( oldYumBonus );
mOldYumBonusFades.push_back( 1.0f );
}
}

if( mYumMultiplier != oldYumMultiplier ) {
int oldSlipIndex = -1;
int newSlipIndex = 0;

for( int i=0; i<2; i++ ) {
if( mYumSlipNumberToShow[i] == oldYumMultiplier ) {
oldSlipIndex = i;
newSlipIndex = ( i + 1 ) % 2;
}
}
if( oldSlipIndex != -1 ) {
mYumSlipPosTargetOffset[ oldSlipIndex ] =
mYumSlipHideOffset[ oldSlipIndex ];
}

mYumSlipPosTargetOffset[ newSlipIndex ] =
mYumSlipHideOffset[ newSlipIndex ];

if( mYumMultiplier > 0 ) {
mYumSlipPosTargetOffset[ newSlipIndex ].y += 36;
}
mYumSlipNumberToShow[ newSlipIndex ] = mYumMultiplier;
}




if( responsiblePlayerID != -1 &&
getLiveObject( responsiblePlayerID ) != NULL &&
		getLiveObject( responsiblePlayerID )->
pendingReceivedMessages.size() > 0 ) {
// someone else fed us, and they're still in the
// middle of a local walk with updates pending after
// they finish

// defer this food update too

LiveObject *rO = getLiveObject( responsiblePlayerID );

printf( "Holding FX message caused by %d until later, "
"%d other messages pending for them\n",
responsiblePlayerID,
rO->pendingReceivedMessages.size() );
rO->pendingReceivedMessages.push_back(
		stringDuplicate( message ) );
}
else {
char foodIncreased = false;
int oldFoodStore = ourLiveObject->foodStore;

if( foodCapacity == ourLiveObject->foodCapacity &&
		foodStore > ourLiveObject->foodStore ) {
foodIncreased = true;
}

ourLiveObject->foodStore = foodStore;
ourLiveObject->foodCapacity = foodCapacity;
ourLiveObject->lastSpeed = lastSpeed;


if( mCurrentLastAteString != NULL ) {


// one to add to erased list
// fade older ones first

for( int i=0; i<mOldLastAteStrings.size(); i++ ) {
float fade =
		mOldLastAteFades.getElementDirect( i );

if( fade > 0.5 ) {
fade -= 0.20;
}
else {
fade -= 0.1;
}

*( mOldLastAteFades.getElement( i ) ) = fade;


// bar must fade slower (different blending mode)
float barFade =
		mOldLastAteBarFades.getElementDirect( i );

barFade -= 0.01;

*( mOldLastAteBarFades.getElement( i ) ) = barFade;


if( fade <= 0 ) {
mOldLastAteStrings.deallocateStringElement( i );
mOldLastAteFillMax.deleteElement( i );
mOldLastAteFades.deleteElement( i );
mOldLastAteBarFades.deleteElement( i );
i--;
}

else if(
strcmp(
		mCurrentLastAteString,
		mOldLastAteStrings.getElementDirect(i) )
== 0 ) {
// already in stack, move to top
mOldLastAteStrings.deallocateStringElement( i );
mOldLastAteFillMax.deleteElement( i );
mOldLastAteFades.deleteElement( i );
mOldLastAteBarFades.deleteElement( i );
i--;
}
}

mOldLastAteStrings.push_back( mCurrentLastAteString );
mOldLastAteFillMax.push_back( mCurrentLastAteFillMax );
mOldLastAteFades.push_back( 1.0f );
mOldLastAteBarFades.push_back( 1.0f );

mCurrentLastAteString = NULL;
mCurrentLastAteFillMax = 0;
}

if( lastAteID != 0 ) {
ObjectRecord *lastAteObj = getObject( lastAteID );

char *strUpper = stringToUpperCase(
		lastAteObj->description );

stripDescriptionComment( strUpper );

const char *key = "lastAte";

if( lastAteObj->permanent ) {
key = "lastAtePermanent";
}

mCurrentLastAteString =
autoSprintf( "%s %s",
			 translate( key ),
			 strUpper );
delete [] strUpper;

mCurrentLastAteFillMax = lastAteFillMax;
}
else if( foodIncreased ) {
// we were fed, but we didn't eat anything
// must have been breast milk
mCurrentLastAteString =
stringDuplicate( translate( "breastMilk" ) );

mCurrentLastAteFillMax = oldFoodStore;
}


printf( "Our food = %d/%d\n",
ourLiveObject->foodStore,
ourLiveObject->foodCapacity );


if( ourLiveObject->foodStore >
ourLiveObject->maxFoodStore ) {

ourLiveObject->maxFoodStore = ourLiveObject->foodStore;
}
if( ourLiveObject->foodCapacity >
ourLiveObject->maxFoodCapacity ) {

ourLiveObject->maxFoodCapacity =
ourLiveObject->foodCapacity;
}
if( ourLiveObject->foodStore ==
ourLiveObject->foodCapacity ) {

mPulseHungerSound = false;

mHungerSlipVisible = 0;
}
else if( ourLiveObject->foodStore + mYumBonus <= 4 &&
computeCurrentAge( ourLiveObject ) < 117.33 ) {

// don't play hunger sounds at end of life
// because it interrupts our end-of-life song
// currently it's 2:37 long


// quiet music so hunger sound can be heard
setMusicLoudness( 0 );
mHungerSlipVisible = 2;

if( ourLiveObject->foodStore > 0 ) {

if( ourLiveObject->foodStore > 1 ) {
if( mHungerSound != NULL ) {
// make sure it can be heard
// even if paused
setSoundLoudness( 1.0 );
playSoundSprite( mHungerSound,
		getSoundEffectsLoudness(),
// middle
0.5 );
}
mPulseHungerSound = false;
}
else {
mPulseHungerSound = true;
}
}
}
else if( ourLiveObject->foodStore + mYumBonus <= 8 ) {
mHungerSlipVisible = 1;
mPulseHungerSound = false;
}
else {
mHungerSlipVisible = -1;
}

if( ourLiveObject->foodStore + mYumBonus > 4 ||
computeCurrentAge( ourLiveObject ) >= 57 ) {
// restore music
setMusicLoudness( musicLoudness );

mPulseHungerSound = false;
}

}
}