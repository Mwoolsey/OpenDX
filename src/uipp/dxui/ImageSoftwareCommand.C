/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#include "ImageSoftwareCommand.h"
#include "ImageWindow.h"

ImageSoftwareCommand::ImageSoftwareCommand(const char   *name,
						 CommandScope *scope,
						 boolean       active,
						 ImageWindow  *w):
    NoUndoCommand(name, scope, active)
{
    this->imageWindow = w;
}

boolean ImageSoftwareCommand::doIt(CommandInterface *ci)
{
    Boolean set;
    XtVaGetValues(ci->getRootWidget(), XmNset, &set, NULL);
    this->imageWindow->setSoftware(set);
    return TRUE;
}
