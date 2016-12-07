/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#include "NewCommand.h"
#include "DXApplication.h"
#include "DXWindow.h"
#include "EditorWindow.h"

NewCommand::NewCommand(const char      *name,
		       CommandScope    *scope,
		       boolean		active,
		       Network	       *net,
		       Widget		dialogParent) :
    OptionalPreActionCommand(name, scope, active,
			     "Save Confirmation",
			     "Do you want to save the program?",
				dialogParent)
{
    this->network = net;
}

void   NewCommand::doPreAction()
{
    Network *net = theDXApplication->network;
    const char    *fname = net->getFileName();


    if(fname)
    {
        if(net->saveNetwork(fname))
            this->doIt(NULL);
    }
    else {
	Widget parent = this->dialogParent;
        if (!parent)
            parent = theApplication->getRootWidget();
        net->postSaveAsDialog(parent, this);
    }

}

boolean NewCommand::doIt(CommandInterface *ci)
{
    theDXApplication->resetServer();
    return this->network->clear();
}


boolean NewCommand::needsConfirmation()
{
#if 0
    //
    // If the parent was not set yet, then try and set it more intelligently
    // then the super class. 
    //
    Widget parent = this->dialogParent;
    if (!parent) {
	EditorWindow *editor = this->network->getEditor();
	DXWindow *anchor = theDXApplication->getAnchor(); 
	if (editor)
	    parent = editor->getRootWidget();
	if (!parent && anchor) 
	    parent = anchor->getRootWidget(); 
	this->dialogParent = parent;
    }
#endif

    return this->network->saveToFileRequired();
}
