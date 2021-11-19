#ifndef ONELIFE_GRAPHIC_DRAWMESSAGE
#define ONELIFE_GRAPHIC_DRAWMESSAGE

#include "minorGems/game/doublePair.h"
#include "minorGems/game/Font.h"

// defaults to alignCenter
void setMessageAlign( TextAlignment inAlign );

TextAlignment getMessageAlign();




void drawMessage(
		const char *inTranslationKey,
		doublePair inCenter,
		char inRed = false,
		double inFade = 1.0 );

#endif
