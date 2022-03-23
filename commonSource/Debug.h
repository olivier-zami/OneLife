//
// Created by olivier on 28/02/2022.
//

#ifndef ONELIFE_DEBUG_H
#define ONELIFE_DEBUG_H

#include "../third_party/openLife/src/Debug.h"
#include "../third_party/json/single_include/nlohmann/json.hpp"
#include "debug/data/type/configuration.h"

namespace oneLife
{
	class Debug :
			public openLife::Debug
	{
		public:
			static void loadConfigurationFile(const char* pathConfigFile);
			static void dumpConfiguration();

		protected:
			static oneLife::debug::dataType::Configuration getDataConfiguration();

		private:
			static oneLife::debug::dataType::Configuration dataConfig;
			static openLife::debug::dataValue::Context context;
	};
}


#endif //ONELIFE_DEBUG_H
