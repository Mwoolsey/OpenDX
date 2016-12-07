/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#include <ctype.h>
#include <Xm/Xm.h>
#include <Xm/MainW.h>
#include <Xm/Text.h>
#include <Xm/ScrolledW.h>
#include <Xm/Form.h>
#include <Xm/PushB.h>
#include <Xm/Separator.h>
#include <Xm/ScrollBar.h>
#include <Xm/RowColumn.h>
#include <Xm/CascadeB.h>
#include <Xm/Label.h>

#include "Browser.h"
#include "BrowserCommand.h"
#include "MainWindow.h"
#include "IBMApplication.h"
#include "../base/WarningDialogManager.h"
#include "../base/ButtonInterface.h"
#include "../base/DXStrings.h"

#include <sys/stat.h>

#if HAVE_REGCOMP && HAVE_REGEX_H
extern "C" {
#include <regex.h>
}
#undef HAVE_RE_COMP
#undef HAVE_FINDFIRST
#elif  defined(HAVE_RE_COMP)
#undef HAVE_REGCMP
#undef HAVE_REGCOMP
#undef HAVE_FINDFIRST
extern "C" char *re_comp(char *s);
extern "C" int re_exec(char *);
#elif defined(HAVE_REGCMP)
#undef HAVE_REGCOMP
#undef HAVE_FINDFIRST
extern "C" char *regcmp(...);
extern "C" char *regex(char *, char *, ...);
#elif HAVE_REGCOMP && HAVE_REGEXP_H
extern "C" {
#include <regexp.h>
}
#undef HAVE_RE_COMP
#undef HAVE_FINDFIRST
#elif  defined(HAVE_RE_COMP)
#undef HAVE_FINDFIRST
extern "C" char *re_comp(char *s);
extern "C" int re_exec(char *);
#elif  defined(HAVE_FINDFIRST)
#include <mingw32/dir.h>
#endif

boolean Browser::ClassInitialized = FALSE;

String Browser::DefaultResources[] =
{
    "*Btext.fontList:		-misc-fixed-medium-r-normal--13*",
    ".iconName:                                   Browser",
    ".title:                                      File Browser",
    ".geometry:					  +435+105",
    "*XmScrollBar.initialDelay:                   250",
    "*XmScrollBar.repeatDelay:                    50",
    "*browFileMenu.labelString:                   File",
    "*browFileMenu.mnemonic:                      F",
    "*browCloseOption.labelString:                Close",
    "*browCloseOption.mnemonic:                   C",

    "*browMarkMenu.labelString:                   Mark",
    "*browMarkMenu.mnemonic:                      M",
    "*browPlaceMarkOption.labelString:            Place mark",
    "*browPlaceMarkOption.mnemonic:               P",
    "*browClearMarkOption.labelString:            Clear mark",
    "*browClearMarkOption.mnemonic:               P",
    "*browGotoMarkOption.labelString:             Goto mark",
    "*browGotoMarkOption.mnemonic:                P",

    "*browPageMenu.labelString:                   Page",
    "*browPageMenu.mnemonic:                      P",
    "*browFirstPageOption.labelString:            First page",
    "*browFirstPageOption.mnemonic:               F",
    "*browLastPageOption.labelString:             Last page",
    "*browLastPageOption.mnemonic:                L",

    "*browSearchMenu.labelString:                 Search",
    "*browSearchMenu.mnemonic:                    S",
    "*browSearchOption.labelString:               Search...",
    "*browSearchOption.mnemonic:                  S",
    "*browByteOffsets.labelString:                Byte Offsets:",
    "*browLineOffsets.labelString:                Line Offsets:",
    "*browFromMark.labelString:                   From mark:",
    "*browFromTopOfFile.labelString:              From top of file:",
    "*browFromStartOfLine.labelString:            From start of line:",

    NULL
};


Browser::Browser(MainWindow *) : IBMMainWindow("GARBrowser", TRUE)
{
    //
    // Initialize member data.
    //

    this->page_size = 1024*20;
    this->ignore_cb = False;
    this->marker_char[0] = '\0';
    this->marker_char[1] = '\0';

    this->closeOption = NUL(CommandInterface*);
    this->placeMarkOption = NUL(CommandInterface*);
    this->clearMarkOption = NUL(CommandInterface*);
    this->gotoMarkOption = NUL(CommandInterface*);
    this->firstPageOption = NUL(CommandInterface*);
    this->lastPageOption = NUL(CommandInterface*);
    this->searchOption = NUL(CommandInterface*);

    this->placeMarkCmd =
        new BrowserCommand("placeMark",this->commandScope,
                TRUE, this, this->PLACE_MARK);
    this->clearMarkCmd =
        new BrowserCommand("clearMark",this->commandScope,
                TRUE, this, this->CLEAR_MARK);
    this->gotoMarkCmd =
        new BrowserCommand("gotoMark",this->commandScope,
                TRUE, this, this->GOTO_MARK);
    this->firstPageCmd =
        new BrowserCommand("firstPage",this->commandScope,
                TRUE, this, this->FIRST_PAGE);
    this->lastPageCmd =
        new BrowserCommand("lastPage",this->commandScope,
                TRUE, this, this->LAST_PAGE);
    this->searchCmd =
        new BrowserCommand("search",this->commandScope,
                TRUE, this, this->SEARCH);
    this->closeCmd =
        new BrowserCommand("close",this->commandScope,
                TRUE, this, this->CLOSE);

    this->searchDialog = (SearchDialog*)NULL;


    //
    // Install the default resources for THIS class (not the derived classes)
    //
    if (NOT Browser::ClassInitialized)
    {
	ASSERT(theApplication);
        Browser::ClassInitialized = TRUE;
	this->installDefaultResources(theApplication->getRootWidget());
    }

}

Browser::~Browser()
{
    if (this->searchDialog)   delete this->searchDialog;
    if (this->searchOption)   delete this->searchOption;
    if (this->lastPageOption) delete this->lastPageOption;
    if (this->firstPageOption)delete this->firstPageOption;
    if (this->gotoMarkOption) delete this->gotoMarkOption;
    if (this->clearMarkOption)delete this->clearMarkOption;
    if (this->placeMarkOption)delete this->placeMarkOption;
    if (this->closeOption)    delete this->closeOption;

    delete this->closeCmd;
    delete this->searchCmd;
    delete this->lastPageCmd;
    delete this->firstPageCmd;
    delete this->gotoMarkCmd;
    delete this->clearMarkCmd;
    delete this->placeMarkCmd;

}

