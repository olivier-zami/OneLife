//
// Created by olivier on 18/04/2022.
//

#ifndef ONELIFE_SERVER_COMPONENT_FEATURE_APOCALYPSE_H
#define ONELIFE_SERVER_COMPONENT_FEATURE_APOCALYPSE_H

#include "../../dataType/LiveObject.h"

char isApocalypseTrigger( int inID );
int getMonumentStatus( int inID );
void apocalypseStep();
void backToBasics( LiveObject *inPlayer );
void triggerApocalypseNow();

#endif //ONELIFE_SERVER_COMPONENT_FEATURE_APOCALYPSE_H
