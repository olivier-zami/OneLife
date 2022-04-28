//
// Created by olivier on 25/04/2022.
//

#include "Biome.h"

#include "../../../commonSource/Debug.h"

//DB biomeDB;
//char biomeDBOpen = false;

OneLife::server::database::Biome::Biome(
		const char* path,
		int mode,
		unsigned long hash_table_size,
		unsigned long key_size,
		unsigned long value_size):
		OneLife::server::bank::LinearDB::LinearDB(path, mode, hash_table_size, key_size, value_size)
{
	OneLife::Debug::write("init biomeDB : %s", this->settings.path);
	//this->settings.db = &biomeDB;
}

OneLife::server::database::Biome::~Biome() {}
