#ifndef ONELIFE_SERVER_FEATURE_CRAVING_H
#define ONELIFE_SERVER_FEATURE_CRAVING_H

typedef struct Craving {
        int foodID;
        int uniqueID;
        int bonus;
    } Craving;


extern Craving noCraving;



Craving getCravedFood( int inLineageEveID, int inPlayerGenerationNumber,
                       Craving inLastCraved = noCraving );


void logFoodDepth( int inLineageEveID, int inEatenID );

// call periodically to free memory
// deletes records that contain uniqueID < inLowestUniqueID
void purgeStaleCravings( int inLowestUniqueID );

#endif// ONELIFE_SERVER_FEATURE_CRAVING_H