//
// called by MainWindow::CloseCallback().
//
void Browser::closeWindow()
{
    this->closeCmd->execute();
}

void Browser::createMenus(Widget parent)
{
    this->createFileMenu(parent);
    this->createMarkMenu(parent);
    this->createPageMenu(parent);
    this->createSearchMenu(parent);
    this->createHelpMenu(parent);

    Browser::AddHelpCallbacks(this->fileMenu);
    Browser::AddHelpCallbacks(this->markMenu);
    Browser::AddHelpCallbacks(this->pageMenu);
    Browser::AddHelpCallbacks(this->searchMenu);
    Browser::AddHelpCallbacks(this->helpMenu);
}
void Browser::createFileMenu(Widget parent)
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
            ("browFileMenu",
             xmCascadeButtonWidgetClass,
             parent,
             XmNsubMenuId, pulldown,
             NULL);

    this->closeOption =
        new ButtonInterface(pulldown,
                            "browCloseOption",
                            this->closeCmd);
    Browser::AddHelpCallbacks(pulldown);
}
void Browser::createMarkMenu(Widget parent)
{
    ASSERT(parent);

    Widget pulldown;

    //
    // Create "Mark" menu and options.
    //
    pulldown =
        this->fileMenuPulldown =
            XmCreatePulldownMenu(parent, "browMarkMenuPulldown",NUL(ArgList),0);

    this->markMenu =
        XtVaCreateManagedWidget
            ("browMarkMenu",
             xmCascadeButtonWidgetClass,
             parent,
             XmNsubMenuId, pulldown,
             NULL);

    this->placeMarkOption =
        new ButtonInterface(pulldown,
                            "browPlaceMarkOption",
                            this->placeMarkCmd);
    this->clearMarkOption =
        new ButtonInterface(pulldown,
                            "browClearMarkOption",
                            this->clearMarkCmd);
    this->gotoMarkOption =
        new ButtonInterface(pulldown,
                            "browGotoMarkOption",
                            this->gotoMarkCmd);

    Browser::AddHelpCallbacks(pulldown);
}
void Browser::createPageMenu(Widget parent)
{
    ASSERT(parent);

    Widget pulldown;

    //
    // Create "File" menu and options.
    //
    pulldown =
        this->pageMenuPulldown =
            XmCreatePulldownMenu(parent, "browPageMenuPulldown",NUL(ArgList),0);

    this->pageMenu =
        XtVaCreateManagedWidget
            ("browPageMenu",
             xmCascadeButtonWidgetClass,
             parent,
             XmNsubMenuId, pulldown,
             NULL);

    this->firstPageOption =
        new ButtonInterface(pulldown,
                            "browFirstPageOption",
                            this->firstPageCmd);
    this->lastPageOption =
        new ButtonInterface(pulldown,
                            "browLastPageOption",
                            this->lastPageCmd);

    Browser::AddHelpCallbacks(pulldown);
}
void Browser::createSearchMenu(Widget parent)
{
    ASSERT(parent);

    Widget pulldown;

    //
    // Create "Search" menu and options.
    //
    pulldown =
        this->searchMenuPulldown =
            XmCreatePulldownMenu(parent, "browSearchMenuPulldown", 
		NUL(ArgList), 0);

    this->searchMenu =
        XtVaCreateManagedWidget
            ("browSearchMenu",
             xmCascadeButtonWidgetClass,
             parent,
             XmNsubMenuId, pulldown,
             NULL);

    this->searchOption =
        new ButtonInterface(pulldown,
                            "browSearchOption",
                            this->searchCmd);

    Browser::AddHelpCallbacks(pulldown);
}
void Browser::createHelpMenu(Widget parent)
{
    this->createBaseHelpMenu(parent);
}

//
// Install the default resources for this class.
//
void Browser::installDefaultResources(Widget baseWidget)
{
    this->setDefaultResources(baseWidget, Browser::DefaultResources);
    this->IBMMainWindow::installDefaultResources(baseWidget);
}
void Browser::initialize()
{

    if (!this->isInitialized()) {
	//
	// Now, call the superclass initialize().
	//
	this->IBMMainWindow::initialize();
    }
}

