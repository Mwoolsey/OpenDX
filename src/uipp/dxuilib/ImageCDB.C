/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#include "ImageCDB.h"


#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/Label.h>
#include <Xm/PushB.h>
#include <Xm/RowColumn.h>

#include "Application.h"
#include "ImageNode.h"
#include "ImageDefinition.h"

boolean ImageCDB::ClassInitialized = FALSE;
String ImageCDB::DefaultResources[] =
{

    "*imageCacheLabel.labelString:      Internal Caching",
    "*imageCacheLabel.foreground:       SteelBlue",
    "*cacheFull.labelString:            All Results",
    "*cacheLast.labelString:            Last Result",
    "*cacheOff.labelString:             No Results",

#if defined(aviion)
    "*imageCacheOM.labelString:",
#endif

    NULL
};

ConfigurationDialog*
ImageCDB::AllocateConfigurationDialog(Widget parent, Node *node)
{
    return new ImageCDB(parent, node);
}

ImageCDB::ImageCDB( Widget parent, Node *node):
    ConfigurationDialog("imageConfigurationDialog", parent, node)
{
    //
    // Install the default resources for THIS class (not the derived classes)
    //
    if (NOT ImageCDB::ClassInitialized)
    {
	ASSERT(theApplication);
        ImageCDB::ClassInitialized = TRUE;
	this->installDefaultResources(theApplication->getRootWidget());
    }
    this->imageCacheOM = NUL(Widget);
    this->imageCacheLabel = NUL(Widget);
    this->internalFullButton = NUL(Widget);
    this->internalLastButton = NUL(Widget);
    this->internalOffButton = NUL(Widget);
}

ImageCDB::~ImageCDB()
{
}


//
// Install the default resources for this class.
//
void ImageCDB::installDefaultResources(Widget baseWidget)
{
    this->setDefaultResources(baseWidget, ImageCDB::DefaultResources);
    this->ConfigurationDialog::installDefaultResources(baseWidget);
}

Widget ImageCDB::createOutputs (Widget parent, Widget top)
{
Widget but, form_parent;
Arg args[10];
int n;

    //
    // the widget we'll use as parent isn't stored in the class so this
    // could cause trouble in case the c.d.b is changed much.
    //
    Widget retVal = this->ConfigurationDialog::createOutputs(parent, top);
    form_parent = XtParent(this->outputCacheLabel);

    //
    // Create a single extra label / option menu (with hardcoded options inside) combo.
    //
    n = 0;
    XtSetArg (args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
    XtSetArg (args[n], XmNleftOffset, 15); n++;
    XtSetArg (args[n], XmNleftWidget, this->outputCacheLabel); n++;
    XtSetArg (args[n], XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
    XtSetArg (args[n], XmNtopWidget, this->outputCacheLabel); n++;
    XtSetArg (args[n], XmNalignment, XmALIGNMENT_BEGINNING); n++;
    this->imageCacheLabel =
	XmCreateLabel (form_parent, "imageCacheLabel", args, n);
    XtManageChild (this->imageCacheLabel);


    //
    // Create the option menu
    //
    n = 0;
    Widget menu_pane = XmCreatePulldownMenu (form_parent, "image_pane", args, n);

    n = 0;
    but = this->internalFullButton = XmCreatePushButton (menu_pane, "cacheFull", args, n);
    XtManageChild (but);
    XtAddCallback (but, XmNactivateCallback, 
	(XtCallbackProc)ImageCDB_ActivateOutputCacheCB, (XtPointer)this);
    n = 0;
    but = this->internalLastButton = XmCreatePushButton (menu_pane, "cacheLast", args, n);
    XtManageChild (but);
    XtAddCallback (but, XmNactivateCallback, 
	(XtCallbackProc)ImageCDB_ActivateOutputCacheCB, (XtPointer)this);
    n = 0;
    but = this->internalOffButton = XmCreatePushButton (menu_pane, "cacheOff", args, n);
    XtManageChild (but);
    XtAddCallback (but, XmNactivateCallback, 
	(XtCallbackProc)ImageCDB_ActivateOutputCacheCB, (XtPointer)this);

    n = 0;
    XtSetArg (args[n], XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
    XtSetArg (args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
    XtSetArg (args[n], XmNtopWidget, this->imageCacheLabel); n++;
    XtSetArg (args[n], XmNleftWidget, this->imageCacheLabel); n++;
    XtSetArg (args[n], XmNleftOffset, -10); n++;
    XtSetArg (args[n], XmNsubMenuId, menu_pane); n++;

    ImageNode *inode = (ImageNode*)this->node;
    switch (inode->getInternalCacheability()) {
	case InternalsNotCached:
	    XtSetArg (args[n], XmNmenuHistory, this->internalOffButton); n++;
	    break;
	case InternalsFullyCached:
	    XtSetArg (args[n], XmNmenuHistory, this->internalFullButton); n++;
	    break;
	case InternalsCacheOnce:
	    XtSetArg (args[n], XmNmenuHistory, this->internalLastButton); n++;
	    break;
	default:
	    break;
    }

    this->imageCacheOM = 
	XmCreateOptionMenu (form_parent, "imageCacheOM", args, n);
    XtManageChild (this->imageCacheOM);

    return retVal;
}


//
// There is just this one callback for each of the 3 buttons in the option menu.
//
extern "C" void ImageCDB_ActivateOutputCacheCB(Widget widget,
					      XtPointer clientData,
					      XtPointer)
{
    ImageCDB *dialog = (ImageCDB *)clientData;
    ImageNode *inode = (ImageNode*)dialog->node;

    if (widget == dialog->internalFullButton) {
	inode->setInternalCacheability (InternalsFullyCached);
    } else if (widget == dialog->internalLastButton) {
	inode->setInternalCacheability (InternalsCacheOnce);
    } else if (widget == dialog->internalOffButton) {
	inode->setInternalCacheability (InternalsNotCached);
    } else ASSERT(0);
}

//
// This will be called too often, but there is no mechanism in place as far as I
// know, for setting output values once at the module level for the c.d.b.
//
void ImageCDB::changeOutput(int i)
{
Widget butn;
ImageNode *inode = (ImageNode*)this->node;

    this->ConfigurationDialog::changeOutput(i);

    //
    // Wow, we can just do nothing if the widgets haven't been created yet?
    //
    if (!this->internalFullButton) return ;

    switch (inode->getInternalCacheability()) {
	case InternalsFullyCached:
	    butn = this->internalFullButton;
	    break;
	case InternalsCacheOnce:
	    butn = this->internalLastButton;
	    break;
	case InternalsNotCached:
	    butn = this->internalOffButton;
	    break;
	default:
	    butn = NUL(Widget);
	    break;
    }
    ASSERT(butn);
    XtVaSetValues (this->imageCacheOM, XmNmenuHistory, butn, NULL);
}
