/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>

#include <defines.h>


#include <ctype.h>

#if defined(HAVE_FSTREAM)
#include <fstream>
#elif defined (HAVE_FSTREAM_H)
#include <fstream.h>
#else
#error "no fstream and no fstream.h"
#endif

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
#include <Xm/ArrowB.h>

#include "GARMainWindow.h"
#include "GARChooserWindow.h"
#include "CommandTextPopup.h"
#include "GARApplication.h"
#include "GARCommand.h"
#include "GARNewCommand.h"
#include "FilenameSelectDialog.h"
#include "OpenFileDialog.h"
#include "SADialog.h"
#include "ConfirmedOpenCommand.h"
#include "Browser.h"
#include "CommentDialog.h"

#include "../base/lex.h"
#include "../base/DXStrings.h"
#include "../base/List.h"
#include "../base/WarningDialogManager.h"
#include "../base/ErrorDialogManager.h"
#include "../base/ButtonInterface.h"
#include "../base/CommandScope.h"

#if defined(HAVE_XINERAMA)
#include <X11/extensions/Xinerama.h>
#endif

#include "row_sens.bm"
#include "col_sens.bm"
#include "row_insens.bm"
#include "col_insens.bm"
#include "vector1.bm"
#include "vector2.bm"
#include "series1.bm"
#include "series2.bm"
#include "vector1_insens.bm"
#include "vector2_insens.bm"
#include "series1_insens.bm"
#include "series2_insens.bm"

#define ICON_NAME		"GAR"
#define LABEL_TOP_OFFSET	18
#define DOUBLE_LABEL_TOP_OFFSET	14
#define FIELD_FORM_COL		120
#define SHORT_TEXT_WIDTH	36
#define SHORT_TEXT_WIDTH2	58
#define GF_POS1			24

#define RESTRICT_NONE		0
#define RESTRICT_POSITIONS	1
#define RESTRICT_SAME		2

#define RES_CONVERT(res, str) XtVaTypedArg, res, XmRString, str, strlen(str)+1

#define MAP_NAMES(str)						\
		if(EqualString(str, "Most Significant Byte First"))	\
		{						\
		    delete str;					\
		    str = strdup("msb");			\
		}						\
		else if(EqualString(str, "Least Significant Byte First"))\
		{						\
		    delete str;					\
		    str = strdup("lsb");			\
		}                                               \
		else if(EqualString(str, "Binary (IEEE)"))      \
		{						\
		    delete str;					\
		    str = strdup("ieee");			\
		}                                               \
		else if(EqualString(str, "ASCII (Text)"))       \
		{						\
		    delete str;					\
		    str = strdup("ascii");			\
		}

#define L_CASE(str)						\
		{int si;					\
		for(si = 0; si < STRLEN(str); si++)		\
		    str[si] = tolower(str[si]);}

