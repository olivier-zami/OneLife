//
// Created by olivier on 05/05/2022.
//

#include "ToolBox.h"

OneLife::mapManager::window::ToolBox::ToolBox(SDL_Renderer* renderer)
{
	this->showWindow = true;
	if(renderer) this->renderer = renderer;
}


OneLife::mapManager::window::ToolBox::~ToolBox()
{

}

bool* OneLife::mapManager::window::ToolBox::getStatus()
{
	return &(this->showWindow);
}

void OneLife::mapManager::window::ToolBox::render()
{
	// 3. Show another simple window.
	if (this->showWindow)
	{
		ImGui::Begin("Another Window", &this->showWindow);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
		ImGui::Text("Hello from another window!");
		if (ImGui::Button("Close Me")) this->showWindow = false;
		ImGui::End();
	}
}