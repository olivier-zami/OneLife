//
// Created by olivier on 02/12/2021.
//

#ifndef ONELIFE_GAME_FEATURE_H
#define ONELIFE_GAME_FEATURE_H

#include "OneLife/gameSource/dataTypes/message.h"

namespace OneLife::game
{
	class Feature
	{
		public:
			Feature();
			~Feature();

			void readSocketMessage(char* socketMessage);
			virtual OneLife::dataType::Message getGameMessage();

			unsigned int getType() const;

		protected:
			bool isMessageTypeMatch(const char* message);

			unsigned int type;
			char lastSocketMessage[1024];
	};
}

#endif //ONELIFE_GAME_FEATURE_H
