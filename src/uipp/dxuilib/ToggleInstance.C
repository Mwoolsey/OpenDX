/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#include "ToggleInstance.h"
#include "ToggleNode.h"
#include "ControlPanel.h"
#include "ToggleAttrDialog.h"


ToggleInstance::ToggleInstance(ToggleNode *n) :
                InteractorInstance((InteractorNode*)n)
{
}
	
ToggleInstance::~ToggleInstance() 
{
}
//
// Return TRUE/FALSE indicating if this class of interactor instance has
// a set attributes dialog (i.e. this->newSetAttrDialog returns non-NULL).
//
boolean ToggleInstance::hasSetAttrDialog()
{
    return TRUE;
}

//
// Create the default set attributes dialog box for this type of interactor.
//
SetAttrDialog *ToggleInstance::newSetAttrDialog(Widget parent)
{
    ToggleAttrDialog *d = new ToggleAttrDialog(parent,
                                        "Set Attributes...", this);
    return (SetAttrDialog*)d;
}

boolean ToggleInstance::printAsJava (FILE* jf)
{
    const char* indent = "        ";
    if (this->style->hasJavaStyle() == FALSE)
	return InteractorInstance::printAsJava(jf);

    int x,y,w,h;
    this->getXYPosition (&x, &y);
    this->getXYSize (&w,&h);
    ControlPanel* cpan = this->getControlPanel();
    boolean devstyle = cpan->isDeveloperStyle();
    ToggleNode* tn = (ToggleNode*)this->getNode();
    const char* node_var_name = tn->getJavaVariable();

    const char* ilab = this->getInteractorLabel();
    int line_count = CountLines(ilab);

    fprintf (jf, "\n");
    const char* java_style = this->style->getJavaStyle();
    ASSERT(java_style);
    const char* var_name = this->getJavaVariable();
    fprintf (jf, "%s%s %s = new %s();\n", indent, java_style, var_name, java_style);
    fprintf (jf, "        %s.addInteractor(%s);\n", node_var_name, var_name);

    fprintf (jf, "%s%s.setStyle(%d);\n", indent, var_name, devstyle?1:0);
    fprintf (jf, "%s%s.setLabelLines(%d);\n", indent, var_name, line_count);
    int i;
    for (i=1; i<=line_count; i++) {
        const char* cp = FetchLine (ilab, i);
        fprintf (jf, "%s%s.setLabel(\"%s\");\n", indent, var_name, cp);
    }

    if (this->isVerticalLayout())
        fprintf (jf, "%s%s.setVertical();\n", indent, var_name);
    else
        fprintf (jf, "%s%s.setHorizontal();\n", indent, var_name);
    if (this->style->hasJavaStyle())
        fprintf (jf, "%s%s.setBounds (%s, %d,%d,%d,%d);\n", 
	    indent,node_var_name,var_name,x,y,w,h);
    else
        fprintf (jf, "%s%s.setBounds (%s, %d,%d,%d,%d);\n", 
	    indent,node_var_name,var_name,x,y,w,65);

    tn->printJavaType(jf, indent, var_name);

    fprintf(jf, "        %s.addInteractor(%s);\n", cpan->getJavaVariableName(), var_name);

    return TRUE;
}

void ToggleInstance::setVerticalLayout(boolean )
{

    if (FALSE == this->verticalLayout)
	return;
    this->verticalLayout = FALSE;
}

