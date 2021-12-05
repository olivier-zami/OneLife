//
// Created by olivier on 05/12/2021.
//

#include "casting.h"

OneLife::game::Casting::Casting()
{
	this->recentInsertedGameObjectIndex = -1;
}

OneLife::game::Casting::~Casting() {}

void OneLife::game::Casting::setIndexRecentlyInsertedGameObject(unsigned int idx)
{
	this->recentInsertedGameObjectIndex = idx;
}

unsigned int OneLife::game::Casting::getIndexRecentlyInsertedGameObject()
{
	return this->recentInsertedGameObjectIndex;
}