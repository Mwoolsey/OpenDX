/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#ifndef _TimedDialog_h
#define _TimedDialog_h


#include "Dialog.h"


//
// Class name definition:
//
#define ClassTimedDialog	"TimedDialog"

//
// XtCallbackProc (*CB), XtEventHandler (*EH) and XtActionProc (*AP)
// DialogCallback (*DCB) functions for this and derived classes
//
extern "C" void TimedDialog_TimeoutTO(XtPointer, XtIntervalId*);


//
// TimedDialog class definition:
//				
class TimedDialog : public Dialog
{
  private:
    static boolean ClassInitialized;

    friend void TimedDialog_TimeoutTO(XtPointer clientData, XtIntervalId*);

  protected:
    int          timeout;
    XtIntervalId timeoutId;
    //
    // Implementation of okCallback() for this class:
    //   Cleans up timeout.  Must be called by derived class's okCallback,
    //   if there is one.
    //
    virtual boolean okCallback(Dialog *clientData);

    virtual void cleanUp(){};

  public:
    //
    // Constructor:
    //
    TimedDialog(const char* name, Widget parent, int timeout);

    //
    // Destructor:
    //
    ~TimedDialog();

    //
    // A variation on the superclass post() function:
    //   Only the message variable parameter is specifiable.
    //   All other parameters are defaulted.
    //
    virtual void post();

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassTimedDialog;
    }
};
#endif // _TimedDialogManager_h
