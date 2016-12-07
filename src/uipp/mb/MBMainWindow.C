/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>

#include "defines.h"

#include <ctype.h>

#if defined(HAVE_FSTREAM)
#include <fstream>
#elif defined(HAVE_FSTREAM_H)
#include <fstream.h>
#endif

//#include <X11/IntrinsicP.h>
//#include <X11/CoreP.h>

#include <Xm/Xm.h>
#include <Xm/CascadeB.h>
#include <Xm/DrawingA.h>
#include <Xm/Frame.h>
#include <Xm/Form.h>
#include <Xm/FileSB.h>
#include <Xm/Label.h>
#include <Xm/List.h>
#include <Xm/ToggleB.h>
#include <Xm/RowColumn.h>
#include <Xm/ScrolledW.h>
#include <Xm/Text.h>
#include <Xm/Separator.h>
#include <Xm/PushB.h>

#include "XmDX.h"
#include "Number.h"
#include "Stepper.h"

#include "MBMainWindow.h"
#include "MBApplication.h"
#include "MBCommand.h"
#include "MBNewCommand.h"
#include "lex.h"
#include "DXStrings.h"
#include "List.h"
#include "InfoDialogManager.h"
#include "WarningDialogManager.h"
#include "ErrorDialogManager.h"
#include "QuestionDialogManager.h"
#include "ButtonInterface.h"
#include "CommandScope.h"
#include "OptionsDialog.h"

extern "C" {
extern int Generate(int, char *);
}

#define ICON_NAME	"MB"
#define LABEL_TOP_OFFSET 19
#define TOGGLE_OFFSET 19
#define STEPPER_TOP_OFFSET 10
#define SHORT_TEXT_WIDTH 150
#define LONG_TEXT_WIDTH  300
#define OPTION_MENU_WIDTH  180
#define FIRST_COLUMN_END  39 
#define SECOND_COLUMN_START  45 
#define THIRD_COLUMN_START  65 
#define OPTION_MENU_END  85 

#define L_CASE(str)						\
		{int si;					\
		for(si = 0; si < STRLEN(str); si++)		\
		    str[si] = tolower(str[si]);}

#define NAME_OUT(strm, w)					\
		{char *tmp = strdup(XtName(w));			\
		L_CASE(tmp);					\
		strm << tmp;					\
		delete tmp;}

#define OM_NAME_OUT(strm, w)					\
		{Widget wtmp;					\
		XtVaGetValues(w, XmNmenuHistory, &wtmp, NULL);	\
		NAME_OUT(strm, wtmp);}

#define RESET_OM(w)						\
		{Widget wtmp;					\
		WidgetList children;				\
		XtVaGetValues(w, XmNsubMenuId, &wtmp, NULL);	\
		XtVaGetValues(wtmp, XmNchildren, &children, NULL);\
		XtManageChild(children[0]);			\
		XtVaSetValues(w, XmNmenuHistory, children[0], NULL);}
		
#define SET_OM_NAME(w, name)					\
	{Widget wtmp;						\
	WidgetList children;					\
	int num_children;					\
	int itmp;						\
	XtVaGetValues(w, XmNsubMenuId, &wtmp, NULL);		\
	XtVaGetValues(wtmp, XmNchildren, &children, 		\
		XmNnumChildren, &num_children, NULL);		\
	for(itmp = 0; itmp < num_children; itmp++)		\
	{                                                       \
	    if(EqualString(XtName(children[itmp]), name))	\
	    {							\
		XtVaSetValues(w, XmNmenuHistory, children[itmp],NULL);\
		break;						\
	    }							\
	}							\
	if(itmp == num_children)				\
	    WarningMessage("Set value failed:%s", name);}

#define STRIP_QUOTES(str)					\
	{int itmp;						\
	if((str[0] == '"') && (str[STRLEN(str)-1] == '"'))	\
	{							\
	    for(itmp = 1; itmp < STRLEN(str)-1; itmp++)		\
		str[itmp-1] = str[itmp];			\
	    str[itmp-1] = '\0';					\
	}}


static const char MBExtension[] = ".mb";

String MBMainWindow::DefaultResources[] =
{
    ".width:                                      1030",
    ".height:                                     760",
    ".minWidth:                                   825",
    ".minHeight:                                  700",
    ".iconName:                                   Module Builder",
    ".title:                                      Module Builder",
    "*XmNumber.editable:                          True",
    "*topOffset:                                  15",
    "*bottomOffset:                               15",
    "*leftOffset:                                 10",
    "*rightOffset:                                10",
    "*XmToggleButton.shadowThickness:             0",
#ifdef aviion
    "*om.labelString:                             ",
#endif
    "*fileMenu.labelString:                       File",
    "*fileMenu.mnemonic:                          F",

    "*editMenu.labelString:                       Edit",
    "*editMenu.mnemonic:                          E",

    "*buildMenu.labelString:                      Build",
    "*buildMenu.mnemonic:                         B",

    "*newOption.labelString:                      New",
    "*newOption.mnemonic:                         N",
    "*openOption.labelString:                     Open...",
    "*openOption.mnemonic:                        O",

    "*saveOption.labelString:                     Save",
    "*saveOption.mnemonic:                        S",
    "*saveOption.accelerator:                     Ctrl<Key>S",
    "*saveOption.acceleratorText:                 Ctrl+S",

    "*saveAsOption.labelString:                   Save As...",
    "*saveAsOption.mnemonic:                      a",

    "*quitOption.labelString:                     Quit",
    "*quitOption.mnemonic:                        Q",
    "*quitOption.accelerator:                     Ctrl<Key>Q",
    "*quitOption.acceleratorText:                 Ctrl+Q",

    "*XmForm.accelerators:			#augment\n\
    <Key>Return:				BulletinBoardReturn()",

    "*buildMDFOption.labelString:                  Create MDF",
    "*buildMDFOption.mnemonic:                     M",
    "*buildCOption.labelString:                    Create C",
    "*buildCOption.mnemonic:                       C",
    "*buildMakefileOption.labelString:             Create Makefile",
    "*buildMakefileOption.mnemonic:                K",
    "*buildAllOption.labelString:                  Create All",
    "*buildAllOption.mnemonic:                     A",
    "*buildExecutableOption.labelString:           Build Executable",
    "*buildExecutableOption.mnemonic:              B",
    "*buildRunOption.labelString:                  Build and Run DX",
    "*buildRunOption.mnemonic:                     R",

    "*commentOption.labelString:                   Comment...",
    "*commentOption.mnemonic:                      C",

    "*optionsOption.labelString:                    Options...",
    "*optionsOption.mnemonic:                       O",

    "*helpTutorialOption.labelString:           Start Tutorial...",

    NULL
};


boolean MBMainWindow::ClassInitialized = FALSE;

MBMainWindow::MBMainWindow() : IBMMainWindow("MBMWName", TRUE)
{

    //
    // Initialize member data.
    //

    this->title = NUL(char*); 
    this->comment_text = NULL;

    //
    // Initialize member data.
    //
    this->fileMenu          = NUL(Widget);
    this->editMenu          = NUL(Widget);

    this->fileMenuPulldown  = NUL(Widget);
    this->editMenuPulldown  = NUL(Widget);
	
    this->newOption         = NUL(CommandInterface*);
    this->openOption        = NUL(CommandInterface*);
    this->saveOption        = NUL(CommandInterface*);
    this->saveAsOption      = NUL(CommandInterface*);
    this->quitOption        = NUL(CommandInterface*);
    this->commentOption     = NUL(CommandInterface*);
    this->optionsOption     = NUL(CommandInterface*);

    if (!this->commandScope)
	this->commandScope = new CommandScope();

    this->newCmd =
        new MBNewCommand("new",this->commandScope,TRUE,this, NULL);

    this->openCmd =
        new MBCommand("open",this->commandScope,TRUE,this, this->OPEN);

    this->saveCmd =
        new MBCommand("save",this->commandScope,FALSE,this, this->SAVE);

    this->saveAsCmd =
        new MBCommand("saveAs",this->commandScope,TRUE,this, this->SAVE_AS);

    this->quitCmd =
        new ConfirmedQCommand("quit",this->commandScope,
		TRUE, this, theMBApplication);

    this->buildMDFCmd =
        new MBCommand("buildMDF",this->commandScope,
		TRUE,this, this->BUILD_MDF);

    this->buildCCmd =
        new MBCommand("buildC",this->commandScope,
		TRUE,this, this->BUILD_C);

    this->buildMakefileCmd =
        new MBCommand("buildMakefile",this->commandScope,
	    TRUE,this, this->BUILD_MAKEFILE);

    this->buildAllCmd =
        new MBCommand("buildAll",this->commandScope,
		TRUE,this, this->BUILD_ALL);

    this->buildExecutableCmd =
        new MBCommand("buildExecutable",this->commandScope,
		TRUE,this, this->BUILD_EXECUTABLE);

    this->buildRunCmd =
        new MBCommand("buildRunn",this->commandScope,
		TRUE,this, this->BUILD_RUN);

    this->commentCmd =
        new MBCommand("comment",this->commandScope,TRUE,this, this->COMMENT);

    this->optionsCmd =
        new MBCommand("options",this->commandScope,TRUE, this, this->OPTIONS);

    this->filename = NULL;

    this->helpTutorialOption = NULL;

    //
    // input param #1 is required!!!
    //
    MBParameter *param = new MBParameter;
    param->setRequired(True);
    param->setName("input_1");
    this->inputParamList.appendElement(param);
    param = new MBParameter;
    param->setName("output_1");
    this->outputParamList.appendElement(param);
    this->is_input = TRUE;
    this->ignore_cb = FALSE;

	
    //
    // Install the default resources for THIS class (not the derived classes)
    //
    if (NOT MBMainWindow::ClassInitialized)
    {
	ASSERT(theApplication);
        MBMainWindow::ClassInitialized = TRUE;
	this->installDefaultResources(theApplication->getRootWidget());
    }
}


MBMainWindow::~MBMainWindow()
{
    // FIXME: memory leak city !!!! delete everything that was allocated.

    //
    // Make the panel disappear from the display (i.e. don't wait for
    // (two phase destroy to remove the panel). 
    //
    if (this->getRootWidget())
        XtUnmapWidget(this->getRootWidget());
}

//
// Install the default resources for this class.
//
void MBMainWindow::installDefaultResources(Widget baseWidget)
{
    this->setDefaultResources(baseWidget, MBMainWindow::DefaultResources);
    this->IBMMainWindow::installDefaultResources(baseWidget);
}

void MBMainWindow::initialize()
{

    if (!this->isInitialized()) {
	//
	// Now, call the superclass initialize().
	//
	this->IBMMainWindow::initialize();
    }
}

void MBMainWindow::manage()
{
    this->IBMMainWindow::manage();
}


