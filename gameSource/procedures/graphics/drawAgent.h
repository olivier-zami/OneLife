//
// Created by olivier on 26/10/2021.
//

#ifndef ONELIFE_PROCEDURE_GRAPHIC_DRAWAGENT_H
#define ONELIFE_PROCEDURE_GRAPHIC_DRAWAGENT_H

#include "minorGems/game/doublePair.h"
#include "minorGems/util/SimpleVector.h"
#include "OneLife/gameSource/components/pages/LivingLifePage.h"

typedef void (LivingLifePage::*PtrDrawMapCell)(int, int, int, char, char);
typedef void (LivingLifePage::*PtrPutInMap)(int, ExtraMapObject*);
typedef ExtraMapObject (LivingLifePage::*PtrCopyFromMap)(int);
typedef LiveObject* (LivingLifePage::*PtrGetLiveObject)( int );
typedef ObjectAnimPack (LivingLifePage::*PtrDrawLiveObject)(LiveObject*, SimpleVector<LiveObject *>*, SimpleVector<doublePair>*);

void drawAgent(
	LivingLifePage* livingLifePage,
	PtrDrawMapCell drawMapCell,//void (*drawMapCell)( int inMapI, int inScreenX, int inScreenY, char inHighlightOnly, char inNoTimeEffects),
	PtrPutInMap putInMap,//void (*putInMap)( int inMapI, ExtraMapObject *inObj ),
	PtrCopyFromMap copyFromMap,//ExtraMapObject (*copyFromMap)( int inMapI ),
	PtrGetLiveObject getLiveObject,//LiveObject* (*getLiveObject)( int inID ),
	PtrDrawLiveObject drawLiveObject,//ObjectAnimPack (*drawLiveObject)(LiveObject *inObj, SimpleVector<LiveObject *> *inSpeakers, SimpleVector<doublePair> *inSpeakersPos ),
	int xStart,
	int xEnd,
	int yStart,
	int yEnd,
	int mMapOffsetX,
	int mMapOffsetY,
	int mapWidth,//mMapD from LivingLifePage
	int mapHeight,//mMapD from LivingLifePage
	char* cellDrawn,
	int *mMap,
	double *mMapMoveSpeeds,//livingLifePAge::mMap
	//doublePair cellPos,
	SimpleVector<LiveObject> gameObjects,
	int numMoving,
	int* movingObjectsIndices,
	doublePair *mMapMoveOffsets,
	SimpleVector<ExtraMapObject> mMapExtraMovingObjects,
	SimpleVector<LiveObject *>* speakers,
	SimpleVector<doublePair>* speakersPos,
	Font *mainFont,
	SimpleVector<GridPos> mMapExtraMovingObjectsDestWorldPos);

#endif //ONELIFE_PROCEDURE_GRAPHIC_DRAWAGENT_H
