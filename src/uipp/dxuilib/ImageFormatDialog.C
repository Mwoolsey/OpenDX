/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#include "DXStrings.h"
#include "DXApplication.h"
#include "QuestionDialogManager.h"
#include "ImageFormatDialog.h"
#include "ImageFormat.h"
#include "ImageFormatCommand.h"
#include "ImageNode.h"
#include "Command.h"
#include "ListIterator.h"
#include "ToggleButtonInterface.h"
#include "WarningDialogManager.h"

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif
#include <sys/stat.h>
#include <Xm/PushB.h>
#include <Xm/Form.h>
#include <Xm/RowColumn.h>
#include <Xm/Scale.h>
#include <Xm/TextF.h>
#include <Xm/Separator.h>
#include <Xm/SeparatoG.h>
#include <Xm/ToggleB.h>
#include <Xm/Label.h>
#include <X11/StringDefs.h>
#include "../widgets/Number.h"

boolean ImageFormatDialog::ClassInitialized = FALSE;

extern void BuildTheImageFormatDictionary();
extern Dictionary* theImageFormatDictionary;

Cursor ImageFormatDialog::WatchCursor = 0;

String ImageFormatDialog::DefaultResources[] = {
    "*rerenderOption.shadowThickness:	0",
    "*rerenderOption.labelString:	Allow Rerendering",
    "*delayedOption.shadowThickness:	0",
    "*delayedOption.labelString:	Delayed Colors",
    "*applyButton.labelString:		Apply",
    "*cancelButton.labelString:		Close",
    "*restoreButton.labelString:	Restore",
    "*applyButton.width:		70",
    "*cancelButton.width:		70",
    "*restoreButton.width:		70",
    "*formats.labelString:		Format:",
    "*units.labelString:		Units:",
    "*gammaLabel.labelString:          	Gamma Correction:",
    "*accelerators:             	#augment\n"
    "<Key>Return:                   	BulletinBoardReturn()",

    NUL(char*)
};


ImageFormatDialog::ImageFormatDialog (char *name,Widget parent,ImageNode *node, 
    CommandScope* commandScope) : 
	Dialog(name, parent), NoOpCommand(name, theDXApplication->getCommandScope(), TRUE)
{

    this->choice 	= NUL(ImageFormat*);
    this->node 		= node;
    //this->units 	= NUL(Widget);
    this->parent 	= parent;
    this->dirty 	= 0;
    this->busyCursors	= 0;
    this->commandScope 	= commandScope;
    this->resetting	= FALSE;

    this->rerenderOption = NUL(ToggleButtonInterface*);
    this->rerenderCmd = new ImageFormatCommand ("rerenderCmd", this->commandScope,
	TRUE, this, ImageFormatCommand::AllowRerender);

    this->delayedOption = NUL(ToggleButtonInterface*);
    this->delayedCmd = new ImageFormatCommand ("delayedCmd", this->commandScope,
	TRUE, this, ImageFormatCommand::DelayedColors);

    if (!ImageFormatDialog::ClassInitialized) {
	BuildTheImageFormatDictionary();
	ImageFormatDialog::ClassInitialized = TRUE;
    }
    ImageFormatAllocator ifa;
    int size = theImageFormatDictionary->getSize();
    int i;
    this->image_formats = new List;
    for (i=1; i<=size; i++) {
	ifa = (ImageFormatAllocator)theImageFormatDictionary->getDefinition(i);
	ImageFormat *imgfmt = ifa(this);
	this->image_formats->appendElement((void*)imgfmt);
    }
    this->choice = NUL(ImageFormat*);

    theDXApplication->notExecutingCmd->autoActivate(this);
    theDXApplication->executingCmd->autoDeactivate(this);
}

void ImageFormatDialog::installDefaultResources (Widget baseWidget)
{
    this->setDefaultResources (baseWidget, ImageFormatDialog::DefaultResources);
    this->Dialog::installDefaultResources (baseWidget);
}


