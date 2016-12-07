/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#include "NonimportableChoice.h"
#include "ButtonInterface.h"

#include <Xm/Xm.h>
#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/Separator.h>
#include <Xm/List.h>


String NonimportableChoice::DefaultResources[] = 
{
    NUL(char*)
};


#define IN_A_ROW 2
void NonimportableChoice::setAttachments
    (ButtonInterface *option[], int num_buts, Widget top)
{
    if ((num_buts % IN_A_ROW) == 0) {
	XtVaSetValues (option[num_buts]->getRootWidget(),
	    XmNleftAttachment, 	XmATTACH_FORM,
	    XmNleftOffset,	14,
	NULL);
    } else {
	XtVaSetValues (option[num_buts]->getRootWidget(),
	    XmNleftAttachment, 	XmATTACH_WIDGET,
	    XmNleftWidget,	option[num_buts-1]->getRootWidget(),
	    XmNleftOffset,	10,
	NULL);
    }
    if (num_buts == IN_A_ROW) {
	int i;
	for (i=0; i<IN_A_ROW; i++) {
	    XtVaSetValues (option[i]->getRootWidget(),
		XmNbottomAttachment, XmATTACH_NONE,
	    NULL);
	}
    }
    if (num_buts < IN_A_ROW) {
	XtVaSetValues (option[num_buts]->getRootWidget(),
	    XmNtopAttachment,	XmATTACH_WIDGET,
	    XmNtopWidget,	top,
	    XmNtopOffset,	8,
	    XmNbottomAttachment,XmATTACH_FORM,
	    XmNbottomOffset,	6,
	NULL);
    } else {
	XtVaSetValues (option[num_buts]->getRootWidget(),
	    XmNtopAttachment,	XmATTACH_WIDGET,
	    XmNtopWidget,	option[num_buts-2]->getRootWidget(),
	    XmNtopOffset,	4,
	    XmNbottomAttachment,XmATTACH_FORM,
	    XmNbottomOffset,	6,
	NULL);
    }
}

