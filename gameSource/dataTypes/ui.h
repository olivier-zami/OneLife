//
// Created by olivier on 04/11/2021.
//

#ifndef ONELIFE_DATATYPE_UI_H
#define ONELIFE_DATATYPE_UI_H

#include "minorGems/game/doublePair.h"

namespace OneLife::dataType::ui
{
	typedef struct{
		const char* label;
		struct{
			struct{
				bool isDrawn;
				float alpha;
			}pausePanel;
		}modal;
		struct{
			double pause;
			bool fullScreen;
		}status;
		struct{
		}_default;
		struct{
			bool grabInput;
		}settings;
	}Screen;
}

typedef struct LocationSpeech {
	doublePair pos;
	char *speech;
	double fade;
	double fadeETATime;// wall clock time when speech should start fading
} LocationSpeech;

#endif //ONELIFE_DATATYPE_UI_H
