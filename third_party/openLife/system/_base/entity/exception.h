//
// Created by olivier on 20/10/2021.
//

#ifndef ONELIFE_EXCEPTION_H
#define ONELIFE_EXCEPTION_H

#include <exception>
#include <string>

namespace openLife::system
{
	class Exception : public std::exception
	{
		public:
			Exception(const char* message = nullptr, ...);
			~Exception();

			openLife::system::Exception* operator()(const char* message);

			std::string getMessage();

		private:
			std::string message;
	};
}

#endif //ONELIFE_EXCEPTION_H
