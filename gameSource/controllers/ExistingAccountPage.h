#ifndef ONELIFE_GAME_ACCOUNT_H
#define ONELIFE_GAME_ACCOUNT_H

#include "OneLife/gameSource/components/controller.h"

#include "OneLife/gameSource/TextField.h"
#include "OneLife/gameSource/TextButton.h"
#include "OneLife/gameSource/KeyEquivalentTextButton.h"
#include "minorGems/ui/event/ActionListener.h"

class ExistingAccountPage : public OneLife::game::Controller, public ActionListener {
        
    public:
        ExistingAccountPage();
        virtual ~ExistingAccountPage();

		void handle(OneLife::dataType::UiComponent* screen);
        void clearFields();
        void showReviewButton( char inShow );// defaults to true
        void showDisableCustomServerButton( char inShow );// defaults to false
        virtual void actionPerformed( GUIComponent *inTarget );
        virtual void makeActive( char inFresh );
        virtual void makeNotActive();
        virtual void step();
        virtual void keyDown( unsigned char inASCII );// for TAB and ENTER (switch fields and start login)
        virtual void specialKeyDown( int inKeyCode );  // for arrow keys (switch fields)
        virtual void draw(doublePair inViewCenter, double inViewSize);

    protected:
		void processLogin(char inStore, unsigned int signalType, unsigned int page=0, const char* message=nullptr);
		void switchFields();

        TextField mEmailField;
        TextField mKeyField;
        TextField *mFields[2];
        TextButton mAtSignButton;
        KeyEquivalentTextButton mPasteButton;
        TextButton mDisableCustomServerButton;
        TextButton mLoginButton;
        TextButton mFriendsButton;
        TextButton mGenesButton;
        TextButton mFamilyTreesButton;
        TextButton mTechTreeButton;
        TextButton mClearAccountButton;
        TextButton mCancelButton;
        TextButton mSettingsButton;
        TextButton mReviewButton;
        TextButton mRetryButton;
        TextButton mRedetectButton;
        TextButton mViewAccountButton;
        TextButton mTutorialButton;
        double mPageActiveStartTime;
        int mFramesCounted;
        char mFPSMeasureDone;
        char mHideAccount;
    };

#endif //ONELIFE_GAME_ACCOUNT_H
