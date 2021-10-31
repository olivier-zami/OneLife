//
// Created by olivier on 30/10/2021.
//

#ifndef INC_2HOL_KEYBOARD_H
#define INC_2HOL_KEYBOARD_H

namespace OneLife::component
{
	class Keyboard
	{
		public:
			Keyboard();
			~Keyboard();

	};
}

// maps SDL-specific special (non-ASCII) key-codes (SDLK) to minorGems key
// codes (MG_KEY)
int mapSDLSpecialKeyToMG( int inSDLKey );

// for ascii key
char mapSDLKeyToASCII( int inSDLKey );

int mapSDLSpecialKeyToMG( int inSDLKey );
char mapSDLKeyToASCII( int inSDLKey );

// special key codes
#define  MG_KEY_F1                        0x0001
#define  MG_KEY_F2                        0x0002
#define  MG_KEY_F3                        0x0003
#define  MG_KEY_F4                        0x0004
#define  MG_KEY_F5                        0x0005
#define  MG_KEY_F6                        0x0006
#define  MG_KEY_F7                        0x0007
#define  MG_KEY_F8                        0x0008
#define  MG_KEY_F9                        0x0009
#define  MG_KEY_F10                       0x000A
#define  MG_KEY_F11                       0x000B
#define  MG_KEY_F12                       0x000C
#define  MG_KEY_LEFT                      0x0064
#define  MG_KEY_UP                        0x0065
#define  MG_KEY_RIGHT                     0x0066
#define  MG_KEY_DOWN                      0x0067
#define  MG_KEY_PAGE_UP                   0x0068
#define  MG_KEY_PAGE_DOWN                 0x0069
#define  MG_KEY_HOME                      0x006A
#define  MG_KEY_END                       0x006B
#define  MG_KEY_INSERT                    0x006C

#define  MG_KEY_RSHIFT                    0x0070
#define  MG_KEY_LSHIFT                    0x0071
#define  MG_KEY_RCTRL                     0x0072
#define  MG_KEY_LCTRL                     0x0073
#define  MG_KEY_RALT                      0x0074
#define  MG_KEY_LALT                      0x0075
#define  MG_KEY_RMETA                     0x0076
#define  MG_KEY_LMETA                     0x0077

#endif //INC_2HOL_KEYBOARD_H
