/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#ifndef _SetDecoratorTextDialog_h
#define _SetDecoratorTextDialog_h


#include "Dialog.h"

//
// Class name definition:
//
#define ClassSetDecoratorTextDialog	"SetDecoratorTextDialog"

extern "C" void SetDecoratorTextDialog_DirtyTextCB(Widget , XtPointer , XtPointer);
extern "C" void SetDecoratorTextDialog_DirtyJustifyCB(Widget , XtPointer , XtPointer);
extern "C" void SetDecoratorTextDialog_DirtyColorsCB(Widget , XtPointer , XtPointer);
extern "C" void SetDecoratorTextDialog_DirtyFontCB(Widget , XtPointer , XtPointer);
extern "C" void SetDecoratorTextDialog_ApplyCB(Widget , XtPointer , XtPointer);
extern "C" void SetDecoratorTextDialog_RestoreCB(Widget , XtPointer , XtPointer);
extern "C" void SetDecoratorTextDialog_ReformatCB(Widget , XtPointer , XtPointer);
extern "C" void SetDecoratorTextDialog_StartPosCB(Widget , XtPointer , XtPointer);
extern "C" void SetDecoratorTextDialog_EndPosCB(Widget , XtPointer , XtPointer);
extern "C" void SetDecoratorTextDialog_ArrowMotionEH 
    (Widget , XtPointer , XEvent *, Boolean *);



class LabelDecorator;

//
// SetDecoratorTextDialog class definition:
//				

class SetDecoratorTextDialog : public Dialog
{
  private:
    //
    // Private member data:
    //
    static boolean ClassInitialized;
    static String  DefaultResources[];

    //
    // used as a bit field to show which attrs have been changed since
    // the dialog box was posted.
    //
    int dirty;

    int min_width;
    boolean negotiated;

    Widget justifyOM;
    Widget colorOM;
    Widget fontOM;
    Widget editorText;
    Widget editor_magic;
    Widget colorPulldown;
    Widget justifyPulldown;
    Widget fontPulldown;

    Widget createColorMenu (Widget );
    Widget createJustifyMenu (Widget );
    Widget createFontMenu (Widget );

    //
    // For justification
    //
    boolean measureLines(XmString, char*);
    char*   kern(char* src, char* font);
    int     line_count;
    int*    kern_lengths;
    char**  kern_lines;
    static  char* Pad(char* , int , int);

  protected:
    //
    // Protected member data:
    //
    Widget apply, restore, reformat, resize_arrow, resize_arrow_label;
    int start_pos;
    int initial_xpos;

    //
    // 1:1 correspondence between SetDecoratorTextDialog and LabelDecorator
    //
    LabelDecorator 	*decorator;

    friend void SetDecoratorTextDialog_DirtyTextCB(Widget , XtPointer , XtPointer);
    friend void SetDecoratorTextDialog_DirtyJustifyCB(Widget , XtPointer , XtPointer);
    friend void SetDecoratorTextDialog_DirtyColorsCB(Widget , XtPointer , XtPointer);
    friend void SetDecoratorTextDialog_DirtyFontCB(Widget , XtPointer , XtPointer);
    friend void SetDecoratorTextDialog_ApplyCB(Widget , XtPointer , XtPointer);
    friend void SetDecoratorTextDialog_RestoreCB(Widget , XtPointer , XtPointer);
    friend void SetDecoratorTextDialog_ReformatCB(Widget , XtPointer , XtPointer);
    friend void SetDecoratorTextDialog_StartPosCB(Widget , XtPointer , XtPointer);
    friend void SetDecoratorTextDialog_EndPosCB(Widget , XtPointer , XtPointer);
    friend void SetDecoratorTextDialog_ArrowMotionEH 
	    (Widget , XtPointer , XEvent *, Boolean *);

    virtual boolean okCallback (Dialog * );
    virtual boolean restoreCallback (Dialog * );
    virtual boolean reformatCallback (SetDecoratorTextDialog * );

    Widget createDialog (Widget parent);

    //
    // Install the default resources for this class and then call the
    // same super class method to get the default resources from the
    // super classes.
    //
    virtual void installDefaultResources(Widget baseWidget);

    //
    // Constructor:
    // There are 2 constructors because this class can be instantiated directly
    // as well as subclassed.
    SetDecoratorTextDialog(Widget , boolean , LabelDecorator *, const char* );

    //
    // The subclass might want to justify text differently
    //
    virtual const char* defaultJustifySetting();

  public:

    //
    // Constructor:
    //
    SetDecoratorTextDialog(Widget parent, boolean readonly, LabelDecorator *dec);

    //
    // Destructor:
    //
    ~SetDecoratorTextDialog();

    virtual void post();

    virtual void setDecorator(LabelDecorator*);

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassSetDecoratorTextDialog;
    }
};


#endif // _SetDecoratorTextDialog_h
