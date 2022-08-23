//
// Created by olivier on 23/08/2022.
//

#ifndef oneLife_common_handler_message_sequenceNumber_H
#define oneLife_common_handler_message_sequenceNumber_H

#include "../../../third_party/minorGems/network/Socket.h"
#include "../../../server/dataType/connection.h"
#include "../../dataType/message.h"


namespace oneLife::handler::message
{
	class SequenceNumber
	{
		public:
			SequenceNumber();
			~SequenceNumber();

			oneLife::handler::message::SequenceNumber* sendMessage(oneLife::dataType::message::SequenceNumber sequenceNumber);
			virtual void to(FreshConnection newConnection) = 0;

			char* getString();

		//protected:
			char *outputMessage;
	};
}

#endif //oneLife_common_handler_message_sequenceNumber_H
