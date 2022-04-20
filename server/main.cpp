#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <math.h>
#include <assert.h>
#include <float.h>
#include <random>
#include <string>

//2HOL, password-protected objects: <fstream>, <iostream> added to handle runtime storage of in-game passwords
#include <fstream>
#include <iostream>
//  <time.h> added to add time stamps to recorded data
#include <time.h>

#include "Server.h"

#include "minorGems/util/stringUtils.h"
#include "minorGems/util/SettingsManager.h"
#include "minorGems/util/SimpleVector.h"
#include "minorGems/network/SocketServer.h"
#include "minorGems/network/SocketPoll.h"
#include "minorGems/network/web/WebRequest.h"
#include "minorGems/network/web/URLUtils.h"

#include "minorGems/crypto/hashes/sha1.h"

#include "minorGems/system/Thread.h"
#include "minorGems/system/Time.h"

#include "minorGems/game/doublePair.h"

#include "minorGems/util/log/AppLog.h"
#include "minorGems/util/log/FileLog.h"

#include "minorGems/formats/encodingUtils.h"

#include "minorGems/io/file/File.h"

#include "map.h"
#include "component/Map.h"
#include "component/Speech.h"
#include "../gameSource/transitionBank.h"
#include "../gameSource/objectBank.h"
#include "../gameSource/objectMetadata.h"
#include "../gameSource/animationBank.h"
#include "../gameSource/categoryBank.h"
#include "../commonSource/sayLimit.h"

#include "lifeLog.h"
#include "foodLog.h"
#include "backup.h"
#include "triggers.h"
#include "playerStats.h"
#include "lineageLog.h"
#include "serverCalls.h"
#include "failureLog.h"
#include "names.h"
#include "curses.h"
#include "lineageLimit.h"
#include "objectSurvey.h"
#include "language.h"
#include "familySkipList.h"
#include "lifeTokens.h"
#include "fitnessScore.h"
#include "arcReport.h"
#include "curseDB.h"
#include "cravings.h"


#include "minorGems/util/random/JenkinsRandomSource.h"

#include "Debug.h"


//#define IGNORE_PRINTF

#ifdef IGNORE_PRINTF
#define printf(fmt, ...) (0)
#endif


#include "../gameSource/GridPos.h"
#include "component/feature/apocalypse.h"
#include "Server.h"
#include "dataType/Settings.h"

#define PERSON_OBJ_ID 12

extern SimpleVector<LiveObject> players;
extern int apocalypsePossible;
extern char apocalypseTriggered;
extern GridPos apocalypseLocation;

float targetHeat = 10;
double secondsPerYear = 60.0;
int minPickupBabyAge = 10;
int babyAge = 5;

// age when bare-hand actions become available to a baby (opening doors, etc.)
int defaultActionAge = 3;


double forceDeathAge = 120;
// UncleGus Custom Variables
double adultAge = 20;
double oldAge = 104;
double fertileAge = 14;
// End UncleGus Custom Variables
double minSayGapInSeconds = 1.0;
int maxLineageTracked = 20;
char apocalypseRemote = false;
int lastApocalypseNumber = 0;
double apocalypseStartTime = 0;
char apocalypseStarted = false;
char postApocalypseStarted = false;


double remoteApocalypseCheckInterval = 30;
double lastRemoteApocalypseCheckTime = 0;
WebRequest *apocalypseRequest = NULL;



char monumentCallPending = false;
int monumentCallX = 0;
int monumentCallY = 0;
int monumentCallID = 0;

static FILE *familyDataLogFile = NULL;
static JenkinsRandomSource randSource;
static double minFoodDecrementSeconds = 5.0;
static double maxFoodDecrementSeconds = 20;
static int babyBirthFoodDecrement = 10;

// bonus applied to all foods
// makes whole server a bit easier (or harder, if negative)
static int eatBonus = 0;

// static double eatBonusFloor = 0;
// static double eatBonusHalfLife = 50;

static int canYumChainBreak = 0;

static double minAgeForCravings = 10;


// static double posseSizeSpeedMultipliers[4] = { 0.75, 1.25, 1.5, 2.0 };



static int minActivePlayersForLanguages = 15;


// keep a running sequence number to challenge each connecting client
// to produce new login hashes, avoiding replay attacks.
static unsigned int nextSequenceNumber = 1;


static int requireClientPassword = 1;
static int requireTicketServerCheck = 1;
static char *clientPassword = NULL;
static char *ticketServerURL = NULL;
static char *reflectorURL = NULL;

// larger of dataVersionNumber.txt or serverCodeVersionNumber.txt
static int versionNumber = 1;


static double childSameRaceLikelihood = 0.9;
static int familySpan = 2;


// phrases that trigger baby and family naming
static SimpleVector<char*> nameGivingPhrases;
static SimpleVector<char*> familyNameGivingPhrases;
static SimpleVector<char*> cursingPhrases;

char *curseYouPhrase = NULL;
char *curseBabyPhrase = NULL;

static SimpleVector<char*> forgivingPhrases;
static SimpleVector<char*> youForgivingPhrases;


static SimpleVector<char*> youGivingPhrases;
static SimpleVector<char*> namedGivingPhrases;

//2HOL additions for: password-protected objects
int passwordTransitionsAllowed = 0;
int passwordInvocationAndSettingAreSeparated = 0;
int passwordOverhearRadius = 6;
int passwordSilent = 0;

static SimpleVector<char*> passwordSettingPhrases;
static SimpleVector<char*> passwordInvokingPhrases;

static SimpleVector<char*> infertilityDeclaringPhrases;
static SimpleVector<char*> fertilityDeclaringPhrases;

static char *eveName = NULL;

static char *infertilitySuffix = NULL;
static char *fertilitySuffix = NULL;

// maps extended ascii codes to true/false for characters allowed in SAY
// messages
static char allowedSayCharMap[256];

static const char *allowedSayChars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ.-,'?! ";


static int killEmotionIndex = 2;
static int victimEmotionIndex = 2;

static int starvingEmotionIndex = 2;
static int satisfiedEmotionIndex = 2;

static int afkEmotionIndex = 2;
static double afkTimeSeconds = 0;

static int drunkEmotionIndex = 2;
static int trippingEmotionIndex = 2;


static double lastBabyPassedThresholdTime = 0;


static double eveWindowStart = 0;

OneLife::Server* oneLifeServer = nullptr;


typedef struct PeaceTreaty {
        int lineageAEveID;
        int lineageBEveID;
        
        // they have to say it in both directions
        // before it comes into effect
        char dirAToB;
        char dirBToA;

        // track directions of breaking it later
        char dirAToBBroken;
        char dirBToABroken;
    } PeaceTreaty;

    

static SimpleVector<PeaceTreaty> peaceTreaties;


// may be partial
static PeaceTreaty *getMatchingTreaty( int inLineageAEveID, 
                                       int inLineageBEveID ) {
    
    for( int i=0; i<peaceTreaties.size(); i++ ) {
        PeaceTreaty *p = peaceTreaties.getElement( i );
        

        if( ( p->lineageAEveID == inLineageAEveID &&
              p->lineageBEveID == inLineageBEveID )
            ||
            ( p->lineageAEveID == inLineageBEveID &&
              p->lineageBEveID == inLineageAEveID ) ) {
            // they match a treaty.
            return p;
            }
        }
    return NULL;
    }



// parial treaty returned if it's requested
static char isPeaceTreaty( int inLineageAEveID, int inLineageBEveID,
                           PeaceTreaty **outPartialTreaty = NULL ) {
    
    PeaceTreaty *p = getMatchingTreaty( inLineageAEveID, inLineageBEveID );
        
    if( p != NULL ) {
        
        if( !( p->dirAToB && p->dirBToA ) ) {
            // partial treaty
            if( outPartialTreaty != NULL ) {
                *outPartialTreaty = p;
                }
            return false;
            }
        return true;
        }
    return false;
    }


void sendPeaceWarMessage( const char *inPeaceOrWar,
                          char inWar,
                          int inLineageAEveID, int inLineageBEveID );


static void addPeaceTreaty( int inLineageAEveID, int inLineageBEveID ) {
    PeaceTreaty *p = getMatchingTreaty( inLineageAEveID, inLineageBEveID );
    
    if( p != NULL ) {
        char peaceBefore = p->dirAToB && p->dirBToA;
        
        // maybe it has been sealed in a new direction?
        if( p->lineageAEveID == inLineageAEveID ) {
            p->dirAToB = true;
            p->dirBToABroken = false;
            }
        if( p->lineageBEveID == inLineageAEveID ) {
            p->dirBToA = true;
            p->dirBToABroken = false;
            }
        if( p->dirAToB && p->dirBToA &&
            ! peaceBefore ) {
            // new peace!
            sendPeaceWarMessage( "PEACE", 
                                 false,
                                 p->lineageAEveID, p->lineageBEveID );
            }
        }
    else {
        // else doesn't exist, create new unidirectional
        PeaceTreaty p = { inLineageAEveID, inLineageBEveID,
                          true, false,
                          false, false };
        
        peaceTreaties.push_back( p );
        }
    }



static void removePeaceTreaty( int inLineageAEveID, int inLineageBEveID ) {
    PeaceTreaty *p = getMatchingTreaty( inLineageAEveID, inLineageBEveID );
    
    char remove = false;
    
    if( p != NULL ) {
        if( p->dirAToB && p->dirBToA ) {
            // established
            
            // maybe it has been broken in a new direction?
            if( p->lineageAEveID == inLineageAEveID ) {
                p->dirAToBBroken = true;
                }
            if( p->lineageBEveID == inLineageAEveID ) {
                p->dirBToABroken = true;
                }
            
            if( p->dirAToBBroken && p->dirBToABroken ) {
                // fully broken
                // remove it
                remove = true;

                // new war!
                sendPeaceWarMessage( "WAR",
                                     true,
                                     p->lineageAEveID, p->lineageBEveID );
                }
            }
        else {
            // not fully established
            // remove it 
            
            // this means if one person says PEACE and the other
            // responds with WAR, the first person's PEACE half-way treaty
            // is canceled.  Both need to say PEACE again once WAR has been
            // mentioned
            remove = true;
            }
        }
    
    if( remove ) {
        for( int i=0; i<peaceTreaties.size(); i++ ) {
            PeaceTreaty *otherP = peaceTreaties.getElement( i );
            
            if( otherP->lineageAEveID == p->lineageAEveID &&
                otherP->lineageBEveID == p->lineageBEveID ) {
                
                peaceTreaties.deleteElement( i );
                return;
                }
            }
        }
    }




// for incoming socket connections that are still in the login process
typedef struct FreshConnection {
        Socket *sock;
        SimpleVector<char> *sockBuffer;

        unsigned int sequenceNumber;
        char *sequenceNumberString;
        
        WebRequest *ticketServerRequest;
        char ticketServerAccepted;
        char lifeTokenSpent;

        float fitnessScore;

        double ticketServerRequestStartTime;
        
        char error;
        const char *errorCauseString;
        
        double rejectedSendTime;

        char shutdownMode;

        // for tracking connections that have failed to LOGIN 
        // in a timely manner
        double connectionStartTimeSeconds;

        char *email;
        uint32_t hashedSpawnSeed;
        
        int tutorialNumber;
        CurseStatus curseStatus;
        
        char *twinCode;
        int twinCount;

    } FreshConnection;


SimpleVector<FreshConnection> newConnections;
SimpleVector<FreshConnection> waitingForTwinConnections;
SimpleVector<LiveObject> tutorialLoadingPlayers;



char doesEveLineExist( int inEveID ) {
    for( int i=0; i<players.size(); i++ ) {
        LiveObject *o = players.getElement( i );
        
        if( ( ! o->error ) && o->lineageEveID == inEveID ) {
            return true;
            }
        }
    return false;
    }




typedef struct DeadObject {
        int id;
        
        int displayID;
        
        char *name;
        
        SimpleVector<int> *lineage;
        
        // id of Eve that started this line
        int lineageEveID;
        


        // time that this life started (for computing age)
        // not actual creation time (can be adjusted to tweak starting age,
        // for example, in case of Eve who starts older).
        double lifeStartTimeSeconds;
        
        // time this person died
        double deathTimeSeconds;
        

    } DeadObject;



static double lastPastPlayerFlushTime = 0;

SimpleVector<DeadObject> pastPlayers;



static void addPastPlayer( LiveObject *inPlayer ) {
    
    DeadObject o;
    
    o.id = inPlayer->id;
    o.displayID = inPlayer->displayID;
    o.name = NULL;
    if( inPlayer->name != NULL ) {
        o.name = stringDuplicate( inPlayer->name );
        }
    o.lineageEveID = inPlayer->lineageEveID;
    o.lifeStartTimeSeconds = inPlayer->lifeStartTimeSeconds;
    o.deathTimeSeconds = inPlayer->deathTimeSeconds;
    
    o.lineage = new SimpleVector<int>();
    for( int i=0; i< inPlayer->lineage->size(); i++ ) {
        o.lineage->push_back( inPlayer->lineage->getElementDirect( i ) );
        }
    
    pastPlayers.push_back( o );
    }



char isOwned( LiveObject *inPlayer, int inX, int inY ) {
    for( int i=0; i<inPlayer->ownedPositions.size(); i++ ) {
        GridPos *p = inPlayer->ownedPositions.getElement( i );
        
        if( p->x == inX && p->y == inY ) {
            return true;
            }
        }
    return false;
    }



char isOwned( LiveObject *inPlayer, GridPos inPos ) {
    return isOwned( inPlayer, inPos.x, inPos.y );
    }



char isKnownOwned( LiveObject *inPlayer, int inX, int inY ) {
    for( int i=0; i<inPlayer->knownOwnedPositions.size(); i++ ) {
        GridPos *p = inPlayer->knownOwnedPositions.getElement( i );
        
        if( p->x == inX && p->y == inY ) {
            return true;
            }
        }
    return false;
    }



char isKnownOwned( LiveObject *inPlayer, GridPos inPos ) {
    return isKnownOwned( inPlayer, inPos.x, inPos.y );
    }

void sendMessageToPlayer( LiveObject *inPlayer, 
                          char *inMessage, int inLength );

SimpleVector<GridPos> recentlyRemovedOwnerPos;


void removeAllOwnership( LiveObject *inPlayer ) {
    double startTime = Time::getCurrentTime();
    int num = inPlayer->ownedPositions.size();
    
    for( int i=0; i<inPlayer->ownedPositions.size(); i++ ) {
        GridPos *p = inPlayer->ownedPositions.getElement( i );

        recentlyRemovedOwnerPos.push_back( *p );
        
        int oID = getMapObject( p->x, p->y );

        if( oID <= 0 ) {
            continue;
            }

        char noOtherOwners = true;
        
        for( int j=0; j<players.size(); j++ ) {
            LiveObject *otherPlayer = players.getElement( j );
            
            if( otherPlayer != inPlayer ) {
                if( isOwned( otherPlayer, *p ) ) {
                    noOtherOwners = false;
                    break;
                    }
                }
            }
        
        if( noOtherOwners ) {
            // last owner of p just died
            // force end transition
            SimpleVector<int> *deathMarkers = getAllPossibleDeathIDs();
            for( int j=0; j<deathMarkers->size(); j++ ) {
                int deathID = deathMarkers->getElementDirect( j );
                TransRecord *t = getTrans( deathID, oID );
                
                if( t != NULL ) {
                    
                    setMapObject( p->x, p->y, t->newTarget );
                    break;
                    }
                }
            }
        }
    
    inPlayer->ownedPositions.deleteAll();

    AppLog::infoF( "Removing all ownership (%d owned) for "
                   "player %d (%s) took %lf sec",
                   num, inPlayer->id, inPlayer->email, 
                   Time::getCurrentTime() - startTime );
    }



char *getOwnershipString( int inX, int inY ) {    
    SimpleVector<char> messageWorking;
    
    for( int j=0; j<players.size(); j++ ) {
        LiveObject *otherPlayer = players.getElement( j );
        if( ! otherPlayer->error &&
            isOwned( otherPlayer, inX, inY ) ) {
            char *playerIDString = 
                autoSprintf( " %d", otherPlayer->id );
            messageWorking.appendElementString( 
                playerIDString );
            delete [] playerIDString;
            }
        }
    char *message = messageWorking.getElementString();
    return message;
    }


char *getOwnershipString( GridPos inPos ) {
    return getOwnershipString( inPos.x, inPos.y );
    }



static char checkReadOnly() {
    const char *testFileName = "testReadOnly.txt";
    
    FILE *testFile = fopen( testFileName, "w" );
    
    if( testFile != NULL ) {
        
        fclose( testFile );
        remove( testFileName );
        return false;
        }
    return true;
    }




// returns a person to their natural state
static void backToBasics( LiveObject *inPlayer ) {
    LiveObject *p = inPlayer;

    // do not heal dying people
    if( ! p->holdingWound && p->holdingID > 0 ) {
        
        p->holdingID = 0;
        
        p->holdingEtaDecay = 0;
        
        p->heldOriginValid = false;
        p->heldTransitionSourceID = -1;
        
        
        p->numContained = 0;
        if( p->containedIDs != NULL ) {
            delete [] p->containedIDs;
            delete [] p->containedEtaDecays;
            p->containedIDs = NULL;
        p->containedEtaDecays = NULL;
            }
        
        if( p->subContainedIDs != NULL ) {
            delete [] p->subContainedIDs;
            delete [] p->subContainedEtaDecays;
            p->subContainedIDs = NULL;
            p->subContainedEtaDecays = NULL;
            }
        }
        
        
    p->clothing = getEmptyClothingSet();
    
    for( int c=0; c<NUM_CLOTHING_PIECES; c++ ) {
        p->clothingEtaDecay[c] = 0;
        p->clothingContained[c].deleteAll();
        p->clothingContainedEtaDecays[c].deleteAll();
        }

    p->emotFrozen = false;
    p->emotUnfreezeETA = 0;
    }




typedef struct GraveInfo {
        GridPos pos;
        int playerID;
        // eve that started the line of this dead person
        // used for tracking whether grave is part of player's family or not
        int lineageEveID;
    } GraveInfo;


typedef struct GraveMoveInfo {
        GridPos posStart;
        GridPos posEnd;
        int swapDest;
    } GraveMoveInfo;




// tracking spots on map that inflicted a mortal wound
// put them on timeout afterward so that they don't attack
// again immediately
typedef struct DeadlyMapSpot {
        GridPos pos;
        double timeOfAttack;
    } DeadlyMapSpot;


static double deadlyMapSpotTimeoutSec = 10;

static SimpleVector<DeadlyMapSpot> deadlyMapSpots;


static char wasRecentlyDeadly( GridPos inPos ) {
    double curTime = Time::getCurrentTime();
    
    for( int i=0; i<deadlyMapSpots.size(); i++ ) {
        
        DeadlyMapSpot *s = deadlyMapSpots.getElement( i );
        
        if( curTime - s->timeOfAttack >= deadlyMapSpotTimeoutSec ) {
            deadlyMapSpots.deleteElement( i );
            i--;
            }
        else if( s->pos.x == inPos.x && s->pos.y == inPos.y ) {
            // note that this is a lazy method that only walks through
            // the whole list and checks for timeouts when
            // inPos isn't found
            return true;
            }
        }
    return false;
    }



static void addDeadlyMapSpot( GridPos inPos ) {
    // don't check for duplicates
    // we're only called to add a new deadly spot when the spot isn't
    // currently on deadly cooldown anyway
    DeadlyMapSpot s = { inPos, Time::getCurrentTime() };
    deadlyMapSpots.push_back( s );
    }




static LiveObject *getLiveObject( int inID ) {
    for( int i=0; i<players.size(); i++ ) {
        LiveObject *o = players.getElement( i );
        
        if( o->id == inID ) {
            return o;
            }
        }
    
    return NULL;
    }


char *getPlayerName( int inID ) {
    LiveObject *o = getLiveObject( inID );
    if( o != NULL ) {
        return o->name;
        }
    return NULL;
    }




static double pickBirthCooldownSeconds() {
    // Kumaraswamy distribution
    // PDF:
    // k(x,a,b) = a * b * x**( a - 1 ) * (1-x**a)**(b-1)
    // CDF:
    // kCDF(x,a,b) = 1 - (1-x**a)**b
    // Invers CDF:
    // kCDFInv(y,a,b) = ( 1 - (1-y)**(1.0/b) )**(1.0/a)

    // For b=1, PDF curve starts at 0 and curves upward, for all a > 2
    // good values seem to be a=1.5, b=1

    // actually, make it more bell-curve like, with a=2, b=3

    double a = 2;
    double b = 3;
    
    // mean is around 2 minutes
    

    // uniform
    double u = randSource.getRandomDouble();
    
    // feed into inverted CDF to sample a value from the distribution
    double v = pow( 1 - pow( 1-u, (1/b) ), 1/a );
    
    // v is in [0..1], the value range of Kumaraswamy

    // put max at 5 minutes
    return v * 5 * 60;
    }




typedef struct FullMapContained{ 
        int numContained;
        int *containedIDs;
        timeSec_t *containedEtaDecays;
        SimpleVector<int> *subContainedIDs;
        SimpleVector<timeSec_t> *subContainedEtaDecays;
    } FullMapContained;



// including contained and sub contained in one call
FullMapContained getFullMapContained( int inX, int inY ) {
    FullMapContained r;
    
    r.containedIDs = getContained( inX, inY, &( r.numContained ) );
    r.containedEtaDecays = 
        getContainedEtaDecay( inX, inY, &( r.numContained ) );
    
    if( r.numContained == 0 ) {
        r.subContainedIDs = NULL;
        r.subContainedEtaDecays = NULL;
        }
    else {
        r.subContainedIDs = new SimpleVector<int>[ r.numContained ];
        r.subContainedEtaDecays = new SimpleVector<timeSec_t>[ r.numContained ];
        }
    
    for( int c=0; c< r.numContained; c++ ) {
        if( r.containedIDs[c] < 0 ) {
            
            int numSub;
            int *subContainedIDs = getContained( inX, inY, &numSub,
                                                 c + 1 );
            
            if( subContainedIDs != NULL ) {
                
                r.subContainedIDs[c].appendArray( subContainedIDs, numSub );
                delete [] subContainedIDs;
                }
            
            timeSec_t *subContainedEtaDecays = 
                getContainedEtaDecay( inX, inY, &numSub,
                                      c + 1 );

            if( subContainedEtaDecays != NULL ) {
                
                r.subContainedEtaDecays[c].appendArray( subContainedEtaDecays, 
                                                        numSub );
                delete [] subContainedEtaDecays;
                }
            }
        }
    
    return r;
    }



void freePlayerContainedArrays( LiveObject *inPlayer ) {
    if( inPlayer->containedIDs != NULL ) {
        delete [] inPlayer->containedIDs;
        }
    if( inPlayer->containedEtaDecays != NULL ) {
        delete [] inPlayer->containedEtaDecays;
        }
    if( inPlayer->subContainedIDs != NULL ) {
        delete [] inPlayer->subContainedIDs;
        }
    if( inPlayer->subContainedEtaDecays != NULL ) {
        delete [] inPlayer->subContainedEtaDecays;
        }

    inPlayer->containedIDs = NULL;
    inPlayer->containedEtaDecays = NULL;
    inPlayer->subContainedIDs = NULL;
    inPlayer->subContainedEtaDecays = NULL;
    }



void setContained( LiveObject *inPlayer, FullMapContained inContained ) {
    
    inPlayer->numContained = inContained.numContained;
     
    freePlayerContainedArrays( inPlayer );
    
    inPlayer->containedIDs = inContained.containedIDs;
    
    inPlayer->containedEtaDecays =
        inContained.containedEtaDecays;
    
    inPlayer->subContainedIDs =
        inContained.subContainedIDs;
    inPlayer->subContainedEtaDecays =
        inContained.subContainedEtaDecays;
    }
    
    
    
    
void clearPlayerHeldContained( LiveObject *inPlayer ) {
    inPlayer->numContained = 0;
    
    delete [] inPlayer->containedIDs;
    delete [] inPlayer->containedEtaDecays;
    delete [] inPlayer->subContainedIDs;
    delete [] inPlayer->subContainedEtaDecays;
    
    inPlayer->containedIDs = NULL;
    inPlayer->containedEtaDecays = NULL;
    inPlayer->subContainedIDs = NULL;
    inPlayer->subContainedEtaDecays = NULL;
    }
    



void transferHeldContainedToMap( LiveObject *inPlayer, int inX, int inY ) {
    if( inPlayer->numContained != 0 ) {
        timeSec_t curTime = Time::timeSec();
        float stretch = 
            getObject( inPlayer->holdingID )->slotTimeStretch;
        
        for( int c=0;
             c < inPlayer->numContained;
             c++ ) {
            
            // undo decay stretch before adding
            // (stretch applied by adding)
            if( stretch != 1.0 &&
                inPlayer->containedEtaDecays[c] != 0 ) {
                
                timeSec_t offset = 
                    inPlayer->containedEtaDecays[c] - curTime;
                
                offset = offset * stretch;
                
                inPlayer->containedEtaDecays[c] =
                    curTime + offset;
                }

            addContained( 
                inX, inY,
                inPlayer->containedIDs[c],
                inPlayer->containedEtaDecays[c] );

            int numSub = inPlayer->subContainedIDs[c].size();
            if( numSub > 0 ) {

                int container = inPlayer->containedIDs[c];
                
                if( container < 0 ) {
                    container *= -1;
                    }
                
                float subStretch = getObject( container )->slotTimeStretch;
                    
                
                int *subIDs = 
                    inPlayer->subContainedIDs[c].getElementArray();
                timeSec_t *subDecays = 
                    inPlayer->subContainedEtaDecays[c].
                    getElementArray();
                
                for( int s=0; s < numSub; s++ ) {
                    
                    // undo decay stretch before adding
                    // (stretch applied by adding)
                    if( subStretch != 1.0 &&
                        subDecays[s] != 0 ) {
                
                        timeSec_t offset = subDecays[s] - curTime;
                        
                        offset = offset * subStretch;
                        
                        subDecays[s] = curTime + offset;
                        }

                    addContained( inX, inY,
                                  subIDs[s], subDecays[s],
                                  c + 1 );
                    }
                delete [] subIDs;
                delete [] subDecays;
                }
            }

        clearPlayerHeldContained( inPlayer );
        }
    }




