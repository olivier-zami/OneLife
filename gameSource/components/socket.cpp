//
// Created by olivier on 23/10/2021.
//

#include "socket.h"
#include <cstdio>
#include "minorGems/game/game.h"
#include "OneLife/gameSource/dataTypes/web.h"
#include "minorGems/util/SimpleVector.h"
#include "minorGems/network/HostAddress.h"
#include "minorGems/util/stringUtils.h"
#include "minorGems/network/SocketClient.h"
#include "OneLife/gameSource/application.h"
#include "minorGems/util/log/AppLog.h"

SimpleVector<SocketConnectionRecord> socketConnectionRecords;

extern OneLife::game::Application *screen;
static int nextSocketConnectionHandle = 0;

OneLife::game::component::Socket::Socket(
	SimpleVector<unsigned char>* serverSocketBuffer,//serverSocketBuffer
	int* bytesInCount,//bytesInCount
	int* idServerSocket)//mServerSocket
{
	this->serverSocketBuffer = serverSocketBuffer;
	this->bytesInCount = bytesInCount;
	this->idServerSocket = idServerSocket;
	*(this->idServerSocket) = -1;
	this->serverSocketConnected = false;
	this->numServerBytesRead = 0;
	this->numServerBytesSent = 0;
	this->overheadServerBytesRead = 0;
	this->overheadServerBytesSent = 0;
	this->timeLastMessageSent = 0;
	this->numServerMessageSent = 0;
}

OneLife::game::component::Socket::~Socket() {}

void OneLife::game::component::Socket::connect(OneLife::game::dataType::ServerSocket socket)
{
	*(this->idServerSocket) = openSocketConnection( socket.ip, socket.port );
	this->timeLastMessageSent = game_getCurrentTime();
}

bool OneLife::game::component::Socket::isConnected()
{
	if(*(this->idServerSocket) == -1) this->serverSocketConnected = false;
	return (bool) (*(this->idServerSocket) != -1);
}

/**
 *
 * @param message
 */
void OneLife::game::component::Socket::sendMessage(OneLife::game::dataType::Message message)
{
	this->timeLastMessageSent = game_getCurrentTime();
	printf( "Sending message to server: %s\n", message.body );
	//replaceLastMessageSent( stringDuplicate( inMessage ) );TODO: if lastMessageSentToServer used create method getlastMessageSent()
	int len = strlen( message.body );
	//TODO: test idServerSocket
	int numSent = sendToSocket( *(this->idServerSocket), (unsigned char*)(message.body), len );

	if( numSent == len )
	{
		this->numServerBytesSent += len;
		this->overheadServerBytesSent += 52;
		this->numServerMessageSent++;//TODO: messagesOutCount++;
		this->bytesOutCount += len;
	}
	else
	{
		printf( "Failed to send message to server socket at time %f (tried to send %d, but numSent=%d)\n", game_getCurrentTime(), len, numSent );
		closeSocket( *(this->idServerSocket) ); *(this->idServerSocket) = -1;//TODO: set idServerSocket to -1 in closeSocket();
	}
}

/**
 *
 * @param inServerSocket
 * @return
 * @note reads all waiting data from socket and stores it in buffer returns false on socket error
 */
char OneLife::game::component::Socket::readMessage()
{
	if(this->forceDisconnect)
	{
		this->forceDisconnect = false;
		return false;
	}


	unsigned char buffer[512];

	int numRead = readFromSocket( *(this->idServerSocket), buffer, 512 );


	while( numRead > 0 ) {
		if( ! this->serverSocketConnected ) {
			this->serverSocketConnected = true;
			this->connectedTime = game_getCurrentTime();
		}

		(*(this->serverSocketBuffer)).appendArray( buffer, numRead );
		this->numServerBytesRead += numRead;
		*(this->bytesInCount) += numRead;

		numRead = readFromSocket( *(this->idServerSocket), buffer, 512 );
	}

	if( numRead == -1 ) {
		printf( "Failed to read from server socket at time %f\n",
				game_getCurrentTime() );
		return false;
	}

	return true;
}

/**
 *
 * @return
 */
double OneLife::game::component::Socket::getLastQueryLifeTime()
{
	return  game_getCurrentTime() - this->connectedTime;
}

double OneLife::game::component::Socket::getTimeLastMessageSent()
{
	return game_getCurrentTime() - this->timeLastMessageSent;
}

void OneLife::game::component::Socket::disconnect()
{
	this->connectedTime = 0;
	this->serverSocketConnected = false;
}

void OneLife::game::component::Socket::close()
{
	//*(this->idServerSocket) = -1;//TODO: close socket before
	this->forceDisconnect = true;
}

void OneLife::game::component::Socket::resetStats()
{

}

int OneLife::game::component::Socket::getTotalServerBytesRead()
{
	return this->numServerBytesRead;
}

int OneLife::game::component::Socket::getTotalServerBytesSent()
{
	return this->numServerBytesSent;
}

int OneLife::game::component::Socket::getTotalServerOverheadBytesRead()
{
	return this->overheadServerBytesRead;
}