Widget Browser::createWorkArea(Widget parent)
{
    Widget form1;
    Widget hsb;
    Widget label;
    Pixel bg;
    Arg wargs[50];
    int n;

    form1 = XtVaCreateManagedWidget("browForm", xmFormWidgetClass, parent,NULL);

    n = 0;
    XtSetArg(wargs[n], XmNwordWrap, False); n++;
    XtSetArg(wargs[n], XmNeditMode, XmMULTI_LINE_EDIT); n++;
    XtSetArg(wargs[n], XmNtopAttachment, XmATTACH_FORM); n++;
    XtSetArg(wargs[n], XmNleftAttachment, XmATTACH_FORM); n++;
    XtSetArg(wargs[n], XmNrightAttachment, XmATTACH_FORM); n++;
    XtSetArg(wargs[n], XmNeditable, False); n++;
    XtSetArg(wargs[n], XmNcolumns, 80); n++;
    XtSetArg(wargs[n], XmNrows, 16); n++;
    this->text = XmCreateScrolledText(form1, "Btext", wargs, n);
    XtManageChild(this->text);

    XtAddCallback(this->text, 
		  XmNmotionVerifyCallback, 
		  Browser::motionCB, 
		  (XtPointer)this);

    XtVaGetValues(XtParent(this->text),
		  XmNverticalScrollBar, &this->vsb,
		  XmNhorizontalScrollBar, &hsb,
		  NULL);
    XtVaGetValues(this->text,
		  XmNbackground, &bg,
		  NULL);
    XtVaSetValues(this->vsb, XmNforeground, bg, NULL);
    XtVaSetValues(hsb, XmNforeground, bg, NULL);

    XtAddCallback(this->vsb, 
		  XmNincrementCallback, 
		  Browser::pageIncCB, 
		  (XtPointer)this);

    XtAddCallback(this->vsb, 
		  XmNdecrementCallback, 
		  Browser::pageDecCB, 
		  (XtPointer)this);

    XtAddCallback(this->vsb, 
		  XmNpageIncrementCallback, 
		  Browser::pageIncCB, 
		  (XtPointer)this);

    XtAddCallback(this->vsb, 
		  XmNpageDecrementCallback, 
		  Browser::pageDecCB, 
		  (XtPointer)this);

    XtAddCallback(this->vsb, 
		  XmNtoTopCallback, 
		  Browser::pageDecCB, 
		  (XtPointer)this);

    XtAddCallback(this->vsb, 
		  XmNtoBottomCallback, 
		  Browser::pageIncCB, 
		  (XtPointer)this);

    XtAddCallback(this->vsb, 
		  XmNvalueChangedCallback, 
		  Browser::pageControlCB, 
		  (XtPointer)this);

    label = XtVaCreateManagedWidget(
		"browFromMark", xmLabelWidgetClass, form1,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNbottomOffset, 20,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNrightPosition, 20,
		NULL);
	
    this->byte_mark_offset = XtVaCreateManagedWidget(
		"byte_mark_offset", xmTextWidgetClass, form1,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, label,
		XmNleftOffset, 10,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNtopOffset, -2,
		XmNtopWidget, label,
		XmNcolumns, 8,
		NULL);

    XtAddCallback(this->byte_mark_offset, 
		  XmNactivateCallback, 
		  Browser::gotoByteMarkOffsetCB, 
		  (XtPointer)this);

    XtAddCallback(this->byte_mark_offset,
		  XmNmodifyVerifyCallback,
		  Browser::integerMVCB,
		  (XtPointer)this);

    label = XtVaCreateManagedWidget(
		"browFromMark", xmLabelWidgetClass, form1,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNbottomOffset, 20,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNrightPosition, 70,
		NULL);
	
    this->line_mark_offset = XtVaCreateManagedWidget(
		"line_mark_offset", xmTextWidgetClass, form1,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, label,
		XmNleftOffset, 10,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNtopOffset, -2,
		XmNtopWidget, label,
		XmNcolumns, 8,
		NULL);

    XtAddCallback(this->line_mark_offset, 
		  XmNactivateCallback, 
		  Browser::gotoLineMarkOffsetCB, 
		  (XtPointer)this);

    XtAddCallback(this->line_mark_offset,
		  XmNmodifyVerifyCallback,
		  Browser::integerMVCB,
		  (XtPointer)this);

    label = XtVaCreateManagedWidget(
		"browFromStartOfLine", xmLabelWidgetClass, form1,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNbottomWidget, label,
		XmNbottomOffset, 10,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNrightPosition, 20,
		NULL);

    this->byte_start_offset = XtVaCreateManagedWidget(
		"byte_start_offset", xmTextWidgetClass, form1,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, label,
		XmNleftOffset, 10,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNtopWidget, label,
		XmNtopOffset, -2,
		XmNcolumns, 8,
		NULL);

    XtAddCallback(this->byte_start_offset, 
		  XmNactivateCallback, 
		  Browser::gotoByteStartOffsetCB, 
		  (XtPointer)this);

    XtAddCallback(this->byte_start_offset,
		  XmNmodifyVerifyCallback,
		  Browser::integerMVCB,
		  (XtPointer)this);

    label = XtVaCreateManagedWidget(
		"browFromTopOfFile", xmLabelWidgetClass, form1,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNbottomWidget, label,
		XmNbottomOffset, 10,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNrightPosition, 20,
		NULL);

    this->byte_top_offset = XtVaCreateManagedWidget(
		"byte_top_offset", xmTextWidgetClass, form1,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, label,
		XmNleftOffset, 10,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNtopWidget, label,
		XmNtopOffset, -2,
		XmNcolumns, 8,
		NULL);

    XtAddCallback(this->byte_top_offset, 
		  XmNactivateCallback, 
		  Browser::gotoByteTopOffsetCB, 
		  (XtPointer)this);

    XtAddCallback(this->byte_top_offset,
		  XmNmodifyVerifyCallback,
		  Browser::integerMVCB,
		  (XtPointer)this);

    label = XtVaCreateManagedWidget(
		"browFromTopOfFile", xmLabelWidgetClass, form1,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomWidget, label,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNrightPosition, 70,
		NULL);

    this->line_top_offset = XtVaCreateManagedWidget(
		"line_top_offset", xmTextWidgetClass, form1,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, label,
		XmNleftOffset, 10,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNtopWidget, label,
		XmNtopOffset, -2,
		XmNcolumns, 8,
		NULL);
	
    XtAddCallback(this->line_top_offset, 
		  XmNactivateCallback, 
		  Browser::gotoLineTopOffsetCB, 
		  (XtPointer)this);

    XtAddCallback(this->line_top_offset,
		  XmNmodifyVerifyCallback,
		  Browser::integerMVCB,
		  (XtPointer)this);

    label = XtVaCreateManagedWidget(
		"browByteOffsets", xmLabelWidgetClass, form1,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNbottomWidget, label,
		XmNbottomOffset, 10,
		XmNrightAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNrightWidget, this->byte_top_offset,
		XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNleftWidget, this->byte_top_offset,
		XmNleftOffset, -10,
		XmNrightOffset, -10,
		NULL);
	
    label = XtVaCreateManagedWidget(
		"browLineOffsets", xmLabelWidgetClass, form1,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomWidget, label,
		XmNrightAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNrightWidget, this->line_top_offset,
		XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNleftWidget, this->line_top_offset,
		XmNleftOffset, -10,
		XmNrightOffset, -10,
		NULL);

    XtVaSetValues(XtParent(this->text), 
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNbottomWidget, label,
		XmNbottomOffset, 10,
		NULL);

    Browser::AddHelpCallbacks(form1);
    return form1;
}

//
// Called when the slide bar is moved down.
//
void Browser::pageIncCB(Widget w, XtPointer client, XtPointer )
{
    Browser *browser = (Browser *)client;
    int max;
    int value;
    int slider_size;

    if(browser->ignore_cb) return;
    theIBMApplication->setBusyCursor(TRUE);
    XtVaGetValues(w, 
		  XmNmaximum, &max,
		  XmNsliderSize, &slider_size,
		  XmNvalue, &value,
		  NULL);

    if(max == (value + slider_size))
    {
	if(browser->nextPage())
	    browser->adjustScrollBar(True);
    }
    theIBMApplication->setBusyCursor(FALSE);
}

