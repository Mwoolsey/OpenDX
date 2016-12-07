/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"

#include <sys/types.h>

#if 0
#if defined(windows) && defined(HAVE_WINSOCK_H)
#include <winsock.h>
#elif defined(HAVE_CYGWIN_SOCKET_H)
#include <cygwin/socket.h>
#elif defined(HAVE_SYS_SOCKET_H)
#include <sys/socket.h>
#endif
#endif

#if defined(HAVE_SYS_UTSNAME_H)
#include <sys/utsname.h>
#endif

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#ifdef OS2
#include <netdb.h>
#endif

#if defined(sgi)
#include <CC/osfcn.h>
#endif
#if defined(sun4)
#include <sysent.h>
#endif


#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>

#include <Xm/CascadeB.h>
#include <Xm/DrawingA.h>
#include <Xm/Frame.h>
#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/List.h>
#include <Xm/RowColumn.h>
#include <Xm/ScrolledW.h>
#include <Xm/Separator.h>
#include <Xm/SelectioB.h>
#include <Xm/Text.h>

#include "ImageWindow.h"
#include "DXApplication.h"
#include "MsgWin.h"
#include "Network.h"
#include "ImageNode.h"
#include "ButtonInterface.h"
#include "ToggleButtonInterface.h"
#include "NoOpCommand.h"
#include "NoUndoImageCommand.h"
#include "ImageApproxCommand.h"
#include "ImageConstraintCommand.h"
#include "ImageLookCommand.h"
#include "ImagePerspectiveCommand.h"
#include "ImageRedoCommand.h"
#include "ImageResetCommand.h"
#include "ImageSetModeCommand.h"
#include "ImageSetViewCommand.h"
#include "ImageHardwareCommand.h"
#include "ImageSoftwareCommand.h"
#include "ImageUndoCommand.h"
#include "DXChild.h"
#include "Stack.h"

#include "ErrorDialogManager.h"
#include "ViewControlDialog.h"
#include "RenderingOptionsDialog.h"
#include "SetBGColorDialog.h"
#include "SetImageNameDialog.h"
#include "WarningDialogManager.h"
#include "ThrottleDialog.h"
#include "AutoAxesDialog.h"
#include "SaveImageDialog.h"
#include "PrintImageDialog.h"
#if WORKSPACE_PAGES
#include "ProcessGroupManager.h"
#endif

#include "ProbeNode.h"
#include "PickNode.h"
#include "Parameter.h"
#include "EditorWindow.h"
#include "XHandler.h"
#include "DXStrings.h"
#include "ListIterator.h"
#include "CloseWindowCommand.h"
#include "PanelAccessManager.h"
#include "../widgets/Picture.h"
#include "TransferAccelerator.h"

#include "CascadeMenu.h"

#ifdef ABS_IN_MATH_H
# define abs __Dont_define_abs
#endif
#include <math.h>
#ifdef ABS_IN_MATH_H
# undef abs
#endif

#ifdef NEEDS_GETHOSTNAME_DECL
extern "C" int gethostname(char *,int);
#endif

#if defined(DXD_WIN)
#include <winsock.h>
#endif

#ifndef OS2
extern "C" unsigned int sleep(unsigned int);
#endif

// Defines for the where parameter
#define FB_WHERE_SWAP	0x00000001

boolean ImageWindow::NeedsSyncForResize = FALSE;

#include "IWDefaultResources.h"

boolean ImageWindow::ClassInitialized = FALSE;


ImageWindow::ImageWindow(boolean  isAnchor, Network* network) : 
			DXWindow("imageWindow", isAnchor,
				theDXApplication->appAllowsImageMenus())
{
    ASSERT(network);

    //
    // Save associated network and add self to network image list.
    //
    this->network = network;
    this->network->addImage(this);
   
    this->viewControlDialog      = NULL;
    this->renderingOptionsDialog = NULL;
    this->backgroundColorDialog  = NULL;
    this->throttleDialog	 = NULL;
    this->autoAxesDialog	 = NULL;
    this->changeImageNameDialog  = NULL;
    this->saveImageDialog      = NULL;
    this->printImageDialog       = NULL;

    this->fbEventHandler	 = NULL;

    //
    // Initialize member data.
    //
    this->fileMenu       = NUL(Widget);
    this->windowsMenu    = NUL(Widget);
    this->optionsMenu    = NUL(Widget);
    this->managed_state  = NUL(Stack*);

    this->fileMenuPulldown       = NUL(Widget);
    this->windowsMenuPulldown    = NUL(Widget);
    this->optionsMenuPulldown    = NUL(Widget);
	
    this->openOption         = NUL(CommandInterface*);
    this->saveOption         = NUL(CommandInterface*);
    this->saveAsOption       = NUL(CommandInterface*);
    this->cfgSettingsCascadeMenu = NULL;
    this->saveCfgOption      = NUL(CommandInterface*);
    this->openCfgOption      = NUL(CommandInterface*);
    this->loadMacroOption    = NUL(CommandInterface*);
    this->loadMDFOption      = NUL(CommandInterface*);
#if 0
    this->quitOption         = NUL(CommandInterface*);
#endif
    this->closeOption        = NUL(CommandInterface*);
    this->saveImageOption    = NUL(CommandInterface*);
    this->printImageOption   = NUL(CommandInterface*);

    this->openVisualProgramEditorOption = NUL(CommandInterface*);
    this->openAllControlPanelsOption    = NUL(CommandInterface*);
    this->openControlPanelByNameMenu	= NULL;
    this->openAllColormapEditorsOption  = NUL(CommandInterface*);
    this->messageWindowOption 	 	= NUL(CommandInterface*);

    this->renderingOptionsOption     = NUL(CommandInterface*);
    this->autoAxesOption             = NUL(CommandInterface*);
    this->throttleOption             = NUL(CommandInterface*);
    this->viewControlOption          = NUL(CommandInterface*);
    this->modeOptionCascade = NULL;
    this->undoOption                 = NUL(CommandInterface*);
    this->redoOption                 = NUL(CommandInterface*);
    this->resetOption                = NUL(CommandInterface*);
    this->changeImageNameOption      = NUL(CommandInterface*);
    this->backgroundColorOption	     = NUL(CommandInterface*);
    this->displayRotationGlobeOption = NUL(CommandInterface*);
    this->imageDepthCascade = NULL;
    this->imageDepth8Option	     = NUL(ToggleButtonInterface*); 
    this->imageDepth12Option	     = NUL(ToggleButtonInterface*);
    this->imageDepth15Option	     = NUL(ToggleButtonInterface*);
    this->imageDepth16Option	     = NUL(ToggleButtonInterface*);
    this->imageDepth24Option	     = NUL(ToggleButtonInterface*);
    this->imageDepth32Option	     = NUL(ToggleButtonInterface*);
    this->setPanelAccessOption	     = NUL(CommandInterface*);
    this->onVisualProgramOption	     = NUL(CommandInterface*);

    //
    // Intialize state.
    //
    this->state.width			= 0;
    this->state.height			= 0;
    this->state.pixmap			= XtUnspecifiedPixmap;
    this->state.gc			= NULL;
    this->state.hardwareWindow		= 0;
    this->state.hardwareRender		= FALSE;
    this->state.hardwareRenderExists 	= FALSE;
    this->state.resizeFromServer	= FALSE;
    this->state.frameBuffer		= FALSE;
    this->state.globeDisplayed		= FALSE;
    this->state.degenerateBox		= FALSE;
    this->state.imageCount		= 0;
    this->state.parent.window		= 0;
    this->state.resizeCausesExecution   = TRUE;

    this->state.hardwareCamera.undoable = FALSE;
    this->state.hardwareCamera.redoable = FALSE;

    this->node = NULL;
    this->directInteraction = FALSE;
    this->switchingSoftware = FALSE;
    this->pushedSinceExec   = FALSE;
    this->currentInteractionMode = NONE;
    this->pendingInteractionMode = NONE;
    //
    // Create the commands.
    //
    this->renderingOptionsCmd =
	new NoUndoImageCommand("renderingOptions", this->commandScope,
			  FALSE, this, NoUndoImageCommand::RenderingOptions);
    this->softwareCmd = new ImageSoftwareCommand("software", this->commandScope,
			    FALSE, this);
    this->hardwareCmd = new ImageHardwareCommand("hardware", this->commandScope,
			    FALSE, this);
    this->upNoneCmd = new ImageApproxCommand("upNone",
	this->commandScope,
	FALSE, this, TRUE, APPROX_NONE);
    this->upWireframeCmd = new ImageApproxCommand("upWireframe",
	this->commandScope,
	FALSE, this, TRUE, APPROX_WIREFRAME);
    this->upDotsCmd = new ImageApproxCommand("upDots",
	this->commandScope,
	FALSE, this, TRUE, APPROX_DOTS);
    this->upBoxCmd = new ImageApproxCommand("upBox",
	this->commandScope,
	FALSE, this, TRUE, APPROX_BOX);
    this->downNoneCmd = new ImageApproxCommand("downNone",
	this->commandScope,
	FALSE, this, FALSE, APPROX_NONE);
    this->downWireframeCmd = new ImageApproxCommand("downWireframe",
	this->commandScope,
	FALSE, this, FALSE, APPROX_WIREFRAME);
    this->downDotsCmd = new ImageApproxCommand("downDots",
	this->commandScope,
	FALSE, this, FALSE, APPROX_DOTS);
    this->downBoxCmd = new ImageApproxCommand("downBox",
	this->commandScope,
	FALSE, this, FALSE, APPROX_BOX);

    this->autoAxesCmd =
	new NoUndoImageCommand("autoAxes", this->commandScope,
			  FALSE, this,
			  NoUndoImageCommand::AutoAxes);

    this->throttleCmd =
	new NoUndoImageCommand("throttle", this->commandScope,
			  FALSE, this, NoUndoImageCommand::Throttle);
    this->viewControlCmd =
	new NoUndoImageCommand("viewControl", this->commandScope,
			  FALSE, this,
			  NoUndoImageCommand::ViewControl);
    this->modeNoneCmd = 
	new ImageSetModeCommand("modeNone", this->commandScope, FALSE,
				this, NONE);
    this->modeCameraCmd = 
	new ImageSetModeCommand("modeCamera", this->commandScope, FALSE,
				this, CAMERA);
    this->modeCursorsCmd = 
	new ImageSetModeCommand("modeCursors", this->commandScope, FALSE,
				this, CURSORS);
    this->modePickCmd = 
	new ImageSetModeCommand("modePick", this->commandScope, FALSE,
				this, PICK);
    this->modeNavigateCmd = 
	new ImageSetModeCommand("modeNavigate", this->commandScope, FALSE,
				this, NAVIGATE);
    this->modePanZoomCmd = 
	new ImageSetModeCommand("modePanZoom", this->commandScope, FALSE,
				this, PANZOOM);
    this->modeRoamCmd = 
	new ImageSetModeCommand("modeRoam", this->commandScope, FALSE,
				this, ROAM);
    this->modeRotateCmd = 
	new ImageSetModeCommand("modeRotate", this->commandScope, FALSE,
				this, ROTATE);
    this->modeZoomCmd = 
	new ImageSetModeCommand("modeZoom", this->commandScope, FALSE,
				this, ZOOM);

    this->setViewNoneCmd = 
	new ImageSetViewCommand("setViewNone", this->commandScope, FALSE,
				this, VIEW_NONE);
    this->setViewTopCmd = 
	new ImageSetViewCommand("setViewTop", this->commandScope, TRUE,
				this, VIEW_TOP);
    this->setViewBottomCmd = 
	new ImageSetViewCommand("setViewBottom", this->commandScope, TRUE,
				this, VIEW_BOTTOM);
    this->setViewFrontCmd = 
	new ImageSetViewCommand("setViewFront", this->commandScope, TRUE,
				this, VIEW_FRONT);
    this->setViewBackCmd = 
	new ImageSetViewCommand("setViewBack", this->commandScope, TRUE,
				this, VIEW_BACK);
    this->setViewLeftCmd = 
	new ImageSetViewCommand("setViewLeft", this->commandScope, TRUE,
				this, VIEW_LEFT);
    this->setViewRightCmd =
	new ImageSetViewCommand("setViewRight", this->commandScope, TRUE,
				this, VIEW_RIGHT);
    this->setViewDiagonalCmd = 
	new ImageSetViewCommand("setViewDiagonal", this->commandScope, TRUE,
				this, VIEW_DIAGONAL);
    this->setViewOffTopCmd = 
	new ImageSetViewCommand("setViewOffTop", this->commandScope, TRUE,
				this, VIEW_OFF_TOP);
    this->setViewOffBottomCmd = 
	new ImageSetViewCommand("setViewOffBottom", this->commandScope, TRUE,
				this, VIEW_OFF_BOTTOM);
    this->setViewOffFrontCmd = 
	new ImageSetViewCommand("setViewOffFront", this->commandScope, TRUE,
				this, VIEW_OFF_FRONT);
    this->setViewOffBackCmd = 
	new ImageSetViewCommand("setViewOffBack", this->commandScope, TRUE,
				this, VIEW_OFF_BACK);
    this->setViewOffLeftCmd = 
	new ImageSetViewCommand("setViewOffLeft", this->commandScope, TRUE,
				this, VIEW_OFF_LEFT);
    this->setViewOffRightCmd =
	new ImageSetViewCommand("setViewOffRight", this->commandScope, TRUE,
				this, VIEW_OFF_RIGHT);
    this->setViewOffDiagonalCmd = 
	new ImageSetViewCommand("setViewOffDiagonal", this->commandScope, TRUE,
				this, VIEW_OFF_DIAGONAL);

    this->perspectiveCmd = 
	new ImagePerspectiveCommand("perspective", this->commandScope,
				    TRUE, this, TRUE);
    this->parallelCmd = 
	new ImagePerspectiveCommand("parallel", this->commandScope,
				    TRUE, this, FALSE);
    this->constrainNoneCmd = 
	new ImageConstraintCommand("constrainNone", this->commandScope, TRUE,
				    this, CONSTRAINT_NONE);
    this->constrainXCmd = 
	new ImageConstraintCommand("constrainX", this->commandScope, TRUE,
				    this, CONSTRAINT_X);
    this->constrainYCmd = 
	new ImageConstraintCommand("constrainY", this->commandScope, TRUE,
				    this, CONSTRAINT_Y);
    this->constrainZCmd = 
	new ImageConstraintCommand("constrainZ", this->commandScope, TRUE,
				    this, CONSTRAINT_Z);

    this->lookForwardCmd =
	new ImageLookCommand("lookForwardCmd", this->commandScope, TRUE,
			     this, LOOK_FORWARD);
    this->lookLeft45Cmd =
	new ImageLookCommand("lookLeft45Cmd", this->commandScope, TRUE,
			     this, LOOK_LEFT45);
    this->lookRight45Cmd =
	new ImageLookCommand("lookRight45Cmd", this->commandScope, TRUE,
			     this, LOOK_RIGHT45);
    this->lookUp45Cmd =
	new ImageLookCommand("lookUp45Cmd", this->commandScope, TRUE,
			     this, LOOK_UP45);
    this->lookDown45Cmd =
	new ImageLookCommand("lookDown45Cmd", this->commandScope, TRUE,
			     this, LOOK_DOWN45);
    this->lookLeft90Cmd =
	new ImageLookCommand("lookLeft90Cmd", this->commandScope, TRUE,
			     this, LOOK_LEFT90);
    this->lookRight90Cmd =
	new ImageLookCommand("lookRight90Cmd", this->commandScope, TRUE,
			     this, LOOK_RIGHT90);
    this->lookUp90Cmd =
	new ImageLookCommand("lookUp90Cmd", this->commandScope, TRUE,
			     this, LOOK_UP90);
    this->lookDown90Cmd =
	new ImageLookCommand("lookDown90Cmd", this->commandScope, TRUE,
			     this, LOOK_DOWN90);
    this->lookBackwardCmd =
	new ImageLookCommand("lookBackwardCmd", this->commandScope, TRUE,
			     this, LOOK_BACKWARD);
    this->lookAlignCmd =
	new ImageLookCommand("lookAlignCmd", this->commandScope, TRUE,
			     this, LOOK_ALIGN);


    this->undoCmd =
	new ImageUndoCommand("undo", this->commandScope, FALSE, this);
    this->redoCmd =
	new ImageRedoCommand("redo", this->commandScope, FALSE, this);
    this->resetCmd =
	new ImageResetCommand("reset", this->commandScope,
			  this->directInteractionAllowed(), this);

    // FIXME: this should be DXApplication command
    this->openVPECmd =
	    new NoUndoImageCommand("openVPE", this->commandScope, TRUE,
				   this, NoUndoImageCommand::OpenVPE);
    this->displayRotationGlobeCmd =
	new NoUndoImageCommand("displayRotationGlobe", this->commandScope,
			  FALSE, this, NoUndoImageCommand::DisplayGlobe);

    this->backgroundColorCmd =
	new NoUndoImageCommand("setBackgroundColor", this->commandScope,
			       FALSE, this, NoUndoImageCommand::SetBGColor);
 
    if (!theDXApplication->appLimitsImageOptions()) {
	this->imageDepth8Cmd =
	    new NoUndoImageCommand("imageDepth8", this->commandScope,
	       FALSE, this, NoUndoImageCommand::Depth8);

	this->imageDepth12Cmd =
	    new NoUndoImageCommand("imageDepth12", this->commandScope,
	       FALSE, this, NoUndoImageCommand::Depth12);

	this->imageDepth15Cmd =
	    new NoUndoImageCommand("imageDepth15", this->commandScope,
	       FALSE, this, NoUndoImageCommand::Depth15);

	this->imageDepth16Cmd =
	    new NoUndoImageCommand("imageDepth16", this->commandScope,
	       FALSE, this, NoUndoImageCommand::Depth16);

	this->imageDepth24Cmd =
	    new NoUndoImageCommand("imageDepth24", this->commandScope,
	       FALSE, this, NoUndoImageCommand::Depth24);

	this->imageDepth32Cmd =
	    new NoUndoImageCommand("imageDepth32", this->commandScope,
	       FALSE, this, NoUndoImageCommand::Depth32);

	this->changeImageNameCmd =
	    new NoUndoImageCommand("changeImageName", this->commandScope, 
		FALSE, this, NoUndoImageCommand::ChangeImageName);
	// FIXME: this should be DXApplication command
	this->setPanelAccessCmd =
		new NoUndoImageCommand("setPanelAccess", this->commandScope,
		   FALSE, this, NoUndoImageCommand::SetCPAccess);
    } else {
	this->changeImageNameCmd = NULL;
	this->setPanelAccessCmd = NULL;
	this->imageDepth8Cmd = NULL;
	this->imageDepth12Cmd = NULL;
	this->imageDepth15Cmd = NULL;
	this->imageDepth16Cmd = NULL;
	this->imageDepth24Cmd = NULL;
	this->imageDepth32Cmd = NULL;
    }

    // FIXME: this should be DXApplication command
    if (theDXApplication->appAllowsImageSaving())
	this->saveImageCmd =
	    new NoUndoImageCommand("saveImage", this->commandScope,
	       FALSE, this, NoUndoImageCommand::SaveAsImage);
    else
	this->saveImageCmd = NULL;

    // FIXME: this should be DXApplication command
    if (theDXApplication->appAllowsImagePrinting())
	this->printImageCmd =
	    new NoUndoImageCommand("printImage", this->commandScope,
	       FALSE, this, NoUndoImageCommand::PrintImage);
    else
	this->printImageCmd = NULL;

    this->closeCmd =
        new CloseWindowCommand("close",this->commandScope,TRUE,this);

    this->allowDirectInteraction(FALSE);

    //this->currentProbeNode = NULL;
    this->currentProbeInstance = -1;
    this->currentPickInstance = -1;
    //this->currentPickNode = NULL;

    this->resetWindowTitle();

    this->cameraInitialized = False;

    //
    // By default the only startup image window is the anchor window.
    //
    this->startup = this->anchor || (this->network->getImageCount() == 1);

    this->reset_eor_wp = NUL(XtWorkProcId);

    //
    // Rather than triggering an execution following a window resize,
    // Queue the execution so that it happens soon after the execution
    // but not immediately after.  This gives better results for users
    // with window mgrs that deliver tons of resize events
    //
    this->execute_after_resize_to = 0;

    //
    // Install the default resources for THIS class (not the derived classes)
    //
    if (NOT ImageWindow::ClassInitialized)
    {
	ASSERT(theApplication);
        ImageWindow::ClassInitialized = TRUE;
	this->installDefaultResources(theApplication->getRootWidget());
    }
}


ImageWindow::~ImageWindow()
{
 
    //
    // Purify caught an abr originating in ImageWindow_RedrawCB
    // if you do File/New with many opened image tools.  So remove
    // the callback prior to going completely away.
    //
    if (this->getCanvas()) {
	XtRemoveCallback(this->getCanvas(),
	    XmNexposeCallback,
	    (XtCallbackProc)ImageWindow_RedrawCB,
	    (XtPointer)this);
	if(this->state.gc)
	    XFreeGC(XtDisplay(this->getCanvas()),this->state.gc);
	this->state.gc = NULL;
    }


    if(this->state.hardwareRenderExists)
    {
	this->sendClientMessage(this->atoms.gl_destroy_window);
	this->wait4GLAcknowledge();
    }

    XmUpdateDisplay(this->getCanvas());
    XSync(XtDisplay(this->getCanvas()), False);

    //
    // Disconnect from the node
    //
    DisplayNode *n = (DisplayNode*)this->node;
    if (n) {
	this->node = NULL;
	n->associateImage(NULL);
    }

    if (this->changeImageNameDialog)
        delete this->changeImageNameDialog;
    if (this->viewControlDialog) 
	delete this->viewControlDialog;
    if (this->renderingOptionsDialog)
	delete this->renderingOptionsDialog;
    if (this->backgroundColorDialog)
	delete this->backgroundColorDialog;
    if (this->throttleDialog)
	delete this->throttleDialog;
    if (this->autoAxesDialog)
	delete this->autoAxesDialog;
    if (this->saveImageDialog)
	delete this->saveImageDialog;
    if (this->printImageDialog)
	delete this->printImageDialog;
    if (this->fbEventHandler)
	delete this->fbEventHandler;

    //
    // File menu optoins
    //
    if (this->openOption) delete this->openOption;
    if (this->saveOption) delete this->saveOption;
    if (this->saveAsOption) delete this->saveAsOption;
    if (this->saveCfgOption) delete this->saveCfgOption;
    if (this->openCfgOption) delete this->openCfgOption;
    if (this->cfgSettingsCascadeMenu) delete this->cfgSettingsCascadeMenu; 
    if (this->loadMacroOption) delete this->loadMacroOption;
    if (this->loadMDFOption) delete this->loadMDFOption;
#if 0
    if (this->quitOption) delete this->quitOption;
#endif
    if (this->closeOption) delete this->closeOption;
    if (this->saveImageOption) delete this->saveImageOption;
    if (this->printImageOption) delete this->printImageOption;

    //
    // Windows options
    //
    if (this->openVisualProgramEditorOption) delete this->openVisualProgramEditorOption;
    if (this->openAllControlPanelsOption) delete this->openAllControlPanelsOption;
    if (this->openControlPanelByNameMenu) delete this->openControlPanelByNameMenu;
    if (this->openAllColormapEditorsOption) delete this->openAllColormapEditorsOption;
    if (this->messageWindowOption) delete this->messageWindowOption;
    this->panelNameList.clear();
    this->panelGroupList.clear();

    //
    // Option options
    //
    if (this->renderingOptionsOption) delete this->renderingOptionsOption;
    if (this->autoAxesOption) delete this->autoAxesOption;
    if (this->throttleOption) delete this->throttleOption;
    if (this->viewControlOption) delete this->viewControlOption;
    if (this->modeOptionCascade) delete this->modeOptionCascade;
    if (this->undoOption) delete this->undoOption;
    if (this->redoOption) delete this->redoOption;
    if (this->resetOption) delete this->resetOption;
    if (this->changeImageNameOption) delete this->changeImageNameOption;
    if (this->backgroundColorOption) delete this->backgroundColorOption;
    if (this->displayRotationGlobeOption) delete this->displayRotationGlobeOption;
    //
    // Delete the image depth cascade which contains the imageDepth*Options.
    // Deleting the cascade deletes the contained options so we don't need
    // to explicitly delete them ourselves.
    //
    if (this->imageDepthCascade) delete this->imageDepthCascade;

    if (this->setPanelAccessOption) delete this->setPanelAccessOption;
    if (this->onVisualProgramOption) delete this->onVisualProgramOption;

    //
    // Delete the commands after the command interfaces.
    //
    delete this->renderingOptionsCmd;
    delete this->softwareCmd;
    delete this->hardwareCmd;
    delete this->upNoneCmd;
    delete this->upWireframeCmd;
    delete this->upDotsCmd;
    delete this->upBoxCmd;
    delete this->downNoneCmd;
    delete this->downWireframeCmd;
    delete this->downDotsCmd;
    delete this->downBoxCmd;
    delete this->autoAxesCmd;
    delete this->throttleCmd;
    if (this->imageDepth8Cmd)   delete this->imageDepth8Cmd;
    if (this->imageDepth12Cmd)  delete this->imageDepth12Cmd;
    if (this->imageDepth15Cmd)  delete this->imageDepth15Cmd;
    if (this->imageDepth16Cmd)  delete this->imageDepth16Cmd;
    if (this->imageDepth24Cmd)  delete this->imageDepth24Cmd;
    if (this->imageDepth32Cmd)  delete this->imageDepth32Cmd;
    if (this->setPanelAccessCmd) delete this->setPanelAccessCmd;
    delete this->viewControlCmd;
    delete this->modeNoneCmd;
    delete this->modeCameraCmd;
    delete this->modeCursorsCmd;
    delete this->modePickCmd;
    delete this->modeNavigateCmd;
    delete this->modePanZoomCmd;
    delete this->modeRoamCmd;
    delete this->modeRotateCmd;
    delete this->modeZoomCmd;
    delete this->setViewNoneCmd;
    delete this->setViewTopCmd;
    delete this->setViewBottomCmd;
    delete this->setViewFrontCmd;
    delete this->setViewBackCmd;
    delete this->setViewLeftCmd;
    delete this->setViewRightCmd;
    delete this->setViewDiagonalCmd;
    delete this->setViewOffTopCmd;
    delete this->setViewOffBottomCmd;
    delete this->setViewOffFrontCmd;
    delete this->setViewOffBackCmd;
    delete this->setViewOffLeftCmd;
    delete this->setViewOffRightCmd;
    delete this->setViewOffDiagonalCmd;
    delete this->perspectiveCmd;
    delete this->parallelCmd;
    delete this->constrainNoneCmd;
    delete this->constrainXCmd;
    delete this->constrainYCmd;
    delete this->constrainZCmd;
    delete this->lookForwardCmd;
    delete this->lookLeft45Cmd;
    delete this->lookRight45Cmd;
    delete this->lookUp45Cmd;
    delete this->lookDown45Cmd;
    delete this->lookLeft90Cmd;
    delete this->lookRight90Cmd;
    delete this->lookUp90Cmd;
    delete this->lookDown90Cmd;
    delete this->lookBackwardCmd;
    delete this->lookAlignCmd;
    delete this->closeCmd;
    delete this->undoCmd;
    delete this->redoCmd;
    delete this->resetCmd;
    if (this->changeImageNameCmd) delete this->changeImageNameCmd;
    delete this->backgroundColorCmd;
    delete this->displayRotationGlobeCmd;
    if (this->saveImageCmd)  delete this->saveImageCmd;
    if (this->printImageCmd) delete this->printImageCmd;
    if (this->openVPECmd) delete this->openVPECmd;


    //
    // Remove self from the network image list.
    //
    this->network->removeImage(this);
    if (this->managed_state) delete this->managed_state;

    if (this->reset_eor_wp) XtRemoveWorkProc(this->reset_eor_wp);
    if (this->execute_after_resize_to) XtRemoveTimeOut(this->execute_after_resize_to);
}


//
// Install the default resources for this class.
//
void ImageWindow::installDefaultResources(Widget baseWidget)
{
    this->setDefaultResources(baseWidget, ImageWindow::DefaultResources);
    this->DXWindow::installDefaultResources(baseWidget);
}
void ImageWindow::initialize()
{
    //
    // Now, call the superclass initialize().
    //
    this->DXWindow::initialize();

    //
    // Make sure that this window is resizable.
    this->allowResize(TRUE);

    XtRealizeWidget(this->getRootWidget());

    this->completePictureCreation();

    if(this->node)
    {
	this->changeDepth(((ImageNode *)(this->node))->getDepth());
    }
}

//
// Get Vals from the picture widget that we need to keep in synch.
// Also set up exposure handling for the frame buffer overlay.
//
void ImageWindow::completePictureCreation()
{

    /*
     * Determine if we are working with a frame buffer.
     */
    Boolean frameBuffer;
    Window  overlayWID;
    int     itrans;
    int     irot;
    XtVaGetValues(this->getCanvas(),
	XmNframeBuffer,    &frameBuffer,
	XmNoverlayWid,     &overlayWID,
	XmNtranslateSpeed, &itrans,
	XmNrotateSpeed,    &irot,
	NULL);
    this->state.navigateTranslateSpeed = itrans;
    this->state.navigateRotateSpeed    = irot;
    this->state.frameBuffer = frameBuffer;
    if (frameBuffer)
    {
	this->fbEventHandler = new XHandler(Expose, overlayWID,
	    HandleExposures, (void*)this);
    }
}

