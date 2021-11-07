#include "ExtendedMessagePage.h"

#include "minorGems/game/Font.h"
#include "OneLife/gameSource/game.h"
#include "minorGems/util/stringUtils.h"
#include "OneLife/gameSource/buttonStyle.h"
#include "OneLife/gameSource/message.h"

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

void ExtendedMessagePage::handle(OneLife::dataType::ui::Screen* screen)
{
	screen->label = nullptr;
	//memset(screen->label, 0, sizeof(screen->label));
	//strcpy(screen->label, "ExtendedMessagePage");
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

