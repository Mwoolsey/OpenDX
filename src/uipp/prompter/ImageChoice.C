/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#include "ImageChoice.h"
#include "GARChooserWindow.h"
#include "DXStrings.h"
#include "XmUtility.h"
#include "WarningDialogManager.h"
#include "GARApplication.h"


#include <Xm/Xm.h>
#include <Xm/Form.h>
#include <Xm/RowColumn.h>
#include <Xm/PushB.h>
#include <ctype.h>

#include "../widgets/Number.h"


boolean ImageChoice::ClassInitialized = FALSE;

String ImageChoice::DefaultResources[] = {
#if defined(aviion)
    "*formats.labelString:",
#endif

    "*conform.accelerators: #augment\n"
    "<Key>Return:    BulletinBoardReturn()",

    NUL(char*)
};

//
// Used as the name of a widget
String ImageChoice::FormatTypes[] = {
    "tiff",
    "miff",
    "gif",
    "rgb",
    "r+g+b",
    "yuv",
    NUL(char*)
};

//
// Used as the labelString of a widget
String ImageChoice::FormatNames[] = {
    "TIFF",
    "MIFF",
    "GIF",
    "RGB",
    "R+G+B",
    "YUV",
    NUL(char*)
};

//
// Used in userData of a widget
String ImageChoice::FormatExt[] = {
    ".TIFF",
    ".MIFF",
    ".GIF",
    ".RGB",
    ".R",
    ".YUV",
    NUL(char*)
};


ImageChoice::ImageChoice (GARChooserWindow *gcw, Symbol sym) :
    NonimportableChoice ("spreadSheet", FALSE, FALSE, TRUE, FALSE, gcw, sym) 
{
    this->format_om = NUL(Widget);
}

ImageChoice::~ImageChoice()
{
}


void ImageChoice::initialize() 
{
    if (ImageChoice::ClassInitialized) return ;
    ImageChoice::ClassInitialized = TRUE;

    this->setDefaultResources
	(theApplication->getRootWidget(), TypeChoice::DefaultResources);
    this->setDefaultResources
	(theApplication->getRootWidget(), NonimportableChoice::DefaultResources);
    this->setDefaultResources
	(theApplication->getRootWidget(), ImageChoice::DefaultResources);
}


Widget ImageChoice::createBody (Widget parent, Widget top)
{
    Widget conform = XtVaCreateManagedWidget ("conform",
	xmFormWidgetClass,      parent,
	XmNtopAttachment,       (top?XmATTACH_WIDGET:XmATTACH_FORM),
	XmNtopWidget,           top,
	XmNtopOffset,           (top?0:2),
	XmNleftAttachment,      XmATTACH_FORM,
	XmNrightAttachment,     XmATTACH_FORM,
	XmNleftOffset,          2,
	XmNrightOffset,         2,
    NULL);
 
    //
    // Format menu
    //
    int n = 0;
    Arg args[16];
    XtSetArg (args[n], XmNleftAttachment, XmATTACH_FORM); n++;
    XtSetArg (args[n], XmNleftOffset,     2); n++;
    XtSetArg (args[n], XmNtopAttachment,  XmATTACH_FORM); n++;
    XtSetArg (args[n], XmNtopOffset,      2); n++;
    this->format_om = XmCreateOptionMenu (conform, "formats", args, n);
    XtManageChild (this->format_om);
    n = 0;
    XtSetArg (args[n], XmNentryAlignment, XmALIGNMENT_CENTER); n++;
    Widget pulldown = XmCreatePulldownMenu (this->format_om, "formats_pd", args, n);
    XtVaSetValues (this->format_om, XmNsubMenuId, pulldown, NULL);

    int i = 0;
    while (ImageChoice::FormatNames[i]) {
	XmString xmstr = XmStringCreate (ImageChoice::FormatNames[i], "bold");
	this->format_widget[i] = 
	XtVaCreateManagedWidget (ImageChoice::FormatTypes[i],
	    xmPushButtonWidgetClass, pulldown,
	    XmNuserData, ImageChoice::FormatExt[i],
	    XmNlabelString, xmstr,
	NULL);
	XmStringFree(xmstr);
	i++;
    }

    return conform;
}




//
// set DXLInputs:
//	filename
//	delimiter
//	variable
//	record_start
//	record_end
//	record_delta
//
boolean ImageChoice::visualize()
{
    //
    // ExecMode would be better however MOD-ReadImage doesn't seem to make decent
    // error messages available without dxui.
    //
    if (!this->connect(TypeChoice::ImageMode/*ExecMode*/)) return FALSE;
    DXLConnection* conn = this->gcw->getConnection();

    char net_file[512];
    const char *net_dir = theGARApplication->getResourcesNetDir();
    sprintf (net_file, "%s/ui/ReadImage.net", net_dir);

    Widget wid;
    XtVaGetValues (this->format_om, XmNmenuHistory, &wid, NULL);
    if (!wid) return FALSE;

    char *args[4];
    args[0] = "_filename_";
    args[1] = DuplicateString (this->gcw->getDataFilename());
    args[2] = "_image_format_";
    args[3] = XtName(wid);
    this->gcw->loadAndSet (conn, net_file, args, 4);
    delete args[1];

    this->hideList();
    DXLExecuteOnce (conn);

    return TRUE;
}


//
// file_checked means that the caller has looked for the file and found
// that is exists.
//
void ImageChoice::setCommandActivation(boolean file_checked)
{
    this->NonimportableChoice::setCommandActivation(file_checked);

    //
    // Does the text widget at the top contain a valid data file name?
    //
    const char *cp = this->gcw->getDataFilename();
    const char *file_name = NUL(char*);
    if ((cp) && (cp[0]))
	file_name = theGARApplication->resolvePathName(cp, this->getFileExtension());

    int name_len = 0;
    if (file_name)
	name_len = strlen(file_name);

    if (name_len) {
	char* fname = DuplicateString(file_name);

	int i = name_len - 1;
	while ((i>=0) && (fname[i] != '.')) i--;
	const char* ext = &fname[i];

	i++;
	while (i<name_len) {
	    fname[i] = toupper(fname[i]);
	    i++;
	}

	i = 0;
	while (ImageChoice::FormatNames[i]) {
	    XtPointer ud;
	    XtVaGetValues (this->format_widget[i], XmNuserData, &ud, NULL);
	    char* cp = (char*)ud;
	    if (EqualString(ext, cp)) {
		XtVaSetValues (this->format_om, 
		    XmNmenuHistory, this->format_widget[i],
		NULL);
		break;
	    }
	    i++;
	}
	delete fname;
    }
}


boolean ImageChoice::canHandle(const char* ext)
{
    int i = 0;
    while (ImageChoice::FormatNames[i]) {
	XtPointer ud;
	XtVaGetValues (this->format_widget[i], XmNuserData, &ud, NULL);
	char* cp = (char*)ud;
	if (EqualString(ext, cp)) return TRUE;
	i++;
    }
    return FALSE;
}