//
// Unlike other dialog classes in dxui, ImageFormatDialog uses Command and
// CommandInterface objects.  (My way is better.)  The dialog's destructor
// will delete both the CommandInterface (a ToggleButtonInterface in this
// case) and the Command object.  Purify on sgi reports a problem which I
// believe to be originating in the libXm.  I'll delete the CommandInterface
// which is really a widget, and I'll delete the ImageFormatDialog which
// is collection of widgets and normally would result in the destruction
// of the CommandInterface's widget as well.  So, we wind up with an fmr.
// So, in the following destructor, I'll unmanage the CommandInterface first,
// and then delete it.  According to purify that takes care of the problem.
// Strange.  ...and ditto for the ImageFormat objects.
//
ImageFormatDialog::~ImageFormatDialog()
{
    if (this->isManaged())
	this->unmanage();

    if (this->rerenderOption) {
	this->rerenderOption->unmanage();
	delete this->rerenderOption;
    }
    if (this->delayedOption) {
	this->delayedOption->unmanage();
	delete this->delayedOption;
    }

    if (this->rerenderCmd) delete this->rerenderCmd;
    if (this->delayedCmd) delete this->delayedCmd;

    ListIterator it(*this->image_formats);
    ImageFormat *imgfmt;
    while ( (imgfmt = (ImageFormat*)it.getNext()) ) {
	imgfmt->unmanage();
	delete imgfmt;
    }
    delete this->image_formats;
}

boolean ImageFormatDialog::getDelayedColors()
{
    return this->delayedOption->getState();
}

