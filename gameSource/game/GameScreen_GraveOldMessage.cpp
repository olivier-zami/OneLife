//
// Created by olivier on 23/08/2022.
//

int posX, posY, playerID, displayID;
double age;

int eveID = -1;


char nameBuffer[200];

nameBuffer[0] = '\0';

int numRead = sscanf( message, "GO\n%d %d %d %d %lf %199s",
					  &posX, &posY, &playerID, &displayID,
					  &age, nameBuffer );
if( numRead == 6 ) {
applyReceiveOffset( &posX, &posY );

GridPos thisPos = { posX, posY };

for( int i=0; i<graveRequestPos.size(); i++ ) {
if( equal( graveRequestPos.getElementDirect( i ),
		thisPos ) ) {
graveRequestPos.deleteElement( i );
break;
}
}

int nameLen = strlen( nameBuffer );
for( int i=0; i<nameLen; i++ ) {
if( nameBuffer[i] == '_' ) {
nameBuffer[i] = ' ';
}
}


SimpleVector<int> otherLin;

int numLines;

char **lines = split( message, "\n", &numLines );

if( numLines > 1 ) {
SimpleVector<char *> *tokens =
		tokenizeString( lines[1] );

int numNormalTokens = tokens->size();

if( tokens->size() > 6 ) {
char *lastToken =
		tokens->getElementDirect(
				tokens->size() - 1 );

if( strstr( lastToken, "eve=" ) ) {
// eve tag at end
numNormalTokens--;

sscanf( lastToken, "eve=%d", &( eveID ) );
}
}

for( int t=6; t<numNormalTokens; t++ ) {
char *tok = tokens->getElementDirect( t );

int mID = 0;
sscanf( tok, "%d", &mID );

if( mID != 0 ) {
otherLin.push_back( mID );
}
}
tokens->deallocateStringElements();
delete tokens;
}


for( int i=0; i<numLines; i++ ) {
delete [] lines[i];
}
delete [] lines;

LiveObject *ourLiveObject = getOurLiveObject();

char *relationName = getRelationName(
		&( ourLiveObject->lineage ),
		&otherLin,
		ourID,
		playerID,
		ourLiveObject->displayID,
		displayID,
		ourLiveObject->age,
		age,
		ourLiveObject->lineageEveID,
		eveID );

GraveInfo g;
g.worldPos.x = posX;
g.worldPos.y = posY;

char *des = relationName;
char *desToDelete = NULL;

if( des == NULL ) {
des = (char*)translate( "unrelated" );

if( strcmp( nameBuffer, "" ) == 0 ||
strcmp( nameBuffer, "~" ) == 0 ) {
// call them nameless instead
des = (char*)translate( "namelessPerson" );

if( playerID == 0 ) {
// call them forgotten instead
des = (char*)translate( "forgottenPerson" );
}
}
}
if( strcmp( nameBuffer, "" ) != 0 &&
strcmp( nameBuffer, "~" ) != 0 ) {
des = autoSprintf( "%s - %s",
				   nameBuffer, des );
desToDelete = des;
}

g.relationName = stringDuplicate( des );

if( desToDelete != NULL ) {
delete [] desToDelete;
}

if( relationName != NULL ) {
delete [] relationName;
}

g.creationTime =
game_getCurrentTime() - age / ourLiveObject->ageRate;

if( age == -1 ) {
g.creationTime = 0;
g.creationTimeUnknown = true;
}
else {
g.creationTimeUnknown = false;
}

g.lastMouseOverYears = -1;
g.lastMouseOverTime = g.creationTime;

mGraveInfo.push_back( g );
}
