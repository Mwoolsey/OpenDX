/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#include "PixelImageFormat.h"
#include "ImageNode.h"
#include "ImageFormatDialog.h"
#include "Application.h"
#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/Text.h>
#include "SymbolManager.h"
#include "../widgets/Number.h"

String PixelImageFormat::DefaultResources[] = {
    "*sizeLabel.labelString:		Image Size:",
    NUL(char*)
};


PixelImageFormat::PixelImageFormat (const char *name, ImageFormatDialog *dialog) : 
    ImageFormat(name, dialog)
{
    this->dirty = 0;
    this->size_val = NUL(char*);
    this->size_text = NUL(Widget);
    this->use_nodes_resolution = TRUE;
    this->use_nodes_aspect = TRUE;
    this->size_timer = 0;
}

PixelImageFormat::~PixelImageFormat()
{
    if (this->size_val) delete this->size_val;
    if (this->size_timer) XtRemoveTimeOut (this->size_timer);
}

Widget PixelImageFormat::createBody (Widget parent)
{
    this->initialize();

    //
    // Widgets for typing in the widthXheight of the image
    //
    Widget body = XtVaCreateManagedWidget (this->name,
	xmFormWidgetClass,	parent,
	XmNmappedWhenManaged,	False,
	XmNleftAttachment,	XmATTACH_FORM,
	XmNleftOffset,		2,
	XmNrightAttachment,	XmATTACH_FORM,
	XmNrightOffset,		2,
    NULL);

    Widget lab = XtVaCreateManagedWidget ("sizeLabel",
	xmLabelWidgetClass,	body,
	XmNtopAttachment,	XmATTACH_FORM,
	XmNleftAttachment,	XmATTACH_FORM,
	XmNleftOffset,		2,
	XmNtopOffset,		7,
    NULL);

    this->size_text = XtVaCreateManagedWidget ("sizeText",
	xmTextWidgetClass,	body,
	XmNtopAttachment,	XmATTACH_FORM,
	XmNtopOffset,		2,
	XmNleftAttachment,	XmATTACH_WIDGET,
	XmNleftWidget,		lab,
	XmNleftOffset,		2,
	XmNcolumns,		10,
    NULL);	
    XtAddCallback (this->size_text, XmNactivateCallback, (XtCallbackProc)
	PixelImageFormat_ParseSizeCB, (XtPointer)this);
    XtAddCallback (this->size_text, XmNmodifyVerifyCallback, (XtCallbackProc)
	PixelImageFormat_ModifyCB, (XtPointer)this);
    XtAddEventHandler (this->size_text, LeaveWindowMask, False,
	(XtEventHandler)PixelImageFormat_ParseSizeEH, (XtPointer)this);

    this->setRootWidget(body);
    return body;
}

boolean PixelImageFormat::useLocalResolution()
{
ImageNode *node = this->dialog->getNode();
    boolean rescon = node->isRecordResolutionConnected();
    boolean rerender = this->dialog->isRerenderAllowed();

    if ((rescon) || (!rerender)) return FALSE;
    return ((this->dirty & PixelImageFormat::DirtyResolution)?TRUE:FALSE) ;
}

boolean PixelImageFormat::useLocalAspect()
{
ImageNode *node = this->dialog->getNode();
    boolean aspcon = node->isRecordAspectConnected();
    boolean rerender = this->dialog->isRerenderAllowed();

    if ((aspcon) || (!rerender)) return FALSE;
    return ((this->dirty & PixelImageFormat::DirtyAspect)?TRUE:FALSE) ;
}

