//
// Created by olivier on 04/11/2021.
//

#ifndef ONELIFE_DATATYPE_UI_H
#define ONELIFE_DATATYPE_UI_H

#include "minorGems/game/doublePair.h"

namespace OneLife::dataType
{
	typedef struct _uiComponent{
		const char* label;
		void (*draw)(void* screen);
		void *body;
	}UiComponent;
}

namespace OneLife::dataType::ui
{
	typedef struct{
		const char* label;
		struct{
			double pause;//TODO: pause must be bool
			bool fullScreen;
		}status;
		struct{
			struct{
				bool enable;
				float alpha;
			}pausePanel;
		}modal;
		/*****/
		struct{
			bool grabInput;
		}settings;
		 /*****/
	}Screen;
}

typedef struct LocationSpeech {
	doublePair pos;
	char *speech;
	double fade;
	double fadeETATime;// wall clock time when speech should start fading
} LocationSpeech;

#endif //ONELIFE_DATATYPE_UI_H
