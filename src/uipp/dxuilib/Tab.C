/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include <defines.h>




#include <Xm/Xm.h>
#include <Xm/PushB.h>

#include "DXStrings.h"
#include "Tab.h"
#include "Application.h"
#include "StandIn.h"

Tab::Tab(StandIn *standIn):
	UIComponent("tab")
{
    this->standIn = standIn;

    this->lineWidget = NULL;

    x = y = 0; 
}

static XmString nullXmString = NULL;

void Tab::createTab(Widget parent, boolean createManaged)
{
    Arg      arg[10];
    int      n = 0;
    Widget w;

    if (!nullXmString)
        nullXmString = XmStringCreateLtoR("", "canvas");

    XtSetArg(arg[n], XmNlabelString, nullXmString);		n++;
    XtSetArg(arg[n], XmNwidth,       standIn->getIOWidth());	n++;
    XtSetArg(arg[n], XmNheight,      standIn->getIOHeight());	n++;
    XtSetArg(arg[n], XmNborderWidth, 0);			n++;
    XtSetArg(arg[n], XmNfillOnArm,   FALSE);			n++;

    this->x = 0;
    this->y = 0;

    if (createManaged) 
	w = XtCreateManagedWidget ("nodeTab",
                 xmPushButtonWidgetClass,
                 parent,
                 arg,
                 n);
    else
	w = XtCreateWidget ("nodeTab",
                 xmPushButtonWidgetClass,
                 parent,
                 arg,
                 n);

    this->setRootWidget(w);
}

void Tab::setBackground(Pixel  background)
{
    /*
     * If the tab has not been initialized...
     */
    if(!this->getRootWidget()) return;

    /*
     * If the background value is invalid, go no further...
     */
    if (background == ((Pixel) -1))
        return;

    /*
     * Set the widget to the new background color.
     * Set the armColor also because Motif does strange drawing if the
     * colormap fills up.
     */
    XtVaSetValues(this->getRootWidget(), 
	XmNbackground, background, 
	XmNarmColor, background, 
    NULL);
}
void Tab::moveTabX(Position x, boolean update)
{

    /*
     * If the tab has not been initialized...
     */
    if(!this->getRootWidget()) return;

    XtVaSetValues(this->getRootWidget(), XmNx, x, NULL);

    if(update)
	this->x = x;
}
void Tab::moveTabY(Position y, boolean update)
{
    /*
     * If the tab has not been initialized...
     */
    if(!this->getRootWidget()) return;

    XtVaSetValues(this->getRootWidget(), XmNy, y, NULL);

    if(update)
	this->y = y;
}
void Tab::moveTabXY(Position x, Position y, boolean update)
{
    /*
     * If the tab has not been initialized...
     */
    if(!this->getRootWidget()) return;

    XtVaSetValues(this->getRootWidget(), XmNx, x, XmNy, y, NULL);

    if(update){
	this->x = x;
	this->y = y;
    }
}

Tab::~Tab() 
{
    if (this->lineWidget != NULL)
	XtDestroyWidget(this->lineWidget);
}

