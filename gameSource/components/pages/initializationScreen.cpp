//
// Created by olivier on 19/11/2021.
//

#include "initializationScreen.h"

#include "OneLife/gameSource/procedures/graphics/screens.h"

OneLife::game::InitializationScreen::InitializationScreen()
{
	this->taskComplete = false;
	this->minDuration = false;
	this->frameStartSec = 0;
	this->frameStartMSec = 0;
}

OneLife::game::InitializationScreen::~InitializationScreen() {}

void OneLife::game::InitializationScreen::handle(OneLife::game::dataType::ServerSocket* socket)
{
	this->socket = socket;
}

void OneLife::game::InitializationScreen::handle(OneLife::dataType::UiComponent* screen)
{
	screen->label = nullptr;
	screen->draw = OneLife::game::graphic::drawInitializingScreen;
	screen->body = &(this->screen);
	if(!this->frameStartSec)Time::getCurrentTime(&(this->frameStartSec), &(this->frameStartMSec));

	if(!this->minDuration)
	{
		timeSec_t frameCurrentSec;
		unsigned long frameCurrentMSec;
		Time::getCurrentTime(&frameCurrentSec, &frameCurrentMSec);
		if((frameCurrentSec - this->frameStartSec)>2) this->minDuration = true;
	}

	if(this->minDuration)this->taskComplete = true;
}

bool OneLife::game::InitializationScreen::isTaskComplete()
{
	return this->taskComplete;
}