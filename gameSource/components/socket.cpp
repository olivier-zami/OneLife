//
// Created by olivier on 23/10/2021.
//

#include "socket.h"

#include <cstdio>
#include "minorGems/util/log/AppLog.h"
#include "minorGems/util/SimpleVector.h"
#include "minorGems/network/HostAddress.h"
#include "minorGems/util/stringUtils.h"
#include "minorGems/formats/encodingUtils.h"
#include "minorGems/network/SocketClient.h"
#include "OneLife/gameSource/misc.h"
#include "OneLife/gameSource/dataTypes/socket.h"
#include "OneLife/gameSource/application.h"
#include "OneLife/gameSource/debug.h"


extern OneLife::game::Application *gameApplication;

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
int overheadServerBytesRead = 0;

static int nextSocketConnectionHandle = 0;

OneLife::game::component::Socket::Socket()
{
	this->serverSocketConnected = false;
	this->numServerBytesRead = 0;
	this->numServerBytesSent = 0;
	this->overheadServerBytesRead = 0;
	this->overheadServerBytesSent = 0;
	this->timeLastMessageSent = 0;
	this->numServerMessageSent = 0;
	this->forceDisconnect = false;
}

OneLife::game::component::Socket::~Socket() {}

void OneLife::game::component::Socket::handle(
		SimpleVector<unsigned char>* serverSocketBuffer, //serverSocketBuffer
		int* bytesInCount)//bytesInCount
{
	this->serverSocketBuffer = serverSocketBuffer;
	this->bytesInCount = bytesInCount;
	this->idSocket = -1;
}

void OneLife::game::component::Socket::setAddress(OneLife::dataType::socket::Address address)
{
	strcpy(this->address.ip, address.ip);
	this->address.port = address.port;
}

OneLife::dataType::socket::Address OneLife::game::component::Socket::getAddress()
{
	return this->address;
}

void OneLife::game::component::Socket::connect()
{
	OneLife::game::Debug::writeControllerStepInfo("OneLife::game::component::Socket::connect() : %s:%i", this->address.ip, this->address.port);
	SocketConnectionRecord r;
	r.handle = nextSocketConnectionHandle;
	nextSocketConnectionHandle++;
	HostAddress address( stringDuplicate( this->address.ip ), this->address.port );
	char timedOut;
	r.sock = SocketClient::connectToServer( &address, 0, &timedOut );// non-blocking connet
	if( r.sock != NULL )
	{
		socketConnectionRecords.push_back( r );
		this->idSocket = r.handle;
	}
	else
	{
		return;// -1; TODO: exception ...
	}
	this->timeLastMessageSent = game_getCurrentTime();
}

bool OneLife::game::component::Socket::isConnected()
{
	if(this->idSocket == -1) this->serverSocketConnected = false;
	return (bool) (this->idSocket != -1);
}

bool OneLife::game::component::Socket::isClosed()
{
	return (this->idSocket == -1);
}

void OneLife::game::component::Socket::sendMessage(const char* message)
{
	this->timeLastMessageSent = game_getCurrentTime();
	printf( "Sending message to server: %s\n", message );
	replaceLastMessageSent(stringDuplicate( message));//TODO: if lastMessageSentToServer used create method getlastMessageSent()
	size_t len = strlen(message);
	int numSent = sendToSocket(this->idSocket, (unsigned char*)message, (int)len);
	if( numSent == len )
	{
		this->numServerBytesSent += (int)len;
		this->overheadServerBytesSent += 52;
		this->numServerMessageSent++;//TODO: messagesOutCount++;
		this->bytesOutCount += (int)len;
	}
	else
	{
		printf( "Failed to send message to server socket at time %f (tried to send %d, but numSent=%d)\n", game_getCurrentTime(), len, numSent );
		closeSocket(this->idSocket);
		this->idSocket = -1;//TODO: set idSocket to -1 in closeSocket();
	}
}

/**
 *
 * @param message
 */
