//
// Created by olivier on 16/11/2021.
//

#include "misc.h"

#include "minorGems/game/drawUtils.h"
#include "OneLife/commonSource/fractalNoise.h"

// world width of one view
//FOV
int gui_hud_mode = 0;
float gui_fov_scale = 1.0f;
float gui_fov_scale_hud = 1.0f;
float gui_fov_target_scale_hud = 1.0f;
float gui_fov_preferred_max_scale = 3.0f;
int gui_fov_offset_x = (int)(((1280 * gui_fov_target_scale_hud) - 1280)/2);
int gui_fov_offset_y = (int)(((720 * gui_fov_target_scale_hud) - 720)/2);

//FOV
double recalcOffsetX( double x, bool force)
{
	double res;
	if( gui_hud_mode == 0 || force )
	{
		res = x + ( ( x > 0. ) ? gui_fov_offset_x : -gui_fov_offset_x );
		res /= 640. * gui_fov_target_scale_hud;
		res *= 640.;
	}
	else
	{
		res = x / gui_fov_target_scale_hud;
	}
	return res;
}

double recalcOffsetY( double y )
{
	double res;
	res = y + ( ( y > 0. ) ? gui_fov_offset_y : -gui_fov_offset_y );
	res /= 360. * gui_fov_target_scale_hud;
	res *= 360.;
	return res;
}

doublePair recalcOffset( doublePair ofs, bool force)
{
	ofs.x = recalcOffsetX( ofs.x, force );
	ofs.y = recalcOffsetY( ofs.y );
	return ofs;
}

double getBoundedRandom( int inX, int inY, double inUpper, double inLower )
{
	double val = getXYRandom( inX, inY );
	return val * ( inUpper - inLower ) + inLower;
}