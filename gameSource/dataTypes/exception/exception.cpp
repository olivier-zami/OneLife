//
// Created by olivier on 02/11/2021.
//

#include "exception.h"

#include <cstdio>

OneLife::game::dataType::Exception::Exception(const char* message)
{
	printf("\n%s", message);
}