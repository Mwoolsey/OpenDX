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
#include "PageGroupManager.h"
#include "Dictionary.h"
#include "SymbolManager.h"
#include "WarningDialogManager.h"
#include "UIComponent.h"

PageGroupManager::PageGroupManager(Network *net) :
    GroupManager(net, theSymbolManager->registerSymbol(PAGE_GROUP))
{
}


boolean PageGroupManager::printComment(FILE *f)
{
    int count = this->groups.getSize();
    int i;

    //
    //
    for (i=1; i<=count; i++) {
	const char *group_name = this->groups.getStringKey(i);
	PageGroupRecord *prec = (PageGroupRecord*)this->groups.getDefinition(i);
	if (fprintf (f, "// page assignment: %s\torder=%d, windowed=%d, showing=%d\n",
	    group_name, prec->order_in_list, prec->windowed, prec->showing) <= 0)
	    return FALSE;
    }

    return TRUE;
}


//
// Parse the group information from the cfg file.
//
boolean PageGroupManager::parseComment(const char *comment,
                                const char *filename, int lineno,Network *net)
{
    int order, windowed, showing;
    char name[128];
    char *cp = " page assignment:";

    if (!EqualSubstring(cp, comment,strlen(cp)))
	return FALSE;

    int items_parsed = 
	sscanf (comment, " page assignment: %[^\t]\torder=%d, windowed=%d, showing=%d", 
	    name, &order, &windowed, &showing);
    if (items_parsed != 4) {
	WarningMessage ("Unrecognized page (file %s, line %d)", filename, lineno);
	return FALSE;
    }

    if (!this->createGroup (name, net))
	return FALSE;

    PageGroupRecord *prec = (PageGroupRecord*)this->getGroup(name);
    prec->order_in_list = order;
    prec->windowed = windowed;
    prec->showing = showing;

    return TRUE;
}



void PageGroupRecord::setComponents (UIComponent* window, UIComponent* ws)
{
    this->page_window = window;
    this->workspace = ws;
}
