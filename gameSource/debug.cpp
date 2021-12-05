//
// Created by olivier on 05/12/2021.
//

#include "debug.h"

#include <cstdarg>
#include <cstring>
#include <cstdio>

void OneLife::game::Debug::writeAppInfo(const char* message, ...)
{
	va_list args;
	va_start (args, message);
	char buffer[512];
	memset(buffer, 0, sizeof(buffer));
	sprintf(buffer, "============>%s\n", message);
	openLife::system::Trace::writeLine(buffer, args);
}

void OneLife::game::Debug::writeControllerInfo(const char* message, ...)
{
	va_list args;
	va_start (args, message);
	char buffer[512];
	memset(buffer, 0, sizeof(buffer));
	sprintf(buffer, "=========>%s\n", message);
	openLife::system::Trace::writeLine(buffer, args);
}

void OneLife::game::Debug::writeControllerStepInfo(const char* message, ...)
{
	va_list args;
	va_start (args, message);
	char buffer[512];
	memset(buffer, 0, sizeof(buffer));
	sprintf(buffer, "======>%s\n", message);
	openLife::system::Trace::writeLine(buffer, args);
}

void OneLife::game::Debug::write(const char* message, ...)
{
	va_list args;
	va_start (args, message);
	char buffer[512];
	memset(buffer, 0, sizeof(buffer));
	sprintf(buffer, "##DEBUG## %s", message);
	openLife::system::Trace::writeLine(buffer, args);
}