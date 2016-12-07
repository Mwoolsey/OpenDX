/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#include "CommandTextPopup.h"
#include "Command.h"
#include "GARChooserWindow.h"
#include "DXStrings.h"
#include <Xm/Text.h>
#include <Xm/PushB.h>
#include "GARApplication.h"
#include "ListIterator.h"

#ifndef DXD_NON_UNIX_ENV_SEPARATOR
#define SEP_CHAR ':'
#else
#define SEP_CHAR ';'
#endif

boolean CommandTextPopup::ClassInitialized = FALSE;

String CommandTextPopup::DefaultResources[] = {
    NUL(char*)
};

CommandTextPopup::CommandTextPopup (GARChooserWindow* chooser) : TextPopup ("fileCommand")
{
    this->chooser = chooser;
    this->added_names = NUL(List*);
    this->file_timer = 0;
    this->lru_index = -1;
    this->dir_hist_size = 0;
}

CommandTextPopup::~CommandTextPopup()
{
    if (this->added_names) delete this->added_names;
    if (this->file_timer) 
	XtRemoveTimeOut (this->file_timer);
}

void CommandTextPopup::initialize()
{
}

Widget 
CommandTextPopup::createTextPopup(Widget parent, const char **items, int nitems,
	SetTextCallback stc, ChangeTextCallback ctc, void *callData)
{
char help_msg[128];
int i, len;
#define LENLIM 30
#define MAXITEMS 20
char *more_items[MAXITEMS];

    const char *dxd = (char *)getenv("DXDATA");
    if ((dxd) && (dxd[0])) {
	len = strlen(dxd);
	if (len > LENLIM) {
	    char *tmpstr = DuplicateString (dxd);
	    tmpstr[LENLIM] = '\0';
	    sprintf (help_msg, "The raw data file. (DXDATA in use: %s...)", 
		tmpstr);
	    delete tmpstr;
	} else
	    sprintf (help_msg, "The raw data file. (DXDATA in use: %s)", dxd);
    } else {
	sprintf (help_msg, "The raw data file.");
	len = 0;
    }
#undef LENLIM

    //
    // We'll always add uiroot/samples/data
    //
    int next = 0;
    if (len) {
    	char *head = NUL(char*);
    	char *top_head = NUL(char*);
	char *cp = top_head = DuplicateString(dxd);
#if defined(intelnt)
/* Convert to Unix if DOS */
	for(i=0; i<len; i++)
	    if(cp[i] == '\\') cp[i] = '/';
#endif
	for (i=0; i<len; i++) {
	    if (!head) head = &cp[i];
	    if (cp[i]==SEP_CHAR) {
		cp[i] = '\0';
		more_items[next++] = DuplicateString(head);
		head = NUL(char*);
		if (next == (MAXITEMS-3)) break;
	    }
	}
	more_items[next++] = DuplicateString(head);
	delete top_head;
    } 
    const char *uiroot = theGARApplication->getUIRoot();
    if ((!uiroot) || (!uiroot[0]))
	uiroot = (char *)getenv("DXROOT");
    if ((!uiroot) || (!uiroot[0]))
	uiroot = "/usr/local/dx";

    char *sampDat = new char[strlen(uiroot) + strlen("samples/data") + 16];
    strcpy(sampDat, uiroot);
#if defined(intelnt)
/* Convert to Unix if DOS */
	for(i=0; i<strlen(sampDat); i++)
	    if(sampDat[i] == '\\') sampDat[i] = '/';
#endif
    if (sampDat[strlen(uiroot)-1] == '/') {
	strcat(sampDat, "samples/data");
    } else {
	strcat(sampDat, "/samples/data");
    }

    //
    // if DXROOT/samples/data isn't already in DXDATA, then we'll add it, too.
    //
    boolean duplicated = FALSE;
    for (i=0; i<next; i++) {
	if (EqualString(sampDat, more_items[i])) {
	    duplicated = TRUE;
	    break;
	}
    }


    if (duplicated == FALSE) {
	more_items[next++] = sampDat;
	sampDat = NUL(char*);
    } else {
	delete sampDat;
    }

    for (i=0; i<nitems; i++) {
	if (next == MAXITEMS) break;
	more_items[next++] = DuplicateString(items[i]);
    }
    this->lru_index = next;

    Widget w = this->TextPopup::createTextPopup 
	(parent, (const char **)more_items, next, stc, ctc, callData);

    this->added_names = new List;
    for (i=0; i<next; i++)
	this->added_names->appendElement((void*)more_items[i]);

    this->setBubbleHelp (help_msg, this->textField);

    XmTextSource source = (XmTextSource)theGARApplication->getDataFileSource();
    if (source) {
	XmTextSetSource (this->textField, source, 0, 0);
    } else {
	source = XmTextGetSource(this->textField);
	theGARApplication->setDataFileSource((void*)source);
    }

    this->addCallback(this->textField);
    return w;
}



