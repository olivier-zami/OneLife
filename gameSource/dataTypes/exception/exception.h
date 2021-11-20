//
// Created by olivier on 02/11/2021.
//

#ifndef ONELIFE_GAME_EXCEPTION_H
#define ONELIFE_GAME_EXCEPTION_H

//#include "OneLife/third_party/openLife/base/entity/exception.cpp"

namespace OneLife::game
{
	class Exception //: public openLife::Exception::Exception
	{
		public:
			Exception(const char* message);
			~Exception();

			const char* getMessage();

		private:
			char message[255];
	};
}

#endif //ONELIFE_GAME_EXCEPTION_H
