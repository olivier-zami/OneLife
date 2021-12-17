//
// Created by olivier on 29/11/2021.
//

#ifndef ONELIFE_GAME_DATATYPE_SIGNALS_H
#define ONELIFE_GAME_DATATYPE_SIGNALS_H

namespace OneLife::dataValue
{
	typedef enum{
		NONE,
		DONE,
		ACTIVATE,
		DISCONNECT,
		UNDEFINED,
		QUIT
	}Signal;
}

#endif //ONELIFE_GAME_DATATYPE_SIGNALS_H