void CommandTextPopup::activateTextCallback(const char *file_name, void *)
{
int i;
char *cp;

    if (this->file_timer)
	XtRemoveTimeOut (this->file_timer);

    this->chooser->fileNameCB();

    //
    // If the filename contained a directory add it to a short list displayed in the menu
    //
    if ((!file_name) || (!file_name[0])) return ;
    char *base_name = DuplicateString(file_name);
#if defined(intelnt)
/* Convert to Unix if DOS */
	for(i=0; i<strlen(base_name); i++)
	    if(base_name[i] == '\\') base_name[i] = '/';
#endif
    int len = strlen(base_name);
    for (i=len-1; i>0; i--) {
	if (base_name[i] == '/') {
	    base_name[i] = '\0';
	    break;
	}
    }
    if (i>1) {
	//
	// base_name contains a directory now
	//
	boolean match_found = FALSE;
	if (!this->added_names) this->added_names = new List;
	ListIterator it(*this->added_names);
	while (cp = (char *)it.getNext()) {
	    if ((EqualString (cp, base_name)) || (EqualString(cp, file_name))) {
		match_found = TRUE;
		break;
	    }
	}

	//
	// Don't let than the list get longer than...
	//
	// It's possible that the list is already longer than MAX_LIST_CNT
	// anyway, if DXDATA contained a huge number of paths.
	//
#define MAX_LIST_CNT 15
	if ((!match_found) && (this->added_names->getSize() <= MAX_LIST_CNT)) {
	    //
	    // If we're at the upper limit already, then get rid of one
	    // string from the list and one widget from the menu.
	    //
	    if (this->added_names->getSize() == MAX_LIST_CNT) {
		Widget kid, *kids;
		int nkids;
		XtVaGetValues (this->popupMenu, XmNchildren, &kids, 
		    XmNnumChildren, &nkids, NULL);
		ASSERT (nkids == MAX_LIST_CNT);
		kid = kids[this->lru_index];
		XtDestroyWidget (kid);

		char* trash = (char*)this->added_names->getElement(this->lru_index+1);
		this->added_names->deleteElement(this->lru_index+1);
		delete trash;
	    }

	    Widget button;
	    button = XtVaCreateManagedWidget (base_name,
		xmPushButtonWidgetClass, 	this->popupMenu,
	    NULL);
	    XtAddCallback(button, XmNactivateCallback,
		(XtCallbackProc)TextPopup_ItemSelectCB, (XtPointer)this);
	    this->added_names->appendElement ((void*)base_name);
	    base_name = NUL(char*);
	}
#undef MAX_LIST_CNT
    }
    if (base_name) delete base_name;
}

void CommandTextPopup::waitForFile ()
{
    if (this->file_timer)
	XtRemoveTimeOut (this->file_timer);

    this->file_timer = 0;
    if (this->chooser->getChoice())
	this->file_timer = XtAppAddTimeOut (theApplication->getApplicationContext(), 
	    500, (XtTimerCallbackProc)CommandTextPopup_FileTO, (XtPointer)this);
}

void CommandTextPopup::itemSelectCallback(const char *value)
{
    this->chooser->setFileSearchDir(value);
    Command* cmd = this->chooser->getSelectDataCmd();
    cmd->execute();
}

void CommandTextPopup::addCallback (Widget t, XtCallbackProc cbp)
{
    //
    // Set a timer to go off after the text is modified
    //
    XtAddCallback (t, XmNmodifyVerifyCallback, (XtCallbackProc)
	CommandTextPopup_ModifyCB, (XtPointer)this);
    if (cbp) 
	XtAddCallback (this->textField, XmNmodifyVerifyCallback, cbp, (XtPointer)this);
}


extern "C" {

void CommandTextPopup_ModifyCB (Widget , XtPointer clientData, XtPointer)
{
    CommandTextPopup *ctp = (CommandTextPopup*)clientData;
    ASSERT(ctp);
    ctp->waitForFile();
}

void CommandTextPopup_FileTO (XtPointer clientData, XtIntervalId* )
{
    CommandTextPopup* ctp = (CommandTextPopup*)clientData;
    ASSERT(ctp);
    ctp->file_timer = 0;
    ctp->chooser->fileNameCB();
}

} // end extern C
