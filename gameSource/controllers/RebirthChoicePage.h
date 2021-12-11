#include "OneLife/gameSource/components/controller.h"

#include "minorGems/ui/event/ActionListener.h"
#include "OneLife/gameSource/TextButton.h"


class RebirthChoicePage : public OneLife::game::Controller, public ActionListener {
        
    public:
        RebirthChoicePage();

		void handle(OneLife::dataType::UiComponent* screen);

        void showReviewButton( char inShow );


        virtual void actionPerformed( GUIComponent *inTarget );

        virtual void draw( doublePair inViewCenter, 
                           double inViewSize );

        virtual void makeActive( char inFresh );
        
    protected:
        
        TextButton mQuitButton;
        TextButton mReviewButton;
        TextButton mRebornButton;
        TextButton mGenesButton;
        
        TextButton mTutorialButton;
        TextButton mMenuButton;

    };
