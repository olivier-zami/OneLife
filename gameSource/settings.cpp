//
// Created by olivier on 11/11/2021.
//

#include "settings.h"

double viewWidth = 1280;
double viewHeight = 720;
int musicOff = 0;

// should sound be initialized?
char getUsesSound()
{
	return ! musicOff;
}

char doesOverrideGameImageSize()
{
	return true;
}

void getGameImageSize( int *outWidth, int *outHeight )
{
	*outWidth = (int)viewWidth;
	*outHeight = (int)viewHeight;
}