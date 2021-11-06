#include "../GamePage.h"

#include "minorGems/ui/event/ActionListener.h"


class AutoUpdatePage : public GamePage {
        
    public:

		AutoUpdatePage();
		~AutoUpdatePage();

		void handle(OneLife::dataType::ui::Screen* screen);
        
        virtual void draw( doublePair inViewCenter, 
                           double inViewSize );
        
        virtual void step();
                
    };

