//
// Created by olivier on 17/12/2021.
//

#ifndef ONELIFE_GAME_CONTROLLER_FEATURE_APOCALYPSE_H
#define ONELIFE_GAME_CONTROLLER_FEATURE_APOCALYPSE_H

#include "OneLife/gameSource/controllers/feature.h"

namespace OneLife::game::feature
{
	class Apocalypse:
		public OneLife::game::Feature
	{
		public:
			Apocalypse();
			~Apocalypse();

			void handleProgressStatus(int* progressStatus);
			void handleFrameRateFactor(double* frameRateFactor);
			void handleApocalypseDisplayProgress(double* apocalypseDisplayProgress);

			void update();

		protected:
			int* isInProgress; //bool isInProgress;
			double* frameRateFactor;
			double apocalypseDisplaySeconds;
			double* apocalypseDisplayProgress;
	};

}

#endif //ONELIFE_GAME_CONTROLLER_FEATURE_APOCALYPSE_H
