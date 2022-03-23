//
// Created by olivier on 28/02/2022.
//

#include "Debug.h"

#include <cstdio>
#include <string>
#include <fstream>

#include "../third_party/openLife/src/system/Console.h"
#include "io/json.h"

oneLife::debug::dataType::Configuration oneLife::Debug::dataConfig;
openLife::debug::dataValue::Context oneLife::Debug::context = openLife::debug::dataValue::Context::NONE;

void oneLife::Debug::loadConfigurationFile(const char *pathConfigFile)
{
	std::ifstream ifs(pathConfigFile);
	nlohmann::json json = nlohmann::json::parse(ifs);

	memset(oneLife::Debug::dataConfig.appName, 0, sizeof(oneLife::Debug::dataConfig.appName));
	strcpy(oneLife::Debug::dataConfig.appName, json["appName"].get<std::string>().c_str());

	oneLife::Debug::dataConfig.status.isEnabled = json["status"]["enabled"].get<bool>();
	oneLife::Debug::dataConfig.option.showStep = json["option"]["showStep"].get<bool>();
}

void oneLife::Debug::dumpConfiguration()
{
	printf("\n------------------------");
	printf("\nshow debug of %s", oneLife::Debug::dataConfig.appName);
	printf("\nstatus.enabled : %i", oneLife::Debug::dataConfig.status.isEnabled);
	printf("\n------------------------\n");
}

/**********************************************************************************************************************/


oneLife::debug::dataType::Configuration oneLife::Debug::getDataConfiguration()
{
	return oneLife::Debug::dataConfig;
}