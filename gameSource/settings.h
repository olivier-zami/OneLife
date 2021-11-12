//
// Created by olivier on 11/11/2021.
//

#ifndef ONELIFE_GAME_SETTINGS_H
#define ONELIFE_GAME_SETTINGS_H

namespace OneLife::game
{
	typedef struct{
		bool useSound;
	}Settings;
}

char getUsesSound();
char doesOverrideGameImageSize();
void getGameImageSize( int *outWidth, int *outHeight );//legacy: only called if doesOverrideGameImageSize returns true

#endif //ONELIFE_GAME_SETTINGS_H
