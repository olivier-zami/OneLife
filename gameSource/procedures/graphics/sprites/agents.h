//
// Created by olivier on 17/11/2021.
//

#ifndef ONELIFE_GRAPHIC_AGENT_H
#define ONELIFE_GRAPHIC_AGENT_H

#include "OneLife/gameSource/animationBank.h"
#include "OneLife/gameSource/dataTypes/game.h"
#include "minorGems/util/SimpleVector.h"
#include "minorGems/game/doublePair.h"

namespace OneLife::game
{
	ObjectAnimPack drawLiveObject(
			LiveObject *inObj,
			SimpleVector<LiveObject *> *inSpeakers,
			SimpleVector<doublePair> *inSpeakersPos);
}

#endif //ONELIFE_GRAPHIC_AGENT_H
