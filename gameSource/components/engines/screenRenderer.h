//
// Created by olivier on 01/11/2021.
//

#ifndef INC_2HOL_SCREENRENDERER_H
#define INC_2HOL_SCREENRENDERER_H

#include "OneLife/gameSource/dataTypes/ui.h"

namespace OneLife::game
{
	class ScreenRenderer
	{
		public:
			ScreenRenderer(OneLife::dataType::ui::Screen screen);
			~ScreenRenderer();

			void setDefault(
					double width,
					double height,
					bool forceAspectRatio,
					bool doNotChangeNativeResolution,
					bool fullScreen,
					bool forceSpecifiedDimensions);

			void switchFullScreenMode();
			void switchMinimizedMode();
			void render(OneLife::dataType::UiComponent* screen = nullptr);

		private:
			double height;
			double width;
			bool forceAspectRatio;
			bool doNotChangeNativeResolution;
			bool fullScreen;
			bool forceSpecifiedDimensions;

			void setupSurface();
	};
}

int computeAspectRatio( int inW, int inH );//src in application.cpp
void grabInput( char inGrabOn );
void drawFrameNoUpdate( char inUpdate );
void showDiedPage();
void showReconnectPage();

#endif //INC_2HOL_SCREENRENDERER_H