void PixelImageFormat::setCommandActivation()
{
int x,y,height;
double hd, aspect;
char size_val[64];
ImageNode *node = this->dialog->getNode();

    //
    // Flags which will tell use what to do...
    //
    boolean rescon = node->isRecordResolutionConnected();
    boolean resset = rescon || node->isRecordResolutionSet();
    boolean aspcon = node->isRecordAspectConnected();
    boolean aspset = aspcon || node->isRecordAspectSet();
    boolean dirty_res = (this->dirty & PixelImageFormat::DirtyResolution);
    boolean dirty_asp = (this->dirty & PixelImageFormat::DirtyAspect);
    boolean rerender = this->dialog->isRerenderAllowed();

    //
    // What are the node's values?
    //
    boolean resfetched = FALSE;
    boolean aspfetched = FALSE;
    if (rescon) {
	node->getRecordResolution(x,y);
	resfetched = TRUE;
    } 
    if (aspcon) {
	node->getRecordAspect(aspect);
	aspfetched = TRUE;
    } 
    if ((rerender) && (!resfetched) && (resset)) {
	node->getRecordResolution(x,y);
	resfetched = TRUE;
    }
    if ((rerender) && (!aspfetched) && (aspset)) {
	node->getRecordAspect(aspect);
	aspfetched = TRUE;
    }
    if (!resfetched) 
	node->getResolution(x,y);
    if (!aspfetched)
	node->getAspect(aspect);

    //
    // Do we use the node's value or our own?
    //
    this->use_nodes_resolution = (rescon || !rerender || (resset && !dirty_res));
    this->use_nodes_aspect = (aspcon || !rerender || (aspset && !dirty_asp));

    //
    // If we know we're using the node's value, then disable the text widget so
    // that the user sees she can't enter her own value.
    //
    if (this->size_text) {
	if (((rescon) && (aspcon)) || (!rerender))
	    this->setTextSensitive (this->size_text, False);
	else
	    this->setTextSensitive (this->size_text, True);
    }

    //
    // Based on what we already know, lets put a value into the text widget.
    //
    if ((this->use_nodes_resolution) && (this->use_nodes_aspect)) {
	this->aspect = aspect;
	this->width = x;
	hd = (double)this->width * this->aspect;
	this->dirty = 0;


    //
    // Only use the node's resolution value, use a stored 
    // aspect to compute the height.
    //
    } else if (this->use_nodes_resolution) {
	this->width = x;
	hd = (double)this->width * this->aspect;
	this->dirty&= ~PixelImageFormat::DirtyResolution;


    //
    // Use the node's apsect value.  Use a stored resolution value to
    // compute the height.
    //
    } else if (this->use_nodes_aspect) {
	this->aspect = aspect;
	hd = (double)this->width * this->aspect;
	this->dirty&= ~PixelImageFormat::DirtyAspect;


    //
    // Use only the values store here.
    //
    } else {
	hd = (double)this->width * this->aspect;
    }

    //
    // Round the float value for height and put the result into
    // the text widget.
    //
    height = (int)hd;
    if ((hd - height) >= 0.5) height++;
    sprintf (size_val, "%dx%d", this->width, height);
    if (this->size_val) delete this->size_val;
    this->size_val = DuplicateString(size_val);
    if (this->size_text) {
	if (this->size_timer)
	    XtRemoveTimeOut (this->size_timer);
	this->size_timer = 0;
	XtRemoveCallback (this->size_text, XmNmodifyVerifyCallback,
	    (XtCallbackProc)PixelImageFormat_ModifyCB, (XtPointer)this);
	XmTextSetString (this->size_text, this->size_val);
	XtAddCallback (this->size_text, XmNmodifyVerifyCallback,
	    (XtCallbackProc)PixelImageFormat_ModifyCB, (XtPointer)this);
    }
}

