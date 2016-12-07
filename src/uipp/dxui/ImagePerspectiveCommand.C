/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#include "ImagePerspectiveCommand.h"
#include "ImageWindow.h"

ImagePerspectiveCommand::ImagePerspectiveCommand(const char   *name,
				   CommandScope *scope,
				   boolean       active,
				   ImageWindow  *w,
				   boolean       enable):
    NoUndoCommand(name, scope, active)
{
    this->imageWindow = w;
    this->enablePerspective = enable;
}

boolean
ImagePerspectiveCommand::doIt(CommandInterface *ci)
{
    return this->imageWindow->enablePerspective(this->enablePerspective);
}
