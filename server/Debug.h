//
// Created by olivier on 05/04/2022.
//

#ifndef ONELIFE_SERVER_DEBUG_H
#define ONELIFE_SERVER_DEBUG_H

#include "../third_party/openLife/src/Debug.h"

namespace OneLife::server
{
	class Debug:
			public openLife::Debug
	{
	};
}

#endif //ONELIFE_SERVER_DEBUG_H
