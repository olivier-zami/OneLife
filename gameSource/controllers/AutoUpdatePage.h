#include "OneLife/gameSource/components/controller.h"

#include "minorGems/ui/event/ActionListener.h"


class AutoUpdatePage : public OneLife::game::Controller {
        
    public:

		AutoUpdatePage();
		~AutoUpdatePage();

		void handle(OneLife::dataType::UiComponent* screen);
        
        virtual void draw( doublePair inViewCenter, 
                           double inViewSize );
        
        virtual void step();
                
    };