#define NAME_OUT(strm, w)					\
		{char *tmp = strdup(XtName(w));			\
		MAP_NAMES(tmp);					\
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
		
#define SET_OM_NAME(w, name, om_type)					\
	{Widget wtmp;						\
	WidgetList children;					\
	int num_children;					\
	int itmp;						\
	XtVaGetValues(w, XmNsubMenuId, &wtmp, NULL);		\
	XtVaGetValues(wtmp, XmNchildren, &children, 		\
		XmNnumChildren, &num_children, NULL);		\
	for(itmp = 0; itmp < num_children; itmp++)		\
	{							\
	    if(EqualString(XtName(children[itmp]), name))	\
	    {							\
		XtVaSetValues(w, XmNmenuHistory, children[itmp],NULL);\
		break;						\
	    }							\
	}							\
	if(itmp == num_children) 				\
	    WarningMessage("Could not set the %s option menu value to %s", om_type, name);}

#define DESTROY_OM_NAME(w, name)				\
	{Widget wtmp;						\
	WidgetList children;					\
	int num_children;					\
	int itmp;						\
	XtVaGetValues(w, XmNsubMenuId, &wtmp, NULL);		\
	XtVaGetValues(wtmp, XmNchildren, &children, 		\
		XmNnumChildren, &num_children, NULL);		\
	for(itmp = 0; itmp < num_children; itmp++)		\
	{							\
	    if(EqualString(XtName(children[itmp]), name))	\
	    {							\
		XtDestroyWidget(children[itmp]);		\
		break;						\
	    }							\
	}							\
	if(itmp == num_children) ASSERT(FALSE);}

#define STRIP_QUOTES(str)					\
	{int itmp;						\
	if((str[0] == '"') && (str[STRLEN(str)-1] == '"'))	\
	{							\
	    for(itmp = 1; itmp < STRLEN(str)-1; itmp++)		\
		str[itmp-1] = str[itmp];			\
	    str[itmp-1] = '\0';					\
	}}


static XtTranslations translations = NULL;
static XtTranslations tb_translations = NULL;

String GARMainWindow::DefaultResources[] =
{
    //".width:                                      1060",
    //".height:					  830",
    ".minWidth:					  1060",
    //".geometry:					  -0-0",

    ".iconName:                                   Data Prompter",
    ".title:                                      Data Prompter",
    "*topOffset:                                  15",
    "*bottomOffset:                               15",
    "*leftOffset:                                 10",
    "*rightOffset:                                10",
    "*XmToggleButton.shadowThickness:             0",
    "*radiobox.shadowThickness:                   0",
#ifdef aviion
    "*om.labelString:                             ",
#endif
    //"*modify_tb.labelString:             	       none",

    "*fileMenu.labelString:                       File",
    "*fileMenu.mnemonic:                          F",

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

    "*closeGAROption.labelString:                 Close",
    "*closeGAROption.mnemonic:                    C",
    "*closeGAROption.accelerator:                 Ctrl<Key>W",
    "*closeGAROption.acceleratorText:             Ctrl+W",

    "*editMenu.labelString:                       Edit",
    "*editMenu.mnemonic:                          E",

    "*commentOption.labelString:                  Comment...",
    "*commentOption.mnemonic:                     C",

    "*optionMenu.labelString:                     Options",
    "*optionMenu.mnemonic:                        E",

    "*fullOption.labelString:                     Full prompter",
    "*fullOption.mnemonic:                        F",

    "*simplifyOption.labelString:                 Simplified prompter",
    "*simplifyOption.mnemonic:                    S",

    "*file_label.labelString:                     Data file",
    "*od.labelString:                             origin, delta",
    "*field_list_label.labelString:               Field list",
    "*field_name_label.labelString:               Field name",
    "*num_el_label.labelString:                   # elem",

    "*helpGAFormatOption.labelString:		On General Array Format...",
    "*helpTutorialOption.labelString:		Start Tutorial...",

    NULL
};

int GetNumDim(char *line);

boolean GARMainWindow::ClassInitialized = FALSE;

GARMainWindow::GARMainWindow(unsigned long mode) : 
		IBMMainWindow("GARMWName", TRUE)
{
    //
    // Initialize member data.
    //

    this->title = NUL(char*); 
    this->comment_text = NULL; 
    this->mode = mode;

    //
    // Initialize member data.
    //
    this->fileMenu          = NUL(Widget);
    this->editMenu          = NUL(Widget);

    this->fileMenuPulldown  = NUL(Widget);
    this->editMenuPulldown  = NUL(Widget);
    this->optionMenuPulldown  = NUL(Widget);
	

    if (!this->commandScope)
	this->commandScope = new CommandScope();

    this->newCmd =
        new GARNewCommand("new",this->commandScope,TRUE,this, NULL);

    this->openCmd =
        new ConfirmedOpenCommand("open",this->commandScope,
	    TRUE,this,theGARApplication);

    this->saveCmd =
        new GARCommand("save",this->commandScope,FALSE,this, this->SAVE);

    this->saveAsCmd =
        new GARCommand("saveAs",this->commandScope,TRUE,this, this->SAVE_AS);

    this->closeGARCmd =
	new GARCommand ("closeGAR", this->commandScope, TRUE, this, this->CLOSE_GAR);

    this->commentCmd =
        new GARCommand("comment",this->commandScope,TRUE,this, this->COMMENT);

    this->fullCmd =
        new GARCommand("full",this->commandScope,
		!FULL,this, this->FULLGAR);

    this->simplifyCmd =
        new GARCommand("simplify",this->commandScope,
		FULL != 0,this, this->SIMPLIFY);

    this->helpGAFormatOption = NULL;
    this->helpTutorialOption = NULL;
    this->commentOption = NULL; 
    this->closeGAROption = NULL;
    this->saveAsOption =  NULL;
    this->saveOption = NULL;
    this->openOption = NULL;
    this->newOption = NULL;
    this->fsd =  NULL;
    this->open_fd =  NULL;
    this->save_as_fd =  NULL;
    this->comment_dialog =  NULL;
    this->fullOption =  NULL;
    this->simplifyOption =  NULL;

    this->xpos = this->ypos = this->height = this->width = -1;
    this->dimension = 1;
    this->current_field = 0;
    this->filename = NULL;
    this->dependency_restriction = RESTRICT_NONE;
    this->layout_was_on = False;
    this->block_was_on = False;
    this->record_sep_was_on = False;

    //
    // Install the default resources for THIS class (not the derived classes)
    //
    if (NOT GARMainWindow::ClassInitialized)
    {
	ASSERT(theApplication);
        GARMainWindow::ClassInitialized = TRUE;
	this->installDefaultResources(theApplication->getRootWidget());
    }

    //
    // Create the browser
    //
    this->browser = new Browser(this);
    this->browser->initialize();

    this->layout_skip_text = NUL(Widget);
    this->layout_width_text = NUL(Widget);
    this->block_skip_text = NUL(Widget);
    this->block_element_text = NUL(Widget);
    this->block_width_text = NUL(Widget);
    this->string_size = NUL(Widget);
						
}


GARMainWindow::~GARMainWindow()
{
    //
    // Make the panel disappear from the display (i.e. don't wait for
    // (two phase destroy to remove the panel). 
    //
    if (this->getRootWidget())
        XtUnmapWidget(this->getRootWidget());

    delete this->browser;

    if (this->helpGAFormatOption )    delete this->helpGAFormatOption;
    if (this->helpTutorialOption)     delete this->helpTutorialOption;
    if (this->commentOption  )                delete this->commentOption;
    if (this->closeGAROption )            delete this->closeGAROption;
    if (this->saveAsOption)           delete this->saveAsOption;
    if (this->saveOption )            delete this->saveOption;
    if (this->openOption )            delete this->openOption;
    if (this->newOption )             delete this->newOption;
    if (this->fsd )                   delete this->fsd;
    if (this->open_fd )               delete this->open_fd;
    if (this->save_as_fd )            delete this->save_as_fd;
    if (this->comment_dialog )                delete this->comment_dialog;
    if (this->fullOption )            delete this->fullOption;
    if (this->simplifyOption)                 delete this->simplifyOption;

    delete this->simplifyCmd;
    delete this->fullCmd;
    delete this->commentCmd;
    delete this->closeGARCmd;
    delete this->saveAsCmd;
    delete this->saveCmd;
    delete this->openCmd;
    delete this->newCmd;


}



//
// Install the default resources for this class.
//
void GARMainWindow::installDefaultResources(Widget baseWidget)
{
    this->setDefaultResources(baseWidget, GARMainWindow::DefaultResources);
    this->IBMMainWindow::installDefaultResources(baseWidget);
}

void GARMainWindow::manage()
{			
   this->IBMMainWindow::manage();

	// Offset the window from the GARApplication a bit so users
	// can get to it if need be.
	int cx, cy, cw, ch;
	this->getGeometry(&cx, &cy, &cw, &ch);

	this->setGeometry(cx+20, cy+20, cw, ch);	
}


Widget GARMainWindow::createWorkArea(Widget parent)
{
    Widget    frame;
    Widget    rb, rbrec;
    Widget    label;
    Widget    label1;
    Widget    label2;
    Widget    label3;
    Widget    sep;
    Widget    pulldown_menu;
    Widget    top_widget;
    Arg       wargs[50];
    int       n = 0;
    int       i;
    XmString  xms;

    ASSERT(parent);

    this->fsd = new FilenameSelectDialog(this);
    this->open_fd = new OpenFileDialog(this);
    this->save_as_fd = new SADialog(this);
    this->comment_dialog = new CommentDialog(parent, FALSE, this);

    //
    // Create the outer frame and form.
    //
    frame =
	XtVaCreateManagedWidget
	    ("topformframe",
	     xmFrameWidgetClass,
	     parent,
	     XmNshadowType,	XmSHADOW_OUT,
	     NULL);

    //
    // Create the form.
    //
    this->topform =
	XtVaCreateManagedWidget
	    ("topform",
	     xmFormWidgetClass,
	     frame,
	     NULL);

    this->generalform =
	XtVaCreateManagedWidget
	    ("generalform",
	     xmFormWidgetClass,
	     this->topform,
	     XmNtopAttachment,		XmATTACH_FORM,
	     XmNtopOffset,		10,
	     XmNleftAttachment,		XmATTACH_FORM,
	     XmNbottomAttachment,	XmATTACH_FORM,
	     XmNbottomOffset,		10,
	     XmNrightAttachment,	XmATTACH_POSITION,
	     XmNrightPosition,		50,
	     XmNrightOffset,		0,
	     XmNshadowThickness,	1,
	     XmNshadowType,		XmSHADOW_IN,
	     NULL);

    XtAddEventHandler(this->generalform, StructureNotifyMask, False,
	(XtEventHandler)GARMainWindow_MapEH, XtPointer(this));

    this->fieldform =
	XtVaCreateManagedWidget
	    ("fieldform",
	     xmFormWidgetClass,
	     this->topform,
	     XmNtopAttachment,		XmATTACH_FORM,
	     XmNtopOffset,		10,
	     XmNleftAttachment,		XmATTACH_POSITION,
	     XmNleftPosition,		51,
	     XmNleftOffset,		0,
	     XmNbottomAttachment,	XmATTACH_FORM,
	     XmNbottomOffset,		10,
	     XmNrightAttachment,	XmATTACH_FORM,
	     XmNshadowThickness,	1,
	     XmNshadowType,		XmSHADOW_IN,
	     NULL);

    this->file_label = 
	XtVaCreateManagedWidget(
		"file_label", xmLabelWidgetClass, 
		this->generalform, 
		XmNtopAttachment,	XmATTACH_FORM,
		XmNtopOffset,		LABEL_TOP_OFFSET,
		XmNleftAttachment,	XmATTACH_FORM,
		NULL);

    this->elip_pb = 
	XtVaCreateManagedWidget(
		"...", xmPushButtonWidgetClass, 
		this->generalform, 
		XmNtopAttachment,	XmATTACH_FORM,
		XmNheight,		28,
		XmNwidth,		28,
		XmNrecomputeSize,	False,
		XmNrightAttachment,	XmATTACH_FORM,
		NULL);

    this->CreateElipsesPopup();

    this->file_text = 
	XtVaCreateManagedWidget(
		"file_text", xmTextWidgetClass, 
		this->generalform, 
		XmNtopAttachment,	XmATTACH_FORM,
		XmNleftAttachment,	XmATTACH_POSITION,
		XmNleftPosition,	GF_POS1,
		XmNleftOffset,		10,
		XmNrightAttachment,	XmATTACH_WIDGET,
		XmNrightWidget,		this->elip_pb,
		NULL);
    XmTextSource source = (XmTextSource)theGARApplication->getDataFileSource();
    if (source) {
	XmTextSetSource (this->file_text, source, 0, 0);
    } else {
	source = XmTextGetSource (this->file_text);
	theGARApplication->setDataFileSource((void*)source);
    }
    GARChooserWindow *gcw = theGARApplication->getChooserWindow();
    if (gcw) {
	CommandTextPopup* ctp = gcw->getCommandText();
	if (ctp) 
	    ctp->addCallback(this->file_text, (XtCallbackProc)GARMainWindow_DirtyFileCB);
    }
    XtAddCallback (this->file_text, XmNmodifyVerifyCallback,
	(XtCallbackProc)GARMainWindow_DirtyFileCB, (XtPointer)this);

    this->header_tb = XtVaCreateManagedWidget(
		"Header", xmToggleButtonWidgetClass, this->generalform,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		this->file_text,
		XmNtopOffset,		LABEL_TOP_OFFSET-2,
		XmNleftAttachment,	XmATTACH_FORM,
		NULL);

    XtAddCallback(this->header_tb,
	  XmNvalueChangedCallback,
	  (XtCallbackProc)GARMainWindow_HeaderCB,
	  (XtPointer)this);
    //
    // Create the Bytes/Lines/Marker Option menu
    //
    
    pulldown_menu = this->createHeaderPulldown(this->generalform, FALSE, FALSE,
                                               FALSE);

    n = 0;
    XtSetArg(wargs[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(wargs[n], XmNtopWidget, this->file_text); n++;
    XtSetArg(wargs[n], XmNleftAttachment, XmATTACH_POSITION); n++;
    XtSetArg(wargs[n], XmNleftPosition, GF_POS1); n++;
    XtSetArg(wargs[n], XmNleftOffset, 0); n++;
    XtSetArg(wargs[n], XmNsubMenuId, pulldown_menu); n++;
    this->header_om = XmCreateOptionMenu(this->generalform, "om", wargs, n);

    XtManageChild(this->header_om);

    this->header_text = 
	XtVaCreateManagedWidget(
		"header_text", xmTextWidgetClass, 
		this->generalform, 
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		this->file_text,
		XmNleftAttachment,	XmATTACH_WIDGET,
		XmNleftWidget,		this->header_om,
		XmNrightAttachment,	XmATTACH_FORM,
		NULL);

    this->setHeaderSensitivity(False);

    XtAddCallback(this->header_text,
	  XmNmodifyVerifyCallback,
	  (XtCallbackProc)GARMainWindow_IntegerMVCB,
	  (XtPointer)this);

    if(POINTS_IS_ON && GRID_IS_ON)
    {
	sep = XtVaCreateManagedWidget("optionSeparator",
		    xmSeparatorWidgetClass, this->generalform,
		    XmNtopAttachment,	XmATTACH_WIDGET,
		    XmNtopWidget,	this->header_om,
		    XmNleftAttachment,	XmATTACH_FORM,
		    XmNleftOffset,	3,
		    XmNrightAttachment,	XmATTACH_FORM,
		    XmNrightOffset,	3,
		    NULL);
	top_widget = sep;
    }
    else
	top_widget = this->header_om;

    n = 0;
    XtSetArg(wargs[n], XmNtopAttachment, XmATTACH_WIDGET);	n++;
    XtSetArg(wargs[n], XmNtopWidget, top_widget);			n++;
    XtSetArg(wargs[n], XmNleftAttachment, XmATTACH_FORM);	n++;
    XtSetArg(wargs[n], XmNspacing, 15);				n++;
    XtSetArg(wargs[n], XmNmarginWidth, 0); 			n++;
    XtSetArg(wargs[n], XmNshadowThickness, 1);			n++;
    XtSetArg(wargs[n], XmNshadowType, XmSHADOW_IN);		n++;
    rb = XmCreateRadioBox(this->generalform, "radiobox", wargs, n);
    XtManageChild(rb);

    xms = XmStringCreateSimple("Grid size");
    this->grid_tb = XtVaCreateWidget(
		"Grid size", xmToggleButtonWidgetClass, rb,
		XmNindicatorOn,	FULL || 
				(GRID_IS_ON && POINTS_IS_ON),
		XmNlabelString, xms,
		NULL);
    XmStringFree(xms);

    XtAddCallback(this->grid_tb,
	  XmNvalueChangedCallback,
	  (XtCallbackProc)GARMainWindow_GridCB,
	  (XtPointer)this);

    xms = XmStringCreateSimple("# of Points");
    this->points_tb = XtVaCreateWidget(
		"# of Points", xmToggleButtonWidgetClass, rb,
		XmNindicatorOn,	FULL || 
				(GRID_IS_ON && POINTS_IS_ON),
		XmNlabelString, xms,
		NULL);
    XmStringFree(xms);

    this->points_text = 
	XtVaCreateManagedWidget(
		"points_text", xmTextWidgetClass, 
		this->generalform, 
		XmNbottomAttachment,	XmATTACH_OPPOSITE_WIDGET,
		XmNbottomWidget,	rb,
		XmNbottomOffset,	0,
		XmNleftAttachment,	XmATTACH_POSITION,
		XmNleftPosition,	GF_POS1,
		XmNleftOffset,	 	10,
		XmNwidth,		SHORT_TEXT_WIDTH2,
		NULL);

    this->setPointsSensitivity(False);

    XtAddCallback(this->points_text,
	  XmNmodifyVerifyCallback,
	  (XtCallbackProc)GARMainWindow_IntegerMVCB,
	  (XtPointer)this);

    XtAddCallback(this->points_text,
	  XmNactivateCallback,
	  (XtCallbackProc)GARMainWindow_VerifyNonZeroCB,
	  (XtPointer)this);


    this->grid_text[0] = 
	XtVaCreateManagedWidget(
		"grid_text[0]", xmTextWidgetClass, 
		this->generalform, 
		XmNtopAttachment,	XmATTACH_OPPOSITE_WIDGET,
		XmNtopWidget,		rb,
		XmNtopOffset,		0,
		XmNleftAttachment,	XmATTACH_POSITION,
		XmNleftPosition,	GF_POS1,
		XmNleftOffset,		10,
		XmNwidth,		SHORT_TEXT_WIDTH2,
		NULL);

    label1 = XtVaCreateManagedWidget(
		"x", xmLabelWidgetClass, this->generalform,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNtopWidget, this->grid_text[0],
		XmNtopOffset, 0,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomWidget, this->grid_text[0],
		XmNbottomOffset, 0,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, this->grid_text[0],
		XmNleftOffset, 5,
		XmNindicatorOn, False,
		XmNshadowThickness, 1,
		NULL);

    this->grid_text[1] = 
	XtVaCreateManagedWidget(
		"grid_text[1]", xmTextWidgetClass, 
		this->generalform, 
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNtopWidget, this->grid_text[0],
		XmNtopOffset, 0,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, label1,
		XmNleftOffset, 5,
		XmNwidth, SHORT_TEXT_WIDTH2,
		NULL);

    label2 = XtVaCreateManagedWidget(
		"x", xmLabelWidgetClass, this->generalform,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNtopWidget, this->grid_text[0],
		XmNtopOffset, 0,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomWidget, this->grid_text[0],
		XmNbottomOffset, 0,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, this->grid_text[1],
		XmNleftOffset, 5,
		XmNindicatorOn, False,
		XmNshadowThickness, 1,
		NULL);

    this->grid_text[2] = 
	XtVaCreateManagedWidget(
		"grid_text[2]", xmTextWidgetClass, 
		this->generalform, 
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNtopWidget, this->grid_text[0],
		XmNtopOffset, 0,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, label2,
		XmNleftOffset, 5,
		XmNwidth, SHORT_TEXT_WIDTH2,
		NULL);

    label3 = XtVaCreateManagedWidget(
		"x", xmLabelWidgetClass, this->generalform,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNtopWidget, this->grid_text[0],
		XmNtopOffset, 0,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomWidget, this->grid_text[0],
		XmNbottomOffset, 0,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, this->grid_text[2],
		XmNleftOffset, 5,
		XmNindicatorOn, False,
		XmNshadowThickness, 1,
		NULL);

    this->grid_text[3] = 
	XtVaCreateManagedWidget(
		"grid_text[3]", xmTextWidgetClass, 
		this->generalform, 
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNtopWidget, this->grid_text[0],
		XmNtopOffset, 0,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, label3,
		XmNleftOffset, 5,
		XmNwidth, SHORT_TEXT_WIDTH2,
		NULL);

    for(i = 0; i < 4; i++)
    {
	XtAddCallback(this->grid_text[i],
	      XmNmodifyVerifyCallback,
	      (XtCallbackProc)GARMainWindow_IntegerMVCB,
	      (XtPointer)this);
	XtAddCallback(this->grid_text[i],
	      XmNvalueChangedCallback,
	      (XtCallbackProc)GARMainWindow_GridDimCB,
	      (XtPointer)this);
        XtAddCallback(this->grid_text[i],
   	      XmNactivateCallback,
	      (XtCallbackProc)GARMainWindow_VerifyNonZeroCB,
	      (XtPointer)this);

    }
    if(!FULL)
    {
	if(!GRID_IS_ON)
	{
	    XtUnmanageChild(this->grid_text[3]);
	    XtUnmanageChild(label3);
	    XtUnmanageChild(this->grid_text[2]);
	    XtUnmanageChild(label2);
	    XtUnmanageChild(this->grid_text[1]);
	    XtUnmanageChild(label1);
	    XtUnmanageChild(this->grid_text[0]);
	}
	if(!POINTS_IS_ON)
	{
	    XtUnmanageChild(this->points_text);
	}
	if(GRID_IS_ON)
	    XtManageChild(this->grid_tb);
	if(POINTS_IS_ON)
	    XtManageChild(this->points_tb);
    }
    else
    {
	XtManageChild(this->grid_tb);
	XtManageChild(this->points_tb);
    }

    if(POINTS_IS_ON && GRID_IS_ON)
    {
	sep = XtVaCreateManagedWidget("optionSeparator",
		    xmSeparatorWidgetClass, this->generalform,
		    XmNtopAttachment, XmATTACH_WIDGET,
		    XmNtopWidget, rb,
		    XmNleftAttachment, XmATTACH_FORM,
		    XmNleftOffset, 3,
		    XmNrightAttachment, XmATTACH_FORM,
		    XmNrightOffset, 3,
		    NULL);
	top_widget = sep;
    }
    else
	top_widget = rb;

    this->format_label = XtVaCreateManagedWidget(
		"Data format", xmLabelWidgetClass, this->generalform,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, top_widget,
		XmNtopOffset, LABEL_TOP_OFFSET,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);
    //
    // Create the ASCII/Text/Binary/IEEE Option menu
    //
    
    pulldown_menu = this->createFormat2Pulldown(this->generalform);

    n = 0;
    XtSetArg(wargs[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(wargs[n], XmNtopWidget, top_widget); n++;
    XtSetArg(wargs[n], XmNleftAttachment, XmATTACH_POSITION); n++;
    XtSetArg(wargs[n], XmNleftPosition, GF_POS1); n++;
    XtSetArg(wargs[n], XmNleftOffset, 0); n++;
    XtSetArg(wargs[n], XmNsubMenuId, pulldown_menu); n++;
    this->format2_om = XmCreateOptionMenu(this->generalform, "om", wargs, n);

    XtManageChild(this->format2_om);

    //
    // Create the MSB/LSB Option menu
    //
    
    pulldown_menu = this->createFormat1Pulldown(this->generalform);

    n = 0;
    XtSetArg(wargs[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(wargs[n], XmNtopWidget, top_widget); n++;
    XtSetArg(wargs[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(wargs[n], XmNleftWidget, this->format2_om); n++;
    XtSetArg(wargs[n], XmNleftOffset, 0); n++;
    XtSetArg(wargs[n], XmNsubMenuId, pulldown_menu); n++;
    this->format1_om = XmCreateOptionMenu(this->generalform, "om", wargs, n);

    XtManageChild(this->format1_om);

    XtSetSensitive(this->format1_om, False);

    this->majority_label = XtVaCreateManagedWidget(
		"Data order", xmLabelWidgetClass, this->generalform,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, this->format1_om,
		XmNtopOffset, LABEL_TOP_OFFSET,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

    //
    // Create the Row/Column toggle buttons
    //
    
    Display *d = XtDisplay(pulldown_menu);
    Window win = XtWindow(pulldown_menu); 
    Pixel bg, fg;

    XtVaGetValues(this->majority_label, 
		  XmNforeground, &fg, 
		  XmNbackground, &bg, 
		  NULL);

    this->row_sens_pix = XCreatePixmapFromBitmapData(
	d, win, (char *)row_sens_bits, row_sens_width, row_sens_height,
	fg, bg, DefaultDepthOfScreen(XtScreen(parent)));

    this->row_insens_pix = XCreatePixmapFromBitmapData(
	d, win, (char *)row_insens_bits, row_insens_width, row_insens_height,
	fg, bg, DefaultDepthOfScreen(XtScreen(parent)));

    this->col_sens_pix = XCreatePixmapFromBitmapData(
	d, win, (char *)col_sens_bits, col_sens_width, col_sens_height,
	fg, bg, DefaultDepthOfScreen(XtScreen(parent)));

    this->col_insens_pix = XCreatePixmapFromBitmapData(
	d, win, (char *)col_insens_bits, col_insens_width, col_insens_height,
	fg, bg, DefaultDepthOfScreen(XtScreen(parent)));

    this->row_label = XtVaCreateManagedWidget(
                "Row",
                xmLabelWidgetClass, this->generalform,
                XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
                XmNtopWidget, this->majority_label,
                XmNtopOffset, 0,
                XmNleftAttachment, XmATTACH_POSITION,
                XmNleftPosition, GF_POS1,
                XmNalignment, XmALIGNMENT_BEGINNING,
                NULL);

    this->row_tb = XtVaCreateManagedWidget(
		"row_tb", xmToggleButtonWidgetClass, this->generalform,
		XmNtopAttachment,	XmATTACH_OPPOSITE_WIDGET,
		XmNtopWidget,		this->majority_label,
		XmNtopOffset,		-3,
		XmNleftAttachment,	XmATTACH_WIDGET,
		XmNleftWidget,		this->row_label,
		XmNleftOffset,		15,
		XmNindicatorOn,		False,
		XmNlabelType,		XmPIXMAP,
		XmNlabelPixmap,		this->row_sens_pix,
		XmNshadowThickness, 	2,
		XmNset,			True,
		NULL);

    this->col_label = XtVaCreateManagedWidget(
                "Column",
                xmLabelWidgetClass,	this->generalform,
                XmNtopAttachment,	XmATTACH_OPPOSITE_WIDGET,
                XmNtopWidget,		this->majority_label,
                XmNtopOffset,		0,
                XmNleftAttachment,	XmATTACH_WIDGET,
                XmNleftWidget,		this->row_tb,
                XmNalignment,		XmALIGNMENT_BEGINNING,
                NULL);

    this->col_tb = XtVaCreateManagedWidget(
		"col_tb", xmToggleButtonWidgetClass, this->generalform,
                XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
                XmNtopWidget,		this->majority_label,
                XmNtopOffset,		-3,
                XmNleftAttachment,	XmATTACH_WIDGET,
                XmNleftWidget,		this->col_label,
		XmNindicatorOn,		False,
		XmNlabelType,		XmPIXMAP,
		XmNlabelPixmap,		this->col_sens_pix,
		XmNshadowThickness, 	2,
		XmNset,			False,
		NULL);

    XtAddCallback(this->row_tb,
	  XmNdisarmCallback,
	  (XtCallbackProc)GARMainWindow_MajorityCB,
	  (XtPointer)this);
    XtAddCallback(this->col_tb,
	  XmNdisarmCallback,
	  (XtCallbackProc)GARMainWindow_MajorityCB,
	  (XtPointer)this);



    xms = XmStringCreateLtoR("Field\ninterleaving", XmSTRING_DEFAULT_CHARSET);
    this->field_interleaving_label = XtVaCreateWidget(
		"field_interleaving_label", 
		xmLabelWidgetClass, this->generalform,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, this->majority_label,
		XmNtopOffset, DOUBLE_LABEL_TOP_OFFSET,
		XmNleftAttachment, XmATTACH_FORM,
		XmNlabelString, xms,
		XmNalignment, XmALIGNMENT_BEGINNING,
		NULL);
    if(FULL)
	XtManageChild(this->field_interleaving_label);
    XmStringFree(xms);

    //
    // Create the Field/Record Option menu
    //
    
    pulldown_menu = this->createFieldInterleavingPulldown(this->generalform);

    n = 0;
    XtSetArg(wargs[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(wargs[n], XmNtopWidget, this->row_tb); n++;
    XtSetArg(wargs[n], XmNleftAttachment, XmATTACH_POSITION); n++;
    XtSetArg(wargs[n], XmNleftPosition, GF_POS1); n++;
    XtSetArg(wargs[n], XmNleftOffset, 0); n++;
    XtSetArg(wargs[n], XmNsubMenuId, pulldown_menu); n++;
    this->field_interleaving_om = 
	XmCreateOptionMenu(this->generalform, "om", wargs, n);

    if(FULL)
	XtManageChild(this->field_interleaving_om);

    xms = XmStringCreateLtoR("Vector\ninterleaving", XmSTRING_DEFAULT_CHARSET);

    if (FULL) 
    this->vector_interleaving_label = XtVaCreateWidget(
		"vector_interleaving_label", 
		xmLabelWidgetClass, 
		this->generalform,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, this->majority_label,
		XmNtopOffset, DOUBLE_LABEL_TOP_OFFSET,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, field_interleaving_om,
		XmNlabelString, xms,
		XmNalignment, XmALIGNMENT_BEGINNING,
	        NULL);
    else
    this->vector_interleaving_label = XtVaCreateWidget(
		"vector_interleaving_label", 
		xmLabelWidgetClass, 
		this->generalform,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, this->row_tb,
		XmNtopOffset, DOUBLE_LABEL_TOP_OFFSET,
		XmNleftAttachment, XmATTACH_FORM,
		XmNlabelString, xms,
		XmNalignment, XmALIGNMENT_BEGINNING,
		NULL);

    if(FULL || !SPREADSHEET)
	XtManageChild(this->vector_interleaving_label);
    XmStringFree(xms);

    //
    // Create the Vector Interleaving Option menu
    //
    
    pulldown_menu = 
	this->createInsensVectorInterleavingPulldown(this->generalform);

    n = 0;
    XtSetArg(wargs[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(wargs[n], XmNtopWidget, FULL ? this->majority_label :
                                            this->row_tb); n++;
    XtSetArg(wargs[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(wargs[n], XmNleftWidget, vector_interleaving_label); n++;
    XtSetArg(wargs[n], XmNsubMenuId, pulldown_menu); n++;
    this->insens_vector_interleaving_om = 
	XmCreateOptionMenu(this->generalform, "om", wargs, n);

    if(FULL || !SPREADSHEET)
	XtManageChild(this->insens_vector_interleaving_om);

    pulldown_menu = 
	this->createSensVectorInterleavingPulldown(this->generalform);

    n = 0;
    XtSetArg(wargs[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(wargs[n], XmNtopWidget, FULL ? this->majority_label :
                                            this->row_tb); n++;
    XtSetArg(wargs[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(wargs[n], XmNleftWidget, vector_interleaving_label); n++;
    XtSetArg(wargs[n], XmNsubMenuId, pulldown_menu); n++;
    this->sens_vector_interleaving_om = 
	XmCreateOptionMenu(this->generalform, "om", wargs, n);

    if(FULL)
    {
	sep = XtVaCreateManagedWidget("optionSeparator",
		    xmSeparatorWidgetClass, this->generalform,
		    XmNtopAttachment, XmATTACH_WIDGET,
		    XmNtopWidget, (FULL || !SPREADSHEET) ?
				   this->insens_vector_interleaving_om :
				   this->row_tb,
		    XmNleftAttachment, XmATTACH_FORM,
		    XmNleftOffset, 3,
		    XmNrightAttachment, XmATTACH_FORM,
		    XmNrightOffset, 3,
		    NULL);
    }

    if(FULL)
	top_widget = sep;
    else if(!SPREADSHEET)
	top_widget = this->insens_vector_interleaving_om;
    else
	top_widget = this->row_tb;

    this->series_tb = XtVaCreateWidget(
		"Series", xmToggleButtonWidgetClass, this->generalform,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, top_widget,
		XmNtopOffset, LABEL_TOP_OFFSET-2,
		XmNleftAttachment, XmATTACH_FORM,
		XmNset,	False,
		NULL);

    if(FULL)
	XtManageChild(this->series_tb);

    XtAddCallback(this->series_tb,
	  XmNvalueChangedCallback,
	  (XtCallbackProc)GARMainWindow_SeriesCB,
	  (XtPointer)this);

    this->series_n_text = 
	XtVaCreateWidget(
		"series_n_text", xmTextWidgetClass, 
		this->generalform, 
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, top_widget,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNleftPosition, GF_POS1,
		XmNleftOffset, 10,
		XmNwidth, SHORT_TEXT_WIDTH2,
		NULL);

    if(FULL)
	XtManageChild(this->series_n_text);

    XtAddCallback(this->series_n_text,
	  XmNmodifyVerifyCallback,
	  (XtCallbackProc)GARMainWindow_IntegerMVCB,
	  (XtPointer)this);

    XtAddCallback(this->series_n_text,
	  XmNactivateCallback,
	  (XtCallbackProc)GARMainWindow_VerifyNonZeroCB,
	  (XtPointer)this);

    this->series_n_label = XtVaCreateWidget(
		"n", xmLabelWidgetClass, this->generalform,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, top_widget,
		XmNtopOffset, LABEL_TOP_OFFSET,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNrightWidget, this->series_n_text,
		NULL);

    if(FULL)
	XtManageChild(this->series_n_label);

    this->series_start_label = XtVaCreateWidget(
		"start", xmLabelWidgetClass, this->generalform,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, top_widget,
		XmNtopOffset, LABEL_TOP_OFFSET,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, this->series_n_text,
		NULL);

    if(FULL)
	XtManageChild(this->series_start_label);

    this->series_start_text = 
	XtVaCreateWidget(
		"series_start_text", xmTextWidgetClass, 
		this->generalform, 
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, top_widget,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, series_start_label,
		XmNwidth, SHORT_TEXT_WIDTH2,
		NULL);

    if(FULL)
	XtManageChild(this->series_start_text);

    XtAddCallback(this->series_start_text,
	  XmNmodifyVerifyCallback,
	  (XtCallbackProc)GARMainWindow_ScalarMVCB,
	  (XtPointer)this);

    this->series_delta_label = XtVaCreateWidget(
		"delta", xmLabelWidgetClass, this->generalform,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, top_widget,
		XmNtopOffset, LABEL_TOP_OFFSET,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, this->series_start_text,
		NULL);

    if(FULL)
	XtManageChild(this->series_delta_label);

    this->series_delta_text = 
	XtVaCreateWidget(
		"series_delta_text", xmTextWidgetClass, 
		this->generalform, 
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, top_widget,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, series_delta_label,
		XmNwidth, SHORT_TEXT_WIDTH2,
		NULL);

    if(FULL)
	XtManageChild(this->series_delta_text);

    XtAddCallback(this->series_delta_text,
	  XmNmodifyVerifyCallback,
	  (XtCallbackProc)GARMainWindow_ScalarMVCB,
	  (XtPointer)this);

    XmTextSetString(this->series_n_text, "1");
    XmTextSetString(this->series_start_text, "1");
    XmTextSetString(this->series_delta_text, "1");
    this->setSeriesSensitivity(False);

    xms = XmStringCreateLtoR("Series\ninterleaving", XmSTRING_DEFAULT_CHARSET);
    this->series_interleaving_label = XtVaCreateWidget(
		"series_interleaving_label", 
		xmLabelWidgetClass, this->generalform,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, this->series_n_text,
		XmNtopOffset, DOUBLE_LABEL_TOP_OFFSET,
		XmNleftAttachment, XmATTACH_FORM,
		XmNlabelString, xms,
		XmNalignment, XmALIGNMENT_BEGINNING,
		NULL);

    if(FULL)
	XtManageChild(this->series_interleaving_label);
    XmStringFree(xms);

    //
    // Create the Series Interleaving Option menu
    //
    
    pulldown_menu = this->createSeriesInterleavingPulldown(this->generalform);

    n = 0;
    XtSetArg(wargs[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(wargs[n], XmNtopWidget, this->series_n_text); n++;
    XtSetArg(wargs[n], XmNleftAttachment, XmATTACH_POSITION); n++;
    XtSetArg(wargs[n], XmNleftPosition, GF_POS1); n++;
    XtSetArg(wargs[n], XmNleftOffset, 0); n++;
    XtSetArg(wargs[n], XmNsubMenuId, pulldown_menu); n++;
    this->series_interleaving_om = 
	XmCreateOptionMenu(this->generalform, "om", wargs, n);

    if(FULL)
	XtManageChild(this->series_interleaving_om);
    XtUninstallTranslations(this->series_interleaving_om);

    xms = XmStringCreateLtoR("Series\nseparator", XmSTRING_DEFAULT_CHARSET);
    this->series_sep_tb = XtVaCreateWidget(
		"series_separator_tb",
		xmToggleButtonWidgetClass,this->generalform,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, this->series_interleaving_om,
		XmNtopOffset, LABEL_TOP_OFFSET,
		XmNleftAttachment, XmATTACH_FORM,
		XmNlabelString, xms,
		XmNalignment, XmALIGNMENT_BEGINNING,
		NULL);

    if(FULL)
	XtManageChild(this->series_sep_tb);
    XmStringFree(xms);

    XtAddCallback(this->series_sep_tb,
	  XmNvalueChangedCallback,
	  (XtCallbackProc)GARMainWindow_SeriesSepCB,
	  (XtPointer)this);
    //
    // Create the Bytes/Lines/Marker Option menu
    //
    
    pulldown_menu = this->createHeaderPulldown(this->generalform, FALSE, FALSE,
                                               FALSE);

    n = 0;
    XtSetArg(wargs[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(wargs[n], XmNtopWidget, this->series_interleaving_om); n++;
    XtSetArg(wargs[n], XmNleftAttachment, XmATTACH_POSITION); n++;
    XtSetArg(wargs[n], XmNleftPosition, GF_POS1); n++;
    XtSetArg(wargs[n], XmNleftOffset, 0); n++;
    XtSetArg(wargs[n], XmNsubMenuId, pulldown_menu); n++;
    this->series_sep_om = XmCreateOptionMenu(this->generalform, "om", wargs, n);

    if(FULL)
	XtManageChild(this->series_sep_om);

    this->series_sep_text = 
	XtVaCreateWidget(
		"series_sep_text", xmTextWidgetClass, 
		this->generalform, 
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, this->series_interleaving_om,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, this->series_sep_om,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);

    if(FULL)
	XtManageChild(this->series_sep_text);

    XtAddCallback(this->series_sep_text,
	  XmNmodifyVerifyCallback,
	  (XtCallbackProc)GARMainWindow_IntegerMVCB,
	  (XtPointer)this);

    this->setSeriesSepSensitivity();

    if(FULL)
	top_widget = this->series_sep_tb;
    else if(!SPREADSHEET)
	top_widget = this->insens_vector_interleaving_om;
    else
	top_widget = this->row_tb;

    if(FULL || POSITIONS_IS_ON)
    {
	sep = XtVaCreateManagedWidget("optionSeparator",
		    xmSeparatorWidgetClass, this->generalform,
		    XmNtopAttachment, XmATTACH_WIDGET,
		    XmNtopWidget, top_widget,
		    XmNleftAttachment, XmATTACH_FORM,
		    XmNleftOffset, 3,
		    XmNrightAttachment, XmATTACH_FORM,
		    XmNrightOffset, 3,
		    NULL);
	top_widget = sep;
    }

    this->regularity_label = XtVaCreateWidget(
		"Grid positions", xmLabelWidgetClass, this->generalform,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, top_widget,
		XmNalignment, XmALIGNMENT_BEGINNING,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);
    if(FULL || POSITIONS_IS_ON)
	XtManageChild(this->regularity_label);

    //
    // Create the Regularity Option menu
    //
    
    pulldown_menu = this->createRegularityPulldown(this->generalform);

    n = 0;
    XtSetArg(wargs[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(wargs[n], XmNtopWidget, top_widget); n++;
    XtSetArg(wargs[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(wargs[n], XmNleftWidget, this->regularity_label); n++;
    XtSetArg(wargs[n], XmNsubMenuId, pulldown_menu); n++;
    this->regularity_om = XmCreateOptionMenu(this->generalform, "om", wargs, n);

    if(FULL_POSITIONS)
	XtManageChild(this->regularity_om);

    //
    // Create the Regular/Irregular Option menu
    //
    
    pulldown_menu = this->createPositionsPulldown(this->generalform);

    n = 0;
    XtSetArg(wargs[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(wargs[n], XmNtopWidget, this->regularity_om); n++;
    XtSetArg(wargs[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(wargs[n], XmNleftWidget, this->regularity_label); n++;
    XtSetArg(wargs[n], XmNsubMenuId, pulldown_menu); n++;
    this->positions_om[0] = XmCreateOptionMenu(this->generalform, "om", 
		wargs, n);

    if(FULL_POSITIONS)
	XtManageChild(this->positions_om[0]);

    if(FULL_POSITIONS)
	this->position_label[0] = 
	    XtVaCreateWidget(
		    "od", xmLabelWidgetClass, 
		    this->generalform, 
		    XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		    XmNtopWidget, this->positions_om[0],
		    XmNtopOffset, 4,
		    XmNleftAttachment, XmATTACH_WIDGET,
		    XmNleftWidget, this->positions_om[0],
		    NULL);
    else
	this->position_label[0] = 
	    XtVaCreateWidget(
		    "od", xmLabelWidgetClass, 
		    this->generalform, 
		    XmNtopAttachment, XmATTACH_WIDGET,
		    XmNtopWidget, this->regularity_label,
		    XmNtopOffset, LABEL_TOP_OFFSET,
		    XmNleftAttachment, XmATTACH_FORM,
		    NULL);

    if(FULL_POSITIONS || SIMP_POSITIONS)
	XtManageChild(this->position_label[0]);

    this->position_text[0] = 
	XtVaCreateWidget(
		"position_text[0]", xmTextWidgetClass, 
		this->generalform, 
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNtopWidget, this->position_label[0],
		XmNtopOffset, -4,
		XmNleftAttachment, 
			SIMP_POSITIONS ? XmATTACH_POSITION : XmATTACH_WIDGET,
		XmNleftPosition, GF_POS1,
		XmNleftWidget, this->position_label[0],
		XmNrightAttachment, XmATTACH_FORM,
		NULL);

    if(FULL_POSITIONS || SIMP_POSITIONS)
	XtManageChild(this->position_text[0]);

    //
    // Create the Regular/Irregular Option menu
    //
    
    pulldown_menu = this->createPositionsPulldown(this->generalform);

    n = 0;
    XtSetArg(wargs[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(wargs[n], XmNtopWidget, this->positions_om[0]); n++;
    XtSetArg(wargs[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(wargs[n], XmNleftWidget, this->regularity_label); n++;
    XtSetArg(wargs[n], XmNsubMenuId, pulldown_menu); n++;
    this->positions_om[1] = XmCreateOptionMenu(this->generalform, "om", wargs, n);

    if(FULL_POSITIONS)
	XtManageChild(this->positions_om[1]);

    if(FULL_POSITIONS)
	this->position_label[1] = 
	    XtVaCreateWidget(
		    "od", xmLabelWidgetClass, 
		    this->generalform, 
		    XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		    XmNtopWidget, this->positions_om[1],
		    XmNtopOffset, 4,
		    XmNleftAttachment, XmATTACH_WIDGET,
		    XmNleftWidget, this->positions_om[1],
		    NULL);
    else
	this->position_label[1] = 
	    XtVaCreateWidget(
		    "od", xmLabelWidgetClass, 
		    this->generalform, 
		    XmNtopAttachment, XmATTACH_WIDGET,
		    XmNtopWidget, this->position_label[0],
		    XmNtopOffset, LABEL_TOP_OFFSET,
		    XmNleftAttachment, XmATTACH_FORM,
		    NULL);

    if(FULL_POSITIONS || SIMP_POSITIONS)
	XtManageChild(this->position_label[1]);

    this->position_text[1] = 
	XtVaCreateWidget(
		"position_text[1]", xmTextWidgetClass, 
		this->generalform, 
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNtopWidget, this->position_label[1],
		XmNtopOffset, -4,
		XmNleftAttachment, 
			SIMP_POSITIONS ? XmATTACH_POSITION : XmATTACH_WIDGET,
		XmNleftPosition, GF_POS1,
		XmNleftWidget, this->position_label[1],
		XmNrightAttachment, XmATTACH_FORM,
		NULL);

    if(FULL_POSITIONS || SIMP_POSITIONS)
	XtManageChild(this->position_text[1]);

    //
    // Create the Regular/Irregular Option menu
    //
    
    pulldown_menu = this->createPositionsPulldown(this->generalform);

    n = 0;
    XtSetArg(wargs[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(wargs[n], XmNtopWidget, this->positions_om[1]); n++;
    XtSetArg(wargs[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(wargs[n], XmNleftWidget, this->regularity_label); n++;
    XtSetArg(wargs[n], XmNsubMenuId, pulldown_menu); n++;
    this->positions_om[2] = XmCreateOptionMenu(this->generalform, "om", wargs, n);

    if(FULL_POSITIONS)
	XtManageChild(this->positions_om[2]);

    if(FULL_POSITIONS)
	this->position_label[2] = 
	    XtVaCreateWidget(
		    "od", xmLabelWidgetClass, 
		    this->generalform, 
		    XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		    XmNtopWidget, this->positions_om[2],
		    XmNtopOffset, 4,
		    XmNleftAttachment, XmATTACH_WIDGET,
		    XmNleftWidget, this->positions_om[2],
		    NULL);
    else
	this->position_label[2] = 
	    XtVaCreateWidget(
		    "od", xmLabelWidgetClass, 
		    this->generalform, 
		    XmNtopAttachment, XmATTACH_WIDGET,
		    XmNtopWidget, this->position_label[1],
		    XmNtopOffset, LABEL_TOP_OFFSET,
		    XmNleftAttachment, XmATTACH_FORM,
		    NULL);

    if(FULL_POSITIONS || SIMP_POSITIONS)
	XtManageChild(this->position_label[2]);

    this->position_text[2] = 
	XtVaCreateWidget(
		"position_text[2]", xmTextWidgetClass, 
		this->generalform, 
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNtopWidget, this->position_label[2],
		XmNtopOffset, -4,
		XmNleftAttachment, 
			SIMP_POSITIONS ? XmATTACH_POSITION : XmATTACH_WIDGET,
		XmNleftPosition, GF_POS1,
		XmNleftWidget, this->position_label[2],
		XmNrightAttachment, XmATTACH_FORM,
		NULL);

    if(FULL_POSITIONS || SIMP_POSITIONS)
	XtManageChild(this->position_text[2]);

    //
    // Create the Regular/Irregular Option menu
    //
    
    pulldown_menu = this->createPositionsPulldown(this->generalform);

    n = 0;
    XtSetArg(wargs[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(wargs[n], XmNtopWidget, this->positions_om[2]); n++;
    XtSetArg(wargs[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(wargs[n], XmNleftWidget, this->regularity_label); n++;
    XtSetArg(wargs[n], XmNsubMenuId, pulldown_menu); n++;
    this->positions_om[3] = XmCreateOptionMenu(this->generalform, "om", wargs, n);

    if(FULL_POSITIONS)
	XtManageChild(this->positions_om[3]);

    if(FULL_POSITIONS)
	this->position_label[3] = 
	    XtVaCreateWidget(
		    "od", xmLabelWidgetClass, 
		    this->generalform, 
		    XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		    XmNtopWidget, this->positions_om[3],
		    XmNtopOffset, 4,
		    XmNleftAttachment, XmATTACH_WIDGET,
		    XmNleftWidget, this->positions_om[3],
		    NULL);
    else
	this->position_label[3] = 
	    XtVaCreateWidget(
		    "od", xmLabelWidgetClass, 
		    this->generalform, 
		    XmNtopAttachment, XmATTACH_WIDGET,
		    XmNtopWidget, this->position_label[2],
		    XmNtopOffset, LABEL_TOP_OFFSET,
		    XmNleftAttachment, XmATTACH_FORM,
		    NULL);


    if(FULL_POSITIONS || SIMP_POSITIONS)
	XtManageChild(this->position_label[3]);

    this->position_text[3] = 
	XtVaCreateWidget(
		"position_text[3]", xmTextWidgetClass, 
		this->generalform, 
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNtopWidget, this->position_label[3],
		XmNtopOffset, -4,
		XmNleftAttachment, 
			SIMP_POSITIONS ? XmATTACH_POSITION : XmATTACH_WIDGET,
		XmNleftPosition, GF_POS1,
		XmNleftWidget, this->position_label[3],
		XmNrightAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);

    if(FULL_POSITIONS || SIMP_POSITIONS)
	XtManageChild(this->position_text[3]);

    //
    // Set up MV callback
    //
    for(i = 0; i < 4; i++)
    {
	XtAddCallback(this->position_text[i],
	      XmNmodifyVerifyCallback,
	      (XtCallbackProc)GARMainWindow_OriginDeltaMVCB,
	      (XtPointer)this);
	XtAddCallback(this->position_text[i],
	      XmNactivateCallback,
	      (XtCallbackProc)GARMainWindow_VerifyPositionCB,
	      (XtPointer)this);
    }

    //
    // Init the dimension related sensitivities
    //
    for(i = 0; i < 4; i++)
    {
	XtSetSensitive(this->positions_om[i], False);
	XtSetSensitive(this->position_label[i], (i < this->dimension));
	this->setTextSensitivity(this->position_text[i], 
	    (i < this->dimension));

	XmTextSetString(position_text[i], "0, 1");
    }

    //
    // Create the "fieldform" side of the world
    //
    label = 
	XtVaCreateManagedWidget(
		"field_list_label", xmLabelWidgetClass, 
		this->fieldform, 
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

    n = 0;
    XtSetArg(wargs[n], XmNtopAttachment, XmATTACH_FORM); n++;
    XtSetArg(wargs[n], XmNleftAttachment, XmATTACH_POSITION); n++;
    XtSetArg(wargs[n], XmNleftPosition, 30); n++;
    XtSetArg(wargs[n], XmNrightAttachment, XmATTACH_POSITION); n++;
    XtSetArg(wargs[n], XmNrightPosition, 70); n++;

    // How big to make it?
    int vlines;
    if(FULL) vlines = 10; else if(SIMP_POSITIONS) vlines = 6; else vlines = 3;

    XtSetArg(wargs[n], XmNvisibleItemCount, vlines); n++;
    XtSetArg(wargs[n], XmNselectionPolicy, XmSINGLE_SELECT); n++;
//    XtSetArg(wargs[n], XmNlistSizePolicy, XmRESIZE_IF_POSSIBLE); n++;
    XtSetArg(wargs[n], XmNlistSizePolicy, XmCONSTANT); n++;
    XtSetArg(wargs[n], XmNhighlightThickness, 2); n++;
    this->field_list = 
	XmCreateScrolledList(this->fieldform, "fieldlist", wargs, n);
    XtManageChild(this->field_list);

    XtAddCallback(this->field_list,
	  XmNsingleSelectionCallback,
	  (XtCallbackProc)GARMainWindow_ListSelectCB,
	  (XtPointer)this);

    this->move_up_ab = 
	XtVaCreateManagedWidget(
		"move_up", xmArrowButtonWidgetClass, 
		this->fieldform, 
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNtopWidget, XtParent(this->field_list),
		XmNtopOffset, 0,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, XtParent(this->field_list),
		XmNleftOffset, 28,
		XmNarrowDirection, XmARROW_UP,
		NULL);

    this->move_down_ab = 
	XtVaCreateManagedWidget(
		"move_down", xmArrowButtonWidgetClass, 
		this->fieldform, 
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, this->move_up_ab,
		XmNtopOffset, 5,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, XtParent(this->field_list),
		XmNleftOffset, 28,
		XmNarrowDirection, XmARROW_DOWN,
		NULL);

    xms = XmStringCreateLtoR("Move\nfield", XmSTRING_DEFAULT_CHARSET);

    this->move_label =
	XtVaCreateManagedWidget(
		"move_label", xmLabelWidgetClass, 
		this->fieldform, 
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNtopWidget, XtParent(this->field_list),
		XmNtopOffset, -4,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, this->move_up_ab,
		XmNlabelString, xms,
		NULL);
    XmStringFree(xms);

    XtAddCallback(this->move_up_ab,
	  XmNactivateCallback,
	  (XtCallbackProc)GARMainWindow_MoveFieldCB,
	  (XtPointer)this);
    XtAddCallback(this->move_down_ab,
	  XmNactivateCallback,
	  (XtCallbackProc)GARMainWindow_MoveFieldCB,
	  (XtPointer)this);

    label = 
	XtVaCreateManagedWidget(
		"field_name_label", xmLabelWidgetClass, 
		this->fieldform, 
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, XtParent(this->field_list),
		XmNtopOffset, LABEL_TOP_OFFSET,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

    this->field_text = 
	XtVaCreateManagedWidget(
		"field_text", xmTextWidgetClass, 
		this->fieldform, 
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, XtParent(this->field_list),
		XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNleftWidget, XtParent(this->field_list),
		XmNleftOffset, 0,
		XmNrightAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNrightWidget, XtParent(this->field_list),
		XmNrightOffset, 0,
		NULL);

    XtAddCallback(this->field_text,
	  XmNmodifyVerifyCallback,
	  (XtCallbackProc)GARMainWindow_IdentifierMVCB,
	  (XtPointer)this);

/*
    sep = XtVaCreateManagedWidget("optionSeparator",
		xmSeparatorWidgetClass, this->fieldform,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, this->field_text,
		XmNleftAttachment, XmATTACH_FORM,
		XmNleftOffset, 3,
		XmNrightAttachment, XmATTACH_FORM,
		XmNrightOffset, 3,
		NULL);
    label = 
	XtVaCreateManagedWidget(
		"Data", xmLabelWidgetClass, 
		this->fieldform, 
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, sep,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		XmNalignment, XmALIGNMENT_CENTER,
		NULL);
		XmNtopOffset, LABEL_TOP_OFFSET,  */

    this->type_label = 
	XtVaCreateManagedWidget(
		"Type", xmLabelWidgetClass, 
		this->fieldform, 
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, field_text,
		XmNtopOffset, LABEL_TOP_OFFSET,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);
    //
    // Create the Double/Float/Int/Short/Char option menu
    //
    
    pulldown_menu = this->createTypePulldown(this->fieldform);

    n = 0;
    XtSetArg(wargs[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(wargs[n], XmNtopWidget, label); n++;
    XtSetArg(wargs[n], XmNleftAttachment, XmATTACH_FORM); n++;
    XtSetArg(wargs[n], XmNleftOffset, FIELD_FORM_COL); n++;
    XtSetArg(wargs[n], XmNsubMenuId, pulldown_menu); n++;
    this->type_om = XmCreateOptionMenu(this->fieldform, "om", wargs, n);

    XtManageChild(this->type_om);

    this->structure_label = 
	XtVaCreateManagedWidget(
		"Structure", xmLabelWidgetClass, 
		this->fieldform, 
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, this->type_om,
		XmNtopOffset, LABEL_TOP_OFFSET,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);
    //
    // Create the Structure option menu
    //
    
    pulldown_menu = this->createStructurePulldown(this->fieldform);

    n = 0;
    XtSetArg(wargs[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(wargs[n], XmNtopWidget, this->type_om); n++;
    XtSetArg(wargs[n], XmNleftAttachment, XmATTACH_FORM); n++;
    XtSetArg(wargs[n], XmNleftOffset, FIELD_FORM_COL); n++;
    XtSetArg(wargs[n], XmNsubMenuId, pulldown_menu); n++;
    this->structure_om = XmCreateOptionMenu(this->fieldform, "om", wargs, n);

    XtManageChild(this->structure_om);


    //
    // Create the size stepper (used only for string sizes as of 7/95)
    //
    n = 0;
    XtSetArg (wargs[n], XmNcolumns, 3); n++;
    XtSetArg (wargs[n], XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
    XtSetArg (wargs[n], XmNtopWidget, this->structure_om); n++;
    XtSetArg (wargs[n], XmNtopOffset, 2); n++;
    XtSetArg (wargs[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
    XtSetArg (wargs[n], XmNleftWidget, this->structure_om); n++;
    XtSetArg (wargs[n], XmNleftOffset, 100); n++;
    XtSetArg (wargs[n], XmNsensitive, False); n++;
    XtSetArg (wargs[n], XmNeditMode, XmSINGLE_LINE_EDIT); n++;
    XtSetArg (wargs[n], XmNmaxLength, 8); n++;
    this->string_size = XmCreateText (this->fieldform, "string_size", wargs, n);
    XtManageChild(this->string_size);
    XtAddCallback(this->string_size,
	  XmNmodifyVerifyCallback,
	  (XtCallbackProc)GARMainWindow_IntegerMVCB,
	  (XtPointer)this);

    XtAddCallback(this->string_size,
	  XmNactivateCallback,
	  (XtCallbackProc)GARMainWindow_VerifyNonZeroCB,
	  (XtPointer)this);

    //
    // Create a label to go with string_size
    //
    n = 0;
    XtSetArg (wargs[n], XmNalignment, XmALIGNMENT_END); n++;
    XtSetArg (wargs[n], XmNleftAttachment, XmATTACH_NONE); n++;
    XtSetArg (wargs[n], XmNrightAttachment, XmATTACH_WIDGET); n++;
    XtSetArg (wargs[n], XmNrightWidget, this->string_size); n++;
    XtSetArg (wargs[n], XmNrightOffset, 4); n++;
    XtSetArg (wargs[n], XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
    XtSetArg (wargs[n], XmNtopWidget, this->string_size); n++;
    XtSetArg (wargs[n], XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
    XtSetArg (wargs[n], XmNbottomWidget, this->string_size); n++;
    XtSetArg (wargs[n], XmNtopOffset, 0); n++;
    XtSetArg (wargs[n], XmNbottomOffset, 0); n++;
    XtSetArg (wargs[n], XmNsensitive, False); n++;
    this->string_size_label = XmCreateLabel (this->fieldform, "string size", wargs, n);
    XtManageChild (this->string_size_label);


    this->dependency_label = 
	XtVaCreateWidget(
		"Dependency", xmLabelWidgetClass, 
		this->fieldform, 
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, this->structure_om,
		XmNtopOffset, LABEL_TOP_OFFSET,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

    if(FULL)
	XtManageChild(this->dependency_label);


    //
    // Create the Dependency option menu
    //
    
    pulldown_menu = this->createDependencyPulldown(this->fieldform);

    n = 0;
    XtSetArg(wargs[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(wargs[n], XmNtopWidget, this->structure_om); n++;
    XtSetArg(wargs[n], XmNleftAttachment, XmATTACH_FORM); n++;
    XtSetArg(wargs[n], XmNleftOffset, FIELD_FORM_COL); n++;
    XtSetArg(wargs[n], XmNsubMenuId, pulldown_menu); n++;
    this->dependency_om = XmCreateOptionMenu(this->fieldform, "om", wargs, n);

    if(FULL)
	XtManageChild(this->dependency_om);

    this->setDependencySensitivity(True);



    this->layout_tb = 
	XtVaCreateWidget(
		"Layout", xmToggleButtonWidgetClass, 
		this->fieldform, 
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, this->dependency_om,
		XmNtopOffset, LABEL_TOP_OFFSET,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

    XtAddCallback(this->layout_tb,
	  XmNvalueChangedCallback,
	  (XtCallbackProc)GARMainWindow_LayoutTBCB,
	  (XtPointer)this);

    if(FULL)
	XtManageChild(this->layout_tb);

    this->layout_skip_text = 
	XtVaCreateWidget(
		"layout_skip_text", xmTextWidgetClass, 
		this->fieldform, 
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, this->dependency_om,
		XmNleftAttachment, XmATTACH_FORM,
		XmNleftOffset, FIELD_FORM_COL + 10,
		XmNwidth, SHORT_TEXT_WIDTH,
		NULL);

    if(FULL)
	XtManageChild(this->layout_skip_text);

    XtAddCallback(this->layout_skip_text,
	  XmNmodifyVerifyCallback,
	  (XtCallbackProc)GARMainWindow_IntegerMVCB,
	  (XtPointer)this);

    this->layout_skip_label = 
	XtVaCreateWidget(
		"skip", xmLabelWidgetClass, 
		this->fieldform, 
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, this->dependency_om,
		XmNtopOffset, LABEL_TOP_OFFSET,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNrightWidget, this->layout_skip_text,
		NULL);

    if(FULL)
	XtManageChild(this->layout_skip_label);

    this->layout_width_label = 
	XtVaCreateWidget(
		"width", xmLabelWidgetClass, 
		this->fieldform, 
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, this->dependency_om,
		XmNtopOffset, LABEL_TOP_OFFSET,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, this->layout_skip_text,
		XmNleftOffset, 18,
		NULL);

    if(FULL)
	XtManageChild(this->layout_width_label);

    this->layout_width_text = 
	XtVaCreateWidget(
		"layout_width_text", xmTextWidgetClass, 
		this->fieldform, 
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, this->dependency_om,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, this->layout_width_label,
		XmNwidth, SHORT_TEXT_WIDTH,
		NULL);

    if(FULL)
	XtManageChild(this->layout_width_text);

    XtAddCallback(this->layout_width_text,
	  XmNmodifyVerifyCallback,
	  (XtCallbackProc)GARMainWindow_IntegerMVCB,
	  (XtPointer)this);

    this->block_tb = 
	XtVaCreateWidget(
		"Block", xmToggleButtonWidgetClass, 
		this->fieldform, 
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, this->layout_skip_text,
		XmNtopOffset, LABEL_TOP_OFFSET,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

    XtAddCallback(this->block_tb,
	  XmNvalueChangedCallback,
	  (XtCallbackProc)GARMainWindow_BlockTBCB,
	  (XtPointer)this);

    if(FULL)
	XtManageChild(this->block_tb);

    this->block_skip_text = 
	XtVaCreateWidget(
		"block_skip_text", xmTextWidgetClass, 
		this->fieldform, 
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, this->layout_skip_text,
		XmNleftAttachment, XmATTACH_FORM,
		XmNleftOffset, FIELD_FORM_COL + 10,
		XmNwidth, SHORT_TEXT_WIDTH,
		NULL);

    if(FULL)
	XtManageChild(this->block_skip_text);

    XtAddCallback(this->block_skip_text,
	  XmNmodifyVerifyCallback,
	  (XtCallbackProc)GARMainWindow_IntegerMVCB,
	  (XtPointer)this);

    this->block_skip_label = 
	XtVaCreateWidget(
		"skip", xmLabelWidgetClass, 
		this->fieldform, 
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, this->layout_skip_text,
		XmNtopOffset, LABEL_TOP_OFFSET,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNrightWidget, this->block_skip_text,
		NULL);

    if(FULL)
	XtManageChild(this->block_skip_label);

    this->block_element_label = 
	XtVaCreateWidget(
		"num_el_label", xmLabelWidgetClass, 
		this->fieldform, 
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, this->layout_skip_text,
		XmNtopOffset, LABEL_TOP_OFFSET,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, this->block_skip_text,
		NULL);

    if(FULL)
	XtManageChild(this->block_element_label);

    this->block_element_text = 
	XtVaCreateWidget(
		"block_element_text", xmTextWidgetClass, 
		this->fieldform, 
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, this->layout_skip_text,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, this->block_element_label,
		XmNwidth, SHORT_TEXT_WIDTH,
		NULL);

    if(FULL)
	XtManageChild(this->block_element_text);

    XtAddCallback(this->block_element_text,
	  XmNmodifyVerifyCallback,
	  (XtCallbackProc)GARMainWindow_IntegerMVCB,
	  (XtPointer)this);

    this->block_width_label = 
	XtVaCreateWidget(
		"width", xmLabelWidgetClass, 
		this->fieldform, 
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, this->layout_skip_text,
		XmNtopOffset, LABEL_TOP_OFFSET,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, this->block_element_text,
		NULL);

    if(FULL)
	XtManageChild(this->block_width_label);

    this->block_width_text = 
	XtVaCreateWidget(
		"block_width_text", xmTextWidgetClass, 
		this->fieldform, 
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, this->layout_skip_text,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, this->block_width_label,
		XmNwidth, SHORT_TEXT_WIDTH,
		NULL);

    if(FULL)
	XtManageChild(this->block_width_text);

    XtAddCallback(this->block_width_text,
	  XmNmodifyVerifyCallback,
	  (XtCallbackProc)GARMainWindow_IntegerMVCB,
	  (XtPointer)this);

    this->setBlockSensitivity();

    this->add_pb = 
	XtVaCreateManagedWidget(
		"Add", xmPushButtonWidgetClass, 
		this->fieldform, 
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, FULL ? this->block_width_text : 
				     this->structure_om,
		XmNtopOffset, 20,
		XmNrecomputeSize, False,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNleftPosition, 4,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNrightPosition, 24,
		NULL);

    this->insert_pb = 
	XtVaCreateManagedWidget(
		"Insert", xmPushButtonWidgetClass, 
		this->fieldform, 
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, FULL ? this->block_width_text : 
				     this->structure_om,
		XmNtopOffset, 20,
		XmNrecomputeSize, False,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNleftPosition, 28,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNrightPosition, 48,
		NULL);

    this->modify_pb = 
	XtVaCreateManagedWidget(
		"Modify", xmPushButtonWidgetClass, 
		this->fieldform, 
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, FULL ? this->block_width_text : 
				     this->structure_om,
		XmNtopOffset, 20,
		XmNrecomputeSize, False,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNleftPosition, 52,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNrightPosition, 72,
		NULL);

    this->delete_pb = 
	XtVaCreateManagedWidget(
		"Delete", xmPushButtonWidgetClass, 
		this->fieldform, 
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, FULL ? this->block_width_text : 
				     this->structure_om,
		XmNtopOffset, 20,
		XmNrecomputeSize, False,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNleftPosition, 76,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNrightPosition, 96,
		NULL);

    XtSetSensitive(this->modify_pb, False);
    XtSetSensitive(this->delete_pb, False);


    XtAddCallback(this->add_pb,
	  XmNactivateCallback,
	  (XtCallbackProc)GARMainWindow_AddFieldCB,
	  (XtPointer)this);

    XtAddCallback(this->delete_pb,
	  XmNactivateCallback,
	  (XtCallbackProc)GARMainWindow_DeleteFieldCB,
	  (XtPointer)this);

    XtAddCallback(this->insert_pb,
	  XmNactivateCallback,
	  (XtCallbackProc)GARMainWindow_InsertFieldCB,
	  (XtPointer)this);

    XtAddCallback(this->modify_pb,
	  XmNactivateCallback,
	  (XtCallbackProc)GARMainWindow_ModifyFieldCB,
	  (XtPointer)this);

	xms = XmStringCreateSimple(" ");
    this->modify_tb = 
	XtVaCreateManagedWidget(
		"modify_tb", xmToggleButtonWidgetClass, 
		this->fieldform, 
		XmNvisibleWhenOff, False,
		XmNlabelString, xms,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, this->modify_pb,
	/*	XmNleftAttachment, XmATTACH_POSITION, */
        /*	XmNleftPosition, 2,                   */
		XmNleftAttachment, XmATTACH_FORM,
        /*	XmNrightAttachment, XmATTACH_POSITION,*/
        /*	XmNrightPosition, 98,                 */ 
		XmNrightAttachment, XmATTACH_FORM,
		XmNrecomputeSize, True,              
        	XmNalignment, XmALIGNMENT_BEGINNING,  
		RES_CONVERT(XmNselectColor, "Yellow"), 
		NULL);
		
	XmStringFree(xms);

    XtUninstallTranslations(this->modify_tb);

    sep = XtVaCreateWidget("RecordSeparatorSeparator", 
		xmSeparatorWidgetClass, this->fieldform, 
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, this->modify_pb, 
		XmNtopOffset, 85, 
		XmNleftAttachment, XmATTACH_FORM,
		XmNleftOffset, 3,
		XmNrightAttachment, XmATTACH_FORM,
		XmNrightOffset, 3,
		NULL);
    if (FULL)
	XtManageChild(sep);

    this->record_sep_tb = 
	XtVaCreateWidget(
		"Record Separator", xmToggleButtonWidgetClass, 
		this->fieldform, 
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, sep,
		XmNtopOffset, LABEL_TOP_OFFSET,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

    XtAddCallback(this->record_sep_tb,
	  XmNvalueChangedCallback,
	  (XtCallbackProc)GARMainWindow_RecsepTBCB,
	  (XtPointer)this);

    if(FULL)
	XtManageChild(this->record_sep_tb);


    //
    // Now make the radio buttons for record separator options
    //
    n=0;
    XtSetArg(wargs[n], XmNtopAttachment, XmATTACH_WIDGET);      n++;
    XtSetArg(wargs[n], XmNtopWidget, record_sep_tb);            n++;
    XtSetArg(wargs[n], XmNleftAttachment, XmATTACH_FORM);       n++;
    XtSetArg(wargs[n], XmNspacing, 15);				n++;
    XtSetArg(wargs[n], XmNshadowThickness, 1);                  n++;
    XtSetArg(wargs[n], XmNradioAlwaysOne, False);               n++;
    XtSetArg(wargs[n], XmNshadowType, XmSHADOW_IN);             n++;    

    rbrec = XmCreateRadioBox(this->fieldform, "radiobox1", wargs, n);
    if (FULL)
      XtManageChild(rbrec);

    xms = XmStringCreateSimple("same between all records");

    /* XXX will needs some smarts here about which is set */
    this->same_sep_tb = XtVaCreateWidget(
                "same sep", xmToggleButtonWidgetClass, rbrec,
                XmNindicatorOn, 1,
                XmNleftOffset, 25,
                XmNlabelString, xms,
                NULL);
    XmStringFree(xms);

    XtAddCallback(this->same_sep_tb,
          XmNvalueChangedCallback,
          (XtCallbackProc)GARMainWindow_SameSepCB,
          (XtPointer)this);

    if (FULL)
      XtManageChild(same_sep_tb);

    /* initialize the "list" */
    RecordSeparator *recsep = new RecordSeparator("");
    recsep->setType("# of bytes");
    recsep->setNum(NULL); 
    this->recordsepSingle.appendElement(recsep); 

    xms = XmStringCreateSimple("between records:");
    this->diff_sep_tb = XtVaCreateWidget(
                "diff sep", xmToggleButtonWidgetClass, rbrec,
                XmNindicatorOn, 1,  
                XmNleftOffset, 25,
                XmNlabelString, xms,
                NULL);
    XmStringFree(xms);

    XtAddCallback(this->diff_sep_tb,
          XmNvalueChangedCallback,
          (XtCallbackProc)GARMainWindow_DiffSepCB,
          (XtPointer)this);

    if (FULL)
      XtManageChild(diff_sep_tb);

    //
    // Create the Record Separator option menu
    //
   
    this->record_sep_text = 
	XtVaCreateWidget(
		"record_sep_text", xmTextWidgetClass, 
		this->fieldform, 
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNtopWidget, rbrec,
		XmNrightAttachment, XmATTACH_FORM,
		XmNwidth, SHORT_TEXT_WIDTH2,
		NULL); 

    if(FULL)
	XtManageChild(this->record_sep_text);

    XtAddCallback(this->record_sep_text,
	  XmNmodifyVerifyCallback,
	  (XtCallbackProc)GARMainWindow_IntegerMVCB,
	  (XtPointer)this);

    XtAddCallback(this->record_sep_text,
	  XmNactivateCallback,
	  (XtCallbackProc)GARMainWindow_RecSepTextCB,
	  (XtPointer)this);

    pulldown_menu = this->createHeaderPulldown(this->fieldform, FALSE,  FALSE,
                                               TRUE);
    n = 0;
    XtSetArg(wargs[n], XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
    XtSetArg(wargs[n], XmNtopWidget, this->record_sep_text); n++;
    XtSetArg(wargs[n], XmNtopOffset, -3); n++;
    XtSetArg(wargs[n], XmNrightAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(wargs[n], XmNrightWidget, this->record_sep_text); n++;
    XtSetArg(wargs[n], XmNsubMenuId, pulldown_menu); n++;
    this->record_sep_om = XmCreateOptionMenu(this->fieldform, "om", wargs, n);

    if(FULL)
	XtManageChild(this->record_sep_om);

    this->record_sep_text_1 = 
	XtVaCreateWidget(
		"record_sep_text_1", xmTextWidgetClass, 
		this->fieldform, 
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, this->record_sep_text,
		XmNtopOffset, 35,
		XmNrightAttachment, XmATTACH_FORM,
		XmNwidth, SHORT_TEXT_WIDTH2,
		NULL);

    if(FULL)
	XtManageChild(this->record_sep_text_1);

    XtAddCallback(this->record_sep_text_1,
	  XmNmodifyVerifyCallback,
	  (XtCallbackProc)GARMainWindow_IntegerMVCB,
	  (XtPointer)this);

    XtAddCallback(this->record_sep_text_1,
	  XmNactivateCallback,
	  (XtCallbackProc)GARMainWindow_RecSepTextCB,
	  (XtPointer)this);


    pulldown_menu = this->createHeaderPulldown(this->fieldform, FALSE, TRUE,
                                               FALSE);
    n = 0;
    XtSetArg(wargs[n], XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
    XtSetArg(wargs[n], XmNtopWidget, this->record_sep_text_1); n++;
    XtSetArg(wargs[n], XmNtopOffset, -3); n++;
    XtSetArg(wargs[n], XmNrightAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(wargs[n], XmNrightWidget, this->record_sep_text_1); n++;
    XtSetArg(wargs[n], XmNsubMenuId, pulldown_menu); n++;
    this->record_sep_om_1 = XmCreateOptionMenu(this->fieldform, "om", wargs, n);

    if(FULL)
		XtManageChild(this->record_sep_om_1);

    n = 0;
    XtSetArg(wargs[n], XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
    XtSetArg(wargs[n], XmNtopWidget, this->record_sep_text_1); n++;
//    XtSetArg(wargs[n], XmNtopOffset, -3); n++;
    XtSetArg(wargs[n], XmNrightAttachment, XmATTACH_POSITION); n++;   
    XtSetArg(wargs[n], XmNrightPosition, 50); n++;   
    XtSetArg(wargs[n], XmNleftAttachment, XmATTACH_FORM); n++;     
    XtSetArg(wargs[n], XmNvisibleItemCount, 3); n++;
    XtSetArg(wargs[n], XmNselectionPolicy, XmSINGLE_SELECT); n++;
    XtSetArg(wargs[n], XmNlistSizePolicy, XmCONSTANT); n++;
    XtSetArg(wargs[n], XmNscrollBarDisplayPolicy, XmSTATIC); n++;
    XtSetArg(wargs[n], XmNbottomAttachment, XmATTACH_FORM); n++;   

// The record_sep_list is the one that is causing the blemish in
// the simplified prompter.
    this->record_sep_list =
        XmCreateScrolledList(this->fieldform, "recseplist", wargs, n);

    XtAddCallback(this->record_sep_list,
	  XmNsingleSelectionCallback,
	  (XtCallbackProc)GARMainWindow_RecordSepListSelectCB,
	  (XtPointer)this);

    if(FULL)
        XtManageChild(this->record_sep_list);

    if (FULL)
         this->setRecordSepSensitivity();

    //
    // Save a set of translations away for later use.
    //
    XtVaGetValues(this->insens_vector_interleaving_om, XmNtranslations,
        &translations, NULL);
    XtVaGetValues(this->row_tb, XmNtranslations,
        &tb_translations, NULL);

    //
    // Make the insensitive option menu insensitive.
    //
    XtUninstallTranslations(this->insens_vector_interleaving_om);

    this->newGAR(this->mode);

    theGARApplication->addHelpCallbacks(parent);
    this->setBlockLayoutSensitivity();
    this->setMoveFieldSensitivity();
    this->setMajoritySensitivity(!POINTS_ONLY);

    //
    // return the toplevel wid
    //
    return frame;
}

unsigned long GARMainWindow::getMode(char *filenm)
{
    std::ifstream *from = new std::ifstream(filenm);
    if(!*from)
    {
	WarningMessage("File open failed in getMode");
	return GMW_FULL;
    }

    return getMode(from);
}
unsigned long GARMainWindow::getMode(std::istream *from)
{
int	num_error = 0;
int	ndx, ndim;
char	line[4096];
int	lineno = 0;
char	*str;
int	cur_size, i;
Boolean	grid;
Boolean	series		= False;
Boolean	layout		= False;
Boolean	block		= False;
Boolean	spreadsheet	= False;
Boolean	positions	= False;
Boolean	irregular	= False;
Boolean	get_rest	= False;
Boolean	recsep		= False;

    while(from->getline(line, 4096))
    {
	lineno++;
	ndx = 0;
	SkipWhiteSpace(line, ndx);
	if(IsToken(line, "grid", ndx)) 
        {
	    grid = True; 
            ndim = GetNumDim(line);
        }
	else if(IsToken(line, "points", ndx))
        {
	    grid = False;
            ndim = 1;
        }
	else if(IsToken(line, "series", ndx))
	{
	    series = True;
	}
	else if(IsToken(line, "layout", ndx))
	{
	    layout = True;
	}
	else if(IsToken(line, "block", ndx))
	{
	    block = True;
	}
	else if(IsToken(line, "positions", ndx))
	{
	    positions = True;

	    while((line[ndx++] != '=') && line[ndx]);
	    SkipWhiteSpace(line, ndx);

	    cur_size = STRLEN(line);
	    str = (char *)CALLOC(cur_size, sizeof(char));

	    strcat(str, &line[ndx]);

	    // Concat the positions together (they may be on more than 1 line)
	    while(from->getline(line, 4096))
	    {
		ndx = 0;
		SkipWhiteSpace(line, ndx);
		if(IsToken(line, "end", ndx))
		{
		    break;
		}
		else
		{
		    cur_size += STRLEN(line)+1;
		    str = (char *)REALLOC(str, cur_size);
		    strcat(str, line);
		}
	    }

	    //
	    // Now we have one continuous string to process.
	    //
	    ndx = 0;
            SkipWhiteSpace(str, ndx);
            if (IsToken(str, "regular", ndx))
               get_rest = True;
            else if (IsToken(str, "irregular", ndx))
            {
               get_rest = True;
               irregular = True;
            }
            if (get_rest) 
            {
               SkipWhiteSpace(str, ndx);
               if (str[ndx] == ',') ndx++;
               SkipWhiteSpace(str, ndx);
               for (i=1; i< ndim; i++)
               {
                  SkipWhiteSpace(str, ndx);
                  if (IsToken(str, "regular", ndx))
                     break;
                  else if (IsToken(str,"irregular", ndx))
                     irregular = True;
                  else 
                     WarningMessage("Parse error in positions statement. Inconsistent dimension.");
               }
            }
            /* else no keywords on line: either regular or explicit */
            else
            {
               i = 0;
               ndx = 0;
               SkipWhiteSpace(str, ndx);
               while (str[ndx]) 
               {
                 if (str[ndx] == ',') ndx++;
                 while ((str[ndx] != ' ')&&(str[ndx])) ndx++;
                 SkipWhiteSpace(str,ndx);
                 i++;
               }
               if (ndim > 0) 
               {
                  if (i*2 != ndim) 
		    irregular = True;
               }
            }
	}
	else if(IsToken(line, "recordseparator", ndx))
	{
	    recsep = True;
	}
	else if(IsToken(line, "interleaving", ndx))
	{
	    while((line[ndx++] != '=') && line[ndx]);

	    SkipWhiteSpace(line, ndx);

	    if(IsToken(line, "record-vector", ndx))
	    {
		spreadsheet = False;
	    }
	    else if((IsToken(line, "vector", ndx)) || 
		    (IsToken(line, "series-vector", ndx)))
	    {
		series = True;
		spreadsheet = False;
	    }
	    else if(IsToken(line, "record", ndx))
	    {
		spreadsheet = False;
	    }
	    else if(IsToken(line, "field", ndx))
	    {
		spreadsheet = True;
	    }
	}
	else if(IsToken(line, "file", ndx)) {;}
	else if(IsToken(line, "format", ndx)) {;}
	else if(IsToken(line, "field", ndx)) {;}
	else if(IsToken(line, "structure", ndx)) {;}
	else if(IsToken(line, "type", ndx)) {;}
	else if(IsToken(line, "header", ndx)) {;}
	else if(IsToken(line, "majority", ndx)) {;}
	else if(IsToken(line, "dependency", ndx)) {;}
	else if(IsToken(line, "#", ndx)) {;}
	else if(STRLEN(&line[ndx]) == 0) {;}
	else if(IsToken(line, "end", ndx))
	{
	    break;
	}
	else
	{
	    num_error++;
	    if(num_error > 3)
	    {
		ErrorMessage("Too many warnings, giving up");
		return FALSE;
	    }
	    else
	    {
	     WarningMessage("Parse error: line number = %d", lineno);
	    }
	}
    }

    unsigned long mode = 0;

    if(irregular || recsep || block || layout || series)
	mode = GMW_FULL;

    /* why was this here? */
    /*
    if(!grid && positions)
	mode = GMW_FULL;
    */


    if(series)
	mode |= GMW_SERIES;
    if(grid)
	mode |= GMW_GRID;
    else
	mode |= GMW_POINTS;
    if(spreadsheet)
	mode |= GMW_SPREADSHEET;
    if(positions)
	mode |= GMW_POSITIONS;
    if(!(mode & GMW_FULL))
	if(positions)
	    mode |= GMW_SIMP_POSITIONS;
    return mode;
}

void GARMainWindow::setFilename(char *filenm)
{
    ASSERT(filenm);
    if(this->filename)
	delete this->filename;
    this->filename = DuplicateString(filenm);
    this->setWindowTitle(filenm);
    if(filenm)
	this->saveCmd->activate();
    else
	this->saveCmd->deactivate();
}

extern "C" void GARMainWindow_MapEH(Widget w, XtPointer clientData, XEvent *e, Boolean *)
{
    GARMainWindow 	*gmw = (GARMainWindow *) clientData;

    if(e->type != MapNotify) return;

    //
    // Find the Application shell
    //
    while(!XtIsApplicationShell(w)) w = XtParent(w);

    //
    // Get the height/width
    //
    Dimension height;
    Dimension width;
    XtVaGetValues(w, XmNheight, &height, XmNwidth, &width, NULL);

    //
    // Don't allow height changes
    // Fix the min width
    //
    XtVaSetValues(w, XmNminHeight, height, 
		     XmNmaxHeight, height,
		     XmNminWidth, width,
		     NULL);

    Dimension height1, height2;
    XtVaGetValues(gmw->position_text[2], XmNheight, &height1, NULL);
    XtVaGetValues(gmw->position_text[3], XmNheight, &height2, NULL);
    if(height2 > height1)
    {
	XtVaSetValues(gmw->position_text[3],
		XmNbottomAttachment, XmATTACH_NONE,
		XmNheight, height1,
		NULL);
    }

    XtRemoveEventHandler(gmw->generalform, StructureNotifyMask, False,
        (XtEventHandler)GARMainWindow_MapEH, XtPointer(gmw));
}

extern "C" void GARMainWindow_MoveFieldCB(Widget w, XtPointer clientData, XtPointer)
{
    GARMainWindow 	*gmw = (GARMainWindow *) clientData;
    int			*position_list;
    int			position_count;
    int			pos;
    Field		*field;

    //
    // Get the selected list item
    //
    if(!XmListGetSelectedPos(gmw->field_list, &position_list, &position_count))
    {
	return;
    }
    else
    {
	pos = position_list[0];
	XtFree((char *)position_list); 
    }

    //
    // Take care of the widget first
    //
    XmListDeletePos(gmw->field_list, pos);

    field = (Field *)gmw->fieldList.getElement(pos);
    gmw->fieldList.deleteElement(pos);

    if(w == gmw->move_up_ab)
    {
	pos--;
    }
    else
    {
	pos++;
    }
    XmString xmstr = XmStringCreateSimple(field->getName());
    XmListAddItem(gmw->field_list, xmstr, pos);
    XmListSelectPos(gmw->field_list, pos, False);
    gmw->fieldList.insertElement(field, pos);
    gmw->current_field = pos;
    gmw->setMoveFieldSensitivity(); 


    /* gmw->setFieldDirty(); */
    theGARApplication->setDirty(); 
    gmw->updateRecordSeparatorList(); 
}

void GARMainWindow::updateRecordSeparatorList()
{
    int n;
    Arg wargs[50];

    Boolean managed = XtIsManaged(this->record_sep_list);
    if(managed)
    {
	XtUnmanageChild(this->record_sep_list);

        n = 0;
        XtSetArg(wargs[n], XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
        XtSetArg(wargs[n], XmNtopWidget, this->record_sep_text_1); n++;
        XtSetArg(wargs[n], XmNtopOffset, -3); n++;
        XtSetArg(wargs[n], XmNrightAttachment, XmATTACH_POSITION); n++;   
        XtSetArg(wargs[n], XmNrightPosition, 50); n++;   
        XtSetArg(wargs[n], XmNleftAttachment, XmATTACH_FORM); n++;     
        XtSetArg(wargs[n], XmNvisibleItemCount, 3); n++;
        XtSetArg(wargs[n], XmNselectionPolicy, XmSINGLE_SELECT); n++;
        XtSetArg(wargs[n], XmNlistSizePolicy, XmCONSTANT); n++;
        XtSetArg(wargs[n], XmNscrollBarDisplayPolicy, XmSTATIC); n++;
        XtSetArg(wargs[n], XmNbottomAttachment, XmATTACH_FORM); n++;     
        this->record_sep_list =
            XmCreateScrolledList(this->fieldform, "recseplist", wargs, n);

        this->createRecordSeparatorList(this->fieldform);

	XtManageChild(this->record_sep_list);
        XtAddCallback(this->record_sep_list,
	  XmNsingleSelectionCallback,
	  (XtCallbackProc)GARMainWindow_RecordSepListSelectCB,
	  (XtPointer)this);
        /* select the first item */
        XmListSelectPos(this->record_sep_list, 1, TRUE);
        this->setRecordSepSensitivity();
   }
}

extern "C" void GARMainWindow_StructureMapCB(Widget, XtPointer clientData, XtPointer)
{
    GARMainWindow 	*gmw = (GARMainWindow *) clientData;

    if(!XtIsSensitive(gmw->row_label))
	gmw->restrictStructure(False);
}
extern "C" void GARMainWindow_FieldPMCB(Widget w, XtPointer clientData, XtPointer)
{
    GARMainWindow 	*gmw = (GARMainWindow *) clientData;

    WidgetList wl;
    Cardinal num_children;
    XtVaGetValues(w, XmNchildren, &wl, XmNnumChildren, &num_children, NULL);
    ASSERT(num_children == 1);

    Widget wid;
    XtVaGetValues(wl[0], XmNmenuHistory, &wid, NULL);
    gmw->current_pb = wid;
}

extern "C" void GARMainWindow_recsepsingleOMCB(Widget w, 
                                   XtPointer clientData, XtPointer)
{
    GARMainWindow 	*gmw = (GARMainWindow *) clientData;
    RecordSeparator     *recsep;
    Widget              wid;

    recsep = 
       (RecordSeparator *)gmw->recordsepSingle.getElement(1);

    XtVaGetValues(gmw->record_sep_om, XmNmenuHistory, &wid, NULL);
    recsep->setType(XtName(wid));
}
extern "C" void GARMainWindow_recseplistOMCB(Widget w, 
                                   XtPointer clientData, XtPointer)
{
    GARMainWindow 	*gmw = (GARMainWindow *) clientData;
    int                 *position_list, position_count;
    RecordSeparator     *recsep;
    Widget              wid;

    if(!XmListGetSelectedPos(gmw->record_sep_list, &position_list, 
                             &position_count))
    {
       WarningMessage("You must select an item in the record separator list");
       return;
    }
    ASSERT(position_count == 1);
    recsep = 
       (RecordSeparator *)gmw->recordsepList.getElement(position_list[0]);
    XtVaGetValues(gmw->record_sep_om_1, XmNmenuHistory, &wid, NULL);
    recsep->setType(XtName(wid));
}

extern "C" void GARMainWindow_FieldOMCB(Widget w, XtPointer clientData, XtPointer)
{
    GARMainWindow 	*gmw = (GARMainWindow *) clientData;

    if(w == gmw->current_pb) return;
    gmw->setFieldDirty();
    theGARApplication->setDirty();
}
extern "C" void GARMainWindow_MajorityCB(Widget w, XtPointer clientData, XtPointer)
{
    GARMainWindow 	*gmw = (GARMainWindow *) clientData;

    if(EqualString(XtName(w), "row_tb"))
    {
	XmToggleButtonSetState(gmw->row_tb, True, False);
	XmToggleButtonSetState(gmw->col_tb, False, False);
	gmw->restrictStructure(False);
    }
    else
    {
	XmToggleButtonSetState(gmw->row_tb, False, False);
	XmToggleButtonSetState(gmw->col_tb, True, False);
	gmw->restrictStructure(True);
    }

    theGARApplication->setDirty();
}
void GARMainWindow::restrictStructure(Boolean rstrict)
{
    Widget w;
    WidgetList children;
    int nchildren;
    int i;
    Field *field;
    Boolean found_one = False;

    XtVaGetValues(this->structure_om, XmNsubMenuId, &w, NULL);
    XtVaGetValues(w, XmNchildren, &children, XmNnumChildren, &nchildren, NULL);
    ASSERT(nchildren == 10);

    for(i = 5; i < 10; i++)
	XtSetSensitive(children[i], !rstrict);

    if(rstrict)
    {
	XtVaGetValues(this->structure_om, XmNmenuHistory, &w, NULL);
	for(i = 5; i < 10; i++)
	{
	    if(w == children[i])
	    {
		XtVaSetValues(this->structure_om, 
			XmNmenuHistory, children[0], NULL);
		this->setFieldDirty();
		theGARApplication->setDirty();
		found_one = True;
		break;
	    }
	}
	//
	// Check all fields
	//
	for(i = 1; i <= this->fieldList.getSize(); i++)
	{
	    field = (Field *)this->fieldList.getElement(i);
	    if(EqualString(field->getStructure(), "scalar"))
		continue;
	    if(field->getStructure()[0] > '4')
	    {
		field->setStructure("scalar");
		found_one = True;
	    }
	}
	if(found_one)
	    WarningMessage("Columnar format has data structure limited to maximum dimension of 4.\nOne or more fields have been changed to \'scalar\' structure.");
    }

}


void GARMainWindow::createRecordSeparatorListStructure()
{
    int 	i, j, prev_counts, this_counts;
    Widget      current;
    char        name[100];
    int         totalnum=0; 
    int         numfields;
    int         extra_record_sep_counts = 0;
    Field       *field, *prevfield;
    RecordSeparator *recsep;


    numfields = this->fieldList.getSize();

    if (!this->getVectorInterleavingSensitivity())
    {
       extra_record_sep_counts = 0;
    }
    else 
    {
        XtVaGetValues(this->sens_vector_interleaving_om, XmNmenuHistory,
	    &current, NULL);
	if(EqualString(XtName(current), "w1"))
        {
           extra_record_sep_counts = 1;
        }
        else
        {
           extra_record_sep_counts = 0;
        }
    }
    if (!extra_record_sep_counts)
        totalnum = numfields - 1 ;
    else
    {
      totalnum = 0;
      for (i=1; i<=numfields; i++) 
      {
       field = (Field *)this->fieldList.getElement(i); /* YYY */
       char *structure = field->getStructure();
       if (structure[0] =='s')
          totalnum +=1;
       else
          totalnum += structure[0] - '0';
      }
      totalnum -= 1;
    }

    if (numfields == 1)
    {
      if (extra_record_sep_counts)
      {
         char *structure = field->getStructure();
         if (structure[0] =='s')
            this_counts = 1;
         else
            this_counts = structure[0] - '0';

         for (j=1; j<this_counts; j++) 
         {
            sprintf(name, "%s cmp %d and %d", field->getName(), 
                                            j, j+1);
            recsep = new RecordSeparator(name);
            recsep->setType("# of bytes");
            recsep->setNum(NULL); 
            this->recordsepList.appendElement(recsep); 
         }
      }
    }
    else
    {
    if (!extra_record_sep_counts)
    {
       for (i=2; i<=numfields; i++) 
       {
         prevfield = (Field *)this->fieldList.getElement(i-1); /* YYY */
         field = (Field *)this->fieldList.getElement(i); /* YYY */
         sprintf(name, "%s and %s", prevfield->getName(), 
                                            field->getName());
         recsep = new RecordSeparator(name);
         recsep->setType("# of bytes");
         recsep->setNum(NULL);
         this->recordsepList.appendElement(recsep); 
      }
    }
    /* else the interleaving is such that it matters if some are vectors */
    else
    {
       for (i=2; i<=numfields; i++) 
       {
         prevfield = (Field *)this->fieldList.getElement(i-1); /* YYY */
         field = (Field *)this->fieldList.getElement(i); /* YYY */
         char *structure = prevfield->getStructure();
         if (structure[0] =='s')
            prev_counts = 1;
         else
            prev_counts = structure[0] - '0';
         structure = field->getStructure();
         if (structure[0] =='s')
            this_counts = 1;
         else
            this_counts = structure[0] - '0';

         if ((i==2)&&(prev_counts>1))
         {
            for (j=1; j<prev_counts; j++)
            {
              sprintf(name, "%s cmp %d and %d", prevfield->getName(), 
                                            j, j+1);
              recsep = new RecordSeparator(name);
              recsep->setType("# of bytes");
              recsep->setNum(NULL);
              this->recordsepList.appendElement(recsep); 
            }
         }
         sprintf(name, "%s and %s", prevfield->getName(), 
                                                field->getName() );
         recsep = new RecordSeparator(name);
         recsep->setType("# of bytes");
         recsep->setNum(NULL);
         this->recordsepList.appendElement(recsep); 
         for (j=1; j<this_counts; j++) 
         {
            sprintf(name, "%s cmp %d and %d", field->getName(), 
                                            j, j+1);
            recsep = new RecordSeparator(name);
            recsep->setType("# of bytes");
            recsep->setNum(NULL);
            this->recordsepList.appendElement(recsep); 
          }
      }
    }
   }
}
void GARMainWindow::createRecordSeparatorList(Widget parent)
{
    int 	i, j, k, list_count, prev_counts, this_counts;
    XmString 	xms;
    Widget      current;
    char        name[100];
    RecordSeparator *recsep;
    int         totalnum=0; 
    int         numfields;
    int         extra_record_sep_counts = 0;
    Field       *field, *prevfield;
    Boolean     found;


    List *l = this->recordsepList.dup();

    this->recordsepList.clear();

    XtVaGetValues(this->field_list, XmNitemCount, &numfields, NULL);

    if (!this->getVectorInterleavingSensitivity())
    {
       extra_record_sep_counts = 0;
    }
    else 
    {
        XtVaGetValues(this->sens_vector_interleaving_om, XmNmenuHistory,
	    &current, NULL);
	if(EqualString(XtName(current), "w1"))
        {
           extra_record_sep_counts = 1;
        }
        else
        {
           extra_record_sep_counts = 0;
        }
    }
    if (!extra_record_sep_counts)
        totalnum = numfields - 1 ;
    else
    {
      totalnum = 0;
      for (i=1; i<=numfields; i++) 
      {
       field = (Field *)this->fieldList.getElement(i); /* YYY */
       char *structure = field->getStructure();
       if (structure[0] =='s')
          totalnum +=1;
       else
          totalnum += structure[0] - '0';
      }
      totalnum -= 1;
    }

    list_count = 1;
    if (numfields == 1)
    {
      if (extra_record_sep_counts)
      {
         char *structure = field->getStructure();
         if (structure[0] =='s')
            this_counts = 1;
         else
            this_counts = structure[0] - '0';

         for (j=1; j<this_counts; j++) 
         {
            found = FALSE;
            sprintf(name, "%s cmp %d and %d", field->getName(), 
                                            j, j+1);
            xms = XmStringCreateSimple(name);
            XmListAddItemUnselected(this->record_sep_list, xms, list_count);
            /* check to see if this entry was in the old list */
            for (k = 1; k <= l->getSize(); k++)
	    {
	       recsep = (RecordSeparator *)l->getElement(k);
               if (EqualString(recsep->getName(), name)) 
               {
                   this->recordsepList.appendElement(recsep); 
                   found = TRUE;
               }
            }
            if (!found)
            {
               recsep = new RecordSeparator(name);
               recsep->setType("# of bytes");
               recsep->setNum(NULL); 
               this->recordsepList.appendElement(recsep); 
            }
            XmStringFree(xms);
            list_count++;
         }
      }
    }
    else
    {
    if (!extra_record_sep_counts)
    {
       for (i=2; i<=numfields; i++) 
       {
         found = FALSE;
         prevfield = (Field *)this->fieldList.getElement(i-1); /* YYY */
         field = (Field *)this->fieldList.getElement(i); /* YYY */
         sprintf(name, "%s and %s", prevfield->getName(), 
                                            field->getName());
         xms = XmStringCreateSimple(name);
         XmListAddItemUnselected(this->record_sep_list, xms, list_count);
         for (k = 1; k <= l->getSize(); k++)
	 {
	    recsep = (RecordSeparator *)l->getElement(k);
            if (EqualString(recsep->getName(), name)) 
            {
                this->recordsepList.appendElement(recsep); 
                found = TRUE;
            }
         }
         XmStringFree(xms);
         if (!found)
         {
            recsep = new RecordSeparator(name);
            recsep->setType("# of bytes");
            recsep->setNum(NULL);
            this->recordsepList.appendElement(recsep); 
         }
         list_count++;
      }
    }
    /* else the interleaving is such that it matters if some are vectors */
    else
    {
       for (i=2; i<=numfields; i++) 
       {
         prevfield = (Field *)this->fieldList.getElement(i-1); /* YYY */
         field = (Field *)this->fieldList.getElement(i); /* YYY */
         char *structure = prevfield->getStructure();
         if (structure[0] =='s')
            prev_counts = 1;
         else
            prev_counts = structure[0] - '0';
         structure = field->getStructure();
         if (structure[0] =='s')
            this_counts = 1;
         else
            this_counts = structure[0] - '0';

         if ((i==2)&&(prev_counts>1))
         {
            for (j=1; j<prev_counts; j++)
            {
            found = FALSE;
            sprintf(name, "%s cmp %d and %d", prevfield->getName(), 
                                            j, j+1);
            xms = XmStringCreateSimple(name);
            XmListAddItemUnselected(this->record_sep_list, xms, list_count);
            for (k = 1; k <= l->getSize(); k++)
	    {
	       recsep = (RecordSeparator *)l->getElement(k);
               if (EqualString(recsep->getName(), name)) 
               {
                   this->recordsepList.appendElement(recsep); 
                   found = TRUE;
               }
            }
            XmStringFree(xms);
            if (!found)
            {
               recsep = new RecordSeparator(name);
               recsep->setType("# of bytes");
               recsep->setNum(NULL);
               this->recordsepList.appendElement(recsep); 
            }
            list_count++;
            }
         }
         sprintf(name, "%s and %s", prevfield->getName(), 
                                                field->getName() );
         found = FALSE;
         xms = XmStringCreateSimple(name);
         XmListAddItemUnselected(this->record_sep_list, xms, list_count);
         for (k = 1; k <= l->getSize(); k++)
	 {
	    recsep = (RecordSeparator *)l->getElement(k);
            if (EqualString(recsep->getName(), name)) 
            {
                this->recordsepList.appendElement(recsep); 
                found = TRUE;
            }
         }
         XmStringFree(xms);
         if (!found)
         {
            recsep = new RecordSeparator(name);
            recsep->setType("# of bytes");
            recsep->setNum(NULL);
            this->recordsepList.appendElement(recsep); 
         }
         list_count++;
         for (j=1; j<this_counts; j++) 
         {
            sprintf(name, "%s cmp %d and %d", field->getName(), 
                                            j, j+1);
            found = FALSE;
            xms = XmStringCreateSimple(name);
            XmListAddItemUnselected(this->record_sep_list, xms, list_count);
            for (k = 1; k <= l->getSize(); k++)
	    {
	       recsep = (RecordSeparator *)l->getElement(k);
               if (EqualString(recsep->getName(), name)) 
               {
                   this->recordsepList.appendElement(recsep); 
                   found = TRUE;
               }
            }
            XmStringFree(xms);
            if (!found)
            {
               recsep = new RecordSeparator(name);
               recsep->setType("# of bytes");
               recsep->setNum(NULL);
               this->recordsepList.appendElement(recsep); 
            }
            list_count++;
      }
    }
   }
   }
   l->clear();
}
Widget GARMainWindow::createHeaderPulldown(Widget parent, 
                                           boolean setup_fieldCB,
                                           boolean recseplistCB,
                                           boolean recsepsingleCB)
{
    Widget 	pdm = XmCreatePulldownMenu(parent, "pdm", NULL, 0);
    Widget 	w[3];
    int 	i;
    XmString 	xms;
    char   	*name[3];
    name[0] = 	"# of bytes";
    name[1] = 	"# of lines";
    name[2] = 	"string marker";

    xms = XmStringCreateSimple(name[0]);
    w[0] = XtVaCreateManagedWidget(name[0], 
			    xmPushButtonWidgetClass, pdm,
			    XmNlabelString, xms,
			    NULL);
    XmStringFree(xms);

    xms = XmStringCreateSimple(name[1]);
    w[1] = XtVaCreateManagedWidget(name[1], 
			    xmPushButtonWidgetClass, pdm,
			    XmNlabelString, xms,
			    NULL);
    XmStringFree(xms);

    xms = XmStringCreateSimple(name[2]);
    w[2] = XtVaCreateManagedWidget(name[2],
			    xmPushButtonWidgetClass, pdm,
			    XmNlabelString, xms,
			    NULL);
    XmStringFree(xms);

    for(i = 0; i < 3; i++)
	XtAddCallback(w[i],
	      XmNactivateCallback,
	      (XtCallbackProc)GARMainWindow_HeaderTypeCB,
	      (XtPointer)this); 


    /* if it's the record sep option menu associated with the list */
    /* of record separators, we have to update the data structure  */
    /* recordsepList.                                              */

    if (recseplistCB)
    {
	for(i = 0; i < 3; i++)
	    XtAddCallback(w[i],
		  XmNactivateCallback,
		  (XtCallbackProc)GARMainWindow_recseplistOMCB,
		  (XtPointer)this);

    }

    /* if it's the record sep option menu associated with the single */
    /* record separators, we have to update the data structure  */
    /* recordsepSingle.                                              */

    if (recsepsingleCB)
    {
	for(i = 0; i < 3; i++)
	    XtAddCallback(w[i],
		  XmNactivateCallback,
		  (XtCallbackProc)GARMainWindow_recsepsingleOMCB,
		  (XtPointer)this);

    }


    if(setup_fieldCB)
    {
	for(i = 0; i < 3; i++)
	    XtAddCallback(w[i],
		  XmNactivateCallback,
		  (XtCallbackProc)GARMainWindow_FieldOMCB,
		  (XtPointer)this);

	XtAddCallback(XtParent(pdm),
		    XmNpopupCallback,
		    (XtCallbackProc)GARMainWindow_FieldPMCB,
		    (XtPointer)this);
    }

    return pdm;
}
extern "C" void GARMainWindow_GridCB(Widget w, XtPointer clientData, XtPointer)
{
    Widget		wid;
    GARMainWindow 	*gmw = (GARMainWindow *) clientData;
    Boolean		set;

    XtVaGetValues(w, XmNset, &set, NULL);

    XmString xms;
    if(!set) 
    {
	gmw->dimension = 1;
        xms = XmStringCreateLtoR("Point\npositions", XmSTRING_DEFAULT_CHARSET);
    }
    else
    {
	gmw->dimension = gmw->getDimension();
        xms = XmStringCreateLtoR("Grid\npositions", XmSTRING_DEFAULT_CHARSET);
    }
    //
    // Change the regularity label
    //
    XtVaSetValues(gmw->regularity_label, XmNlabelString, xms, NULL);
    XmStringFree(xms);

    gmw->setGridSensitivity(set);
    gmw->setPointsSensitivity(!set);
    gmw->setMajoritySensitivity(set);

    XtVaGetValues(gmw->regularity_om, XmNmenuHistory, &wid, NULL);
    gmw->setPositionsSensitivity();

    theGARApplication->setDirty();
}
extern "C" void GARMainWindow_GridDimCB(Widget, XtPointer clientData, XtPointer)
{
    Widget wid;
    GARMainWindow 	*gmw = (GARMainWindow *) clientData;

    gmw->dimension = gmw->getDimension();

    XtVaGetValues(gmw->regularity_om, XmNmenuHistory, &wid, NULL);
    gmw->changeRegularity(wid); 
    gmw->setPositionsSensitivity();

    theGARApplication->setDirty();
}
extern "C" void GARMainWindow_HeaderCB(Widget w, XtPointer clientData, XtPointer)
{
    GARMainWindow 	*gmw = (GARMainWindow *) clientData;
    Boolean		set;

    XtVaGetValues(w, XmNset, &set, NULL);
    gmw->setHeaderSensitivity(set);

    theGARApplication->setDirty();
}
extern "C" void GARMainWindow_SeriesCB(Widget w, XtPointer clientData, XtPointer)
{
    GARMainWindow 	*gmw = (GARMainWindow *) clientData;
    Boolean		set;

    XtVaGetValues(w, XmNset, &set, NULL);
    gmw->setSeriesSensitivity(set);
    gmw->setSeriesInterleavingSensitivity();
    gmw->setSeriesSepSensitivity();

    theGARApplication->setDirty();
}
extern "C" void GARMainWindow_SeriesSepCB(Widget, XtPointer clientData, XtPointer)
{
    GARMainWindow 	*gmw = (GARMainWindow *) clientData;

    gmw->setSeriesSepSensitivity();

    theGARApplication->setDirty();
}
Widget GARMainWindow::createRegularityPulldown(Widget parent)
{
    Widget pb;
    Widget pdm = XmCreatePulldownMenu(parent, "pdm", NULL, 0);
    XmString xms;
    char     *name[3];
    name[0] = "Completely regular";
    name[1] = "Partially regular";
    name[2] = "Explicit position list";

    xms = XmStringCreateSimple(name[0]);
    pb = XtVaCreateManagedWidget(name[0],    
		xmPushButtonWidgetClass, pdm, 
		XmNlabelString, xms,
		NULL);
    XtAddCallback(pb, 
		  XmNactivateCallback, 
		  (XtCallbackProc)GARMainWindow_RegularityCB, 
		  (XtPointer)this); 
    XmStringFree(xms);


    xms = XmStringCreateSimple(name[1]);
    pb = XtVaCreateManagedWidget(name[1], 
		xmPushButtonWidgetClass, pdm, 
		XmNlabelString, xms,
		NULL);
    XtAddCallback(pb, 
		  XmNactivateCallback, 
		  (XtCallbackProc)GARMainWindow_RegularityCB, 
		  (XtPointer)this); 
    XmStringFree(xms);



    xms = XmStringCreateSimple(name[2]);
    pb = XtVaCreateManagedWidget(name[2], 
		xmPushButtonWidgetClass, pdm, 
		XmNlabelString, xms,
		NULL);
    XtAddCallback(pb, 
		  XmNactivateCallback, 
		  (XtCallbackProc)GARMainWindow_RegularityCB, 
		  (XtPointer)this); 
    XmStringFree(xms);

    return pdm;
}

extern "C" void GARMainWindow_RegularityCB(Widget w, XtPointer clientData, XtPointer)
{
    GARMainWindow 	*gmw = (GARMainWindow *) clientData;

    gmw->changeRegularity(w);

    theGARApplication->setDirty();
}
void GARMainWindow::changeRegularity(Widget w)
{
    int 		i;

    if(!w) return;

    if(EqualString("Completely regular", XtName(w)))
    {
	XmString xms = XmStringCreateSimple("origin, delta");
	for(i = 0; i < 4; i++)
	{
	    XtSetSensitive(this->positions_om[i], False);
	    XtVaSetValues(this->position_label[i], XmNlabelString, xms, NULL);
	    XtSetSensitive(this->position_label[i], (i < this->dimension));
	    this->setTextSensitivity(this->position_text[i], 
		(i < this->dimension));

	    SET_OM_NAME(this->positions_om[i], "regular", "Positions");
            this->changePositions(this->positions_om[i]);
	}
	XmStringFree(xms);

    }
    else if(EqualString("Partially regular", XtName(w)))
    {
	for(i = 0; i < 4; i++)
	{
	    XtSetSensitive(this->positions_om[i], 
		(i < this->dimension));
	    XtSetSensitive(this->position_label[i], 
		(i < this->dimension));
	    this->setTextSensitivity(this->position_text[i], 
		(i < this->dimension));
	}
    }
    else if(EqualString("Explicit position list", XtName(w)))
    {
	SET_OM_NAME(this->positions_om[0], "irregular", "Positions"); 
        this->changePositions(this->positions_om[0]); 

        XtSetSensitive(this->positions_om[0], 0);
        XtSetSensitive(this->position_label[0], 1);
        this->setTextSensitivity(this->position_text[0], 1);
        for(i=1; i < 4; i++)
        {
	    XtSetSensitive(this->positions_om[i], 0);
	    XtSetSensitive(this->position_label[i], 0);
	    this->setTextSensitivity(this->position_text[i], 0);
        }
    }
    else
	ASSERT(False);
}

Widget GARMainWindow::createFieldInterleavingPulldown(Widget parent)
{
    int	   i;
    Widget w[2];
    Widget pdm = XmCreatePulldownMenu(parent, "pdm", NULL, 0);
    XmString xms;
    char *name[2];
    name[0] = "Block";
    name[1] = "Columnar";

    xms = XmStringCreateSimple(name[0]);
    w[0] = XtVaCreateManagedWidget(name[0], 
			xmPushButtonWidgetClass, pdm, 
			XmNlabelString, xms,
			NULL);
    XmStringFree(xms);

    xms = XmStringCreateSimple(name[1]);
    w[1] = XtVaCreateManagedWidget(name[1], 
			xmPushButtonWidgetClass, pdm, 
			XmNlabelString, xms,
			NULL);
    XmStringFree(xms);

    for(i = 0; i < 2; i++)
	XtAddCallback(w[i],
	      XmNactivateCallback,
	      (XtCallbackProc)GARMainWindow_FieldInterleavingCB,
	      (XtPointer)this);
    return pdm;
}
Widget GARMainWindow::createInsensVectorInterleavingPulldown(Widget parent)
{
    Pixel bg, fg;
    Widget pdm = XmCreatePulldownMenu(parent, "pdm", NULL, 0);
    Display *d = XtDisplay(pdm);
    Window win = XtWindow(pdm); 
    Pixmap vector_pix1_insens;
    Pixmap vector_pix2_insens;

    XtVaGetValues(pdm, XmNforeground, &fg, XmNbackground, &bg, NULL);

    vector_pix1_insens = XCreatePixmapFromBitmapData(
			d,
			win,
			(char *)vector1_insens_bits, 
			vector1_insens_width, 
			vector1_insens_height,
			fg, 
			bg, 
			DefaultDepthOfScreen(XtScreen(parent)));

    vector_pix2_insens = XCreatePixmapFromBitmapData(
			d,
			win,
			(char *)vector2_insens_bits, 
			vector2_insens_width, 
			vector2_insens_height, 
			fg, 
			bg, 
			DefaultDepthOfScreen(XtScreen(parent)));

    XtVaCreateManagedWidget("w0", xmPushButtonWidgetClass, pdm, 
		XmNlabelType, XmPIXMAP,
		XmNlabelPixmap, vector_pix1_insens,
		NULL);

    XtVaCreateManagedWidget("w1",xmPushButtonWidgetClass, pdm, 
		XmNlabelType, XmPIXMAP,
		XmNlabelPixmap, vector_pix2_insens,
		NULL);

    return pdm;
}
Widget GARMainWindow::createSensVectorInterleavingPulldown(Widget parent)
{
    int	   i;
    Widget w[2];
    Pixel bg, fg;
    Widget pdm = XmCreatePulldownMenu(parent, "pdm", NULL, 0);
    Display *d = XtDisplay(pdm);
    Window win = XtWindow(pdm); 
    Pixmap vector_pix1;
    Pixmap vector_pix2;

    XtVaGetValues(pdm, XmNforeground, &fg, XmNbackground, &bg, NULL);

    vector_pix1 = XCreatePixmapFromBitmapData(
			d,
			win,
			(char *)vector1_bits, 
			vector1_width, 
			vector1_height, 
			fg, 
			bg, 
			DefaultDepthOfScreen(XtScreen(parent)));

    vector_pix2 = XCreatePixmapFromBitmapData(
			d,
			win,
			(char *)vector2_bits, 
			vector2_width, 
			vector2_height, 
			fg, 
			bg, 
			DefaultDepthOfScreen(XtScreen(parent)));

    w[0] = XtVaCreateManagedWidget("w0", xmPushButtonWidgetClass, pdm, 
		XmNlabelType, XmPIXMAP,
		XmNlabelPixmap, vector_pix1,
		NULL);

    w[1] = XtVaCreateManagedWidget("w1",xmPushButtonWidgetClass, pdm, 
		XmNlabelType, XmPIXMAP,
		XmNlabelPixmap, vector_pix2,
		NULL);

    for(i = 0; i < 2; i++)
	XtAddCallback(w[i],
	      XmNactivateCallback,
	      (XtCallbackProc)GARMainWindow_VectorInterleavingCB,
	      (XtPointer)this);

    return pdm;
}
Widget GARMainWindow::createSeriesInterleavingPulldown(Widget parent)
{
    int	   i;
    Widget w[4];
    Pixel bg, fg;
    Widget pdm = XmCreatePulldownMenu(parent, "pdm", NULL, 0);
    Display *d = XtDisplay(pdm);
    Window win = XtWindow(pdm); 
    Pixmap series_pix1;
    Pixmap series_pix2;
    Pixmap series_pix1_insens;
    Pixmap series_pix2_insens;

    XtVaGetValues(pdm, XmNforeground, &fg, XmNbackground, &bg, NULL);

    series_pix1 = XCreatePixmapFromBitmapData(
			d,
			win,
			(char *)series1_bits, 
			series1_width, 
			series1_height, 
			fg, 
			bg, 
			DefaultDepthOfScreen(XtScreen(parent)));
    series_pix1_insens = XCreatePixmapFromBitmapData(
			d,
			win,
			(char *)series1_insens_bits, 
			series1_insens_width, 
			series1_insens_height,
			fg, 
			bg, 
			DefaultDepthOfScreen(XtScreen(parent)));
    series_pix2 = XCreatePixmapFromBitmapData(
			d,
			win,
			(char *)series2_bits, 
			series2_width, 
			series2_height, 
			fg, 
			bg, 
			DefaultDepthOfScreen(XtScreen(parent)));
    series_pix2_insens = XCreatePixmapFromBitmapData(
			d,
			win,
			(char *)series2_insens_bits, 
			series2_insens_width, 
			series2_insens_height, 
			fg, 
			bg, 
			DefaultDepthOfScreen(XtScreen(parent)));
    w[0] = XtVaCreateWidget("w0", xmPushButtonWidgetClass, pdm,
		XmNlabelType, XmPIXMAP,
		XmNlabelPixmap, series_pix1,
		NULL);
    w[1] = XtVaCreateWidget("w1",xmPushButtonWidgetClass, pdm,
		XmNlabelType, XmPIXMAP,
		XmNlabelPixmap, series_pix2,
		NULL);
    w[2] = XtVaCreateManagedWidget("w0_insens", xmPushButtonWidgetClass, pdm,
		XmNlabelType, XmPIXMAP,
		XmNlabelPixmap, series_pix1_insens,
		NULL);
    w[3] = XtVaCreateManagedWidget("w1_insens",xmPushButtonWidgetClass, pdm,
		XmNlabelType, XmPIXMAP,
		XmNlabelPixmap, series_pix2_insens,
		NULL);

    for(i = 0; i < 2; i++)
	XtAddCallback(w[i],
	      XmNactivateCallback,
	      (XtCallbackProc)GARMainWindow_SeriesInterleavingCB,
	      (XtPointer)this);
    return pdm;
}
extern "C" void GARMainWindow_FieldInterleavingCB(Widget, 
					XtPointer clientData, 
					XtPointer)
{
    GARMainWindow 	*gmw = (GARMainWindow *) clientData;

    gmw->setBlockLayoutSensitivity();
    gmw->setRecordSepTBSensitivity();
    gmw->setVectorInterleavingSensitivity();
    gmw->setSeriesInterleavingSensitivity();
    gmw->setDependencyRestrictions();

    theGARApplication->setDirty();
}
extern "C" void GARMainWindow_VectorInterleavingCB(Widget, 
					XtPointer clientData, 
					XtPointer)
{
    Widget		wid;
    Widget		current;
    Boolean		sens;
    WidgetList		children;
    int			num_children;
    int			i;
    GARMainWindow 	*gmw = (GARMainWindow *) clientData;

    //
    // We only get callbacks from the sensitive one
    //
    XtVaGetValues(gmw->sens_vector_interleaving_om, XmNmenuHistory, &wid, NULL);
    if(EqualString(XtName(wid), "w1"))
    {
	//
	// Set the Series Interleaving to "w0" since we don't support w1
	//
	XtVaGetValues(gmw->series_interleaving_om, XmNsubMenuId, &wid, NULL);
	XtVaGetValues(wid, XmNchildren, &children,
		XmNnumChildren, &num_children, NULL);

	//
	// Find out if we are currently sensitive...
	//
	XtVaGetValues(gmw->series_interleaving_om, 
		XmNmenuHistory, &current, NULL);
	if( EqualString(XtName(current), "w0") ||
	    EqualString(XtName(current), "w1") )
	    sens = True;
	else
	    sens = False;

	for(i = 0; i < num_children; i++)
	{
	    if(EqualString(XtName(children[i]), sens ? "w0" : "w0_insens"))
	    {
		XtVaSetValues(gmw->series_interleaving_om, XmNmenuHistory, 
			children[i], NULL);
		break;
	    }
	}
    }
    gmw->setSeriesInterleavingSensitivity();
    gmw->setRecordSepTBSensitivity();
    theGARApplication->setDirty();
    gmw->updateRecordSeparatorList();
}
extern "C" void GARMainWindow_SeriesInterleavingCB(Widget, 
					XtPointer clientData, 
					XtPointer)
{
    Widget		wid;
    GARMainWindow 	*gmw = (GARMainWindow *) clientData;

    XtVaGetValues(gmw->series_interleaving_om, XmNmenuHistory, &wid, NULL);
    if(EqualString(XtName(wid), "w1"))
    {
	gmw->setVectorInterleavingOM("w0");
    }

    gmw->setSeriesInterleavingSensitivity();
    gmw->setRecordSepTBSensitivity();

    theGARApplication->setDirty();
}
Widget GARMainWindow::createTypePulldown(Widget parent)
{
    int	   i;
    Widget w;
    char   *name[12];
    name[0] = "float";
    name[1] = "byte";
    name[2] = "unsigned byte";
    name[3] = "signed byte";
    name[4] = "short";
    name[5] = "unsigned short";
    name[6] = "signed short";
    name[7] = "int";
    name[8] = "unsigned int";
    name[9] = "signed int";
    name[10] = "double";
    name[11] = "string";

    Widget pdm = XmCreatePulldownMenu(parent, "pdm", NULL, 0);

    for(i = 0; i < XtNumber(name); i++)
    {
	XmString xms = XmStringCreateSimple(name[i]);
	w = XtVaCreateManagedWidget(name[i],
		xmPushButtonWidgetClass,pdm,
		XmNlabelString, xms,
		NULL);
	XmStringFree(xms);

	XtAddCallback(w,
	      XmNactivateCallback,
	      (XtCallbackProc)GARMainWindow_FieldOMCB,
	      (XtPointer)this);

	//
	// Enable/Disable structure menu/size stepper
	//
	if (!strcmp(name[i], "string")) {
	    XtAddCallback (w, XmNactivateCallback,
		(XtCallbackProc)GARMainWindow_StructureInsensCB, (XtPointer)this);
	} else {
	    XtAddCallback (w, XmNactivateCallback,
		(XtCallbackProc)GARMainWindow_StructureSensCB, (XtPointer)this);
	}
    }

    XtAddCallback(XtParent(pdm),
		XmNpopupCallback,
		(XtCallbackProc)GARMainWindow_FieldPMCB,
		(XtPointer)this);
    return pdm;
}
Widget GARMainWindow::createStructurePulldown(Widget parent)
{

    int	   i;
    Widget w;
    XmString xms;
    char   *name[10];

    name[0] = "scalar";  name[1] = "1-vector";name[2] = "2-vector";
    name[3] = "3-vector";name[4] = "4-vector";name[5] = "5-vector";
    name[6] = "6-vector";name[7] = "7-vector";name[8] = "8-vector";
    name[9] = "9-vector"; 

    Widget pdm = XmCreatePulldownMenu(parent, "pdm", NULL, 0);

    for(i = 0; i < 10; i++)
    {
	xms = XmStringCreateSimple(name[i]);
	w = XtVaCreateManagedWidget(name[i],
		xmPushButtonWidgetClass,	pdm,
		XmNlabelString,			xms,
		NULL);
        XmStringFree(xms);

	XtAddCallback(w,
	      XmNactivateCallback,
	      (XtCallbackProc)GARMainWindow_FieldOMCB,
	      (XtPointer)this);
    }

    XtAddCallback(XtParent(pdm),
		XmNpopupCallback,
		(XtCallbackProc)GARMainWindow_FieldPMCB,
		(XtPointer)this);

    XtAddCallback(XtParent(pdm),
		XmNpopupCallback,
		(XtCallbackProc)GARMainWindow_StructureMapCB,
		(XtPointer)this);

    return pdm;
}
Widget GARMainWindow::createDependencyPulldown(Widget parent)
{
    int    i;
    Widget w;
    Widget pdm = XmCreatePulldownMenu(parent, "pdm", NULL, 0);
    XmString xms;
    char   *name[2];
    name[0] = "positions"; name[1] = "connections";

    for(i = 0; i < 2; i++)
    {
	xms = XmStringCreateSimple(name[i]);
	w = XtVaCreateManagedWidget(name[i],
		xmPushButtonWidgetClass,pdm,
		XmNlabelString, xms,
		NULL);
        XmStringFree(xms);

	XtAddCallback(w,
	      XmNactivateCallback,
	      (XtCallbackProc)GARMainWindow_FieldOMCB,
	      (XtPointer)this);
    }

    XtAddCallback(XtParent(pdm),
		XmNpopupCallback,
		(XtCallbackProc)GARMainWindow_FieldPMCB,
		(XtPointer)this);

    return pdm;
}
Widget GARMainWindow::createPositionsPulldown(Widget parent)
{
    Widget pdm = XmCreatePulldownMenu(parent, "pdm",  
                                      NULL, 0);
    XmString xms;


    char *name[2];
    name[0] = "regular";
    name[1] = "irregular";
    Widget w;
    int i;
    for(i = 0; i < 2; i++)
    {
        xms = XmStringCreateSimple(name[i]);
        w = XtVaCreateManagedWidget(name[i],
                    xmPushButtonWidgetClass, pdm,
                    XmNlabelString, xms,
                    NULL);
        XtAddCallback(w, XmNactivateCallback, 
		      (XtCallbackProc)GARMainWindow_PositionsCB, 
		      (XtPointer)this);
        XmStringFree(xms);
    }

    return pdm;
}

void GARMainWindow::changePositions(Widget w)
{
   Widget               button;
   int                  i;
   XmString		xms;

   for (i=0; i<4; i++) 
   {

        /*YYY XtVaGetValues(w, XmNmenuHistory, &button, NULL); */
        XtVaGetValues(this->positions_om[i], XmNmenuHistory, &button, NULL); 
    
        if (w == this->positions_om[i]) {

         if(EqualString("regular", XtName(button))) 
         { 
             xms = XmStringCreateSimple("origin, delta"); 
         } 
         else if(EqualString("irregular", XtName(button)))
         {
             xms = XmStringCreateSimple("position list");
         }
         else
             ASSERT(False);
         XtVaSetValues(this->position_label[i], XmNlabelString, xms, NULL);
         XmStringFree(xms);
      }
    }
}
extern "C" void GARMainWindow_PositionsCB(Widget w, XtPointer clientData, XtPointer)
{
    GARMainWindow 	*gmw = (GARMainWindow *) clientData;
    int 		i;
    Widget 		button;
    XmString		xms;


    for(i = 0; i < 4; i++)
    {
	XtVaGetValues(gmw->positions_om[i], XmNmenuHistory, &button, NULL);
	if(w == button) {
	    if(EqualString("regular", XtName(w)))
	    {
		 xms = XmStringCreateSimple("origin, delta");
	    }
	    else if(EqualString("irregular", XtName(w)))
	    {
		 xms = XmStringCreateSimple("position list");
	    }
	    else
		ASSERT(False);
	    XtVaSetValues(gmw->position_label[i], XmNlabelString, xms, NULL);
            XmStringFree(xms);
	    break;
	}
    }

    theGARApplication->setDirty();
}
Widget GARMainWindow::createFormat1Pulldown(Widget parent)
{
    Widget pdm = XmCreatePulldownMenu(parent, "pdm", NULL, 0);
    XmString xms;
    char *name[2];
    name[0] = "Most Significant Byte First";
    name[1] = "Least Significant Byte First";

    xms = XmStringCreateSimple(name[0]);
    XtVaCreateManagedWidget(name[0], 
		xmPushButtonWidgetClass, pdm, 
		XmNlabelString, xms,
		NULL);
    XmStringFree(xms);

    xms = XmStringCreateSimple(name[1]);
    XtVaCreateManagedWidget(name[1], 
		xmPushButtonWidgetClass, pdm, 
		XmNlabelString, xms,
		NULL);
    XmStringFree(xms);

    return pdm;
}

Widget GARMainWindow::createFormat2Pulldown(Widget parent)
{
    Widget w[4];
    Widget pdm = XmCreatePulldownMenu(parent, "pdm", NULL, 0);
    int i;
    XmString xms;
    char *name[4];
    name[0] = "ASCII (Text)";
    name[1] = "Binary (IEEE)";

    for(i = 0; i < 2; i++)
    {
	xms = XmStringCreateSimple(name[i]);
	w[i] = XtVaCreateManagedWidget(name[i],  
		    xmPushButtonWidgetClass, pdm, 
		    XmNlabelString, xms,
		    NULL);
	XmStringFree(xms);
    }

    for(i = 0; i < 2; i++)
	XtAddCallback(w[i],
	      XmNactivateCallback,
	      (XtCallbackProc)GARMainWindow_Format2CB,
	      (XtPointer)this);

    return pdm;
}
extern "C" void GARMainWindow_Format2CB(Widget w, XtPointer clientData, XtPointer)
{
    GARMainWindow 	*gmw = (GARMainWindow *) clientData;

    gmw->setBlockLayoutSensitivity();

    XtSetSensitive(gmw->format1_om, EqualString(XtName(w), "Binary (IEEE)"));

    theGARApplication->setDirty();
}
void GARMainWindow::setPositionsSensitivity()
{
    int i;
    Field *field;
    Widget w;

    //
    // Make sure the field name != "locations"
    //
    for(i = 1; i <= this->fieldList.getSize(); i++)
    {
	field = (Field *)this->fieldList.getElement(i);
	if(EqualString(field->getName(), "locations"))
	{
	    XtSetSensitive(this->regularity_om, False);
	    XtSetSensitive(this->regularity_label, False);
	    for(i = 0; i < 4; i++)
	    {
		XtSetSensitive(this->positions_om[i], False);
		XtSetSensitive(this->position_label[i], False);
		this->setTextSensitivity(this->position_text[i], False);
	    }
	    return;
	}
    }
    XtSetSensitive(this->regularity_om, True);
    XtSetSensitive(this->regularity_label, True);

    XtVaGetValues(this->regularity_om, XmNmenuHistory, &w, NULL);
    this->changeRegularity(w);

}
void GARMainWindow::setBlockLayoutSensitivity()
{
    this->setBlockTBSensitivity();
    this->setLayoutTBSensitivity();
}
    
void GARMainWindow::setDependencyRestrictions()
{
int	i;
Field	*field;
Boolean	loc = False;
Widget	w;

    //
    // See if there is a locations field
    //
    for(i = 1; i <= this->fieldList.getSize(); i++)
    {
	field = (Field *)this->fieldList.getElement(i);
	if(EqualString(field->getName(), "locations"))
	{
	    loc = True;
	    break;
	}
    }
    XtVaGetValues(this->field_interleaving_om, XmNmenuHistory, &w, NULL);

    if(loc)
    {
	if(EqualString(XtName(w), "Columnar"))
	    this->dependency_restriction = RESTRICT_POSITIONS;
	else
	    this->dependency_restriction = RESTRICT_NONE;
    }
    else
    {
	if(EqualString(XtName(w), "Columnar"))
	    this->dependency_restriction = RESTRICT_SAME;
	else
	    this->dependency_restriction = RESTRICT_NONE;
    }
    this->synchDependencies();
    this->setDependencySensitivity(this->dependency_restriction != 
	RESTRICT_POSITIONS);
}

void GARMainWindow::synchDependencies()
{
Widget	wid;
int	i;
Field	*field;

    if(this->dependency_restriction == RESTRICT_NONE)
	return;
    else if(this->dependency_restriction == RESTRICT_POSITIONS)
    {
	//
	// If we are in the RESTRICT_POSITIONS state, then the dependency_om
	// is insens, and we don't generate a a dependency statement.  So
	// don't bother to change the field values.  This way, if the state
	// reverts to something else, the old values are still available.
	;
    }
    else if(this->dependency_restriction == RESTRICT_SAME)
    {
	XtVaGetValues(this->dependency_om, XmNmenuHistory, &wid, NULL);
	for(i = 1; i <= this->fieldList.getSize(); i++)
	{
	    field = (Field *)this->fieldList.getElement(i);
	    field->setDependency(XtName(wid));
	}
    }
}

extern "C" void GARMainWindow_AddFieldCB(Widget, XtPointer clientData, XtPointer)
{
    GARMainWindow 	*gmw = (GARMainWindow *) clientData;
    int			*position_list;
    int			position_count;
    int			pos;

    //
    // Add ==> Add at end if nothing is seleced
    //         Add after a selected item.
    //
    if(!XmListGetSelectedPos(gmw->field_list, &position_list, &position_count))
    {
	pos = 0;
    }
    else
    {
	pos = position_list[0] + 1;
	XtFree((char *)position_list); 
    }

    if(gmw->addField(pos))
    {
	gmw->setPositionsSensitivity();
	gmw->setMoveFieldSensitivity();

	gmw->setFieldClean();
	theGARApplication->setDirty();
    }
    gmw->updateRecordSeparatorList();
}
extern "C" void GARMainWindow_InsertFieldCB(Widget, XtPointer clientData, XtPointer)
{
    GARMainWindow 	*gmw = (GARMainWindow *) clientData;
    int			*position_list;
    int			position_count;
    int			pos;

    //
    // Insert ==> Insert at top if nothing is seleced
    //            Insert before a selected item.
    //
    if(!XmListGetSelectedPos(gmw->field_list, &position_list, &position_count))
    {
	pos = 1;
    }
    else
    {
	pos = position_list[0];
	XtFree((char *)position_list); 
    }

    if(gmw->addField(pos))
    {
	gmw->setPositionsSensitivity();
	gmw->setMoveFieldSensitivity();

	gmw->setFieldClean();
	theGARApplication->setDirty();
    }
    gmw->updateRecordSeparatorList();
}

char * GARMainWindow::getFilename(void) {
    // Check syntax of filename and change to Unix seps.
    if(!this->filename)
        return NULL;
    char *fn = DuplicateString(this->filename);
    for(int i=0; i<strlen(fn); i++) 
        if(fn[i] == '\\') fn[i] = '/';
    return fn;
}

Boolean GARMainWindow::addField(char *name, char *structure)
{
    XmTextSetString(this->field_text, name);
    //
    // There is one structure name which requires sepcial handling...
    // structure = string[n]
    // We only want the n, and it goes into a text widget not the option menu
    // This also implies that type_om should be holding "string"
    //
    char *cp;
    boolean string_size = FALSE;
    if ((cp = strstr(structure, "string["))) {
	if (strlen(cp) >= 2) {
            char tmpbuf[32];
            cp = strstr(structure, "["); cp++;
            strcpy (tmpbuf, cp);
            cp = strstr(tmpbuf, "]"); *cp = '\0';
	    XmTextSetString (this->string_size, tmpbuf);
	    string_size = TRUE;
	    XtSetSensitive (this->string_size, True);
	    XtSetSensitive (this->structure_om, False);
	}
    }

    if (!string_size) {
	SET_OM_NAME(this->structure_om, structure, "Structure");
	XtSetSensitive (this->string_size, False);
	XtSetSensitive (this->structure_om, True);
    }
    if(this->addField(this->fieldList.getSize()+1))
    {
	this->setPositionsSensitivity();
	this->setMoveFieldSensitivity();

	this->setFieldClean();
	theGARApplication->setDirty();
	return True;
    }
    return False;
}
Boolean GARMainWindow::addField(int pos)
{
    int i;
    Field *field;

    char *str = XmTextGetString(this->field_text);
    //
    // If they did not enter a name, find a reasonable one for them.
    //
    if(str[0] == '\0')
    {
	XtFree(str);

	char newname[256];
	boolean unique = FALSE;

	int n = this->fieldList.getSize();
	while(!unique)
	{
	    unique = TRUE;
	    sprintf(newname, "field%d", n);
	    for(i = 1; i <= this->fieldList.getSize(); i++)
	    {
		field = (Field *)this->fieldList.getElement(i);
		if(EqualString(field->getName(), newname))
		{
		    n++;
		    unique = FALSE;
		    break;
		}
	    }
	}
	str = DuplicateString(newname);
	XmTextSetString(this->field_text, str);
    }

    for(i = 0; i < STRLEN(str); i++)
    {
	if(IsWhiteSpace(str, i))
	{
	    WarningMessage("Spaces and tabs are not allowed in field name.");
	    XtFree(str);
	    return False;
	}
    }

    //
    // Make sure the field name is unique
    //
    for(i = 1; i <= this->fieldList.getSize(); i++)
    {
	field = (Field *)this->fieldList.getElement(i);
	if(EqualString(field->getName(), str))
	{
	    WarningMessage("Field names must be unique.");
	    XtFree(str);
	    return False;
	}
    }

    XmString xmstr = XmStringCreateSimple(str);
    XmListAddItem(this->field_list, xmstr, pos);
    XmListSelectItem(this->field_list, xmstr, False);
    XmStringFree(xmstr);


    //
    // Change pos so it works with our List.
    //
    if(pos == 0) pos = this->fieldList.getSize() + 1;

    this->current_field = pos;

    field = new Field(str);
    this->fieldList.insertElement(field, pos);

    this->widsToField(field);
    XtFree(str);

    XtSetSensitive(this->delete_pb, True);
    XtSetSensitive(this->modify_pb, True);

    this->setVectorInterleavingSensitivity();
    this->setSeriesInterleavingSensitivity();
    this->setDependencyRestrictions();
    this->updateRecordSeparatorList();

    return True;
}

extern "C" void GARMainWindow_DeleteFieldCB(Widget, XtPointer clientData, XtPointer)
{
    GARMainWindow 	*gmw = (GARMainWindow *) clientData;
    int			*position_list;
    int			position_count;
    int			item_count;
    int			new_selected_pos;

    if(!XmListGetSelectedPos(gmw->field_list, &position_list, &position_count))
    {
	WarningMessage("A field must be selected in order to be deleted.");
	return;
    }

    //
    // Since we are not allowing multiple items to be selected...
    //
    ASSERT(position_count < 2);

    //
    // See how many items are in the list
    //
    XtVaGetValues(gmw->field_list, XmNitemCount, &item_count, NULL);

    //
    // Delete the item from the fieldList
    //
    gmw->fieldList.deleteElement(position_list[0]);

    //
    // position_count must == 1 at this point
    //
    XmListDeletePos(gmw->field_list, position_list[0]);

    //
    // Leave the next item selected, unless the last item was deleted.
    //
    if(position_list[0] == item_count)
    {
	if(item_count > 1)
	    new_selected_pos = item_count - 1;
	else 
	    new_selected_pos = 0;
    }
    else
	new_selected_pos = position_list[0];

    if(new_selected_pos > 0)
    {
	XmListSelectPos(gmw->field_list, new_selected_pos, True);
    }
    else
    {
	XtSetSensitive(gmw->delete_pb, False);
	XtSetSensitive(gmw->modify_pb, False);
    }
    
    gmw->current_field = new_selected_pos;
    XtFree((char *)position_list); 

    gmw->setPositionsSensitivity();
    gmw->setVectorInterleavingSensitivity();
    gmw->setMoveFieldSensitivity();
    gmw->setSeriesInterleavingSensitivity();
    gmw->setDependencyRestrictions();

    gmw->setFieldClean();
    theGARApplication->setDirty();
    gmw->updateRecordSeparatorList();
}
extern "C" void GARMainWindow_ModifyFieldCB(Widget, XtPointer clientData, XtPointer)
{
    GARMainWindow 	*gmw = (GARMainWindow *) clientData;
    int			*position_list;
    int			position_count;
    int			i;
    int			pos;
    Field		*field;
    XmStringTable	xms_table;

    if(!XmListGetSelectedPos(gmw->field_list, &position_list, &position_count))
    {
	WarningMessage("A field must be selected in order to be modified.");
	return;
    }
    pos = position_list[0];

    char *str = XmTextGetString(gmw->field_text);
    if(str[0] == '\0')
    {
	WarningMessage("You must enter a valid field name.");
	XtFree(str);
	return;
    }

    //
    // Make sure the field name is unique, but allow the current name
    //
    for(i = 1; i <= gmw->fieldList.getSize(); i++)
    {
	if(i == pos) continue;

	field = (Field *)gmw->fieldList.getElement(i);
	if(EqualString(field->getName(), str))
	{
	    WarningMessage("Field names must be unique.");
	    XtFree(str);
	    return;
	}
    }

    //
    // Change the name in the fieldList
    //
    field = (Field *)gmw->fieldList.getElement(position_list[0]);
    field->setName(str);

    //
    // Get the current list widget xmstring table
    //
    XtVaGetValues(gmw->field_list, XmNitems, &xms_table, NULL);

    XmString xmstr = XmStringCreateSimple(str);
    XmListReplaceItems(gmw->field_list, 
	&xms_table[position_list[0]-1], 1, &xmstr);
    XmListSelectItem(gmw->field_list, xmstr, False);
    XmStringFree(xmstr);
    XtFree(str);

    XtFree((char *)position_list); 
    gmw->widsToField(field);

    gmw->setPositionsSensitivity();
    gmw->setVectorInterleavingSensitivity();
    gmw->setMoveFieldSensitivity();
    gmw->setDependencyRestrictions();

    gmw->setFieldClean();
    theGARApplication->setDirty();
    gmw->updateRecordSeparatorList();
}
extern "C" void GARMainWindow_RecordSepListSelectCB(Widget, XtPointer clientData, XtPointer)
{
    GARMainWindow 	*gmw = (GARMainWindow *) clientData;
    int			*position_list, position_count;
    RecordSeparator	*recsep;

    if(!XmListGetSelectedPos(gmw->record_sep_list, &position_list, &position_count))
    {
       WarningMessage("You must select an item in the record separator list");
       return; 
    }

    ASSERT(position_count == 1);
    gmw->current_rec_sep = position_list[0];
    recsep = (RecordSeparator *)gmw->recordsepList.getElement(position_list[0]);

    gmw->recsepToWids(recsep);
}
extern "C" void GARMainWindow_ListSelectCB(Widget, XtPointer clientData, XtPointer)
{
    GARMainWindow 	*gmw = (GARMainWindow *) clientData;
    int			*position_list;
    int			position_count;
    Field		*field;

    if(!XmListGetSelectedPos(gmw->field_list, &position_list, &position_count))
    {
	gmw->current_field = 0;
	XtSetSensitive(gmw->modify_pb, False);
	XtSetSensitive(gmw->delete_pb, False);
	
	XtSetSensitive(gmw->move_label, False);
	XtSetSensitive(gmw->move_down_ab, False);
	XtSetSensitive(gmw->move_up_ab, False);
	gmw->setTextSensitivity(gmw->move_down_ab, False);
	gmw->setTextSensitivity(gmw->move_up_ab, False);
	return;
    }
    gmw->current_field = position_list[0];
    XtSetSensitive(gmw->modify_pb, True);
    XtSetSensitive(gmw->delete_pb, True);

    if(gmw->fieldList.getSize() > 1)
    {
	XtSetSensitive(gmw->move_label, True);
	if(gmw->current_field == 1)
	{
	    XtSetSensitive(gmw->move_down_ab, True);
	    XtSetSensitive(gmw->move_up_ab, False);
	    gmw->setTextSensitivity(gmw->move_down_ab, True);
	    gmw->setTextSensitivity(gmw->move_up_ab, False);
	}
	else if(gmw->current_field == gmw->fieldList.getSize())
	{
	    XtSetSensitive(gmw->move_down_ab, False);
	    XtSetSensitive(gmw->move_up_ab, True);
	    gmw->setTextSensitivity(gmw->move_down_ab, False);
	    gmw->setTextSensitivity(gmw->move_up_ab, True);
	}
	else
	{
	    XtSetSensitive(gmw->move_down_ab, True);
	    XtSetSensitive(gmw->move_up_ab, True);
	    gmw->setTextSensitivity(gmw->move_down_ab, True);
	    gmw->setTextSensitivity(gmw->move_up_ab, True);
	}
    }
    ASSERT(position_count == 1);

    field = (Field *)gmw->fieldList.getElement(position_list[0]);

    gmw->fieldToWids(field);

    gmw->setFieldClean();

    gmw->setMoveFieldSensitivity();
    theGARApplication->setDirty();
}
void GARMainWindow::setMoveFieldSensitivity()
{
    int			*position_list;
    int			position_count;

    if(!XmListGetSelectedPos(this->field_list, 
			     &position_list, &position_count) ||
       this->fieldList.getSize() < 2)
    {
	XtSetSensitive(this->move_label, False);
	XtSetSensitive(this->move_down_ab, False);
	XtSetSensitive(this->move_up_ab, False);
	this->setTextSensitivity(this->move_down_ab, False);
	this->setTextSensitivity(this->move_up_ab, False);
	return;
    }

    XtSetSensitive(this->move_label, True);
    if(this->current_field == 1)
    {
	XtSetSensitive(this->move_down_ab, True);
	XtSetSensitive(this->move_up_ab, False);
	this->setTextSensitivity(this->move_down_ab, True);
	this->setTextSensitivity(this->move_up_ab, False);
    }
    else if(this->current_field == this->fieldList.getSize())
    {
	XtSetSensitive(this->move_down_ab, False);
	XtSetSensitive(this->move_up_ab, True);
	this->setTextSensitivity(this->move_down_ab, False);
	this->setTextSensitivity(this->move_up_ab, True);
    }
    else
    {
	XtSetSensitive(this->move_down_ab, True);
	XtSetSensitive(this->move_up_ab, True);
	this->setTextSensitivity(this->move_down_ab, True);
	this->setTextSensitivity(this->move_up_ab, True);
    }
}

extern "C" void GARMainWindow_FileSelectCB(Widget, XtPointer clientData, XtPointer)
{
    GARMainWindow 	*gmw = (GARMainWindow *) clientData;

    gmw->fsd->post();
    XmUpdateDisplay(gmw->file_text);

    char *str = XmTextGetString(gmw->file_text);

    if(STRLEN(str) > 0)
    {
	Widget fsd_w = gmw->fsd->getFileSelectionBox();
	Widget ft = XmFileSelectionBoxGetChild(fsd_w, XmDIALOG_FILTER_TEXT);
	Widget st = XmFileSelectionBoxGetChild(fsd_w, XmDIALOG_TEXT);
	char *path = GetDirname(str);
	if(STRLEN(path) > 0)
	{
	    char filter[256];
	    XmTextSetString(ft, path);
	    filter[0] = '\0';
	    strcat(filter, path);
	    strcat(filter,"/*");
	    XmString xmstr = XmStringCreateSimple(filter);
	    XmFileSelectionDoSearch(fsd_w, xmstr);
	    XmStringFree(xmstr);
	}
	XmTextSetString(st, str);
    }

    if(str) XtFree(str);
}
extern "C" void GARMainWindow_IdentifierMVCB(Widget, XtPointer clientData, XtPointer call)
{
    GARMainWindow 	*gmw = (GARMainWindow *) clientData;
    XmTextVerifyCallbackStruct *cbs = (XmTextVerifyCallbackStruct *)call;
    int i;

    if(cbs->text->ptr == NULL) //backspace
    {
	gmw->setFieldDirty();
	theGARApplication->setDirty();
	return;
    }

    int len = cbs->text->length;

    for(i = 0; i < len; i++)
    {
	if(!isprint(cbs->text->ptr[i]) ||
	    cbs->text->ptr[i] == '\n' ||
	    cbs->text->ptr[i] =='\t' ||
	    cbs->text->ptr[i] =='\\' ||
	    cbs->text->ptr[i] ==',' ||
	    cbs->text->ptr[i] == ' ')
	{
	    cbs->doit = False;
	    return;
	}
    }

    gmw->setFieldDirty();

    theGARApplication->setDirty();
}
extern "C" void GARMainWindow_FieldDirtyMVCB(Widget, XtPointer clientData, XtPointer)
{
    GARMainWindow 	*gmw = (GARMainWindow *) clientData;

    gmw->setFieldDirty();
}

extern "C" void GARMainWindow_VerifyNonZeroCB(Widget w, XtPointer clientData , 
					XtPointer)
{
    GARMainWindow 	*gmw = (GARMainWindow *) clientData;
    char 		*str;
   
    if ((w == gmw->string_size) ||
        (w == gmw->points_text) || 
        (w == gmw->series_n_text))  
    {
       str = XmTextGetString(gmw->string_size);
       if(EqualString(str, "0")) 
       {
         WarningMessage("Error: Must be greater than 0");
         XtFree(str);
         return;
      }
      XtFree(str);
    }

    if ((w == gmw->grid_text[0]) || 
        (w == gmw->grid_text[1]) ||
        (w == gmw->grid_text[2]) || 
        (w == gmw->grid_text[3])) 
    {
       str = XmTextGetString(gmw->string_size);
       if(EqualString(str, "0")) 
       {
         WarningMessage("Error: Each grid entry must be greater than 0");
         XtFree(str);
         return;
      }
      XtFree(str);
    }
}


extern "C" void GARMainWindow_RecSepTextCB(Widget w, XtPointer clientData, XtPointer call)
{
    GARMainWindow 	*gmw = (GARMainWindow *) clientData;
    int                 *position_list, position_count;
    RecordSeparator     *recsep;

    if (w==gmw->record_sep_text_1)
    {
      if (!XmListGetSelectedPos(gmw->record_sep_list, &position_list, 
                           &position_count))
      {
        WarningMessage("You must select an item in the record separator list");
        return;
      }
      ASSERT(position_count==1);
      recsep =  
          (RecordSeparator *)gmw->recordsepList.getElement(position_list[0]);
      recsep->setNum(XmTextGetString(w));
    }
    else if (w==gmw->record_sep_text)
    {
      recsep =  
          (RecordSeparator *)gmw->recordsepSingle.getElement(1);

      recsep->setNum(XmTextGetString(w));
    }
  

}
extern "C" void GARMainWindow_IntegerMVCB(Widget w, XtPointer clientData, XtPointer call)
{
    GARMainWindow 	*gmw = (GARMainWindow *) clientData;
    XmTextVerifyCallbackStruct *cbs = (XmTextVerifyCallbackStruct *)call;
    int i;
    int ndx;

    if(cbs->text->ptr == NULL) //backspace
    {
	if( (w == gmw->layout_skip_text)    ||
	    (w == gmw->layout_width_text)   ||
	    (w == gmw->block_skip_text)     ||
	    (w == gmw->block_element_text)  ||
	    (w == gmw->block_width_text)    ||
	    (w == gmw->string_size))  
	{
	    gmw->setFieldDirty();
	}
	theGARApplication->setDirty();
	return;
    }

    int len = cbs->text->length;

    //
    // Strip off leading white space
    //
    ndx = 0;
    while(IsWhiteSpace(cbs->text->ptr, ndx) && (len > 0))
    {
	for(i = 1; i < len; i++)
	{
	    cbs->text->ptr[i-1] = cbs->text->ptr[i];
	}
	len--;
    }
    //
    // Strip off trailing white space
    //
    ndx = 0;
    while(IsWhiteSpace(&cbs->text->ptr[len-1], ndx) && (len > 0))
    {
	len--;
    }
    
    cbs->text->length = len;
    for(i = 0; i < len; i++)
    {
	if(!isdigit(cbs->text->ptr[i]))
	{
	    cbs->doit = False;
	    return;
	}
    }

    if( (w == gmw->layout_skip_text)	||
	(w == gmw->layout_width_text)	||
	(w == gmw->block_skip_text)	||
	(w == gmw->block_element_text)	||
	(w == gmw->block_width_text)	||
	(w == gmw->string_size))		
    gmw->setFieldDirty();

    theGARApplication->setDirty();
}
extern "C" void GARMainWindow_ScalarMVCB(Widget w, XtPointer clientData, XtPointer call)
{
    GARMainWindow 	*gmw = (GARMainWindow *) clientData;
    XmTextVerifyCallbackStruct *cbs = (XmTextVerifyCallbackStruct *)call;
    int i;
    char tmp[4096];
    char *text_str;
    int ptr;

    //
    // We must construct the new string (in tmp) and see if it is valid
    //
    text_str = XmTextGetString(w);
    ASSERT(STRLEN(text_str) + cbs->text->length < 4096);
    ptr = 0;
    for(i = 0; i < cbs->startPos; i++)
	tmp[ptr] = text_str[ptr++];
    for(i = 0; i < cbs->text->length; i++)
	tmp[ptr++] = cbs->text->ptr[i];
    for(i = (int)cbs->startPos; i < STRLEN(text_str); i++)
	tmp[ptr++] = text_str[i];
    tmp[ptr++] = '\0';

    if(cbs->text->ptr == NULL) //backspace
	return;

    if((STRLEN(tmp) == 1) && (tmp[0] == '.'))
    {
	strcpy(tmp, "0.");
	cbs->text->length = 2;
	cbs->text->ptr = DuplicateString(tmp);
	XtAppAddTimeOut(theApplication->getApplicationContext(), 1,
		GARMainWindow::adjustTextCursor, (XtPointer)w);
    }
    if(w == gmw->series_start_text && (STRLEN(tmp) == 1) && (tmp[0] == '-'))
    {
	theGARApplication->setDirty();
	return;
    }
    if(!gmw->IsSingleScalar(tmp))
    {
	cbs->doit = False;
	return;
    }
    theGARApplication->setDirty();
}
void GARMainWindow::adjustTextCursor(XtPointer client , XtIntervalId *)
{
    XmTextSetInsertionPosition((Widget)client, 2);
}
extern "C" void GARMainWindow_OriginDeltaMVCB(Widget, XtPointer , XtPointer call)
{
    XmTextVerifyCallbackStruct *cbs = (XmTextVerifyCallbackStruct *)call;
    int i;

    if(cbs->text->ptr == NULL) //backspace
	return;

    for(i = 0; i < cbs->text->length; i++)
	if(!(isdigit(cbs->text->ptr[i]) ||
	     cbs->text->ptr[i] == ',' ||
	     cbs->text->ptr[i] == ' ' ||
	     cbs->text->ptr[i] == '.' ||
	     cbs->text->ptr[i] == '+' ||
	     cbs->text->ptr[i] == '-') )
	{
	    cbs->doit = False;
	    return;
	}
    theGARApplication->setDirty();
}
extern "C" void GARMainWindow_VerifyPositionCB(Widget w, XtPointer clientData , 
					XtPointer)
{
    GARMainWindow 	*gmw = (GARMainWindow *) clientData;
    int 		i;
    Widget 		wid;
    char 		*str;
    
    //
    // Find which text widget we are working with
    //
    for(i = 0; i < 4; i++)
    {
	if(gmw->position_text[i] == w)
	    break;
    }
    ASSERT(i < 4);

    XtVaGetValues(gmw->regularity_om, XmNmenuHistory, &wid, NULL);
    str = XmTextGetString(gmw->position_text[i]);
    if(EqualString(XtName(wid), "Explicit position list")) 
    {
      int num_elements=1;
      int dim = gmw->getDimension();
      for (i=0; i<dim; i++) 
         num_elements *= atoi(XmTextGetString(gmw->grid_text[i]));
      num_elements *=dim;
      if(!gmw->positionListFormat(str, num_elements))
      {
         WarningMessage("Error in Positions statement #%d.  Entry is not in position list format or the number of elements does not match the grid dimension.", i+1);
         XtFree(str);
         return;
      }
      XtFree(str);
    }
    else
    {
       XtVaGetValues(gmw->positions_om[i], XmNmenuHistory, &wid, NULL);
       if(EqualString(XtName(wid), "regular"))
       {
   	   if(!gmw->originDeltaFormat(str))
	   {
	       WarningMessage("Error in Positions statement #%d. Entry is not in origin, delta format.", i+1);
	       XtFree(str);
	       return;
	   }
	   XtFree(str);
       }
       else
       {
 	   //
	   // The length of a position list is determined by the dim
	   // in the grid statement.
	   //
	   int num_elements =
	       atoi(XmTextGetString(gmw->grid_text[i]));
	   if(!gmw->positionListFormat(str, num_elements))
	   {
	       WarningMessage("Error in Positions statement #%d.  Entry is not in position list format or the number of elements does not match the grid dimension.", i+1);
	       XtFree(str);
	       return;
	   }
	   XtFree(str);
       }
    }
}
//
// This is obviously ineffiecient, but IMHO it simplifies things enough to 
// justify it. (i.e. controlling header_text, series_sep_text and 
// record_sep_text every time one option menu changes.)
//
extern "C" void GARMainWindow_HeaderTypeCB(Widget w, XtPointer clientData, XtPointer)
{
    GARMainWindow 	*gmw = (GARMainWindow *) clientData;



    gmw->changeHeaderType();
}
void GARMainWindow::changeHeaderType()
{
/* XXX this seems strange */
    Widget wid;

    XtRemoveCallback(this->header_text, 
	    XmNmodifyVerifyCallback, 
	    (XtCallbackProc)GARMainWindow_IntegerMVCB, 
	    (XtPointer)this);
    XtVaGetValues(this->header_om, XmNmenuHistory, &wid, NULL);
    if(!EqualString(XtName(wid), "string marker"))
	XtAddCallback(this->header_text,
	      XmNmodifyVerifyCallback,
	      (XtCallbackProc)GARMainWindow_IntegerMVCB,
	      (XtPointer)this);

    XtRemoveCallback(this->series_sep_text, 
	    XmNmodifyVerifyCallback, 
	    (XtCallbackProc)GARMainWindow_IntegerMVCB, 
	    (XtPointer)this);
    XtVaGetValues(this->series_sep_om, XmNmenuHistory, &wid, NULL);
    if(!EqualString(XtName(wid), "string marker"))
	XtAddCallback(this->series_sep_text,
	      XmNmodifyVerifyCallback,
	      (XtCallbackProc)GARMainWindow_IntegerMVCB,
	      (XtPointer)this);

    XtRemoveAllCallbacks(this->record_sep_text, XmNmodifyVerifyCallback);
    XtVaGetValues(this->record_sep_om, XmNmenuHistory, &wid, NULL);
    if(!EqualString(XtName(wid), "string marker"))
	XtAddCallback(this->record_sep_text,
	      XmNmodifyVerifyCallback,
	      (XtCallbackProc)GARMainWindow_IntegerMVCB,
	      (XtPointer)this);


    XtRemoveAllCallbacks(this->record_sep_text_1, XmNmodifyVerifyCallback);
    XtVaGetValues(this->record_sep_om_1, XmNmenuHistory, &wid, NULL);
    if(!EqualString(XtName(wid), "string marker"))
	XtAddCallback(this->record_sep_text_1,
	      XmNmodifyVerifyCallback,
	      (XtCallbackProc)GARMainWindow_IntegerMVCB,
	      (XtPointer)this);
/* jjj
    RecordSeparator->setType(XtName(wid)); */

    theGARApplication->setDirty();
}
int GARMainWindow::getDimension()
{
    int i;
    char *str;

    for(i = 3; i > 0; i--)
    {
	str = XmTextGetString(this->grid_text[i]);
	if(str) {  /* NT may return NULL    */
	    if(!IsAllWhiteSpace(str))
	    {
		XtFree(str);
		return (i+1);
	    }
	    XtFree(str);
	}
    }
    return 1;
}
void GARMainWindow::setDimension(int )
{
    Widget w;

    XtVaGetValues(this->regularity_om, XmNmenuHistory, &w, NULL);
    this->changeRegularity(w);
}

void GARMainWindow::setGridSensitivity(Boolean set)
{
    int i;

    for(i = 0; i < 4; i++)
	this->setTextSensitivity(this->grid_text[i], set);
}
void GARMainWindow::setHeaderSensitivity(Boolean set)
{
    XtSetSensitive(this->header_om, set);
    this->setTextSensitivity(this->header_text, set);
}
void GARMainWindow::setPointsSensitivity(Boolean set)
{
    this->setTextSensitivity(this->points_text, set);
}
void GARMainWindow::setMajoritySensitivity(Boolean set)
{
    XtSetSensitive(this->majority_label, set);
    XtSetSensitive(this->row_label, set);
    XtSetSensitive(this->col_label, set);

    XtVaSetValues(this->col_tb, 
	XmNlabelPixmap, set ? this->col_sens_pix : this->col_insens_pix,
	NULL);
    XtVaSetValues(this->row_tb, 
	XmNlabelPixmap, set ? this->row_sens_pix : this->row_insens_pix,
	NULL);
    if(!set)
    {
        XtUninstallTranslations(this->row_tb);
        XtUninstallTranslations(this->col_tb);
    }
    else
    {
        XtAugmentTranslations(this->row_tb, tb_translations);
        XtAugmentTranslations(this->col_tb, tb_translations);
    }
}

void GARMainWindow::setVectorInterleavingOM(char *name)
{
    if(!(FULL || !SPREADSHEET)) return;

    if(!XtIsManaged(this->insens_vector_interleaving_om))
    {
	XtManageChild(this->insens_vector_interleaving_om);
	SET_OM_NAME(this->insens_vector_interleaving_om, name, 
			"Vector Interleaving");
	XtUnmanageChild(this->insens_vector_interleaving_om);
    }
    else
    {
	SET_OM_NAME(this->insens_vector_interleaving_om, name,
			"Vector Interleaving");
    }

    if(!XtIsManaged(this->sens_vector_interleaving_om))
    {
	XtManageChild(this->sens_vector_interleaving_om);
	SET_OM_NAME(this->sens_vector_interleaving_om, name,
			"Vector Interleaving");
	XtUnmanageChild(this->sens_vector_interleaving_om);
    }
    else
    {
	SET_OM_NAME(this->sens_vector_interleaving_om, name,
			"Vector Interleaving");
    }
}
void GARMainWindow::setVectorInterleavingSensitivity()
{
    Widget	wid;
    Widget	current;
    Boolean	record;
    Boolean	set = False;
    int		i;
    Field	*field;

    if(!(FULL || !SPREADSHEET)) return;
    //
    // Vector interleaving is sensitive if:
    //		1) Field interleaving is "record" AND
    //		2) At least one of the fields has vector data
    //

    XtVaGetValues(this->field_interleaving_om, XmNmenuHistory, &wid, NULL);
    record = EqualString(XtName(wid), "Block");

    if(record)
    {
	for(i = 1; i <= this->fieldList.getSize(); i++)
	{
	    field = (Field *)this->fieldList.getElement(i);

	    char *structure = field->getStructure();
	    int dimension;

	    if(structure[0] == 's')
		dimension = 1;
	    else
		dimension = structure[0] - '0';

	    if(dimension > 1)
	    {
		set = True;
		break;
	    }
	}
    }
    else
	set = False;

    XtSetSensitive(this->vector_interleaving_label, set);

    if(XtIsManaged(this->sens_vector_interleaving_om))
	XtVaGetValues(this->sens_vector_interleaving_om, XmNmenuHistory,
	    &current, NULL);
    else
	XtVaGetValues(this->insens_vector_interleaving_om, XmNmenuHistory,
	    &current, NULL);
    if(set)
    {
	XtManageChild(this->sens_vector_interleaving_om);
	XtUnmanageChild(this->insens_vector_interleaving_om);
    }
    else
    {
	XtManageChild(this->insens_vector_interleaving_om);
	XtUnmanageChild(this->sens_vector_interleaving_om);
    }

    this->setVectorInterleavingOM(XtName(current));
}
Boolean GARMainWindow::getVectorInterleavingSensitivity()
{
    if(!(FULL || !SPREADSHEET)) return False;

    return (XtIsManaged(this->sens_vector_interleaving_om));
}
void GARMainWindow::setSeriesSensitivity(Boolean set)
{
    XtSetSensitive(this->series_n_label, set);
    XtSetSensitive(this->series_start_label, set);
    XtSetSensitive(this->series_delta_label, set);
    this->setTextSensitivity(this->series_n_text, set);
    this->setTextSensitivity(this->series_start_text, set);
    this->setTextSensitivity(this->series_delta_text, set);
}

//
// The Series Interleaving sensitivity depends on:
//	a) The field interleave (field or record)
//	b) Is Series enabled (series_tb)
//	c) Is there more than one field
//
void GARMainWindow::setSeriesInterleavingSensitivity()
{
    Widget wid;
    Boolean set;
    WidgetList children;
    int nchildren;
    int i;
    Widget current;

    XtVaGetValues(this->field_interleaving_om, XmNmenuHistory, &wid, NULL);
    if(!wid) return;

    if(EqualString(XtName(wid), "Columnar"))
    {
	set = False;
    }
    else
    {
	XtVaGetValues(this->series_tb, XmNset, &set, NULL);
    }
    if(this->fieldList.getSize() < 2)
	set = False;
    
    XtSetSensitive(this->series_interleaving_label, set);

    XtVaGetValues(this->series_interleaving_om, XmNsubMenuId, &wid, NULL);
    XtVaGetValues(wid, XmNchildren, &children, 
		       XmNnumChildren, &nchildren, 
		       NULL);
    XtVaGetValues(this->series_interleaving_om, XmNmenuHistory, &current, NULL);

    //
    // Manage them all to avoid any potential wierd stuff.
    //
    for(i = 0; i < nchildren; i++)
    {
	XtManageChild(children[i]);
    }
    if(!set)
    {
	if(EqualString(XtName(current), "w0")) 
	    for(i = 0; i < nchildren; i++)
	    {
		if(EqualString(XtName(children[i]), "w0_insens"))
		{
		    XtVaSetValues(this->series_interleaving_om, 
			XmNmenuHistory, children[i], NULL);
		    break;
		}
	    }
	else if(EqualString(XtName(current), "w1")) 
	    for(i = 0; i < nchildren; i++)
	    {
		if(EqualString(XtName(children[i]), "w1_insens"))
		{
		    XtVaSetValues(this->series_interleaving_om, 
			XmNmenuHistory, children[i], NULL);
		    break;
		}
	    }
    }
    else
    {
	if(EqualString(XtName(current), "w0_insens")) 
	    for(i = 0; i < nchildren; i++)
	    {
		if(EqualString(XtName(children[i]), "w0"))
		{
		    XtVaSetValues(this->series_interleaving_om, 
			XmNmenuHistory, children[i], NULL);
		    break;
		}
	    }
	else if(EqualString(XtName(current), "w1_insens")) 
	    for(i = 0; i < nchildren; i++)
	    {
		if(EqualString(XtName(children[i]), "w1"))
		{
		    XtVaSetValues(this->series_interleaving_om, 
			XmNmenuHistory, children[i], NULL);
		    break;
		}
	    }
    }

    //
    // Manage the appropriate ones
    //
    for(i = 0; i < nchildren; i++)
    {
	if(EqualString(XtName(children[i]), "w0") ||
	   EqualString(XtName(children[i]), "w1") )
	{
	    set ? XtManageChild(children[i]) : XtUnmanageChild(children[i]);
	}
	else
	{
	    set ? XtUnmanageChild(children[i]) : XtManageChild(children[i]);
	}
    }
    if(!set)
    {
	XtUninstallTranslations(this->series_interleaving_om);
    }
    else
	XtAugmentTranslations(this->series_interleaving_om, translations);
}
void GARMainWindow::setSeriesSepSensitivity()
{
    Boolean set;

    XtVaGetValues(this->series_tb, XmNset, &set, NULL);
    XtSetSensitive(this->series_sep_tb, set);

    if(set)
	XtVaGetValues(this->series_sep_tb, XmNset, &set, NULL);

    XtSetSensitive(this->series_sep_om, set);
    this->setTextSensitivity(this->series_sep_text, set);
}
extern "C" void GARMainWindow_LayoutTBCB(Widget, XtPointer clientData, XtPointer)
{
    GARMainWindow 	*gmw = (GARMainWindow *) clientData;

    gmw->setLayoutSensitivity();

    theGARApplication->setDirty();
}
extern "C" void GARMainWindow_BlockTBCB(Widget, XtPointer clientData, XtPointer)
{
    GARMainWindow 	*gmw = (GARMainWindow *) clientData;

    gmw->setBlockSensitivity();

    theGARApplication->setDirty();
}
extern "C" void GARMainWindow_RecsepTBCB(Widget, XtPointer clientData, XtPointer)
{
    GARMainWindow 	*gmw = (GARMainWindow *) clientData;
    
    gmw->setRecordSepSensitivity();
    theGARApplication->setDirty();
}
extern "C" void GARMainWindow_SameSepCB(Widget, XtPointer clientData, XtPointer)
{
    GARMainWindow 	*gmw = (GARMainWindow *) clientData;
    gmw->setRecordOptionsSensitivity(); 
    theGARApplication->setDirty();
}
extern "C" void GARMainWindow_DiffSepCB(Widget, XtPointer clientData, XtPointer)
{
    GARMainWindow 	*gmw = (GARMainWindow *) clientData;
    gmw->setRecordOptionsSensitivity(); 
    theGARApplication->setDirty();
}
void GARMainWindow::setLayoutTBSensitivity()
{
    Widget w;
    Boolean new_sens;
    Boolean tb_state;
    Boolean current_sens = XtIsSensitive(this->layout_tb);

    XtVaGetValues(this->field_interleaving_om, XmNmenuHistory, &w, NULL);
    if( EqualString("Columnar", XtName(w)) )
	new_sens = True;
    else
	new_sens = False;

    //
    // The format has to be ASCII (or Text) for layout to be true.
    //
    XtVaGetValues(this->format2_om, XmNmenuHistory, &w, NULL);
    if(EqualString("Binary (IEEE)", XtName(w)))
	new_sens = False;

    //
    // No change ==> return;
    //
    if(new_sens == current_sens) return;

    //
    // If we are turning it on, restore the old tb state
    //
    if(new_sens && !current_sens)
	tb_state = this->layout_was_on;

    //
    // If we are turning off the sens, set the tb_state to off and remember
    // the current state.
    //
    if(!new_sens)
    {
	tb_state = False;
	this->layout_was_on = XmToggleButtonGetState(this->layout_tb);
    }


    XmToggleButtonSetState(this->layout_tb, tb_state, False);
    XtSetSensitive(this->layout_tb, new_sens);

    this->setLayoutSensitivity();
    
}
void GARMainWindow::setLayoutSensitivity()
{
    Boolean set = XmToggleButtonGetState(this->layout_tb);
    XtSetSensitive(this->layout_skip_label, set);
    XtSetSensitive(this->layout_width_label, set);
    this->setTextSensitivity(this->layout_skip_text, set);
    this->setTextSensitivity(this->layout_width_text, set);
}
void GARMainWindow::setBlockTBSensitivity()
{
    Widget w;
    Boolean new_sens;
    Boolean tb_state;
    Boolean current_sens = XtIsSensitive(this->block_tb);

    XtVaGetValues(this->field_interleaving_om, XmNmenuHistory, &w, NULL);
    if( EqualString("Columnar", XtName(w)) )
	new_sens = False;
    else
	new_sens = True;

    //
    // The format has to be ASCII (or Text) for block/layout to be true.
    //
    XtVaGetValues(this->format2_om, XmNmenuHistory, &w, NULL);
    if( EqualString("Binary (IEEE)", XtName(w)))
	new_sens = False;

    //
    // No change ==> return;
    //
    if(new_sens == current_sens) return;

    //
    // If we are turning it on, restore the old tb state
    //
    if(new_sens && !current_sens)
	tb_state = this->block_was_on;

    //
    // If we are turning off the sens, set the tb_state to off and remember
    // the current state.
    //
    if(!new_sens)
    {
	tb_state = False;
	this->block_was_on = XmToggleButtonGetState(this->block_tb);
    }

    XmToggleButtonSetState(this->block_tb, tb_state, False);
    XtSetSensitive(this->block_tb, new_sens);

    this->setBlockSensitivity();
    
}
void GARMainWindow::setBlockSensitivity()
{
    Boolean set = XmToggleButtonGetState(this->block_tb);
    XtSetSensitive(this->block_skip_label, set);
    XtSetSensitive(this->block_element_label, set);
    XtSetSensitive(this->block_width_label, set);
    this->setTextSensitivity(this->block_skip_text, set);
    this->setTextSensitivity(this->block_element_text, set);
    this->setTextSensitivity(this->block_width_text, set);
}
void GARMainWindow::setRecordSepTBSensitivity()
{
    Widget w;
    Widget sw;
    Boolean new_sens;
    Boolean tb_state;
    Boolean current_sens = XtIsSensitive(this->record_sep_tb);

    XtVaGetValues(this->field_interleaving_om, XmNmenuHistory, &w, NULL);
    if( EqualString("Columnar", XtName(w)) )
	new_sens = False;
    else
    {
	XtVaGetValues(this->series_interleaving_om, XmNmenuHistory, &sw, NULL);
	new_sens = !(EqualString(XtName(sw), "w1"));
    }

    //
    // No change ==> return;
    //
    if(new_sens == current_sens) return;

    //
    // If we are turning it on, restore the old tb state
    //
    if(new_sens)
	tb_state = this->record_sep_was_on;

    //
    // If we are turning off the sens, set the tb_state to off and remember
    // the current state.
    //
    if(!new_sens)
    {
	tb_state = False;
	this->record_sep_was_on = XmToggleButtonGetState(this->record_sep_tb);
    }


    XmToggleButtonSetState(this->record_sep_tb, tb_state, False);
    XtSetSensitive(this->record_sep_tb, new_sens);

    this->setRecordSepSensitivity();
}
void GARMainWindow::setRecordSepSensitivity()
{
    int item_count;

    Boolean set = XmToggleButtonGetState(this->record_sep_tb);

    /* if there aren't any records to separate no sense in letting
       people set it */

    XtVaGetValues(this->record_sep_list, XmNitemCount, &item_count, NULL);
    if (item_count == 0) set = False;

    XtSetSensitive(this->same_sep_tb, set);
    XtSetSensitive(this->diff_sep_tb, set); 
    if (!set) {
      XmToggleButtonSetState(this->same_sep_tb, False, True);
      XmToggleButtonSetState(this->diff_sep_tb, False, True);
    } 
    this->setRecordOptionsSensitivity();   
}
void GARMainWindow::setRecordOptionsSensitivity()
{    
    Boolean set = XmToggleButtonGetState(this->same_sep_tb);
    XtSetSensitive(this->record_sep_om, set);
    this->setTextSensitivity(this->record_sep_text, set);

    set = XmToggleButtonGetState(this->diff_sep_tb);
    XtSetSensitive(this->record_sep_om_1, set);
    XtSetSensitive(this->record_sep_list, set);
    this->setTextSensitivity(this->record_sep_text_1, set);
}
void GARMainWindow::setDependencySensitivity(Boolean set)
{
    XtSetSensitive(this->dependency_label, set);
    XtSetSensitive(this->dependency_om, set);
}


void GARMainWindow::setTextSensitivity(Widget w, Boolean sens)
{
    Pixel fg;

    if(sens)
	fg = BlackPixel(theGARApplication->getDisplay(), 0);
    else
	fg = theGARApplication->getInsensitiveColor();

    XtVaSetValues(w, 
	XmNforeground, fg, 
	XmNeditable, sens,
	NULL);
}
void GARMainWindow::clearText(Widget w)
{
    XmTextSetString(w, "");
}
void GARMainWindow::recsepToWids(RecordSeparator *recsep)
{
    if (recsep) 
    {
      SET_OM_NAME(this->record_sep_om_1, recsep->getType(), 
                   "record separator type"); 
      XmTextSetString(this->record_sep_text_1, recsep->getNum());
    }
}
void GARMainWindow::fieldToWids(Field *field)
{
    XmTextSetString(this->field_text, field->getName()); 

    //
    // Type
    //
    SET_OM_NAME(this->type_om, field->getType(), "Type");

    //
    // Structure
    //
    // There is one structure name which requires sepcial handling...
    // structure = string[n]
    // We only want the n, and it goes into a text widget not the option menu
    // This also implies that type_om should be holding "string"
    //
    char *cp;
    boolean string_size = FALSE;
    if ((cp = strstr(field->getStructure(), "string["))) {
        if (strlen(cp) >= 2) {
            char tmpbuf[32];
	    cp = strstr(field->getStructure(), "["); cp++;
            strcpy (tmpbuf, cp);
	    cp = strstr(tmpbuf, "]"); *cp = '\0';
            XmTextSetString (this->string_size, tmpbuf);
            string_size = TRUE;
            XtSetSensitive (this->string_size, True);
            XtSetSensitive (this->string_size_label, True);
            XtSetSensitive (this->structure_om, False);
        }
    }

    if (!string_size) {
	SET_OM_NAME(this->structure_om, field->getStructure(), "Structure");
        XtSetSensitive (this->string_size, False);
        XtSetSensitive (this->string_size_label, False);
        XtSetSensitive (this->structure_om, True);
    }

    XmTextSetString(this->layout_skip_text, field->getLayoutSkip());
    XmTextSetString(this->layout_width_text, field->getLayoutWidth());
    XmTextSetString(this->block_skip_text, field->getBlockSkip());
    XmTextSetString(this->block_element_text, field->getBlockElement());
    XmTextSetString(this->block_width_text, field->getBlockWidth());

    //
    // Dependency
    //
    SET_OM_NAME(this->dependency_om, field->getDependency(), "Dependency");
}
void GARMainWindow::widsToField(Field *field)
{
    Widget w;

    field->setName(XmTextGetString(this->field_text)); 

    //
    // Type
    //
    XtVaGetValues(this->type_om, XmNmenuHistory, &w, NULL);
    field->setType(XtName(w));

    //
    // Structure
    //
    // Special case... structure = string[n] where n is some int.
    // For this case, you must form the string rather than using a widget name.
    //
    char cp[64];
    if (!strcmp (field->getType(), "string")) {
	int size = 0;
	char *tsize;
	XtVaGetValues (this->string_size, XmNvalue, &tsize, NULL);
	if ((tsize) && (tsize[0])) sscanf (tsize, "%d", &size);
	sprintf (cp, "string[%d]", size);
	free(tsize);
    } else {
	XtVaGetValues(this->structure_om, XmNmenuHistory, &w, NULL);
	strcpy (cp, XtName(w));
    }
    field->setStructure(cp);

    field->setLayoutSkip(   XmTextGetString(this->layout_skip_text)   );
    field->setLayoutWidth(  XmTextGetString(this->layout_width_text)  );
    field->setBlockSkip(    XmTextGetString(this->block_skip_text)    );
    field->setBlockElement( XmTextGetString(this->block_element_text) );
    field->setBlockWidth(   XmTextGetString(this->block_width_text)   );


    //
    // Dependency
    //
    XtVaGetValues(this->dependency_om, XmNmenuHistory, &w, NULL);
    field->setDependency(XtName(w));

}

//
// called by MainWindow::CloseCallback().
//
void GARMainWindow::closeWindow()
{
    this->closeGARCmd->execute();
}

void GARMainWindow::createFileMenu(Widget parent)
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

    this->closeGAROption =
       new ButtonInterface(pulldown,"closeGAROption",this->closeGARCmd);

    theGARApplication->addHelpCallbacks(pulldown);
}
void GARMainWindow::createEditMenu(Widget parent)
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
    theGARApplication->addHelpCallbacks(pulldown);
}
void GARMainWindow::createOptionMenu(Widget parent)
{
    ASSERT(parent);

    Widget pulldown;

    //
    // Create "Option" menu and options.
    //
    pulldown =
	this->optionMenuPulldown =
	    XmCreatePulldownMenu(parent, "optionMenuPulldown", NUL(ArgList), 0);

    this->optionMenu =
	XtVaCreateManagedWidget
	    ("optionMenu",
	     xmCascadeButtonWidgetClass,
	     parent,
	     XmNsubMenuId, pulldown,
	     NULL);

    this->fullOption = new ButtonInterface(pulldown, 
					   "fullOption", 
					   this->fullCmd);
    this->simplifyOption = new ButtonInterface(pulldown, 
					   "simplifyOption", 
					   this->simplifyCmd);
    theGARApplication->addHelpCallbacks(pulldown);
}

void GARMainWindow::createHelpMenu(Widget parent)
{
    this->createBaseHelpMenu(parent,TRUE,TRUE);

    XtVaCreateManagedWidget("separator", xmSeparatorWidgetClass,
                                        this->helpMenuPulldown,
                                        NULL);
    this->helpTutorialOption =
            new ButtonInterface(this->helpMenuPulldown, "helpTutorialOption",
                theIBMApplication->helpTutorialCmd);

    XtVaCreateManagedWidget("separator", xmSeparatorWidgetClass,
                                        this->helpMenuPulldown,
                                        NULL);

    this->helpGAFormatOption =
            new ButtonInterface(this->helpMenuPulldown, "helpGAFormatOption",
                theIBMApplication->genericHelpCmd);



}


void GARMainWindow::createMenus(Widget parent)
{
    ASSERT(parent);

    //
    // Create the menus.
    //
    this->createFileMenu(parent);
    this->createEditMenu(parent);
    this->createOptionMenu(parent);
    this->createHelpMenu(parent);

    theGARApplication->addHelpCallbacks(this->fileMenu);
    theGARApplication->addHelpCallbacks(this->editMenu);
    theGARApplication->addHelpCallbacks(this->optionMenu);
    theGARApplication->addHelpCallbacks(this->helpMenu);
}

void GARMainWindow::newGAR (unsigned long mode)
{
    boolean same_gar = (this->mode==mode);
    if (same_gar) this->newGAR();

    //
    // Make some last minute adjustments
    //
    if(POINTS_ONLY || POINTS_SELECTED)
    {
	XtVaSetValues(this->grid_tb,   XmNset, False, NULL);
	XtVaSetValues(this->points_tb, XmNset, True,  NULL);
	this->setTextSensitivity(this->points_text, True);
        this->setTextSensitivity(this->grid_text[0], False);
        this->setTextSensitivity(this->grid_text[1], False);
        this->setTextSensitivity(this->grid_text[2], False);
        this->setTextSensitivity(this->grid_text[3], False);
        XmString xms = XmStringCreateLtoR("Point\npositions", XmSTRING_DEFAULT_CHARSET);
	XtVaSetValues(this->regularity_label, XmNlabelString, xms, NULL);
	XmStringFree(xms);
    }



    if(POSITION_LIST_ON)
    {
	Widget wid;
	SET_OM_NAME(this->regularity_om, "Explicit position list", "Regularity");
	XtVaGetValues(this->regularity_om, XmNmenuHistory, &wid, NULL);
	this->changeRegularity(wid);
    }

    SET_OM_NAME(this->field_interleaving_om, 
	    SPREADSHEET ? "Columnar" : "Block",
	    "Field Interleaving");

    if(SERIES_IS_ON)
	XmToggleButtonSetState(this->series_tb, True, True);

    if (!same_gar) this->newGAR();
}

void GARMainWindow::newGAR()
{
    int i;
    Field *field;
    RecordSeparator *recsep;

    this->dimension = 1;
    this->current_field = 0;

    this->setWindowTitle("Data Prompter: ");
    this->saveCmd->deactivate();

    this->comment_dialog->clearText();
    if(this->comment_text)
    {
	delete this->comment_text;
	this->comment_text = NULL;
    }

    //
    // Clear all the text widgets
    //
    //this->clearText(this->file_text);
    //this->setDataFilename (NUL(char*));
    this->clearText(this->points_text);
    this->clearText(this->header_text);
    this->clearText(this->series_sep_text);
    this->clearText(this->layout_skip_text);
    this->clearText(this->layout_width_text);
    this->clearText(this->block_skip_text);
    this->clearText(this->block_element_text);
    this->clearText(this->block_width_text);

    this->clearText(this->field_text);

    this->clearText(this->record_sep_text);
    this->clearText(this->record_sep_text_1);

    XmTextSetString(this->series_n_text, "1");
    XmTextSetString(this->series_start_text, "1");
    XmTextSetString(this->series_delta_text, "1");
    for(i = 0; i < 4; i++)
    {
	this->clearText(this->grid_text[i]);
	XmTextSetString(this->position_text[i], "0, 1");
    }

    //
    // Clear the recordlist list
    //
    XmListDeleteAllItems(this->record_sep_list);

    for(i = 1; i <= this->recordsepList.getSize(); i++)
    {
	recsep = (RecordSeparator *)this->recordsepList.getElement(i);
	delete recsep;
    }
    this->recordsepList.clear();

    //
    // Set the toggle buttons
    //


    if(!POINTS_ONLY)
    {
	XmToggleButtonSetState(this->grid_tb,               True,  True);
    }
    else
    {
	XmToggleButtonSetState(this->grid_tb,               False, True);
        XmString xms = XmStringCreateSimple("Point \npositions");
        xms = XmStringCreateLtoR("Point\npositions", XmSTRING_DEFAULT_CHARSET);
	XtVaSetValues(this->regularity_label, XmNlabelString, xms, NULL);
	XmStringFree(xms);
    }
    XmToggleButtonSetState(this->header_tb,                 False, True);
    XmToggleButtonSetState(this->series_tb,                 False, True);
    XmToggleButtonSetState(this->series_sep_tb,             False, True);

    RESET_OM(this->format1_om);
    RESET_OM(this->format2_om);
    XmToggleButtonSetState(this->row_tb, True, False);
    XmToggleButtonSetState(this->col_tb, False, False);
    RESET_OM(this->field_interleaving_om);
    RESET_OM(this->insens_vector_interleaving_om);
    RESET_OM(this->sens_vector_interleaving_om);
    
    RESET_OM(this->series_interleaving_om);
    RESET_OM(this->series_sep_om);
    RESET_OM(this->header_om);
    RESET_OM(this->regularity_om);
    for(i = 0; i < 4; i++)
	RESET_OM(this->positions_om[i]);

    RESET_OM(this->type_om);

    XtSetSensitive(this->string_size, False);
    XtSetSensitive(this->string_size_label, False);
    XtSetSensitive(this->structure_om, True);
    RESET_OM(this->structure_om);

    RESET_OM(this->record_sep_om);
    RESET_OM(this->record_sep_om_1);
    RESET_OM(this->dependency_om);

    if(!POINTS_ONLY)
       this->setPointsSensitivity(False);
    this->setVectorInterleavingSensitivity();
    this->setHeaderSensitivity(False);
    this->setSeriesSensitivity(False);
    this->setSeriesInterleavingSensitivity();
    this->setSeriesSepSensitivity();
    this->restrictStructure(False);
    this->setPositionsSensitivity();
    this->setBlockLayoutSensitivity();
    this->setRecordSepTBSensitivity();

    XtSetSensitive(this->format1_om, False);

    //
    // Clear the field list
    //
    XmListDeleteAllItems(this->field_list);

    for(i = 1; i <= this->fieldList.getSize(); i++)
    {
	field = (Field *)this->fieldList.getElement(i);
	delete field;
    }
    this->fieldList.clear();
    this->setFieldClean();
    this->setMoveFieldSensitivity();
    theGARApplication->setClean();

    GARChooserWindow *gcw = theGARApplication->getChooserWindow();
    if (gcw)
	gcw->setCommandActivation();
}
boolean GARMainWindow::saveGAR(char *file)
{
    this->setFilename(file);
    this->saveCmd->activate();

    if(!validateGAR()) return False;

    std::ofstream *to = new std::ofstream(file);
    if(!*to)
    {
	WarningMessage("File open failed on save");
	return False;
    }

    saveGAR(to);
    return True;
}
void GARMainWindow::saveGAR(std::ostream *to)
{
    int i;
    int k;
    Widget w;
    Field *field;
    char *str;
    char title[256];
    char *file = this->getFilename();

    if(XmToggleButtonGetState(this->modify_tb))
      WarningMessage("Modifications have been made to field parameters without \"applying\" the change");


    title[0] = '\0';
    strcat(title, "Data Prompter: ");
    if (file)
      strcat(title, file);
    this->setWindowTitle(title);
    
    i = 0;
    if(STRLEN(this->comment_text) > 0)
      {
	*to << "# ";
	while(this->comment_text[i])
	  {
	    to->put(this->comment_text[i]);
	    if(this->comment_text[i] == '\n') 
	      if(this->comment_text[i+1]) *to << "# ";
	    i++;
	  }
	*to << std::endl;
      }
    
    
    *to << "file = " << XmTextGetString(this->file_text) << std::endl;
    
    if(XmToggleButtonGetState(this->grid_tb))
      {
	*to << "grid = " << XmTextGetString(this->grid_text[0]);
	for(i = 1; i < this->dimension; i++)
	  *to << " x " << XmTextGetString(this->grid_text[i]);
      }
    else
      {
	*to << "points = " << XmTextGetString(this->points_text);
      }
    *to << std::endl;
    
    *to << "format = ";
    if(XtIsSensitive(this->format1_om))
      {
	OM_NAME_OUT(*to, this->format1_om);
	*to << " ";
      }
    OM_NAME_OUT(*to, this->format2_om);
    *to << std::endl;
    
    //
    // Interleaving translation:
    //
    //	Output		Field Int.	Vector Int.	Series Int.
    //	======		==========	===========	===========
    //	field		Field		Insens		Insens
    //	record		Record		Insens		Insens
    //	record-vector	Record		w0		Insens
    //	record		Record		w1		Insens
    //	record		Record		Insens		w0
    //	series-vector	Record		Insens		w1
    //	record-vector	Record		w0		w0
    //	series-vector	Record		w0		w1
    //	record		Record		w1		w0
    //

    *to << "interleaving = ";
    XtVaGetValues(this->field_interleaving_om, XmNmenuHistory, &w, NULL);
    if(EqualString(XtName(w), "Columnar"))
      {
	*to << "field";
      }
    else
      {
	//
	// Get the information about the state of things...
	//
	Widget vw;	// "vector widget"
	Widget sw;	// "series widget"
	Boolean vector_sens = this->getVectorInterleavingSensitivity();
	Boolean series_sens = 
		XtIsSensitive(this->series_interleaving_label);
	XtVaGetValues(this->series_interleaving_om, 
		XmNmenuHistory, &sw, NULL);
	XtVaGetValues(this->sens_vector_interleaving_om, 
		XmNmenuHistory, &vw, NULL);

	if(!vector_sens && !series_sens)
	  {
	    *to << "record";
	  }
	else
	  {
	    if(!vector_sens)
	      {
		if(EqualString(XtName(sw), "w0"))
		  *to << "record";
		else
		  *to << "series-vector";
	      }
	    else if(EqualString(XtName(vw), "w0"))
	      {
		if(EqualString(XtName(sw), "w1"))
		  *to << "series-vector";
		else
		  *to << "record-vector";
	      }
	    else // vw = w1
	      *to << "record";
	  }
      }
    *to << std::endl;
    
    if(XtIsSensitive(this->majority_label))
      {
	*to << "majority = ";
	if(XmToggleButtonGetState(this->row_tb))
	  *to << "row";
	else
	  *to << "column";
	*to << std::endl;
      }
    
    if(XmToggleButtonGetState(this->header_tb))
      {
	*to << "header = ";
	XtVaGetValues(this->header_om, XmNmenuHistory, &w, NULL);
	
	if(EqualString(XtName(w), "string marker"))
	  *to << "marker";
	else if(EqualString(XtName(w), "# of bytes"))
	  *to << "bytes";
	else
	  *to << "lines";
	//
	// Quote the Marker strings
	//
	XtVaGetValues(this->header_om, XmNmenuHistory, &w, NULL);
	if(EqualString(XtName(w), "string marker"))
	  {
	    str = XmTextGetString(this->header_text);
	    STRIP_QUOTES(str);
	    *to << " \"" << str << "\"" << std::endl;
	    XtFree(str);
	  }
	else
	  {
	    *to << " " << XmTextGetString(this->header_text) << std::endl;
	  }
      }
    
    if(XmToggleButtonGetState(this->series_tb))
      {
	*to << "series = ";
	
	str = XmTextGetString(this->series_n_text);
	*to << (!IsAllWhiteSpace(str) ? str : "1") << ", ";
	XtFree(str);
	
	str = XmTextGetString(this->series_start_text);
	*to << (!IsAllWhiteSpace(str) ? str : "1") << ", ";
	XtFree(str);
	
	str = XmTextGetString(this->series_delta_text);
	*to << (!IsAllWhiteSpace(str) ? str : "1");
	XtFree(str);
	
	if(XmToggleButtonGetState(this->series_sep_tb))
	  {
	    *to << ", separator = ";
	    
	    //
	    // Quote the Marker strings
	    //
	    XtVaGetValues(this->series_sep_om, XmNmenuHistory, &w, NULL);

	    if(EqualString(XtName(w), "string marker"))
	      *to << "marker";
	    else if(EqualString(XtName(w), "# of bytes"))
	      *to << "bytes";
	    else
	      *to << "lines";
	    if(EqualString(XtName(w), "marker"))
	      {
		str = XmTextGetString(this->series_sep_text);
		STRIP_QUOTES(str);
		*to << " \"" << str << "\"";
		XtFree(str);
	      }
	    else
	      {
		*to << " " << XmTextGetString(this->series_sep_text);
	      }
	  }
	*to << std::endl;
      }

    //
    // Per field statements
    //
    if(this->fieldList.getSize() > 0)
      {
	//
	// Field names
	//
	*to << "field = ";
	for(i = 1; i <= this->fieldList.getSize(); i++)
	  {
	    field = (Field *)this->fieldList.getElement(i);
	    *to << field->getName();
	    if(i < this->fieldList.getSize()) *to << ", ";
	  }
	*to << std::endl;
	
	//
	// Structure
	//
	*to << "structure = ";
	for(i = 1; i <= this->fieldList.getSize(); i++)
	  {
	    field = (Field *)this->fieldList.getElement(i);
	    *to << field->getStructure();
	    if(i < this->fieldList.getSize()) *to << ", ";
	  }
	*to << std::endl;
	
	//
	// Type
	//
	*to << "type = ";
	for(i = 1; i <= this->fieldList.getSize(); i++)
	  {
	    field = (Field *)this->fieldList.getElement(i);
	    *to << field->getType();
	    if(i < this->fieldList.getSize()) *to << ", ";
	  }
	*to << std::endl;
	
	//
	// Dependency
	//
	if(XtIsSensitive(this->dependency_label))
	  {
	    *to << "dependency = ";
	    for(i = 1; i <= this->fieldList.getSize(); i++)
	      {
		field = (Field *)this->fieldList.getElement(i);
		*to << field->getDependency();
		if(i < this->fieldList.getSize()) *to << ", ";
	      }
	    *to << std::endl;
	  }

	//
	// Layout
	//

	if(XmToggleButtonGetState(this->layout_tb))
	  {
	    *to << "layout = ";
	    for(i = 1; i <= this->fieldList.getSize(); i++)
	      {
		field = (Field *)this->fieldList.getElement(i);
		*to << field->getLayoutSkip() 
		  << ", " 
		    << field->getLayoutWidth();
		if(i < this->fieldList.getSize()) *to << ", ";
	      }
	    *to << std::endl;
	  }



	//
	// Block
	//

	if(XmToggleButtonGetState(this->block_tb))
	  {
	    *to << "block = ";
	    for(i = 1; i <= this->fieldList.getSize(); i++)
	      {
		field = (Field *)this->fieldList.getElement(i);
		*to << field->getBlockSkip()    << ", " 
		  << field->getBlockElement() << ", "
		    << field->getBlockWidth();
		if(i < this->fieldList.getSize()) *to << ", ";
	      }
	    *to << std::endl;
	  }

      }

	//
	// Record sep 
	//
	if(XmToggleButtonGetState(this->record_sep_tb))
	{

	  /* just a single record separator */
	  if (XmToggleButtonGetState(this->same_sep_tb))
            {
	      RecordSeparator *recsep = 
		(RecordSeparator *)this->recordsepSingle.getElement(1);
	      *to << "recordseparator = ";
	      str = DuplicateString(recsep->getType());
	      if (EqualString(str, "# of bytes"))
		*to << "bytes" << " ";
	      else if (EqualString(str,"# of lines"))
		*to << "lines" << " ";
	      else if (EqualString(str,"string marker"))
		*to << "marker" << " ";
	      delete str;
	      
	      if (EqualString(recsep->getType(),"string marker"))
		{
                  str = DuplicateString(recsep->getNum());
                  STRIP_QUOTES(str);
                  *to << "\"" << str << "\"";
                  delete str;
		}
	      else
		*to << recsep->getNum();
            }
          
	  /* else a set of different record separators */ 
	  else
            {
	      *to << "recordseparator = ";
	      /* how many items in the list? */
	      for (k = 1; k <= this->recordsepList.getSize(); k++)
		{
		  RecordSeparator *recsep = 
		    (RecordSeparator *)this->recordsepList.getElement(k);
		  str = DuplicateString(recsep->getType());
		  if (EqualString(str, "# of bytes"))
		    *to << "bytes" << " ";
		  else if (EqualString(str,"# of lines"))
		    *to << "lines" << " ";
		  else if (EqualString(str,"string marker"))
		    *to << "marker" << " ";
		  delete str;
		  
		  if (EqualString(recsep->getType(),"string marker"))
		    {
                      str = DuplicateString(recsep->getNum());
                      STRIP_QUOTES(str);
                      *to << "\"" << str << "\"";
                      delete str;
		    }
		  else
		    *to << recsep->getNum();
		  if (k != this->recordsepList.getSize())
		    *to << ", ";
                }
	      
	      *to << std::endl;
	    }
	*to << std::endl;
	}

    //
    // Positions statement: MUST be last thing before end statement
    //
    if(XtIsSensitive(this->regularity_om))
      {
	*to << "positions = ";
	Widget reg;	// "regularity widget"
	  XtVaGetValues(this->regularity_om, 
			XmNmenuHistory, &reg, NULL);
	if(EqualString(XtName(reg), "Explicit position list")) 
        {
	    str = XmTextGetString(this->position_text[0]);
	    *to << str ;
	    XtFree(str);
        }
        else 
        {
	for(i = 0; i < this->dimension; i++)
	{
	    OM_NAME_OUT(*to, this->positions_om[i]);
	    *to << ", ";
	}
	for(i = 0; i < this->dimension; i++)
	{
	    str = XmTextGetString(this->position_text[i]);
	    *to << (!IsAllWhiteSpace(str) ? str : "0,1");
	    XtFree(str);

	    if(i < this->dimension-1)
		*to << ", ";
	}
        }
	*to << std::endl;
    }

    *to << std::endl << "end" << std::endl;

    theGARApplication->setClean();
}
//
// "complete" validation takes place when saving a file.  Non-complete
// validation takes place when switching between full/simp or visa-versa.
//
Boolean GARMainWindow::validateGAR(Boolean complete)
{
    int i;
    Widget w;
    Field *field;
    char *str;

    if(complete)
    {
	str =  XmTextGetString(this->file_text);
	if(!str || STRLEN(str) == 0)
	{
	    WarningMessage("You must enter a valid data file name.");
	    XtFree(str);
	    return False;
	}
	XtFree(str);

	if(XmToggleButtonGetState(this->grid_tb))
	{
	    str = XmTextGetString(this->grid_text[0]);
	    if(!IsSingleInteger(str))
	    {
		WarningMessage("Error in Grid statement. The grid size of the first dimension must be provided.");
		XtFree(str);
		return False;
	    }
	    XtFree(str);
	    for(i = 1; i < this->dimension; i++)
	    {
		str = XmTextGetString(this->grid_text[i]);
		if(!IsSingleInteger(str))
		{
		    WarningMessage("Error in Grid statement. %d values must be specified for %d-dimensional data", this->dimension, this->dimension);
		    XtFree(str);
		    return False;
		}
		XtFree(str);
	    }
	}
	else
	{
	    str = XmTextGetString(this->points_text);
	    if(!IsSingleInteger(str))
	    {
		WarningMessage("Error in Points statement. The number of points must be specified.");
		XtFree(str);
		return False;
	    }
	    XtFree(str);
	}
    }

    //
    // Per field statements
    //
    if(this->fieldList.getSize() > 0)
    {
	//
	// Layout
	//
	if(XmToggleButtonGetState(this->layout_tb))
	{
	    for(i = 1; i <= this->fieldList.getSize(); i++)
	    {
		field = (Field *)this->fieldList.getElement(i);
		str = field->getLayoutSkip();
		if(!IsSingleInteger(str))
		{
		    WarningMessage("Error in Field Layout (Skip) statement. Entry for field %s must be specified.", field->getName());
		    return False;
		}
		str = field->getLayoutWidth();
		if(!IsSingleInteger(str))
		{
		    WarningMessage("Error in Field Layout (width) statement. Entry for field %s must be specified.", field->getName());
		    return False;
		}
	    }
	}

	//
	// Block
	//
	if(XmToggleButtonGetState(this->block_tb))
	{
	    for(i = 1; i <= this->fieldList.getSize(); i++)
	    {
		field = (Field *)this->fieldList.getElement(i);
		str = field->getBlockSkip();
		if(!IsSingleInteger(str))
		{
		    WarningMessage("Error in Field Block (skip) statement. Entry for field %s must be specified.", field->getName());
		    return False;
		}
		str = field->getBlockElement();
		if(!IsSingleInteger(str))
		{
		    WarningMessage("Error in Field Block (element) statement. Entry for field %s must be specified.", field->getName());
		    return False;
		}
		str = field->getBlockWidth();
		if(!IsSingleInteger(str))
		{
		    WarningMessage("Error in Field Block (width) statement. Entry for field %s must be specified.", field->getName());
		    return False;
		}
	    }
	}
}

    if(XmToggleButtonGetState(this->header_tb))
    {
	XtVaGetValues(this->header_om, XmNmenuHistory, &w, NULL);
	str = XmTextGetString(this->header_text);
	if(EqualString(XtName(w), "string marker"))
	{
	    if(STRLEN(str) == 0)
	    {
		WarningMessage("Error in Header statement. Marker entry is required.");
		XtFree(str);
		return False;
	    }
	    XtFree(str);
	}
	else
	{
	    if(!IsSingleInteger(str))
	    {
		WarningMessage("Error in Header statement. Entry is not an integer.");
		XtFree(str);
		return False;
	    }
	    XtFree(str);
	}
    }

    if(XmToggleButtonGetState(this->series_tb))
    {
	str = XmTextGetString(this->series_n_text);
	if(!IsSingleInteger(str) && !IsAllWhiteSpace(str))
	{
	    WarningMessage("Error in Series n statement. Entry must be specified.");
	    XtFree(str);
	    return False;
	}
	XtFree(str);

	str = XmTextGetString(this->series_start_text);
	if(!IsSingleScalar(str) && !IsAllWhiteSpace(str))
	{
	    WarningMessage("Error in Series start statement. Entry must be specified.");
	    XtFree(str);
	    return False;
	}
	XtFree(str);

	str = XmTextGetString(this->series_delta_text);
	if(!IsSingleScalar(str) && !IsAllWhiteSpace(str))
	{
	    WarningMessage("Error in Series delta statement. Entry must be specified.");
	    XtFree(str);
	    return False;
	}
	XtFree(str);

	if(XmToggleButtonGetState(this->series_sep_tb))
	{
	    XtVaGetValues(this->series_sep_om, XmNmenuHistory, &w, NULL);
	    str = XmTextGetString(this->series_sep_text);
	    if(EqualString(XtName(w), "string marker"))
	    {
		if(STRLEN(str) == 0)
		{
		    WarningMessage("Error in Series Separator statement. Marker entry is required.");
		    XtFree(str);
		    return False;
		}
		XtFree(str);
	    }
	    else
	    {
		if(!IsSingleInteger(str))
		{
		    WarningMessage("Error in Series Separator statement. Entry is not an integer.");
		    XtFree(str);
		    return False;
		}
		XtFree(str);
	    }
	}
    }

    if(XtIsSensitive(this->regularity_om))
    {
        XtVaGetValues(this->regularity_om, XmNmenuHistory, &w, NULL);
        if(EqualString(XtName(w), "Explicit position list"))
        {
           int num_elements = 1;
	   for(i = 0; i < this->dimension; i++)
               num_elements *= atoi(XmTextGetString(this->grid_text[i]));
           num_elements *= this->dimension;
   	   str = XmTextGetString(this->position_text[0]);
           if(!positionListFormat(str, num_elements))
           { 
    	      WarningMessage("Error in Positions statement #%d.  Entry is not in position list format or the number of elements does not match the grid dimension.", i+1);
              XtFree(str);
	      return False;
	   }
        }
        else 
        {
	   for(i = 0; i < this->dimension; i++)
	   {
	       if(XtIsSensitive(this->position_label[i]))
	       {
		   XtVaGetValues(this->positions_om[i], XmNmenuHistory, &w, NULL);
		   str = XmTextGetString(this->position_text[i]);
		   if(EqualString(XtName(w), "regular"))
		   {
		       if(!originDeltaFormat(str))
		       {
		   	   WarningMessage("Error in Positions statement #%d. Entry is not in origin, delta format.", i+1);
			   XtFree(str);
			   return False;
		       }
		       XtFree(str);
		   }
		   else
		   {
		       int num_elements;
		       //
		       // The length of a position list is determined by the dim
		       // in the grid statement or by the number of points.
		       //
		       if(XmToggleButtonGetState(this->grid_tb))
			   num_elements =
			       atoi(XmTextGetString(this->grid_text[i]));
		       else
		 	   num_elements = 
			       atoi(XmTextGetString(this->points_text));

		       if(!positionListFormat(str, num_elements))
		       { 
			   WarningMessage("Error in Positions statement #%d.  Entry is not in position list format or the number of elements does not match the grid dimension.", i+1);
			   XtFree(str);
			   return False;
		       }
		       XtFree(str);
		   }
	       }
	   }
      }
    }
    return True;
}
Boolean GARMainWindow::originDeltaFormat(char *string)
{
    int ndx;

    if(IsAllWhiteSpace(string)) return True;
    ndx = 0;
    if(!IsScalar(string, ndx)) return False;

    //
    // Allow "," or WhiteSpace as the delimiter between numbers.
    //
    if(!IsToken(string, ",", ndx))
    {
	if(!IsWhiteSpace(string, ndx))
	{
	    return False;
	}
	else
	{
	    SkipWhiteSpace(string, ndx);
	}
    }
    if(!IsSingleScalar(&string[ndx])) return False;

    return True;
}
Boolean GARMainWindow::positionListFormat(char *string, int expected_count)
{
    int ndx;
    int actual_count = 0;

    ndx = 0;
    if(!IsScalar(string, ndx)) return False;
    SkipWhiteSpace(string, ndx);
    actual_count++;
    while(ndx != STRLEN(string))
    {
	SkipWhiteSpace(string, ndx);
	IsToken(string, ",", ndx);
	SkipWhiteSpace(string, ndx);
	if(!IsScalar(string, ndx)) return FALSE;
	actual_count++;
    }

    //
    // If the actual element count != the expected count, no good
    //
    if(actual_count != expected_count) 
	return False;
    else
	return True;
}

boolean GARMainWindow::openGAR(char *filenm)
{
    this->setFilename(filenm);

    std::ifstream *from = new std::ifstream(filenm);
    if(!*from)
    {
	WarningMessage("File open failed.");
	return FALSE;
    }

    if(!openGAR(from))
	return FALSE;

    this->saveCmd->activate();

    return TRUE;
}
boolean GARMainWindow::openGAR(std::istream *from)
{
    int i;
    int ndx;
    char line[4096];
    int lineno = 0;
    Field *field;
    char title[256];
    int num_error = 0;
    char *filenm = this->getFilename();

    //
    // Start with a clean slate
    //
    this->newGAR();

    title[0] = '\0';
    strcat(title, "Data Prompter: ");
    if (filenm)
	strcat(title, filenm);
    this->setWindowTitle(title);

    while(from->getline(line, 4096))
    {
	lineno++;
	ndx = 0;
	SkipWhiteSpace(line, ndx);
	if(IsToken(line, "file", ndx))
	{
	    this->parseFile(line);
	}
	else if(IsToken(line, "grid", ndx))
	{
	    this->parseGrid(line);
	}
	else if(IsToken(line, "points", ndx))
	{
	    this->parsePoints(line);
	}
	else if(IsToken(line, "format", ndx))
	{
	    this->parseFormat(line);
	}
	else if(IsToken(line, "field", ndx))
	{
	    this->parseField(line);
	}
	else if(IsToken(line, "structure", ndx))
	{
	    this->parseStructure(line);
	}
	else if(IsToken(line, "type", ndx))
	{
	    this->parseType(line);
	}
	else if(IsToken(line, "interleaving", ndx))
	{
	    this->parseInterleaving(line);
	}
	else if(IsToken(line, "header", ndx))
	{
	    this->parseHeader(line);
	}
	else if(IsToken(line, "series", ndx))
	{
	    this->parseSeries(line);
	}
	else if(IsToken(line, "majority", ndx))
	{
	    this->parseMajority(line);
	}
	else if(IsToken(line, "layout", ndx))
	{
	    this->parseLayout(line);
	}
	else if(IsToken(line, "block", ndx))
	{
	    this->parseBlock(line);
	}
	else if(IsToken(line, "positions", ndx))
	{
	    this->parsePositions(from, line);
	}
	else if(IsToken(line, "dependency", ndx))
	{
	    this->parseDependency(line);
	}
	else if(IsToken(line, "recordseparator", ndx))
	{
	    this->parseRecordSep(line);
	}
	else if(IsToken(line, "#", ndx))
	{
	    this->parseComment(line);
	}
	else if(IsToken(line, "end", ndx))
	{
	    break;
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
		ErrorMessage("Too many warnings, giving up");
		this->newGAR();
		return FALSE;
	    }
	    else
	    {
	     WarningMessage("Parse error opening file \"%s\", line number = %d",
		filenm, lineno);
	    }
	}
	if(this->managed)
	    XmUpdateDisplay(this->getRootWidget());
    }

    if(this->fieldList.getSize() > 0)
    {
	field = (Field *)this->fieldList.getElement(1);
	this->fieldToWids(field);
	for(i = 1; i <= this->fieldList.getSize(); i++)
	{
	    field = (Field *)this->fieldList.getElement(i);
	    XmString xmstr = XmStringCreateSimple(field->getName());
	    XmListAddItem(this->field_list, xmstr, i);
	    if(i == 1)
		XmListSelectItem(this->field_list, xmstr, False);
	    XmStringFree(xmstr);
	}

	XtSetSensitive(this->delete_pb, True);
	XtSetSensitive(this->modify_pb, True);

	int *position_list;
	int position_count;
	XmListGetSelectedPos(this->field_list, &position_list, &position_count);
	if(position_count > 0)
	{
	    this->current_field = position_list[0];
	    XtFree((char *)position_list); 
	}
    }

    this->setPositionsSensitivity();
    this->setBlockLayoutSensitivity();
    this->setRecordSepTBSensitivity();

    this->setVectorInterleavingSensitivity();

    this->setFieldClean();
    theGARApplication->setClean();
    this->setMoveFieldSensitivity();
    this->updateRecordSeparatorList();
    return TRUE;
}
void GARMainWindow::parseFile(char *line)
{
    int ndx = 0;
    while((line[ndx++] != '=') && line[ndx]);
    SkipWhiteSpace(line, ndx);
    //XmTextSetString(this->file_text, &line[ndx]);
    this->setDataFilename(&line[ndx]);
    XmTextShowPosition(this->file_text, STRLEN((char *)&line[ndx]));
}
int GetNumDim(char *line)
{
    int i;
    int ndx = 0;
    int dim;
    char str[4][1024];

    //
    // Convert 123x234x345 to 123 234 345
    //
    for(i = 0; i < STRLEN(line); i++)
	if(line[i] == 'x' || line[i] == 'X')
	    line[i] = ' ';

    while((line[ndx++] != '=') && line[ndx]);
    SkipWhiteSpace(line, ndx);

    dim = sscanf(&line[ndx],"%s %s %s %s", 
	&str[0][0], &str[1][0], &str[2][0], &str[3][0]);

    return dim;
}
void GARMainWindow::parseGrid(char *line)
{
    int i;
    int ndx = 0;
    int dim;
    char str[4][1024];

    //
    // Convert 123x234x345 to 123 234 345
    //
    for(i = 0; i < STRLEN(line); i++)
	if(line[i] == 'x' || line[i] == 'X')
	    line[i] = ' ';

    while((line[ndx++] != '=') && line[ndx]);
    SkipWhiteSpace(line, ndx);

    dim = sscanf(&line[ndx],"%s %s %s %s", 
	&str[0][0], &str[1][0], &str[2][0], &str[3][0]);

    for(i = 0; i < dim; i++)
	XmTextSetString(this->grid_text[i], str[i]);

    XmToggleButtonSetState(this->grid_tb, True, True);

    this->dimension = dim;
    this->setGridSensitivity(True);
}
void GARMainWindow::parsePoints(char *line)
{
    int ndx = 0;
    while((line[ndx++] != '=') && line[ndx]);
    SkipWhiteSpace(line, ndx);

    XmTextSetString(this->points_text, &line[ndx]);
    XmToggleButtonSetState(this->points_tb, True, True);

    this->dimension = 1;
    this->setPointsSensitivity(True);
    
}
void GARMainWindow::parseField(char *line)
{
    Field *field;
    int i;
    int ndx;
    char *name;
    int n = numFields(line);
    int list_size = this->fieldList.getSize();
    if(list_size == 0)
    {
	createDefaultFields(n);
	list_size = n;
    }

    //
    // Advance past the initial stuff "field = "
    //
    ndx = 0;
    while((line[ndx++] != '=') && line[ndx]);
    SkipWhiteSpace(line, ndx);

    for(i = 0; i < list_size; i++)
    {
	field = (Field *)this->fieldList.getElement(i+1);
	name = this->nextItem(line, ndx);
	field->setName(name);
	delete name;
    }
    this->setVectorInterleavingSensitivity();
}
void GARMainWindow::parseFormat(char *line)
{
    int ndx = 0;
    while((line[ndx++] != '=') && line[ndx]);

    SkipWhiteSpace(line, ndx);
    if(IsToken((const char *)line, "msb", ndx))
    {
	SET_OM_NAME(this->format1_om, "Most Significant Byte First", "Format");
    }
    else if(IsToken((const char *)line, "lsb", ndx))
    {
	SET_OM_NAME(this->format1_om, "Least Significant Byte First", "Format");
    }

    SkipWhiteSpace(line, ndx);
    if(IsToken((const char *)line, "ascii", ndx))
    {
	SET_OM_NAME(this->format2_om, "ASCII (Text)", "Format");
    }
    else if(IsToken((const char *)line, "text", ndx))
    {
	SET_OM_NAME(this->format2_om, "ASCII (Text)", "Format");
    }
    else if(IsToken((const char *)line, "binary", ndx))
    {
	SET_OM_NAME(this->format2_om, "Binary (IEEE)", "Format");
	XtSetSensitive(this->format1_om, True);
    }
    else if(IsToken((const char *)line, "ieee", ndx))
    {
	SET_OM_NAME(this->format2_om, "Binary (IEEE)", "Format");
	XtSetSensitive(this->format1_om, True);
    }
}
void GARMainWindow::parseStructure(char *line)
{
    Field *field;
    int i;
    int ndx;
    char *str;
    int n = numFields(line);
    int list_size = this->fieldList.getSize();
    if(list_size == 0)
    {
	createDefaultFields(n);
	list_size = n;
    }

    //
    // Advance past the initial stuff "structure = "
    //
    ndx = 0;
    while((line[ndx++] != '=') && line[ndx]);
    SkipWhiteSpace(line, ndx);

    for(i = 0; i < list_size; i++)
    {
	field = (Field *)this->fieldList.getElement(i+1);
	str = this->nextItem(line, ndx);
	field->setStructure(str);
	delete str;
    }
    this->setVectorInterleavingSensitivity();
}
void GARMainWindow::parseType(char *line)
{
    Field *field;
    int i;
    int ndx;
    char *str;
    char final_str[256];
    int n = numFields(line);
    int list_size = this->fieldList.getSize();
    if(list_size == 0)
    {
	createDefaultFields(n);
	list_size = n;
    }

    //
    // Advance past the initial stuff "type = "
    //
    ndx = 0;
    while((line[ndx++] != '=') && line[ndx]);
    SkipWhiteSpace(line, ndx);

    for(i = 0; i < list_size; i++)
    {
	field = (Field *)this->fieldList.getElement(i+1);
	final_str[0] = '\0';
	str = this->nextItem(line, ndx);
	strcat(final_str, str);
	delete str;
	if(EqualString("signed",    final_str) || 
	   EqualString("unsigned",  final_str))
	{
	    str = this->nextItem(line, ndx);
	    strcat(final_str, " ");
	    strcat(final_str, str);
	    delete str;
	}
	field->setType(final_str);
    }
}
void GARMainWindow::parseInterleaving(char *line)
{
    int ndx = 0;

    while((line[ndx++] != '=') && line[ndx]);

    SkipWhiteSpace(line, ndx);

    if(IsToken(line, "record-vector", ndx))
    {
	SET_OM_NAME(this->field_interleaving_om,  "Block", 
		"Field Interleaving");
	this->setVectorInterleavingOM("w0");
	SET_OM_NAME(this->series_interleaving_om, "w0", "Series Interleaving");
    }
    else if((IsToken(line, "vector", ndx)) || 
	    (IsToken(line, "series-vector", ndx)))
    {
	SET_OM_NAME(this->field_interleaving_om,  "Block", 
		"Field Interleaving");
	this->setVectorInterleavingOM("w0");
	SET_OM_NAME(this->series_interleaving_om, "w1", "Series Interleaving");
    }
    else if(IsToken(line, "record", ndx))
    {
	SET_OM_NAME(this->field_interleaving_om,  "Block", 
		"Field Interleaving");
	this->setVectorInterleavingOM("w1");
	SET_OM_NAME(this->series_interleaving_om, "w0", "Series Interleaving");
    }
    else if(IsToken(line, "field", ndx))
    {
	SET_OM_NAME(this->field_interleaving_om,  "Columnar", 
		"Field Interleaving");
    }
    this->setVectorInterleavingSensitivity();
    this->setSeriesInterleavingSensitivity();

}
void GARMainWindow::parseHeader(char *line)
{
    int ndx = 0;
    while((line[ndx++] != '=') && line[ndx]);

    SkipWhiteSpace(line, ndx);

    if(IsToken(line, "bytes", ndx))
    {
	SET_OM_NAME(this->header_om, "# of bytes", "Header");
    }
    else if(IsToken(line, "lines", ndx))
    {
	SET_OM_NAME(this->header_om, "# of lines", "Header");
    }
    else if(IsToken(line, "marker", ndx))
    {
	SET_OM_NAME(this->header_om, "string marker", "Header");
    }
    this->changeHeaderType();

    SkipWhiteSpace(line, ndx);
    char *str = DuplicateString(&line[ndx]);
    XmTextSetString(this->header_text, str);
    delete str;
    
    XmToggleButtonSetState(this->header_tb, True, True);
    this->setHeaderSensitivity(True);
}
void GARMainWindow::parseSeries(char *line)
{
    char *str;
    int ndx = 0;
    int tmpndx;
    int n = numFields(line);

    //
    // Must have at least "n" specified.
    // If "start" is specified, "delta" must be also.
    //
    if( (n < 1) || (n == 2) )
    {
	WarningMessage("Parse error: series statement");
	return;
    }

    while((line[ndx++] != '=') && line[ndx]);
    SkipWhiteSpace(line, ndx);

    str = nextItem(line, ndx);
    tmpndx = 0;
    if(!IsInteger(str, tmpndx))
    {
	WarningMessage("Parse error: series statement");
	return;
    }
    XmTextSetString(this->series_n_text, str);
    delete str;

    if(n > 1)
    {
	SkipWhiteSpace(line, ndx);
	str = nextItem(line, ndx);
	tmpndx = 0;
	if(IsInteger(str, tmpndx))
	{
	    XmTextSetString(this->series_start_text, str);
	    delete str;

	    str = nextItem(line, ndx);
	    tmpndx = 0;
	    if(!IsInteger(str, tmpndx))
	    {
		WarningMessage("Parse error: series statement");
		return;
	    }
	    XmTextSetString(this->series_delta_text, str);
	    delete str;

	    SkipWhiteSpace(line, ndx);
	    str = nextItem(line, ndx);
	}

	tmpndx = 0;
	if(IsToken(str, "separator", tmpndx) ||
	   IsToken(str, "header", tmpndx))
	{
	    while((line[ndx++] != '=') && line[ndx]);
	    SkipWhiteSpace(line, ndx);

	    if(IsToken(line, "bytes", ndx))
	    {
		SET_OM_NAME(this->series_sep_om, "# of bytes", "Series Separator");
	    }
	    else if(IsToken(line, "lines", ndx))
	    {
		SET_OM_NAME(this->series_sep_om, "# of lines", "Series Separator");
	    }
	    else if(IsToken(line, "marker", ndx))
	    {
		SET_OM_NAME(this->series_sep_om, "string marker", "Series Separator");
	    }
	    this->changeHeaderType();

	    SkipWhiteSpace(line, ndx);
	    str = DuplicateString(&line[ndx]);
	    XmTextSetString(this->series_sep_text, str);
	    delete str;
	    
	    XmToggleButtonSetState(this->series_sep_tb, True, True);
	}
    }
    XmToggleButtonSetState(this->series_tb, True, True);

    this->setSeriesSensitivity(True);
    this->setSeriesSepSensitivity();
}
void GARMainWindow::parseMajority(char *line)
{
    int ndx = 0;
    while((line[ndx++] != '=') && line[ndx]);

    SkipWhiteSpace(line, ndx);

    if(IsToken(line, "row", ndx))
    {
	XmToggleButtonSetState(this->row_tb, True, False);
	XmToggleButtonSetState(this->col_tb, False, False);
	this->restrictStructure(False);
    }
    else if(IsToken(line, "column", ndx))
    {
	XmToggleButtonSetState(this->col_tb, True, False);
	XmToggleButtonSetState(this->row_tb, False, False);
	this->restrictStructure(True);
    }
}
void GARMainWindow::parseLayout(char *line)
{
    Field *field;
    int i;
    int ndx;
    char *str;
    int n = numFields(line)/2;
    int list_size = this->fieldList.getSize();
    if(list_size == 0)
    {
	createDefaultFields(n);
	list_size = n;
    }

    //
    // Advance past the initial stuff "layout = "
    //
    ndx = 0;
    while((line[ndx++] != '=') && line[ndx]);
    SkipWhiteSpace(line, ndx);

    for(i = 0; i < list_size; i++)
    {
	field = (Field *)this->fieldList.getElement(i+1);
	str = this->nextItem(line, ndx);
	field->setLayoutSkip(str);
	delete str;

	str = this->nextItem(line, ndx);
	field->setLayoutWidth(str);
	delete str;
    }
    XmToggleButtonSetState(this->layout_tb, True, False);
    this->layout_was_on = True;
}
void GARMainWindow::parseBlock(char *line)
{
    Field *field;
    int i;
    int ndx;
    char *str;
    int n = numFields(line)/3;
    int list_size = this->fieldList.getSize();
    if(list_size == 0)
    {
	createDefaultFields(n);
	list_size = n;
    }

    //
    // Advance past the initial stuff "block = "
    //
    ndx = 0;
    while((line[ndx++] != '=') && line[ndx]);
    SkipWhiteSpace(line, ndx);

    for(i = 0; i < list_size; i++)
    {
	field = (Field *)this->fieldList.getElement(i+1);

	str = this->nextItem(line, ndx);
	field->setBlockSkip(str);
	delete str;

	str = this->nextItem(line, ndx);
	field->setBlockElement(str);
	delete str;

	str = this->nextItem(line, ndx);
	field->setBlockWidth(str);
	delete str;
    }
    XmToggleButtonSetState(this->block_tb, True, False);
    this->block_was_on = True;
}
void GARMainWindow::parsePositions(std::istream *from, char *line)
{
    char *str;
    char *tmpstr;
    char *posstr;
    int i;
    int j;
    int maxnum = 1;
    int ndx;
    int tmpndx;
    int num_elements;
    int cur_size;
    Boolean get_rest = False;
    Boolean regular = False;
    Boolean explicitly = False;
    Widget w;

    //
    // Advance past the initial stuff "positions = "
    //
    ndx = 0;
    while((line[ndx++] != '=') && line[ndx]);
    SkipWhiteSpace(line, ndx);

    cur_size = STRLEN(line);
    str = (char *)CALLOC(cur_size, sizeof(char));

    strcat(str, &line[ndx]);

    while(from->getline(line, 4096))
    {
	ndx = 0;
	SkipWhiteSpace(line, ndx);
	if(IsToken(line, "end", ndx))
	{
	    break;
	}
	else
	{
	    cur_size += STRLEN(line)+1;
	    str = (char *)REALLOC(str, cur_size);
	    strcat(str, line);
	}
    }

    //
    // Now we have one continuous string to process.
    ndx = 0;
    SkipWhiteSpace(str, ndx);
    if(IsToken(str, "regular", ndx))
    {
	SET_OM_NAME(this->positions_om[0], "regular", "Positions");
        this->changePositions(this->positions_om[0]);
	regular = True;
	get_rest = True;
    }
    else if(IsToken(str, "irregular", ndx))
    {
	SET_OM_NAME(this->positions_om[0], "irregular", "Positions");
        this->changePositions(this->positions_om[0]);
	/*irregular = True;*/
	get_rest = True;
    }
    if(get_rest)
    {
	SkipWhiteSpace(str, ndx);
	if(str[ndx] == ',') ndx++;
	SkipWhiteSpace(str, ndx);
	for(i = 1; i < this->dimension; i++)
	{
	    tmpstr = nextItem(str, ndx);
	    tmpndx = 0;
	    SkipWhiteSpace(str, ndx);
	    if(IsToken(tmpstr, "regular", tmpndx))
	    {
		SET_OM_NAME(this->positions_om[i], "regular", "Positions");
                this->changePositions(this->positions_om[i]);
		regular = True;
	    }
	    else if(IsToken(tmpstr, "irregular", tmpndx))
	    {
		SET_OM_NAME(this->positions_om[i], "irregular", "Positions");
                this->changePositions(this->positions_om[i]);
		/*irregular = True;*/
	    }
	    else
	    {
		WarningMessage("Parse error in positions statement.  Inconsistent dimension.");
	    }
	}

	if(regular)
	{
            Widget wid;
	    SET_OM_NAME(this->regularity_om, "Completely regular", 
		"Regularity");
            XtVaGetValues(this->regularity_om, XmNmenuHistory, &wid, NULL);
            this->changeRegularity(wid); 
	}
        else
	{
            Widget wid;
	    SET_OM_NAME(this->regularity_om, "Partially regular", 
		"Regularity");
            XtVaGetValues(this->regularity_om, XmNmenuHistory, &wid, NULL);
            this->changeRegularity(wid); 
	}
    }

    /* no keywords on line: either regular or explicit */
    else {
       //
       // Now, get the positions
       //
       /* here's where I need to check the number of positions. If more than
          2*numdim, then it's explicit positions  */
       SkipWhiteSpace(str, ndx);

       /* figure out how many there are */
       /* max number is grid*grid*grid*dim */
       for (i=0; i<this->dimension; i++)
       {
          tmpstr = XmTextGetString(this->grid_text[i]);
          num_elements = atoi(tmpstr);
          delete tmpstr;
          maxnum *= num_elements;
       }
       maxnum *= this->dimension;

       for (i = 0; i < maxnum; i++)
       {
          tmpstr = nextItem(str, ndx); 
	  if ((!tmpstr) || (!tmpstr[0])) break;
       }
       ndx = 0;
       if (i==maxnum)
       {
	  Widget wid;
          SET_OM_NAME(this->regularity_om, "Explicit position list", 
	   	     "Regularity");
          XtVaGetValues(this->regularity_om, XmNmenuHistory, &wid, NULL);
          this->changeRegularity(wid); 
          /*irregular = True;*/
          explicitly = True;
       }
       else
       {
	  Widget wid;
          SET_OM_NAME(this->regularity_om, "Completely regular", 
	   	     "Regularity");
          XtVaGetValues(this->regularity_om, XmNmenuHistory, &wid, NULL);
          this->changeRegularity(wid); 
          /*irregular = False;*/
          explicitly = False;
       }
    }

    if (!explicitly) {
       for(i = 0; i < this->dimension; i++)
       {
	   XtVaGetValues(this->positions_om[i], XmNmenuHistory, &w, NULL);
	   if(EqualString(XtName(w), "regular"))
	       num_elements = 2;
	   else
	   {
	       tmpstr = XmTextGetString(this->grid_text[i]);
	       num_elements = atoi(tmpstr);
	       delete tmpstr;
           }
	   posstr = (char *)CALLOC(15*num_elements, sizeof(char));
	   for(j = 0; j < num_elements; j++)
	   {
	       tmpstr = nextItem(str, ndx);
	       strcat(posstr, tmpstr);
	       if(j < num_elements-1)
	    	   strcat(posstr, ", ");
	       delete tmpstr;
	   }
	   XmTextSetString(this->position_text[i], posstr);
	   delete posstr;
       }
    }
    else
    {
       XtVaGetValues(this->positions_om[0], XmNmenuHistory, &w, NULL);
       num_elements = maxnum;
       posstr = (char *)CALLOC(15*num_elements, sizeof(char));
       for(j = 0; j < num_elements; j++)
       {
           tmpstr = nextItem(str, ndx);
           strcat(posstr, tmpstr);
           if(j < num_elements-1)
   	      strcat(posstr, ", ");
	   delete tmpstr;
       }
       XmTextSetString(this->position_text[0], posstr);
       delete posstr;
    }
}
void GARMainWindow::parseDependency(char *line)
{
    Field *field;
    int i;
    int ndx;
    char *str;
    int n = numFields(line);
    int list_size = this->fieldList.getSize();
    if(list_size == 0)
    {
	createDefaultFields(n);
	list_size = n;
    }

    //
    // Advance past the initial stuff "depend = "
    //
    ndx = 0;
    while((line[ndx++] != '=') && line[ndx]);
    SkipWhiteSpace(line, ndx);

    for(i = 0; i < list_size; i++)
    {
	field = (Field *)this->fieldList.getElement(i+1);

	str = this->nextItem(line, ndx);
	field->setDependency(str);
	delete str;
    }
}
void GARMainWindow::parseRecordSep(char *line)
{
    int k;
    int ndx;
    char *str, *str1;
    int counter=0;

    this->createRecordSeparatorListStructure();

    XmToggleButtonSetState(this->record_sep_tb, True, False);

    //
    // Advance past the initial stuff "recordsep = " or "recordheader = "
    //
    ndx = 0;
    while((line[ndx++] != '=') && line[ndx]);
    SkipWhiteSpace(line, ndx);

    //
    // get the first recordseparator pair
    //
    str = this->nextItem(line, ndx);
    if (STRLEN(str) <= 0)
       WarningMessage("Parse warning: recseparator statement!");
       str = this->nextItem(line, ndx);
       if (STRLEN(str) <= 0)
           WarningMessage("Parse warning: recseparator statement!");
       counter++;

    //
    // count how many items
    //
    str = this->nextItem(line, ndx);
    while (STRLEN(str) > 0)
    {
       str = this->nextItem(line, ndx);
       if (STRLEN(str) <= 0)
           WarningMessage("Parse warning: recseparator statement!");
       counter++;
       str = this->nextItem(line, ndx);
    }

    // reset
    ndx = 0;
    while((line[ndx++] != '=') && line[ndx]);
    SkipWhiteSpace(line, ndx);
       
    // a single recordseparator 
    if (counter==1)
    {
        XmToggleButtonSetState(this->same_sep_tb, True, True);
        RecordSeparator *recsep = 
             (RecordSeparator *)this->recordsepSingle.getElement(1);
        if (!recsep)
        {
           WarningMessage("Parse warning: inconsistent number of entries in recseparator statement!");
           return;
        }
        str = this->nextItem(line, ndx);
        if (STRLEN(str) <=0)
           WarningMessage("Parse warning: inconsistent number of entries in recseparator statement!");

        if(EqualString(str, "bytes"))
            str1 = "# of bytes";
        else if(EqualString(str, "lines"))
            str1 = "# of lines";
        else if(EqualString(str, "marker"))
            str1 = "string marker";
        else
            WarningMessage("Parse warning: invalid recseparator statement!");
        recsep->setType(str1);
        SET_OM_NAME(this->record_sep_om, str1, "record separator"); 
        this->changeHeaderType();

        str = this->nextItem(line, ndx);
        if (STRLEN(str) <=0)
           WarningMessage("Parse warning: inconsistent number of entries in recseparator statement!");
        if(EqualString(str1, "string marker"))
        {
          STRIP_QUOTES(str);
        }
        recsep->setNum(str);
        XmTextSetString(this->record_sep_text, str);
    }
    else
    {
      XmToggleButtonSetState(this->diff_sep_tb, True, True);
      for (k=1; k<=counter; k++)
      { 
        RecordSeparator *recsep = 
          (RecordSeparator *)this->recordsepList.getElement(k);
        if (!recsep)
        {
           WarningMessage("Parse warning: inconsistent number of entries in recseparator statement!");
            return;
        }
        str = this->nextItem(line, ndx);
        if (STRLEN(str) <=0)
           WarningMessage("Parse warning: inconsistent number of entries in recseparator statement!");

        if(EqualString(str, "bytes"))
            str1 = "# of bytes";
        else if(EqualString(str, "lines"))
            str1 = "# of lines";
        else
            str1 = "string marker";
        recsep->setType(str1);
        SET_OM_NAME(this->record_sep_om_1, str1, "record separator"); 
        this->changeHeaderType();

        str = this->nextItem(line, ndx);
        if (STRLEN(str) <=0)
           WarningMessage("Parse warning: inconsistent number of entries in recseparator statement!");
        recsep->setNum(str);
        XmTextSetString(this->record_sep_text_1, str);
     }
    }
    this->record_sep_was_on = True;
}
void GARMainWindow::parseComment(char *line)
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

    //
    // managing a TextEditDialog installs the new text...
    //
    if(this->comment_dialog->isManaged())
	this->comment_dialog->manage();
}
void GARMainWindow::createDefaultFields(int num_fields)
{

    int i;
    Field *field;
    char name[32];

    for(i = 0; i < num_fields; i++)
    {
	
	sprintf(name, "field%d", i+1);
	field = new Field(name);
	this->fieldList.appendElement(field);
   
	//
	// Type
	//
	field->setType("double");

	//
	// Structure
	//
	field->setStructure("scalar");

	field->setLayoutSkip("");
	field->setLayoutWidth("");
	field->setBlockSkip("");
	field->setBlockElement("");
	field->setBlockWidth("");

	//
	// Dependency
	//
	field->setDependency("positions");
    }
}
int GARMainWindow::numFields(char *line)
{
    int ndx = 0;
    int num_fields = 0;
    while((line[ndx++] != '=') && line[ndx]);

    SkipWhiteSpace(line, ndx);

    while(line[ndx])
    {
	if(IsWhiteSpace(line, ndx) || (line[ndx] == ','))
	{
	    ndx++;
	    num_fields++;
	    SkipWhiteSpace(line, ndx);
	    if(!line[ndx]) num_fields--;
	}
	else if(line[ndx] == '"')
	{
	    //
	    // "Inner loop" to eat up a quoted string w/ white space in it.
	    //
	    ndx++;
	    while(line[ndx] && (line[ndx] != '"'))
		ndx++;
	    ndx++;
	    while(IsWhiteSpace(line, ndx) || (line[ndx] == ',')) ndx++;
	    num_fields++;
	    if(!line[ndx]) num_fields--;
	}
	else
	{
	    ndx++;
	}
    }
    return ++num_fields;
}
char *GARMainWindow::nextItem(char *line, int &ndx)
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
	while(!IsWhiteSpace(line, ndx) && !(line[ndx] == ',') && line[ndx])
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

Boolean GARMainWindow::IsSingleInteger(char *string)
{
    int ndx;
    Boolean result = True;

    ndx = 0;
    if(!IsInteger(string, ndx))
    {
	result = False;
    }
    else
    {
	SkipWhiteSpace(string, ndx);
	if(STRLEN(string) != ndx) 
	    result = False;
    }
    return result;
}

Boolean GARMainWindow::IsSingleScalar(char *string)
{
    int ndx;
    Boolean result = True;;

    ndx = 0;
    if(!IsScalar(string, ndx))
    {
	result = False;
    }
    else 
    {
	SkipWhiteSpace(string, ndx);
	if(STRLEN(string) != ndx) 
	    result = False;
    }
    return result;
}
void GARMainWindow::PostSaveAsDialog(GARMainWindow *gmw, Command *cmd)
{
    gmw->save_as_fd->setPostCommand(cmd);
    gmw->save_as_fd->post();
}
void GARMainWindow::postOpenFileDialog()
{
    this->open_fd->post();
}
void GARMainWindow::postBrowser(char *str)
{

    char *file_name = theGARApplication->resolvePathName(str, "");
    if ((file_name) && (file_name[0]) && (this->browser->openFile(file_name))) 
	this->browser->manage();
    else
	WarningMessage("Could not open file: %s", str);

    if (file_name) delete file_name;
}

void GARMainWindow::unpostBrowser()
{
    this->browser->unmanage();
}
void GARMainWindow::setCommentText(char *s)
{

    if(this->comment_text)
	delete this->comment_text;
    this->comment_text = DuplicateString(s);
}
void GARMainWindow::setFieldDirty()
{
    int			*position_list;
    int			position_count;
    XmString		xmstr;

    //
    // If an item was selected, then inform the user he has to "Modify" the
    // field, else he must "Add" the field
    //
    if(!XmListGetSelectedPos(this->field_list, &position_list, &position_count))
    {
	xmstr = 
	    XmStringCreateLtoR("A field parameter has changed.\nAdd the field by pressing the Add or Insert button.", XmSTRING_DEFAULT_CHARSET); 
    }
    else
    {
	xmstr = 
	    XmStringCreateLtoR("A field parameter has changed.\nUpdate the field by pressing the Modify button.", XmSTRING_DEFAULT_CHARSET); 
    }

    XmToggleButtonSetState(this->modify_tb, True, False);
    XtVaSetValues(this->modify_tb, XmNlabelString, xmstr, NULL);

    XmStringFree(xmstr);

}
void GARMainWindow::setFieldClean()
{
    XmToggleButtonSetState(this->modify_tb, False, False);
    XmString xmstr = XmStringCreateSimple(" ");
    XtVaSetValues(this->modify_tb, XmNlabelString, xmstr, NULL);
    XmStringFree(xmstr);
}
void GARMainWindow::CreateElipsesPopup()
{
    char   *name[2];
    Widget wid;
    XmString xms;

    name[0] = "File Selection Dialog...";
    name[1] = "Browser...";

    //
    // Get rid of any existing tranlstions on the label
    //
    XtUninstallTranslations(this->elip_pb);

    this->elip_popup_menu = 
	XmCreatePopupMenu(this->elip_pb, "popup", NULL, 0);
    XtVaSetValues(this->elip_popup_menu, XmNwhichButton, 1, NULL);
    xms = XmStringCreateSimple(name[0]);
    wid = XtVaCreateManagedWidget
	   (name[0], 
	    xmPushButtonWidgetClass,
	    this->elip_popup_menu, 
	    XmNlabelString, xms,
	    NULL);
    XmStringFree(xms);

    XtAddCallback(wid,
	  XmNactivateCallback,
	  (XtCallbackProc)GARMainWindow_FileSelectCB,
	  (XtPointer)this);

    xms = XmStringCreateSimple(name[1]);
    this->browser_pb = XtVaCreateManagedWidget
	   (name[1], 
	    xmPushButtonWidgetClass,
	    this->elip_popup_menu, 
	    XmNlabelString, xms,
	    NULL);
    XmStringFree(xms);

    XtAddCallback(this->browser_pb,
	  XmNactivateCallback,
	  (XtCallbackProc)GARMainWindow_PostBrowserCB,
	  (XtPointer)this);

    XtAddEventHandler(this->elip_pb, ButtonPressMask, False, 
	(XtEventHandler)GARMainWindow_PostElipPopupEH, (XtPointer)this);
}
extern "C" void GARMainWindow_PostElipPopupEH(Widget , XtPointer client, 
					XEvent *e, Boolean *)
{
    GARMainWindow *gmw = (GARMainWindow *)client;

    char *str = XmTextGetString(gmw->file_text);
    XtSetSensitive(gmw->browser_pb, (STRLEN(str) > 0));
    if(str) delete str;
	
    XmMenuPosition(gmw->elip_popup_menu, (XButtonPressedEvent *)e);
    XtManageChild(gmw->elip_popup_menu);
}
extern "C" void GARMainWindow_PostBrowserCB(Widget , XtPointer client, XtPointer)
{
    GARMainWindow *gmw = (GARMainWindow *)client;

    char *str = XmTextGetString(gmw->file_text);
    gmw->postBrowser(str);
    delete str;
}
extern "C" void GARMainWindow_StructureInsensCB(Widget , XtPointer client, XtPointer)
{
    GARMainWindow *gmw = (GARMainWindow *)client;
    XtSetSensitive (gmw->string_size, True);
    XtSetSensitive (gmw->string_size_label, True);
    XtSetSensitive (gmw->structure_om, False);
}
extern "C" void GARMainWindow_StructureSensCB(Widget , XtPointer client, XtPointer)
{
    GARMainWindow *gmw = (GARMainWindow *)client;
    XtSetSensitive (gmw->string_size, False);
    XtSetSensitive (gmw->string_size_label, False);
    XtSetSensitive (gmw->structure_om, True);
}

extern "C" void GARMainWindow_DirtyFileCB(Widget , XtPointer , XtPointer)
{
    theGARApplication->setDirty();
}

void GARMainWindow::setDataFilename (const char *filenm)
{
    if ((!filenm) || (!filenm[0]))
	XmTextSetString (this->file_text, "");
    else {
	char *tmpstr = DuplicateString(filenm);
	XmTextSetString (this->file_text, tmpstr);
	delete tmpstr;
    }
    GARChooserWindow *gcw = theGARApplication->getChooserWindow();
    if (gcw) gcw->fileNameCB();
}

boolean GARMainWindow::isSeries()
{
    return (boolean)XmToggleButtonGetState (this->series_tb);
}

boolean GARMainWindow::isGridded()
{
    return (boolean)XmToggleButtonGetState (this->grid_tb);
}

int GARMainWindow::getDimensionality()
{
    int dimensionality = 0;

    if (this->isGridded()) {
	//
	// just check for entries in grid_text[1..4]
	//
	char *cp = XmTextGetString (this->grid_text[0]);
	if ((cp) && (cp[0])) dimensionality++;
	if (cp) XtFree(cp);

	cp = XmTextGetString (this->grid_text[1]);
	if ((cp) && (cp[0])) dimensionality++;
	if (cp) XtFree(cp);

	cp = XmTextGetString (this->grid_text[2]);
	if ((cp) && (cp[0])) dimensionality++;
	if (cp) XtFree(cp);

	cp = XmTextGetString (this->grid_text[3]);
	if ((cp) && (cp[0])) dimensionality++;
	if (cp) XtFree(cp);
    } else {

	const char* name[10];
	name[0] = "scalar";  name[1] = "1-vector";name[2] = "2-vector";
	name[3] = "3-vector";name[4] = "4-vector";name[5] = "5-vector";
	name[6] = "6-vector";name[7] = "7-vector";name[8] = "8-vector";
	name[9] = "9-vector"; 

	Widget w;
	XtVaGetValues (this->structure_om, XmNmenuHistory, &w, NULL);
	const char* wName = XtName(w);
	int i;
	if (EqualString(wName, name[0])) {
	    dimensionality = this->fieldList.getSize();
	} else {
	    for (i=1; i<10; i++) {
		if (EqualString(wName, name[i])) {
		    dimensionality = i;
		    break;
		}
	    }
	}
    }

    return dimensionality;
}

