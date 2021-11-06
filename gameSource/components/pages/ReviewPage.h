#include "ServerActionPage.h"

#include "minorGems/ui/event/ActionListener.h"
#include "OneLife/gameSource/TextButton.h"
#include "OneLife/gameSource/TextArea.h"
#include "OneLife/gameSource/TextField.h"
#include "OneLife/gameSource/RadioButtonSet.h"

class ReviewPage : public ServerActionPage, public ActionListener {
        
    public:
        
        ReviewPage( const char *inReviewServerURL );
        ~ReviewPage();

		void handle(OneLife::dataType::ui::Screen* screen);

        virtual void draw( doublePair inViewCenter, 
                           double inViewSize );

        virtual void step();

        virtual void actionPerformed( GUIComponent *inTarget );
        
        virtual void keyDown( unsigned char inASCII );

        
        virtual void makeActive( char inFresh );
        virtual void makeNotActive();

    protected:

        TextField mReviewNameField;
        
        RadioButtonSet *mRecommendChoice;

        TextArea mReviewTextArea;
        
        CheckboxButton mSpellcheckButton;

        TextButton mBackButton;

        
        TextButton mPostButton;
        TextButton mRemoveButton;
        

        TextButton mCopyButton;
        TextButton mPasteButton;

        TextButton mClearButton;
        
        char mGettingSequenceNumber;
        char mRemoving;
        

        void switchFields();

        void checkCanPost();
        void checkCanPaste();
        
        void saveReview();

    };