void ImageWindow::installCallbacks()
{
    Widget canvas = this->getCanvas();

    //
    // Add callbacks
    //
    XtAddCallback(canvas,
	XmNexposeCallback,
	(XtCallbackProc)ImageWindow_RedrawCB,
	(XtPointer)this);
    XtAddCallback(canvas,
	XmNresizeCallback,
	(XtCallbackProc)ImageWindow_ResizeCB,
	(XtPointer)this);
    XtAddCallback(canvas,
	XmNzoomCallback,
	(XtCallbackProc)ImageWindow_ZoomCB,
	(XtPointer)this);
    XtAddCallback(canvas,
	XmNroamCallback,
	(XtCallbackProc)ImageWindow_RoamCB,
	(XtPointer)this);
    XtAddCallback(canvas,
	XmNnavigateCallback,
	(XtCallbackProc)ImageWindow_NavigateCB,
	(XtPointer)this);
    XtAddCallback(canvas,
	XmNrotationCallback,
	(XtCallbackProc)ImageWindow_RotationCB,
	(XtPointer)this);
    XtAddCallback(canvas,
	XmNcursorCallback,
	(XtCallbackProc)ImageWindow_CursorCB,
	(XtPointer)this);
    XtAddCallback(canvas,
	XmNpickCallback,
	(XtCallbackProc)ImageWindow_PickCB,
	(XtPointer)this);
    XtAddCallback(canvas,
	XmNmodeCallback,
	(XtCallbackProc)ImageWindow_ModeCB,
	(XtPointer)this);
    XtAddCallback(canvas,
	XmNundoCallback,
	(XtCallbackProc)ImageWindow_UndoCB,
	(XtPointer)this);
    XtAddCallback(canvas,
	XmNkeyCallback,
	(XtCallbackProc)ImageWindow_KeyCB,
	(XtPointer)this);
    XtAddCallback(canvas,
	XmNclientMessageCallback,
	(XtCallbackProc)ImageWindow_ClientMessageCB,
	(XtPointer)this);
    XtAddCallback(canvas,
	XmNpropertyNotifyCallback,
	(XtCallbackProc)ImageWindow_PropertyNotifyCB,
	(XtPointer)this);

    this->installComponentHelpCallback(canvas);
}

void ImageWindow::manage()
{
    this->DXWindow::manage();

    if (this->state.frameBuffer)
    {
	// FIXME: is this really intalling a handler on each manage?
	XtAddEventHandler(this->getRootWidget(),
	    StructureNotifyMask,
	    False, (XtEventHandler)ImageWindow_TrackFrameBufferEH, (XtPointer)this);
    }
}

void ImageWindow::unmanage()
{
    if(this->state.frameBuffer)
    {
	XtRemoveEventHandler(this->getRootWidget(),
	    StructureNotifyMask,
	    False, (XtEventHandler)ImageWindow_TrackFrameBufferEH, (XtPointer)this);
    }
    this->DXWindow::unmanage();

    if (this->viewControlDialog)
	this->viewControlDialog->unmanage();
    if (this->renderingOptionsDialog)
	this->renderingOptionsDialog->unmanage();
    if (this->backgroundColorDialog)
	this->backgroundColorDialog->unmanage();
    if (this->throttleDialog)
	this->throttleDialog->unmanage();
    if (this->autoAxesDialog)
	this->autoAxesDialog->unmanage();
    if (this->changeImageNameDialog)
	this->changeImageNameDialog->unmanage();
}

Widget ImageWindow::createWorkArea(Widget parent)
{
    Widget outerFrame;
    Widget innerFrame;

    ASSERT(parent);

    /*
     * Create atoms needed for GL support.
     */
    this->atoms.stop =
	XInternAtom(XtDisplay(parent), "StopInteraction",         False);
    this->atoms.set_view =
	XInternAtom(XtDisplay(parent), "Set_View",                False);
    this->atoms.start_rotate =
	XInternAtom(XtDisplay(parent), "StartRotateInteraction",  False);
    this->atoms.start_zoom =
	XInternAtom(XtDisplay(parent), "StartZoomInteraction",    False);
    this->atoms.start_panzoom =
	XInternAtom(XtDisplay(parent), "StartPanZoomInteraction", False);
    this->atoms.start_cursor =
	XInternAtom(XtDisplay(parent), "StartCursorInteraction",  False);
    this->atoms.start_roam =
	XInternAtom(XtDisplay(parent), "StartRoamInteraction",    False);
    this->atoms.start_navigate =
	XInternAtom(XtDisplay(parent), "StartNavigateInteraction",False);
    this->atoms.from_vector =
	XInternAtom(XtDisplay(parent), "FromPoint",               False);
    this->atoms.up_vector =
	XInternAtom(XtDisplay(parent), "UpVector",                False);
    this->atoms.zoom1 =
	XInternAtom(XtDisplay(parent), "Zoom1",                   False);
    this->atoms.zoom2 =
	XInternAtom(XtDisplay(parent), "Zoom2",                   False);
    this->atoms.roam =
	XInternAtom(XtDisplay(parent), "RoamPoint",               False);
    this->atoms.cursor_change =
	XInternAtom(XtDisplay(parent), "CursorChange",            False);
    this->atoms.cursor_constraint =
	XInternAtom(XtDisplay(parent), "SetCursorConstraint",     False);
    this->atoms.cursor_speed =
        XInternAtom(XtDisplay(parent), "SetCursorSpeed",          False);
    this->atoms.gl_window0 =
	XInternAtom(XtDisplay(parent), "GLWindow0",               False);
    this->atoms.gl_window1 =
	XInternAtom(XtDisplay(parent), "GLWindow1",               False);
    this->atoms.gl_window2 =
	XInternAtom(XtDisplay(parent), "GLWindow2",               False);
    this->atoms.gl_window2_execute =
	XInternAtom(XtDisplay(parent), "GLWindow2Execute",        False);
    this->atoms.dx_flag =
	XInternAtom(XtDisplay(parent), "DX_FLAG",                 False);
    this->atoms.dx_pixmap_id =
	XInternAtom(XtDisplay(parent), "DX_PIXMAP_ID",            False);
    this->atoms.display_globe =
	XInternAtom(XtDisplay(parent), "DisplayGlobe",            False);
    this->atoms.motion =
	XInternAtom(XtDisplay(parent), "NavigateMotion",          False);
    this->atoms.pivot =
	XInternAtom(XtDisplay(parent), "NavigatePivot",           False);
    this->atoms.undoable =
	XInternAtom(XtDisplay(parent), "CameraUndoable",          False);
    this->atoms.redoable =
	XInternAtom(XtDisplay(parent), "CameraRedoable",          False);
    this->atoms.redo_camera =
	XInternAtom(XtDisplay(parent), "RedoCamera",              False);
    this->atoms.undo_camera =
	XInternAtom(XtDisplay(parent), "UndoCamera",              False);
    this->atoms.push_camera =
	XInternAtom(XtDisplay(parent), "PushCamera",              False);
    this->atoms.button_mapping1 =
	XInternAtom(XtDisplay(parent), "ButtonMapping1",          False);
    this->atoms.button_mapping2 =
	XInternAtom(XtDisplay(parent), "ButtonMapping2",          False);
    this->atoms.button_mapping3 =
	XInternAtom(XtDisplay(parent), "ButtonMapping3",          False);
    this->atoms.navigate_look_at =
	XInternAtom(XtDisplay(parent), "NavigateLookAt",          False);
    this->atoms.execute_on_change =
	XInternAtom(XtDisplay(parent), "ExecuteOnChange",         False);
    this->atoms.gl_destroy_window =
	XInternAtom(XtDisplay(parent), "GLDestroyWindow",         False);
    this->atoms.gl_shutdown =
	XInternAtom(XtDisplay(parent), "GLShutdown",              False);
    this->atoms.start_pick =
	XInternAtom(XtDisplay(parent), "StartPickInteraction",    False);
    this->atoms.pick_point =
	XInternAtom(XtDisplay(parent), "PickPoint",               False);
    this->atoms.image_reset =
	XInternAtom(XtDisplay(parent), "ImageReset",               False);

    //
    // Create the outer and inner frame.
    //
    outerFrame =
	XtVaCreateManagedWidget
	    ("imageOuterFrame",
	     xmFrameWidgetClass,
	     parent,
	     XmNshadowType,   XmSHADOW_OUT,
	     XmNmarginHeight, 5,
	     XmNmarginWidth,  5,
	     NULL);

    innerFrame =
	XtVaCreateManagedWidget
	    ("imageInnerFrame",
	     xmFrameWidgetClass,
	     outerFrame,
	     XmNshadowType, XmSHADOW_IN,
	     NULL);

    this->form =
	XtVaCreateManagedWidget
	    ("imageInnerFrame",
	     xmFormWidgetClass,
	     innerFrame,
	     XmNmarginWidth, 0,
	     XmNmarginHeight, 0,
	     XmNbackground, BlackPixel(XtDisplay(innerFrame), 0),
	     NULL);

    //
    // Create the canvas.
    //
    this->canvas =
	XtVaCreateManagedWidget
	    ("imageCanvas",
	     xmPictureWidgetClass,
	     this->form,
	     XmNsendMotion,		False,
	     XmNmode,			XmNULL_MODE,
	     XmNshowRotatingBBox,	False,
	     XmNglobeRadius,		10,
	     XmNleftAttachment,		XmATTACH_FORM,
	     XmNrightAttachment,	XmATTACH_FORM,
	     XmNtopAttachment,		XmATTACH_FORM,
	     XmNbottomAttachment,	XmATTACH_FORM,
	     XmNhighlightPixmap,	XmUNSPECIFIED_PIXMAP,
	     NULL);

    this->completePictureCreation();

    //
    // Add callbacks
    //
    this->installCallbacks();

    //
    // Return the topmost widget of the work area.
    //
    return outerFrame;
}


void ImageWindow::createFileMenu(Widget parent)
{
    ASSERT(parent);
    int buttons = 0;
    Widget pulldown;

    //
    // Create "File" menu and options.
    //
    pulldown =
	this->fileMenuPulldown =
	    XmCreatePulldownMenu(parent, "fileMenuPulldown", NUL(ArgList), 0);
    this->fileMenu =
	XtVaCreateManagedWidget
	    ("fileMenu",
	     xmCascadeButtonWidgetClass,
	     parent,
	     XmNsubMenuId, pulldown,
	     NULL);

    //
    // Net file options. 
    //
    
    if (theDXApplication->appAllowsImageRWNetFile())  {
	this->openOption =
	   new ButtonInterface(pulldown, "imageOpenOption", 
			theDXApplication->openFileCmd);

	this->createFileHistoryMenu(pulldown);

	this->saveOption =
	    new ButtonInterface(pulldown, "imageSaveOption", 
			    this->network->getSaveCommand());

	this->saveAsOption =
	    new ButtonInterface(pulldown, "imageSaveAsOption", 
			    this->network->getSaveAsCommand());
	buttons = 1;
    }

    
    if (theDXApplication->appAllowsRWConfig())  {

	Command *openCfgCmd = this->network->getOpenCfgCommand();
	Command *saveCfgCmd = this->network->getSaveCfgCommand();
	if (openCfgCmd && saveCfgCmd) {
	    CascadeMenu *cascade_menu = this->cfgSettingsCascadeMenu =
			    new CascadeMenu("imageSettingsCascade",pulldown);
	    Widget menu_parent = cascade_menu->getMenuItemParent();
	    this->saveCfgOption = new ButtonInterface(menu_parent,
					    "imageSaveCfgOption", saveCfgCmd);
	    this->openCfgOption = new ButtonInterface(menu_parent,
					    "imageOpenCfgOption", openCfgCmd);
	} else if (openCfgCmd) {
	    this->openCfgOption = new ButtonInterface(pulldown,
					    "imageOpenCfgOption", openCfgCmd);
	} else if (saveCfgCmd) {
	    this->saveCfgOption = new ButtonInterface(pulldown,
					    "imageSaveCfgOption", saveCfgCmd);
	}


	buttons = 1;
    }



    //
    // Macro/mdf options.
    //
    if (buttons) {
	XtVaCreateManagedWidget
		("optionSeparator", xmSeparatorWidgetClass, pulldown, NULL);
	buttons = 0;
    }
    if (theDXApplication->appAllowsImageLoad()) {
	this->loadMacroOption =
	    new ButtonInterface(pulldown, "imageLoadMacroOption", 
			theDXApplication->loadMacroCmd);
	this->loadMDFOption =
	    new ButtonInterface(pulldown, "imageLoadMDFOption", 	
			theDXApplication->loadMDFCmd);
	buttons = 1;
    }


    //
    // Image options.
    //
    if (buttons) {
	XtVaCreateManagedWidget
		("optionSeparator", xmSeparatorWidgetClass, pulldown, NULL);
	buttons = 0;
    }

    if (this->saveImageCmd) {
	this->saveImageOption =
	    new ButtonInterface(pulldown, 
		"imageSaveImageOption", this->saveImageCmd);
	buttons = 1;
    } 
    
    if (this->printImageCmd) {
	this->printImageOption =
	    new ButtonInterface(pulldown, 
		"imagePrintImageOption", this->printImageCmd);
	buttons = 1;
    } 


    //
    // Close/quite
    //
    if (buttons) 
	XtVaCreateManagedWidget
		("optionSeparator", xmSeparatorWidgetClass, pulldown, NULL);

#if 0
    if(!this->isAnchor())
#endif
        this->closeOption =
           new ButtonInterface(pulldown,"imageCloseOption",this->closeCmd);
#if 0
    else
    {
        this->quitOption =
           new ButtonInterface(pulldown,"imageQuitOption",theDXApplication->exitCmd);
    }
#endif

    XtAddCallback(pulldown,
                  XmNmapCallback,
                  (XtCallbackProc)ImageWindow_FileMenuMapCB,
                  (XtPointer)this);

    if ((theDXApplication->appAllowsExitOptions() && this->isAnchor()) ||
	theDXApplication->inDataViewerMode()) 
    {
	ASSERT(this->closeOption);
	if(this->network->saveToFileRequired())
	    ((ButtonInterface*)this->closeOption)->setLabel("Quit...");
	else
	    ((ButtonInterface*)this->closeOption)->setLabel("Quit");
	XmString accText = XmStringCreateSimple("Ctrl+Q");
	XtVaSetValues(this->closeOption->getRootWidget(),
		    XmNmnemonic, XK_Q,
		    XmNaccelerator, "Ctrl<Key>Q",
		    XmNacceleratorText,accText,
		    NULL);
	XmStringFree(accText);
    }
    else 
    {
        ((ButtonInterface*)this->closeOption)->setLabel("Close");
        XmString accText = XmStringCreateSimple("Ctrl+W");
	XtVaSetValues(this->closeOption->getRootWidget(),
		    XmNmnemonic, XK_C,
		    XmNaccelerator, "Ctrl<Key>W",
		    XmNacceleratorText,accText,
		    NULL);
	XmStringFree(accText);
    }

    if (!this->isAnchor()) 
    {
	if (this->openOption)
	    this->openOption->deactivate();
	if (this->saveOption)
	    this->saveOption->deactivate();
	if (this->saveAsOption)
	    this->saveAsOption->deactivate();
	if (this->saveCfgOption)
	    this->saveCfgOption->deactivate();
	if (this->openCfgOption)
	    this->openCfgOption->deactivate();
	if (this->loadMacroOption)
	    this->loadMacroOption->deactivate();
	if (this->loadMDFOption)
	    this->loadMDFOption->deactivate();
    }
}


void ImageWindow::createWindowsMenu(Widget parent)
{
    ASSERT(parent);

    Widget            pulldown;

    if (!theDXApplication->appAllowsWindowsMenus())
	return;	

    //
    // Create "Windows" menu and options.
    //
    pulldown =
	this->windowsMenuPulldown =
	    XmCreatePulldownMenu
		(parent, "windowsMenuPulldown", NUL(ArgList), 0);
    this->windowsMenu =
	XtVaCreateManagedWidget
	    ("windowsMenu",
	     xmCascadeButtonWidgetClass,
	     parent,
	     XmNsubMenuId, pulldown,
	     NULL);

    if (theDXApplication->appAllowsEditorAccess())
	this->openVisualProgramEditorOption =
	    new ButtonInterface
		(pulldown, "imageOpenVisualProgramEditorOption", this->openVPECmd);

    if (theDXApplication->appAllowsPanelAccess()) {
	if (theDXApplication->appAllowsOpenAllPanels()) 
	    this->openAllControlPanelsOption =
		new ButtonInterface
		    (pulldown,
		     "imageOpenAllControlPanelsOption",
		     this->network->getOpenAllPanelsCommand());
   
	this->openControlPanelByNameMenu = 
		new CascadeMenu("imageOpenControlPanelByNameOption",pulldown);

	// Builds the list of control panels
	XtAddCallback(pulldown,
		      XmNmapCallback,
		      (XtCallbackProc)ImageWindow_WindowsMenuMapCB,
		      (XtPointer)this);
    } 

    this->openAllColormapEditorsOption =
	new ButtonInterface
	    	(pulldown, "imageOpenAllColormapEditorsOption", 
			theDXApplication->openAllColormapCmd);

    this->messageWindowOption =
	new ButtonInterface
	    (pulldown, "imageMessageWindowOption",
	     theDXApplication->messageWindowCmd);


}

extern "C" void ImageWindow_FileMenuMapCB(Widget ,
                                   XtPointer ,
                                   XtPointer )
{
    //
    // Used to set the accelerator for quit.  We may not need this
    // callback anymore.
    //
}


extern "C" void ImageWindow_WindowsMenuMapCB(Widget w,
                                 XtPointer clientdata,
                                 XtPointer calldata)
{
    PanelAccessManager *panelMgr; 
    ImageWindow *iw = (ImageWindow*)clientdata;

    if (iw->openControlPanelByNameMenu) {
	Network *network = iw->network;
	CascadeMenu *menu = iw->openControlPanelByNameMenu;
	if (!network) {
	    menu->deactivate();
	} else {
	    //
	    // The ImageWindow uses it's associated Node's PanelManger if
	    // it is activated.  If not available or not activated, it uses 
	    // the Network's.
	    //
	    if (iw->node)
		panelMgr = ((DisplayNode*)iw->node)->getPanelManager();
	    else
		panelMgr = NULL;
	    if (!panelMgr || !panelMgr->isActivated())
		panelMgr = network->getPanelAccessManager();
	    network->fillPanelCascade(menu, panelMgr);
	}
    }

}

void ImageWindow::createOptionsMenu(Widget parent)
{
    ASSERT(parent);

    Widget            pulldown;

    //
    // Create "Options" menu and options.
    //
    pulldown =
	this->optionsMenuPulldown =
	    XmCreatePulldownMenu
		(parent, "optionsMenuPulldown", NUL(ArgList), 0);
    this->optionsMenu =
	XtVaCreateManagedWidget
	    ("optionsMenu",
	     xmCascadeButtonWidgetClass,
	     parent,
	     XmNsubMenuId, pulldown,
	     NULL);


    //
    // View Control
    //
	
    this->viewControlOption =
	new ButtonInterface(pulldown, "imageViewControlOption",
			    this->viewControlCmd);
    
    ToggleButtonInterface *tbm;
    CascadeMenu *cmm = this->modeOptionCascade = 
			new CascadeMenu("imageModeOption",pulldown); 

    Widget mitem_parent = cmm->getMenuItemParent();

    ASSERT(this->modeNoneCmd);
    this->modeNoneOption = tbm = new ToggleButtonInterface(mitem_parent,
		"imageNoneOption", this->modeNoneCmd, TRUE);
    cmm->appendComponent(tbm);

    ASSERT(this->modeCameraCmd);
    this->modeCameraOption = tbm = new ToggleButtonInterface(mitem_parent,
		"imageCameraOption", this->modeCameraCmd, FALSE);
    cmm->appendComponent(tbm);

    ASSERT(this->modeCursorsCmd);
    this->modeCursorsOption = tbm = new ToggleButtonInterface(mitem_parent,
		"imageCursorsOption", this->modeCursorsCmd, FALSE);
    cmm->appendComponent(tbm);

    ASSERT(this->modePickCmd);
    this->modePickOption = tbm = new ToggleButtonInterface(mitem_parent,
		"imagePickOption", this->modePickCmd, FALSE);
    cmm->appendComponent(tbm);

    ASSERT(this->modeNavigateCmd);
    this->modeNavigateOption = tbm = new ToggleButtonInterface(mitem_parent,
		"imageNavigateOption", this->modeNavigateCmd, FALSE);
    cmm->appendComponent(tbm);

    ASSERT(this->modePanZoomCmd);
    this->modePanZoomOption = tbm = new ToggleButtonInterface(mitem_parent,
		"imagePanZoomOption", this->modePanZoomCmd, FALSE);
    cmm->appendComponent(tbm);

    ASSERT(this->modeRoamCmd);
    this->modeRoamOption = tbm = new ToggleButtonInterface(mitem_parent,
		"imageRoamOption", this->modeRoamCmd, FALSE);
    cmm->appendComponent(tbm);

    ASSERT(this->modeRotateCmd);
    this->modeRotateOption = tbm = new ToggleButtonInterface(mitem_parent,
		"imageRotateOption", this->modeRotateCmd, FALSE);
    cmm->appendComponent(tbm);

    ASSERT(this->modeZoomCmd);
    this->modeZoomOption = tbm = new ToggleButtonInterface(mitem_parent,
		"imageZoomOption", this->modeZoomCmd, FALSE);
    cmm->appendComponent(tbm);    

    XtVaCreateManagedWidget
	    ("optionSeparator", xmSeparatorWidgetClass, pulldown, NULL);

    this->undoOption =
	new ButtonInterface(pulldown, "imageUndoOption",
			    this->undoCmd);

    this->redoOption =
	new ButtonInterface(pulldown, "imageRedoOption",
			    this->redoCmd);

    this->resetOption =
	new ButtonInterface(pulldown, "imageResetOption",
			    this->resetCmd);

    //
    // Image options/modes 
    //
    XtVaCreateManagedWidget
	    ("optionSeparator", xmSeparatorWidgetClass, pulldown, NULL);

    this->autoAxesOption =
	new ButtonInterface(pulldown, "imageAutoAxesOption",
			    this->autoAxesCmd);
    this->backgroundColorOption =
	new ButtonInterface(pulldown, "imageBackgroundColorOption",
			    this->backgroundColorCmd);
    this->displayRotationGlobeOption =
	new ToggleButtonInterface
	    (pulldown, "imageDisplayRotationGlobeOption",
			    this->displayRotationGlobeCmd, FALSE);
    //
    // Execution options/modes 
    //
    XtVaCreateManagedWidget
	    ("optionSeparator", xmSeparatorWidgetClass, pulldown, NULL);

    this->renderingOptionsOption =
	new ButtonInterface(pulldown, "imageRenderingOptionsOption",
			    this->renderingOptionsCmd);

    if (!theDXApplication->appLimitsImageOptions()) {
	// 'Set the frame buffer depth' pulldown option.

	ToggleButtonInterface *tbi;
	CascadeMenu *cm = this->imageDepthCascade = 
			new CascadeMenu("imageSetImageDepthOption",pulldown); 

	Widget item_parent = cm->getMenuItemParent();

	ASSERT(this->imageDepth8Cmd);
	this->imageDepth8Option = tbi = new ToggleButtonInterface(item_parent,
		"8", this->imageDepth8Cmd, FALSE);
	cm->appendComponent(tbi);

	ASSERT(this->imageDepth12Cmd);
	this->imageDepth12Option = tbi = new ToggleButtonInterface(item_parent,
		"12", this->imageDepth12Cmd, FALSE);
	cm->appendComponent(tbi);

	ASSERT(this->imageDepth15Cmd);
	this->imageDepth15Option = tbi = new ToggleButtonInterface(item_parent,
		"15", this->imageDepth15Cmd, FALSE);
	cm->appendComponent(tbi);

	ASSERT(this->imageDepth16Cmd);
	this->imageDepth16Option = tbi = new ToggleButtonInterface(item_parent,
		"16", this->imageDepth16Cmd, FALSE);
	cm->appendComponent(tbi);

	ASSERT(this->imageDepth24Cmd);
	this->imageDepth24Option = tbi = new ToggleButtonInterface(item_parent,
		"24", this->imageDepth24Cmd, FALSE);
	cm->appendComponent(tbi);

	ASSERT(this->imageDepth32Cmd);
	this->imageDepth32Option = tbi = new ToggleButtonInterface(item_parent,
		"32", this->imageDepth32Cmd, FALSE);
	cm->appendComponent(tbi);

	this->throttleOption =
	    new ButtonInterface(pulldown, "imageThrottleOption",
				this->throttleCmd);
    }


    //
    // Miscellaneous options 
    //
    if (!theDXApplication->appLimitsImageOptions()) {
	XtVaCreateManagedWidget
		    ("optionSeparator", xmSeparatorWidgetClass, pulldown, NULL);
	    
	this->changeImageNameOption =
		new ButtonInterface(pulldown, "imageChangeImageNameOption",
				    this->changeImageNameCmd);


	this->setPanelAccessOption =
		new ButtonInterface(pulldown, "imageSetPanelAccessOption",
				    this->setPanelAccessCmd);

#if USE_STARTUP_OPTION
        XtVaCreateManagedWidget
                    ("optionSeparator", xmSeparatorWidgetClass, pulldown, NULL);

	this->addStartupToggleOption(pulldown);
#endif
    }

}

void ImageWindow::notify(const Symbol message, const void *msgdata, const char *msg)
{
    if (message == DXApplication::MsgPanelChanged) {
        //
        // Set the command activations that depend on the number of panels.
        //
	if (this->setPanelAccessCmd) {
	    if(this->node AND (this->network->getPanelCount() > 0))
		this->setPanelAccessCmd->activate();
	    else
		this->setPanelAccessCmd->deactivate();
	}
    } 
    this->DXWindow::notify(message, msgdata, msg);

}

void ImageWindow::createHelpMenu(Widget parent)
{
    ASSERT(parent);

    this->DXWindow::createHelpMenu(parent);

    XtVaCreateManagedWidget("separator", xmSeparatorWidgetClass,
                                        this->helpMenuPulldown,
                                        NULL);

    this->onVisualProgramOption =
        new ButtonInterface(this->helpMenuPulldown,
                                "imageOnVisualProgramOption", 
				this->network->getHelpOnNetworkCommand());
}



void ImageWindow::createMenus(Widget parent)
{
    ASSERT(parent);


    //
    // Create the menus.
    //
    this->createFileMenu(parent);
    this->createExecuteMenu(parent);
    this->createWindowsMenu(parent);
    this->createConnectionMenu(parent);
    this->createOptionsMenu(parent);
    this->createHelpMenu(parent);

    //
    // Right justify the help menu (if it exists).
    //
    if (this->helpMenu)
    {
	XtVaSetValues(parent, XmNmenuHelpWidget, this->helpMenu, NULL);
    }
}

void ImageWindow::setDisplayGlobe()
{
     ((ToggleButtonInterface*)this->displayRotationGlobeOption)->toggleState();
     this->state.globeDisplayed = ((ToggleButtonInterface*)
				this->displayRotationGlobeOption)->getState();

     if (this->state.hardwareRender)
     {
	long l = this->state.globeDisplayed;
	this->sendClientMessage(this->atoms.display_globe,l);
     }
     else
     	XtVaSetValues(this->canvas, 
		      XmNdisplayGlobe, 
		      this->state.globeDisplayed, 
		      NULL);
}

