//
// Created by olivier on 25/08/2022.
//

#include "ServerInfo.h"

oneLife::handler::message::ServerInfo::ServerInfo() {}

oneLife::handler::message::ServerInfo::~ServerInfo() {}

oneLife::handler::message::ServerInfo * oneLife::handler::message::ServerInfo::sendMessage(
		oneLife::dataType::message::ServerInfo serverInfo)
{
	this->outputMessage = autoSprintf( "SI\n%lu#",
			serverInfo.totalBiomes);
	printf("\ngenerate sequence number: %s", this->outputMessage);
	return this;
}
