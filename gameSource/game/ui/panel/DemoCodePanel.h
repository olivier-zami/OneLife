//
// Created by olivier on 18/08/2022.
//

#ifndef ONE_LIFE__GAME__GRAPHICS__UI__DEMO_CODE_PANEL_H
#define ONE_LIFE__GAME__GRAPHICS__UI__DEMO_CODE_PANEL_H

#include "minorGems/graphics/openGL/ScreenGL.h"


class DemoCodePanel
{

};

// shows panel until a correct code has been entered
// assumes font TGA in "graphics" folder
void showDemoCodePanel( ScreenGL *inScreen, const char *inFontTGAFileName, int inWidth, int inHeight );

// use this to check if demo checking done
char isDemoCodePanelShowing();

// call if app exits while panel still showing
void freeDemoCodePanel();


#endif //ONE_LIFE__GAME__GRAPHICS__UI__DEMO_CODE_PANEL_H
