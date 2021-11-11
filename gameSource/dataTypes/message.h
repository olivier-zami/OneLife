//
// Created by olivier on 08/11/2021.
//

#ifndef ONELIFE_GAME_DATATYPE_MESSAGE_H
#define ONELIFE_GAME_DATATYPE_MESSAGE_H

namespace OneLife::game::dataType
{
	typedef struct{
		unsigned int type;
		unsigned int size;
		void* content;
	}Message;
}

namespace OneLife::game::dataType::message
{
	typedef enum{
		NONE,
		KEYBOARD
	}TYPE;

	typedef struct{
		void* keyboard;
		void* mouse;
	}Device;
}

#endif //ONELIFE_GAME_DATATYPE_MESSAGE_H
