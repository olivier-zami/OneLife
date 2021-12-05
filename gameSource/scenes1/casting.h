//
// Created by olivier on 05/12/2021.
//

#ifndef ONELIFE_GAME_CASTING_H
#define ONELIFE_GAME_CASTING_H

namespace OneLife::game
{
	class Casting
	{
		public:
			Casting();
			~Casting();

			void setIndexRecentlyInsertedGameObject(unsigned int idx);
			unsigned int getIndexRecentlyInsertedGameObject();

		private:
			unsigned int recentInsertedGameObjectIndex;

	};
}

#endif //ONELIFE_GAME_CASTING_H
