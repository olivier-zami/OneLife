//
// Created by olivier on 30/08/2021.
//

#ifndef OPENLIFE_EXTENSION_NLOHMANN_H
#define OPENLIFE_EXTENSION_NLOHMANN_H

#include "../../third_party/json/single_include/nlohmann/json.hpp"

using Json = nlohmann::json;

namespace oneLife::io::json
{
	Json getObject(const char* filename);
}

#endif //OPENLIFE_EXTENSION_NLOHMANN_H
