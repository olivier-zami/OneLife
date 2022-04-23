//
// Created by olivier on 20/04/2022.
//

#ifndef ONELIFE_COMMON_MATH_GEOMETRIC_H
#define ONELIFE_COMMON_MATH_GEOMETRIC_H

#include "../../gameSource/GridPos.h"

double intDist( int inXA, int inYA, int inXB, int inYB );
char isGridAdjacentDiag( int inXA, int inYA, int inXB, int inYB );
char isGridAdjacentDiag( GridPos inA, GridPos inB );
char isGridAdjacent( int inXA, int inYA, int inXB, int inYB );
char equal(GridPos inA, GridPos inB);

#endif //ONELIFE_COMMON_MATH_GEOMETRIC_H
