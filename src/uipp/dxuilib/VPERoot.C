/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>




#include "SymbolManager.h"
#include "VPERoot.h"
#include "VPEPage.h"
#include "PageSelector.h"
#include "EditorWindow.h"
#include "UndoGrid.h"

VPERoot::~VPERoot()
{
    if (this->show_wpid) XtRemoveWorkProc(this->show_wpid);
    if (this->move_wpid) XtRemoveWorkProc(this->move_wpid);
}

WorkSpace*
VPERoot::newPage (int width, int height)
{

    Symbol s = theSymbolManager->registerSymbol(ClassVPERoot);
    ASSERT(this->isA(s));

    WorkSpaceInfo *wsi = new WorkSpaceInfo;
    EditorWorkSpace *ews = new VPEPage("vpeCanvas",
	 this->getRootWidget(), wsi, this->editor, this);
		  
    ews->setWidth(width);
    ews->setHeight(height);
    ews->initializeRootWidget();
    return ews;
}

//
// undo processing:  If we're changing gridding, then checkpoint the
// grid options and the location of every standIn (not just the ones
// in the current page).
//
void VPERoot::installInfo (WorkSpaceInfo* info) 
{

    if ((info) || (info==this->getInfo())) {
	//
	// This case is not called from GridDialog::ok() so
	// we don't need to provide any undo processing
	//
	this->EditorWorkSpace::installInfo(info);
	this->WorkSpaceRoot::installPageInfo(info);
	return ;
    }

    UndoGrid* undoGrid = new UndoGrid(this->editor, this);
    this->editor->saveAllLocationsForUndo(undoGrid);

    this->EditorWorkSpace::installInfo(info);
    this->WorkSpaceRoot::installPageInfo(info);
}

void VPERoot::showWorkSpace (int page)
{
    //
    // Store current scroll loc for  current page.
    //
    int x,y;
    this->editor->getWorkspaceWindowPos (&x,&y);
    int pno = this->getCurrentPage();
    if (pno != page) {
	if (pno > 0) {
	    EditorWorkSpace* ews = (EditorWorkSpace*)this->getElement(pno);
	    ews->setRecordedScrollPos(x,y);
	} else {
	    this->setRecordedScrollPos(x,y);
	}
    }

    //
    // By toggling XmNmappedWhenManaged, we can give the scrollbars an opportunity
    // to move and reposition the selected canvas with the contents of the canvas
    // hidden.  This way the user doesn't see the canvas repositioning.  It just
    // appears at the proper location.
    //
    boolean reset_scrollbars = FALSE;
    this->to_be_shown = this;
    if (page) 
	this->to_be_shown = (EditorWorkSpace*)this->getElement(page);
    reset_scrollbars = this->to_be_shown->hasScrollBarPositions();
    if (reset_scrollbars) 
	XtVaSetValues (this->getRootWidget(), XmNmappedWhenManaged, False, NULL);

    this->WorkSpaceRoot::showWorkSpace(page);
    XtAppContext apcxt = theApplication->getApplicationContext();
    XSync (XtDisplay(this->getRootWidget()), False);

    this->selector->selectPage(this->to_be_shown);
    if (reset_scrollbars) {
	if (this->show_wpid)
	    XtRemoveWorkProc(this->show_wpid);
	if (this->move_wpid)
	    XtRemoveWorkProc(this->move_wpid);
	this->show_wpid = XtAppAddWorkProc (apcxt, VPERoot_ShowWindowWP, (XtPointer)this);
	this->move_wpid = XtAppAddWorkProc (apcxt, VPERoot_MoveWindowWP, (XtPointer)this);
    }
}

extern "C" Boolean VPERoot_MoveWindowWP (XtPointer clientData)
{
    VPERoot* vper = (VPERoot*)clientData;
    ASSERT(vper);
    vper->to_be_shown->restorePosition();
    vper->move_wpid = NUL(XtWorkProcId);
    return True;
}

extern "C" Boolean VPERoot_ShowWindowWP (XtPointer clientData)
{
    VPERoot* vper = (VPERoot*)clientData;
    ASSERT(vper);
    XtVaSetValues (vper->getRootWidget(), XmNmappedWhenManaged, True, NULL);
    vper->show_wpid = NUL(XtWorkProcId);
    return True;
}

boolean VPERoot::isA (Symbol classname)
{
    Symbol s = theSymbolManager->registerSymbol(ClassVPERoot);
    return (s == classname);
}

boolean VPERoot::isA (const char* classname)
{
    Symbol s = theSymbolManager->registerSymbol(classname);
    return this->isA(s);
}
