//
// Created by olivier on 19/08/2022.
//

#ifndef ONE_LIFE__GAME__GRAPHICS__UI__DEMO_CODE_PANEL__KEYBOARD__HANDLER_H
#define ONE_LIFE__GAME__GRAPHICS__UI__DEMO_CODE_PANEL__KEYBOARD__HANDLER_H

#include "DemoCodeChecker.h"
#include "minorGems/graphics/openGL/gui/TextFieldGL.h"
#include "minorGems/graphics/openGL/gui/LabelGL.h"
#include "minorGems/graphics/openGL/KeyboardHandlerGL.h"
#include "minorGems/util/SettingsManager.h"

extern TextFieldGL *enterDemoCodeField;
extern DemoCodeChecker *codeChecker;

//TODO put this in DemoCodePanel.cpp
const char *getDemoCodeServerURL();
const char *getDemoCodeSharedSecret();

//TODO: put this in game/gui/Label.cpp
extern LabelGL *messageLabel;
void setLabelString( LabelGL *inLabel, const char *inTranslationString, double inScaleFactor = 1 );
/*********/

class DemoCodePanelKeyboardHandler : public KeyboardHandlerGL {
public:
	void keyPressed( unsigned char inKey, int inX, int inY ) {


		if( inKey == 13 && enterDemoCodeField->isFocused() ) {

			// don't destroy this
			char *enteredCode = enterDemoCodeField->getText();

			if( strlen( enteredCode ) > 0 ) {
				// disable further input
				enterDemoCodeField->setEnabled( false );
				enterDemoCodeField->setFocus( false );
				enterDemoCodeField->lockFocus( false );

				setLabelString( messageLabel,
								"checkingCode" );

				// save this for next time
				SettingsManager::setSetting( "demoCode",
											 enteredCode );

				// start
				codeChecker = new DemoCodeChecker(
						enteredCode,
						getDemoCodeSharedSecret(),
						getDemoCodeServerURL() );
			}
		}
		else if( inKey == 'q' || inKey == 'Q' || inKey == 27 ) {
			// q or escape
			exit( 0 );
		}
	}


	char isFocused() {
		// always focused
		return true;
	}

	void specialKeyPressed( int inKey, int inX, int inY ) {
	}

	void keyReleased( unsigned char inKey, int inX, int inY ) {
	}

	void specialKeyReleased( int inKey, int inX, int inY ) {
	}

};

#endif //ONE_LIFE__GAME__GRAPHICS__UI__DEMO_CODE_PANEL__KEYBOARD__HANDLER_H
