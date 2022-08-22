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

#include "game/Server.h"

#include "minorGems/util/stringUtils.h"
#include "minorGems/util/SettingsManager.h"
#include "minorGems/util/SimpleVector.h"
#include "minorGems/network/SocketServer.h"
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

//#define IGNORE_PRINTF

#ifdef IGNORE_PRINTF
#define printf(fmt, ...) (0)
#endif

#include "../commonSource/math/geometry.h"
#include "../gameSource/GridPos.h"
#include "component/feature/apocalypse.h"
#include "component/container/Player.h"
#include "component/handler/Player.h"
#include "minorGems/util/crc32.h"
#include "spiral.h"
#include "dataType/info.h"
#include "dataType/LiveObject.h"
#include "dataType/Settings.h"

#define PERSON_OBJ_ID 12

extern SimpleVector<LiveObject> players;
extern int apocalypsePossible;
extern char apocalypseTriggered;
extern GridPos apocalypseLocation;
extern SimpleVector<DeadObject> pastPlayers;
extern double secondsPerYear;
extern char apocalypseRemote;
extern int lastApocalypseNumber;
extern FILE *familyDataLogFile;
static JenkinsRandomSource randSource;
extern double minFoodDecrementSeconds;
extern double maxFoodDecrementSeconds;
extern int babyBirthFoodDecrement;
extern int eatBonus;// bonus applied to all foods// makes whole server a bit easier (or harder, if negative)
// static double eatBonusFloor = 0;
// static double eatBonusHalfLife = 50;
// static double posseSizeSpeedMultipliers[4] = { 0.75, 1.25, 1.5, 2.0 };
extern int minActivePlayersForLanguages;
extern unsigned int nextSequenceNumber;// keep a running sequence number to challenge each connecting client// to produce new login hashes, avoiding replay attacks.
extern int requireClientPassword;
extern int requireTicketServerCheck;
extern char *clientPassword;
extern char *ticketServerURL;
extern char *reflectorURL;
extern double childSameRaceLikelihood;
extern int familySpan;

extern SimpleVector<char*> nameGivingPhrases;
extern SimpleVector<char*> familyNameGivingPhrases;
extern SimpleVector<char*> cursingPhrases;
extern char *curseYouPhrase;
extern char *curseBabyPhrase;
extern SimpleVector<char*> forgivingPhrases;
extern SimpleVector<char*> youForgivingPhrases;
extern SimpleVector<char*> youGivingPhrases;
extern SimpleVector<char*> namedGivingPhrases;

//2HOL additions for: password-protected objects
extern int passwordTransitionsAllowed;
extern int passwordInvocationAndSettingAreSeparated;
extern int passwordOverhearRadius;
extern int passwordSilent;

extern SimpleVector<char*> passwordSettingPhrases;
extern SimpleVector<char*> passwordInvokingPhrases;
extern SimpleVector<char*> infertilityDeclaringPhrases;
extern SimpleVector<char*> fertilityDeclaringPhrases;
extern char *eveName;
extern char *infertilitySuffix;
extern char *fertilitySuffix;
extern char allowedSayCharMap[];// maps extended ascii codes to true/false for characters allowed in SAY// messages

static const char *allowedSayChars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ.-,'?! ";

extern int killEmotionIndex;
extern int victimEmotionIndex;
extern int starvingEmotionIndex;
extern int satisfiedEmotionIndex;
extern int afkEmotionIndex;
extern double afkTimeSeconds;
extern int drunkEmotionIndex;
extern int trippingEmotionIndex;

OneLife::Server* oneLifeServer = nullptr;

extern SimpleVector<FreshConnection> waitingForTwinConnections;
extern SimpleVector<LiveObject> tutorialLoadingPlayers;

char doesEveLineExist( int inEveID )
{
    for( int i=0; i<players.size(); i++ ) {
        LiveObject *o = players.getElement( i );
        
        if( ( ! o->error ) && o->lineageEveID == inEveID ) {
            return true;
            }
        }
    return false;
}



char *getPlayerName( int inID )
{
    LiveObject *o = getLiveObject( inID );
    if( o != NULL ) {
        return o->name;
        }
    return NULL;
}

extern int nextID;
extern SimpleVector<char *> curseWords;
extern char *curseSecret;



volatile char quit = false;

