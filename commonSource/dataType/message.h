//
// Created by olivier on 04/05/2022.
//

#ifndef oneLife_common_dataType_message_H
#define oneLife_common_dataType_message_H

namespace oneLife::dataType::message
{
	typedef struct
	{
		struct{
			int x;
			int y;
		}origin;
		struct{
			int width;
			int height;
		}dimension;
	}MapChunk;

	typedef struct
	{
		int totalPlayers;
		int maxPlayers;
		char* string;
		int serverVersion;
	}SequenceNumber;

	typedef struct{
		unsigned int totalBiomes;
	}ServerInfo;
}

#endif //oneLife_common_dataType_message_H
