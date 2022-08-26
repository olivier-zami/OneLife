//
// Created by olivier on 22/08/2022.
//

#include "Socket.h"

#include "../../../third_party/minorGems/io/file/File.h"
#include "../../../third_party/minorGems/util/SimpleVector.h"
#include "../../../third_party/minorGems/util/log/AppLog.h"
#include "../../../third_party/minorGems/util/SettingsManager.h"
#include "../../component/feature/Diet.h"
#include "../../component/Map.h"
#include "../Server.h"

SocketPoll sockPoll;


extern int chunkDimensionX;
extern int chunkDimensionY;
extern int maxUncompressedSize;
extern SimpleVector<FreshConnection> newConnections;
extern SimpleVector<LiveObject> players;
extern SocketPoll sockPoll;

oneLife::server::game::handler::Socket::Socket()
{
	//this->lastClientListened = {nullptr, 0};
	this->lastConnection = nullptr;
	this->maxConnection = 0;
	this->outputMessage = nullptr;
	this->port = 0;
	this->readySock = nullptr;
	this->remoteHost = nullptr;
	this->socketServer = nullptr;
}

/**
 *
 * @note from server.cpp
 * // stop listening on server socket immediately, before running
// cleanup steps.  Cleanup may take a while, and we don't want to leave
// server socket listening, because it will answer reflector and player
// connection requests but then just hang there.

// Closing the server socket makes these connection requests fail
// instantly (instead of relying on this->remoteHost timeouts).
 */
oneLife::server::game::handler::Socket::~Socket()
{
	/*
	if(this->lastClientListened.address)
	{
		delete[] this->lastClientListened.address;
		this->lastClientListened.address = nullptr;
	}
	 */
	if(this->remoteHost)
	{
		delete this->remoteHost;
		this->remoteHost = nullptr;
	}
	if(this->socketServer) delete this->socketServer;
}

void oneLife::server::game::handler::Socket::listen()
{
	this->readySock = nullptr;
	this->lastConnection = nullptr;
	if(this->remoteHost)
	{
		delete this->remoteHost;
		this->remoteHost = nullptr;
	}

	if(!this->socketServer) this->initSocketServer();
	this->readySock = sockPoll.wait((int)(this->pollTimeout * 1000));

	/*
	if(this->lastClientListened.address)
	{
		delete[] this->lastClientListened.address;
		this->lastClientListened.address = nullptr;
	}
	this->lastClientListened.port = 0;
	*/

	if(this->isConnectionReceived())
	{
		this->lastConnection = this->socketServer->acceptConnection(0);
		if(this->lastConnection)
		{
			this->remoteHost = this->lastConnection->getRemoteHostAddress();
			/*
			if(this->remoteHost)
			{
				this->isClientKnown = true;
				this->lastClientListened.address = new char[strlen(this->remoteHost->mAddressString)+1];
				strcpy(this->lastClientListened.address, this->remoteHost->mAddressString);
				this->lastClientListened.port = this->remoteHost->mPort;
			}
			else this->isClientKnown = false;
			 */
		}
	}
}

Socket* oneLife::server::game::handler::Socket::getLastConnection()
{
	return this->lastConnection;
}

int oneLife::server::game::handler::Socket::getPort()
{
	return this->port;
}

oneLife::dataType::Connection oneLife::server::game::handler::Socket::getRemoteHost()
{
	oneLife::dataType::Connection remoteHost = {this->remoteHost->mAddressString, this->remoteHost->mPort};
	return remoteHost;
}

bool oneLife::server::game::handler::Socket::isConnectionAccepted()
{
	return (bool) this->lastConnection;
}

bool oneLife::server::game::handler::Socket::isConnectionReceived()
{
	return (bool) (this->readySock!=NULL && !this->readySock->isSocket);
}

bool oneLife::server::game::handler::Socket::isRemoteHostFound()
{
	return (bool) (this->remoteHost);
}

void oneLife::server::game::handler::Socket::setMaximumConnectionListened(int maxConnection)
{
	this->maxConnection = maxConnection;
}

