/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#include <Xm/Xm.h>
#include <Xm/ToggleB.h>


#include "Application.h"
#include "ToggleInteractor.h"
#include "InteractorInstance.h"
#include "InteractorStyle.h"

#if defined(aviion) || defined(alphax)
#define XMSTRING_COMPARE_IS_BROKEN
#endif


String ToggleInteractor::DefaultResources[] =  {
    "*recomputeSize: 			True",
    "*toggleButton.shadowThickness: 	0",
    "*toggleButton.selectColor: 	White",
    "*toggleButton.topAttachment:     XmATTACH_FORM",
    "*toggleButton.leftAttachment:    XmATTACH_FORM",
    "*toggleButton.rightAttachment:   XmATTACH_FORM",
    "*toggleButton.bottomAttachment:  XmATTACH_FORM",
    "*toggleButton.topOffset:         2",
    "*toggleButton.leftOffset:        2",
    "*toggleButton.rightOffset:       0",
    "*toggleButton.bottomOffset:      2",
    "*allowHorizontalResizing:		True",

	NUL(char*) 
};

//
// Splain:
// Normally WorkSpaceComponent takes care of this stuff, but ToggleInteractors
// are different because they get rid of the interactor label which means there
// is no place left to click.  The following translations connect the various
// button clicks on workspace widget children including drag-n-drop.  
// I would rather put this stuff into DefaultResources but I could never find
// the proper combination of line termination symbols.
//
String ToggleInteractor::WWTable = "\
Shift<Btn1Down>:        	select_w()\n\
Ctrl<Btn1Down>:         	select_w()\n\
<Btn1Down>,<Btn1Up>:    	Arm() Select() Disarm()\n\
<Btn1Down>,<Btn1Motion>:	select_w()\n\
<Btn1Up>(2+):           	select_w() release_w() select_w() release_w()\n\
<Btn1Down>:             	Arm()\n\
<Btn1Up>:               	Select() Disarm() release_w()\n\
<Btn1Motion>:           	move_w()\n\
~Shift<Btn2Down>:       	select_w() DragSource_StartDrag()\n\
Shift<Btn2Down>:        	DragSource_StartDrag()\n\
<Btn2Up>:               	release_w()\n\
";

XtTranslations ToggleInteractor::WWTranslations = 0;

ToggleInteractor::ToggleInteractor(const char *name,
		 InteractorInstance *ii) : Interactor(name,ii)
{
    this->toggleButton = NULL;
}

//
// Accepts value changes and reflects them into other interactors, cdbs
// and off course the interactor node output.
//
extern "C" void ToggleInteractor_ToggleCB(Widget                  widget,
			        XtPointer               clientData,
			        XtPointer               callData)
{
    ToggleInteractor *ti = (ToggleInteractor*)clientData;
    ASSERT(widget);
    ASSERT(ti);

    Boolean setting;
    XtVaGetValues(widget, XmNset, &setting, NULL);

    ti->toggleCallback(widget, setting, callData);
}

//
// Build the toggle button and install the valueChange callback for it. 
//
Widget ToggleInteractor::createInteractivePart(Widget form)
{
    Arg			wargs[20];
    int			n;

    n = 0;
    this->toggleButton = XtCreateManagedWidget("toggleButton",
                                        xmToggleButtonWidgetClass, 
					form, wargs, n);
    XtAddCallback(this->toggleButton,
			XmNvalueChangedCallback, 
			(XtCallbackProc)ToggleInteractor_ToggleCB,
			(XtPointer)this);

    XtManageChild(this->toggleButton);
    return this->toggleButton;
}




