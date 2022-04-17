//
// Created by olivier on 17/04/2022.
//

#include <math.h>

/**
 *
 * @param inInput
 * @param inKnee
 * @return
 * @note from server/map.cpp
 * // inKnee in 0..inf, smaller values make harder knees
// intput in 0..1
// output in 0..1

// from Simplest AI trick in the book:
// Normalized Tunable SIgmoid Function
// Dino Dini, GDC 2013
 */
double sigmoid(double inInput, double inKnee)
{

	// in -1,-1
	double shiftedInput = inInput * 2 - 1;

	double sign = 1;
	if (shiftedInput < 0) { sign = -1; }

	double k = -1 - inKnee;

	double absInput = fabs(shiftedInput);

	// out in -1..1
	double out = sign * absInput * k / (1 + k - absInput);

	return (out + 1) * 0.5;
}
