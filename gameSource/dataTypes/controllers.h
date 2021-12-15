//
// Created by olivier on 15/12/2021.
//

#ifndef ONELIFE_GAME_CONTROLLERS_H
#define ONELIFE_GAME_CONTROLLERS_H

namespace OneLife::dataValue::Controller
{
	typedef enum{
		NONE,
		POLL_PAGE,
		GENETIC_HISTORY_PAGE,
		SETTINGS_PAGE,
		REVIEW_PAGE,
		TWIN_PAGE,
		FINAL_MESSAGE_PAGE,
		TUTORIAL
	}Type;
}

#endif //ONELIFE_GAME_CONTROLLERS_H
