//
// Created by olivier on 15/11/2021.
//

#ifndef ONELIFE_DATATYPE_UICOMPONENT_SCREENS_H
#define ONELIFE_DATATYPE_UICOMPONENT_SCREENS_H

#include "minorGems/game/doublePair.h"

namespace OneLife::dataType::uiComponent
{
	typedef struct{
		struct{
			bool showProgressBar;
		}status;
		const char* taskName;
		float progressBar;
	}LoadingScreen;

	typedef struct{

	}OutdoorSceneScreen;

	typedef struct{
		int errCode;
	}UnimplementedScreen;

	typedef struct{
		doublePair center;
		struct{
			struct{
				doublePair position;
				struct{float red; float green; float blue; float alpha;}color;
				char* value;
			}statusMessage;
			struct{
				doublePair position;
				struct{float red; float green; float blue; float alpha;}color;
				char* value;
			}connectingMessage;
			struct{
				doublePair position;
				struct{float red; float green; float blue; float alpha;}color;
				char* value;
			}serverAddressMessage;
			struct{
				doublePair position;
				struct{float red; float green; float blue; float alpha;}color;
				char* value;
			}cancelMessage;
		}component;
	}WaitingScreen;
}

#endif //ONELIFE_DATATYPE_UICOMPONENT_SCREENS_H
