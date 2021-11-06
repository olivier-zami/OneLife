#include "../GamePage.h"

#include "minorGems/ui/event/ActionListener.h"
#include "OneLife/gameSource/TextButton.h"
#include "OneLife/gameSource/CheckboxButton.h"
#include "OneLife/gameSource/RadioButtonSet.h"
#include "OneLife/gameSource/ValueSlider.h"
#include "OneLife/gameSource/SoundUsage.h"
#include "OneLife/gameSource/DropdownList.h"


class SettingsPage : public GamePage, public ActionListener {
        
    public:
        
        SettingsPage();
        ~SettingsPage();

		void handle(OneLife::dataType::ui::Screen* screen);

        virtual void draw( doublePair inViewCenter, 
                           double inViewSize );

        virtual void step();

        virtual void actionPerformed( GUIComponent *inTarget );

        
        virtual void makeActive( char inFresh );
        virtual void makeNotActive();

    protected:
        
        int mOldFullscreenSetting;
        int mOldBorderlessSetting;
        int mEnableNudeSetting;
        int mEnableFOVSetting;
        int mEnableKActionsSetting;
        int mEnableCenterCameraSetting;
        
        SoundUsage mTestSound;

        double mMusicStartTime;


        TextButton mInfoSeeds;
        TextButton mBackButton;
        TextButton mEditAccountButton;
        TextButton mRestartButton;
        TextButton mRedetectButton;

        CheckboxButton mFullscreenBox;
        CheckboxButton mBorderlessBox;
        
		CheckboxButton mEnableNudeBox;
		CheckboxButton mEnableFOVBox;
		CheckboxButton mEnableKActionsBox;
		CheckboxButton mEnableCenterCameraBox;

        ValueSlider mMusicLoudnessSlider;
        ValueSlider mSoundEffectsLoudnessSlider;


        DropdownList mSpawnSeed;

        RadioButtonSet *mCursorModeSet;
        
        ValueSlider mCursorScaleSlider;

    };