//
// Called when the slide bar is moved up.
//
void Browser::pageDecCB(Widget w, XtPointer client, XtPointer )
{
    Browser *browser = (Browser *)client;
    int min;
    int value;
    int slider_size;

    if(browser->ignore_cb) return;
    theIBMApplication->setBusyCursor(TRUE);
    XtVaGetValues(w, 
		  XmNminimum, &min,
		  XmNvalue, &value,
		  XmNsliderSize, &slider_size,
		  NULL);

    if(value == min)
    {
	if(browser->prevPage())
	    browser->adjustScrollBar(False);
    }
    theIBMApplication->setBusyCursor(FALSE);
}
void Browser::pageControlCB(Widget w, XtPointer client, XtPointer )
{
    Browser *browser = (Browser *)client;
    int max;
    int value;
    int slider_size;

    if(browser->ignore_cb) return;

    theIBMApplication->setBusyCursor(TRUE);
    XtVaGetValues(w, 
		  XmNmaximum, &max,
		  XmNvalue, &value,
		  XmNsliderSize, &slider_size,
		  NULL);
    if(value == 0)
    {
	if(browser->prevPage())
	    browser->adjustScrollBar(False);
    }
    else if(value == (max - slider_size))
    {
	if(browser->nextPage())
	    browser->adjustScrollBar(True);
    }
    theIBMApplication->setBusyCursor(FALSE);
}
void Browser::adjustScrollBar(Boolean top)
{
    int min;
    int max;
    int value;
    int slider_size;
    int inc;
    int page_inc;

    //
    // The idea is to place the slide bar 1/2 a "slider_size" away from the end.
    //
    XtVaGetValues(this->vsb, 
		  XmNminimum, &min,
		  XmNmaximum, &max,
		  NULL);
    XmScrollBarGetValues(this->vsb, &value, &slider_size, &inc, &page_inc);

    if(top)
    {
	value = min + slider_size/2;
	    if(this->page_start == 0) value = min;
	if(value > max - slider_size) value = max - slider_size;
    }
    else
    {
	value = max - slider_size/2 - slider_size;
	if(value > max - slider_size) value = max - slider_size;
	if(this->page_start == this->file_size - this->page_size) 
	    value = max - slider_size;
	if(value < min) value = min;
    }
	
    XmScrollBarSetValues(this->vsb, value, slider_size, inc, page_inc, True);
}
boolean Browser::nextPage()
{
    float percentage_overlap;

    char *str = XmTextGetString(this->text);
    int i;
    int lines = 0;
    short visible_lines;
    XtVaGetValues(this->text, XmNrows, &visible_lines, NULL);
    for(i = STRLEN(str)-1; i >= 0; i--)
    {
	if(str[i] == '\n') lines++;
	if(lines == visible_lines) break;
    }

    percentage_overlap = (float)(STRLEN(str) - i)/(float)STRLEN(str);
    if(percentage_overlap > 50.0) percentage_overlap = 50.0;
	
    //
    // If we are already on the last page, forget it.
    //
    if( (this->page_start >= this->file_size - this->page_size) ||
        (this->file_size <= this->page_size) ) {
	XtFree(str);
	return FALSE;
    }

    //
    // Next page
    //
    this->page_start += this->page_size;

    //
    // Back off visible_lines lines, or 50%, whichever is less
    //
    this->page_start -= (int)(percentage_overlap*this->page_size);

    //
    // Clamp to file size
    //
    if(this->page_start > this->file_size - this->page_size)
	this->page_start = this->file_size - this->page_size;

    this->loadBuffer(this->page_start);

    this->updateOffsets(0);
    XtFree(str);
    return TRUE;
}
boolean Browser::prevPage()
{
    float percentage_overlap;

    char *str = XmTextGetString(this->text);
    int i;
    int lines = 0;
    short visible_lines;
    XtVaGetValues(this->text, XmNrows, &visible_lines, NULL);
    for(i = 0; i < STRLEN(str)-1; i++)
    {
	if(str[i] == '\n') lines++;
	if(lines == visible_lines) break;
    }

    percentage_overlap = (float)i/(float)STRLEN(str);
    if(percentage_overlap > 50.0) percentage_overlap = 50.0;
	
    if(this->page_start == 0) {
	XtFree(str);
	return FALSE;
    }

    //
    // Prev page
    //
    if(this->page_start > this->page_size)
	this->page_start -= this->page_size;
    else
	this->page_start = 0;

    //
    // Provide some overlap, unless this is the start of the file
    //
    if(this->page_start != 0)
	this->page_start += (int)(percentage_overlap*this->page_size);

    //
    // Clamp to start of file
    //
    /*if(this->page_start < 0)
      this->page_start = 0;*/

    this->loadBuffer(this->page_start);

    this->updateOffsets(0);
    XtFree(str);
    return TRUE;
}
void Browser::placeMark()
{
    int i;
    int line_no;
    char mark_str[2];

    mark_str[0] = '\001';
    mark_str[1] = '\0';

    theIBMApplication->setBusyCursor(TRUE);
    char *str = XmTextGetString(this->text);

    this->clearMark();

    //
    // Calc where it should go
    //
    unsigned long pos = XmTextGetInsertionPosition(this->text);
    this->marker_pos = this->page_start + pos;

    line_no = 0;
    for(i = 0; i < pos; i++)
    {
	if(str[i] == '\n') line_no++;
    }
    this->marker_line_no = this->page_start_line_no + line_no;
    this->updateOffsets(pos);
    
    //
    // Save the marker character
    //
    this->marker_char[0] = str[pos];

    this->ignore_cb = True;
    XmTextReplace(this->text, pos, pos+1, mark_str);
    this->ignore_cb = False;

    XmTextSetInsertionPosition(this->text, pos);

    XtFree(str);
    theIBMApplication->setBusyCursor(FALSE);
}
void Browser::clearMark()
{
    unsigned long pos;

    if(!this->marker_char[0]) return;
    //
    // If the marker is on this page...
    //
    if( (this->marker_pos >= this->page_start) &&
	(this->marker_pos < this->page_start + this->page_size) )
    {
	pos = this->marker_pos - this->page_start;

	this->ignore_cb = True;
	XmTextReplace(this->text, pos, pos+1, this->marker_char);
	this->ignore_cb = False;
    }
    else
    {
	pos = XmTextGetInsertionPosition(this->text) - 1;
    }
    this->marker_pos = 0;
    this->marker_char[0] = '\0';
    this->updateOffsets(pos+1);
}
void Browser::gotoMark()
{
    this->gotoByte(this->marker_pos);
}
void Browser::gotoByte(unsigned long offset)
{
    XmTextPosition pos;

    //
    // Is it on this page?
    //
    if( (offset > this->page_start) &&
	(offset < (this->page_start + this->page_size)) )
    {
	pos = offset - this->page_start;
    }
    else
    {
	if(offset > this->page_size/2)
	    this->page_start = offset - this->page_size/2;
	else
	    this->page_start = 0;

	if(this->page_start + this->page_size > this->file_size)
	    this->page_start = this->file_size - this->page_size;

	this->loadBuffer(this->page_start);
	pos = offset - this->page_start;
    }

    XmTextShowPosition(this->text, pos);
    XmTextSetInsertionPosition(this->text, pos);
}
void Browser::gotoLineTopOffsetCB(Widget w, XtPointer client, XtPointer )
{
    Browser *browser = (Browser *)client;
    unsigned long line_num;

    theIBMApplication->setBusyCursor(TRUE);
    char *str = XmTextGetString(w);
    if(STRLEN(str) > 0)
	line_num = atoi(str);
    else
	return;
    browser->gotoLine(line_num);
    theIBMApplication->setBusyCursor(FALSE);
}
void Browser::gotoLineMarkOffsetCB(Widget w, XtPointer client, XtPointer)
{
    Browser *browser = (Browser *)client;
    unsigned long line_num;

    theIBMApplication->setBusyCursor(TRUE);
    char *str = XmTextGetString(w);
    if(STRLEN(str) > 0)
	line_num = atoi(str) + browser->marker_line_no;
    else
	return;
    browser->gotoLine(line_num);
    theIBMApplication->setBusyCursor(FALSE);
}
void Browser::gotoLine(unsigned long line_num)
{
    int i;
    unsigned long offset;
    unsigned long current_line_num;
    char *buf = (char *)CALLOC(this->page_size+1, sizeof(char));

    if(line_num >= this->page_start_line_no)
    {
	offset = this->page_start;
	current_line_num = this->page_start_line_no;
    }
    else
    {
	offset = 0;
	current_line_num = 0;
    }
    //
    // Calc the line number offset of this position
    //
    while (offset < this->file_size)
    {
	this->from->seekg((std::streampos)offset);
	if(!this->from)
	{
	    std::cerr << "Seekg failed in Browser::gotoLine()" << std::endl;
	    break;
	}
	this->from->read(buf, this->page_size);
	if(!this->from)
	{
	    std::cerr << "Read failed in Browser::gotoLine()" << std::endl;
	    break;
	}
	if(this->from->fail()) this->from->clear();

	//
	// Get the actual number of chars read in
	//
	int nread = this->from->gcount();

	for(i = 0; i < nread; i++)
	{
	    if(buf[i] == '\n')
	    {
		current_line_num++;
	    }
	    if(current_line_num == line_num) break;
	}
	if(current_line_num == line_num) break;
	offset += nread;
    }

    //
    // Move to the start of the next line
    //
    i++;
    this->gotoByte(offset+i);

    delete buf;
}

