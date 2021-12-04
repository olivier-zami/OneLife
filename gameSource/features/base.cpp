//
// Created by olivier on 02/12/2021.
//

#include "base.h"

#include <cstdio>
#include "OneLife/gameSource/dataTypes/feature.h"

OneLife::game::feature::Base::Base()
{
	this->type = OneLife::dataType::feature::Type::BASE;
}

OneLife::game::feature::Base::~Base() {}

OneLife::dataType::Message OneLife::game::feature::Base::getGameMessage()
{
	OneLife::dataType::Message message = {0};
	printf("\n===>attempt de create message from : %s", this->lastSocketMessage);
	if(this->isMessageTypeMatch("SN"))
	{
		printf("\n===>Parse SN => SEQUENCE NUMBER");
	}
	return message;
}
