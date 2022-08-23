//
// Created by olivier on 19/08/2022.
//

#ifndef ONE_LIFE__GAME__APPLICATION_H
#define ONE_LIFE__GAME__APPLICATION_H

#include "handler/Socket.h"

#include <map>
#include <string>

namespace oneLife::game
{
	class Application
	{
		public:
			Application();
			~Application();

			const char* getDirectory(std::string name);
			void setDirectory(std::string name, std::string path);

		protected:
			std::map<std::string, std::string>* directory;
			::oneLife::client::game::handler::Socket* socket;
	};
}

char getCountingOnVsync();
void loadingComplete();
void loadingFailed( const char *inFailureMessage );
void saveFrameRateSettings();

#endif //ONE_LIFE__GAME__APPLICATION_H