void oneLife::server::game::handler::Socket::setPort(int port)
{
	this->port = port;
}

/*
char* oneLife::server::game::handler::Socket::getOutputMessage()
{
	this->outputMessage = oneLife::handler::Message::getOutputMessage();
}
*/

void oneLife::server::game::handler::Socket::to(FreshConnection newConnection)
{
	this->outputMessage = this->getOutputMessage();
	int messageLength = strlen(this->outputMessage);
	int numSent = newConnection.sock->send((unsigned char*)this->outputMessage,
							 messageLength,
							 false,
							 false);
	delete [] this->outputMessage;
	if( numSent != messageLength )
	{
		// failed or blocked on our first send attempt
		// reject it right away
		if(newConnection.sock)
		{
			delete newConnection.sock;
			newConnection.sock = NULL;
		}
	}
	else
	{
		// first message sent okay
		/*
		newConnection.sockBuffer = new SimpleVector<char>();
		sockPoll.addSocket( newConnection.sock );//create Connection
		newConnections.push_back( newConnection );
		 */
	}
}

void oneLife::server::game::handler::Socket::initSocketServer()
{
	this->socketServer = new SocketServer( this->port, this->maxConnection );
	sockPoll.addSocketServer( this->socketServer );
}

/**********************************************************************************************************************/

/**
 *
 * @param inO
 * @param inDestOverride
 * @param inDestOverrideX
 * @param inDestOverrideY
 * @return
 * @note from server/server.cpp => server/main.cpp
 * // sets lastSentMap in inO if chunk goes through
// returns result of send, auto-marks error in inO
 */
int sendMapChunkMessage( LiveObject *inO, char inDestOverride, int inDestOverrideX, int inDestOverrideY)
{
	if( ! inO->connected ) {
		// act like it was a successful send so we can move on until
		// they reconnect later
		return 1;
	}

	int messageLength = 0;

	int xd = inO->xd;
	int yd = inO->yd;

	if( inDestOverride ) {
		xd = inDestOverrideX;
		yd = inDestOverrideY;
	}


	int halfW = chunkDimensionX / 2;
	int halfH = chunkDimensionY / 2;

	int fullStartX = xd - halfW;
	int fullStartY = yd - halfH;

	int numSent = 0;



	if( ! inO->firstMapSent ) {
		// send full rect centered on x,y

		inO->firstMapSent = true;

		unsigned char *mapChunkMessage = getChunkMessage( fullStartX,
														  fullStartY,
														  chunkDimensionX,
														  chunkDimensionY,
														  inO->birthPos,
														  &messageLength );

		numSent +=
				inO->sock->send( mapChunkMessage,
								 messageLength,
								 false, false );

		delete [] mapChunkMessage;
	}
	else {

		// our closest previous chunk center
		int lastX = inO->lastSentMapX;
		int lastY = inO->lastSentMapY;


		// split next chunk into two bars by subtracting last chunk

		int horBarStartX = fullStartX;
		int horBarStartY = fullStartY;
		int horBarW = chunkDimensionX;
		int horBarH = chunkDimensionY;

		if( yd > lastY ) {
			// remove bottom of bar
			horBarStartY = lastY + halfH;
			horBarH = yd - lastY;
		}
		else {
			// remove top of bar
			horBarH = lastY - yd;
		}


		int vertBarStartX = fullStartX;
		int vertBarStartY = fullStartY;
		int vertBarW = chunkDimensionX;
		int vertBarH = chunkDimensionY;

		if( xd > lastX ) {
			// remove left part of bar
			vertBarStartX = lastX + halfW;
			vertBarW = xd - lastX;
		}
		else {
			// remove right part of bar
			vertBarW = lastX - xd;
		}

		// now trim vert bar where it intersects with hor bar
		if( yd > lastY ) {
			// remove top of vert bar
			vertBarH -= horBarH;
		}
		else {
			// remove bottom of vert bar
			vertBarStartY = horBarStartY + horBarH;
			vertBarH -= horBarH;
		}


		// only send if non-zero width and height
		if( horBarW > 0 && horBarH > 0 ) {
			int len;
			unsigned char *mapChunkMessage = getChunkMessage( horBarStartX,
															  horBarStartY,
															  horBarW,
															  horBarH,
															  inO->birthPos,
															  &len );
			messageLength += len;

			numSent +=
					inO->sock->send( mapChunkMessage,
									 len,
									 false, false );

			delete [] mapChunkMessage;
		}
		if( vertBarW > 0 && vertBarH > 0 ) {
			int len;
			unsigned char *mapChunkMessage = getChunkMessage( vertBarStartX,
															  vertBarStartY,
															  vertBarW,
															  vertBarH,
															  inO->birthPos,
															  &len );
			messageLength += len;

			numSent +=
					inO->sock->send( mapChunkMessage,
									 len,
									 false, false );

			delete [] mapChunkMessage;
		}
	}


	inO->gotPartOfThisFrame = true;


	if( numSent == messageLength ) {
		// sent correctly
		inO->lastSentMapX = xd;
		inO->lastSentMapY = yd;
	}
	else {
		setPlayerDisconnected( inO, "Socket write failed" );
	}
	return numSent;
}

