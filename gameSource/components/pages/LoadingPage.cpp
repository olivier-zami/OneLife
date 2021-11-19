#include "LoadingPage.h"

#include "OneLife/gameSource/message.h"
#include "minorGems/game/drawUtils.h"
#include "OneLife/gameSource/procedures/graphics/screens.h"

LoadingPage::LoadingPage()
{
	this->screen.status.showProgressBar = true;
}

LoadingPage::~LoadingPage() {}

void LoadingPage::handle(OneLife::dataType::UiComponent* screen)
{
	screen->label = nullptr;
	screen->draw = OneLife::game::graphic::drawLoadingScreen;
	screen->body = &(this->screen);
}

void LoadingPage::showProgressBar(bool showProgressBar)
{
	this->screen.status.showProgressBar = showProgressBar;
}

void LoadingPage::setCurrentPhase( const char *inPhaseName )
{
    this->screen.taskName = inPhaseName;
}

void LoadingPage::setCurrentProgress( float inProgress )
{
    this->screen.progressBar = inProgress;
}

void LoadingPage::draw( doublePair inViewCenter, double inViewSize ){}

/**********************************************************************************************************************/

void LoadingPage::step()
{
	//printf("\n=====>loadingPage execute step()");
}