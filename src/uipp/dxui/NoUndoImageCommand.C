/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#include "NoUndoImageCommand.h"
#include "ImageWindow.h"
#include "ImageNode.h"
#include "PanelAccessManager.h"
#include "ControlPanelAccessDialog.h"

NoUndoImageCommand::NoUndoImageCommand(const char*   name,
				       CommandScope* scope,
				       boolean       active,
				       ImageWindow  *image,
				       ImageCommandType comType):
    NoUndoCommand(name, scope, active)
{
    this->commandType = comType;
    this->image = image;
}


boolean NoUndoImageCommand::doIt(CommandInterface *ci)
{
    ImageWindow *image = this->image;
    boolean  ret = TRUE;
    ImageNode *in = (ImageNode*)image->node;

    ASSERT(image);

    switch (this->commandType) {
    case NoUndoImageCommand::SetBGColor:
	image->postBackgroundColorDialog();
	break;
    case NoUndoImageCommand::DisplayGlobe:
        image->setDisplayGlobe();
        break;
    case NoUndoImageCommand::ViewControl:
	ret = image->postViewControlDialog();
	break;
    case NoUndoImageCommand::RenderingOptions:
	ret = image->postRenderingOptionsDialog();
	break;
    case NoUndoImageCommand::Throttle:
	ret = image->postThrottleDialog();
	break;
    case NoUndoImageCommand::ChangeImageName:
	ret = image->postChangeImageNameDialog();
	break;
    case NoUndoImageCommand::AutoAxes:
	ret = image->postAutoAxesDialog();
	break;
    case NoUndoImageCommand::OpenVPE:
	ret = image->postVPE();	
	break;
    case NoUndoImageCommand::Depth8:
	if( in && image->imageDepth8Option->getState() )
	    image->changeDepth(8);
	break;
    case NoUndoImageCommand::Depth12:
	if( in && image->imageDepth12Option->getState() )
	    image->changeDepth(12);
	break;
    case NoUndoImageCommand::Depth15:
	if( in && image->imageDepth15Option->getState() )
	    image->changeDepth(15);
	break;
    case NoUndoImageCommand::Depth16:
	if( in && image->imageDepth16Option->getState() )
	    image->changeDepth(16);
	break;
    case NoUndoImageCommand::Depth24:
	if( in && image->imageDepth24Option->getState() )
	    image->changeDepth(24);
	break;
    case NoUndoImageCommand::Depth32:
	if( in && image->imageDepth32Option->getState() )
	    image->changeDepth(32);
	break;
    case NoUndoImageCommand::SetCPAccess:
	image->postPanelAccessDialog(
		((DisplayNode*)image->node)->getPanelManager());
	break;
    case NoUndoImageCommand::SaveImage:
	image->saveImage();
	break;
    case NoUndoImageCommand::SaveAsImage:
	 ret = image->postSaveImageDialog();
	break;
    case NoUndoImageCommand::PrintImage:
	 ret = image->postPrintImageDialog();
	break;
    default:
	ASSERT(0);
    }

    return ret;
}