/**
 *
 * @param inPlayer
 * @param inMessage
 * @param inLength
 * @note from server/server.cpp => server/main.cpp
 */
void sendMessageToPlayer( LiveObject *inPlayer, char *inMessage, int inLength )
{
	if( ! inPlayer->connected ) {
		// stop sending messages to disconnected players
		return;
	}


	unsigned char *message = (unsigned char*)inMessage;
	int len = inLength;

	char deleteMessage = false;

	if( inLength > maxUncompressedSize ) {
		message = makeCompressedMessage( inMessage, inLength, &len );
		deleteMessage = true;
	}

	int numSent =
			inPlayer->sock->send( message,
								  len,
								  false, false );

	if( numSent != len ) {
		setPlayerDisconnected( inPlayer, "Socket write failed" );
	}

	inPlayer->gotPartOfThisFrame = true;

	if( deleteMessage ) {
		delete [] message;
	}
}

/**
 *
 * @param inBuffer
 * @return
 * @note from server/server.cpp => server/main.cpp
 * // NULL if there's no full message available
 */
char *getNextClientMessage( SimpleVector<char> *inBuffer )
{
	// find first terminal character #

	int index = inBuffer->getElementIndex( '#' );

	if( index == -1 ) {

		if( inBuffer->size() > 200 ) {
			// 200 characters with no message terminator?
			// this->remoteHost is sending us nonsense
			// cut it off here to avoid buffer overflow

			AppLog::info( "More than 200 characters in this->remoteHost receive buffer "
						  "with no messsage terminator present, "
						  "generating NONSENSE message." );

			return stringDuplicate( "NONSENSE 0 0" );
		}

		return NULL;
	}

	if( index > 1 &&
		inBuffer->getElementDirect( 0 ) == 'K' &&
		inBuffer->getElementDirect( 1 ) == 'A' ) {

		// a KA (keep alive) message
		// short-cicuit the processing here

		inBuffer->deleteStartElements( index + 1 );
		return NULL;
	}



	char *message = new char[ index + 1 ];

	// all but terminal character
	for( int i=0; i<index; i++ ) {
		message[i] = inBuffer->getElementDirect( i );
	}

	// delete from buffer, including terminal character
	inBuffer->deleteStartElements( index + 1 );

	message[ index ] = '\0';

	return message;
}

/**
 *
 * @param inSock
 * @param inBuffer
 * @return
 * @note from server/server.cpp => server/main.cpp
 * // reads all waiting data from socket and stores it in buffer
// returns true if socket still good, false on error
 */
