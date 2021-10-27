//
// Created by olivier on 26/10/2021.
//

#ifndef ONELIFE_DATATYPE_ANIMATION_H
#define ONELIFE_DATATYPE_ANIMATION_H

typedef struct DrawOrderRecord {
	char person;
	// if person
	LiveObject *personO;

	// if cell
	int mapI;
	int screenX, screenY;

	char extraMovingObj;
	// if extra moving obj
	int extraMovingIndex;

} DrawOrderRecord;

#endif //ONELIFE_DATATYPE_ANIMATION_H
