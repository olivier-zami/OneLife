//
// Created by olivier on 22/03/2022.
//

#include "Socket.h"

#include "../../commonSource/Debug.h"

oneLife::game::component::Socket::Socket() {}
oneLife::game::component::Socket::~Socket() {}

/**
 *
 * @param message
 * @note: allow to add hand crafted socketMessage in the queue for test or anything else
 */
void oneLife::game::component::Socket::readMessage(const char* message)
{
	oneLife::Debug::write(message);
}