/*****************************************************************************/
/* uinAssignDisplayString -						     */
/*                                                                           */
/*                                                                           */
/*****************************************************************************/
char *ImageWindow::getDisplayString()
{
#if defined(HAVE_SYS_UTSNAME_H)
	struct
		utsname   name;
#else
#define HOST_NAMELEN 33
	char *cp;
	struct    _dummy_utsname 
	{
		char    nodename[HOST_NAMELEN];
	} name;
#endif   
	Window    window;
	Window    child;
	boolean   frame_buffer;
	int       x;
	int       y;
	char*     display;
	char      host[64];
	char      unit[16];
	static char      string[512];

	if (this->execute_after_resize_to) XtRemoveTimeOut (this->execute_after_resize_to);
	this->execute_after_resize_to = 0;

	//
	// If there is a pending resize, then make sure it gets
	// processed first so that the proper WHERE param is sent.
	//
	if (this->hasPendingWindowPlacement()) {
		if (this->reset_eor_wp) XtRemoveWorkProc (this->reset_eor_wp);
		this->reset_eor_wp = 0;
		this->setGeometry(this->pending_resize_x, this->pending_resize_y,
			this->pending_resize_width, this->pending_resize_height);
		this->setExecuteOnResize(TRUE);
	}

	/*
	* Determine whether we're working with a frame buffer or not....
	*/
	window = XtWindow(this->getCanvas());

	frame_buffer = this->state.frameBuffer;

	DisplayNode *in = (DisplayNode *)this->node;
	/*
	* Determine the X server host.
	*/
	display = DisplayString(theApplication->getDisplay());
#if	defined(DXD_WIN)
	/* 
	as DISPLAY Enviroment is haveing some king of problen with EXCEED...
	if DISPLAY is set to be DXPENT:0 (which is hostname:o), DXUI startup
	gives an error "Unable to open CONNECT Strean". This problem is also
	encountered while running EXCEED samples also.

	If it is set to be "localhost:o" than eEXEC fails in "gethostname()".

	*/
#endif
	if (display)
	{
#if defined(HAVE_SYS_UTSNAME_H)
		if (uname(&name) < 0)
#else
		if (gethostname(name.nodename, HOST_NAMELEN) < 0)
#endif
		{
			return NULL;
		}
#if !defined(HAVE_SYS_UTSNAME_H)
		cp = strchr(name.nodename,'.');
		if (cp != NULL) 
		{
			*cp = '\0';
		}
#endif
		const char *serverHost;
		theDXApplication->getServerParameters(NULL, &serverHost,
			NULL, NULL, NULL, NULL, NULL);
		if (sscanf(display, "%[^:]:%s", host, unit) == 2)
		{
			const char* group_name =
#if WORKSPACE_PAGES
				in->getGroupName(theSymbolManager->getSymbol(PROCESS_GROUP));
#else
				in->getGroupName();
#endif
#ifdef	DXD_OS_NON_UNIX
			if(!DXChild::HostIsLocal(serverHost))
#else
			if (EqualString(host, "unix") &&
				(group_name != NULL ||
				!DXChild::HostIsLocal(serverHost)))
#endif
			{
				display = new char[ STRLEN(name.nodename) + STRLEN(unit) + 4];
				SPRINTF(display, "%s:%s", name.nodename, unit);
			}
			else
			{
				display = DuplicateString(display);
			}
		}
		else if (sscanf(display, ":%s", unit) == 1)
		{
			const char* group_name =
#if WORKSPACE_PAGES
				in->getGroupName(theSymbolManager->getSymbol(PROCESS_GROUP));
#else
				in->getGroupName();
#endif
			if (group_name != NULL || !DXChild::HostIsLocal(serverHost))
			{

				display = new char[STRLEN(name.nodename) + STRLEN(unit) + 4];
				SPRINTF(display, "%s:%s", name.nodename, unit);
			}
			else
			{
				display = DuplicateString(display);
			}
		}
		else
		{
			return NULL;
		}
	}
	else
	{
		(void)gethostname(host, 63);
#if defined(HAVE_SYS_UTSNAME_H)
		if (uname(&name) < 0)
#else
		if (gethostname(name.nodename, HOST_NAMELEN) < 0)
#endif
		{
			return NULL;
		}
#if !defined(HAVE_SYS_UTSNAME_H)
		cp = strchr(name.nodename,'.');
		if (cp != NULL) 
		{
			*cp = '\0';
		}
#endif

		display = new char[STRLEN(name.nodename) + STRLEN(unit) + 4];
		SPRINTF(display, "%s:0", name.nodename);
	}

	/*
	* Compose the display string accordingly.
	*/
	if (!frame_buffer)
	{
		SPRINTF(string, "X%d,%s,##%ld", 
			in->getDepth(), display, window);
	}
	else
	{
		XTranslateCoordinates
			(XtDisplay(this->getCanvas()),
			XtWindow(this->getCanvas()),
			RootWindowOfScreen(XtScreen(this->getCanvas())),
			0, 0,
			&x, &y,
			&child);

#ifdef FB_FLAG
		unsigned int flag = 0x00000000;
		if(in->isLastImage())
			flag |= FB_WHERE_SWAP;

		SPRINTF(string,
			"FB,%s,%d,%d,##%d,%#010x",
			display,
			(in->isLastImage() ? x : -(1 + x)),
			y,
			window,
			flag);
#endif
		SPRINTF(string,
			"FB,%s,%d,%d,##%ld",
			display,
			(in->isLastImage() ? x : -(1 + x)),
			y,
			window);
	}
	delete display;

	return string;
}
extern "C" void ImageWindow_RedrawCB(Widget	drawingArea,
			   XtPointer	clientData,
			   XtPointer	callData)
{
    XmPictureCallbackStruct    *pictureData =(XmPictureCallbackStruct*)callData;
    XEvent		       *event = pictureData->event;
    ImageWindow		       *obj = (ImageWindow*) clientData;

    Atom		actual_type;
    int			actual_format;
    unsigned long	n_items;
    unsigned long	bytes_after;
    Pixmap	       *pixmap;

    /*
     * Get the pixmap id from the window property list each time
     * in case the pixmap has been changed by the SVS server.
     */
    XGetWindowProperty
	(XtDisplay(drawingArea),
	 event->xexpose.window,
	 obj->atoms.dx_pixmap_id,
	 0,
	 1,
	 FALSE,
	 XA_PIXMAP,
	 &actual_type,
	 &actual_format,
	 &n_items,
	 &bytes_after,
	 (unsigned char**)&pixmap);
    
    /*
     * Draw the image if we have a pixmap to use...
     */
    if (actual_type != None)
    {
	obj->state.pixmap = *pixmap;
    
	if(!obj->state.gc)
	{
	    //
	    // Create Graphics Context
	    //
	    obj->state.gc = XCreateGC
		(XtDisplay(obj->getCanvas()),
		 XtWindow(obj->getCanvas()),
		 NUL(long),
		 NUL(XGCValues*));
	    Pixel bg;
	    XtVaGetValues(obj->getCanvas(), XmNbackground, &bg, NULL);
	    XSetForeground(XtDisplay(obj->getCanvas()), obj->state.gc, bg);
	}

	XCopyArea
	    (XtDisplay(drawingArea),
	     obj->state.pixmap,
	     XtWindow(obj->getCanvas()),
	     obj->state.gc,
	     0, 0,
	     obj->state.width,
	     obj->state.height,
	     0, 0);
	Dimension wwidth, wheight;
	XtVaGetValues(obj->getCanvas(),
	    XmNwidth, &wwidth,
	    XmNheight, &wheight,
	    NULL);
	if (wwidth > obj->state.width)
	{
	    XFillRectangle(XtDisplay(obj->getCanvas()),
			   XtWindow(obj->getCanvas()),
			   obj->state.gc, 
			   obj->state.width, 0,
			   wwidth - obj->state.width, wheight);
	}
	if (wheight > obj->state.height)
	{
	    XFillRectangle(XtDisplay(obj->getCanvas()),
			   XtWindow(obj->getCanvas()),
			   obj->state.gc, 
			   0, obj->state.height,
			   obj->state.width, wheight - obj->state.height);
	}

    
	XFlush(XtDisplay(drawingArea));
	XtVaSetValues(obj->getCanvas(), 
	    XmNpicturePixmap, obj->state.pixmap, NULL);
	XFree((char*)pixmap);
    }
    else
    {
	XClearWindow(XtDisplay(drawingArea), XtWindow(drawingArea));
    }
}
extern "C" void ImageWindow_ZoomCB(Widget	 	drawingArea,
		         XtPointer	clientData,
		         XtPointer	callData)
{
    ((ImageWindow*)clientData)->zoomImage((XmPictureCallbackStruct*)callData);

}

void ImageWindow::zoomImage(XmPictureCallbackStruct* pictureData)
{
    boolean execOnChange = theDXApplication->getExecCtl()->inExecOnChange();


    /*
     * NOP if we have not recieved an image from the server.
     */
    if (this->state.pixmap == NUL(Pixmap))
    {
	return;
    }

    ImageNode *in = (ImageNode *)this->node;

    if(pictureData->projection == 0)
    {
	double w;
	in->getWidth(w);
	w *= pictureData->zoom_factor;
	in->setWidth(w, FALSE);
    }
    else if(pictureData->projection == 1)
    {
	in->setViewAngle(pictureData->view_angle, FALSE);
    }

    double v[3];
    v[0] = pictureData->x;
    v[1] = pictureData->y;
    v[2] = pictureData->z;
    in->setTo(v, FALSE);
    v[0] = pictureData->from_x;
    v[1] = pictureData->from_y;
    v[2] = pictureData->from_z;
    in->setFrom(v, TRUE);

    if (!execOnChange)
	theDXApplication->getExecCtl()->executeOnce();
}

extern "C" void ImageWindow_RoamCB(Widget	 	drawingArea,
		         XtPointer	clientData,
		         XtPointer	callData)
{
    ((ImageWindow*)clientData)->roamImage((XmPictureCallbackStruct*)callData);
}

void ImageWindow::roamImage(XmPictureCallbackStruct* calldata)
{
    double 	   v[3];
    ViewControlDialog	       *viewCtl = this->viewControlDialog;
    ImageCamera    	       *camera = &this->state.hardwareCamera;
    ImageNode 		       *in = (ImageNode *)this->node;
    boolean execOnChange = theDXApplication->getExecCtl()->inExecOnChange();


    if (calldata->reason != XmPCR_MOVE)
    {
        return;
    }

    if (this->state.pixmap == NUL(Pixmap))
    {
        return;
    }

    if(viewCtl)
	viewCtl->resetSetView();

    in->getFrom(camera->from);
    in->getTo(camera->to);

    v[0] = calldata->x;
    v[1] = calldata->y;
    v[2] = calldata->z;
    in->setTo(v, FALSE);

    if(NOT this->getPerspective()) 
    {
        v[0] = calldata->x + camera->from[0] - camera->to[0];
        v[1] = calldata->y + camera->from[1] - camera->to[1];
        v[2] = calldata->z + camera->from[2] - camera->to[2];
    }
    else
    	in->getFrom(v);

    in->setFrom(v, TRUE);

    if (!execOnChange)
	theDXApplication->getExecCtl()->executeOnce();
}

extern "C" void ImageWindow_NavigateCB(Widget	drawingArea,
			     XtPointer	clientData,
			     XtPointer	callData)
{
    ((ImageWindow*)clientData)->navigateImage((XmPictureCallbackStruct*)callData);

}

void ImageWindow::navigateImage(XmPictureCallbackStruct* pictureData)
{
    ImageNode *in = (ImageNode *)this->node;

    if (pictureData->reason == XmPCR_SELECT)
    {
	in->setButtonUp(FALSE, FALSE);
    }

    if (pictureData->reason == XmPCR_DRAG || 
	pictureData->reason == XmPCR_SELECT)
    {
	/*
	 * Update the Set View option menu to "None"
	 */
	if (this->viewControlDialog)
	    this->viewControlDialog->resetSetView();

	// Stuff to do the first time.
	if (pictureData->reason == XmPCR_SELECT)
	{
	    /*
	     * Update the option menu to indicate we are in perspective.
	     * The autocamera param assignment happens in _uipNavigateImage.
	     */
	    boolean p;
	    in->getProjection(p);
	    if (!p)
	    {
		in->setProjection(TRUE, FALSE);
		if (this->viewControlDialog)
		    this->viewControlDialog->resetProjection();
	    }
	}

	double v[3];
	v[0] = pictureData->x;
	v[1] = pictureData->y;
	v[2] = pictureData->z;
	in->setTo(v, FALSE);
	v[0] = pictureData->up_x;
	v[1] = pictureData->up_y;
	v[2] = pictureData->up_z;
	in->setUp(v, FALSE);
	v[0] = pictureData->from_x;
	v[1] = pictureData->from_y;
	v[2] = pictureData->from_z;
	in->setFrom(v, TRUE);

	theDXApplication->getExecCtl()->enableExecOnChange();
    }
    else if (pictureData->reason == XmPCR_TRANSLATE_SPEED)
    {
	this->setTranslateSpeed(pictureData->translate_speed);
    }
    else if (pictureData->reason == XmPCR_ROTATE_SPEED)
    {
	this->setRotateSpeed(pictureData->rotate_speed);
    }
    if (pictureData->reason == XmPCR_MOVE)
    {
	in->setButtonUp(TRUE);
    }
}

extern "C" void ImageWindow_RotationCB(Widget	drawingArea,
			     XtPointer	clientData,
			     XtPointer	callData)
{
    ((ImageWindow*)clientData)->rotateImage((XmPictureCallbackStruct*)callData);

}

void ImageWindow::rotateImage(XmPictureCallbackStruct* pictureData)
{
    boolean execOnChange = theDXApplication->getExecCtl()->inExecOnChange();

    /*
     * NOP if we have not recieved an image from the server.
     */
    if (this->state.pixmap == NUL(Pixmap) || !this->directInteractionAllowed())
    {
	return;
    }

    if ( ((pictureData->reason == XmPCR_DRAG) ||
	  (pictureData->reason == XmPCR_SELECT)) && 
	 !execOnChange)
	return;

    ImageNode *in = (ImageNode*)this->node;

    if (pictureData->reason == XmPCR_SELECT)
    {
	double v[3];
	in->getFrom(v);
	in->setButtonUp(FALSE, TRUE);
	return;
    }

    /*
     * Update the Set View option menu to "None"
     */
    if (this->viewControlDialog)
    {
	this->viewControlDialog->resetSetView();
	this->viewControlDialog->resetLookDirection();
    }

    if (pictureData->reason == XmPCR_MOVE)
	in->setButtonUp(TRUE, FALSE);

    double v[3];
    v[0] = pictureData->from_x;
    v[1] = pictureData->from_y;
    v[2] = pictureData->from_z;
    in->setFrom(v, FALSE);
    v[0] = pictureData->up_x;
    v[1] = pictureData->up_y;
    v[2] = pictureData->up_z;
    in->setUp(v, TRUE);

    if (!execOnChange)
	theDXApplication->getExecCtl()->executeOnce();
}
extern "C" void ImageWindow_CursorCB(Widget	drawingArea,
			   XtPointer	clientData,
			   XtPointer	callData)
{
    XmPictureCallbackStruct    *pictureData =(XmPictureCallbackStruct*)callData;
    ImageWindow		       *image = (ImageWindow*) clientData;

    image->handleCursor(pictureData->reason, pictureData->cursor_num,
			pictureData->x, pictureData->y, pictureData->z);
}

void ImageWindow::handleCursor(int reason, int cursor_num,
			       double x, double y, double z)
{
    if(NOT this->state.hardwareRender AND this->state.pixmap == 0)
      return;

    ASSERT(this->currentProbeInstance > 0);
    ProbeNode* probe = (ProbeNode*)this->network->getNode("Probe",
					this->currentProbeInstance);
    if (!probe)
    	probe = (ProbeNode*)this->network->getNode("ProbeList",
					this->currentProbeInstance);
    ASSERT(probe);

    Parameter* param = probe->getOutput();
    long       l[5];


    ImageNode *in = (ImageNode*)this->node;

    switch(reason)
    {
    case XmPCR_SELECT:
	in->setButtonUp(FALSE, TRUE);
	break;

    case XmPCR_CREATE:
	if(EqualString(probe->getNameString(),"Probe")
	   AND param->hasValue())
	{
	    if(NOT this->state.hardwareRender)
		XmPictureDeleteCursors((XmPictureWidget)this->canvas,
				       cursor_num);
	    else
	    {
		l[0] = cursor_num;
		l[1] = XmPCR_DELETE;
		this->sendClientMessage(this->atoms.cursor_change, 2, l);
	    }
	    XmPictureResetCursor((XmPictureWidget)this->canvas);
	    ErrorMessage("The maximum number of cursors for a probe is 1");
	}
	else
	{ 
	    if(STRLEN(probe->getOutputValueString(1))+100 > UI_YYLMAX)
	    {
		if(NOT this->state.hardwareRender)
		    XmPictureDeleteCursors((XmPictureWidget)this->canvas,
					   cursor_num);
		else
		{
		    l[0] = cursor_num;
		    l[1] = XmPCR_DELETE;
		    this->sendClientMessage(this->atoms.cursor_change, 2, l);
		}
		XmPictureResetCursor((XmPictureWidget)this->canvas);
		ErrorMessage("Sorry, you can't add more probes in this probe list.");
	    }
	    else
		//
		// c1wright50
		// use -1 in software mode because (hack) there will always be
		// a motionnotify event to follow, but not in hardware mode.
		if (this->state.hardwareRender)
		    probe->setCursorValue(cursor_num, x, y, z);
		else
		    probe->setCursorValue(-1, x, y, z);
	}
	break;

    case XmPCR_DRAG:
	probe->setCursorValue(cursor_num, x, y, z);
	break;

    case XmPCR_MOVE:
	in->setButtonUp(TRUE, TRUE);
	probe->setCursorValue(cursor_num, x, y, z);
	break;

    case XmPCR_DELETE:
	probe->deleteCursor(cursor_num);
	break;

    default:
	ASSERT(False);
    }
}
extern "C" void ImageWindow_PickCB(Widget	drawingArea,
			   XtPointer	clientData,
			   XtPointer	callData)
{
    XmPictureCallbackStruct    *pictureData =(XmPictureCallbackStruct*)callData;
    ImageWindow		       *image = (ImageWindow*) clientData;

    image->pickImage(pictureData->x, pictureData->y);
}

void ImageWindow::pickImage(double x, double y)
{
    if (!this->node->isA(ClassImageNode) ||
	this->getInteractionMode() != PICK)
	return;
    
    if (this->currentPickInstance > 0) {
	PickNode *p = this->getCurrentPickNode();
	p->setCursorValue(-1, x, y, 0.0);
    }
}

extern "C" void ImageWindow_ModeCB(Widget	 	drawingArea,
		         XtPointer	clientData,
		         XtPointer	callData)
{
    XmPictureCallbackStruct    *pictureData =(XmPictureCallbackStruct*)callData;
    ImageWindow		       *obj = (ImageWindow*) clientData;

    if (NOT XtIsRealized(drawingArea) ||
	!obj->directInteractionAllowed())
    {
	return;
    }

    obj->handleMode(pictureData);
}

void ImageWindow::handleMode(XmPictureCallbackStruct *pictureData)
{
    switch (pictureData->mode) {
    case XmROAM_MODE:
	if (this->modeRoamCmd->isActive())
	    this->modeRoamCmd->execute();
	break;
    case XmCURSOR_MODE:
	if (this->modeCursorsCmd->isActive())
	    this->modeCursorsCmd->execute();
	break;
    case XmPICK_MODE:
	if (this->modePickCmd->isActive())
	    this->modePickCmd->execute();
	break;
    case XmROTATION_MODE:
	if (this->modeRotateCmd->isActive())
	    this->modeRotateCmd->execute();
	break;
    case XmZOOM_MODE:
	if (this->modeZoomCmd->isActive())
	    this->modeZoomCmd->execute();
	break;
    case XmPANZOOM_MODE:
	if (this->modePanZoomCmd->isActive())
	    this->modePanZoomCmd->execute();
	break;
    case XmNAVIGATE_MODE:
	if (this->modeNavigateCmd->isActive())
	    this->modeNavigateCmd->execute();
	break;
    default:
	ASSERT(FALSE);
    }
}

extern "C" void ImageWindow_KeyCB(Widget	 	drawingArea,
		        XtPointer	clientData,
		        XtPointer	callData)
{
    XmPictureCallbackStruct    *pictureData =(XmPictureCallbackStruct*)callData;
    ImageWindow		       *obj = (ImageWindow*) clientData;

    
    if (NOT XtIsRealized(drawingArea) ||
	!obj->directInteractionAllowed())
    {
	return;
    }

    obj->handleKey(pictureData);
}

void ImageWindow::handleKey(XmPictureCallbackStruct *pictureData)
{
    switch (pictureData->keysym) {
    case XK_k:
    case XK_K:
	if (this->modeCameraCmd->isActive())
	    this->modeCameraCmd->execute();
	this->postViewControlDialog();
	break;

    case XK_f:
    case XK_F:
	if (this->resetCmd->isActive())
	    this->resetCmd->execute();
	break;

    default:
	ASSERT(FALSE);
    }
}
extern "C" void ImageWindow_UndoCB(Widget	 	drawingArea,
		         XtPointer	clientData,
		         XtPointer	callData)
{
    ImageWindow		       *obj = (ImageWindow*) clientData;

    obj->undoCamera();
}

extern "C" void ImageWindow_ResizeCB(Widget	drawingArea,
			   XtPointer	clientData,
			   XtPointer	callData)
{
    ImageWindow* iw = (ImageWindow*)clientData;

    // Without this check, the resize event will cause an execution if
    // the image window is being managed for the 1st time ever.
    if (!iw->isManaged()) return ;

    //
    // We used to proceed with this->imageResize() in response to a resize
    // event.  Now we pause for a short time in order to wait for more
    // resize events to arrive.  The length of the pause - well it's a hack
    // really - is arbitrary.  This problem this is intended to work around
    // is the excessive amount of executing that's requested when displaying
    // on a redhat or like system whose window manage resizes windows
    // immediately rather than rubberbanding the new size the way Mwm always
    // used to.
    //
    if (iw->execute_after_resize_to)
	XtRemoveTimeOut (iw->execute_after_resize_to);
    XtAppContext apcxt = theApplication->getApplicationContext();
    iw->execute_after_resize_to = XtAppAddTimeOut (apcxt, 500, (XtTimerCallbackProc)
	ImageWindow_ResizeTO, (XtPointer)iw);
}

extern "C" void ImageWindow_ResizeTO (XtPointer clientData, XtIntervalId* )
{
    ImageWindow* iw = (ImageWindow*)clientData;
    iw->execute_after_resize_to = 0;
    iw->resizeImage();

    if(iw->printImageDialog AND iw->printImageDialog->isManaged())
	iw->printImageDialog->update();
    if(iw->saveImageDialog AND iw->saveImageDialog->isManaged())
	iw->saveImageDialog->update();
}

//
// The XmPictureCallbackStruct may be NULL
//
void ImageWindow::resizeImage(boolean ok_to_send)
{
    boolean execOnChange = theDXApplication->getExecCtl()->inExecOnChange();
    Widget canvas = this->getCanvas();

    //
    // If we were called because the server changed the size.
    // do not restart an execution sequence    OR
    // If this window is not associated with an Image node,
    // ignore this request.
    //
    if (this->node == NUL(Node*)) return ;
    if (this->node->isA(ClassImageNode) == FALSE) return ;
    if (this->state.resizeFromServer) return ;

    ImageNode *in = (ImageNode *)this->node;

    //
    // Otherwise, get the new width and height.
    // Dimensions are sometimes 0 when the window is being created and
    // initial geometry negotiation is going on.
    //
    Dimension width, height;
    XtVaGetValues(canvas, XmNwidth,  &width, XmNheight, &height, NULL);
    if (width > 0) {
	in->setResolution(width, height, FALSE);
	double aspect = (height + 0.5) / width;
	if (ok_to_send) {
	    in->setAspect(aspect, this->isManaged());

	    //
	    // Prevent an execution if the image window is unmanaged.
	    //
	    boolean force = ((this->isExecuteOnResize()) && (!execOnChange));
	    if ((this->isManaged()) && (force))
		theDXApplication->getExecCtl()->executeOnce();

	    if (this->viewControlDialog && this->directInteractionAllowed()) 
		this->viewControlDialog->setSensitivity(in->useVector());
	} else {
	    in->setAspect(aspect, FALSE);
	}
    }
}

