/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#include "InteractorInstance.h"
#include "InteractorNode.h"
#include "InteractorStyle.h"
#include "Interactor.h"
#include "ControlPanel.h"
#include "Network.h"
#include "SetAttrDialog.h"
#include "WarningDialogManager.h"


InteractorInstance::InteractorInstance(InteractorNode *n)
{ 
    this->localLabel = NUL(char*);
    this->interactor = NUL(Interactor*);
    this->controlPanel = NUL(ControlPanel*);
    this->setAttrDialog = NUL(SetAttrDialog*);
    this->node = n;
    this->verticalLayout = this->defaultVertical();
    this->selected = FALSE;
    this->java_var_name = NUL(char*);
}
	
InteractorInstance::~InteractorInstance() 
{ 
    ASSERT(this->controlPanel);
    this->controlPanel->removeInteractorInstance(this);
    if (this->interactor)
	delete this->interactor;

    if (this->localLabel)
	delete this->localLabel; 

    if (this->setAttrDialog)
        delete this->setAttrDialog;
    if (this->java_var_name)
	delete this->java_var_name;
} 

void InteractorInstance::setXYPosition(int x, int y)	
{ 
    this->xpos  = x; this->ypos = y; 
    if (this->interactor)
    	this->interactor->setXYPosition(x,y);
} 
void InteractorInstance::setXYSize(int width, int height)	
{ 
    this->width  = width; this->height = height; 
    if (this->interactor)
    	this->interactor->setXYSize(width,height);
} 

void InteractorInstance::setVerticalLayout(boolean vertical) 
{

    if (vertical == this->verticalLayout)
	return;

    this->verticalLayout = vertical;

    if (this->interactor)
	this->interactor->setVerticalLayout(vertical);

    this->getNetwork()->setFileDirty();
}
void InteractorInstance::setSelected(boolean select) 
{ 
    this->selected = select; 
    if (this->interactor)
    	this->interactor->indicateSelect(select);
    ASSERT(this->controlPanel);
    this->controlPanel->handleInteractorInstanceStatusChange(this,
		select ?
		Interactor::InteractorSelected :
		Interactor::InteractorDeselected);
}

void InteractorInstance::setLocalLabel(const char *label) 
{ 
    if(this->localLabel)
	delete this->localLabel;

    if(IsBlankString(label))
    {
	this->localLabel = NULL;
	if(this->interactor)
	    this->interactor->setLabel(this->node->getInteractorLabel());
    }
    else
    {
    	this->localLabel = DuplicateString(label); 
	if(this->interactor)
    	    this->interactor->setLabel(this->localLabel);
    }

    this->node->getNetwork()->setFileDirty();
}

//
// Ask the InteractorStyle to build an Interactor for this instance.
//
void InteractorInstance::createInteractor() 
{ 
    ASSERT(this->style);
    if (!this->interactor)
	this->interactor = this->style->createInteractor(this);
    this->interactor->manage();
    this->interactor->indicateSelect(this->selected); 
}

void InteractorInstance::changeStyle(InteractorStyle *style)
{
    ASSERT(style);

    if(style == this->style)
	return;

    this->uncreateInteractor();

    this->setStyle(style);
    this->createInteractor();
    this->getNetwork()->setFileDirty();
}

const char *InteractorInstance::getInteractorLabel()
{ 
    return (this->localLabel ? this->localLabel : 
				this->node->getInteractorLabel());
}

Network  *InteractorInstance::getNetwork()   
{ 
    return this->node->getNetwork();
}
   

Widget InteractorInstance::getParentWidget()
{ 
    return this->controlPanel->getWorkSpaceWidget();
}

//
// Send notification to this->interactor that the attributes may have
// changed and to reflect these changes in the display Interactor.
// Some changes can be ignored if 'this != src_ii'.  This is typically
// called by InteractorNode::notifyAllInteractorsOfChangeAttributes().
//
void InteractorInstance::handleInteractorStateChange(
		InteractorInstance *src_ii, boolean unmanage)
{ 
    if (this->interactor)
    	this->interactor->handleInteractorStateChange(src_ii, unmanage);

    if (this->setAttrDialog && this->setAttrDialog->isManaged())
        this->setAttrDialog->refreshDisplayedAttributes();

}