void Browser::gotoByteTopOffsetCB(Widget w, XtPointer client, XtPointer)
{
    Browser *browser = (Browser *)client;
    XmTextPosition pos;

    theIBMApplication->setBusyCursor(TRUE);
    char *str = XmTextGetString(w);
    if(STRLEN(str) > 0)
    {
	pos = atoi(str);
    }
    else
    {
	theIBMApplication->setBusyCursor(FALSE);
	return;
    }
    if( !((pos >= browser->page_start) &&
	 (pos < (browser->page_start + browser->page_size))) )
    {
	browser->page_start = pos - browser->page_size/2;
	if(browser->page_start + browser->page_size > browser->file_size)
	    browser->page_start = browser->file_size - browser->page_size;
	/*if(browser->page_start < 0) browser->page_start = 0;*/
	browser->loadBuffer(browser->page_start);
    }
    pos = pos - browser->page_start;
    XmTextShowPosition(browser->text, pos);
    XmTextSetInsertionPosition(browser->text, pos);
    theIBMApplication->setBusyCursor(FALSE);
}
void Browser::gotoByteStartOffsetCB(Widget w, XtPointer client, XtPointer )
{
    Browser *browser = (Browser *)client;
    XmTextPosition pos;

    theIBMApplication->setBusyCursor(TRUE);
    char *str = XmTextGetString(w);
    if(STRLEN(str) > 0)
    {
	pos = atoi(str);
    }
    else
    {
	theIBMApplication->setBusyCursor(FALSE);
	return;
    }
    char *text_str = XmTextGetString(browser->text);
    XmTextPosition current_pos = XmTextGetInsertionPosition(browser->text);
    long line_start = current_pos;

    //
    // Search backwards for the start of the line
    //
    while (line_start >= 0)
    {
	if(text_str[line_start] == '\n') break;
	line_start--;
    }
    line_start++;
	
    XmTextShowPosition(browser->text, line_start + pos);
    XmTextSetInsertionPosition(browser->text, line_start + pos);

    theIBMApplication->setBusyCursor(FALSE);
}
void Browser::gotoByteMarkOffsetCB(Widget w, XtPointer client, XtPointer)
{
    Browser *browser = (Browser *)client;
    XmTextPosition pos;

    theIBMApplication->setBusyCursor(TRUE);
    char *str = XmTextGetString(w);
    if(STRLEN(str) > 0)
    {
	pos = atoi(str) + browser->marker_pos;
    }
    else
    {
	theIBMApplication->setBusyCursor(FALSE);
	return;
    }
    if(!(pos >= browser->page_start) &&
	(pos < (browser->page_start + browser->page_size)) )
    {
	browser->page_start = browser->marker_pos - browser->page_size/2;
	if(browser->page_start + browser->page_size > browser->file_size)
	    browser->page_start = browser->file_size - browser->page_size;
	/*if(browser->page_start < 0) browser->page_start = 0;*/
	browser->loadBuffer(browser->page_start);
    }
    pos = pos - browser->page_start;
    XmTextShowPosition(browser->text, pos);
    XmTextSetInsertionPosition(browser->text, pos);

    theIBMApplication->setBusyCursor(FALSE);
}