void ImageWindow_PropertyNotifyCB(Widget    imageWindow,
			      XtPointer clientData,
			      XtPointer callData)
{
XmPictureCallbackStruct *pictureData =(XmPictureCallbackStruct*)callData;
XEvent			*event = pictureData->event;
XEvent			e;
ImageWindow		*obj = (ImageWindow*) clientData;
Atom			required_type;
Atom			actual_type;
int			actual_format;
unsigned long		n_items;
unsigned long		bytes_after;
Window			*window;
Widget			shell;
XWindowAttributes 	attributes;
Boolean			iconic;

    if(event->xproperty.state == PropertyDelete)
	return; 

    required_type = XInternAtom(XtDisplay(imageWindow), "GL_WINDOW_ID", False);
    if(required_type != event->xproperty.atom)
    {
	// If  the property is not for GL rendering, return.
	return;
    }

    //
    // Determine if we are iconified
    //
    if(!obj->isManaged())
    {
	iconic = False;
    }
    else
    {
	shell = imageWindow;
	while(!XtIsShell(shell)) shell = XtParent(shell);

	XGetWindowAttributes(XtDisplay(imageWindow),
			     XtWindow(shell),
			     &attributes);
	if(attributes.map_state == IsUnmapped) 
	    iconic = True;
	else
	    iconic = False;
    }

    if(XGetWindowProperty
        (XtDisplay(imageWindow),
         event->xproperty.window,
         event->xproperty.atom,
         0,
         1,
         FALSE,
         required_type,
         &actual_type,
         &actual_format,
         &n_items,
         &bytes_after,
         (unsigned char**)&window) != Success)
    {
        WarningMessage("XGetWindowProperty failed in (XtCallbackProc)ImageWindow_PropertyNotifyCB"); 
	return;
    }

    if(actual_type != required_type)
    {
        // This case happens when the property is destroyed.
	XFree(window);
        return;
    }

    if(!iconic)
    {
	// Ask for Map Events
	XSelectInput(XtDisplay(imageWindow), 
		     *window, 
		     StructureNotifyMask);

	obj->manage();

	// Map it for the exec
	XMapWindow(XtDisplay(imageWindow), *window);

	// Make sure we are inited to something other than MapNotify
	e.type = MapNotify - 1;

	// Wait for the map event
	while(e.type != MapNotify)
	    XWindowEvent(XtDisplay(imageWindow), 
			 *window, 
			 StructureNotifyMask, 
			 &e);
    }

    // This is the signal to the exec that the window has been mapped
    XDeleteProperty
	(XtDisplay(imageWindow),
	 event->xproperty.window,
	 event->xproperty.atom);

    XFree(window);
}
void ImageWindow_ClientMessageCB(Widget    imageWindow,
			     XtPointer clientData,
			     XtPointer callData)
{
    XmPictureCallbackStruct    *pictureData =(XmPictureCallbackStruct*)callData;
    XEvent		       *event = pictureData->event;
    ImageWindow		       *obj = (ImageWindow*) clientData;
    ImageNode			*in = (ImageNode*)obj->node;
    Dimension			width;
    Dimension			height;

    //
    // Don't access in if it is not an image node.
    //
    if(!obj->node->isA(ClassImageNode)) in = NULL;

    if(event->xclient.message_type == obj->atoms.gl_shutdown)
    {
	obj->state.hardwareRender = False;
	obj->state.hardwareRenderExists = False;
    }
    else if (event->xclient.message_type == obj->atoms.from_vector)
    {
	float temp;
	memcpy(&temp, &event->xclient.data.l[0], sizeof(float));
	obj->state.hardwareCamera.from[0] = (double)temp;
	memcpy(&temp, &event->xclient.data.l[1], sizeof(float));
	obj->state.hardwareCamera.from[1] = (double)temp;
	memcpy(&temp, &event->xclient.data.l[2], sizeof(float));
	obj->state.hardwareCamera.from[2] = (double)temp;
    }
    else if (event->xclient.message_type == obj->atoms.motion)
    {
	int itemp;
	memcpy(&itemp, &event->xclient.data.l[0], sizeof(int));
	XtVaSetValues(obj->getCanvas(),
	    XmNtranslateSpeed, itemp,
	    NULL);
	if (obj->viewControlDialog)
	    obj->viewControlDialog->setNavigateSpeed(itemp);
    }
    else if (event->xclient.message_type == obj->atoms.pivot)
    {
	int itemp;
	memcpy(&itemp, &event->xclient.data.l[0], sizeof(int));
	XtVaSetValues(obj->getCanvas(),
	    XmNrotateSpeed, itemp,
	    NULL);
	if (obj->viewControlDialog)
	    obj->viewControlDialog->setNavigatePivot(itemp);
    }
    else if (event->xclient.message_type == obj->atoms.undoable)
    {
	obj->state.hardwareCamera.undoable = (boolean)event->xclient.data.l[0];
	if (obj->state.hardwareCamera.undoable)
	    obj->undoCmd->activate();
	else
	    obj->undoCmd->deactivate();
    }
    else if (event->xclient.message_type == obj->atoms.redoable)
    {
	obj->state.hardwareCamera.redoable = (boolean)event->xclient.data.l[0];
	if (obj->state.hardwareCamera.redoable)
	    obj->redoCmd->activate();
	else
	    obj->redoCmd->deactivate();
    }
    else if (event->xclient.message_type == obj->atoms.up_vector)
    {
    /*
     * NOP if we have not recieved an image from gl.
     */
	if (obj->state.hardwareWindow == NUL(Window) ||
	    !obj->directInteractionAllowed())
	{
	    return;
	}
	double v[3];
	float temp;

	memcpy(&temp, &event->xclient.data.l[0], sizeof(float));
	v[0] = (double)temp;
	memcpy(&temp, &event->xclient.data.l[1], sizeof(float));
	v[1] = (double)temp;
	memcpy(&temp, &event->xclient.data.l[2], sizeof(float));
	v[2] = (double)temp;

	if(in)
	{
	    in->setFrom(obj->state.hardwareCamera.from, FALSE);
	    in->setUp(v);
	}

	boolean execOnChange = theDXApplication->getExecCtl()->inExecOnChange();
	if (!execOnChange)
	    theDXApplication->getExecCtl()->executeOnce();
    }

    /*
     * Roam message from GL:
     */
    else if (event->xclient.message_type == obj->atoms.roam)
    {
    /*
     * NOP if we have not recieved an image from the server.
     */
	if (obj->state.hardwareWindow == NUL(Window) || 
	    !obj->directInteractionAllowed())
	{
	    return;
	}

	float x, y, z;
	memcpy(&x, &event->xclient.data.l[0], sizeof(float));
	memcpy(&y, &event->xclient.data.l[1], sizeof(float));
	memcpy(&z, &event->xclient.data.l[2], sizeof(float));


	if (obj->state.hardwareCamera.projection == 0)
	{
	    double from[3];
	    double dir_x, dir_y, dir_z;
	    dir_x = obj->state.hardwareCamera.from[0] - 
			obj->state.hardwareCamera.to[0];
	    dir_y = obj->state.hardwareCamera.from[1] - 
			obj->state.hardwareCamera.to[1];
	    dir_z = obj->state.hardwareCamera.from[2] - 
			obj->state.hardwareCamera.to[2];
	    if(in)
	    {
		from[0] = dir_x + x;
		from[1] = dir_y + y;
		from[2] = dir_z + z;
		in->setFrom(from, FALSE);
	    }
	}

	if(in)
	{
	    double to[3];
	    to[0] = x;
	    to[1] = y;
	    to[2] = z;
	    in->setTo(to, TRUE);
	}

	boolean execOnChange = theDXApplication->getExecCtl()->inExecOnChange();
	if (!execOnChange)
	    theDXApplication->getExecCtl()->executeOnce();
    }

    /*
     * Zoom1 message from GL:
     */
    else if (event->xclient.message_type == obj->atoms.zoom1)
    {
    /*
     * NOP if we have not recieved an image from the server.
     */
	if (obj->state.hardwareWindow == NUL(Window) ||
	    !obj->directInteractionAllowed())
	{
	    return;
	}

	memcpy(&obj->state.hardwareCamera.projection, 
			&event->xclient.data.l[0], sizeof(int));
	float temp;
	memcpy(&temp, &event->xclient.data.l[1], sizeof(float));
	obj->state.hardwareCamera.zoomFactor = temp;
	memcpy(&temp, &event->xclient.data.l[2], sizeof(float));
	obj->state.hardwareCamera.viewAngle = temp;
	memcpy(&temp, &event->xclient.data.l[3], sizeof(float));
	obj->state.hardwareCamera.to[0] = temp;
	memcpy(&temp, &event->xclient.data.l[4], sizeof(float));
	obj->state.hardwareCamera.to[1] = temp;

    }
    /*
     * Zoom2 message from GL:
     */
    else if (event->xclient.message_type == obj->atoms.zoom2)
    {
    /*
     * NOP if we have not recieved an image from the server.
     */
	if (obj->state.hardwareWindow == NUL(Window) ||
	    !obj->directInteractionAllowed())
	{
	    return;
	}

	/*
	 * Use the information from the Zoom1 message...
	 */
	if(in)
	{
	    if (obj->state.hardwareCamera.projection == 0)
	    {
		double w;
		in->getWidth(w);
		w *= obj->state.hardwareCamera.zoomFactor;
		in->setWidth(w, FALSE);
	    }
	    else
	    {
		in->setViewAngle(obj->state.hardwareCamera.viewAngle, FALSE);
	    }
	}
	float temp;
	memcpy(&temp, &event->xclient.data.l[0], sizeof(float));
	obj->state.hardwareCamera.to[2] = temp;
	memcpy(&temp, &event->xclient.data.l[1], sizeof(float));
	obj->state.hardwareCamera.from[0] = temp;
	memcpy(&temp, &event->xclient.data.l[2], sizeof(float));
	obj->state.hardwareCamera.from[1] = temp;
	memcpy(&temp, &event->xclient.data.l[3], sizeof(float));
	obj->state.hardwareCamera.from[2] = temp;

	if(in)
	{
	    in->setTo(obj->state.hardwareCamera.to, FALSE);
	    in->setFrom(obj->state.hardwareCamera.from, TRUE);
	}

	if (!theDXApplication->getExecCtl()->inExecOnChange())
	    theDXApplication->getExecCtl()->executeOnce();
    }

    /*
     * Cursor Change message from GL:
     */
    else if (event->xclient.message_type == obj->atoms.cursor_change)
    {
	/*
	 * NOP if we have not recieved an image from gl_render.
	 */
	if (obj->state.hardwareWindow == NUL(Window))
	{
	    return;
	}

	int cursor_num = event->xclient.data.l[0];
	int reason     = event->xclient.data.l[1];
	float temp;
	double x, y, z;

	memcpy(&temp, &event->xclient.data.l[2], sizeof(float));
	x = (double)temp;
	memcpy(&temp, &event->xclient.data.l[3], sizeof(float));
	y = (double)temp;
	memcpy(&temp, &event->xclient.data.l[4], sizeof(float));
	z = (double)temp;
	obj->handleCursor(reason, cursor_num, x, y, z);
    }

    /*
     *  Image available from GL:
     */
    else if (event->xclient.message_type == obj->atoms.gl_window0)
    {

	/*
	 * Save the info.
	 */
	obj->state.hardwareRender = TRUE;
	obj->state.hardwareRenderExists = TRUE;
	obj->state.hardwareWindow = event->xclient.data.l[0];
	float temp;
	memcpy(&temp, &event->xclient.data.l[1], sizeof(float));
	obj->state.hardwareCamera.to[0] = temp;
	memcpy(&temp, &event->xclient.data.l[2], sizeof(float));
	obj->state.hardwareCamera.to[1] = temp;
	memcpy(&temp, &event->xclient.data.l[3], sizeof(float));
	obj->state.hardwareCamera.to[2] = temp;
	memcpy(&temp, &event->xclient.data.l[4], sizeof(float));
	obj->state.hardwareCamera.up[0] = temp;

        obj->configureImageDepthMenu();
    }   
    else if (event->xclient.message_type == obj->atoms.gl_window1)
    {
	/*
	 * Save the info.
	 */
	float temp;
	memcpy(&temp, &event->xclient.data.l[0], sizeof(float));
	obj->state.hardwareCamera.up[1] = temp;
	memcpy(&temp, &event->xclient.data.l[1], sizeof(float));
	obj->state.hardwareCamera.up[2] = temp;
	memcpy(&temp, &event->xclient.data.l[2], sizeof(float));
	obj->state.hardwareCamera.from[0] = temp;
	memcpy(&temp, &event->xclient.data.l[3], sizeof(float));
	obj->state.hardwareCamera.from[1] = temp;
	memcpy(&temp, &event->xclient.data.l[4], sizeof(float));
	obj->state.hardwareCamera.from[2] = temp;
    }   
    else if ( (event->xclient.message_type == obj->atoms.gl_window2) ||
	      (event->xclient.message_type == obj->atoms.gl_window2_execute) )
    {
	obj->pushedSinceExec = FALSE;

	float temp;
	memcpy(&temp, &event->xclient.data.l[0], sizeof(float));
	obj->state.hardwareCamera.width = temp;
	obj->state.hardwareCamera.windowWidth = event->xclient.data.l[1];
	memcpy(&temp, &event->xclient.data.l[2], sizeof(float));
	obj->state.hardwareCamera.aspect = temp;
	memcpy(&temp, &event->xclient.data.l[3], sizeof(float));
	obj->state.hardwareCamera.viewAngle = temp;
	obj->state.hardwareCamera.projection = event->xclient.data.l[4];

	obj->state.hardwareCamera.windowHeight = (int)
	  ( obj->state.hardwareCamera.aspect *
	    obj->state.hardwareCamera.windowWidth );

	//
	// Seems like we should keep these in sync.
	//
	obj->state.width  = obj->state.hardwareCamera.windowWidth;
	obj->state.height = obj->state.hardwareCamera.windowHeight;

	/*
	 * If the projection is orthographic, calculate the equivalent view
	 * angle.  If it is perspective, calc the autocamera width.
	 */
	double x = obj->state.hardwareCamera.from[0] - 
		    obj->state.hardwareCamera.to[0];
	double y = obj->state.hardwareCamera.from[1] - 
		    obj->state.hardwareCamera.to[1];
	double z = obj->state.hardwareCamera.from[2] - 
		    obj->state.hardwareCamera.to[2];
	double dist = sqrt(x * x + y * y + z * z);

	if (obj->state.hardwareCamera.projection == 0)
	{
	    double t = atan((obj->state.hardwareCamera.width/2)/dist);
	    obj->state.hardwareCamera.viewAngle = (t * 360)/3.1415926;
	}
	else
	{
	    obj->state.hardwareCamera.width = 
		2*tan(3.1415926*obj->state.hardwareCamera.viewAngle/360.0)*dist;
	}

	
	if(in)
	{
	    in->setWidth(obj->state.hardwareCamera.width, FALSE);
	    in->setResolution(obj->state.hardwareCamera.windowWidth,
			      obj->state.hardwareCamera.windowHeight, FALSE);
	    in->setAspect(obj->state.hardwareCamera.aspect, FALSE);
	    in->setTo(obj->state.hardwareCamera.to, FALSE);
	    in->setUp(obj->state.hardwareCamera.up, FALSE);
	    in->setFrom(obj->state.hardwareCamera.from, FALSE);
	    in->setViewAngle(obj->state.hardwareCamera.viewAngle, FALSE);
	    in->setProjection(obj->state.hardwareCamera.projection, FALSE);
	}
	if (obj->viewControlDialog)
	{
	    obj->viewControlDialog->resetProjection();
	    obj->viewControlDialog->setWhichCameraVector();
	}

	if(in)
	    in->enableVector(TRUE, FALSE);
	    
	if (event->xclient.message_type == obj->atoms.gl_window2_execute)
	{
	    if(in)
		in->sendValues(FALSE);
	    boolean execOnChange =
		theDXApplication->getExecCtl()->inExecOnChange();
	    if (!execOnChange)
		theDXApplication->getExecCtl()->executeOnce();
	}
	else if (in)
	    in->sendValuesQuietly();

	//
	// Set the View Control Dialog Sensitivity
	//
	if (obj->viewControlDialog && obj->directInteractionAllowed())
	{
	    if(in)
		obj->viewControlDialog->setSensitivity(in->useVector());
	}
	if(!obj->isManaged()) obj->DXWindow::manage();

	obj->newCanvasImage();
	obj->allowDirectInteraction(in == NULL ? FALSE : TRUE);
    }   
    else if (event->xclient.message_type == obj->atoms.execute_on_change)
    {
	theDXApplication->executeOnChangeCmd->execute();
    }	
    else if (event->xclient.message_type == obj->atoms.pick_point)
    {
    /*
     * NOP if we have not recieved an image from the server.
     */
	if (obj->state.hardwareWindow == NUL(Window) ||
	    !obj->directInteractionAllowed())
	{
	    return;
	}

	float x,y;

	memcpy(&x, &event->xclient.data.l[0], sizeof(float));
	memcpy(&y, &event->xclient.data.l[1], sizeof(float));

	obj->pickImage((double)x, (double)y);
    }

    /*
     *  Image information message from the (Workstation) DX server:
     */
    else
    {
	obj->state.hardwareRender = FALSE;
	/*
	 * Resize the image.
	 */
	switch (event->xclient.format)
	{
	  case 16:
	    width  = event->xclient.data.s[0];
	    height = event->xclient.data.s[1];
	    //depth  = event->xclient.data.s[2];
	    break;

	  case 32:
	    width  = event->xclient.data.l[0];
	    height = event->xclient.data.l[1];
	    //depth  = event->xclient.data.l[2];
	    break;

	  default:
	    ErrorMessage("Invalid format of UI/Server client message");
	    return;
	}

	//
	// resize_from_server = True ==> do not cause a re-execution
	// in the resize callback.
	//

	obj->state.resizeFromServer = TRUE;
	XtVaSetValues(obj->getCanvas(), 
	    XmNwidth,  width,
	    XmNheight, height, NULL);
	obj->state.resizeFromServer = FALSE;
	obj->state.width  = width;
	obj->state.height = height;

	/*
	 * Get the pixmap id from the window property list each time
	 * in case the pixmap has been changed by the SVS server.
	 */
	Atom		actual_type;
	int		actual_format;
	unsigned long	n_items;
	unsigned long	bytes_after;
	Pixmap        *pixmap;
	XGetWindowProperty
	    (XtDisplay(imageWindow),
	     event->xclient.window,
	     obj->atoms.dx_pixmap_id,
	     0,
	     1,
	     FALSE,
	     XA_PIXMAP,
	     &actual_type,
	     &actual_format,
	     &n_items,
	     &bytes_after,
	     (unsigned char**)&pixmap);
    
	/*
	 * Draw the image if we have a pixmap to use...
	 */
	if (actual_type != None)
	{
	    obj->state.pixmap = *pixmap;
	    if(!obj->state.gc)
	    {
		//
		// Create Graphics Context
		//
		obj->state.gc = XCreateGC
		    (XtDisplay(obj->getCanvas()),
		     XtWindow(obj->getCanvas()),
		     NUL(long),
		     NUL(XGCValues*));
		Pixel bg;
		XtVaGetValues(obj->getCanvas(), XmNbackground, &bg, NULL);
		XSetForeground(XtDisplay(obj->getCanvas()), obj->state.gc, bg);
	    }
    
	    XCopyArea
		(XtDisplay(imageWindow),
		 obj->state.pixmap,
		 XtWindow(obj->getCanvas()),
		 obj->state.gc,
		 0, 0,
		 obj->state.width,
		 obj->state.height,
		 0, 0);

	    Dimension wwidth, wheight;
	    XtVaGetValues(obj->getCanvas(),
		XmNwidth, &wwidth,
		XmNheight, &wheight,
		NULL);
	    if (wwidth > obj->state.width)
	    {
		XFillRectangle(XtDisplay(obj->getCanvas()),
			       XtWindow(obj->getCanvas()),
			       obj->state.gc, 
			       obj->state.width, 0,
			       wwidth - obj->state.width, wheight);
	    }
	    if (wheight > obj->state.height)
	    {
		XFillRectangle(XtDisplay(obj->getCanvas()),
			       XtWindow(obj->getCanvas()),
			       obj->state.gc, 
			       0, obj->state.height,
			       obj->state.width, wheight - obj->state.height);
	    }

	    XFlush(XtDisplay(imageWindow));
	    XtVaSetValues(obj->getCanvas(), 
		XmNpicturePixmap,   obj->state.pixmap,
		XmNoverlayExposure, True,
		XmNnewImage, True,
		NULL);
	    XFree((char*)pixmap);
	}
	else
	{
	    XClearWindow(XtDisplay(imageWindow), XtWindow(imageWindow));
	    XDeleteProperty
		(XtDisplay(imageWindow),
		 event->xclient.window,
		 obj->atoms.dx_pixmap_id);
	}

	XDeleteProperty
	    (XtDisplay(imageWindow),
	     event->xclient.window,
	     obj->atoms.dx_flag);

	// put the system back into rotate mode if that's the mode it was
	// in before.
	boolean sw;
	obj->getSoftware(sw);
	if (obj->switchingSoftware && sw)
	{
	    XmPictureAlign((XmPictureWidget)obj->getCanvas());
	    if (obj->viewControlDialog)
		obj->viewControlDialog->resetLookDirection();

	    DirectInteractionMode m = obj->getInteractionMode();
	    obj->setInteractionMode(NONE);
	    obj->setInteractionMode(m);
	    obj->undoCmd->deactivate();
	    obj->redoCmd->deactivate();
	    obj->switchingSoftware = FALSE;
	}
    
	//
	// Set the View Control Dialog Sensitivity
	//
	if (obj->viewControlDialog && obj->directInteractionAllowed())
	{
	    if(in)
		obj->viewControlDialog->setSensitivity(in->useVector());
	}

	if(!obj->isManaged()) obj->DXWindow::manage();
    }
}

//
// Return TRUE/FALSE if the window is/isn't associated with a node.
//
boolean ImageWindow::isAssociatedWithNode()
{
    return this->node != NULL;
}
//
// This routine is used to associate/disassociate a node with an ImageWindow.
// If n is NULL, it disassociates the ImageWindow from the current Node.
// Otherwise, if it's already associated with a node, it returns false.
// Otherwise, it associates itself, setting up commands, ....
//
boolean ImageWindow::associateNode(Node *n)
{
    boolean ret;

    if (n == NULL)
    {
	if (this->node == NULL)	// Stop recursion with DisplayNode::associateImage()
	    return TRUE;

	DisplayNode *n = (DisplayNode*)this->node;
	this->node = NULL;
	n->associateImage(NULL);
	this->allowDirectInteraction(FALSE);
	this->setInteractionMode(NONE);

	//
	// Deactivate commands 
	//
	this->renderingOptionsCmd->deactivate();
	this->autoAxesCmd->deactivate();
	this->throttleCmd->deactivate();
	this->viewControlCmd->deactivate();
	this->backgroundColorCmd->deactivate();
	this->undoCmd->deactivate();
	this->redoCmd->deactivate();
	this->resetCmd->deactivate();
	if (this->changeImageNameCmd)
	    this->changeImageNameCmd->deactivate();
	this->backgroundColorCmd->deactivate();
	this->displayRotationGlobeCmd->deactivate();
	// Image depth menu is done below with configureImageDepthMenu()
	if (this->saveImageCmd)  this->saveImageCmd->deactivate();
	if (this->printImageCmd) this->printImageCmd->deactivate();
	if (this->setPanelAccessCmd)
	    this->setPanelAccessCmd->deactivate();

	//
	// Clean up the dialogs
	//
	if (this->viewControlDialog)
	{
	    this->viewControlDialog->unmanage();
	    delete this->viewControlDialog;
	    this->viewControlDialog = NULL;
	}
	if (this->renderingOptionsDialog)
	{
	    this->renderingOptionsDialog->unmanage();
	    delete this->renderingOptionsDialog;
	    this->renderingOptionsDialog = NULL;
	}
	if (this->backgroundColorDialog)
	{
	    this->backgroundColorDialog->unmanage();
	    delete this->backgroundColorDialog;
	    this->backgroundColorDialog = NULL;
	}
	if (this->throttleDialog)
	{
	    this->throttleDialog->unmanage();
	    delete this->throttleDialog;
	    this->throttleDialog = NULL;
	}
	if (this->autoAxesDialog)
	{
	    this->autoAxesDialog->unmanage();
	    delete this->autoAxesDialog;
	    this->autoAxesDialog = NULL;
	}
	if (this->changeImageNameDialog)
	{
	    this->changeImageNameDialog->unmanage();
	    delete this->changeImageNameDialog;
	    this->changeImageNameDialog = NULL;
	}
	if (this->saveImageDialog)
	{
	    this->saveImageDialog->unmanage();
	    delete this->saveImageDialog;
	    this->saveImageDialog = NULL;
	}
	if (this->printImageDialog)
	{
	    this->printImageDialog->unmanage();
	    delete this->printImageDialog;
	    this->printImageDialog = NULL;
	}


        //
        // Remove the commands from the auto list.
        //
/*
	if (this->printImageCmd) {
	    theDXApplication->executingCmd->removeAutoCmd(this->printImageCmd);
	    theDXApplication->notExecutingCmd->removeAutoCmd(
							this->printImageCmd);
	}
	if (this->saveImageCmd) {
	    theDXApplication->executingCmd->removeAutoCmd(this->saveImageCmd);
            theDXApplication->notExecutingCmd->removeAutoCmd(
							this->saveImageCmd);
        }
*/

	this->softwareCmd->deactivate();
	this->hardwareCmd->deactivate();
	this->upNoneCmd->deactivate();
	this->upWireframeCmd->deactivate();
	this->upDotsCmd->deactivate();
	this->upBoxCmd->deactivate();
	this->downNoneCmd->deactivate();
	this->downWireframeCmd->deactivate();
	this->downDotsCmd->deactivate();
	this->downBoxCmd->deactivate();

	this->modeCursorsCmd->deactivate();
	this->modePickCmd->deactivate();

	if(this->state.hardwareRenderExists)
	{
	    this->sendClientMessage(this->atoms.gl_destroy_window);
	    this->wait4GLAcknowledge();
	}

	ret = TRUE;
    }
    else
    {
	if (this->node == NULL)
	{
	    ASSERT(n->isA(ClassDisplayNode));
	    this->node = n;
	    int depth = ((DisplayNode*)n)->getDepth();
	    this->adjustDepth(depth);
	    this->changeDepth(depth);


 	    if (this->setPanelAccessCmd && this->network->getPanelCount())
		this->setPanelAccessCmd->activate();

	    if (n->isA(ClassImageNode))
	    {
		if (this->changeImageNameCmd &&
				((ImageNode*)n)->isImageNameInputSet())
		    this->changeImageNameCmd->activate();

		this->viewControlCmd->activate();
		this->autoAxesCmd->activate();
		if (this->saveImageCmd)
			 this->saveImageCmd->activate();	
		if (this->printImageCmd)
		    this->printImageCmd->activate();	
		this->throttleCmd->activate();
		this->backgroundColorCmd->activate();
		this->resetCmd->activate();
		this->displayRotationGlobeCmd->activate();
		this->renderingOptionsCmd->activate();
		this->softwareCmd->activate();
		this->hardwareCmd->activate();
		this->upNoneCmd->activate();
		this->upDotsCmd->activate();
		this->upBoxCmd->activate();
		this->downNoneCmd->activate();
		this->downDotsCmd->activate();
		this->downBoxCmd->activate();

		if (this->viewControlDialog)
		{
		    this->viewControlDialog->setSensitivity(
			((ImageNode*)n)->useVector());
		}

                //
                // Set the activation of commands that depend on execution.
                //
		if (this->saveImageDialog)
		    this->saveImageDialog->associateNode((ImageNode*)n);
		if (this->printImageDialog)
		    this->printImageDialog->associateNode((ImageNode*)n);

		if (!((DisplayNode*)n)->useSoftwareRendering())
		{
		    this->upWireframeCmd->activate();
		    this->downWireframeCmd->activate();
		}
		else
		{
		    this->upWireframeCmd->deactivate();
		    this->downWireframeCmd->deactivate();
		}


		List *l = this->network->makeClassifiedNodeList(ClassProbeNode,
					FALSE);
		if (l) {
		    delete l;
		    this->modeCursorsCmd->activate();
		} else
		    this->modeCursorsCmd->deactivate();

		l = this->network->makeClassifiedNodeList(ClassPickNode);
		if (l) {
		    delete l;
		    this->modePickCmd->activate();
		} else
		    this->modePickCmd->deactivate();
	    }

	    ret = TRUE;
	}
	else
	{
	    ret = FALSE;
	}
    }

    this->configureImageDepthMenu();
    this->resetWindowTitle();
    this->configureModeMenu();

    if (this->execute_after_resize_to) XtRemoveTimeOut (this->execute_after_resize_to);
    this->execute_after_resize_to = 0;

    return ret;
}

void ImageWindow::resetWindowTitle()
{
    DisplayNode *dmn = (DisplayNode*)this->node;
    const char *title = NULL; 
    char *t = NULL;

    if (dmn) 
	title = dmn->getTitle();

    if (!title) {
	const char *file = this->network->getFileName();
	const char *node_name ;
	if (dmn)
	    node_name = dmn->getNameString(); 
	else
	    node_name = "Image"; 
	if (!file) {
	    title = node_name;
	} else {
	    t = new char[STRLEN(file) + STRLEN(node_name) + 3];
	    sprintf(t,"%s: %s", node_name,file);
	    title = t;
	}
    }
    ASSERT(title);
    this->setWindowTitle(title);
    if (t) delete t;
}

void ImageWindow::allowDirectInteraction(boolean allow)
{
    if (this->directInteraction == allow)
	return;

    ASSERT(!allow || (this->node && this->node->isA(ClassImageNode)));

    this->directInteraction = allow;
    if (allow)
    {
	this->setInteractionMode(NONE);

	ImageNode *in = (ImageNode*)this->node;

	boolean sw;
	this->getSoftware(sw);
	if (sw)
	{
	    if (XmPictureUndoable((XmPictureWidget)this->getCanvas()))
		this->getUndoCmd()->activate();
	    else
		this->getUndoCmd()->deactivate();
	    if (XmPictureRedoable((XmPictureWidget)this->getCanvas()))
		this->getRedoCmd()->activate();
	    else
		this->getRedoCmd()->deactivate();
	}

	this->resetCmd->activate();
	this->displayRotationGlobeCmd->activate();
	if (this->viewControlDialog)
	{
	    this->viewControlDialog->setSensitivity(in->useVector());
	    this->viewControlDialog->sensitizePickOptionMenu(TRUE);
	    this->viewControlDialog->sensitizeProbeOptionMenu(TRUE);
	}


	if (this->network->containsClassOfNode(ClassProbeNode) > 0)
	    this->modeCursorsCmd->activate();
	else
	    this->modeCursorsCmd->deactivate();
	if (this->network->containsClassOfNode(ClassPickNode))
	    this->modePickCmd->activate();
	else
	    this->modePickCmd->deactivate();

	this->modeNoneCmd->activate();
	this->modeCameraCmd->activate();
	this->modeNavigateCmd->activate();
	this->modePanZoomCmd->activate();
	this->modeRoamCmd->activate();
	this->modeRotateCmd->activate();
	this->modeZoomCmd->activate();

    }
    else
    {
	this->undoCmd->deactivate();
	this->redoCmd->deactivate();
	this->resetCmd->deactivate();
	this->displayRotationGlobeCmd->deactivate();

	this->modeCursorsCmd->deactivate();
	this->modePickCmd->deactivate();

	this->modeNoneCmd->deactivate();
	this->modeCameraCmd->deactivate();
	this->modeNavigateCmd->deactivate();
	this->modePanZoomCmd->deactivate();
	this->modeRoamCmd->deactivate();
	this->modeRotateCmd->deactivate();
	this->modeZoomCmd->deactivate();
	if(this->viewControlDialog)
	{
	    this->viewControlDialog->setSensitivity(FALSE);
	    this->viewControlDialog->sensitizePickOptionMenu(FALSE);
	    this->viewControlDialog->sensitizeProbeOptionMenu(FALSE);
	}
    }
        this->configureModeMenu();
}

boolean ImageWindow::postRenderingOptionsDialog()
{
    if (this->renderingOptionsDialog == NULL)
	this->renderingOptionsDialog = new RenderingOptionsDialog(
	    this->getRootWidget(),
	    this);
    this->renderingOptionsDialog->post();
    // Force Image's Translations onto renderingOptionsDialog here.
    TransferAccelerator(this->renderingOptionsDialog->getRootWidget(), 
    		this->saveOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->renderingOptionsDialog->getRootWidget(), 
    		this->saveImageOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->renderingOptionsDialog->getRootWidget(), 
    		this->printImageOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->renderingOptionsDialog->getRootWidget(), 
    		this->closeOption->getRootWidget(), "ArmAndActivate");
    if(this->openAllControlPanelsOption)
    TransferAccelerator(this->renderingOptionsDialog->getRootWidget(), 
    		this->openAllControlPanelsOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->renderingOptionsDialog->getRootWidget(), 
    		this->openAllColormapEditorsOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->renderingOptionsDialog->getRootWidget(), 
    		this->messageWindowOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->renderingOptionsDialog->getRootWidget(), 
    		this->undoOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->renderingOptionsDialog->getRootWidget(), 
    		this->redoOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->renderingOptionsDialog->getRootWidget(), 
    		this->resetOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->renderingOptionsDialog->getRootWidget(), 
    		this->executeOnceOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->renderingOptionsDialog->getRootWidget(), 
    		this->executeOnChangeOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->renderingOptionsDialog->getRootWidget(), 
    		this->endExecutionOption->getRootWidget(), "ArmAndActivate");

    TransferAccelerator(this->renderingOptionsDialog->getRootWidget(), 
    		this->modeCameraOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->renderingOptionsDialog->getRootWidget(), 
    		this->modeCursorsOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->renderingOptionsDialog->getRootWidget(), 
    		this->modePickOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->renderingOptionsDialog->getRootWidget(), 
    		this->modeNavigateOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->renderingOptionsDialog->getRootWidget(), 
    		this->modePanZoomOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->renderingOptionsDialog->getRootWidget(), 
    		this->modeRoamOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->renderingOptionsDialog->getRootWidget(), 
    		this->modeRotateOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->renderingOptionsDialog->getRootWidget(), 
    		this->modeZoomOption->getRootWidget(), "ArmAndActivate");

    return TRUE;
}
boolean ImageWindow::postAutoAxesDialog()
{
    if (this->autoAxesDialog == NULL)
	this->autoAxesDialog = new AutoAxesDialog(this);
    
    this->autoAxesDialog->post();
    // Force Image's Translations onto autoAxesDialog here.
    TransferAccelerator(this->autoAxesDialog->getRootWidget(), 
    		this->saveOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->autoAxesDialog->getRootWidget(), 
    		this->saveImageOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->autoAxesDialog->getRootWidget(), 
    		this->printImageOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->autoAxesDialog->getRootWidget(), 
    		this->closeOption->getRootWidget(), "ArmAndActivate");
    if(this->openAllControlPanelsOption)
    TransferAccelerator(this->autoAxesDialog->getRootWidget(), 
    		this->openAllControlPanelsOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->autoAxesDialog->getRootWidget(), 
    		this->openAllColormapEditorsOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->autoAxesDialog->getRootWidget(), 
    		this->messageWindowOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->autoAxesDialog->getRootWidget(), 
    		this->undoOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->autoAxesDialog->getRootWidget(), 
    		this->redoOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->autoAxesDialog->getRootWidget(), 
    		this->resetOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->autoAxesDialog->getRootWidget(), 
    		this->executeOnceOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->autoAxesDialog->getRootWidget(), 
    		this->executeOnChangeOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->autoAxesDialog->getRootWidget(), 
    		this->endExecutionOption->getRootWidget(), "ArmAndActivate");

    TransferAccelerator(this->autoAxesDialog->getRootWidget(), 
    		this->modeCameraOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->autoAxesDialog->getRootWidget(), 
    		this->modeCursorsOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->autoAxesDialog->getRootWidget(), 
    		this->modePickOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->autoAxesDialog->getRootWidget(), 
    		this->modeNavigateOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->autoAxesDialog->getRootWidget(), 
    		this->modePanZoomOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->autoAxesDialog->getRootWidget(), 
    		this->modeRoamOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->autoAxesDialog->getRootWidget(), 
    		this->modeRotateOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->autoAxesDialog->getRootWidget(), 
    		this->modeZoomOption->getRootWidget(), "ArmAndActivate");

    return TRUE;
}

boolean ImageWindow::postVPE()
{
    if(NOT this->network)
	    return FALSE;

    EditorWindow* editor = this->network->getEditor();

    if (NOT editor)
	editor = theDXApplication->newNetworkEditor(this->network);

    if (editor) {
	editor->manage();
	XMapRaised(XtDisplay(editor->getRootWidget()),
		   XtWindow(editor->getRootWidget()));
    }

    return TRUE;
}

boolean ImageWindow::postThrottleDialog()
{
    if (this->throttleDialog == NULL)
	this->throttleDialog = new ThrottleDialog(
	    "throttleDialog",
	    this);
    
    this->throttleDialog->post();
    // Force Image's Translations onto throttleDialog here.
    TransferAccelerator(this->throttleDialog->getRootWidget(), 
    		this->saveOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->throttleDialog->getRootWidget(), 
    		this->saveImageOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->throttleDialog->getRootWidget(), 
    		this->printImageOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->throttleDialog->getRootWidget(), 
    		this->closeOption->getRootWidget(), "ArmAndActivate");
    if(this->openAllControlPanelsOption)
    TransferAccelerator(this->throttleDialog->getRootWidget(), 
    		this->openAllControlPanelsOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->throttleDialog->getRootWidget(), 
    		this->openAllColormapEditorsOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->throttleDialog->getRootWidget(), 
    		this->messageWindowOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->throttleDialog->getRootWidget(), 
    		this->undoOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->throttleDialog->getRootWidget(), 
    		this->redoOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->throttleDialog->getRootWidget(), 
    		this->resetOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->throttleDialog->getRootWidget(), 
    		this->executeOnceOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->throttleDialog->getRootWidget(), 
    		this->executeOnChangeOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->throttleDialog->getRootWidget(), 
    		this->endExecutionOption->getRootWidget(), "ArmAndActivate");

    TransferAccelerator(this->throttleDialog->getRootWidget(), 
    		this->modeCameraOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->throttleDialog->getRootWidget(), 
    		this->modeCursorsOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->throttleDialog->getRootWidget(), 
    		this->modePickOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->throttleDialog->getRootWidget(), 
    		this->modeNavigateOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->throttleDialog->getRootWidget(), 
    		this->modePanZoomOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->throttleDialog->getRootWidget(), 
    		this->modeRoamOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->throttleDialog->getRootWidget(), 
    		this->modeRotateOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->throttleDialog->getRootWidget(), 
    		this->modeZoomOption->getRootWidget(), "ArmAndActivate");


    return TRUE;
}
boolean ImageWindow::postViewControlDialog()
{
    boolean newDialog = FALSE;
    if (this->viewControlDialog == NULL)
    {
	this->viewControlDialog = new ViewControlDialog(this->getRootWidget(),
							this);
	newDialog = TRUE;
    }

    if (this->viewControlDialog->isManaged())
	return TRUE;

    this->viewControlDialog->post();
    // Force Image's Translations onto ViewControlDialog here.
    TransferAccelerator(this->viewControlDialog->getRootWidget(), 
    		this->saveOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->viewControlDialog->getRootWidget(), 
    		this->saveImageOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->viewControlDialog->getRootWidget(), 
    		this->printImageOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->viewControlDialog->getRootWidget(), 
    		this->closeOption->getRootWidget(), "ArmAndActivate");
    if(this->openAllControlPanelsOption)
    TransferAccelerator(this->viewControlDialog->getRootWidget(), 
    		this->openAllControlPanelsOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->viewControlDialog->getRootWidget(), 
    		this->openAllColormapEditorsOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->viewControlDialog->getRootWidget(), 
    		this->messageWindowOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->viewControlDialog->getRootWidget(), 
    		this->undoOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->viewControlDialog->getRootWidget(), 
    		this->redoOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->viewControlDialog->getRootWidget(), 
    		this->resetOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->viewControlDialog->getRootWidget(), 
    		this->executeOnceOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->viewControlDialog->getRootWidget(), 
    		this->executeOnChangeOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->viewControlDialog->getRootWidget(), 
    		this->endExecutionOption->getRootWidget(), "ArmAndActivate");

    if (this->directInteractionAllowed())
    {
	ImageNode *in = (ImageNode*)this->node;
	this->viewControlDialog->setSensitivity(
	    this->state.pixmap != 0 && in->useVector());
	if (newDialog)
	{
	    DirectInteractionMode m = this->getInteractionMode();
	    this->setInteractionMode(NONE);
	    this->setInteractionMode(m);
	}
    }
    else
	this->viewControlDialog->setSensitivity(FALSE);

    return TRUE;
}
boolean ImageWindow::postChangeImageNameDialog()
{
    if (NOT this->changeImageNameDialog)
	this->changeImageNameDialog = new SetImageNameDialog(this); 
    this->changeImageNameDialog->post();
    
    return TRUE;
}

