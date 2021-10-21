//
// Created by olivier on 20/10/2021.
//

#include "exception.h"

#include <iostream>
#include <cstdarg>
#include <cstring>
#include <cstdlib>

openLife::system::Exception::Exception(const char *message, ...)
{
	if(message != nullptr)
	{
		char *response = (char*)malloc(255 * sizeof(char));
		memset(response, 0, 255);

		va_list arg;
		va_start(arg, message);
		for(unsigned int i=0; i<strlen(message); i++)
		{
			if(message[i]=='%')
			{
				i++;
				switch(message[i])
				{
					case 'i':
						std:: cout << va_arg(arg, int);
						break;
					case 'f':
						std::cout << va_arg(arg, double);
						break;
					case 'c':
						std::cout << (char)va_arg(arg, int);
						break;
					case 's':
						std::cout << va_arg( arg, char*);
						break;
					default:
						std::cout << "%" << message[i];
						break;
				}
			}
			else std::cout << message[i];
		}
		va_end(arg);
		this->message = message;
	}
	else this->message = "Throw of an exception without description";
}

openLife::system::Exception::~Exception()
{}

openLife::system::Exception *openLife::system::Exception::operator()(const char *message)
{
	this->message = message;
	return this;
}

std::string openLife::system::Exception::getMessage()
{
	return this->message;
}

