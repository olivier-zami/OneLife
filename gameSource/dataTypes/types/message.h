//
// Created by olivier on 20/12/2021.
//

#ifndef ONELIFE_DATA_TYPE_MESSAGE_H
#define ONELIFE_DATA_TYPE_MESSAGE_H

namespace OneLife::data::type::message
{
	typedef struct{
		struct{
			int x;
			int y;
		}topLeftPosition;
		struct{
			int width;
			int height;
		}dimension;
	}MapChunk;
}

#endif //ONELIFE_DATA_TYPE_MESSAGE_H