void OneLife::game::component::Socket::sendMessage(OneLife::dataType::socket::Message message)
{
	this->timeLastMessageSent = game_getCurrentTime();
	printf( "Sending message to server: %s\n", message.body );
	replaceLastMessageSent( stringDuplicate( message.body ) );//TODO: if lastMessageSentToServer used create method getlastMessageSent()
	int len = strlen( message.body );
	//TODO: test idSocket
	int numSent = sendToSocket(this->idSocket, (unsigned char*)(message.body), len );

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
		closeSocket(this->idSocket);
		this->idSocket = -1;//TODO: set idSocket to -1 in closeSocket();
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

	int numRead = readFromSocket(this->idSocket, buffer, 512 );


	while( numRead > 0 ) {
		if( ! this->serverSocketConnected ) {
			this->serverSocketConnected = true;
			this->connectedTime = game_getCurrentTime();
		}

		(*(this->serverSocketBuffer)).appendArray( buffer, numRead );
		this->numServerBytesRead += numRead;
		*(this->bytesInCount) += numRead;

		numRead = readFromSocket(this->idSocket, buffer, 512 );
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
	this->idSocket = -1;//TODO: close socket before
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


// non-blocking send
// returns number sent (maybe 0) on success, -1 on error
int sendToSocket( int inHandle, unsigned char *inData, int inDataLength )
{
	if( gameApplication->isPlayingBack() ) {
		// play back result of this send

		int nextType, nextNumBodyBytes;
		gameApplication->getSocketEventTypeAndSize( inHandle,
				&nextType, &nextNumBodyBytes );

		while( nextType == 2 && nextNumBodyBytes == 0 ) {
			// skip over any lingering waiting-for-read events
			// sometimes there are extra in recording that aren't needed
			// on playback for some reason
			gameApplication->getSocketEventTypeAndSize( inHandle,
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

		gameApplication->registerSocketEvent( inHandle, type, numSent, NULL );

		return numSent;
	}

	return -1;
}

void closeSocket( int inHandle ) {

	if( gameApplication->isPlayingBack() ) {
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

	if( gameApplication->isPlayingBack() ) {
		// play back result of this read

		int nextType, nextNumBodyBytes;
		gameApplication->getSocketEventTypeAndSize( inHandle, &nextType, &nextNumBodyBytes );

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
					gameApplication->getSocketEventBodyBytes( inHandle );

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

		gameApplication->registerSocketEvent( inHandle, type, numRead, bodyBytes );

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

	if( getMessageType( message ) == MAP_CHUNK ) {
		pendingMapChunkMessage = message;

		int sizeX, sizeY, x, y, binarySize;
		sscanf( message, "MC\n%d %d %d %d\n%d %d\n",
				&sizeX, &sizeY,
				&x, &y, &binarySize, &pendingCompressedChunkSize );


		return getNextServerMessageRaw();
	}
	else if( getMessageType( message ) == COMPRESSED_MESSAGE ) {
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
char *getNextServerMessage()
{
	overheadServerBytesRead += 52;

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
		if( !serverFrameReady )
		{
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

		if( serverFrameReady )
		{
			char *message = serverFrameMessages.getElementDirect( 0 );

			serverFrameMessages.deleteElement( 0 );

			if( serverFrameMessages.size() == 0 ) {
				serverFrameReady = false;
			}
			return message;
		}
		else
		{
			overheadServerBytesRead -= 52;
			return NULL;
		}
	}
}

messageType getMessageType( char *inMessage ) {
	char *copy = stringDuplicate( inMessage );

	char *firstBreak = strstr( copy, "\n" );

	if( firstBreak == NULL ) {
		delete [] copy;
		return UNKNOWN;
	}

	firstBreak[0] = '\0';

	messageType returnValue = UNKNOWN;

	if( strcmp( copy, "CM" ) == 0 ) {
		returnValue = COMPRESSED_MESSAGE;
	}
	else if( strcmp( copy, "MC" ) == 0 ) {
		returnValue = MAP_CHUNK;
	}
	else if( strcmp( copy, "MX" ) == 0 ) {
		returnValue = MAP_CHANGE;
	}
	else if( strcmp( copy, "PU" ) == 0 ) {
		returnValue = PLAYER_UPDATE;
	}
	else if( strcmp( copy, "PM" ) == 0 ) {
		returnValue = PLAYER_MOVES_START;
	}
	else if( strcmp( copy, "PO" ) == 0 ) {
		returnValue = PLAYER_OUT_OF_RANGE;
	}
	else if( strcmp( copy, "PS" ) == 0 ) {
		returnValue = PLAYER_SAYS;
	}
	else if( strcmp( copy, "LS" ) == 0 ) {
		returnValue = LOCATION_SAYS;
	}
	else if( strcmp( copy, "PE" ) == 0 ) {
		returnValue = PLAYER_EMOT;
	}
	else if( strcmp( copy, "FX" ) == 0 ) {
		returnValue = FOOD_CHANGE;
	}
	else if( strcmp( copy, "HX" ) == 0 ) {
		returnValue = HEAT_CHANGE;
	}
	else if( strcmp( copy, "LN" ) == 0 ) {
		returnValue = LINEAGE;
	}
	else if( strcmp( copy, "CU" ) == 0 ) {
		returnValue = CURSED;
	}
	else if( strcmp( copy, "CX" ) == 0 ) {
		returnValue = CURSE_TOKEN_CHANGE;
	}
	else if( strcmp( copy, "CS" ) == 0 ) {
		returnValue = CURSE_SCORE;
	}
	else if( strcmp( copy, "NM" ) == 0 ) {
		returnValue = NAMES;
	}
	else if( strcmp( copy, "AP" ) == 0 ) {
		returnValue = APOCALYPSE;
	}
	else if( strcmp( copy, "AD" ) == 0 ) {
		returnValue = APOCALYPSE_DONE;
	}
	else if( strcmp( copy, "DY" ) == 0 ) {
		returnValue = DYING;
	}
	else if( strcmp( copy, "HE" ) == 0 ) {
		returnValue = HEALED;
	}
	else if( strcmp( copy, "MN" ) == 0 ) {
		returnValue = MONUMENT_CALL;
	}
	else if( strcmp( copy, "GV" ) == 0 ) {
		returnValue = GRAVE;
	}
	else if( strcmp( copy, "GM" ) == 0 ) {
		returnValue = GRAVE_MOVE;
	}
	else if( strcmp( copy, "GO" ) == 0 ) {
		returnValue = GRAVE_OLD;
	}
	else if( strcmp( copy, "OW" ) == 0 ) {
		returnValue = OWNER;
	}
	else if( strcmp( copy, "VS" ) == 0 ) {
		returnValue = VALLEY_SPACING;
	}
	else if( strcmp( copy, "FD" ) == 0 ) {
		returnValue = FLIGHT_DEST;
	}
	else if( strcmp( copy, "VU" ) == 0 ) {
		returnValue = VOG_UPDATE;
	}
	else if( strcmp( copy, "PH" ) == 0 ) {
		returnValue = PHOTO_SIGNATURE;
	}
	else if( strcmp( copy, "PONG" ) == 0 ) {
		returnValue = PONG;
	}
	else if( strcmp( copy, "SHUTDOWN" ) == 0 ) {
		returnValue = SHUTDOWN;
	}
	else if( strcmp( copy, "SERVER_FULL" ) == 0 ) {
		returnValue = SERVER_FULL;
	}
	else if( strcmp( copy, "SN" ) == 0 ) {
		returnValue = SEQUENCE_NUMBER;
	}
	else if( strcmp( copy, "ACCEPTED" ) == 0 ) {
		returnValue = ACCEPTED;
	}
	else if( strcmp( copy, "REJECTED" ) == 0 ) {
		returnValue = REJECTED;
	}
	else if( strcmp( copy, "NO_LIFE_TOKENS" ) == 0 ) {
		returnValue = NO_LIFE_TOKENS;
	}
	else if( strcmp( copy, "SD" ) == 0 ) {
		returnValue = FORCED_SHUTDOWN;
	}
	else if( strcmp( copy, "MS" ) == 0 ) {
		returnValue = GLOBAL_MESSAGE;
	}
	else if( strcmp( copy, "FL" ) == 0 ) {
		returnValue = FLIP;
	}
	else if( strcmp( copy, "CR" ) == 0 ) {
		returnValue = CRAVING;
	}

	delete [] copy;
	return returnValue;
}

// destroyed internally if not NULL
void replaceLastMessageSent( char *inNewMessage ) {
	if( lastMessageSentToServer != NULL ) {
		delete [] lastMessageSentToServer;
	}
	lastMessageSentToServer = inNewMessage;
}

/**********************************************************************************************************************/
#include "OneLife/gameSource/components/controller.h"
#include "minorGems/util/SettingsManager.h"
extern char userReconnect;
char usingCustomServer = false;
char *serverIP = nullptr;
int serverPort = 0;
extern OneLife::game::Controller* currentGamePage;
#include "OneLife/gameSource/controllers/LivingLifePage.h"
extern LivingLifePage *livingLifePage;
#include "OneLife/gameSource/controllers/ServerActionPage.h"
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
