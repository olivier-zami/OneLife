//
// Created by olivier on 23/08/2022.
//

#include "SequenceNumber.h"

#include "../../../third_party/minorGems/util/stringUtils.h"

oneLife::handler::message::SequenceNumber::SequenceNumber(){}
oneLife::handler::message::SequenceNumber::~SequenceNumber(){}

oneLife::handler::message::SequenceNumber* oneLife::handler::message::SequenceNumber::sendMessage(
		oneLife::dataType::message::SequenceNumber sequenceNumber)
{
	this->outputMessage = autoSprintf( "SN\n%d/%d\n%s\n%lu#",
		   sequenceNumber.totalPlayers,
		   sequenceNumber.maxPlayers,
		   sequenceNumber.string,
		   sequenceNumber.serverVersion);
	printf("\ngenerate sequence number: %s", this->outputMessage);
	return this;
}