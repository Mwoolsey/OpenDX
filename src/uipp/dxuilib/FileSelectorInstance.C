/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#include "FileSelectorInstance.h"
#include "FileSelectorNode.h"
#include "ControlPanel.h"

FileSelectorInstance::FileSelectorInstance(FileSelectorNode *n) : 
				ValueInstance(n)
{ 
    this->fileFilter = NULL;
}
	
FileSelectorInstance::~FileSelectorInstance() 
{ 
    if (this->fileFilter)
	delete this->fileFilter;
} 
void FileSelectorInstance::setFileFilter(const char *filter) 
{
    if (this->fileFilter)
	delete this->fileFilter;

#ifdef DXD_WIN
    char *cf;
    int i;
    cf = (char *) malloc ((strlen(filter) +1)*sizeof(char));
    strcpy(cf, filter);
    for(i=0; i<strlen(cf); i++)
	if(cf[i] == '/') cf[i] = '\\';
    this->fileFilter = DuplicateString(cf);
    free(cf);
#else
    this->fileFilter = DuplicateString(filter);
#endif
}

boolean FileSelectorInstance::printAsJava (FILE* jf)
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
	fprintf (jf, "        %s.setUseQuotes(true);\n", var_name);
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

