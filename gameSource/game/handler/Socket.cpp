//
// Created by olivier on 23/08/2022.
//

#include "Socket.h"

#include "../../../third_party/minorGems/formats/encodingUtils.h"
#include "../../../third_party/minorGems/game/game.h"
#include "../../../third_party/minorGems/util/SimpleVector.h"
#include "../../../third_party/minorGems/util/stringUtils.h"

#include "../../../commonSource/dataType/message_client.h"//TODO merge message_client.h with message.h

#include <cstdio>
#include <cstdlib>
#include <cstring>

int bytesInCount = 0;
int bytesOutCount = 0;
double connectedTime = 0;
char forceDisconnect = false;
char *lastMessageSentToServer = nullptr;
double lastServerMessageReceiveTime = 0;
double largestPendingMessageTimeGap = 0;// while player action pending, measure largest gap between sequential // server messages// This is an approximation of our outtage time.
int messagesInCount = 0;
int messagesOutCount = 0;
int numServerBytesRead = 0;
int numServerBytesSent = 0;
int overheadServerBytesSent = 0;
int pendingCMCompressedSize = 0;
char pendingCMData = false;
int pendingCMDecompressedSize = 0;
int pendingCompressedChunkSize;
char *pendingMapChunkMessage = NULL;
SimpleVector<char*> readyPendingReceivedMessages;
SimpleVector<char*> serverFrameMessages;
char serverFrameReady;
char serverSocketConnected = false;
SimpleVector<unsigned char> serverSocketBuffer;
double timeLastMessageSent = 0;
char waitForFrameMessages = false;

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


/**
 *
 * @return
 * @note from gameSource/LivingLifePage.cpp
 * // either returns a pending recieved message (one that was received earlier
// or held back
//
// or receives the next message from the server socket (if we are not waiting
// for full frames of messages)
//
// or returns NULL until a full frame of messages is available, and
// then returns the first message from the frame
 */
char *getNextServerMessage()
{
	if( readyPendingReceivedMessages.size() > 0 )
	{
		char *message = readyPendingReceivedMessages.getElementDirect( 0 );
		readyPendingReceivedMessages.deleteElement( 0 );
		printf( "Playing a held pending message\n" );
		return message;
	}

	if( !waitForFrameMessages )
	{
		return getNextServerMessageRaw();
	}
	else
	{
		if( !serverFrameReady ) {
			// read more and look for end of frame

			char *message = getNextServerMessageRaw();

			while( message != NULL )
			{
				messageType t = getMessageType( message );

				if( strstr( message, "FM" ) == message ) {
					// end of frame, discard the marker message
					delete [] message;

					if( serverFrameMessages.size() > 0 ) {
						serverFrameReady = true;
						// see end of frame, stop reading more messages
						// for now (they are part of next frame)
						// and start returning message to caller from
						// this frame
						break;
					}
				}
				else if( t == MAP_CHUNK ||
						 t == PONG ||
						 t == FLIGHT_DEST ||
						 t == PHOTO_SIGNATURE ) {
					// map chunks are followed by compressed data
					// they cannot be queued

					// PONG messages should be returned instantly

					// FLIGHT_DEST messages also should be returned instantly
					// otherwise, they will be queued and seen by
					// the client after the corresponding MC message
					// for the new location.
					// which will invalidate the map around player's old
					// location
					return message;
				}
				else {
					// some other message in the middle of the frame
					// keep it
					serverFrameMessages.push_back( message );
				}

				// keep reading messages, until we either see the
				// end of the frame or read all available messages
				message = getNextServerMessageRaw();
			}
		}

		if( serverFrameReady ) {
			char *message = serverFrameMessages.getElementDirect( 0 );

			serverFrameMessages.deleteElement( 0 );

			if( serverFrameMessages.size() == 0 ) {
				serverFrameReady = false;
			}
			return message;
		}
		else {
			return NULL;
		}
	}
}

/**
 *
 * @return
 * @note from gameSource/LivingLifePage.cpp
 * // NULL if there's no full message available
 */
char *getNextServerMessageRaw()
{
	if( pendingMapChunkMessage != NULL )
	{
		// wait for full binary data chunk to arrive completely
		// after message before we report that the message is ready

		if( serverSocketBuffer.size() >= pendingCompressedChunkSize ) {
			char *returnMessage = pendingMapChunkMessage;
			pendingMapChunkMessage = NULL;

			messagesInCount++;

			return returnMessage;
		}
		else
		{
			// wait for more data to arrive before saying this MC message
			// is ready
			return NULL;
		}
	}

	if( pendingCMData )
	{
		if( serverSocketBuffer.size() >= pendingCMCompressedSize ) {
			pendingCMData = false;
			unsigned char *compressedData = new unsigned char[ pendingCMCompressedSize ];
			for( int i=0; i<pendingCMCompressedSize; i++ )
			{
				compressedData[i] = serverSocketBuffer.getElementDirect( i );
			}
			serverSocketBuffer.deleteStartElements( pendingCMCompressedSize );
			unsigned char *decompressedMessage = zipDecompress( compressedData,
								   pendingCMCompressedSize,
								   pendingCMDecompressedSize );
			delete [] compressedData;

			if( decompressedMessage == NULL )
			{
				printf( "Decompressing CM message failed\n" );
				return NULL;
			}
			else
			{
				char *textMessage = new char[ pendingCMDecompressedSize + 1 ];
				memcpy( textMessage, decompressedMessage, pendingCMDecompressedSize );
				textMessage[ pendingCMDecompressedSize ] = '\0';
				delete [] decompressedMessage;
				messagesInCount++;
				return textMessage;
			}
		}
		else
		{
			// wait for more data to arrive
			return NULL;
		}
	}
	// find first terminal character #
	int index = serverSocketBuffer.getElementIndex( '#' );
	if( index == -1 )
	{
		return NULL;
	}
	// terminal character means message arrived
	double curTime = game_getCurrentTime();
	double gap = curTime - lastServerMessageReceiveTime;
	if( gap > largestPendingMessageTimeGap )
	{
		largestPendingMessageTimeGap = gap;
	}
	lastServerMessageReceiveTime = curTime;
	char *message = new char[ index + 1 ];
	for( int i=0; i<index; i++ )
	{
		message[i] = (char)( serverSocketBuffer.getElementDirect( i ) );
	}
	// delete message and terminal character
	serverSocketBuffer.deleteStartElements( index + 1 );
	message[ index ] = '\0';
	if( getMessageType( message ) == MAP_CHUNK )
	{
		pendingMapChunkMessage = message;
		int sizeX, sizeY, x, y, binarySize;
		sscanf( message, "MC\n%d %d %d %d\n%d %d\n",
				&sizeX, &sizeY,
				&x, &y, &binarySize, &pendingCompressedChunkSize );
		return getNextServerMessageRaw();
	}
	else if( getMessageType( message ) == COMPRESSED_MESSAGE )
	{
		pendingCMData = true;
		printf( "Got compressed message header:\n%s\n\n", message );
		sscanf( message, "CM\n%d %d\n", &pendingCMDecompressedSize, &pendingCMCompressedSize );
		delete [] message;
		return NULL;
	}
	else
	{
		messagesInCount++;
		return message;
	}
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