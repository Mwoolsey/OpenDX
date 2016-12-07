/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#include <stdlib.h>

#include "DXStrings.h"
#include "AnnotationGroupManager.h"
#include "Dictionary.h"
#include "ListIterator.h"
#include "List.h"
#include "SymbolManager.h"
#include "WarningDialogManager.h"

AnnotationGroupManager::AnnotationGroupManager(Network *net) :
    GroupManager(net, theSymbolManager->registerSymbol(ANNOTATION_GROUP))
{
}


boolean AnnotationGroupManager::printComment(FILE* f)
{
    int count = this->groups.getSize();
    int i;

    for (i=1; i<=count; i++) {
	const char *group_name = this->groups.getStringKey(i);
	if (fprintf (f, "// annotation assignment: %s\n", group_name) <= 0)
	    return FALSE;
    }

    return TRUE;

}

boolean AnnotationGroupManager::parseComment(const char *comment,
                                const char *filename, int lineno,Network *net)
{
    char name[128];
    char *cp = " annotation assignment:";

    if (!EqualSubstring(cp, comment,strlen(cp)))
	return FALSE;

    int items_parsed =
	sscanf (comment, " annotation assignment: %[^\n]", name);
    if (items_parsed != 1) {
	WarningMessage ("Unrecognized page (file %s, line %d)", filename, lineno);
	return FALSE;
    }

    if (!this->createGroup (name, net))
	return FALSE;

    return TRUE;
}

