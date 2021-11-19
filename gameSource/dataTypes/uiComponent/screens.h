//
// Created by olivier on 15/11/2021.
//

#ifndef ONELIFE_DATATYPE_UICOMPONENT_LOADINGSCREEN_H
#define ONELIFE_DATATYPE_UICOMPONENT_LOADINGSCREEN_H

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
		int errCode;
	}UnimplementedScreen;
}

#endif //ONELIFE_DATATYPE_UICOMPONENT_LOADINGSCREEN_H
