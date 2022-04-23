//
// Created by olivier on 22/04/2022.
//

#ifndef ONELIFE_SERVER_COMPONENT_FEATURE_DIPLOMACY_H
#define ONELIFE_SERVER_COMPONENT_FEATURE_DIPLOMACY_H

#include <cstddef>

#include "diplomacy/dataType.h"

namespace OneLife::server::feature
{
	class Diplomacy
	{

	};
}

void removePeaceTreaty( int inLineageAEveID, int inLineageBEveID );
void addPeaceTreaty( int inLineageAEveID, int inLineageBEveID );
char isPeaceTreaty( int inLineageAEveID, int inLineageBEveID, PeaceTreaty **outPartialTreaty = NULL );
PeaceTreaty *getMatchingTreaty( int inLineageAEveID, int inLineageBEveID );
void sendPeaceWarMessage( const char *inPeaceOrWar, char inWar, int inLineageAEveID, int inLineageBEveID );

#endif //ONELIFE_SERVER_COMPONENT_FEATURE_DIPLOMACY_H
