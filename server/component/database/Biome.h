//
// Created by olivier on 25/04/2022.
//

#ifndef ONELIFE_SERVER_COMPONENT_DATABASE_BIOME_H
#define ONELIFE_SERVER_COMPONENT_DATABASE_BIOME_H

#include "../../prototype/bank/LinearDB.h"

namespace OneLife::server::database
{
	class Biome:
			public OneLife::server::bank::LinearDB
	{
		public:
			Biome(
					const char* path,
				  	int mode,
				  	unsigned long hash_table_size,
				  	unsigned long key_size,
				  	unsigned long value_size);

			~Biome();

			void clean();
			unsigned int getRecordNumber();

		protected:
			void enable();

		private:
	};
}

int biomeDBGet(int inX, int inY, int *outSecondPlaceBiome = NULL, double *outSecondPlaceGap = NULL);
void biomeDBPut(int inX, int inY, int inValue, int inSecondPlace, double inSecondPlaceGap);

#endif //ONELIFE_SERVER_COMPONENT_DATABASE_BIOME_H
