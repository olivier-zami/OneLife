#ifndef ONELIFE_GAME_ASSETLOADER_H
#define ONELIFE_GAME_ASSETLOADER_H

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
		void loadSprites();
		void loadSounds();
		void loadAnimations();
		void loadObjects();
		void loadCategories();
		void loadTransitions();
		void loadGroundTextures();
		void loadSocketConfiguration();
		void initOther();

		struct{
			bool isSpritesLoaded;
			bool isSoundsLoaded;
			bool isAnimationsLoaded;
			bool isObjectsLoaded;
			bool isCategoriesLoaded;
			bool isTransitionsLoaded;
			bool isGroundTexturesLoaded;
			bool isSocketSet;
			bool isInitOther;
		}status;

		OneLife::dataType::uiComponent::LoadingScreen screen;
};

#endif //ONELIFE_GAME_ASSETLOADER_H