int OneLife::game::component::Socket::getTotalServerOverheadBytesSent()
{
	return this->overheadServerBytesSent;
}

int OneLife::game::component::Socket::getTotalServerMessageSent()
{
	return this->numServerMessageSent;
}

/**********************************************************************************************************************/

// returns unique int handle for socket connection, -1 on error
int openSocketConnection( const char *inNumericalAddress, int inPort )
{
	SocketConnectionRecord r;

	r.handle = nextSocketConnectionHandle;
	nextSocketConnectionHandle++;

	HostAddress address( stringDuplicate( inNumericalAddress ), inPort );

	char timedOut;

	// non-blocking connet
	r.sock = SocketClient::connectToServer( &address, 0, &timedOut );

	if( r.sock != NULL ) {
		socketConnectionRecords.push_back( r );

		return r.handle;
	}
	else {
		return -1;
	}
}

// non-blocking send
// returns number sent (maybe 0) on success, -1 on error
int sendToSocket( int inHandle, unsigned char *inData, int inDataLength )
{
	if( screen->isPlayingBack() ) {
		// play back result of this send

		int nextType, nextNumBodyBytes;
		screen->getSocketEventTypeAndSize( inHandle,
				&nextType, &nextNumBodyBytes );

		while( nextType == 2 && nextNumBodyBytes == 0 ) {
			// skip over any lingering waiting-for-read events
			// sometimes there are extra in recording that aren't needed
			// on playback for some reason
			screen->getSocketEventTypeAndSize( inHandle,
					&nextType, &nextNumBodyBytes );
		}


		if( nextType == 0 ) {
			return nextNumBodyBytes;
		}
		else {
			return -1;
		}
	}


	Socket *sock = getSocketByHandle( inHandle );

	if( sock != NULL ) {

		int numSent = 0;


		if( sock->isConnected() ) {

			numSent = sock->send( inData, inDataLength, false, false );

			if( numSent == -2 ) {
				// would block
				numSent = 0;
			}
		}

		int type = 0;

		if( numSent == -1 ) {
			type = 1;
			numSent = 0;
		}

		screen->registerSocketEvent( inHandle, type, numSent, NULL );

		return numSent;
	}

	return -1;
}

void closeSocket( int inHandle ) {

	if( screen->isPlayingBack() ) {
		// not a real socket, do nothing
		return;
	}

	for( int i=0; i<socketConnectionRecords.size(); i++ ) {
		SocketConnectionRecord *r = socketConnectionRecords.getElement( i );

		if( r->handle == inHandle ) {
			delete r->sock;

			socketConnectionRecords.deleteElement( i );

			// found, done
			return;
		}
	}

	// else not found?
	AppLog::error( "gameSDL - closeSocket:  "
				   "Requested Socket handle not found\n" );
}

Socket *getSocketByHandle( int inHandle ) {
	for( int i=0; i<socketConnectionRecords.size(); i++ ) {
		SocketConnectionRecord *r = socketConnectionRecords.getElement( i );

		if( r->handle == inHandle ) {
			return r->sock;
		}
	}

	// else not found?
	AppLog::error( "gameSDL - getSocketByHandle:  "
				   "Requested Socket handle not found\n" );
	return NULL;
}

// non-blocking read
// returns number of bytes read (maybe 0), -1 on error
int readFromSocket( int inHandle,
					unsigned char *inDataBuffer, int inBytesToRead ) {

	if( screen->isPlayingBack() ) {
		// play back result of this read

		int nextType, nextNumBodyBytes;
		screen->getSocketEventTypeAndSize( inHandle,
				&nextType, &nextNumBodyBytes );

		if( nextType == 2 ) {

			if( nextNumBodyBytes == 0 ) {
				return 0;
			}
			// else there are body bytes waiting

			if( nextNumBodyBytes > inBytesToRead ) {
				AppLog::errorF( "gameSDL - readFromSocket:  "
								"Expecting to read at most %d bytes, but "
								"recording has %d bytes waiting\n",
						inBytesToRead, nextNumBodyBytes );
				return -1;
			}

			unsigned char *bodyBytes =
					screen->getSocketEventBodyBytes( inHandle );

			memcpy( inDataBuffer, bodyBytes, nextNumBodyBytes );
			delete [] bodyBytes;


			return nextNumBodyBytes;
		}
		else {
			return -1;
		}
	}


	Socket *sock = getSocketByHandle( inHandle );

	if( sock != NULL ) {

		int numRead = 0;

		if( sock->isConnected() ) {

			numRead = sock->receive( inDataBuffer, inBytesToRead, 0 );

			if( numRead == -2 ) {
				// would block
				numRead = 0;
			}
		}

		int type = 2;
		if( numRead == -1 ) {
			type = 3;
		}

		unsigned char *bodyBytes = NULL;
		if( numRead > 0 ) {
			bodyBytes = inDataBuffer;
		}

		screen->registerSocketEvent( inHandle, type, numRead, bodyBytes );

		return numRead;
	}

	return -1;
}