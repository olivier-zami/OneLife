//
// Created by olivier on 01/05/2022.
//

#ifndef ONELIFE_SERVER_COMPONENT_CACHE_BLOCKING_H
#define ONELIFE_SERVER_COMPONENT_CACHE_BLOCKING_H

#include "../../prototype/bank/Cache.h"

#include "../../dataType/cacheRecord.h"

namespace OneLife::server::cache
{
	class Blocking:
		public OneLife::server::bank::Cache
	{
		public:
			Blocking();
			~Blocking();

	};
}

char blockingGetCached(int inX, int inY);
void blockingPutCached(int inX, int inY, char inBlocking);
void blockingClearCached(int inX, int inY);

#endif //ONELIFE_SERVER_COMPONENT_CACHE_BLOCKING_H
