//
// Created by olivier on 22/03/2022.
//

#ifndef ONELIFE_GAME_COMPONENT_SOCKET_H
#define ONELIFE_GAME_COMPONENT_SOCKET_H

namespace oneLife::game::component
{
	class Socket
	{
		public:
			Socket();
			~Socket();

			void readMessage(const char* message);
	};
}

#endif //ONELIFE_GAME_COMPONENT_SOCKET_H
