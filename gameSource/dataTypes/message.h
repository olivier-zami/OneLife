//
// Created by olivier on 08/11/2021.
//

#ifndef ONELIFE_GAME_DATATYPE_MESSAGE_H
#define ONELIFE_GAME_DATATYPE_MESSAGE_H

namespace OneLife::dataType
{
	typedef struct{
		unsigned int type;
		unsigned int size;
		void* content;
	}Message;

	typedef struct{
		unsigned int type;
		unsigned int target;
		char message[255];
	}Signal;
}

namespace OneLife::dataType::message
{
	typedef enum{
		NONE,
		KEYBOARD,
		DEATH
	}TYPE;

	typedef struct{
		void* keyboard;
		void* mouse;
	}Device;
}

#endif //ONELIFE_GAME_DATATYPE_MESSAGE_H
