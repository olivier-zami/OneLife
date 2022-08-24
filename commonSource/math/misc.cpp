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

/**
 *
 * @param inF
 * @return
 * @note from server/server.cpp =>server/main.cpp
 */
float sign( float inF ) {
	if (inF > 0) return 1;
	if (inF < 0) return -1;
	return 0;
}

/**
 *
 * @param inRA
 * @param inRB
 * @return
 * @note from server/server.cpp => server/main.cpp
 * // blend R-values multiplicatively, for layers
// 1 - R( A + B ) = (1 - R(A)) * (1 - R(B))
//
// or
//
//R( A + B ) =  R(A) + R(B) - R(A) * R(B)
 */
double rCombine( double inRA, double inRB ) {
	return inRA + inRB - inRA * inRB;
}