//
// Created by olivier on 22/12/2021.
//

#ifndef ONELIFE_DATA_TYPE_H
#define ONELIFE_DATA_TYPE_H

namespace OneLife::data::type
{
	typedef struct{
		unsigned int type;
		unsigned int size;
		void* content;
	}Message;

	typedef struct{
		char* message;
	}ClientRequest;
}

#endif //ONELIFE_DATA_TYPE_H
