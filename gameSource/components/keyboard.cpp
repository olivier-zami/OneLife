//
// Created by olivier on 30/10/2021.
//

#include "keyboard.h"

#include <SDL/SDL.h>

unsigned char keyMap[256];
char keyMapOn = true;

int mapSDLSpecialKeyToMG( int inSDLKey ) {
	switch( inSDLKey ) {
		case SDLK_F1: return MG_KEY_F1;
		case SDLK_F2: return MG_KEY_F2;
		case SDLK_F3: return MG_KEY_F3;
		case SDLK_F4: return MG_KEY_F4;
		case SDLK_F5: return MG_KEY_F5;
		case SDLK_F6: return MG_KEY_F6;
		case SDLK_F7: return MG_KEY_F7;
		case SDLK_F8: return MG_KEY_F8;
		case SDLK_F9: return MG_KEY_F9;
		case SDLK_F10: return MG_KEY_F10;
		case SDLK_F11: return MG_KEY_F11;
		case SDLK_F12: return MG_KEY_F12;
		case SDLK_LEFT: return MG_KEY_LEFT;
		case SDLK_UP: return MG_KEY_UP;
		case SDLK_RIGHT: return MG_KEY_RIGHT;
		case SDLK_DOWN: return MG_KEY_DOWN;
		case SDLK_PAGEUP: return MG_KEY_PAGE_UP;
		case SDLK_PAGEDOWN: return MG_KEY_PAGE_DOWN;
		case SDLK_HOME: return MG_KEY_HOME;
		case SDLK_END: return MG_KEY_END;
		case SDLK_INSERT: return MG_KEY_INSERT;
		case SDLK_RSHIFT: return MG_KEY_RSHIFT;
		case SDLK_LSHIFT: return MG_KEY_LSHIFT;
		case SDLK_RCTRL: return MG_KEY_RCTRL;
		case SDLK_LCTRL: return MG_KEY_LCTRL;
		case SDLK_RALT: return MG_KEY_RALT;
		case SDLK_LALT: return MG_KEY_LALT;
		case SDLK_RMETA: return MG_KEY_RMETA;
		case SDLK_LMETA: return MG_KEY_LMETA;
		default: return 0;
	}
}

char mapSDLKeyToASCII( int inSDLKey ) {
	// map world keys  (SDLK_WORLD_X) directly to ASCII
	if( inSDLKey >= 160 && inSDLKey <=255 ) {
		return inSDLKey;
	}


	switch( inSDLKey ) {
		case SDLK_UNKNOWN: return 0;
		case SDLK_BACKSPACE: return 8;
		case SDLK_TAB: return 9;
		case SDLK_CLEAR: return 12;
		case SDLK_RETURN: return 13;
		case SDLK_PAUSE: return 19;
		case SDLK_ESCAPE: return 27;
		case SDLK_SPACE: return ' ';
		case SDLK_EXCLAIM: return '!';
		case SDLK_QUOTEDBL: return '"';
		case SDLK_HASH: return '#';
		case SDLK_DOLLAR: return '$';
		case SDLK_AMPERSAND: return '&';
		case SDLK_QUOTE: return '\'';
		case SDLK_LEFTPAREN: return '(';
		case SDLK_RIGHTPAREN: return ')';
		case SDLK_ASTERISK: return '*';
		case SDLK_PLUS: return '+';
		case SDLK_COMMA: return ',';
		case SDLK_MINUS: return '-';
		case SDLK_PERIOD: return '.';
		case SDLK_SLASH: return '/';
		case SDLK_0: return '0';
		case SDLK_1: return '1';
		case SDLK_2: return '2';
		case SDLK_3: return '3';
		case SDLK_4: return '4';
		case SDLK_5: return '5';
		case SDLK_6: return '6';
		case SDLK_7: return '7';
		case SDLK_8: return '8';
		case SDLK_9: return '9';
		case SDLK_COLON: return ':';
		case SDLK_SEMICOLON: return ';';
		case SDLK_LESS: return '<';
		case SDLK_EQUALS: return '=';
		case SDLK_GREATER: return '>';
		case SDLK_QUESTION: return '?';
		case SDLK_AT: return '@';
		case SDLK_LEFTBRACKET: return '[';
		case SDLK_BACKSLASH: return '\\';
		case SDLK_RIGHTBRACKET: return ']';
		case SDLK_CARET: return '^';
		case SDLK_UNDERSCORE: return '_';
		case SDLK_BACKQUOTE: return '`';
		case SDLK_a: return 'a';
		case SDLK_b: return 'b';
		case SDLK_c: return 'c';
		case SDLK_d: return 'd';
		case SDLK_e: return 'e';
		case SDLK_f: return 'f';
		case SDLK_g: return 'g';
		case SDLK_h: return 'h';
		case SDLK_i: return 'i';
		case SDLK_j: return 'j';
		case SDLK_k: return 'k';
		case SDLK_l: return 'l';
		case SDLK_m: return 'm';
		case SDLK_n: return 'n';
		case SDLK_o: return 'o';
		case SDLK_p: return 'p';
		case SDLK_q: return 'q';
		case SDLK_r: return 'r';
		case SDLK_s: return 's';
		case SDLK_t: return 't';
		case SDLK_u: return 'u';
		case SDLK_v: return 'v';
		case SDLK_w: return 'w';
		case SDLK_x: return 'x';
		case SDLK_y: return 'y';
		case SDLK_z: return 'z';
		case SDLK_DELETE: return 127;
		case SDLK_WORLD_0:

		default: return 0;
	}
}