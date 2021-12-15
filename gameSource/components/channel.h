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

			void setLastSignal(unsigned int lastSignal, unsigned int target=0, const char* message=nullptr);
			OneLife::dataType::Signal getLastSignal();

			void stackMessage(OneLife::dataType::Message message);
			OneLife::dataType::Message unstackMessage();
			void setInitMapMessage(const char* message);
			void setExitMapMessage(const char* message);

			void insert(OneLife::dataType::Message* message);
			OneLife::dataType::Message* output();

			void flush();

		private:
			OneLife::dataType::Signal lastSignal;
			unsigned int lastSignalValue;
			unsigned int lastSignalTarget;
			char lastSignalMessage[254];
			char waitingMessage[32];
			std::queue<OneLife::dataType::Message> message;

			static const unsigned int SIZE_MESSAGE;
	};
}

#endif //ONELIFE_GAME_COMPONENT_MESSAGECHANNEL_H