void ToggleInteractor::completeInteractivePart()
{
InteractorInstance *ii = this->interactorInstance;

    this->setLabel (ii->getInteractorLabel(), True);
    this->Interactor::setLabel("",FALSE);

    //
    // ::passEvents is not applied to the widget(s) in the interactor
    // which do the actual work i.e. the pushbuttons or togglebuttons.
    // The problem here is that there is no place on a toggle button to
    // click to do defaultAction because the border is so thin and there
    // is no label showing expect that which belongs to the toggle button.
    // So apply a translation to the togglebutton which sends double clicks
    // on the the parent.
    //
    if (!ToggleInteractor::WWTranslations) {
	ToggleInteractor::WWTranslations = XtParseTranslationTable(WWTable);
    }
    this->setDragWidget (this->toggleButton);
    ASSERT(ToggleInteractor::WWTranslations);
    XtOverrideTranslations (this->toggleButton, ToggleInteractor::WWTranslations);
}
void ToggleInteractor::handleInteractivePartStateChange(InteractorInstance *ii,
						boolean major_change)
{
    this->updateDisplayedInteractorValue();
}
//
// Set the displayed label of the interactor and do a layout if
// indicated to handle a significant change in label width.
// We override the default to always set the toggle label instead
// and set the interactor label to "".
//
void ToggleInteractor::setLabel(
        const char *labelString, boolean re_layout)
{
XmStringContext cxt;
char *text, *tag;
XmStringDirection dir;
Boolean sep;
unsigned char rp;
int i, linesNew, linesOld;
XmString oldXmStr = NULL;

    if(NOT this->toggleButton)
        return;

    linesNew = linesOld = 1;
    XtVaGetValues (this->toggleButton, XmNlabelString, &oldXmStr, NULL);
    ASSERT(oldXmStr);

    char *filtered = WorkSpaceComponent::FilterLabelString (labelString);
    for (i=0; filtered[i]!='\0'; i++) if (filtered[i]=='\n') linesNew++;

#if 0
    //
    // FIXME
    // This piece of code should work, but it doesn't.  The XmStringCompare always
    // returns false.  The same code does work in Interactor::setLabel so I guess
    // something happens to an XmNlabelString setting inside a toggle button.
    // Way to go OSF.
    //
#if !defined(XMSTRING_COMPARE_IS_BROKEN)
    // For efficiency only
    XmString tmpXmStr = XmStringCreateLtoR (filtered, "canvas");
    if (XmStringCompare (tmpXmStr, oldXmStr)) return ;
    XmStringFree (tmpXmStr);
#endif
#endif
 

    if (XmStringInitContext (&cxt, oldXmStr)) {
        while (XmStringGetNextSegment (cxt, &text, &tag, &dir, &sep)) {
            if (sep) linesOld++;
            XtFree(text);
	    if (tag) XtFree(tag);
        }
        XmStringFreeContext(cxt);
    }
    XmStringFree(oldXmStr);


    // toggle XmNresizePolicy so that the interactor doesn't snap back
    // to a reasonable size.  If the old string is taller/shorter than the new string
    // then let it change.

    XtVaGetValues (this->getRootWidget(), XmNresizePolicy, &rp, NULL);
    if (linesNew == linesOld)
        XtVaSetValues (this->getRootWidget(), XmNresizePolicy, XmRESIZE_GROW, NULL);
    else
        XtVaSetValues (this->getRootWidget(), XmNresizePolicy, XmRESIZE_ANY, NULL);

    //this->WorkSpaceComponent::setLabelResource(this->toggleButton, labelString);
    XmString xmstr = XmStringCreateLtoR (filtered, "canvas");
    XtVaSetValues (this->toggleButton, XmNlabelString, xmstr, NULL);
    XmStringFree(xmstr);
    delete filtered;

    //
    // Re-layout the interactor in case the width has changed significantly
    //
    if (re_layout)
        this->layoutInteractor();

    XtVaSetValues (this->getRootWidget(), XmNresizePolicy, rp, NULL);
}


// Don't do the normal things because there is no way
// to change the layout of a toggle button.
void ToggleInteractor::layoutInteractorVertically() 
{ this->Interactor::layoutInteractorVertically(); }
void ToggleInteractor::layoutInteractorHorizontally() 
{ this->Interactor::layoutInteractorVertically(); }

