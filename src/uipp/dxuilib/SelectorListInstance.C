/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#include "ControlPanel.h"
#include "SelectorNode.h"
#include "SelectorListInstance.h"


boolean SelectorListInstance::printAsJava (FILE* jf)
{
    const char* indent = "        ";
    if (this->style->hasJavaStyle() == FALSE)
	return InteractorInstance::printAsJava(jf);

    int x,y,w,h;
    this->getXYPosition (&x, &y);
    this->getXYSize (&w,&h);
    ControlPanel* cpan = this->getControlPanel();
    boolean devstyle = cpan->isDeveloperStyle();
    const char* ilab = this->getInteractorLabel();
    int line_count = CountLines(ilab);

    InteractorNode* ino = (InteractorNode*)this->node;
    const char* node_var_name = ino->getJavaVariable();

    fprintf (jf, "\n");
    const char* java_style = this->style->getJavaStyle();
    ASSERT(java_style);
    const char* var_name = this->getJavaVariable();
    fprintf (jf, "%s%s %s = new %s();\n", indent, java_style, var_name, java_style);
    fprintf (jf, "        %s.addInteractor(%s);\n", node_var_name, var_name);

    fprintf (jf, "%s%s.setStyle(%d);\n", indent, var_name, devstyle?1:0);
    fprintf (jf, "%s%s.setMode(true);\n", indent, var_name);
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
	    indent, node_var_name, var_name, x,y,w,h);
    else
	fprintf (jf, "%s%s.setBounds (%s, %d,%d,%d,%d);\n", 
	    indent, node_var_name, var_name, x,y,w,65);

    SelectionNode* n = (SelectionNode*)this->node;
    n->printJavaType(jf, indent, var_name);

    fprintf(jf, "        %s.addInteractor(%s);\n", cpan->getJavaVariableName(), var_name);

    return TRUE;
}

