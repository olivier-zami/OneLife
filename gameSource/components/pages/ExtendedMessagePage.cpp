#include "ExtendedMessagePage.h"

#include "minorGems/game/Font.h"
#include "OneLife/gameSource/misc.h"
#include "minorGems/util/stringUtils.h"
#include "OneLife/gameSource/buttonStyle.h"
#include "OneLife/gameSource/procedures/graphics/sprites/drawMessage.h"

extern Font *mainFont;
extern char *userEmail;
extern char *accountKey;


ExtendedMessagePage::ExtendedMessagePage()
        : mOKButton( mainFont, 0, -128, 
                     translate( "okay" ) ),
          mMessageKey( "" ),
          mSubMessage( NULL ) {

    addComponent( &mOKButton );
    
    setButtonStyle( &mOKButton );
    
    mOKButton.addActionListener( this );
    }


ExtendedMessagePage::~ExtendedMessagePage() {
    if( mSubMessage != NULL ) {
        delete [] mSubMessage;
        }
    }

void ExtendedMessagePage::handle(OneLife::dataType::UiComponent* screen)
{
	screen->label = nullptr;
	screen->draw = nullptr;
}


void ExtendedMessagePage::setMessageKey( const char *inMessageKey ) {
    mMessageKey = inMessageKey;
    }

void ExtendedMessagePage::setSubMessage( const char *inMessage ) {
    if( mSubMessage != NULL ) {
        delete [] mSubMessage;
        }
    mSubMessage = stringDuplicate( inMessage );
    }



        
void ExtendedMessagePage::actionPerformed( GUIComponent *inTarget ) {
    if( inTarget == &mOKButton ) {
        setSignal( "done" );
        }
    }



void ExtendedMessagePage::draw( doublePair inViewCenter, 
                                  double inViewSize ) {
    
    doublePair pos = { 0, 200 };
    
    drawMessage( mMessageKey, pos );
    
    if( mSubMessage != NULL ) {
        pos.y = 50;
        drawMessage( mSubMessage, pos );
        }
    
    }

