//
// Created by olivier on 06/11/2021.
//

#ifndef ONELIFE_DATATYPE_MESSAGE_KEYBOARD_H
#define ONELIFE_DATATYPE_MESSAGE_KEYBOARD_H

namespace Onelife::dataType::message
{
	class Keyboard
	{
		public:
			Keyboard();
			~Keyboard();

			void set(int idx, char value);
			void reset();
			bool isPressed(int key);

		private:
			char* key;
			long int size;

	};
}



#endif //ONELIFE_DATATYPE_MESSAGE_KEYBOARD_H
