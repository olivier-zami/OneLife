//
// Created by olivier on 28/02/2022.
//

#include "Debug.h"

#include <cstdio>
#include <string>
#include <fstream>

#include "../third_party/openLife/src/system/Console.h"
#include "io/json.h"

OneLife::debug::dataType::Configuration OneLife::Debug::dataConfig;
openLife::debug::dataValue::Context OneLife::Debug::context = openLife::debug::dataValue::Context::NONE;

void OneLife::Debug::loadConfigurationFile(const char *pathConfigFile)
{
	std::ifstream ifs(pathConfigFile);
	nlohmann::json json = nlohmann::json::parse(ifs);

	memset(OneLife::Debug::dataConfig.appName, 0, sizeof(OneLife::Debug::dataConfig.appName));
	strcpy(OneLife::Debug::dataConfig.appName, json["appName"].get<std::string>().c_str());

	OneLife::Debug::dataConfig.status.isEnabled = json["status"]["enabled"].get<bool>();
	OneLife::Debug::dataConfig.option.showStep = json["option"]["showStep"].get<bool>();
}

void OneLife::Debug::dumpConfiguration()
{
	printf("\n------------------------");
	printf("\nshow debug of %s", OneLife::Debug::dataConfig.appName);
	printf("\nstatus.enabled : %i", OneLife::Debug::dataConfig.status.isEnabled);
	printf("\n------------------------\n");
}

/**********************************************************************************************************************/


OneLife::debug::dataType::Configuration OneLife::Debug::getDataConfiguration()
{
	return OneLife::Debug::dataConfig;
}