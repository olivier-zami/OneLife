#include "../GamePage.h"

#include "minorGems/ui/event/ActionListener.h"
#include "OneLife/gameSource/TextButton.h"


class FinalMessagePage : public GamePage, ActionListener {
        
    public:

        FinalMessagePage();
        
        ~FinalMessagePage();

		void handle(OneLife::dataType::ui::Screen* screen);

        virtual void actionPerformed( GUIComponent *inTarget );

        
        // must be a string literal
        void setMessageKey( const char *inKey );

        // copied internally
        // set to NULL to clear
        void setSubMessage( const char *inSubMessage );

        virtual void makeActive( char inFresh );

        virtual void draw( doublePair inViewCenter, 
                           double inViewSize );

    protected:
        const char *mKey;

        char *mSubMessage;

        TextButton mQuitButton;
        
    };

