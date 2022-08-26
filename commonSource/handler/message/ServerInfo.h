//
// Created by olivier on 25/08/2022.
//

#ifndef oneLife_common_handler_message_serverInfo_H
#define oneLife_common_handler_message_serverInfo_H

#include "../../../third_party/minorGems/network/Socket.h"
#include "../../dataType/message.h"
#include "../Message.h"

namespace oneLife::handler::message
{
	class ServerInfo:
		virtual public ::oneLife::handler::Message
	{
		public:
			ServerInfo();
			~ServerInfo();

			oneLife::handler::message::ServerInfo* sendMessage(oneLife::dataType::message::ServerInfo serverInfo);
			virtual void to(FreshConnection newConnection) = 0;
	};
}

#endif //oneLife_common_handler_message_serverInfo_H