boolean ImageWindow::enablePerspective(boolean enable)
{
    if (!this->node->isA(ClassImageNode))
	return FALSE;

    ImageNode *in = (ImageNode*)this->node;

    boolean oldProj;
    in->getProjection(oldProj);
    if (oldProj == enable)
	return TRUE;
    

    /*
     * Save this camera so it can be undone later.
     */
    if (!this->pushedSinceExec)
    {
	if (this->state.hardwareRender) 
	    this->sendClientMessage(this->atoms.push_camera);
	else
	    XmPicturePushUndoCamera((XmPictureWidget)this->getCanvas());
	this->pushedSinceExec = TRUE;
    }

    /*
     * If we are going to an orthographic projection, calculate an autocamera
     * width from the perspective camera.  If we are going to a perspective
     * projection, calculate a view angle from the orthographic camera.
     */
    if (this->state.hardwareRender)
    {
	double dx = this->state.hardwareCamera.to[0] -
		    this->state.hardwareCamera.from[0];
	double dy = this->state.hardwareCamera.to[1] -
		    this->state.hardwareCamera.from[1];
	double dz = this->state.hardwareCamera.to[2] -
		    this->state.hardwareCamera.from[2];
	double length = sqrt( (dx * dx) + (dy * dy) + (dz * dz) );


	if (!enable)
	{
	    double viewAngle = this->state.hardwareCamera.viewAngle;
	    
	    double width = 2*length*tan((viewAngle/2.0)*3.14159/180.0);
	    in->setWidth(width, FALSE);
	}
	else
	{
	    double width = this->state.hardwareCamera.width;
	    double viewAngle = 2*atan((width/2)/length)*180.0 / 3.14159;
	    in->setViewAngle(viewAngle, FALSE);
	}
    }
    else
    {
	double to[3];
	in->getTo(to);
	double from[3];
	in->getFrom(from);
	double dx = to[0] - from[0];
	double dy = to[1] - from[1];
	double dz = to[2] - from[2];
	double length = sqrt( (dx * dx) + (dy * dy) + (dz * dz) );
	if (!enable)
	{
	    double viewAngle;
	    in->getViewAngle(viewAngle);
	    double width = 2*length*tan((viewAngle/2.0)*3.14159/180.0);
	    in->setWidth(width, FALSE);
	}
	else
	{
	    double width;
	    in->getWidth(width);
	    double viewAngle = 2*atan((width/2)/length)*180.0 / 3.14159;
	    in->setViewAngle(viewAngle, FALSE);
	}
    }
    in->setProjection(enable, TRUE);

    if (this->viewControlDialog)
    {
	this->viewControlDialog->resetProjection();
    }

    return TRUE;
}
boolean ImageWindow::getPerspective()
{
    if (!this->node->isA(ClassImageNode))
	return FALSE;

    ImageNode *in = (ImageNode*)this->node;
    
    boolean persp;
    in->getProjection(persp);

    return persp;
}
void ImageWindow::getViewAngle(double &viewAngle)
{
    if (!this->node->isA(ClassImageNode))
	return;

    ImageNode *in = (ImageNode*)this->node;
    
    in->getViewAngle(viewAngle);
}
void ImageWindow::setViewAngle(double viewAngle)
{
    if (!this->node->isA(ClassImageNode))
	return;

    if (!this->pushedSinceExec)
    {
	if (this->state.hardwareRender) 
	    this->sendClientMessage(this->atoms.push_camera);
	else
	    XmPicturePushUndoCamera((XmPictureWidget)this->getCanvas());
	this->pushedSinceExec = TRUE;
    }
    
    //
    // If the image window isn't in navigate mode, then move the from up
    // or back to keep the object the same size.
    //     find the original width, determine the new distance to make the
    //     view angle make stuff at the old distance look the same.
    //
    ImageNode *in = (ImageNode*)this->node;
    if (this->currentInteractionMode != NAVIGATE)
    {
	double oldViewAngle;
	in->getViewAngle(oldViewAngle);

	oldViewAngle /= 360/(2*3.1415926);
	double newViewAngle = viewAngle/(360/(2*3.1415926));

	double to[3];
	double from[3];
	in->getTo(to);
	in->getFrom(from);
	double x = from[0] - to[0];
	double y = from[1] - to[1];
	double z = from[2] - to[2];
	double dist = sqrt(x * x + y * y + z * z);

	double oldWidth = dist * tan(oldViewAngle/2);
	double newDist = oldWidth/tan(newViewAngle/2);

	x *= newDist/dist;
	y *= newDist/dist;
	z *= newDist/dist;

	from[0] = to[0] + x;
	from[1] = to[1] + y;
	from[2] = to[2] + z;

	in->setFrom(from, FALSE);
    }

    in->setViewAngle(viewAngle, TRUE);
}
void ImageWindow::setResolution(int x, int y)
{
    if (!this->node->isA(ClassImageNode))
	return;

    ImageNode *in = (ImageNode*)this->node;

    /*
     * Otherwise, get the new width and height.
     */
    in->setResolution(x, y, FALSE);
    if (x == 0)
	return;
    double aspect = (y + 0.5) / x;
    in->setAspect(aspect, FALSE);

    in->notifyWhereChange(TRUE);
}

void ImageWindow::getResolution(int &x, int &y)
{
    if (!this->node->isA(ClassImageNode))
	return;

    ImageNode *in = (ImageNode*)this->node;
    
    in->getResolution(x, y);
}
void ImageWindow::setThrottle(double w)
{
    if (!this->node->isA(ClassImageNode))
	return;
    ((ImageNode*)this->node)->setThrottle(w);
}

void ImageWindow::getThrottle(double &w)
{
    if (!this->node->isA(ClassImageNode))
	return;
    ((ImageNode*)this->node)->getThrottle(w);
}

boolean ImageWindow::isAutoAxesCornersSet ()
{
    if (!this->node->isA(ClassImageNode))
	return FALSE;
    return ((ImageNode*)this->node)->isAutoAxesCornersSet();
}

boolean ImageWindow::isAutoAxesLabelScaleSet ()
{
    if (!this->node->isA(ClassImageNode))
	return FALSE;
    return ((ImageNode*)this->node)->isAutoAxesLabelScaleSet();
}

boolean ImageWindow::isAutoAxesAnnotationSet ()
{
    if (!this->node->isA(ClassImageNode))
	return FALSE;
    return ((ImageNode*)this->node)->isAutoAxesAnnotationSet();
}

boolean ImageWindow::isAutoAxesLabelsSet ()
{
    if (!this->node->isA(ClassImageNode))
	return FALSE;
    return ((ImageNode*)this->node)->isAutoAxesLabelsSet();
}

boolean ImageWindow::isAutoAxesColorsSet ()
{
    if (!this->node->isA(ClassImageNode))
	return FALSE;
    return ((ImageNode*)this->node)->isAutoAxesColorsSet();
}

boolean ImageWindow::isAutoAxesFontSet ()
{
    if (!this->node->isA(ClassImageNode))
	return FALSE;
    return ((ImageNode*)this->node)->isAutoAxesFontSet();
}

boolean ImageWindow::isAutoAxesCursorSet ()
{
    if (!this->node->isA(ClassImageNode))
	return FALSE;
    return ((ImageNode*)this->node)->isAutoAxesCursorSet();
}

boolean ImageWindow::isSetAutoAxesFrame ()
{
    if (!this->node->isA(ClassImageNode))
	return FALSE;
    return ((ImageNode*)this->node)->isSetAutoAxesFrame();
}

boolean ImageWindow::isSetAutoAxesGrid ()
{
    if (!this->node->isA(ClassImageNode))
	return FALSE;
    return ((ImageNode*)this->node)->isSetAutoAxesGrid();
}

boolean ImageWindow::isSetAutoAxesAdjust ()
{
    if (!this->node->isA(ClassImageNode))
	return FALSE;
    return ((ImageNode*)this->node)->isSetAutoAxesAdjust();
}

boolean ImageWindow::isAutoAxesTicksSet ()
{
    if (!this->node->isA(ClassImageNode))
	return FALSE;
    return ((ImageNode*)this->node)->isAutoAxesTicksSet();
}

boolean ImageWindow::isAutoAxesXTickLocsSet ()
{
    if (!this->node->isA(ClassImageNode))
	return FALSE;
    return ((ImageNode*)this->node)->isAutoAxesXTickLocsSet();
}

boolean ImageWindow::isAutoAxesYTickLocsSet ()
{
    if (!this->node->isA(ClassImageNode))
	return FALSE;
    return ((ImageNode*)this->node)->isAutoAxesYTickLocsSet();
}

boolean ImageWindow::isAutoAxesZTickLocsSet ()
{
    if (!this->node->isA(ClassImageNode))
	return FALSE;
    return ((ImageNode*)this->node)->isAutoAxesZTickLocsSet();
}

boolean ImageWindow::isAutoAxesXTickLabelsSet ()
{
    if (!this->node->isA(ClassImageNode))
	return FALSE;
    return ((ImageNode*)this->node)->isAutoAxesXTickLabelsSet();
}

boolean ImageWindow::isAutoAxesYTickLabelsSet ()
{
    if (!this->node->isA(ClassImageNode))
	return FALSE;
    return ((ImageNode*)this->node)->isAutoAxesYTickLabelsSet();
}

boolean ImageWindow::isAutoAxesZTickLabelsSet ()
{
    if (!this->node->isA(ClassImageNode))
	return FALSE;
    return ((ImageNode*)this->node)->isAutoAxesZTickLabelsSet();
}

void ImageWindow::unsetAutoAxesTicks (boolean send)
{
    if (!this->node->isA(ClassImageNode))
	return ;
    ((ImageNode*)this->node)->unsetAutoAxesTicks(send);
}

void ImageWindow::unsetAutoAxesXTickLocs (boolean send)
{
    if (!this->node->isA(ClassImageNode))
	return ;
    ((ImageNode*)this->node)->unsetAutoAxesXTickLocs(send);
}

void ImageWindow::unsetAutoAxesYTickLocs (boolean send)
{
    if (!this->node->isA(ClassImageNode))
	return ;
    ((ImageNode*)this->node)->unsetAutoAxesYTickLocs(send);
}

void ImageWindow::unsetAutoAxesZTickLocs (boolean send)
{
    if (!this->node->isA(ClassImageNode))
	return ;
    ((ImageNode*)this->node)->unsetAutoAxesZTickLocs(send);
}

void ImageWindow::unsetAutoAxesXTickLabels (boolean send)
{
    if (!this->node->isA(ClassImageNode))
	return ;
    ((ImageNode*)this->node)->unsetAutoAxesXTickLabels(send);
}

void ImageWindow::unsetAutoAxesYTickLabels (boolean send)
{
    if (!this->node->isA(ClassImageNode))
	return ;
    ((ImageNode*)this->node)->unsetAutoAxesYTickLabels(send);
}

void ImageWindow::unsetAutoAxesZTickLabels (boolean send)
{
    if (!this->node->isA(ClassImageNode))
	return ;
    ((ImageNode*)this->node)->unsetAutoAxesZTickLabels(send);
}

void ImageWindow::unsetAutoAxesLabels (boolean send)
{
    if (!this->node->isA(ClassImageNode))
	return ;
    ((ImageNode*)this->node)->unsetAutoAxesLabels(send);
}

void ImageWindow::unsetAutoAxesLabelScale (boolean send)
{
    if (!this->node->isA(ClassImageNode))
	return ;
    ((ImageNode*)this->node)->unsetAutoAxesLabelScale(send);
}

void ImageWindow::unsetAutoAxesFont (boolean send)
{
    if (!this->node->isA(ClassImageNode))
	return ;
    ((ImageNode*)this->node)->unsetAutoAxesFont(send);
}

void ImageWindow::unsetAutoAxesEnable (boolean send)
{
    if (!this->node->isA(ClassImageNode))
	return ;
    ((ImageNode*)this->node)->unsetAutoAxesEnable(send);
}

void ImageWindow::unsetAutoAxesFrame (boolean send)
{
    if (!this->node->isA(ClassImageNode))
	return ;
    ((ImageNode*)this->node)->unsetAutoAxesFrame(send);
}

void ImageWindow::unsetAutoAxesGrid (boolean send)
{
    if (!this->node->isA(ClassImageNode))
	return ;
    ((ImageNode*)this->node)->unsetAutoAxesGrid(send);
}

void ImageWindow::unsetAutoAxesAdjust (boolean send)
{
    if (!this->node->isA(ClassImageNode))
	return ;
    ((ImageNode*)this->node)->unsetAutoAxesAdjust(send);
}

void ImageWindow::getAutoAxesCursor (double *x, double *y, double *z)
{
    if (!this->node->isA(ClassImageNode))
	return ;
    ((ImageNode*)this->node)->getAutoAxesCursor(x,y,z);
}

void ImageWindow::getAutoAxesCorners (double dval[])
{
    if (!this->node->isA(ClassImageNode))
	return ;
    ((ImageNode*)this->node)->getAutoAxesCorners(dval);
}

void ImageWindow::unsetAutoAxesCursor (boolean send)
{
    if (!this->node->isA(ClassImageNode))
	return ;
    ((ImageNode*)this->node)->unsetAutoAxesCursor(send);
}

void ImageWindow::unsetAutoAxesAnnotation (boolean send)
{
    if (!this->node->isA(ClassImageNode))
	return ;
    ((ImageNode*)this->node)->unsetAutoAxesAnnotation(send);
}

void ImageWindow::unsetAutoAxesColors (boolean send)
{
    if (!this->node->isA(ClassImageNode))
	return ;
    ((ImageNode*)this->node)->unsetAutoAxesColors(send);
}

void ImageWindow::setAutoAxesCursor (double x, double  y, double  z, boolean send)
{
    if (!this->node->isA(ClassImageNode))
	return ;
    ((ImageNode*)this->node)->setAutoAxesCursor(x, y, z, send);
}

void ImageWindow::unsetAutoAxesCorners (boolean send)
{
    if (!this->node->isA(ClassImageNode))
	return ;
    ((ImageNode*)this->node)->unsetAutoAxesCorners(send);
}

void ImageWindow::setAutoAxesCorners (double dval[], boolean send)
{
    if (!this->node->isA(ClassImageNode))
	return ;
    ((ImageNode*)this->node)->setAutoAxesCorners(dval, send);
}

void ImageWindow::setAutoAxesTicks (int t1, int t2, int t3, boolean send)
{
    if (!this->node->isA(ClassImageNode))
	return ;
    ((ImageNode*)this->node)->setAutoAxesTicks (t1, t2, t3, send);
}

void ImageWindow::setAutoAxesTicks (int t, boolean send)
{
    if (!this->node->isA(ClassImageNode))
	return ;
    ((ImageNode*)this->node)->setAutoAxesTicks (t, send);
}

void ImageWindow::setAutoAxesXTickLocs (double *t, int size, boolean send)
{
    if (!this->node->isA(ClassImageNode))
	return ;
    ((ImageNode*)this->node)->setAutoAxesXTickLocs (t, size, send);
}

void ImageWindow::setAutoAxesYTickLocs (double *t, int size, boolean send)
{
    if (!this->node->isA(ClassImageNode))
	return ;
    ((ImageNode*)this->node)->setAutoAxesYTickLocs (t, size, send);
}

void ImageWindow::setAutoAxesZTickLocs (double *t, int size, boolean send)
{
    if (!this->node->isA(ClassImageNode))
	return ;
    ((ImageNode*)this->node)->setAutoAxesZTickLocs (t, size, send);
}

void ImageWindow::setAutoAxesXTickLabels (char *value, boolean send)
{
    if (!this->node->isA(ClassImageNode))
	return ;
    ((ImageNode*)this->node)->setAutoAxesXTickLabels (value, send);
}

void ImageWindow::setAutoAxesYTickLabels (char *value, boolean send)
{
    if (!this->node->isA(ClassImageNode))
	return ;
    ((ImageNode*)this->node)->setAutoAxesYTickLabels (value, send);
}

void ImageWindow::setAutoAxesZTickLabels (char *value, boolean send)
{
    if (!this->node->isA(ClassImageNode))
	return ;
    ((ImageNode*)this->node)->setAutoAxesZTickLabels (value, send);
}

void ImageWindow::setAutoAxesAnnotation(char *value, boolean send)
{
    if (!this->node->isA(ClassImageNode))
	return ;
    ((ImageNode*)this->node)->setAutoAxesAnnotation(value, send);
}

void ImageWindow::setAutoAxesLabels (char *value, boolean send)
{
    if (!this->node->isA(ClassImageNode))
	return ;
    ((ImageNode*)this->node)->setAutoAxesLabels(value, send);
}

void ImageWindow::setAutoAxesColors (char *value, boolean send)
{
    if (!this->node->isA(ClassImageNode))
	return ;
    ((ImageNode*)this->node)->setAutoAxesColors(value, send);
}

void ImageWindow::setAutoAxesFont (char *value, boolean send)
{
    if (!this->node->isA(ClassImageNode))
	return ;
    ((ImageNode*)this->node)->setAutoAxesFont(value, send);
}

int ImageWindow::getAutoAxesAnnotation (char *sval[])
{
    if (!this->node->isA(ClassImageNode))
	return 0;
    return ((ImageNode*)this->node)->getAutoAxesAnnotation(sval);
}

int ImageWindow::getAutoAxesLabels (char *sval[])
{
    if (!this->node->isA(ClassImageNode))
	return 0;
    return ((ImageNode*)this->node)->getAutoAxesLabels(sval);
}

int ImageWindow::getAutoAxesColors (char *sval[])
{
    if (!this->node->isA(ClassImageNode))
	return 0;
    return ((ImageNode*)this->node)->getAutoAxesColors(sval);
}

int ImageWindow::getAutoAxesFont (char *sval)
{
    if (!this->node->isA(ClassImageNode))
	return 0;
    return ((ImageNode*)this->node)->getAutoAxesFont(sval);
}

void ImageWindow::getAutoAxesTicks (int *t1, int *t2, int *t3)
{
    if (!this->node->isA(ClassImageNode))
	return ;
    ((ImageNode*)this->node)->getAutoAxesTicks(t1, t2, t3);
}

void ImageWindow::getAutoAxesXTickLocs (double **t, int *size)
{
    if (!this->node->isA(ClassImageNode))
	return ;
    ((ImageNode*)this->node)->getAutoAxesXTickLocs(t, size);
}

void ImageWindow::getAutoAxesYTickLocs (double **t, int *size)
{
    if (!this->node->isA(ClassImageNode))
	return ;
    ((ImageNode*)this->node)->getAutoAxesYTickLocs(t, size);
}

void ImageWindow::getAutoAxesZTickLocs (double **t, int *size)
{
    if (!this->node->isA(ClassImageNode))
	return ;
    ((ImageNode*)this->node)->getAutoAxesZTickLocs(t, size);
}

int ImageWindow::getAutoAxesXTickLabels (char *sval[])
{
    if (!this->node->isA(ClassImageNode))
	return 0;
    return ((ImageNode*)this->node)->getAutoAxesXTickLabels(sval);
}

int ImageWindow::getAutoAxesYTickLabels (char *sval[])
{
    if (!this->node->isA(ClassImageNode))
	return 0;
    return ((ImageNode*)this->node)->getAutoAxesYTickLabels(sval);
}

int ImageWindow::getAutoAxesZTickLabels (char *sval[])
{
    if (!this->node->isA(ClassImageNode))
	return 0;
    return ((ImageNode*)this->node)->getAutoAxesZTickLabels(sval);
}

int ImageWindow::getAutoAxesEnable ()
{
    if (!this->node->isA(ClassImageNode))
	return 0;
    return ((ImageNode*)this->node)->getAutoAxesEnable();
}

double ImageWindow::getAutoAxesLabelScale ()
{
    if (!this->node->isA(ClassImageNode))
	return 0.0;
    return ((ImageNode*)this->node)->getAutoAxesLabelScale();
}

void ImageWindow::getAutoAxesTicks (int *t)
{
    if (!this->node->isA(ClassImageNode))
	return ;
    ((ImageNode*)this->node)->getAutoAxesTicks(t);
}

int ImageWindow::getAutoAxesTicksCount ()
{
    if (!this->node->isA(ClassImageNode))
	return 0;
    return ((ImageNode*)this->node)->getAutoAxesTicksCount ();
}

void ImageWindow::setAutoAxesFrame (boolean state, boolean send)
{
    if (!this->node->isA(ClassImageNode))
	return ;
    ((ImageNode*)this->node)->setAutoAxesFrame(state, send);
}

void ImageWindow::setAutoAxesGrid (boolean state, boolean send)
{
    if (!this->node->isA(ClassImageNode))
	return ;
    ((ImageNode*)this->node)->setAutoAxesGrid(state, send);
}

void ImageWindow::setAutoAxesAdjust (boolean state, boolean send)
{
    if (!this->node->isA(ClassImageNode))
	return ;
    ((ImageNode*)this->node)->setAutoAxesAdjust(state, send);
}

void ImageWindow::setAutoAxesEnable (int d, boolean send)
{
    if (!this->node->isA(ClassImageNode))
	return ;
    ((ImageNode*)this->node)->setAutoAxesEnable(d, send);
}

void ImageWindow::setAutoAxesLabelScale (double d, boolean send)
{
    if (!this->node->isA(ClassImageNode))
	return ;
    ((ImageNode*)this->node)->setAutoAxesLabelScale(d, send);
}


void ImageWindow::setWidth(double w)
{
    if (!this->node->isA(ClassImageNode))
	return;

    if (!this->pushedSinceExec)
    {
	if (this->state.hardwareRender) 
	    this->sendClientMessage(this->atoms.push_camera);
	else
	    XmPicturePushUndoCamera((XmPictureWidget)this->getCanvas());
	this->pushedSinceExec = TRUE;
    }
    
    ImageNode *in = (ImageNode*)this->node;
    in->setWidth(w);
}
void ImageWindow::getWidth(double &w)
{
    if (!this->node->isA(ClassImageNode))
	return;

    ImageNode *in = (ImageNode*)this->node;
    
    in->getWidth(w);
}
void ImageWindow::setTo(double *v)
{
    if (!this->node->isA(ClassImageNode))
	return;

    if (!this->pushedSinceExec)
    {
	if (this->state.hardwareRender) 
	    this->sendClientMessage(this->atoms.push_camera);
	else
	    XmPicturePushUndoCamera((XmPictureWidget)this->getCanvas());
	this->pushedSinceExec = TRUE;
    }
 
    ImageNode *in = (ImageNode*)this->node;
    in->setTo(v);
}
void ImageWindow::setFrom(double *v)
{
    if (!this->node->isA(ClassImageNode))
	return;

    if (!this->pushedSinceExec)
    {
	if (this->state.hardwareRender) 
	    this->sendClientMessage(this->atoms.push_camera);
	else
	    XmPicturePushUndoCamera((XmPictureWidget)this->getCanvas());
	this->pushedSinceExec = TRUE;
    }
 
    ImageNode *in = (ImageNode*)this->node;
    in->setFrom(v);
}
void ImageWindow::setUp(double *v)
{
    if (!this->node->isA(ClassImageNode))
	return;

    if (!this->pushedSinceExec)
    {
	if (this->state.hardwareRender) 
	    this->sendClientMessage(this->atoms.push_camera);
	else
	    XmPicturePushUndoCamera((XmPictureWidget)this->getCanvas());
	this->pushedSinceExec = TRUE;
    }
 
    ImageNode *in = (ImageNode*)this->node;
    in->setUp(v);
}
void ImageWindow::getTo(double *v)
{
    if (!this->node->isA(ClassImageNode))
	return;

    ImageNode *in = (ImageNode*)this->node;
    
    in->getTo(v);
}
void ImageWindow::getFrom(double *v)
{
    if (!this->node->isA(ClassImageNode))
	return;

    ImageNode *in = (ImageNode*)this->node;
    
    in->getFrom(v);
}
void ImageWindow::getUp(double *v)
{
    if (!this->node->isA(ClassImageNode))
	return;

    ImageNode *in = (ImageNode*)this->node;
    
    in->getUp(v);
}

boolean ImageWindow::setInteractionMode(DirectInteractionMode mode)
{
    return this->setInteractionMode(mode,FALSE);
}
boolean ImageWindow::applyPendingInteractionMode()
{
    DirectInteractionMode mode = this->pendingInteractionMode;
    if (mode != NONE)
	return this->setInteractionMode(mode);
    else 
	return TRUE;
}
#if 00
boolean ImageWindow::activateInteractionMode()
{
    DirectInteractionMode mode = this->pendingInteractionMode;
    if (mode == NONE)
	mode = this->currentInteractionMode;
    return this->setInteractionMode(mode,TRUE);
}
#endif

