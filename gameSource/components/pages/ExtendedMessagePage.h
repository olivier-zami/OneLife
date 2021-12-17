#include "OneLife/gameSource/controller.h"

#include "minorGems/ui/event/ActionListener.h"
#include "OneLife/gameSource/TextButton.h"


class ExtendedMessagePage : public Controller, public ActionListener {
        
    public:
        ExtendedMessagePage();
        ~ExtendedMessagePage();

		void handle(OneLife::dataType::UiComponent* screen);

        void setMessageKey( const char *inMessageKey );
        
        // destroyed by caller
        void setSubMessage( const char *inMessage );
        

        virtual void actionPerformed( GUIComponent *inTarget );

        virtual void draw( doublePair inViewCenter, 
                           double inViewSize );
        
    protected:
        
        TextButton mOKButton;

        const char *mMessageKey;
        char *mSubMessage;


    };