void Browser::firstPage()
{

    this->loadBuffer(0);
    XmTextSetInsertionPosition(this->text, 0);
    this->updateOffsets(0);
}
void Browser::lastPage()
{
    unsigned long offset;

    if(this->page_size > this->file_size)
	offset = 0;
    else
	offset = this->file_size - this->page_size;
    this->loadBuffer(offset);

    //
    // Put the slider at the end.
    //
    int max;
    int value;
    int slider_size;
    int inc;
    int page_inc;

    XtVaGetValues(this->vsb, 
		  XmNmaximum, &max,
		  NULL);

    XmScrollBarGetValues(this->vsb, &value, &slider_size, &inc, &page_inc);
    XmScrollBarSetValues(this->vsb, max-slider_size, 
			 slider_size, inc, page_inc, True);
    this->updateOffsets(0);
}

void Browser::motionCB(Widget , XtPointer client, XtPointer call)
{
XmTextVerifyCallbackStruct *cbs = (XmTextVerifyCallbackStruct *)call;
Browser *browser = (Browser *)client;

    browser->updateOffsets((unsigned long)cbs->newInsert);
}

//
// Note:new_pos is the byte offset from the start of the CURRENT page!
//
void Browser::updateOffsets(unsigned long new_pos)
{
char		string[256];
unsigned long 	i;
int 		start_of_line_offset;

    unsigned long position = this->page_start + new_pos;
    long line_no = this->page_start_line_no;
    char *str = XmTextGetString(this->text);

    //
    // Replace the marker char with the original char in case the original
    // was a newline.
    //
    if( (this->marker_pos >= this->page_start) &&
	(this->marker_pos < this->page_start + this->page_size) &&
	(this->marker_char[0]) )
    {
	str[this->marker_pos - this->page_start] = this->marker_char[0];
    }

    start_of_line_offset = 0;
    for(i = 0; i < new_pos; i++)
    {
	start_of_line_offset++;
	if(str[i] == '\n')
	{
	    start_of_line_offset = 0;
	    line_no++;
	}
    }
    //
    // Number of lines from top of file
    //
    sprintf(string, "%ld", line_no);
    XmTextSetString(this->line_top_offset, string);

    //
    // Number of bytes from top of file
    //
    sprintf(string, "%ld", position);
    XmTextSetString(this->byte_top_offset, string);

    //
    // Number of bytes from start of line
    //
    sprintf(string, "%d", start_of_line_offset);
    XmTextSetString(this->byte_start_offset, string);

    
    //
    // Number of bytes from marker
    //
    long marker_byte_offset = position - this->marker_pos;
    sprintf(string, "%ld", marker_byte_offset);
    XmTextSetString(this->byte_mark_offset, string);

    //
    // Number of lines from marker
    //

    //
    // If the marker is on a prior page...
    //
    if(this->marker_pos < this->page_start)
    {
	line_no = this->page_start_line_no;
	for(i = 0; i < new_pos; i++)
	{
	    if(str[i] == '\n')
		line_no++;
	}
	line_no = line_no - this->marker_line_no;
    }
    //
    // If the marker is on a subsequent page...
    //
    else if(this->marker_pos >= this->page_start + this->page_size)
    {
	line_no = this->page_start_line_no;
	for(i = 0; i < new_pos; i++)
	{
	    if(str[i] == '\n')
		line_no++;
	}
	line_no = line_no - this->marker_line_no;
    }
    //
    // If the marker is on this page...
    //
    else if( (this->marker_pos >= this->page_start) &&
	     (this->marker_pos < this->page_start + this->page_size) )
    {
	unsigned long start;
	unsigned long end;
	line_no = 0;
	unsigned long marker_pos = this->marker_pos - this->page_start;

	if(marker_pos > new_pos)
	{
	    start = new_pos;
	    end = marker_pos;
	}
	else
	{
	    start = marker_pos;
	    end = new_pos;
	}
	for(i = start; i < end; i++)
	{
	    if(str[i] == '\n')
	    {
		if(marker_pos > new_pos)
		    line_no--;
		else
		    line_no++;
	    }
	}
    }

    sprintf(string, "%ld", line_no);
    XmTextSetString(this->line_mark_offset, string);

    XtFree(str);
}

void Browser::output_formatCB(Widget , XtPointer , XtPointer )
{
#ifdef Comment
UIVDATA *uivdata;
    
    XtVaGetValues(w, XmNuserData, &uivdata, NULL);
    set_watch(uivdata->text);
    if(w == uivdata->ascii_w)
	uivdata->output_format = ASCII;
    else if(w == uivdata->hex8_w)
	uivdata->output_format = HEX8;
    else if(w == uivdata->hex16_w)
	uivdata->output_format = HEX16;
    else if(w == uivdata->hex32_w)
	uivdata->output_format = HEX32;
    else if(w == uivdata->float_w)
	uivdata->output_format = FLOAT;
    else if(w == uivdata->double_w)
	uivdata->output_format = DOUBLE;

    setPageNum(uivdata, uivdata->page_num);
    reset_cursor(uivdata->text);
#endif
}
void Browser::integerMVCB(Widget , XtPointer clientData, XtPointer call)
{
    /*Browser *browser = (Browser *) clientData;*/
    XmTextVerifyCallbackStruct *cbs = (XmTextVerifyCallbackStruct *)call;
    int i;

    if(cbs->text->ptr == NULL) //backspace
        return;

    for(i = 0; i < cbs->text->length; i++)
        if(!(isdigit(cbs->text->ptr[i]) || cbs->text->ptr[i] == '-'))
        {
            cbs->doit = False;
            return;
        }
}

