//
// Created by olivier on 08/11/2021.
//

#ifndef ONELIFE_GAME_COMPONENT_DEVICELISTENER_H
#define ONELIFE_GAME_COMPONENT_DEVICELISTENER_H

#include <vector>
#include "../../dataTypes/message.h"

namespace OneLife::game
{
	class DeviceListener
	{
		public:
			DeviceListener();
			~DeviceListener();

			void listen();
			void reset();
			std::vector<OneLife::game::dataType::Message> getEvent();
			bool receiveQuitSignal();
			void updateKeyboard(int key, int value);

		private:
			bool exit;
			std::vector<OneLife::game::dataType::Message> lastEvent;
			struct{
				size_t size;
				char* key;
			}keyboard;

			int localValue(int sdlValue);

	};
}

#endif //ONELIFE_GAME_COMPONENT_DEVICELISTENER_H
