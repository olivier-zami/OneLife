//
// Created by olivier on 29/10/2021.
//

#ifndef ONELIFE_COMPONENT_ASYNCFILERECORD_H
#define ONELIFE_COMPONENT_ASYNCFILERECORD_H

typedef struct AsyncFileRecord {
	int handle;
	char *filePath;

	int dataLength;
	unsigned char *data;

	char doneReading;

} AsyncFileRecord;

#endif //ONELIFE_COMPONENT_ASYNCFILERECORD_H
