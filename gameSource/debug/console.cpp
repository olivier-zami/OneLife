//
// Created by olivier on 05/12/2021.
//

#include "console.h"

#include <cstdarg>
#include <cstring>
#include <cstdio>

void OneLife::debug::Console::showApplication(const char* message, ...)
{
	va_list args;
	va_start (args, message);
	char buffer[512];
	memset(buffer, 0, sizeof(buffer));
	sprintf(buffer, "%s\n", message);
	openLife::system::Trace::writeLine(buffer, args);
}

void OneLife::debug::Console::showController(const char* message, ...)
{
	va_list args;
	va_start (args, message);
	char buffer[512];
	memset(buffer, 0, sizeof(buffer));
	sprintf(buffer, "\n##CALL CONTROLLER##===>%s", message);
	openLife::system::Trace::writeLine(buffer, args);
}

void OneLife::debug::Console::showControllerStep(const char* message, ...)
{
	va_list args;
	va_start (args, message);
	char buffer[512];
	memset(buffer, 0, sizeof(buffer));
	sprintf(buffer, "##CTRL STEP##===>%s", message);
	openLife::system::Trace::writeLine(buffer, args);
}

void OneLife::debug::Console::showFunction(const char* message, ...)
{
	va_list args;
	va_start (args, message);
	char buffer[512];
	memset(buffer, 0, sizeof(buffer));
	sprintf(buffer, "\n##CALL FUNCTION##===>%s", message);
	openLife::system::Trace::writeLine(buffer, args);
}

void OneLife::debug::Console::showFunctionStep(const char* message, ...)
{
	va_list args;
	va_start (args, message);
	char buffer[512];
	memset(buffer, 0, sizeof(buffer));
	sprintf(buffer, "##FUNC STEP##===>%s", message);
	openLife::system::Trace::writeLine(buffer, args);
}

void OneLife::debug::Console::write(const char* message, ...)
{
	va_list args;
	va_start (args, message);
	char buffer[512];
	memset(buffer, 0, sizeof(buffer));
	sprintf(buffer, "##DEBUG## %s", message);
	openLife::system::Trace::writeLine(buffer, args);
}