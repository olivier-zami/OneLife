//
// Created by olivier on 28/04/2022.
//

#ifndef ONELIFE_SERVER_COMPONENT_DATABASE_LOOKTIME_H
#define ONELIFE_SERVER_COMPONENT_DATABASE_LOOKTIME_H

#include "../../prototype/bank/LinearDB.h"

#include "../../../third_party/minorGems/system/Time.h"

namespace OneLife::server::database
{
	class LookTime:
			public OneLife::server::bank::LinearDB
	{
		public:
			LookTime(
					const char* path,
					int mode,
					unsigned long hash_table_size,
					unsigned long key_size,
					unsigned long value_size
					);
			~LookTime();

			void setStaleDuration(int staleInSecond);

			void clean();

		private:
			int staleInSecond;
	};
}

void dbLookTimePut(int inX, int inY, timeSec_t inTime);

#endif //ONELIFE_SERVER_COMPONENT_DATABASE_LOOKTIME_H
