//
// Created by olivier on 19/12/2021.
//

#ifndef ONELIFE_PROCEDURE_CONVERSION_MESSAGE_H
#define ONELIFE_PROCEDURE_CONVERSION_MESSAGE_H

#include "OneLife/gameSource/dataTypes/types/message.h"
#include "OneLife/gameSource/dataTypes/values/message.h"

namespace OneLife::procedure::conversion
{
	OneLife::data::value::message::Type getMessageType(const char *inMessage );
	void setMapChunkMessage(OneLife::data::type::message::MapChunk* mapChunk, const char* socketMessage);
}

#endif //ONELIFE_PROCEDURE_CONVERSION_MESSAGE_H
