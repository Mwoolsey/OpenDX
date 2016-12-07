/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _ColormapWaveDialog_h
#define _ColormapWaveDialog_h


#include "Dialog.h"
#include "ColormapEditor.h"
#include "ColormapNode.h"

//
// Class name definition:
//
#define ClassColormapWaveDialog	"ColormapWaveDialog"

//
// XtCallbackProc (*CB), XtEventHandler (*EH) and XtActionProc (*AP)
// DialogCallback (*DCB), XtInputCallbackProc (*ICP), XtWorkProc (*WP)
// functions for this and derived classes
//
extern "C" void ColormapWaveDialog_AddCB(Widget, XtPointer, XtPointer);
extern "C" void ColormapWaveDialog_CloseCB(Widget, XtPointer, XtPointer);
extern "C" void ColormapWaveDialog_PopdownCB(Widget, XtPointer, XtPointer);
extern "C" void ColormapWaveDialog_RangeCB(Widget, XtPointer, XtPointer);

class ColormapEditor;

//
// ColormapWaveDialog class definition:
//				

class ColormapWaveDialog : public Dialog
{
  friend class ColormapEditor;

  private:
    //
    // Private member data:
    //
    static boolean ClassInitialized;

    ColormapEditor* editor;

    Widget waveoption;
    Widget rangeoption;
    Widget rangefull;
    Widget rangeselected;
    Widget nstep;
    Widget step;
    Widget square_sol;
    Widget square_sor;
    Widget saw_sol;
    Widget saw_sor;
    Widget closebtn;
    Widget applybtn;

  protected:
    //
    // Protected member data:
    //
    static String  DefaultResources[];


    friend void ColormapWaveDialog_RangeCB(Widget, XtPointer, XtPointer);
    friend void ColormapWaveDialog_PopdownCB(Widget, XtPointer , XtPointer);
    friend void ColormapWaveDialog_CloseCB(Widget, XtPointer , XtPointer);
    friend void ColormapWaveDialog_AddCB(Widget, XtPointer , XtPointer);
    void rangeCallback(Widget    widget);


    virtual Widget createDialog(Widget);

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
    ColormapWaveDialog(Widget parent,ColormapEditor* editor);

    //
    // Destructor:
    //
    ~ColormapWaveDialog();

    void        setStepper();


    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassColormapWaveDialog;
    }
};


#endif // _ColormapWaveDialog_h
