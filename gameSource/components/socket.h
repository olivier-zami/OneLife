//
// Created by olivier on 23/10/2021.
//

#ifndef ONELIFE_SOCKET_H
#define ONELIFE_SOCKET_H

#include "../../../minorGems/util/SimpleVector.h"
#include "minorGems/network/SocketClient.h"
#include "OneLife/gameSource/dataTypes/type.h"
#include "OneLife/gameSource/dataTypes/game.h"
#include "OneLife/gameSource/dataTypes/socket.h"

namespace OneLife::game::component
{
	class Socket
	{
		public:
			Socket();
			~Socket();

			void handle(int* bytesInCount = nullptr);

			void setAddress(OneLife::game::dataType::socket::Address address);
			OneLife::game::dataType::socket::Address getAddress();
			void enablePendingMode(bool status);
			bool isPendingModeEnabled();

			void connect();
			bool isConnected();
			char readMessage();
			void sendMessage(OneLife::game::dataType::socket::Message message);
			OneLife::data::type::Message getMessage(const char* message = nullptr);
			char *getNextMessage();
			void disconnect();
			void reset(int mask = 0);
			void close();
			char* _read();

			char *getNextServerMessageRaw();
			double getLastQueryLifeTime();
			double getTimeLastMessageSent();
			void deleteAllMessages();
			int getTotalServerBytesRead();
			int getTotalServerBytesSent();
			int getTotalServerOverheadBytesRead();
			int getTotalServerOverheadBytesSent();
			int getTotalServerMessageSent();

		private:
			void computeLastMessageLength();
			int id;
			struct{
				bool isConnected;
				bool isPendingModeEnabled;
				bool isTerminalCharacterFound;
				bool isStartMessageReading;
			}status;
			OneLife::game::dataType::socket::Address address;
			size_t lastMessageMaxSize;
			OneLife::data::type::Message lastMessage;
		public:
			SimpleVector<unsigned char> serverSocketBuffer;
		private:
			OneLife::data::type::Message currentMessage;

			double connectedTime;
			double timeLastMessageSent;
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
void replaceLastMessageSent( char *inNewMessage );
void startConnecting();
void playPendingReceivedMessages( LiveObject *inPlayer );
void playPendingReceivedMessagesRegardingOthers( LiveObject *inPlayer );
void dropPendingReceivedMessagesRegardingID( LiveObject *inPlayer, int inIDToDrop );


#endif //ONELIFE_SOCKET_H
