//
// Created by olivier on 24/08/2022.
//

#ifndef oneLife_client_game_dataType_Settings_H
#define oneLife_client_game_dataType_Settings_H

namespace oneLife::game::dataType
{
	typedef struct{
		bool useCustomServer;
		struct{
			char* ip;
			int port;
		}server;
	}Settings;
}

#endif //oneLife_client_game_dataType_Settings_H
