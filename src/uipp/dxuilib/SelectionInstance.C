/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#include "SelectionNode.h"
#include "SelectionInstance.h"
#include "SelectionAttrDialog.h"


boolean SelectionInstance::appendOptionPair(const char *value, const char *name)
{
    SelectionNode *n = (SelectionNode*)this->getNode();
    return n->appendOptionPair(value,name);
}
int  SelectionInstance::getOptionCount()
{
    SelectionNode *n = (SelectionNode*)this->getNode();
    return n->getOptionCount();
}
const char  *SelectionInstance::getValueOptionsAttribute()
{
    SelectionNode *n = (SelectionNode*)this->getNode();
    return n->getValueOptionsAttribute();
}
char *SelectionInstance::getOptionValueString(int optind)
{
    SelectionNode *n = (SelectionNode*)this->getNode();
    return n->getOptionValueString(optind);
}

char *SelectionInstance::getOptionNameString(int optind, boolean keep_quotes)
{
    SelectionNode *n = (SelectionNode*)this->getNode();
    return n->getOptionNameString(optind, keep_quotes);
}

//
// Return TRUE/FALSE indicating if this class of interactor instance has
// a set attributes dialog (i.e. this->newSetAttrDialog returns non-NULL).
//
boolean SelectionInstance::hasSetAttrDialog()
{
    return TRUE;
}

//
// Create the default  set attributes dialog box for this class of
// Interactor.
//
SetAttrDialog *SelectionInstance::newSetAttrDialog(Widget parent)
{
    Node *n = this->getNode();
    char *name = (char*)n->getClassName();
    ASSERT(name);
    name = DuplicateString(name);
    char *p = strstr(name,"Node");
    if (p)
	*p = '\0';
    char title[128];
    sprintf(title,"Set %s Attributes...",name);
    SetAttrDialog *d = new SelectionAttrDialog(parent, title, this);
    delete name;
    return d;
}


