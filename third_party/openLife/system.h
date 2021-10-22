//
// Created by olivier on 20/10/2021.
//

#ifndef OPENLIFE_SYSTEM_H
#define OPENLIFE_SYSTEM_H

#define OPENLIFE_SYSTEM_VAR_STRING_NUMBER 5

namespace openLife
{
	class System
	{
		public:
			System();
			~System();

			static void setString(unsigned int id, char* ptr);
			static char* getString(unsigned int id);

			static const unsigned int CURRENT_APPLICATION_FILENAME;


		private:
			static System* instance;
			static void* memory;
			static void* ptr;
			static char* string[OPENLIFE_SYSTEM_VAR_STRING_NUMBER];
	};
}

#endif //ONELIFE_SYSTEM_H
