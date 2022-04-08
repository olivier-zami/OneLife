//
// Created by olivier on 07/04/2022.
//

#include "MapWindow.h"

OneLife::mapManager::MapWindow::MapWindow()
{
	this->showWindow = true;
}

OneLife::mapManager::MapWindow::~MapWindow() {}

void OneLife::mapManager::MapWindow::render()
{
	ImGui::Begin("World Map", &(this->showWindow));   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
	ImGui::Text("Hello from world map window!");
	if (ImGui::Button("Close Me"))
		this->showWindow = false;
	ImGui::End();
}
