//
// Created by olivier on 23/10/2021.
//

#ifndef ONELIFE_SOCKET_H
#define ONELIFE_SOCKET_H

#include "../../../minorGems/util/SimpleVector.h"
#include "minorGems/network/SocketClient.h"
#include "OneLife/gameSource/dataTypes/message.h"
#include "OneLife/gameSource/dataTypes/game.h"
#include "OneLife/gameSource/dataTypes/socket.h"

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

			void setAddress(OneLife::game::dataType::socket::Address address);
			OneLife::game::dataType::socket::Address getAddress();
			void enablePendingMode(bool status);
			bool isPendingModeEnabled();

			void connect();
			bool isConnected();
			char readMessage();
			void sendMessage(OneLife::game::dataType::socket::Message message);
			OneLife::game::dataType::Message getMessage(const char* message = nullptr);
			char *getNextMessage();
			void disconnect();
			void reset(int mask = 0);
			void close();

			double getLastQueryLifeTime();
			double getTimeLastMessageSent();
			void deleteAllMessages();
			int getTotalServerBytesRead();
			int getTotalServerBytesSent();
			int getTotalServerOverheadBytesRead();
			int getTotalServerOverheadBytesSent();
			int getTotalServerMessageSent();

		private:
			int id;
			struct{
				bool isConnected;
				bool isPendingModeEnabled;
			}status;
			OneLife::game::dataType::socket::Address address;
			OneLife::game::dataType::Message currentMessage;
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

int readFromSocket( int inHandle, unsigned char *inDataBuffer, int inBytesToRead );
int openSocketConnection( const char *inNumericalAddress, int inPort );
int sendToSocket( int inHandle, unsigned char *inData, int inDataLength );
void closeSocket( int inHandle );
Socket *getSocketByHandle( int inHandle );
int readFromSocket( int inHandle, unsigned char *inDataBuffer, int inBytesToRead );
char *getNextServerMessageRaw();
void replaceLastMessageSent( char *inNewMessage );
void startConnecting();
void playPendingReceivedMessages( LiveObject *inPlayer );
void playPendingReceivedMessagesRegardingOthers( LiveObject *inPlayer );
void dropPendingReceivedMessagesRegardingID( LiveObject *inPlayer, int inIDToDrop );


#endif //ONELIFE_SOCKET_H
