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
#include "ExecCommandDialog.h"

#include "DXApplication.h"
#include "DXPacketIF.h"
#include "MsgWin.h"
#include "lex.h"

boolean ExecCommandDialog::ClassInitialized = FALSE;
String ExecCommandDialog::DefaultResources[] =
{
    "*dialogTitle:               	Executive Script Command...", 
    "*nameLabel.labelString:            Script Command:", 
    NULL
};


ExecCommandDialog::ExecCommandDialog(Widget parent) :
    SetNameDialog("execCommandDialog", parent, TRUE)
{
    this->lastCommand = NULL;

    if (NOT ExecCommandDialog::ClassInitialized)
    {
        ExecCommandDialog::ClassInitialized = TRUE;
	this->installDefaultResources(theApplication->getRootWidget());
    }
}

ExecCommandDialog::~ExecCommandDialog()
{
    if (this->lastCommand)
	delete this->lastCommand;
}

//
// Install the default resources for this class.
//
void ExecCommandDialog::installDefaultResources(Widget  baseWidget)
{
    this->setDefaultResources(baseWidget, ExecCommandDialog::DefaultResources);
    this->SetNameDialog::installDefaultResources( baseWidget);
}

const char *ExecCommandDialog::getText()
{
    if (this->lastCommand)
	return this->lastCommand; 
    else
	return ""; 
}

void ExecCommandDialog::saveLastCommand(const char *s)
{
    if (this->lastCommand)
	delete this->lastCommand;
    this->lastCommand = DuplicateString(s);
}

boolean ExecCommandDialog::saveText(const char *s)
{
    if (!IsBlankString(s)) {
	DXPacketIF *p = theDXApplication->getPacketIF();
	if (p) {
	    int i = 0; 
	    SkipWhiteSpace(s, i);
	    if (s[i] == '$') {	
		theDXApplication->getMessageWindow()->addInformation(&s[i]);
		this->saveLastCommand(&s[i]);
		p->sendImmediate(&s[i+1]);
	    } else {
		char *c = new char[STRLEN(s) + 2];
		this->saveLastCommand(s);
		sprintf(c,"%s\n",s);
		theDXApplication->getMessageWindow()->addInformation(c);
		p->send(DXPacketIF::FOREGROUND, c);
		delete c;
	    }
	}
    }
    return TRUE;
}

