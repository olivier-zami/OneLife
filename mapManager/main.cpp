//
// Created by olivier on 07/04/2022.
//

#include <cstdio>
#include "Application.h"

int main()
{
	printf("\nopen mapManager");

	OneLife::mapManager::Application* mapManager = new OneLife::mapManager::Application();
	mapManager->start();

	printf("\n");
	return 0;
}