Widget ImageFormatDialog::createDialog(Widget parent)
{
    Arg args[20];
    int n = 0;
    Widget form_diag = this->CreateMainForm (parent, this->UIComponent::name, args, n);

    //
    // topDiagForm will contain filename, image format, allow rerender,
    // units(inch/cm) option menu, button to get a fsdialog
    //
    Widget topDiagForm = XtVaCreateManagedWidget ("diagTop",
	xmFormWidgetClass,	form_diag,
	XmNtopAttachment,	XmATTACH_FORM,
	XmNleftAttachment,	XmATTACH_FORM,
	XmNrightAttachment,	XmATTACH_FORM,
	XmNtopOffset,		2,
	XmNleftOffset,		2,
	XmNrightOffset,		2,
    NULL);

    this->rerenderOption = new ToggleButtonInterface (topDiagForm, "rerenderOption",
	this->rerenderCmd, FALSE);
    XtVaSetValues (this->rerenderOption->getRootWidget(),
	XmNleftAttachment,	XmATTACH_FORM,
	XmNleftOffset,		2,
	XmNtopAttachment,	XmATTACH_FORM,
	XmNtopOffset,		5,
    NULL);

    //
    // D E L A Y E D      C O L O R S             D E L A Y E D      C O L O R S  
    // D E L A Y E D      C O L O R S             D E L A Y E D      C O L O R S  
    // D E L A Y E D      C O L O R S             D E L A Y E D      C O L O R S  
    //
    this->delayedOption = new ToggleButtonInterface (topDiagForm, "delayedOption",
	this->delayedCmd, FALSE);
    XtVaSetValues (this->delayedOption->getRootWidget(),
	XmNleftAttachment,	XmATTACH_FORM,
	XmNleftOffset,		2,
	XmNtopAttachment,	XmATTACH_WIDGET,
	XmNtopWidget,		this->rerenderOption->getRootWidget(),
	XmNtopOffset,		5,
    NULL);


    //
    // G A M M A      G A M M A      G A M M A      G A M M A      G A M M A
    // G A M M A      G A M M A      G A M M A      G A M M A      G A M M A
    // G A M M A      G A M M A      G A M M A      G A M M A      G A M M A
    //
#ifdef PASSDOUBLEVALUE
    XtArgVal    dx_l1, dx_l2, dx_l3, dx_l4;
#endif
    double inc = 0.1;
    double value = DEFAULT_GAMMA;
    double min = -1000.0;
    double max = 1000.0;
    this->gamma_number = XtVaCreateManagedWidget ("gammaNumber",
	xmNumberWidgetClass,    topDiagForm,
	XmNtopAttachment,	XmATTACH_FORM,
	XmNtopOffset,		5,
	XmNrightAttachment,	XmATTACH_WIDGET,
	XmNrightOffset,		6,
	XmNdataType,            DOUBLE,
	XmNdValueStep,          DoubleVal(inc, dx_l2),
	XmNdValue,		DoubleVal(value, dx_l1),
	XmNdMinimum,		DoubleVal(min, dx_l3),
	XmNdMaximum,		DoubleVal(max, dx_l3),
	XmNdecimalPlaces,       2,
	XmNfixedNotation,       False,
	XmNeditable,		True,
	XmNcharPlaces,		7,
	XmNrecomputeSize,	False,
    NULL);
    XtAddCallback (this->gamma_number, XmNactivateCallback,
	(XtCallbackProc)ImageFormatDialog_DirtyGammaCB, (XtPointer)this);
    XtVaCreateManagedWidget ("gammaLabel",
	xmLabelWidgetClass,	topDiagForm,
	XmNtopAttachment,	XmATTACH_FORM,
	XmNtopOffset,		8,
	XmNrightAttachment,	XmATTACH_WIDGET,
	XmNrightWidget,		this->gamma_number,
	XmNrightOffset,		4,
	XmNleftAttachment,	XmATTACH_WIDGET,
	XmNleftWidget,		this->rerenderOption->getRootWidget(),
	XmNleftOffset,		4,
	XmNalignment,		XmALIGNMENT_END,
    NULL);


    //
    // F O R M A T     M E N U                F O R M A T     M E N U 
    // F O R M A T     M E N U                F O R M A T     M E N U 
    // F O R M A T     M E N U                F O R M A T     M E N U 
    //
    n = 0;
    XtSetArg (args[n], XmNrightAttachment, XmATTACH_FORM); n++;
    XtSetArg (args[n], XmNrightOffset,	  2); n++;
    XtSetArg (args[n], XmNtopAttachment,  XmATTACH_WIDGET); n++;
    XtSetArg (args[n], XmNtopWidget,      this->gamma_number); n++;
    XtSetArg (args[n], XmNtopOffset,	  2); n++;
    this->format_om = XmCreateOptionMenu (topDiagForm, "formats", args, n);
    XtManageChild (this->format_om);
    n = 0;
    XtSetArg (args[n], XmNentryAlignment, XmALIGNMENT_CENTER); n++;
    this->format_pd = XmCreatePulldownMenu (this->format_om, "formats_pd", args, n);
    XtVaSetValues (this->format_om, XmNsubMenuId, this->format_pd, NULL);

    ListIterator it(*this->image_formats);
    ImageFormat *imgfmt;
    while ( (imgfmt = (ImageFormat*)it.getNext()) ) {
	Widget but = XtVaCreateManagedWidget (imgfmt->menuString(),
	    xmPushButtonWidgetClass,	this->format_pd,
	    XmNuserData,	imgfmt,
	    XmNsensitive, 	(this->isPrinting()?imgfmt->supportsPrinting():TRUE),
	NULL);
	if (imgfmt == this->choice) {
	    XtVaSetValues (this->format_om, XmNmenuHistory, but, NULL);
	}
	imgfmt->setMenuButton(but);
	XtAddCallback (but, XmNactivateCallback, (XtCallbackProc)
	    ImageFormatDialog_ChoiceCB, (XtPointer)this);
    }


    //
    // aboveBody and belowBody are 2 separators which surround the widgets
    // needed by the image format.
    //
    this->aboveBody = XtVaCreateManagedWidget ("aboveBody",
	xmSeparatorWidgetClass,	form_diag,
	XmNtopAttachment,	XmATTACH_WIDGET,
	XmNleftAttachment,	XmATTACH_FORM,
	XmNrightAttachment,	XmATTACH_FORM,
	XmNtopWidget,		topDiagForm,
	XmNtopOffset,		2,
	XmNleftOffset,		0,
	XmNrightOffset,		0,
    NULL);

    //
    // Create the buttons at the bottom
    //
    Widget button_form = XtVaCreateManagedWidget ("buttonForm",
	xmFormWidgetClass,	form_diag,
	XmNleftAttachment,	XmATTACH_FORM,
	XmNrightAttachment,	XmATTACH_FORM,
	XmNbottomAttachment,	XmATTACH_FORM,
	XmNleftOffset,		2,
	XmNrightOffset,		2,
	XmNbottomOffset,	6,
    NULL);

    this->ok = XtVaCreateManagedWidget ("applyButton",
	xmPushButtonWidgetClass,	button_form,
	XmNtopAttachment,		XmATTACH_FORM,
	XmNtopOffset,			4,
	XmNleftAttachment,		XmATTACH_FORM,
	XmNleftOffset,			10,
    NULL);
    this->cancel = XtVaCreateManagedWidget ("cancelButton",
	xmPushButtonWidgetClass,	button_form,
	XmNtopAttachment,		XmATTACH_FORM,
	XmNtopOffset,			4,
	XmNrightAttachment,		XmATTACH_FORM,
	XmNrightOffset,			10,
    NULL);
    this->restore = XtVaCreateManagedWidget ("restoreButton",
	xmPushButtonWidgetClass,	button_form,
	XmNtopAttachment,		XmATTACH_FORM,
	XmNtopOffset,			4,
	XmNrightAttachment,		XmATTACH_WIDGET,
	XmNrightWidget,			this->cancel,
	XmNrightOffset,			20,
    NULL);
    XtAddCallback (this->restore, XmNactivateCallback, (XtCallbackProc)
	ImageFormatDialog_RestoreCB, (XtPointer)this);

    Widget sep = XtVaCreateManagedWidget ("sep",
	xmSeparatorWidgetClass,	form_diag,
	XmNleftAttachment,	XmATTACH_FORM,
	XmNrightAttachment,	XmATTACH_FORM,
	XmNleftOffset,		0,
	XmNrightOffset,		0,
	XmNbottomAttachment,	XmATTACH_WIDGET,
	XmNbottomWidget,	button_form,
	XmNbottomOffset,	2,
    NULL);

    Widget controls_form = this->createControls(form_diag);
    ASSERT(controls_form);
    XtVaSetValues (controls_form,
	XmNbottomAttachment,	XmATTACH_WIDGET,
	XmNbottomWidget,	sep,
	XmNbottomOffset,	2,
    NULL);

    this->belowBody = XtVaCreateManagedWidget ("belowBody",
	xmSeparatorWidgetClass,	form_diag,
	XmNleftAttachment,	XmATTACH_FORM,
	XmNleftOffset,		0,
	XmNrightAttachment,	XmATTACH_FORM,
	XmNrightOffset,		0,
	XmNbottomAttachment,	XmATTACH_WIDGET,
	XmNbottomWidget,	controls_form,
	XmNbottomOffset,	4,
    NULL);

    //
    // Loop over the image formats.  For each one create its container of
    // special settings.
    //
    it.setList(*this->image_formats);
    imgfmt = NUL(ImageFormat*);
    while ( (imgfmt = (ImageFormat*)it.getNext()) ) {
	Widget body = imgfmt->createBody(form_diag);
	XtVaSetValues (body,
	    XmNtopAttachment,		XmATTACH_WIDGET,
	    XmNtopWidget,		this->aboveBody,
	    XmNtopOffset,		4,
	    XmNbottomAttachment,	XmATTACH_WIDGET,
	    XmNbottomWidget,		this->belowBody,
	    XmNbottomOffset,		8,
	    XmNleftAttachment,		XmATTACH_FORM,
	    XmNrightAttachment,		XmATTACH_FORM,
	    XmNleftOffset,		2,
	    XmNrightOffset,		2,
	    XmNmappedWhenManaged, 	False,
	NULL);
    }
    // because rootWidget is still unset...
    imgfmt = (ImageFormat*)this->image_formats->getElement(1);
    this->setChoice(imgfmt);
    XtVaSetValues (imgfmt->getRootWidget(), XmNmappedWhenManaged, True, NULL);
    imgfmt->setCommandActivation();

    if (!ImageFormatDialog::WatchCursor) 
	ImageFormatDialog::WatchCursor = 
	    XCreateFontCursor (XtDisplay(parent), XC_watch);

    return form_diag;
}

