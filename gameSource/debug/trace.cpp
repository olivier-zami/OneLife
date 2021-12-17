//
// Created by olivier on 01/12/2021.
//

#include "trace.h"

#include <cstdio>

void OneLife::debug::Trace::log(char* message)
{
	printf("\n===>%s", message);
}
