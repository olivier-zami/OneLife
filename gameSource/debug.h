//
// Created by olivier on 05/12/2021.
//

#ifndef ONELIFE_GAME_DEBUG_H
#define ONELIFE_GAME_DEBUG_H

#include "OneLife/third_party/openlife/system/trace.h"

namespace OneLife::game
{
	class Debug :
			public openLife::system::Trace
	{
		public:
			static void writeAppInfo(const char* message, ...);
			static void writeControllerInfo(const char* message, ...);
			static void writeControllerStepInfo(const char* message, ...);
			static void writeMethodInfo(const char* message, ...);
			static void writeMethodStepInfo(const char* message, ...);
			static void write(const char* message, ...);
	};
}

#endif //ONELIFE_GAME_DEBUG_H