boolean ImageFormatDialog::okCallback(Dialog *)
{
    //
    // We send param values to the node for every apply/ok operation, but perhaps
    // it would make sense to do this only if the continuous button is pushed in.
    // There is no need for the param values to be in the node if we're only going
    // to use the saveCurrentImage macro, however the values will not be saved
    // unless they're stuck into the node.
    //
    if (this->choice->useLocalResolution())
	this->node->setRecordResolution (this->choice->getRecordResolution(), FALSE);
    else if ((this->isRerenderAllowed() == FALSE) && 
	     (this->dirty & ImageFormatDialog::DirtyRerender) &&
	     (this->node->isRecordResolutionConnected() == FALSE))
	this->node->unsetRecordResolution(FALSE);

    if (this->choice->useLocalAspect())
	this->node->setRecordAspect (this->choice->getRecordAspect(), FALSE);
    else if ((this->isRerenderAllowed() == FALSE) && 
	     (this->dirty & ImageFormatDialog::DirtyRerender)&&
	     (this->node->isRecordAspectConnected() == FALSE))
	this->node->unsetRecordAspect(FALSE);

    if (this->choice->useLocalFormat())
	this->node->setRecordFormat (this->choice->getRecordFormat(), FALSE);

    this->dirty = 0;
    this->choice->applyValues();

    //
    // return FALSE so that superclass doesn't unmanage me.
    //
    return FALSE;
}


