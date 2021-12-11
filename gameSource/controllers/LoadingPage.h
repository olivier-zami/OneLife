#include "OneLife/gameSource/components/controller.h"

#include "minorGems/ui/event/ActionListener.h"
#include "OneLife/gameSource/dataTypes/uiComponent/screens.h"


class LoadingPage : public OneLife::game::Controller
{
    public:
        LoadingPage();
		~LoadingPage();

		void handle(OneLife::dataType::UiComponent* screen);
		void showProgressBar(bool showProgressBar);// on by default
        void setCurrentPhase( const char *inPhaseName );
        void setCurrentProgress( float inProgress );

        virtual void draw( doublePair inViewCenter,double inViewSize );

	protected:
		void step();

    private:
		OneLife::dataType::uiComponent::LoadingScreen screen;
};
