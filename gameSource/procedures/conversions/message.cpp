//
// Created by olivier on 19/12/2021.
//

#include "message.h"

#include <cstring>
#include <cstdio>
#include "minorGems/util/stringUtils.h"
#include "OneLife/gameSource/dataTypes/values/message.h"
#include "OneLife/gameSource/debug/console.h"

using MESSAGE_TYPE = OneLife::data::value::message::Type;

OneLife::data::value::message::Type OneLife::procedure::conversion::getMessageType(const char *inMessage )
{
	char *copy = stringDuplicate( inMessage );

	char *firstBreak = strstr( copy, "\n" );

	if( firstBreak == NULL ) {
		delete [] copy;
		return MESSAGE_TYPE::UNKNOWN;
	}

	firstBreak[0] = '\0';

	OneLife::data::value::message::Type returnValue = MESSAGE_TYPE::UNKNOWN;

	if( strcmp( copy, "CM" ) == 0 ) {
		returnValue = MESSAGE_TYPE::COMPRESSED_MESSAGE;
	}
	else if( strcmp( copy, "MC" ) == 0 ) {
		returnValue = MESSAGE_TYPE::MAP_CHUNK;
	}
	else if( strcmp( copy, "MX" ) == 0 ) {
		returnValue = MESSAGE_TYPE::MAP_CHANGE;
	}
	else if( strcmp( copy, "PU" ) == 0 ) {
		returnValue = MESSAGE_TYPE::PLAYER_UPDATE;
	}
	else if( strcmp( copy, "PM" ) == 0 ) {
		returnValue = MESSAGE_TYPE::PLAYER_MOVES_START;
	}
	else if( strcmp( copy, "PO" ) == 0 ) {
		returnValue = MESSAGE_TYPE::PLAYER_OUT_OF_RANGE;
	}
	else if( strcmp( copy, "BW" ) == 0 ) {
		returnValue = MESSAGE_TYPE::BABY_WIGGLE;
	}
	else if( strcmp( copy, "PS" ) == 0 ) {
		returnValue = MESSAGE_TYPE::PLAYER_SAYS;
	}
	else if( strcmp( copy, "LS" ) == 0 ) {
		returnValue = MESSAGE_TYPE::LOCATION_SAYS;
	}
	else if( strcmp( copy, "PE" ) == 0 ) {
		returnValue = MESSAGE_TYPE::PLAYER_EMOT;
	}
	else if( strcmp( copy, "FX" ) == 0 ) {
		returnValue = MESSAGE_TYPE::FOOD_CHANGE;
	}
	else if( strcmp( copy, "HX" ) == 0 ) {
		returnValue = MESSAGE_TYPE::HEAT_CHANGE;
	}
	else if( strcmp( copy, "LN" ) == 0 ) {
		returnValue = MESSAGE_TYPE::LINEAGE;
	}
	else if( strcmp( copy, "CU" ) == 0 ) {
		returnValue = MESSAGE_TYPE::CURSED;
	}
	else if( strcmp( copy, "CX" ) == 0 ) {
		returnValue = MESSAGE_TYPE::CURSE_TOKEN_CHANGE;
	}
	else if( strcmp( copy, "CS" ) == 0 ) {
		returnValue = MESSAGE_TYPE::CURSE_SCORE;
	}
	else if( strcmp( copy, "NM" ) == 0 ) {
		returnValue = MESSAGE_TYPE::NAMES;
	}
	else if( strcmp( copy, "AP" ) == 0 ) {
		returnValue = MESSAGE_TYPE::APOCALYPSE;
	}
	else if( strcmp( copy, "AD" ) == 0 ) {
		returnValue = MESSAGE_TYPE::APOCALYPSE_DONE;
	}
	else if( strcmp( copy, "DY" ) == 0 ) {
		returnValue = MESSAGE_TYPE::DYING;
	}
	else if( strcmp( copy, "HE" ) == 0 ) {
		returnValue = MESSAGE_TYPE::HEALED;
	}
	else if( strcmp( copy, "PJ" ) == 0 ) {
		returnValue = MESSAGE_TYPE::POSSE_JOIN;
	}
	else if( strcmp( copy, "MN" ) == 0 ) {
		returnValue = MESSAGE_TYPE::MONUMENT_CALL;
	}
	else if( strcmp( copy, "GV" ) == 0 ) {
		returnValue = MESSAGE_TYPE::GRAVE;
	}
	else if( strcmp( copy, "GM" ) == 0 ) {
		returnValue = MESSAGE_TYPE::GRAVE_MOVE;
	}
	else if( strcmp( copy, "GO" ) == 0 ) {
		returnValue = MESSAGE_TYPE::GRAVE_OLD;
	}
	else if( strcmp( copy, "OW" ) == 0 ) {
		returnValue = MESSAGE_TYPE::OWNER;
	}
	else if( strcmp( copy, "FW" ) == 0 ) {
		returnValue = MESSAGE_TYPE::FOLLOWING;
	}
	else if( strcmp( copy, "EX" ) == 0 ) {
		returnValue = MESSAGE_TYPE::EXILED;
	}
	else if( strcmp( copy, "VS" ) == 0 ) {
		returnValue = MESSAGE_TYPE::VALLEY_SPACING;
	}
	else if( strcmp( copy, "FD" ) == 0 ) {
		returnValue = MESSAGE_TYPE::FLIGHT_DEST;
	}
	else if( strcmp( copy, "BB" ) == 0 ) {
		returnValue = MESSAGE_TYPE::BAD_BIOMES;
	}
	else if( strcmp( copy, "VU" ) == 0 ) {
		returnValue = MESSAGE_TYPE::VOG_UPDATE;
	}
	else if( strcmp( copy, "PH" ) == 0 ) {
		returnValue = MESSAGE_TYPE::PHOTO_SIGNATURE;
	}
	else if( strcmp( copy, "PONG" ) == 0 ) {
		returnValue = MESSAGE_TYPE::PONG;
	}
	else if( strcmp( copy, "SHUTDOWN" ) == 0 ) {
		returnValue = MESSAGE_TYPE::SHUTDOWN;
	}
	else if( strcmp( copy, "SERVER_FULL" ) == 0 ) {
		returnValue = MESSAGE_TYPE::SERVER_FULL;
	}
	else if( strcmp( copy, "SN" ) == 0 ) {
		returnValue = MESSAGE_TYPE::SEQUENCE_NUMBER;
	}
	else if( strcmp( copy, "ACCEPTED" ) == 0 ) {
		returnValue = MESSAGE_TYPE::ACCEPTED;
	}
	else if( strcmp( copy, "REJECTED" ) == 0 ) {
		returnValue = MESSAGE_TYPE::REJECTED;
	}
	else if( strcmp( copy, "NO_LIFE_TOKENS" ) == 0 ) {
		returnValue = MESSAGE_TYPE::NO_LIFE_TOKENS;
	}
	else if( strcmp( copy, "SD" ) == 0 ) {
		returnValue = MESSAGE_TYPE::FORCED_SHUTDOWN;
	}
	else if( strcmp( copy, "MS" ) == 0 ) {
		returnValue = MESSAGE_TYPE::GLOBAL_MESSAGE;
	}
	else if( strcmp( copy, "WR" ) == 0 ) {
		returnValue = MESSAGE_TYPE::WAR_REPORT;
	}
	else if( strcmp( copy, "LR" ) == 0 ) {
		returnValue = MESSAGE_TYPE::LEARNED_TOOL_REPORT;
	}
	else if( strcmp( copy, "TE" ) == 0 ) {
		returnValue = MESSAGE_TYPE::TOOL_EXPERTS;
	}
	else if( strcmp( copy, "TS" ) == 0 ) {
		returnValue = MESSAGE_TYPE::TOOL_SLOTS;
	}
	else if( strcmp( copy, "HL" ) == 0 ) {
		returnValue = MESSAGE_TYPE::HOMELAND;
	}
	else if( strcmp( copy, "FL" ) == 0 ) {
		returnValue = MESSAGE_TYPE::FLIP;
	}
	else if( strcmp( copy, "CR" ) == 0 ) {
		returnValue = MESSAGE_TYPE::CRAVING;
	}

	delete [] copy;
	return returnValue;
}

void OneLife::procedure::conversion::setMapChunkMessage(OneLife::data::type::message::MapChunk* mapChunk, const char* socketMessage)
{
	int binarySize = 0;
	int compressedSize = 0;

	sscanf(socketMessage, "MC\n%d %d %d %d\n%d %d\n",
			&((*mapChunk).dimension.width),
			&((*mapChunk).dimension.height),
			&((*mapChunk).topLeftPosition.x),
			&((*mapChunk).topLeftPosition.y), &binarySize, &compressedSize);
}