#include "FinalMessagePage.h"

#include "minorGems/game/Font.h"
#include "OneLife/gameSource/game.h"
#include "minorGems/util/stringUtils.h"
#include "OneLife/gameSource/procedures/graphics/sprites/drawMessage.h"
#include "OneLife/gameSource/buttonStyle.h"


extern Font *mainFont;



FinalMessagePage::FinalMessagePage()
        : mKey( "" ), mSubMessage( NULL ),
          mQuitButton( mainFont, 0, -300, 
                       translate( "quit" ) ) {


    setButtonStyle( &mQuitButton );
    addComponent( &mQuitButton );
    
    mQuitButton.addActionListener( this );
    }



FinalMessagePage::~FinalMessagePage() {
    setSubMessage( NULL );
    }

void FinalMessagePage::handle(OneLife::dataType::UiComponent* screen)
{
	screen->label = nullptr;
	screen->draw = nullptr;
}


void FinalMessagePage::actionPerformed( GUIComponent *inTarget ) {
    if( inTarget == &mQuitButton ) {
        setSignal( "quit" );
        }
    }




void FinalMessagePage::setMessageKey( const char *inKey ) {
    mKey = inKey;
    }



void FinalMessagePage::setSubMessage( const char *inSubMessage ) {
    if( mSubMessage != NULL ) {
        delete [] mSubMessage;
        mSubMessage = NULL;
        }
    if( inSubMessage != NULL ) {
        mSubMessage = stringDuplicate( inSubMessage );
        }
    }




void FinalMessagePage::makeActive( char inFresh ) {
    if( !inFresh ) {
        return;
        }
    
    setWaiting( false );
    }


void FinalMessagePage::draw( doublePair inViewCenter, 
                              double inViewSize ) {

    doublePair labelPos = { 0, 100 };

    
    drawMessage( translate( mKey ), labelPos, false );

    if( mSubMessage != NULL ) {    
        labelPos.y -= 200;
        
        drawMessage( mSubMessage, labelPos, false );
        }
    
    }


