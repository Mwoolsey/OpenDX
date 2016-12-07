/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#ifndef _CommandInterface_h
#define _CommandInterface_h


#include <Xm/Xm.h>
#include "UIComponent.h"
#include "Client.h"


//
// Class name definition:
//
#define ClassCommandInterface	"CommandInterface"

//
// XtCallbackProc (*CB), XtEventHandler (*EH) and XtActionProc (*AP)
// DialogCallback (*DCB) functions for this and derived classes
//
extern "C" void CommandInterface_ExecuteCommandCB(Widget, XtPointer, XtPointer);


//
// Referenced classes:
//
class Command;

//
// CommandInterface class definition:
//				
class CommandInterface : public UIComponent, public Client
{

  protected:
    Command* command;	// Pointer to the associated Command object.

    //
    // This callback function is provided as a convenience to derived
    // classes.  This callback executes the Command object associated
    // with the CommandInterface object that invokes the callback.
    // Derived classes can register the callback, if desired.
    //
    friend void CommandInterface_ExecuteCommandCB(Widget    widget,
				       XtPointer clientData,
				       XtPointer callData);

    //
    // Constructor:
    //
    CommandInterface(char*    name,
		     Command* command = NUL(Command*));

    //
    // Replaces the old associated command with the specified command.
    // The new command may be NULL to disassociate the interface from
    // its command without replacing it.
    //
    inline void setCommand(Command* command);

    //
    // Implementation of superclass notify() pure function:
    //
    virtual void notify
	(const Symbol message, const void *msgdata=NULL, const char *msg=NULL);

    //
    // Process the widget callback
    //
    void executeCommand();

  public:
    //
    // Destructor:
    //
    ~CommandInterface();

    //
    // Get what should be made the parent of a modal dialog given.  The parent
    // will be an ancestor of the CommandInterface if not NULL.
    //
    Widget getDialogParent();

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassCommandInterface;
    }
};


#endif // _CommandInterface_h
