//
// Created by olivier on 08/11/2021.
//

#ifndef ONELIFE_GAME_COMPONENT_DEVICELISTENER_H
#define ONELIFE_GAME_COMPONENT_DEVICELISTENER_H

#include <SDL/SDL.h>
#include <vector>
#include <cstdint>
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
			std::vector<OneLife::dataType::message::Device> getEvent();
			bool receiveQuitSignal();
			void updateKeyboard(int key, int value);
			void updateMouse(SDL_MouseButtonEvent ev, int value);

		private:
			bool exit;
			std::vector<OneLife::dataType::message::Device> lastEvent;
			struct{
				size_t size;
				char* key;
			}keyboard;
			struct{
				uint32_t timestamp;
				struct{
					int x;
					int y;
					int z;//wheel
				}position;
				char leftButton;
				char rightButton;
				char middleButton;
			}mouse;

			int localValue(int sdlValue);

	};
}

#endif //ONELIFE_GAME_COMPONENT_DEVICELISTENER_H