boolean ImageWindow::setInteractionMode(DirectInteractionMode mode, 
			boolean ignoreMatchingModes)
{
    ImageNode *in = NUL(ImageNode*);
    if ((this->node) && (this->node->isA(ClassImageNode))) 
	in = (ImageNode*)this->node;
    boolean success = TRUE;

    if (!ignoreMatchingModes && (this->currentInteractionMode == mode))
	return TRUE;

    if( ((mode == CURSORS) || (mode == ROAM)) && this->state.degenerateBox )
    {
	WarningMessage("Degenerate bounding box.  "
			"Can't enter Cursor or Roam modes");
	success = FALSE;
	mode = NONE;
    }

    this->pendingInteractionMode = NONE;
    if (mode != this->currentInteractionMode) {
	DirectInteractionMode oldMode = this->currentInteractionMode;
	this->currentInteractionMode = mode;
	//
	// Reset the mode debore unmanaging sub-form, currentInteractionMode
	// must be set at this point.
	//
	if (this->viewControlDialog)
	    this->viewControlDialog->resetMode();
	//
	// Do what we need to leave the old mode.
	//
	PickNode *curr_pick;
	switch (oldMode)
	{
	case NONE:
	    break;
	case CAMERA:
	    if (this->viewControlDialog)
		this->viewControlDialog->unmanageCameraForm();
	    break;
	case CURSORS:
	    if (this->viewControlDialog)
		this->viewControlDialog->unmanageCursorForm();
	    break;
	case PICK:
	    if (this->viewControlDialog)
		this->viewControlDialog->unmanagePickForm();
	    //
	    // We may be unsetting the mode as a result of a window reset,
	    // which is the result of reading in a new net.  When reading
	    // new nets, the net is cleared so that there are no Pick
	    // nodes, which results in an ASSERT failure.
	    //
	    curr_pick = this->getCurrentPickNode();
	    if (curr_pick)
		curr_pick->pickFrom(NULL);
	    break;
	case NAVIGATE:
	    if (this->viewControlDialog)
		this->viewControlDialog->unmanageNavigationForm();
	    break;
	case ROAM:
	    if (this->viewControlDialog)
		this->viewControlDialog->unmanageRoamForm();
	    break;
	case PANZOOM:
	case ROTATE:
	case ZOOM:
	    break;
	}
    }

    switch (mode)
    {
    case NONE:
	if (this->state.hardwareRender)
	    this->sendClientMessage(this->atoms.stop);

	// Set the Picture widget into NULL mode, even if we are doing
	// GL render.  This is because the Picture widget fields
	// key press events for translate speed and rotate speed.
	XtVaSetValues(this->getCanvas(),
	    XmNmode, XmNULL_MODE,
	    NULL);
	if (in) in->setInteractionModeParameter(NONE);
	break;
    case CAMERA:
	if (this->viewControlDialog)
	{
	    this->viewControlDialog->manageCameraForm();
	    this->viewControlDialog->resetLookDirection();
	}
	XmPictureAlign((XmPictureWidget)this->getCanvas());

	if (this->state.hardwareRender)
	    this->sendClientMessage(this->atoms.stop);
	else
	    XtVaSetValues(this->getCanvas(),
		XmNmode, XmNULL_MODE,
		NULL);
	if (in) in->setInteractionModeParameter(CAMERA);
	break;

    case CURSORS:
	if (this->viewControlDialog)
	{
	    this->viewControlDialog->manageCursorForm();
	    this->viewControlDialog->resetLookDirection();
	}
	XmPictureAlign((XmPictureWidget)this->getCanvas());

	if (this->state.hardwareRender)
	{
	    Pixel p[4];
	    XtVaGetValues(this->getCanvas(),
		XmNunselectedInCursorColor, &p[0],
		XmNunselectedOutCursorColor, &p[1],
		XmNselectedInCursorColor, &p[2],
		XmNselectedOutCursorColor, &p[3],
		NULL);
	    long l[4];
	    l[0] = p[0];
	    l[1] = p[1];
	    l[2] = p[2];
	    l[3] = p[3];

	    this->sendClientMessage(this->atoms.start_cursor, 4, l);
	}
	else
	    XtVaSetValues(this->getCanvas(),
		XmNmode, XmCURSOR_MODE,
		NULL);

	if (!this->selectProbeByInstance(this->currentProbeInstance <= 0 ? 0 : 
	                            this->currentProbeInstance)) {
	    success = FALSE;
	    this->setInteractionMode(NONE);
	    if (in) in->setInteractionModeParameter(NONE);
	} else 
	    if (in) in->setInteractionModeParameter(CURSORS);
	break;

    case PICK:
	if (!this->selectPickByInstance(this->currentPickInstance <= 0 ? 0 : 
					this->currentPickInstance)) {
	    this->setInteractionMode(NONE);
	    success = FALSE;
	    break;
	}

	if (this->viewControlDialog)
	    this->viewControlDialog->managePickForm();

	if ((this->state.pixmap == 0 && !this->state.hardwareRender) ||
	    (this->state.hardwareWindow == 0 && this->state.hardwareRender)||
	    (!this->directInteractionAllowed()))
	{
	    // We didn't install it, so save it away and apply it later.
	    this->pendingInteractionMode = this->currentInteractionMode;	
	    this->currentInteractionMode = NONE;	
	    success = FALSE;
	}
	else
	{
	    if (this->state.hardwareRender) 
	    {
		this->sendClientMessage(this->atoms.start_pick);
	    }
	    else
	    {
		XtVaSetValues(this->getCanvas(), 
		    XmNmode, XmPICK_MODE,
		    NULL);
	    }
	}
	if (in) in->setInteractionModeParameter(PICK);
	break;

    case NAVIGATE:
	if (this->viewControlDialog)
	    this->viewControlDialog->manageNavigationForm();

	if (this->state.hardwareRender) 
	    this->sendClientMessage(this->atoms.start_navigate);
	/*
	 * Set the Picture widget into navigate mode, even if we are doing 
	 * GL render.  This is because the Picture widget will field
	 * key press events for translate speed and rotate speed.
	 */
	XtVaSetValues(this->getCanvas(),
	    XmNmode, XmNAVIGATE_MODE,
	    NULL);

	XmPictureInitializeNavigateMode((XmPictureWidget)this->getCanvas());

	if (in) in->setInteractionModeParameter(NAVIGATE);
	break;

    case PANZOOM:
	/*
	 * Align the navigation direction with the current camera
	 */
	XmPictureAlign((XmPictureWidget)this->getCanvas());

	/*
	 * Indicate that we are looking forward
	 */
	if (this->viewControlDialog)
	    this->viewControlDialog->resetLookDirection();

	if ((this->state.pixmap == 0 && !this->state.hardwareRender) ||
	    (this->state.hardwareWindow == 0 && this->state.hardwareRender)||
	    (!this->directInteractionAllowed()))
	{
	    // We didn't install it, so save it away and apply it later.
	    this->pendingInteractionMode = this->currentInteractionMode;	
	    this->currentInteractionMode = NONE;	
	    success = FALSE;
	}
	else
	{
	    if (this->state.hardwareRender) 
	    {
		this->sendClientMessage(this->atoms.start_panzoom);
	    }
	    else
	    {
		XtVaSetValues(this->getCanvas(), 
		    XmNmode, XmPANZOOM_MODE,
		    NULL);
	    }
	}
	if (in) in->setInteractionModeParameter(PANZOOM);
	break;

    case ROAM:
	if (this->state.degenerateBox) 
	    break;

	/*
	 * Align the navigation direction with the current camera
	 */
	XmPictureAlign((XmPictureWidget)this->getCanvas());

	/*
	 * Indicate that we are looking forward
	 */
	if (this->viewControlDialog)
	{
	    this->viewControlDialog->manageRoamForm();
	    this->viewControlDialog->resetLookDirection();
	}

	/*
	 * Change the Globe Display status based on the state of the toggle
	 * button.  This is in case we are switching from H/W->S/W or 
	 * S/W->H/W rendering.
	 * Set up the Picture widget or send a message to the server.
	 */
	if (!this->state.hardwareRender)
	{
	    XtVaSetValues(this->getCanvas(),
		XmNdisplayGlobe, this->state.globeDisplayed,
		XmNmode, XmROAM_MODE,
		NULL);
	}
	else
	{
	    long l = this->state.globeDisplayed;
	    this->sendClientMessage(this->atoms.display_globe, l);
	    this->sendClientMessage(this->atoms.start_roam);
	}
	if (in) in->setInteractionModeParameter(ROAM);
	break;

    case ROTATE:
	XmPictureAlign((XmPictureWidget)this->getCanvas());
	if (this->viewControlDialog)
	    this->viewControlDialog->resetLookDirection();

	// put up the globe
        if (!this->state.hardwareRender)
        {
            XtVaSetValues(this->getCanvas(), 
		XmNdisplayGlobe, this->state.globeDisplayed, 
		NULL);
        }
        else
        {
	    long l = this->state.globeDisplayed;
            this->sendClientMessage(this->atoms.display_globe, l);
        }
	// If we are in direct interaction mode(...), enter the mode.
        if ((this->state.pixmap == 0 && !this->state.hardwareRender) ||
            (this->state.hardwareWindow == 0 && this->state.hardwareRender)||
            (!this->directInteractionAllowed()))
        {
	    // We didn't install it, so save it away and apply it later.
	    this->pendingInteractionMode = this->currentInteractionMode;	
	    this->currentInteractionMode = NONE;	
	    success = FALSE;
        }
        else
        {
            if (this->state.hardwareRender)
            {
                this->sendClientMessage(this->atoms.start_rotate);
	    }
            else
            {
                XtVaSetValues(this->getCanvas(), 
		    XmNmode, XmROTATION_MODE,
		    NULL);
            }
        }
	if (in) in->setInteractionModeParameter(ROTATE);
	break;

    case ZOOM:
	/*
	 * Align the navigation direction with the current camera
	 */
	XmPictureAlign((XmPictureWidget)this->getCanvas());

	/*
	 * Indicate that we are looking forward
	 */
	if (this->viewControlDialog)
	    this->viewControlDialog->resetLookDirection();

	if ((this->state.pixmap == 0 && !this->state.hardwareRender) ||
	    (this->state.hardwareWindow == 0 && this->state.hardwareRender)||
	    (!this->directInteractionAllowed()))
	{
	    // We didn't install it, so save it away and apply it later.
	    this->pendingInteractionMode = this->currentInteractionMode;	
	    this->currentInteractionMode = NONE;	
	    success = FALSE;
	}
	else
	{
	    if (this->state.hardwareRender) 
	    {
		this->sendClientMessage(this->atoms.start_zoom);
	    }
	    else
	    {
		XtVaSetValues(this->getCanvas(),
		    XmNmode, XmZOOM_MODE,
		    NULL);
	    }
	}
	if (in) in->setInteractionModeParameter(ZOOM);
	break;
    }
    return success;
}
DirectInteractionMode ImageWindow::getInteractionMode()
{
    //
    // Arguably, this should always return the currently installed
    // interaction mode (currentInteractionMode), but adding 
    // pendingInteractionMode which reset currentInteractionMode to 
    // NONE when setInteractionMode() fails, changed the semantics
    // of this function.  It used to return the last value passed to
    // setInteractionMode() which WAS in currentInteractionMode, but
    // now is not always.  So, to keep the semantics the same, we return
    // pendingInteractionMode if not NONE.  
    //
    // NOTE: the fact that the old semantics may have not been very useful
    //	there may actually be the source of a bug in these semantics. 
    //
    DirectInteractionMode mode = this->pendingInteractionMode;
    if (mode == NONE)
	mode = this->currentInteractionMode;
    return mode; 
}

void ImageWindow::beginExecution()
{
    this->DXWindow::beginExecution();
    boolean execOnChange = theDXApplication->getExecCtl()->inExecOnChange();
    if (execOnChange)
	XmPictureExecutionState((XmPictureWidget)this->getCanvas(), True);

    //
    // disable the menu options for changing depth becuase if the user
    // changes one during execution, the exec crashes (we pulled the
    // rug out from under him.)  The options will be turned back on 
    // appropriately because I added configureImageDepth() in endExecution.
    // and in standBy (for eoc mode).
    if (this->imageDepth8Cmd) this->imageDepth8Cmd->deactivate();
    if (this->imageDepth12Cmd) this->imageDepth12Cmd->deactivate();
    if (this->imageDepth15Cmd) this->imageDepth15Cmd->deactivate();
    if (this->imageDepth16Cmd) this->imageDepth16Cmd->deactivate();
    if (this->imageDepth24Cmd) this->imageDepth24Cmd->deactivate();
    if (this->imageDepth32Cmd) this->imageDepth32Cmd->deactivate();
}
void ImageWindow::standBy()
{
    this->DXWindow::standBy();

    boolean execOnChange = theDXApplication->getExecCtl()->inExecOnChange();
    if (this->state.hardwareRender)
    {
	long l = execOnChange? 1: 0;
	this->sendClientMessage(this->atoms.execute_on_change, l);
    }
    if (execOnChange)
	XmPictureExecutionState((XmPictureWidget)this->getCanvas(), False);

    //
    // reset the greying of the image depth buttons because they're all turned
    // grey when beginning an execution.  Changing depth during an execution
    // really pulls the rug out from under the exec.
    //
    this->configureImageDepthMenu();
}
void ImageWindow::endExecution()
{
    this->DXWindow::endExecution();

    boolean execOnChange = theDXApplication->getExecCtl()->inExecOnChange();
    if (this->state.hardwareRender)
    {
	long l = execOnChange? 1: 0;
	this->sendClientMessage(this->atoms.execute_on_change, l);
    }
    if (execOnChange)
	XmPictureExecutionState((XmPictureWidget)this->getCanvas(), False);

    //
    // reset the greying of the image depth buttons because they're all turned
    // grey when beginning an execution.  Changing depth during an execution
    // really pulls the rug out from under the exec.
    //
    this->configureImageDepthMenu();
}

void ImageWindow::newCamera(int image_width, int image_height)
{
    this->state.width = image_width;
    this->state.height = image_height;

    this->pushedSinceExec = FALSE;

    this->newCanvasImage();
}

void ImageWindow::newCamera(double box[4][3], double aamat[4][4],
	double *from, double *to, double *up,
	int image_width, int image_height, double width,
	boolean perspective, double viewAngle)
{
    this->newCamera(image_width, image_height);

    if (!this->node->isA(ClassImageNode))
	return;

    XmBasis basis;
    ImageNode *in = (ImageNode *)this->node;

    basis.Bw[3][0] = box[3][0];
    basis.Bw[2][0] = box[2][0];
    basis.Bw[1][0] = box[1][0];
    basis.Bw[0][0] = box[0][0];

    basis.Bw[3][1] = box[3][1];
    basis.Bw[2][1] = box[2][1];
    basis.Bw[1][1] = box[1][1];
    basis.Bw[0][1] = box[0][1];

    basis.Bw[3][2] = box[3][2];
    basis.Bw[2][2] = box[2][2];
    basis.Bw[1][2] = box[1][2];
    basis.Bw[0][2] = box[0][2];

    basis.Bw[3][3] = basis.Bs[3][3] = 1;
    basis.Bw[2][3] = basis.Bs[2][3] = 1;
    basis.Bw[1][3] = basis.Bs[1][3] = 1;
    basis.Bw[0][3] = basis.Bs[0][3] = 1;

    // Setup the vector value depending on whether it's to, from, or up.
    // Setup the width/heights.

    // Update the picture widget's idea of the camera.
    //
    // XmPictureNewCamera checks to see if we have a degenerate bounding box.
    // It also completely updates the Picture widget's camera information.
    //
    if (!XmPictureNewCamera((XmPictureWidget) this->getCanvas(),
			    basis, aamat,
			    from[0], from[1], from[2],
			    to[0], to[1], to[2],
			    up[0], up[1], up[2],
			    image_width, image_height, width,
			    in->useAutoAxes(), perspective, viewAngle))
    {
	this->state.degenerateBox = TRUE;
	/*
	 * See if we are in Roam or Cursor mode, and, if we are, kick out.
	 */
	if (this->currentInteractionMode == ROAM ||
	    this->currentInteractionMode == CURSORS)
	{
	    WarningMessage("Degenerate bounding box: Roam and 3D Cursors disabled.");
	    this->setInteractionMode(NONE);
	    if (this->viewControlDialog)
		this->viewControlDialog->resetMode();
	}

	if (this->viewControlDialog)
	    this->viewControlDialog->newCamera(
		from, to, up,
		image_width, image_height, width,
		perspective, viewAngle);
    }
    else
    {
	this->cameraInitialized = TRUE;

	this->state.degenerateBox = FALSE;
	if (this->viewControlDialog)
	    this->viewControlDialog->newCamera(
		from, to, up,
		image_width, image_height, width,
		perspective, viewAngle);
	if (XmPictureUndoable((XmPictureWidget)this->getCanvas()))
	    this->getUndoCmd()->activate();
	else
	    this->getUndoCmd()->deactivate();
	if (XmPictureRedoable((XmPictureWidget)this->getCanvas()))
	    this->getRedoCmd()->activate();
	else
	    this->getRedoCmd()->deactivate();
    }
}

void ImageWindow::undoCamera()
{
    if (this->viewControlDialog)
	this->viewControlDialog->resetSetView();

    if (this->state.hardwareRender)
    {
	this->sendClientMessage(this->atoms.undo_camera);
    }
    else
    {
	double to[3];
	double from[3];
	double up[3];
	double width;
	boolean projection;
	double viewAngle;

	int tmp;
	if(!XmPictureGetUndoCamera((XmPictureWidget)this->getCanvas(),
	    &to[0], &to[1], &to[2],
	    &from[0], &from[1], &from[2],
	    &up[0], &up[1], &up[2],
	    &width, &tmp,
	    &viewAngle))
	    return;

	// pc build changed boolean from int to unsigned char
	projection = tmp;
	
	ImageNode *in = (ImageNode *)this->node;
	in->setTo(to, FALSE);
	in->setFrom(from, FALSE);
	in->setUp(up, FALSE);
	in->setWidth(width, FALSE);
	in->setProjection(projection, FALSE);
	in->setViewAngle(viewAngle, FALSE);

	int image_width;
	int image_height;
	in->getResolution(image_width, image_height);

	if (this->viewControlDialog)
	{
	    this->viewControlDialog->resetSetView();
	    this->viewControlDialog->newCamera(
		from, to, up,
		image_width, image_height, width,
		projection, viewAngle);
	}

	in->sendValues(FALSE);

	boolean execOnChange = theDXApplication->getExecCtl()->inExecOnChange();
	if (!execOnChange)
	    theDXApplication->getExecCtl()->executeOnce();
    }

    /*
     * Align the navigation direction with the current camera
     */
    XmPictureAlign((XmPictureWidget)this->getCanvas());

    /*
     * Indicate that we are looking forward
     * Indicate that we are looking at front
     */
    if (this->viewControlDialog)
    {
        this->viewControlDialog->resetLookDirection();
        this->viewControlDialog->resetSetView();
    }

}
void ImageWindow::redoCamera()
{
    if (this->viewControlDialog)
	this->viewControlDialog->resetSetView();

    if (this->state.hardwareRender)
    {
	this->sendClientMessage(this->atoms.redo_camera);
    }
    else
    {
	double to[3];
	double from[3];
	double up[3];
	double width;
	boolean projection;
	double viewAngle;

	int tmp;
	XmPictureGetRedoCamera((XmPictureWidget)this->getCanvas(),
	    &to[0], &to[1], &to[2],
	    &from[0], &from[1], &from[2],
	    &up[0], &up[1], &up[2],
	    &width, &tmp,
	    &viewAngle);

	projection = tmp;
	
	ImageNode *in = (ImageNode *)this->node;
	in->setTo(to, FALSE);
	in->setFrom(from, FALSE);
	in->setUp(up, FALSE);
	in->setWidth(width, FALSE);
	in->setProjection(projection, FALSE);
	in->setViewAngle(viewAngle, FALSE);

	int image_width;
	int image_height;
	in->getResolution(image_width, image_height);

	if (this->viewControlDialog)
	{
	    this->viewControlDialog->resetSetView();
	    this->viewControlDialog->newCamera(
		from, to, up,
		image_width, image_height, width,
		projection, viewAngle);
	}

	in->sendValues(FALSE);

	boolean execOnChange = theDXApplication->getExecCtl()->inExecOnChange();
	if (!execOnChange)
	    theDXApplication->getExecCtl()->executeOnce();
    }
}
void ImageWindow::resetCamera()
{
    /*
     * Save the current camera so undo will work as expected
     */
    if (this->state.hardwareRender) 
    {
	if (!this->pushedSinceExec)
	{
	    this->sendClientMessage(this->atoms.push_camera);
	    this->pushedSinceExec = TRUE;
	}
	this->sendClientMessage(this->atoms.image_reset);
    }
    else
    {
	/*
	 * If the picture widget does not yet have an image,
	 * don't push the undo camera onto the stack
	 */
	Pixmap pixmap;
	XtVaGetValues(this->getCanvas(), XmNpicturePixmap, &pixmap, NULL);
	if (pixmap != XmUNSPECIFIED_PIXMAP)
	{
	    if (!this->pushedSinceExec)
	    {
		XmPicturePushUndoCamera((XmPictureWidget)this->getCanvas());
		this->pushedSinceExec = TRUE;
	    }
	}
    }

    /*
     * Align the navigation direction with the current camera
     */
    XmPictureAlign((XmPictureWidget)this->getCanvas());

    /*
     * Indicate that we are looking forward
     * Indicate that we are looking at front
     */
    if (this->viewControlDialog)
    {
	this->viewControlDialog->resetLookDirection();
	this->viewControlDialog->resetSetView();
    }

    /*
     * Reset up, and trigger an execution.
     */
    ImageNode *in = (ImageNode *)this->node;
    double v[3];
    v[0] = 0; v[1] = 1; v[2] = 0;
    in->setUp(v, FALSE);
    in->enableVector(FALSE,FALSE);// This will not result in a sendValues().
    in->sendValues(FALSE);

    DXExecCtl *ctl = theDXApplication->getExecCtl();
#if NOT_YET
    if (ctl->isVcrExecuting()) {
	ctl->pauseVcr();
	ctl->getExecCtl()->executeOnce();
	ctl->continueVcr();
    } else 
#endif
    if (ctl->assignmentRequiresExecution())
	ctl->executeOnce();
}

boolean ImageWindow::setView(ViewDirection dir)
{
    if (!this->directInteractionAllowed())
	return FALSE;

    long widgetsDirection = TOP;

    switch (dir) {
    case VIEW_NONE:
	return TRUE;

    case VIEW_TOP:
	widgetsDirection = TOP;
	break;
    case VIEW_BOTTOM:
	widgetsDirection = BOTTOM;
	break;
    case VIEW_FRONT:
	widgetsDirection = FRONT;
	break;
    case VIEW_BACK:
	widgetsDirection = BACK;
	break;
    case VIEW_LEFT:
	widgetsDirection = LEFT;
	break;
    case VIEW_RIGHT:
	widgetsDirection = RIGHT;
	break;
    case VIEW_DIAGONAL:
	widgetsDirection = DIAGONAL;
	break;
    case VIEW_OFF_TOP:
	widgetsDirection = OFF_TOP;
	break;
    case VIEW_OFF_BOTTOM:
	widgetsDirection = OFF_BOTTOM;
	break;
    case VIEW_OFF_FRONT:
	widgetsDirection = OFF_FRONT;
	break;
    case VIEW_OFF_BACK:
	widgetsDirection = OFF_BACK;
	break;
    case VIEW_OFF_LEFT:
	widgetsDirection = OFF_LEFT;
	break;
    case VIEW_OFF_RIGHT:
	widgetsDirection = OFF_RIGHT;
	break;
    case VIEW_OFF_DIAGONAL:
	widgetsDirection = OFF_DIAGONAL;
	break;
    }

    if (this->state.hardwareRender)
    {
	if (!this->pushedSinceExec)
	{
	    this->sendClientMessage(this->atoms.push_camera);
	    this->pushedSinceExec = TRUE;
	}
	this->sendClientMessage(this->atoms.set_view, widgetsDirection);
    }
    else
    {
	if (!this->pushedSinceExec)
	{
	    XmPicturePushUndoCamera((XmPictureWidget)this->getCanvas());
	    this->pushedSinceExec = TRUE;
	}
	double from[3];
	double up[3];
	XmPictureSetView((XmPictureWidget)this->getCanvas(), widgetsDirection,
	    &from[0], &from[1], &from[2],
	    &up[0], &up[1], &up[2]);

	ImageNode *in = (ImageNode*)this->node;
	in->setUp(up, FALSE);
	in->setFrom(from, TRUE);
        if (theDXApplication->getExecCtl()->assignmentRequiresExecution())
	    theDXApplication->getExecCtl()->executeOnce();
#if NOT_YET
	// This code doesn't seem to have the same problem as resetCamera().
#endif
    }

    return TRUE;
}

boolean ImageWindow::setLook(LookDirection dir)
{
    if (!this->directInteractionAllowed())
	return FALSE;
    int mdir = XmLOOK_FORWARD;
    double angle = 0;
    const double OurPI = 3.14159;

    switch (dir) {
    case LOOK_FORWARD:
	mdir = XmLOOK_FORWARD;
	break;
    case LOOK_LEFT45:
	mdir = XmLOOK_LEFT;
	angle = OurPI/4;
	break;
    case LOOK_RIGHT45:
	mdir = XmLOOK_RIGHT;
	angle = OurPI/4;
	break;
    case LOOK_UP45:
	mdir = XmLOOK_UP;
	angle = OurPI/4;
	break;
    case LOOK_DOWN45:
	mdir = XmLOOK_DOWN;
	angle = OurPI/4;
	break;
    case LOOK_LEFT90:
	mdir = XmLOOK_LEFT;
	angle = OurPI/2;
	break;
    case LOOK_RIGHT90:
	mdir = XmLOOK_RIGHT;
	angle = OurPI/2;
	break;
    case LOOK_UP90:
	mdir = XmLOOK_UP;
	angle = OurPI/2;
	break;
    case LOOK_DOWN90:
	mdir = XmLOOK_DOWN;
	angle = OurPI/2;
	break;
    case LOOK_BACKWARD:
	mdir = XmLOOK_BACKWARD;
	break;
    case LOOK_ALIGN:
	if (this->state.hardwareRender)
	    // Send 6 by convention with exec
	    this->sendClientMessage(this->atoms.navigate_look_at, (long)6);
	XmPictureAlign((XmPictureWidget)this->getCanvas());
	if (this->viewControlDialog)
	    this->viewControlDialog->resetLookDirection();

	return TRUE;
    }

    if (!this->pushedSinceExec)
    {
	if (this->state.hardwareRender)
	    this->sendClientMessage(this->atoms.push_camera);
	else
	    XmPicturePushUndoCamera((XmPictureWidget)this->getCanvas());
	this->pushedSinceExec = TRUE;
    }

    if (this->state.hardwareRender)
    {
	float tmpfloat = angle;
	long l[5];
	l[0] = mdir;
	memcpy((void*)&l[1], (void*)&tmpfloat, sizeof(float));
	this->sendClientMessage(this->atoms.navigate_look_at, 2, l);
    }
    double to[3];
    double from[3];
    double up[3];
    double width;
    XmPictureChangeLookAt((XmPictureWidget)this->getCanvas(), mdir, angle,
	&to[0], &to[1], &to[2],
	&from[0], &from[1], &from[2],
	&up[0], &up[1], &up[2],
	&width);

    ImageNode *in = (ImageNode*)this->node;
    if (!this->state.hardwareRender)
    {
	in->setTo(to, FALSE);
	in->setFrom(from, FALSE);
	in->setUp(up, TRUE);
    }
    else
    {
	in->setTo(to, FALSE);
	in->setFrom(from, FALSE);
	in->setUp(up, FALSE);
	in->sendValuesQuietly();
    }

    return TRUE;
}

boolean ImageWindow::setConstraint(ConstraintDirection dir)
{
    if (!this->directInteractionAllowed())
	return FALSE;
    int hwconstraint = 0;	// Hardware constraint
    int pwconstraint = 0;	// picture widget constraint

    switch (dir) {
    case CONSTRAINT_NONE:
	hwconstraint = 0;
	pwconstraint = XmCONSTRAIN_NONE;
	break;
    case CONSTRAINT_X:
	hwconstraint = 1;
	pwconstraint = XmCONSTRAIN_X;
	break;
    case CONSTRAINT_Y:
	hwconstraint = 2;
	pwconstraint = XmCONSTRAIN_Y;
	break;
    case CONSTRAINT_Z:
	hwconstraint = 3;
	pwconstraint = XmCONSTRAIN_Z;
	break;
    }

    if (this->state.hardwareRender)
    {
	this->sendClientMessage(this->atoms.cursor_constraint,
				(long)hwconstraint);
    }

    XtVaSetValues(this->getCanvas(),
	XmNconstrainCursor, pwconstraint, NULL);

    return TRUE;
}

ConstraintDirection ImageWindow::getConstraint()
{
    ConstraintDirection dir = CONSTRAINT_NONE;
    int pwconstraint;

    XtVaGetValues(this->getCanvas(),
	XmNconstrainCursor, &pwconstraint, NULL);

    switch (pwconstraint) {
    case XmCONSTRAIN_NONE:
	dir = CONSTRAINT_NONE;
	break;
    case XmCONSTRAIN_X:
	dir = CONSTRAINT_X;
	break;
    case XmCONSTRAIN_Y:
	dir = CONSTRAINT_Y;
	break;
    case XmCONSTRAIN_Z:
	dir = CONSTRAINT_Z;
	break;
    }
    return dir;
}

void ImageWindow::setSoftware(boolean sw)
{
    long glob;
    Pixel p[4];
    long l[4];

    if (!this->node->isA(ClassImageNode))
	return;

    boolean oldSw;
    this->getSoftware(oldSw);
    if (oldSw != sw)
	this->switchingSoftware = TRUE;

    if (!sw)
    {
	this->upWireframeCmd->activate();
	this->downWireframeCmd->activate();

	switch (this->currentInteractionMode)
	{
	     case CURSORS:
		  XtVaGetValues(this->getCanvas(),
		      XmNunselectedInCursorColor, &p[0],
		      XmNunselectedOutCursorColor, &p[1],
		      XmNselectedInCursorColor, &p[2],
		      XmNselectedOutCursorColor, &p[3],
		      NULL);
		  l[0] = p[0];
		  l[1] = p[1];
		  l[2] = p[2];
		  l[3] = p[3];
 
		 this->sendClientMessage(this->atoms.start_cursor, 4, l);
		 break;
 
	     case PICK:
		 this->sendClientMessage(this->atoms.start_pick);
		 break;
	 	
	     case NAVIGATE:
		 this->sendClientMessage(this->atoms.start_navigate);
		 break;
	 	
	     case PANZOOM:
		 this->sendClientMessage(this->atoms.start_panzoom);
		 break;
 
	     case ROAM:
		 glob = this->state.globeDisplayed;
		 this->sendClientMessage(this->atoms.display_globe, glob);
		 this->sendClientMessage(this->atoms.start_roam);
		 break;
	 	
	     case ZOOM:
		 this->sendClientMessage(this->atoms.start_zoom);
		 break;
	    
	     case ROTATE:
		 glob = this->state.globeDisplayed;
		 this->sendClientMessage(this->atoms.display_globe, glob);
		 this->sendClientMessage(this->atoms.start_rotate);
		 break;

	     case NONE:
	     case CAMERA:
	     default:
		 break;
	}
    }
    else
    {
	//
	// Reset the approx mode back to none if it was wireframe.  Use
	// this->node->setApprox, not this->setApproximation so that we can
	// set send to FALSE.
	ImageNode *in = (ImageNode *)this->node;
	ApproxMode mode;
	this->getApproximation(TRUE, mode);
	if(mode == APPROX_WIREFRAME)
	    in->setApprox(TRUE, "\"none\"", FALSE);
	this->upWireframeCmd->deactivate();

	this->getApproximation(FALSE, mode);
	if(mode == APPROX_WIREFRAME)
	    in->setApprox(FALSE, "\"none\"", FALSE);
	this->downWireframeCmd->deactivate();
    }

    ImageNode *in = (ImageNode *)this->node;
    in->enableSoftwareRendering(sw, (oldSw != sw));

    this->configureImageDepthMenu();

    if (this->renderingOptionsDialog)
	this->renderingOptionsDialog->resetApproximations();
}
void ImageWindow::getSoftware(boolean &sw)
{
    DisplayNode *n = (DisplayNode*)this->node;
    ASSERT(n);
    ASSERT(((Node*)n)->isA(ClassDisplayNode));
    sw = n->useSoftwareRendering();
}
void ImageWindow::setApproximation(boolean up, ApproxMode mode)
{
    if (!this->node->isA(ClassImageNode))
	return;
    ImageNode *in = (ImageNode *)this->node;
    switch (mode) {
    case APPROX_NONE:
	in->setApprox(up, "\"none\"");
	break;
    case APPROX_WIREFRAME:
	in->setApprox(up, "\"wireframe\"");
	break;
    case APPROX_DOTS:
	in->setApprox(up, "\"dots\"");
	break;
    case APPROX_BOX:
	in->setApprox(up, "\"box\"");
	break;
    }
}
void ImageWindow::getApproximation(boolean up, ApproxMode &mode)
{
    if (!this->node->isA(ClassImageNode))
	return;
    ImageNode *in = (ImageNode *)this->node;
    const char *s;
    in->getApprox(up, s);
    if      (EqualString(s, "\"none\""))
	mode = APPROX_NONE;
    else if (EqualString(s, "\"wireframe\""))
	mode = APPROX_WIREFRAME;
    else if (EqualString(s, "\"dots\""))
	mode = APPROX_DOTS;
    else if (EqualString(s, "\"box\""))
	mode = APPROX_BOX;
    else
	mode = (ApproxMode)-9999;
}
void ImageWindow::setDensity(boolean up, int density)
{
    if (!this->node->isA(ClassImageNode))
	return;
    ImageNode *in = (ImageNode *)this->node;
    in->setDensity(up, density, TRUE);
}
void ImageWindow::getDensity(boolean up, int &density)
{
    if (!this->node->isA(ClassImageNode))
	return;
    ImageNode *in = (ImageNode *)this->node;
    in->getDensity(up, density);
}

 boolean ImageWindow::setCurrentProbe(int i)
{
    if (! this->selectProbeByInstance(i))
      return FALSE;
    
    if (this->viewControlDialog)
      this->viewControlDialog->setCurrentProbeByInstance(i);

    return TRUE;
}

boolean ImageWindow::setCurrentPick(int i)
{
    if (! this->selectPickByInstance(i))
      return FALSE;
    
    if (this->viewControlDialog)
      this->viewControlDialog->setCurrentPickByInstance(i);

    return TRUE;
}

void ImageWindow::wait4GLAcknowledge()
{

    XEvent e;

    e.xclient.message_type = ((Atom) None); // Undefined atom
    while(e.xclient.message_type != this->atoms.gl_shutdown)
    {
	XCheckTypedWindowEvent(theApplication->getDisplay(), 
			       XtWindow(this->getCanvas()),
			       ClientMessage, 
			       &e);
    }
    this->state.hardwareRenderExists = FALSE;
}

