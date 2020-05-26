#include <stdio.h>
#include "minorGems/util/SimpleVector.h"
#include "minorGems/util/stringUtils.h"


typedef struct EmailRecord {
        char *email;
        double lifeMinutes;
        int curses;
    } EmailRecord;


static SimpleVector<EmailRecord> records;


EmailRecord *getRecord( char *inEmail ) {
    
    for( int i=0; i<records.size(); i++ ) {
        EmailRecord *r = records.getElement( i );
        
        if( strcmp( r->email, inEmail ) == 0 ) {
            return r;
            }
        }
    
    EmailRecord r = { stringDuplicate( inEmail ), 0.0, 0 };
    
    records.push_back( r );
    
    return records.getElement( records.size() - 1 );
    }



int main() {
    FILE *curseFile = fopen( "tempCursedEmails.txt", "r" );

    FILE *livesFile = fopen( "tempEmailLives.txt", "r" );


    if( curseFile == NULL || livesFile == NULL ) {
        printf( "Needed files not found\n" );
        return 0;    
        }


    char email[500];
    double minutesLived = 0;
    int curses = 0;
    
    int numRead = 2;
    
    
    while( numRead == 2 ) {
    
        numRead = fscanf( livesFile, "%499s age=%lf", email, &minutesLived );
        
        if( numRead == 2 ) {
            EmailRecord *r = getRecord( email );
            r->lifeMinutes += minutesLived;
            }
        }

    numRead == 2;

    while( numRead == 2 ) {
    
        numRead = fscanf( curseFile, "%d %499s", &curses, email );
        
        if( numRead == 2 ) {
            EmailRecord *r = getRecord( email );
            r->curses += curses;
            }
        }
    
    

    fclose( curseFile );
    fclose( livesFile );




    
    for( int i=0; i<records.size(); i++ ) {
        EmailRecord *r = records.getElement( i );
    
        printf( "%f %d\n", r->lifeMinutes, r->curses );
    
        delete [] r->email;
        }
    

    return 1;
    }
