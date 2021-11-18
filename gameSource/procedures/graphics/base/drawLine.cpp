//
// Created by olivier on 17/11/2021.
//

#include "../base.h"

void drawLine(
		SpriteHandle inSegmentSprite,
		doublePair inStart,
		doublePair inEnd,
		FloatColor inColor )
{
	doublePair dir = normalize( sub( inEnd, inStart ) );
	doublePair perpDir = { -dir.y, dir.x };
	perpDir = mult( perpDir, 2 );

	doublePair spriteVerts[4] =
			{ { inStart.x - perpDir.x,
					  inStart.y - perpDir.y },
			  { inEnd.x - perpDir.x,
					  inEnd.y - perpDir.y },
			  { inEnd.x + perpDir.x,
					  inEnd.y + perpDir.y },
			  { inStart.x + perpDir.x,
					  inStart.y + perpDir.y } };

	FloatColor spriteColors[4] =
			{ inColor, inColor, inColor, inColor };

	drawSprite( inSegmentSprite, spriteVerts, spriteColors );
}

