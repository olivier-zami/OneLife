//
// Created by olivier on 23/10/2021.
//

#ifndef ONELIFE_SOCKET_H
#define ONELIFE_SOCKET_H

#include "../../../minorGems/util/SimpleVector.h"
#include "OneLife/gameSource/dataTypes/web.h"
#include "minorGems/network/SocketClient.h"

namespace OneLife::game::component
{
	class Socket
	{
		public:
			Socket(
				SimpleVector<unsigned char>* serverSocketBuffer,
				int* bytesInCount,
				int* idServerSocket);

			~Socket();

			void connect(OneLife::game::dataType::ServerSocket socket);
			bool isConnected();
			void sendMessage(OneLife::game::dataType::Message message);
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
			int* idServerSocket;
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

int readFromSocket( int inHandle, unsigned char *inDataBuffer, int inBytesToRead );
int openSocketConnection( const char *inNumericalAddress, int inPort );
int sendToSocket( int inHandle, unsigned char *inData, int inDataLength );
void closeSocket( int inHandle );
Socket *getSocketByHandle( int inHandle );
int readFromSocket( int inHandle, unsigned char *inDataBuffer, int inBytesToRead );

#endif //ONELIFE_SOCKET_H
