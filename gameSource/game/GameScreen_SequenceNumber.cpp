//
// Created by olivier on 23/08/2022.
//

// need to respond with LOGIN message

char challengeString[200];

// we don't use these for anything in client
int currentPlayers = 0;
int maxPlayers = 0;
mRequiredVersion = versionNumber;

sscanf( message,
"SN\n"
"%d/%d\n"
"%199s\n"
"%d\n", &currentPlayers, &maxPlayers, challengeString,
&mRequiredVersion );


if( mRequiredVersion > versionNumber ||
( mRequiredVersion < versionNumber &&
		mRequiredVersion < dataVersionNumber ) ) {

// if server is using a newer version than us, we must upgrade
// our client

// if server is using an older version, check that
// their version is not behind our data version at least

socketHandler->disconnect();

setWaiting( false );

if( ! usingCustomServer &&
		mRequiredVersion < dataVersionNumber ) {
// we have a newer data version than the server
// the servers must be in the process of updating, and
// we connected at just the wrong time
// Don't display a confusing version mismatch message here.
setSignal( "serverUpdate" );
}
else {
setSignal( "versionMismatch" );
}

delete [] message;
return;
}

char *pureKey = getPureAccountKey();

char *password =
		SettingsManager::getStringSetting( "serverPassword" );

if( password == NULL ) {
password = stringDuplicate( "x" );
}


char *pwHash = hmac_sha1( password, challengeString );

char *keyHash = hmac_sha1( pureKey, challengeString );

delete [] pureKey;
delete [] password;


// we record the number of characters sent for playback
// if playback is using a different email.ini setting, this
// will fail.
// So pad the email with up to 80 space characters
// Thus, the login message is always this same length

char *twinExtra;

if( userTwinCode != NULL ) {
char *hash = computeSHA1Digest( userTwinCode );
twinExtra = autoSprintf( " %s %d", hash, userTwinCount );
delete [] hash;
}
else {
twinExtra = stringDuplicate( "" );
}


char *outMessage;

char *tempEmail;

if( strlen( userEmail ) > 0 ) {
std::string seededEmail = std::string( userEmail );

// If user doesn't have a seed in their email field
if( seededEmail.find('|') == std::string::npos && useSpawnSeed ) {
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
else {
// a blank email
// this will cause LOGIN message to have one less token

// stick a place-holder in there instead
tempEmail = stringDuplicate( "blank_email" );
}


if( strlen( tempEmail ) <= 80 ) {
outMessage = autoSprintf( "LOGIN %-80s %s %s %d%s#",
						  tempEmail, pwHash, keyHash,
						  mTutorialNumber, twinExtra );
}
else {
// their email is too long for this trick
// don't cut it off.
// but note that the playback will fail if email.ini
// doesn't match on the playback machine
outMessage = autoSprintf( "LOGIN %s %s %s %d%s#",
						  tempEmail, pwHash, keyHash,
						  mTutorialNumber, twinExtra );
}

delete [] tempEmail;
delete [] twinExtra;
delete [] pwHash;
delete [] keyHash;

sendToServerSocket( outMessage );

delete [] outMessage;

delete [] message;
return;