boolean ImageFormatDialog::isMetric()
{
    return theDXApplication->isMetricUnits();
}

//
// Put the dialog box to one side of the Image window.  This way the image
// will not be obscured just by asking to save.
//
void ImageFormatDialog::manage()
{
    Dimension dialogWidth;
    if (!XtIsRealized(this->getRootWidget()))
	XtRealizeWidget(this->getRootWidget());
    
    if (this->isManaged() == FALSE) {
	XtVaGetValues(this->getRootWidget(), XmNwidth, &dialogWidth, NULL);

	Position x;
	Position y;
	Dimension width;
	XtVaGetValues(parent, XmNx, &x, XmNy, &y, XmNwidth, &width, NULL);

	if (x > dialogWidth + 25) x -= dialogWidth + 25;
	else x += width + 25;

	Display *dpy = XtDisplay(this->getRootWidget());
	if (x > WidthOfScreen(DefaultScreenOfDisplay(dpy)) - dialogWidth)
	    x = WidthOfScreen(DefaultScreenOfDisplay(dpy)) - dialogWidth;
	XtVaSetValues(this->getRootWidget(), XmNdefaultPosition, False, NULL);
	XtVaSetValues(XtParent(this->getRootWidget()), XmNx, x, XmNy, y, NULL);
    }

    this->dirty = 0;
    
    //
    // Make sure the buttons are insensitive when this dialog is created
    // during execution.
    //
    if (theDXApplication->getExecCtl()->isExecuting())
	this->deactivate();

    this->setCommandActivation();

    this->Dialog::manage();

    //
    // Give the dialog sufficient room.
    //
    Widget diag = this->getRootWidget();
    while ((diag) && (!XtIsShell(diag)))
	diag = XtParent(diag);
    ASSERT(diag);
    int height = this->getRequiredHeight() + this->choice->getRequiredHeight();
    XtVaSetValues (diag,
	XmNminHeight, height,
	XmNheight, height,
	XmNminWidth, this->getRequiredWidth(),
    NULL);
}

void ImageFormatDialog::update()
{
    this->setCommandActivation();
}


void ImageFormatDialog::currentImage()
{
    ASSERT(this->node);

    if(NOT theDXApplication->getPacketIF()) {
        WarningMessage("No connection to the server.");
    	return ;
    }

    DXExecCtl  *execCtl = theDXApplication->getExecCtl();

    if(execCtl->inExecOnChange())
        execCtl->suspendExecOnChange();

    //
    // Strip surrounding quotes
    //
    const char *output_file = this->getOutputFile();
    char *string_to_use = NUL(char*);
    if (output_file[0] == '"') {
	string_to_use = DuplicateString(&output_file[1]);
	string_to_use[strlen(string_to_use)-1] = '\0';
    } else {
	string_to_use = DuplicateString(output_file);
    }


    //
    // If we're currently doing hardware rendering and allowRerendering is turned
    // off, then call the dxfSaveCurrentImage macro in such a way that it will
    // use the new ReadImageWindow() module instead of calling Render().
    //
    if ((this->node->hardwareMode()) && (this->rerenderOption->getState() == FALSE)) {
	const char* where_param = this->node->getInputValueString(WHERE);
	this->makeExecOutputImage (where_param, 
	    this->choice->getRecordFormat(), string_to_use);
    } else {
	this->makeExecOutputImage (this->node->getPickIdentifier(), 
	    this->choice->getRecordResolution(), this->choice->getRecordAspect(),
	    this->choice->getRecordFormat(), string_to_use);
    }
    delete string_to_use;

    if(execCtl->inExecOnChange())
        execCtl->resumeExecOnChange();
}

boolean ImageFormatDialog::makeExecOutputImage(const char* where, 
    const char* format, const char *file)
{
    char buf[1024];

    DXPacketIF *p = theDXApplication->getPacketIF();
    if (!p) {
	WarningMessage("No connection to server");
	return FALSE;
    }

    //
    // dxfSaveCurrentImage() is a macro that is loaded automatically
    // from dxroot/lib/dxrc by the exec.
    //
    sprintf(buf, "dxfSaveCurrentImage(NULL, 0, 0.0, \"%s\", \"%s\", %s);",
	    file, format, where);
    p->send(DXPacketIF::FOREGROUND, buf, 
		ImageFormatDialog::ImageOutputCompletePH, (void*)this);
    theDXApplication->setBusyCursor(TRUE);
    this->setBusyCursor(TRUE);

    return TRUE;
}

