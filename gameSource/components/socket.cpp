//
// Created by olivier on 23/10/2021.
//

#include "socket.h"

#include <cstdio>
#include <cstring>
#include "minorGems/util/log/AppLog.h"
#include "minorGems/util/SimpleVector.h"
#include "minorGems/network/HostAddress.h"
#include "minorGems/util/stringUtils.h"
#include "minorGems/formats/encodingUtils.h"
#include "minorGems/network/SocketClient.h"
#include "OneLife/gameSource/procedures/conversions/message.h"
#include "OneLife/gameSource/game.h"
#include "OneLife/gameSource/dataTypes/socket.h"
#include "OneLife/gameSource/application.h"
#include "OneLife/gameSource/debug/console.h"

using MESSAGE_TYPE = OneLife::data::value::message::Type;


extern OneLife::game::Application *screen;

SimpleVector<SocketConnectionRecord> socketConnectionRecords;
SimpleVector<unsigned char> serverSocketBuffer;
char *pendingMapChunkMessage = NULL;
int pendingCompressedChunkSize;
int pendingCMDecompressedSize = 0;
int pendingCMCompressedSize = 0;
int messagesInCount = 0;
char pendingCMData = false;
double lastServerMessageReceiveTime = 0;
double largestPendingMessageTimeGap = 0;// while player action pending, measure largest gap between sequential // server messages// This is an approximation of our outtage time.
SimpleVector<char*> readyPendingReceivedMessages;
char waitForFrameMessages = false;
char serverFrameReady;
SimpleVector<char*> serverFrameMessages;
char *lastMessageSentToServer = NULL;

static int nextSocketConnectionHandle = 0;

OneLife::game::component::Socket::Socket()
{
	this->status.isConnected = false;
	this->currentMessage.type = 0;
	this->currentMessage.size = 0;
	this->currentMessage.content = malloc(1024*sizeof(char));
	memset(this->currentMessage.content, 0, 1024*sizeof(char));
	this->numServerBytesRead = 0;
	this->numServerBytesSent = 0;
	this->overheadServerBytesRead = 0;
	this->overheadServerBytesSent = 0;
	this->timeLastMessageSent = 0;
	this->numServerMessageSent = 0;
}

OneLife::game::component::Socket::~Socket() {}

/**********************************************************************************************************************/

void OneLife::game::component::Socket::handle(
		SimpleVector<unsigned char>* serverSocketBuffer, //serverSocketBuffer
		int* bytesInCount)//bytesInCount
{
	this->serverSocketBuffer = serverSocketBuffer;
	this->bytesInCount = bytesInCount;
	this->id = -1;
}

/**********************************************************************************************************************/

void OneLife::game::component::Socket::setAddress(OneLife::game::dataType::socket::Address address)
{
	printf("\n===>OneLife::game::component::Socket::setAddress({%s:%i})", address.ip, address.port);
	strcpy(this->address.ip, address.ip);
	this->address.port = address.port;
}

OneLife::game::dataType::socket::Address OneLife::game::component::Socket::getAddress()
{
	return this->address;
}

void OneLife::game::component::Socket::connect()
{
	OneLife::debug::Console::showFunction("OneLife::game::component::Socket::connect(%s : %i)", this->address.ip, this->address.port);
	this->id = openSocketConnection( this->address.ip, this->address.port );
	this->timeLastMessageSent = game_getCurrentTime();
}

bool OneLife::game::component::Socket::isConnected()
{
	if(this->id == -1) this->status.isConnected = false;
	return (bool) (this->id != -1);
}

/**
 *
 * @param inServerSocket
 * @return
 * @note reads all waiting data from socket and stores it in buffer returns false on socket error
 */
