//
// Created by olivier on 04/05/2022.
//

#ifndef ONELIFE_SERVER_DATATYPE_MESSAGE_H
#define ONELIFE_SERVER_DATATYPE_MESSAGE_H

namespace OneLife::dataType::message
{
	typedef struct
	{
		struct{
			int x;
			int y;
		}origin;
		struct{
			int width;
			int height;
		}dimension;
	}MapChunk;
}

#endif //ONELIFE_SERVER_DATATYPE_MESSAGE_H
