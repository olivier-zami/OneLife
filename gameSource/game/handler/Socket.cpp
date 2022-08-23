//
// Created by olivier on 23/08/2022.
//

#include "Socket.h"

#include "../../../third_party/minorGems/game/game.h"
#include "../../../third_party/minorGems/util/SimpleVector.h"
#include "../../../third_party/minorGems/util/stringUtils.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

int bytesInCount = 0;
int bytesOutCount = 0;
double connectedTime = 0;
char forceDisconnect = false;
char *lastMessageSentToServer = nullptr;
int messagesOutCount = 0;
int numServerBytesRead = 0;
int numServerBytesSent = 0;
int overheadServerBytesSent = 0;
char serverSocketConnected = false;
SimpleVector<unsigned char> serverSocketBuffer;
double timeLastMessageSent = 0;

namespace oneLife::debug
{
	static void print(SimpleVector<unsigned char> string)
	{
		char* bufferContent;
		bufferContent = (char*)malloc(string.size()*sizeof(char)+1);
		memset(bufferContent, 0, string.size()+1);
		for(int i=0; i<string.size(); i++)
		{
			bufferContent[i] = (string.getElementDirect(i));
		}
		if(strlen(bufferContent))
		{
			printf("\n===debug==>:");
			printf("\n%s", bufferContent);
			printf("\n\n");
		}
		free(bufferContent);
	}

	static void print(const char* label, char* message)
	{
		printf("\n===debug===>:(%s)\n%s\n", label, message);
	}
}

/**********************************************************************************************************************/

oneLife::client::game::handler::Socket::Socket()
{
	this->lastSendSuccess = false;
}

oneLife::client::game::handler::Socket::~Socket() {}

void oneLife::client::game::handler::Socket::close()
{

}

bool oneLife::client::game::handler::Socket::isLastSendSucceed()
{
	return this->lastSendSuccess;
}

oneLife::client::game::handler::Socket* oneLife::client::game::handler::Socket::sendMessage(char *message)
{
	this->lastSendSuccess = false;
	timeLastMessageSent = game_getCurrentTime();
	oneLife::debug::print("sendMessage", message);
	replaceLastMessageSent( stringDuplicate( message ) );
	int len = strlen( message );
	int numSent = sendToSocket( *(this->mServerSocket), (unsigned char*)message, len );

	if( numSent == len )
	{
		numServerBytesSent += len;
		overheadServerBytesSent += 52;
		messagesOutCount++;
		bytesOutCount += len;
		this->lastSendSuccess = true;
	}
	else
	{
		printf( "Failed to send message to server socket "
				"at time %f "
				"(tried to send %d, but numSent=%d)\n",
				game_getCurrentTime(), len, numSent );
	}
}

/**
 *
 *
 * @note from gameSource/LivingPage.cpp
 * // reads all waiting data from socket and stores it in buffer
// returns false on socket error
 */
char readServerSocketFull( int inServerSocket )
{

	if( forceDisconnect )
	{
		forceDisconnect = false;
		return false;
	}

	unsigned char buffer[512];
	int numRead = readFromSocket( inServerSocket, buffer, 512 );
	while( numRead > 0 )
	{
		if( ! serverSocketConnected ) {
			serverSocketConnected = true;
			connectedTime = game_getCurrentTime();
		}

		serverSocketBuffer.appendArray( buffer, numRead );
		numServerBytesRead += numRead;
		bytesInCount += numRead;

		numRead = readFromSocket( inServerSocket, buffer, 512 );
	}

	if( numRead == -1 )
	{
		printf( "Failed to read from server socket at time %f\n", game_getCurrentTime() );
		return false;
	}

	oneLife::debug::print(serverSocketBuffer);

	return true;
}

/**
 *
 * @param inNewMessage
 * @note from gameSource/LivingLifePage
 * // destroyed internally if not NULL
 */
void replaceLastMessageSent( char *inNewMessage )
{
	if( lastMessageSentToServer != NULL ) {
		delete [] lastMessageSentToServer;
	}
	lastMessageSentToServer = inNewMessage;
}