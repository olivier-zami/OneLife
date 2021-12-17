//
// Created by olivier on 05/12/2021.
//

#ifndef ONELIFE_GAME_DEBUG_H
#define ONELIFE_GAME_DEBUG_H

#include "OneLife/third_party/openlife/system/trace.h"

namespace OneLife::debug
{
	class Console :
			public openLife::system::Trace
	{
		public:
			static void showApplication(const char* message, ...);
			static void showController(const char* message, ...);
			static void showControllerStep(const char* message, ...);
			static void showFunction(const char* message, ...);
			static void showFunctionStep(const char* message, ...);
			static void write(const char* message, ...);
	};
}

#endif //ONELIFE_GAME_DEBUG_H
