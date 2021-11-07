//
// Created by olivier on 31/10/2021.
//

#ifndef ONELIFE_DATATYPE_HARDWARE_H
#define ONELIFE_DATATYPE_HARDWARE_H

namespace OneLife::dataType::hardware
{
	namespace Keyboard
	{
		typedef enum{
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
