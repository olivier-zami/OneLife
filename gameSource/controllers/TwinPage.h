#include "OneLife/gameSource/components/controller.h"

#include "minorGems/ui/event/ActionListener.h"
#include "minorGems/util/SimpleVector.h"
#include "minorGems/util/random/JenkinsRandomSource.h"
#include "OneLife/gameSource/TextField.h"
#include "OneLife/gameSource/TextButton.h"
#include "OneLife/gameSource/RadioButtonSet.h"

class TwinPage : public OneLife::game::Controller, public ActionListener {
        
    public:
        
        TwinPage();

		void handle(OneLife::dataType::UiComponent* screen);

        virtual ~TwinPage();
        
        virtual void actionPerformed( GUIComponent *inTarget );

        
        virtual void makeActive( char inFresh );
        
        virtual void draw( doublePair inViewCenter, 
                           double inViewSize );

    protected:
        JenkinsRandomSource mRandSource;
        
        TextField mCodeField;

        TextButton mGenerateButton;

        TextButton mCopyButton;
        TextButton mPasteButton;
        
        TextButton mLoginButton;
        
        TextButton mCancelButton;
        
        RadioButtonSet *mPlayerCountRadioButtonSet;
        
        SimpleVector<char*> mWordList;
        
        
    };

