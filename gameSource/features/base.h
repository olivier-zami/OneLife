//
// Created by olivier on 02/12/2021.
//

#ifndef ONELIFE_GAME_FEATURE_BASE_H
#define ONELIFE_GAME_FEATURE_BASE_H

#include "../feature.h"
#include "OneLife/gameSource/dataTypes/message.h"

namespace OneLife::game::feature
{
	class Base :
	public OneLife::game::Feature
	{
		public:
			Base();
			~Base();

			OneLife::dataType::Message getGameMessage();

	};
}

#endif //ONELIFE_GAME_FEATURE_BASE_H
