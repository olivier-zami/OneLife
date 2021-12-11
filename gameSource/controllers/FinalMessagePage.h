#include "OneLife/gameSource/components/controller.h"

#include "minorGems/ui/event/ActionListener.h"
#include "OneLife/gameSource/TextButton.h"


class FinalMessagePage : public OneLife::game::Controller, ActionListener {
        
    public:

        FinalMessagePage();
        
        ~FinalMessagePage();

		void handle(OneLife::dataType::UiComponent* screen);

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