boolean ImageFormatDialog::makeExecOutputImage(const char *pickId,
			int width, float aspect, 
			const char *format, const char *file)
{
    char buf[1024];

    DXPacketIF *p = theDXApplication->getPacketIF();
    if (!p) {
	WarningMessage("No connection to server");
	return FALSE;
    }

    //
    // dxfSaveCurrentImage() is a macro that is loaded automatically
    // from dxroot/lib/dxrc by the exec.
    //
    sprintf(buf, "dxfSaveCurrentImage(\"%s\", %d, %f, \"%s\", \"%s\", NULL);",
	    pickId, width, aspect, file, format);
    p->send(DXPacketIF::FOREGROUND, buf, 
		ImageFormatDialog::ImageOutputCompletePH, (void*)this);
    theDXApplication->setBusyCursor(TRUE);
    this->setBusyCursor(TRUE);

    return TRUE;
}

void ImageFormatDialog::ImageOutputCompletePH(void *clientData, int , void *)
{
    ImageFormatDialog* ifd = (ImageFormatDialog*)clientData;
    theDXApplication->setBusyCursor(FALSE); 
    ifd->setBusyCursor(FALSE);
}



//
// Control greying out of options due to data-driving.  Certain toggle button
// values imply certain others so set them too.  e.g. push rerender implies
// both size toggle pushed and resolution toggle pushed.
//
void ImageFormatDialog::setCommandActivation()
{
    if (this->resetting) return ;

    //
    // Record format (just the first chunk of the string)
    //
    boolean setFormat = FALSE;
    if (this->node->isRecordFormatConnected()) {
	setFormat = TRUE;
	XtSetSensitive (this->format_om, False);
	XtSetSensitive (this->gamma_number, False);
	this->dirty&= ~ImageFormatDialog::DirtyGamma;
	this->dirty&= ~ImageFormatDialog::DirtyDelayed;
    } else {
	XtSetSensitive (this->format_om, True);
	XtSetSensitive (this->gamma_number, True);
	if ((this->dirty & ImageFormatDialog::DirtyFormat) == 0)
	    setFormat = TRUE;
    }

    //
    // If record resolution is set and is different from resolution,
    // then force the allow-rerender toggle button
    //
    boolean recresset = this->node->isRecordResolutionSet();
    boolean recaspset = this->node->isRecordAspectSet();
    if (recresset || recaspset) {
	int recx,recy;
	double recaspect;
	this->node->getRecordResolution(recx,recy);
	this->node->getRecordAspect(recaspect);

	int x,y;
	double aspect;
	this->node->getResolution(x,y);
	this->node->getAspect(aspect);

	if ((recx != x) || (recaspect != aspect)) {
	    if ((this->dirty & DirtyRerender) == 0)
		this->rerenderOption->setState (TRUE, TRUE);
	}
    }
    boolean rescon = this->node->isRecordResolutionConnected();
    boolean aspcon = this->node->isRecordAspectConnected();
    if (rescon && aspcon) {
	this->rerenderCmd->deactivate();
    }


    if (setFormat) {
	const char *value;
	this->node->getRecordFormat(value);
	ASSERT(value);
	this->parseRecordFormat(value);
	const char *fmt_name = strchr(value, '"');

	//
	// there should be no text available if the param is wired and the
	// net is not executed yet.  In that case just default to the first
	// entry in the dictionary.
	//
	if ((!fmt_name) || (!fmt_name[0])) {
	    ImageFormat* imgfmt = (ImageFormat*)this->image_formats->getElement(1);
	    fmt_name = imgfmt->paramString();
	} else {
	    fmt_name++;
	}
	ListIterator it (*this->image_formats);
	ImageFormat *imgfmt;
	while ( (imgfmt = (ImageFormat*)it.getNext()) ) {
	    const char *cp = imgfmt->paramString();
	    if (EqualSubstring (fmt_name, cp, strlen(cp))) {
		XtVaSetValues (this->format_om, 
		    XmNmenuHistory, imgfmt->getMenuButton(), 
		NULL);
		imgfmt->parseRecordFormat(value);
		this->setChoice(imgfmt);
	    } 
	}
    } else {
	this->choice->setCommandActivation();
    }

    if (this->choice->supportsDelayedColors()) {
	if (this->node->isRecordFormatConnected() == FALSE) 
	    this->delayedCmd->activate();
	else
	    this->delayedCmd->deactivate();
    } else {
	this->delayedCmd->deactivate();
	if (this->choice->requiresDelayedColors())
	    this->delayedOption->setState(TRUE, TRUE);
	else
	    this->delayedOption->setState(FALSE, TRUE);
    }

    ASSERT(this->choice);
}


