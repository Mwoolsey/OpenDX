/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#ifndef _CommandTextPopup_h
#define _CommandTextPopup_h


#include "TextPopup.h"
#include <X11/Intrinsic.h>

class GARChooserWindow;
class List;

extern "C" void CommandTextPopup_FileTO (XtPointer, XtIntervalId*);
extern "C" void CommandTextPopup_ModifyCB (Widget, XtPointer, XtPointer);

//
// Class name definition:
//
#define ClassCommandTextPopup	"CommandTextPopup"

//
// CommandTextPopup class definition:
//				
class CommandTextPopup : public TextPopup
{
  private:
    //
    // Private member data:
    //
    static boolean ClassInitialized;

    GARChooserWindow *chooser;

    //
    // The index of the directory history button to erase.  
    //
    int	lru_index;
    int dir_hist_size;
    XtIntervalId file_timer;

    void waitForFile();

    virtual void itemSelectCallback(const char *);

  protected:
    //
    // Protected member data:
    //

    static String DefaultResources[];

    //
    // One time class initializations.
    //
    virtual void initialize();

    //
    // Called when the user hits a return in the text window.
    //
    virtual void activateTextCallback(const char *, void *);

    List*	added_names;

    friend void CommandTextPopup_FileTO (XtPointer, XtIntervalId*);
    friend void CommandTextPopup_ModifyCB (Widget, XtPointer, XtPointer);

  public:
    //
    // Constructor:
    //
    CommandTextPopup(GARChooserWindow *chooser);

    //
    // Destructor:
    //
    ~CommandTextPopup();

    virtual Widget createTextPopup(Widget parent, 
				const char **items = NULL, int n = 0,
				SetTextCallback stc = NULL,
				ChangeTextCallback ctc = NULL,
				void *callbackData = NULL);

    //
    // Keep track of modifications to anyone using our XmTextSource.  It seems
    // that modifications in other text widgets don't trigger our modifyVerify
    // callback, so we will add a callback to the other widget(s).
    //
    void addCallback(Widget, XtCallbackProc cbp=NUL(XtCallbackProc));

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassCommandTextPopup;
    }
};


#endif // _CommandTextPopup_h


