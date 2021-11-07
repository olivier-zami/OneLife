//
// Created by olivier on 06/11/2021.
//

#include "keyboard.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "OneLife/gameSource/dataTypes/hardware.h"

using namespace OneLife::dataType::hardware::Keyboard;

Onelife::dataType::message::Keyboard::Keyboard()
{
	printf("\n==========>Virtual Keyboard Instantiation");
	printf("\nSize : %i", KEY::TOTAL_NUMBER);
	printf("\n");
	this->size = KEY::TOTAL_NUMBER * sizeof(char);
	this->key = (char*)malloc(this->size);
	memset(this->key, 0,this->size);
}

Onelife::dataType::message::Keyboard::~Keyboard()
{
	free(this->key);
}

void Onelife::dataType::message::Keyboard::set(int idx, char value)
{
	this->key[idx] = value;
}

void Onelife::dataType::message::Keyboard::reset()
{
	memset(this->key, 0,this->size);
}

bool Onelife::dataType::message::Keyboard::isPressed(int key)
{
	return (bool)(this->key[key] == 1);
}

