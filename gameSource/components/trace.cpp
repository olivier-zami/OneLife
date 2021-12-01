//
// Created by olivier on 01/12/2021.
//

#include "trace.h"

#include <cstdio>

void OneLife::game::Trace::log(char* message)
{
	printf("\n===>%s", message);
}