boolean Browser::openFile(char *filenm)
{
    struct STATSTRUCT statb;
    char title[512];

    this->from = new std::ifstream(filenm);
    if(!this->from)
    {
	WarningMessage("File open failed for file %s", filenm);
	return FALSE;
    }

    if(STATFUNC(filenm, &statb) == -1)
    {
	// stat failed
        return FALSE;
    }
    this->file_size = statb.st_size;

    //
    // Clear any existing mark
    //
    this->marker_pos = 0;
    this->marker_char[0] = '\0';

    this->page_start = 0;
    this->page_start_line_no = 0;

    this->loadBuffer(0);
    title[0] = '\0';
    strcat(title, "File Browser: ");
    strcat(title, filenm);
    this->setWindowTitle(title);
    return TRUE;
}
void Browser::loadBuffer(unsigned long file_offset)
{
    char *buf = (char *)CALLOC(this->page_size+1, sizeof(char));
    char *linebuf = (char *)CALLOC(this->page_size+1, sizeof(char));
    int i;

    //
    // HACK!
    //
    if(this->from->fail()) this->from->clear();

    //
    // Read in a buffer
    //
    this->from->seekg((std::streampos)file_offset);
    if(!this->from)
    {
	std::cerr << "Seekg failed in Browser::loadBuffer()" << std::endl;
    }
    this->from->read(buf, this->page_size);
    if(!this->from)
    {
	std::cerr << "Read failed in Browser::loadBuffer()" << std::endl;
    }
    if(this->from->fail()) this->from->clear();

    this->page_start = file_offset;

    //
    // Get the actual number of chars read in
    //
    int nread = this->from->gcount();

    //
    // And NULL terminate the string
    //
    buf[nread] = '\0';
    
    //
    // Convert non-printing chars to '.'
    for(i = 0; i < nread; i++)
    {
	if(((!isprint(buf[i])) && (buf[i] != '\n') && (buf[i] !='\t') &&
		 (buf[i] != ' ')) || (!isascii(buf[i])))
	    buf[i] = '.';
    }
    //
    // If the marker is on this page, change the character
    //
    if( (this->marker_pos >= this->page_start) &&
	(this->marker_pos < this->page_start + this->page_size) &&
	 this->marker_char[0] )
    {
	unsigned long pos = this->marker_pos - this->page_start;
	buf[pos] = '\001';
    }

    //
    // Give it to the text widget
    //
    this->ignore_cb = True;
    XmTextSetString(this->text, buf);
    this->ignore_cb = False;

    //
    // Calc the line number offset of this position
    //
    this->from->seekg((std::streampos)0);
    if(!this->from)
    {
	std::cerr << "Seekg failed in Browser::loadBuffer()" << std::endl;
    }
    int offset = 0;
    this->page_start_line_no = 0;
    while (offset < file_offset)
    {
	this->from->getline(linebuf, this->page_size);
	if(!this->from)
	{
	    std::cerr << "Getline failed in Browser::gotoLine()" << std::endl;
	    break;
	}
	// Add one, since getline does not get the '\n'
	offset += STRLEN(linebuf)+1;
	if(offset < file_offset)
	    this->page_start_line_no++;
    }

    FREE(buf);
    FREE(linebuf);
}

void Browser::postSearchDialog()
{
    if(!this->searchDialog)
	this->searchDialog = new SearchDialog(this->getRootWidget(), this);

    this->searchDialog->post();
}
void Browser::unpostSearchDialog()
{
    this->searchDialog->unmanage();
}
void Browser::searchForward(char *text)
{

    boolean wrap = FALSE;
    boolean found = FALSE;
    char *buf = (char *)CALLOC(this->page_size+1, sizeof(char));
    int i;

    long start_pos = 
	1 + XmTextGetInsertionPosition(this->text) + this->page_start;
    if(start_pos >= this->file_size) start_pos = 0;
    long pos = start_pos;
 

    theIBMApplication->setBusyCursor(TRUE);

#if defined(HAVE_REGCOMP) && defined(HAVE_REGEX_H)

    regex_t search_for;
    ASSERT(regcomp(&search_for, text, REG_NOSUB) == 0);

#elif defined(HAVE_REGCOMP) && defined(HAVE_REGEXP_H)

    char *search_for = (char *)regcomp(text);
    ASSERT(search_for != NULL);

#elif defined(HAVE_REGCMP)

    char *search_for = regcmp(text, NULL);
    ASSERT(search_for != NULL);

#elif defined(HAVE_RE_COMP)

    char *error_string = re_comp(text);
    if(error_string)
	WarningMessage("Couldn't compile regular expression! %s", error_string);

#endif

    while(!found && !wrap || pos < start_pos)
    {
	//
	// Read in a buffer
	//
	this->from->seekg((std::streampos)pos);
	if(!this->from)
	{
	    std::cerr << "Seekg failed in Browser::searchForward()" << std::endl;
	}
	this->from->read(buf, this->page_size);
	if(!this->from)
	{
	    std::cerr << "Read failed in Browser::searchForward()" << std::endl;
	}
	if(this->from->fail()) this->from->clear();

	//
	// Get the actual number of chars read in
	//
	int nread = this->from->gcount();

	//
	// And NULL terminate the string
	//
	buf[nread] = '\0';
       
	//
	// Convert non-printing chars to '.'
	//
	for(i = 0; i < nread; i++)
	{
	    if(((!isprint(buf[i])) && (buf[i] != '\n') && (buf[i] !='\t') &&
		     (buf[i] != ' ')) || (!isascii(buf[i])))
		buf[i] = '.';
	}

	int offset;

#if defined(HAVE_REGCOMP) && defined(HAVE_REGEX_H)

	int i;
	for (i = 0; i < STRLEN(buf); i++)
	    if (regexec(&search_for, buf + i, 0, NULL, 0) != 0)
		break;
	
	if (i)
	{
	    offset = i - 1;
	    found = 1;
	}	    

#elif defined(HAVE_REGCOMP) && defined(HAVE_REGEXP_H)

	int i;
	for (i = 0; i < STRLEN(buf); i++)
	    if (! regexec((regexp *)search_for, buf + i ))
	    	break;

	if (i)
	{
	    offset = i - 1;
	    found = 1;
	}

#elif defined(HAVE_REGCMP)

	extern char *__loc1;
	if (regex(search_for, buf))
	{
	    offset = __loc1 - buf;
	    found = 1;
	}

#elif defined(HAVE_RE_COMP)

	int i;
	for (i = 0; i < STRLEN(buf); i++)
	    if (! re_exec(buf + i))
	    	break;

	if (i)
	{
	    offset = i - 1;
	    found = 1;
	}

#else

	char *s;
	s = strstr(text,  buf);
	if (s)
	{
	    offset = s - buf;
	    found = 1;
	}
        
#endif

	if(found)
	{
	    this->gotoByte(offset);
	    found = TRUE;
	    break;
	}
	else
	{
	    int nread = this->from->gcount();
	    //
	    // "Loop" around on the search
	    //
	    if(nread == this->page_size)
	    {
		pos += (int)(0.95*this->page_size);
	    }
	    else
	    {
		// If we have already wrapped, break out.
		if(wrap)
		    break;
		else
		    pos = 0;
		wrap = TRUE;
	    }
	}
    }
    if(!found)
	WarningMessage("Pattern not found");


#if defined(HAVE_REGCOMP) && defined(HAVE_REGEX_H)
    regfree(&search_for);
#elif (defined(HAVE_REGCOMP) && defined(HAVE_REGEXP_H)) || defined(HAVE_REGCMP)
    free(search_for);
#endif

    FREE(buf);
    theIBMApplication->setBusyCursor(FALSE);
}

