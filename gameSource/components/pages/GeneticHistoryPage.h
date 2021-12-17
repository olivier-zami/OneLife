#include "OneLife/gameSource/controller.h"

#include "minorGems/ui/event/ActionListener.h"
#include "../../TextButton.h"

class GeneticHistoryPage : public Controller, public ActionListener {

    public:
        GeneticHistoryPage();
        ~GeneticHistoryPage();

		void handle(OneLife::dataType::UiComponent* screen);
        

        virtual void actionPerformed( GUIComponent *inTarget );

        virtual void draw( doublePair inViewCenter, 
                           double inViewSize );

        virtual void makeActive( char inFresh );

        virtual void specialKeyDown( int inKeyCode );

    protected:
        TextButton mBackButton;

        TextButton mRefreshButton;

        TextButton mLeaderboardButton;

        double mRefreshTime;
        

        int mSkip;
        
    };