Widget MBMainWindow::createWorkArea(Widget parent)
{
    Widget    topform;
    Widget    sep;
    Widget    pulldown_menu;
    int       n;
    Arg	      wargs[50];
    Dimension width;
    XmString  xms;

    ASSERT(parent);

    this->open_fd = new OpenFileDialog(this);

    this->save_as_fd = new SADialog(this);

    this->comment_dialog = new CommentDialog(parent, FALSE, this);

    this->options_dialog = new OptionsDialog(parent, this);
    // this->options_dialog->setDialogTitle("Options");

    topform =
	XtVaCreateManagedWidget
	    ("mbfform",
	     xmFormWidgetClass,
	     parent,
	     NULL);
    //
    // Create the form.
    //
    this->mdfform =
	XtVaCreateManagedWidget
	    ("mbfform",
	     xmFormWidgetClass,
	     topform,
	     XmNtopAttachment,		XmATTACH_FORM,
	     XmNleftAttachment,		XmATTACH_FORM,
	     XmNbottomAttachment,	XmATTACH_FORM,
	     XmNrightAttachment,	XmATTACH_FORM,
	     XmNshadowThickness, 	1,
	     XmNshadowType, 		XmSHADOW_IN,
	     NULL);

    xms = XmStringCreateSimple("Overall Module Description");
    module_label =
	XtVaCreateManagedWidget
	    ("Module",
	     xmLabelWidgetClass,
	     this->mdfform,
	     XmNtopAttachment,		XmATTACH_FORM,
	     XmNtopOffset,		LABEL_TOP_OFFSET,
	     XmNleftAttachment,		XmATTACH_FORM,
	     XmNrightAttachment,	XmATTACH_FORM,
	     XmNlabelString,		xms,
	     NULL);
    XmStringFree(xms);

    xms = XmStringCreateSimple("Name");
    this->module_name_label =
	XtVaCreateManagedWidget
	    ("Name",
	     xmLabelWidgetClass,
	     this->mdfform,
	     XmNrecomputeSize,		False,
	     XmNwidth,			90,
	     XmNalignment,		XmALIGNMENT_BEGINNING,
	     XmNtopAttachment,		XmATTACH_WIDGET,
	     XmNtopWidget,		module_label,
	     XmNtopOffset,		LABEL_TOP_OFFSET,
	     XmNleftAttachment,		XmATTACH_FORM,
	     XmNlabelString,		xms,
	     NULL);
    XmStringFree(xms);

    this->module_name_text =
	XtVaCreateManagedWidget
	    ("nameText",
	     xmTextWidgetClass,
	     this->mdfform,
	     XmNtopAttachment,		XmATTACH_WIDGET,
	     XmNtopWidget,		module_label,
	     XmNleftAttachment,		XmATTACH_WIDGET,
	     XmNleftWidget,		this->module_name_label,
	     XmNrightAttachment,	XmATTACH_POSITION,
	     XmNrightPosition,		FIRST_COLUMN_END,
	     NULL);

    XtAddCallback(this->module_name_text,
	  XmNmodifyVerifyCallback,
	  (XtCallbackProc)MBMainWindow_setDirtyCB,
	  (XtPointer)this);

    xms = XmStringCreateSimple("Category");
    this->category_label =
	XtVaCreateManagedWidget
	    ("Category",
	     xmLabelWidgetClass,
	     this->mdfform,
	     XmNtopAttachment,		XmATTACH_WIDGET,
	     XmNtopWidget,		this->module_name_text,
	     XmNtopOffset,		LABEL_TOP_OFFSET,
	     XmNleftAttachment,		XmATTACH_FORM,
	     XmNlabelString,		xms,
	     NULL);
    XmStringFree(xms);

    xms = XmStringCreateSimple("Existing");
    this->existing_category =
	XtVaCreateManagedWidget
	    ("Existing",
	     xmPushButtonWidgetClass,
	     this->mdfform,
	     XmNtopAttachment,		XmATTACH_WIDGET,
	     XmNtopWidget,		this->module_name_text,
	     XmNtopOffset,		LABEL_TOP_OFFSET,
	     XmNrightAttachment,	XmATTACH_POSITION,
	     XmNrightPosition,		FIRST_COLUMN_END,
	     XmNlabelString,		xms,
	     NULL);
    XmStringFree(xms);

    this->CreateExistingCategoryPopup();

    this->category_text =
	XtVaCreateManagedWidget
	    ("categoryText",
	     xmTextWidgetClass,
	     this->mdfform,
	     XmNtopAttachment,		XmATTACH_WIDGET,
	     XmNtopWidget,		this->module_name_text,
	     XmNleftAttachment,		XmATTACH_OPPOSITE_WIDGET,
	     XmNleftWidget,		this->module_name_text,
	     XmNleftOffset,		0,
	     XmNrightAttachment,	XmATTACH_WIDGET,
	     XmNrightWidget,		this->existing_category,
	     NULL);

    XtAddCallback(this->category_text,
	  XmNmodifyVerifyCallback,
	  (XtCallbackProc)MBMainWindow_setDirtyCB,
	  (XtPointer)this);

    xms = XmStringCreateSimple("Description");
    this->mod_description_label =
	XtVaCreateManagedWidget
	    ("Description ",
	     xmLabelWidgetClass,
	     this->mdfform,
	     XmNtopAttachment,		XmATTACH_WIDGET,
	     XmNtopWidget,		this->category_text,
	     XmNtopOffset,		LABEL_TOP_OFFSET,
	     XmNleftAttachment,		XmATTACH_FORM,
	     XmNlabelString,		xms,
	     NULL);
    XmStringFree(xms);

    this->mod_description_text =
	XtVaCreateManagedWidget
	    ("descriptionText",
	     xmTextWidgetClass,
	     this->mdfform,
	     XmNtopAttachment,		XmATTACH_WIDGET,
	     XmNtopWidget,		this->category_text,
	     XmNleftAttachment,		XmATTACH_OPPOSITE_WIDGET,
	     XmNleftWidget,		this->module_name_text,
	     XmNleftOffset,		0,
	     XmNrightAttachment,	XmATTACH_POSITION,
	     XmNrightPosition,		FIRST_COLUMN_END,
	     NULL);

    XtAddCallback(this->mod_description_text,
	  XmNmodifyVerifyCallback,
	  (XtCallbackProc)MBMainWindow_setDirtyCB,
	  (XtPointer)this);

	
    xms = XmStringCreateSimple("Number of inputs");
    this->num_inputs_label =
	XtVaCreateManagedWidget
	    ("Number of inputs ",
	     xmLabelWidgetClass,
	     this->mdfform,
	     XmNtopAttachment,		XmATTACH_WIDGET,
	     XmNtopWidget,		mod_description_label,
             XmNtopOffset,             	LABEL_TOP_OFFSET+5,
	     XmNleftAttachment,		XmATTACH_FORM,
	     XmNlabelString,		xms,
	     NULL);
    XmStringFree(xms);

    this->num_inputs_stepper = XtVaCreateManagedWidget
	    ("numInputs",
	     xmStepperWidgetClass,
	     this->mdfform,
	     XmNtopAttachment,		XmATTACH_WIDGET,
	     XmNtopWidget,		mod_description_label,
             XmNtopOffset,             	LABEL_TOP_OFFSET+5,
	     XmNleftAttachment,		XmATTACH_WIDGET,
	     XmNleftWidget,		num_inputs_label,
	     XmNleftOffset,		50,
	     XmNdataType,		INTEGER,
	     XmNiValue,			1,
	     XmNiMinimum,		1,
	     XmNiMaximum,		1000,
	     NULL);

    XtAddCallback(this->num_inputs_stepper,
	  XmNactivateCallback,
	  (XtCallbackProc)MBMainWindow_numInputsCB,
	  (XtPointer)this);

	
    xms = XmStringCreateSimple("Number of outputs");
    this->num_outputs_label =
	XtVaCreateManagedWidget
	    ("Number of outputs ",
	     xmLabelWidgetClass,
	     this->mdfform,
	     XmNtopAttachment,		XmATTACH_WIDGET,
	     XmNtopWidget,		this->num_inputs_label,
             XmNtopOffset,             	LABEL_TOP_OFFSET,
	     XmNleftAttachment,		XmATTACH_FORM,
	     XmNlabelString,		xms,
	     NULL);
    XmStringFree(xms);

    this->num_outputs_stepper = XtVaCreateManagedWidget
	    ("numOutputs",
	     xmStepperWidgetClass,
	     this->mdfform,
	     XmNtopAttachment,		XmATTACH_WIDGET,
	     XmNtopWidget,		this->num_inputs_label,
             XmNtopOffset,             	LABEL_TOP_OFFSET,
	     XmNleftAttachment,		XmATTACH_OPPOSITE_WIDGET,
	     XmNleftWidget,		num_inputs_stepper,
	     XmNleftOffset,		0,
	     XmNdataType,		INTEGER,
	     XmNiValue,			1,
	     XmNiMinimum,		1,
	     XmNiMaximum,		1000,
	     NULL);

    XtAddCallback(this->num_outputs_stepper,
	  XmNactivateCallback,
	  (XtCallbackProc)MBMainWindow_numOutputsCB,
	  (XtPointer)this);

    XtVaGetValues(this->num_outputs_stepper, XmNwidth, &width, NULL);

    /* right hand side */

    xms = XmStringCreateSimple("Module Type");
    this->moduletype_label =
	XtVaCreateManagedWidget
	    ("Module Type",
	     xmLabelWidgetClass,
	     this->mdfform,
	     XmNtopAttachment,		XmATTACH_WIDGET,
	     XmNtopWidget,		this->module_label,
             XmNtopOffset,             	LABEL_TOP_OFFSET,
	     XmNleftAttachment,		XmATTACH_POSITION,
	     XmNleftPosition,		SECOND_COLUMN_START,
	     XmNleftOffset,		0,
	     XmNlabelString,		xms,
	     NULL);
    XmStringFree(xms);
    /* the new radio button */
    n = 0;
    XtSetArg(wargs[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(wargs[n], XmNtopWidget, moduletype_label); n++;
    XtSetArg(wargs[n], XmNtopOffset, 5); n++;
    XtSetArg(wargs[n], XmNleftAttachment, XmATTACH_POSITION); n++;
    XtSetArg(wargs[n], XmNleftPosition, SECOND_COLUMN_START); n++;
    XtSetArg(wargs[n], XmNleftOffset, 20); n++;
    this->module_type_rb = XmCreateRadioBox(this->mdfform,
                                           "module type", wargs, n);
    XtManageChild(this->module_type_rb);

    xms = XmStringCreateSimple("inboard");
    this->inboard_tb = XtVaCreateManagedWidget("inboard_tb",
                       xmToggleButtonWidgetClass, this->module_type_rb,
                       XmNlabelString,      xms, 
                       NULL);
    XmStringFree(xms);

    xms = XmStringCreateSimple("outboard");
    this->outboard_tb = XtVaCreateManagedWidget("outboard_tb",
                       xmToggleButtonWidgetClass, this->module_type_rb,
                       XmNlabelString,      xms, 
                       NULL);
    XmStringFree(xms);
    XtAddCallback(this->outboard_tb,
          XmNvalueChangedCallback,
          (XtCallbackProc)MBMainWindow_outboardCB,
          (XtPointer)this);

    xms = XmStringCreateSimple("runtime-loadable");
    this->runtime_tb = XtVaCreateManagedWidget("runtime_tb",
                       xmToggleButtonWidgetClass, this->module_type_rb,
                       XmNlabelString,      xms, 
                       NULL);
    XmStringFree(xms);
    XtAddCallback(this->runtime_tb,
          XmNvalueChangedCallback,
          (XtCallbackProc)MBMainWindow_runtimeCB,
          (XtPointer)this);
    XmToggleButtonSetState(this->inboard_tb,TRUE,TRUE);


    xms = XmStringCreateSimple("Asynchronous");
    this->asynch_tb=
    XtVaCreateManagedWidget("asynchronousToggle",
                xmToggleButtonWidgetClass, this->mdfform,
                XmNleftAttachment,      XmATTACH_POSITION,
                XmNleftPosition,        THIRD_COLUMN_START,
                XmNleftOffset,		0,
                XmNtopAttachment,       XmATTACH_WIDGET,
                XmNtopWidget,           this->module_label,
	        XmNlabelString,		xms,
                NULL);
    XmStringFree(xms);

    XtAddCallback(this->asynch_tb,
	  XmNvalueChangedCallback,
	  (XtCallbackProc)MBMainWindow_asynchCB,
	  (XtPointer)this);
	

    /* subset of items for outboard */
    xms = XmStringCreateSimple("Executable Name");
    this->outboard_label =
	XtVaCreateManagedWidget
	    ("Executable name",
	     xmLabelWidgetClass,
	     this->mdfform,
	     XmNtopAttachment,	        XmATTACH_WIDGET,
	     XmNtopWidget,	        this->asynch_tb,
	     XmNtopOffset,	        LABEL_TOP_OFFSET,
	     XmNleftAttachment,	        XmATTACH_POSITION,
	     XmNleftPosition,	        THIRD_COLUMN_START,
	     XmNleftOffset,	        0,
	     XmNlabelString,		xms,
	     NULL);
    XmStringFree(xms);


    this->outboard_text =
	XtVaCreateManagedWidget
	    ("outboardText",
	     xmTextWidgetClass,
	     this->mdfform,
	     XmNtopAttachment,		XmATTACH_WIDGET,
	     XmNtopWidget,		this->asynch_tb,
	     XmNtopOffset,		LABEL_TOP_OFFSET,
	     XmNleftAttachment,		XmATTACH_WIDGET,
	     XmNleftWidget,		this->outboard_label,
	     XmNrightAttachment,	XmATTACH_FORM,
             XmNwidth,                  220,
	     NULL);


    XtAddCallback(this->outboard_text,
	  XmNmodifyVerifyCallback,
	  (XtCallbackProc)MBMainWindow_isIdentifierCB,
	  (XtPointer)this);

    XtAddCallback(this->outboard_text,
	  XmNmodifyVerifyCallback,
	  (XtCallbackProc)MBMainWindow_setDirtyCB,
	  (XtPointer)this);

    xms = XmStringCreateSimple("Outboard Host");
    this->host_label=
        XtVaCreateManagedWidget("Host",
                xmLabelWidgetClass,     this->mdfform,
                XmNleftAttachment,      XmATTACH_POSITION,
                XmNleftPosition,        THIRD_COLUMN_START,
                XmNleftOffset,		0,
                XmNtopAttachment,       XmATTACH_WIDGET,
                XmNtopWidget,           this->outboard_label,
	        XmNlabelString,		xms,
                NULL);
    XmStringFree(xms);

    this->host_text=
        XtVaCreateManagedWidget("host_text",
                xmTextWidgetClass,      this->mdfform,
                XmNrightAttachment,     XmATTACH_FORM,
                XmNleftAttachment,      XmATTACH_WIDGET,
                XmNleftWidget,		this->host_label,
                XmNtopAttachment,       XmATTACH_WIDGET,
                XmNtopWidget,           this->outboard_label,
                XmNwidth,               220,
                NULL);


    xms = XmStringCreateSimple("Persistent");
    this->persistent_tb=
    XtVaCreateManagedWidget("asynchronousToggle",
                xmToggleButtonWidgetClass, this->mdfform,
                XmNleftAttachment,      XmATTACH_POSITION,
                XmNleftPosition,        THIRD_COLUMN_START,
                XmNleftOffset,		0,
                XmNtopAttachment,       XmATTACH_WIDGET,
                XmNtopWidget,           this->host_label,
	        XmNlabelString,		xms,
                NULL);
    XmStringFree(xms);



    this->notifyOutboardStateChange(False);



    xms = XmStringCreateSimple("Include File Name");
    this->includefile_tb =
	XtVaCreateManagedWidget
	    ("Include File",
	     xmToggleButtonWidgetClass,
	     this->mdfform,
	     XmNtopAttachment,		XmATTACH_WIDGET,
	     XmNtopWidget,		this->persistent_tb,
	     XmNleftAttachment,		XmATTACH_POSITION,
	     XmNleftPosition,		THIRD_COLUMN_START,
             XmNleftOffset,		0,
	     XmNlabelString,		xms,
	     NULL);
    XmStringFree(xms);

    XtAddCallback(this->includefile_tb,
	  XmNvalueChangedCallback,
	  (XtCallbackProc)MBMainWindow_includefileCB,
	  (XtPointer)this);

    this->includefile_text =
	XtVaCreateManagedWidget
	    ("includefileText",
	     xmTextWidgetClass,
	     this->mdfform,
	     XmNtopAttachment,		XmATTACH_WIDGET,
	     XmNtopWidget,		this->persistent_tb,
	     XmNrightAttachment,	XmATTACH_FORM,
	     XmNleftAttachment,		XmATTACH_WIDGET,
	     XmNleftWidget,		this->includefile_tb,
	     XmNwidth,			220,
	     NULL);

    XtAddCallback(this->includefile_text,
          XmNmodifyVerifyCallback,
          (XtCallbackProc)MBMainWindow_isFileNameCB,
          (XtPointer)this);

    XtAddCallback(this->includefile_text,
          XmNmodifyVerifyCallback,
          (XtCallbackProc)MBMainWindow_setDirtyCB,
          (XtPointer)this);

    this->setTextSensitivity(this->includefile_text, False);

    sep = XtVaCreateManagedWidget
	    ("categoryText",
	     xmSeparatorWidgetClass,
	     this->mdfform,
	     XmNtopAttachment,		XmATTACH_WIDGET,
	     XmNtopWidget,		this->num_outputs_label,
	     XmNleftAttachment,		XmATTACH_FORM,
	     XmNrightAttachment,	XmATTACH_FORM,
	     NULL);

    xms = XmStringCreateSimple("Individual Parameter Information");
    inout_label =
	XtVaCreateManagedWidget
	    ("Inputs/Outputs",
	     xmLabelWidgetClass,
	     this->mdfform,
	     XmNtopAttachment,		XmATTACH_WIDGET,
	     XmNtopWidget,		sep,
	     XmNleftAttachment,		XmATTACH_FORM,
	     XmNrightAttachment,	XmATTACH_FORM,
	     XmNlabelString,		xms,
	     NULL);
    XmStringFree(xms);

    xms = XmStringCreateSimple("Input or Output?");
    inorout_label =
	XtVaCreateManagedWidget
	    ("Inputs or Outputs",
	     xmLabelWidgetClass,
	     this->mdfform,
	     XmNtopAttachment,		XmATTACH_WIDGET,
	     XmNtopWidget,		inout_label,
	     XmNleftAttachment,		XmATTACH_FORM,
	     XmNlabelString,		xms,
	     NULL);
    XmStringFree(xms);

    pulldown_menu = this->createIOPulldown(this->mdfform);

    n = 0;
    XtSetArg(wargs[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(wargs[n], XmNtopWidget, inorout_label); n++;
    XtSetArg(wargs[n], XmNtopOffset, 0); n++;
    XtSetArg(wargs[n], XmNleftAttachment, XmATTACH_FORM); n++;
    XtSetArg(wargs[n], XmNsubMenuId, pulldown_menu); n++;
    this->io_om = XmCreateOptionMenu(this->mdfform, "io", wargs, n);

    XtManageChild(this->io_om);

    xms = XmStringCreateSimple("Number");
    this->param_num_label =
	XtVaCreateManagedWidget
	    ("Number",
	     xmLabelWidgetClass,
	     this->mdfform,
	     XmNbottomAttachment,	XmATTACH_OPPOSITE_WIDGET,
	     XmNbottomWidget,		this->io_om,
	     XmNbottomOffset, 	        5,
	     XmNleftAttachment,		XmATTACH_WIDGET,
	     XmNleftWidget,		this->io_om,
	     XmNlabelString,		xms,
	     NULL);
    XmStringFree(xms);

    this->param_num_stepper = XtVaCreateManagedWidget
	    ("inputNum",
	     xmStepperWidgetClass,
	     this->mdfform,
	     XmNbottomAttachment,	XmATTACH_OPPOSITE_WIDGET,
	     XmNbottomWidget,		this->io_om,
	     XmNbottomOffset,		5,
	     XmNleftAttachment,		XmATTACH_WIDGET,
	     XmNleftWidget,		this->param_num_label,
	     XmNrightAttachment,	XmATTACH_NONE,
	     XmNdataType,		INTEGER,
	     XmNiMinimum,		1,
	     XmNiMaximum,		1,
	     XmNdigits,			5,
	     XmNrecomputeSize,		False,
	     NULL);

    XtAddCallback(this->param_num_stepper,
	  XmNactivateCallback,
	  (XtCallbackProc)MBMainWindow_paramNumberCB,
	  (XtPointer)this);
	
    xms = XmStringCreateSimple("Name");
    this->name_label =
	XtVaCreateManagedWidget
	    ("Name",
	     xmLabelWidgetClass,
	     this->mdfform,
	     XmNtopAttachment,		XmATTACH_WIDGET,
	     XmNtopWidget,		this->param_num_stepper,
	     XmNtopOffset,		LABEL_TOP_OFFSET,
	     XmNalignment,		XmALIGNMENT_BEGINNING,
	     XmNleftAttachment,		XmATTACH_FORM,
	     XmNlabelString,		xms,
	     NULL);
    XmStringFree(xms);

    this->name_text =
	XtVaCreateManagedWidget
	    ("inputNameText",
	     xmTextWidgetClass,
	     this->mdfform,
	     XmNtopAttachment,		XmATTACH_WIDGET,
	     XmNtopWidget,		this->param_num_stepper,
	     XmNleftAttachment,		XmATTACH_WIDGET,
	     XmNleftWidget,		this->module_name_label,
	     XmNrightAttachment,	XmATTACH_POSITION,
	     XmNrightPosition,		FIRST_COLUMN_END,
	     NULL);

    XtAddCallback(this->name_text,
	  XmNmodifyVerifyCallback,
	  (XtCallbackProc)MBMainWindow_isIdentifierCB,
	  (XtPointer)this);

    XtAddCallback(this->name_text,
	  XmNmodifyVerifyCallback,
	  (XtCallbackProc)MBMainWindow_paramNameCB,
	  (XtPointer)this);
	
    xms = XmStringCreateSimple("Description");
    this->param_description_label =
	XtVaCreateManagedWidget
	    ("Description ",
	     xmLabelWidgetClass,
	     this->mdfform,
	     XmNtopAttachment,		XmATTACH_WIDGET,
	     XmNtopWidget,		this->name_text,
	     XmNtopOffset,		LABEL_TOP_OFFSET,
	     XmNleftAttachment,		XmATTACH_FORM,
	     XmNlabelString,		xms,
	     NULL);
    XmStringFree(xms);

    this->param_description_text =
	XtVaCreateManagedWidget
	    ("descriptionText",
	     xmTextWidgetClass,
	     this->mdfform,
	     XmNtopAttachment,		XmATTACH_WIDGET,
	     XmNtopWidget,		this->name_text,
	     XmNleftAttachment,		XmATTACH_OPPOSITE_WIDGET,
	     XmNleftWidget,		this->name_text,
	     XmNleftOffset,		0,
	     XmNalignment,		XmALIGNMENT_BEGINNING,
	     XmNrightAttachment,	XmATTACH_POSITION,
	     XmNrightPosition,		FIRST_COLUMN_END,
	     NULL);

    XtAddCallback(this->param_description_text,
	  XmNmodifyVerifyCallback,
	  (XtCallbackProc)MBMainWindow_paramDescriptionCB,
	  (XtPointer)this);
	
    xms = XmStringCreateSimple("Required");
    this->required_tb =
	XtVaCreateManagedWidget
	    ("Required",
	     xmToggleButtonWidgetClass,
	     this->mdfform,
	     XmNtopAttachment,		XmATTACH_WIDGET,
	     XmNtopWidget,		this->param_description_text,
	     XmNleftAttachment,		XmATTACH_FORM,
	     XmNlabelString,		xms,
	     NULL);
    XmStringFree(xms);

    XtAddCallback(this->required_tb,
	  XmNvalueChangedCallback,
	  (XtCallbackProc)MBMainWindow_requiredCB,
	  (XtPointer)this);
	
    xms = XmStringCreateSimple("Default value");
    this->default_value_label =
	XtVaCreateManagedWidget
	    ("Default value",
	     xmLabelWidgetClass,
	     this->mdfform,
	     XmNtopAttachment,		XmATTACH_WIDGET,
	     XmNtopWidget,		this->required_tb,
	     XmNtopOffset,		LABEL_TOP_OFFSET,
	     XmNalignment,		XmALIGNMENT_BEGINNING,
	     XmNleftAttachment,		XmATTACH_FORM,
	     XmNlabelString,		xms,
	     NULL);
    XmStringFree(xms);

    this->default_value_text =
	XtVaCreateManagedWidget
	    ("inputDefaultValueText",
	     xmTextWidgetClass,
	     this->mdfform,
	     XmNtopAttachment,		XmATTACH_WIDGET,
	     XmNtopWidget,		this->required_tb,
	     XmNleftAttachment,		XmATTACH_WIDGET,
	     XmNleftWidget,		this->default_value_label,
	     XmNrightAttachment,	XmATTACH_POSITION,
	     XmNrightPosition,		FIRST_COLUMN_END,
	     NULL);

    XtAddCallback(this->default_value_text,
	  XmNmodifyVerifyCallback,
	  (XtCallbackProc)MBMainWindow_paramDefaultValueCB,
	  (XtPointer)this);
	
    xms = XmStringCreateSimple("Descriptive");
    this->descriptive_tb =
	XtVaCreateManagedWidget
	    ("Descriptive",
	     xmToggleButtonWidgetClass,
	     this->mdfform,
	     XmNtopAttachment,		XmATTACH_WIDGET,
	     XmNtopWidget,		this->default_value_text,
	     XmNleftAttachment,		XmATTACH_FORM,
	     XmNlabelString,		xms,
	     NULL);
    XmStringFree(xms);

    XtAddCallback(this->descriptive_tb,
	  XmNvalueChangedCallback,
	  (XtCallbackProc)MBMainWindow_descriptiveCB,
	  (XtPointer)this);

   /* a label for the radio button */
    xms = XmStringCreateSimple("Object Type");
    this->object_type_label =
	XtVaCreateManagedWidget
	    ("Object Type",
	     xmLabelWidgetClass,
	     this->mdfform,
	     XmNtopAttachment,		XmATTACH_WIDGET,
	     XmNtopWidget,		descriptive_tb,
	     XmNleftAttachment,		XmATTACH_FORM,
	     XmNlabelString,		xms,
	     NULL);
    XmStringFree(xms);
	

    /* the new radio button */
    n = 0;
    XtSetArg(wargs[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(wargs[n], XmNtopWidget, object_type_label); n++;
    XtSetArg(wargs[n], XmNtopOffset, 5); n++;
    XtSetArg(wargs[n], XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
    XtSetArg(wargs[n], XmNleftWidget, descriptive_tb); n++;
    XtSetArg(wargs[n], XmNleftOffset, 25); n++;
    this->input_type_rb = XmCreateRadioBox(this->mdfform,
                                           "input type", wargs, n);
    XtManageChild(this->input_type_rb);

    xms = XmStringCreateSimple("field object");
    this->field_tb = XtVaCreateManagedWidget("field_tb",
                       xmToggleButtonWidgetClass, this->input_type_rb,
                       XmNlabelString,      xms, 
                       NULL);
    XmStringFree(xms);
    XtAddCallback(this->field_tb,
          XmNvalueChangedCallback,
          (XtCallbackProc)MBMainWindow_fieldCB,
          (XtPointer)this);

    xms = XmStringCreateSimple("simple parameter");
    this->simple_param_tb = XtVaCreateManagedWidget("simple_param_tb",
                       xmToggleButtonWidgetClass, this->input_type_rb,
                       XmNlabelString,      xms, 
                       NULL);
    XmStringFree(xms);
    XtAddCallback(this->simple_param_tb,
          XmNvalueChangedCallback,
          (XtCallbackProc)MBMainWindow_simpleparamCB,
          (XtPointer)this);

    this->createFieldOptions();
    this->createSimpleParameterOptions();

    XtSetSensitive(this->element_type_label, False);
    XtSetSensitive(this->element_type_om, False);

    this->paramToWids(this->getCurrentParam());

    theMBApplication->setClean();

    //
    // return the toplevel wid
    //
    return topform;
}


void MBMainWindow::unmanageFieldOptions()
{
    XtUnmanageChild(this->fieldForm);
}
void MBMainWindow::manageFieldOptions()
{
    XtManageChild(this->fieldForm);
}
void MBMainWindow::unmanageSimpleParameterOptions()
{
    XtUnmanageChild(this->simpleParameterForm);
}
void MBMainWindow::manageSimpleParameterOptions()
{
    XtManageChild(this->simpleParameterForm);
}
void MBMainWindow::createFieldOptions()
{
    XmString  xms;
    Widget    pulldown_menu;
    int n;
    Arg	      wargs[50];

    fieldForm = XtVaCreateWidget("fieldForm", xmFormWidgetClass, this->mdfform,
               	        XmNtopAttachment, 	XmATTACH_WIDGET,
               	        XmNtopWidget,     	this->inout_label,
               	        XmNleftAttachment,	XmATTACH_POSITION,
	       	        XmNleftPosition,	SECOND_COLUMN_START,
               	        XmNrightAttachment,	XmATTACH_FORM,
	     		NULL);

    xms = XmStringCreateSimple("Field Parameter Options");
    this->field_label =
	XtVaCreateManagedWidget
	    ("Field Parameters",
	     xmLabelWidgetClass,
	     this->fieldForm,
	     XmNtopAttachment,		XmATTACH_FORM,
	     XmNleftAttachment,		XmATTACH_FORM,
	     XmNlabelString,		xms,
	     NULL);
    XmStringFree(xms);


    pulldown_menu = this->createDataTypePulldown(this->mdfform);


    n = 0;
    XtSetArg(wargs[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(wargs[n], XmNtopWidget, this->field_label); n++;
    XtSetArg(wargs[n], XmNrightAttachment, XmATTACH_POSITION); n++;
    XtSetArg(wargs[n], XmNrightPosition, OPTION_MENU_END); n++;
    XtSetArg(wargs[n], XmNsubMenuId, pulldown_menu); n++;
    this->data_type_om = XmCreateOptionMenu(this->fieldForm, "om", wargs, n);

    XtManageChild(this->data_type_om);

    xms = XmStringCreateSimple("Data type");
    this->data_type_label =
	XtVaCreateManagedWidget
	    ("Data type",
	     xmLabelWidgetClass,
	     this->fieldForm,
	     XmNtopAttachment,		XmATTACH_WIDGET,
	     XmNtopWidget,		this->field_label,
	     XmNtopOffset,		LABEL_TOP_OFFSET,
	     XmNrightAttachment,	XmATTACH_WIDGET,
	     XmNrightWidget,		this->data_type_om,
	     XmNlabelString,		xms,
	     NULL);
    XmStringFree(xms);

    pulldown_menu = this->createDataShapePulldown(this->mdfform);

    n = 0;
    XtSetArg(wargs[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(wargs[n], XmNtopWidget, this->data_type_om); n++;
    XtSetArg(wargs[n], XmNrightAttachment, XmATTACH_POSITION); n++;
    XtSetArg(wargs[n], XmNrightPosition, OPTION_MENU_END); n++;
    XtSetArg(wargs[n], XmNsubMenuId, pulldown_menu); n++;
    this->data_shape_om = XmCreateOptionMenu(this->fieldForm, "om", wargs, n);

    XtManageChild(this->data_shape_om);

    xms = XmStringCreateSimple("Data Shape");
    this->data_shape_label =
	XtVaCreateManagedWidget
	    ("Data Shape",
	     xmLabelWidgetClass,
	     this->fieldForm,
	     XmNtopAttachment,		XmATTACH_WIDGET,
	     XmNtopWidget,		this->data_type_om,
	     XmNtopOffset,		LABEL_TOP_OFFSET,
	     XmNrightAttachment,	XmATTACH_WIDGET,
	     XmNrightWidget,		this->data_shape_om,
	     XmNlabelString,		xms,
	     NULL);
    XmStringFree(xms);

    pulldown_menu = this->createPositionsPulldown(this->mdfform);

    n = 0;
    XtSetArg(wargs[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(wargs[n], XmNtopWidget, this->data_shape_om); n++;
    XtSetArg(wargs[n], XmNrightAttachment, XmATTACH_POSITION); n++;
    XtSetArg(wargs[n], XmNrightPosition, OPTION_MENU_END); n++;
    XtSetArg(wargs[n], XmNsubMenuId, pulldown_menu); n++;
    this->positions_om = XmCreateOptionMenu(this->fieldForm, "om", wargs, n);

    XtManageChild(this->positions_om);

    xms = XmStringCreateSimple("Positions");
    this->positions_label =
	XtVaCreateManagedWidget
	    ("Positions",
	     xmLabelWidgetClass,
	     this->fieldForm,
	     XmNtopAttachment,		XmATTACH_WIDGET,
	     XmNtopWidget,		this->data_shape_om,
	     XmNtopOffset,		LABEL_TOP_OFFSET,
	     XmNrightAttachment,	XmATTACH_WIDGET,
	     XmNrightWidget,		this->positions_om,
	     XmNlabelString,		xms,
	     NULL);
    XmStringFree(xms);

    pulldown_menu = this->createConnectionsPulldown(this->mdfform);

    n = 0;
    XtSetArg(wargs[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(wargs[n], XmNtopWidget, this->positions_om); n++;
    XtSetArg(wargs[n], XmNrightAttachment, XmATTACH_POSITION); n++;
    XtSetArg(wargs[n], XmNrightPosition, OPTION_MENU_END); n++;
    XtSetArg(wargs[n], XmNsubMenuId, pulldown_menu); n++;
    this->connections_om = XmCreateOptionMenu(this->fieldForm, "om", wargs, n);

    XtManageChild(this->connections_om);

    xms = XmStringCreateSimple("Connections");
    this->connections_label =
	XtVaCreateManagedWidget
	    ("Connections",
	     xmLabelWidgetClass,
	     this->fieldForm,
	     XmNtopAttachment,		XmATTACH_WIDGET,
	     XmNtopWidget,		this->positions_om,
	     XmNtopOffset,		LABEL_TOP_OFFSET,
	     XmNrightAttachment,	XmATTACH_WIDGET,
	     XmNrightWidget,		this->connections_om,
	     XmNlabelString,		xms,
	     NULL);
    XmStringFree(xms);

    pulldown_menu = this->createElementTypePulldown(this->mdfform);

    n = 0;
    XtSetArg(wargs[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(wargs[n], XmNtopWidget, this->connections_om); n++;
    XtSetArg(wargs[n], XmNrightAttachment, XmATTACH_POSITION); n++;
    XtSetArg(wargs[n], XmNrightPosition, OPTION_MENU_END); n++;
    XtSetArg(wargs[n], XmNsubMenuId, pulldown_menu); n++;
    this->element_type_om = XmCreateOptionMenu(this->fieldForm, "om", wargs, n);

    XtManageChild(this->element_type_om);

    xms = XmStringCreateSimple("Element type");
    this->element_type_label =
	XtVaCreateManagedWidget
	    ("Element type",
	     xmLabelWidgetClass,
	     this->fieldForm,
	     XmNtopAttachment,		XmATTACH_WIDGET,
	     XmNtopWidget,		this->connections_om,
	     XmNtopOffset,		LABEL_TOP_OFFSET,
	     XmNrightAttachment,	XmATTACH_WIDGET,
	     XmNrightWidget,		this->element_type_om,
	     XmNlabelString,		xms,
	     NULL);
    XmStringFree(xms);

    pulldown_menu = this->createDependencyPulldown(this->fieldForm);

    n = 0;
    XtSetArg(wargs[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(wargs[n], XmNtopWidget, this->element_type_om); n++;
    XtSetArg(wargs[n], XmNrightAttachment, XmATTACH_POSITION); n++;
    XtSetArg(wargs[n], XmNrightPosition, OPTION_MENU_END); n++;
    XtSetArg(wargs[n], XmNsubMenuId, pulldown_menu); n++;
    this->dependency_om = XmCreateOptionMenu(this->fieldForm, "om", wargs, n);

    XtManageChild(this->dependency_om);

    xms = XmStringCreateSimple("Dependency");
    this->dependency_label =
	XtVaCreateManagedWidget
	    ("Dependency",
	     xmLabelWidgetClass,
	     this->fieldForm,
	     XmNtopAttachment,		XmATTACH_WIDGET,
	     XmNtopWidget,		this->element_type_om,
	     XmNtopOffset,		LABEL_TOP_OFFSET,
	     XmNrightAttachment,	XmATTACH_WIDGET,
	     XmNrightWidget,		this->dependency_om,
	     XmNlabelString,		xms,
	     NULL);
    XmStringFree(xms);
}
void MBMainWindow::createSimpleParameterOptions()
{
    XmString  xms;
    Widget    pulldown_menu;
    int n;
    Arg	      wargs[50];

    simpleParameterForm = XtVaCreateWidget("simpleParameterForm", 
                        xmFormWidgetClass, this->mdfform,
               	        XmNtopAttachment, 	XmATTACH_WIDGET,
               	        XmNtopWidget,     	this->inout_label,
               	        XmNleftAttachment,	XmATTACH_POSITION,
	       	        XmNleftPosition,	SECOND_COLUMN_START,
               	        XmNrightAttachment,	XmATTACH_FORM,
	     		NULL);

    xms = XmStringCreateSimple("Simple Parameter Options");
    simple_param_label =
	XtVaCreateManagedWidget
	    ("Simple Parameter Options",
	     xmLabelWidgetClass,
	     this->simpleParameterForm,
	     XmNtopAttachment,		XmATTACH_FORM,
	     XmNleftAttachment,		XmATTACH_FORM,
	     XmNlabelString,		xms,
	     NULL);
    XmStringFree(xms);

    xms = XmStringCreateSimple("Type");
    type_label =
	XtVaCreateManagedWidget
	    ("Type(s)",
	     xmLabelWidgetClass,
	     this->simpleParameterForm,
	     XmNtopAttachment,		XmATTACH_WIDGET,
	     XmNtopWidget,		simple_param_label,
	     XmNtopOffset,		LABEL_TOP_OFFSET,
	     XmNleftAttachment,		XmATTACH_FORM,
	     XmNlabelString,		xms,
	     NULL);
    XmStringFree(xms);

    type_rc = CreateTypeList();
    XtVaSetValues(this->type_rc,
	     XmNtopAttachment,		XmATTACH_WIDGET,
	     XmNtopWidget,		type_label,
	     XmNleftAttachment,		XmATTACH_WIDGET,
	     XmNleftWidget,	 	type_label,		
	     NULL);

    pulldown_menu = this->createDataTypePulldown(this->simpleParameterForm);

/*
    n = 0;
    XtSetArg(wargs[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(wargs[n], XmNtopWidget, type_rc); n++;
    XtSetArg(wargs[n], XmNrightAttachment, XmATTACH_FORM); n++;
    XtSetArg(wargs[n], XmNsubMenuId, pulldown_menu); n++;
    simple_data_type_om = XmCreateOptionMenu(this->simpleParameterForm, 
                                                   "om", wargs, n);

    XtManageChild(simple_data_type_om);

    xms = XmStringCreateSimple("Data type");
    simple_data_type_label =
	XtVaCreateManagedWidget
	    ("Data type",
	     xmLabelWidgetClass,
	     this->simpleParameterForm,
	     XmNtopAttachment,		XmATTACH_WIDGET,
	     XmNtopWidget,		type_rc,
	     XmNtopOffset,		LABEL_TOP_OFFSET,
	     XmNrightAttachment,	XmATTACH_WIDGET,
	     XmNrightWidget,		simple_data_type_om,
	     XmNlabelString,		xms,
	     NULL);
    XmStringFree(xms);
*/
    pulldown_menu = this->createSimpleDataShapePulldown(this->simpleParameterForm);


    xms = XmStringCreateSimple("Vector Length");
    simple_data_shape_label =
	XtVaCreateManagedWidget
	    ("Data Shape",
	     xmLabelWidgetClass,
	     this->simpleParameterForm,
	     XmNtopAttachment,		XmATTACH_WIDGET,
	     XmNtopWidget,		type_rc,
	     XmNtopOffset,		LABEL_TOP_OFFSET,
	     XmNleftAttachment,		XmATTACH_WIDGET,
	     XmNleftWidget,		type_label,
	     XmNlabelString,		xms,
	     NULL);
    XmStringFree(xms);

    n = 0;
    XtSetArg(wargs[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(wargs[n], XmNtopWidget, type_rc); n++;
    XtSetArg(wargs[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(wargs[n], XmNleftWidget, simple_data_shape_label); n++;
    XtSetArg(wargs[n], XmNsubMenuId, pulldown_menu); n++;
    simple_data_shape_om = 
                        XmCreateOptionMenu(this->simpleParameterForm,
                                                     "om", wargs, n);

    XtManageChild(simple_data_shape_om);
}

Widget MBMainWindow::createIOPulldown(Widget parent)
{
    int    i;
    char   *name[2];
    Widget w;
    XmString xms;

    name[0] = "Input";
    name[1] = "Output";
    Widget pdm = XmCreatePulldownMenu(parent, "pdm", NULL, 0);

    for(i = 0; i < 2; i++)
    {
	
	xms = XmStringCreateSimple(name[i]);
	w = XtVaCreateManagedWidget(name[i], 
		xmPushButtonWidgetClass, pdm, 
		XmNlabelString, xms,
		NULL);
	XmStringFree(xms);

	XtAddCallback(w,
	      XmNactivateCallback,
	      (XtCallbackProc)MBMainWindow_IOCB,
	      (XtPointer)this);
    }
	

    return pdm;
}
Widget MBMainWindow::createDataTypePulldown(Widget parent)
{
    int    i;
    Widget w;
    char   *name[8];
    XmString xms;

    name[0] = "float"; 
    name[1] = "double";
    name[2] = "int";   
    name[3] = "short";
    name[4] = "uint";  
    name[5] = "ushort";
    name[6] = "ubyte";
    name[7] = "byte";
    Widget pdm = XmCreatePulldownMenu(parent, "pdm", NULL, 0);

    for(i = 0; i < 8; i++)
    {
	xms = XmStringCreateSimple(name[i]);
	w = XtVaCreateManagedWidget(name[i],
		xmPushButtonWidgetClass,pdm,
		XmNrecomputeSize, 	False,
		XmNwidth,		OPTION_MENU_WIDTH,
		XmNlabelString,		xms,
		NULL);
	XmStringFree(xms);

	XtAddCallback(w,
	      XmNactivateCallback,
	      (XtCallbackProc)MBMainWindow_dataTypeCB,
	      (XtPointer)this);
    }

    return pdm;
}
Widget MBMainWindow::createSimpleDataShapePulldown(Widget parent)
{
    int    i;
    char   *name[10];
    Widget w;
    XmString xms;

    name[0] = "1-vector";
    name[1] = "2-vector";
    name[2] = "3-vector";
    name[3] = "4-vector";
    name[4] = "5-vector";
    name[5] = "6-vector";
    name[6] = "7-vector";
    name[7] = "8-vector";
    name[8] = "9-vector";
    Widget pdm = XmCreatePulldownMenu(parent, "pdm", NULL, 0);

    for(i = 0; i < 9; i++)
    {
	xms = XmStringCreateSimple(name[i]);
	w = XtVaCreateManagedWidget(name[i], xmPushButtonWidgetClass, pdm, 
		XmNrecomputeSize, 	False,
		XmNwidth,		OPTION_MENU_WIDTH,
		XmNlabelString,		xms,
		NULL);
	XmStringFree(xms);

	XtAddCallback(w,
	      XmNactivateCallback,
	      (XtCallbackProc)MBMainWindow_simpledataShapeCB,
	      (XtPointer)this);
    }

    return pdm;
}
Widget MBMainWindow::createDataShapePulldown(Widget parent)
{
    int    i;
    char   *name[10];
    Widget w;
    XmString xms;

    name[0] = "Scalar";
    name[1] = "1-vector";
    name[2] = "2-vector";
    name[3] = "3-vector";
    name[4] = "4-vector";
    name[5] = "5-vector";
    name[6] = "6-vector";
    name[7] = "7-vector";
    name[8] = "8-vector";
    name[9] = "9-vector";
    Widget pdm = XmCreatePulldownMenu(parent, "pdm", NULL, 0);

    for(i = 0; i < 10; i++)
    {
	xms = XmStringCreateSimple(name[i]);
	w = XtVaCreateManagedWidget(name[i], xmPushButtonWidgetClass, pdm, 
		XmNrecomputeSize, 	False,
		XmNwidth,		OPTION_MENU_WIDTH,
		XmNlabelString,		xms,
		NULL);
	XmStringFree(xms);

	XtAddCallback(w,
	      XmNactivateCallback,
	      (XtCallbackProc)MBMainWindow_dataShapeCB,
	      (XtPointer)this);
    }

    return pdm;
}
Widget MBMainWindow::createPositionsPulldown(Widget parent)
{
    int    i;
    char   *name[3];
    Widget w;
    XmString xms;

    name[0] = "Not required";
    name[1] = "Regular";
    name[2] = "Irregular";
    Widget pdm = XmCreatePulldownMenu(parent, "pdm", NULL, 0);

    for(i = 0; i < 3; i++)
    {
	xms = XmStringCreateSimple(name[i]);
	w = XtVaCreateManagedWidget(name[i], xmPushButtonWidgetClass, pdm, 
		XmNrecomputeSize, 	False,
		XmNwidth,		OPTION_MENU_WIDTH,
		XmNlabelString,		xms,
		NULL);
	XmStringFree(xms);

	XtAddCallback(w,
	      XmNactivateCallback,
	      (XtCallbackProc)MBMainWindow_positionsCB,
	      (XtPointer)this);
    }

    return pdm;
}
Widget MBMainWindow::createConnectionsPulldown(Widget parent)
{
    int    i;
    char   *name[3];
    Widget w;
    XmString xms;

    name[0] = "Not required";
    name[1] = "Regular";
    name[2] = "Irregular";
    Widget pdm = XmCreatePulldownMenu(parent, "pdm", NULL, 0);

    for(i = 0; i < 3; i++)
    {
	xms = XmStringCreateSimple(name[i]);
	w = XtVaCreateManagedWidget(name[i], xmPushButtonWidgetClass, pdm, 
		XmNrecomputeSize, 	False,
		XmNwidth,		OPTION_MENU_WIDTH,
		XmNlabelString,		xms,
		NULL);
	XmStringFree(xms);

	XtAddCallback(w,
	      XmNactivateCallback,
	      (XtCallbackProc)MBMainWindow_connectionsCB,
	      (XtPointer)this);
    }

    return pdm;
}
Widget MBMainWindow::createElementTypePulldown(Widget parent)
{
    int    i;
    char   *name[6];
    Widget w;
    XmString xms;

    name[0] = "Not required";
    name[1] = "Lines";
    name[2] = "Quads";
    name[3] = "Cubes";
    name[4] = "Triangles";
    name[5] = "Tetrahedra";
    Widget pdm = XmCreatePulldownMenu(parent, "pdm", NULL, 0);

    for(i = 0; i < 6; i++)
    {
	xms = XmStringCreateSimple(name[i]);
	w = XtVaCreateManagedWidget(name[i], xmPushButtonWidgetClass, pdm, 
		XmNrecomputeSize, 	False,
		XmNwidth,		OPTION_MENU_WIDTH,
		XmNlabelString,		xms,
		NULL);
	XmStringFree(xms);

	XtAddCallback(w,
	      XmNactivateCallback,
	      (XtCallbackProc)MBMainWindow_elementTypeCB,
	      (XtPointer)this);
    }

    return pdm;
}
Widget MBMainWindow::createDependencyPulldown(Widget parent)
{
    int    i;
    char   *name[3];
    Widget w;
    XmString xms;

    name[0] = "Positions or connections";
    name[1] = "Positions only";
    name[2] = "Connections only";
    Widget pdm = XmCreatePulldownMenu(parent, "pdm", NULL, 0);

    for(i = 0; i < 3; i++)
    {
	xms = XmStringCreateSimple(name[i]);
	w = XtVaCreateManagedWidget(name[i], xmPushButtonWidgetClass, pdm, 
		XmNrecomputeSize, 	False,
		XmNwidth,		OPTION_MENU_WIDTH,
		XmNlabelString,		xms,
		NULL);
	XmStringFree(xms);

	XtAddCallback(w,
	      XmNactivateCallback,
	      (XtCallbackProc)MBMainWindow_dependencyCB,
	      (XtPointer)this);
    }

    return pdm;
}

Widget MBMainWindow::CreateTypeList()
{
    Widget rc;
    int    i;
    XmString xms;
    Arg	      wargs[50];

    this->param_type[0].name = "integer";
    this->param_type[0].type = INTEGER_TYPE;
    this->param_type[1].name = "integer vector";
    this->param_type[1].type = INTEGER_VECTOR_TYPE;
    this->param_type[2].name = "scalar";
    this->param_type[2].type = SCALAR_TYPE;
    this->param_type[3].name = "vector";
    this->param_type[3].type = VECTOR_TYPE;
    this->param_type[4].name = "flag";
    this->param_type[4].type = FLAG_TYPE;
    this->param_type[5].name = "integer list";
    this->param_type[5].type = INTEGER_LIST_TYPE;
    this->param_type[6].name = "integer vector list";
    this->param_type[6].type = INTEGER_VECTOR_LIST_TYPE;
    this->param_type[7].name = "scalar list";
    this->param_type[7].type = SCALAR_LIST_TYPE;
    this->param_type[8].name = "vector list";
    this->param_type[8].type = VECTOR_LIST_TYPE;
    this->param_type[9].name = "string";
    this->param_type[9].type = STRING_TYPE;
    this->param_type[10].name = "field";
    this->param_type[10].type = FIELD_TYPE;

    rc = XtVaCreateManagedWidget(
		"typeList",
		xmRowColumnWidgetClass,
		this->simpleParameterForm,
		XmNpacking, XmPACK_COLUMN,
		XmNnumColumns, 2,
		XmNradioBehavior, TRUE,
		XmNradioAlwaysOne, FALSE,
		NULL); 

     
    for(i = 0; i < num_simple_types; i++)
    {
	xms = XmStringCreateSimple(this->param_type[i].name);
	this->param_type[i].wid = XtVaCreateManagedWidget(
		this->param_type[i].name,
		xmToggleButtonWidgetClass,
		rc,
		XmNlabelString,		xms,
		NULL);
	XmStringFree(xms);

	XtAddCallback(this->param_type[i].wid,
		XmNvalueChangedCallback,
		(XtCallbackProc)MBMainWindow_typeCB,
		(XtPointer)this);
    }
    return rc;
}
void MBMainWindow::CreateExistingCategoryPopup()
{
    char   *name[20];
    Widget wid;
    int    i;
    XmString xms;

    name[0] = "Annotation";
    name[1] = "DXLink";
    name[2] = "Debugging";
    name[3] = "Flow Control";
    name[4] = "Import and Export";
    name[5] = "Interactor"; 
    name[6] = "Interface Control"; 
    name[7] = "Realization";
    name[8] = "Rendering";
    name[9] = "Special";
    name[10] = "Structuring";
    name[11] = "Transformation";
    name[12] = NULL; 

    //
    // Get rid of any existing tranlstions on the label
    //
    XtUninstallTranslations(this->existing_category);

    this->exisiting_popup_menu = 
	XmCreatePopupMenu(this->existing_category, "popup", NULL, 0);
    XtVaSetValues(this->exisiting_popup_menu, XmNwhichButton, 1, NULL);
    for(i = 0; i < 12; i++)
    {
	if (name[i]) {
	    xms = XmStringCreateSimple(name[i]);
	    wid = XtVaCreateManagedWidget
	       (name[i], 
		xmPushButtonWidgetClass,
		this->exisiting_popup_menu, 
		XmNlabelString,		xms,
		NULL);
	    XmStringFree(xms);

	    XtAddCallback(wid,
	      XmNactivateCallback,
	      (XtCallbackProc)MBMainWindow_existingCB,
	      (XtPointer)this);
	}
	
    }
    XtAddEventHandler(this->existing_category, ButtonPressMask, False, 
	(XtEventHandler)MBMainWindow_PostExistingPopupEH, (XtPointer)this);
}
extern "C" void MBMainWindow_PostExistingPopupEH(Widget , XtPointer client, 
					XEvent *e, Boolean *)
{
    MBMainWindow *mbmw = (MBMainWindow *)client;

    XmMenuPosition(mbmw->exisiting_popup_menu, (XButtonPressedEvent *)e);
    XtManageChild(mbmw->exisiting_popup_menu);
}
extern "C" void MBMainWindow_setDirtyCB(Widget , XtPointer , XtPointer)
{
    theMBApplication->setDirty();
}
extern "C" void MBMainWindow_InputTypeCB(Widget , XtPointer , XtPointer)
{
}
extern "C" void MBMainWindow_isIdentifierCB(Widget w , XtPointer , XtPointer cd)
{
    XmTextVerifyCallbackStruct *cbs = (XmTextVerifyCallbackStruct *)cd;
    int i;
    int len = cbs->text->length;

    if(cbs->doit == False) return;

    for(i = 0; i < len; i++)
    {
	if(!((cbs->text->ptr[i] >= '0' AND cbs->text->ptr[i] <= '9') OR
             (cbs->text->ptr[i] >= 'a' AND cbs->text->ptr[i] <= 'z') OR
             (cbs->text->ptr[i] >= 'A' AND cbs->text->ptr[i] <= 'Z') OR
              cbs->text->ptr[i] == '_' OR
              cbs->text->ptr[i] == '@'))
	{
	    cbs->doit = False;
	    return;
	}
    }
}
extern "C" void MBMainWindow_isFileNameCB(Widget w , XtPointer , XtPointer cd)
{
    XmTextVerifyCallbackStruct *cbs = (XmTextVerifyCallbackStruct *)cd;
    int i;
    int len = cbs->text->length;

    if(cbs->doit == False) return;

    for(i = 0; i < len; i++)
    {
	if(!((cbs->text->ptr[i] >= '0' AND cbs->text->ptr[i] <= '9') OR
             (cbs->text->ptr[i] >= 'a' AND cbs->text->ptr[i] <= 'z') OR
             (cbs->text->ptr[i] >= 'A' AND cbs->text->ptr[i] <= 'Z') OR
              cbs->text->ptr[i] == '_' OR
              cbs->text->ptr[i] == '/' OR
              cbs->text->ptr[i] == '.' OR
              cbs->text->ptr[i] == '@'))
	{
	    cbs->doit = False;
	    return;
	}
    }
}
extern "C" void MBMainWindow_existingCB(Widget w, XtPointer clientData, XtPointer)
{
    MBMainWindow *mbmw = (MBMainWindow *) clientData;

    XmTextSetString(mbmw->category_text, XtName(w));
    theMBApplication->setDirty();
}
extern "C" void MBMainWindow_numInputsCB(Widget, XtPointer clientData, XtPointer call)
{
    Widget      wid;
    WidgetList  children;
    int         num_children;
    int         i;
    int         num;
    char        name_string[64];
    MBParameter *param;
    XmDoubleCallbackStruct *cbs = (XmDoubleCallbackStruct *)call;
    if(cbs->reason != XmCR_ACTIVATE) return;

    MBMainWindow *mbmw = (MBMainWindow *) clientData;
    int current_num_inputs = mbmw->inputParamList.getSize();
    int num_outputs;

    XtVaGetValues(mbmw->num_inputs_stepper, XmNiValue, &num, NULL);

    
    XtVaGetValues(mbmw->num_outputs_stepper, XmNiValue, &num_outputs, NULL);
    if((num == 0) && (num_outputs == 0))
    {
	ErrorMessage("The number of inputs and outputs can't both be set to zero\nThe number of inputs has been reset to 1");
	num = 1;
	XtVaSetValues(mbmw->num_inputs_stepper, XmNiValue, num, NULL);
    }

    if(current_num_inputs < num)
    {
	int i;
	for(i = current_num_inputs; i < num; i++)
	{
	    mbmw->inputParamList.appendElement(new MBParameter);
	    param = (MBParameter *)mbmw->inputParamList.getElement(i+1);
            sprintf(name_string,"input_%d",i+1);
            param->setName(name_string);
	}
    }

    //
    // Get the io option menu widgets
    //
    XtVaGetValues(mbmw->io_om, XmNsubMenuId, &wid, NULL);
    XtVaGetValues(wid,
		  XmNchildren, &children,
		  XmNnumChildren, &num_children,
		  NULL);
    ASSERT(num_children == 2);

    if(num == 0)
    {
	mbmw->is_input = False;

	for(i = 0; i < num_children; i++)
	{
	    if(EqualString(XtName(children[i]), "Input"))
		XtSetSensitive(children[i], False);
	    if(EqualString(XtName(children[i]), "Output"))
		XtVaSetValues(mbmw->io_om, XmNmenuHistory, children[i], NULL);
	}
    }
    else
    {
	for(i = 0; i < num_children; i++)
	{
	    if(EqualString(XtName(children[i]), "Input"))
		XtSetSensitive(children[i], True);
	}
    }

    mbmw->setParamNumStepper();
    theMBApplication->setDirty();
}
extern "C" void MBMainWindow_numOutputsCB(Widget, XtPointer clientData, XtPointer call)
{
    Widget      wid;
    WidgetList  children;
    int         num_children;
    int         i;
    char        name_string[64];
    MBParameter *param;
    XmDoubleCallbackStruct *cbs = (XmDoubleCallbackStruct *)call;
    if(cbs->reason != XmCR_ACTIVATE) return;

    MBMainWindow *mbmw = (MBMainWindow *) clientData;
    int current_num_outputs = mbmw->outputParamList.getSize();
    int num_inputs;
    int num;

    XtVaGetValues(mbmw->num_outputs_stepper, XmNiValue, &num, NULL);

    XtVaGetValues(mbmw->num_inputs_stepper, XmNiValue, &num_inputs, NULL);
    if((num == 0) && (num_inputs == 0))
    {
	ErrorMessage("The number of inputs and outputs can't both be set to zero\nThe number of outputs has been reset to 1");
	num = 1;
	XtVaSetValues(mbmw->num_outputs_stepper, XmNiValue, num, NULL);
    }

    if(current_num_outputs < num)
    {
	int i;
	for(i = current_num_outputs; i < num; i++)
	{
	    mbmw->outputParamList.appendElement(new MBParameter);
            param = (MBParameter *)mbmw->outputParamList.getElement(i+1);
            sprintf(name_string,"output_%d",i+1);
            param->setName(name_string);

	}
    }

    //
    // Get the io option menu widgets
    //
    XtVaGetValues(mbmw->io_om, XmNsubMenuId, &wid, NULL);
    XtVaGetValues(wid,
		  XmNchildren, &children,
		  XmNnumChildren, &num_children,
		  NULL);
    ASSERT(num_children == 2);

    if(num == 0)
    {
	mbmw->is_input = True;

	for(i = 0; i < num_children; i++)
	{
	    if(EqualString(XtName(children[i]), "Output"))
		XtSetSensitive(children[i], False);
	    if(EqualString(XtName(children[i]), "Input"))
		XtVaSetValues(mbmw->io_om, XmNmenuHistory, children[i], NULL);
	}
    }
    else
    {
	for(i = 0; i < num_children; i++)
	{
	    if(EqualString(XtName(children[i]), "Output"))
		XtSetSensitive(children[i], True);
	}
    }

    mbmw->setParamNumStepper();
    theMBApplication->setDirty();
}
extern "C" void MBMainWindow_paramNumberCB(Widget, XtPointer clientData, XtPointer call)
{
    XmDoubleCallbackStruct *cbs = (XmDoubleCallbackStruct *)call;
    if(cbs->reason != XmCR_ACTIVATE) return;

    MBMainWindow *mbmw = (MBMainWindow *) clientData;
    MBParameter    *param;
    int          num;

    XtVaGetValues(mbmw->param_num_stepper, XmNiValue, &num, NULL);
    if(mbmw->is_input)
	ASSERT(num <= mbmw->inputParamList.getSize());
    else
	ASSERT(num <= mbmw->outputParamList.getSize());

    if(mbmw->is_input)
	param = (MBParameter *)mbmw->inputParamList.getElement(num);
    else
	param = (MBParameter *)mbmw->outputParamList.getElement(num);

    mbmw->paramToWids(param);
    theMBApplication->setDirty();
}
extern "C" void MBMainWindow_IOCB(Widget w, XtPointer clientData, XtPointer)
{
    MBMainWindow *mbmw = (MBMainWindow *) clientData;

    mbmw->is_input = EqualString("Input", XtName(w));
    mbmw->setParamNumStepper();
    theMBApplication->setDirty();
}
extern "C" void MBMainWindow_requiredCB(Widget, XtPointer clientData, XtPointer)
{
    MBMainWindow *mbmw = (MBMainWindow *) clientData;
    MBParameter    *param;

    mbmw->setRequiredSens();
    param = mbmw->getCurrentParam();
    param->setRequired(XmToggleButtonGetState(mbmw->required_tb));
    theMBApplication->setDirty();
}
extern "C" void MBMainWindow_descriptiveCB(Widget, XtPointer clientData, XtPointer)
{
    MBMainWindow *mbmw = (MBMainWindow *) clientData;
    MBParameter    *param;

    param = mbmw->getCurrentParam();
    param->setDescriptive(XmToggleButtonGetState(mbmw->descriptive_tb));
    theMBApplication->setDirty();
}
void MBMainWindow::setRequiredSens()
{

    if(!this->is_input)
    {
	XtSetSensitive(this->required_tb, False);
	this->setTextSensitivity(this->default_value_text, False);
	XtSetSensitive(this->default_value_label, False);
	XtSetSensitive(this->descriptive_tb, False);
        /* need to grey out all the "list" types for output */
        /* since we don't know the size to allocate */
        XtSetSensitive(this->param_type[5].wid, False); 
        XtSetSensitive(this->param_type[6].wid, False); 
        XtSetSensitive(this->param_type[7].wid, False); 
        XtSetSensitive(this->param_type[8].wid, False); 
        XtSetSensitive(this->param_type[9].wid, False); 
    }
    else
    {
	//
	// Don't let anyone change input param 1 from being required!
	//
	int param_number;
	XtVaGetValues(this->param_num_stepper, XmNiValue, &param_number, NULL);
	XtSetSensitive(this->required_tb, param_number != 1);

	Boolean set = XmToggleButtonGetState(this->required_tb);
	this->setTextSensitivity(this->default_value_text, !set);
	XtSetSensitive(this->default_value_label, !set);
	XtSetSensitive(this->descriptive_tb, !set);
        XtSetSensitive(this->param_type[5].wid, True); 
        XtSetSensitive(this->param_type[6].wid, True); 
        XtSetSensitive(this->param_type[7].wid, True); 
        XtSetSensitive(this->param_type[8].wid, True); 
        XtSetSensitive(this->param_type[9].wid, True); 
    }
}
void MBMainWindow::setElementTypeSens()
{
    Widget	wid;
    WidgetList	children;
    int		num_children;
    int		i;

    XtVaGetValues(this->element_type_om, XmNsubMenuId, &wid, NULL);
    XtVaGetValues(wid, 
		  XmNchildren, &children, 
		  XmNnumChildren, &num_children,
		  NULL);
    XtVaGetValues(this->connections_om, XmNmenuHistory, &wid, NULL);

    ASSERT(num_children == 6);

    if(EqualString(XtName(wid), "Not required"))
    {
	XtSetSensitive(this->element_type_om, False);
	XtSetSensitive(this->element_type_label, False);
    }
    else if(EqualString(XtName(wid), "Irregular"))
    {
	XtSetSensitive(this->element_type_om, True);
	XtSetSensitive(this->element_type_label, True);
	for(i = 0; i < 6; i++)
	    XtSetSensitive(children[i], True);
    }
    else
    {
	XtSetSensitive(this->element_type_om, True);
	XtSetSensitive(this->element_type_label, True);
	for(i = 0; i < 6; i++)
	{
	    if(EqualString(XtName(children[i]), "Triangles") ||
	       EqualString(XtName(children[i]), "Tetrahedra"))
		XtSetSensitive(children[i], False);
	    else
		XtSetSensitive(children[i], True);
	}
    }
}
void MBMainWindow::setParamNumStepper()
{
    int		value;
    MBParameter	*param;
    int		num;

    XtVaGetValues(
	this->is_input ? this->num_inputs_stepper : this->num_outputs_stepper,
	XmNiValue, &value,
	NULL);
    XtVaSetValues(this->param_num_stepper, XmNiMaximum, value, NULL);

    XtVaGetValues(this->param_num_stepper, XmNiValue, &num, NULL);
    if(this->is_input)
	ASSERT(num <= this->inputParamList.getSize());
    else
	ASSERT(num <= this->outputParamList.getSize());

    if(this->is_input)
	param = (MBParameter *)this->inputParamList.getElement(num);
    else
	param = (MBParameter *)this->outputParamList.getElement(num);

    this->paramToWids(param);

    this->setPositionsConnectionsSens();
}

void MBMainWindow::setPositionsConnectionsSens()
{

    Boolean is_value = XmToggleButtonGetState(this->simple_param_tb);

    if(is_value)
    {
	XtSetSensitive(this->positions_label, !is_value);
	XtSetSensitive(this->positions_om, !is_value);
	XtSetSensitive(this->connections_label, !is_value);
	XtSetSensitive(this->connections_om, !is_value);
	return;
    }

    int num;
    XtVaGetValues(this->param_num_stepper, XmNiValue, &num, NULL);

    XtSetSensitive(this->positions_label,   this->is_input && (num == 1));
    XtSetSensitive(this->positions_om,      this->is_input && (num == 1));
    XtSetSensitive(this->connections_label, this->is_input && (num == 1));
    XtSetSensitive(this->connections_om,    this->is_input && (num == 1));
}

extern "C" void MBMainWindow_fieldCB(Widget, XtPointer clientData, XtPointer)
{
    MBMainWindow *mbmw = (MBMainWindow *) clientData;
    MBParameter *param = mbmw->getCurrentParam();
    Boolean set = XmToggleButtonGetState(mbmw->field_tb);

    if (set)
    {
      mbmw->manageFieldOptions();
      param->setStructure("Field/Group");
      param->setType(FIELD_TYPE);
      param->setTypeName("field");
    }
    else
    {
      mbmw->unmanageFieldOptions();
      param->setStructure("Value");
    }
}
extern "C" void MBMainWindow_simpleparamCB(Widget, XtPointer clientData, XtPointer)
{ 
    MBMainWindow *mbmw = (MBMainWindow *) clientData;
    MBParameter *param = mbmw->getCurrentParam();
    Boolean set = XmToggleButtonGetState(mbmw->simple_param_tb);
    if (set)
    {
      mbmw->manageSimpleParameterOptions();
      param->setStructure("Value");
    }
    else
    {
      mbmw->unmanageSimpleParameterOptions();
      param->setStructure("Field/Group");
      param->setType(FIELD_TYPE);
      param->setTypeName("field");
    }
    mbmw->paramToWids(param); 
}
extern "C" void MBMainWindow_includefileCB(Widget, XtPointer clientData, XtPointer)
{
    MBMainWindow *mbmw = (MBMainWindow *) clientData;

    Boolean set = XmToggleButtonGetState(mbmw->includefile_tb);

    mbmw->setTextSensitivity(mbmw->includefile_text, set);

    theMBApplication->setDirty();
}
extern "C" void MBMainWindow_outboardCB(Widget, XtPointer clientData, XtPointer)
{
    MBMainWindow *mbmw = (MBMainWindow *) clientData;

    Boolean set = XmToggleButtonGetState(mbmw->outboard_tb);

    mbmw->notifyOutboardStateChange(set); 

    theMBApplication->setDirty();

}
extern "C" void MBMainWindow_runtimeCB(Widget, XtPointer clientData, XtPointer)
{
    MBMainWindow *mbmw = (MBMainWindow *) clientData;

    Boolean set = XmToggleButtonGetState(mbmw->runtime_tb);

    mbmw->notifyLoadableStateChange(set); 

    theMBApplication->setDirty();

}

extern "C" void MBMainWindow_asynchCB(Widget, XtPointer clientData, XtPointer)
{
    MBMainWindow *mbmw = (MBMainWindow *) clientData;

    Boolean set = XmToggleButtonGetState(mbmw->asynch_tb);
    Boolean outboard = XmToggleButtonGetState(mbmw->outboard_tb);

    if (outboard && set)
       XmToggleButtonSetState(mbmw->persistent_tb, True, False);

    theMBApplication->setDirty();

}

extern "C" void MBMainWindow_dataTypeCB(Widget w, XtPointer clientData, XtPointer)
{
    MBMainWindow *mbmw = (MBMainWindow *) clientData;
    MBParameter *param = mbmw->getCurrentParam();
    param->setDataType(XtName(w));
    theMBApplication->setDirty();
}
extern "C" void MBMainWindow_simpledataShapeCB(Widget w, XtPointer clientData, XtPointer)
{
    MBMainWindow *mbmw = (MBMainWindow *) clientData;
    MBParameter *param = mbmw->getCurrentParam();
    param->setDataShape(XtName(w));
    theMBApplication->setDirty();
}
extern "C" void MBMainWindow_dataShapeCB(Widget w, XtPointer clientData, XtPointer)
{
    MBMainWindow *mbmw = (MBMainWindow *) clientData;
    MBParameter *param = mbmw->getCurrentParam();
    param->setDataShape(XtName(w));
    theMBApplication->setDirty();
}
extern "C" void MBMainWindow_positionsCB(Widget w, XtPointer clientData,XtPointer)
{
    MBMainWindow *mbmw = (MBMainWindow *) clientData;
    MBParameter *param = mbmw->getCurrentParam();
    param->setPositions(XtName(w));
    theMBApplication->setDirty();
}
extern "C" void MBMainWindow_connectionsCB(Widget w, XtPointer clientData,XtPointer)
{
    MBMainWindow *mbmw = (MBMainWindow *) clientData;
    MBParameter *param = mbmw->getCurrentParam();
    param->setConnections(XtName(w));
    mbmw->setElementTypeSens();
    theMBApplication->setDirty();
}
extern "C" void MBMainWindow_elementTypeCB(Widget w, XtPointer clientData,XtPointer)
{
    MBMainWindow *mbmw = (MBMainWindow *) clientData;
    MBParameter *param = mbmw->getCurrentParam();
    param->setElementType(XtName(w));
    theMBApplication->setDirty();
}
extern "C" void MBMainWindow_dependencyCB(Widget w, XtPointer clientData, XtPointer)
{
    MBMainWindow *mbmw = (MBMainWindow *) clientData;
    MBParameter *param = mbmw->getCurrentParam();
    param->setDependency(XtName(w));
    theMBApplication->setDirty();
}
extern "C" void MBMainWindow_typeCB(Widget w, XtPointer clientData, XtPointer)
{
    int i;
    MBMainWindow *mbmw = (MBMainWindow *) clientData;
    Boolean set = XmToggleButtonGetState(w);
    char *name = XtName(w);
    MBParameter *param = mbmw->getCurrentParam();

    for(i = 0; i < num_simple_types; i++)
    {
	if(EqualString(name,mbmw->param_type[i].name))
	{
	    if(set) 
            {
		param->setType(mbmw->param_type[i].type);
		param->setTypeName(mbmw->param_type[i].name);
            }
	    break;
	}
    }
    if(EqualString(name,"scalar")&&set)
    {
      param->setDataType("float");
      param->setDataShape("Scalar");
      XtSetSensitive(mbmw->simple_data_shape_om, False);
      XtSetSensitive(mbmw->simple_data_shape_label, False);
    }
    if(EqualString(name,"string")&&set)
    {
      param->setDataType("string");
      param->setDataShape("Scalar");
      XtSetSensitive(mbmw->simple_data_shape_om, False);
      XtSetSensitive(mbmw->simple_data_shape_label, False);
    }
    else if(EqualString(name,"scalar list")&&set)
    {
      param->setDataType("float");
      param->setDataShape("Scalar");
      XtSetSensitive(mbmw->simple_data_shape_om, False);
      XtSetSensitive(mbmw->simple_data_shape_label, False);
    }
    else if(EqualString(name,"vector")&&set)
    {
      param->setDataType("float");
      param->setDataShape("2-vector");
      SET_OM_NAME(mbmw->simple_data_shape_om,       "2-vector");
      XtSetSensitive(mbmw->simple_data_shape_om, True);
      XtSetSensitive(mbmw->simple_data_shape_label, True);
    }
    else if(EqualString(name,"vector list")&&set)
    {
      param->setDataType("float");
      param->setDataShape("2-vector");
      SET_OM_NAME(mbmw->simple_data_shape_om,       "2-vector");
      XtSetSensitive(mbmw->simple_data_shape_om, True);
      XtSetSensitive(mbmw->simple_data_shape_label, True);
    }
    else if(EqualString(name,"integer")&&set)
    {
      param->setDataType("int");
      param->setDataShape("Scalar");
      XtSetSensitive(mbmw->simple_data_shape_om, False);
      XtSetSensitive(mbmw->simple_data_shape_label, False);
    }
    else if(EqualString(name,"integer list")&&set)
    {
      param->setDataType("int");
      param->setDataShape("Scalar");
      XtSetSensitive(mbmw->simple_data_shape_om, False);
      XtSetSensitive(mbmw->simple_data_shape_label, False);
    }
    else if(EqualString(name,"integer vector")&&set)
    {
      param->setDataType("int");
      param->setDataShape("2-vector");
      SET_OM_NAME(mbmw->simple_data_shape_om,       "2-vector");
      XtSetSensitive(mbmw->simple_data_shape_om, True);
      XtSetSensitive(mbmw->simple_data_shape_label, True);
    }
    else if(EqualString(name,"integer vector list")&&set)
    {
      param->setDataType("int");
      param->setDataShape("2-vector");
      SET_OM_NAME(mbmw->simple_data_shape_om,       "2-vector");
      XtSetSensitive(mbmw->simple_data_shape_om, True);
      XtSetSensitive(mbmw->simple_data_shape_label, True);
    }
    else if(EqualString(name,"flag")&&set)
    {
      param->setDataType("int");
      param->setDataShape("2-vector");
      XtSetSensitive(mbmw->simple_data_shape_om, False);
      XtSetSensitive(mbmw->simple_data_shape_label, False);
    }
    theMBApplication->setDirty();
}
extern "C" void MBMainWindow_paramNameCB(Widget w, XtPointer clientData, XtPointer call)
{
    int i;
    char tmp[4096];
    char *text_str;
    int ptr;
    MBMainWindow *mbmw = (MBMainWindow *) clientData;
    XmTextVerifyCallbackStruct *cbs = (XmTextVerifyCallbackStruct *)call;

    if(mbmw->ignore_cb) return;
    if(cbs->doit == False) return;


    //
    // We must construct the new string (in tmp)
    //
    text_str = XmTextGetString(w);
    ASSERT(STRLEN(text_str) + cbs->text->length < 4096);
    ptr = 0;
    for(i = 0; i < cbs->startPos; i++)
    {
	tmp[ptr] = text_str[ptr];
	ptr++;
    }
    for(i = 0; i < cbs->text->length; i++)
	tmp[ptr++] = cbs->text->ptr[i];
    for(i = (int)cbs->startPos; i < STRLEN(text_str); i++)
	tmp[ptr++] = text_str[i];
    tmp[ptr++] = '\0';

    MBParameter *param = mbmw->getCurrentParam();
    param->setName(tmp);
    XtFree( text_str);
    theMBApplication->setDirty();
}
extern "C" void MBMainWindow_paramDescriptionCB(Widget w, XtPointer clientData, 
		XtPointer call)
{
    int i;
    char tmp[4096];
    char *text_str;
    int ptr;
    MBMainWindow *mbmw = (MBMainWindow *) clientData;
    XmTextVerifyCallbackStruct *cbs = (XmTextVerifyCallbackStruct *)call;

    if(mbmw->ignore_cb) return;

    //
    // We must construct the new string (in tmp)
    //
    text_str = XmTextGetString(w);
    ASSERT(STRLEN(text_str) + cbs->text->length < 4096);
    ptr = 0;
    for(i = 0; i < cbs->startPos; i++)
    {
	tmp[ptr] = text_str[ptr];
	ptr++;
    }
    for(i = 0; i < cbs->text->length; i++)
	tmp[ptr++] = cbs->text->ptr[i];
    for(i = (int)cbs->startPos; i < STRLEN(text_str); i++)
	tmp[ptr++] = text_str[i];
    tmp[ptr++] = '\0';

    MBParameter *param = mbmw->getCurrentParam();
    param->setDescription(tmp);
    theMBApplication->setDirty();
}
extern "C" void MBMainWindow_paramDefaultValueCB(Widget w, XtPointer clientData,
			XtPointer call)
{
    int i;
    char tmp[4096];
    char *text_str;
    int ptr;
    MBMainWindow *mbmw = (MBMainWindow *) clientData;
    XmTextVerifyCallbackStruct *cbs = (XmTextVerifyCallbackStruct *)call;

    if(mbmw->ignore_cb) return;

    //
    // We must construct the new string (in tmp)
    //
    text_str = XmTextGetString(w);
    ASSERT(STRLEN(text_str) + cbs->text->length < 4096);
    ptr = 0;
    for(i = 0; i < cbs->startPos; i++)
    {
	tmp[ptr] = text_str[ptr];
	ptr++;
    }
    for(i = 0; i < cbs->text->length; i++)
	tmp[ptr++] = cbs->text->ptr[i];
    for(i = (int)cbs->startPos; i < STRLEN(text_str); i++)
	tmp[ptr++] = text_str[i];
    tmp[ptr++] = '\0';

    MBParameter *param = mbmw->getCurrentParam();
    param->setDefaultValue(tmp);
    theMBApplication->setDirty();
}
MBParameter* MBMainWindow::getCurrentParam()
{
    int param_number;

    XtVaGetValues(this->param_num_stepper, XmNiValue, &param_number, NULL);

    if(this->is_input)
	return (MBParameter *)this->inputParamList.getElement(param_number);
    else
	return (MBParameter *)this->outputParamList.getElement(param_number);
}
void MBMainWindow::setTextSensitivity(Widget w, Boolean sens)
{
    Pixel fg;

    if(sens)
	fg = BlackPixel(theMBApplication->getDisplay(), 0);
    else
	fg = theMBApplication->getInsensitiveColor();

    XtVaSetValues(w, 
	XmNforeground, fg, 
	XmNeditable, sens,
	NULL);
}
void MBMainWindow::clearText(Widget w)
{
    XmTextSetString(w, "");
}

//
// called by IBMMainWindow::CloseCallback().
//
void MBMainWindow::closeWindow()
{
    this->quitCmd->execute();
}

void MBMainWindow::createFileMenu(Widget parent)
{
    ASSERT(parent);

    Widget pulldown;

    //
    // Create "File" menu and options.
    //
    pulldown =
	this->fileMenuPulldown =
	    XmCreatePulldownMenu(parent, "fileMenuPulldown", NUL(ArgList), 0);

    this->fileMenu =
	XtVaCreateManagedWidget
	    ("fileMenu",
	     xmCascadeButtonWidgetClass,
	     parent,
	     XmNsubMenuId, pulldown,
	     NULL);

    this->newOption =
	new ButtonInterface(pulldown,
			    "newOption",
			    this->newCmd);

    this->openOption =
	new ButtonInterface(pulldown, 
                            "openOption", this->openCmd);

    this->saveOption =
	new ButtonInterface(pulldown, 
                            "saveOption", 
                            this->saveCmd);

    this->saveAsOption = new ButtonInterface(pulldown, 
                                             "saveAsOption", 
                                             this->saveAsCmd);

    XtVaCreateManagedWidget
	    ("optionSeparator", xmSeparatorWidgetClass, pulldown, NULL);

    this->quitOption =
       new ButtonInterface(pulldown,"quitOption",this->quitCmd);
}
void MBMainWindow::createEditMenu(Widget parent)
{
    ASSERT(parent);

    Widget pulldown;

    //
    // Create "Edit" menu and options.
    //
    pulldown =
	this->editMenuPulldown =
	    XmCreatePulldownMenu(parent, "editMenuPulldown", NUL(ArgList), 0);

    this->editMenu =
	XtVaCreateManagedWidget
	    ("editMenu",
	     xmCascadeButtonWidgetClass,
	     parent,
	     XmNsubMenuId, pulldown,
	     NULL);

    this->commentOption = new ButtonInterface(pulldown,
					      "commentOption",
					      this->commentCmd);

    this->optionsOption = new ButtonInterface(pulldown,
					      "optionsOption",
					      this->optionsCmd);

}
void MBMainWindow::createBuildMenu(Widget parent)
{
    ASSERT(parent);

    Widget pulldown;

    //
    // Create "Build" menu and options.
    //
    pulldown =
	this->buildMenuPulldown =
	    XmCreatePulldownMenu(parent, "buildMenuPulldown", NUL(ArgList), 0);

    this->buildMenu =
	XtVaCreateManagedWidget
	    ("buildMenu",
	     xmCascadeButtonWidgetClass,
	     parent,
	     XmNsubMenuId, pulldown,
	     NULL);

    this->buildMDFOption = new ButtonInterface(pulldown,
					       "buildMDFOption",
					        this->buildMDFCmd);
    this->buildCOption = new ButtonInterface(pulldown,
					     "buildCOption",
					     this->buildCCmd);
    this->buildMakefileOption = new ButtonInterface(pulldown,
					     "buildMakefileOption",
					     this->buildMakefileCmd);
    this->buildAllOption = new ButtonInterface(pulldown,
					       "buildAllOption",
					        this->buildAllCmd);
    this->buildExecutableOption = new ButtonInterface(pulldown,
					       "buildExecutableOption",
					        this->buildExecutableCmd);
    this->buildRunOption = new ButtonInterface(pulldown,
					       "buildRunOption",
					        this->buildRunCmd);
}

void MBMainWindow::createMenus(Widget parent)
{
    ASSERT(parent);

    //
    // Create the menus.
    //
    this->createFileMenu(parent);
    this->createEditMenu(parent);
    this->createBuildMenu(parent);
    this->createHelpMenu(parent);

}
void MBMainWindow::createHelpMenu(Widget parent)
{
    this->createBaseHelpMenu(parent,TRUE,TRUE);

    XtVaCreateManagedWidget("separator", xmSeparatorWidgetClass,
                                         this->helpMenuPulldown,
                                         NULL);

    this->helpTutorialOption=
            new ButtonInterface(this->helpMenuPulldown, "helpTutorialOption",
                theIBMApplication->helpTutorialCmd);

}
void MBMainWindow::PostSaveAsDialog(MBMainWindow *mbmw, Command *cmd)
{
    mbmw->save_as_fd->setPostCommand(cmd);
    mbmw->save_as_fd->post();
}

void MBMainWindow::newMB(boolean create_default_params)
{
    int i;
    Widget w;
    MBParameter *param;
    WidgetList	children;

    this->ignore_cb = TRUE;
    this->setWindowTitle("Module Builder");
    this->setFilename(NULL);
    this->saveCmd->deactivate();

    if(this->comment_text) 
    {
	delete this->comment_text;
	this->comment_text = NULL;
	if(this->comment_dialog->isManaged())
	    this->comment_dialog->installEditorText(this->comment_text);
    }
    this->clearText(module_name_text);
    this->clearText(category_text);
    this->clearText(mod_description_text);
    this->clearText(name_text);
    this->clearText(param_description_text);
    this->clearText(default_value_text);
    this->clearText(outboard_text);
    this->clearText(includefile_text);
    XmToggleButtonSetState(this->field_tb,TRUE,TRUE);
    XmToggleButtonSetState(this->inboard_tb,TRUE,TRUE);
    RESET_OM(this->io_om);
    RESET_OM(this->data_type_om);
    RESET_OM(this->data_shape_om);
    RESET_OM(this->positions_om);
    RESET_OM(this->connections_om);
    RESET_OM(this->element_type_om);
    RESET_OM(this->dependency_om);

    XtVaSetValues(this->num_inputs_stepper, XmNiValue, 1, NULL);
    XtVaSetValues(this->num_outputs_stepper, XmNiValue, 1, NULL);
    XtVaSetValues(this->param_num_stepper, 
		 XmNiValue, 1, 
		 XmNiMaximum, 1,
		 NULL);
    //
    // Get the io option menu widgets
    //
    XtVaGetValues(this->io_om, XmNsubMenuId, &w, NULL);
    XtVaGetValues(w, XmNchildren, &children, NULL);
    for(i = 0; i < 2; i++)
	XtSetSensitive(children[i], True);

    for(i = 1; i < num_simple_types ; i++)
	XmToggleButtonSetState(this->param_type[i].wid, False, False);

    XmToggleButtonSetState(this->outboard_tb, False, True);
    XmToggleButtonSetState(this->includefile_tb, False, True);
    XmToggleButtonSetState(this->required_tb, False, True);
    XmToggleButtonSetState(this->descriptive_tb, False, True);

    for(i = 1; i <= this->inputParamList.getSize(); i++)
    {
	param = (MBParameter *)this->inputParamList.getElement(i);
	delete param;
    }
    for(i = 1; i <= this->outputParamList.getSize(); i++)
    {
	param = (MBParameter *)this->outputParamList.getElement(i);
	delete param;
    }

    this->inputParamList.clear();
    this->outputParamList.clear();


    if(create_default_params)
    {
        MBParameter *iparam = new MBParameter;
        iparam->setRequired(True);
        iparam->setName("input_1");
        this->inputParamList.appendElement(iparam);
        MBParameter *oparam = new MBParameter;
        oparam->setName("output_1");
        this->outputParamList.appendElement(oparam);
         /*
        this->is_input = TRUE;
        this->ignore_cb = FALSE; */
        this->paramToWids(iparam);
    }

    //
    // Dialogs
    //

    this->options_dialog->setPin(FALSE);
    this->options_dialog->setSideEffect(FALSE);


    XmToggleButtonSetState(this->asynch_tb, False, True);
    XmToggleButtonSetState(this->persistent_tb, False, True);
    this->clearText(this->host_text);

    this->is_input = TRUE; 

    this->setElementTypeSens();

    theMBApplication->setClean();
    this->ignore_cb = FALSE;
    this->setRequiredSens();
}
boolean MBMainWindow::saveMB(char *filenm)
{
    int i;
    int j;
    char *str;
    char title[256];
    const char *ext;
    char *file;
    int len = strlen(MBExtension);
    MBParameter *param;
    unsigned long param_type;
    boolean first;
    boolean persistent;
    boolean asychronous;
    boolean pin;
    boolean side_effect;
    char    *host;
    int	    num_inputs;
    int	    num_outputs;

    file = new char[strlen(filenm) + len + 1];
    strcpy(file, filenm);

    // 
    // Build/add the extension
    // 
    ext = strrstr(file, (char *)MBExtension);
    if (!ext || (strlen(ext) != len))
	strcat(file,MBExtension);

    this->setFilename(file);
    this->saveCmd->activate();


    if(!validateMB()) return FALSE;

    std::ofstream to(file);
    if(!to)
	WarningMessage("File open failed on save");

    title[0] = '\0';
    strcat(title, "Module Builder: ");
    strcat(title, file);
    this->setWindowTitle(title);

    i = 0;
    if(STRLEN(this->comment_text) > 0)
    {
	to << "# ";
	while(this->comment_text[i])
	{
	    to.put(this->comment_text[i]);
	    if(this->comment_text[i] == '\n')
		if(this->comment_text[i+1]) to << "# ";
	    i++;
	}
	to << std::endl;
    }

    str = XmTextGetString(this->module_name_text);
    to << "MODULE_NAME = " 
       << str
       << std::endl;
    XtFree(str);

    str = XmTextGetString(this->category_text); 
    to << "CATEGORY = " 
       << str
       << std::endl;
    XtFree(str);

    str = XmTextGetString(this->mod_description_text); 
    to << "MODULE_DESCRIPTION = " 
       << str
       << std::endl;
    XtFree(str);


    if(XmToggleButtonGetState(this->includefile_tb))
    {
        str = XmTextGetString(this->includefile_text); 
	to << "INCLUDE_FILE = " 
	   << str
	   << std::endl;
	XtFree(str);
    }


	
    this->options_dialog->getValues(pin, side_effect);

    asychronous = XmToggleButtonGetState(this->asynch_tb);
    persistent = XmToggleButtonGetState(this->persistent_tb);
    host = XmTextGetString(this->host_text);

    to << "PINNED = ";
    if(pin)
	to << "TRUE";
    else
	to << "FALSE";
    to << std::endl;
	
    to << "SIDE_EFFECT = ";
    if(side_effect)
	to << "TRUE";
    else
	to << "FALSE";
    to << std::endl;

    to << "ASYNCHRONOUS = ";
    if(asychronous)
	to << "TRUE";
    else
	to << "FALSE";
    to << std::endl;

    if(XmToggleButtonGetState(this->runtime_tb))
    {
        str = XmTextGetString(this->outboard_text); 
	to << "LOADABLE_EXECUTABLE = " 
	   << str
	   << std::endl;
	XtFree(str);
    }

    if(XmToggleButtonGetState(this->outboard_tb))
    {
        str = XmTextGetString(this->outboard_text); 
	to << "OUTBOARD_EXECUTABLE = " 
	   << str
	   << std::endl;
	XtFree(str);

	to << "OUTBOARD_PERSISTENT = ";
	if(persistent)
	    to << "TRUE";
	else
	    to << "FALSE";
	to << std::endl;
	
	if(STRLEN(host) > 0)
	    to << "OUTBOARD_HOST = " << host << std::endl;
    }
	
    // Space after module section
    to << std::endl;

    XtVaGetValues(this->num_inputs_stepper, XmNiValue, &num_inputs, NULL);
    for(i = 1; i <= num_inputs; i++)
    {
	param = (MBParameter *)inputParamList.getElement(i);

	to << "INPUT = "
	   << param->getName()
	   << std::endl;

	if(param->getDescription()) {
	    to << "DESCRIPTION = "
	       << param->getDescription()
	       << std::endl;
	   }
	else {
	    to << "DESCRIPTION = "
	       << " "
	       << std::endl;
	}

	to << "REQUIRED = ";
	if(param->getRequired())
	    to << "TRUE";
	else
	    to << "FALSE";
	to << std::endl;

	if(!param->getRequired())
	{
	    to << "DEFAULT_VALUE = "
	       << param->getDefaultValue()
	       << std::endl;

	    to << "DESCRIPTIVE = ";
	    if(param->getDescriptive())
		to << "TRUE";
	    else
		to << "FALSE";
	    to << std::endl;
	}
	param_type = param->getType();
	first = TRUE;
	for(j = 0; j < total_num_types; j++)
	{
	    if(this->param_type[j].type & param_type)
	    {
		if(first)
		{
		    to << "TYPES = "
		       << this->param_type[j].name;
		    first = FALSE;
		}
		else
		{
		    to << ", " 
		       << this->param_type[j].name;
		}
	    }
	}
	if(!first)
	    to << std::endl;

	to << "STRUCTURE = " 
	   << param->getStructure()
	   << std::endl;


	if(!EqualString(param->getStructure(), "Value"))
	{
	    to << "POSITIONS = "
	       << param->getPositions()
	       << std::endl;

	    to << "CONNECTIONS = "
	       << param->getConnections()
	       << std::endl;

	    to << "ELEMENT_TYPE = "
	       << param->getElementType()
	       << std::endl;

	    to << "DEPENDENCY = "
	       << param->getDependency()
	       << std::endl;

 	    to << "DATA_TYPE = "
	       << param->getDataType()
	       << std::endl;

	    to << "DATA_SHAPE = " 
	       << param->getDataShape()
	       << std::endl;
	}
        else 
        {
 	    to << "DATA_TYPE = "
	       << param->getDataType()
	       << std::endl;

	    to << "DATA_SHAPE = "
	       << param->getDataShape()
	       << std::endl;
        }
	to << std::endl;
    }

    XtVaGetValues(this->num_outputs_stepper, XmNiValue, &num_outputs, NULL);
    for(i = 1; i <= num_outputs; i++)
    {
	param = (MBParameter *)outputParamList.getElement(i);

	to << "OUTPUT = "
	   << param->getName()
	   << std::endl;

	if(param->getDescription()) {
	    to << "DESCRIPTION = "
	       << param->getDescription()
	       << std::endl;
	}
	else {
	    to << "DESCRIPTION = "
	       << " "
	       << std::endl;
	}

	param_type = param->getType();
	first = TRUE;
	for(j = 0; j < total_num_types; j++)
	{
	    if(this->param_type[j].type & param_type)
	    {
		if(first)
		{
		    to << "TYPES = "
		       << this->param_type[j].name;
		    first = FALSE;
		}
		else
		{
		    to << ", " 
		       << this->param_type[j].name;
		}
	    }
	}
	if(!first)
	    to << std::endl;

	to << "STRUCTURE = "
	   << param->getStructure()
	   << std::endl;

	to << "DATA_TYPE = "
	   << param->getDataType()
	   << std::endl;

	to << "DATA_SHAPE = "
	   << param->getDataShape()
	   << std::endl;

	if(!EqualString(param->getStructure(), "Value"))
	{
	    to << "POSITIONS = "
	       << param->getPositions()
	       << std::endl;

	    to << "CONNECTIONS = "
	       << param->getConnections()
	       << std::endl;

	    to << "ELEMENT_TYPE = "
	       << param->getElementType()
	       << std::endl;

	    to << "DEPENDENCY = "
	       << param->getDependency()
	       << std::endl;
	}
    to << std::endl;
    }

    theMBApplication->setClean();
    return TRUE;
}
boolean MBMainWindow::validateMB()
{
    int i, j;
    int num_inputs;
    int num_outputs;
    char *str, *type_name;
    unsigned long param_type;
    MBParameter *param, *jparam;

    str =  XmTextGetString(this->module_name_text);
    if(!str || STRLEN(str) == 0)
    {
	WarningMessage("You must enter a valid module name.");
	XtFree(str);
	return False;
    }
    if (!isalpha(str[0]))
    {
	WarningMessage("Module name must start with a letter and contain only numbers or letters.");
	XtFree(str);
	return False;
    }
    for (i=0; i<STRLEN(str); i++)
    {
       if (!isalpha(str[i])&&!isdigit(str[i])) 
       {
	WarningMessage("Module name must start with a letter and contain only numbers or letters.");
	XtFree(str);
	return False;
       }
    }
    XtFree(str);

    str =  XmTextGetString(this->category_text);
    if(!str || STRLEN(str) == 0)
    {
	WarningMessage("You must enter a valid module category.");
	XtFree(str);
	return False;
    }
    XtFree(str);

/*
    str =  XmTextGetString(this->mod_description_text);
    if(!str || STRLEN(str) == 0)
    {
	WarningMessage("You must enter a valid module description.");
	XtFree(str);
	return False;
    }
    XtFree(str);
*/

    if(XmToggleButtonGetState(this->outboard_tb))
    {
	str = XmTextGetString(this->outboard_text);
	if(!str || STRLEN(str) == 0)
	{
	    WarningMessage("When Outboard is selected, an executable name must be provided.");
	    XtFree(str);
	    return False;
	}
	XtFree(str);
    }

    if(XmToggleButtonGetState(this->runtime_tb))
    {
	str = XmTextGetString(this->outboard_text);
	if(!str || STRLEN(str) == 0)
	{
	    WarningMessage("When Runtime-Loadable is selected, an executable name must be provided.");
	    XtFree(str);
	    return False;
	}
	XtFree(str);
    }

    if(XmToggleButtonGetState(this->includefile_tb))
    {
	str = XmTextGetString(this->includefile_text);
	if(!str || STRLEN(str) == 0)
	{
	    WarningMessage("When Include File is selected, a file name must be provided.");
	    XtFree(str);
	    return False;
	}
	XtFree(str);
    }

    //
    // Per parameter statements
    //
    XtVaGetValues(this->num_inputs_stepper, XmNiValue, &num_inputs, NULL);
    XtVaGetValues(this->num_outputs_stepper, XmNiValue, &num_outputs, NULL);

    for(i = 1; i <= num_inputs; i++)
    {
	param = (MBParameter *)inputParamList.getElement(i);

	if(!param->getName() || STRLEN(param->getName()) == 0)
	{
	    WarningMessage("Input parameter %d must have a name.", i);
	    return False;
	}

        for (j=i+1; j<= num_inputs; j++)
        {
            jparam = (MBParameter *)inputParamList.getElement(j);
            if (!strcmp(param->getName(), jparam->getName()))
            {
	      WarningMessage("Duplicate name %s", param->getName());
	      return False;
	    }
        }
        for (j=1; j<= num_outputs; j++)
        {
            jparam = (MBParameter *)outputParamList.getElement(j);
            if (!strcmp(param->getName(), jparam->getName()))
            {
	      WarningMessage("Duplicate name %s", param->getName());
	      return False;
	    }
        }

        /*
	if(!param->getDescription() || STRLEN(param->getDescription()) == 0)
	{
	    WarningMessage("Input parameter %d must have a description.", i);
	    return False;
	}
        */
	if(!param->getRequired())
	{
	    if(!param->getDefaultValue() || 
	       STRLEN(param->getDefaultValue()) == 0)
	    {
		WarningMessage("Input parameter %d must have default value.",i);
		return False;
	    }
	}
/* XXX
        type_name = param->getTypeName();
        Boolean is_value = XmToggleButtonGetState(this->simple_param_tb);
	if (!(strcmp(type_name,"field") && (is_value))) 
	    {
		WarningMessage("Input parameter %d must have a set type.",i);
		return False;
	    }
*/
    }

    for(i = 1; i <= num_outputs; i++)
    {
	param = (MBParameter *)outputParamList.getElement(i);

	if(!param->getName() || STRLEN(param->getName()) == 0)
	{
	    WarningMessage("Output parameter %d must have a name.", i);
	    return False;
	}
       
        /*
	if(!param->getDescription() || STRLEN(param->getDescription()) == 0)
	{
	    WarningMessage("Output parameter %d must have a description.", i);
	    return False;
	}
        */
/*
        type_name = param->getTypeName();
        Boolean is_value = XmToggleButtonGetState(this->simple_param_tb);
	if (!(strcmp(type_name,"field") && (is_value))) 
	    {
		WarningMessage("Output parameter %d must have a set type.",i);
		return False;
	    }
*/
    }
    return True;
}

void MBMainWindow::openMB(char *filenm)
{
    int i;
    int ndx;
    char line[4096];
    int lineno = 0;
    char title[256];
    int num_error = 0;
    MBParameter *param;
    Widget w;
    WidgetList	children;

    param = NULL;

    std::ifstream from(filenm);
    if(!from)
    {
	WarningMessage("File open failed.");
	return;
    }

    //
    // Start with a clean slate
    //
    this->newMB(FALSE);

    this->setFilename(filenm);

    this->saveCmd->activate();

    title[0] = '\0';
    strcat(title, "Module Builder: ");
    strcat(title, filenm);
    this->setWindowTitle(title);

    this->inputParamList.clear();
    this->outputParamList.clear();
    while(from.getline(line, 4096))
    {
	lineno++;
	ndx = 0;
	SkipWhiteSpace(line, ndx);
	if(IsToken(line, "MODULE_NAME", ndx))
	{
	    this->parseModuleName(line);
	}
	else if(IsToken(line, "CATEGORY", ndx))
	{
	    this->parseCategory(line);
	}
	else if(IsToken(line, "MODULE_DESCRIPTION", ndx))
	{
	    this->parseModuleDescription(line);
	}
	else if(IsToken(line, "OUTBOARD_EXECUTABLE", ndx))
	{
	    this->parseOutboardExecutable(line);
	}
	else if(IsToken(line, "LOADABLE_EXECUTABLE", ndx))
	{
	    this->parseLoadableExecutable(line);
	}
	else if(IsToken(line, "OUTBOARD_HOST", ndx))
	{
	    this->parseOutboardHost(line);
	}
	else if(IsToken(line, "OUTBOARD_PERSISTENT", ndx))
	{
	    this->parseOutboardPersistent(line);
	}
	else if(IsToken(line, "ASYNCHRONOUS", ndx))
	{
	    this->parseAsychronous(line);
	}
	else if(IsToken(line, "PINNED", ndx))
	{
	    this->parsePin(line);
	}
	else if(IsToken(line, "INCLUDE_FILE", ndx))
	{
	    this->parseIncludeFileName(line);
	}
	else if(IsToken(line, "SIDE_EFFECT", ndx))
	{
	    this->parseSideEffect(line);
	}
	else if(IsToken(line, "INPUT", ndx))
	{
	    this->inputParamList.appendElement(param = new MBParameter);
	    this->parseParamName(param, line);
	}
	else if(IsToken(line, "OUTPUT", ndx))
	{
	    this->outputParamList.appendElement(param = new MBParameter);
	    this->parseParamName(param, line);
	}
	else if(IsToken(line, "DESCRIPTION", ndx) && param)
	{
	    this->parseParamDescription(param, line);
	}
	else if(IsToken(line, "REQUIRED", ndx) && param)
	{
	    this->parseParamRequired(param, line);
	}
	else if(IsToken(line, "DEFAULT_VALUE", ndx) && param)
	{
	    this->parseParamDefaultValue(param, line);
	}
	else if(IsToken(line, "DESCRIPTIVE", ndx) && param)
	{
	    this->parseParamDescriptive(param, line);
	}
	else if(IsToken(line, "TYPES", ndx) && param)
	{
	    this->parseParamTypes(param, line);
	}
	else if(IsToken(line, "STRUCTURE", ndx) && param)
	{
	    this->parseParamStructure(param, line);
	}
	else if(IsToken(line, "DATA_TYPE", ndx) && param)
	{
	    this->parseParamDataType(param, line);
	}
	else if(IsToken(line, "DATA_SHAPE", ndx) && param)
	{
	    this->parseParamDataShape(param, line);
	}
	else if(IsToken(line, "POSITIONS", ndx) && param)
	{
	    this->parseParamPositions(param, line);
	}
	else if(IsToken(line, "CONNECTIONS", ndx) && param)
	{
	    this->parseParamConnections(param, line);
	}
	else if(IsToken(line, "ELEMENT_TYPE", ndx) && param)
	{
	    this->parseParamElementType(param, line);
	}
	else if(IsToken(line, "DEPENDENCY", ndx) && param)
	{
	    this->parseParamDependency(param, line);
	}
	else if(IsToken(line, "#", ndx))
	{
	    this->parseComment(line);
	}
	else if(STRLEN(&line[ndx]) == 0)
	{
	    ; // newline char
	}
	else
	{
	    num_error++;
	    if(num_error > 3)
	    {
		WarningMessage("Too many errors, giving up");
		this->newMB(TRUE);
		return;
	    }
	    else
	    {
	     WarningMessage("Parse error opening file \"%s\", line number = %d",
		filenm, lineno);
	    }
	}
	XmUpdateDisplay(this->getRootWidget());
    }

    if(this->inputParamList.getSize() > 0)
    {
	param = (MBParameter *)this->inputParamList.getElement(1);
	this->paramToWids(param);
    }
    else if(this->outputParamList.getSize() > 0)
    {
	SET_OM_NAME(this->io_om, "Output");
	this->is_input = False;
	param = (MBParameter *)this->outputParamList.getElement(1);
	this->paramToWids(param);
    }
    else 
    {
	WarningMessage("No inputs or outputs found in file \"%s\"", filenm);
	this->newMB(TRUE);
    }

    if(this->outputParamList.getSize() == 0)
    {
	//
	// Get the io option menu widgets
	//
	XtVaGetValues(this->io_om, XmNsubMenuId, &w, NULL);
	XtVaGetValues(w, XmNchildren, &children, NULL);
	
	for(i = 0; i < 2; i++)
	    if(EqualString(XtName(children[i]), "Output"))
		XtSetSensitive(children[i], False);
    }
    if(this->inputParamList.getSize() == 0)
    {
	//
	// Get the io option menu widgets
	//
	XtVaGetValues(this->io_om, XmNsubMenuId, &w, NULL);
	XtVaGetValues(w, XmNchildren, &children, NULL);
	
	for(i = 0; i < 2; i++)
	    if(EqualString(XtName(children[i]), "Input"))
		XtSetSensitive(children[i], False);
    }

    XtVaSetValues(this->num_inputs_stepper, XmNiValue, 
		  this->inputParamList.getSize(), NULL);

    XtVaSetValues(this->num_outputs_stepper, XmNiValue, 
		  this->outputParamList.getSize(), NULL);

    XtVaSetValues(this->param_num_stepper, 
		  XmNiValue, 	1,
		  XmNiMaximum, 	this->is_input ?
				this->inputParamList.getSize() :
				this->outputParamList.getSize(),
		  		NULL);

    theMBApplication->setClean();
}
void MBMainWindow::setFilename(char *filenm)
{
    if(this->filename)
	delete this->filename;
    if(filenm)
	this->filename = DuplicateString(filenm);
    else
	this->filename = NULL;
}
void MBMainWindow::setCommentText(char *s)
{

    if(this->comment_text)
	delete this->comment_text;
    this->comment_text = DuplicateString(s);
}
void MBMainWindow::parseModuleName(char *line)
{
    int ndx = 0;
    while((line[ndx++] != '=') && line[ndx]);
    SkipWhiteSpace(line, ndx);
    XmTextSetString(this->module_name_text, &line[ndx]);
}
void MBMainWindow::parseCategory(char *line)
{
    int ndx = 0;
    while((line[ndx++] != '=') && line[ndx]);
    SkipWhiteSpace(line, ndx);
    XmTextSetString(this->category_text, &line[ndx]);
}
void MBMainWindow::parseModuleDescription(char *line)
{
    int ndx = 0;
    while((line[ndx++] != '=') && line[ndx]);
    SkipWhiteSpace(line, ndx);
    XmTextSetString(this->mod_description_text, &line[ndx]);
}
void MBMainWindow::parseLoadableExecutable(char *line)
{
    int ndx = 0;
    while((line[ndx++] != '=') && line[ndx]);
    SkipWhiteSpace(line, ndx);
    XmTextSetString(this->outboard_text, &line[ndx]);
    XmToggleButtonSetState(this->runtime_tb, True, True);
    this->notifyLoadableStateChange(True);

}
void MBMainWindow::parseOutboardExecutable(char *line)
{
    int ndx = 0;
    while((line[ndx++] != '=') && line[ndx]);
    SkipWhiteSpace(line, ndx);
    XmTextSetString(this->outboard_text, &line[ndx]);
    XmToggleButtonSetState(this->outboard_tb, True, True);
    this->notifyOutboardStateChange(True);

}
void MBMainWindow::parseIncludeFileName(char *line)
{
    int ndx = 0;
    while((line[ndx++] != '=') && line[ndx]);
    SkipWhiteSpace(line, ndx);
    XmTextSetString(this->includefile_text, &line[ndx]);
    XmToggleButtonSetState(this->includefile_tb, True, True);
}
void MBMainWindow::parseOutboardHost(char *line)
{
    int ndx = 0;
    while((line[ndx++] != '=') && line[ndx]);
    SkipWhiteSpace(line, ndx);
    XmTextSetString(this->host_text, &line[ndx]);
}
void MBMainWindow::parseAsychronous(char *line)
{
    int ndx = 0;
    while((line[ndx++] != '=') && line[ndx]);
    SkipWhiteSpace(line, ndx);
    if(EqualString("TRUE", &line[ndx])) 
    {
	XmToggleButtonSetState(this->asynch_tb, True, False);
    }
    else if(EqualString("FALSE", &line[ndx])) 
    {
	XmToggleButtonSetState(this->asynch_tb, False, False);
    }
    else
	WarningMessage("Parse error in parseAsynchronous");
}
void MBMainWindow::parsePin(char *line)
{
    int ndx = 0;
    while((line[ndx++] != '=') && line[ndx]);
    SkipWhiteSpace(line, ndx);
    if(EqualString("TRUE", &line[ndx])) 
	this->options_dialog->setPin(True);
    else if(EqualString("FALSE", &line[ndx])) 
	this->options_dialog->setPin(False);
    else
	WarningMessage("Parse error in parsePin");
}
void MBMainWindow::parseSideEffect(char *line)
{
    int ndx = 0;
    while((line[ndx++] != '=') && line[ndx]);
    SkipWhiteSpace(line, ndx);
    if(EqualString("TRUE", &line[ndx])) 
	this->options_dialog->setSideEffect(True);
    else if(EqualString("FALSE", &line[ndx])) 
	this->options_dialog->setSideEffect(False);
    else
	WarningMessage("Parse error in parseSideEffect");
}
void MBMainWindow::parseOutboardPersistent(char *line)
{
    int ndx = 0;
    while((line[ndx++] != '=') && line[ndx]);
    SkipWhiteSpace(line, ndx);
    if(EqualString("TRUE", &line[ndx])) 
        XmToggleButtonSetState(this->persistent_tb, True, False);
    else if(EqualString("FALSE", &line[ndx])) 
        XmToggleButtonSetState(this->persistent_tb, False, False);
    else
	WarningMessage("Parse error in parsePersistent");
}
void MBMainWindow::parseParamName(MBParameter *param, char *line)
{
    int ndx = 0;
    while((line[ndx++] != '=') && line[ndx]);
    SkipWhiteSpace(line, ndx);
    param->setName(&line[ndx]);
}
void MBMainWindow::parseParamDescription(MBParameter *param, char *line)
{
    int ndx = 0;
    while((line[ndx++] != '=') && line[ndx]);
    SkipWhiteSpace(line, ndx);
    param->setDescription(&line[ndx]);
}
void MBMainWindow::parseParamRequired(MBParameter *param, char *line)
{
    int ndx = 0;
    while((line[ndx++] != '=') && line[ndx]);
    SkipWhiteSpace(line, ndx);
    if(EqualString("TRUE", &line[ndx])) 
	param->setRequired(True);
    else if(EqualString("FALSE", &line[ndx])) 
	param->setRequired(False);
    else
	WarningMessage("Parse error in parseParamRequired");
}
void MBMainWindow::parseParamDefaultValue(MBParameter *param, char *line)
{
    int ndx = 0;
    while((line[ndx++] != '=') && line[ndx]);
    SkipWhiteSpace(line, ndx);
    param->setDefaultValue(&line[ndx]);
}
void MBMainWindow::parseParamDescriptive(MBParameter *param, char *line)
{
    int ndx = 0;
    while((line[ndx++] != '=') && line[ndx]);
    SkipWhiteSpace(line, ndx);
    if(EqualString("TRUE", &line[ndx])) 
	param->setDescriptive(True);
    else if(EqualString("FALSE", &line[ndx])) 
	param->setDescriptive(False);
    else
	WarningMessage("Parse error in parseParamDescriptive");
}
void MBMainWindow::parseParamTypes(MBParameter *param, char *line)
{
    int ndx = 0;
    int i;
    int foundone;
    char *str;

    foundone=0;
    while((line[ndx++] != '=') && line[ndx]);
    SkipWhiteSpace(line, ndx);
    while((str = this->nextItem(line, ndx)) &&
	  (STRLEN(str) > 0))
    {
	for(i = 0; i < total_num_types; i++)
	{
	    if(EqualString(this->param_type[i].name, str) &&
	       STRLEN(this->param_type[i].name) == STRLEN(str))
	    {
                if (!foundone)
                {
	 	  param->setType(this->param_type[i].type);
	 	  param->setTypeName(this->param_type[i].name);
                  foundone=1;
                }
                else
                {
	          WarningMessage("only first input type found is used");
                }
	        break;
	    }
	}
        if (i==total_num_types) {
           if (!strcmp(str,"camera") || !strcmp(str,"matrix") ||
               !strcmp(str,"value") ||  !strcmp(str,"string") ||
               !strcmp(str,"matrix list") || !strcmp(str,"value list"))
               {
	          WarningMessage("%s is not supported, set to scalar", str);
	 	  param->setType(SCALAR_TYPE);
	 	  param->setTypeName("scalar");
               }
           else 
               {
	          WarningMessage("%s is not supported, set to field", str);
	 	  param->setType(FIELD_TYPE);
	 	  param->setTypeName("field");
               }
        }
	delete str;
    }
}
void MBMainWindow::parseParamStructure(MBParameter *param, char *line)
{
    char *param_name, *type_name;
    unsigned long type;

    int ndx = 0;
    while((line[ndx++] != '=') && line[ndx]);
    SkipWhiteSpace(line, ndx);
    param->setStructure(&line[ndx]);
    /* need to compare against TYPE */
    type = param->getType();
    param_name = param->getName();
    type_name = param->getTypeName();


    if (!strcmp("Field/Group",&line[ndx]))
    {
       if (!type_name || !strcmp(type_name,"")) /* check for NULL   */
       {
          param->setType(FIELD_TYPE);
          param->setTypeName("field");
       }
       else if (type != FIELD_TYPE && type != GROUP_TYPE && type != OBJECT_TYPE) 
       {
	  WarningMessage("%s parameter type \"%s\" incompatible with structure Field/Group; set to field", param_name, type_name);
          param->setType(FIELD_TYPE);
          param->setTypeName("field");
       }
    }
    else 
    {
       if (!strcmp(type_name,""))
       {
          param->setType(SCALAR_TYPE);
          param->setTypeName("scalar");
       }
       if (type != INTEGER_TYPE && type != INTEGER_LIST_TYPE
          && type != SCALAR_TYPE && type != SCALAR_LIST_TYPE
          && type != VECTOR_TYPE && type != VECTOR_LIST_TYPE
          && type != FLAG_TYPE && type != INTEGER_VECTOR_TYPE
          && type != INTEGER_VECTOR_LIST_TYPE && type != STRING_TYPE) 
       {
	  WarningMessage("%s parameter type \"%s\" incompatible with structure Value; set to scalar", param_name, type_name);
          param->setType(SCALAR_TYPE);
          param->setTypeName("scalar");
       }
    }
    
}
void MBMainWindow::parseParamDataType(MBParameter *param, char *line)
{
    int ndx = 0;
    while((line[ndx++] != '=') && line[ndx]);
    SkipWhiteSpace(line, ndx);
    param->setDataType(&line[ndx]);
}
void MBMainWindow::parseParamDataShape(MBParameter *param, char *line)
{
    int ndx = 0;
    while((line[ndx++] != '=') && line[ndx]);
    SkipWhiteSpace(line, ndx);
    param->setDataShape(&line[ndx]);
}
void MBMainWindow::parseParamPositions(MBParameter *param, char *line)
{
    int ndx = 0;
    while((line[ndx++] != '=') && line[ndx]);
    SkipWhiteSpace(line, ndx);
    param->setPositions(&line[ndx]);
}
void MBMainWindow::parseParamConnections(MBParameter *param, char *line)
{
    int ndx = 0;
    while((line[ndx++] != '=') && line[ndx]);
    SkipWhiteSpace(line, ndx);
    param->setConnections(&line[ndx]);
}
void MBMainWindow::parseParamElementType(MBParameter *param, char *line)
{
    int ndx = 0;
    while((line[ndx++] != '=') && line[ndx]);
    SkipWhiteSpace(line, ndx);
    param->setElementType(&line[ndx]);
}
void MBMainWindow::parseParamDependency(MBParameter *param, char *line)
{
    int ndx = 0;
    while((line[ndx++] != '=') && line[ndx]);
    SkipWhiteSpace(line, ndx);
    param->setDependency(&line[ndx]);
}
void MBMainWindow::parseComment(char *line)
{
    char new_string[4096];
    int ndx = 0;

    new_string[0] = '\0';

    if(STRLEN(this->comment_text) > 0)
    {
	strcat(new_string, this->comment_text);
	strcat(new_string, "\n");
    }

    while((line[ndx++] != '#') && line[ndx]);
    SkipWhiteSpace(line, ndx);
    strcat(new_string, &line[ndx]);

    ASSERT(STRLEN(new_string) < 4096);
    if(this->comment_text)
        delete this->comment_text;
    this->comment_text = DuplicateString(new_string);

    if(this->comment_dialog->isManaged())
	this->comment_dialog->installEditorText(this->comment_text);
}
void MBMainWindow::paramToWids(MBParameter *param)
{
    int i;
    unsigned long param_type;

    this->ignore_cb = TRUE;
    XmTextSetString(this->name_text,              param->getName());
    XmTextSetString(this->param_description_text, param->getDescription());
    XmTextSetString(this->default_value_text,     param->getDefaultValue());

    XmToggleButtonSetState(this->required_tb,     param->getRequired(), True);
    XmToggleButtonSetState(this->descriptive_tb,  param->getDescriptive(),True);

    if (!strcmp("Field/Group",param->getStructure())) {
      XmToggleButtonSetState(this->field_tb,  True, True);
      param->setType(FIELD_TYPE);
      param->setTypeName("field");
      SET_OM_NAME(this->data_type_om,               param->getDataType());
      SET_OM_NAME(this->data_shape_om,              param->getDataShape());
      SET_OM_NAME(this->positions_om,               param->getPositions());
      SET_OM_NAME(this->connections_om,             param->getConnections());
      SET_OM_NAME(this->element_type_om,            param->getElementType());
      SET_OM_NAME(this->dependency_om,              param->getDependency());
    }
    else { 
      XmToggleButtonSetState(this->simple_param_tb,  True, True);
      if (!EqualString(param->getDataShape(),"Scalar"))
        SET_OM_NAME(this->simple_data_shape_om,       param->getDataShape());
      param_type = param->getType();
      for(i = 0; i < num_simple_types; i++)
      {
	XmToggleButtonSetState(this->param_type[i].wid,
		(this->param_type[i].type & param_type) ? True : False , False);
      }
    }
    this->ignore_cb = FALSE;

    this->setElementTypeSens();
    this->setRequiredSens();
}
char *MBMainWindow::nextItem(char *line, int &ndx)
{
    int start_index;
    char *str;

    SkipWhiteSpace(line, ndx);

    //
    // If we are at the end, return an empty string.
    //
    if(!line[ndx])
    {
	str = DuplicateString("");
	return str;
    }
    start_index = ndx;
    if(line[ndx] == '"')
    {
	//
	// "Inner loop" to eat up a quoted string w/ white space in it.
	//
	ndx++;
	while(line[ndx] && (line[ndx] != '"'))
	    ndx++;
	ndx++;
    }
    else
    {
	while(!(line[ndx] == ',') && line[ndx])
	{
	    ndx++;
	}
    }

    str = (char *)CALLOC((ndx - start_index) + 2, sizeof(char));
    strncpy(str, &line[start_index], (ndx - start_index));

    //
    // If this was a quoted item, then ndx is currently pointed at the end
    // quote, so move past it, and any "," or white space.
    //
    if(line[start_index] == '"')
    {
	while(IsWhiteSpace(line, ndx) || (line[ndx] == ',')) ndx++;
    }
    else
    {
	if(line[ndx]) ndx++;
    }

    while(IsWhiteSpace(line, ndx) || (line[ndx] == ',') && line[ndx]) ndx++;

    return str;
}
void MBMainWindow::build(int flag)
{
    char          *saved_fname;
    char          *fname;
    char          *path;
    char          *dest_file;
    char	  title[512];
    char	  build_name[512];
    char	  exists_msg[512];
    boolean	  c_exists;
    boolean	  mdf_exists;
    boolean	  makefile_exists;
    boolean	  dirty;

    c_exists = mdf_exists = makefile_exists = FALSE;

    saved_fname = this->getFilename();
    fname = this->getFilename();

    if(!fname)
	fname = XmTextGetString(this->module_name_text);

    if(!fname || IsAllWhiteSpace(fname))
    {
	WarningMessage("Could not determine what name to use to build.");
	return;
    }
    path = GetDirname(fname);
    if(!path || STRLEN(path) == 0)
	path = ".";
    fname = GetFileBaseName(fname, ".mb");


    sprintf(build_name,"%s/%s", theIBMApplication->getTmpDirectory(), fname);

    dirty = theMBApplication->isDirty();
    if(!this->saveMB(build_name))
    {
	title[0] = '\0';
	strcat(title, "Module Builder: ");
	strcat(title, saved_fname ? saved_fname : "");
	this->setWindowTitle(title);

	this->setFilename(saved_fname);
	if(STRLEN(saved_fname) == 0)
	    this->saveCmd->deactivate();
	return;
    }
    // saveMB "cleans" the application
    if(dirty)
	theMBApplication->setDirty();

    if(STRLEN(saved_fname) == 0)
	this->saveCmd->deactivate();

    //
    // Since the call to saveMB(build_name) "corrupted" the filename, restore
    // it. to it's original value.
    //
    title[0] = '\0';
    strcat(title, "Module Builder: ");
    strcat(title, saved_fname ? saved_fname : "");
    this->setWindowTitle(title);
    this->setFilename(saved_fname);

    dest_file = (char *)CALLOC(STRLEN(path) + STRLEN(fname) + 10, sizeof(char));

    sprintf(exists_msg, "Overwrite existing file(s)?\n");
    if(flag & DO_C)
    {
	sprintf(dest_file,"%s/%s.c", path, fname);
	// Open for input
	std::ifstream *from = new std::ifstream(dest_file);
	if(!from->fail())
	{
	    c_exists = TRUE;
	    strcat(exists_msg, (const char *)dest_file);
	    strcat(exists_msg, (const char *)"\n");
	}
	from->close();
	delete from;

	// Make sure the file is writable
	std::ofstream *to = new std::ofstream(dest_file, std::ios::app);
	if(to->fail())
	{
	    ErrorMessage("File: %s is not writeable!", dest_file);
	    return;
	}
	to->close();
	delete to;
    }
    if(flag & DO_MDF)
    {
	sprintf(dest_file,"%s/%s.mdf", path, fname);
	// Open for input
	std::ifstream *from = new std::ifstream(dest_file);
	if(!from->fail())
	{
	    mdf_exists = TRUE;
	    strcat(exists_msg, (const char *)dest_file);
	    strcat(exists_msg, (const char *)"\n");
	}
	from->close();
	delete from;

	// Make sure the file is writable
	std::ofstream *to = new std::ofstream(dest_file, std::ios::app);
	if(to->fail())
	{
	    ErrorMessage("File: %s is not writeable!", dest_file);
	    return;
	}
	to->close();
	delete to;
    }
    if(flag & DO_MAKE)
    {
	sprintf(dest_file,"%s/%s.make", path, fname);
	// Open for input
	std::ifstream *from = new std::ifstream(dest_file);
	if(!from->fail())
	{
	    makefile_exists = TRUE;
	    strcat(exists_msg, (const char *)dest_file);
	    strcat(exists_msg, (const char *)"\n");
	}
	from->close();
	delete from;

	// Make sure the file is writable
	std::ofstream *to = new std::ofstream(dest_file, std::ios::app);
	if(to->fail())
	{
	    ErrorMessage("File: %s is not writeable!", dest_file);
	    return;
	}
	to->close();
	delete to;
    }
    FREE(dest_file);

    XtVaSetValues(this->getMainWindow(), XmNuserData, flag, NULL);

    if(c_exists || mdf_exists || makefile_exists)
    {
	theQuestionDialogManager->post(
		    this->getRootWidget(),
		    exists_msg,
		    "Overwrite confirmation",
		    this,
		    MBMainWindow::reallyBuild,
		    NULL,
		    NULL,
		    "OK",
		    "Cancel",
		    NULL);
	return;
    }



    this->reallyBuild((void *)this);

}
void MBMainWindow::reallyBuild(void *data)
{
    char	  gen_msg[512];
    char	  system_command[256];
    char          *fname;
    char          *path;
    char          *command;
    int		  flags;
    MBMainWindow  *mbmw = (MBMainWindow *)data;
    const char *tmpdir = theIBMApplication->getTmpDirectory();

    XtVaGetValues(mbmw->getMainWindow(), XmNuserData, &flags, NULL);

    fname = mbmw->getFilename();

    if(!fname)
	fname = XmTextGetString(mbmw->module_name_text);

    if(!fname || IsAllWhiteSpace(fname))
    {
	WarningMessage("Could not determine what name to use to build.");
	return;
    }
    path = GetDirname(fname);
    fname = GetFileBaseName(fname, ".mb");

    if(STRLEN(path) == 0) path = ".";
    command = (char *)CALLOC(STRLEN(path) + STRLEN(fname) + 10, sizeof(char));
    sprintf(command,"%s/%s", theIBMApplication->getTmpDirectory(), fname);

    if(!Generate(flags, command))
    {
	FREE(command);
	return;
    }
    FREE(command);

    //
    // Now, move the generated files to the where they belong
    //
    gen_msg[0] = '\0';
    strcat(gen_msg, "The following files have been generated:\n");
    if(flags & DO_C)
    {
	sprintf(system_command, "mv \"%s/%s.c\" \"%s/%s.c\"", tmpdir, fname, path, fname);
	system(system_command);
	sprintf(&gen_msg[STRLEN(gen_msg)], " %s/%s.c\n", path, fname);
    }
    if(flags & DO_MDF)
    {
	sprintf(system_command, "mv \"%s/%s.mdf\" \"%s/%s.mdf\"", tmpdir, fname, path, fname);
	system(system_command);
	sprintf(&gen_msg[STRLEN(gen_msg)], " %s/%s.mdf\n", path, fname);
    }
    if(flags & DO_MAKE)
    {
	sprintf(system_command, "mv \"%s/%s.make\" \"%s/%s.make\"",tmpdir,fname,path,fname);
	system(system_command);
	sprintf(&gen_msg[STRLEN(gen_msg)], "%s/%s.make", path, fname);
    }
    InfoMessage(gen_msg);

    sprintf(system_command, "rm \"%s/%s.mb\"", tmpdir, fname);
    system(system_command);


    if (flags & EXECUTABLE) 
    {
#ifdef HAVE_SETENV
       if(!getenv("DXARCH")) setenv("DXARCH", DXD_ARCHNAME,1);
#endif
       sprintf(system_command,"make -f %s.make &", fname);
       InfoMessage("executing: %s", system_command);
       system(system_command);
    }
    else if (flags & RUN)
    {
#ifdef HAVE_SETENV
       if(!getenv("DXARCH")) setenv("DXARCH", DXD_ARCHNAME,1);
#endif
       sprintf(system_command,"make -f %s.make run &", fname);
       InfoMessage("executing: %s", system_command);
       system(system_command);
    }
}
void MBMainWindow::notifyLoadableStateChange(Boolean set)
{
    this->setTextSensitivity(this->outboard_text, set);
    XtSetSensitive(this->outboard_label, set);
}
void MBMainWindow::notifyOutboardStateChange(Boolean set)
{
    this->setTextSensitivity(this->outboard_text, set);
    XtSetSensitive(this->outboard_label, set);
    this->setTextSensitivity(this->host_text, set);
    XtSetSensitive(this->host_label, set);
    XtSetSensitive(this->persistent_tb, set);
    Boolean async = XmToggleButtonGetState(this->asynch_tb);
    if (async && set)
          XmToggleButtonSetState(this->persistent_tb, True, False);
    if (!set)
          XmToggleButtonSetState(this->persistent_tb, False, False);
}