void intHandler( int inUnused )
{
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

GridPos killPlayer( const char *inEmail )
{
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

void forcePlayerAge( const char *inEmail, double inAge )
{
    for( int i=0; i<players.size(); i++ ) {
        LiveObject *o = players.getElement( i );
        
        if( strcmp( o->email, inEmail ) == 0 ) {
            double ageSec = inAge / getAgeRate();
            
            o->lifeStartTimeSeconds = Time::getCurrentTime() - ageSec;
            o->needsUpdate = true;
            }
        }
}

int computeOverflowFoodCapacity( int inBaseCapacity )
{
    // even littlest baby has +2 overflow, to get everyone used to the
    // concept.
    // by adulthood (when base cap is 20), overflow cap is 91.6
    return 2 + pow( inBaseCapacity, 8 ) * 0.0000000035;
}

extern SimpleVector<ChangePosition> newSpeechPos;
extern SimpleVector<char*> newSpeechPhrases;
extern SimpleVector<int> newSpeechPlayerIDs;
extern SimpleVector<char> newSpeechCurseFlags;
extern SimpleVector<char> newSpeechPasswordFlags;//2HOL additions for: password-protected objects
extern SimpleVector<char*> newLocationSpeech;
extern SimpleVector<ChangePosition> newLocationSpeechPos;

char *isCurseNamingSay( char *inSaidString );

extern SimpleVector<GraveInfo> newGraves;
extern SimpleVector<GraveMoveInfo> newGraveMoves;
extern SimpleVector<char*> tempTwinEmails;// fill this with emails that should also affect lineage ban// if any twin in group is banned, all should be

void readPhrases( const char *inSettingsName, SimpleVector<char*> *inList ) {
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

int readIntFromFile( const char *inFileName, int inDefaultValue )
{
    FILE *f = fopen( inFileName, "r" );
    
    if( f == NULL ) {
        return inDefaultValue;
        }
    
    int val = inDefaultValue;
    
    fscanf( f, "%d", &val );

    fclose( f );

    return val;
}



int main()
{
	OneLife::server::Settings settings;
	settings.codeVersion = readIntFromFile( "serverCodeVersionNumber.txt", 20289 );
	settings.dataVersion = readIntFromFile( "dataVersionNumber.txt", 1 );
	settings.flushLookTimes = SettingsManager::getIntSetting("flushLookTimes", 0);
	settings.forceShutdownMode = SettingsManager::getIntSetting( "forceShutdownMode", 0 );
	settings.shutdownMode = SettingsManager::getIntSetting( "shutdownMode", 0 );
	settings.skipLookTimeCleanup = SettingsManager::getIntSetting("skipLookTimeCleanup", 0);
	settings.lookTimeDbName = SettingsManager::getStringSetting( "lookTimeDB_name", "lookTime.db" );//!Settings lookTimeDB
	settings.mapBiomeOrder = SettingsManager::getIntSettingMulti("biomeOrder");
	settings.mapBiomeWeight = SettingsManager::getFloatSettingMulti("biomeWeights");
	settings.mapBiomeSpecial = SettingsManager::getIntSettingMulti("specialBiomes");
	settings.maxLoadForOpenCalls = (double)SettingsManager::getFloatSetting( "maxLoadForOpenCalls", 0.80 );
	settings.staleSec = SettingsManager::getIntSetting("mapCellForgottenSeconds", 0);


    if( checkReadOnly() )
    {
        printf( "File system read-only.  Server exiting.\n" );
        return 1;
	}
    
    familyDataLogFile = fopen( "familyDataLog.txt", "a" );

    if( familyDataLogFile != NULL )
    {
        fprintf( familyDataLogFile, "%.2f server starting up\n",
                 Time::getCurrentTime() );
	}


    memset( allowedSayCharMap, false, 256 );
    
    int numAllowed = strlen( allowedSayChars );
    for( int i=0; i<numAllowed; i++ ) {
        allowedSayCharMap[ (int)( allowedSayChars[i] ) ] = true;
        }
    

    nextID = SettingsManager::getIntSetting( "nextPlayerID", 2 );


    // make backup and delete old backup every day
    AppLog::setLog( new FileLog( "log.txt", 86400 ) );

    AppLog::setLoggingLevel( Log::DETAIL_LEVEL );
    AppLog::printAllMessages( true );

    nextSequenceNumber = 
        SettingsManager::getIntSetting( "sequenceNumber", 1 );

    requireClientPassword =
        SettingsManager::getIntSetting( "requireClientPassword", 1 );
    
    requireTicketServerCheck =
        SettingsManager::getIntSetting( "requireTicketServerCheck", 1 );
    
    clientPassword = 
        SettingsManager::getStringSetting( "clientPassword" );


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
    apocalypsePossible = SettingsManager::getIntSetting( "apocalypsePossible", 0 );
    lastApocalypseNumber = SettingsManager::getIntSetting( "lastApocalypseNumber", 0 );
    childSameRaceLikelihood = (double)SettingsManager::getFloatSetting( "childSameRaceLikelihood", 0.90 );
    familySpan = SettingsManager::getIntSetting( "familySpan", 2 );
    
    
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

	settings.strInfertilitySuffix = strInfertilitySuffix;
    
    curseYouPhrase = SettingsManager::getSettingContents( "curseYouPhrase", "CURSE YOU" );
    curseBabyPhrase = SettingsManager::getSettingContents( "curseBabyPhrase", "CURSE MY BABY" );
    killEmotionIndex = SettingsManager::getIntSetting( "killEmotionIndex", 2 );
    victimEmotionIndex = SettingsManager::getIntSetting( "victimEmotionIndex", 2 );
    starvingEmotionIndex = SettingsManager::getIntSetting( "starvingEmotionIndex", 2 );
    afkEmotionIndex = SettingsManager::getIntSetting( "afkEmotionIndex", 2 );
    drunkEmotionIndex = SettingsManager::getIntSetting( "drunkEmotionIndex", 2 );
    trippingEmotionIndex = SettingsManager::getIntSetting( "trippingEmotionIndex", 2 );
    afkTimeSeconds = SettingsManager::getDoubleSetting( "afkTimeSeconds", 120.0 );
    satisfiedEmotionIndex = SettingsManager::getIntSetting( "satisfiedEmotionIndex", 2 );


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


    if( false )
    {
        
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


    int port = SettingsManager::getIntSetting( "port", 5077 );
	settings.port = port;
    //SocketServer *server = new SocketServer( port, 256 ); TODO: moved in server delete if working
    //sockPoll.addSocketServer( server );TODO: moved in server delete if working

    AppLog::infoF( "Listening for connection on port %d", port );


    // if we received one the last time we looped, don't sleep when
    // polling for socket being ready, because there could be more data
    // waiting in the buffer for a given socket
    settings.someClientMessageReceived = false;


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



	oneLifeServer = new OneLife::Server(settings);
	oneLife::server::game::application::Information serverInformation = oneLifeServer->getInformation();

	//!
	AppLog::info("\n\nServer starting up" );
	AppLog::info("\n\tabout:");
	AppLog::infoF("\n\t\tServer using version number %d", serverInformation.about.versionNumber);

	oneLifeServer->loadObjects();
	oneLifeServer->initBiomes();
	if(oneLifeServer->initMap() != true )
	{
		// cannot continue after map init fails
		return 1;
	}

	oneLifeServer->start();

	//!clean settings
	if(settings.lookTimeDbName) delete settings.lookTimeDbName;
	delete settings.mapBiomeOrder;
	delete settings.mapBiomeWeight;
	delete settings.mapBiomeSpecial;


    quitCleanup();
    
    
    AppLog::info( "Done." );

    return 0;
}



// implement null versions of these to allow a headless build
// we never call drawObject, but we need to use other objectBank functions
void *getSprite( int ) {return NULL;}
char *getSpriteTag( int ) {return NULL;}
char isSpriteBankLoaded() {return false;}
char markSpriteLive( int ) {return false;}
void stepSpriteBank() {}
void drawSprite( void*, doublePair, double, double, char ) {}
void setDrawColor( float inR, float inG, float inB, float inA ) {}
void setDrawFade( float ) {}
float getTotalGlobalFade() {return 1.0f;}
void toggleAdditiveTextureColoring( char inAdditive ) {}
void toggleAdditiveBlend( char ) {}
void drawSquare( doublePair, double ) {}
void startAddingToStencil( char, char, float ) {}
void startDrawingThroughStencil( char ) {}
void stopStencil() {}

// dummy implementations of these functions, which are used in editor
// and client, but not server
#include "../gameSource/spriteBank.h"
SpriteRecord *getSpriteRecord( int inSpriteID ) {return NULL;}

#include "../gameSource/soundBank.h"
void checkIfSoundStillNeeded( int inID ) {}
char getSpriteHit( int inID, int inXCenterOffset, int inYCenterOffset ) {return false;}
char getUsesMultiplicativeBlending( int inID ) {return false;}
void toggleMultiplicativeBlend( char inMultiplicative ) {}
void countLiveUse( SoundUsage inUsage ) {}
void unCountLiveUse( SoundUsage inUsage ) {}

// animation bank calls these only if lip sync hack is enabled, which
// it never is for server
void *loadSpriteBase( const char*, char ) {return NULL;}
void freeSprite( void* ) {}
void startOutputAllFrames() {}
void stopOutputAllFrames() {}