void ImageWindow::sendClientMessage(Atom atom, int num, float  *floats)
{
    XEvent e;
    if (!this->state.hardwareRenderExists || this->state.hardwareWindow == 0)
	return;
    
    e.type = ClientMessage;
    e.xclient.format = 32;
    e.xclient.message_type = atom;
    int i;
    for (i = 0; i < num && i < 5; ++i)
	memcpy(&e.xclient.data.l[i], &floats[i], sizeof(float));
    for (; i < 5; ++i)
	e.xclient.data.l[i] = 0;

    XSendEvent(XtDisplay(this->getRootWidget()), this->state.hardwareWindow,
	True, 0, &e);
}
void ImageWindow::sendClientMessage(Atom atom, int num, long  *longs)
{
    XEvent e;
    if (!this->state.hardwareRenderExists || this->state.hardwareWindow == 0)
	return;
    
    e.type = ClientMessage;
    e.xclient.format = 32;
    e.xclient.message_type = atom;
    int i;
    for (i = 0; i < num && i < 5; ++i)
	e.xclient.data.l[i] = longs[i];
    for (; i < 5; ++i)
	e.xclient.data.l[i] = 0;

    XSendEvent(XtDisplay(this->getRootWidget()), this->state.hardwareWindow,
	True, 0, &e);
}
void ImageWindow::sendClientMessage(Atom atom, int num, short *shorts)
{
    XEvent e;
    if (!this->state.hardwareRenderExists || this->state.hardwareWindow == 0)
	return;
    
    e.type = ClientMessage;
    e.xclient.format = 16;
    e.xclient.message_type = atom;
    int i;
    for (i = 0; i < num && i < 10; ++i)
	e.xclient.data.s[i] = shorts[i];
    for (; i < 10; ++i)
	e.xclient.data.s[i] = 0;

    XSendEvent(XtDisplay(this->getRootWidget()), this->state.hardwareWindow,
	True, 0, &e);
}
void ImageWindow::sendClientMessage(Atom atom, int num, char  *chars)
{
    XEvent e;
    if (!this->state.hardwareRenderExists || this->state.hardwareWindow == 0)
	return;
    
    e.type = ClientMessage;
    e.xclient.format = 8;
    e.xclient.message_type = atom;
    int i;
    for (i = 0; i < num && i < 20; ++i)
	e.xclient.data.b[i] = chars[i];
    for (; i < 20; ++i)
	e.xclient.data.b[i] = 0;

    XSendEvent(XtDisplay(this->getRootWidget()), this->state.hardwareWindow,
	True, 0, &e);
}

//
// Probe Management
//
void ImageWindow::addProbe(Node* probe)
{
    this->getModeCursorsCmd()->activate();
    if (this->viewControlDialog)
	this->viewControlDialog->createProbePulldown();
}

void ImageWindow::deleteProbe(Node* probe)
{
    if (!this->network->containsClassOfNode(ClassProbeNode))
    {
	this->getModeCursorsCmd()->deactivate();
	if (this->getInteractionMode() == CURSORS)
	{
	    this->setInteractionMode(NONE);
	    if (this->viewControlDialog)
		this->viewControlDialog->resetMode();
	}
	this->currentProbeInstance = -1;
    }
    else
    {
	if(probe->getInstanceNumber() == this->currentProbeInstance)
	    this->selectProbeByInstance(0);
	if (this->viewControlDialog)
	    this->viewControlDialog->createProbePulldown();
    }
}

void ImageWindow::changeProbe(Node*)
{
    if (this->viewControlDialog)
	this->viewControlDialog->createProbePulldown();
}

ProbeNode *ImageWindow::getCurrentProbeNode()
{
    ProbeNode *p; 
    if (this->currentProbeInstance <= 0)
	return NULL;

    p = (ProbeNode*)this->getNodeByInstance(ClassProbeNode,"Probe",
			this->currentProbeInstance);
    if (!p)
	p = (ProbeNode*)this->getNodeByInstance(ClassProbeNode,"ProbeList",
			this->currentProbeInstance);
    return p;
}


boolean ImageWindow::selectProbeByInstance(int i)
{

    long       l[5];
    float      tmpfloat;
    double     *darray;	
    double     **vlist;	
    const char *strValue;
    int	       tuple;
    ProbeNode  *probe;

    probe = (ProbeNode*)this->getNodeByInstance(ClassProbeNode,"Probe",i);
    if (!probe)
	probe = (ProbeNode*)this->getNodeByInstance(ClassProbeNode,"ProbeList",
							i);
    if (!probe)
	return FALSE;

    Parameter  *param = probe->getOutput();

    this->currentProbeInstance = probe->getInstanceNumber();

    if(EqualString(probe->getNameString(),"Probe")
       AND param->hasValue())
    {
	double *v = new double[3];
	v[0] = param->getVectorComponentValue(1);
	v[1] = param->getVectorComponentValue(2);
	v[2] = param->getVectorComponentValue(3);
	if(NOT this->state.hardwareRender)
    	{
            XmPictureLoadCursors((XmPictureWidget)this->canvas, 1, &v);
    	}
    	else
    	{

            l[0] = -1;
            l[1] = XmPCR_DELETE;
            this->sendClientMessage(this->atoms.cursor_change, 2, l);

            l[0] = 0;
            l[1] = XmPCR_CREATE;
            tmpfloat = v[0];
            memcpy(&l[2], &tmpfloat, sizeof(float));
            tmpfloat = v[1];
            memcpy(&l[3], &tmpfloat, sizeof(float));
            tmpfloat = v[2];
            memcpy(&l[4], &tmpfloat, sizeof(float));

            this->sendClientMessage(this->atoms.cursor_change, 5, l);
    	}
	delete v;
    }
    else if(EqualString(probe->getNameString(),"ProbeList")
            AND param->hasValue())
    {
	int	  i;
	int	  j;

	strValue = probe->getOutputValueString(1);

	int n = 0;
	int count = DXValue::GetDoublesFromList(strValue, 
					DXType::VectorListType,
					&darray, &tuple);
	vlist = (double**)new double*[count];
	for(i = 0; i < count; i++)
	{
	    vlist[i] = new double[tuple];
	    for(j = 0; j < tuple; j++)
		vlist[i][j] = darray[n++];
	}
	delete darray;

	if(NOT this->state.hardwareRender)
	{
	    XmPictureLoadCursors((XmPictureWidget)this->canvas,count,vlist);
	}
	else {
	    l[0] = -1;
	    l[1] = XmPCR_DELETE;
	    this->sendClientMessage(this->atoms.cursor_change, 2, l);

	    l[1] = XmPCR_CREATE;
	    for (i = 0; i < count; i++)
	    {
		l[0] = i;
		tmpfloat = vlist[i][0]; 
		memcpy(&l[2], &tmpfloat, sizeof(float));
		tmpfloat = vlist[i][1]; 
		memcpy(&l[3], &tmpfloat, sizeof(float));
		tmpfloat = vlist[i][2]; 
		memcpy(&l[4], &tmpfloat, sizeof(float));

		this->sendClientMessage(this->atoms.cursor_change, 5, l);
	    }
	}
	for(i = 0; i < count; i++)
	    delete vlist[i];
	delete vlist;
    }
    else {
	if(NOT this->state.hardwareRender)
	    XmPictureDeleteCursors((XmPictureWidget)this->canvas, -1);
	else
	{
	    l[0] = -1;
	    l[1] = XmPCR_DELETE;
	    this->sendClientMessage(this->atoms.cursor_change, 2, l);
	}
    }

    return TRUE;
}

void ImageWindow::addPick(Node* pick)
{
    this->getModePickCmd()->activate();
    if (this->viewControlDialog)
	this->viewControlDialog->createPickPulldown();
}

void ImageWindow::deletePick(Node* pick)
{
    if (!this->network->containsClassOfNode(ClassPickNode))
    {
	this->getModePickCmd()->deactivate();
	if (this->getInteractionMode() == PICK)
	{
	    this->setInteractionMode(NONE);
	    if (this->viewControlDialog)
		this->viewControlDialog->resetMode();
	}
	this->currentPickInstance = -1;
    }
    else
    {
	if(pick->getInstanceNumber() == this->currentPickInstance)
	    this->selectPickByInstance(0);
	if (this->viewControlDialog)
	    this->viewControlDialog->createPickPulldown();
    }
}

void ImageWindow::changePick(Node*)
{
    if (this->viewControlDialog)
	this->viewControlDialog->createPickPulldown();
}

PickNode *ImageWindow::getCurrentPickNode()
{
    if (this->currentPickInstance <= 0)
	return NULL;
    else
	return (PickNode*)this->getNodeByInstance(ClassPickNode,"Pick",
			this->currentPickInstance);
}

Node *ImageWindow::getNodeByInstance(const char *classname, const char *name,
					int instance)
{
    Node *node = NULL;
    if (instance <= 0) {
	//
	// Find the pick with the lowest instance number.
	//
	List *l = this->network->makeClassifiedNodeList(classname, FALSE);
	if (l) {
	    int mininst = 0;
	    Node *first=NULL;
	    ListIterator iter(*l);
	    while ( (node = (Node*)iter.getNext()) ) {
		int newinst = node->getInstanceNumber();
		if ((mininst == 0) || (newinst < mininst)) {
		    mininst = newinst;
		    first = node;
		}
	    } 
	    node = first;
	    delete l;
	}
    } else {
	node = this->getNetwork()->getNode(name,instance);
    }
    return node;
}

boolean ImageWindow::selectPickByInstance(int i)
{
    PickNode *pick;
    PickNode *curr_pick = this->getCurrentPickNode();

    pick = (PickNode*)this->getNodeByInstance(ClassPickNode,"Pick",i);
    if (!pick)
	return FALSE;

    if (curr_pick && curr_pick != pick)
	curr_pick->pickFrom(NULL);

    this->currentPickInstance = pick->getInstanceNumber();;

    ListIterator images(*this->network->getImageList());
    ImageWindow *w;
    while ( (w = (ImageWindow*)images.getNext()) )
	if (w != this && w->getInteractionMode() == PICK &&
			 w->getCurrentPickNode() == pick)
	    w->setInteractionMode(NONE);

    pick->pickFrom(this->node);

    return TRUE;
}

boolean ImageWindow::setBackgroundColor(const char *color)
{
    ASSERT(this->node->isA(ClassImageNode));
    ImageNode *n = (ImageNode*)this->node;
    n->setBackgroundColor(color);
    return TRUE;
}

boolean ImageWindow::updateBGColorDialog(const char *color)
{
    if (this->backgroundColorDialog)
	this->backgroundColorDialog->installEditorText(color);
    return TRUE;
}


boolean ImageWindow::updateThrottleDialog(double v)
{
    if (this->throttleDialog)
	this->throttleDialog->installThrottleValue(v);
    return TRUE;
}

const char *ImageWindow::getBackgroundColor()
{
    ASSERT(this->node->isA(ClassImageNode));
    ImageNode *n = (ImageNode*)this->node;
    const char *c;
    n->getBackgroundColor(c);
    return c;
}

boolean ImageWindow::postBackgroundColorDialog()
{
    if (!this->backgroundColorDialog)
	this->backgroundColorDialog = new SetBGColorDialog(this);
    this->backgroundColorDialog->post();
    return TRUE;
}

boolean ImageWindow::enableRecord(boolean enable)
{
    ASSERT(this->node->isA(ClassImageNode));
    ImageNode *n = (ImageNode*)this->node;
    n->setRecordEnable (RECORD_ENABLE_NO_RERENDER);
    return TRUE;
}
boolean ImageWindow::setRecordFile(const char *file)
{
    ASSERT(this->node->isA(ClassImageNode));
    ImageNode *n = (ImageNode*)this->node;
    n->setRecordFile(file);
    return TRUE;
}

boolean ImageWindow::setRecordFormat(const char *format)
{
    ASSERT(this->node->isA(ClassImageNode));
    ImageNode *n = (ImageNode*)this->node;
    n->setRecordFormat(format);
    return TRUE;
}

const char *ImageWindow::getRecordFile()
{
    ASSERT(this->node->isA(ClassImageNode));
    ImageNode *n = (ImageNode*)this->node;

    const char *file;
    n->getRecordFile(file);

    return file;
}
const char *ImageWindow::getRecordFormat()
{
    ASSERT(this->node->isA(ClassImageNode));
    ImageNode *n = (ImageNode*)this->node;

    const char *format;
    n->getRecordFormat(format);

    return format;
}

void ImageWindow::getRecordResolution(int &x, int &y)
{
    ASSERT(this->node->isA(ClassImageNode));
    ImageNode *n = (ImageNode*)this->node;

    n->getRecordResolution(x, y);
}

double ImageWindow::getRecordAspect()
{
    ASSERT(this->node->isA(ClassImageNode));
    ImageNode *n = (ImageNode*)this->node;

    double aspect;
    n->getRecordAspect(aspect);

    return aspect;
}

boolean ImageWindow::useRecord()
{
    int recenab;
    ASSERT(this->node->isA(ClassImageNode));
    ImageNode *n = (ImageNode*)this->node;
    n->getRecordEnable (recenab);
    return recenab == NO_RECORD_ENABLE;

}
void ImageWindow::clearFrameBufferOverlay()
{
    
    Window  overlayWID;

    XtVaGetValues(this->getCanvas(),
        XmNoverlayWid, &overlayWID,
        XmNframeBuffer, &this->state.frameBuffer,
        NULL);

    if (this->state.frameBuffer)
    {
        XSetWindowAttributes attributes;
        attributes.background_pixel =
            WhitePixelOfScreen(XtScreen(this->getCanvas())) + 1;
        XChangeWindowAttributes(XtDisplay(this->getCanvas()),
            overlayWID, CWBackPixel, &attributes);
        XClearWindow(XtDisplay(this->getCanvas()), overlayWID);
    }

}
void ImageWindow::newCanvasImage()
{
    if (++this->state.imageCount == 1)
    {
	this->clearFrameBufferOverlay();

	if (this->node->isA(ClassImageNode))
	{
	    this->modeNoneCmd->activate();
	    this->modeCameraCmd->activate();
	    if (this->node && 
		this->network->containsClassOfNode(ClassProbeNode))
		this->modeCursorsCmd->activate();
	    if (this->node && this->network->containsClassOfNode(ClassPickNode))
		this->modePickCmd->activate();
	    this->modeNavigateCmd->activate();
	    this->modePanZoomCmd->activate();
	    this->modeRoamCmd->activate();
	    this->modeRotateCmd->activate();
	    this->modeZoomCmd->activate();
	    this->configureModeMenu();
	}
    }
    this->state.pixmap = XtUnspecifiedPixmap;
    Position x;
    Position y;
    if (this->state.frameBuffer)
    {
	XtVaGetValues(XtParent(XtParent(this->getCanvas())),
	    XmNx, &x, XmNy, &y, NULL);
    }

    Dimension w = this->state.width;
    Dimension h = this->state.height;
    this->state.resizeFromServer = TRUE;
    XtVaSetValues(this->getCanvas(), XmNwidth, w, XmNheight, h, NULL);
    this->state.resizeFromServer = FALSE;

    if (this->state.frameBuffer)
    {
	Position new_x;
	Position new_y;
	if (this->state.frameBuffer)
	{
	    XtVaGetValues(XtParent(XtParent(this->getCanvas())),
		XmNx, &new_x, XmNy, &new_y, NULL);
	}
	if (new_x != x || new_y != y)
	{
	    DisplayNode *dn = (DisplayNode*)this->node;
	    dn->notifyWhereChange(TRUE);
	}
    }

    if (this->getInteractionMode() == PICK)
    {
	XmPictureDeleteCursors((XmPictureWidget)this->getCanvas(), -1);
	PickNode *n = this->getCurrentPickNode();
	if (n)
	    n->resetCursor();
    }

    boolean sw;
    this->getSoftware(sw);
    if (this->switchingSoftware && !sw)
    {
	XmPictureAlign((XmPictureWidget)this->getCanvas());
	if (this->viewControlDialog)
	    this->viewControlDialog->resetLookDirection();
	DirectInteractionMode m = this->getInteractionMode();
	this->setInteractionMode(NONE);
	this->setInteractionMode(m);
	this->setConstraint(this->getConstraint());
	this->undoCmd->deactivate();
	this->redoCmd->deactivate();
	this->switchingSoftware = FALSE;
    } else {
	this->applyPendingInteractionMode();
    }
}

extern "C" void ImageWindow_TrackFrameBufferEH(
    Widget,
    XtPointer clientData,
    XEvent    *event,
    Boolean   *continue_to_dispatch)
{
    ImageWindow *image = (ImageWindow *)clientData;

    image->trackFrameBuffer(event, continue_to_dispatch);
}

void ImageWindow::trackFrameBuffer(XEvent *event, Boolean *continue_to_dispatch)
{
    XWindowAttributes attributes;
    boolean execOnChange = theDXApplication->getExecCtl()->inExecOnChange();
    DisplayNode *dn = (DisplayNode*)this->node;

    *continue_to_dispatch = True;

    switch (event->type) {
    case ReparentNotify:
	Window window;
	this->state.parent.window = event->xreparent.parent;

	XSelectInput(XtDisplay(this->getRootWidget()),
		     this->state.parent.window,
		     StructureNotifyMask);
	XGetWindowAttributes(XtDisplay(this->getRootWidget()),
			     this->state.parent.window,
			     &attributes);

	XTranslateCoordinates
	    (XtDisplay(this->getRootWidget()),
	     this->state.parent.window,
	     RootWindowOfScreen(XtScreen(this->getRootWidget())),
	     attributes.x,
	     attributes.y,
	     &this->state.parent.x,
	     &this->state.parent.y,
	     &window);

	this->state.parent.width  = attributes.width;
	this->state.parent.height = attributes.height;

	/*
	 * If connected, assign a new display string and cause another
	 * execution of the network to occur.
	 */
	dn->notifyWhereChange(TRUE);

	if (!execOnChange)
	    theDXApplication->getExecCtl()->executeOnce();
	break;

    case ConfigureNotify: {

	//
	// Draw a 4 pixel wide/high rectangle at right/bottom of 24 bit
	// window to hide the schmutz. Actually, calc the width/height
	// of the schmutz rectangle base on the position of the Picture
	// widget relative to the root.
	//
	if(this->canvas && 
	   XtIsRealized(this->canvas) && 
	   this->node && 
	   this->state.imageCount > 0)
	{

	    int canvas_root_x, canvas_root_y;

	    XTranslateCoordinates
		(XtDisplay(this->canvas),
		 XtWindow(this->canvas),
		 RootWindowOfScreen(XtScreen(this->getRootWidget())),
		 0,
		 0,
		 &canvas_root_x,
		 &canvas_root_y,
		 &window);
	    int schmutz_width, schmutz_height;
	    schmutz_width = canvas_root_x%4;
	    schmutz_height = canvas_root_y%4;
	
	    Window owid;
	    XtVaGetValues(this->canvas, XmNoverlayWid, &owid, NULL);
	    XUnmapWindow(XtDisplay(this->canvas), owid);
	    XSync(XtDisplay(this->canvas), False);
	    XGetWindowAttributes(XtDisplay(this->canvas),
			 XtWindow(this->canvas),
			 &attributes);
	    
	    unsigned long valuemask=0;

	    GC gc = XCreateGC(XtDisplay(this->canvas), XtWindow(this->canvas),
			valuemask, NULL);
	    XSetForeground(XtDisplay(this->canvas), gc, (Pixel)0);
	    if(schmutz_width > 0)
		XFillRectangle(XtDisplay(this->canvas),
			       XtWindow(this->canvas),
			       gc,
			       attributes.width-schmutz_width, 
			       0, 
			       schmutz_width, 
			       attributes.height);

	    if(schmutz_height > 0)
		XFillRectangle(XtDisplay(this->canvas),
			       XtWindow(this->canvas),
			       gc,
			       0, 
			       attributes.height-schmutz_height, 
			       attributes.width, 
			       schmutz_height);
	    XFlush(XtDisplay(this->canvas));
	    XFreeGC(XtDisplay(this->canvas), gc);
	    XMapWindow(XtDisplay(this->canvas), owid);
	    XFlush(XtDisplay(this->canvas));
	}

	if(!this->state.parent.window) return;

	/*
	 * Get the new (x,y) coordinates.
	 */
	XGetWindowAttributes(XtDisplay(this->getRootWidget()),
			     this->state.parent.window,
			     &attributes);

	XTranslateCoordinates
	    (XtDisplay(this->getRootWidget()),
	     this->state.parent.window,
	     RootWindowOfScreen(XtScreen(this->getRootWidget())),
	     attributes.x,
	     attributes.y,
	     &attributes.x,
	     &attributes.y,
	     &window);

	/*
	 * Save the old x, y values and set new x, y values.
	 */
	int old_x = this->state.parent.x;
	int old_y = this->state.parent.y;
	this->state.parent.x = attributes.x;
	this->state.parent.y = attributes.y;

	/*
	 * If width or height has changed, let the image window
	 * resize handler take care of this problem (which will
	 * also take care of the (x,y) change as well).
	 */
	if (this->state.parent.width  != event->xconfigure.width ||
	    this->state.parent.height != event->xconfigure.height)
	{
	    this->state.parent.width  = event->xconfigure.width;
	    this->state.parent.height = event->xconfigure.height;
	}

	/*
	 * If the new (x,y) coordinates are identical to previous
	 * coordinates, return.
	 */
	if (this->state.parent.x == old_x &&
	    this->state.parent.y == old_y)
	{
	    break;
	}

	if (this->node == NULL)
	    break;

	/*
	 * If connected, assign a new display string and cause another
	 * execution of the network to occur.
	 */
	dn->notifyWhereChange(TRUE);

	if (!execOnChange)
	    theDXApplication->getExecCtl()->executeOnce();
	break;
	}
	

    default:
	break;
    }
}

boolean ImageWindow::HandleExposures(XEvent *event, void *clientData)
{
    ImageWindow *iw = (ImageWindow *)clientData;
    return iw->handleExposures(event);
}

boolean ImageWindow::handleExposures(XEvent*event)
{
    XtVaSetValues(this->getCanvas(), 
	XmNoverlayExposure, True,
	NULL);
    return TRUE;
}

void ImageWindow::setTranslateSpeed(double s)
{
    this->state.navigateTranslateSpeed = s;
    if (this->state.hardwareRender)
    {
	float tmpfloat = s;
	this->sendClientMessage(this->atoms.motion, tmpfloat);
    }
    else
    {
	XtVaSetValues(this->getCanvas(), XmNtranslateSpeed, (int)s, NULL);
    }
    if (this->viewControlDialog)
	this->viewControlDialog->setNavigateSpeed(s);
}

void ImageWindow::setRotateSpeed(double s)
{
    this->state.navigateRotateSpeed = s;
    if (this->state.hardwareRender)
    {
	float tmpfloat = s;
	this->sendClientMessage(this->atoms.pivot, tmpfloat);
    }
    else
    {
	XtVaSetValues(this->getCanvas(), XmNrotateSpeed, (int)s, NULL);
    }
    if (this->viewControlDialog)
	this->viewControlDialog->setNavigatePivot(s);
}

double ImageWindow::getTranslateSpeed()
{
    return this->state.navigateTranslateSpeed;
}

double ImageWindow::getRotateSpeed()
{
    return this->state.navigateRotateSpeed;
}

void ImageWindow::serverDisconnected()
{
    this->resetWindow();
    this->DXWindow::serverDisconnected();
}

void ImageWindow::resetWindow()
{

    //
    // c1orrang1  File/New causes a resetWindow which could happen while in
    // hardware mode.  If so, we'ld better tell the exec about it.
    //
    if ((this->state.hardwareRenderExists) && (theDXApplication->getPacketIF()))
    {
	this->sendClientMessage(this->atoms.gl_destroy_window);
	this->wait4GLAcknowledge();
    }

    this->state.imageCount = 0;

    XtVaSetValues(this->getCanvas(),
	XmNpicturePixmap, XmUNSPECIFIED_PIXMAP,
	NULL);
    
    this->setInteractionMode(NONE);
    if (this->viewControlDialog)
	this->viewControlDialog->setSensitivity(FALSE);

    XmPictureReset((XmPictureWidget)this->getCanvas());

    Window  overlayWID;
    XtVaGetValues(this->getCanvas(),
		    XmNoverlayWid, &overlayWID,
		    XmNframeBuffer, &this->state.frameBuffer,
		    NULL);

    if (this->state.frameBuffer)
    {

	XSetWindowAttributes attributes;
	attributes.background_pixel =
	    BlackPixelOfScreen(XtScreen(this->getCanvas()));
	XChangeWindowAttributes(XtDisplay(this->getCanvas()),
	    overlayWID, CWBackPixel, &attributes);
	XClearWindow(XtDisplay(this->getCanvas()), overlayWID);
    }

    this->undoCmd->deactivate();
    this->redoCmd->deactivate();
    this->modeNoneCmd->deactivate();
    this->modeCameraCmd->deactivate();
    this->modeCursorsCmd->deactivate();
    this->modePickCmd->deactivate();
    this->modeNavigateCmd->deactivate();
    this->modePanZoomCmd->deactivate();
    this->modeRoamCmd->deactivate();
    this->modeRotateCmd->deactivate();
    this->modeZoomCmd->deactivate();
    this->configureModeMenu();

    if (this->state.pixmap)
	XDeleteProperty(
	    XtDisplay(this->getCanvas()),
	    XtWindow(this->getCanvas()),
	    this->atoms.dx_pixmap_id);
    this->state.pixmap = XtUnspecifiedPixmap;
    this->state.hardwareRender = FALSE;
    this->state.hardwareRenderExists = FALSE;
    XSync(XtDisplay(this->getCanvas()), False);

    DisplayNode *in = (DisplayNode*)this->node;
    if (in)
	in->notifyWhereChange(FALSE);
}

void ImageWindow::changeDepth(int depth)
{
int 	  canvas_depth;
ImageNode *in = (ImageNode*)this->node;
Dimension width, height;
int new_depth = depth;
Boolean frame_buffer;
boolean sw;

    //
    // If we have a HW renderer, we need to get rid of it now, while we
    // still have the window to send the ClientMessage.
    //
    if(this->state.hardwareRenderExists)
    {
	this->sendClientMessage(this->atoms.gl_destroy_window);
	this->wait4GLAcknowledge();
    }
    this->getSoftware(sw);
    if(sw)
	this->allowDirectInteraction(FALSE);

    XtVaGetValues(this->getCanvas(), 
		  XmNdepth, &canvas_depth, 
		  XmNframeBuffer, &frame_buffer,
		  NULL);
    if(canvas_depth != depth)
    {
#if 0
	Boolean sup8, sup12, sup16, sup24, sup32;
	XtVaGetValues(this->getCanvas(),
		      XmN8supported,  &sup8,
		      XmN12supported, &sup12,
		      XmN16supported, &sup16,
		      XmN24supported, &sup24,
		      XmN32supported, &sup32,
		      NULL);
#endif
	//
	// If an unsupported depth was requested, revert to the default depth.
	// If the resulting depth is the default depth, we are done, so return.
	//
	if(this->adjustDepth(new_depth))
	    if(new_depth == canvas_depth) return;
	    
	//
	// Get the old width and height so we can restore it
	//
	XtVaGetValues(this->getCanvas(), 
		      XmNwidth,  &width,
		      XmNheight, &height,
		      NULL);
	//
	// Destroy the old picture widget, since it is not possible to
	// dynamically change the depth of a widget.
	//
	XtVaSetValues(this->form, XmNresizePolicy, XmRESIZE_NONE, NULL);
	//
	// UnSetting XmNpicturePixmap is really a strange thing to do to
	// a widget your're about to destroy, but if you look carefully, you'll
	// see that at some future point, after destruction, the widget will
	// try to repaint itself.  In doing so it will use the old pixmap which
	// no longer exists and it will generate an X protocol error.  This
	// defies logic.  To test this, comment out this SetValues, then change
	// depth, turn on rotation, and resize the image window.  Then load
	// just the cfg file.
	//
	XtVaSetValues (this->getCanvas(), XmNpicturePixmap, XmUNSPECIFIED_PIXMAP, NULL);
	XtDestroyWidget(this->getCanvas());

	//
	// Create the new picture widget, with the desired depth.
	//
	this->canvas =
	    XtVaCreateManagedWidget
		("canvas",
		 xmPictureWidgetClass,
		 this->form,
		 XmNsendMotion,			False,
		 XmNmode,			XmNULL_MODE,
		 XmNshowRotatingBBox,		False,
		 XmNglobeRadius,		10,
		 XmNleftAttachment,		XmATTACH_FORM,
		 XmNrightAttachment,		XmATTACH_FORM,
		 XmNtopAttachment,		XmATTACH_FORM,
		 XmNbottomAttachment,		XmATTACH_FORM,
		 XmNdepth,			depth,  
		 XmNwidth,			width,  
		 XmNheight,			height,  
		 XmNhighlightPixmap,		XmUNSPECIFIED_PIXMAP,
		 NULL);

	this->completePictureCreation();

	XtVaSetValues(this->form, XmNresizePolicy, XmRESIZE_ANY, NULL);
	//
	// Install all of the callbacks...
	//
	this->installCallbacks();

	//
	// Blow away the old gc, since the depth does no match any more.
	// remember the old interaction mode and state of the rotation globe.
	// Must turn off the globe before resetting old_mode because the picture
	// widget can't cope otherwise. FIXME
	//
	DirectInteractionMode old_mode = this->getInteractionMode();

	if (this->state.globeDisplayed)
	    this->setDisplayGlobe();

	if(this->state.gc) 
	    XFreeGC(XtDisplay(this->canvas),this->state.gc);
	this->state.gc = NULL;
	this->setInteractionMode((DirectInteractionMode)NONE);
	if(in)
	    in->setDepth(depth);

	//
	// restore the old interaction mode.
	//
	this->pendingInteractionMode = old_mode;
    }

    this->configureImageDepthMenu();

}
boolean ImageWindow::adjustDepth(int &depth)
{
ImageNode *in = (ImageNode*)this->node;
Boolean sup8, sup12, sup15, sup16, sup24, sup32;
int new_depth;

    XtVaGetValues(this->getCanvas(),
		  XmN8supported,  &sup8,
		  XmN12supported, &sup12,
		  XmN15supported, &sup15,
		  XmN16supported, &sup16,
		  XmN24supported, &sup24,
		  XmN32supported, &sup32,
		  NULL);
    //
    // If an unsupported depth was requested revert to the default depth.
    // If the current depth is the default depth, we are done, so return.
    //
    if( ((depth == 8)  && !sup8)  ||
	((depth == 12) && !sup12) ||
	((depth == 15) && !sup16) ||
	((depth == 16) && !sup16) ||
	((depth == 24) && !sup24) ||
	((depth == 32) && !sup32) )
    {
	new_depth = DefaultDepth(theApplication->getDisplay(), 
			   DefaultScreen(theApplication->getDisplay()));

#if 0
	char msg[256];
	if (new_depth < depth) {
	    sprintf (msg, "An image depth of %d is not supported on this hardware. The default image depth (%d) will be used.", depth, new_depth);
	    theDXApplication->getMessageWindow()->addWarning (msg);
	} 
#endif 
	// Inform the Node
	if(in)
	    in->setDepth(new_depth);
	depth = new_depth;
	return TRUE;
    }
    return FALSE;
}

