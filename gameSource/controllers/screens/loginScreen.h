//
// Created by olivier on 21/12/2021.
//

#ifndef ONELIFE_GAME_CONTROLLER_SCREEN_LOGIN_H
#define ONELIFE_GAME_CONTROLLER_SCREEN_LOGIN_H

#include "minorGems/ui/event/ActionListener.h"
#include "minorGems/ui/GUIComponent.h"
#include "OneLife/gameSource/controller.h"
#include "OneLife/gameSource/dataTypes/ui.h"
#include "OneLife/gameSource/dataTypes/types/message.h"

namespace OneLife::game
{
	class LoginScreen:
			public Controller,
			public ActionListener
	{
		public:
			LoginScreen();
			~LoginScreen();

			void handle(OneLife::dataType::UiComponent* screen);

			void readMessage(OneLife::data::type::message::LoginPrerequisite loginPrerequisite);//TODO should be private

		private:

		public://TODO put this in another class => remove them from Controlled if needed(Controller)
			void setToolTip(const char *inTip);
			void clearToolTip(const char *inTipToClear);
			void base_draw(doublePair inViewCenter, double inViewSize);
			//void base_step();
			void base_keyDown(unsigned char inASCII);
			char checkSignal(const char *inSignalName);
			void setWaiting(char inWaiting, char inWarningOnly=false);
			void showShutdownPendingWarning();
			void setSignal( const char *inSignalName );
			void clearSignal();
			char isAnySignalSet();

		public://TODO put this in another class => remove them from Controlled if needed (ActionListener)
			void actionPerformed( GUIComponent *inTarget );
	};
}
#endif //ONELIFE_GAME_CONTROLLER_SCREEN_LOGIN_H
