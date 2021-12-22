//
// Created by olivier on 21/12/2021.
//

#include "loginScreen.h"

#include <cstddef>
#include <cstring>
#include <string>
#include "minorGems/util/stringUtils.h"
#include "minorGems/util/SettingsManager.h"
#include "minorGems/crypto/hashes/sha1.h"
#include "OneLife/gameSource/accountHmac.h"
#include "OneLife/gameSource/application.h"
#include "OneLife/gameSource/dataTypes/type.h"
#include "OneLife/gameSource/debug/console.h"

extern int versionNumber;
extern int dataVersionNumber;
extern char usingCustomServer;
extern char *userTwinCode;
extern int userTwinCount;
extern char *userEmail;

#include "OneLife/gameSource/components/pages/LivingLifePage.h"
extern LivingLifePage *livingLifePage;

OneLife::game::LoginScreen::LoginScreen() {}
OneLife::game::LoginScreen::~LoginScreen() {}

/**********************************************************************************************************************/

void OneLife::game::LoginScreen::handle(OneLife::dataType::UiComponent* screen)
{

}

/**********************************************************************************************************************/
void OneLife::game::LoginScreen::readMessage(OneLife::data::type::message::LoginPrerequisite loginPrerequisite)
{
	OneLife::debug::Console::showFunction("LivingLifePage::readMessage(loginPrerequisite, message)");

	// need to respond with LOGIN message

	// we don't use these for anything in client
	//loginPrerequisite.currentPlayers = 0;
	//loginPrerequisite.maxPlayers = 0;
	OneLife::game::Application::setLastServerVersion(loginPrerequisite.requiredVersion);//TODO: push this information with signal channel

	if((loginPrerequisite.requiredVersion>versionNumber)
	   || (loginPrerequisite.requiredVersion<versionNumber && loginPrerequisite.requiredVersion<dataVersionNumber))
	{
		// if server is using a newer version than us, we must upgrade
		// our client

		// if server is using an older version, check that
		// their version is not behind our data version at least
		this->socket->close();
		setWaiting( false );

		if(!usingCustomServer && loginPrerequisite.requiredVersion<dataVersionNumber)
		{
			// we have a newer data version than the server
			// the servers must be in the process of updating, and
			// we connected at just the wrong time
			// Don't display a confusing version mismatch message here.
			setSignal( "serverUpdate" );
		}
		else
		{
			setSignal( "versionMismatch" );
		}
		return;
	}

	char *pureKey = getPureAccountKey();
	char *password = SettingsManager::getStringSetting( "serverPassword" );
	if(password == NULL) password = stringDuplicate( "x" );
	char *pwHash = hmac_sha1( password, loginPrerequisite.challengeString );
	char *keyHash = hmac_sha1( pureKey, loginPrerequisite.challengeString );

	delete [] pureKey;
	delete [] password;

	// we record the number of characters sent for playback
	// if playback is using a different email.ini setting, this
	// will fail.
	// So pad the email with up to 80 space characters
	// Thus, the login message is always this same length

	char *twinExtra;

	if( userTwinCode != NULL )
	{
		char *hash = computeSHA1Digest( userTwinCode );
		twinExtra = autoSprintf( " %s %d", hash, userTwinCount );
		delete [] hash;
	}
	else
	{
		twinExtra = stringDuplicate( "" );
	}

	char *outMessage;
	char *tempEmail;
	if( strlen( userEmail ) > 0 )
	{
		std::string seededEmail = std::string( userEmail );

		// If user doesn't have a seed in their email field
		if( seededEmail.find('|') == std::string::npos ) {
			std::string seedList = SettingsManager::getSettingContents( "spawnSeed", "" );
			std::string seed = "";
			if( seedList == "" ) {
				seed = "";
			} else if( seedList.find('\n') == std::string::npos ) {
				seed = seedList;
			} else if( seedList.find('\n') != std::string::npos ) {
				seed = seedList.substr( 0, seedList.find('\n') );
			}

			// And if the user has a seed set in settings
			if( seed != "" ) {
				// Add seed delim and then seed
				seededEmail += '|';
				seededEmail += seed;
			}
		}

		tempEmail = stringDuplicate( seededEmail.c_str() );
	}
	else
	{
		// a blank email
		// this will cause LOGIN message to have one less token

		// stick a place-holder in there instead
		tempEmail = stringDuplicate( "blank_email" );
	}

	if( strlen( tempEmail ) <= 80 )
	{
		outMessage = autoSprintf( "LOGIN %-80s %s %s %d%s#",
				tempEmail,
				pwHash,
				keyHash,
				!SettingsManager::getIntSetting( "tutorialDone", 0 ),//TODO: add enableTutorial field in loginPrerequisites
				twinExtra );
	}
	else
	{
		// their email is too long for this trick
		// don't cut it off.
		// but note that the playback will fail if email.ini
		// doesn't match on the playback machine
		outMessage = autoSprintf( "LOGIN %s %s %s %d%s#",
				tempEmail,
				pwHash,
				keyHash,
				!SettingsManager::getIntSetting( "tutorialDone", 0 ),//TODO: add enableTutorial field in loginPrerequisites
				twinExtra );
	}
	delete [] tempEmail;
	delete [] twinExtra;
	delete [] pwHash;
	delete [] keyHash;

	livingLifePage->sendToServerSocket( outMessage );
	OneLife::data::type::ClientRequest clientRequest = {
			outMessage
	};
	this->send(clientRequest);

	delete [] outMessage;
}

/**********************************************************************************************************************/
void OneLife::game::LoginScreen::setToolTip(const char *inTip){}
void OneLife::game::LoginScreen::clearToolTip(const char *inTipToClear){}
void OneLife::game::LoginScreen::base_draw(doublePair inViewCenter, double inViewSize){}
//void OneLife::game::LoginScreen::base_step(){}
void OneLife::game::LoginScreen::base_keyDown(unsigned char inASCII){}
char OneLife::game::LoginScreen::checkSignal(const char *inSignalName){return -1;}
void OneLife::game::LoginScreen::setWaiting(char inWaiting, char inWarningOnly){}
void OneLife::game::LoginScreen::showShutdownPendingWarning(){}
void OneLife::game::LoginScreen::setSignal( const char *inSignalName ){}
void OneLife::game::LoginScreen::clearSignal(){}
char OneLife::game::LoginScreen::isAnySignalSet(){return -1;}

void OneLife::game::LoginScreen::actionPerformed( GUIComponent *inTarget ){};