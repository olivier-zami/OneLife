//
// Created by olivier on 23/08/2022.
//

#include "Message.h"

//#include "../dataType/message_client.h"

oneLife::handler::Message::Message() {}

oneLife::handler::Message::~Message() {}

char* oneLife::handler::Message::getInputMessage()
{
	return this->inputMessage;
}

char* oneLife::handler::Message::getOutputMessage()
{
	return this->outputMessage;
}

oneLife::handler::Message* oneLife::handler::Message::sendMessage(char *message)
{
	this->outputMessage = message;
}


messageType getMessageType( char *inMessage )
{
	char *copy = stringDuplicate( inMessage );

	char *firstBreak = strstr( copy, "\n" );

	if( firstBreak == NULL ) {
		delete [] copy;
		return UNKNOWN;
	}

	firstBreak[0] = '\0';

	messageType returnValue = UNKNOWN;

	if( strcmp( copy, "CM" ) == 0 ) {
		returnValue = COMPRESSED_MESSAGE;
	}
	else if( strcmp( copy, "MC" ) == 0 ) {
		returnValue = MAP_CHUNK;
	}
	else if( strcmp( copy, "MX" ) == 0 ) {
		returnValue = MAP_CHANGE;
	}
	else if( strcmp( copy, "PU" ) == 0 ) {
		returnValue = PLAYER_UPDATE;
	}
	else if( strcmp( copy, "PM" ) == 0 ) {
		returnValue = PLAYER_MOVES_START;
	}
	else if( strcmp( copy, "PO" ) == 0 ) {
		returnValue = PLAYER_OUT_OF_RANGE;
	}
	else if( strcmp( copy, "PS" ) == 0 ) {
		returnValue = PLAYER_SAYS;
	}
	else if( strcmp( copy, "LS" ) == 0 ) {
		returnValue = LOCATION_SAYS;
	}
	else if( strcmp( copy, "PE" ) == 0 ) {
		returnValue = PLAYER_EMOT;
	}
	else if( strcmp( copy, "FX" ) == 0 ) {
		returnValue = FOOD_CHANGE;
	}
	else if( strcmp( copy, "HX" ) == 0 ) {
		returnValue = HEAT_CHANGE;
	}
	else if( strcmp( copy, "LN" ) == 0 ) {
		returnValue = LINEAGE;
	}
	else if( strcmp( copy, "CU" ) == 0 ) {
		returnValue = CURSED;
	}
	else if( strcmp( copy, "CX" ) == 0 ) {
		returnValue = CURSE_TOKEN_CHANGE;
	}
	else if( strcmp( copy, "CS" ) == 0 ) {
		returnValue = CURSE_SCORE;
	}
	else if( strcmp( copy, "NM" ) == 0 ) {
		returnValue = NAMES;
	}
	else if( strcmp( copy, "AP" ) == 0 ) {
		returnValue = APOCALYPSE;
	}
	else if( strcmp( copy, "AD" ) == 0 ) {
		returnValue = APOCALYPSE_DONE;
	}
	else if( strcmp( copy, "DY" ) == 0 ) {
		returnValue = DYING;
	}
	else if( strcmp( copy, "HE" ) == 0 ) {
		returnValue = HEALED;
	}
	else if( strcmp( copy, "MN" ) == 0 ) {
		returnValue = MONUMENT_CALL;
	}
	else if( strcmp( copy, "GV" ) == 0 ) {
		returnValue = GRAVE;
	}
	else if( strcmp( copy, "GM" ) == 0 ) {
		returnValue = GRAVE_MOVE;
	}
	else if( strcmp( copy, "GO" ) == 0 ) {
		returnValue = GRAVE_OLD;
	}
	else if( strcmp( copy, "OW" ) == 0 ) {
		returnValue = OWNER;
	}
	else if( strcmp( copy, "VS" ) == 0 ) {
		returnValue = VALLEY_SPACING;
	}
	else if( strcmp( copy, "FD" ) == 0 ) {
		returnValue = FLIGHT_DEST;
	}
	else if( strcmp( copy, "VU" ) == 0 ) {
		returnValue = VOG_UPDATE;
	}
	else if( strcmp( copy, "PH" ) == 0 ) {
		returnValue = PHOTO_SIGNATURE;
	}
	else if( strcmp( copy, "PONG" ) == 0 ) {
		returnValue = PONG;
	}
	else if( strcmp( copy, "SHUTDOWN" ) == 0 ) {
		returnValue = SHUTDOWN;
	}
	else if( strcmp( copy, "SERVER_FULL" ) == 0 ) {
		returnValue = SERVER_FULL;
	}
	else if( strcmp( copy, "SN" ) == 0 ) {
		returnValue = SEQUENCE_NUMBER;
	}
	else if( strcmp( copy, "ACCEPTED" ) == 0 ) {
		returnValue = ACCEPTED;
	}
	else if( strcmp( copy, "REJECTED" ) == 0 ) {
		returnValue = REJECTED;
	}
	else if( strcmp( copy, "NO_LIFE_TOKENS" ) == 0 ) {
		returnValue = NO_LIFE_TOKENS;
	}
	else if( strcmp( copy, "SD" ) == 0 ) {
		returnValue = FORCED_SHUTDOWN;
	}
	else if( strcmp( copy, "MS" ) == 0 ) {
		returnValue = GLOBAL_MESSAGE;
	}
	else if( strcmp( copy, "FL" ) == 0 ) {
		returnValue = FLIP;
	}
	else if( strcmp( copy, "CR" ) == 0 ) {
		returnValue = CRAVING;
	}

	delete [] copy;
	return returnValue;
}