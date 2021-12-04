//
// Created by olivier on 07/11/2021.
//

#ifndef ONELIFE_GAME_H
#define ONELIFE_GAME_H

#include <SDL/SDL_main.h>// let SDL override our main function with SDLMain

int mainFunction( int inArgCount, char **inArgs );// must do this before SDL include to prevent WinMain linker errors on win32

#endif //ONELIFE_GAME_H