// diagonal steps are longer
static double measurePathLength( int inXS, int inYS, 
                                 GridPos *inPathPos, int inPathLength ) {
    
    // diags are square root of 2 in length
    double diagLength = 1.41421356237;
    

    double totalLength = 0;
    
    GridPos lastPos = { inXS, inYS };
    
    for( int i=0; i<inPathLength; i++ ) {
        
        GridPos thisPos = inPathPos[i];
        
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




static double getPathSpeedModifier( GridPos *inPathPos, int inPathLength ) {
    
    if( inPathLength < 1 ) {
        return 1;
        }
    

    int floor = getMapFloor( inPathPos[0].x, inPathPos[0].y );

    if( floor == 0 ) {
        return 1;
        }

    double speedMult = getObject( floor )->speedMult;
    
    if( speedMult == 1 ) {
        return 1;
        }
    

    // else we have a speed mult for at least first step in path
    // see if we have same floor for rest of path

    for( int i=1; i<inPathLength; i++ ) {
        
        int thisFloor = getMapFloor( inPathPos[i].x, inPathPos[i].y );
        
        if( thisFloor != floor ) {
            // not same floor whole way
            return 1;
            }
        }
    // same floor whole way
    printf( "Speed modifier = %f\n", speedMult );
    return speedMult;
    }



static int getLiveObjectIndex( int inID ) {
    for( int i=0; i<players.size(); i++ ) {
        LiveObject *o = players.getElement( i );
        
        if( o->id == inID ) {
            return i;
            }
        }

    return -1;
    }





int nextID = 2;


static void deleteMembers( FreshConnection *inConnection ) {
    delete inConnection->sock;
    delete inConnection->sockBuffer;
    
    if( inConnection->sequenceNumberString != NULL ) {    
        delete [] inConnection->sequenceNumberString;
        }
    
    if( inConnection->ticketServerRequest != NULL ) {
        delete inConnection->ticketServerRequest;
        }
    
    if( inConnection->email != NULL ) {
        delete [] inConnection->email;
        }
    
    if( inConnection->twinCode != NULL ) {
        delete [] inConnection->twinCode;
        }
    }



static SimpleVector<char *> curseWords;

static char *curseSecret = NULL;




void quitCleanup() {
    AppLog::info( "Cleaning up on quit..." );

    // FreshConnections are in two different lists
    // free structures from both
    SimpleVector<FreshConnection> *connectionLists[2] =
        { &newConnections, &waitingForTwinConnections };

    for( int c=0; c<2; c++ ) {
        SimpleVector<FreshConnection> *list = connectionLists[c];
        
        for( int i=0; i<list->size(); i++ ) {
            FreshConnection *nextConnection = list->getElement( i );
            deleteMembers( nextConnection );
            }
        list->deleteAll();
        }
    
    // add these to players to clean them up togeter
    for( int i=0; i<tutorialLoadingPlayers.size(); i++ ) {
        LiveObject nextPlayer = tutorialLoadingPlayers.getElementDirect( i );
        players.push_back( nextPlayer );
        }
    tutorialLoadingPlayers.deleteAll();
    


    for( int i=0; i<players.size(); i++ ) {
        LiveObject *nextPlayer = players.getElement(i);
        
        removeAllOwnership( nextPlayer );

        if( nextPlayer->sock != NULL ) {
            delete nextPlayer->sock;
            nextPlayer->sock = NULL;
            }
        if( nextPlayer->sockBuffer != NULL ) {
            delete nextPlayer->sockBuffer;
            nextPlayer->sockBuffer = NULL;
            }

        delete nextPlayer->lineage;

        delete nextPlayer->ancestorIDs;

        nextPlayer->ancestorEmails->deallocateStringElements();
        delete nextPlayer->ancestorEmails;
        
        nextPlayer->ancestorRelNames->deallocateStringElements();
        delete nextPlayer->ancestorRelNames;
        
        delete nextPlayer->ancestorLifeStartTimeSeconds;
		delete nextPlayer->ancestorLifeEndTimeSeconds;
        

        if( nextPlayer->name != NULL ) {
            delete [] nextPlayer->name;
            }

        if( nextPlayer->displayedName != NULL ) {
            delete [] nextPlayer->displayedName;
            }

        if( nextPlayer->familyName != NULL ) {
            delete [] nextPlayer->familyName;
            }

        if( nextPlayer->lastSay != NULL ) {
            delete [] nextPlayer->lastSay;
            }
        
        if( nextPlayer->email != NULL  ) {
            delete [] nextPlayer->email;
            }
        if( nextPlayer->origEmail != NULL  ) {
            delete [] nextPlayer->origEmail;
            }
        if( nextPlayer->lastBabyEmail != NULL  ) {
            delete [] nextPlayer->lastBabyEmail;
            }
        if( nextPlayer->lastSidsBabyEmail != NULL ) {
            delete [] nextPlayer->lastSidsBabyEmail;
            }

        if( nextPlayer->murderPerpEmail != NULL  ) {
            delete [] nextPlayer->murderPerpEmail;
            }


        freePlayerContainedArrays( nextPlayer );
        
        
        if( nextPlayer->pathToDest != NULL ) {
            delete [] nextPlayer->pathToDest;
            }
        
        if( nextPlayer->deathReason != NULL ) {
            delete [] nextPlayer->deathReason;
            }


        delete nextPlayer->babyBirthTimes;
        delete nextPlayer->babyIDs;        
        }
    players.deleteAll();


    for( int i=0; i<pastPlayers.size(); i++ ) {
        DeadObject *o = pastPlayers.getElement( i );
        
        delete [] o->name;
        delete o->lineage;
        }
    pastPlayers.deleteAll();
    

    freeLineageLimit();
    
    freePlayerStats();
    freeLineageLog();
    
    freeNames();
    
    freeCurses();
    
    freeCurseDB();
    
    freeLifeTokens();

    freeFitnessScore();

    freeLifeLog();
    
    freeFoodLog();
    freeFailureLog();
    
    freeObjectSurvey();
    
    freeLanguage();
    freeFamilySkipList();

    freeTriggers();

    freeMap();

    freeTransBank();
    freeCategoryBank();
    freeObjectBank();
    freeAnimationBank();
    
    freeArcReport();
    

    if( clientPassword != NULL ) {
        delete [] clientPassword;
        clientPassword = NULL;
        }
    

    if( ticketServerURL != NULL ) {
        delete [] ticketServerURL;
        ticketServerURL = NULL;
        }

    if( reflectorURL != NULL ) {
        delete [] reflectorURL;
        reflectorURL = NULL;
        }

    nameGivingPhrases.deallocateStringElements();
    familyNameGivingPhrases.deallocateStringElements();
    cursingPhrases.deallocateStringElements();
    
    forgivingPhrases.deallocateStringElements();
    youForgivingPhrases.deallocateStringElements();
    
    youGivingPhrases.deallocateStringElements();
    namedGivingPhrases.deallocateStringElements();
	infertilityDeclaringPhrases.deallocateStringElements();
	fertilityDeclaringPhrases.deallocateStringElements();
    
    //2HOL, password-protected objects: maintenance
    passwordSettingPhrases.deallocateStringElements();
    passwordInvokingPhrases.deallocateStringElements();
	
    if( curseYouPhrase != NULL ) {
        delete [] curseYouPhrase;
        curseYouPhrase = NULL;
        }
    if( curseBabyPhrase != NULL ) {
        delete [] curseBabyPhrase;
        curseBabyPhrase = NULL;
        }
    

    if( eveName != NULL ) {
        delete [] eveName;
        eveName = NULL;
        }
    if( infertilitySuffix != NULL ) {
        delete [] infertilitySuffix;
        infertilitySuffix = NULL;
        }
    if( fertilitySuffix != NULL ) {
        delete [] fertilitySuffix;
        fertilitySuffix = NULL;
        }

    if( apocalypseRequest != NULL ) {
        delete apocalypseRequest;
        apocalypseRequest = NULL;
        }

    if( familyDataLogFile != NULL ) {
        fclose( familyDataLogFile );
        familyDataLogFile = NULL;
        }

    curseWords.deallocateStringElements();
    
    if( curseSecret != NULL ) {
        delete [] curseSecret;
        curseSecret = NULL;
        }
    }






#include "minorGems/util/crc32.h"

JenkinsRandomSource curseSource;


static int cursesUseSenderEmail = 0;

static int useCurseWords = 1;


// result NOT destroyed by caller
static const char *getCurseWord( char *inSenderEmail,
                                 char *inEmail, int inWordIndex ) {
    if( ! useCurseWords || curseWords.size() == 0 ) {
        return "X";
        }

    if( curseSecret == NULL ) {
        curseSecret = 
            SettingsManager::getStringSetting( 
                "statsServerSharedSecret", "sdfmlk3490sadfm3ug9324" );
        }
    
    char *emailPlusSecret;

    if( cursesUseSenderEmail ) {
        emailPlusSecret =
            autoSprintf( "%s_%s_%s", inSenderEmail, inEmail, curseSecret );
        }
    else {
        emailPlusSecret = 
            autoSprintf( "%s_%s", inEmail, curseSecret );
        }
    
    unsigned int c = crc32( (unsigned char*)emailPlusSecret, 
                            strlen( emailPlusSecret ) );
    
    delete [] emailPlusSecret;

    curseSource.reseed( c );
    
    // mix based on index
    for( int i=0; i<inWordIndex; i++ ) {
        curseSource.getRandomDouble();
        }

    int index = curseSource.getRandomBoundedInt( 0, curseWords.size() - 1 );
    
    return curseWords.getElementDirect( index );
    }




volatile char quit = false;

void intHandler( int inUnused ) {
    AppLog::info( "Quit received for unix" );
    
    // since we handled this singal, we will return to normal execution
    quit = true;
    }


#ifdef WIN_32
#include <windows.h>
BOOL WINAPI ctrlHandler( DWORD dwCtrlType ) {
    if( CTRL_C_EVENT == dwCtrlType ) {
        AppLog::info( "Quit received for windows" );
        
        // will auto-quit as soon as we return from this handler
        // so cleanup now
        //quitCleanup();
        
        // seems to handle CTRL-C properly if launched by double-click
        // or batch file
        // (just not if launched directly from command line)
        quit = true;
        }
    return true;
    }
#endif


int numConnections = 0;







// reads all waiting data from socket and stores it in buffer
// returns true if socket still good, false on error
char readSocketFull( Socket *inSock, SimpleVector<char> *inBuffer ) {

    char buffer[512];
    
    int numRead = inSock->receive( (unsigned char*)buffer, 512, 0 );
    
    if( numRead == -1 ) {

        if( ! inSock->isSocketInFDRange() ) {
            // the internal FD of this socket is out of range
            // probably some kind of heap corruption.

            // save a bug report
            int allow = 
                SettingsManager::getIntSetting( "allowBugReports", 0 );

            if( allow ) {
                char *bugName = 
                    autoSprintf( "bug_socket_%f", Time::getCurrentTime() );
                
                char *bugOutName = autoSprintf( "%s_out.txt", bugName );
                
                File outFile( NULL, "serverOut.txt" );
                if( outFile.exists() ) {
                    fflush( stdout );
                    File outCopyFile( NULL, bugOutName );
                    
                    outFile.copy( &outCopyFile );
                    }
                delete [] bugName;
                delete [] bugOutName;
                }
            }
        
            
        return false;
        }
    
    while( numRead > 0 ) {
        inBuffer->appendArray( buffer, numRead );

        numRead = inSock->receive( (unsigned char*)buffer, 512, 0 );
        }

    return true;
    }



// NULL if there's no full message available
char *getNextClientMessage( SimpleVector<char> *inBuffer ) {
    // find first terminal character #

    int index = inBuffer->getElementIndex( '#' );
        
    if( index == -1 ) {

        if( inBuffer->size() > 200 ) {
            // 200 characters with no message terminator?
            // client is sending us nonsense
            // cut it off here to avoid buffer overflow
            
            AppLog::info( "More than 200 characters in client receive buffer "
                          "with no messsage terminator present, "
                          "generating NONSENSE message." );
            
            return stringDuplicate( "NONSENSE 0 0" );
            }

        return NULL;
        }
    
    if( index > 1 && 
        inBuffer->getElementDirect( 0 ) == 'K' &&
        inBuffer->getElementDirect( 1 ) == 'A' ) {
        
        // a KA (keep alive) message
        // short-cicuit the processing here
        
        inBuffer->deleteStartElements( index + 1 );
        return NULL;
        }
    
        

    char *message = new char[ index + 1 ];
    
    // all but terminal character
    for( int i=0; i<index; i++ ) {
        message[i] = inBuffer->getElementDirect( i );
        }
    
    // delete from buffer, including terminal character
    inBuffer->deleteStartElements( index + 1 );
    
    message[ index ] = '\0';
    
    return message;
    }





typedef enum messageType {
	MOVE,
    USE,
    SELF,
    BABY,
    UBABY,
    REMV,
    SREMV,
    DROP,
    KILL,
    SAY,
    EMOT,
    JUMP,
    DIE,
    GRAVE,
    OWNER,
    FORCE,
    MAP,
    TRIGGER,
    BUG,
    PING,
    VOGS,
    VOGN,
    VOGP,
    VOGM,
    VOGI,
    VOGT,
    VOGX,
    PHOTO,
    FLIP,
    UNKNOWN
    } messageType;




typedef struct ClientMessage {
        messageType type;
        int x, y, c, i, id;
        
        int trigger;
        int bug;

        // some messages have extra positions attached
        int numExtraPos;

        // NULL if there are no extra
        GridPos *extraPos;

        // null if type not SAY
        char *saidText;
        
        // null if type not BUG
        char *bugText;

        // for MOVE messages
        int sequenceNumber;

    } ClientMessage;


static int pathDeltaMax = 16;


// if extraPos present in result, destroyed by caller
// inMessage may be modified by this call
ClientMessage parseMessage( LiveObject *inPlayer, char *inMessage ) {
    
    char nameBuffer[100];
    
    ClientMessage m;
    
    m.i = -1;
    m.c = -1;
    m.id = -1;
    m.trigger = -1;
    m.numExtraPos = 0;
    m.extraPos = NULL;
    m.saidText = NULL;
    m.bugText = NULL;
    m.sequenceNumber = -1;
    
    // don't require # terminator here
    
    
    //int numRead = sscanf( inMessage, 
    //                      "%99s %d %d", nameBuffer, &( m.x ), &( m.y ) );
    

    // profiler finds sscanf as a hotspot
    // try a custom bit of code instead
    
    int numRead = 0;
    
    int parseLen = strlen( inMessage );
    if( parseLen > 99 ) {
        parseLen = 99;
        }
    
    for( int i=0; i<parseLen; i++ ) {
        if( inMessage[i] == ' ' ) {
            switch( numRead ) {
                case 0:
                    if( i != 0 ) {
                        memcpy( nameBuffer, inMessage, i );
                        nameBuffer[i] = '\0';
                        numRead++;
                        // rewind back to read the space again
                        // before the first number
                        i--;
                        }
                    break;
                case 1:
                    m.x = atoi( &( inMessage[i] ) );
                    numRead++;
                    break;
                case 2:
                    m.y = atoi( &( inMessage[i] ) );
                    numRead++;
                    break;
                }
            if( numRead == 3 ) {
                break;
                }
            }
        }
    

    
    if( numRead >= 2 &&
        strcmp( nameBuffer, "BUG" ) == 0 ) {
        m.type = BUG;
        m.bug = m.x;
        m.bugText = stringDuplicate( inMessage );
        return m;
        }


    if( numRead != 3 ) {
        
        if( numRead == 2 &&
            strcmp( nameBuffer, "TRIGGER" ) == 0 ) {
            m.type = TRIGGER;
            m.trigger = m.x;
            }
        else {
            m.type = UNKNOWN;
            }
        
        return m;
        }
    

    if( strcmp( nameBuffer, "MOVE" ) == 0) {
        m.type = MOVE;
        
        char *atPos = strstr( inMessage, "@" );
        
        int offset = 3;
        
        if( atPos != NULL ) {
            offset = 4;            
            }
        

        // in place, so we don't need to deallocate them
        SimpleVector<char *> *tokens =
            tokenizeStringInPlace( inMessage );
        
        // require an even number of extra coords beyond offset
        if( tokens->size() < offset + 2 || 
            ( tokens->size() - offset ) %2 != 0 ) {
            
            delete tokens;
            
            m.type = UNKNOWN;
            return m;
            }
        
        if( atPos != NULL ) {
            // skip @ symbol in token and parse int
            m.sequenceNumber = atoi( &( tokens->getElementDirect( 3 )[1] ) );
            }

        int numTokens = tokens->size();
        
        m.numExtraPos = (numTokens - offset) / 2;
        
        m.extraPos = new GridPos[ m.numExtraPos ];

        for( int e=0; e<m.numExtraPos; e++ ) {
            
            char *xToken = tokens->getElementDirect( offset + e * 2 );
            char *yToken = tokens->getElementDirect( offset + e * 2 + 1 );
            
            // profiler found sscanf is a bottleneck here
            // try atoi instead
            //sscanf( xToken, "%d", &( m.extraPos[e].x ) );
            //sscanf( yToken, "%d", &( m.extraPos[e].y ) );

            m.extraPos[e].x = atoi( xToken );
            m.extraPos[e].y = atoi( yToken );
            
            
            if( abs( m.extraPos[e].x ) > pathDeltaMax ||
                abs( m.extraPos[e].y ) > pathDeltaMax ) {
                // path goes too far afield
                
                // terminate it here
                m.numExtraPos = e;
                
                if( e == 0 ) {
                    delete [] m.extraPos;
                    m.extraPos = NULL;
                    m.numExtraPos = 0;
                    m.type = UNKNOWN;
                    delete tokens;
                    return m;
                    }
                break;
                }
                

            // make them absolute
            m.extraPos[e].x += m.x;
            m.extraPos[e].y += m.y;
            }
        
        delete tokens;
        }
    else if( strcmp( nameBuffer, "JUMP" ) == 0 ) {
        m.type = JUMP;
        }
    else if( strcmp( nameBuffer, "DIE" ) == 0 ) {
        m.type = DIE;
        }
    else if( strcmp( nameBuffer, "GRAVE" ) == 0 ) {
        m.type = GRAVE;
        }
    else if( strcmp( nameBuffer, "OWNER" ) == 0 ) {
        m.type = OWNER;
        }
    else if( strcmp( nameBuffer, "FORCE" ) == 0 ) {
        m.type = FORCE;
        }
    else if( strcmp( nameBuffer, "USE" ) == 0 ) {
        m.type = USE;
        // read optional id parameter
        numRead = sscanf( inMessage, 
                          "%99s %d %d %d %d", 
                          nameBuffer, &( m.x ), &( m.y ), &( m.id ), &( m.i ) );
        
        if( numRead < 5 ) {
            m.i = -1;
            }
        if( numRead < 4 ) {
            m.id = -1;
            }
        }
    else if( strcmp( nameBuffer, "SELF" ) == 0 ) {
        m.type = SELF;

        numRead = sscanf( inMessage, 
                          "%99s %d %d %d", 
                          nameBuffer, &( m.x ), &( m.y ), &( m.i ) );
        
        if( numRead != 4 ) {
            m.type = UNKNOWN;
            }
        }
    else if( strcmp( nameBuffer, "UBABY" ) == 0 ) {
        m.type = UBABY;

        // id param optional
        numRead = sscanf( inMessage, 
                          "%99s %d %d %d %d", 
                          nameBuffer, &( m.x ), &( m.y ), &( m.i ), &( m.id ) );
        
        if( numRead != 4 && numRead != 5 ) {
            m.type = UNKNOWN;
            }
        if( numRead != 5 ) {
            m.id = -1;
            }
        }
    else if( strcmp( nameBuffer, "BABY" ) == 0 ) {
        m.type = BABY;
        // read optional id parameter
        numRead = sscanf( inMessage, 
                          "%99s %d %d %d", 
                          nameBuffer, &( m.x ), &( m.y ), &( m.id ) );
        
        if( numRead != 4 ) {
            m.id = -1;
            }
        }
    else if( strcmp( nameBuffer, "PING" ) == 0 ) {
        m.type = PING;
        // read unique id parameter
        numRead = sscanf( inMessage, 
                          "%99s %d %d %d", 
                          nameBuffer, &( m.x ), &( m.y ), &( m.id ) );
        
        if( numRead != 4 ) {
            m.id = 0;
            }
        }
    else if( strcmp( nameBuffer, "SREMV" ) == 0 ) {
        m.type = SREMV;
        
        numRead = sscanf( inMessage, 
                          "%99s %d %d %d %d", 
                          nameBuffer, &( m.x ), &( m.y ), &( m.c ),
                          &( m.i ) );
        
        if( numRead != 5 ) {
            m.type = UNKNOWN;
            }
        }
    else if( strcmp( nameBuffer, "REMV" ) == 0 ) {
        m.type = REMV;
        
        numRead = sscanf( inMessage, 
                          "%99s %d %d %d", 
                          nameBuffer, &( m.x ), &( m.y ), &( m.i ) );
        
        if( numRead != 4 ) {
            m.type = UNKNOWN;
            }
        }
    else if( strcmp( nameBuffer, "DROP" ) == 0 ) {
        m.type = DROP;
        numRead = sscanf( inMessage, 
                          "%99s %d %d %d", 
                          nameBuffer, &( m.x ), &( m.y ), &( m.c ) );
        
        if( numRead != 4 ) {
            m.type = UNKNOWN;
            }
        }
    else if( strcmp( nameBuffer, "KILL" ) == 0 ) {
        m.type = KILL;
        
        // read optional id parameter
        numRead = sscanf( inMessage, 
                          "%99s %d %d %d", 
                          nameBuffer, &( m.x ), &( m.y ), &( m.id ) );
        
        if( numRead != 4 ) {
            m.id = -1;
            }
        }
    else if( strcmp( nameBuffer, "MAP" ) == 0 ) {
        m.type = MAP;
        }
    else if( strcmp( nameBuffer, "SAY" ) == 0 ) {
        m.type = SAY;

        // look after second space
        char *firstSpace = strstr( inMessage, " " );
        
        if( firstSpace != NULL ) {
            
            char *secondSpace = strstr( &( firstSpace[1] ), " " );
            
            if( secondSpace != NULL ) {

                char *thirdSpace = strstr( &( secondSpace[1] ), " " );
                
                if( thirdSpace != NULL ) {
                    m.saidText = stringDuplicate( &( thirdSpace[1] ) );
                    }
                }
            }
        }
    else if( strcmp( nameBuffer, "EMOT" ) == 0 ) {
        m.type = EMOT;

        numRead = sscanf( inMessage, 
                          "%99s %d %d %d", 
                          nameBuffer, &( m.x ), &( m.y ), &( m.i ) );
        
        if( numRead != 4 ) {
            m.type = UNKNOWN;
            }
        }
    else if( strcmp( nameBuffer, "VOGS" ) == 0 ) {
        m.type = VOGS;
        }
    else if( strcmp( nameBuffer, "VOGN" ) == 0 ) {
        m.type = VOGN;
        }
    else if( strcmp( nameBuffer, "VOGP" ) == 0 ) {
        m.type = VOGP;
        }
    else if( strcmp( nameBuffer, "VOGM" ) == 0 ) {
        m.type = VOGM;
        }
    else if( strcmp( nameBuffer, "VOGI" ) == 0 ) {
        m.type = VOGI;
        numRead = sscanf( inMessage, 
                          "%99s %d %d %d", 
                          nameBuffer, &( m.x ), &( m.y ), &( m.id ) );
        
        if( numRead != 4 ) {
            m.id = -1;
            }
        }
    else if( strcmp( nameBuffer, "VOGT" ) == 0 ) {
        m.type = VOGT;

        // look after second space
        char *firstSpace = strstr( inMessage, " " );
        
        if( firstSpace != NULL ) {
            
            char *secondSpace = strstr( &( firstSpace[1] ), " " );
            
            if( secondSpace != NULL ) {

                char *thirdSpace = strstr( &( secondSpace[1] ), " " );
                
                if( thirdSpace != NULL ) {
                    m.saidText = stringDuplicate( &( thirdSpace[1] ) );
                    }
                }
            }
        }
    else if( strcmp( nameBuffer, "VOGX" ) == 0 ) {
        m.type = VOGX;
        }
   else if( strcmp( nameBuffer, "PHOTO" ) == 0 ) {
        m.type = PHOTO;
        numRead = sscanf( inMessage, 
                          "%99s %d %d %d", 
                          nameBuffer, &( m.x ), &( m.y ), &( m.id ) );
        
        if( numRead != 4 ) {
            m.id = 0;
            }
        }
    else if( strcmp( nameBuffer, "FLIP" ) == 0 ) {
        m.type = FLIP;
        }
     else {
        m.type = UNKNOWN;
        }
    
    // incoming client messages are relative to birth pos
    // except NOT map pull messages, which are absolute
    if( m.type != MAP ) {    
        m.x += inPlayer->birthPos.x;
        m.y += inPlayer->birthPos.y;

        for( int i=0; i<m.numExtraPos; i++ ) {
            m.extraPos[i].x += inPlayer->birthPos.x;
            m.extraPos[i].y += inPlayer->birthPos.y;
            }
        }

    return m;
    }

GridPos getPlayerPos( LiveObject *inPlayer ) {
    if( inPlayer->xs == inPlayer->xd &&
        inPlayer->ys == inPlayer->yd ) {
        
        GridPos cPos = { inPlayer->xs, inPlayer->ys };
        
        return cPos;
        }
    else {
        return computePartialMoveSpot( inPlayer );
        }
    }



GridPos killPlayer( const char *inEmail ) {
    for( int i=0; i<players.size(); i++ ) {
        LiveObject *o = players.getElement( i );
        
        if( strcmp( o->email, inEmail ) == 0 ) {
            o->error = true;
            
            return computePartialMoveSpot( o );
            }
        }
    
    GridPos noPos = { 0, 0 };
    return noPos;
    }



void forcePlayerAge( const char *inEmail, double inAge ) {
    for( int i=0; i<players.size(); i++ ) {
        LiveObject *o = players.getElement( i );
        
        if( strcmp( o->email, inEmail ) == 0 ) {
            double ageSec = inAge / getAgeRate();
            
            o->lifeStartTimeSeconds = Time::getCurrentTime() - ageSec;
            o->needsUpdate = true;
            }
        }
    }





double computeFoodDecrementTimeSeconds( LiveObject *inPlayer ) {
	
	float baseHeat = inPlayer->heat;
	
	if( inPlayer->tripping ) {
		
		// Increased food drain when tripping
		if( inPlayer->heat >= 0.5 ) {
			baseHeat =  1.0 - (2/3) * ( 1.0 - baseHeat );
			} 
		else {
			baseHeat = (2/3) * baseHeat;
			}
		
		}
	
	
    double value = maxFoodDecrementSeconds * 2 * baseHeat;
    
    if( value > maxFoodDecrementSeconds ) {
        // also reduce if too hot (above 0.5 heat)
        
        double extra = value - maxFoodDecrementSeconds;

        value = maxFoodDecrementSeconds - extra;
        }
    
    // all player temp effects push us up above min
    value += minFoodDecrementSeconds;

    return value;
    }


double getAgeRate() {
    return 1.0 / secondsPerYear;
    }


static void setDeathReason( LiveObject *inPlayer, const char *inTag,
                            int inOptionalID = 0 ) {
    
    if( inPlayer->deathReason != NULL ) {
        delete [] inPlayer->deathReason;
        }
    
    // leave space in front so it works at end of PU line
    if( strcmp( inTag, "killed" ) == 0 ||
        strcmp( inTag, "succumbed" ) == 0 ) {
        
        inPlayer->deathReason = autoSprintf( " reason_%s_%d", 
                                             inTag, inOptionalID );
        }
    else {
        // ignore ID
        inPlayer->deathReason = autoSprintf( " reason_%s", inTag );
        }
    }



int longestShutdownLine = -1;

void handleShutdownDeath( LiveObject *inPlayer,
                          int inX, int inY ) {
    if( inPlayer->curseStatus.curseLevel == 0 &&
        inPlayer->parentChainLength > longestShutdownLine ) {
        
        // never count a cursed player as a long line
        
        longestShutdownLine = inPlayer->parentChainLength;
        
        FILE *f = fopen( "shutdownLongLineagePos.txt", "w" );
        if( f != NULL ) {
            fprintf( f, "%d,%d", inX, inY );
            fclose( f );
            }
        }
    }



double computeAge( double inLifeStartTimeSeconds ) {
    
    double deltaSeconds = 
        Time::getCurrentTime() - inLifeStartTimeSeconds;
    
    double age = deltaSeconds * getAgeRate();
    
    return age;
    }



double computeAge( LiveObject *inPlayer ) {
    double age = computeAge( inPlayer->lifeStartTimeSeconds );
    if( age >= forceDeathAge ) {
        setDeathReason( inPlayer, "age" );
        
        inPlayer->error = true;
        
        age = forceDeathAge;
        }
    return age;
    }



int getSayLimit( LiveObject *inPlayer ) {
    return getSayLimit( computeAge( inPlayer ) );
    }




int getSecondsPlayed( LiveObject *inPlayer ) {
    double deltaSeconds = 
        Time::getCurrentTime() - inPlayer->trueStartTimeSeconds;

    return lrint( deltaSeconds );
    }


// false for male, true for female
char getFemale( LiveObject *inPlayer ) {
    ObjectRecord *r = getObject( inPlayer->displayID );
    
    return ! r->male;
    }



char isFertileAge( LiveObject *inPlayer ) {
    double age = computeAge( inPlayer );
                    
    char f = getFemale( inPlayer );
                    
    if( age >= fertileAge && age <= oldAge && f ) {
        return true;
        }
    else {
        return false;
        }
    }



static int countYoungFemalesInLineage( int inLineageEveID ) {
    int count = 0;
    
    for( int i=0; i<players.size(); i++ ) {
        LiveObject *o = players.getElement( i );
		
		if( o->error ) {
			continue;
			}
        if( o->isTutorial ) {
            continue;
            }    
        if( o->vogMode ) {
            continue;
            }
        if( o->curseStatus.curseLevel > 0 ) {
            continue;
            }
        
        // while this doesn't match up with what this function is called, the way it's used
        // is for counting how many potentially fertile females a lineage has currently
        // for the force baby girl feature
        if( o->declaredInfertile ) {
            continue;
            }
			
		if( o->lineageEveID == inLineageEveID ) {
			double age = computeAge( o );
			char f = getFemale( o );
			if( age <= oldAge && f ) {
				count ++;
				}
            }
			
        }
    return count;
    }




int computeFoodCapacity( LiveObject *inPlayer ) {
    int ageInYears = lrint( computeAge( inPlayer ) );
    
    int returnVal = 0;
    
    if( ageInYears < oldAge ) {
        
        if( ageInYears > adultAge - 4 ) {
            ageInYears = adultAge - 4;
            }
        
        returnVal = ageInYears + 4;
        }
    else {
        // food capacity decreases as we near death
        int cap = forceDeathAge - ageInYears + 4;
        
        if( cap < 4 ) {
            cap = 4;
            }
        
        int lostBars = 20 - cap;

        if( lostBars > 0 && inPlayer->fitnessScore > 0 ) {
        
            // consider effect of fitness on reducing lost bars

            // for now, let's make it quadratic
            double maxLostBars = 
                16 - 16 * pow( inPlayer->fitnessScore / 60.0, 2 );
            
            if( lostBars > maxLostBars ) {
                lostBars = maxLostBars;
                }
            }
        
        returnVal = 20 - lostBars;
        }

    return ceil( returnVal * inPlayer->foodCapModifier );
    }



int computeOverflowFoodCapacity( int inBaseCapacity ) {
    // even littlest baby has +2 overflow, to get everyone used to the
    // concept.
    // by adulthood (when base cap is 20), overflow cap is 91.6
    return 2 + pow( inBaseCapacity, 8 ) * 0.0000000035;
    }



char *slurSpeech( int inSpeakerID,
                  char *inTranslatedPhrase, double inDrunkenness ) {
    char *working = stringDuplicate( inTranslatedPhrase );
    
    char *starPos = strstr( working, " *" );

    char *extraData = NULL;
    
    if( starPos != NULL ) {
        extraData = stringDuplicate( starPos );
        starPos[0] = '\0';
        }
    
    SimpleVector<char> slurredChars;
    
    // 1 in 10 letters slurred with 1 drunkenness
    // all characters slurred with 10 drunkenness
    double baseSlurChance = 0.1;
    
    double slurChance = baseSlurChance * inDrunkenness;

    // 2 in 10 words mixed up in order with 6 drunkenness
    // all words mixed up at 10 drunkenness
    double baseWordSwapChance = 0.1;

    // but don't start mixing up words at all until 6 drunkenness
    // thus, the 0 to 100% mix up range is from 6 to 10 drunkenness
    double wordSwapChance = 2 * baseWordSwapChance * ( inDrunkenness - 5 );



    // first, swap word order
    SimpleVector<char *> *words = tokenizeString( working );

    // always slurr exactly the same for a given speaker
    // repeating the same phrase won't keep remapping
    // but map different length phrases differently
    JenkinsRandomSource slurRand( inSpeakerID + 
                                  words->size() + 
                                  inDrunkenness );
    

    for( int i=0; i<words->size(); i++ ) {
        if( slurRand.getRandomBoundedDouble( 0, 1 ) < wordSwapChance ) {
            char *temp = words->getElementDirect( i );
            
            // possible swap distance based on drunkenness
            
            // again, don't start reording words until 6 drunkenness
            int maxDist = inDrunkenness - 5;

            if( maxDist >= words->size() - i ) {
                maxDist = words->size() - i - 1;
                }
            
            if( maxDist > 0 ) {
                int jump = slurRand.getRandomBoundedInt( 0, maxDist );
            
                
                *( words->getElement( i ) ) = 
                    words->getElementDirect( i + jump );
            
                *( words->getElement( i + jump ) ) = temp;
                }
            }
        }
    

    char **allWords = words->getElementArray();
    char *wordsTogether = join( allWords, words->size(), " " );
    
    words->deallocateStringElements();
    delete words;
    
    delete [] allWords;

    delete [] working;
    
    working = wordsTogether;


    int len = strlen( working );
    for( int i=0; i<len; i++ ) {
        char c = working[i];
        
        slurredChars.push_back( c );

        if( c < 'A' || c > 'Z' ) {
            // only A-Z, no slurred punctuation
            continue;
            }

        if( slurRand.getRandomBoundedDouble( 0, 1 ) < slurChance ) {
            slurredChars.push_back( c );
            }
        }

    delete [] working;
    
    if( extraData != NULL ) {
        slurredChars.appendElementString( extraData );
        delete [] extraData;
        }
    

    return slurredChars.getElementString();
    }


char *yellingSpeech( int inSpeakerID,
                  char *inTranslatedPhrase ) {
    char *working = stringDuplicate( inTranslatedPhrase );
    
    char *starPos = strstr( working, " *" );

    char *extraData = NULL;
    
    if( starPos != NULL ) {
        extraData = stringDuplicate( starPos );
        starPos[0] = '\0';
        }
    
    SimpleVector<char> workedChars;

    int len = strlen( working );
    for( int i=0; i<len; i++ ) {
        char c = working[i];
		
		char r;

        if( c == 'A' ) {
            r = c;
            }
		else if( c == 'E' ) {
			r = c;
			}
		else if( c == 'O' ) {
			r = c;
			}
		else if( c == 'Y' ) {
			r = c;
			}
		else if( c == ',' || c == '.' || c == '!' ) {
			r = '!';
			}
		else {
			workedChars.push_back( c );
			continue;
			}
			

        for(int i = 0; i < 5; i++) {
            workedChars.push_back( r );
            }
        }

    delete [] working;
	
	if( len > 0 ) {
		int repeatLen = randSource.getRandomBoundedDouble( 0, 1 ) * 4 + 1;
        for(int i = 0; i < repeatLen; i++) {
            workedChars.push_back( '!' );
            }
		}
    
    if( extraData != NULL ) {
        workedChars.appendElementString( extraData );
        delete [] extraData;
        }
    

    return workedChars.getElementString();
    }



// with 128-wide tiles, character moves at 480 pixels per second
// at 60 fps, this is 8 pixels per frame
// important that it's a whole number of pixels for smooth camera movement
static double baseWalkSpeed = 3.75;

// min speed for takeoff
static double minFlightSpeed = 15;



double computeMoveSpeed( LiveObject *inPlayer ) {
    double age = computeAge( inPlayer );
    

    double speed = baseWalkSpeed;
    
    // baby moves at 360 pixels per second, or 6 pixels per frame
    double babySpeedFactor = 0.75;

    double fullSpeedAge = 10.0;
    

    if( age < fullSpeedAge ) {
        
        double speedFactor = babySpeedFactor + 
            ( 1.0 - babySpeedFactor ) * age / fullSpeedAge;
        
        speed *= speedFactor;
        }


    // for now, try no age-based speed decrease
    /*
    if( age < 20 ) {
        speed *= age / 20;
        }
    if( age > 40 ) {
        // half speed by 60, then keep slowing down after that
        speed -= (age - 40 ) * 2.0 / 20.0;
        
        }
    */
    // no longer slow down with hunger
    /*
    int foodCap = computeFoodCapacity( inPlayer );
    
    
    if( inPlayer->foodStore <= foodCap / 2 ) {
        // jumps instantly to 1/2 speed at half food, then decays after that
        speed *= inPlayer->foodStore / (double) foodCap;
        }
    */



    // apply character's speed mult
    speed *= getObject( inPlayer->displayID )->speedMult;
    

    char riding = false;
    
    if( inPlayer->holdingID > 0 ) {
        ObjectRecord *r = getObject( inPlayer->holdingID );

        if( r->clothing == 'n' ) {
            // clothing only changes your speed when it's worn
            speed *= r->speedMult;
            }
        
        if( r->rideable ) {
            riding = true;
            }
        }
    

    if( !riding ) {
        // clothing can affect speed

        for( int i=0; i<NUM_CLOTHING_PIECES; i++ ) {
            ObjectRecord *c = clothingByIndex( inPlayer->clothing, i );
            
            if( c != NULL ) {
                
                speed *= c->speedMult;
                }
            }
			
		if( inPlayer->tripping ) {
			speed *= 1.2;
			}
		else if( inPlayer->drunkennessEffect ) {
			speed *= 0.9;
			}
        }

    // never move at 0 speed, divide by 0 errors for eta times
    if( speed < 0.01 ) {
        speed = 0.01;
        }

    
    // after all multipliers, make sure it's a whole number of pixels per frame

    double pixelsPerFrame = speed * 128.0 / 60.0;
    
    
    if( pixelsPerFrame > 0.5 ) {
        // can round to at least one pixel per frame
        pixelsPerFrame = lrint( pixelsPerFrame );
        }
    else {
        // fractional pixels per frame
        
        // ensure a whole number of frames per pixel
        double framesPerPixel = 1.0 / pixelsPerFrame;
        
        framesPerPixel = lrint( framesPerPixel );
        
        pixelsPerFrame = 1.0 / framesPerPixel;
        }
    
    speed = pixelsPerFrame * 60 / 128.0;
        
    return speed;
    }







static float sign( float inF ) {
    if (inF > 0) return 1;
    if (inF < 0) return -1;
    return 0;
    }


// how often do we check what a player is standing on top of for attack effects?
static double playerCrossingCheckStepTime = 0.25;


// for steps in main loop that shouldn't happen every loop
// (loop runs faster or slower depending on how many messages are incoming)
static double periodicStepTime = 0.25;
static double lastPeriodicStepTime = 0;




// recompute heat for fixed number of players per timestep
static int numPlayersRecomputeHeatPerStep = 8;
static int lastPlayerIndexHeatRecomputed = -1;
static double lastHeatUpdateTime = 0;
static double heatUpdateTimeStep = 0.1;


// how often the player's personal heat advances toward their environmental
// heat value
static double heatUpdateSeconds = 2;


// air itself offers some insulation
// a vacuum panel has R-value that is 25x greater than air
static float rAir = 0.04;



// blend R-values multiplicatively, for layers
// 1 - R( A + B ) = (1 - R(A)) * (1 - R(B))
//
// or
//
//R( A + B ) =  R(A) + R(B) - R(A) * R(B)
static double rCombine( double inRA, double inRB ) {
    return inRA + inRB - inRA * inRB;
    }




static float computeClothingR( LiveObject *inPlayer ) {
    
    float headWeight = 0.25;
    float chestWeight = 0.35;
    float buttWeight = 0.2;
    float eachFootWeigth = 0.1;
            
    float backWeight = 0.1;


    float clothingR = 0;
            
    if( inPlayer->clothing.hat != NULL ) {
        clothingR += headWeight *  inPlayer->clothing.hat->rValue;
        }
    if( inPlayer->clothing.tunic != NULL ) {
        clothingR += chestWeight * inPlayer->clothing.tunic->rValue;
        }
    if( inPlayer->clothing.frontShoe != NULL ) {
        clothingR += 
            eachFootWeigth * inPlayer->clothing.frontShoe->rValue;
        }
    if( inPlayer->clothing.backShoe != NULL ) {
        clothingR += eachFootWeigth * 
            inPlayer->clothing.backShoe->rValue;
        }
    if( inPlayer->clothing.bottom != NULL ) {
        clothingR += buttWeight * inPlayer->clothing.bottom->rValue;
        }
    if( inPlayer->clothing.backpack != NULL ) {
        clothingR += backWeight * inPlayer->clothing.backpack->rValue;
        }
    
    // even if the player is naked, they are insulated from their
    // environment by rAir
    return rCombine( rAir, clothingR );
    }



static float computeClothingHeat( LiveObject *inPlayer ) {
    // clothing can contribute heat
    // apply this separate from heat grid above
    float clothingHeat = 0;
    for( int c=0; c<NUM_CLOTHING_PIECES; c++ ) {
                
        ObjectRecord *cO = clothingByIndex( inPlayer->clothing, c );
            
        if( cO != NULL ) {
            clothingHeat += cO->heatValue;

            // contained items in clothing can contribute
            // heat, shielded by clothing r-values
            double cRFactor = 1 - cO->rValue;

            for( int s=0; 
                 s < inPlayer->clothingContained[c].size(); s++ ) {
                        
                ObjectRecord *sO = 
                    getObject( inPlayer->clothingContained[c].
                               getElementDirect( s ) );
                        
                clothingHeat += 
                    sO->heatValue * cRFactor;
                }
            }
        }
    return clothingHeat;
    }



static float computeHeldHeat( LiveObject *inPlayer ) {
    float heat = 0;
    
    // what player is holding can contribute heat
    // add this to the grid, since it's "outside" the player's body
    if( inPlayer->holdingID > 0 ) {
        ObjectRecord *heldO = getObject( inPlayer->holdingID );
                
        heat += heldO->heatValue;
                
        double heldRFactor = 1 - heldO->rValue;
                
        // contained can contribute too, but shielded by r-value
        // of container
        for( int c=0; c<inPlayer->numContained; c++ ) {
                    
            int cID = inPlayer->containedIDs[c];
            char hasSub = false;
                    
            if( cID < 0 ) {
                hasSub = true;
                cID = -cID;
                }

            ObjectRecord *contO = getObject( cID );
                    
            heat += 
                contO->heatValue * heldRFactor;
                    

            if( hasSub ) {
                // sub contained too, but shielded by both r-values
                double contRFactor = 1 - contO->rValue;

                for( int s=0; 
                     s<inPlayer->subContainedIDs[c].size(); s++ ) {
                        
                    ObjectRecord *subO =
                        getObject( inPlayer->subContainedIDs[c].
                                   getElementDirect( s ) );
                            
                    heat += 
                        subO->heatValue * 
                        contRFactor * heldRFactor;
                    }
                }
            }
        }
    return heat;
    }




static void recomputeHeatMap( LiveObject *inPlayer ) {
    
    int gridSize = HEAT_MAP_D * HEAT_MAP_D;

    // assume indoors until we find an air boundary of space
    inPlayer->isIndoors = true;
    

    // what if we recompute it from scratch every time?
    for( int i=0; i<gridSize; i++ ) {
        inPlayer->heatMap[i] = 0;
        }

    float heatOutputGrid[ HEAT_MAP_D * HEAT_MAP_D ];
    float rGrid[ HEAT_MAP_D * HEAT_MAP_D ];
    float rFloorGrid[ HEAT_MAP_D * HEAT_MAP_D ];


    GridPos pos = getPlayerPos( inPlayer );


    // held baby's pos matches parent pos
    if( inPlayer->heldByOther ) {
        LiveObject *parentObject = getLiveObject( inPlayer->heldByOtherID );
        
        if( parentObject != NULL ) {
            pos = getPlayerPos( parentObject );
            }
        } 

    
    

    for( int y=0; y<HEAT_MAP_D; y++ ) {
        int mapY = pos.y + y - HEAT_MAP_D / 2;
                
        for( int x=0; x<HEAT_MAP_D; x++ ) {
                    
            int mapX = pos.x + x - HEAT_MAP_D / 2;
                    
            int j = y * HEAT_MAP_D + x;
            heatOutputGrid[j] = 0;
            rGrid[j] = rAir;
            rFloorGrid[j] = rAir;


            // call Raw version for better performance
            // we don't care if object decayed since we last looked at it
            ObjectRecord *o = getObject( getMapObjectRaw( mapX, mapY ) );
                    
                    
                    

            if( o != NULL ) {
                heatOutputGrid[j] += o->heatValue;
                if( o->permanent ) {
                    // loose objects sitting on ground don't
                    // contribute to r-value (like dropped clothing)
                    rGrid[j] = rCombine( rGrid[j], o->rValue );
                    }


                // skip checking for heat-producing contained items
                // for now.  Consumes too many server-side resources
                // can still check for heat produced by stuff in
                // held container (below).
                        
                if( false && o->numSlots > 0 ) {
                    // contained can produce heat shielded by container
                    // r value
                    double oRFactor = 1 - o->rValue;
                            
                    int numCont;
                    int *cont = getContained( mapX, mapY, &numCont );
                            
                    if( cont != NULL ) {
                                
                        for( int c=0; c<numCont; c++ ) {
                                    
                            int cID = cont[c];
                            char hasSub = false;
                            if( cID < 0 ) {
                                hasSub = true;
                                cID = -cID;
                                }

                            ObjectRecord *cO = getObject( cID );
                            heatOutputGrid[j] += 
                                cO->heatValue * oRFactor;
                                    
                            if( hasSub ) {
                                double cRFactor = 1 - cO->rValue;
                                        
                                int numSub;
                                int *sub = getContained( mapX, mapY, 
                                                         &numSub, 
                                                         c + 1 );
                                if( sub != NULL ) {
                                    for( int s=0; s<numSub; s++ ) {
                                        ObjectRecord *sO = 
                                            getObject( sub[s] );
                                                
                                        heatOutputGrid[j] += 
                                            sO->heatValue * 
                                            cRFactor * 
                                            oRFactor;
                                        }
                                    delete [] sub;
                                    }
                                }
                            }
                        delete [] cont;
                        }
                    }
                }
                    

            // floor can insulate or produce heat too
            ObjectRecord *fO = getObject( getMapFloor( mapX, mapY ) );
                    
            if( fO != NULL ) {
                heatOutputGrid[j] += fO->heatValue;
                rFloorGrid[j] = rCombine( rFloorGrid[j], fO->rValue );
                }
            }
        }


    
    int numNeighbors = 8;
    int ndx[8] = { 0, 1,  0, -1,  1,  1, -1, -1 };
    int ndy[8] = { 1, 0, -1,  0,  1, -1,  1, -1 };
    
            
    int playerMapIndex = 
        ( HEAT_MAP_D / 2 ) * HEAT_MAP_D +
        ( HEAT_MAP_D / 2 );
        

    
            
    heatOutputGrid[ playerMapIndex ] += computeHeldHeat( inPlayer );
    

    // grid of flags for points that are in same airspace (surrounded by walls)
    // as player
    // This is the area where heat spreads evenly by convection
    char airSpaceGrid[ HEAT_MAP_D * HEAT_MAP_D ];
    
    memset( airSpaceGrid, false, HEAT_MAP_D * HEAT_MAP_D );
    
    airSpaceGrid[ playerMapIndex ] = true;

    SimpleVector<int> frontierA;
    SimpleVector<int> frontierB;
    frontierA.push_back( playerMapIndex );
    
    SimpleVector<int> *thisFrontier = &frontierA;
    SimpleVector<int> *nextFrontier = &frontierB;

    while( thisFrontier->size() > 0 ) {

        for( int f=0; f<thisFrontier->size(); f++ ) {
            
            int i = thisFrontier->getElementDirect( f );
            
            char negativeYCutoff = false;
            
            if( rGrid[i] > rAir ) {
                // grid cell is insulating, and somehow it's in our
                // frontier.  Player must be standing behind a closed
                // door.  Block neighbors to south
                negativeYCutoff = true;
                }
            

            int x = i % HEAT_MAP_D;
            int y = i / HEAT_MAP_D;
            
            for( int n=0; n<numNeighbors; n++ ) {
                        
                int nx = x + ndx[n];
                int ny = y + ndy[n];
                
                if( negativeYCutoff && ndy[n] < 1 ) {
                    continue;
                    }

                if( nx >= 0 && nx < HEAT_MAP_D &&
                    ny >= 0 && ny < HEAT_MAP_D ) {

                    int nj = ny * HEAT_MAP_D + nx;
                    
                    if( ! airSpaceGrid[ nj ]
                        && rGrid[ nj ] <= rAir ) {

                        airSpaceGrid[ nj ] = true;
                        
                        nextFrontier->push_back( nj );
                        }
                    }
                }
            }
        
        thisFrontier->deleteAll();
        
        SimpleVector<int> *temp = thisFrontier;
        thisFrontier = nextFrontier;
        
        nextFrontier = temp;
        }
    
    if( rGrid[playerMapIndex] > rAir ) {
        // player standing in insulated spot
        // don't count this as part of their air space
        airSpaceGrid[ playerMapIndex ] = false;
        }

    int numInAirspace = 0;
    for( int i=0; i<gridSize; i++ ) {
        if( airSpaceGrid[ i ] ) {
            numInAirspace++;
            }
        }
    
    
    float rBoundarySum = 0;
    int rBoundarySize = 0;
    
    for( int i=0; i<gridSize; i++ ) {
        if( airSpaceGrid[i] ) {
            
            int x = i % HEAT_MAP_D;
            int y = i / HEAT_MAP_D;
            
            for( int n=0; n<numNeighbors; n++ ) {
                        
                int nx = x + ndx[n];
                int ny = y + ndy[n];
                
                if( nx >= 0 && nx < HEAT_MAP_D &&
                    ny >= 0 && ny < HEAT_MAP_D ) {
                    
                    int nj = ny * HEAT_MAP_D + nx;
                    
                    if( ! airSpaceGrid[ nj ] ) {
                        
                        // boundary!
                        rBoundarySum += rGrid[ nj ];
                        rBoundarySize ++;
                        }
                    }
                else {
                    // boundary is off edge
                    // assume air R-value
                    rBoundarySum += rAir;
                    rBoundarySize ++;
                    inPlayer->isIndoors = false;
                    }
                }
            }
        }

    
    // floor counts as boundary too
    // 4x its effect (seems more important than one of 8 walls
    
    // count non-air floor tiles while we're at it
    int numFloorTilesInAirspace = 0;

    if( numInAirspace > 0 ) {
        for( int i=0; i<gridSize; i++ ) {
            if( airSpaceGrid[i] ) {
                rBoundarySum += 4 * rFloorGrid[i];
                rBoundarySize += 4;
                
                if( rFloorGrid[i] > rAir ) {
                    numFloorTilesInAirspace++;
                    }
                else {
                    // gap in floor
                    inPlayer->isIndoors = false;
                    }
                }
            }
        }
    


    float rBoundaryAverage = rAir;
    
    if( rBoundarySize > 0 ) {
        rBoundaryAverage = rBoundarySum / rBoundarySize;
        }

    
    



    float airSpaceHeatSum = 0;
    
    for( int i=0; i<gridSize; i++ ) {
        if( airSpaceGrid[i] ) {
            airSpaceHeatSum += heatOutputGrid[i];
            }
        }


    float airSpaceHeatVal = 0;
    
    if( numInAirspace > 0 ) {
        // spread heat evenly over airspace
        airSpaceHeatVal = airSpaceHeatSum / numInAirspace;
        }

    float containedAirSpaceHeatVal = airSpaceHeatVal * rBoundaryAverage;
    


    float radiantAirSpaceHeatVal = 0;

    GridPos playerHeatMapPos = { playerMapIndex % HEAT_MAP_D, 
                                 playerMapIndex / HEAT_MAP_D };
    

    int numRadiantHeatSources = 0;
    
    for( int i=0; i<gridSize; i++ ) {
        if( airSpaceGrid[i] && heatOutputGrid[i] > 0 ) {
            
            int x = i % HEAT_MAP_D;
            int y = i / HEAT_MAP_D;
            
            GridPos heatPos = { x, y };
            

            double d = distance( playerHeatMapPos, heatPos );
            
            // avoid infinite heat when player standing on source

            radiantAirSpaceHeatVal += heatOutputGrid[ i ] / ( 1.5 * d + 1 );
            numRadiantHeatSources ++;
            }
        }
    

    float biomeHeatWeight = 1;
    float radiantHeatWeight = 1;
    
    float containedHeatWeight = 4;


    // boundary r-value also limits affect of biome heat on player's
    // environment... keeps biome "out"
    float boundaryLeak = 1 - rBoundaryAverage;

    if( numFloorTilesInAirspace != numInAirspace ) {
        // biome heat invades airspace if entire thing isn't covered by
        // a floor (not really indoors)
        boundaryLeak = 1;
        }


    // a hot biome only pulls us up above perfect
    // (hot biome leaking into a building can never make the building
    //  just right).
    // Enclosed walls can make a hot biome not as hot, but never cool
    float biomeHeat = getBiomeHeatValue( getMapBiome( pos.x, pos.y ) );
    
    if( biomeHeat > targetHeat ) {
        biomeHeat = boundaryLeak * (biomeHeat - targetHeat) + targetHeat;
        }
    else if( biomeHeat < 0 ) {
        // a cold biome's coldness is modulated directly by walls, however
        biomeHeat = boundaryLeak * biomeHeat;
        }
    
    // small offset to ensure that naked-on-green biome the same
    // in new heat model as old
    float constHeatValue = 1.1;

    inPlayer->envHeat = 
        radiantHeatWeight * radiantAirSpaceHeatVal + 
        containedHeatWeight * containedAirSpaceHeatVal +
        biomeHeatWeight * biomeHeat +
        constHeatValue;

    inPlayer->biomeHeat = biomeHeat + constHeatValue;
    }




typedef struct MoveRecord {
        int playerID;
        char *formatString;
        int absoluteX, absoluteY;
    } MoveRecord;



// formatString in returned record destroyed by caller
MoveRecord getMoveRecord( LiveObject *inPlayer,
                          char inNewMovesOnly,
                          SimpleVector<ChangePosition> *inChangeVector = 
                          NULL ) {

    MoveRecord r;
    r.playerID = inPlayer->id;
    
    // p_id xs ys xd yd fraction_done eta_sec
    
    double deltaSec = Time::getCurrentTime() - inPlayer->moveStartTime;
    
    double etaSec = inPlayer->moveTotalSeconds - deltaSec;
    
    if( etaSec < 0 ) {
        etaSec = 0;
        }

    
    r.absoluteX = inPlayer->xs;
    r.absoluteY = inPlayer->ys;
            
            
    SimpleVector<char> messageLineBuffer;
    
    // start is absolute
    char *startString = autoSprintf( "%d %%d %%d %.3f %.3f %d", 
                                     inPlayer->id, 
                                     inPlayer->moveTotalSeconds, etaSec,
                                     inPlayer->pathTruncated );
    // mark that this has been sent
    inPlayer->pathTruncated = false;

    if( inNewMovesOnly ) {
        inPlayer->newMove = false;
        }

            
    messageLineBuffer.appendElementString( startString );
    delete [] startString;
            
    for( int p=0; p<inPlayer->pathLength; p++ ) {
                // rest are relative to start
        char *stepString = autoSprintf( " %d %d", 
                                        inPlayer->pathToDest[p].x
                                        - inPlayer->xs,
                                        inPlayer->pathToDest[p].y
                                        - inPlayer->ys );
        
        messageLineBuffer.appendElementString( stepString );
        delete [] stepString;
        }
    
    messageLineBuffer.appendElementString( "\n" );
    
    r.formatString = messageLineBuffer.getElementString();    
    
    if( inChangeVector != NULL ) {
        ChangePosition p = { inPlayer->xd, inPlayer->yd, false };
        inChangeVector->push_back( p );
        }

    return r;
    }



SimpleVector<MoveRecord> getMoveRecords( 
    char inNewMovesOnly,
    SimpleVector<ChangePosition> *inChangeVector = NULL ) {
    
    SimpleVector<MoveRecord> v;
    
    int numPlayers = players.size();
                
    for( int i=0; i<numPlayers; i++ ) {
                
        LiveObject *o = players.getElement( i );                
        
        if( o->error ) {
            continue;
            }

        if( ( o->xd != o->xs || o->yd != o->ys )
            &&
            ( o->newMove || !inNewMovesOnly ) ) {
            
 
            MoveRecord r = getMoveRecord( o, inNewMovesOnly, inChangeVector );
            
            v.push_back( r );
            }
        }

    return v;
    }



char *getMovesMessageFromList( SimpleVector<MoveRecord> *inMoves,
                               GridPos inRelativeToPos ) {

    int numLines = 0;
    
    SimpleVector<char> messageBuffer;

    messageBuffer.appendElementString( "PM\n" );

    for( int i=0; i<inMoves->size(); i++ ) {
        MoveRecord r = inMoves->getElementDirect(i);
        
        char *line = autoSprintf( r.formatString, 
                                  r.absoluteX - inRelativeToPos.x,
                                  r.absoluteY - inRelativeToPos.y );
        
        messageBuffer.appendElementString( line );
        delete [] line;
        
        numLines ++;
        }
    
    if( numLines > 0 ) {
        
        messageBuffer.push_back( '#' );
                
        char *message = messageBuffer.getElementString();
        
        return message;
        }
    
    return NULL;
    }



double intDist( int inXA, int inYA, int inXB, int inYB ) {
    double dx = (double)inXA - (double)inXB;
    double dy = (double)inYA - (double)inYB;

    return sqrt(  dx * dx + dy * dy );
    }
    
    
    
// returns NULL if there are no matching moves
// positions in moves relative to inRelativeToPos
// filters out moves that are taking place further than 32 away from inLocalPos
char *getMovesMessage( char inNewMovesOnly,
                       GridPos inRelativeToPos,
                       GridPos inLocalPos,
                       SimpleVector<ChangePosition> *inChangeVector = NULL ) {
    
    
    SimpleVector<MoveRecord> v = getMoveRecords( inNewMovesOnly, 
                                                 inChangeVector );
    
    SimpleVector<MoveRecord> closeRecords;

    for( int i=0; i<v.size(); i++ ) {
        MoveRecord r = v.getElementDirect( i );
        
        double d = intDist( r.absoluteX, r.absoluteY,
                            inLocalPos.x, inLocalPos.y );
        
        if( d <= 32 ) {
            closeRecords.push_back( r );
            }
        }
    
    

    char *message = getMovesMessageFromList( &closeRecords, inRelativeToPos );
    
    for( int i=0; i<v.size(); i++ ) {
        delete [] v.getElement(i)->formatString;
        }
    
    return message;
    }



static char isGridAdjacent( int inXA, int inYA, int inXB, int inYB ) {
    if( ( abs( inXA - inXB ) == 1 && inYA == inYB ) 
        ||
        ( abs( inYA - inYB ) == 1 && inXA == inXB ) ) {
        
        return true;
        }

    return false;
    }


//static char isGridAdjacent( GridPos inA, GridPos inB ) {
//    return isGridAdjacent( inA.x, inA.y, inB.x, inB.y );
//    }


static char isGridAdjacentDiag( int inXA, int inYA, int inXB, int inYB ) {
    if( isGridAdjacent( inXA, inYA, inXB, inYB ) ) {
        return true;
        }
    
    if( abs( inXA - inXB ) == 1 && abs( inYA - inYB ) == 1 ) {
        return true;
        }
    
    return false;
    }


static char isGridAdjacentDiag( GridPos inA, GridPos inB ) {
    return isGridAdjacentDiag( inA.x, inA.y, inB.x, inB.y );
    }



static char equal( GridPos inA, GridPos inB ) {
    if( inA.x == inB.x && inA.y == inB.y ) {
        return true;
        }
    return false;
    }

static int chunkDimensionX = 32;
static int chunkDimensionY = 30;


static int getMaxChunkDimension() {
    return chunkDimensionX;
    }


static SocketPoll sockPoll;



static void setPlayerDisconnected( LiveObject *inPlayer, 
                                   const char *inReason ) {    
    /*
    setDeathReason( inPlayer, "disconnected" );
    
    inPlayer->error = true;
    inPlayer->errorCauseString = inReason;
    */
    // don't kill them
    
    // just mark them as not connected

    AppLog::infoF( "Player %d (%s) marked as disconnected (%s).",
                   inPlayer->id, inPlayer->email, inReason );
    inPlayer->connected = false;

    // when player reconnects, they won't get a force PU message
    // so we shouldn't be waiting for them to ack
    inPlayer->waitingForForceResponse = false;


    if( inPlayer->vogMode ) {    
        inPlayer->vogMode = false;
                        
        GridPos p = inPlayer->preVogPos;
        
        inPlayer->xd = p.x;
        inPlayer->yd = p.y;
        
        inPlayer->xs = p.x;
        inPlayer->ys = p.y;

        inPlayer->birthPos = inPlayer->preVogBirthPos;
        }
    
    
    if( inPlayer->sock != NULL ) {
        // also, stop polling their socket, which will trigger constant
        // socket events from here on out, and cause us to busy-loop
        sockPoll.removeSocket( inPlayer->sock );

        delete inPlayer->sock;
        inPlayer->sock = NULL;
        }
    if( inPlayer->sockBuffer != NULL ) {
        delete inPlayer->sockBuffer;
        inPlayer->sockBuffer = NULL;
        }
    }




static void sendGlobalMessage( char *inMessage ) {
    char found;
    char *noSpaceMessage = replaceAll( inMessage, " ", "_", &found );

    char *fullMessage = autoSprintf( "MS\n%s\n#", noSpaceMessage );
    
    delete [] noSpaceMessage;

    int len = strlen( fullMessage );
    
    for( int i=0; i<players.size(); i++ ) {
        LiveObject *o = players.getElement( i );
        
        if( ! o->error && ! o->isTutorial && o->connected ) {
            int numSent = 
                o->sock->send( (unsigned char*)fullMessage, 
                               len, 
                               false, false );
        
            if( numSent != len ) {
                setPlayerDisconnected( o, "Socket write failed" );
                }
            }
        }
    delete [] fullMessage;
    }



typedef struct WarPeaceMessageRecord {
        char war;
        int lineageAEveID;
        int lineageBEveID;
        double t;
    } WarPeaceMessageRecord;

SimpleVector<WarPeaceMessageRecord> warPeaceRecords;



void sendPeaceWarMessage( const char *inPeaceOrWar,
                          char inWar,
                          int inLineageAEveID, int inLineageBEveID ) {
    
    double curTime = Time::getCurrentTime();
    
    for( int i=0; i<warPeaceRecords.size(); i++ ) {
        WarPeaceMessageRecord *r = warPeaceRecords.getElement( i );
        
        if( inWar != r->war ) {
            continue;
            }
        
        if( ( r->lineageAEveID == inLineageAEveID &&
              r->lineageBEveID == inLineageBEveID )
            ||
            ( r->lineageAEveID == inLineageBEveID &&
              r->lineageBEveID == inLineageAEveID ) ) {

            if( r->t > curTime - 3 * 60 ) {
                // stil fresh, last similar message happened
                // less than three minutes ago
                return;
                }
            else {
                // stale
                // remove it
                warPeaceRecords.deleteElement( i );
                break;
                }
            }
        }
    WarPeaceMessageRecord r = { inWar, inLineageAEveID, inLineageBEveID,
                                curTime };
    warPeaceRecords.push_back( r );


    const char *nameA = "NAMELESS";
    const char *nameB = "NAMELESS";
    
    for( int j=0; j<players.size(); j++ ) {
        LiveObject *o = players.getElement( j );
                        
        if( ! o->error && 
            o->lineageEveID == inLineageAEveID &&
            o->familyName != NULL ) {
            nameA = o->familyName;
            break;
            }
        }
    for( int j=0; j<players.size(); j++ ) {
        LiveObject *o = players.getElement( j );
                        
        if( ! o->error && 
            o->lineageEveID == inLineageBEveID &&
            o->familyName != NULL ) {
            nameB = o->familyName;
            break;
            }
        }

    char *message = autoSprintf( "%s BETWEEN %s**AND %s FAMILIES",
                                 inPeaceOrWar,
                                 nameA, nameB );

    sendGlobalMessage( message );
    
    delete [] message;
    }




void checkCustomGlobalMessage() {
    
    if( ! SettingsManager::getIntSetting( "customGlobalMessageOn", 0 ) ) {
        return;
        }


    double spacing = 
        SettingsManager::getDoubleSetting( 
            "customGlobalMessageSecondsSpacing", 10.0 );
    
    double lastTime = 
        SettingsManager::getDoubleSetting( 
            "customGlobalMessageLastSendTime", 0.0 );

    double curTime = Time::getCurrentTime();
    
    if( curTime - lastTime < spacing ) {
        return;
        }
        

    
    // check if there's a new custom message waiting
    char *message = 
        SettingsManager::getSettingContents( "customGlobalMessage", 
                                             "" );
    
    if( strcmp( message, "" ) != 0 ) {
        

        int numLines;
        
        char **lines = split( message, "\n", &numLines );
        
        int nextLine = 
            SettingsManager::getIntSetting( 
                "customGlobalMessageNextLine", 0 );
        
        if( nextLine < numLines ) {
            sendGlobalMessage( lines[nextLine] );
            
            nextLine++;
            SettingsManager::setSetting( 
                "customGlobalMessageNextLine", nextLine );

            SettingsManager::setDoubleSetting( 
                "customGlobalMessageLastSendTime", curTime );
            }
        else {
            // out of lines
            SettingsManager::setSetting( "customGlobalMessageOn", 0 );
            SettingsManager::setSetting( "customGlobalMessageNextLine", 0 );
            }

        for( int i=0; i<numLines; i++ ) {
            delete [] lines[i];
            }
        delete [] lines;
        }
    else {
        // no message, disable
        SettingsManager::setSetting( "customGlobalMessageOn", 0 );
        }
    
    delete [] message;
    }





// sets lastSentMap in inO if chunk goes through
// returns result of send, auto-marks error in inO
int sendMapChunkMessage( LiveObject *inO, 
                         char inDestOverride = false,
                         int inDestOverrideX = 0, 
                         int inDestOverrideY = 0 ) {
    
    if( ! inO->connected ) {
        // act like it was a successful send so we can move on until
        // they reconnect later
        return 1;
        }
    
    int messageLength = 0;

    int xd = inO->xd;
    int yd = inO->yd;
    
    if( inDestOverride ) {
        xd = inDestOverrideX;
        yd = inDestOverrideY;
        }
    
    
    int halfW = chunkDimensionX / 2;
    int halfH = chunkDimensionY / 2;
    
    int fullStartX = xd - halfW;
    int fullStartY = yd - halfH;
    
    int numSent = 0;

    

    if( ! inO->firstMapSent ) {
        // send full rect centered on x,y
        
        inO->firstMapSent = true;
        
        unsigned char *mapChunkMessage = getChunkMessage( fullStartX,
                                                          fullStartY,
                                                          chunkDimensionX,
                                                          chunkDimensionY,
                                                          inO->birthPos,
                                                          &messageLength );
                
        numSent += 
            inO->sock->send( mapChunkMessage, 
                             messageLength, 
                             false, false );
                
        delete [] mapChunkMessage;
        }
    else {
        
        // our closest previous chunk center
        int lastX = inO->lastSentMapX;
        int lastY = inO->lastSentMapY;


        // split next chunk into two bars by subtracting last chunk
        
        int horBarStartX = fullStartX;
        int horBarStartY = fullStartY;
        int horBarW = chunkDimensionX;
        int horBarH = chunkDimensionY;
        
        if( yd > lastY ) {
            // remove bottom of bar
            horBarStartY = lastY + halfH;
            horBarH = yd - lastY;
            }
        else {
            // remove top of bar
            horBarH = lastY - yd;
            }
        

        int vertBarStartX = fullStartX;
        int vertBarStartY = fullStartY;
        int vertBarW = chunkDimensionX;
        int vertBarH = chunkDimensionY;
        
        if( xd > lastX ) {
            // remove left part of bar
            vertBarStartX = lastX + halfW;
            vertBarW = xd - lastX;
            }
        else {
            // remove right part of bar
            vertBarW = lastX - xd;
            }
        
        // now trim vert bar where it intersects with hor bar
        if( yd > lastY ) {
            // remove top of vert bar
            vertBarH -= horBarH;
            }
        else {
            // remove bottom of vert bar
            vertBarStartY = horBarStartY + horBarH;
            vertBarH -= horBarH;
            }
        
        
        // only send if non-zero width and height
        if( horBarW > 0 && horBarH > 0 ) {
            int len;
            unsigned char *mapChunkMessage = getChunkMessage( horBarStartX,
                                                              horBarStartY,
                                                              horBarW,
                                                              horBarH,
                                                              inO->birthPos,
                                                              &len );
            messageLength += len;
            
            numSent += 
                inO->sock->send( mapChunkMessage, 
                                 len, 
                                 false, false );
            
            delete [] mapChunkMessage;
            }
        if( vertBarW > 0 && vertBarH > 0 ) {
            int len;
            unsigned char *mapChunkMessage = getChunkMessage( vertBarStartX,
                                                              vertBarStartY,
                                                              vertBarW,
                                                              vertBarH,
                                                              inO->birthPos,
                                                              &len );
            messageLength += len;
            
            numSent += 
                inO->sock->send( mapChunkMessage, 
                                 len, 
                                 false, false );
            
            delete [] mapChunkMessage;
            }
        }
    
    
    inO->gotPartOfThisFrame = true;
                

    if( numSent == messageLength ) {
        // sent correctly
        inO->lastSentMapX = xd;
        inO->lastSentMapY = yd;
        }
    else {
        setPlayerDisconnected( inO, "Socket write failed" );
        }
    return numSent;
    }







char *getHoldingString( LiveObject *inObject ) {
    
    int holdingID = hideIDForClient( inObject->holdingID );    


    if( inObject->numContained == 0 ) {
        return autoSprintf( "%d", holdingID );
        }

    
    SimpleVector<char> buffer;
    

    char *idString = autoSprintf( "%d", holdingID );
    
    buffer.appendElementString( idString );
    
    delete [] idString;
    
    
    if( inObject->numContained > 0 ) {
        for( int i=0; i<inObject->numContained; i++ ) {
            
            char *idString = autoSprintf( 
                ",%d", 
                hideIDForClient( abs( inObject->containedIDs[i] ) ) );
    
            buffer.appendElementString( idString );
    
            delete [] idString;

            if( inObject->subContainedIDs[i].size() > 0 ) {
                for( int s=0; s<inObject->subContainedIDs[i].size(); s++ ) {
                    
                    idString = autoSprintf( 
                        ":%d", 
                        hideIDForClient( 
                            inObject->subContainedIDs[i].
                            getElementDirect( s ) ) );
    
                    buffer.appendElementString( idString );
                
                    delete [] idString;
                    }
                }
            }
        }
    
    return buffer.getElementString();
    }



// only consider living, non-moving players
char isMapSpotEmptyOfPlayers( int inX, int inY ) {

    int numLive = players.size();
    
    for( int i=0; i<numLive; i++ ) {
        LiveObject *nextPlayer = players.getElement( i );
        
        if( // not about to be deleted
            ! nextPlayer->error &&
            // held players aren't on map (their coordinates are stale)
            ! nextPlayer->heldByOther &&
            // stationary
            nextPlayer->xs == nextPlayer->xd &&
            nextPlayer->ys == nextPlayer->yd &&
            // in this spot
            inX == nextPlayer->xd &&
            inY == nextPlayer->yd ) {
            return false;            
            } 
        }
    
    return true;
    }




// checks both grid of objects and live, non-moving player positions
char isMapSpotEmpty( int inX, int inY, char inConsiderPlayers = true ) {
    int target = getMapObject( inX, inY );
    
    if( target != 0 ) {
        return false;
        }
    
    if( !inConsiderPlayers ) {
        return true;
        }
    
    return isMapSpotEmptyOfPlayers( inX, inY );
    }



static void setFreshEtaDecayForHeld( LiveObject *inPlayer ) {
    
    if( inPlayer->holdingID == 0 ) {
        inPlayer->holdingEtaDecay = 0;
        }
    
    // does newly-held object have a decay defined?
    TransRecord *newDecayT = getMetaTrans( -1, inPlayer->holdingID );
                    
    if( newDecayT != NULL ) {
        inPlayer->holdingEtaDecay = 
            Time::timeSec() + newDecayT->autoDecaySeconds;
        }
    else {
        // no further decay
        inPlayer->holdingEtaDecay = 0;
        }
    }



void handleMapChangeToPaths( 
    int inX, int inY, ObjectRecord *inNewObject,
    SimpleVector<int> *inPlayerIndicesToSendUpdatesAbout ) {
    
    if( inNewObject->blocksWalking ) {
    
        GridPos dropSpot = { inX, inY };
          
        int numLive = players.size();
                      
        for( int j=0; j<numLive; j++ ) {
            LiveObject *otherPlayer = 
                players.getElement( j );
            
            if( otherPlayer->error ) {
                continue;
                }

            if( otherPlayer->xd != otherPlayer->xs ||
                otherPlayer->yd != otherPlayer->ys ) {
                
                GridPos cPos = 
                    computePartialMoveSpot( otherPlayer );
                                        
                if( distance( cPos, dropSpot ) 
                    <= 2 * pathDeltaMax ) {
                                            
                    // this is close enough
                    // to this path that it might
                    // block it
                
                    int c = computePartialMovePathStep( otherPlayer );

                    // -1 means starting, pre-path pos is closest
                    // push it up to first path step
                    if( c < 0 ) {
                        c = 0;
                        }

                    char blocked = false;
                    int blockedStep = -1;
                                            
                    for( int p=c; 
                         p<otherPlayer->pathLength;
                         p++ ) {
                                                
                        if( equal( 
                                otherPlayer->
                                pathToDest[p],
                                dropSpot ) ) {
                                                    
                            blocked = true;
                            blockedStep = p;
                            break;
                            }
                        }
                                            
                    if( blocked ) {
                        printf( 
                            "  Blocked by drop\n" );
                        }
                                            

                    if( blocked &&
                        blockedStep > 0 ) {
                                                
                        otherPlayer->pathLength
                            = blockedStep;
                        otherPlayer->pathTruncated
                            = true;

                        // update timing
                        double dist = 
                            measurePathLength( otherPlayer->xs,
                                               otherPlayer->ys,
                                               otherPlayer->pathToDest,
                                               otherPlayer->pathLength );    
                                                
                        double distAlreadyDone =
                            measurePathLength( otherPlayer->xs,
                                               otherPlayer->ys,
                                               otherPlayer->pathToDest,
                                               c );
                            
                        double moveSpeed = computeMoveSpeed( otherPlayer ) *
                            getPathSpeedModifier( otherPlayer->pathToDest,
                                                  otherPlayer->pathLength );

                        otherPlayer->moveTotalSeconds 
                            = 
                            dist / 
                            moveSpeed;
                            
                        double secondsAlreadyDone = 
                            distAlreadyDone / 
                            moveSpeed;
                                
                        otherPlayer->moveStartTime = 
                            Time::getCurrentTime() - 
                            secondsAlreadyDone;
                            
                        otherPlayer->newMove = true;
                                                
                        otherPlayer->xd 
                            = otherPlayer->pathToDest[
                                blockedStep - 1].x;
                        otherPlayer->yd 
                            = otherPlayer->pathToDest[
                                blockedStep - 1].y;
                                                
                        }
                    else if( blocked ) {
                        // cutting off path
                        // right at the beginning
                        // nothing left

                        // end move now
                        otherPlayer->xd = 
                            otherPlayer->xs;
                                                
                        otherPlayer->yd = 
                            otherPlayer->ys;
                             
                        otherPlayer->posForced = true;
                    
                        inPlayerIndicesToSendUpdatesAbout->push_back( j );
                        }
                    } 
                                        
                }                                    
            }
        }
    
    }



// returns true if found
char findDropSpot( int inX, int inY, int inSourceX, int inSourceY, 
                   GridPos *outSpot ) {
    char found = false;
    int foundX = inX;
    int foundY = inY;
    
    // change direction of throw
    // to match direction of 
    // drop action
    int xDir = inX - inSourceX;
    int yDir = inY - inSourceY;
                                    
        
    if( xDir == 0 && yDir == 0 ) {
        xDir = 1;
        }
    
    // cap to magnitude
    // death drops can be non-adjacent
    if( xDir > 1 ) {
        xDir = 1;
        }
    if( xDir < -1 ) {
        xDir = -1;
        }
    
    if( yDir > 1 ) {
        yDir = 1;
        }
    if( yDir < -1 ) {
        yDir = -1;
        }
        

    // check in y dir first at each
    // expanded radius?
    char yFirst = false;
        
    if( yDir != 0 ) {
        yFirst = true;
        }
        
    for( int d=1; d<10 && !found; d++ ) {
            
        char doneY0 = false;
            
        for( int yD = -d; yD<=d && !found; 
             yD++ ) {
                
            if( ! doneY0 ) {
                yD = 0;
                }
                
            if( yDir != 0 ) {
                yD *= yDir;
                }
                
            char doneX0 = false;
                
            for( int xD = -d; 
                 xD<=d && !found; 
                 xD++ ) {
                    
                if( ! doneX0 ) {
                    xD = 0;
                    }
                    
                if( xDir != 0 ) {
                    xD *= xDir;
                    }
                    
                    
                if( yD == 0 && xD == 0 ) {
                    if( ! doneX0 ) {
                        doneX0 = true;
                            
                        // back up in loop
                        xD = -d - 1;
                        }
                    continue;
                    }
                                                
                int x = 
                    inSourceX + xD;
                int y = 
                    inSourceY + yD;
                                                
                if( yFirst ) {
                    // swap them
                    // to reverse order
                    // of expansion
                    x = 
                        inSourceX + yD;
                    y =
                        inSourceY + xD;
                    }
                                                


                if( 
                    isMapSpotEmpty( x, y ) ) {
                                                    
                    found = true;
                    foundX = x;
                    foundY = y;
                    }
                                                    
                if( ! doneX0 ) {
                    doneX0 = true;
                                                        
                    // back up in loop
                    xD = -d - 1;
                    }
                }
                                                
            if( ! doneY0 ) {
                doneY0 = true;
                                                
                // back up in loop
                yD = -d - 1;
                }
            }
        }

    outSpot->x = foundX;
    outSpot->y = foundY;
    return found;
    }



#include "spiral.h"

GridPos findClosestEmptyMapSpot( int inX, int inY, int inMaxPointsToCheck,
                                 char *outFound ) {

    GridPos center = { inX, inY };

    for( int i=0; i<inMaxPointsToCheck; i++ ) {
        GridPos p = getSpriralPoint( center, i );

        if( isMapSpotEmpty( p.x, p.y, false ) ) {    
            *outFound = true;
            return p;
            }
        }
    
    *outFound = false;
    GridPos p = { inX, inY };
    return p;
    }



// returns NULL if not found
static LiveObject *getPlayerByEmail( char *inEmail ) {
    for( int j=0; j<players.size(); j++ ) {
        LiveObject *otherPlayer = players.getElement( j );
        if( ! otherPlayer->error &&
            otherPlayer->email != NULL &&
            strcmp( otherPlayer->email, inEmail ) == 0 ) {
            
            return otherPlayer;
            }
        }
    return NULL;
    }



static int usePersonalCurses = 0;





SimpleVector<ChangePosition> newSpeechPos;

SimpleVector<char*> newSpeechPhrases;
SimpleVector<int> newSpeechPlayerIDs;
SimpleVector<char> newSpeechCurseFlags;
//2HOL additions for: password-protected objects
SimpleVector<char> newSpeechPasswordFlags;



SimpleVector<char*> newLocationSpeech;
SimpleVector<ChangePosition> newLocationSpeechPos;




char *isCurseNamingSay( char *inSaidString );

//2HOL additions for: password-protected objects
char *isPasswordSettingSay( char *inSaidString );
char *isPasswordInvokingSay( char *inSaidString );

static void makePlayerSay( LiveObject *inPlayer, char *inToSay, bool inPrivate = false ) {    
                        
    if( inPlayer->lastSay != NULL ) {
        delete [] inPlayer->lastSay;
        inPlayer->lastSay = NULL;
        }
    inPlayer->lastSay = stringDuplicate( inToSay );

    //2HOL additions for: password-protected objects
    char *sayingPassword = NULL;
    char *assigningPassword = NULL;
    if ( passwordTransitionsAllowed ) {
        
        if ( passwordInvocationAndSettingAreSeparated ) {
            sayingPassword = isPasswordInvokingSay( inToSay );
            if( sayingPassword != NULL ) {
                // AppLog::infoF( "2HOL DEBUG: Player says password. New password assigned to a player." );
                inPlayer->saidPassword = stringDuplicate( sayingPassword );
                // AppLog::infoF( "2HOL DEBUG: Player's password is %s", inPlayer->saidPassword );
				//if passwordSilent = true, no need to display anything, as well as make any further checks, just cut it after the assignment is done
				if( passwordSilent ) { return; }
                }

            }
    
    
        assigningPassword = isPasswordSettingSay( inToSay );
        if( assigningPassword != NULL ) {
            // AppLog::infoF( "2HOL DEBUG: Player sets new password for future assignment." );
            inPlayer->assignedPassword = stringDuplicate( assigningPassword );
            if ( !passwordInvocationAndSettingAreSeparated ) { inPlayer->saidPassword = stringDuplicate( assigningPassword ); }
            // AppLog::infoF( "2HOL DEBUG: Password for future assignment password is %s", inPlayer->assignedPassword );
            //if passwordSilent = true, no need to display anything, as well as make any further checks, just cut it after the assignment is done
            if( passwordSilent ) { return; }
            }

        }

    char isCurse = false;

    char *cursedName = isCurseNamingSay( inToSay );

    char isYouShortcut = false;
    char isBabyShortcut = false;
    if( strcmp( inToSay, curseYouPhrase ) == 0 ) {
        isYouShortcut = true;
        }
    if( strcmp( inToSay, curseBabyPhrase ) == 0 ) {
        isBabyShortcut = true;
        }
    

    if( cursedName != NULL ) {
        // it's a pointer into inToSay
        
        // make a copy so we can delete it later
        cursedName = stringDuplicate( cursedName );
        }


            
    int curseDistance = SettingsManager::getIntSetting( "curseDistance", 200 );
    
        
    if( cursedName == NULL &&
        players.size() >= minActivePlayersForLanguages ) {
        
        // consider cursing in other languages

        int speakerAge = computeAge( inPlayer );
        
        GridPos speakerPos = getPlayerPos( inPlayer );
        
        for( int i=0; i<players.size(); i++ ) {
            LiveObject *otherPlayer = players.getElement( i );
            
            if( otherPlayer == inPlayer ||
                otherPlayer->error ||
                otherPlayer->lineageEveID == inPlayer->lineageEveID ) {
                continue;
                }

            if( distance( speakerPos, getPlayerPos( otherPlayer ) ) >
                curseDistance ) {
                // only consider nearby players
                continue;
                }
                
            char *translatedPhrase =
                mapLanguagePhrase( 
                    inToSay,
                    inPlayer->lineageEveID,
                    otherPlayer->lineageEveID,
                    inPlayer->id,
                    otherPlayer->id,
                    speakerAge,
                    computeAge( otherPlayer ),
                    inPlayer->parentID,
                    otherPlayer->parentID );
            
            cursedName = isCurseNamingSay( translatedPhrase );
            
            if( strcmp( translatedPhrase, curseYouPhrase ) == 0 ) {
                // said CURSE YOU in other language
                isYouShortcut = true;
                }

            // make copy so we can delete later an delete the underlying
            // translatedPhrase now
            
            if( cursedName != NULL ) {
                cursedName = stringDuplicate( cursedName );
                }

            delete [] translatedPhrase;

            if( cursedName != NULL ) {
                int namedPersonLineageEveID = 
                    getCurseReceiverLineageEveID( cursedName );
                
                if( namedPersonLineageEveID == otherPlayer->lineageEveID ) {
                    // the named person belonged to the lineage of the 
                    // person who spoke this language!
                    break;
                    }
                // else cursed in this language, for someone outside
                // this language's line
                delete [] cursedName;
                cursedName = NULL;
                }
            }
        }



    LiveObject *youCursePlayer = NULL;
    LiveObject *babyCursePlayer = NULL;

    if( isYouShortcut ) {
        // find closest player
        GridPos speakerPos = getPlayerPos( inPlayer );
        
        LiveObject *closestOther = NULL;
        double closestDist = 9999999;
        
        for( int i=0; i<players.size(); i++ ) {
            LiveObject *otherPlayer = players.getElement( i );
            
            if( otherPlayer == inPlayer ||
                otherPlayer->error ) {
                continue;
                }
            double dist = distance( speakerPos, getPlayerPos( otherPlayer ) );

            if( dist > getMaxChunkDimension() ) {
                // only consider nearby players
                // don't use curseDistance setting here,
                // because we don't want CURSE YOU to apply from too
                // far away (would likely be a random target player)
                continue;
                }
            if( dist < closestDist ) {
                closestDist = dist;
                closestOther = otherPlayer;
                }
            }


        if( closestOther != NULL ) {
            youCursePlayer = closestOther;
            
            if( cursedName != NULL ) {
                delete [] cursedName;
                cursedName = NULL;
                }

            if( youCursePlayer->name != NULL ) {
                // allow name-based curse to go through, if possible
                cursedName = stringDuplicate( youCursePlayer->name );
                }
            }
        }
    else if( isBabyShortcut ) {
        // this case is more robust (below) by simply using the lastBabyEmail
        // in all cases

        // That way there's no confusing about who MY BABY is (always the
        // most-recent baby).
        }


    // make sure, no matter what, we can't curse living 
    // people at a great distance
    // note that, sice we're not tracking dead people here
    // that case will be caught below, since the curses.h tracks death
    // locations
    GridPos speakerPos = getPlayerPos( inPlayer );
    
    if( cursedName != NULL &&
        strcmp( cursedName, "" ) != 0 ) {

        for( int i=0; i<players.size(); i++ ) {
            LiveObject *otherPlayer = players.getElement( i );
            
            if( otherPlayer == inPlayer ||
                otherPlayer->error ) {
                continue;
                }
            if( otherPlayer->name != NULL &&
                strcmp( otherPlayer->name, cursedName ) == 0 ) {
                // matching player
                
                double dist = 
                    distance( speakerPos, getPlayerPos( otherPlayer ) );

                if( dist > curseDistance ) {
                    // too far
                    delete [] cursedName;
                    cursedName = NULL;
                    }
                break;
                }
            }
        }
    
    
    char *dbCurseTargetEmail = NULL;

    
    char canCurse = false;
    
    if( inPlayer->curseTokenCount > 0 ) {
        canCurse = true;
        }
    

    if( canCurse && 
        cursedName != NULL && 
        strcmp( cursedName, "" ) != 0 ) {
        
        isCurse = cursePlayer( inPlayer->id,
                               inPlayer->lineageEveID,
                               inPlayer->email,
                               speakerPos,
                               curseDistance,
                               cursedName );
        
        if( isCurse ) {
            char *targetEmail = getCurseReceiverEmail( cursedName );
            if( targetEmail != NULL ) {
                setDBCurse( inPlayer->email, targetEmail );
                dbCurseTargetEmail = targetEmail;
                }
            }
        }
    
    
    if( cursedName != NULL ) {
        delete [] cursedName;
        }
    

    if( canCurse && !isCurse ) {
        // named curse didn't happen above
        // maybe we used a shortcut, and target didn't have name
        
        if( isYouShortcut && youCursePlayer != NULL &&
            spendCurseToken( inPlayer->email ) ) {
            
            isCurse = true;
            setDBCurse( inPlayer->email, youCursePlayer->email );
            dbCurseTargetEmail = youCursePlayer->email;
            }
        else if( isBabyShortcut && babyCursePlayer != NULL &&
            spendCurseToken( inPlayer->email ) ) {
            
            isCurse = true;
            char *targetEmail = babyCursePlayer->email;
            
            if( strcmp( targetEmail, "email_cleared" ) == 0 ) {
                // deleted players allowed here
                targetEmail = babyCursePlayer->origEmail;
                }
            if( targetEmail != NULL ) {
                setDBCurse( inPlayer->email, targetEmail );
                dbCurseTargetEmail = targetEmail;
                }
            }
        else if( isBabyShortcut && babyCursePlayer == NULL &&
                 inPlayer->lastBabyEmail != NULL &&
                 spendCurseToken( inPlayer->email ) ) {
            
            isCurse = true;
            
            setDBCurse( inPlayer->email, inPlayer->lastBabyEmail );
            dbCurseTargetEmail = inPlayer->lastBabyEmail;
            }
        }

    if( dbCurseTargetEmail != NULL && usePersonalCurses ) {
        LiveObject *targetP = getPlayerByEmail( dbCurseTargetEmail );
        
        if( targetP != NULL ) {
            char *message = autoSprintf( "CU\n%d 1 %s_%s\n#", targetP->id,
                                         getCurseWord( inPlayer->email,
                                                       targetP->email, 0 ),
                                         getCurseWord( inPlayer->email,
                                                       targetP->email, 1 ) );
            sendMessageToPlayer( inPlayer,
                                 message, strlen( message ) );
            delete [] message;
            }
        }
    


    if( isCurse ) {
        if( inPlayer->curseStatus.curseLevel == 0 &&
            hasCurseToken( inPlayer->email ) ) {
            inPlayer->curseTokenCount = 1;
            }
        else {
            inPlayer->curseTokenCount = 0;
            }
        inPlayer->curseTokenUpdate = true;
        }

    

    int curseFlag = 0;
    if( isCurse ) {
        curseFlag = 1;
        }
    

    newSpeechPhrases.push_back( stringDuplicate( inToSay ) );
    newSpeechCurseFlags.push_back( curseFlag );
    newSpeechPlayerIDs.push_back( inPlayer->id );
	
    //2HOL additions for: password-protected objects
    //  newSpeechPasswordFlags added to communicate with an algorithm
    //  which decides whether send the info about something was said
    //  to neighboring players or not
    int passwordFlag = 0;
    if ( sayingPassword || assigningPassword ) { passwordFlag = 1; }
    newSpeechPasswordFlags.push_back( passwordFlag );

                        
    ChangePosition p = { inPlayer->xd, inPlayer->yd, false, -1 };
	if( inPrivate ) p.responsiblePlayerID = inPlayer->id;
                        
    // if held, speech happens where held
    if( inPlayer->heldByOther ) {
        LiveObject *holdingPlayer = 
            getLiveObject( inPlayer->heldByOtherID );
                
        if( holdingPlayer != NULL ) {
            p.x = holdingPlayer->xd;
            p.y = holdingPlayer->yd;
            }
        }

    newSpeechPos.push_back( p );
	if( inPrivate ) return;


    SimpleVector<int> pipesIn;
    GridPos playerPos = getPlayerPos( inPlayer );
    
    
    if( inPlayer->heldByOther ) {    
        LiveObject *holdingPlayer = 
            getLiveObject( inPlayer->heldByOtherID );
                
        if( holdingPlayer != NULL ) {
            playerPos = getPlayerPos( holdingPlayer );
            }
        }
    
    getSpeechPipesIn( playerPos.x, playerPos.y, &pipesIn );
    
    if( pipesIn.size() > 0 ) {
        for( int p=0; p<pipesIn.size(); p++ ) {
            int pipeIndex = pipesIn.getElementDirect( p );

            SimpleVector<GridPos> *pipesOut = getSpeechPipesOut( pipeIndex );

            for( int i=0; i<pipesOut->size(); i++ ) {
                GridPos outPos = pipesOut->getElementDirect( i );
                
                newLocationSpeech.push_back( stringDuplicate( inToSay ) );
                
                ChangePosition outChangePos = { outPos.x, outPos.y, false, -1 };
                newLocationSpeechPos.push_back( outChangePos );
                }
            }
        }
    }


static void forcePlayerToRead( LiveObject *inPlayer,
                               int inObjectID ) {
            
    char metaData[ MAP_METADATA_LENGTH ];
    char found = getMetadata( inObjectID, 
                              (unsigned char*)metaData );

    if( found ) {
        // read what they picked up, subject to limit
                
        unsigned int sayLimit = getSayLimit( inPlayer );
        
        if( computeAge( inPlayer ) < 10 &&
            strlen( metaData ) > sayLimit ) {
            // truncate with ...
            metaData[ sayLimit ] = '.';
            metaData[ sayLimit + 1 ] = '.';
            metaData[ sayLimit + 2 ] = '.';
            metaData[ sayLimit + 3 ] = '\0';
            
            // watch for truncated map metadata
            // trim it off (too young to read maps)
            char *starLoc = strstr( metaData, " *" );
            
            if( starLoc != NULL ) {
                starLoc[0] = '\0';
                }
            }
        char *quotedPhrase = autoSprintf( ":%s", metaData );
        makePlayerSay( inPlayer, quotedPhrase );
        delete [] quotedPhrase;
        }
    }
	
//2HOL mechanics to read written objects
static void forceObjectToRead( LiveObject *inPlayer,
                               int inObjectID,
							   GridPos inReadPos,
							   bool passToRead ) {

	//avoid spamming location speech
	//different behavior for clickToRead and/or passToRead objects
	for( int j = 0; j < inPlayer->readPositions.size(); j++ ) {
		GridPos p = inPlayer->readPositions.getElementDirect( j );
		double eta = inPlayer->readPositionsETA.getElementDirect( j );
		
		if( !passToRead )
		if( p.x == inReadPos.x && p.y == inReadPos.y && Time::getCurrentTime() <= eta ){
			return;
		}
		
		if( passToRead )
		if( p.x == inReadPos.x && p.y == inReadPos.y ){
			return;
		}
	}

    char metaData[ MAP_METADATA_LENGTH ];
    char found = getMetadata( inObjectID, 
                              (unsigned char*)metaData );

    if( found ) {
		//speech limit is ignored here
        char *quotedPhrase = autoSprintf( ":%s", metaData );
		
		ChangePosition cp;
		cp.x = inReadPos.x;
		cp.y = inReadPos.y;
		cp.global = false;
		cp.responsiblePlayerID = inPlayer->id;

		newLocationSpeechPos.push_back( cp );
		newLocationSpeech.push_back( 
			stringDuplicate( quotedPhrase ) );
		
		//longer time for longer speech
		//roughly matching but slightly longer than client speech bubbles duration
		double speechETA = Time::getCurrentTime() + 3.25 + strlen( quotedPhrase ) / 5;
		inPlayer->readPositions.push_back( inReadPos );
		inPlayer->readPositionsETA.push_back( speechETA );
		
        delete [] quotedPhrase;
        }
    }

static void holdingSomethingNew( LiveObject *inPlayer, 
                                 int inOldHoldingID = 0 ) {
    if( inPlayer->holdingID > 0 ) {
       
        ObjectRecord *o = getObject( inPlayer->holdingID );
        
        ObjectRecord *oldO = NULL;
        if( inOldHoldingID > 0 ) {
            oldO = getObject( inOldHoldingID );
            }
        
        if( o->written &&
            ( oldO == NULL ||
              ! ( oldO->written || oldO->writable ) ) ) {

            forcePlayerToRead( inPlayer, inPlayer->holdingID );
            }

        if( o->isFlying ) {
            inPlayer->holdingFlightObject = true;
            }
        else {
            inPlayer->holdingFlightObject = false;
            }
        }
    else {
        inPlayer->holdingFlightObject = false;
        }
    }




static SimpleVector<GraveInfo> newGraves;
static SimpleVector<GraveMoveInfo> newGraveMoves;



static int isGraveSwapDest( int inTargetX, int inTargetY,
                            int inDroppingPlayerID ) {
    
    for( int i=0; i<players.size(); i++ ) {
        LiveObject *o = players.getElement( i );
        
        if( o->error || o->id == inDroppingPlayerID ) {
            continue;
            }
        
        if( o->holdingID > 0 && strstr( getObject( o->holdingID )->description,
                                        "origGrave" ) != NULL ) {
            
            if( inTargetX == o->heldGraveOriginX &&
                inTargetY == o->heldGraveOriginY ) {
                return true;
                }
            }
        }
    
    return false;
    }



// drops an object held by a player at target x,y location
// doesn't check for adjacency (so works for thrown drops too)
// if target spot blocked, will search for empty spot to throw object into
// if inPlayerIndicesToSendUpdatesAbout is NULL, it is ignored
void handleDrop( int inX, int inY, LiveObject *inDroppingPlayer,
                 SimpleVector<int> *inPlayerIndicesToSendUpdatesAbout ) {
    
    int oldHoldingID = inDroppingPlayer->holdingID;
    

    if( oldHoldingID > 0 &&
        getObject( oldHoldingID )->permanent ) {
        // what they are holding is stuck in their
        // hand

        // see if a use-on-bare-ground drop 
        // action applies (example:  dismounting
        // a horse)
                            
        // note that if use on bare ground
        // also has a new actor, that will be lost
        // in this process.
        // (example:  holding a roped lamb when dying,
        //            lamb is dropped, rope is lost)

        TransRecord *bareTrans =
            getPTrans( oldHoldingID, -1 );
                            
        if( bareTrans != NULL &&
            bareTrans->newTarget > 0 ) {
                            
            oldHoldingID = bareTrans->newTarget;
            
            inDroppingPlayer->holdingID = 
                bareTrans->newTarget;
            holdingSomethingNew( inDroppingPlayer, oldHoldingID );

            setFreshEtaDecayForHeld( inDroppingPlayer );
            }
        }
    else if( oldHoldingID > 0 &&
             ! getObject( oldHoldingID )->permanent ) {
        // what they are holding is NOT stuck in their
        // hand

        // see if a use-on-bare-ground drop 
        // action applies (example:  getting wounded while holding a goose)
                            
        // do not consider doing this if use-on-bare-ground leaves something
        // in the hand

        TransRecord *bareTrans =
            getPTrans( oldHoldingID, -1 );
                            
        if( bareTrans != NULL &&
            bareTrans->newTarget > 0 &&
            bareTrans->newActor == 0 ) {
                            
            oldHoldingID = bareTrans->newTarget;
            
            inDroppingPlayer->holdingID = 
                bareTrans->newTarget;
            holdingSomethingNew( inDroppingPlayer, oldHoldingID );

            setFreshEtaDecayForHeld( inDroppingPlayer );
            }
        }

    int targetX = inX;
    int targetY = inY;

    int mapID = getMapObject( inX, inY );
    char mapSpotBlocking = false;
    if( mapID > 0 ) {
        mapSpotBlocking = getObject( mapID )->blocksWalking;
        }
    

    if( ( inDroppingPlayer->holdingID < 0 && mapSpotBlocking )
        ||
        ( inDroppingPlayer->holdingID > 0 && mapID != 0 ) ) {
        
        // drop spot blocked
        // search for another
        // throw held into nearest empty spot
                                    
        
        GridPos spot;

        GridPos playerPos = getPlayerPos( inDroppingPlayer );
        
        char found = findDropSpot( inX, inY, 
                                   playerPos.x, playerPos.y,
                                   &spot );
        
        int foundX = spot.x;
        int foundY = spot.y;



        if( found && inDroppingPlayer->holdingID > 0 ) {
            targetX = foundX;
            targetY = foundY;
            }
        else {
            // no place to drop it, it disappears

            // OR we're holding a baby,
            // then just put the baby where we are
            // (don't ever throw babies, that's weird and exploitable)
            if( inDroppingPlayer->holdingID < 0 ) {
                int babyID = - inDroppingPlayer->holdingID;
                
                LiveObject *babyO = getLiveObject( babyID );
                
                if( babyO != NULL ) {
                    babyO->xd = inDroppingPlayer->xd;
                    babyO->xs = inDroppingPlayer->xd;
                    
                    babyO->yd = inDroppingPlayer->yd;
                    babyO->ys = inDroppingPlayer->yd;

                    babyO->heldByOther = false;

                    if( isFertileAge( inDroppingPlayer ) ) {    
                        // reset food decrement time
                        babyO->foodDecrementETASeconds =
                            Time::getCurrentTime() +
                            computeFoodDecrementTimeSeconds( babyO );
                        }
                    
                    if( inPlayerIndicesToSendUpdatesAbout != NULL ) {    
                        inPlayerIndicesToSendUpdatesAbout->push_back( 
                            getLiveObjectIndex( babyID ) );
                        }
                    
                    }
                
                }
            
            inDroppingPlayer->holdingID = 0;
            inDroppingPlayer->holdingEtaDecay = 0;
            inDroppingPlayer->heldOriginValid = 0;
            inDroppingPlayer->heldOriginX = 0;
            inDroppingPlayer->heldOriginY = 0;
            inDroppingPlayer->heldTransitionSourceID = -1;
            
            if( inDroppingPlayer->numContained != 0 ) {
                clearPlayerHeldContained( inDroppingPlayer );
                }
            return;
            }            
        }
    
    
    if( inDroppingPlayer->holdingID < 0 ) {
        // dropping a baby
        
        int babyID = - inDroppingPlayer->holdingID;
                
        LiveObject *babyO = getLiveObject( babyID );
        
        if( babyO != NULL ) {
            babyO->xd = targetX;
            babyO->xs = targetX;
                    
            babyO->yd = targetY;
            babyO->ys = targetY;
            
            babyO->heldByOther = false;
            
            // force baby pos
            // baby can wriggle out of arms in same server step that it was
            // picked up.  In that case, the clients will never get the
            // message that the baby was picked up.  The baby client could
            // be in the middle of a client-side move, and we need to force
            // them back to their true position.
            babyO->posForced = true;
            
            if( isFertileAge( inDroppingPlayer ) ) {    
                // reset food decrement time
                babyO->foodDecrementETASeconds =
                    Time::getCurrentTime() +
                    computeFoodDecrementTimeSeconds( babyO );
                }

            if( inPlayerIndicesToSendUpdatesAbout != NULL ) {
                inPlayerIndicesToSendUpdatesAbout->push_back( 
                    getLiveObjectIndex( babyID ) );
                }
            }
        
        inDroppingPlayer->holdingID = 0;
        inDroppingPlayer->holdingEtaDecay = 0;
        inDroppingPlayer->heldOriginValid = 0;
        inDroppingPlayer->heldOriginX = 0;
        inDroppingPlayer->heldOriginY = 0;
        inDroppingPlayer->heldTransitionSourceID = -1;
        
        return;
        }
    
    setResponsiblePlayer( inDroppingPlayer->id );
    
    ObjectRecord *o = getObject( inDroppingPlayer->holdingID );
                                
    if( strstr( o->description, "origGrave" ) 
        != NULL ) {
                                    
        setGravePlayerID( 
            targetX, targetY, inDroppingPlayer->heldGravePlayerID );
        
        int swapDest = isGraveSwapDest( targetX, targetY, 
                                        inDroppingPlayer->id );
        
        // see if another player has target location in air


        GraveMoveInfo g = { 
            { inDroppingPlayer->heldGraveOriginX,
              inDroppingPlayer->heldGraveOriginY },
            { targetX,
              targetY },
            swapDest };
        newGraveMoves.push_back( g );
        }


    setMapObject( targetX, targetY, inDroppingPlayer->holdingID );
    setEtaDecay( targetX, targetY, inDroppingPlayer->holdingEtaDecay );

    transferHeldContainedToMap( inDroppingPlayer, targetX, targetY );
    
                                

    setResponsiblePlayer( -1 );
                                
    inDroppingPlayer->holdingID = 0;
    inDroppingPlayer->holdingEtaDecay = 0;
    inDroppingPlayer->heldOriginValid = 0;
    inDroppingPlayer->heldOriginX = 0;
    inDroppingPlayer->heldOriginY = 0;
    inDroppingPlayer->heldTransitionSourceID = -1;
                                
    // watch out for truncations of in-progress
    // moves of other players
            
    ObjectRecord *droppedObject = getObject( oldHoldingID );
   
    if( inPlayerIndicesToSendUpdatesAbout != NULL ) {    
        handleMapChangeToPaths( targetX, targetY, droppedObject,
                                inPlayerIndicesToSendUpdatesAbout );
        }
    
    
    }



LiveObject *getAdultHolding( LiveObject *inBabyObject ) {
    int numLive = players.size();
    
    for( int j=0; j<numLive; j++ ) {
        LiveObject *adultO = players.getElement( j );

        if( - adultO->holdingID == inBabyObject->id ) {
            return adultO;
            }
        }
    return NULL;
    }



void handleForcedBabyDrop( 
    LiveObject *inBabyObject,
    SimpleVector<int> *inPlayerIndicesToSendUpdatesAbout ) {
    
    int numLive = players.size();
    
    for( int j=0; j<numLive; j++ ) {
        LiveObject *adultO = players.getElement( j );

        if( - adultO->holdingID == inBabyObject->id ) {

            // don't need to send update about this adult's
            // holding status.
            // the update sent about the baby will inform clients
            // that the baby is no longer held by this adult
            //inPlayerIndicesToSendUpdatesAbout->push_back( j );
            
            GridPos dropPos;
            
            if( adultO->xd == 
                adultO->xs &&
                adultO->yd ==
                adultO->ys ) {
                
                dropPos.x = adultO->xd;
                dropPos.y = adultO->yd;
                }
            else {
                dropPos = 
                    computePartialMoveSpot( adultO );
                }
            
            
            handleDrop( 
                dropPos.x, dropPos.y, 
                adultO,
                inPlayerIndicesToSendUpdatesAbout );

            
            break;
            }
        }
    }



static void handleHoldingChange( LiveObject *inPlayer, int inNewHeldID );



static void swapHeldWithGround( 
    LiveObject *inPlayer, int inTargetID, 
    int inMapX, int inMapY,
    SimpleVector<int> *inPlayerIndicesToSendUpdatesAbout) {
    
    
    timeSec_t newHoldingEtaDecay = getEtaDecay( inMapX, inMapY );
    
    FullMapContained f = getFullMapContained( inMapX, inMapY );
    
    
    
    clearAllContained( inMapX, inMapY );
    setMapObject( inMapX, inMapY, 0 );
    
    handleDrop( inMapX, inMapY, inPlayer, inPlayerIndicesToSendUpdatesAbout );
    
    
    inPlayer->holdingID = inTargetID;
    inPlayer->holdingEtaDecay = newHoldingEtaDecay;
    
    setContained( inPlayer, f );


    // does bare-hand action apply to this newly-held object
    // one that results in something new in the hand and
    // nothing on the ground?
    
    // if so, it is a pick-up action, and it should apply here
    
    TransRecord *pickupTrans = getPTrans( 0, inTargetID );

    char newHandled = false;
                
    if( pickupTrans != NULL && pickupTrans->newActor > 0 &&
        pickupTrans->newTarget == 0 ) {
                    
        int newTargetID = pickupTrans->newActor;
        
        if( newTargetID != inTargetID ) {
            handleHoldingChange( inPlayer, newTargetID );
            newHandled = true;
            }
        }
    
    if( ! newHandled ) {
        holdingSomethingNew( inPlayer );
        }
    
    inPlayer->heldOriginValid = 1;
    inPlayer->heldOriginX = inMapX;
    inPlayer->heldOriginY = inMapY;
    inPlayer->heldTransitionSourceID = -1;
    }









// returns 0 for NULL
static int objectRecordToID( ObjectRecord *inRecord ) {
    if( inRecord == NULL ) {
        return 0;
        }
    else {
        return inRecord->id;
        }
    }



typedef struct UpdateRecord{
        char *formatString;
        char posUsed;
        int absolutePosX, absolutePosY;
        GridPos absoluteActionTarget;
        int absoluteHeldOriginX, absoluteHeldOriginY;
    } UpdateRecord;



static char *getUpdateLineFromRecord( 
    UpdateRecord *inRecord, GridPos inRelativeToPos, GridPos inObserverPos ) {
    
    if( inRecord->posUsed ) {
        
        GridPos updatePos = { inRecord->absolutePosX, inRecord->absolutePosY };
        
        if( distance( updatePos, inObserverPos ) > 
            getMaxChunkDimension() * 2 ) {
            
            // this update is for a far-away player
            
            // put dummy positions in to hide their coordinates
            // so that people sniffing the protocol can't get relative
            // location information
            
            return autoSprintf( inRecord->formatString,
                                1977, 1977,
                                1977, 1977,
                                1977, 1977 );
            }


        return autoSprintf( inRecord->formatString,
                            inRecord->absoluteActionTarget.x 
                            - inRelativeToPos.x,
                            inRecord->absoluteActionTarget.y 
                            - inRelativeToPos.y,
                            inRecord->absoluteHeldOriginX - inRelativeToPos.x, 
                            inRecord->absoluteHeldOriginY - inRelativeToPos.y,
                            inRecord->absolutePosX - inRelativeToPos.x, 
                            inRecord->absolutePosY - inRelativeToPos.y );
        }
    else {
        // posUsed false only if thise is a DELETE PU message
        // set all positions to 0 in that case
        return autoSprintf( inRecord->formatString,
                            0, 0,
                            0, 0 );
        }
    }



static SimpleVector<int> newEmotPlayerIDs;
static SimpleVector<int> newEmotIndices;
// 0 if no ttl specified
static SimpleVector<int> newEmotTTLs;



static char isYummy( LiveObject *inPlayer, int inObjectID ) {
    ObjectRecord *o = getObject( inObjectID );
    
    if( o->isUseDummy ) {
        inObjectID = o->useDummyParent;
        o = getObject( inObjectID );
        }

    if( o->foodValue == 0 ) {
        return false;
        }

    if( inObjectID == inPlayer->cravingFood.foodID &&
        computeAge( inPlayer ) >= minAgeForCravings ) {
        return true;
        }

    for( int i=0; i<inPlayer->yummyFoodChain.size(); i++ ) {
        if( inObjectID == inPlayer->yummyFoodChain.getElementDirect(i) ) {
            return false;
            }
        }
    return true;
    }
    
static char isReallyYummy( LiveObject *inPlayer, int inObjectID ) {
    
    // whether the food is actually not in the yum chain
    // return false for meh food that the player is craving
    // which is displayed "yum" client-side
    
    ObjectRecord *o = getObject( inObjectID );
    
    if( o->isUseDummy ) {
        inObjectID = o->useDummyParent;
        o = getObject( inObjectID );
        }

    if( o->foodValue == 0 ) {
        return false;
        }

    for( int i=0; i<inPlayer->yummyFoodChain.size(); i++ ) {
        if( inObjectID == inPlayer->yummyFoodChain.getElementDirect(i) ) {
            return false;
            }
        }
    return true;
    }



static void updateYum( LiveObject *inPlayer, int inFoodEatenID,
                       char inFedSelf = true ) {

    char wasYummy = true;
    
    if( ! isYummy( inPlayer, inFoodEatenID ) ) {
        wasYummy = false;
        
        // chain broken
        
        // only feeding self can break chain
        if( inFedSelf && canYumChainBreak ) {
            inPlayer->yummyFoodChain.deleteAll();
            }
        }
    
    
    ObjectRecord *o = getObject( inFoodEatenID );
    
    if( o->isUseDummy ) {
        inFoodEatenID = o->useDummyParent;
        }
    
    
    // add to chain
    // might be starting a new chain
    // (do this if fed yummy food by other player too)
    if( wasYummy ||
        inPlayer->yummyFoodChain.size() == 0 ) {
        
        int eatenID = inFoodEatenID;
        
        if( isReallyYummy( inPlayer, eatenID ) ) {
            inPlayer->yummyFoodChain.push_back( eatenID );
            }
		
        // now it is possible to "grief" the craving pool
        // by eating high tech food without craving them
        // but this also means that it requires more effort to
        // cheese the craving system by deliberately eating
        // easy food first in an advanced town
        logFoodDepth( inPlayer->lineageEveID, eatenID );
        
        if( eatenID == inPlayer->cravingFood.foodID &&
            computeAge( inPlayer ) >= minAgeForCravings ) {
            
            for( int i=0; i< inPlayer->cravingFood.bonus; i++ ) {
                // add extra copies to YUM chain as a bonus
                inPlayer->yummyFoodChain.push_back( eatenID );
                }
            
            // craving satisfied, go on to next thing in list
            inPlayer->cravingFood = 
                getCravedFood( inPlayer->lineageEveID,
                               inPlayer->parentChainLength,
                               inPlayer->cravingFood );
            // reset generational bonus counter
            inPlayer->cravingFoodYumIncrement = 1;
            
            // flag them for getting a new craving message
            inPlayer->cravingKnown = false;
            
            // satisfied emot
            
            if( satisfiedEmotionIndex != -1 ) {
                inPlayer->emotFrozen = false;
                inPlayer->emotUnfreezeETA = 0;
        
                newEmotPlayerIDs.push_back( inPlayer->id );
                
                newEmotIndices.push_back( satisfiedEmotionIndex );
                // 3 sec
                newEmotTTLs.push_back( 1 );
                
                // don't leave starving status, or else non-starving
                // change might override our satisfied emote
                inPlayer->starving = false;
                }
            }
        }
    

    int currentBonus = inPlayer->yummyFoodChain.size() - 1;

    if( currentBonus < 0 ) {
        currentBonus = 0;
        }    

    if( wasYummy ) {
        // only get bonus if actually was yummy (whether fed self or not)
        // chain not broken if fed non-yummy by other, but don't get bonus
        inPlayer->yummyBonusStore += currentBonus;
        }
    
    }





static UpdateRecord getUpdateRecord( 
    LiveObject *inPlayer,
    char inDelete,
    char inPartial = false ) {

    char *holdingString = getHoldingString( inPlayer );
    
    // this is 0 if still in motion (mid-move update)
    int doneMoving = 0;
    
    if( inPlayer->xs == inPlayer->xd &&
        inPlayer->ys == inPlayer->yd &&
        ! inPlayer->heldByOther ) {
        // not moving
        doneMoving = inPlayer->lastMoveSequenceNumber;
        }
    
    char midMove = false;
    
    if( inPartial || 
        inPlayer->xs != inPlayer->xd ||
        inPlayer->ys != inPlayer->yd ) {
        
        midMove = true;
        }
    

    UpdateRecord r;
        

    char *posString;
    if( inDelete ) {
        posString = stringDuplicate( "0 0 X X" );
        r.posUsed = false;
        }
    else {
        int x, y;

        r.posUsed = true;

        if( doneMoving > 0 || ! midMove ) {
            x = inPlayer->xs;
            y = inPlayer->ys;
            }
        else {
            // mid-move, and partial position requested
            GridPos p = computePartialMoveSpot( inPlayer );
            
            x = p.x;
            y = p.y;
            }
        
        posString = autoSprintf( "%d %d %%d %%d",          
                                 doneMoving,
                                 inPlayer->posForced );
        r.absolutePosX = x;
        r.absolutePosY = y;
        }
    
    SimpleVector<char> clothingListBuffer;
    
    for( int c=0; c<NUM_CLOTHING_PIECES; c++ ) {
        ObjectRecord *cObj = clothingByIndex( inPlayer->clothing, c );
        int id = 0;
        
        if( cObj != NULL ) {
            id = objectRecordToID( cObj );
            }
        
        char *idString = autoSprintf( "%d", hideIDForClient( id ) );
        
        clothingListBuffer.appendElementString( idString );
        delete [] idString;
        
        if( cObj != NULL && cObj->numSlots > 0 ) {
            
            for( int cc=0; cc<inPlayer->clothingContained[c].size(); cc++ ) {
                char *contString = 
                    autoSprintf( 
                        ",%d", 
                        hideIDForClient( 
                            inPlayer->
                            clothingContained[c].getElementDirect( cc ) ) );
                
                clothingListBuffer.appendElementString( contString );
                delete [] contString;
                }
            }

        if( c < NUM_CLOTHING_PIECES - 1 ) {
            clothingListBuffer.push_back( ';' );
            }
        }
    
    char *clothingList = clothingListBuffer.getElementString();


    char *deathReason;
    
    if( inDelete && inPlayer->deathReason != NULL ) {
        deathReason = stringDuplicate( inPlayer->deathReason );
        }
    else {
        deathReason = stringDuplicate( "" );
        }
    
    
    int heldYum = 0;
    
    if( inPlayer->holdingID > 0 &&
        isYummy( inPlayer, inPlayer->holdingID ) ) {
        heldYum = 1;
        }


    r.formatString = autoSprintf( 
        "%d %d %d %d %%d %%d %s %d %%d %%d %d "
        "%.2f %s %.2f %.2f %.2f %s %d %d %d %d%s\n",
        inPlayer->id,
        inPlayer->displayID,
        inPlayer->facingOverride,
        inPlayer->actionAttempt,
        //inPlayer->actionTarget.x - inRelativeToPos.x,
        //inPlayer->actionTarget.y - inRelativeToPos.y,
        holdingString,
        inPlayer->heldOriginValid,
        //inPlayer->heldOriginX - inRelativeToPos.x,
        //inPlayer->heldOriginY - inRelativeToPos.y,
        hideIDForClient( inPlayer->heldTransitionSourceID ),
        inPlayer->heat,
        posString,
        computeAge( inPlayer ),
        1.0 / getAgeRate(),
        computeMoveSpeed( inPlayer ),
        clothingList,
        inPlayer->justAte,
        hideIDForClient( inPlayer->justAteID ),
        inPlayer->responsiblePlayerID,
        heldYum,
        deathReason );
    
    delete [] deathReason;
    

    r.absoluteActionTarget = inPlayer->actionTarget;
    
    if( inPlayer->heldOriginValid ) {
        r.absoluteHeldOriginX = inPlayer->heldOriginX;
        r.absoluteHeldOriginY = inPlayer->heldOriginY;
        }
    else {
        // we set 0,0 to clear held origins in many places in the code
        // if we leave that as an absolute pos, our birth pos leaks through
        // when we make it birth-pos relative
        
        // instead, substitute our birth pos for all invalid held pos coords
        // to prevent this
        r.absoluteHeldOriginX = inPlayer->birthPos.x;
        r.absoluteHeldOriginY = inPlayer->birthPos.y;
        }
    
        

    inPlayer->justAte = false;
    inPlayer->justAteID = 0;
    
    // held origin only valid once
    inPlayer->heldOriginValid = 0;
    
    inPlayer->facingOverride = 0;
    inPlayer->actionAttempt = 0;

    delete [] holdingString;
    delete [] posString;
    delete [] clothingList;
    
    return r;
    }



// inDelete true to send X X for position
// inPartial gets update line for player's current possition mid-path
// positions in update line will be relative to inRelativeToPos
static char *getUpdateLine( LiveObject *inPlayer, GridPos inRelativeToPos,
                            GridPos inObserverPos,
                            char inDelete,
                            char inPartial = false ) {
    
    UpdateRecord r = getUpdateRecord( inPlayer, inDelete, inPartial );
    
    char *line = getUpdateLineFromRecord( &r, inRelativeToPos, inObserverPos );

    delete [] r.formatString;
    
    return line;
    }




// if inTargetID set, we only detect whether inTargetID is close enough to
// be hit
// otherwise, we find the lowest-id player that is hit and return that
static LiveObject *getHitPlayer( int inX, int inY,
                                 int inTargetID = -1,
                                 char inCountMidPath = false,
                                 int inMaxAge = -1,
                                 int inMinAge = -1,
                                 int *outHitIndex = NULL ) {
    GridPos targetPos = { inX, inY };

    int numLive = players.size();
                                    
    LiveObject *hitPlayer = NULL;
                                    
    for( int j=0; j<numLive; j++ ) {
        LiveObject *otherPlayer = 
            players.getElement( j );
        
        if( otherPlayer->error ) {
            continue;
            }
        
        if( otherPlayer->heldByOther ) {
            // ghost position of a held baby
            continue;
            }
        
        if( inMaxAge != -1 &&
            computeAge( otherPlayer ) > inMaxAge ) {
            continue;
            }

        if( inMinAge != -1 &&
            computeAge( otherPlayer ) < inMinAge ) {
            continue;
            }
        
        if( inTargetID != -1 &&
            otherPlayer->id != inTargetID ) {
            continue;
            }

        if( otherPlayer->xd == 
            otherPlayer->xs &&
            otherPlayer->yd ==
            otherPlayer->ys ) {
            // other player standing still
                                            
            if( otherPlayer->xd ==
                inX &&
                otherPlayer->yd ==
                inY ) {
                                                
                // hit
                hitPlayer = otherPlayer;
                if( outHitIndex != NULL ) {
                    *outHitIndex = j;
                    }
                break;
                }
            }
        else {
            // other player moving
                
            GridPos cPos = 
                computePartialMoveSpot( 
                    otherPlayer );
                                        
            if( equal( cPos, targetPos ) ) {
                // hit
                hitPlayer = otherPlayer;
                if( outHitIndex != NULL ) {
                    *outHitIndex = j;
                    }
                break;
                }
            else if( inCountMidPath ) {
                
                int c = computePartialMovePathStep( otherPlayer );

                // consider path step before and after current location
                for( int i=-1; i<=1; i++ ) {
                    int testC = c + i;
                    
                    if( testC >= 0 && testC < otherPlayer->pathLength ) {
                        cPos = otherPlayer->pathToDest[testC];
                 
                        if( equal( cPos, targetPos ) ) {
                            // hit
                            hitPlayer = otherPlayer;
                            if( outHitIndex != NULL ) {
                                *outHitIndex = j;
                                }
                            break;
                            }
                        }
                    }
                if( hitPlayer != NULL ) {
                    break;
                    }
                }
            }
        }

    return hitPlayer;
    }




static int countFertileMothers() {
    
    int barrierRadius = 
        SettingsManager::getIntSetting( 
            "barrierRadius", 250 );
    int barrierOn = SettingsManager::getIntSetting( 
        "barrierOn", 1 );
    
    int c = 0;
    
    for( int i=0; i<players.size(); i++ ) {
        LiveObject *p = players.getElement( i );
        
        if( p->error ) {
            continue;
            }
        
        if( isFertileAge( p ) ) {
            if( barrierOn ) {
                // only fertile mothers inside the barrier
                GridPos pos = getPlayerPos( p );
                
                if( abs( pos.x ) < barrierRadius &&
                    abs( pos.y ) < barrierRadius ) {
                    c++;
                    }
                }
            else {
                c++;
                }
            }
        }
    
    return c;
    }



static int countHelplessBabies() {
    
    int barrierRadius = 
        SettingsManager::getIntSetting( 
            "barrierRadius", 250 );
    int barrierOn = SettingsManager::getIntSetting( 
        "barrierOn", 1 );
    
    int c = 0;
    
    for( int i=0; i<players.size(); i++ ) {
        LiveObject *p = players.getElement( i );
        
        if( p->error ) {
            continue;
            }

        if( computeAge( p ) < defaultActionAge ) {
            if( barrierOn ) {
                // only babies inside the barrier
                GridPos pos = getPlayerPos( p );
                
                if( abs( pos.x ) < barrierRadius &&
                    abs( pos.y ) < barrierRadius ) {
                    c++;
                    }
                }
            else {
                c++;
                }
            }
        }
    
    return c;
    }




static int countFamilies() {
    
    int barrierRadius = 
        SettingsManager::getIntSetting( 
            "barrierRadius", 250 );
    int barrierOn = SettingsManager::getIntSetting( 
        "barrierOn", 1 );
    
    SimpleVector<int> uniqueLines;

    
    for( int i=0; i<players.size(); i++ ) {
        LiveObject *p = players.getElement( i );
        
        if( p->error ) {
            continue;
            }
        if( p->isTutorial ) {
            continue;
            }    
        if( p->vogMode ) {
            continue;
            }
        if( p->curseStatus.curseLevel > 0 ) {
            continue;
            }

        int lineageEveID = p->lineageEveID;
            
        if( uniqueLines.getElementIndex( lineageEveID ) == -1 ) {
            
            if( barrierOn ) {
                // only those inside the barrier
                GridPos pos = getPlayerPos( p );
                
                if( abs( pos.x ) < barrierRadius &&
                    abs( pos.y ) < barrierRadius ) {
                    uniqueLines.push_back( lineageEveID );
                    }
                }
            else {
                uniqueLines.push_back( lineageEveID );
                }
            }
        }
    
    return uniqueLines.size();
    }



static char isEveWindow() {
    
    if( players.size() <=
        SettingsManager::getIntSetting( "minActivePlayersForEveWindow", 15 ) ) {
        // not enough players
        // always Eve window
        
        // new window starts if we ever get enough players again
        eveWindowStart = 0;
        
        return true;
        }

    if( eveWindowStart == 0 ) {
        // start window now
        eveWindowStart = Time::getCurrentTime();
        return true;
        }
    else {
        double secSinceStart = Time::getCurrentTime() - eveWindowStart;
        
        if( secSinceStart >
            SettingsManager::getIntSetting( "eveWindowSeconds", 3600 ) ) {
            return false;
            }
        return true;
        }
    }



static void triggerApocalypseNow() {
    apocalypseTriggered = true;
    
    // restart Eve window, and let this player be the
    // first new Eve
    eveWindowStart = 0;
    
    // reset other apocalypse trigger
    lastBabyPassedThresholdTime = 0;
    }



static int countLivingChildren( int inMotherID ) {
    int count = 0;
    
    for( int i=0; i<players.size(); i++ ) {
        LiveObject *o = players.getElement( i );
        
        if( o->parentID == inMotherID && ! o->error ) {
            count ++;
            }
        }
    return count;
    }




// for placement of tutorials out of the way 
static int maxPlacementX = 5000000;

// tutorial is alwasy placed 400,000 to East of furthest birth/Eve
// location
static int tutorialOffsetX = 400000;


// each subsequent tutorial gets put in a diferent place
static int tutorialCount = 0;



// fill this with emails that should also affect lineage ban
// if any twin in group is banned, all should be
static SimpleVector<char*> tempTwinEmails;

static char nextLogInTwin = false;

static int firstTwinID = -1;


// returns ID of new player,
// or -1 if this player reconnected to an existing ID
int processLoggedInPlayer( char inAllowReconnect,
                           Socket *inSock,
                           SimpleVector<char> *inSockBuffer,
                           char *inEmail,
                           uint32_t hashedSpawnSeed,
                           int inTutorialNumber,
                           CurseStatus inCurseStatus,
						   float inFitnessScore,
                           // set to -2 to force Eve
                           int inForceParentID = -1,
                           int inForceDisplayID = -1,
                           GridPos *inForcePlayerPos = NULL ) {
    

    usePersonalCurses = SettingsManager::getIntSetting( "usePersonalCurses",
                                                        0 );
    
    if( usePersonalCurses ) {
        // ignore what old curse system said
        inCurseStatus.curseLevel = 0;
        inCurseStatus.excessPoints = 0;
        
        initPersonalCurseTest( inEmail );
        
        for( int p=0; p<players.size(); p++ ) {
            LiveObject *o = players.getElement( p );
        
            if( ! o->error && 
                ! o->isTutorial &&
                o->curseStatus.curseLevel == 0 &&
                strcmp( o->email, inEmail ) != 0 ) {

                // non-tutorial, non-cursed, non-us player
                addPersonToPersonalCurseTest( o->email, inEmail,
                                              getPlayerPos( o ) );
                }
            }
        }
    


    // new behavior:
    // allow this new connection from same
    // email (most likely a re-connect
    // by same person, when the old connection
    // hasn't broken on our end yet)
    
    // to make it work, force-mark
    // the old connection as broken
    for( int p=0; p<players.size(); p++ ) {
        LiveObject *o = players.getElement( p );
        
        if( ! o->error && 
            o->connected && 
            strcmp( o->email, inEmail ) == 0 ) {
            
            setPlayerDisconnected( o, "Authentic reconnect received" );
            
            break;
            }
        }



    // see if player was previously disconnected
    for( int i=0; i<players.size(); i++ ) {
        LiveObject *o = players.getElement( i );
        
        if( ! o->error && ! o->connected &&
            strcmp( o->email, inEmail ) == 0 ) {

            if( ! inAllowReconnect ) {
                // trigger an error for them, so they die and are removed
                o->error = true;
                o->errorCauseString = "Reconnected as twin";
                break;
                }
            
            // else allow them to reconnect to existing life

            // give them this new socket and buffer
            if( o->sock != NULL ) {
                delete o->sock;
                o->sock = NULL;
                }
            if( o->sockBuffer != NULL ) {
                delete o->sockBuffer;
                o->sockBuffer = NULL;
                }
            
            o->sock = inSock;
            o->sockBuffer = inSockBuffer;
            
            // they are connecting again, need to send them everything again
            o->firstMapSent = false;
            o->firstMessageSent = false;
            o->inFlight = false;
            
            o->connected = true;
            o->cravingKnown = false;
            
            if( o->heldByOther ) {
                // they're held, so they may have moved far away from their
                // original location
                
                // their first PU on reconnect should give an estimate of this
                // new location
                
                LiveObject *holdingPlayer = 
                    getLiveObject( o->heldByOtherID );
                
                if( holdingPlayer != NULL ) {
                    o->xd = holdingPlayer->xd;
                    o->yd = holdingPlayer->yd;
                    
                    o->xs = holdingPlayer->xs;
                    o->ys = holdingPlayer->ys;
                    }
                }
            
            AppLog::infoF( "Player %d (%s) has reconnected.",
                           o->id, o->email );

            delete [] inEmail;
            
            return -1;
            }
        }
    


    // a baby needs to be born

    char eveWindow = isEveWindow();
    char forceGirl = false;
    
    int familyLimitAfterEveWindow = SettingsManager::getIntSetting( 
            "familyLimitAfterEveWindow", 15 );

    int cM = countFertileMothers();
    int cB = countHelplessBabies();
    int cFam = countFamilies();

    if( ! eveWindow ) {
        
        float ratio = SettingsManager::getFloatSetting( 
            "babyMotherApocalypseRatio", 6.0 );
        
        if( cM == 0 || (float)cB / (float)cM >= ratio ) {
            // too many babies per mother inside barrier

            triggerApocalypseNow();
            }
        else {
            int minFertile = players.size() / 15;
            if( minFertile < 2 ) {
                minFertile = 2;
                }
            if( cM < minFertile ) {
                // less than 1/15 of the players are fertile mothers
                forceGirl = true;
                }
            }

        if( !apocalypseTriggered && familyLimitAfterEveWindow > 0 ) {
            
            // there's a family limit
            // see if we passed it
            
            if( cFam > familyLimitAfterEveWindow ) {
                // too many families
                
                // that means we've reach a state where no one is surviving
                // and there are lots of eves scrounging around
                triggerApocalypseNow();
                }
            }
            
        }

    
    int barrierRadius = SettingsManager::getIntSetting( "barrierRadius", 250 );
    int barrierOn = SettingsManager::getIntSetting( "barrierOn", 1 );
    

    // reload these settings every time someone new connects
    // thus, they can be changed without restarting the server
    minFoodDecrementSeconds = 
        SettingsManager::getFloatSetting( "minFoodDecrementSeconds", 5.0f );
    
    maxFoodDecrementSeconds = 
        SettingsManager::getFloatSetting( "maxFoodDecrementSeconds", 20 );

    babyBirthFoodDecrement = 
        SettingsManager::getIntSetting( "babyBirthFoodDecrement", 10 );


    eatBonus = 
        SettingsManager::getIntSetting( "eatBonus", 0 );
		
    useCurseWords = 
        SettingsManager::getIntSetting( "useCurseWords", 1 );

    minActivePlayersForLanguages =
        SettingsManager::getIntSetting( "minActivePlayersForLanguages", 15 );

    canYumChainBreak = SettingsManager::getIntSetting( "canYumChainBreak", 0 );
    

    
    minAgeForCravings = SettingsManager::getDoubleSetting( "minAgeForCravings",
                                                           10 );
    

    numConnections ++;
                
    LiveObject newObject;

    newObject.email = inEmail;
    newObject.origEmail = NULL;
    
    newObject.lastSidsBabyEmail = NULL;

    newObject.lastBabyEmail = NULL;

    newObject.cravingFood = noCraving;
    newObject.cravingFoodYumIncrement = 0;
    newObject.cravingKnown = false;
    
    newObject.id = nextID;
    nextID++;




    if( familyDataLogFile != NULL ) {
        int eveCount = 0;
        int inCount = 0;
        
        double ageSum = 0;
        int ageSumCount = 0;
        
        for( int i=0; i<players.size(); i++ ) {
            LiveObject *o = players.getElement( i );
        
            if( ! o->error && o->connected ) {
                if( o->parentID == -1 ) {
                    eveCount++;
                    }
                if( barrierOn ) {
                    // only those inside the barrier
                    GridPos pos = getPlayerPos( o );
                
                    if( abs( pos.x ) < barrierRadius &&
                        abs( pos.y ) < barrierRadius ) {
                        inCount++;
                        
                        ageSum += computeAge( o );
                        ageSumCount++;
                        }
                    }
                else {
                    ageSum += computeAge( o );
                    ageSumCount++;
                    }
                }
            }
        
        double averageAge = 0;
        if( ageSumCount > 0 ) {
            averageAge = ageSum / ageSumCount;
            }
        
        fprintf( familyDataLogFile,
                 "%.2f nid:%d fam:%d mom:%d bb:%d plr:%d eve:%d rft:%d "
                 "avAge:%.2f\n",
                 Time::getCurrentTime(), newObject.id, 
                 cFam, cM, cB,
                 players.size(),
                 eveCount,
                 inCount,
                 averageAge );
        }


    
    newObject.fitnessScore = inFitnessScore;
    

    SettingsManager::setSetting( "nextPlayerID",
                                 (int)nextID );


    newObject.responsiblePlayerID = -1;
    
    newObject.displayID = getRandomPersonObject();
    
    newObject.isEve = false;
    
    newObject.isTutorial = false;
    
    if( inTutorialNumber > 0 ) {
        newObject.isTutorial = true;
        }

    newObject.trueStartTimeSeconds = Time::getCurrentTime();
    newObject.lifeStartTimeSeconds = newObject.trueStartTimeSeconds;
                            

    newObject.lastSayTimeSeconds = Time::getCurrentTime();
    

    newObject.heldByOther = false;
    newObject.everHeldByParent = false;
    

    int numPlayers = players.size();

    SimpleVector<LiveObject*> parentChoices;
    
    int numBirthLocationsCurseBlocked = 0;

    int numOfAge = 0;
    

    // first, find all mothers that could possibly have us

    // three passes, once with birth cooldown limit and lineage limits on, 
    // then more passes with them off (if needed)
    char checkCooldown = true;
    

    for( int p=0; p<2; p++ ) {
    
        for( int i=0; i<numPlayers; i++ ) {
            LiveObject *player = players.getElement( i );
            
            if( player->error ) {
                continue;
                }
            
            if( player->isTutorial ) {
                continue;
                }
            
            if( player->vogMode ) {
                continue;
                }
				
			//skips over solo players who declare themselves infertile
		    if( player->declaredInfertile ) {
				continue;
				}

            //GridPos motherPos = getPlayerPos( player );
                
            
            if( player->lastSidsBabyEmail != NULL &&
                strcmp( player->lastSidsBabyEmail,
                        newObject.email ) == 0 ) {
                // this baby JUST committed SIDS for this mother
                // skip her
                // (don't ever send SIDS baby to same mother twice in a row)
                continue;
                }

            if( isFertileAge( player ) ) {
                numOfAge ++;
                
                if( checkCooldown &&
                    Time::timeSec() < player->birthCoolDown ) {    
                    continue;
                    }
                
                GridPos motherPos = getPlayerPos( player );

                if( usePersonalCurses &&
                    isBirthLocationCurseBlocked( newObject.email, 
                                                 motherPos ) ) {
                    // this spot forbidden
                    // because someone nearby cursed new player
                    numBirthLocationsCurseBlocked++;
                    continue;
                    }
            
                // test any twins also
                char twinBanned = false;
                for( int s=0; s<tempTwinEmails.size(); s++ ) {
                    if( usePersonalCurses &&
                        // non-cached version for twin emails
                        // (otherwise, we interfere with caching done
                        //  for our email)
                        isBirthLocationCurseBlockedNoCache( 
                            tempTwinEmails.getElementDirect( s ), 
                            motherPos ) ) {
                        twinBanned = true;
                        
                        numBirthLocationsCurseBlocked++;
                        
                        break;
                        }
                    }
                
                if( twinBanned ) {
                    continue;
                    }
                
            
                if( ( inCurseStatus.curseLevel <= 0 && 
                      player->curseStatus.curseLevel <= 0 ) 
                    || 
                    ( inCurseStatus.curseLevel > 0 && 
                      player->curseStatus.curseLevel > 0 ) ) {
                    // cursed babies only born to cursed mothers
                    // non-cursed babies never born to cursed mothers
                    parentChoices.push_back( player );
                    }
                }
            }
        
        

        if( p == 0 ) {
            if( parentChoices.size() > 0 || numOfAge == 0 ) {
                // found some mothers off-cool-down, 
                // or there are none at all
                // skip second pass
                break;
                }
            
            // else found no mothers (but some on cool-down?)
            // start over with cooldowns off
            
            AppLog::infoF( 
                "Trying to place new baby %s, out of %d fertile moms, "
                "all are on cooldown, lineage banned, or curse blocked.  "
                "Trying again ignoring cooldowns.", newObject.email, numOfAge );
            
            checkCooldown = false;
            numBirthLocationsCurseBlocked = 0;
            numOfAge = 0;
            }
        
        }
    
	


    if( parentChoices.size() == 0 && numBirthLocationsCurseBlocked > 0 ) {
        // they are blocked from being born EVERYWHERE by curses

        AppLog::infoF( "No available mothers, and %d are curse blocked, "
                       "sending a new Eve to donkeytown",
                       numBirthLocationsCurseBlocked );

        // d-town
        inCurseStatus.curseLevel = 1;
        inCurseStatus.excessPoints = 1;
        }

    

    if( inTutorialNumber > 0 ) {
        // Tutorial always played full-grown
        parentChoices.deleteAll();
        }

    if( inForceParentID == -2 ) {
        // force eve
        parentChoices.deleteAll();
        }
    else if( inForceParentID > -1 ) {
        // force parent choice
        parentChoices.deleteAll();
        
        LiveObject *forcedParent = getLiveObject( inForceParentID );
        
        if( forcedParent != NULL ) {
            parentChoices.push_back( forcedParent );
            }
        }
    
    
    if( SettingsManager::getIntSetting( "forceAllPlayersEve", 0 ) ) {
        parentChoices.deleteAll();
        }
		
    if( hashedSpawnSeed != 0 && SettingsManager::getIntSetting( "forceEveOnSeededSpawn", 0 ) ) {
        parentChoices.deleteAll();
        }




    newObject.parentChainLength = 1;

    if( parentChoices.size() == 0 ) {
        // new Eve
        // she starts almost full grown

        newObject.isEve = true;
        newObject.lineageEveID = newObject.id;
        
        newObject.lifeStartTimeSeconds -= 14 * ( 1.0 / getAgeRate() );
        
        // she starts off craving a food right away
        newObject.cravingFood = getCravedFood( newObject.lineageEveID,
                                               newObject.parentChainLength );
        // initilize increment
        newObject.cravingFoodYumIncrement = 1;

        int femaleID = getRandomFemalePersonObject();
        
        if( femaleID != -1 ) {
            newObject.displayID = femaleID;
            }

        }
    
                
    // else player starts as newborn
                

    newObject.foodCapModifier = 1.0;

    newObject.fever = 0;

    // start full up to capacity with food
    newObject.foodStore = computeFoodCapacity( &newObject );

    newObject.drunkenness = 0;
	newObject.drunkennessEffectETA = 0;
	newObject.drunkennessEffect = false;
	
	newObject.tripping = false;
	newObject.gonnaBeTripping = false;
	newObject.trippingEffectStartTime = 0;
	newObject.trippingEffectETA = 0;
    

    if( ! newObject.isEve ) {
        // babies start out almost starving
        newObject.foodStore = 2;
        }
    
    if( newObject.isTutorial && newObject.foodStore > 10 ) {
        // so they can practice eating at the beginning of the tutorial
        newObject.foodStore -= 6;
        }
    
    double currentTime = Time::getCurrentTime();
    

    newObject.envHeat = targetHeat;
    newObject.bodyHeat = targetHeat;
    newObject.biomeHeat = targetHeat;
    newObject.lastBiomeHeat = targetHeat;
    newObject.heat = 0.5;
    newObject.heatUpdate = false;
    newObject.lastHeatUpdate = currentTime;
    newObject.isIndoors = false;
    

    newObject.foodDecrementETASeconds =
        currentTime + 
        computeFoodDecrementTimeSeconds( &newObject );
                
    newObject.foodUpdate = true;
    newObject.lastAteID = 0;
    newObject.lastAteFillMax = 0;
    newObject.justAte = false;
    newObject.justAteID = 0;
    
    newObject.yummyBonusStore = 0;

    newObject.clothing = getEmptyClothingSet();

    for( int c=0; c<NUM_CLOTHING_PIECES; c++ ) {
        newObject.clothingEtaDecay[c] = 0;
        }
    
    newObject.xs = 0;
    newObject.ys = 0;
    newObject.xd = 0;
    newObject.yd = 0;
    
    newObject.facingLeft = 0;
    newObject.lastFlipTime = currentTime;
    
    newObject.lastRegionLookTime = 0;
    newObject.playerCrossingCheckTime = 0;
    
    
    LiveObject *parent = NULL;

    char placed = false;
    
    if( parentChoices.size() > 0 ) {
        placed = true;
        
        if( newObject.isEve ) {
            // spawned next to random existing player
            int parentIndex = 
                randSource.getRandomBoundedInt( 0,
                                                parentChoices.size() - 1 );
            
            parent = parentChoices.getElementDirect( parentIndex );
            }
        else {
            // baby


            
            // filter parent choices by this baby's skip list
            SimpleVector<LiveObject *> 
                filteredParentChoices( parentChoices.size() );
            
            for( int i=0; i<parentChoices.size(); i++ ) {
                LiveObject *p = parentChoices.getElementDirect( i );
                
                if( ! isSkipped( inEmail, p->lineageEveID ) ) {
                    filteredParentChoices.push_back( p );
                    }
                }

            if( filteredParentChoices.size() == 0 ) {
                // baby has skipped everyone
                
                // clear their list and let them start over again
                clearSkipList( inEmail );
                
                filteredParentChoices.push_back_other( &parentChoices );
                }
            

            
            // pick random mother from a weighted distribution based on 
            // each mother's temperature
            
            // AND each mother's current YUM multiplier
            
            int maxYumMult = 1;

            for( int i=0; i<filteredParentChoices.size(); i++ ) {
                LiveObject *p = filteredParentChoices.getElementDirect( i );
                
                int yumMult = p->yummyFoodChain.size() - 1;
                
                if( yumMult < 0 ) {
                    yumMult = 0;
                    }
                
                if( yumMult > maxYumMult ) {
                    maxYumMult = yumMult;
                    }
                }
            
            // 0.5 temp is worth .5 weight
            // 1.0 temp and 0 are worth 0 weight
            
            // max YumMult worth same that perfect temp is worth (0.5 weight)

            double totalWeight = 0;
            
            for( int i=0; i<filteredParentChoices.size(); i++ ) {
                LiveObject *p = filteredParentChoices.getElementDirect( i );

                // temp part of weight
                totalWeight += 0.5 - fabs( p->heat - 0.5 );
                

                int yumMult = p->yummyFoodChain.size() - 1;
                                
                if( yumMult < 0 ) {
                    yumMult = 0;
                    }

                // yum mult part of weight
                totalWeight += 0.5 * yumMult / (double) maxYumMult;
                }

            double choice = 
                randSource.getRandomBoundedDouble( 0, totalWeight );
            
            
            totalWeight = 0;
            
            for( int i=0; i<filteredParentChoices.size(); i++ ) {
                LiveObject *p = filteredParentChoices.getElementDirect( i );

                totalWeight += 0.5 - fabs( p->heat - 0.5 );


                int yumMult = p->yummyFoodChain.size() - 1;
                                
                if( yumMult < 0 ) {
                    yumMult = 0;
                    }

                // yum mult part of weight
                totalWeight += 0.5 * yumMult / (double) maxYumMult;

                if( totalWeight >= choice ) {
                    parent = p;
                    break;
                    }                
                }
            }
        

        
        if( ! newObject.isEve ) {
            // mother giving birth to baby
            // take a ton out of her food store

            int min = 4;
            if( parent->foodStore < min ) {
                min = parent->foodStore;
                }
            parent->foodStore -= babyBirthFoodDecrement;
            if( parent->foodStore < min ) {
                parent->foodStore = min;
                }

            parent->foodDecrementETASeconds +=
                computeFoodDecrementTimeSeconds( parent );
            
            parent->foodUpdate = true;
            

            // only set race if the spawn-near player is our mother
            // otherwise, we are a new Eve spawning next to a baby
            
            timeSec_t curTime = Time::timeSec();
            
            parent->babyBirthTimes->push_back( curTime );
            parent->babyIDs->push_back( newObject.id );
            
            if( parent->lastBabyEmail != NULL ) {
                delete [] parent->lastBabyEmail;
                }
            parent->lastBabyEmail = stringDuplicate( newObject.email );
            

            // set cool-down time before this worman can have another baby
            parent->birthCoolDown = pickBirthCooldownSeconds() + curTime;

            ObjectRecord *parentObject = getObject( parent->displayID );

            // pick race of child
            int numRaces;
            int *races = getRaces( &numRaces );
        
            int parentRaceIndex = -1;
            
            for( int i=0; i<numRaces; i++ ) {
                if( parentObject->race == races[i] ) {
                    parentRaceIndex = i;
                    break;
                    }
                }
            

            if( parentRaceIndex != -1 ) {
                
                int childRace = parentObject->race;
                
                char forceDifferentRace = false;

                if( getRaceSize( parentObject->race ) < 3 ) {
                    // no room in race for diverse family members
                    
                    // pick a different race for child to ensure village 
                    // diversity
                    // (otherwise, almost everyone is going to look the same)
                    forceDifferentRace = true;
                    }
                
                // everyone has a small chance of having a neighboring-race
                // baby, even if not forced by parent's small race size
                if( forceDifferentRace ||
                    randSource.getRandomDouble() > 
                    childSameRaceLikelihood ) {
                    
                    // different race than parent
                    
                    int offset = 1;
                    
                    if( randSource.getRandomBoolean() ) {
                        offset = -1;
                        }
                    int childRaceIndex = parentRaceIndex + offset;
                    
                    // don't wrap around
                    // but push in other direction instead
                    if( childRaceIndex >= numRaces ) {
                        childRaceIndex = numRaces - 2;
                        }
                    if( childRaceIndex < 0 ) {
                        childRaceIndex = 1;
                        }
                    
                    // stay in bounds
                    if( childRaceIndex >= numRaces ) {
                        childRaceIndex = numRaces - 1;
                        }
                    

                    childRace = races[ childRaceIndex ];
                    }
                
                if( childRace == parentObject->race ) {
					
					if( countYoungFemalesInLineage( parent->lineageEveID ) <
						SettingsManager::getIntSetting( "minYoungFemalesToForceGirl", 2 ) ) {
						forceGirl = true;
						}
					
                    newObject.displayID = getRandomFamilyMember( 
                        parentObject->race, parent->displayID, familySpan,
                        forceGirl );
                    }
                else {
                    newObject.displayID = 
                        getRandomPersonObjectOfRace( childRace );
                    }
            
                }
        
            delete [] races;
            }
        
        if( parent->xs == parent->xd && 
            parent->ys == parent->yd ) {
                        
            // stationary parent
            newObject.xs = parent->xs;
            newObject.ys = parent->ys;
                        
            newObject.xd = parent->xs;
            newObject.yd = parent->ys;
            }
        else {
            // find where parent is along path
            GridPos cPos = computePartialMoveSpot( parent );
                        
            newObject.xs = cPos.x;
            newObject.ys = cPos.y;
                        
            newObject.xd = cPos.x;
            newObject.yd = cPos.y;
            }
        
        if( newObject.xs > maxPlacementX ) {
            maxPlacementX = newObject.xs;
            }
        }
    else if( inTutorialNumber > 0 ) {
        
        int startX = maxPlacementX + tutorialOffsetX;
        int startY = tutorialCount * 25;

        newObject.xs = startX;
        newObject.ys = startY;
        
        newObject.xd = startX;
        newObject.yd = startY;

        char *mapFileName = autoSprintf( "tutorial%d.txt", inTutorialNumber );
        
        placed = loadTutorialStart( &( newObject.tutorialLoad ),
                                    mapFileName, startX, startY );
        
        delete [] mapFileName;

        tutorialCount ++;

        int maxPlayers = 
            SettingsManager::getIntSetting( "maxPlayers", 200 );

        if( tutorialCount > maxPlayers ) {
            // wrap back to 0 so we don't keep getting farther
            // and farther away on map if server runs for a long time.

            // The earlier-placed tutorials are over by now, because
            // we can't have more than maxPlayers tutorials running at once
            
            tutorialCount = 0;
            }
        }
    
    
    if( !placed ) {
        // tutorial didn't happen if not placed
        newObject.isTutorial = false;
        
        char allowEveRespawn = true;
        
        if( numOfAge >= 4 ) {
            // there are at least 4 fertile females on the server
            // why is this player spawning as Eve?
            // they must be on lineage ban everywhere
            // (and they are NOT a solo player on an empty server)
            // don't allow them to spawn back at their last old-age Eve death
            // location.
            allowEveRespawn = false;
            }

        // else starts at civ outskirts (lone Eve)
        
        SimpleVector<GridPos> otherPeoplePos( numPlayers );


        // consider players to be near Eve location that match
        // Eve's curse status
        char seekingCursed = false;
        
        if( inCurseStatus.curseLevel > 0 ) {
            seekingCursed = true;
            }
        

        for( int i=0; i<numPlayers; i++ ) {
            LiveObject *player = players.getElement( i );
            
            if( player->error || 
                ! player->connected ||
                player->isTutorial ||
                player->vogMode ) {
                continue;
                }

            if( seekingCursed && player->curseStatus.curseLevel <= 0 ) {
                continue;
                }
            else if( ! seekingCursed &&
                     player->curseStatus.curseLevel > 0 ) {
                continue;
                }

            GridPos p = { player->xs, player->ys };
            otherPeoplePos.push_back( p );
            }
        

        int startX, startY;
        getEvePosition( newObject.email, 
                        newObject.id, &startX, &startY, 
                        &otherPeoplePos, allowEveRespawn );

        if( inCurseStatus.curseLevel > 0 ) {
            // keep cursed players away

            // 20K away in X and 20K away in Y, pushing out away from 0
            // in both directions

            if( startX > 0 )
                startX += 20000;
            else
                startX -= 20000;
            
            if( startY > 0 )
                startY += 20000;
            else
                startY -= 20000;
            }
        

        if( SettingsManager::getIntSetting( "forceEveLocation", 0 ) && inCurseStatus.curseLevel == 0 ) {

            startX = 
                SettingsManager::getIntSetting( "forceEveLocationX", 0 );
            startY = 
                SettingsManager::getIntSetting( "forceEveLocationY", 0 );
            }
        
        uint32_t tempHashedSpawnSeed;
        int useSeedList = SettingsManager::getIntSetting( "useSeedList", 0 );
        //pick a random seed from a list to be the default spawn
        if ( useSeedList && hashedSpawnSeed == 0 ) {
            
            //parse the seeds
            SimpleVector<char *> *list = 
                SettingsManager::getSetting( 
                    "defaultSeedList" );
            
            //chose a random seed from the list
            int seedIndex = 
                randSource.getRandomBoundedInt( 0, list->size() - 1 );
            
            char *choseSeed;
            for( int i=0; i<list->size(); i++ ) {
                if( seedIndex == i ) {
                    choseSeed = list->getElementDirect( i );
                    break;
                    }
                }
                
            std::string seed( choseSeed );
            
            //convert and apply seed hash (copy pasted code)
            //make this a separate method in the future to prevent redundancy
            
            // FNV-1a Hashing algorithm
            auto hashStr = [](std::string &s, const uint32_t FNV_init = 2166136261u){
                const size_t FNV_prime = 111337;

                // Hash seed to 4 byte int
                uint32_t hash = FNV_init;
                for( auto c : s ) {
                    hash ^= c;
                    hash *= FNV_prime;
                }

                return hash;
            };
            
            // Get the substr from one after the seed delim
            std::string seedSalt { SettingsManager::getStringSetting("seedSalt", "default salt") };
            
            tempHashedSpawnSeed =
                hashStr(seed, hashStr(seedSalt));
          }
        else {
            //use defalt seed configuration
            tempHashedSpawnSeed = hashedSpawnSeed;
        }

        if( tempHashedSpawnSeed != 0 ) {
            // Get bounding box from setting, default to 10k
            int seedSpawnBoundingBox =
                SettingsManager::getIntSetting( "seedSpawnBoundingBox", 10000 );

            std::seed_seq ssq { tempHashedSpawnSeed };
            std::mt19937_64 mt { ssq };

            std::uniform_int_distribution<int> dist( -seedSpawnBoundingBox/2, seedSpawnBoundingBox/2 );

            startX = dist(mt);
            startY = dist(mt);

            AppLog::infoF( "Player %s seed evaluated to (%d,%d)",
                    newObject.email, startX, startY );
            }
        
        
        newObject.xs = startX;
        newObject.ys = startY;
        
        newObject.xd = startX;
        newObject.yd = startY;

        if( newObject.xs > maxPlacementX ) {
            maxPlacementX = newObject.xs;
            }
        }
    

    if( inForceDisplayID != -1 ) {
        newObject.displayID = inForceDisplayID;
        }

    if( inForcePlayerPos != NULL ) {
        int startX = inForcePlayerPos->x;
        int startY = inForcePlayerPos->y;
        
        newObject.xs = startX;
        newObject.ys = startY;
        
        newObject.xd = startX;
        newObject.yd = startY;

        if( newObject.xs > maxPlacementX ) {
            maxPlacementX = newObject.xs;
            }
        }
    

    
    if( parent == NULL ) {
        // Eve
        int forceID = SettingsManager::getIntSetting( "forceEveObject", 0 );
    
        if( forceID > 0 ) {
            newObject.displayID = forceID;
            }
        
        
        float forceAge = SettingsManager::getFloatSetting( "forceEveAge", 0.0 );
        
        if( forceAge > 0 ) {
            newObject.lifeStartTimeSeconds = 
                Time::getCurrentTime() - forceAge * ( 1.0 / getAgeRate() );
            }
        }
    

    newObject.holdingID = 0;


    if( areTriggersEnabled() ) {
        int id = getTriggerPlayerDisplayID( inEmail );
        
        if( id != -1 ) {
            newObject.displayID = id;
            
            newObject.lifeStartTimeSeconds = 
                Time::getCurrentTime() - 
                getTriggerPlayerAge( inEmail ) * ( 1.0 / getAgeRate() );
        
            GridPos pos = getTriggerPlayerPos( inEmail );
            
            newObject.xd = pos.x;
            newObject.yd = pos.y;
            newObject.xs = pos.x;
            newObject.ys = pos.y;
            newObject.xd = pos.x;
            
            newObject.holdingID = getTriggerPlayerHolding( inEmail );
            newObject.clothing = getTriggerPlayerClothing( inEmail );
            }
        }
    
    
    newObject.lineage = new SimpleVector<int>();
    
    newObject.name = NULL;
	newObject.displayedName = NULL;
    newObject.familyName = NULL;
    
    newObject.nameHasSuffix = false;
    newObject.lastSay = NULL;
    newObject.curseStatus = inCurseStatus;
    
    //2HOL additions for: password-protected-objects, additional fields initialization
    newObject.saidPassword = NULL;
    newObject.assignedPassword = NULL;
    

    if( newObject.curseStatus.curseLevel == 0 &&
        hasCurseToken( inEmail ) ) {
        newObject.curseTokenCount = 1;
        }
    else {
        newObject.curseTokenCount = 0;
        }

    newObject.curseTokenUpdate = true;

    
    newObject.pathLength = 0;
    newObject.pathToDest = NULL;
    newObject.pathTruncated = 0;
    newObject.firstMapSent = false;
    newObject.lastSentMapX = 0;
    newObject.lastSentMapY = 0;
    newObject.moveStartTime = Time::getCurrentTime();
    newObject.moveTotalSeconds = 0;
    newObject.facingOverride = 0;
    newObject.actionAttempt = 0;
    newObject.actionTarget.x = 0;
    newObject.actionTarget.y = 0;
    newObject.holdingEtaDecay = 0;
    newObject.heldOriginValid = 0;
    newObject.heldOriginX = 0;
    newObject.heldOriginY = 0;

    newObject.heldGraveOriginX = 0;
    newObject.heldGraveOriginY = 0;
    newObject.heldGravePlayerID = 0;
    
    newObject.heldTransitionSourceID = -1;
    newObject.numContained = 0;
    newObject.containedIDs = NULL;
    newObject.containedEtaDecays = NULL;
    newObject.subContainedIDs = NULL;
    newObject.subContainedEtaDecays = NULL;
    newObject.embeddedWeaponID = 0;
    newObject.embeddedWeaponEtaDecay = 0;
    newObject.murderSourceID = 0;
    newObject.holdingWound = false;
    
    newObject.murderPerpID = 0;
    newObject.murderPerpEmail = NULL;
    
    newObject.deathSourceID = 0;
    
    newObject.everKilledAnyone = false;
    newObject.suicide = false;
    

    newObject.sock = inSock;
    newObject.sockBuffer = inSockBuffer;
    
    newObject.gotPartOfThisFrame = false;
    
    newObject.isNew = true;
    newObject.isNewCursed = false;
    newObject.firstMessageSent = false;
    newObject.inFlight = false;
    
    newObject.dying = false;
    newObject.dyingETA = 0;
    
    newObject.emotFrozen = false;
    newObject.emotUnfreezeETA = 0;
    newObject.emotFrozenIndex = 0;
    
    newObject.starving = false;

    newObject.connected = true;
    newObject.error = false;
    newObject.errorCauseString = "";
	
	newObject.lastActionTime = Time::getCurrentTime();
	newObject.isAFK = false;
    
    newObject.customGraveID = -1;
    newObject.deathReason = NULL;
    
    newObject.deleteSent = false;
    newObject.deathLogged = false;
    newObject.newMove = false;
    
    newObject.posForced = false;
    newObject.waitingForForceResponse = false;
    
    // first move that player sends will be 2
    newObject.lastMoveSequenceNumber = 1;

    newObject.needsUpdate = false;
    newObject.updateSent = false;
    newObject.updateGlobal = false;
    
    newObject.babyBirthTimes = new SimpleVector<timeSec_t>();
    newObject.babyIDs = new SimpleVector<int>();
    
    newObject.birthCoolDown = 0;
	newObject.declaredInfertile = false;
    
    newObject.monumentPosSet = false;
    newObject.monumentPosSent = true;
    
    newObject.holdingFlightObject = false;

    newObject.vogMode = false;
    newObject.postVogMode = false;
    newObject.vogJumpIndex = 0;
    
                
    for( int i=0; i<HEAT_MAP_D * HEAT_MAP_D; i++ ) {
        newObject.heatMap[i] = 0;
        }

    
    newObject.parentID = -1;
    char *parentEmail = NULL;

    if( parent != NULL && isFertileAge( parent ) ) {
        // do not log babies that new Eve spawns next to as parents
        newObject.parentID = parent->id;
        parentEmail = parent->email;

        if( parent->familyName != NULL ) {
            newObject.familyName = stringDuplicate( parent->familyName );
            }

        newObject.lineageEveID = parent->lineageEveID;

        newObject.parentChainLength = parent->parentChainLength + 1;

        // mother
        newObject.lineage->push_back( newObject.parentID );

        
        // inherit mother's craving at time of birth
        newObject.cravingFood = parent->cravingFood;
        
        // increment for next generation
        newObject.cravingFoodYumIncrement = parent->cravingFoodYumIncrement + 1;
        

        // inherit last heard monument, if any, from parent
        newObject.monumentPosSet = parent->monumentPosSet;
        newObject.lastMonumentPos = parent->lastMonumentPos;
        newObject.lastMonumentID = parent->lastMonumentID;
        if( newObject.monumentPosSet ) {
            newObject.monumentPosSent = false;
            }
        
        
        for( int i=0; 
             i < parent->lineage->size() && 
                 i < maxLineageTracked - 1;
             i++ ) {
            
            newObject.lineage->push_back( 
                parent->lineage->getElementDirect( i ) );
            }
        }

    newObject.birthPos.x = newObject.xd;
    newObject.birthPos.y = newObject.yd;
    
    newObject.originalBirthPos = newObject.birthPos;
    

    newObject.heldOriginX = newObject.xd;
    newObject.heldOriginY = newObject.yd;
    
    newObject.actionTarget = newObject.birthPos;



    newObject.ancestorIDs = new SimpleVector<int>();
    newObject.ancestorEmails = new SimpleVector<char*>();
    newObject.ancestorRelNames = new SimpleVector<char*>();
    newObject.ancestorLifeStartTimeSeconds = new SimpleVector<double>();
	newObject.ancestorLifeEndTimeSeconds = new SimpleVector<double>();
                                                  
    for( int j=0; j<players.size(); j++ ) {
        LiveObject *otherPlayer = players.getElement( j );
        
        if( otherPlayer->error ) {
            continue;
            }
        
        // a living other player
        
        // consider all men here
        // and any childless women (they are counted as aunts
        // for any children born before they themselves have children
        // or after all their own children die)
        if( newObject.parentID != otherPlayer->id 
            &&
            ( ! getFemale( otherPlayer ) ||
              countLivingChildren( otherPlayer->id ) == 0 ) ) {
            
				//Only direct mother-son/daughter parenting is counted

            }
        else {
            // females, look for direct ancestry

            for( int i=0; i<newObject.lineage->size(); i++ ) {
                    
                if( newObject.lineage->getElementDirect( i ) ==
                    otherPlayer->id ) {
                        
                    //Only direct mother-son/daughter parenting is counted
					if( i != 0 ) continue;
                    
                    newObject.ancestorIDs->push_back( otherPlayer->id );

                    newObject.ancestorEmails->push_back( 
                        stringDuplicate( otherPlayer->email ) );

                    // i tells us how many greats and grands
                    SimpleVector<char> workingName;
                    
                    for( int g=1; g<=i; g++ ) {
                        if( g == i ) {
                            workingName.appendElementString( "Grand" );
                            }
                        else {
                            workingName.appendElementString( "Great_" );
                            }
                        }
                    
                    
                    if( i != 0 ) {
                        if( ! getFemale( &newObject ) ) {
                            workingName.appendElementString( "son" );
                            }
                        else {
                            workingName.appendElementString( "daughter" );
                            }
                        }
                    else {
                        // no "Grand"
                        if( ! getFemale( &newObject ) ) {
                                workingName.appendElementString( "Son" );
                            }
                        else {
                            workingName.appendElementString( "Daughter" );
                            }
                        }
                    
                    
                    newObject.ancestorRelNames->push_back(
                        workingName.getElementString() );
                    
                    newObject.ancestorLifeStartTimeSeconds->push_back(
                            otherPlayer->lifeStartTimeSeconds );
                    newObject.ancestorLifeEndTimeSeconds->push_back(
                            -1.0 );
                    
                    break;
                    }
                }
            }
        

        }
    

    

    
    // parent pointer possibly no longer valid after push_back, which
    // can resize the vector
    parent = NULL;


    if( newObject.isTutorial ) {
        AppLog::infoF( "New player %s pending tutorial load (tutorial=%d)",
                       newObject.email,
                       inTutorialNumber );

        // holding bay for loading tutorial maps incrementally
        tutorialLoadingPlayers.push_back( newObject );
        }
    else {
        players.push_back( newObject );            
        }

    if( newObject.isEve ) {
        addEveLanguage( newObject.id );
        }
    else {
        incrementLanguageCount( newObject.lineageEveID );
        }
    

    // addRecentScore( newObject.email, inFitnessScore );
    

    if( ! newObject.isTutorial )     
    logBirth( newObject.id,
              newObject.email,
              newObject.parentID,
              parentEmail,
              ! getFemale( &newObject ),
              newObject.xd,
              newObject.yd,
              players.size(),
              newObject.parentChainLength );
    
    AppLog::infoF( "New player %s connected as player %d (tutorial=%d) (%d,%d)"
                   " (maxPlacementX=%d)",
                   newObject.email, newObject.id,
                   inTutorialNumber, newObject.xs, newObject.ys,
                   maxPlacementX );
    
    return newObject.id;
    }




static void processWaitingTwinConnection( FreshConnection inConnection ) {
    AppLog::infoF( "Player %s waiting for twin party of %d", 
                   inConnection.email,
                   inConnection.twinCount );
    waitingForTwinConnections.push_back( inConnection );
    
    CurseStatus anyTwinCurseLevel = inConnection.curseStatus;
    

    // count how many match twin code from inConnection
    // is this the last one to join the party?
    SimpleVector<FreshConnection*> twinConnections;
    

    for( int i=0; i<waitingForTwinConnections.size(); i++ ) {
        FreshConnection *nextConnection = 
            waitingForTwinConnections.getElement( i );
        
        if( nextConnection->error ) {
            continue;
            }
        
        if( nextConnection->twinCode != NULL
            &&
            strcmp( inConnection.twinCode, nextConnection->twinCode ) == 0 
            &&
            inConnection.twinCount == nextConnection->twinCount ) {
            
            if( strcmp( inConnection.email, nextConnection->email ) == 0 ) {
                // don't count this connection itself
                continue;
                }

            if( nextConnection->curseStatus.curseLevel > 
                anyTwinCurseLevel.curseLevel ) {
                anyTwinCurseLevel = nextConnection->curseStatus;
                }
            
            twinConnections.push_back( nextConnection );
            }
        }

    
    if( twinConnections.size() + 1 >= inConnection.twinCount ) {
        // everyone connected and ready in twin party

        AppLog::infoF( "Found %d other people waiting for twin party of %s, "
                       "ready", 
                       twinConnections.size(), inConnection.email );
        
        char *emailCopy = stringDuplicate( inConnection.email );
        
        // set up twin emails for lineage ban
        for( int i=0; i<twinConnections.size(); i++ ) {
            FreshConnection *nextConnection = 
                twinConnections.getElementDirect( i );
        
            tempTwinEmails.push_back( nextConnection->email );
            }
        
        nextLogInTwin = true;
        firstTwinID = -1;
        
        int newID = processLoggedInPlayer( false,
                                           inConnection.sock,
                                           inConnection.sockBuffer,
                                           inConnection.email,
                                           inConnection.hashedSpawnSeed,
                                           inConnection.tutorialNumber,
                                           anyTwinCurseLevel,
                                           inConnection.fitnessScore );
        tempTwinEmails.deleteAll();
        
        if( newID == -1 ) {
            AppLog::infoF( "%s reconnected to existing life, not triggering "
                           "fellow twins to spawn now.",
                           emailCopy );

            // take them out of waiting list too
            for( int i=0; i<waitingForTwinConnections.size(); i++ ) {
                if( waitingForTwinConnections.getElement( i )->sock ==
                    inConnection.sock ) {
                    // found
                    
                    waitingForTwinConnections.deleteElement( i );
                    break;
                    }
                }

            delete [] emailCopy;

            if( inConnection.twinCode != NULL ) {
                delete [] inConnection.twinCode;
                inConnection.twinCode = NULL;
                }
            nextLogInTwin = false;
            return;
            }

        delete [] emailCopy;
        
        firstTwinID = newID;
        
        LiveObject *newPlayer = NULL;

        if( inConnection.tutorialNumber == 0 ) {
            newPlayer = getLiveObject( newID );
            }
        else {
            newPlayer = tutorialLoadingPlayers.getElement(
                tutorialLoadingPlayers.size() - 1 );
            }


        int parent = newPlayer->parentID;
        int displayID = newPlayer->displayID;
        GridPos playerPos = { newPlayer->xd, newPlayer->yd };
        
        GridPos *forcedEvePos = NULL;
        
        if( parent == -1 ) {
            // first twin placed was Eve
            // others are identical Eves
            forcedEvePos = &playerPos;
            // trigger forced Eve placement
            parent = -2;
            }


        char usePersonalCurses = 
            SettingsManager::getIntSetting( "usePersonalCurses", 0 );
    


        // save these out here, because newPlayer points into 
        // tutorialLoadingPlayers, which may expand during this loop,
        // invalidating that pointer
        char isTutorial = newPlayer->isTutorial;
        TutorialLoadProgress sharedTutorialLoad = newPlayer->tutorialLoad;

        for( int i=0; i<twinConnections.size(); i++ ) {
            FreshConnection *nextConnection = 
                twinConnections.getElementDirect( i );
            
            processLoggedInPlayer( false, 
                                   nextConnection->sock,
                                   nextConnection->sockBuffer,
                                   nextConnection->email,
                                   nextConnection->hashedSpawnSeed,
                                   // ignore tutorial number of all but
                                   // first player
                                   0,
                                   anyTwinCurseLevel,
                                   nextConnection->fitnessScore,
                                   parent,
                                   displayID,
                                   forcedEvePos );
            
            // just added is always last object in list
            
            if( usePersonalCurses ) {
                // curse level not known until after first twin logs in
                // their curse level is set based on blockage caused
                // by any of the other twins in the party
                // pass it on.
                LiveObject *newTwinPlayer = 
                    players.getElement( players.size() - 1 );
                newTwinPlayer->curseStatus = newPlayer->curseStatus;
                }



            LiveObject newTwinPlayer = 
                players.getElementDirect( players.size() - 1 );

            if( isTutorial ) {
                // force this one to wait for same tutorial map load
                newTwinPlayer.tutorialLoad = sharedTutorialLoad;

                // flag them as a tutorial player too, so they can't have
                // babies in the tutorial, and they won't be remembered
                // as a long-lineage position at shutdown
                newTwinPlayer.isTutorial = true;

                players.deleteElement( players.size() - 1 );
                
                tutorialLoadingPlayers.push_back( newTwinPlayer );
                }
            }
        
        firstTwinID = -1;

        char *twinCode = stringDuplicate( inConnection.twinCode );
        
        for( int i=0; i<waitingForTwinConnections.size(); i++ ) {
            FreshConnection *nextConnection = 
                waitingForTwinConnections.getElement( i );
            
            if( nextConnection->error ) {
                continue;
                }
            
            if( nextConnection->twinCode != NULL 
                &&
                nextConnection->twinCount == inConnection.twinCount
                &&
                strcmp( nextConnection->twinCode, twinCode ) == 0 ) {
                
                delete [] nextConnection->twinCode;
                waitingForTwinConnections.deleteElement( i );
                i--;
                }
            }
        
        delete [] twinCode;
        
        nextLogInTwin = false;
        }
    }



// doesn't check whether dest itself is blocked
static char directLineBlocked( GridPos inSource, GridPos inDest ) {
    // line algorithm from here
    // https://en.wikipedia.org/wiki/Bresenham's_line_algorithm
    
    double deltaX = inDest.x - inSource.x;
    
    double deltaY = inDest.y - inSource.y;
    

    int xStep = 1;
    if( deltaX < 0 ) {
        xStep = -1;
        }
    
    int yStep = 1;
    if( deltaY < 0 ) {
        yStep = -1;
        }
    

    if( deltaX == 0 ) {
        // vertical line
        
        // just walk through y
        for( int y=inSource.y; y != inDest.y; y += yStep ) {
            if( isMapSpotBlocking( inSource.x, y ) ) {
                return true;
                }
            }
        }
    else {
        double deltaErr = fabs( deltaY / (double)deltaX );
        
        double error = 0;
        
        int y = inSource.y;
        for( int x=inSource.x; x != inDest.x || y != inDest.y; x += xStep ) {
            if( isMapSpotBlocking( x, y ) ) {
                return true;
                }
            error += deltaErr;
            
            if( error >= 0.5 ) {
                y += yStep;
                error -= 1.0;
                }
            
            // we may need to take multiple steps in y
            // if line is vertically oriented
            while( error >= 0.5 ) {
                if( isMapSpotBlocking( x, y ) ) {
                    return true;
                    }

                y += yStep;
                error -= 1.0;
                }
            }
        }

    return false;
    }



char removeFromContainerToHold( LiveObject *inPlayer, 
                                int inContX, int inContY,
                                int inSlotNumber );




// find index of spot on container held item can swap with, or -1 if none found
static int getContainerSwapIndex( LiveObject *inPlayer,
                                  int idToAdd,
                                  int inStillHeld,
                                  int inSearchLimit,
                                  int inContX, int inContY ) {
    // take what's on bottom of container, but only if it's different
    // from what's in our hand
    // AND we are old enough to take it
    double playerAge = computeAge( inPlayer );
    
    // if we find a same object on bottom, keep going up until
    // we find a non-same one to swap
    for( int botInd = 0; botInd < inSearchLimit; botInd ++ ) {
        
        char same = false;
        
        int bottomItem = 
            getContained( inContX, inContY, botInd, 0 );
        
        if( bottomItem > 0 &&
            getObject( bottomItem )->minPickupAge > playerAge ) {
            // too young to hold!
            same = true;
            }
        else if( bottomItem == idToAdd ) {
            if( bottomItem > 0 ) {
                // not sub conts
                same = true;
                }
            else {
                // they must contain same stuff to be same
                int bottomNum = getNumContained( inContX, inContY,
                                                 botInd + 1 );
                int topNum;

                if( inStillHeld ) {
                    topNum = inPlayer->numContained;
                    }
                else {
                    // already in the container
                    topNum =  getNumContained( inContX, inContY,
                                               inSearchLimit + 1 );
                    }
                
                if( bottomNum != topNum ) {
                    same = false;
                    }
                else {
                    same = true;
                    for( int b=0; b<bottomNum; b++ ) {
                        int subB = getContained( inContX, inContY,
                                                 b, botInd + 1 );
                        int subT;

                        if( inStillHeld ) {
                            subT = inPlayer->containedIDs[b];
                            }
                        else {
                            subT = getContained( inContX, inContY,
                                                 b, inSearchLimit + 1 );
                            }
                        
                                
                        if( subB != subT ) {
                            same = false;
                            break;
                            }
                        }
                    }
                }
            }
        if( !same ) {
            return botInd;
            }
        }
    
    return -1;
    }



// checks for granular +cont containment limitations
// assumes that container size limitation and 
// containable property checked elsewhere
static char containmentPermitted( int inContainerID, int inContainedID ) {
    ObjectRecord *containedO = getObject( inContainedID );
    
    char *contLoc = strstr( containedO->description, "+cont" );
    
    if( contLoc == NULL ) {
        // not a limited containable object
        return true;
        }
    
    char *limitNameLoc = &( contLoc[5] );
    
    if( limitNameLoc[0] != ' ' &&
        limitNameLoc[0] != '\0' ) {

        // there's something after +cont
        // scan the whole thing, including +cont

        char tag[100];
        
        int numRead = sscanf( contLoc, "%99s", tag );
        
        if( numRead == 1 ) {
            
            // clean up # character that might delimit end of string
            int tagLen = strlen( tag );
            
            for( int i=0; i<tagLen; i++ ) {
                if( tag[i] == '#' ) {
                    tag[i] = '\0';
                    tagLen = i;
                    break;
                    }
                }

            char *locInContainerName =
                strstr( getObject( inContainerID )->description, tag );
            
            if( locInContainerName != NULL ) {
                // skip to end of tag
                // and make sure tag isn't a sub-tag of container tag
                // don't want contained to be +contHot
                // and contaienr to be +contHotPlates
                
                char end = locInContainerName[ tagLen ];
                
                if( end == ' ' ||
                    end == '\0'||
                    end == '#' ) {
                    return true;
                    }
                }
            return false;
            }
        }
    
    // +cont with nothing after it, no limit
    return true;
    }

        




// swap indicates that we want to put the held item at the bottom
// of the container and take the top one
// returns true if added
static char addHeldToContainer( LiveObject *inPlayer,
                                int inTargetID,
                                int inContX, int inContY,
                                char inSwap = false ) {
    
    int target = inTargetID;
        
    int targetSlots = 
        getNumContainerSlots( target );
                                        
    ObjectRecord *targetObj =
        getObject( target );
    
    if( isGrave( target ) ) {
        return false;
        }
    if( targetObj->slotsLocked ) {
        return false;
        }

    float slotSize =
        targetObj->slotSize;
    
    float containSize =
        getObject( 
            inPlayer->holdingID )->
        containSize;

    int numIn = 
        getNumContained( inContX, inContY );

    
    int isRoom = false;
    

    if( numIn < targetSlots ) {
        isRoom = true;
        }
    else {
        // container full
        // but check if swap is possible

        if( inSwap ) {
            
            int idToAdd = inPlayer->holdingID;
            TransRecord *r = getPTrans( idToAdd, -1 );

            if( r != NULL && r->newActor == 0 && r->newTarget > 0 ) {
                idToAdd = r->newTarget;
                }
            
            int swapInd = getContainerSwapIndex ( inPlayer,
                                                  idToAdd,
                                                  true,
                                                  numIn,
                                                  inContX, inContY );
            if( swapInd != -1 ) {
                isRoom = true;
                }
            }
        }
    


    
    if( isRoom &&
        isContainable( 
            inPlayer->holdingID ) &&
        containSize <= slotSize &&
        containmentPermitted( inTargetID, inPlayer->holdingID ) ) {
        
        // add to container
        
        setResponsiblePlayer( 
            inPlayer->id );
        

        // adding something to a container acts like a drop
        // but some non-permanent held objects are supposed to go through 
        // a transition when they drop (example:  held wild piglet is
        // non-permanent, so it can be containable, but it supposed to
        // switch to a moving wild piglet when dropped.... we should
        // switch to this other wild piglet when putting it into a container
        // too)

        // "set-down" type bare ground
        // trans exists for what we're 
        // holding?
        TransRecord *r = getPTrans( inPlayer->holdingID, -1 );

        if( r != NULL && r->newActor == 0 && r->newTarget > 0 ) {
                                            
            // only applies if the 
            // bare-ground
            // trans leaves nothing in
            // our hand
            
            // first, change what they
            // are holding to this 
            // newTarget
            

            handleHoldingChange( 
                inPlayer,
                r->newTarget );
            }


        int idToAdd = inPlayer->holdingID;


        float stretch = getObject( idToAdd )->slotTimeStretch;
                    
                    

        if( inPlayer->numContained > 0 ) {
            // negative to indicate sub-container
            idToAdd *= -1;
            }

        

        addContained( 
            inContX, inContY,
            idToAdd,
            inPlayer->holdingEtaDecay );

        if( inPlayer->numContained > 0 ) {
            timeSec_t curTime = Time::timeSec();
            
            for( int c=0; c<inPlayer->numContained; c++ ) {
                
                // undo decay stretch before adding
                // (stretch applied by adding)
                if( stretch != 1.0 &&
                    inPlayer->containedEtaDecays[c] != 0 ) {
                
                    timeSec_t offset = 
                        inPlayer->containedEtaDecays[c] - curTime;
                    
                    offset = offset * stretch;
                    
                    inPlayer->containedEtaDecays[c] = curTime + offset;
                    }


                addContained( inContX, inContY, inPlayer->containedIDs[c],
                              inPlayer->containedEtaDecays[c],
                              numIn + 1 );
                }
            
            clearPlayerHeldContained( inPlayer );
            }
        

        
        setResponsiblePlayer( -1 );
        
        inPlayer->holdingID = 0;
        inPlayer->holdingEtaDecay = 0;
        inPlayer->heldOriginValid = 0;
        inPlayer->heldOriginX = 0;
        inPlayer->heldOriginY = 0;
        inPlayer->heldTransitionSourceID = -1;

        int numInNow = getNumContained( inContX, inContY );
        
        if( inSwap &&  numInNow > 1 ) {
            
            int swapInd = getContainerSwapIndex( inPlayer, 
                                                 idToAdd,
                                                 false,
                                                 // don't consider top slot
                                                 // where we just put this
                                                 // new item
                                                 numInNow - 1,
                                                 inContX, inContY );
            if( swapInd != -1 ) {
                // found one to swap
                removeFromContainerToHold( inPlayer, inContX, inContY, 
                                           swapInd );
                }
            // if we didn't remove one, it means whole container is full
            // of identical items.
            // the swap action doesn't work, so we just let it
            // behave like an add action instead.
            }

        return true;
        }

    return false;
    }



// returns true if succeeded
char removeFromContainerToHold( LiveObject *inPlayer, 
                                int inContX, int inContY,
                                int inSlotNumber ) {
    inPlayer->heldOriginValid = 0;
    inPlayer->heldOriginX = 0;
    inPlayer->heldOriginY = 0;                        
    inPlayer->heldTransitionSourceID = -1;
    

    if( isGridAdjacent( inContX, inContY,
                        inPlayer->xd, 
                        inPlayer->yd ) 
        ||
        ( inContX == inPlayer->xd &&
          inContY == inPlayer->yd ) ) {
                            
        inPlayer->actionAttempt = 1;
        inPlayer->actionTarget.x = inContX;
        inPlayer->actionTarget.y = inContY;
                            
        if( inContX > inPlayer->xd ) {
            inPlayer->facingOverride = 1;
            }
        else if( inContX < inPlayer->xd ) {
            inPlayer->facingOverride = -1;
            }

        // can only use on targets next to us for now,
        // no diags
                            
        int target = getMapObject( inContX, inContY );
                            
        if( target != 0 ) {
                            
            if( target > 0 && getObject( target )->slotsLocked ) {
                return false;
                }

            int numIn = 
                getNumContained( inContX, inContY );
                                
            int toRemoveID = 0;
            
            double playerAge = computeAge( inPlayer );

            
            if( inSlotNumber < 0 ) {
                inSlotNumber = numIn - 1;
                
                // no slot specified
                // find top-most object that they can actually pick up

                int toRemoveID = getContained( 
                    inContX, inContY,
                    inSlotNumber );
                
                if( toRemoveID < 0 ) {
                    toRemoveID *= -1;
                    }
                
                while( inSlotNumber > 0 &&
                       getObject( toRemoveID )->minPickupAge >
                       playerAge )  {
            
                    inSlotNumber--;
                    
                    toRemoveID = getContained( 
                        inContX, inContY,
                        inSlotNumber );
                
                    if( toRemoveID < 0 ) {
                        toRemoveID *= -1;
                        }
                    }
                }
            


                                
            if( numIn > 0 ) {
                toRemoveID = getContained( inContX, inContY, inSlotNumber );
                }
            
            char subContain = false;
            
            if( toRemoveID < 0 ) {
                toRemoveID *= -1;
                subContain = true;
                }

            
            if( toRemoveID == 0 ) {
                // this should never happen, except due to map corruption
                
                // clear container, to be safe
                clearAllContained( inContX, inContY );
                return false;
                }


            if( inPlayer->holdingID == 0 && 
                numIn > 0 &&
                // old enough to handle it
                getObject( toRemoveID )->minPickupAge <= 
                computeAge( inPlayer ) ) {
                // get from container


                if( subContain ) {
                    int subSlotNumber = inSlotNumber;
                    
                    if( subSlotNumber == -1 ) {
                        subSlotNumber = numIn - 1;
                        }

                    inPlayer->containedIDs =
                        getContained( inContX, inContY, 
                                      &( inPlayer->numContained ), 
                                      subSlotNumber + 1 );
                    inPlayer->containedEtaDecays =
                        getContainedEtaDecay( inContX, inContY, 
                                              &( inPlayer->numContained ), 
                                              subSlotNumber + 1 );

                    // these will be cleared when removeContained is called
                    // for this slot below, so just get them now without clearing

                    // empty vectors... there are no sub-sub containers
                    inPlayer->subContainedIDs = 
                        new SimpleVector<int>[ inPlayer->numContained ];
                    inPlayer->subContainedEtaDecays = 
                        new SimpleVector<timeSec_t>[ inPlayer->numContained ];
                
                    }
                
                
                setResponsiblePlayer( - inPlayer->id );
                
                inPlayer->holdingID =
                    removeContained( 
                        inContX, inContY, inSlotNumber,
                        &( inPlayer->holdingEtaDecay ) );
                

                // does bare-hand action apply to this newly-held object
                // one that results in something new in the hand and
                // nothing on the ground?

                // if so, it is a pick-up action, and it should apply here

                TransRecord *pickupTrans = getPTrans( 0, inPlayer->holdingID );
                
                if( pickupTrans != NULL && pickupTrans->newActor > 0 &&
                    pickupTrans->newTarget == 0 ) {
                    
                    handleHoldingChange( inPlayer, pickupTrans->newActor );
                    }
                else {
                    holdingSomethingNew( inPlayer );
                    }
                
                setResponsiblePlayer( -1 );

                if( inPlayer->holdingID < 0 ) {
                    // sub-contained
                    
                    inPlayer->holdingID *= -1;    
                    }
                
                // contained objects aren't animating
                // in a way that needs to be smooth
                // transitioned on client
                inPlayer->heldOriginValid = 0;
                inPlayer->heldOriginX = 0;
                inPlayer->heldOriginY = 0;

                return true;
                }
            }
        }        
    
    return false;
    }



// outCouldHaveGoneIn, if non-NULL, is set to TRUE if clothing
// could potentialy contain what we're holding (even if clothing too full
// to contain it)
static char addHeldToClothingContainer( LiveObject *inPlayer, 
                                        int inC,
                                        // true if we should over-pack
                                        // container in anticipation of a swap
                                        char inWillSwap = false,
                                        char *outCouldHaveGoneIn = NULL ) {    
    // drop into own clothing
    ObjectRecord *cObj = 
        clothingByIndex( 
            inPlayer->clothing,
            inC );
                                    
    if( cObj != NULL &&
        isContainable( 
            inPlayer->holdingID ) ) {
                                        
        int oldNum =
            inPlayer->
            clothingContained[inC].size();
                                        
        float slotSize =
            cObj->slotSize;
                                        
        float containSize =
            getObject( inPlayer->holdingID )->
            containSize;
    
        char permitted = false;
        
        if( containSize <= slotSize &&
            cObj->numSlots > 0 &&
            containmentPermitted( cObj->id, inPlayer->holdingID ) ) {
            permitted = true;
            }
        
        if( containSize <= slotSize &&
            cObj->numSlots > 0 &&
            permitted &&
            outCouldHaveGoneIn != NULL ) {
            *outCouldHaveGoneIn = true;
            }

        if( ( oldNum < cObj->numSlots
              || ( oldNum == cObj->numSlots && inWillSwap ) )
            &&
            containSize <= slotSize &&
            permitted ) {
            // room (or will swap, so we can over-pack it)
            inPlayer->clothingContained[inC].
                push_back( 
                    inPlayer->holdingID );

            if( inPlayer->
                holdingEtaDecay != 0 ) {
                                                
                timeSec_t curTime = 
                    Time::timeSec();
                                            
                timeSec_t offset = 
                    inPlayer->
                    holdingEtaDecay - 
                    curTime;
                                            
                offset = 
                    offset / 
                    cObj->
                    slotTimeStretch;
                                                
                inPlayer->holdingEtaDecay =
                    curTime + offset;
                }
                                            
            inPlayer->
                clothingContainedEtaDecays[inC].
                push_back( inPlayer->
                           holdingEtaDecay );
                                        
            inPlayer->holdingID = 0;
            inPlayer->holdingEtaDecay = 0;
            inPlayer->heldOriginValid = 0;
            inPlayer->heldOriginX = 0;
            inPlayer->heldOriginY = 0;
            inPlayer->heldTransitionSourceID =
                -1;

            return true;
            }
        }

    return false;
    }


static void setHeldGraveOrigin( LiveObject *inPlayer, int inX, int inY,
                                int inNewTarget ) {
    // make sure that there is nothing left there
    // for now, transitions that remove graves leave nothing behind
    if( inNewTarget == 0 ) {
        
        // make sure that that there was a grave there before
        int gravePlayerID = getGravePlayerID( inX, inY );
        
        if( gravePlayerID > 0 ) {
            
            // player action actually picked up this grave
            
            inPlayer->heldGraveOriginX = inX;
            inPlayer->heldGraveOriginY = inY;
            
            inPlayer->heldGravePlayerID = getGravePlayerID( inX, inY );
            
            // clear it from ground
            setGravePlayerID( inX, inY, 0 );
            }
        }
    
    }



static void pickupToHold( LiveObject *inPlayer, int inX, int inY, 
                          int inTargetID ) {
    inPlayer->holdingEtaDecay = 
        getEtaDecay( inX, inY );
    
    FullMapContained f =
        getFullMapContained( inX, inY );
    
    setContained( inPlayer, f );
    
    clearAllContained( inX, inY );
    
    setResponsiblePlayer( - inPlayer->id );
    setMapObject( inX, inY, 0 );
    setResponsiblePlayer( -1 );
    
    inPlayer->holdingID = inTargetID;
    holdingSomethingNew( inPlayer );

    setHeldGraveOrigin( inPlayer, inX, inY, 0 );
    
    inPlayer->heldOriginValid = 1;
    inPlayer->heldOriginX = inX;
    inPlayer->heldOriginY = inY;
    inPlayer->heldTransitionSourceID = -1;
    }


// returns true if it worked
static char removeFromClothingContainerToHold( LiveObject *inPlayer,
                                               int inC,
                                               int inI = -1 ) {    
    
    ObjectRecord *cObj = 
        clothingByIndex( inPlayer->clothing, 
                         inC );
                                
    float stretch = 1.0f;
    
    if( cObj != NULL ) {
        stretch = cObj->slotTimeStretch;
        }
    
    int oldNumContained = 
        inPlayer->clothingContained[inC].size();

    int slotToRemove = inI;

    double playerAge = computeAge( inPlayer );

                                
    if( slotToRemove < 0 ) {
        slotToRemove = oldNumContained - 1;

        // no slot specified
        // find top-most object that they can actually pick up

        while( slotToRemove > 0 &&
               getObject( inPlayer->clothingContained[inC].
                          getElementDirect( slotToRemove ) )->minPickupAge >
               playerAge ) {
            
            slotToRemove --;
            }
        }
                                
    int toRemoveID = -1;
                                
    if( oldNumContained > 0 &&
        oldNumContained > slotToRemove &&
        slotToRemove >= 0 ) {
                                    
        toRemoveID = 
            inPlayer->clothingContained[inC].
            getElementDirect( slotToRemove );
        }

    if( oldNumContained > 0 &&
        oldNumContained > slotToRemove &&
        slotToRemove >= 0 &&
        // old enough to handle it
        getObject( toRemoveID )->minPickupAge <= playerAge ) {
                                    

        inPlayer->holdingID = 
            inPlayer->clothingContained[inC].
            getElementDirect( slotToRemove );
        holdingSomethingNew( inPlayer );

        inPlayer->holdingEtaDecay = 
            inPlayer->
            clothingContainedEtaDecays[inC].
            getElementDirect( slotToRemove );
                                    
        timeSec_t curTime = Time::timeSec();

        if( inPlayer->holdingEtaDecay != 0 ) {
                                        
            timeSec_t offset = 
                inPlayer->holdingEtaDecay
                - curTime;
            offset = offset * stretch;
            inPlayer->holdingEtaDecay =
                curTime + offset;
            }

        inPlayer->clothingContained[inC].
            deleteElement( slotToRemove );
        inPlayer->clothingContainedEtaDecays[inC].
            deleteElement( slotToRemove );

        inPlayer->heldOriginValid = 0;
        inPlayer->heldOriginX = 0;
        inPlayer->heldOriginY = 0;
        inPlayer->heldTransitionSourceID = -1;
        return true;
        }
    
    return false;
    }



static ObjectRecord **getClothingSlot( LiveObject *targetPlayer, int inIndex ) {
    
    ObjectRecord **clothingSlot = NULL;    


    if( inIndex == 2 &&
        targetPlayer->clothing.frontShoe != NULL ) {
        clothingSlot = 
            &( targetPlayer->clothing.frontShoe );
        }
    else if( inIndex == 3 &&
             targetPlayer->clothing.backShoe 
             != NULL ) {
        clothingSlot = 
            &( targetPlayer->clothing.backShoe );
        }
    else if( inIndex == 0 && 
             targetPlayer->clothing.hat != NULL ) {
        clothingSlot = 
            &( targetPlayer->clothing.hat );
        }
    else if( inIndex == 1 &&
             targetPlayer->clothing.tunic 
             != NULL ) {
        clothingSlot = 
            &( targetPlayer->clothing.tunic );
        }
    else if( inIndex == 4 &&
             targetPlayer->clothing.bottom 
             != NULL ) {
        clothingSlot = 
            &( targetPlayer->clothing.bottom );
        }
    else if( inIndex == 5 &&
             targetPlayer->
             clothing.backpack != NULL ) {
        clothingSlot = 
            &( targetPlayer->clothing.backpack );
        }
    
    return clothingSlot;
    }

    

static void removeClothingToHold( LiveObject *nextPlayer, 
                                  LiveObject *targetPlayer,
                                  ObjectRecord **clothingSlot,
                                  int clothingSlotIndex ) {
    int ind = clothingSlotIndex;
    
    nextPlayer->holdingID =
        ( *clothingSlot )->id;
    holdingSomethingNew( nextPlayer );
    
    *clothingSlot = NULL;
    nextPlayer->holdingEtaDecay =
        targetPlayer->clothingEtaDecay[ind];
    targetPlayer->clothingEtaDecay[ind] = 0;
    
    nextPlayer->numContained =
        targetPlayer->
        clothingContained[ind].size();
    
    freePlayerContainedArrays( nextPlayer );
    
    nextPlayer->containedIDs =
        targetPlayer->
        clothingContained[ind].
        getElementArray();
    
    targetPlayer->clothingContained[ind].
        deleteAll();
    
    nextPlayer->containedEtaDecays =
        targetPlayer->
        clothingContainedEtaDecays[ind].
        getElementArray();
    
    targetPlayer->
        clothingContainedEtaDecays[ind].
        deleteAll();
    
    // empty sub contained in clothing
    nextPlayer->subContainedIDs =
        new SimpleVector<int>[
            nextPlayer->numContained ];
    
    nextPlayer->subContainedEtaDecays =
        new SimpleVector<timeSec_t>[
            nextPlayer->numContained ];
    
    
    nextPlayer->heldOriginValid = 0;
    nextPlayer->heldOriginX = 0;
    nextPlayer->heldOriginY = 0;
    }



static TransRecord *getBareHandClothingTrans( LiveObject *nextPlayer,
                                              ObjectRecord **clothingSlot ) {
    TransRecord *bareHandClothingTrans = NULL;
    
    if( clothingSlot != NULL ) {
        bareHandClothingTrans =
            getPTrans( 0, ( *clothingSlot )->id );
                                    
        if( bareHandClothingTrans != NULL ) {
            int na =
                bareHandClothingTrans->newActor;
            
            if( na > 0 &&
                getObject( na )->minPickupAge >
                computeAge( nextPlayer ) ) {
                // too young for trans
                bareHandClothingTrans = NULL;
                }
            
            if( bareHandClothingTrans != NULL ) {
                int nt = 
                    bareHandClothingTrans->
                    newTarget;
                
                if( nt > 0 &&
                    getObject( nt )->clothing 
                    == 'n' ) {
                    // don't allow transitions
                    // that leave a non-wearable
                    // item on your body
                    bareHandClothingTrans = NULL;
                    }
                }
            }
        }
    
    return bareHandClothingTrans;
    }




// change held as the result of a transition
static void handleHoldingChange( LiveObject *inPlayer, int inNewHeldID ) {
    
    LiveObject *nextPlayer = inPlayer;

    int oldHolding = nextPlayer->holdingID;
    
    int oldContained = 
        nextPlayer->numContained;
    
    
    nextPlayer->heldOriginValid = 0;
    nextPlayer->heldOriginX = 0;
    nextPlayer->heldOriginY = 0;
    
    // can newly changed container hold
    // less than what it could contain
    // before?
    
    int newHeldSlots = getNumContainerSlots( inNewHeldID );
    
    if( newHeldSlots < oldContained ) {
        // new container can hold less
        // truncate
                            
        GridPos dropPos = 
            getPlayerPos( inPlayer );
                            
        // offset to counter-act offsets built into
        // drop code
        dropPos.x += 1;
        dropPos.y += 1;
        
        char found = false;
        GridPos spot;
        
        if( getMapObject( dropPos.x, dropPos.y ) == 0 ) {
            spot = dropPos;
            found = true;
            }
        else {
            found = findDropSpot( 
                dropPos.x, dropPos.y,
                dropPos.x, dropPos.y,
                &spot );
            }
        
        
        if( found ) {
            
            // throw it on map temporarily
            handleDrop( 
                spot.x, spot.y, 
                inPlayer,
                // only temporary, don't worry about blocking players
                // with this drop
                NULL );
                                

            // responsible player for stuff thrown on map by shrink
            setResponsiblePlayer( inPlayer->id );

            // shrink contianer on map
            shrinkContainer( spot.x, spot.y, 
                             newHeldSlots );
            
            setResponsiblePlayer( -1 );
            

            // pick it back up
            nextPlayer->holdingEtaDecay = 
                getEtaDecay( spot.x, spot.y );
                                    
            FullMapContained f =
                getFullMapContained( spot.x, spot.y );

            setContained( inPlayer, f );
            
            clearAllContained( spot.x, spot.y );
            setMapObject( spot.x, spot.y, 0 );
            }
        else {
            // no spot to throw it
            // cannot leverage map's container-shrinking
            // just truncate held container directly
            
            // truncated contained items will be lost
            inPlayer->numContained = newHeldSlots;
            }
        }

    nextPlayer->holdingID = inNewHeldID;
    holdingSomethingNew( inPlayer, oldHolding );

    if( newHeldSlots > 0 && 
        oldHolding != 0 ) {
                                        
        restretchDecays( 
            newHeldSlots,
            nextPlayer->containedEtaDecays,
            oldHolding,
            nextPlayer->holdingID );
        }
    
    
    if( oldHolding != inNewHeldID ) {
            
        char kept = false;

        // keep old decay timeer going...
        // if they both decay to the same thing in the same time
        if( oldHolding > 0 && inNewHeldID > 0 ) {
            
            TransRecord *oldDecayT = getMetaTrans( -1, oldHolding );
            TransRecord *newDecayT = getMetaTrans( -1, inNewHeldID );
            
            if( oldDecayT != NULL && newDecayT != NULL ) {
                if( oldDecayT->autoDecaySeconds == newDecayT->autoDecaySeconds
                    && 
                    oldDecayT->newTarget == newDecayT->newTarget ) {
                    
                    kept = true;
                    }
                }
            }
        if( !kept ) {
            setFreshEtaDecayForHeld( nextPlayer );
            }
        }

    }



static unsigned char *makeCompressedMessage( char *inMessage, int inLength,
                                             int *outLength ) {
    
    int compressedSize;
    unsigned char *compressedData =
        zipCompress( (unsigned char*)inMessage, inLength, &compressedSize );



    char *header = autoSprintf( "CM\n%d %d\n#", 
                                inLength,
                                compressedSize );
    int headerLength = strlen( header );
    int fullLength = headerLength + compressedSize;
    
    unsigned char *fullMessage = new unsigned char[ fullLength ];
    
    memcpy( fullMessage, (unsigned char*)header, headerLength );
    
    memcpy( &( fullMessage[ headerLength ] ), compressedData, compressedSize );

    delete [] compressedData;
    
    *outLength = fullLength;
    
    delete [] header;
    
    return fullMessage;
    }



static int maxUncompressedSize = 256;


void sendMessageToPlayer( LiveObject *inPlayer, 
                                 char *inMessage, int inLength ) {
    if( ! inPlayer->connected ) {
        // stop sending messages to disconnected players
        return;
        }
    
    
    unsigned char *message = (unsigned char*)inMessage;
    int len = inLength;
    
    char deleteMessage = false;

    if( inLength > maxUncompressedSize ) {
        message = makeCompressedMessage( inMessage, inLength, &len );
        deleteMessage = true;
        }

    int numSent = 
        inPlayer->sock->send( message, 
                              len, 
                              false, false );
        
    if( numSent != len ) {
        setPlayerDisconnected( inPlayer, "Socket write failed" );
        }

    inPlayer->gotPartOfThisFrame = true;
    
    if( deleteMessage ) {
        delete [] message;
        }
    }
    


void readPhrases( const char *inSettingsName, 
                  SimpleVector<char*> *inList ) {
    char *cont = SettingsManager::getSettingContents( inSettingsName, "" );
    
    if( strcmp( cont, "" ) == 0 ) {
        delete [] cont;
        return;    
        }
    
    int numParts;
    char **parts = split( cont, "\n", &numParts );
    delete [] cont;
    
    for( int i=0; i<numParts; i++ ) {
        if( strcmp( parts[i], "" ) != 0 ) {
            inList->push_back( stringToUpperCase( parts[i] ) );
            }
        delete [] parts[i];
        }
    delete [] parts;
    }



// returns pointer to name in string
char *isNamingSay( char *inSaidString, SimpleVector<char*> *inPhraseList ) {
    char *saidString = inSaidString;
    
    if( saidString[0] == ':' ) {
        // first : indicates reading a written phrase.
        // reading written phrase aloud does not have usual effects
        // (block curse exploit)
        return NULL;
        }
    
    for( int i=0; i<inPhraseList->size(); i++ ) {
        char *testString = inPhraseList->getElementDirect( i );
        
        if( strstr( inSaidString, testString ) == saidString ) {
            // hit
            int phraseLen = strlen( testString );
            // skip spaces after
            while( saidString[ phraseLen ] == ' ' ) {
                phraseLen++;
                }
            return &( saidString[ phraseLen ] );
            }
        }
    return NULL;
    }


// returns newly allocated name, or NULL
// looks for phrases that start with a name
char *isReverseNamingSay( char *inSaidString, 
                          SimpleVector<char*> *inPhraseList ) {
    
    if( inSaidString[0] == ':' ) {
        // first : indicates reading a written phrase.
        // reading written phrase aloud does not have usual effects
        // (block curse exploit)
        return NULL;
        }

    for( int i=0; i<inPhraseList->size(); i++ ) {
        char *testString = inPhraseList->getElementDirect( i );
        
        char *hitLoc = strstr( inSaidString, testString );

        if( hitLoc != NULL ) {

            char *saidDupe = stringDuplicate( inSaidString );

            hitLoc = strstr( saidDupe, testString );

            // back one, to exclude space from name
            if( hitLoc != saidDupe ) {
                hitLoc[-1] = '\0';
                return saidDupe;
                }
            else {
                delete [] saidDupe;
                return NULL;
                }
            }
        }
    return NULL;
    }



char *isBabyNamingSay( char *inSaidString ) {
    return isNamingSay( inSaidString, &nameGivingPhrases );
    }

char *isFamilyNamingSay( char *inSaidString ) {
    return isNamingSay( inSaidString, &familyNameGivingPhrases );
    }

char *isCurseNamingSay( char *inSaidString ) {
    return isNamingSay( inSaidString, &cursingPhrases );
    }

char *isNamedGivingSay( char *inSaidString ) {
    return isReverseNamingSay( inSaidString, &namedGivingPhrases );
    }

char *isInfertilityDeclaringSay( char *inSaidString ) {
    return isNamingSay( inSaidString, &infertilityDeclaringPhrases );
    }

char *isFertilityDeclaringSay( char *inSaidString ) {
    return isNamingSay( inSaidString, &fertilityDeclaringPhrases );
    }



static char isWildcardGivingSay( char *inSaidString,
                                 SimpleVector<char*> *inPhrases ) {
    if( inSaidString[0] == ':' ) {
        // first : indicates reading a written phrase.
        // reading written phrase aloud does not have usual effects
        // (block curse exploit)
        return false;
        }

    for( int i=0; i<inPhrases->size(); i++ ) {
        char *testString = inPhrases->getElementDirect( i );
        
        if( strcmp( inSaidString, testString ) == 0 ) {
            return true;
            }
        }
    return false;
    }


char isYouGivingSay( char *inSaidString ) {
    return isWildcardGivingSay( inSaidString, &youGivingPhrases );
    }


char isYouForgivingSay( char *inSaidString ) {
    return isWildcardGivingSay( inSaidString, &youForgivingPhrases );
    }

// returns pointer into inSaidString
char *isNamedForgivingSay( char *inSaidString ) {
    return isNamingSay( inSaidString, &forgivingPhrases );
    }

//2HOL additions for: password-protected objects
char *isPasswordSettingSay( char *inSaidString ) {
    return isNamingSay( inSaidString, &passwordSettingPhrases );
    }
char *isPasswordInvokingSay( char *inSaidString ) {
    return isNamingSay( inSaidString, &passwordInvokingPhrases );
    }
    

LiveObject *getClosestOtherPlayer( LiveObject *inThisPlayer,
                                   double inMinAge = 0,
                                   char inNameMustBeNULL = false ) {
    GridPos thisPos = getPlayerPos( inThisPlayer );

    // don't consider anyone who is too far away
    double closestDist = 20;
    LiveObject *closestOther = NULL;
    
    for( int j=0; j<players.size(); j++ ) {
        LiveObject *otherPlayer = 
            players.getElement(j);
        
        if( otherPlayer != inThisPlayer &&
            ! otherPlayer->error &&
            computeAge( otherPlayer ) >= inMinAge &&
            ( ! inNameMustBeNULL || otherPlayer->name == NULL ) ) {
                                        
            GridPos otherPos = 
                getPlayerPos( otherPlayer );
            
            double dist =
                distance( thisPos, otherPos );
            
            if( dist < closestDist ) {
                closestDist = dist;
                closestOther = otherPlayer;
                }
            }
        }
    return closestOther;
    }



int readIntFromFile( const char *inFileName, int inDefaultValue ) {
    FILE *f = fopen( inFileName, "r" );
    
    if( f == NULL ) {
        return inDefaultValue;
        }
    
    int val = inDefaultValue;
    
    fscanf( f, "%d", &val );

    fclose( f );

    return val;
    }



typedef struct KillState {
        int killerID;
        int killerWeaponID;
        int targetID;
        double killStartTime;
        double emotStartTime;
        int emotRefreshSeconds;
    } KillState;


SimpleVector<KillState> activeKillStates;




void apocalypseStep() {
    
    double curTime = Time::getCurrentTime();

    if( !apocalypseTriggered ) {
        
        if( apocalypseRequest == NULL &&
            curTime - lastRemoteApocalypseCheckTime > 
            remoteApocalypseCheckInterval ) {

            lastRemoteApocalypseCheckTime = curTime;

            // don't actually send request to reflector if apocalypse
            // not possible locally
            // or if broadcast mode disabled
            if( SettingsManager::getIntSetting( "remoteReport", 0 ) &&
                SettingsManager::getIntSetting( "apocalypsePossible", 0 ) &&
                SettingsManager::getIntSetting( "apocalypseBroadcast", 0 ) ) {

                printf( "Checking for remote apocalypse\n" );
            
                char *url = autoSprintf( "%s?action=check_apocalypse", 
                                         reflectorURL );
        
                apocalypseRequest =
                    new WebRequest( "GET", url, NULL );
            
                delete [] url;
                }
            }
        else if( apocalypseRequest != NULL ) {
            int result = apocalypseRequest->step();

            if( result == -1 ) {
                AppLog::info( 
                    "Apocalypse check:  Request to reflector failed." );
                }
            else if( result == 1 ) {
                // done, have result

                char *webResult = 
                    apocalypseRequest->getResult();
                
                if( strstr( webResult, "OK" ) == NULL ) {
                    AppLog::infoF( 
                        "Apocalypse check:  Bad response from reflector:  %s.",
                        webResult );
                    }
                else {
                    int newApocalypseNumber = lastApocalypseNumber;
                    
                    sscanf( webResult, "%d\n", &newApocalypseNumber );
                
                    if( newApocalypseNumber > lastApocalypseNumber ) {
                        lastApocalypseNumber = newApocalypseNumber;
                        apocalypseTriggered = true;
                        apocalypseRemote = true;
                        AppLog::infoF( 
                            "Apocalypse check:  New remote apocalypse:  %d.",
                            lastApocalypseNumber );
                        SettingsManager::setSetting( "lastApocalypseNumber",
                                                     lastApocalypseNumber );
                        }
                    }
                    
                delete [] webResult;
                }
            
            if( result != 0 ) {
                delete apocalypseRequest;
                apocalypseRequest = NULL;
                }
            }
        }
        


    if( apocalypseTriggered ) {

        if( !apocalypseStarted ) {
            apocalypsePossible = 
                SettingsManager::getIntSetting( "apocalypsePossible", 0 );

            if( !apocalypsePossible ) {
                // settings change since we last looked at it
                apocalypseTriggered = false;
                return;
                }
            
            AppLog::info( "Apocalypse triggerered, starting it" );


            reportArcEnd();
            

            // only broadcast to reflector if apocalypseBroadcast set
            if( !apocalypseRemote &&
                SettingsManager::getIntSetting( "remoteReport", 0 ) &&
                SettingsManager::getIntSetting( "apocalypseBroadcast", 0 ) &&
                apocalypseRequest == NULL && reflectorURL != NULL ) {
                
                AppLog::info( "Apocalypse broadcast set, telling reflector" );

                
                char *reflectorSharedSecret = 
                    SettingsManager::
                    getStringSetting( "reflectorSharedSecret" );
                
                if( reflectorSharedSecret != NULL ) {
                    lastApocalypseNumber++;

                    AppLog::infoF( 
                        "Apocalypse trigger:  New local apocalypse:  %d.",
                        lastApocalypseNumber );

                    SettingsManager::setSetting( "lastApocalypseNumber",
                                                 lastApocalypseNumber );

                    int closestPlayerIndex = -1;
                    double closestDist = 999999999;
                    
                    for( int i=0; i<players.size(); i++ ) {
                        LiveObject *nextPlayer = players.getElement( i );
                        if( !nextPlayer->error ) {
                            
                            double dist = 
                                abs( nextPlayer->xd - apocalypseLocation.x ) +
                                abs( nextPlayer->yd - apocalypseLocation.y );
                            if( dist < closestDist ) {
                                closestPlayerIndex = i;
                                closestDist = dist;
                                }
                            }
                        
                        }
                    char *name = NULL;
                    if( closestPlayerIndex != -1 ) {
                        name = 
                            players.getElement( closestPlayerIndex )->
                            name;
                        }
                    
                    if( name == NULL ) {
                        name = stringDuplicate( "UNKNOWN" );
                        }
                    
                    char *idString = autoSprintf( "%d", lastApocalypseNumber );
                    
                    char *hash = hmac_sha1( reflectorSharedSecret, idString );

                    delete [] idString;

                    char *url = autoSprintf( 
                        "%s?action=trigger_apocalypse"
                        "&id=%d&id_hash=%s&name=%s",
                        reflectorURL, lastApocalypseNumber, hash, name );

                    delete [] hash;
                    delete [] name;
                    
                    printf( "Starting new web request for %s\n", url );
                    
                    apocalypseRequest =
                        new WebRequest( "GET", url, NULL );
                                
                    delete [] url;
                    delete [] reflectorSharedSecret;
                    }
                }


            // send all players the AP message
            const char *message = "AP\n#";
            int messageLength = strlen( message );
            
            for( int i=0; i<players.size(); i++ ) {
                LiveObject *nextPlayer = players.getElement( i );
                if( !nextPlayer->error && nextPlayer->connected ) {
                    
                    int numSent = 
                        nextPlayer->sock->send( 
                            (unsigned char*)message, 
                            messageLength,
                            false, false );
                    
                    nextPlayer->gotPartOfThisFrame = true;
                    
                    if( numSent != messageLength ) {
                        setPlayerDisconnected( nextPlayer, 
                                               "Socket write failed" );
                        }
                    }
                }
            
            apocalypseStartTime = Time::getCurrentTime();
            apocalypseStarted = true;
            postApocalypseStarted = false;
            }

        if( apocalypseRequest != NULL ) {
            
            int result = apocalypseRequest->step();
                

            if( result == -1 ) {
                AppLog::info( 
                    "Apocalypse trigger:  Request to reflector failed." );
                }
            else if( result == 1 ) {
                // done, have result

                char *webResult = 
                    apocalypseRequest->getResult();
                printf( "Apocalypse trigger:  "
                        "Got web result:  '%s'\n", webResult );
                
                if( strstr( webResult, "OK" ) == NULL ) {
                    AppLog::infoF( 
                        "Apocalypse trigger:  "
                        "Bad response from reflector:  %s.",
                        webResult );
                    }
                delete [] webResult;
                }
            
            if( result != 0 ) {
                delete apocalypseRequest;
                apocalypseRequest = NULL;
                }
            }

        if( apocalypseRequest == NULL &&
            Time::getCurrentTime() - apocalypseStartTime >= 8 ) {
            
            if( ! postApocalypseStarted  ) {
                AppLog::infoF( "Enough warning time, %d players still alive",
                               players.size() );
                
                
                double startTime = Time::getCurrentTime();
                
                if( familyDataLogFile != NULL ) {
                    fprintf( familyDataLogFile, "%.2f apocalypse triggered\n",
                             startTime );
                    }
    

                // clear map
                freeMap( true );

                AppLog::infoF( "Apocalypse freeMap took %f sec",
                               Time::getCurrentTime() - startTime );
                wipeMapFiles();

                AppLog::infoF( "Apocalypse wipeMapFiles took %f sec",
                               Time::getCurrentTime() - startTime );
                
                reseedMap( true );

                //!
				oneLifeServer->initMap();
                
                AppLog::infoF( "Apocalypse initMap took %f sec", Time::getCurrentTime() - startTime );
                
                peaceTreaties.deleteAll();
                warPeaceRecords.deleteAll();
                activeKillStates.deleteAll();

                lastRemoteApocalypseCheckTime = curTime;
                
                for( int i=0; i<players.size(); i++ ) {
                    LiveObject *nextPlayer = players.getElement( i );
                    backToBasics( nextPlayer );
                    }
                
                // send everyone update about everyone
                for( int i=0; i<players.size(); i++ ) {
                    LiveObject *nextPlayer = players.getElement( i );
                    nextPlayer->firstMessageSent = false;
                    nextPlayer->firstMapSent = false;
                    nextPlayer->inFlight = false;
                    }

                postApocalypseStarted = true;
                }
            else {
                // make sure all players have gotten map and update
                char allMapAndUpdate = true;
                
                for( int i=0; i<players.size(); i++ ) {
                    LiveObject *nextPlayer = players.getElement( i );
                    if( ! nextPlayer->firstMapSent ) {
                        allMapAndUpdate = false;
                        break;
                        }
                    }
                
                if( allMapAndUpdate ) {
                    
                    // send all players the AD message
                    const char *message = "AD\n#";
                    int messageLength = strlen( message );
            
                    for( int i=0; i<players.size(); i++ ) {
                        LiveObject *nextPlayer = players.getElement( i );
                        if( !nextPlayer->error && nextPlayer->connected ) {
                    
                            int numSent = 
                                nextPlayer->sock->send( 
                                    (unsigned char*)message, 
                                    messageLength,
                                    false, false );
                            
                            nextPlayer->gotPartOfThisFrame = true;
                    
                            if( numSent != messageLength ) {
                                setPlayerDisconnected( nextPlayer, 
                                                       "Socket write failed" );
                                }
                            }
                        }

                    // totally done
                    apocalypseStarted = false;
                    apocalypseTriggered = false;
                    apocalypseRemote = false;
                    postApocalypseStarted = false;
                    }
                }    
            }
        }
    }




void monumentStep() {
    if( monumentCallPending ) {
        
        // send to all players
        for( int i=0; i<players.size(); i++ ) {
            LiveObject *nextPlayer = players.getElement( i );
            // remember it to tell babies about it
            nextPlayer->monumentPosSet = true;
            nextPlayer->lastMonumentPos.x = monumentCallX;
            nextPlayer->lastMonumentPos.y = monumentCallY;
            nextPlayer->lastMonumentID = monumentCallID;
            nextPlayer->monumentPosSent = true;
            
            if( !nextPlayer->error && nextPlayer->connected ) {
                
                char *message = autoSprintf( "MN\n%d %d %d\n#", 
                                             monumentCallX -
                                             nextPlayer->birthPos.x, 
                                             monumentCallY -
                                             nextPlayer->birthPos.y,
                                             hideIDForClient( 
                                                 monumentCallID ) );
                int messageLength = strlen( message );


                int numSent = 
                    nextPlayer->sock->send( 
                        (unsigned char*)message, 
                        messageLength,
                        false, false );
                
                nextPlayer->gotPartOfThisFrame = true;
                
                delete [] message;

                if( numSent != messageLength ) {
                    setPlayerDisconnected( nextPlayer, "Socket write failed" );
                    }
                }
            }

        monumentCallPending = false;
        }
    }




// inPlayerName may be destroyed inside this function
// returns a uniquified name, sometimes newly allocated.
// return value destroyed by caller
char *getUniqueCursableName( char *inPlayerName, char *outSuffixAdded,
                             char inIsEve ) {
    
    char dup = isNameDuplicateForCurses( inPlayerName );
    
    if( ! dup ) {
        *outSuffixAdded = false;

        if( inIsEve ) {
            // make sure Eve doesn't have same last name as any living person
            char firstName[99];
            char lastName[99];
            
            sscanf( inPlayerName, "%s %s", firstName, lastName );
            
            for( int i=0; i<players.size(); i++ ) {
                LiveObject *o = players.getElement( i );
                
                if( ! o->error && o->familyName != NULL &&
                    strcmp( o->familyName, lastName ) == 0 ) {
                    
                    dup = true;
                    break;
                    }
                }
            }
        

        return inPlayerName;
        }    
    
    
    if( false ) {
        // old code, add suffix to make unique

        *outSuffixAdded = true;

        int targetPersonNumber = 1;
        
        char *fullName = stringDuplicate( inPlayerName );

        while( dup ) {
            // try next roman numeral
            targetPersonNumber++;
            
            int personNumber = targetPersonNumber;            
        
            SimpleVector<char> romanNumeralList;
        
            while( personNumber >= 100 ) {
                romanNumeralList.push_back( 'C' );
                personNumber -= 100;
                }
            while( personNumber >= 50 ) {
                romanNumeralList.push_back( 'L' );
                personNumber -= 50;
                }
            while( personNumber >= 40 ) {
                romanNumeralList.push_back( 'X' );
                romanNumeralList.push_back( 'L' );
                personNumber -= 40;
                }
            while( personNumber >= 10 ) {
                romanNumeralList.push_back( 'X' );
                personNumber -= 10;
                }
            while( personNumber >= 9 ) {
                romanNumeralList.push_back( 'I' );
                romanNumeralList.push_back( 'X' );
                personNumber -= 9;
                }
            while( personNumber >= 5 ) {
                romanNumeralList.push_back( 'V' );
                personNumber -= 5;
                }
            while( personNumber >= 4 ) {
                romanNumeralList.push_back( 'I' );
                romanNumeralList.push_back( 'V' );
                personNumber -= 4;
                }
            while( personNumber >= 1 ) {
                romanNumeralList.push_back( 'I' );
                personNumber -= 1;
                }
            
            char *romanString = romanNumeralList.getElementString();

            delete [] fullName;
            
            fullName = autoSprintf( "%s %s", inPlayerName, romanString );
            delete [] romanString;
            
            dup = isNameDuplicateForCurses( fullName );
            }
        
        delete [] inPlayerName;
        
        return fullName;
        }
    else {
        // new code:
        // make name unique by finding close matching name that hasn't been
        // used recently
        
        *outSuffixAdded = false;

        char firstName[99];
        char lastName[99];
        
        int numNames = sscanf( inPlayerName, "%s %s", firstName, lastName );
        
        if( numNames == 1 ) {
            // special case, find a totally unique first name for them
            
            int i = getFirstNameIndex( firstName );

            while( dup ) {

                int nextI;
                
                dup = isNameDuplicateForCurses( getFirstName( i, &nextI ) );
            
                if( dup ) {
                    i = nextI;
                    }
                }
            
            if( dup ) {
                // ran out of names, yikes
                return inPlayerName;
                }
            else {
                delete [] inPlayerName;
                int nextI;
                return stringDuplicate( getFirstName( i, &nextI ) );
                }
            }
        else if( numNames == 2 ) {
            if( inIsEve ) {
                // cycle last names until we find one not used by any
                // family
                
                int i = getLastNameIndex( lastName );
            
                const char *tempLastName = "";
                
                while( dup ) {
                    
                    int nextI;
                    tempLastName = getLastName( i, &nextI );
                    
                    dup = false;

                    for( int j=0; j<players.size(); j++ ) {
                        LiveObject *o = players.getElement( j );
                        
                        if( ! o->error && 
                            o->familyName != NULL &&
                            strcmp( o->familyName, tempLastName ) == 0 ) {
                    
                            dup = true;
                            break;
                            }
                        }
                    
                    if( dup ) {
                        i = nextI;
                        }
                    }
            
                if( dup ) {
                    // ran out of names, yikes
                    return inPlayerName;
                    }
                else {
                    delete [] inPlayerName;
                    return autoSprintf( "%s %s", firstName, tempLastName );
                    }
                }
            else {
                // cycle first names until we find one
                int i = getFirstNameIndex( firstName );
            
                char *tempName = NULL;
                
                while( dup ) {                    
                    if( tempName != NULL ) {
                        delete [] tempName;
                        }
                    
                    int nextI;
                    tempName = autoSprintf( "%s %s", getFirstName( i, &nextI ),
                                            lastName );
                    

                    dup = isNameDuplicateForCurses( tempName );
                    if( dup ) {
                        i = nextI;
                        }
                    }
            
                if( dup ) {
                    // ran out of names, yikes
                    if( tempName != NULL ) {
                        delete [] tempName;
                        }
                    return inPlayerName;
                    }
                else {
                    delete [] inPlayerName;
                    return tempName;
                    }
                }
            }
        else {
            // weird case, name doesn't even have two string parts, give up
            return inPlayerName;
            }
        }
    
    }




typedef struct ForcedEffects {
        // -1 if no emot specified
        int emotIndex;
        int ttlSec;
        
        char foodModifierSet;
        double foodCapModifier;
        
        char feverSet;
        float fever;
    } ForcedEffects;
        


ForcedEffects checkForForcedEffects( int inHeldObjectID ) {
    ForcedEffects e = { -1, 0, false, 1.0, false, 0.0f };
    
    ObjectRecord *o = getObject( inHeldObjectID );
    
    if( o != NULL ) {
        char *emotPos = strstr( o->description, "emot_" );
        
        if( emotPos != NULL ) {
            sscanf( emotPos, "emot_%d_%d", 
                    &( e.emotIndex ), &( e.ttlSec ) );
            }

        char *foodPos = strstr( o->description, "food_" );
        
        if( foodPos != NULL ) {
            int numRead = sscanf( foodPos, "food_%lf", 
                                  &( e.foodCapModifier ) );
            if( numRead == 1 ) {
                e.foodModifierSet = true;
                }
            }

        char *feverPos = strstr( o->description, "fever_" );
        
        if( feverPos != NULL ) {
            int numRead = sscanf( feverPos, "fever_%f", 
                                  &( e.fever ) );
            if( numRead == 1 ) {
                e.feverSet = true;
                }
            }
        }
    
    
    return e;
    }




void setNoLongerDying( LiveObject *inPlayer, 
                       SimpleVector<int> *inPlayerIndicesToSendHealingAbout ) {
    inPlayer->dying = false;
    inPlayer->murderSourceID = 0;
    inPlayer->murderPerpID = 0;
    if( inPlayer->murderPerpEmail != 
        NULL ) {
        delete [] 
            inPlayer->murderPerpEmail;
        inPlayer->murderPerpEmail =
            NULL;
        }
    
    inPlayer->deathSourceID = 0;
    inPlayer->holdingWound = false;
    inPlayer->customGraveID = -1;
    
    inPlayer->emotFrozen = false;
    inPlayer->emotUnfreezeETA = 0;
    
    inPlayer->foodCapModifier = 1.0;
    inPlayer->foodUpdate = true;

    inPlayer->fever = 0;

    if( inPlayer->deathReason 
        != NULL ) {
        delete [] inPlayer->deathReason;
        inPlayer->deathReason = NULL;
        }
                                        
    inPlayerIndicesToSendHealingAbout->
        push_back( 
            getLiveObjectIndex( 
                inPlayer->id ) );
    }



static void checkSickStaggerTime( LiveObject *inPlayer ) {
    ObjectRecord *heldObj = NULL;
    
    if( inPlayer->holdingID > 0 ) {
        heldObj = getObject( inPlayer->holdingID );
        }
    else {
        return;
        }

    
    char isSick = false;
    
    if( strstr(
            heldObj->
            description,
            "sick" ) != NULL ) {
        isSick = true;
        
        // sicknesses override basic death-stagger
        // time.  The person can live forever
        // if they are taken care of until
        // the sickness passes
        
        int staggerTime = 
            SettingsManager::getIntSetting(
                "deathStaggerTime", 20 );
        
        double currentTime = 
            Time::getCurrentTime();
        
        // 10x base stagger time should
        // give them enough time to either heal
        // from the disease or die from its
        // side-effects
        inPlayer->dyingETA = 
            currentTime + 10 * staggerTime;
        }
    
    if( isSick ) {
        // what they have will heal on its own 
        // with time.  Sickness, not wound.
        
        // death source is sickness, not
        // source
        inPlayer->deathSourceID = 
            inPlayer->holdingID;
        
        setDeathReason( inPlayer, 
                        "succumbed",
                        inPlayer->holdingID );
        }
    }



typedef struct FlightDest {
        int playerID;
        GridPos destPos;
    } FlightDest;
        



// inEatenID = 0 for nursing
static void checkForFoodEatingEmot( LiveObject *inPlayer,
                                    int inEatenID ) {
    
    char wasStarving = inPlayer->starving;
    inPlayer->starving = false;

    
    if( inEatenID > 0 ) {
        
        ObjectRecord *o = getObject( inEatenID );
        
        if( o != NULL ) {
            char *emotPos = strstr( o->description, "emotEat_" );
            
            if( emotPos != NULL ) {
                int e, t;
                int numRead = sscanf( emotPos, "emotEat_%d_%d", &e, &t );
                
                if( numRead == 2 && !inPlayer->emotFrozen ) {
                    inPlayer->emotFrozen = true;
                    inPlayer->emotFrozenIndex = e;
                    
                    inPlayer->emotUnfreezeETA = Time::getCurrentTime() + t;
                    
                    newEmotPlayerIDs.push_back( inPlayer->id );
                    newEmotIndices.push_back( e );
                    newEmotTTLs.push_back( t );
                    return;
                    }
                }
            }
        }

    // no food emot found
    if( wasStarving && !inPlayer->emotFrozen ) {
        // clear their starving emot
        newEmotPlayerIDs.push_back( inPlayer->id );
        newEmotIndices.push_back( -1 );
        newEmotTTLs.push_back( 0 );
        }
                
    }
    
static void drinkAlcohol( LiveObject *inPlayer, int inAlcoholAmount ) {
    double doneGrowingAge = 16;
    
    double multiplier = 1.0;
    

    double age = computeAge( inPlayer );
    
    // alcohol affects a baby 2x
    // affects an 8-y-o 1.5x
    if( age < doneGrowingAge ) {
        multiplier += 1.0 - age / doneGrowingAge;
        }

    double amount = inAlcoholAmount * multiplier;
    
    inPlayer->drunkenness += amount;
	
	if( inPlayer->drunkenness >= 6 ) {
		
		double drunkennessEffectDuration = 60.0;
		
		inPlayer->drunkennessEffectETA = Time::getCurrentTime() + drunkennessEffectDuration;
		inPlayer->drunkennessEffect = true;
		
		makePlayerSay( inPlayer, (char*)"+DRUNK+", true );
		
		}
    }


static void doDrug( LiveObject *inPlayer ) {
	
	double trippingEffectDelay = 15.0;
	double trippingEffectDuration = 30.0;
	double curTime = Time::getCurrentTime();
	
	if( !inPlayer->tripping && !inPlayer->gonnaBeTripping ) {
		inPlayer->gonnaBeTripping = true;
		inPlayer->trippingEffectStartTime = curTime + trippingEffectDelay;
		inPlayer->trippingEffectETA = curTime + trippingEffectDelay + trippingEffectDuration;
		}
	else if( !inPlayer->tripping && inPlayer->gonnaBeTripping ) {
		// Half the delay if they keep munching drug before effect hits
		float remainingDelay = inPlayer->trippingEffectStartTime - curTime;
		if( remainingDelay > 0 ) {
			inPlayer->trippingEffectStartTime = curTime + 0.5 * remainingDelay;
			}
		}
	else {
		// Refresh duration if they are already tripping
		inPlayer->trippingEffectETA = curTime + trippingEffectDuration;
		}
		
    }
	
	
// returns true if frozen emote cleared successfully
static bool clearFrozenEmote( LiveObject *inPlayer, int inEmoteIndex ) {
	
	if( inPlayer->emotFrozen &&
		inPlayer->emotFrozenIndex == inEmoteIndex ) {
			
		inPlayer->emotFrozen = false;
		inPlayer->emotUnfreezeETA = 0;
		
		newEmotPlayerIDs.push_back( inPlayer->id );
		newEmotIndices.push_back( -1 );
		newEmotTTLs.push_back( 0 );
		
		return true;
		}
	
	return false;
    }


// return true if it worked
char addKillState( LiveObject *inKiller, LiveObject *inTarget ) {
    char found = false;
    
    
    if( distance( getPlayerPos( inKiller ), getPlayerPos( inTarget ) )
        > 8 ) {
        // out of range
        return false;
        }
    
    

    for( int i=0; i<activeKillStates.size(); i++ ) {
        KillState *s = activeKillStates.getElement( i );
        
        if( s->killerID == inKiller->id ) {
            found = true;
            s->killerWeaponID = inKiller->holdingID;
            s->targetID = inTarget->id;

            double curTime = Time::getCurrentTime();
            s->emotStartTime = curTime;
            s->killStartTime = curTime;

            s->emotRefreshSeconds = 10;
            break;
            }
        }
    
    if( !found ) {
        // add new
		double curTime = Time::getCurrentTime();
        KillState s = { inKiller->id, 
                        inKiller->holdingID,
                        inTarget->id, 
                        curTime,
						curTime,
                        10 };
        activeKillStates.push_back( s );

        // force target to gasp
        makePlayerSay( inTarget, (char*)"[GASP]" );
        }
    return true;
    }



static void removeKillState( LiveObject *inKiller, LiveObject *inTarget ) {
    for( int i=0; i<activeKillStates.size(); i++ ) {
        KillState *s = activeKillStates.getElement( i );
    
        if( s->killerID == inKiller->id &&
            s->targetID == inTarget->id ) {
            activeKillStates.deleteElement( i );
            
            break;
            }
        }

    if( inKiller != NULL ) {
        // clear their emot
        inKiller->emotFrozen = false;
        inKiller->emotUnfreezeETA = 0;
        
        newEmotPlayerIDs.push_back( inKiller->id );
        
        newEmotIndices.push_back( -1 );
        newEmotTTLs.push_back( 0 );
        }
    
    if( inTarget != NULL &&
        inTarget->emotFrozen &&
        inTarget->emotFrozenIndex == victimEmotionIndex ) {
        
        // inTarget's emot hasn't been replaced, end it
        inTarget->emotFrozen = false;
        inTarget->emotUnfreezeETA = 0;
        
        newEmotPlayerIDs.push_back( inTarget->id );
        
        newEmotIndices.push_back( -1 );
        newEmotTTLs.push_back( 0 );
        }
    }



static void removeAnyKillState( LiveObject *inKiller ) {
    for( int i=0; i<activeKillStates.size(); i++ ) {
        KillState *s = activeKillStates.getElement( i );
    
        if( s->killerID == inKiller->id ) {
            
            LiveObject *target = getLiveObject( s->targetID );
            
            if( target != NULL ) {
                removeKillState( inKiller, target );
                i--;
                }
            }
        }
    }

            



static void interruptAnyKillEmots( int inPlayerID, 
                                   int inInterruptingTTL ) {
    for( int i=0; i<activeKillStates.size(); i++ ) {
        KillState *s = activeKillStates.getElement( i );
        
        if( s->killerID == inPlayerID ) {
            s->emotStartTime = Time::getCurrentTime();
            s->emotRefreshSeconds = inInterruptingTTL;
            break;
            }
        }
    }    



static void setPerpetratorHoldingAfterKill( LiveObject *nextPlayer,
                                            TransRecord *woundHit,
                                            TransRecord *rHit,
                                            TransRecord *r ) {

    int oldHolding = nextPlayer->holdingID;


    if( rHit != NULL ) {
        // if hit trans exist
        // leave bloody knife or
        // whatever in hand
        nextPlayer->holdingID = rHit->newActor;
        holdingSomethingNew( nextPlayer,
                             oldHolding );
        }
    else if( woundHit != NULL ) {
        // result of hit on held weapon 
        // could also be
        // specified in wound trans
        nextPlayer->holdingID = 
            woundHit->newActor;
        holdingSomethingNew( nextPlayer,
                             oldHolding );
        }
    else if( r != NULL ) {
        nextPlayer->holdingID = r->newActor;
        holdingSomethingNew( nextPlayer,
                             oldHolding );
        }
                        
    if( r != NULL || rHit != NULL || woundHit != NULL ) {
        
        nextPlayer->heldTransitionSourceID = 0;
        
        if( oldHolding != 
            nextPlayer->holdingID ) {
            
            setFreshEtaDecayForHeld( 
                nextPlayer );
            }
        }
    }




void executeKillAction( int inKillerIndex,
                        int inTargetIndex,
                        SimpleVector<int> *playerIndicesToSendUpdatesAbout,
                        SimpleVector<int> *playerIndicesToSendDyingAbout,
                        SimpleVector<int> *newEmotPlayerIDs,
                        SimpleVector<int> *newEmotIndices,
                        SimpleVector<int> *newEmotTTLs ) {
    int i = inKillerIndex;
    LiveObject *nextPlayer = players.getElement( inKillerIndex );    

    LiveObject *hitPlayer = players.getElement( inTargetIndex );

    GridPos targetPos = getPlayerPos( hitPlayer );


    // send update even if action fails (to let them
    // know that action is over)
    playerIndicesToSendUpdatesAbout->push_back( i );
                        
    if( nextPlayer->holdingID > 0 ) {
                            
        nextPlayer->actionAttempt = 1;
        nextPlayer->actionTarget.x = targetPos.x;
        nextPlayer->actionTarget.y = targetPos.y;
                            
        if( nextPlayer->actionTarget.x > nextPlayer->xd ) {
            nextPlayer->facingOverride = 1;
            }
        else if( nextPlayer->actionTarget.x < nextPlayer->xd ) {
            nextPlayer->facingOverride = -1;
            }

        // holding something
        ObjectRecord *heldObj = 
            getObject( nextPlayer->holdingID );
                            
        if( heldObj->deadlyDistance > 0 ) {
            // it's deadly

            GridPos playerPos = getPlayerPos( nextPlayer );
                                
            double d = distance( targetPos,
                                 playerPos );
                                
            if( heldObj->deadlyDistance >= d &&
                ! directLineBlocked( playerPos, 
                                     targetPos ) ) {
                // target is close enough
                // and no blocking objects along the way                

                char someoneHit = false;


                if( hitPlayer != NULL &&
                    strstr( heldObj->description,
                            "otherFamilyOnly" ) ) {
                    // make sure victim is in
                    // different family
                    // and no treaty
                                        
                    if( hitPlayer->lineageEveID ==
                        nextPlayer->lineageEveID
                        || 
                        isPeaceTreaty( hitPlayer->lineageEveID,
                                       nextPlayer->lineageEveID ) ) {
                                            
                        hitPlayer = NULL;
                        }
                    }
                

                // special case:
                // non-lethal no_replace ends up in victim's hand
                // they aren't dying, but they may have an emot
                // effect only
                if( hitPlayer != NULL ) {

                    TransRecord *woundHit = 
                        getPTrans( nextPlayer->holdingID, 
                                   0, true, false );

                    if( woundHit != NULL && woundHit->newTarget > 0 &&
                        strstr( getObject( woundHit->newTarget )->description,
                                "no_replace" ) != NULL ) {
                        
                        
                        TransRecord *rHit = 
                            getPTrans( nextPlayer->holdingID, 0, false, true );
                        
                        TransRecord *r = 
                            getPTrans( nextPlayer->holdingID, 0 );

                        setPerpetratorHoldingAfterKill( nextPlayer,
                                                        woundHit, rHit, r );
                        
                        ForcedEffects e = 
                            checkForForcedEffects( woundHit->newTarget );
                            
                        // emote-effect only for no_replace
                        // no fever or food effect
                        if( e.emotIndex != -1 ) {
                            hitPlayer->emotFrozen = 
                                true;
                            hitPlayer->emotFrozenIndex = e.emotIndex;
                            
                            hitPlayer->emotUnfreezeETA =
                                Time::getCurrentTime() + e.ttlSec;
                            
                            newEmotPlayerIDs->push_back( 
                                hitPlayer->id );
                            newEmotIndices->push_back( 
                                e.emotIndex );
                            newEmotTTLs->push_back( 
                                e.ttlSec );

                            interruptAnyKillEmots( hitPlayer->id,
                                                   e.ttlSec );
                            }
                        return;
                        }
                    }
                

                if( hitPlayer != NULL ) {
                    someoneHit = true;
                    // break the connection with 
                    // them, eventually
                    // let them stagger a bit first

                    hitPlayer->murderSourceID =
                        nextPlayer->holdingID;
                                        
                    hitPlayer->murderPerpID =
                        nextPlayer->id;
                                        
                    // brand this player as a murderer
                    nextPlayer->everKilledAnyone = true;

                    if( hitPlayer->murderPerpEmail 
                        != NULL ) {
                        delete [] 
                            hitPlayer->murderPerpEmail;
                        }
                                        
                    hitPlayer->murderPerpEmail =
                        stringDuplicate( 
                            nextPlayer->email );
                                        

                    setDeathReason( hitPlayer, 
                                    "killed",
                                    nextPlayer->holdingID );

                    // if not already dying
                    if( ! hitPlayer->dying ) {
                        int staggerTime = 
                            SettingsManager::getIntSetting(
                                "deathStaggerTime", 20 );
                                            
                        double currentTime = 
                            Time::getCurrentTime();
                                            
                        hitPlayer->dying = true;
                        hitPlayer->dyingETA = 
                            currentTime + staggerTime;

                        playerIndicesToSendDyingAbout->
                            push_back( 
                                getLiveObjectIndex( 
                                    hitPlayer->id ) );
                                        
                        hitPlayer->errorCauseString =
                            "Player killed by other player";
                        }
                    else {
                        // already dying, 
                        // and getting attacked again
                        
                        // halve their remaining 
                        // stagger time
                        double currentTime = 
                            Time::getCurrentTime();
                                             
                        double staggerTimeLeft = 
                            hitPlayer->dyingETA - 
                            currentTime;
                        
                        if( staggerTimeLeft > 0 ) {
                            staggerTimeLeft /= 2;
                            hitPlayer->dyingETA = 
                                currentTime + 
                                staggerTimeLeft;
                            }
                        }
                    }
                                    
                                    
                // a player either hit or not
                // in either case, weapon was used
                                    
                // check for a transition for weapon

                // 0 is generic "on person" target
                TransRecord *r = 
                    getPTrans( nextPlayer->holdingID, 
                               0 );

                TransRecord *rHit = NULL;
                TransRecord *woundHit = NULL;
                                    
                if( someoneHit ) {
                    // last use on target specifies
                    // grave and weapon change on hit
                    // non-last use (r above) specifies
                    // what projectile ends up in grave
                    // or on ground
                    rHit = 
                        getPTrans( nextPlayer->holdingID, 
                                   0, false, true );
                                        
                    if( rHit != NULL &&
                        rHit->newTarget > 0 ) {
                        hitPlayer->customGraveID = 
                            rHit->newTarget;
                        }
                                        
                    char wasSick = false;
                                        
                    if( hitPlayer->holdingID > 0 &&
                        strstr(
                            getObject( 
                                hitPlayer->holdingID )->
                            description,
                            "sick" ) != NULL ) {
                        wasSick = true;
                        }

                    // last use on actor specifies
                    // what is left in victim's hand
                    woundHit = 
                        getPTrans( nextPlayer->holdingID, 
                                   0, true, false );
                                        
                    if( woundHit != NULL &&
                        woundHit->newTarget > 0 ) {
                                            
                        // don't drop their wound
                        if( hitPlayer->holdingID != 0 &&
                            ! hitPlayer->holdingWound ) {
                            handleDrop( 
                                targetPos.x, targetPos.y, 
                                hitPlayer,
                                playerIndicesToSendUpdatesAbout );
                            }

                        // give them a new wound
                        // if they don't already have
                        // one, but never replace their
                        // original wound.  That allows
                        // a healing exploit where you
                        // intentionally give someone
                        // an easier-to-treat wound
                        // to replace their hard-to-treat
                        // wound

                        // however, do let wounds replace
                        // sickness
                        char woundChange = false;
                                            
                        if( ! hitPlayer->holdingWound ||
                            wasSick ) {
                            woundChange = true;
                                                
                            hitPlayer->holdingID = 
                                woundHit->newTarget;
                            holdingSomethingNew( 
                                hitPlayer );
                            setFreshEtaDecayForHeld( 
                                hitPlayer );
                            }
                                            
                                            
                        hitPlayer->holdingWound = true;
                                            
                        if( woundChange ) {
                                                
                            ForcedEffects e = 
                                checkForForcedEffects( 
                                    hitPlayer->holdingID );
                            
                            if( e.emotIndex != -1 ) {
                                hitPlayer->emotFrozen = 
                                    true;
                                hitPlayer->emotFrozenIndex = e.emotIndex;
                                
                                newEmotPlayerIDs->push_back( 
                                    hitPlayer->id );
                                newEmotIndices->push_back( 
                                    e.emotIndex );
                                newEmotTTLs->push_back( 
                                    e.ttlSec );
                                interruptAnyKillEmots( hitPlayer->id,
                                                       e.ttlSec );
                                }
                                            
                            if( e.foodModifierSet && 
                                e.foodCapModifier != 1 ) {
                                                
                                hitPlayer->
                                    foodCapModifier = 
                                    e.foodCapModifier;
                                hitPlayer->foodUpdate = 
                                    true;
                                }
                                                
                            if( e.feverSet ) {
                                hitPlayer->fever = e.fever;
                                }

                            checkSickStaggerTime( 
                                hitPlayer );
                                                
                            playerIndicesToSendUpdatesAbout->
                                push_back( 
                                    getLiveObjectIndex( 
                                        hitPlayer->id ) );
                            }   
                        }
                    }
                                    

                int oldHolding = nextPlayer->holdingID;

                setPerpetratorHoldingAfterKill( nextPlayer, 
                                                woundHit, rHit, r );

                timeSec_t oldEtaDecay = 
                    nextPlayer->holdingEtaDecay;
                                    

                if( r != NULL ) {
                                    
                    if( hitPlayer != NULL &&
                        r->newTarget != 0 ) {
                                        
                        hitPlayer->embeddedWeaponID = 
                            r->newTarget;
                                        
                        if( oldHolding == r->newTarget ) {
                            // what we are holding
                            // is now embedded in them
                            // keep old decay
                            hitPlayer->
                                embeddedWeaponEtaDecay =
                                oldEtaDecay;
                            }
                        else {
                                            
                            TransRecord *newDecayT = 
                                getMetaTrans( 
                                    -1, 
                                    r->newTarget );
                    
                            if( newDecayT != NULL ) {
                                hitPlayer->
                                    embeddedWeaponEtaDecay = 
                                    Time::timeSec() + 
                                    newDecayT->
                                    autoDecaySeconds;
                                }
                            else {
                                // no further decay
                                hitPlayer->
                                    embeddedWeaponEtaDecay 
                                    = 0;
                                }
                            }
                        }
                    else if( hitPlayer == NULL &&
                             isMapSpotEmpty( targetPos.x, 
                                             targetPos.y ) ) {
                        // this is old code, and probably never gets executed
                        
                        // no player hit, and target ground
                        // spot is empty
                        setMapObject( targetPos.x, targetPos.y, 
                                      r->newTarget );
                                        
                        // if we're thowing a weapon
                        // target is same as what we
                        // were holding
                        if( oldHolding == r->newTarget ) {
                            // preserve old decay time 
                            // of what we were holding
                            setEtaDecay( targetPos.x, targetPos.y,
                                         oldEtaDecay );
                            }
                        }
                    // else new target, post-kill-attempt
                    // is lost
                    }
                }
            }
        }
    }




void nameBaby( LiveObject *inNamer, LiveObject *inBaby, char *inName,
               SimpleVector<int> *playerIndicesToSendNamesAbout ) {    

    LiveObject *nextPlayer = inNamer;
    LiveObject *babyO = inBaby;
    
    char *name = inName;
    
    
    const char *lastName = "";
    if( nextPlayer->name != NULL ) {
        lastName = strstr( nextPlayer->name, 
                           " " );
                                        
        if( lastName != NULL ) {
            // skip space
            lastName = &( lastName[1] );
            }

        if( lastName == NULL ) {
            lastName = "";

            if( nextPlayer->familyName != 
                NULL ) {
                lastName = 
                    nextPlayer->familyName;
                }    
            }
        else if( nextPlayer->nameHasSuffix ) {
            // only keep last name
            // if it contains another
            // space (the suffix is after
            // the last name).  Otherwise
            // we are probably confused,
            // and what we think
            // is the last name IS the suffix.
                                            
            char *suffixPos =
                strstr( (char*)lastName, " " );
                                            
            if( suffixPos == NULL ) {
                // last name is suffix, actually
                // don't pass suffix on to baby
                lastName = "";
                }
            else {
                // last name plus suffix
                // okay to pass to baby
                // because we strip off
                // third part of name
                // (suffix) below.
                }
            }
        }
    else if( nextPlayer->familyName != NULL ) {
        lastName = nextPlayer->familyName;
        }
    else if( babyO->familyName != NULL ) {
        lastName = babyO->familyName;
        }
                                    


    const char *close = 
        findCloseFirstName( name );

    if( strcmp( lastName, "" ) != 0 ) {    
        babyO->name = autoSprintf( "%s %s",
                                   close, 
                                   lastName );
        }
    else {
        babyO->name = stringDuplicate( close );
        }
    
    
    if( babyO->familyName == NULL &&
        nextPlayer->familyName != NULL ) {
        // mother didn't have a family 
        // name set when baby was born
        // now she does
        // or whatever player named 
        // this orphaned baby does
        babyO->familyName = 
            stringDuplicate( 
                nextPlayer->familyName );
        }
                                    

    int spaceCount = 0;
    int lastSpaceIndex = -1;

    int nameLen = strlen( babyO->name );
    for( int s=0; s<nameLen; s++ ) {
        if( babyO->name[s] == ' ' ) {
            lastSpaceIndex = s;
            spaceCount++;
            }
        }
                                    
    if( spaceCount > 1 ) {
        // remove suffix from end
        babyO->name[ lastSpaceIndex ] = '\0';
        }
                                    
    babyO->name = getUniqueCursableName( 
        babyO->name, 
        &( babyO->nameHasSuffix ), false );
                                    
    logName( babyO->id,
             babyO->email,
             babyO->name,
             babyO->lineageEveID );
                                    
    playerIndicesToSendNamesAbout->push_back( 
        getLiveObjectIndex( babyO->id ) );
    }




void getLineageLineForPlayer( LiveObject *inPlayer,
                              SimpleVector<char> *inVector ) {
    
    char *pID = autoSprintf( "%d", inPlayer->id );
    inVector->appendElementString( pID );
    delete [] pID;
    
    for( int j=0; j<inPlayer->lineage->size(); j++ ) {
        char *mID = 
            autoSprintf( 
                " %d",
                inPlayer->lineage->getElementDirect( j ) );
        inVector->appendElementString( mID );
        delete [] mID;
        }        
    // include eve tag at end
    char *eveTag = autoSprintf( " eve=%d",
                                inPlayer->lineageEveID );
    inVector->appendElementString( eveTag );
    delete [] eveTag;
    
    inVector->push_back( '\n' );            
    }





void logFitnessDeath( LiveObject *nextPlayer ) {
	
	double curTime = Time::getCurrentTime();
	for( int i=0; i<players.size(); i++ ) {
			
		LiveObject *o = players.getElement( i );
		
		if( o->error ||
			o->isTutorial ||
			o->id == nextPlayer->id ) {
			continue;
		}
		
		SimpleVector<double> *newAncestorLifeEndTimeSeconds = new SimpleVector<double>();

		for( int e=0; e< o->ancestorIDs->size(); e++ ) {
			
			if( o->ancestorIDs->getElementDirect( e ) == nextPlayer->id ) {
				newAncestorLifeEndTimeSeconds->push_back( curTime );
			} else {
				newAncestorLifeEndTimeSeconds->push_back( o->ancestorLifeEndTimeSeconds->getElementDirect( e ) );
			}
		}
			
		for( int e=0; e< o->ancestorIDs->size(); e++ ) {
			o->ancestorLifeEndTimeSeconds->deleteElement( e );
		}
		delete o->ancestorLifeEndTimeSeconds;
		o->ancestorLifeEndTimeSeconds = newAncestorLifeEndTimeSeconds;
		
	}
    
    // log this death for fitness purposes,
    // for both tutorial and non    


    // if this person themselves died before their mom, gma, etc.
    // remove them from the "ancestor" list of everyone who is older than they
    // are and still alive

    // You only get genetic points for ma, gma, and other older ancestors
    // if you are alive when they die.

    // This ends an exploit where people suicide as a baby (or young person)
    // yet reap genetic benefit from their mother living a long life
    // (your mother, gma, etc count for your genetic score if you yourself
    //  live beyond 3, so it is in your interest to protect them)
    double deadPersonAge = computeAge( nextPlayer );
    if( deadPersonAge < forceDeathAge ) {
        for( int i=0; i<players.size(); i++ ) {
                
            LiveObject *o = players.getElement( i );
            
            if( o->error ||
                o->isTutorial ||
                o->id == nextPlayer->id ) {
                continue;
                }
            
            if( computeAge( o ) < deadPersonAge ) {
                // this person was born after the dead person
                // thus, there's no way they are their ma, gma, etc.
                continue;
                }

            for( int e=0; e< o->ancestorIDs->size(); e++ ) {
                if( o->ancestorIDs->getElementDirect( e ) == nextPlayer->id ) {
                    o->ancestorIDs->deleteElement( e );
                    
                    delete [] o->ancestorEmails->getElementDirect( e );
                    o->ancestorEmails->deleteElement( e );
                
                    delete [] o->ancestorRelNames->getElementDirect( e );
                    o->ancestorRelNames->deleteElement( e );
                    
                    o->ancestorLifeStartTimeSeconds->deleteElement( e );
					o->ancestorLifeEndTimeSeconds->deleteElement( e );

                    break;
                    }
                }
            }
        }


    SimpleVector<int> emptyAncestorIDs;
    SimpleVector<char*> emptyAncestorEmails;
    SimpleVector<char*> emptyAncestorRelNames;
    SimpleVector<double> emptyAncestorLifeStartTimeSeconds;
	SimpleVector<double> emptyAncestorLifeEndTimeSeconds;
    

    //SimpleVector<int> *ancestorIDs = nextPlayer->ancestorIDs;
    SimpleVector<char*> *ancestorEmails = nextPlayer->ancestorEmails;
    SimpleVector<char*> *ancestorRelNames = nextPlayer->ancestorRelNames;
    //SimpleVector<double> *ancestorLifeStartTimeSeconds = 
    //    nextPlayer->ancestorLifeStartTimeSeconds;
    SimpleVector<double> *ancestorLifeEndTimeSeconds = 
        nextPlayer->ancestorLifeEndTimeSeconds;   

	SimpleVector<char*> ancestorData;
	double deadPersonLifeStartTime = nextPlayer->trueStartTimeSeconds;
	double ageRate = getAgeRate();
	
	for( int i=0; i<ancestorEmails->size(); i++ ) {
		
		double endTime = ancestorLifeEndTimeSeconds->getElementDirect( i );
		double parentingTime = 0.0;
		
		if( endTime > 0 ) {
			parentingTime = ageRate * (endTime - deadPersonLifeStartTime);
		} else {
			parentingTime = ageRate * (curTime - deadPersonLifeStartTime);
		}
		
		char buffer[16];
		snprintf(buffer, sizeof buffer, "%.6f", parentingTime);
		
		ancestorData.push_back( buffer );
		
	} 

    logFitnessDeath( players.size(),
                     nextPlayer->email, 
                     nextPlayer->name, nextPlayer->displayID,
                     computeAge( nextPlayer ),
                     ancestorEmails, 
                     ancestorRelNames,
					 &ancestorData
					 );
    }

    
    
// access blocked b/c of access direction or ownership?
static char isAccessBlocked( LiveObject *inPlayer, 
                             int inTargetX, int inTargetY,
                             int inTargetID ) {
    int target = inTargetID;
    
    int x = inTargetX;
    int y = inTargetY;
    

    char wrongSide = false;
    char ownershipBlocked = false;
	//2HOL additions for: password-protected objects
	char blockedByPassword = false;
    
    if( target > 0 ) {
        ObjectRecord *targetObj = getObject( target );

        if( isGridAdjacent( x, y,
                            inPlayer->xd, 
                            inPlayer->yd ) ) {
            
            if( targetObj->sideAccess ) {
                
                if( y > inPlayer->yd ||
                    y < inPlayer->yd ) {
                    // access from N or S
                    wrongSide = true;
                    }
                }
            else if( targetObj->noBackAccess ) {
                if( y < inPlayer->yd ) {
                    // access from N
                    wrongSide = true;
                    }
                }
            }
        if( targetObj->isOwned ) {
            // make sure player owns this pos
            ownershipBlocked = 
                ! isOwned( inPlayer, x, y );
            }
			
		//2HOL additions for: password-protected objects
		//the check to block the transition for the object which password was not guessed correctly
		if( passwordTransitionsAllowed && targetObj->canHaveInGamePassword ) {
			// AppLog::infoF( "2HOL DEBUG: attempt to interact with an object potentially having password" );
			// AppLog::infoF( "2HOL DEBUG: there are %i protected tiles with object ID %i in the world", targetObj->IndX.size(), targetObj->id );
			// AppLog::infoF( "2HOL DEBUG: interaction location, x: %i", x );
			// AppLog::infoF( "2HOL DEBUG: interaction location, y: %i", y );
			for( int i=0; i<targetObj->IndX.size(); i++ ) {
				if ( x == targetObj->IndX.getElementDirect(i) && y == targetObj->IndY.getElementDirect(i) ) {
					// AppLog::infoF( "2HOL DEBUG: protected tile #%i, password: %s", i, targetObj->IndPass.getElementDirect(i) );
					if ( inPlayer->saidPassword == NULL ) {
							// AppLog::infoF( "2HOL DEBUG: player didn't say any password." );
							blockedByPassword = true;
					}
					else {
						// AppLog::infoF( "2HOL DEBUG: player's password: %s", inPlayer->saidPassword );
						std::string tryPw( inPlayer->saidPassword );
						std::string truePw( targetObj->IndPass.getElementDirect(i) );
						bool pass = tryPw.compare(truePw) == 0;
						if ( pass ) {
							// AppLog::infoF( "2HOL DEBUG: passwords match." );
							blockedByPassword = false;
							}
						else {
							// AppLog::infoF( "2HOL DEBUG: passwords do not match." );
							blockedByPassword = true;
							}
						}
						break;
					}
				}
			// 2HOL, password-protected objects: or for which the password was not guessed
			if ( blockedByPassword ) {
				 // AppLog::infoF( "2HOL DEBUG: attempt to interact was blocked: wrong password." );
				}
			}
        }
    return wrongSide || ownershipBlocked || blockedByPassword;
    }



// returns NULL if not found
static LiveObject *getPlayerByName( char *inName, 
                                    LiveObject *inPlayerSayingName ) {
    for( int j=0; j<players.size(); j++ ) {
        LiveObject *otherPlayer = players.getElement( j );
        if( ! otherPlayer->error &&
            otherPlayer != inPlayerSayingName &&
            otherPlayer->name != NULL &&
            strcmp( otherPlayer->name, inName ) == 0 ) {
            
            return otherPlayer;
            }
        }
    
    // no exact match.

    // does name contain no space?
    char *spacePos = strstr( inName, " " );

    if( spacePos == NULL ) {
        // try again, using just the first name for each potential target

        // stick a space at the end to forbid matching prefix of someone's name
        char *firstName = autoSprintf( "%s ", inName );

        LiveObject *matchingPlayer = NULL;
        double matchingDistance = DBL_MAX;
        
        GridPos playerPos = getPlayerPos( inPlayerSayingName );
        

        for( int j=0; j<players.size(); j++ ) {
            LiveObject *otherPlayer = players.getElement( j );
            if( ! otherPlayer->error &&
                otherPlayer != inPlayerSayingName &&
                otherPlayer->name != NULL &&
                // does their name start with firstName
                strstr( otherPlayer->name, firstName ) == otherPlayer->name ) {
                
                GridPos pos = getPlayerPos( otherPlayer );            
                double d = distance( pos, playerPos );
                
                if( d < matchingDistance ) {
                    matchingDistance = d;
                    matchingPlayer = otherPlayer;
                    }
                }
            }
        
        delete [] firstName;

        return matchingPlayer;
        }
        

    return NULL;
    }
	
	


static void sendCraving( LiveObject *inPlayer ) {
    // they earn the normal YUM multiplier increase (+1) if food is actually yum PLUS the bonus
    // increase, so send them the total.
    
    int totalBonus = inPlayer->cravingFood.bonus;
    if( isReallyYummy( inPlayer, inPlayer->cravingFood.foodID ) ) totalBonus = totalBonus + 1;
    
    char *message = autoSprintf( "CR\n%d %d\n#", 
                                 inPlayer->cravingFood.foodID,
                                 totalBonus );
    sendMessageToPlayer( inPlayer, message, strlen( message ) );
    delete [] message;

    inPlayer->cravingKnown = true;
    }


int main()
{
	OneLife::server::Settings settings;



    if( checkReadOnly() ) {
        printf( "File system read-only.  Server exiting.\n" );
        return 1;
        }
    
    familyDataLogFile = fopen( "familyDataLog.txt", "a" );

    if( familyDataLogFile != NULL ) {
        fprintf( familyDataLogFile, "%.2f server starting up\n",
                 Time::getCurrentTime() );
        }


    memset( allowedSayCharMap, false, 256 );
    
    int numAllowed = strlen( allowedSayChars );
    for( int i=0; i<numAllowed; i++ ) {
        allowedSayCharMap[ (int)( allowedSayChars[i] ) ] = true;
        }
    

    nextID = 
        SettingsManager::getIntSetting( "nextPlayerID", 2 );


    // make backup and delete old backup every day
    AppLog::setLog( new FileLog( "log.txt", 86400 ) );

    AppLog::setLoggingLevel( Log::DETAIL_LEVEL );
    AppLog::printAllMessages( true );

    printf( "\n" );
    AppLog::info( "Server starting up" );

    printf( "\n" );
    
    
    

    nextSequenceNumber = 
        SettingsManager::getIntSetting( "sequenceNumber", 1 );

    requireClientPassword =
        SettingsManager::getIntSetting( "requireClientPassword", 1 );
    
    requireTicketServerCheck =
        SettingsManager::getIntSetting( "requireTicketServerCheck", 1 );
    
    clientPassword = 
        SettingsManager::getStringSetting( "clientPassword" );


    int dataVer = readIntFromFile( "dataVersionNumber.txt", 1 );
    int codVer = readIntFromFile( "serverCodeVersionNumber.txt", 1 );
    
    versionNumber = dataVer;
    if( codVer > versionNumber ) {
        versionNumber = codVer;
        }
    
    printf( "\n" );
    AppLog::infoF( "Server using version number %d", versionNumber );

    printf( "\n" );
    



    minFoodDecrementSeconds = 
        SettingsManager::getFloatSetting( "minFoodDecrementSeconds", 5.0f );

    maxFoodDecrementSeconds = 
        SettingsManager::getFloatSetting( "maxFoodDecrementSeconds", 20 );

    babyBirthFoodDecrement = 
        SettingsManager::getIntSetting( "babyBirthFoodDecrement", 10 );


    eatBonus = 
        SettingsManager::getIntSetting( "eatBonus", 0 );


    secondsPerYear = 
        SettingsManager::getFloatSetting( "secondsPerYear", 60.0f );
    

    if( clientPassword == NULL ) {
        requireClientPassword = 0;
        }


    ticketServerURL = 
        SettingsManager::getStringSetting( "ticketServerURL" );
    

    if( ticketServerURL == NULL ) {
        requireTicketServerCheck = 0;
        }

    
    reflectorURL = SettingsManager::getStringSetting( "reflectorURL" );

    apocalypsePossible = 
        SettingsManager::getIntSetting( "apocalypsePossible", 0 );

    lastApocalypseNumber = 
        SettingsManager::getIntSetting( "lastApocalypseNumber", 0 );


    childSameRaceLikelihood =
        (double)SettingsManager::getFloatSetting( "childSameRaceLikelihood",
                                                  0.90 );
    
    familySpan =
        SettingsManager::getIntSetting( "familySpan", 2 );
    
    
    readPhrases( "babyNamingPhrases", &nameGivingPhrases );
    readPhrases( "familyNamingPhrases", &familyNameGivingPhrases );

    readPhrases( "cursingPhrases", &cursingPhrases );

    readPhrases( "forgivingPhrases", &forgivingPhrases );
    readPhrases( "forgiveYouPhrases", &youForgivingPhrases );

    
    readPhrases( "youGivingPhrases", &youGivingPhrases );
    readPhrases( "namedGivingPhrases", &namedGivingPhrases );
    
    //2HOL additions for: password-protected objects
    passwordTransitionsAllowed =
        SettingsManager::getIntSetting( "passwordTransitionsAllowed", 0 );
    passwordInvocationAndSettingAreSeparated =
        SettingsManager::getIntSetting( "passwordInvocationAndSettingAreSeparated", 0 );
    passwordOverhearRadius =
        SettingsManager::getIntSetting( "passwordOverhearRadius", 6 );
    if (passwordOverhearRadius == -1) { passwordSilent = 1; }
    else { passwordSilent = 0; }
    readPhrases( "passwordSettingPhrases", &passwordSettingPhrases );
    readPhrases( "passwordInvokingPhrases", &passwordInvokingPhrases );
    
	readPhrases( "infertilityDeclaringPhrases", &infertilityDeclaringPhrases );
	readPhrases( "fertilityDeclaringPhrases", &fertilityDeclaringPhrases );

    eveName = 
        SettingsManager::getStringSetting( "eveName", "EVE" );
    infertilitySuffix = 
        SettingsManager::getStringSetting( "infertilitySuffix", "+INFERTILE+" );
    fertilitySuffix = 
        SettingsManager::getStringSetting( "fertilitySuffix", "+FERTILE+" );
	//Pad the suffix to have some space between player name and the suffix
	//padding it in the ini file wouldnt work, for some unknown reason...
	std::string strInfertilitySuffix(infertilitySuffix);
	std::string strFertilitySuffix(fertilitySuffix);
	strInfertilitySuffix = " " + strInfertilitySuffix;
	strFertilitySuffix = " " + strFertilitySuffix;
	infertilitySuffix = strdup( strInfertilitySuffix.c_str() );
	fertilitySuffix = strdup( strFertilitySuffix.c_str() );
    
    curseYouPhrase = 
        SettingsManager::getSettingContents( "curseYouPhrase", 
                                             "CURSE YOU" );
    
    curseBabyPhrase = 
        SettingsManager::getSettingContents( "curseBabyPhrase", 
                                             "CURSE MY BABY" );



    
    killEmotionIndex =
        SettingsManager::getIntSetting( "killEmotionIndex", 2 );

    victimEmotionIndex =
        SettingsManager::getIntSetting( "victimEmotionIndex", 2 );

    starvingEmotionIndex =
        SettingsManager::getIntSetting( "starvingEmotionIndex", 2 );

    afkEmotionIndex =
        SettingsManager::getIntSetting( "afkEmotionIndex", 2 );

    drunkEmotionIndex =
        SettingsManager::getIntSetting( "drunkEmotionIndex", 2 );

    trippingEmotionIndex =
        SettingsManager::getIntSetting( "trippingEmotionIndex", 2 );

    afkTimeSeconds =
        SettingsManager::getDoubleSetting( "afkTimeSeconds", 120.0 );

    satisfiedEmotionIndex =
        SettingsManager::getIntSetting( "satisfiedEmotionIndex", 2 );


    FILE *f = fopen( "curseWordList.txt", "r" );
    
    if( f != NULL ) {
    
        int numRead = 1;
        
        char buff[100];
        
        while( numRead == 1 ) {
            numRead = fscanf( f, "%99s", buff );
            
            if( numRead == 1 ) {
                if( strlen( buff ) < 6 ) {
                    // short words only, 3, 4, 5 letters
                    curseWords.push_back( stringToUpperCase( buff ) );
                    }
                }
            }
        fclose( f );
        }
    printf( "Curse word list has %d words\n", curseWords.size() );
    

#ifdef WIN_32
    printf( "\n\nPress CTRL-C to shut down server gracefully\n\n" );

    SetConsoleCtrlHandler( ctrlHandler, TRUE );
#else
    printf( "\n\nPress CTRL-Z to shut down server gracefully\n\n" );

    signal( SIGTSTP, intHandler );
#endif

    initNames();

    initCurses();
    
    initLifeTokens();
    
    initFitnessScore();
    

    initLifeLog();
    //initBackup();
    
    initPlayerStats();
    initLineageLog();
    
    initLineageLimit();
    
    initCurseDB();
    

    char rebuilding;

    initAnimationBankStart( &rebuilding );
    while( initAnimationBankStep() < 1.0 );
    initAnimationBankFinish();


    initObjectBankStart( &rebuilding, true, true );
    while( initObjectBankStep() < 1.0 );
    initObjectBankFinish();

    
    initCategoryBankStart( &rebuilding );
    while( initCategoryBankStep() < 1.0 );
    initCategoryBankFinish();


    // auto-generate category-based transitions
    initTransBankStart( &rebuilding, true, true, true, true );
    while( initTransBankStep() < 1.0 );
    initTransBankFinish();
    

    // defaults to one hour
    int epochSeconds = 
        SettingsManager::getIntSetting( "epochSeconds", 3600 );
    
    setTransitionEpoch( epochSeconds );


    initFoodLog();
    initFailureLog();

    initObjectSurvey();
    
    initLanguage();
    initFamilySkipList();
    
    
    initTriggers();


    if(oneLifeServer->initMap() != true )
    {
        // cannot continue after map init fails
        return 1;
	}
    


    if( false ) {
        
        printf( "Running map sampling\n" );
    
        int idA = 290;
        int idB = 942;
        
        int totalCountA = 0;
        int totalCountB = 0;
        int numRuns = 2;

        for( int i=0; i<numRuns; i++ ) {
        
        
            int countA = 0;
            int countB = 0;
        
            int x = randSource.getRandomBoundedInt( 10000, 300000 );
            int y = randSource.getRandomBoundedInt( 10000, 300000 );
        
            printf( "Sampling at %d,%d\n", x, y );


            for( int yd=y; yd<y + 2400; yd++ ) {
                for( int xd=x; xd<x + 2400; xd++ ) {
                    int oID = getMapObject( xd, yd );
                
                    if( oID == idA ) {
                        countA ++;
                        }
                    else if( oID == idB ) {
                        countB ++;
                        }
                    }
                }
            printf( "   Count at %d,%d is %d = %d, %d = %d\n",
                    x, y, idA, countA, idB, countB );

            totalCountA += countA;
            totalCountB += countB;
            }
        printf( "Average count %d (%s) = %f,  %d (%s) = %f  over %d runs\n",
                idA, getObject( idA )->description, 
                totalCountA / (double)numRuns,
                idB, getObject( idB )->description, 
                totalCountB / (double)numRuns,
                numRuns );
        printf( "Press ENTER to continue:\n" );
    
        int readInt;
        scanf( "%d", &readInt );
        }
    


    
    int port = 
        SettingsManager::getIntSetting( "port", 5077 );
    
    
    
    SocketServer *server = new SocketServer( port, 256 );
    
    sockPoll.addSocketServer( server );
    
    AppLog::infoF( "Listening for connection on port %d", port );

    // if we received one the last time we looped, don't sleep when
    // polling for socket being ready, because there could be more data
    // waiting in the buffer for a given socket
    char someClientMessageReceived = false;


	settings.shutdownMode = SettingsManager::getIntSetting( "shutdownMode", 0 );
    int forceShutdownMode = SettingsManager::getIntSetting( "forceShutdownMode", 0 );
        
    
    // test code for printing sample eve locations
    // direct output from server to out.txt
    // then run:
    // grep "Eve location" out.txt | sed -e "s/Eve location //" | 
    //      sed -e "s/,/ /" > eveTest.txt
    // Then in gnuplot, do:
    //  plot "eveTest.txt" using 1:2 with linespoints;

    /*
    for( int i=0; i<1000; i++ ) {
        int x, y;
        
        SimpleVector<GridPos> temp;
        
        getEvePosition( "test@blah", 1, &x, &y, &temp, false );
        
        printf( "Eve location %d,%d\n", x, y );
        }
    */

	oneLifeServer = new OneLife::Server();

	oneLifeServer->start();
    
    // stop listening on server socket immediately, before running
    // cleanup steps.  Cleanup may take a while, and we don't want to leave
    // server socket listening, because it will answer reflector and player
    // connection requests but then just hang there.

    // Closing the server socket makes these connection requests fail
    // instantly (instead of relying on client timeouts).
    delete server;

    quitCleanup();
    
    
    AppLog::info( "Done." );

    return 0;
    }



// implement null versions of these to allow a headless build
// we never call drawObject, but we need to use other objectBank functions


void *getSprite( int ) {
    return NULL;
    }

char *getSpriteTag( int ) {
    return NULL;
    }

char isSpriteBankLoaded() {
    return false;
    }

char markSpriteLive( int ) {
    return false;
    }

void stepSpriteBank() {
    }

void drawSprite( void*, doublePair, double, double, char ) {
    }

void setDrawColor( float inR, float inG, float inB, float inA ) {
    }

void setDrawFade( float ) {
    }

float getTotalGlobalFade() {
    return 1.0f;
    }

void toggleAdditiveTextureColoring( char inAdditive ) {
    }

void toggleAdditiveBlend( char ) {
    }

void drawSquare( doublePair, double ) {
    }

void startAddingToStencil( char, char, float ) {
    }

void startDrawingThroughStencil( char ) {
    }

void stopStencil() {
    }





// dummy implementations of these functions, which are used in editor
// and client, but not server
#include "../gameSource/spriteBank.h"
SpriteRecord *getSpriteRecord( int inSpriteID ) {
    return NULL;
    }

#include "../gameSource/soundBank.h"
void checkIfSoundStillNeeded( int inID ) {
    }



char getSpriteHit( int inID, int inXCenterOffset, int inYCenterOffset ) {
    return false;
    }


char getUsesMultiplicativeBlending( int inID ) {
    return false;
    }


void toggleMultiplicativeBlend( char inMultiplicative ) {
    }


void countLiveUse( SoundUsage inUsage ) {
    }

void unCountLiveUse( SoundUsage inUsage ) {
    }



// animation bank calls these only if lip sync hack is enabled, which
// it never is for server
void *loadSpriteBase( const char*, char ) {
    return NULL;
    }

void freeSprite( void* ) {
    }

void startOutputAllFrames() {
    }

void stopOutputAllFrames() {
    }