void ImageFormatDialog::parseRecordFormat(const char* value)
{
    if (this->dirtyGamma() == FALSE) {
	boolean refreshed = FALSE;
	char *matchstr = "gamma=";
	const char *cp = strstr (value, matchstr);
	if (cp) {
	    double gamma;
	    cp+= strlen(matchstr);
	    int items_parsed = sscanf (cp, "%lg", &gamma);
	    if (items_parsed == 1) {
		this->setGamma(gamma);
		this->dirty|= ImageFormatDialog::DirtyGamma;
		refreshed = TRUE;
	    } 
	}
	if (!refreshed) this->setGamma(DEFAULT_GAMMA);
    }

    if ((this->dirty & ImageFormatDialog::DirtyDelayed) == 0) {
	boolean refreshed = FALSE;
	char *matchstr = "delayed=";
	const char *cp = strstr (value, matchstr);
	if (cp) {
	    int delayed;
	    cp+= strlen(matchstr);
	    int items_parsed = sscanf (cp, "%d", &delayed);
	    if (items_parsed == 1) {
		if (delayed == 1) {
		    this->delayedOption->setState (TRUE, TRUE);
		    refreshed = TRUE;
		}
	    }
	}
	if ((!refreshed) && (this->delayedOption->getState())) 
	    this->delayedOption->setState(FALSE, TRUE);
    }
}

void ImageFormatDialog::deactivate(const char *msg)
{
    if(this->ok) XtSetSensitive(this->ok, False);
    if(this->restore) XtSetSensitive(this->restore, False);
    this->NoOpCommand::deactivate(msg);
}

void ImageFormatDialog::activate()
{
    if (this->ok) XtSetSensitive(this->ok, True);
    if (this->restore) XtSetSensitive(this->restore, True);
    this->NoOpCommand::activate();
}


void ImageFormatDialog::setChoice(ImageFormat* selected)
{
ImageFormat* old_choice = this->choice;

    if (this->choice == selected) {
	this->choice->setCommandActivation();
	return ;
    }
    if (this->getRootWidget() == NUL(Widget)) {
	this->choice = selected;
	return ;
    }

    //
    // Hide the box of the current selection.
    //
    if (this->choice) {
	XtVaSetValues (this->choice->getRootWidget(),
	    XmNmappedWhenManaged, 	FALSE,
	NULL);
    }
    this->choice = selected;

    //
    // Show the box of the new selection.
    //
    ASSERT(selected);
    XtVaSetValues (selected->getRootWidget(),
	XmNmappedWhenManaged,	TRUE,
    NULL);

    //
    // Set up the option menu selector.
    //
    XtVaSetValues (this->format_om, XmNmenuHistory, selected->getMenuButton(), NULL);
    this->choice->shareSettings(old_choice);
    this->choice->setCommandActivation();

    //
    // Give the dialog sufficient room.
    //
    Widget diag = this->getRootWidget();
    while ((diag) && (!XtIsShell(diag)))
	diag = XtParent(diag);
    ASSERT(diag);
    int height = this->getRequiredHeight() + this->choice->getRequiredHeight();
    XtVaSetValues (diag,
	XmNminHeight, height,
	XmNheight, height,
    NULL);

    if (this->choice->supportsDelayedColors())
	this->delayedCmd->activate();
    else {
	this->delayedCmd->deactivate();
	if (this->choice->requiresDelayedColors())
	    this->delayedOption->setState(TRUE, TRUE);
	else
	    this->delayedOption->setState(FALSE, TRUE);
    }
}

boolean ImageFormatDialog::allowRerender()
{
    this->dirty|= ImageFormatDialog::DirtyRerender;
    this->setCommandActivation();
    return TRUE;
}

boolean ImageFormatDialog::isRerenderAllowed()
{
    if (!this->rerenderOption) return FALSE;
    return this->rerenderOption->getState();
}

void ImageFormatDialog::setGamma(double gamma)
{
#ifdef PASSDOUBLEVALUE
    XtArgVal    dx_l1; 
#endif
    XtVaSetValues (this->gamma_number, 
	XmNdValue, DoubleVal(gamma, dx_l1),
    NULL);
}

