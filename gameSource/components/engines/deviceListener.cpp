//
// Created by olivier on 08/11/2021.
//

#include "deviceListener.h"

#include <SDL/SDL.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "../../dataTypes/hardware.h"

using namespace OneLife::dataType::hardware::keyboard;

OneLife::game::DeviceListener::DeviceListener()
{
	this->exit = false;
	this->keyboard.size = KEY::TOTAL_NUMBER * sizeof(char);
	this->keyboard.key = (char*)malloc(this->keyboard.size);
	memset(this->keyboard.key, 0, this->keyboard.size);
	printf("\n=====>keyboard.size nbr element: %i => %li", KEY::TOTAL_NUMBER, this->keyboard.size);
}

OneLife::game::DeviceListener::~DeviceListener()
{
	free(this->keyboard.key);
}

void OneLife::game::DeviceListener::listen()
{
	this->reset();
	SDL_Event event;
	while(SDL_PollEvent(&event))
	{
		switch(event.type)
		{
			case SDL_QUIT:
				printf("\n===>handle quit signal");
				this->exit = true;
				break;
			case SDL_KEYDOWN:
				this->updateKeyboard(event.key.keysym.sym, KEY_ACTION::PRESS);
				break;
			default:
				break;
		}
	}
	OneLife::game::dataType::Message msg;
	msg.type = OneLife::game::dataType::message::KEYBOARD;
	msg.size = 0;
	msg.content = this->keyboard.key;
	this->lastEvent.push_back(msg);
}

void OneLife::game::DeviceListener::reset()
{
	memset(this->keyboard.key, 0, this->keyboard.size);
	this->lastEvent.clear();
}

std::vector <OneLife::game::dataType::Message> OneLife::game::DeviceListener::getEvent()
{
	return this->lastEvent;
}

bool OneLife::game::DeviceListener::receiveQuitSignal()
{
	return this->exit;
}

void OneLife::game::DeviceListener::updateKeyboard(int key, int value)
{
	this->keyboard.key[(unsigned long int)this->localValue(key)] = value;
	SDLMod mods = SDL_GetModState();
	this->keyboard.key[KEY::ALT_LEFT] = (mods & KMOD_LALT) ? KEY_ACTION::PRESS : KEY_ACTION::NONE;
	this->keyboard.key[KEY::ALT_RIGHT] = (mods & KMOD_RALT) ? KEY_ACTION::PRESS : KEY_ACTION::NONE;
	this->keyboard.key[KEY::CTRL_LEFT] = (mods & KMOD_LCTRL) ? KEY_ACTION::PRESS : KEY_ACTION::NONE;
	this->keyboard.key[KEY::CTRL_RIGHT] = (mods & KMOD_LCTRL) ? KEY_ACTION::PRESS : KEY_ACTION::NONE;
	this->keyboard.key[KEY::META_LEFT] = (mods & KMOD_LMETA) ? KEY_ACTION::PRESS : KEY_ACTION::NONE;
	this->keyboard.key[KEY::META_RIGHT] = (mods & KMOD_RMETA) ? KEY_ACTION::PRESS : KEY_ACTION::NONE;
	this->keyboard.key[KEY::SHIFT_LEFT] = (mods & KMOD_LSHIFT) ? KEY_ACTION::PRESS : KEY_ACTION::NONE;
	this->keyboard.key[KEY::SHIFT_RIGHT] = (mods & KMOD_RSHIFT) ? KEY_ACTION::PRESS : KEY_ACTION::NONE;
	/*KMOD_NONE  = 0x0000,
	KMOD_NUM   = 0x1000,
	KMOD_CAPS  = 0x2000,
	KMOD_MODE  = 0x4000,
	*/
}

/**********************************************************************************************************************/
//!private

int OneLife::game::DeviceListener::localValue(int sdlValue)
{
	int value;
	switch(sdlValue)
	{
		case SDLK_0: value = KEY::NUM_0; break;
		case SDLK_1: value = KEY::NUM_1; break;
		case SDLK_2: value = KEY::NUM_2; break;
		case SDLK_3: value = KEY::NUM_3; break;
		case SDLK_4: value = KEY::NUM_4; break;
		case SDLK_5: value = KEY::NUM_5; break;
		case SDLK_6: value = KEY::NUM_6; break;
		case SDLK_7: value = KEY::NUM_7; break;
		case SDLK_8: value = KEY::NUM_8; break;
		case SDLK_a: value = KEY::A; break;
		case SDLK_b: value = KEY::B; break;
		case SDLK_c: value = KEY::C; break;
		case SDLK_d: value = KEY::D; break;
		case SDLK_e: value = KEY::E; break;
		case SDLK_f: value = KEY::F; break;
		case SDLK_g: value = KEY::G; break;
		case SDLK_h: value = KEY::H; break;
		case SDLK_i: value = KEY::I; break;
		case SDLK_j: value = KEY::J; break;
		case SDLK_k: value = KEY::K; break;
		case SDLK_l: value = KEY::L; break;
		case SDLK_m: value = KEY::M; break;
		case SDLK_n: value = KEY::N; break;
		case SDLK_o: value = KEY::O; break;
		case SDLK_p: value = KEY::P; break;
		case SDLK_q: value = KEY::Q; break;
		case SDLK_r: value = KEY::R; break;
		case SDLK_s: value = KEY::S; break;
		case SDLK_t: value = KEY::T; break;
		case SDLK_u: value = KEY::U; break;
		case SDLK_v: value = KEY::V; break;
		case SDLK_w: value = KEY::W; break;
		case SDLK_x: value = KEY::X; break;
		case SDLK_y: value = KEY::Y; break;
		case SDLK_z: value = KEY::Z; break;
		case SDLK_LALT: value = KEY::ALT_LEFT; break;
		case SDLK_RALT: value = KEY::ALT_RIGHT; break;
		case SDLK_LMETA: value = KEY::META_LEFT; break;
		case SDLK_RMETA: value = KEY::META_RIGHT; break;
		case SDLK_RETURN: value = KEY::RETURN; break;
		default: value = 0; printf("\n===>catch value = %i",  sdlValue); break;
	}
	return value;
}
