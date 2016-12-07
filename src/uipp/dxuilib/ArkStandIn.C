/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#include "ArkStandIn.h"
#include "../widgets/WorkspaceW.h"

//
// Print a representation of the stand in on a PostScript device.  We
// assume that the device is in the same coordinate space as the parent
// of this uicomponent (i.e. we send the current geometry to the 
// PostScript output file).  
//
boolean ArkStandIn::printAsPostScript(FILE *f)
{
    int i, *x = NULL, *y = NULL;

    if (this->line) {
	int points = XmWorkspaceLineGetPath(this->line,&x,&y);
	if (points > 1) {
	    for (i=0 ; i<points  ; i++) {
		if (fprintf(f,"%d %d ",x[i],y[i]) <= 0)
		    goto error;
		if (i == 0) { 
		    if (fprintf(f,"moveto ") <= 0)
			goto error;
		} else {
		    if (fprintf(f,"lineto ") <= 0)
			goto error;
		}
	    }
	    
	    if (fprintf(f,"stroke\n") <= 0)
		goto error;
	}

	if (x) XtFree((char*)x);
	if (y) XtFree((char*)y);
    }

    return TRUE;

error:
    if (x) XtFree((char*)x);
    if (y) XtFree((char*)y);

    return FALSE;
}

