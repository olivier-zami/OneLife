//
// Created by olivier on 12/11/2021.
//

#include "drawPausePanel.h"

#include "minorGems/game/gameGraphics.h"
#include "minorGems/game/drawUtils.h"
#include "minorGems/game/doublePair.h"
#include "minorGems/game/Font.h"
#include "minorGems/util/stringUtils.h"
#include "OneLife/gameSource/procedures/graphics/sprites/drawMessage.h"


#include "OneLife/gameSource/components/GamePage.h"
extern GamePage *currentGamePage;
#include "OneLife/gameSource/controllers/LivingLifePage.h"
extern LivingLifePage *livingLifePage;
#include "OneLife/gameSource/misc.h"

extern double viewWidth;
extern double viewHeight;
extern double viewHeightFraction;
extern float pauseScreenFade;
extern doublePair lastScreenViewCenter;
extern Font *mainFont;
extern char *currentUserTypedMessage;
extern float gui_fov_scale;
extern SpriteHandle instructionsSprite;



void drawPauseScreen()
{
	double viewHeight = viewHeightFraction * viewWidth;
	setDrawColor( 1, 1, 1, 0.5 * pauseScreenFade );
	drawSquare( lastScreenViewCenter, 1.05 * ( viewHeight / 3 ) );
	setDrawColor( 0.2, 0.2, 0.2, 0.85 * pauseScreenFade  );
	drawSquare( lastScreenViewCenter, viewHeight / 3 );
	setDrawColor( 1, 1, 1, pauseScreenFade );
	doublePair messagePos = lastScreenViewCenter;
	messagePos.y += 4.5  * (viewHeight / 15);
	mainFont->drawString( translate( "pauseMessage1" ), messagePos, alignCenter );
	messagePos.y -= 1.25 * (viewHeight / 15);
	mainFont->drawString( translate( "pauseMessage2" ), messagePos, alignCenter );
	if( currentGamePage == livingLifePage )
	{
		doublePair drawPos = { -9, 0 };
		drawPos = add( drawPos, lastScreenViewCenter );
		drawSprite( instructionsSprite, drawPos, gui_fov_scale );
		TextAlignment a = getMessageAlign();
		drawPos = lastScreenViewCenter;
		drawPos.x -= 600 * gui_fov_scale;
		drawPos.y += 320 * gui_fov_scale;
		doublePair rectPos = drawPos;
		rectPos.x += 155 * gui_fov_scale;
		rectPos.y -= 320 * gui_fov_scale;
		setDrawColor( 1, 1, 1, 0.5 * pauseScreenFade );
		drawRect( rectPos, 182 * gui_fov_scale, 362 * gui_fov_scale );
		setDrawColor( 0.2, 0.2, 0.2, 0.85 * pauseScreenFade  );
		drawRect( rectPos, 170 * gui_fov_scale, 350 * gui_fov_scale  );
		setMessageAlign( alignLeft );
		drawMessage( translate( "commandHintsA" ), drawPos, false, pauseScreenFade );
		drawPos = lastScreenViewCenter;
		drawPos.x += 285 * gui_fov_scale;
		drawPos.y += 320 * gui_fov_scale;
		rectPos = drawPos;
		rectPos.x += 160 * gui_fov_scale;
		rectPos.y -= 320 * gui_fov_scale;
		setDrawColor( 1, 1, 1, 0.5 * pauseScreenFade );
		drawRect( rectPos, 187 * gui_fov_scale, 362 * gui_fov_scale );
		setDrawColor( 0.2, 0.2, 0.2, 0.85 * pauseScreenFade  );
		drawRect( rectPos, 175 * gui_fov_scale, 350 * gui_fov_scale  );
		setMessageAlign( alignLeft );
		drawMessage( translate( "commandHintsB" ), drawPos, false, pauseScreenFade );
		setMessageAlign( a );
	}

	if( currentUserTypedMessage != NULL )
	{
		messagePos.y -= 1.25 * (viewHeight / 15);
		double maxWidth = 0.95 * ( viewHeight / 1.5 );
		int maxLines = 9;
		SimpleVector<char *> *tokens = tokenizeString( currentUserTypedMessage );
		SimpleVector<char *> lines;// collect all lines before drawing them
		while( tokens->size() > 0 )
		{
			// build up a a line
			// always take at least first token, even if it is too long
			char *currentLineString = stringDuplicate( *( tokens->getElement( 0 ) ) );
			delete [] *( tokens->getElement( 0 ) );
			tokens->deleteElement( 0 );
			char nextTokenIsFileSeparator = false;
			char *nextLongerString = NULL;
			if( tokens->size() > 0 )
			{
				char *nextToken = *( tokens->getElement( 0 ) );
				if( nextToken[0] == 28 )
				{
					nextTokenIsFileSeparator = true;
				}
				else
				{
					nextLongerString = autoSprintf( "%s %s ", currentLineString, *( tokens->getElement( 0 ) ) );
				}
			}

			while( !nextTokenIsFileSeparator
				   && nextLongerString != NULL
				   && mainFont->measureString( nextLongerString ) < maxWidth
				   && tokens->size() > 0 )
			{

				delete [] currentLineString;

				currentLineString = nextLongerString;

				nextLongerString = NULL;

				// token consumed
				delete [] *( tokens->getElement( 0 ) );
				tokens->deleteElement( 0 );

				if( tokens->size() > 0 ) {

					char *nextToken = *( tokens->getElement( 0 ) );

					if( nextToken[0] == 28 ) {
						nextTokenIsFileSeparator = true;
					}
					else {
						nextLongerString =
								autoSprintf( "%s%s ",
										currentLineString,
										*( tokens->getElement( 0 ) ) );
					}
				}
			}

			if( nextLongerString != NULL )
			{
				delete [] nextLongerString;
			}

			while( mainFont->measureString( currentLineString ) > maxWidth )
			{

				// single token that is too long by itself
				// simply trim it and discard part of it
				// (user typing nonsense anyway)

				currentLineString[ strlen( currentLineString ) - 1 ] =
						'\0';
			}

			if( currentLineString[ strlen( currentLineString ) - 1 ] == ' ' ) {
				// trim last bit of whitespace
				currentLineString[ strlen( currentLineString ) - 1 ] = '\0';
			}

			lines.push_back( currentLineString );

			if( nextTokenIsFileSeparator )
			{
				// file separator

				// put a paragraph separator in
				lines.push_back( stringDuplicate( "---" ) );

				// token consumed
				delete [] *( tokens->getElement( 0 ) );
				tokens->deleteElement( 0 );
			}
		}

		// all tokens deleted above
		delete tokens;

		double messageLineSpacing = 0.625 * (viewHeight / 15);
		int numLinesToSkip = lines.size() - maxLines;
		if( numLinesToSkip < 0 )
		{
			numLinesToSkip = 0;
		}


		for( int i=0; i<numLinesToSkip-1; i++ )
		{
			char *currentLineString = *( lines.getElement( i ) );
			delete [] currentLineString;
		}

		int lastSkipLine = numLinesToSkip - 1;

		if( lastSkipLine >= 0 )
		{
			char *currentLineString = *( lines.getElement( lastSkipLine ) );
			// draw above and faded out somewhat
			doublePair lastSkipLinePos = messagePos;
			lastSkipLinePos.y += messageLineSpacing;
			setDrawColor( 1, 1, 0.5, 0.125 * pauseScreenFade );
			mainFont->drawString( currentLineString, lastSkipLinePos, alignCenter );
			delete [] currentLineString;
		}

		setDrawColor( 1, 1, 0.5, pauseScreenFade );

		for( int i=numLinesToSkip; i<lines.size(); i++ )
		{
			char *currentLineString = *( lines.getElement( i ) );
			if( false && lastSkipLine >= 0 )
			{
				if( i == numLinesToSkip )
				{
					// next to last
					setDrawColor( 1, 1, 0.5, 0.25 * pauseScreenFade );
				}
				else if( i == numLinesToSkip + 1 )
				{
					// next after that
					setDrawColor( 1, 1, 0.5, 0.5 * pauseScreenFade );
				}
				else if( i == numLinesToSkip + 2 )
				{
					// rest are full fade
					setDrawColor( 1, 1, 0.5, pauseScreenFade );
				}
			}

			mainFont->drawString( currentLineString, messagePos, alignCenter );
			delete [] currentLineString;
			messagePos.y -= messageLineSpacing;
		}
	}

	setDrawColor( 1, 1, 1, pauseScreenFade );
	messagePos = lastScreenViewCenter;
	messagePos.y -= 3.75 * ( viewHeight / 15 );
	//mainFont->drawString( translate( "pauseMessage3" ),
	//                      messagePos, alignCenter );
	messagePos.y -= 0.625 * (viewHeight / 15);
	const char* quitMessageKey = "pauseMessage3";

	if( isQuittingBlocked() )
	{
		quitMessageKey = "pauseMessage3b";
	}

	mainFont->drawString( translate( quitMessageKey ), messagePos, alignCenter );
}