double ImageFormatDialog::getGamma()
{
double gamma;

    XtVaGetValues (this->gamma_number, XmNdValue, &gamma, NULL);
    return gamma;
}

void ImageFormatDialog::cancelCallback(Dialog*)
{
    this->unmanage();
}

//
// Problem:  The restoreCallback is actually a reset, not a restore.  When
// you push the button it flips the all params tab-up.  Ordinarily you would
// think of a restore as going back to the set of values you had the last time
// you clicked apply.  If you don't like the way this "restore" is working,
// it's easy to change to be "ordinary".  All you have to do is remove what
// it does (and to do that you also have to see subclass:restoreCallback() since
// restoreCallback is virtual, and replace it it with this->manage().  Notice
// that you get the restore function if you go back to the ImageWindow's menubar
// and click on File/Save Image.
//
// Other Note:  I'm recording the old choice and resetting it at the end
// of the method because to me, that behavior seems nicer than setting everything
// that back to original state.  There is already on bug report on this,
// though: gresh988
//
void ImageFormatDialog::restoreCallback()
{
    ImageFormat* imgfmt = this->choice;
    this->dirty = ImageFormatDialog::DirtyFormat;
    this->choice->restore();
    this->resetting = TRUE;
    this->rerenderOption->setState(FALSE, TRUE);
    if (!this->node->isRecordResolutionConnected())
	this->node->unsetRecordResolution (FALSE);
    if (!this->node->isRecordAspectConnected())
	this->node->unsetRecordAspect (FALSE);
    if (!this->node->isRecordFormatConnected())
	this->node->unsetRecordFormat (FALSE);
    this->resetting = FALSE;
    this->setCommandActivation();
    this->setChoice(imgfmt);
    if (this->node->isRecordFormatConnected() == FALSE) {
	this->setGamma(DEFAULT_GAMMA);
	//
	// It seems strange to be touching this value here.  In the beginning of
	// restoreCallback, I'm putting DirtyFormat into this->dirty in order to
	// save the current choice.  Otherwise it would be reset in setCommandActivation.
	// Since I've put this value into this->dirty, I'm unable to reset the delayed
	// option in setCommandActivation, so do it now.
	// If this choice didn't support delayed colors, then the proper value
	// for this choice would have been set elsewhere.
	//
	if (this->choice->supportsDelayedColors()) 
	    this->delayedOption->setState(FALSE);
    }
}

void ImageFormatDialog::setBusyCursor(boolean busy)
{
    Widget root = this->getRootWidget();

    if (busy) this->busyCursors++;
    else this->busyCursors--;
    boolean on = (boolean)this->busyCursors;
    if (on) {
	XDefineCursor (XtDisplay(root), XtWindow(root),
	    ImageFormatDialog::WatchCursor);
	XSync (XtDisplay(root), False);
    } else {
	XUndefineCursor (XtDisplay(root), XtWindow(root));
    }
}

boolean ImageFormatDialog::delayedColors()
{
    this->dirty|= ImageFormatDialog::DirtyDelayed;
    return TRUE;
}

extern "C" {

#if 0
void ImageFormatDialog_UnitsCB(Widget w, XtPointer clientData, XtPointer)
{
    ImageFormatDialog *ifd = (ImageFormatDialog*)clientData;
    ASSERT(ifd);
    ifd->units = w;
    ifd->choice->changeUnits();
}
#endif

void ImageFormatDialog_ChoiceCB(Widget w, XtPointer clientData, XtPointer)
{
    ImageFormatDialog *ifd = (ImageFormatDialog*)clientData;
    ASSERT(ifd);
    ifd->dirty|= ImageFormatDialog::DirtyFormat;
    ImageFormat *imgfmt = NUL(ImageFormat*);
    XtVaGetValues (w, XmNuserData, &imgfmt, NULL);
    ifd->setChoice(imgfmt);
}

void ImageFormatDialog_RestoreCB (Widget , XtPointer clientData, XtPointer)
{
    ImageFormatDialog* ifd = (ImageFormatDialog*)clientData;
    ASSERT(ifd);
    ifd->restoreCallback();
}

void ImageFormatDialog_DirtyGammaCB (Widget , XtPointer clientData, XtPointer)
{
    ImageFormatDialog* ifd = (ImageFormatDialog*)clientData;
    ASSERT(ifd);
    ifd->dirty|= ImageFormatDialog::DirtyGamma;
}

} // end extern C

