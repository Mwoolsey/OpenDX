/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#ifndef _ColormapAddCtlDialog_h
#define _ColormapAddCtlDialog_h


#include "Dialog.h"

//
// Class name definition:
//
#define ClassColormapAddCtlDialog	"ColormapAddCtlDialog"

//
// XtCallbackProc (*CB), XtEventHandler (*EH) and XtActionProc (*AP)
// DialogCallback (*DCB), XtInputCallbackProc (*ICP), XtWorkProc (*WP)
// functions for this and derived classes
//
extern "C" void ColormapAddCtlDialog_LevelRangeCB(Widget, XtPointer, XtPointer);
extern "C" void ColormapAddCtlDialog_ValueRangeCB(Widget, XtPointer, XtPointer);
extern "C" void ColormapAddCtlDialog_AddCB(Widget, XtPointer, XtPointer);

class ColormapEditor;

//
// ColormapAddCtlDialog class definition:
//				

class ColormapAddCtlDialog : public Dialog
{
  private:
    //
    // Private member data:
    //
    static Boolean ClassInitialized;

  protected:
    //
    // Protected member data:
    //
    static String  DefaultResources[];
    friend void ColormapAddCtlDialog_AddCB(Widget, XtPointer , XtPointer);
    friend void ColormapAddCtlDialog_ValueRangeCB(Widget, XtPointer , XtPointer);
    friend void ColormapAddCtlDialog_LevelRangeCB(Widget, XtPointer , XtPointer);

    ColormapEditor* editor;

    Widget valuestepper;
    Widget valuelabel;
    Widget levelstepper;
    Widget levellabel;
    Widget addbtn;

    Widget createDialog(Widget);

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
    ColormapAddCtlDialog(Widget parent,ColormapEditor* editor);

    //
    // Destructor:
    //
    ~ColormapAddCtlDialog();

    void        setStepper();
    void        setFieldLabel(int);

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassColormapAddCtlDialog;
    }
};


#endif // _ColormapAddCtlDialog_h
