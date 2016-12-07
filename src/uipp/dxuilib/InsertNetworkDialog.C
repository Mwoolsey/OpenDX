/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"






#include <stdio.h>

#include "DXStrings.h"
#include "List.h"
#include "ListIterator.h"
#include "DXApplication.h"
#include "InsertNetworkDialog.h"
#include "OpenNetCommentDialog.h"
#include "ErrorDialogManager.h"
#include "InfoDialogManager.h"
#include "Network.h"
#include "Node.h"
#include "EditorWindow.h"
#include "WarningDialogManager.h"



boolean InsertNetworkDialog::ClassInitialized = FALSE;

String InsertNetworkDialog::DefaultResources[] =
{
        ".dialogTitle:     Insert...",
        "*dirMask:         *.net",
        "*helpLabelString: Comments",
        NULL
};

InsertNetworkDialog::InsertNetworkDialog(Widget parent) : 
		OpenNetworkDialog("insertNetworkDialog",parent) 
{ 
    if (NOT InsertNetworkDialog::ClassInitialized)
    {
        InsertNetworkDialog::ClassInitialized = TRUE;
	this->installDefaultResources(theApplication->getRootWidget());
    }
}

InsertNetworkDialog::InsertNetworkDialog(const char *name, Widget parent) : 
                       OpenNetworkDialog(name, parent) { }

InsertNetworkDialog::~InsertNetworkDialog() { }

//
// Install the default resources for this class.
//
void InsertNetworkDialog::installDefaultResources(Widget  baseWidget)
{
    this->setDefaultResources(baseWidget,InsertNetworkDialog::DefaultResources);
    this->OpenNetworkDialog::installDefaultResources( baseWidget);
}



//
// Create a temporary network, read the new file into it, then
// merge the new network into the existing net.  It's just like
// a drag-n-drop operation in the vpe.
//
void InsertNetworkDialog::okFileWork(const char *string)
{
int x1,y1, x2,y2;
Network *net = theDXApplication->network;

    // ...could happen if someone deleted all the text in the text widget.
    if ((!string)||(!string[0])) return ;

    // dnd doesn't call terminateExecution... Should we?
    // theDXApplication->getExecCtl()->terminateExecution();

    Network *tmpnet = theDXApplication->newNetwork(TRUE);
    ASSERT (tmpnet);

    // Do we want the .cfg file if there is one?  ...readNetwork automatically
    // looks for one.
    // If the readNetwork operation fails, someone else will call WarningMessage
    if (!tmpnet->readNetwork (string, NULL, TRUE)) {
	delete tmpnet;
	return ;
    }

    // find the most easterly and/or most southerly.  Then set x,y so 
    // that the new net doesn't plop down on top of the old one.
    // If it were known how much space the new chunk required, then
    // possibly we could place it above or to the left of the existing net.
    // Maybe, deselect all old nodes, and select all nodes in the
    // temporary net.  That way, the new nodes are set up to be moved to
    // a better location.
    net->getBoundingBox (&x1, &y1, &x2, &y2);
    if (x2 > y2) 
	tmpnet->setTopLeftPos ((x2=0),y2+5);
    else 
	tmpnet->setTopLeftPos (x2+5,(y2=0));

    List *l = tmpnet->getNonEmptyPanels();
    if (net->mergeNetworks(tmpnet, l, TRUE)) {
	EditorWindow *ed = net->getEditor();
	// x2,y2 is probably a little off, but moveWorkspaceWindow
	// will perform error checking.  It would be uncivilized to examine
	// scrollbar values right here.
	ed->moveWorkspaceWindow (x2,y2,FALSE); 
    }

    if (l) delete l;
    delete tmpnet;
}





