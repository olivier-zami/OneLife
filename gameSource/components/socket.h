//
// Created by olivier on 23/10/2021.
//

#ifndef ONELIFE_SOCKET_H
#define ONELIFE_SOCKET_H

#include "../../../minorGems/util/SimpleVector.h"
#include "minorGems/network/SocketClient.h"
#include "../dataTypes/game.h"
#include "../dataTypes/socket.h"

namespace OneLife::game::component
{
	class Socket
	{
		public:
			Socket();
			~Socket();

			void handle(
					SimpleVector<unsigned char>* serverSocketBuffer = nullptr,
					int* bytesInCount = nullptr);

			void setAddress(OneLife::dataType::socket::Address address);
			OneLife::dataType::socket::Address getAddress();

			void connect();
			bool isConnected();
			bool isClosed();

			void sendMessage(const char* message);
			void sendMessage(OneLife::dataType::socket::Message message);
			char readMessage();
			double getLastQueryLifeTime();
			double getTimeLastMessageSent();
			void disconnect();
			void close();

			void resetStats();
			int getTotalServerBytesRead();
			int getTotalServerBytesSent();
			int getTotalServerOverheadBytesRead();
			int getTotalServerOverheadBytesSent();
			int getTotalServerMessageSent();

		private:
			OneLife::dataType::socket::Address address;
			int idSocket;
			char forceDisconnect;
			char serverSocketConnected;
			double connectedTime;
			double timeLastMessageSent;
			SimpleVector<unsigned char>* serverSocketBuffer;
			int numServerBytesRead;
			int numServerBytesSent;
			int overheadServerBytesRead;
			int overheadServerBytesSent;
			int numServerMessageSent;
			int bytesOutCount = 0;
			int* bytesInCount;
	};
}

typedef enum messageType {
	SHUTDOWN,
	SERVER_FULL,
	SEQUENCE_NUMBER,
	ACCEPTED,
	NO_LIFE_TOKENS,
	REJECTED,
	MAP_CHUNK,
	MAP_CHANGE,
	PLAYER_UPDATE,
	PLAYER_MOVES_START,
	PLAYER_OUT_OF_RANGE,
	PLAYER_SAYS,
	LOCATION_SAYS,
	PLAYER_EMOT,
	FOOD_CHANGE,
	HEAT_CHANGE,
	LINEAGE,
	CURSED,
	CURSE_TOKEN_CHANGE,
	CURSE_SCORE,
	NAMES,
	APOCALYPSE,
	APOCALYPSE_DONE,
	DYING,
	HEALED,
	MONUMENT_CALL,
	GRAVE,
	GRAVE_MOVE,
	GRAVE_OLD,
	OWNER,
	VALLEY_SPACING,
	FLIGHT_DEST,
	VOG_UPDATE,
	PHOTO_SIGNATURE,
	FORCED_SHUTDOWN,
	GLOBAL_MESSAGE,
	FLIP,
	CRAVING,
	PONG,
	COMPRESSED_MESSAGE,
	UNKNOWN
} messageType;

int readFromSocket( int inHandle, unsigned char *inDataBuffer, int inBytesToRead );
int openSocketConnection( const char *inNumericalAddress, int inPort );
int sendToSocket( int inHandle, unsigned char *inData, int inDataLength );
void closeSocket( int inHandle );
Socket *getSocketByHandle( int inHandle );
int readFromSocket( int inHandle, unsigned char *inDataBuffer, int inBytesToRead );
char *getNextServerMessageRaw();
char *getNextServerMessage();
messageType getMessageType( char *inMessage );
void replaceLastMessageSent( char *inNewMessage );
void startConnecting();
void playPendingReceivedMessages( LiveObject *inPlayer );
void playPendingReceivedMessagesRegardingOthers( LiveObject *inPlayer );
void dropPendingReceivedMessagesRegardingID( LiveObject *inPlayer, int inIDToDrop );


#endif //ONELIFE_SOCKET_H
