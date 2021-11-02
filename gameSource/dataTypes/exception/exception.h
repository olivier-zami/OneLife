//
// Created by olivier on 02/11/2021.
//

#ifndef ONELIFE_GAME_DATA_EXCEPTION_H
#define ONELIFE_GAME_DATA_EXCEPTION_H

//#include "OneLife/third_party/openLife/base/entity/exception.cpp"

namespace OneLife::game::dataType
{
	class Exception //: public openLife::Exception::Exception
	{
		public:
			Exception(const char* message);
			~Exception();
	};
}

#endif //ONELIFE_GAME_DATA_EXCEPTION_H
