/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"

#include <X11/keysym.h>
#define XK_MISCELLANY
#include <X11/keysym.h>

#include "EditorToolSelector.h"
#include "EditorWindow.h"
#include "EditorWorkSpace.h"
#include "WarningDialogManager.h"
#include "DXApplication.h"
#include "DXStrings.h"
#include "List.h"
#include "ListIterator.h"
#include "ResourceManager.h"
#include "Notebook.h"
#include <Xm/Form.h>
#include <Xm/Frame.h>

EditorToolSelector::EditorToolSelector(const char *name, EditorWindow *editor) :
    ToolSelector(name)
{
    this->editor = editor;
    this->notebook = NUL(Notebook*);
}

EditorToolSelector::~EditorToolSelector()
{
}

boolean EditorToolSelector::inAnchor() 
{
    return this->editor->isAnchor();
}
void EditorToolSelector::toolSelect(Symbol s)
{
    
    this->ToolSelector::toolSelect(s);

    if (this->getCurrentSelection())
        this->editor->setCursor(UPPER_LEFT);
    else
        this->editor->resetCursor();
}

void EditorToolSelector::deselectAllTools()
{
    this->ToolSelector::deselectAllTools();
    this->editor->resetCursor();
}

Widget EditorToolSelector::layoutWidgets(Widget parent)
{
    Widget form = XtVaCreateManagedWidget("toolSelector", xmFrameWidgetClass, parent,
	XmNshadowThickness, 0,
    NULL);
    this->notebook = new Notebook(form);
    XtVaSetValues (this->notebook->getRootWidget(),
	XmNtopAttachment, XmATTACH_FORM,
	XmNleftAttachment, XmATTACH_FORM,
	XmNrightAttachment, XmATTACH_FORM,
	XmNbottomAttachment, XmATTACH_FORM,
	XmNtopOffset, 0,
	XmNleftOffset, 0,
	XmNrightOffset, 0,
	XmNbottomOffset, 0,
    NULL);

    Widget tools = this->ToolSelector::layoutWidgets (this->notebook->getPageManager());
    this->notebook->addPage ("Tools", tools);

    return form;
}

//
// The Notebook provides a scrolled window.  Ensure that the
// selection identified by the rectangle passed in is visibile.
// To do that request the Notebook to move its scrollbars.
//
void EditorToolSelector::adjustVisibility(int x1, int y1, int x2, int y2)
{
    this->ToolSelector::adjustVisibility(x1,y1,x2,y2);
    this->notebook->showRectangle(x1,y1,x2,y2);
}
