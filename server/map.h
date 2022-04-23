#ifndef MAP_H_INCLUDED
#define MAP_H_INCLUDED



#include "../third_party/minorGems/system/Time.h"

#include "../gameSource/GridPos.h"
#include "../gameSource/transitionBank.h"

#include "minorGems/game/doublePair.h"
#include "component/Map.h"

typedef struct GlobalTriggerState
{
	SimpleVector<GridPos> triggerOnLocations;

	// receivers for this trigger that are waiting to be turned on
	SimpleVector<GridPos> receiverLocations;

	SimpleVector<GridPos> triggeredLocations;
	SimpleVector<int>     triggeredIDs;
	// what we revert to when global trigger turns off (back to receiver)
	SimpleVector<int> triggeredRevertIDs;
} GlobalTriggerState;

// make Eve placement radius bigger
void doubleEveRadius();

// return Eve placement radius to starting value
void resetEveRadius();

// save recent placements on Eve's death so that this player can spawn
// near them if they are ever Eve again
void mapEveDeath( const char *inEmail, double inAge, GridPos inDeathMapPos );

// sets the player responsible for subsequent map changes
// meant to track who set down an object
// should be set to -1 (default) except for object set-down
void setResponsiblePlayer( int inPlayerID );

char isMapSpotBlocking( int inX, int inY );

// is the object returned by getMapObject still in motion with
// destination inX, inY
char isMapObjectInTransit( int inX, int inY );

// removes contained item from specified slot, or remove from top of stack
// if inSlot is -1
// if inSubCont = 0, then sub-container in that slot is cleared by this call
int removeContained( int inX, int inY, int inSlot, timeSec_t *outEtaDecay,
                     int inSubCont = 0 );

// line for a map change message
char *getMapChangeLineString( ChangePosition inPos );

char *getMapChangeLineString( MapChangeRecord *inRecord,
                              int inRelativeToX, int inRelativeToY );

// returns true on success
// example:
// loadTutorial( newPlayer.tutorialLoad, "tutorialA.txt", 10000, 10000 )
char loadTutorialStart( TutorialLoadProgress *inTutorialLoad,
                        const char *inMapFileName, int inX, int inY );

// returns true if more steps are needed
// false if done
char loadTutorialStep( TutorialLoadProgress *inTutorialLoad,
                       double inTimeLimitSec );

// inBuffer must be at least MAP_METADATA_LENGTH bytes
// returns true if metadata found
char getMetadata( int inMapID, unsigned char *inBuffer );

// returns full map ID with embedded metadata ID for new metadata record
int addMetadata( int inObjectID, unsigned char *inBuffer );

// returned vector NOT destroyed or modified by caller
SimpleVector<GridPos> *getSpeechPipesOut( int inIndex );

// next landing strip in line, in round-the-world circuit across all
// landing positions
// radius limit limits flights from inside that square radius
// from leaving (though flights from outside are unrestriced)
GridPos getNextFlightLandingPos( int inCurrentX, int inCurrentY,
                                 doublePair inDir,
                                 int inRadiusLimit = -1 );

// get and set player ID for grave on map
// returns 0 if not found
int getGravePlayerID( int inX, int inY );
void setGravePlayerID( int inX, int inY, int inPlayerID );

// culling regions of map that haven't been seen in a long time
void stepMapLongTermCulling( int inNumCurrentPlayers );

#endif
