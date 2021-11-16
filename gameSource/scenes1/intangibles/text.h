//
// Created by olivier on 16/11/2021.
//

#ifndef ONELIFE_GAME_TEXT_H
#define ONELIFE_GAME_TEXT_H

#include "minorGems/util/SimpleVector.h"

namespace OneLife::game
{
	class Text
	{
	};
}

SimpleVector<char*> *splitLines( const char *inString, double inMaxWidth );

#endif //ONELIFE_GAME_TEXT_H
