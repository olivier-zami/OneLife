//
// Created by olivier on 17/12/2021.
//

#include "apocalypse.h"

#include "OneLife/gameSource/debug/console.h"

OneLife::game::feature::Apocalypse::Apocalypse()
{
	this->isInProgress = nullptr;//this->isInProgress = false;
	this->frameRateFactor = nullptr;
	this->apocalypseDisplaySeconds = 6;
	this->apocalypseDisplayProgress = nullptr;
}

OneLife::game::feature::Apocalypse::~Apocalypse() {}

void OneLife::game::feature::Apocalypse::handleProgressStatus(int* progressStatus)
{
	this->isInProgress = progressStatus;
}

void OneLife::game::feature::Apocalypse::handleFrameRateFactor(double* frameRateFactor)
{
	this->frameRateFactor = frameRateFactor;
}

void OneLife::game::feature::Apocalypse::handleApocalypseDisplayProgress(double* apocalypseDisplayProgress)
{
	this->apocalypseDisplayProgress = apocalypseDisplayProgress;
}

void OneLife::game::feature::Apocalypse::update()
{
	//OneLife::debug::Console::write("OneLife::game::feature::Apocalypse::update()");
	if(this->isInProgress)
	{
		double stepSize = *(this->frameRateFactor) / ( this->apocalypseDisplaySeconds * 60.0 );
		*(this->apocalypseDisplayProgress) += stepSize;
	}
}