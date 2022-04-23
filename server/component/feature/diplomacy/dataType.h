//
// Created by olivier on 23/04/2022.
//

#ifndef ONELIFE_SERVER_COMPONENT_FEATURE_DIPLOMACY_DATATYPE_H
#define ONELIFE_SERVER_COMPONENT_FEATURE_DIPLOMACY_DATATYPE_H

typedef struct PeaceTreaty {
	int lineageAEveID;
	int lineageBEveID;

	// they have to say it in both directions
	// before it comes into effect
	char dirAToB;
	char dirBToA;

	// track directions of breaking it later
	char dirAToBBroken;
	char dirBToABroken;
} PeaceTreaty;

typedef struct WarPeaceMessageRecord
{
	char war;
	int lineageAEveID;
	int lineageBEveID;
	double t;
} WarPeaceMessageRecord;

#endif //ONELIFE_SERVER_COMPONENT_FEATURE_DIPLOMACY_DATATYPE_H