char readSocketFull( Socket *inSock, SimpleVector<char> *inBuffer )
{

	char buffer[512];

	int numRead = inSock->receive( (unsigned char*)buffer, 512, 0 );

	if( numRead == -1 ) {

		if( ! inSock->isSocketInFDRange() ) {
			// the internal FD of this socket is out of range
			// probably some kind of heap corruption.

			// save a bug report
			int allow =
					SettingsManager::getIntSetting( "allowBugReports", 0 );

			if( allow ) {
				char *bugName =
						autoSprintf( "bug_socket_%f", Time::getCurrentTime() );

				char *bugOutName = autoSprintf( "%s_out.txt", bugName );

				File outFile( NULL, "serverOut.txt" );
				if( outFile.exists() ) {
					fflush( stdout );
					File outCopyFile( NULL, bugOutName );

					outFile.copy( &outCopyFile );
				}
				delete [] bugName;
				delete [] bugOutName;
			}
		}


		return false;
	}

	while( numRead > 0 ) {
		inBuffer->appendArray( buffer, numRead );

		numRead = inSock->receive( (unsigned char*)buffer, 512, 0 );
	}

	return true;
}

/**
 *
 * @param inMessage
 * @note from server/server.cpp => server/main.cpp
 */
void sendGlobalMessage( char *inMessage )
{
	char found;
	char *noSpaceMessage = replaceAll( inMessage, " ", "_", &found );

	char *fullMessage = autoSprintf( "MS\n%s\n#", noSpaceMessage );

	delete [] noSpaceMessage;

	int len = strlen( fullMessage );

	for( int i=0; i<players.size(); i++ ) {
		LiveObject *o = players.getElement( i );

		if( ! o->error && ! o->isTutorial && o->connected ) {
			int numSent =
					o->sock->send( (unsigned char*)fullMessage,
								   len,
								   false, false );

			if( numSent != len ) {
				setPlayerDisconnected( o, "Socket write failed" );
			}
		}
	}
	delete [] fullMessage;
}

/**
 *
 * @param inPlayer
 * @note from server/server.cpp => server/main.cpp
 */
void sendCraving( LiveObject *inPlayer ) {
	// they earn the normal YUM multiplier increase (+1) if food is actually yum PLUS the bonus
	// increase, so send them the total.

	int totalBonus = inPlayer->cravingFood.bonus;
	if( isReallyYummy( inPlayer, inPlayer->cravingFood.foodID ) ) totalBonus = totalBonus + 1;

	char *message = autoSprintf( "CR\n%d %d\n#",
								 inPlayer->cravingFood.foodID,
								 totalBonus );
	sendMessageToPlayer( inPlayer, message, strlen( message ) );
	delete [] message;

	inPlayer->cravingKnown = true;
}

/**
 *
 * @param inPlayer
 * @param inReason
 * @note from server/server.cpp => server/main.cpp
 */
void setPlayerDisconnected( LiveObject *inPlayer, const char *inReason )
{
	/*
	setDeathReason( inPlayer, "disconnected" );

	inPlayer->error = true;
	inPlayer->errorCauseString = inReason;
	*/
	// don't kill them

	// just mark them as not connected

	AppLog::infoF( "Player %d (%s) marked as disconnected (%s).",
				   inPlayer->id, inPlayer->email, inReason );
	inPlayer->connected = false;

	// when player reconnects, they won't get a force PU message
	// so we shouldn't be waiting for them to ack
	inPlayer->waitingForForceResponse = false;


	if( inPlayer->vogMode ) {
		inPlayer->vogMode = false;

		GridPos p = inPlayer->preVogPos;

		inPlayer->xd = p.x;
		inPlayer->yd = p.y;

		inPlayer->xs = p.x;
		inPlayer->ys = p.y;

		inPlayer->birthPos = inPlayer->preVogBirthPos;
	}


	if( inPlayer->sock != NULL ) {
		// also, stop polling their socket, which will trigger constant
		// socket events from here on out, and cause us to busy-loop
		sockPoll.removeSocket( inPlayer->sock );

		delete inPlayer->sock;
		inPlayer->sock = NULL;
	}
	if( inPlayer->sockBuffer != NULL ) {
		delete inPlayer->sockBuffer;
		inPlayer->sockBuffer = NULL;
	}
}
