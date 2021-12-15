//
// Created by olivier on 19/11/2021.
//

#include "initScreen.h"

#include "OneLife/gameSource/procedures/graphics/screens.h"
#include "OneLife/gameSource/dataTypes/exception/exception.h"
#include "OneLife/gameSource/dataTypes/signals.h"
#include "OneLife/gameSource/debug.h"

using signal = OneLife::dataValue::Signal;

extern SimpleVector<unsigned char> serverSocketBuffer; //serverSocketBuffer
extern int bytesInCount;//bytesInCount

OneLife::game::initScreen::initScreen()
{
	this->status.isConfigurationLoaded = false;
	this->status.isInitGraphicContext = false;
	this->status.isInitSocket = false;
	this->controller.sceneBuilder = nullptr;
	this->controller.gameScene = nullptr;
	this->socket = nullptr;
	this->minDuration = false;
	this->frameStartSec = 0;
	this->frameStartMSec = 0;
}

OneLife::game::initScreen::~initScreen() {}

/**********************************************************************************************************************/

void OneLife::game::initScreen::handle(OneLife::dataType::UiComponent* screen)
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

	if(!this->status.isInitGraphicContext)
	{
		this->initGraphicContext();
	}
	else if(!this->status.isInitSocket)
	{
		this->initSocket();
	}
	else if(this->minDuration)
	{
		this->sendSignal(signal::DONE);
	}
}

void OneLife::game::initScreen::handle(OneLife::game::component::Socket** socket)
{
	this->socket = socket;
}

/**********************************************************************************************************************/

void OneLife::game::initScreen::setServerSocketAddress(OneLife::dataType::socket::Address socket)
{
	this->data.socket = socket;
}

/**********************************************************************************************************************/

void OneLife::game::initScreen::initGraphicContext()
{
	OneLife::game::Debug::writeControllerStepInfo("OneLife::game::initScreen::initGraphicContext()");
	this->status.isInitGraphicContext = true;
}

void OneLife::game::initScreen::initSocket()
{
	OneLife::game::Debug::writeControllerStepInfo("OneLife::game::initScreen::initSocket()");
	if(!(*(this->socket)))
	{
		*(this->socket) = new OneLife::game::component::Socket();
		(*(this->socket))->handle();
		(*(this->socket))->handle(
				&serverSocketBuffer,
				&bytesInCount
				);
	}

	(*(this->socket))->setAddress(this->data.socket);
	//TODO: ping server before set this->status.isInitSocket = true;

	this->status.isInitSocket = true;
}