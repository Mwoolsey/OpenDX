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
#include <Xm/Separator.h>

#include "XmUtility.h"
#include "../widgets/WorkspaceW.h"
#include "Application.h"
#include "DXStrings.h"
#include "Dictionary.h"
#include "SeparatorDecorator.h"
#include "../widgets/XmDX.h"
#include "WarningDialogManager.h"
#include "ntractor.bm"
#include "ntractormask.bm"
#include "SetSeparatorAttrDlg.h"
#include "ListIterator.h"

#define WANT_DND_IN_DECORATORS 1

boolean SeparatorDecorator::SeparatorDecoratorClassInitialized = FALSE;

Dictionary* SeparatorDecorator::DragTypeDictionary = new Dictionary;
Widget SeparatorDecorator::DragIcon;

String SeparatorDecorator::DefaultResources[] =
{
  ".decorator.topOffset:      2",
  ".decorator.bottomOffset:   2",
  ".decorator.leftOffset:     2",
  ".decorator.rightOffset:    2",
  NUL(char*)
};


SeparatorDecorator::SeparatorDecorator(boolean developerStyle) : 
	Decorator (xmSeparatorWidgetClass, "SeparatorDecorator", developerStyle)
{
    this->setAttrDialog = NUL(SetSeparatorAttrDlg*);
}

SeparatorDecorator::~SeparatorDecorator()
{
    if (this->setAttrDialog) delete this->setAttrDialog;
}
 
 
void SeparatorDecorator::initialize()
{
    //
    // Initialize default resources (once only).
    //
    if (NOT SeparatorDecorator::SeparatorDecoratorClassInitialized)
    {
	this->setDefaultResources(theApplication->getRootWidget(),
                                        SeparatorDecorator::DefaultResources);
	this->setDefaultResources(theApplication->getRootWidget(),
                                        Decorator::DefaultResources);
	this->setDefaultResources(theApplication->getRootWidget(),
                                        WorkSpaceComponent::DefaultResources);
        SeparatorDecorator::SeparatorDecoratorClassInitialized = TRUE;

#if WANT_DND_IN_DECORATORS
	SeparatorDecorator::DragIcon = this->createDragIcon
	 (ntractor_width,ntractor_height,(char *)ntractor_bits,(char *)ntractormask_bits);
	this->addSupportedType (Decorator::Modules, DXINTERACTORS, TRUE);
	this->addSupportedType (Decorator::Trash, DXTRASH, FALSE);
#endif
    }
}

void SeparatorDecorator::uncreateDecorator()
{
    if (this->setAttrDialog) delete this->setAttrDialog;
    this->setAttrDialog = NULL;
    this->Decorator::uncreateDecorator();
}


//
// This is where we would handle DynamicResources if we had any.
//
void SeparatorDecorator::completeDecorativePart()
{
int w,h;

    this->getXYSize(&w,&h);
    if ((w==0) || (h==0) || (/*for sun4*/(w<3)&&(h<3)))
	this->setXYSize(200, 8);

    this->resizeCB();

#if WANT_DND_IN_DECORATORS
    this->setDragIcon (SeparatorDecorator::DragIcon);
    this->setDragWidget (this->customPart);
#endif
}

void SeparatorDecorator::setVerticalLayout (boolean vertical)
{
int x,y,w,h;
boolean swap_dimensions = FALSE;

    if (!this->customPart) return ;

    if ((vertical) && (this->currentLayout & WorkSpaceComponent::Horizontal)) {
	swap_dimensions = TRUE;
    } else if ((!vertical) && (this->currentLayout & WorkSpaceComponent::Vertical)) {
	swap_dimensions = TRUE;
    }
    if (swap_dimensions) {
	this->getXYSize (&w,&h);
	if (vertical) {
	    this->setXYSize (8,w);
	    XtVaSetValues (this->getRootWidget(), XmNwidth, 0, NULL);
	    XtVaSetValues (this->getRootWidget(), XmNwidth, 8, NULL);
	} else {
	    this->setXYSize (h,8);
	    XtVaSetValues (this->getRootWidget(), XmNheight, 0, NULL);
	    XtVaSetValues (this->getRootWidget(), XmNheight, 8, NULL);
	}
	this->getXYPosition (&x,&y);
	this->setXYPosition (MAX(0,x+(w>>1)-(h>>1)),  MAX(0,y+(h>>1)-(w>>1)));
	this->resizeCB();
    }
    this->WorkSpaceComponent::setVerticalLayout (vertical);
    this->currentLayout&= ~WorkSpaceComponent::NotSet;
}


//
// format:
//  // decorator Separator pos=(%d,%d), size=%dx%d
//
//
boolean SeparatorDecorator::printComment (FILE *f)
{
    if (!this->Decorator::printComment (f)) return FALSE;
    if (fprintf (f, "\n") < 0) 
	return FALSE;
    boolean printOK=TRUE;
    if (this->setResourceList) {
        ListIterator it(*this->setResourceList);
        DynamicResource *dr;
        while ((printOK)&&(dr = (DynamicResource *)it.getNext())) {
            printOK = dr->printComment(f);
        }
    }
 
    return TRUE;
}

