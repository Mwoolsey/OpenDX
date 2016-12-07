/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include <defines.h>


#include "TimedDialog.h"
#include "Application.h"
boolean TimedDialog::ClassInitialized = FALSE;

TimedDialog::TimedDialog(const char* name, Widget parent, int timeout):
    Dialog(name, parent)
{
    this->timeout   = timeout;
    this->timeoutId = 0;
    if (NOT TimedDialog::ClassInitialized)
    {
        TimedDialog::ClassInitialized = TRUE;
	this->installDefaultResources(theApplication->getRootWidget());
    }
}

TimedDialog::~TimedDialog()
{
    if (this->timeoutId)
	XtRemoveTimeOut(this->timeoutId);
}

boolean TimedDialog::okCallback(Dialog *)
{
    this->unmanage();
    return TRUE;
}

extern "C" void TimedDialog_TimeoutTO(XtPointer clientData, XtIntervalId*)
{
    TimedDialog *d = (TimedDialog *)clientData;

    d->cleanUp();

    delete d;
}

void TimedDialog::post()
{
    this->Dialog::post();

    this->timeoutId =
	XtAppAddTimeOut(theApplication->getApplicationContext(), this->timeout,
		(XtTimerCallbackProc)TimedDialog_TimeoutTO, (XtPointer)this);
}
