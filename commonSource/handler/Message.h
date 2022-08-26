//
// Created by olivier on 23/08/2022.
//

#ifndef oneLife_common_handler_message_H
#define oneLife_common_handler_message_H

#include "../dataType/messageType.h"
#include "../dataType/socket.h"

namespace oneLife::handler
{
	class Message
	{
		public:
			Message();
			~Message();

			virtual char* getInputMessage();
			virtual char* getOutputMessage();
			Message* sendMessage(char* message);
			virtual void to(FreshConnection newConnection) = 0;

		protected:
			char* outputMessage;
			char* inputMessage;

	};
}

messageType getMessageType( char *inMessage );//TODO: put this in handler/Message.h

#endif //oneLife_common_handler_message_H
