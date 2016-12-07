/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#ifndef _SetSeparatorAttrDlg_h
#define _SetSeparatorAttrDlg_h


#include "Dialog.h"

//
// Class name definition:
//
#define ClassSetSeparatorAttrDlg	"SetSeparatorAttrDlg"

extern "C" void SetSeparatorAttrDlg_DirtyColorsCB(Widget , XtPointer , XtPointer);
extern "C" void SetSeparatorAttrDlg_DirtyStyleCB(Widget , XtPointer , XtPointer);
extern "C" void SetSeparatorAttrDlg_ApplyCB(Widget , XtPointer , XtPointer);
extern "C" void SetSeparatorAttrDlg_RestoreCB(Widget , XtPointer , XtPointer);


class SeparatorDecorator;

//
// SetSeparatorAttrDlg class definition:
//				

class SetSeparatorAttrDlg : public Dialog
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

    Widget colorOM;
    Widget colorPulldown;
    Widget styleOM;
    Widget stylePulldown;

    Widget createColorMenu (Widget );
    Widget createStyleMenu (Widget );

  protected:
    //
    // Protected member data:
    //

    //
    // 1:1 correspondence between SetSeparatorAttrDlg and SeparatorDecorator
    //
    SeparatorDecorator 	*decorator;

    friend void SetSeparatorAttrDlg_DirtyColorsCB(Widget , XtPointer , XtPointer);
    friend void SetSeparatorAttrDlg_DirtyStyleCB(Widget , XtPointer , XtPointer);
    friend void SetSeparatorAttrDlg_ApplyCB(Widget , XtPointer , XtPointer);
    friend void SetSeparatorAttrDlg_RestoreCB(Widget , XtPointer , XtPointer);

    boolean okCallback (Dialog * );
    boolean restoreCallback (Dialog * );

    Widget createDialog (Widget parent);

    //
    // Install the default resources for this class and then call the
    // same super class method to get the default resources from the
    // super classes.
    //
    virtual void installDefaultResources(Widget baseWidget);

  public:

    //
    // Constructor:
    //
    SetSeparatorAttrDlg(Widget parent, boolean readonly, SeparatorDecorator *dec);

    //
    // Destructor:
    //
    ~SetSeparatorAttrDlg();


    virtual void post();

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassSetSeparatorAttrDlg;
    }
};


#endif // _SetSeparatorAttrDlg_h
