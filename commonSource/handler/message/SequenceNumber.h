//
// Created by olivier on 23/08/2022.
//

#ifndef oneLife_common_handler_message_sequenceNumber_H
#define oneLife_common_handler_message_sequenceNumber_H

#include "../../../third_party/minorGems/network/Socket.h"
//#include "../../../server/game/dataType/connection.h"
#include "../../dataType/message.h"
#include "../Message.h"


namespace oneLife::handler::message
{
	class SequenceNumber:
		virtual public ::oneLife::handler::Message
	{
		public:
			SequenceNumber();
			~SequenceNumber();

			oneLife::handler::message::SequenceNumber* sendMessage(oneLife::dataType::message::SequenceNumber sequenceNumber);
			virtual void to(FreshConnection newConnection) = 0;
	};
}

#endif //oneLife_common_handler_message_sequenceNumber_H
