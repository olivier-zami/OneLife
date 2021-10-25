//
// Created by olivier on 25/10/2021.
//

#include "special.h"

#include <cmath>
#include "minorGems/system/Time.h"
#include "minorGems/game/gameGraphics.h"
#include "minorGems/util/random/JenkinsRandomSource.h"

extern JenkinsRandomSource randSource2;

void setTrippingColor( double x, double y ) {

	// Nothing fancy, just wanna map the screen x, y into [0, 1]
	// So hue change is continuous across the screen
	double factor = (int)(abs(x + 2 * y) / 3 / 128) % 10;
	factor /= 10;

	double curTime = Time::getCurrentTime();

	// Time between each color change
	int period = 2;

	int t1 = (int)curTime;
	int t_progress = (int)t1 % period;
	if( t_progress != 0 ) t1 -= t_progress;
	int t2 = t1 + period;

	randSource2.reseed( t1 );
	double r1 = randSource2.getRandomBoundedDouble( 0, 1 );
	double g1 = randSource2.getRandomBoundedDouble( 0, 1 );
	double b1 = randSource2.getRandomBoundedDouble( 0, 1 );
	r1 = (1 + factor) * (1 + r1);
	g1 = (1 + factor) * (1 + g1);
	b1 = (1 + factor) * (1 + b1);
	r1 = r1 - (int)r1;
	g1 = g1 - (int)g1;
	b1 = b1 - (int)b1;

	randSource2.reseed( t2 );
	double r2 = randSource2.getRandomBoundedDouble( 0, 1 );
	double g2 = randSource2.getRandomBoundedDouble( 0, 1 );
	double b2 = randSource2.getRandomBoundedDouble( 0, 1 );
	r2 = (1 + factor) * (1 + r2);
	g2 = (1 + factor) * (1 + g2);
	b2 = (1 + factor) * (1 + b2);
	r2 = r2 - (int)r2;
	g2 = g2 - (int)g2;
	b2 = b2 - (int)b2;

	// Colors fade from one period to the next
	double r = (r2 - r1) * (curTime - t1) / period + r1;
	double g = (g2 - g1) * (curTime - t1) / period + g1;
	double b = (b2 - b1) * (curTime - t1) / period + b1;
	setDrawColor( r, g, b, 1 );

}