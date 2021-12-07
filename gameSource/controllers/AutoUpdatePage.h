#include "OneLife/gameSource/components/GamePage.h"

#include "minorGems/ui/event/ActionListener.h"


class AutoUpdatePage : public GamePage {
        
    public:

		AutoUpdatePage();
		~AutoUpdatePage();

		void handle(OneLife::dataType::UiComponent* screen);
        
        virtual void draw( doublePair inViewCenter, 
                           double inViewSize );
        
        virtual void step();
                
    };