//
// If the interactor for this instance exists, then return TRUE
// and the width and height in *x and *y respectively.
// If the interactor does not exists return FALSE and set *x and *y to 0.
//
boolean InteractorInstance::getXYSize(int *x, int *y)	
{
    if (this->interactor) {
	this->interactor->getXYSize(x,y);
	return TRUE;
    } else {
	*x = this->width;
	*y = this->height;
	return FALSE;
    }
}
void InteractorInstance::getXYPosition(int *x, int *y)	
{
    if (this->interactor) {
	this->interactor->getXYPosition(x,y);
	this->xpos = *x; 
	this->ypos = *y;  
    } else {
	*x = this->xpos; 
	*y = this->ypos;  
    }
} 
//
// Delete an interactor in such a way that we could create another one
// with createInteractor().
//
void InteractorInstance::uncreateInteractor()
{
    if (this->interactor) {
	int x,y;
	this->getXYPosition(&x,&y);// Save the position of the interactor.
        delete this->interactor;
        this->interactor = NULL;
	this->setXYSize (0,0);
    }

    // 
    // We always delete this dialog incase a change is being made to the
    // interactor that the dialog can't keep up with (i.e. a dimensionality
    // change).
    // 
    if (this->setAttrDialog) {
        delete this->setAttrDialog;
        this->setAttrDialog = NULL;
    }
 
}
//
// Return TRUE/FALSE indicating if this class of interactor instance has
// a set attributes dialog (i.e. this->newSetAttrDialog returns non-NULL).
//
boolean InteractorInstance::hasSetAttrDialog()
{
    return FALSE;
}
//
// Open the set attributes dialog for this interactor instance.
//
void InteractorInstance::openSetAttrDialog()
{

    if (!this->hasSetAttrDialog()) {
	WarningMessage(
	    "%s interactors do not have 'Set Attributes...' dialogs",
			    this->getNode()->getNameString());
	return;
    }

    if (!this->setAttrDialog) {
        this->setAttrDialog = this->newSetAttrDialog(
                                this->getControlPanel()->getRootWidget());
	ASSERT(this->setAttrDialog);
        this->setAttrDialog->post();
    } else {
        this->setAttrDialog->manage();
    }

}

//
// Create the set attributes dialog.
// This should also perform the function of this->loadSetAttrDialog().
// The default is that InteractorInstances do not have Set attributes dialogs.
//
SetAttrDialog *InteractorInstance::newSetAttrDialog(Widget parent )
{
    ASSERT(!this->hasSetAttrDialog());
    return NULL;
}
//
// Make sure the given output's current value complies with any attributes.
// This is called by InteractorInstance::setOutputValue() which is
// intern intended to be called by the Interactors.
// If verification fails (returns FALSE), then a reason is expected to
// placed in *reason.  This string must be freed by the caller. 
// At this level we always return TRUE (assuming that there are no
// attributes) and set *reason to NULL.
//
boolean InteractorInstance::verifyValueAgainstAttributes(int output, 
							const char *val,
							Type t,
							char **reason)
{
    if (*reason)
	*reason = NULL;
    return TRUE;
}
//
// Make sure the given value complies with any attributes and if so
// set the value in the Node.  This should generally be called from
// interactors that can't directly enforce  attributes.  For example,
// the Text style versus the Stepper style Vector interactor.  The
// Stepper style enforces its attributes itself, but the Text style
// accepts any value and then must have the value checked for type
// and attribute compliance.
// If we fail because attribute verification fails, then *reason contains
// the reason (as passed back by verifyValueAgainstAttributes()) for
// failure.  This string is expected to be freed by the caller.
//
boolean InteractorInstance::setAndVerifyOutput(int index, const char *val,
                                        Type type, boolean send,
					char **reason)
{
    Node *n = this->getNode();
    char *oldval = (char*)n->getOutputValueString(index);
    if (oldval) {
	// Save it in case the attribute verification fails.
	oldval = DuplicateString(oldval); 
    }

    boolean r = FALSE;
    if (reason)
	*reason = NULL;
    if ( (type = n->setOutputValue(index,val,type,FALSE)) ) {
	const char *coerced = n->getOutputValueString(index);
	if (this->verifyValueAgainstAttributes(index, coerced, type, reason)) {
	    if (send)
		n->sendValues();
	    r = TRUE;
	} else {	// Restore original state
	    n->setOutputValue(index,oldval,DXType::UndefinedType,FALSE);
	    n->clearOutputDirty(index);
	}
    }
    if (oldval)
	delete oldval;
    return r;
}


