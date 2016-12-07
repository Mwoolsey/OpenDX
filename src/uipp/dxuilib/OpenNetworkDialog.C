/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"






#include <stdio.h>

#include "DXStrings.h"
#include "DXApplication.h"
#include "OpenNetworkDialog.h"
#include "OpenNetCommentDialog.h"
#include "ErrorDialogManager.h"
#include "InfoDialogManager.h"
#include "Network.h"



boolean OpenNetworkDialog::ClassInitialized = FALSE;

String OpenNetworkDialog::DefaultResources[] =
{
        "*dialogTitle:     Open...",
        "*dirMask:         *.net",
        "*helpLabelString: Comments",
        NULL
};

OpenNetworkDialog::OpenNetworkDialog(Widget parent) : 
                       FileDialog("openNetworkDialog", parent)
{
    this->hasCommentButton = TRUE;
    this->netCommentDialog = NULL;

    if (NOT OpenNetworkDialog::ClassInitialized)
    {
        OpenNetworkDialog::ClassInitialized = TRUE;
	this->installDefaultResources(theApplication->getRootWidget());
    }
}
OpenNetworkDialog::OpenNetworkDialog(const char *name, Widget parent) : 
                       FileDialog(name, parent)
{
    this->hasCommentButton = TRUE;
    this->netCommentDialog = NULL;
}

OpenNetworkDialog::~OpenNetworkDialog() 
{
    if (this->netCommentDialog)
	delete this->netCommentDialog;
}

//
// Install the default resources for this class.
//
void OpenNetworkDialog::installDefaultResources(Widget  baseWidget)
{
    this->setDefaultResources(baseWidget, OpenNetworkDialog::DefaultResources);
    this->FileDialog::installDefaultResources( baseWidget);
}
#if 0
void OpenNetworkDialog::unmanage(const char *string)
{
    if (this->netCommentDialog)
	this->netCommentDialog->unmanage();
}
#endif
void OpenNetworkDialog::okFileWork(const char *string)
{
    theDXApplication->getExecCtl()->terminateExecution();
    theDXApplication->openFile(string);
}

void OpenNetworkDialog::helpCallback(Dialog* dialog)
{
#define BUFSIZE UI_YYLMAX 
#define REALLOC_CHUNK	512
    char *f = this->getSelectedFileName();
    char buffer[BUFSIZE];
    boolean parsing = FALSE, done = FALSE;
    char *pc = NULL, *comment = NULL, *netfile = NULL; 
    int comment_len=0, maxlen=0;
    FILE *fp = NULL;
    char *errmsg = "";
    boolean wasEncoded;
 
    if (!f) {
	ModalErrorMessage(this->getRootWidget(),"A file must be selected.");
	goto out;
    }

    netfile = Network::FilenameToNetname(f);

    fp = Network::OpenNetworkFILE(netfile,&wasEncoded,&errmsg);
    if (!fp) {
	ModalErrorMessage(this->getRootWidget(),errmsg);
	if (errmsg[0]) delete errmsg;
	goto out;
    }

    while (!done && fgets(buffer, BUFSIZE, fp)) {
	if (EqualSubstring(buffer,"// comment: ",12)) {
	    char *p = buffer + 12;
	    int  len = STRLEN(p) + 1;	
	    if (comment_len+len > maxlen) {
	        int max = comment_len+len;
		maxlen += REALLOC_CHUNK;
		if (max > maxlen)
		    maxlen = max;
		comment = (char*)REALLOC(comment,maxlen*sizeof(char));
		pc = comment;
		if (comment_len == 0) {
		    *pc = '\0';
		    parsing = TRUE;
	 	}
	    }
	    strcat(pc, p);
	    comment_len += len;
	    pc += STRLEN(pc); 
	} else if (parsing) {
	    done = TRUE;
	}
    }
    Network::CloseNetworkFILE(fp,wasEncoded);


    if (!comment) {
	ModalInfoMessage(this->getRootWidget(),
		"No comments were found in file %s", netfile);
	goto out;
    }

    if (!this->netCommentDialog) {
	this->netCommentDialog = 
	    new OpenNetCommentDialog(this->getRootWidget());
        this->netCommentDialog->post();
    } else {
	this->netCommentDialog->manage();
    }
    this->netCommentDialog->installEditorText(comment);

out:
    if (f) 	 delete f;
    if (netfile) delete netfile;
    if (comment) delete comment;
}