void PixelImageFormat::parseImageSize(const char *str)
{
    if ((!str) || (!str[0])) return ;

    char *dimstr = DuplicateString(str);

    boolean width_parsed = FALSE;
    boolean height_parsed = FALSE;
    int width, height;

    char *cp = strchr(dimstr, (int)'x');
    if (!cp) cp = strchr(dimstr, (int)'X');
    if (cp) {
	*cp = '\0';
    }
    if ((!cp) || (cp != dimstr)) {
	int items_parsed = sscanf (dimstr, "%d", &width);
	if (items_parsed == 1) width_parsed = TRUE;
    }
    delete dimstr;


    dimstr = DuplicateString(str);
    cp = strchr(dimstr, (int)'x');
    if (!cp) cp = strchr(dimstr, (int)'X');
    if (cp) {
	cp++;
	if (cp[0]) {
	    int items_parsed = sscanf (cp, "%d", &height);
	    if (items_parsed == 1) height_parsed = TRUE;
	}
    }
    delete dimstr;

    if ((width_parsed) && (height_parsed)) {
	double wd = width;
	double hd = height;
	this->width = width;
	this->aspect = hd / wd;
	this->dirty|= PixelImageFormat::DirtyResolution;
	this->dirty|= PixelImageFormat::DirtyAspect;


    } else if (width_parsed) {
	ImageNode *node = this->dialog->getNode();
	this->width = width;
	node->getAspect(this->aspect);
	this->dirty|= PixelImageFormat::DirtyResolution;
	this->dirty&= ~PixelImageFormat::DirtyAspect;

    } else if (height_parsed) {
	ImageNode *node = this->dialog->getNode();
	node->getAspect(this->aspect);
	double hd = height;
	double wd = hd / this->aspect;
	this->width = (int)wd;
	this->dirty|= PixelImageFormat::DirtyResolution;
	this->dirty&= ~PixelImageFormat::DirtyAspect;

    } else {
	// The user can't type very well.
	int y;
	ImageNode *node = this->dialog->getNode();
	node->getResolution (this->width,y);
	node->getAspect(this->aspect);
	this->dirty&= ~PixelImageFormat::DirtyResolution;
	this->dirty&= ~PixelImageFormat::DirtyAspect;
    }
}

void PixelImageFormat::shareSettings (ImageFormat *imgfmt)
{
    if (imgfmt->useLocalResolution()) {
	this->width = imgfmt->getRecordResolution();
	this->dirty|= PixelImageFormat::DirtyResolution;
    }
	 
    if (imgfmt->useLocalAspect()) {
	this->aspect = imgfmt->getRecordAspect();
	this->dirty|= PixelImageFormat::DirtyAspect;
    }
      
    if (imgfmt->useLocalFormat()) {
	const char *cp = imgfmt->getRecordFormat();
	this->parseRecordFormat(cp);
    }
}

boolean PixelImageFormat::isA(Symbol classname)
{
    Symbol s = theSymbolManager->registerSymbol(ClassPixelImageFormat);
    if (s == classname)
	return TRUE;
    else
	return this->ImageFormat::isA(classname);
}

extern "C" {

void PixelImageFormat_ModifyCB (Widget , XtPointer clientData, XtPointer)
{
    PixelImageFormat* pif = (PixelImageFormat*)clientData;
    ASSERT(pif);
    if (pif->size_timer)
	XtRemoveTimeOut (pif->size_timer);
    pif->size_timer = 0;
    pif->size_timer = XtAppAddTimeOut (theApplication->getApplicationContext(),
	5000, (XtTimerCallbackProc)PixelImageFormat_SizeTO, (XtPointer)pif);
}

void PixelImageFormat_ParseSizeCB (Widget w, XtPointer clientData, XtPointer)
{
    PixelImageFormat* pif = (PixelImageFormat*)clientData;
    ASSERT(pif);
    if (pif->size_timer)
	XtRemoveTimeOut (pif->size_timer);
    pif->size_timer = 0;
    char *str = XmTextGetString(w);
    pif->parseImageSize (str);
    pif->setCommandActivation();
    if (str) XtFree(str);
}

void PixelImageFormat_SizeTO (XtPointer clientData, XtIntervalId* )
{
    PixelImageFormat* pif = (PixelImageFormat*)clientData;
    ASSERT(pif);
    char *str = XmTextGetString(pif->size_text);
    pif->size_timer = 0;
    pif->parseImageSize (str);
    pif->setCommandActivation();
    if (str) XtFree(str);
}

void PixelImageFormat_ParseSizeEH (Widget w, XtPointer clientData, XEvent*, Boolean*)
{
    PixelImageFormat* pif = (PixelImageFormat*)clientData;
    ASSERT(pif);
    if (!pif->size_timer) return;
    PixelImageFormat_ParseSizeCB(w, clientData, (XtPointer)NULL);
}


} // end extern C