boolean SeparatorDecorator::parseComment (const char *comment, const char *f, int l)
{
    return this->Decorator::parseComment(comment, f, l);
}

//
// Redundant info exists so be careful.  this->currentLayout and XmNorientation.
// Should the guy who sets XmNorientation set this->currentLayout (using
// this->setVerticalLayout or not) or should it be the other way around?
// this->setVerticalLayout() calls this->resizeCB.  this->resizeCB is also
// called from other places.
//
void SeparatorDecorator::resizeCB ()
{
Dimension w,h;
Boolean pinSide2Side = False;
Boolean pinTop2Bottom = False;
Boolean allowVertical = False;
Boolean allowHorizontal = False;

    if (!this->customPart) return ;
    XtVaGetValues (this->getRootWidget(), XmNwidth, &w, XmNheight, &h, NULL);
    if (w<h) {
	XtVaSetValues (this->customPart, XmNorientation, XmVERTICAL, NULL);
	pinTop2Bottom = True;
	allowVertical = True;
	this->currentLayout = WorkSpaceComponent::Vertical;
    } else {
	XtVaSetValues (this->customPart, XmNorientation, XmHORIZONTAL, NULL);
	pinSide2Side = True;
	allowHorizontal = True;
	this->currentLayout = WorkSpaceComponent::Horizontal;
    }
    XtVaSetValues (this->getRootWidget(), 
	XmNallowVerticalResizing, allowVertical, 
	XmNallowHorizontalResizing, allowHorizontal, 
	XmNpinLeftRight, pinSide2Side,
	XmNpinTopBottom, pinTop2Bottom, 
    NULL);
    this->WorkSpaceComponent::setVerticalLayout (w<h);
}

Decorator*
SeparatorDecorator::AllocateDecorator (boolean devStyle)
{
    return new SeparatorDecorator (devStyle);
}


boolean
SeparatorDecorator::isA(Symbol classname)
{
    Symbol s = theSymbolManager->registerSymbol(ClassSeparatorDecorator);
    if (s == classname) return TRUE;
    return this->Decorator::isA(classname);
}


boolean SeparatorDecorator::decodeDragType (int tag,
	char * a, XtPointer *value, unsigned long *length, long operation)
{
boolean retVal;

    switch (tag) {
	case Decorator::Modules:
	    retVal = this->convert(this->getNetwork(), a, value, length, operation);
	    break;
 
	case Decorator::Trash:
	    retVal = TRUE;
	    // dummy pointer
	    *value = (XtPointer)malloc(4);
	    *length = 4;
	    break;
 
	default:
	    retVal = FALSE;
	    break;
    }
 
    return retVal;
}

void
SeparatorDecorator::openDefaultWindow()
{
    ASSERT(this->getRootWidget());
    if (!this->setAttrDialog) {
        this->setAttrDialog = 
	    new SetSeparatorAttrDlg (this->getRootWidget(), FALSE, this);
    }
 
    this->setAttrDialog->post();
}

//
// Set {left,right} Offset so that the separator will always cut the
// dialog box is half.
//
void
SeparatorDecorator::setAppearance (boolean developerStyle)
{
int hilite = (developerStyle? Decorator::HiLites : 0);

    if ((this->width > this->height) && (this->customPart))
	XtVaSetValues (this->customPart, 
	    XmNleftOffset,  hilite,
	    XmNrightOffset,  hilite,
	NULL);
    this->Decorator::setAppearance (developerStyle);
}

boolean SeparatorDecorator::printAsJava (FILE* jf, const char* var_name, int instance_no)
{
    const char* indent = "        ";
    int x,y,w,h;
    this->getXYSize(&w,&h);
    this->getXYPosition(&x,&y);

    char svar[128];
    sprintf (svar, "%s_sep_%d", var_name, instance_no);

    fprintf (jf, "\n");
    fprintf (jf, "%sSeparator %s = new Separator();\n", indent, svar);
    if (this->currentLayout & WorkSpaceComponent::Vertical)
	fprintf (jf, "%s%s.setVertical();\n", indent, svar);
    else if (this->currentLayout & WorkSpaceComponent::Horizontal)
	fprintf (jf, "%s%s.setHorizontal();\n", indent, svar);
    else if ((this->currentLayout & WorkSpaceComponent::NotSet) && (w > h)) 
	fprintf (jf, "%s%s.setHorizontal();\n", indent, svar);
    else if ((this->currentLayout & WorkSpaceComponent::NotSet) && (w <= h)) 
	fprintf (jf, "%s%s.setVertical();\n", indent, svar);
	
    if (this->printJavaResources(jf, indent, svar) == FALSE)
	return FALSE;

    fprintf (jf, "%s%s.setBounds(%d,%d,%d,%d);\n", indent,svar, x,y,w,h);
    fprintf (jf, "%s%s.addDecorator(%s);\n", indent, var_name, svar);

    return TRUE;
}

//
// convert line style resource into something java will recognize
//
boolean SeparatorDecorator::printJavaResources
    (FILE* jf, const char* indent, const char* var)
{
    return this->Decorator::printJavaResources (jf, indent, var);
}