void Browser::searchBackward(char *text)
{

    boolean wrap = FALSE;
    boolean last_buf = FALSE;
    boolean found = FALSE;
    char *buf = (char *)CALLOC(this->page_size+1, sizeof(char));
    long start_pos = XmTextGetInsertionPosition(this->text) + this->page_start;
    if(start_pos < 0) start_pos = this->file_size;
    long pos = start_pos;
    long prev_pos;
    long bufstart;

    theIBMApplication->setBusyCursor(TRUE);

#if defined(HAVE_REGCOMP) && defined(HAVE_REGEX_H)

    regex_t search_for;
    ASSERT(regcomp(&search_for, text, REG_NOSUB) == 0);
    
#elif defined(HAVE_REGCOMP) && defined(HAVE_REGEXP_H)

    char *search_for = (char *)regcomp(text);
    ASSERT(search_for != NULL);

#elif defined(HAVE_REGCMP)

    char *search_for = regcmp(text, NULL);
    ASSERT(search_for != NULL);

#elif defined(HAVE_RE_COMP)

    char *error_string = re_comp(text);
    if(error_string)
	WarningMessage("Couldn't compile regular expression! %s", error_string);

#endif


    bufstart = start_pos - this->page_size;
    if(bufstart < 0) bufstart = 0;
    
    while(!wrap || pos >= start_pos)
    {
	//
	// Read in a buffer
	//
	this->from->seekg((std::streampos)bufstart);
	if(!this->from)
	{
	    std::cerr << "Seekg failed in Browser::searchBackward()" << std::endl;
	}
	this->from->read(buf, this->page_size);
	if(!this->from)
	{
	    std::cerr << "Read failed in Browser::searchBackward()" << std::endl;
	}
	if(this->from->fail()) this->from->clear();

        //
        // Get the actual number of chars read in
        //
        int nread = this->from->gcount();

        //
        // And NULL terminate the string
        //
        buf[nread] = '\0';

        //
        // Convert non-printing chars to '.'
        //
        int i;
        for(i = 0; i < nread; i++)
        {
	    if(((!isprint(buf[i])) && (buf[i] != '\n') && (buf[i] !='\t') &&
		     (buf[i] != ' ')) || (!isascii(buf[i])))
		buf[i] = '.';
        }

	int offset;

#if defined(HAVE_REGCOMP) && defined(HAVE_REGEX_H)

	if (regexec(&search_for, buf, 0, NULL, 0) == 0)
	{
	    found = 1;
	    
	    for (i = STRLEN(buf)-1; i >= 0; i--)
		if (regexec(&search_for, buf + i, 0, NULL, 0) != 0)
		    break;
	    
	    offset = i + 1;
	}
	
#elif defined(HAVE_REGCOMP) && defined(HAVE_REGEXP_H)

	if (regexec((regexp *)search_for, buf))
	{
	    found = 1;

	    for (i = STRLEN(buf)-1; i >= 0; i--)
		if (! regexec((regexp *)search_for, buf + i ))
		    break;

	    offset = i + 1;
	}

#elif defined(HAVE_REGCMP)

	if (regex(search_for, buf))
	{
	    found = 1;

	    for (i = STRLEN(buf)-1; i >= 0; i--)
		if (! regex(search_for, buf + i ))
		    break;

	    offset = i + 1;
	}

#elif defined(HAVE_RE_COMP)

	if (re_exec(buf))
	{
	    found = 1;

	    for (i = STRLEN(buf)-1; i >= 0; i--)
		if (! re_exec(buf + i))
		    break;

	    offset = i + 1;
	}

#else

	if (strstr(text,  buf))
	{
	    found = 1;

	    for (i = STRLEN(buf)-1; i >= 0; i--)
		if (! strstr(text,  buf + i ))
		    break;

	    offset = i + 1;
	}
#endif

	pos = bufstart + offset;
	if(!wrap)
	{
	    if(pos >= start_pos)
	    {
		break;
	    }
	    else
	    {
		prev_pos = pos;
		found = TRUE;
	    }
	}
	else
	{
	    prev_pos = pos;
	    found = TRUE;
	}
    
	if(!found)
	{
	    //
	    // "Loop" around on the search
	    //
	    if(bufstart != 0)
	    {
		bufstart -= (int)(0.95*this->page_size);
		if(bufstart < 0) bufstart = 0;
		if(wrap)
		{
		    if(bufstart < start_pos) 
		    {
			if(last_buf) break;
			bufstart = start_pos;
			last_buf = TRUE;
		    }
		}
	    }
	    else
	    {
		bufstart = this->file_size - this->page_size;
		if(bufstart < 0) bufstart = 0;
		pos = this->file_size;
		if(wrap) break;
		wrap = TRUE;
	    }
	}
	else
	{
	    this->gotoByte(prev_pos);
	    break;
	}
    }
    if(!found)
	WarningMessage("Pattern not found");

#if defined(HAVE_REGCOMP) && defined(HAVE_REGEX_H)
    regfree(&search_for);
#elif (defined(HAVE_REGCOMP) && defined(HAVE_REGEXP_H)) || defined(HAVE_REGCMP)
    free(search_for);
#endif

    FREE(buf);
    theIBMApplication->setBusyCursor(FALSE);
}

void Browser::AddHelpCallbacks(Widget w)
{

    //
    // Add the callback for this one...
    //
    XtAddCallback(w, XmNhelpCallback, (XtCallbackProc)Browser_HelpCB, (XtPointer)NULL);

    //
    // Add the callback for any descendants
    //
    if(XtIsComposite(w))
    {
	WidgetList      children;
	Cardinal	nchild;
	int	     i;

	XtVaGetValues(w, XmNchildren, &children, XmNnumChildren, &nchild, NULL);
	for(i = 0; i < nchild; i++)
	{
	    Browser::AddHelpCallbacks(children[i]);
	}
    }
}
extern "C" void Browser_HelpCB(Widget w, XtPointer, XtPointer)
{
    theApplication->helpOn(XtName(w));
}

