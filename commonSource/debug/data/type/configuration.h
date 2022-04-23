//
// Created by olivier on 28/02/2022.
//

#ifndef ONELIFE_COMMON_DEBUG_DATATYPE_CONFIGURATION_H
#define ONELIFE_COMMON_DEBUG_DATATYPE_CONFIGURATION_H

namespace OneLife::debug::dataType
{
	typedef struct{
		char appName[32];
		struct{
			bool isEnabled;
		}status;
		struct{
			bool showStep;
		}option;
	}Configuration;
}

#endif //ONELIFE_COMMON_DEBUG_DATATYPE_CONFIGURATION_H
