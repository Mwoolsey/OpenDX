/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#ifndef _ThrottleDialog_h
#define _ThrottleDialog_h


#include "Dialog.h"

//
// Class name definition:
//
#define ClassThrottleDialog	"ThrottleDialog"

//
// XtCallbackProc (*CB), XtEventHandler (*EH) and XtActionProc (*AP)
// DialogCallback (*DCB), XtInputCallbackProc (*ICP), XtWorkProc (*WP)
// functions for this and derived classes
//
extern "C" void ThrottleDialog_DoAllCB(Widget, XtPointer, XtPointer);

class ImageWindow;

//
// ThrottleDialog class definition:
//				

class ThrottleDialog : public Dialog
{
  private:
    //
    // Private member data:
    //
    static boolean ClassInitialized;

    Widget closebtn;
    Widget seconds;
    Widget label;

    ImageWindow* image;
 
  protected:
    //
    // Protected member data:
    //
    static String  DefaultResources[];
    friend void ThrottleDialog_DoAllCB(Widget, XtPointer , XtPointer);

    virtual Widget createDialog(Widget);

  public:

    //
    // Constructor:
    //
    ThrottleDialog(char *name, ImageWindow*);

    //
    // Destructor:
    //
    ~ThrottleDialog();

    virtual void    	manage();
    virtual void    	initialize();

    void installThrottleValue(double value);

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassThrottleDialog;
    }
};


#endif // _ThrottleDialog_h
