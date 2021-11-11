//
// Created by olivier on 31/10/2021.
//

#ifndef ONELIFE_DATATYPE_HARDWARE_H
#define ONELIFE_DATATYPE_HARDWARE_H

namespace OneLife::dataType::hardware
{
	typedef enum{
		NONE		= 0,
		PRESS 		= 1,
		RELEASE 	= -1
	}SIGNAL;
	namespace keyboard
	{
		typedef enum{
			NONE		= 0,
			PRESS 		= 1,
			RELEASE 	= -1
		}KEY_ACTION;

		typedef enum{
			F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
			NUM_0, NUM_1, NUM_2, NUM_3, NUM_4, NUM_5, NUM_6, NUM_7, NUM_8, NUM_9,
			A,B,C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q,R,S,T,U,V,W,X,Y,Z,
			SPACE, TAB,
			ALT, ALT_LEFT, ALT_RIGHT,
			CTRL, CTRL_LEFT, CTRL_RIGHT,
			META, META_LEFT, META_RIGHT,//aka windows, command(apple), meta
			SHIFT, SHIFT_LEFT, SHIFT_RIGHT,
			NUM,
			CAPS,
			MODE,
			//KMOD_RESERVED = 0x8000
			RETURN,
			PERCENT,
			TOTAL_NUMBER //used to get number of key => should always be at the end
		}KEY;
	}
}

// FOVMOD NOTE:  Change 1/3 - Take these lines during the merge process
namespace MouseButton {
	enum {
		NONE,
		LEFT,
		MIDDLE,
		RIGHT,
		WHEELUP,
		WHEELDOWN
	};
}

#endif //ONELIFE_DATATYPE_HARDWARE_H
