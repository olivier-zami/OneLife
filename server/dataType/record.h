//
// Created by olivier on 29/04/2022.
//

#ifndef ONELIFE_SERVER_DATATYPE_RECORD_H
#define ONELIFE_SERVER_DATATYPE_RECORD_H

#include <cstdint>

namespace OneLife::server::dataType::record
{
	typedef struct
	{
		int32_t type;
		int32_t secondPlace;
		int32_t secondPlaceGap;
	}Biome;
}

#endif //ONELIFE_SERVER_DATATYPE_RECORD_H
