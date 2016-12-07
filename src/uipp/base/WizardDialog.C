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
#include <sys/types.h>
#include <sys/stat.h>
#include <Xm/ToggleB.h>
 
#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif
#include "IBMApplication.h"
#include "WizardDialog.h"
#include "DXStrings.h"
#include "ListIterator.h"

boolean WizardDialog::ClassInitialized = FALSE;

List* WizardDialog::AlreadyWizzered = NUL(List*);

String WizardDialog::DefaultResources[] =
{
   "*dialogTitle:			Window Comment..." ,
   "*nameLabel.labelString:		Window Comment:" ,
   "*closeToggle.labelString:   	Do not display this message again.",
   "*closeToggle.shadowThickness: 	0",

    "*editorText.columns:       	40",  
    "*editorText.rows:          	10",

    NUL(char*)
};

WizardDialog::WizardDialog(Widget parent, const char* parent_name)
			: TextEditDialog("wizard", parent, TRUE, TRUE)
{
    this->parent_name = DuplicateString(parent_name);
    this->text = NUL(char*);
    this->file_read = FALSE;
    this->closeToggle = NUL(Widget);

    if (NOT WizardDialog::ClassInitialized)
    {
        WizardDialog::ClassInitialized = TRUE;
	this->installDefaultResources(theApplication->getRootWidget());
    }
}

WizardDialog::~WizardDialog()
{
    if (this->parent_name) delete this->parent_name;
    if (this->text) delete this->text;
}

//
// Install the default resources for this class.
//
void WizardDialog::installDefaultResources(Widget  baseWidget)
{
    this->setDefaultResources(baseWidget, WizardDialog::DefaultResources);
    this->TextEditDialog::installDefaultResources( baseWidget);
}

boolean WizardDialog::okCallback(Dialog* dialog)
{
    //
    // Check the state of the toggle
    //
    Boolean close_permanently;
    XtVaGetValues (this->closeToggle,
	XmNset,		&close_permanently,
    NULL);
    if (close_permanently)
	theIBMApplication->appendNoWizardName(this->parent_name);

    return this->TextEditDialog::okCallback(dialog);
}

Widget WizardDialog::createDialog (Widget parent)
{
    Widget diag = this->TextEditDialog::createDialog(parent);

    //
    // Add the toggle button
    //
    this->closeToggle = XtVaCreateManagedWidget ("closeToggle",
	xmToggleButtonWidgetClass,	XtParent(this->ok),
	XmNrightAttachment,		XmATTACH_FORM,
	XmNrightOffset,			10,
	XmNbottomAttachment,		XmATTACH_FORM,
	XmNbottomOffset,		10,
	XmNset,				False,
    NULL);

    return diag;
}

//
// Stat the file.  If not exists, then return NULL and the wizard ought to 
// remain down.
//
#if defined(intelnt) || defined(WIN32)
#define FILE_SEPARATOR '/'
#define ISGOOD(a) ((a&_S_IFREG)&&((a&_S_IFDIR)==0))
#else
#define FILE_SEPARATOR '/'
#define ISGOOD(a) ((a&S_IFREG)&&((a&(S_IFDIR|S_IFCHR|S_IFBLK))==0))
#endif
const char* WizardDialog::getText()
{
char pathname[512];
struct STATSTRUCT statb;

    if (this->text) return this->text;
    if (this->file_read) return NUL(char*);
    this->file_read = TRUE;

    const char* uiroot = theIBMApplication->getUIRoot();
    ASSERT(uiroot);
    if (uiroot[strlen(uiroot)-1] == FILE_SEPARATOR)
	sprintf (pathname, "%sui/%s", uiroot, this->parent_name);
    else
	sprintf (pathname, "%s/ui/%s", uiroot, this->parent_name);

    if (STATFUNC(pathname, &statb) == -1) return NUL(char*);

    if (!ISGOOD(statb.st_mode)) return NUL(char*);

    this->text = new char[1+statb.st_size];

    FILE* f = fopen(pathname, "r");
    if (!f) return NUL(char*);

    fread(this->text, sizeof(unsigned char), statb.st_size, f);
    fclose(f);
    this->text[statb.st_size] = '\0';

    return this->text;
}

//
// Don't show yourself if you've already presented text for this class
//
void WizardDialog::post()
{
    boolean found = FALSE;

    if (WizardDialog::AlreadyWizzered) {
	ListIterator it(*WizardDialog::AlreadyWizzered);
	char *wizzed;
	while ( (wizzed = (char*)it.getNext()) ) {
	    if (EqualString (wizzed, this->parent_name)) {
		found = TRUE;
		break;
	    }
	}
    }

    if (!found) {
	this->TextEditDialog::post();

	//
	// Mark the parent as wizzered.
	//
	if (!WizardDialog::AlreadyWizzered) 
	    WizardDialog::AlreadyWizzered = new List;
	WizardDialog::AlreadyWizzered->appendElement
	    ((void*)DuplicateString(this->parent_name));
    }

}
