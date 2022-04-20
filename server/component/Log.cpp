//
// Created by olivier on 20/04/2022.
//

#include "Log.h"

#include <cstdio>
#include <cstddef>

extern int eveRadius;

/**
 * @from server/map.cpp
 */
void writeEveRadius()
{
	FILE *eveRadFile = fopen("eveRadius.txt", "w");
	if (eveRadFile != NULL)
	{

		fprintf(eveRadFile, "%d", eveRadius);

		fclose(eveRadFile);
	}
}