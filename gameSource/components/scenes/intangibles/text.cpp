//
// Created by olivier on 16/11/2021.
//

#include "text.h"

#include "minorGems/util/stringUtils.h"
#include "minorGems/game/Font.h"

extern Font *handwritingFont;

SimpleVector<char*> *splitLines( const char *inString, double inMaxWidth )
{
	// break into lines
	SimpleVector<char *> *tokens = tokenizeString( inString );

	// collect all lines before drawing them
	SimpleVector<char *> *lines = new SimpleVector<char*>();

	if( tokens->size() > 0 )
	{
		// start with firt token
		char *firstToken = tokens->getElementDirect( 0 );
		lines->push_back( firstToken );
		tokens->deleteElement( 0 );
	}

	while( tokens->size() > 0 )
	{
		char *nextToken = tokens->getElementDirect( 0 );
		char *currentLine = lines->getElementDirect( lines->size() - 1 );
		char *expandedLine = autoSprintf( "%s %s", currentLine, nextToken );
		if( handwritingFont->measureString( expandedLine ) <= inMaxWidth )
		{
			// replace current line
			delete [] currentLine;
			lines->deleteElement(  lines->size() - 1 );
			lines->push_back( expandedLine );
		}
		else
		{
			// expanded is too long
			// put token at start of next line
			delete [] expandedLine;
			lines->push_back( stringDuplicate( nextToken ) );
		}
		delete [] nextToken;
		tokens->deleteElement( 0 );
	}
	delete tokens;
	return lines;
}