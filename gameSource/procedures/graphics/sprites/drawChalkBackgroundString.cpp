//
// Created by olivier on 16/11/2021.
//

#include "strings.h"

#include "minorGems/util/stringUtils.h"
#include "minorGems/game/Font.h"
#include "minorGems/util/random/JenkinsRandomSource.h"
#include "minorGems/graphics/Image.h"
#include "minorGems/io/file/File.h"
#include "minorGems/game/drawUtils.h"
#include "OneLife/gameSource/game.h"
#include "OneLife/gameSource/procedures/maths/misc.h"
#include "OneLife/gameSource/components/scenes/intangibles/text.h"

extern Font *handwritingFont;
extern float gui_fov_scale_hud;
extern doublePair lastScreenViewCenter;
extern float gui_fov_scale;
extern char savingSpeech;
extern char savingSpeechColor;
extern Image *speechMaskImage;
extern Image *speechColorImage;
extern char savingSpeechNumber;
extern char savingSpeechMask;

/**
 *
 * @param inPos
 * @param inString
 * @param inFade
 * @param inMaxWidth
 * @param inSpeaker
 * @param inForceMinChalkBlots
 * @param inForceBlotColor
 * @param inForceTextColor
 * @note: forces uppercase
 */
void OneLife::graphic::drawChalkBackgroundString(
		doublePair inPos,
		const char *inString,
		double inFade,
		double inMaxWidth,
		LiveObject *inSpeaker,
		int inForceMinChalkBlots,
		FloatColor *inForceBlotColor,
		FloatColor *inForceTextColor,
		SpriteHandle mChalkBlotSprite)
{
	char *stringUpper = stringToUpperCase( inString );
	SimpleVector<char*> *lines = splitLines( inString, inMaxWidth );
	delete [] stringUpper;
	if( lines->size() == 0 )
	{
		delete lines;
		return;
	}

	double lineSpacing = handwritingFont->getFontHeight() / 2 + ( 5 * gui_fov_scale_hud );

	double firstLineY =  inPos.y + ( lines->size() - 1 ) * lineSpacing;

	if( firstLineY > lastScreenViewCenter.y + recalcOffsetY( 330 ) * gui_fov_scale ) {
		firstLineY = lastScreenViewCenter.y + recalcOffsetY( 330 ) * gui_fov_scale;
	}


	if( inForceBlotColor != NULL ) {
		setDrawColor( *inForceBlotColor );
		setDrawFade( inFade );
	}
	else if( inSpeaker != NULL && inSpeaker->dying ) {
		if( inSpeaker->sick ) {
			// sick-ish yellow
			setDrawColor( 0.874510, 0.658824, 0.168627, inFade );
		}
		else {
			// wounded, blood red
			setDrawColor( .65, 0, 0, inFade );
		}
	}
	else if( inSpeaker != NULL && inSpeaker->curseLevel > 0 ) {
		setDrawColor( 0, 0, 0, inFade );
	}
	else {
		setDrawColor( 1, 1, 1, inFade );
	}



	char maskOnly = false;
	char colorOnly = false;

	if( savingSpeech && savingSpeechColor && inFade == 1.0 ) {
		drawSquare( inPos, 1024 * gui_fov_scale );
		colorOnly = true;
	}
	else if( savingSpeech && savingSpeechMask && inFade == 1.0 ) {
		setDrawColor( 0, 0, 0, 1.0 );
		drawSquare( inPos, 1024 * gui_fov_scale );
		setDrawColor( 1, 1, 1, 1 );
		maskOnly = true;
	}



	// with a fixed seed
	JenkinsRandomSource blotRandSource( 0 );

	for( int i=0; i<lines->size(); i++ ) {
		char *line = lines->getElementDirect( i );


		double length = handwritingFont->measureString( line );

		//FOV
		/*int numBlots = lrint( 0.25 + length / 20 ) + 1;

		if( inForceMinChalkBlots != -1 && numBlots < inForceMinChalkBlots ) {
			numBlots = inForceMinChalkBlots;
			}

		doublePair blotSpacing = { 20, 0 };*/

		doublePair firstBlot =
				{ inPos.x, firstLineY - i * lineSpacing};


		for( doublePair blotPos = firstBlot; blotPos.x < inPos.x + ( length + 20 * gui_fov_scale_hud ); blotPos.x += 20 * gui_fov_scale_hud ) {
			//doublePair blotPos = add( firstBlot, mult( blotSpacing, b ) );
			blotPos.y = firstBlot.y;

			double rot = blotRandSource.getRandomDouble();
			drawSprite( mChalkBlotSprite, blotPos, gui_fov_scale_hud, rot );
			drawSprite( mChalkBlotSprite, blotPos, gui_fov_scale_hud, rot );

			// double hit vertically
			blotPos.y += ( 5 * gui_fov_scale_hud );
			rot = blotRandSource.getRandomDouble();
			drawSprite( mChalkBlotSprite, blotPos, gui_fov_scale_hud, rot );

			blotPos.y -= ( 10 * gui_fov_scale_hud );
			rot = blotRandSource.getRandomDouble();
			drawSprite( mChalkBlotSprite, blotPos, gui_fov_scale_hud, rot );
		}
	}


	if( inForceTextColor != NULL ) {
		setDrawColor( *inForceTextColor );
		setDrawFade( inFade );
	}
	else if( inSpeaker != NULL && inSpeaker->dying && ! inSpeaker->sick ) {
		setDrawColor( 1, 1, 1, inFade );
	}
	else if( inSpeaker != NULL && inSpeaker->curseLevel > 0 ) {
		setDrawColor( 1, 1, 1, inFade );
		if( inSpeaker->speechIsSuccessfulCurse ) {
			setDrawColor( 0.875, 0, 0.875, inFade );
		}
	}
	else if( inSpeaker != NULL && inSpeaker->speechIsSuccessfulCurse ) {
		setDrawColor( 0.5, 0, 0.5, inFade );
	}
	else {
		setDrawColor( 0, 0, 0, inFade );
	}


	if( maskOnly ) {
		// font should add to opacity of mask too
		setDrawColor( 1, 1, 1, 1 );
	}


	for( int i=0; i<lines->size(); i++ ) {
		char *line = lines->getElementDirect( i );

		doublePair lineStart =
				{ inPos.x, firstLineY - i * lineSpacing};

		handwritingFont->drawString( line, lineStart, alignLeft );
		delete [] line;
	}

	delete lines;


	if( colorOnly ) {
		saveScreenShot( "speechColor", &speechColorImage );
		savingSpeechColor = false;
		savingSpeechMask = true;
	}
	else if( maskOnly ) {
		saveScreenShot( "speechMask", &speechMaskImage );
		savingSpeechMask = false;
		savingSpeech = false;
	}

	if( speechColorImage != NULL && speechMaskImage != NULL ) {
		// both screen shot requests are done

		Image *subColor = speechColorImage->getSubImage( 0, 0, 1280, 500 );
		Image *subMask = speechMaskImage->getSubImage( 0, 0, 1280, 500 );

		int w = subColor->getWidth();
		int h = subColor->getHeight();

		Image blend( w, h, 4, true );
		blend.paste( subColor );
		double *alpha = blend.getChannel( 3 );

		memcpy( alpha, subMask->getChannel( 0 ),
				w * h * sizeof( double ) );

		int minX = w -1;
		int maxX = 0;
		int minY = h -1;
		int maxY = 0;

		for( int y=0; y<h; y++ ) {
			for( int x=0; x<w; x++ ) {
				if( alpha[ y * w + x ] > 0 ) {

					if( x < minX ) {
						minX = x;
					}
					if( x > maxX ) {
						maxX = x;
					}

					if( y < minY ) {
						minY = y;
					}
					if( y > maxY ) {
						maxY = y;
					}
				}
			}
		}

		// expand 1 pixel to be safe
		if( minX > 0 ) {
			minX --;
		}
		if( minY > 0 ) {
			minY --;
		}
		if( maxX < w - 1 ) {
			maxX ++;
		}
		if( maxY < h - 1 ) {
			maxY ++;
		}


		Image *trimmed = blend.getSubImage( minX, minY,
				maxX - minX,
				maxY - minY );

		File screenShots( NULL, "screenShots" );

		char *fileName = autoSprintf( "speechBlend%04d.tga",
				savingSpeechNumber );
		savingSpeechNumber++;

		File *tgaFile = screenShots.getChildFile( fileName );

		delete [] fileName;

		char *tgaPath = tgaFile->getFullFileName();

		delete tgaFile;

		writeTGAFile( tgaPath, trimmed );

		delete [] tgaPath;

		delete trimmed;


		delete subColor;
		delete subMask;


		delete speechColorImage;
		speechColorImage = NULL;
		delete speechMaskImage;
		speechMaskImage = NULL;
	}
}
