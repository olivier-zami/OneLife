//
// Created by olivier on 20/04/2022.
//

#include "../gameSource/categoryBank.h"
#include "../gameSource/objectMetadata.h"
#include "../gameSource/objectBank.h"
#include "../gameSource/transitionBank.h"
#include "../server/arcReport.h"
#include "../server/component/feature/apocalypse.h"
#include "../server/CoordinateTimeTracking.h"
#include "../server/map.h"
#include "../server/monument.h"
#include "../server/Server.h"
#include "../third_party/minorGems/util/log/AppLog.h"
#include "../third_party/minorGems/util/SettingsManager.h"

//!list of non used function. this file should not exist in a usable version

ObjectRecord *getObject(int inID){return nullptr;}//from gameSource/objectBank.cpp
int hideIDForClient(int inObjectID){return 0;}//from gameSource/objectBank.cpp
int getObjectHeight(int inObjectID){return 0;}//from gameSource/objectBank.cpp
int getMaxWideRadius(){return 0;}//from gameSource/objectBank.cpp
int getMetaTriggerObject( int inTriggerIndex ){return 0;}//from gameSource/objectBank.cpp
TapoutRecord *getTapoutRecord(int inObjectID){return nullptr;}
char canPickup( int inObjectID, double inPlayerAge ){return false;}
int getNumContainerSlots( int inID ){return 0;}

TransRecord *getPTrans( int inActor, int inTarget, char inLastUseActor, char inLastUseTarget){return nullptr;}//from gameSource/transitionBank.cpp
int getObjectDepth( int inObjectID ){return 0;}//from gameSource/transitionBank.cpp
TransRecord *getTrans( int inActor, int inTarget, char inLastUseActor, char inLastUseTarget ){return nullptr;}//from gameSource/transitionBank.cpp

void AppLog::infoF( const char *inFormatString, ... ){}//from minorGems/util/log/AppLog.cpp
void AppLog::errorF( const char *inFormatString, ... ){}//from minorGems/util/log/AppLog.cpp
void AppLog::info( const char *inString ){}//from minorGems/util/log/AppLog.cpp

CategoryRecord *getCategory( int inParentID ){return nullptr;}//from gameSource/categoryBank.cpp


void reportArcEnd(){}//from server/arcReport.cpp

char *SettingsManager::getStringSetting( const char *inSettingName, const char *inDefaultValue ){return nullptr;}
char *SettingsManager::getStringSetting( const char *inSettingName){char*o=nullptr;return o;}
int SettingsManager::getIntSetting(const char *inSettingName, char *outValueFound){return 0;}
int SettingsManager::getIntSetting(const char *inSettingName, int inDefaultValue){return 0;}

void logMapChange(int inX, int inY, int inID){}//from server/map.cpp

TransRecord *getMetaTrans( int inActor, int inTarget, char inLastUseActor, char inLastUseTarget){return nullptr;}//from gameSource/objectMetadata.cpp
//void setLastMetadataID( int inMetadataID ){}//from gameSource/objectMetadata.cpp

void writeRecentPlacements(){}//from server/component/Log.h

char isApocalypseTrigger( int inID ){return false;}//from server/component/feature/apocalypse.cpp
int getMonumentStatus( int inID ){return 0;}//from server/component/feature/apocalypse.cpp

void monumentAction( int inX, int inY, int inObjectID, int inPlayerID, int inAction ){}//from server/monument.cpp

//void restretchMapContainedDecays(int inX, int inY, int inOldContainerID, int inNewContainerID, int inSubCont){}//from server/server.cpp

void CoordinateTimeTracking::cleanStale( timeSec_t inStaleTime ){}//from server/coordinateTracking.cpp
char CoordinateTimeTracking::checkExists( int inX, int inY, timeSec_t inCurTime ){return false;}//from server/coordinateTracking.cpp
CoordinateTimeTracking::CoordinateTimeTracking(){}//from server/coordinateTracking.cpp