//
// Save the image with the current(or default) name.
//
void ImageWindow::saveImage()
{


}

boolean ImageWindow::postSaveImageDialog()
{
    if(NOT this->saveImageDialog)
	this->saveImageDialog = new SaveImageDialog
	    (this->getRootWidget(), (ImageNode*)this->node, this->commandScope);

    this->saveImageDialog->post();
    // Force Image's Translations onto saveImageDialog here.
    TransferAccelerator(this->saveImageDialog->getRootWidget(), 
    		this->saveOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->saveImageDialog->getRootWidget(), 
    		this->saveImageOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->saveImageDialog->getRootWidget(), 
    		this->printImageOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->saveImageDialog->getRootWidget(), 
    		this->closeOption->getRootWidget(), "ArmAndActivate");
    if(this->openAllControlPanelsOption)
    TransferAccelerator(this->saveImageDialog->getRootWidget(), 
    		this->openAllControlPanelsOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->saveImageDialog->getRootWidget(), 
    		this->openAllColormapEditorsOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->saveImageDialog->getRootWidget(), 
    		this->messageWindowOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->saveImageDialog->getRootWidget(), 
    		this->undoOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->saveImageDialog->getRootWidget(), 
    		this->redoOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->saveImageDialog->getRootWidget(), 
    		this->resetOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->saveImageDialog->getRootWidget(), 
    		this->executeOnceOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->saveImageDialog->getRootWidget(), 
    		this->executeOnChangeOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->saveImageDialog->getRootWidget(), 
    		this->endExecutionOption->getRootWidget(), "ArmAndActivate");

    TransferAccelerator(this->saveImageDialog->getRootWidget(), 
    		this->modeCameraOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->saveImageDialog->getRootWidget(), 
    		this->modeCursorsOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->saveImageDialog->getRootWidget(), 
    		this->modePickOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->saveImageDialog->getRootWidget(), 
    		this->modeNavigateOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->saveImageDialog->getRootWidget(), 
    		this->modePanZoomOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->saveImageDialog->getRootWidget(), 
    		this->modeRoamOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->saveImageDialog->getRootWidget(), 
    		this->modeRotateOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->saveImageDialog->getRootWidget(), 
    		this->modeZoomOption->getRootWidget(), "ArmAndActivate");

    return TRUE;
}

boolean ImageWindow::postPrintImageDialog()
{
    if(NOT this->printImageDialog)
	this->printImageDialog = new PrintImageDialog
	    (this->getRootWidget(), (ImageNode*)this->node, this->commandScope);

    this->printImageDialog->post();
    // Force Image's Translations onto PrintImageDialog here.
    TransferAccelerator(this->printImageDialog->getRootWidget(), 
    		this->saveOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->printImageDialog->getRootWidget(), 
    		this->saveImageOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->printImageDialog->getRootWidget(), 
    		this->printImageOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->printImageDialog->getRootWidget(), 
    		this->closeOption->getRootWidget(), "ArmAndActivate");
    if(this->openAllControlPanelsOption)
    TransferAccelerator(this->printImageDialog->getRootWidget(), 
    		this->openAllControlPanelsOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->printImageDialog->getRootWidget(), 
    		this->openAllColormapEditorsOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->printImageDialog->getRootWidget(), 
    		this->messageWindowOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->printImageDialog->getRootWidget(), 
    		this->undoOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->printImageDialog->getRootWidget(), 
    		this->redoOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->printImageDialog->getRootWidget(), 
    		this->resetOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->printImageDialog->getRootWidget(), 
    		this->executeOnceOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->printImageDialog->getRootWidget(), 
    		this->executeOnChangeOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->printImageDialog->getRootWidget(), 
    		this->endExecutionOption->getRootWidget(), "ArmAndActivate");

    TransferAccelerator(this->printImageDialog->getRootWidget(), 
    		this->modeCameraOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->printImageDialog->getRootWidget(), 
    		this->modeCursorsOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->printImageDialog->getRootWidget(), 
    		this->modePickOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->printImageDialog->getRootWidget(), 
    		this->modeNavigateOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->printImageDialog->getRootWidget(), 
    		this->modePanZoomOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->printImageDialog->getRootWidget(), 
    		this->modeRoamOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->printImageDialog->getRootWidget(), 
    		this->modeRotateOption->getRootWidget(), "ArmAndActivate");
    TransferAccelerator(this->printImageDialog->getRootWidget(), 
    		this->modeZoomOption->getRootWidget(), "ArmAndActivate");

    return TRUE;
}


//
// Before calling the super class method we verify that doing this will
// leave at least one startup image window.  If not, issue an error and
// return without calling the super class method.
//
void ImageWindow::toggleWindowStartup()
{
    List *images = this->network->getImageList();
    int doit = FALSE;
    ImageWindow *iw;
    ListIterator iterator(*images);

    while ( (iw = (ImageWindow*)iterator.getNext()) )  {
	if ((iw != this) && iw->isStartup())  {
	    doit = TRUE;
	    break;
	}
    }

    if (doit)
	this->DXWindow::toggleWindowStartup();
    else
	ErrorMessage("You must have at least one start up image window");

}
//
// Set the state and sensitivity of the ImageDepth cascade menu and its
// command interfaces.
//
void ImageWindow::configureImageDepthMenu()
{
    if (this->imageDepthCascade) {
	ASSERT(this->imageDepth8Option);
	ASSERT(this->imageDepth12Option);
	ASSERT(this->imageDepth15Option);
	ASSERT(this->imageDepth16Option);
	ASSERT(this->imageDepth24Option);
	ASSERT(this->imageDepth32Option);

	if (!this->node) {
	    this->imageDepthCascade->deactivate();
	    this->imageDepth8Cmd->deactivate();
	    this->imageDepth12Cmd->deactivate();
	    this->imageDepth15Cmd->deactivate();
	    this->imageDepth16Cmd->deactivate();
	    this->imageDepth24Cmd->deactivate();
	    this->imageDepth32Cmd->deactivate();
	} else {
	    boolean sw;
	    this->getSoftware(sw);
	    if (sw) {
		this->imageDepthCascade->activate();
	    } else {
		this->imageDepthCascade->deactivate();
	    }

	    Boolean sup8, sup12, sup15, sup16, sup24, sup32, frame_buffer;
	    int canvas_depth;
	    XtVaGetValues(this->getCanvas(),
		      XmNdepth, 	&canvas_depth, 
		      XmNframeBuffer,  &frame_buffer,
		      XmN8supported,  &sup8,
		      XmN12supported, &sup12,
		      XmN15supported, &sup15,
		      XmN16supported, &sup16,
		      XmN24supported, &sup24,
		      XmN32supported, &sup32,
		      NULL);

	    if (frame_buffer || !sw) {
		//
		// Set depth to 24 so the menu options look correct.
		//
		if (frame_buffer)
		    canvas_depth = 24;
		this->imageDepth8Cmd->deactivate();
		this->imageDepth12Cmd->deactivate();
		this->imageDepth15Cmd->deactivate();
		this->imageDepth16Cmd->deactivate();
		this->imageDepth24Cmd->deactivate();
		this->imageDepth32Cmd->deactivate();
	    } else { 
		if (sup8 && canvas_depth != 8) 
		    this->imageDepth8Cmd->activate();
		else
		    this->imageDepth8Cmd->deactivate();
		if (sup12 && canvas_depth != 12) 
		    this->imageDepth12Cmd->activate();
		if (sup15 && canvas_depth != 15) 
		    this->imageDepth15Cmd->activate();
		else
		    this->imageDepth15Cmd->deactivate();
		if (sup16 && canvas_depth != 16) 
		    this->imageDepth16Cmd->activate();
		else
		    this->imageDepth16Cmd->deactivate();
		if (sup24 && canvas_depth != 24) 
		    this->imageDepth24Cmd->activate();
		else
		    this->imageDepth24Cmd->deactivate();
		if (sup32 && canvas_depth != 32) 
		    this->imageDepth32Cmd->activate();
		else
		    this->imageDepth32Cmd->deactivate();
	     }
		
	    this->imageDepth8Option->setState(canvas_depth == 8);
	    this->imageDepth12Option->setState(canvas_depth == 12);
	    this->imageDepth15Option->setState(canvas_depth == 15);
	    this->imageDepth16Option->setState(canvas_depth == 16);
	    this->imageDepth24Option->setState(canvas_depth == 24);
	    this->imageDepth32Option->setState(canvas_depth == 32);
	}
    }
}

void ImageWindow::configureModeMenu()
{
    this->modeNoneOption->setState(this->getInteractionMode() == NONE);
    this->modeCameraOption->setState(this->getInteractionMode() == CAMERA);
    this->modeCursorsOption->setState(this->getInteractionMode() == CURSORS);
    this->modePickOption->setState(this->getInteractionMode() == PICK);
    this->modeNavigateOption->setState(this->getInteractionMode() == NAVIGATE);
    this->modePanZoomOption->setState(this->getInteractionMode() == PANZOOM);
    this->modeRoamOption->setState(this->getInteractionMode() == ROAM);
    this->modeRotateOption->setState(this->getInteractionMode() == ROTATE);
    this->modeZoomOption->setState(this->getInteractionMode() == ZOOM);

    this->modeOptionCascade->setActivationFromChildren();
}

Network *ImageWindow::getNetwork()
{
    return this->network;
}
//
// Update any displayed information from the new cfg state found in
// the associated ImageNode. 
//
void ImageWindow::updateFromNewCfgState()
{
    Dialog *d; 

    // 
    // Ask the dialogs that display cfg information to update themselves
    // if they are managed.
    // 
    d = this->viewControlDialog;
    if (d && d->isManaged()) d->manage();
    d = this->renderingOptionsDialog;
    if (d && d->isManaged()) d->manage();
    d = this->backgroundColorDialog;
    if (d && d->isManaged()) d->manage();
    d = this->throttleDialog;
    if (d && d->isManaged()) d->manage();
    d = this->autoAxesDialog;
    if (d && d->isManaged()) d->manage();

	
}

void ImageWindow::setAutoAxesDialogTicks()
{
    if (this->autoAxesDialog)
        this->autoAxesDialog->setAutoAxesDialogTicks();
}

void ImageWindow::setAutoAxesDialogXTickLocs()
{
    if (this->autoAxesDialog)
        this->autoAxesDialog->setAutoAxesDialogXTickLocs();
}

void ImageWindow::setAutoAxesDialogYTickLocs()
{
    if (this->autoAxesDialog)
        this->autoAxesDialog->setAutoAxesDialogYTickLocs();
}

void ImageWindow::setAutoAxesDialogZTickLocs()
{
    if (this->autoAxesDialog)
        this->autoAxesDialog->setAutoAxesDialogZTickLocs();
}

void ImageWindow::setAutoAxesDialogXTickLabels()
{
    if (this->autoAxesDialog)
        this->autoAxesDialog->setAutoAxesDialogXTickLabels();
}

void ImageWindow::setAutoAxesDialogYTickLabels()
{
    if (this->autoAxesDialog)
        this->autoAxesDialog->setAutoAxesDialogYTickLabels();
}

void ImageWindow::setAutoAxesDialogZTickLabels()
{
    if (this->autoAxesDialog)
        this->autoAxesDialog->setAutoAxesDialogZTickLabels();
}

void ImageWindow::setAutoAxesDialogFrame()
{
    if (this->autoAxesDialog)
        this->autoAxesDialog->setAutoAxesDialogFrame();
}

void ImageWindow::setAutoAxesDialogGrid()
{
    if (this->autoAxesDialog)
        this->autoAxesDialog->setAutoAxesDialogGrid();
}

void ImageWindow::setAutoAxesDialogAdjust()
{
    if (this->autoAxesDialog)
        this->autoAxesDialog->setAutoAxesDialogAdjust();
}

void ImageWindow::setAutoAxesDialogLabels()
{
    if (this->autoAxesDialog)
        this->autoAxesDialog->setAutoAxesDialogLabels();
}

void ImageWindow::setAutoAxesDialogLabelScale()
{
    if (this->autoAxesDialog)
        this->autoAxesDialog->setAutoAxesDialogLabelScale();
}

void ImageWindow::setAutoAxesDialogFont()
{
    if (this->autoAxesDialog)
        this->autoAxesDialog->setAutoAxesDialogFont();
}

void ImageWindow::setAutoAxesDialogAnnotationColors()
{
    if (this->autoAxesDialog)
        this->autoAxesDialog->setAutoAxesDialogAnnotationColors();
}

void ImageWindow::setAutoAxesDialogCorners()
{
    if (this->autoAxesDialog)
        this->autoAxesDialog->setAutoAxesDialogCorners();
}

void ImageWindow::setAutoAxesDialogCursor()
{
    if (this->autoAxesDialog)
        this->autoAxesDialog->setAutoAxesDialogCursor();
}

void ImageWindow::setAutoAxesDialogEnable()
{
    if (this->autoAxesDialog)
        this->autoAxesDialog->setAutoAxesDialogEnable();
}

boolean ImageWindow::isBGColorConnected()
{
    if (!this->node->isA(ClassImageNode))
	return False;
    return ((ImageNode*)this->node)->isBGColorConnected();
}

boolean ImageWindow::isThrottleConnected()
{
    if (!this->node->isA(ClassImageNode))
	return False;
    return ((ImageNode*)this->node)->isThrottleConnected();
}

boolean ImageWindow::isRecordEnableConnected()
{
    if (!this->node->isA(ClassImageNode))
	return False;
    return ((ImageNode*)this->node)->isRecordEnableConnected();
}

boolean ImageWindow::isRecordFileConnected()
{
    if (!this->node->isA(ClassImageNode))
	return False;
    return ((ImageNode*)this->node)->isRecordFileConnected();
}

boolean ImageWindow::isRecordFormatConnected()
{
    if (!this->node->isA(ClassImageNode))
	return False;
    return ((ImageNode*)this->node)->isRecordFormatConnected();
}

boolean ImageWindow::isRecordResolutionConnected()
{
    if (!this->node->isA(ClassImageNode))
      return False;
    return ((ImageNode*)this->node)->isRecordResolutionConnected();
}

boolean ImageWindow::isRecordAspectConnected()
{
    if (!this->node->isA(ClassImageNode))
      return False;
    return ((ImageNode*)this->node)->isRecordAspectConnected();
}

boolean ImageWindow::isAutoAxesEnableConnected()
{
    if (!this->node->isA(ClassImageNode))
	return False;
    return ((ImageNode*)this->node)->isAutoAxesEnableConnected();
}

boolean ImageWindow::isAutoAxesCornersConnected()
{
    if (!this->node->isA(ClassImageNode))
	return False;
    return ((ImageNode*)this->node)->isAutoAxesCornersConnected();
}

boolean ImageWindow::isAutoAxesCursorConnected()
{
    if (!this->node->isA(ClassImageNode))
	return False;
    return ((ImageNode*)this->node)->isAutoAxesCursorConnected();
}

boolean ImageWindow::isAutoAxesFrameConnected()
{
    if (!this->node->isA(ClassImageNode))
	return False;
    return ((ImageNode*)this->node)->isAutoAxesFrameConnected();
}

boolean ImageWindow::isAutoAxesGridConnected()
{
    if (!this->node->isA(ClassImageNode))
	return False;
    return ((ImageNode*)this->node)->isAutoAxesGridConnected();
}

boolean ImageWindow::isAutoAxesAdjustConnected()
{
    if (!this->node->isA(ClassImageNode))
	return False;
    return ((ImageNode*)this->node)->isAutoAxesAdjustConnected();
}

boolean ImageWindow::isAutoAxesAnnotationConnected()
{
    if (!this->node->isA(ClassImageNode))
	return False;
    return ((ImageNode*)this->node)->isAutoAxesAnnotationConnected();
}

boolean ImageWindow::isAutoAxesLabelsConnected()
{
    if (!this->node->isA(ClassImageNode))
	return False;
    return ((ImageNode*)this->node)->isAutoAxesLabelsConnected();
}

boolean ImageWindow::isAutoAxesColorsConnected()
{
    if (!this->node->isA(ClassImageNode))
	return False;
    return ((ImageNode*)this->node)->isAutoAxesColorsConnected();
}

boolean ImageWindow::isAutoAxesFontConnected()
{
    if (!this->node->isA(ClassImageNode))
	return False;
    return ((ImageNode*)this->node)->isAutoAxesFontConnected();
}

boolean ImageWindow::isAutoAxesTicksConnected()
{
    if (!this->node->isA(ClassImageNode))
	return False;
    return ((ImageNode*)this->node)->isAutoAxesTicksConnected();
}

boolean ImageWindow::isAutoAxesXTickLocsConnected()
{
    if (!this->node->isA(ClassImageNode))
	return False;
    return ((ImageNode*)this->node)->isAutoAxesXTickLocsConnected();
}

boolean ImageWindow::isAutoAxesYTickLocsConnected()
{
    if (!this->node->isA(ClassImageNode))
	return False;
    return ((ImageNode*)this->node)->isAutoAxesYTickLocsConnected();
}

boolean ImageWindow::isAutoAxesZTickLocsConnected()
{
    if (!this->node->isA(ClassImageNode))
	return False;
    return ((ImageNode*)this->node)->isAutoAxesZTickLocsConnected();
}

boolean ImageWindow::isAutoAxesXTickLabelsConnected()
{
    if (!this->node->isA(ClassImageNode))
	return False;
    return ((ImageNode*)this->node)->isAutoAxesXTickLabelsConnected();
}

boolean ImageWindow::isAutoAxesYTickLabelsConnected()
{
    if (!this->node->isA(ClassImageNode))
	return False;
    return ((ImageNode*)this->node)->isAutoAxesYTickLabelsConnected();
}

boolean ImageWindow::isAutoAxesZTickLabelsConnected()
{
    if (!this->node->isA(ClassImageNode))
	return False;
    return ((ImageNode*)this->node)->isAutoAxesZTickLabelsConnected();
}

boolean ImageWindow::isAutoAxesLabelScaleConnected()
{
    if (!this->node->isA(ClassImageNode))
	return False;
    return ((ImageNode*)this->node)->isAutoAxesLabelScaleConnected();
}

boolean ImageWindow::isRenderModeConnected()
{
    if (!this->node->isA(ClassImageNode))
      return False;
    return ((ImageNode*)this->node)->isRenderModeConnected();
}

boolean ImageWindow::isButtonUpApproxConnected()
{
    if (!this->node->isA(ClassImageNode))
      return False;
    return ((ImageNode*)this->node)->isButtonUpApproxConnected();
}

boolean ImageWindow::isButtonDownApproxConnected()
{
    if (!this->node->isA(ClassImageNode))
      return False;
    return ((ImageNode*)this->node)->isButtonDownApproxConnected();
}

boolean ImageWindow::isButtonUpDensityConnected()
{
    if (!this->node->isA(ClassImageNode))
      return False;
    return ((ImageNode*)this->node)->isButtonUpDensityConnected();
}

boolean ImageWindow::isButtonDownDensityConnected()
{
    if (!this->node->isA(ClassImageNode))
      return False;
    return ((ImageNode*)this->node)->isButtonDownDensityConnected();
}

void ImageWindow::sensitizePrintImageDialog(boolean flag)
{
    if (this->printImageCmd)
	this->printImageCmd->activate();

    if (this->printImageDialog)
	this->printImageDialog->setCommandActivation();
}

void ImageWindow::sensitizeSaveImageDialog(boolean flag)
{
    if (this->saveImageCmd)
	this->saveImageCmd->activate();

    if (this->saveImageDialog)
	this->saveImageDialog->setCommandActivation();
}

void ImageWindow::sensitizeThrottleDialog(boolean flag)
{
    if (flag == False)
    {
	if (this->throttleCmd)
	    this->throttleCmd->deactivate();
	if (this->throttleDialog && this->throttleDialog->isManaged())
	    this->throttleDialog->unmanage();
    }
    else 
    {
	if (this->throttleCmd)
	    this->throttleCmd->activate();
	double v;
	this->getThrottle(v);
	this->updateThrottleDialog(v);
    }
}

void ImageWindow::sensitizeBackgroundColorDialog(boolean flag)
{
    if (flag == False)
    {
	if (this->backgroundColorCmd)
	    this->backgroundColorCmd->deactivate();
	if (this->backgroundColorDialog &&
			this->backgroundColorDialog->isManaged())
	    this->backgroundColorDialog->unmanage();
    }
    else 
    {
	if (this->backgroundColorCmd)
	    this->backgroundColorCmd->activate();
	this->updateBGColorDialog(NULL);
    }
}



void ImageWindow::updateAutoAxesDialog()
{
    if (this->autoAxesDialog)
	this->autoAxesDialog->update();
}

void ImageWindow::sensitizeViewControl(boolean flag)
{
    if (flag)
    {
	List *l = this->network->makeClassifiedNodeList(ClassProbeNode, FALSE);
	if (l) {
	    delete l;
	    this->modeCursorsCmd->activate();
	} 

	l = this->network->makeClassifiedNodeList(ClassPickNode);
	if (l) {
	    delete l;
	    this->modePickCmd->activate();
	} 
	this->modeNoneCmd->activate();
	this->modeCameraCmd->activate();
	this->modeNavigateCmd->activate();
	this->modePanZoomCmd->activate();
	this->modeRoamCmd->activate();
	this->modeRotateCmd->activate();
	this->modeZoomCmd->activate();
	if (this->viewControlDialog)
	{
	    this->viewControlDialog->sensitizeProbeOptionMenu(TRUE);
	    this->viewControlDialog->sensitizePickOptionMenu(TRUE);
	}
    }
    else
    {
	this->modeNoneCmd->deactivate();
	this->modeCameraCmd->deactivate();
	this->modeNavigateCmd->deactivate();
	this->modePanZoomCmd->deactivate();
	this->modeRoamCmd->deactivate();
	this->modeRotateCmd->deactivate();
	this->modeZoomCmd->deactivate();
	this->modeCursorsCmd->deactivate();
	this->modePickCmd->deactivate();
	if (this->viewControlDialog)
	{
	    this->viewControlDialog->resetMode();
	    this->viewControlDialog->sensitizeProbeOptionMenu(FALSE);
	    this->viewControlDialog->sensitizePickOptionMenu(FALSE);
	}
    }
    this->configureModeMenu();
}

void ImageWindow::sensitizeChangeImageName(boolean flag)
{
    if (flag)
    {
	if (this->changeImageNameCmd)
	    this->changeImageNameCmd->activate();
	if ((this->changeImageNameDialog) && (this->changeImageNameDialog->isManaged()))
	    this->changeImageNameDialog->installEditorText();
    }
    else
    {
	if (this->changeImageNameCmd)
	    this->changeImageNameCmd->deactivate();
	if (this->changeImageNameDialog)
	    this->changeImageNameDialog->unmanage();
    }
}

void ImageWindow::sensitizeRenderMode(boolean flag)
{
    if (this->renderingOptionsDialog)
       this->renderingOptionsDialog->sensitizeRenderMode(flag);
}

void ImageWindow::sensitizeButtonUpApprox(boolean flag)
{
    if (this->renderingOptionsDialog)
       this->renderingOptionsDialog->sensitizeButtonUpApprox(flag);
}

void ImageWindow::sensitizeButtonDownApprox(boolean flag)
{
    if (this->renderingOptionsDialog)
       this->renderingOptionsDialog->sensitizeButtonDownApprox(flag);
}

void ImageWindow::sensitizeButtonUpDensity(boolean flag)
{
    if (this->renderingOptionsDialog)
       this->renderingOptionsDialog->sensitizeButtonUpDensity(flag);
}

void ImageWindow::sensitizeButtonDownDensity(boolean flag)
{
    if (this->renderingOptionsDialog)
       this->renderingOptionsDialog->sensitizeButtonDownDensity(flag);
}

void ImageWindow::updateRenderingOptionsDialog()
{
    if (this->renderingOptionsDialog)
      this->renderingOptionsDialog->update();
}


//
// If the X window id of the canvas changed then set the WHERE parameter
// dirty.  As of Aug,95 MainWindow::setGeometry was implemented using
// unrealize/realize.
//
void ImageWindow::setGeometry(int  x, int  y, int  width, int  height)
{
    //
    // The comments, if they exist may cause a resize, but we don't
    // want to cause an execution that normally results from  a resize
    // if we're currently reading a network.  So we'll "push" them.
    // When we're finished reading the network, we'll go back and "pop"
    // them and apply the settings.  Important result:  when you're
    // reading both .net and .cfg the image window won't move, then
    // go away and reappear in the same spot - a result of applying
    // window placment info twice.  
    //
    boolean resettable_eoc_mode = FALSE;
    if ((this->isExecuteOnResize()) && (this->network->isReadingNetwork()))
	resettable_eoc_mode = TRUE;
    if (resettable_eoc_mode) 
	this->setExecuteOnResize(FALSE);

    DisplayNode* dn = (DisplayNode*)this->node;
    if (dn) {
	ASSERT(this->node->isA(ClassDisplayNode));
    }
    Widget canvas = this->getCanvas();
    Window oldWindow = XtWindow(canvas);
    if ((this->hasPendingWindowPlacement()) || (resettable_eoc_mode)) {
	this->pending_resize_x = x;
	this->pending_resize_y = y;
	this->pending_resize_width = width;
	this->pending_resize_height = height;
        if (dn) dn->notifyWhereChange(FALSE);
    } else {
	//
	// The way it's supposed to work:  If you call setGeometry, then
	// ImageWindow::resizeImage will be called also - as the window
	// resizes.  But if the args to setGeometry are equal to current
	// dimensions then resizeImage won't be called.  That's bad because
	// it means that loading a new cfg file which contains Camera(resolution)
	// and window dims which disagree will result in the Camera(resolution)
	// param giving us a new size after execution.  Problem is we don't know
	// what resolution and aspect should be.  We rely on Motif to tell
	// us that.
	//
	// Normal behavior inside a Picture widget includes setting its
	// own XmNpicturePixmap to XmUNSPECIFIED_PIXMAP whenever the widget
	// changes size.  We're doing that for Picture in this case because
	// when we set new size and new size == old size, the widget's Resize
	// method is not called.  Note that MainWindow::setGeometry could
	// possible do the Unrealize/Realize thing even the new dimensions
	// are equal to the old. (It should also work to unset XmNpicturePixmap
	// inside the Realize method of Picture widget.)
	//
	this->DXWindow::setGeometry(x, y, width, height);
	XSync (XtDisplay(theApplication->getRootWidget()), False);
	this->resizeImage(FALSE);
    }
 
    Window newWindow = XtWindow(canvas);
 
    if ((oldWindow != newWindow) && (dn)) {
        dn->notifyWhereChange(FALSE);
	this->allowDirectInteraction(FALSE);
    }

    if (resettable_eoc_mode)
	this->resetExecuteOnResizeWhenAble();
}

//
// Called by the MainWindow CloseCallback.  We call the super class
// method and then, if we are not an anchor and are in DataViewer mode
// then we exit the program.
//
void ImageWindow::closeWindow()
{
    this->DXWindow::closeWindow();
    if (!this->anchor && theDXApplication->inDataViewerMode())
	theDXApplication->shutdownApplication();
}

void ImageWindow::postWizard()
{
    if (theDXApplication->inWizardMode() == FALSE) return;

    boolean keep_trying = TRUE;
    if (theDXApplication->isWizardWindow(this->UIComponent::name)) {
	Command *cmd = this->network->getHelpOnNetworkCommand();
	if (cmd->isActive()) {
	    cmd->execute();
	    keep_trying = FALSE;
	} 
    }

    if (keep_trying)
	this->DXWindow::postWizard();
}

//
// Use a workproc to turn execute-on-resize back on.  This is done on behalf
// of DisplayNode::parseCommonComments() which is triggering an execution.
// It calls setGeometry(), setExecuteOnResize(TRUE) in an attempt to prevent an
// execution following parsing/setting geometry, but it doesn't work, because
// the resize happens async.
//
void ImageWindow::resetExecuteOnResizeWhenAble()
{
    if (this->reset_eor_wp)
	XtRemoveWorkProc (this->reset_eor_wp);
    XtAppContext apcxt = theApplication->getApplicationContext();
    this->reset_eor_wp = XtAppAddWorkProc (apcxt, (XtWorkProc)
	ImageWindow_ResetEORWP, (XtPointer)this);
    ImageWindow::NeedsSyncForResize = TRUE;
}

extern "C" Boolean ImageWindow_ResetEORWP (XtPointer clientData)
{
    ImageWindow* iw = (ImageWindow*)clientData;
    ASSERT(iw);
    if (ImageWindow::NeedsSyncForResize) {
	XSync (XtDisplay(iw->getRootWidget()), False);
	ImageWindow::NeedsSyncForResize = FALSE;
	return False;
    }

    iw->reset_eor_wp = NUL(XtWorkProcId);
    iw->setGeometry( iw->pending_resize_x, iw->pending_resize_y,
	iw->pending_resize_width, iw->pending_resize_height);
    iw->setExecuteOnResize(TRUE);
    return TRUE;
}


void ImageWindow::getGeometryAlternateNames(String* names, int* count, int max)
{
    int cnt = *count;

    if (cnt < (max-1)) {
	List* il = this->network->getImageList();
	int pos = il->getPosition((void*)this);

	if (pos) {
	    char* name = new char[32];
	    sprintf (name, "%s%d", this->name,pos);
	    names[cnt++] = name;
	    *count = cnt;
	}
    }
    this->DXWindow::getGeometryAlternateNames(names, count, max);
}

void ImageWindow::setWindowTitle(const char* name, boolean check_geometry)
{
    DXPacketIF* p = theDXApplication->getPacketIF();
    //
    // We can't handle a where param change during an execution.  The
    // new window placement code could cause one: connect a wire to
    // the image title parameter.
    //
    this->DXWindow::setWindowTitle(name, (check_geometry && (p==NUL(DXPacketIF*))));
}