//
// The caller has already removed "this" from it's old ControlPanel.
// My responsiblity is to search the new network for the node I want
// to belong to.  The caller will then add "this" to the appropriate
// ControlPanel.
//
boolean InteractorInstance::switchNets (Network *newnet)
{
    // find my node and bid farewell so that I don't get hurt
    // when he is deleted.   I know that the instance numbers of my old node
    // and my new node will be the same, which allows me to find my new
    // node in the new net.
    InteractorNode *oldnode = this->node;
    ASSERT (oldnode);
    if (!oldnode->removeInstance (this)) return FALSE;
    InteractorNode *newnode = (InteractorNode *)
	newnet->findNode (oldnode->getNameSymbol(), oldnode->getInstanceNumber());
    ASSERT (newnode);
    if (!newnode->appendInstance (this)) return FALSE;
    this->node = newnode;

    return TRUE;
}


boolean InteractorInstance::printAsJava (FILE* jf)
{
    int x,y,w,h;
    this->getXYPosition (&x, &y);
    this->getXYSize (&w,&h);
    ControlPanel* cpan = this->getControlPanel();
    boolean devstyle = cpan->isDeveloperStyle();

    InteractorNode* ino = (InteractorNode*)this->node;
    const char* node_var_name = ino->getJavaVariable();

    //
    // Count up the lines in the label and split it up since java doesn't
    // know how to do this for us.
    //
    const char* ilab = this->getInteractorLabel();
    int line_count = CountLines(ilab);

    const char* java_style = "Interactor";
    if (this->style->hasJavaStyle())
	java_style = this->style->getJavaStyle();
    const char* var_name = this->getJavaVariable();
    fprintf (jf, "        %s %s = new %s();\n", java_style, var_name, java_style);
    fprintf (jf, "        %s.addInteractor(%s);\n", node_var_name, var_name);
    if (this->style->hasJavaStyle() == FALSE)
	fprintf (jf, 
	    "        %s.setUseQuotes(false);\n", var_name);
    fprintf (jf, "        %s.setStyle(%d);\n", var_name, devstyle?1:0);

    fprintf (jf, "        %s.setLabelLines(%d);\n", var_name, line_count);
    int i;
    for (i=1; i<=line_count; i++) {
	const char* cp = InteractorInstance::FetchLine (ilab, i);
	fprintf (jf, "        %s.setLabel(\"%s\");\n", var_name, cp);
    }

    if (this->isVerticalLayout())
	fprintf (jf, "        %s.setVertical();\n", var_name);
    else
	fprintf (jf, "        %s.setHorizontal();\n", var_name);

    if (this->style->hasJavaStyle())
	fprintf (jf, "        %s.setBounds (%s, %d,%d,%d,%d);\n", 
	    node_var_name, var_name, x,y,w,h);
    else
	fprintf (jf, "        %s.setBounds (%s, %d,%d,%d,%d);\n", 
	    node_var_name, var_name,x,y,w,65);

    fprintf (jf, "        %s.addInteractor(%s);\n", cpan->getJavaVariableName(), var_name);
	
    return TRUE;
}

int InteractorInstance::CountLines (const char* str)
{
    if ((!str) || (!str[0])) return 0;
    if ((str[0] == '\\') && (str[1] == '0')) return 0;
    int i;
    int lines = 1;
    for (i = 0; str[i] != '\0'; i++)
	if ((str[i] == 'n') && (i>0) && (str[i-1] == '\\'))
	    lines++;
    return lines;
}

const char* InteractorInstance::FetchLine (const char* str, int line)
{
    static char lbuf[256];
    lbuf[0] = '\0';
    ASSERT(str);
    int len = strlen(str);
    char* head = (char*)&str[0];
    char* tail = (char*)&str[len];
    int current_line = 1;
    char* cp = head;
    int next_slot = 0;
    while ((cp != tail) && (current_line <= line)) {
	if (*cp == '\\') {
	    cp++;
	    if (cp != tail) {
		if (*cp == 'n')
		    current_line++;
		cp++;
	    }
	    continue;
	}
	if (current_line == line) {
	    if (*cp == '"') 
		lbuf[next_slot++] = '\\';
	    lbuf[next_slot++] = *cp;
	    lbuf[next_slot] = '\0';
	}
	cp++;
    }
    return lbuf;
}

const char* InteractorInstance::getJavaVariable()
{
    if (this->java_var_name == NUL(char*)) {
	const char* var_part = this->javaName();
	const char* name_string = this->node->getNameString();
	this->java_var_name = new char[strlen(name_string) + strlen(var_part) + 16];
	int ino = this->node->getInstanceNumber();
	sprintf (this->java_var_name, "%s_%s_%d", var_part, name_string, ino);
    }
    return this->java_var_name;
}

