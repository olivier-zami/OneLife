//
// Created by olivier on 01/05/2022.
//

#ifndef ONELIFE_SERVER_PROTOTYPE_BANK_CACHE_H
#define ONELIFE_SERVER_PROTOTYPE_BANK_CACHE_H

#define BIOME_CACHE_SIZE 131072 // optimization:// cache biomeIndex results in RAM // 3.1 MB of RAM for this.
#define DB_CACHE_SIZE 131072// optimization:// cache dbGet results in RAM// 2.6 MB of RAM for this.
#define CACHE_PRIME_A 776509273
#define CACHE_PRIME_B 904124281
#define CACHE_PRIME_C 528383237
#define CACHE_PRIME_D 148497157

namespace OneLife::server::bank
{
	class Cache
	{
		public:
			Cache();
			~Cache();
	};
}

int computeXYCacheHash(int inKeyA, int inKeyB);

#endif //ONELIFE_SERVER_PROTOTYPE_BANK_CACHE_H
