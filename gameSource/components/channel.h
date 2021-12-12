//
// Created by olivier on 27/11/2021.
//

#ifndef ONELIFE_GAME_COMPONENT_MESSAGECHANNEL_H
#define ONELIFE_GAME_COMPONENT_MESSAGECHANNEL_H

#include <queue>
#include "../dataTypes/message.h"

namespace OneLife::game
{
	class Channel //Extend Pipe
	{
		public:
			Channel();
			~Channel();

			void setLastSignal(unsigned int lastSignal);
			unsigned int getLastSignal();

			void stackMessage(OneLife::dataType::Message message);
			OneLife::dataType::Message unstackMessage();
			void setInitMapMessage(const char* message);
			void setExitMapMessage(const char* message);

			void insert(OneLife::dataType::Message* message);
			OneLife::dataType::Message* output();

			void flush();

		private:
			unsigned int lastSignalValue;
			char waitingMessage[32];
			std::queue<OneLife::dataType::Message> message;
	};
}

#endif //ONELIFE_GAME_COMPONENT_MESSAGECHANNEL_H
