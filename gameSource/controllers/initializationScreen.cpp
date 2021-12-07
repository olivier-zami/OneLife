//
// Created by olivier on 19/11/2021.
//

#include "initializationScreen.h"

#include "OneLife/gameSource/procedures/graphics/screens.h"
#include "OneLife/gameSource/dataTypes/exception/exception.h"
#include "OneLife/gameSource/dataTypes/signals.h"
#include "OneLife/gameSource/debug.h"

using signal = OneLife::dataType::Signal;

extern SimpleVector<unsigned char> serverSocketBuffer; //serverSocketBuffer
extern int bytesInCount;//bytesInCount

OneLife::game::InitializationScreen::InitializationScreen()
{
	this->status.isConfigurationLoaded = false;
	this->status.isSocketSet = false;
	this->status.isInitSceneGeneratorController = false;
	this->status.isInitGameSceneController = false;
	this->controller.sceneGeneratorController = nullptr;
	this->controller.gameSceneController = nullptr;
	this->socket = nullptr;
	this->taskComplete = false;
	this->minDuration = false;
	this->frameStartSec = 0;
	this->frameStartMSec = 0;
}

OneLife::game::InitializationScreen::~InitializationScreen() {}

/**********************************************************************************************************************/

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

	if(!this->status.isSocketSet)
	{
		this->initSocket();
	}
	else if(!this->status.isInitSceneGeneratorController)
	{
		this->initSceneGeneratorController();
	}
	else if(!this->status.isInitGameSceneController)
	{
		this->initGameSceneController();
	}
	else if(this->minDuration)
	{
		this->sendSignal(signal::DONE);
		this->taskComplete = true;
	}
}

void OneLife::game::InitializationScreen::handle(OneLife::game::component::Socket* socket)
{
	this->socket = socket;
}

void OneLife::game::InitializationScreen::handle(OneLife::game::WaitingScreen* controller)
{
	this->controller.sceneGeneratorController = controller;
}

void OneLife::game::InitializationScreen::handle(LivingLifePage* controller)
{
	this->controller.gameSceneController = controller;
}

/**********************************************************************************************************************/

void OneLife::game::InitializationScreen::setServerSocketAddress(OneLife::dataType::socket::Address socket)
{
	this->data.socket = socket;
}

/**********************************************************************************************************************/

void OneLife::game::InitializationScreen::initSocket()
{
	OneLife::game::Debug::writeControllerStepInfo("Trying to set Socket Data");
	if(!this->socket)
	{
		this->socket = new OneLife::game::component::Socket();
		this->socket->handle();
		this->socket->handle(
				&serverSocketBuffer,
				&bytesInCount
				);
	}

	//TODO: ping server before set this->status.isSocketSet = true;
	this->status.isSocketSet = true;
}

void OneLife::game::InitializationScreen::initSceneGeneratorController()
{
	if(!this->controller.sceneGeneratorController)
	{
		OneLife::game::Debug::writeControllerStepInfo("generate sceneLoaderController");
		this->controller.sceneGeneratorController = new OneLife::game::WaitingScreen();
		this->controller.sceneGeneratorController->handle(this->socket);
	}
	this->status.isInitSceneGeneratorController = true;
}

void OneLife::game::InitializationScreen::initGameSceneController()
{
	if(!this->controller.gameSceneController)
	{
		OneLife::game::Debug::writeControllerStepInfo("generate gameSceneController");
		this->controller.gameSceneController = new LivingLifePage();
	}
	this->status.isInitGameSceneController = true;
}

bool OneLife::game::InitializationScreen::isTaskComplete()
{
	return this->taskComplete;
}