char OneLife::game::component::Socket::readMessage()
{
	if(this->id==-1) return false;

	unsigned char buffer[512];
	int numRead = readFromSocket( this->id, buffer, 512 );
	while( numRead > 0 )
	{
		if(!this->status.isConnected )
		{
			this->status.isConnected = true;
			this->connectedTime = game_getCurrentTime();
		}

		(*(this->serverSocketBuffer)).appendArray( buffer, numRead );
		this->numServerBytesRead += numRead;
		*(this->bytesInCount) += numRead;

		numRead = readFromSocket( this->id, buffer, 512 );
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
 * @param message
 */
void OneLife::game::component::Socket::sendMessage(OneLife::game::dataType::socket::Message message)
{
	OneLife::debug::Console::showFunction("OneLife::game::component::Socket::sendMessage(%s)", message.body);
	this->timeLastMessageSent = game_getCurrentTime();
	replaceLastMessageSent(stringDuplicate( message.body ) );//TODO: if lastMessageSentToServer used create method getlastMessageSent()
	int len = strlen( message.body );
	//TODO: test id
	int numSent = sendToSocket( this->id, (unsigned char*)(message.body), len );

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
		this->close();//TODO: set id to -1 in closeSocket();
	}
}

/**
 *
 * @param message
 * @return
 */
OneLife::game::dataType::Message OneLife::game::component::Socket::getMessage(const char* message)
{
	int sizeX = 0;
	int sizeY = 0;
	int x = 0;
	int y = 0;

	int binarySize = 0;
	int compressedSize = 0;

	sscanf(message, "MC\n%d %d %d %d\n%d %d\n", &sizeX, &sizeY, &x, &y, &binarySize, &compressedSize );
	printf("Got map chunk with bin size %d, compressed size %d\n", binarySize, compressedSize );

	unsigned char *compressedChunk = new unsigned char[ compressedSize ];
	for( int i=0; i<compressedSize; i++ )
	{
		compressedChunk[i] = (*(this->serverSocketBuffer)).getElementDirect( i );
	}
	(*(this->serverSocketBuffer)).deleteStartElements( compressedSize );
	//unsigned char *decompressedChunk = zipDecompress(compressedChunk, compressedSize, binarySize);
	this->currentMessage.size = binarySize;
	this->currentMessage.content = zipDecompress(compressedChunk, compressedSize, binarySize);
	delete [] compressedChunk;

	return this->currentMessage;
}

void OneLife::game::component::Socket::deleteAllMessages()
{
	(*(this->serverSocketBuffer)).deleteAll();
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
	this->status.isConnected = false;
}

void OneLife::game::component::Socket::close()
{
	if(this->id != -1)
	{
		closeSocket(this->id);
		this->id = -1;//TODO: close socket before
	}
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
		screen->getSocketEventTypeAndSize( inHandle, &nextType, &nextNumBodyBytes );

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

// NULL if there's no full message available
char *getNextServerMessageRaw() {

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
		else {
			// wait for more data to arrive before saying this MC message
			// is ready
			return NULL;
		}
	}

	if( pendingCMData )
	{
		if( serverSocketBuffer.size() >= pendingCMCompressedSize ) {
			pendingCMData = false;

			unsigned char *compressedData =
					new unsigned char[ pendingCMCompressedSize ];

			for( int i=0; i<pendingCMCompressedSize; i++ ) {
				compressedData[i] = serverSocketBuffer.getElementDirect( i );
			}
			serverSocketBuffer.deleteStartElements( pendingCMCompressedSize );

			unsigned char *decompressedMessage =
					zipDecompress( compressedData,
							pendingCMCompressedSize,
							pendingCMDecompressedSize );

			delete [] compressedData;

			if( decompressedMessage == NULL ) {
				printf( "Decompressing CM message failed\n" );
				return NULL;
			}
			else {
				char *textMessage = new char[ pendingCMDecompressedSize + 1 ];
				memcpy( textMessage, decompressedMessage,
						pendingCMDecompressedSize );
				textMessage[ pendingCMDecompressedSize ] = '\0';

				delete [] decompressedMessage;

				messagesInCount++;
				return textMessage;
			}
		}
		else {
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

	if( gap > largestPendingMessageTimeGap ) {
		largestPendingMessageTimeGap = gap;
	}

	lastServerMessageReceiveTime = curTime;



	char *message = new char[ index + 1 ];

	for( int i=0; i<index; i++ ) {
		message[i] = (char)( serverSocketBuffer.getElementDirect( i ) );
	}
	// delete message and terminal character
	serverSocketBuffer.deleteStartElements( index + 1 );

	message[ index ] = '\0';

	if( OneLife::procedure::conversion::getMessageType( message ) == MESSAGE_TYPE::MAP_CHUNK ) {
		pendingMapChunkMessage = message;

		int sizeX, sizeY, x, y, binarySize;
		sscanf( message, "MC\n%d %d %d %d\n%d %d\n",
				&sizeX, &sizeY,
				&x, &y, &binarySize, &pendingCompressedChunkSize );


		return getNextServerMessageRaw();
	}
	else if( OneLife::procedure::conversion::getMessageType( message ) == MESSAGE_TYPE::COMPRESSED_MESSAGE ) {
		pendingCMData = true;

		printf( "Got compressed message header:\n%s\n\n", message );

		sscanf( message, "CM\n%d %d\n",
				&pendingCMDecompressedSize, &pendingCMCompressedSize );

		delete [] message;
		return NULL;
	}
	else {
		messagesInCount++;
		return message;
	}
}

// either returns a pending recieved message (one that was received earlier
// or held back
//
// or receives the next message from the server socket (if we are not waiting
// for full frames of messages)
//
// or returns NULL until a full frame of messages is available, and
// then returns the first message from the frame
char *getNextServerMessage() {

	if( readyPendingReceivedMessages.size() > 0 ) {
		char *message = readyPendingReceivedMessages.getElementDirect( 0 );
		readyPendingReceivedMessages.deleteElement( 0 );
		printf( "Playing a held pending message\n" );
		return message;
	}

	if( !waitForFrameMessages ) {
		return getNextServerMessageRaw();
	}
	else {
		if( !serverFrameReady ) {
			// read more and look for end of frame

			char *message = getNextServerMessageRaw();

			while( message != NULL ) {
				OneLife::data::value::message::Type t = OneLife::procedure::conversion::getMessageType( message );

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
				else if( t == MESSAGE_TYPE::MAP_CHUNK ||
						 t == MESSAGE_TYPE::PONG ||
						 t == MESSAGE_TYPE::FLIGHT_DEST ||
						 t == MESSAGE_TYPE::PHOTO_SIGNATURE ) {
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


// destroyed internally if not NULL
void replaceLastMessageSent( char *inNewMessage ) {
	if( lastMessageSentToServer != NULL ) {
		delete [] lastMessageSentToServer;
	}
	lastMessageSentToServer = inNewMessage;
}

/**********************************************************************************************************************/
#include "OneLife/gameSource/controller.h"
#include "minorGems/util/SettingsManager.h"
extern char userReconnect;
char usingCustomServer = false;
char *serverIP = NULL;
int serverPort = 0;
extern Controller *currentGamePage;
#include "OneLife/gameSource/components/pages/LivingLifePage.h"
extern LivingLifePage *livingLifePage;
#include "OneLife/gameSource/components/pages/ServerActionPage.h"
extern ServerActionPage *getServerAddressPage;
extern char *reflectorURL;
extern char *userEmail;
extern char *userTwinCode;
#include "minorGems/crypto/hashes/sha1.h"

void startConnecting()
{
	userReconnect = false;

	if( SettingsManager::getIntSetting( "useCustomServer", 0 ) ) {
		usingCustomServer = true;

		if( serverIP != NULL ) {
			delete [] serverIP;
			serverIP = NULL;
		}

		serverIP = SettingsManager::getStringSetting("customServerAddress" );
		if( serverIP == NULL ) { serverIP = stringDuplicate( "127.0.0.1" ); }
		serverPort = SettingsManager::getIntSetting("customServerPort", 8005 );
		printf( "Using custom server address: %s:%d\n", serverIP, serverPort );

		currentGamePage = livingLifePage;
		currentGamePage->base_makeActive( true );
	}
	else {
		usingCustomServer = false;

		printf( "Starting fetching server URL from reflector %s\n",
				reflectorURL );

		getServerAddressPage->clearActionParameters();


		getServerAddressPage->setActionParameter( "email",
				userEmail );

		if( userTwinCode != NULL ) {
			char *codeHash = computeSHA1Digest( userTwinCode );
			getServerAddressPage->setActionParameter( "twin_code",
					codeHash );
			delete [] codeHash;
		}


		currentGamePage = getServerAddressPage;
		currentGamePage->base_makeActive( true );
	}
}

void playPendingReceivedMessages( LiveObject *inPlayer )
{
	printf( "Playing %d pending received messages for %d\n",
			inPlayer->pendingReceivedMessages.size(), inPlayer->id );

	for( int i=0; i<inPlayer->pendingReceivedMessages.size(); i++ ) {
		readyPendingReceivedMessages.push_back(
				inPlayer->pendingReceivedMessages.getElementDirect( i ) );
	}
	inPlayer->pendingReceivedMessages.deleteAll();
}

void playPendingReceivedMessagesRegardingOthers( LiveObject *inPlayer ) {
	printf( "Playing %d pending received messages for %d "
			"    (skipping those that don't affect other players or map)\n",
			inPlayer->pendingReceivedMessages.size(), inPlayer->id );

	for( int i=0; i<inPlayer->pendingReceivedMessages.size(); i++ ) {
		char *message = inPlayer->pendingReceivedMessages.getElementDirect( i );

		if( strstr( message, "PU" ) == message ) {
			// only keep PU's not about this player

			int messageID = -1;

			sscanf( message, "PU\n%d", &messageID );

			if( messageID != inPlayer->id ) {
				readyPendingReceivedMessages.push_back( message );
			}
			else {
				delete [] message;
			}
		}
		else if( strstr( message, "PM" ) == message ) {
			// only keep PM's not about this player

			int messageID = -1;

			sscanf( message, "PM\n%d", &messageID );

			if( messageID != inPlayer->id ) {
				readyPendingReceivedMessages.push_back( message );
			}
			else {
				delete [] message;
			}
		}
		else {
			// not a PU, keep it no matter what (map change, etc.
			readyPendingReceivedMessages.push_back( message );
		}
	}
	inPlayer->pendingReceivedMessages.deleteAll();
}

void dropPendingReceivedMessagesRegardingID( LiveObject *inPlayer, int inIDToDrop )
{
	for( int i=0; i<inPlayer->pendingReceivedMessages.size(); i++ ) {
		char *message = inPlayer->pendingReceivedMessages.getElementDirect( i );
		char match = false;

		if( strstr( message, "PU" ) == message ) {
			// only keep PU's not about this player

			int messageID = -1;

			sscanf( message, "PU\n%d", &messageID );

			if( messageID == inIDToDrop ) {
				match = true;
			}
		}
		else if( strstr( message, "PM" ) == message ) {
			// only keep PM's not about this player

			int messageID = -1;

			sscanf( message, "PM\n%d", &messageID );

			if( messageID == inIDToDrop ) {
				match = true;
			}
		}

		if( match ) {
			delete [] message;
			inPlayer->pendingReceivedMessages.deleteElement( i );
			i--;
		}
	}
}
