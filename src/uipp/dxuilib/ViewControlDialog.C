/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/PushB.h>
#include <Xm/RowColumn.h>
#include <Xm/Scale.h>
#include <Xm/Separator.h>
#define INT32 __INT32_HIDE
#define BOOL __BOOL_HIDE
#include <Xm/MwmUtil.h>
#undef INT32
#undef BOOL

#include "../widgets/Stepper.h"

#include "ViewControlDialog.h"
#include "ImageWindow.h"
#include "ImageNode.h"
#include "Application.h"
#include "ButtonInterface.h"
#include "ViewControlWhichCameraCommand.h"
#include "List.h"
#include "DictionaryIterator.h"
#include "Network.h"
#include "PickNode.h"
#include "ProbeNode.h"

#include "VCDefaultResources.h"

boolean ViewControlDialog::ClassInitialized = FALSE;

ViewControlDialog::ViewControlDialog(Widget 	  parent,
				     ImageWindow *w):
    Dialog("viewControl", parent)
{

    if (NOT ViewControlDialog::ClassInitialized)
    {
        ViewControlDialog::ClassInitialized = TRUE;
	this->installDefaultResources(theApplication->getRootWidget());
    }
    this->imageWindow = w;

    // Mode pulldown
    this->modeNone = NULL;
    this->modeCamera = NULL;
    this->modeCursors = NULL;
    this->modePick = NULL;
    this->modeNavigate = NULL;
    this->modePanZoom = NULL;
    this->modeRoam = NULL;
    this->modeRotate = NULL;
    this->modeZoom = NULL;

    // View pulldown
    this->setViewNone = NULL;
    this->setViewTop = NULL;
    this->setViewBottom = NULL;
    this->setViewFront = NULL;
    this->setViewBack = NULL;
    this->setViewLeft = NULL;
    this->setViewRight = NULL;
    this->setViewDiagonal = NULL;
    this->setViewOffTop = NULL;
    this->setViewOffBottom = NULL;
    this->setViewOffFront = NULL;
    this->setViewOffBack = NULL;
    this->setViewOffLeft = NULL;
    this->setViewOffRight = NULL;
    this->setViewOffDiagonal = NULL;

    // Projection pulldown
    this->orthographic = NULL;
    this->perspective = NULL;

    // Constraint pulldown
    this->constraintNone = NULL;
    this->constraintX = NULL;
    this->constraintY = NULL;
    this->constraintZ = NULL;

    // Camera pulldown
    this->cameraTo = NULL;
    this->cameraFrom = NULL;
    this->cameraUp = NULL;

    // Navigate pulldown
    this->lookForward = NULL;
    this->lookLeft45 = NULL;
    this->lookRight45 = NULL;
    this->lookUp45 = NULL;
    this->lookDown45 = NULL;
    this->lookLeft90 = NULL;
    this->lookRight90 = NULL;
    this->lookUp90 = NULL;
    this->lookDown90 = NULL;
    this->lookBackward = NULL;
    this->lookAlign = NULL;

    // Main dialog buttons
    this->undoButton = NULL;
    this->redoButton = NULL; 
    this->resetButton = NULL;

    this->cameraToVectorCmd = new ViewControlWhichCameraCommand(
	"cameraToVectorCommand", w->getCommandScope(), TRUE, this);
    this->cameraFromVectorCmd = new ViewControlWhichCameraCommand(
	"cameraFromVectorCommand", w->getCommandScope(), TRUE, this);
    this->cameraUpVectorCmd = new ViewControlWhichCameraCommand(
	"cameraUpVectorCommand", w->getCommandScope(), TRUE, this);
    
    this->cameraFormManaged = 
	this->cursorFormManaged = 
	this->roamFormManaged = 
	this->navigateFormManaged = FALSE;

    this->probeOptionMenu = NUL(Widget);
    this->pickOptionMenu  = NUL(Widget);
}

ViewControlDialog::~ViewControlDialog()
{
    // FIXME:: Holy !@*$ Batman, there's a monster memory leak here.

    //
    // Delete the command interfaces.
    //

    // Mode pulldown
    if (this->modeNone) 		delete this->modeNone;
    if (this->modeCamera) 		delete this->modeCamera;
    if (this->modeCursors) 		delete this->modeCursors;
    if (this->modePick) 		delete this->modePick;
    if (this->modeNavigate) 		delete this->modeNavigate;
    if (this->modePanZoom) 		delete this->modePanZoom;
    if (this->modeRoam) 		delete this->modeRoam;
    if (this->modeRotate) 		delete this->modeRotate;
    if (this->modeZoom) 		delete this->modeZoom;

    // View pulldown
    if (this->setViewNone) 		delete this->setViewNone;
    if (this->setViewTop) 		delete this->setViewTop;
    if (this->setViewBottom) 		delete this->setViewBottom;
    if (this->setViewFront) 		delete this->setViewFront;
    if (this->setViewBack) 		delete this->setViewBack;
    if (this->setViewLeft) 		delete this->setViewLeft;
    if (this->setViewRight) 		delete this->setViewRight;
    if (this->setViewDiagonal) 		delete this->setViewDiagonal;
    if (this->setViewOffTop) 		delete this->setViewOffTop;
    if (this->setViewOffBottom) 	delete this->setViewOffBottom;
    if (this->setViewOffFront) 		delete this->setViewOffFront;
    if (this->setViewOffBack) 		delete this->setViewOffBack;
    if (this->setViewOffLeft) 		delete this->setViewOffLeft;
    if (this->setViewOffRight) 		delete this->setViewOffRight;
    if (this->setViewOffDiagonal) 	delete this->setViewOffDiagonal;

    // Projection pulldown
    if (this->orthographic) 		delete this->orthographic;
    if (this->perspective) 		delete this->perspective;

    // Constraint pulldown
    if (this->constraintNone) 		delete this->constraintNone;
    if (this->constraintX) 		delete this->constraintX;
    if (this->constraintY) 		delete this->constraintY;
    if (this->constraintZ) 		delete this->constraintZ;

    // Camera pulldown
    if (this->cameraTo) 		delete this->cameraTo;
    if (this->cameraFrom) 		delete this->cameraFrom;
    if (this->cameraUp) 		delete this->cameraUp;

    // Navigate pulldown
    if (this->lookForward)		delete this->lookForward;
    if (this->lookLeft45)		delete this->lookLeft45;
    if (this->lookRight45)		delete this->lookRight45;
    if (this->lookUp45)			delete this->lookUp45;
    if (this->lookDown45)		delete this->lookDown45;
    if (this->lookLeft90)		delete this->lookLeft90;
    if (this->lookRight90)		delete this->lookRight90;
    if (this->lookUp90)			delete this->lookUp90;
    if (this->lookDown90)		delete this->lookDown90;
    if (this->lookBackward)		delete this->lookBackward;
    if (this->lookAlign)		delete this->lookAlign;

    //
    // Main dialog buttons
    // Unmanage the buttons prior to deletion to avoid purify problems.
    //
    if (this->undoButton) {
	this->undoButton->unmanage();
	delete this->undoButton;
    }
    if (this->redoButton) {
	this->redoButton->unmanage();
	delete this->redoButton;
    }
    if (this->resetButton) {
	this->resetButton->unmanage();
	delete this->resetButton;
    }

    //
    // Delete the commands
    //
    if (this->cameraToVectorCmd)  	delete this->cameraToVectorCmd;
    if (this->cameraFromVectorCmd)  	delete this->cameraFromVectorCmd;
    if (this->cameraUpVectorCmd)	delete this->cameraUpVectorCmd;
}

Widget ViewControlDialog::createModePulldown(Widget parent)
{
    Widget pulldown = XmCreatePulldownMenu(parent, "modePulldownMenu",
	NULL, 0);
    this->modeNone = new ButtonInterface(pulldown, 
	"modeNone", this->imageWindow->getModeNoneCmd());
    XtVaCreateManagedWidget(
	"modeSeparator", xmSeparatorWidgetClass, pulldown,
	NULL);
    this->modeCamera = new ButtonInterface(pulldown, 
	"modeCamera", this->imageWindow->getModeCameraCmd());
    this->modeCursors = new ButtonInterface(pulldown, 
	"modeCursors", this->imageWindow->getModeCursorsCmd());
    this->modePick = new ButtonInterface(pulldown, 
	"modePick", this->imageWindow->getModePickCmd());
    XtVaCreateManagedWidget(
	"modeSeparator", xmSeparatorWidgetClass, pulldown,
	NULL);
    this->modeNavigate = new ButtonInterface(pulldown, 
	"modeNavigate", this->imageWindow->getModeNavigateCmd());
    this->modePanZoom = new ButtonInterface(pulldown, 
	"modePanZoom", this->imageWindow->getModePanZoomCmd());
    this->modeRoam = new ButtonInterface(pulldown, 
	"modeRoam", this->imageWindow->getModeRoamCmd());
    this->modeRotate = new ButtonInterface(pulldown, 
	"modeRotate", this->imageWindow->getModeRotateCmd());
    this->modeZoom = new ButtonInterface(pulldown, 
	"modeZoom", this->imageWindow->getModeZoomCmd());

    return pulldown;
}

Widget ViewControlDialog::createSetViewPulldown(Widget parent)
{
    Widget pulldown = XmCreatePulldownMenu(parent,
	"setViewPulldownMenu",
	NULL, 0);
    this->setViewNone = new ButtonInterface(pulldown, 
	"setViewNone", this->imageWindow->getSetViewNoneCmd());
    this->setViewTop = new ButtonInterface(pulldown, 
	"setViewTop", this->imageWindow->getSetViewTopCmd());
    this->setViewBottom = new ButtonInterface(pulldown, 
	"setViewBottom", this->imageWindow->getSetViewBottomCmd());
    this->setViewFront = new ButtonInterface(pulldown, 
	"setViewFront", this->imageWindow->getSetViewFrontCmd());
    this->setViewBack = new ButtonInterface(pulldown, 
	"setViewBack", this->imageWindow->getSetViewBackCmd());
    this->setViewLeft = new ButtonInterface(pulldown, 
	"setViewLeft", this->imageWindow->getSetViewLeftCmd());
    this->setViewRight = new ButtonInterface(pulldown, 
	"setViewRight", this->imageWindow->getSetViewRightCmd());
    this->setViewDiagonal = new ButtonInterface(pulldown, 
	"setViewDiagonal", this->imageWindow->getSetViewDiagonalCmd());
    this->setViewOffTop = new ButtonInterface(pulldown, 
	"setViewOffTop", this->imageWindow->getSetViewOffTopCmd());
    this->setViewOffBottom = new ButtonInterface(pulldown, 
	"setViewOffBottom", this->imageWindow->getSetViewOffBottomCmd());
    this->setViewOffFront = new ButtonInterface(pulldown, 
	"setViewOffFront", this->imageWindow->getSetViewOffFrontCmd());
    this->setViewOffBack = new ButtonInterface(pulldown, 
	"setViewOffBack", this->imageWindow->getSetViewOffBackCmd());
    this->setViewOffLeft = new ButtonInterface(pulldown, 
	"setViewOffLeft", this->imageWindow->getSetViewOffLeftCmd());
    this->setViewOffRight = new ButtonInterface(pulldown, 
	"setViewOffRight", this->imageWindow->getSetViewOffRightCmd());
    this->setViewOffDiagonal = new ButtonInterface(pulldown, 
	"setViewOffDiagonal", this->imageWindow->getSetViewOffDiagonalCmd());

    return pulldown;
}

Widget ViewControlDialog::createProjectionPulldown(Widget parent)
{
    Widget pulldown = XmCreatePulldownMenu(parent,
	"projectionPulldownMenu",
	NULL, 0);
    this->orthographic = new ButtonInterface(pulldown, 
	"orthographic", this->imageWindow->getParallelCmd());
    this->perspective = new ButtonInterface(pulldown, 
	"perspective", this->imageWindow->getPerspectiveCmd());

    return pulldown;
}

void ViewControlDialog::createProbePulldown()
{
    DictionaryIterator  di;
    Widget        w;
    Node	 *p;

    if(this->probeWidgetList.getSize() > 0)
    {
	di.setList(this->probeWidgetList);
	while( (w = (Widget)di.getNextDefinition()) )
	    XtDestroyWidget(w);
	this->probeWidgetList.clear();			
    }

    // FALSE = don't include PickNodes which are a subclass of Probe.
    List *l = this->imageWindow->getNetwork()->makeClassifiedNodeList(
							ClassProbeNode, FALSE);
    if (!l) 
    {
	if (!this->probeWidgetList.findDefinition("noProbes")) {
	    w = XtVaCreateManagedWidget("noProbes",
				    xmPushButtonWidgetClass, 
				    this->probePulldown,
        			    NULL);
	    this->probeWidgetList.addDefinition("noProbes",(void*)w);
	}
	
	return;
    }

    ListIterator li;
    li.setList(*l);
    Node *currentProbe = (Node*)this->imageWindow->getCurrentProbeNode();
    Widget menuHistory = NULL;
    while( (p = (Node*)li.getNext()) )
    {
	const char *label = p->getLabelString();
	if (!this->probeWidgetList.findDefinition(label)) {
	    XmString xmlabel = XmStringCreateSimple((char*)label);
	    w = XtVaCreateManagedWidget(p->getLabelString(), 
				    xmPushButtonWidgetClass, 
				    this->probePulldown,
				    XmNlabelString,  	xmlabel,
        			    NULL);
	    XmStringFree(xmlabel);
	    XtAddCallback(w,
		      XmNactivateCallback,
		      (XtCallbackProc)ViewControlDialog_SelectProbeCB,
		      (XtPointer)this);

	    this->probeWidgetList.addDefinition(label,(void*)w);

	    if ((p == currentProbe) || (!menuHistory))
		menuHistory = w;
	}
    }
    delete l;

    // I think the ASSERT belongs, but it's too close to release time.
    //ASSERT(menuHistory);
    if (this->probeOptionMenu && menuHistory)
	XtVaSetValues(this->probeOptionMenu, 
		      XmNmenuHistory, 
		      menuHistory,
		      NULL);
}

void ViewControlDialog::createPickPulldown()
{
    DictionaryIterator  di;
    Widget        w;
    Node	 *p;

    if(this->pickWidgetList.getSize() > 0)
    {
	di.setList(this->pickWidgetList);
	while( (w = (Widget)di.getNextDefinition()) ) {
	    XtDestroyWidget(w);
	}
	this->pickWidgetList.clear();			
    }

    List *l = this->imageWindow->getNetwork()->makeClassifiedNodeList(
							ClassPickNode);
    if(!l)
    {
	if (!this->pickWidgetList.findDefinition("noPicks")) {
	    w = XtVaCreateManagedWidget("noPicks",
				    xmPushButtonWidgetClass, 
				    this->pickPulldown,
        			    NULL);
	    this->pickWidgetList.addDefinition("noPicks",(void*)w);
	}
	
	return;
    }

    Node *currentPick = (Node*)this->imageWindow->getCurrentPickNode();
    Widget menuHistory = NULL;
    ListIterator li;
    li.setList(*l);
    while( (p = (Node*)li.getNext()) )
    {
	const char *label = p->getLabelString();
	if (!this->pickWidgetList.findDefinition(label)) {
	    XmString xmlabel = XmStringCreateSimple((char*)label);
	    w = XtVaCreateManagedWidget(/*"pickButton"*/label,
				    xmPushButtonWidgetClass, 
				    this->pickPulldown,
				    XmNlabelString,  	xmlabel,
        			    NULL);
	    XmStringFree(xmlabel);
	    XtAddCallback(w,
		      XmNactivateCallback,
		      (XtCallbackProc)ViewControlDialog_SelectPickCB,
		      (XtPointer)this);

	    this->pickWidgetList.addDefinition(label,(void*)w); 

	    if ((p == currentPick) || (!menuHistory))
		menuHistory = w;
	}
    }
    delete l;

    if (this->pickOptionMenu && menuHistory)
	XtVaSetValues(this->pickOptionMenu, 
		      XmNmenuHistory, 
		      menuHistory,
		      NULL);
}

Widget ViewControlDialog::createConstraintPulldown(Widget parent)
{
    Widget pulldown = XmCreatePulldownMenu(parent,
	"constraintPulldownMenu",
	NULL, 0);
    this->constraintNone = new ButtonInterface(pulldown, 
	"None", this->imageWindow->getConstrainNoneCmd());

    this->constraintX = new ButtonInterface(pulldown, 
	"X", this->imageWindow->getConstrainXCmd());
    this->constraintY = new ButtonInterface(pulldown, 
	"Y", this->imageWindow->getConstrainYCmd());
    this->constraintZ = new ButtonInterface(pulldown, 
	"Z", this->imageWindow->getConstrainZCmd());
    return pulldown;
}

Widget ViewControlDialog::createCameraWhichPulldown(Widget parent)
{
    Widget pulldown = XmCreatePulldownMenu(parent,
	"cameraWhichPulldownMenu",
	NULL, 0);

    this->cameraTo = new ButtonInterface(pulldown, 
	"to", this->cameraToVectorCmd);
    this->cameraFrom = new ButtonInterface(pulldown, 
	"from", this->cameraFromVectorCmd);
    this->cameraUp = new ButtonInterface(pulldown, 
	"up", this->cameraUpVectorCmd);

    return pulldown;
}

Widget ViewControlDialog::createNavigateLookPulldown(Widget parent)
{
    Widget pulldown = XmCreatePulldownMenu(parent,
	"navigateLookPulldownMenu",
	NULL, 0);

    this->lookForward = new ButtonInterface(pulldown, 
	"forward", this->imageWindow->getLookForwardCmd());
    this->lookLeft45 = new ButtonInterface(pulldown, 
	"left45", this->imageWindow->getLookLeft45Cmd());
    this->lookRight45 = new ButtonInterface(pulldown, 
	"right45", this->imageWindow->getLookRight45Cmd());
    this->lookUp45 = new ButtonInterface(pulldown, 
	"up45", this->imageWindow->getLookUp45Cmd());
    this->lookDown45 = new ButtonInterface(pulldown, 
	"down45", this->imageWindow->getLookDown45Cmd());
    this->lookLeft90 = new ButtonInterface(pulldown, 
	"left90", this->imageWindow->getLookLeft90Cmd());
    this->lookRight90 = new ButtonInterface(pulldown, 
	"right90", this->imageWindow->getLookRight90Cmd());
    this->lookUp90 = new ButtonInterface(pulldown, 
	"up90", this->imageWindow->getLookUp90Cmd());
    this->lookDown90 = new ButtonInterface(pulldown, 
	"down90", this->imageWindow->getLookDown90Cmd());
    this->lookBackward = new ButtonInterface(pulldown, 
	"backward", this->imageWindow->getLookBackwardCmd());
    this->lookAlign = new ButtonInterface(pulldown, 
	"align", this->imageWindow->getLookAlignCmd());

    return pulldown;
}

Widget ViewControlDialog::createDialog(Widget parent)
{
    Arg arg[10];

#define VC_OFFSET 5
    XtSetArg(arg[0], XmNautoUnmanage, False);
//    Widget dialog = XmCreateFormDialog( parent, this->name, arg, 1);
    Widget dialog = this->CreateMainForm( parent, this->name, arg, 1);

#ifdef PASSDOUBLEVALUE
	XtArgVal dx_l1, dx_l2;
#endif

    XtVaSetValues(XtParent(dialog),
	XmNtitle, "View Control...",
	XmNmwmFunctions, MWM_FUNC_MOVE | MWM_FUNC_CLOSE,
	NULL);

    Widget mainForm = this->mainForm = XtVaCreateWidget(
	"mainForm", xmFormWidgetClass, dialog,
//	XmNwidth, VC_WIDTH,
	NULL);

    this->undoButton = new ButtonInterface(mainForm,
	  "undoButton", this->imageWindow->getUndoCmd());
    XtVaSetValues(this->undoButton->getRootWidget(),
	XmNtopAttachment  , XmATTACH_FORM,
	XmNtopOffset      , VC_OFFSET,
	XmNleftAttachment , XmATTACH_FORM,
	XmNleftOffset     , 2*VC_OFFSET,
	XmNrightAttachment, XmATTACH_NONE,
//	XmNwidth          , 100,
	XmNrecomputeSize  , False,
	NULL);

    this->redoButton = new ButtonInterface(mainForm,
	  "redoButton", this->imageWindow->getRedoCmd());
    XtVaSetValues(this->redoButton->getRootWidget(),
	XmNtopAttachment  , XmATTACH_FORM,
	XmNtopOffset      , VC_OFFSET,
	XmNleftAttachment , XmATTACH_NONE,
	XmNrightAttachment, XmATTACH_FORM,
	XmNrightOffset    , 2*VC_OFFSET,
//	XmNwidth          , 100,
	XmNrecomputeSize  , False,
	NULL);

    Widget separator1 = XtVaCreateManagedWidget(
	"separator1", xmSeparatorWidgetClass, mainForm,
	XmNtopAttachment   , XmATTACH_WIDGET,
	XmNtopWidget       , undoButton->getRootWidget(),
	XmNtopOffset       , VC_OFFSET,
	XmNleftAttachment  , XmATTACH_FORM,
	XmNleftOffset      , VC_OFFSET,
	XmNrightAttachment , XmATTACH_FORM,
	XmNrightOffset     , VC_OFFSET,
	NULL);

    Widget modePulldown = this->createModePulldown(mainForm);
    Widget modeOptionMenu = this->modeOptionMenu = XtVaCreateManagedWidget(
	"modeOptionMenu", xmRowColumnWidgetClass, mainForm,
	XmNtopAttachment   , XmATTACH_WIDGET,
	XmNtopWidget       , separator1,
	XmNtopOffset       , VC_OFFSET,
	XmNrightAttachment , XmATTACH_FORM,
	XmNrightOffset     , VC_OFFSET,
	XmNrowColumnType   , XmMENU_OPTION,
	XmNsubMenuId       , modePulldown,
	NULL);
    this->installComponentHelpCallback(modeOptionMenu);


    Widget modeLabel = XtVaCreateManagedWidget(
	"modeLabel", xmLabelWidgetClass, mainForm,
	XmNtopAttachment  , XmATTACH_OPPOSITE_WIDGET,
	XmNtopWidget      , modeOptionMenu,
	XmNtopOffset      , 3,
	XmNleftAttachment , XmATTACH_FORM,
	XmNleftOffset     , VC_OFFSET,
	NULL);

    Widget separator2 = XtVaCreateManagedWidget(
	"separator2", xmSeparatorWidgetClass, mainForm,
	XmNtopAttachment   , XmATTACH_WIDGET,
	XmNtopWidget       , modeOptionMenu,
	XmNtopOffset       , VC_OFFSET,
	XmNleftAttachment  , XmATTACH_FORM,
	XmNleftOffset      , VC_OFFSET,
	XmNrightAttachment , XmATTACH_FORM,
	XmNrightOffset     , VC_OFFSET,
	NULL);

    Widget setViewPulldown = this->createSetViewPulldown(mainForm);
    Widget setViewOptionMenu = this->setViewOptionMenu= XtVaCreateManagedWidget(
	"setViewOptionMenu", xmRowColumnWidgetClass, mainForm,
	XmNtopAttachment   , XmATTACH_WIDGET,
	XmNtopWidget       , separator2,
	XmNtopOffset       , VC_OFFSET,
	XmNrightAttachment , XmATTACH_FORM,
	XmNrightOffset     , VC_OFFSET,
	XmNrowColumnType   , XmMENU_OPTION,
	XmNsubMenuId       , setViewPulldown,
	XmNentryAlignment  , XmALIGNMENT_CENTER,
	NULL);
    this->installComponentHelpCallback(setViewOptionMenu);

    Widget setViewLabel = XtVaCreateManagedWidget(
	"setViewLabel", xmLabelWidgetClass, mainForm,
	XmNtopAttachment  , XmATTACH_OPPOSITE_WIDGET,
	XmNtopWidget      , setViewOptionMenu,
	XmNtopOffset      , 3,
	XmNleftAttachment , XmATTACH_FORM,
	XmNleftOffset     , VC_OFFSET,
	NULL);

    Widget separator3 = XtVaCreateManagedWidget(
	"separator3", xmSeparatorWidgetClass, mainForm,
	XmNtopAttachment   , XmATTACH_WIDGET,
	XmNtopWidget       , setViewOptionMenu,
	XmNtopOffset       , VC_OFFSET,
	XmNleftAttachment  , XmATTACH_FORM,
	XmNleftOffset      , VC_OFFSET,
	XmNrightAttachment , XmATTACH_FORM,
	XmNrightOffset     , VC_OFFSET,
	NULL);

    Widget projectionLabel = XtVaCreateManagedWidget(
	"projectionLabel", xmLabelWidgetClass, mainForm,
	XmNtopAttachment  , XmATTACH_WIDGET,
	XmNtopWidget      , separator3,
	XmNtopOffset      , VC_OFFSET + 3,
	XmNleftAttachment , XmATTACH_FORM,
	XmNleftOffset     , VC_OFFSET,
	NULL);

    Widget projectionPulldown = this->createProjectionPulldown(mainForm);
    Widget projectionOptionMenu = this->projectionOptionMenu = XtVaCreateManagedWidget(
	"projectionMenu", xmRowColumnWidgetClass, mainForm,
	XmNtopAttachment   , XmATTACH_WIDGET,
	XmNtopWidget       , separator3,
	XmNtopOffset       , VC_OFFSET,
	XmNrightAttachment , XmATTACH_FORM,
	XmNrightOffset     , VC_OFFSET,
//	XmNleftAttachment  , XmATTACH_WIDGET,
//	XmNleftWidget      , projectionLabel,
//	XmNleftOffset      , 5*VC_OFFSET,
	XmNrowColumnType   , XmMENU_OPTION,
	XmNsubMenuId       , projectionPulldown,
	XmNentryAlignment  , XmALIGNMENT_CENTER,
	NULL);
    this->installComponentHelpCallback(projectionOptionMenu);

    Widget viewAngleLabel = XtVaCreateManagedWidget(
	"viewAngleLabel", xmLabelWidgetClass, mainForm,
	XmNtopAttachment  , XmATTACH_WIDGET,
	XmNtopWidget      , projectionOptionMenu,
	XmNtopOffset      , VC_OFFSET,
	XmNleftAttachment , XmATTACH_FORM,
	XmNleftOffset     , VC_OFFSET,
	NULL);
    double vamin = 0.001;
    double vamax = 179.0;
    Widget viewAngleStepper = this->viewAngleStepper = XtVaCreateManagedWidget(
	"viewAngleStepper", xmStepperWidgetClass, mainForm,
	XmNtopAttachment   , XmATTACH_WIDGET,
	XmNtopWidget       , projectionOptionMenu,
	XmNtopOffset       , VC_OFFSET,
	XmNrightAttachment , XmATTACH_FORM,
	XmNrightOffset     , VC_OFFSET,
	XmNrecomputeSize   , False,
	XmNdataType        , DOUBLE,
	XmNdigits          , 6,
	XmNdecimalPlaces   , 3,
	XmNdMinimum        , DoubleVal(vamin, dx_l1),
	XmNdMaximum        , DoubleVal(vamax, dx_l2),
	XmNeditable        , True,
	NULL);
    // FIXME: Warning and activate callbacks for above.
    XtAddCallback(viewAngleStepper, XmNactivateCallback,
	(XtCallbackProc)ViewControlDialog_NumberCB, (XtPointer)this);
    this->installComponentHelpCallback(viewAngleStepper);

    //
    // Widgets for the Cursor Form.
    //
    Widget cursorForm = this->cursorForm = XtVaCreateWidget(
	"cursorForm", xmFormWidgetClass, dialog,
	XmNtopAttachment   , XmATTACH_WIDGET,
	XmNtopWidget       , mainForm,
	XmNleftAttachment  , XmATTACH_OPPOSITE_WIDGET,
	XmNleftWidget      , mainForm,
	XmNrightAttachment , XmATTACH_OPPOSITE_WIDGET,
	XmNrightWidget     , mainForm,
	XmNresizePolicy    , XmRESIZE_ANY,
	XmNallowResize     , True,
	NULL);

    this->probeSeparator = XtVaCreateManagedWidget(
       "separator5", xmSeparatorWidgetClass, cursorForm,
	XmNtopAttachment   , XmATTACH_FORM,
	XmNtopOffset       , VC_OFFSET,
	XmNleftAttachment  , XmATTACH_FORM,
	XmNleftOffset      , VC_OFFSET,
	XmNrightAttachment , XmATTACH_FORM,
	XmNrightOffset     , VC_OFFSET,
	NULL);

    this->probeLabel = XtVaCreateManagedWidget(
       "probeLabel", xmLabelWidgetClass, cursorForm,
	XmNtopAttachment  , XmATTACH_WIDGET,
	XmNtopWidget      , this->probeSeparator,
	XmNtopOffset      , VC_OFFSET + 3,
	XmNleftAttachment , XmATTACH_FORM,
	XmNleftOffset     , VC_OFFSET,
	XmNrecomputeSize  , False,
	NULL);


    this->probePulldown = XmCreatePulldownMenu(this->cursorForm,
						  "probePulldownMenu",
						  NULL, 
						  0);
    this->createProbePulldown();

    this->probeOptionMenu = XtVaCreateManagedWidget(
	"probeMenu", xmRowColumnWidgetClass, cursorForm,
	XmNtopAttachment   , XmATTACH_WIDGET,
	XmNtopWidget       , this->probeSeparator,
	XmNtopOffset       , VC_OFFSET,
	XmNrightAttachment , XmATTACH_FORM,
	XmNrightOffset     , VC_OFFSET,
	XmNentryAlignment  , XmALIGNMENT_CENTER,
	XmNrowColumnType   , XmMENU_OPTION,
	XmNsubMenuId       , this->probePulldown,
	XmNrecomputeSize   , False,
	NULL);
    this->installComponentHelpCallback(this->probeOptionMenu);

    this->constraintLabel = XtVaCreateManagedWidget(
       "constraintLabel", xmLabelWidgetClass, cursorForm,
	XmNtopAttachment  , XmATTACH_WIDGET,
	XmNtopWidget      , this->probeOptionMenu,
	XmNtopOffset      , VC_OFFSET + 3,
	XmNleftAttachment , XmATTACH_FORM,
	XmNleftOffset     , VC_OFFSET,
//	XmNbottomAttachment , XmATTACH_FORM,
	XmNrecomputeSize  , False,
	NULL);

    Widget constraintPulldown = this->createConstraintPulldown(cursorForm);
    this->constraintOptionMenu = XtVaCreateManagedWidget(
	"constraintMenu", xmRowColumnWidgetClass, cursorForm,
	XmNtopAttachment   , XmATTACH_WIDGET,
	XmNtopWidget       , this->probeOptionMenu,
	XmNtopOffset       , VC_OFFSET,
	XmNrightAttachment , XmATTACH_FORM,
	XmNrightOffset     , VC_OFFSET,
	XmNrowColumnType   , XmMENU_OPTION,
	XmNsubMenuId       , constraintPulldown,
	XmNbottomAttachment, XmATTACH_FORM,
	XmNrecomputeSize   , False,
	NULL);
    this->installComponentHelpCallback(this->constraintOptionMenu);

    //
    // Widgets for the Pick Form.
    //
    Widget pickForm = this->pickForm = XtVaCreateWidget(
	"pickForm", xmFormWidgetClass, dialog,
	XmNtopAttachment   , XmATTACH_WIDGET,
	XmNtopWidget       , mainForm,
	XmNleftAttachment  , XmATTACH_OPPOSITE_WIDGET,
	XmNleftWidget      , mainForm,
	XmNrightAttachment , XmATTACH_OPPOSITE_WIDGET,
	XmNrightWidget     , mainForm,
	NULL);
    this->installComponentHelpCallback(pickForm);

    Widget separator6 = XtVaCreateManagedWidget(
       "separator6", xmSeparatorWidgetClass, pickForm,
	XmNtopAttachment   , XmATTACH_FORM,
	XmNtopOffset       , VC_OFFSET,
	XmNleftAttachment  , XmATTACH_FORM,
	XmNleftOffset      , VC_OFFSET,
	XmNrightAttachment , XmATTACH_FORM,
	XmNrightOffset     , VC_OFFSET,
	NULL);

    Widget pickLabel = XtVaCreateManagedWidget(
       "pickLabel", xmLabelWidgetClass, pickForm,
	XmNtopAttachment  , XmATTACH_WIDGET,
	XmNtopWidget      , separator6,
	XmNtopOffset      , VC_OFFSET + 3,
	XmNleftAttachment , XmATTACH_FORM,
	XmNleftOffset     , VC_OFFSET,
	XmNrecomputeSize  , False,
	NULL);


    this->pickPulldown = XmCreatePulldownMenu(this->pickForm,
						  "pickPulldownMenu",
						  NULL, 
						  0);
    this->createPickPulldown();

    Widget pickOptionMenu = this->pickOptionMenu = XtVaCreateManagedWidget(
	"pickMenu", xmRowColumnWidgetClass, pickForm,
	XmNtopAttachment   , XmATTACH_WIDGET,
	XmNtopWidget       , separator6,
	XmNtopOffset       , VC_OFFSET,
	XmNrightAttachment , XmATTACH_FORM,
	XmNrightOffset     , VC_OFFSET,
	XmNentryAlignment  , XmALIGNMENT_CENTER,
	XmNrowColumnType   , XmMENU_OPTION,
	XmNsubMenuId       , this->pickPulldown,
	NULL);
    this->installComponentHelpCallback(pickOptionMenu);

    //
    // Widgets for the Camera Form.
    //
    Widget cameraForm = this->cameraForm = XtVaCreateWidget(
	"cameraForm", xmFormWidgetClass, dialog,
	XmNtopAttachment   , XmATTACH_WIDGET,
	XmNtopWidget       , mainForm,
	XmNleftAttachment  , XmATTACH_OPPOSITE_WIDGET,
	XmNleftWidget      , mainForm,
	XmNrightAttachment , XmATTACH_OPPOSITE_WIDGET,
	XmNrightWidget     , mainForm,
	NULL);
    this->installComponentHelpCallback(cameraForm);

    Widget cameraSep0 = XtVaCreateManagedWidget(
	"cameraSep0", xmSeparatorWidgetClass, cameraForm,
	XmNtopAttachment   , XmATTACH_FORM,
	XmNtopOffset       , VC_OFFSET,
	XmNleftAttachment  , XmATTACH_FORM,
	XmNleftOffset      , VC_OFFSET,
	XmNrightAttachment , XmATTACH_FORM,
	XmNrightOffset     , VC_OFFSET,
	NULL);

    Widget cameraXNumber = this->cameraXNumber = XtVaCreateManagedWidget(
       "cameraXNumber", xmNumberWidgetClass, cameraForm,
	XmNtopAttachment  , XmATTACH_WIDGET,
	XmNtopWidget      , cameraSep0,
	XmNtopOffset      , VC_OFFSET,
	XmNrightAttachment , XmATTACH_FORM,
	XmNrightOffset     , VC_OFFSET,
	XmNdataType        , DOUBLE,
	XmNdecimalPlaces   , 5,
	XmNcharPlaces      , 9,
	XmNeditable        , True,
	XmNfixedNotation   , False,
	NULL);
    Widget cameraYNumber = this->cameraYNumber = XtVaCreateManagedWidget(
       "cameraYNumber", xmNumberWidgetClass, cameraForm,
	XmNtopAttachment  , XmATTACH_WIDGET,
	XmNtopWidget      , cameraXNumber,
	XmNtopOffset      , VC_OFFSET,
	XmNrightAttachment , XmATTACH_FORM,
	XmNrightOffset     , VC_OFFSET,
	XmNdataType        , DOUBLE,
	XmNdecimalPlaces   , 5,
	XmNcharPlaces      , 7,
	XmNeditable        , True,
	XmNfixedNotation   , False,
	NULL);
    Widget cameraZNumber = this->cameraZNumber = XtVaCreateManagedWidget(
       "cameraZNumber", xmNumberWidgetClass, cameraForm,
	XmNtopAttachment  , XmATTACH_WIDGET,
	XmNtopWidget      , cameraYNumber,
	XmNtopOffset      , VC_OFFSET,
	XmNrightAttachment , XmATTACH_FORM,
	XmNrightOffset     , VC_OFFSET,
	XmNdataType        , DOUBLE,
	XmNdecimalPlaces   , 5,
	XmNcharPlaces      , 7,
	XmNeditable        , True,
	XmNfixedNotation   , False,
	NULL);
    XtAddCallback(cameraXNumber, XmNactivateCallback,
	(XtCallbackProc)ViewControlDialog_NumberCB, (XtPointer)this);
    XtAddCallback(cameraYNumber, XmNactivateCallback,
	(XtCallbackProc)ViewControlDialog_NumberCB, (XtPointer)this);
    XtAddCallback(cameraZNumber, XmNactivateCallback,
	(XtCallbackProc)ViewControlDialog_NumberCB, (XtPointer)this);

    Widget cameraXLabel = XtVaCreateManagedWidget(
       "cameraXLabel", xmLabelWidgetClass, cameraForm,
	XmNtopAttachment  , XmATTACH_WIDGET,
	XmNtopWidget      , cameraSep0,
	XmNtopOffset      , VC_OFFSET,
	XmNrightAttachment , XmATTACH_WIDGET,
	XmNrightWidget    , cameraXNumber,
	NULL);
    Widget cameraYLabel = XtVaCreateManagedWidget(
       "cameraYLabel", xmLabelWidgetClass, cameraForm,
	XmNtopAttachment  , XmATTACH_WIDGET,
	XmNtopWidget      , cameraXLabel,
	XmNtopOffset      , VC_OFFSET,
	XmNrightAttachment , XmATTACH_WIDGET,
	XmNrightWidget    , cameraYNumber,
	NULL);
    Widget cameraZLabel = XtVaCreateManagedWidget(
       "cameraZLabel", xmLabelWidgetClass, cameraForm,
	XmNtopAttachment  , XmATTACH_WIDGET,
	XmNtopWidget      , cameraYLabel,
	XmNtopOffset      , VC_OFFSET,
	XmNrightAttachment , XmATTACH_WIDGET,
	XmNrightWidget    , cameraZNumber,
	NULL);

    Widget cameraWhichPulldown = this->createCameraWhichPulldown(cameraForm);
    Widget cameraWhichOptionMenu = this->cameraWhichOptionMenu = XtVaCreateManagedWidget(
	"cameraWhichMenu", xmRowColumnWidgetClass, cameraForm,
	XmNtopAttachment   , XmATTACH_WIDGET,
	XmNtopWidget       , cameraXLabel,
	XmNtopOffset       , VC_OFFSET,
	XmNleftAttachment  , XmATTACH_FORM,
	XmNleftOffset      , VC_OFFSET,
	XmNrightAttachment , XmATTACH_WIDGET,
	XmNrightWidget     , cameraYLabel,
	XmNrightOffset     , VC_OFFSET,
	XmNrowColumnType   , XmMENU_OPTION,
	XmNsubMenuId       , cameraWhichPulldown,
	NULL);
    this->installComponentHelpCallback(cameraWhichOptionMenu);

    Widget cameraSep1 = XtVaCreateManagedWidget(
	"cameraSep1", xmSeparatorWidgetClass, cameraForm,
	XmNtopAttachment   , XmATTACH_WIDGET,
	XmNtopWidget       , cameraZNumber,
	XmNtopOffset       , VC_OFFSET,
	XmNleftAttachment  , XmATTACH_FORM,
	XmNleftOffset      , VC_OFFSET,
	XmNrightAttachment , XmATTACH_FORM,
	XmNrightOffset     , VC_OFFSET,
	NULL);

    Widget cameraWindowWidthLabel = XtVaCreateManagedWidget(
       "cameraWindowWidthLabel", xmLabelWidgetClass, cameraForm,
	XmNtopAttachment  , XmATTACH_WIDGET,
	XmNtopWidget      , cameraSep1,
	XmNtopOffset      , VC_OFFSET,
	XmNleftAttachment , XmATTACH_FORM,
	XmNleftOffset     , VC_OFFSET,
	NULL);
    Widget cameraWindowHeightLabel = XtVaCreateManagedWidget(
       "cameraWindowHeightLabel", xmLabelWidgetClass, cameraForm,
	XmNtopAttachment  , XmATTACH_WIDGET,
	XmNtopWidget      , cameraWindowWidthLabel,
	XmNtopOffset      , VC_OFFSET,
	XmNleftAttachment , XmATTACH_FORM,
	XmNleftOffset     , VC_OFFSET,
	NULL);

    Widget cameraWindowWidthNumber = this->cameraWindowWidthNumber = 
	XtVaCreateManagedWidget(
       "cameraWindowWidthNumber", xmNumberWidgetClass, cameraForm,
	XmNtopAttachment   , XmATTACH_WIDGET,
	XmNtopWidget       , cameraSep1,
	XmNtopOffset       , VC_OFFSET,
	XmNrightAttachment , XmATTACH_FORM,
	XmNrightOffset     , VC_OFFSET,
	XmNdataType        , INTEGER,
	XmNiValue          , 0,
	XmNiMinimum        , 1,
//	XmNiMaximum        , 4096,
	XmNdecimalPlaces   , 0,
	XmNcharPlaces      , 9,
	XmNrecomputeSize   , False,
	XmNeditable        , True,
	NULL);
    Widget cameraWindowHeightNumber = this->cameraWindowHeightNumber =
	XtVaCreateManagedWidget(
       "cameraWindowHeightNumber", xmNumberWidgetClass, cameraForm,
	XmNtopAttachment   , XmATTACH_WIDGET,
	XmNtopWidget       , cameraWindowWidthNumber,
	XmNtopOffset       , VC_OFFSET,
	XmNrightAttachment , XmATTACH_FORM,
	XmNrightOffset     , VC_OFFSET,
	XmNdataType        , INTEGER,
	XmNiValue          , 0,
	XmNiMinimum        , 1,
//	XmNiMaximum        , 4096,
	XmNdecimalPlaces   , 0,
	XmNcharPlaces      , 9,
	XmNrecomputeSize   , False,
	XmNeditable        , True,
	NULL);
    XtAddCallback(cameraWindowHeightNumber, XmNactivateCallback,
	(XtCallbackProc)ViewControlDialog_NumberCB, (XtPointer)this);
    XtAddCallback(cameraWindowWidthNumber, XmNactivateCallback,
	(XtCallbackProc)ViewControlDialog_NumberCB, (XtPointer)this);

    Widget cameraSep2 = XtVaCreateManagedWidget(
	"cameraSep2", xmSeparatorWidgetClass, cameraForm,
	XmNtopAttachment   , XmATTACH_WIDGET,
	XmNtopWidget       , cameraWindowHeightNumber,
	XmNtopOffset       , VC_OFFSET,
	XmNleftAttachment  , XmATTACH_FORM,
	XmNleftOffset      , VC_OFFSET,
	XmNrightAttachment , XmATTACH_FORM,
	XmNrightOffset     , VC_OFFSET,
	NULL);

    Widget cameraWidthLabel = XtVaCreateManagedWidget(
       "cameraWidthLabel", xmLabelWidgetClass, cameraForm,
	XmNtopAttachment  , XmATTACH_WIDGET,
	XmNtopWidget      , cameraSep2,
	XmNtopOffset      , VC_OFFSET,
	XmNleftAttachment , XmATTACH_FORM,
	XmNleftOffset     , VC_OFFSET,
	NULL);
    Widget cameraWidthNumber = this->cameraWidthNumber =XtVaCreateManagedWidget(
       "cameraWidthNumber", xmNumberWidgetClass, cameraForm,
	XmNtopAttachment   , XmATTACH_WIDGET,
	XmNtopWidget       , cameraSep2,
	XmNtopOffset       , VC_OFFSET,
	XmNrightAttachment , XmATTACH_FORM,
	XmNrightOffset     , VC_OFFSET,
	XmNdataType        , DOUBLE,
	XmNdecimalPlaces   , 5,
	XmNcharPlaces      , 7,
	XmNeditable        , True,
	XmNfixedNotation   , False,
	NULL);
    XtAddCallback(cameraWidthNumber, XmNactivateCallback,
	(XtCallbackProc)ViewControlDialog_NumberCB, (XtPointer)this);

    //
    // Widgets for the Navigate form
    //
    Widget navigateForm = this->navigateForm = XtVaCreateWidget(
	"navigateForm", xmFormWidgetClass, dialog,
	XmNtopAttachment   , XmATTACH_WIDGET,
	XmNtopWidget       , mainForm,
	XmNleftAttachment  , XmATTACH_OPPOSITE_WIDGET,
	XmNleftWidget      , mainForm,
	XmNrightAttachment , XmATTACH_OPPOSITE_WIDGET,
	XmNrightWidget     , mainForm,
	NULL);
    this->installComponentHelpCallback(navigateForm);

    Widget navigateSep0 = XtVaCreateManagedWidget(
	"navigateSep0", xmSeparatorWidgetClass, navigateForm,
	XmNtopAttachment   , XmATTACH_FORM,
	XmNtopOffset       , VC_OFFSET,
	XmNleftAttachment  , XmATTACH_FORM,
	XmNleftOffset      , VC_OFFSET,
	XmNrightAttachment , XmATTACH_FORM,
	XmNrightOffset     , VC_OFFSET,
	NULL);

    Widget navigationLabel = XtVaCreateManagedWidget(
	"navigationLabel", xmLabelWidgetClass, navigateForm,
        XmNtopAttachment   , XmATTACH_WIDGET,
        XmNtopWidget       , navigateSep0,
        XmNtopOffset       , VC_OFFSET,
        XmNleftAttachment  , XmATTACH_FORM,
        XmNleftOffset      , VC_OFFSET,
        XmNrightAttachment , XmATTACH_FORM,
        XmNrightOffset     , VC_OFFSET,
	NULL);

    Widget navigateLookPulldown = this->createNavigateLookPulldown(navigateForm);
    Widget navigateLookOptionMenu = this->navigateLookOptionMenu = XtVaCreateManagedWidget(
	"navigateLookMenu", xmRowColumnWidgetClass, navigateForm,
	XmNtopAttachment   , XmATTACH_WIDGET,
	XmNtopWidget       , navigationLabel,
	XmNtopOffset       , VC_OFFSET,
	XmNrightAttachment , XmATTACH_FORM,
	XmNrightOffset     , VC_OFFSET,
	XmNentryAlignment  , XmALIGNMENT_CENTER,
	XmNrowColumnType   , XmMENU_OPTION,
	XmNsubMenuId       , navigateLookPulldown,
	NULL);

    Widget lookLabel = XtVaCreateManagedWidget(
	"lookLabel", xmLabelWidgetClass, navigateForm,
        XmNtopAttachment  , XmATTACH_OPPOSITE_WIDGET,
        XmNtopWidget      , navigateLookOptionMenu,
        XmNtopOffset      , 3,
        XmNleftAttachment , XmATTACH_FORM,
        XmNleftOffset     , VC_OFFSET,
	NULL);

    Widget motionScale = this->motionScale = XtVaCreateManagedWidget(
	"motionScale", xmScaleWidgetClass, navigateForm,
        XmNtopAttachment   , XmATTACH_WIDGET,
        XmNtopWidget       , navigateLookOptionMenu,
        XmNtopOffset       , VC_OFFSET,
        XmNrightAttachment , XmATTACH_FORM,
        XmNrightOffset     , VC_OFFSET,
        XmNwidth           , 100,
        XmNorientation     , XmHORIZONTAL,
        XmNshowValue       , True,
	XmNvalue           , (int)this->imageWindow->getTranslateSpeed(),
	NULL);
    XtAddCallback(motionScale, XmNvalueChangedCallback,
	(XtCallbackProc)ViewControlDialog_ScaleCB, (XtPointer)this);

    Widget motionLabel = XtVaCreateManagedWidget(
	"motionLabel", xmLabelWidgetClass, navigateForm,
        XmNtopAttachment  , XmATTACH_OPPOSITE_WIDGET,
        XmNtopWidget      , motionScale,
        XmNtopOffset      , VC_OFFSET + 3,
        XmNleftAttachment , XmATTACH_FORM,
        XmNleftOffset     , VC_OFFSET,
	NULL);

    Widget pivotScale = this->pivotScale = XtVaCreateManagedWidget(
	"pivotScale", xmScaleWidgetClass, navigateForm,
        XmNtopAttachment   , XmATTACH_WIDGET,
        XmNtopWidget       , motionScale,
        XmNtopOffset       , VC_OFFSET,
        XmNrightAttachment , XmATTACH_FORM,
        XmNrightOffset     , VC_OFFSET,
        XmNwidth           , 100,
        XmNorientation     , XmHORIZONTAL,
        XmNshowValue       , True,
	XmNvalue           , (int)this->imageWindow->getRotateSpeed(),
	NULL);
    XtAddCallback(pivotScale, XmNvalueChangedCallback,
	(XtCallbackProc)ViewControlDialog_ScaleCB, (XtPointer)this);

    Widget pivotLabel = XtVaCreateManagedWidget(
	"pivotLabel", xmLabelWidgetClass, navigateForm,
        XmNtopAttachment   , XmATTACH_OPPOSITE_WIDGET,
        XmNtopWidget       , pivotScale,
        XmNtopOffset       , VC_OFFSET + 3,
        XmNleftAttachment  , XmATTACH_FORM,
        XmNleftOffset      , VC_OFFSET,
	NULL);


    //
    // Widgets for the Button form
    //
    Widget buttonForm = this->buttonForm = XtVaCreateManagedWidget(
	"buttonForm", xmFormWidgetClass, dialog,
	XmNtopAttachment   , XmATTACH_WIDGET,
	XmNtopWidget       , mainForm,
	XmNleftAttachment  , XmATTACH_OPPOSITE_WIDGET,
	XmNleftWidget      , mainForm,
	XmNrightAttachment , XmATTACH_OPPOSITE_WIDGET,
	XmNrightWidget     , mainForm,
	NULL);

    Widget separator4 = XtVaCreateManagedWidget(
	"separator4", xmSeparatorWidgetClass, buttonForm,
	XmNtopAttachment   , XmATTACH_FORM,
	XmNtopOffset       , VC_OFFSET,
	XmNleftAttachment  , XmATTACH_FORM,
	XmNleftOffset      , VC_OFFSET,
	XmNrightAttachment , XmATTACH_FORM,
	XmNrightOffset     , VC_OFFSET,
	NULL);

    this->cancel =  XtVaCreateManagedWidget(
	"closeButton", xmPushButtonWidgetClass, buttonForm,
	XmNtopAttachment  , XmATTACH_WIDGET,
	XmNtopWidget      , separator4,
	XmNtopOffset      , VC_OFFSET,
	XmNleftAttachment , XmATTACH_FORM,
	XmNleftOffset     , 2*VC_OFFSET,
	XmNbottomAttachment, XmATTACH_FORM,
	XmNbottomOffset   , VC_OFFSET,
	XmNrecomputeSize  , False,
	NULL);
    this->resetButton =  new ButtonInterface(buttonForm,
		"resetButton", this->imageWindow->getResetCmd());
    XtVaSetValues(this->resetButton->getRootWidget(),
	XmNtopAttachment   , XmATTACH_WIDGET,
	XmNtopWidget       , separator4,
	XmNtopOffset       , VC_OFFSET,
	XmNrightAttachment , XmATTACH_FORM,
	XmNrightOffset     , 2*VC_OFFSET,
	XmNbottomAttachment, XmATTACH_FORM,
	XmNbottomOffset   , VC_OFFSET,
	XmNrecomputeSize   , False,
	NULL);
	
    XtManageChild(mainForm);

    return dialog;    
}

void ViewControlDialog::manageCursorForm()
{
    Dimension height;

    this->createProbePulldown();

    XtManageChild(this->cursorForm);

    XtVaGetValues(this->constraintOptionMenu,
	XmNheight, &height, 
	NULL);

    Widget list[2];
    list[0] = this->probeLabel;
    list[1] = this->probeOptionMenu;
    XtManageChildren(list, 2);

    XtVaSetValues(this->constraintLabel,
	XmNtopWidget, this->probeOptionMenu, NULL);
    XtVaSetValues(this->constraintOptionMenu,
	XmNtopWidget, this->probeOptionMenu, NULL);

    XtVaSetValues(this->constraintOptionMenu,
	XmNheight, &height, 
	NULL);

    XtVaSetValues(this->buttonForm, XmNtopWidget, this->cursorForm, NULL);

    this->cursorFormManaged = TRUE;
}
void ViewControlDialog::unmanageCursorForm()
{
    XtUnmanageChild(this->cursorForm);
    if(!this->isExpanding())
    	XtVaSetValues(this->buttonForm, XmNtopWidget, this->mainForm, NULL);
    this->cursorFormManaged = FALSE;
}
void ViewControlDialog::manageRoamForm()
{
    Widget list[2];
    list[0] = this->probeLabel;
    list[1] = this->probeOptionMenu;
    XtUnmanageChildren(list, 2);

    XtVaSetValues(this->constraintLabel,
	XmNtopWidget, this->probeSeparator, NULL);
    XtVaSetValues(this->constraintOptionMenu,
	XmNtopWidget, this->probeSeparator, NULL);

    Position y;
    Dimension height;
    XtVaGetValues(this->constraintOptionMenu,
	XmNy, &y,
	XmNheight, &height, 
	NULL);

    XtManageChild(this->cursorForm);
    XtVaSetValues(this->buttonForm, XmNtopWidget, this->cursorForm, NULL);

    height += y;
    XtVaSetValues(this->cursorForm, XmNheight, height, NULL);

    this->roamFormManaged = TRUE;
}
void ViewControlDialog::unmanageRoamForm()
{
    XtUnmanageChild(this->cursorForm);
    if(!this->isExpanding())
    	XtVaSetValues(this->buttonForm, XmNtopWidget, this->mainForm, NULL);

    this->roamFormManaged = FALSE;
}
void ViewControlDialog::managePickForm()
{
    this->createPickPulldown();

    XtManageChild(this->pickForm);
    XtVaSetValues(this->buttonForm, XmNtopWidget, this->pickForm, NULL);
    this->pickFormManaged = TRUE;
}
void ViewControlDialog::unmanagePickForm()
{
    XtUnmanageChild(this->pickForm);
    if(!this->isExpanding())
    	XtVaSetValues(this->buttonForm, XmNtopWidget, this->mainForm, NULL);
    this->pickFormManaged = FALSE;
}

void ViewControlDialog::manageCameraForm()
{
    XtManageChild(this->cameraForm);
    XtVaSetValues(this->buttonForm, XmNtopWidget, this->cameraForm, NULL);
    this->cameraFormManaged = TRUE;
    this->setWhichCameraVector();
}
void ViewControlDialog::unmanageCameraForm()
{
    XtUnmanageChild(this->cameraForm);
    if(!this->isExpanding())
    	XtVaSetValues(this->buttonForm, XmNtopWidget, this->mainForm, NULL);
    this->cameraFormManaged = FALSE;
}
void ViewControlDialog::manageNavigationForm()
{
    XtManageChild(this->navigateForm);
    XtVaSetValues(this->buttonForm, XmNtopWidget, this->navigateForm, NULL);
    this->navigateFormManaged = TRUE;
}
void ViewControlDialog::unmanageNavigationForm()
{
    XtUnmanageChild(this->navigateForm);
    if(!this->isExpanding())
    	XtVaSetValues(this->buttonForm, XmNtopWidget, this->mainForm, NULL);
    this->navigateFormManaged = FALSE;
}

void ViewControlDialog::manage()
{
    Dimension dialogWidth;
    if (!XtIsRealized(this->getRootWidget()))
	XtRealizeWidget(this->getRootWidget());
    XtVaGetValues(this->getRootWidget(),
	XmNwidth, &dialogWidth,
	NULL);

    Position x;
    Position y;
    Dimension width;
    XtVaGetValues(parent,
	XmNx, &x,
	XmNy, &y,
	XmNwidth, &width,
	NULL);

    if (x > dialogWidth + 25)
	x -= dialogWidth + 25;
    else
	x += width + 25;

    Display *dpy = XtDisplay(this->getRootWidget());
    if (x > WidthOfScreen(DefaultScreenOfDisplay(dpy)) - dialogWidth)
	x = WidthOfScreen(DefaultScreenOfDisplay(dpy)) - dialogWidth;
    XtVaSetValues(this->getRootWidget(),
	XmNdefaultPosition, False, 
	NULL);
    XtVaSetValues(XtParent(this->getRootWidget()),
	XmNdefaultPosition, False, 
	XmNx, x, 
	XmNy, y, 
	NULL);

    this->resetProjection();
    this->setSensitivity(this->imageWindow->directInteractionAllowed());

    if (this->cameraFormManaged)
    {
    }
    if (this->navigateFormManaged)
    {
    }

    this->Dialog::manage();
}

boolean ViewControlDialog::isExpanding()
{
    switch (this->imageWindow->getInteractionMode()) 
    {
        case CAMERA:
        case CURSORS:
        case PICK:
        case NAVIGATE:
        case ROAM:
             return TRUE;
             break;
	default:
	     break;
    }

    return FALSE;
}

void ViewControlDialog::resetMode()
{
    Widget w = ((Widget) None);

    switch (this->imageWindow->getInteractionMode()) {
    case NONE:
	w = this->modeNone->getRootWidget();
	break;
    case CAMERA:
	w = this->modeCamera->getRootWidget();
	break;
    case CURSORS:
	w = this->modeCursors->getRootWidget();
	break;
    case PICK:
	w = this->modePick->getRootWidget();
	break;
    case NAVIGATE:
	w = this->modeNavigate->getRootWidget();
	break;
    case PANZOOM:
	w = this->modePanZoom->getRootWidget();
	break;
    case ROAM:
	w = this->modeRoam->getRootWidget();
	break;
    case ROTATE:
	w = this->modeRotate->getRootWidget();
	break;
    case ZOOM:
	w = this->modeZoom->getRootWidget();
	break;
    default:
	ASSERT(0);
    }

    XtVaSetValues(this->modeOptionMenu, XmNmenuHistory, w, NULL);
}

void ViewControlDialog::resetLookDirection()
{
    XtVaSetValues(this->navigateLookOptionMenu,
	XmNmenuHistory, this->lookForward->getRootWidget(),
	NULL);
}
void ViewControlDialog::resetSetView()
{
    XtVaSetValues(this->setViewOptionMenu,
	XmNmenuHistory, this->setViewNone->getRootWidget(),
	NULL);
}
void ViewControlDialog::resetProjection()
{
    double viewAngle;
#ifdef PASSDOUBLEVALUE
    XtArgVal dx_l;
#endif

    this->imageWindow->getViewAngle(viewAngle);
    XtVaSetValues(this->viewAngleStepper, 
	    XmNdValue, DoubleVal(viewAngle, dx_l),NULL);
    double width;
    this->imageWindow->getWidth(width);
    XtVaSetValues(this->cameraWidthNumber, 
	    XmNdValue, DoubleVal(width, dx_l), NULL);

    int x,y;
    this->imageWindow->getResolution(x, y);
    XtVaSetValues(this->cameraWindowWidthNumber, XmNiValue, x, NULL);
    XtVaSetValues(this->cameraWindowHeightNumber, XmNiValue, y, NULL);

    if (this->imageWindow->getPerspective())
    {
	XtVaSetValues(this->projectionOptionMenu,
	    XmNmenuHistory, this->perspective->getRootWidget(), NULL);
	XtSetSensitive(this->viewAngleStepper, TRUE);
	XtSetSensitive(this->cameraWidthNumber, FALSE);
    }
    else
    {
	XtVaSetValues(this->projectionOptionMenu,
	    XmNmenuHistory, this->orthographic->getRootWidget(), NULL);
	XtSetSensitive(this->viewAngleStepper, FALSE);
	XtSetSensitive(this->cameraWidthNumber, TRUE);
    }
}

void ViewControlDialog::setNavigateSpeed(double s)
{
    XtVaSetValues(this->motionScale, XmNvalue, (int)s, NULL);
}
void ViewControlDialog::setNavigatePivot(double s)
{
    XtVaSetValues(this->pivotScale, XmNvalue, (int)s, NULL);
}

void ViewControlDialog::newCamera(double *from, double *to, double *up,
	int image_width, int image_height, double width,
	boolean perspective, double viewAngle)
{
#ifdef PASSDOUBLEVALUE
    XtArgVal dx_l1, dx_l2, dx_l3;
#endif
    this->resetProjection();

    XtVaSetValues(this->cameraWindowWidthNumber, XmNiValue, image_width, NULL);
    XtVaSetValues(this->cameraWindowHeightNumber,XmNiValue, image_height,NULL);
    XtVaSetValues(this->cameraWidthNumber, 
	XmNdValue, DoubleVal(width, dx_l1), NULL);

    Widget button;
    XtVaGetValues(this->cameraWhichOptionMenu, XmNmenuHistory, &button, NULL);

    double *v;
    if (button == this->cameraTo->getRootWidget())
    {
	v = to;
    }
    else if (button == this->cameraFrom->getRootWidget())
    {
	v = from;
    }
    else if (button == this->cameraUp->getRootWidget())
    {
	v = up;
    }
    XtVaSetValues(this->cameraXNumber, XmNdValue, DoubleVal(v[0], dx_l1), NULL);
    XtVaSetValues(this->cameraYNumber, XmNdValue, DoubleVal(v[1], dx_l2), NULL);
    XtVaSetValues(this->cameraZNumber, XmNdValue, DoubleVal(v[2], dx_l3), NULL);

    this->setSensitivity(TRUE);
}

void ViewControlDialog::setSensitivity(boolean s)
{
    int b = s? True: False;
    XtSetSensitive(this->modeOptionMenu, b);
    XtSetSensitive(this->setViewOptionMenu, b);
    XtSetSensitive(this->cameraWhichOptionMenu, b);
    XtSetSensitive(this->cameraXNumber, b);
    XtSetSensitive(this->cameraYNumber, b);
    XtSetSensitive(this->cameraZNumber, b);
    XtSetSensitive(this->cameraWindowHeightNumber, b);
    XtSetSensitive(this->cameraWindowWidthNumber, b);

    if (s)
	this->resetProjection();
}

void ViewControlDialog::setWhichCameraVector()
{
    Widget button;
    XtVaGetValues(this->cameraWhichOptionMenu, XmNmenuHistory, &button, NULL);

    double v[3];
    if (button == this->cameraTo->getRootWidget())
    {
	this->imageWindow->getTo(v);
    }
    else if (button == this->cameraFrom->getRootWidget())
    {
	this->imageWindow->getFrom(v);
    }
    else if (button == this->cameraUp->getRootWidget())
    {
	this->imageWindow->getUp(v);
    }
#ifdef PASSDOUBLEVALUE
    XtArgVal dx_l1, dx_l2, dx_l3;
#endif
    XtVaSetValues(this->cameraXNumber, XmNdValue, DoubleVal(v[0], dx_l1), NULL);
    XtVaSetValues(this->cameraYNumber, XmNdValue, DoubleVal(v[1], dx_l2), NULL);
    XtVaSetValues(this->cameraZNumber, XmNdValue, DoubleVal(v[2], dx_l3), NULL);
}
extern "C" void ViewControlDialog_NumberCB(Widget	 widget,
				 XtPointer clientData,
				 XtPointer callData)
{
    ViewControlDialog *obj = (ViewControlDialog*)clientData;
    XmDoubleCallbackStruct *dcs = (XmDoubleCallbackStruct*) callData;

    if (dcs->reason != XmCR_ACTIVATE)
	return;

    Widget button;
    XtVaGetValues(obj->cameraWhichOptionMenu, XmNmenuHistory, &button, NULL);

    double v[3];
    if(button == obj->cameraTo->getRootWidget())
    {
	obj->imageWindow->getTo(v);
    }
    else if (button == obj->cameraFrom->getRootWidget())
    {
	obj->imageWindow->getFrom(v);
    }
    else if (button == obj->cameraUp->getRootWidget())
    {
	obj->imageWindow->getUp(v);
    }

    if (widget == obj->viewAngleStepper)
    {
	double w;
	XtVaGetValues(widget, XmNdValue, &w, NULL);
	obj->imageWindow->setViewAngle(w);
    }
    else if (widget == obj->cameraXNumber)
    {
	XtVaGetValues(widget, XmNdValue, &v[0], NULL);
	if (button == obj->cameraTo->getRootWidget())
	{
	    obj->imageWindow->setTo(v);
	}
	else if (button == obj->cameraFrom->getRootWidget())
	{
	    obj->imageWindow->setFrom(v);
	}
	else if (button == obj->cameraUp->getRootWidget())
	{
	    obj->imageWindow->setUp(v);
	}
    }
    else if (widget == obj->cameraYNumber)
    {
	XtVaGetValues(widget, XmNdValue, &v[1], NULL);
	if (button == obj->cameraTo->getRootWidget())
	{
	    obj->imageWindow->setTo(v);
	}
	else if (button == obj->cameraFrom->getRootWidget())
	{
	    obj->imageWindow->setFrom(v);
	}
	else if (button == obj->cameraUp->getRootWidget())
	{
	    obj->imageWindow->setUp(v);
	}
    }
    else if (widget == obj->cameraZNumber)
    {
	XtVaGetValues(widget, XmNdValue, &v[2], NULL);
	if (button == obj->cameraTo->getRootWidget())
	{
	    obj->imageWindow->setTo(v);
	}
	else if (button == obj->cameraFrom->getRootWidget())
	{
	    obj->imageWindow->setFrom(v);
	}
	else if (button == obj->cameraUp->getRootWidget())
	{
	    obj->imageWindow->setUp(v);
	}
    }
    else if (widget == obj->cameraWidthNumber)
    {
	double d;
	XtVaGetValues(widget, XmNdValue, &d, NULL);
	obj->imageWindow->setWidth(d);
    }
    else if (widget == obj->cameraWindowHeightNumber)
    {
	int x, y;

	obj->imageWindow->getResolution(x, y);
	XtVaGetValues(widget, XmNiValue, &y, NULL);
	obj->imageWindow->setResolution(x, y);
    }
    else if (widget == obj->cameraWindowWidthNumber)
    {
	int x, y;

	obj->imageWindow->getResolution(x, y);
	XtVaGetValues(widget, XmNiValue, &x, NULL);
	obj->imageWindow->setResolution(x, y);
    }
}

extern "C" void ViewControlDialog_SelectProbeCB(Widget button,
				      XtPointer clientData,
				      XtPointer)
{
    ViewControlDialog*  vc = (ViewControlDialog*)clientData;
    XmUpdateDisplay(vc->imageWindow->getRootWidget());

    Widget w;
    int    i;
    for(i = 1; (w = (Widget)vc->probeWidgetList.getDefinition(i)); ++i)
    {
	if (w == button)
	{
	    const char *label = vc->probeWidgetList.getStringKey(i);
	    Node *node = vc->imageWindow->getNetwork()->findNode(label,NULL,TRUE);
	    ASSERT(node);
	    vc->imageWindow->selectProbeByInstance(node->getInstanceNumber());
	    break;
	}
    }
}

extern "C" void ViewControlDialog_SelectPickCB(Widget button,
				      XtPointer clientData,
				      XtPointer)
{
    ViewControlDialog*  vc = (ViewControlDialog*)clientData;
    XmUpdateDisplay(vc->imageWindow->getRootWidget());

    Widget w;
    int    i;
    for(i = 1; (w = (Widget)vc->pickWidgetList.getDefinition(i)) ; ++i)
    {
	if (w == button)
	{
            const char *label = vc->pickWidgetList.getStringKey(i);
            Node *node = vc->imageWindow->getNetwork()->findNode(label,NULL,TRUE);
            ASSERT(node);
            vc->imageWindow->selectPickByInstance(node->getInstanceNumber());
	    break;
	}
    }
}

extern "C" void ViewControlDialog_ScaleCB(Widget widget,
				XtPointer clientData,
				XtPointer callData)
{
    ViewControlDialog     *vc = (ViewControlDialog*)clientData;
    XmScaleCallbackStruct *sc = (XmScaleCallbackStruct*)callData;

    if (widget == vc->motionScale)
    {
	vc->imageWindow->setTranslateSpeed(sc->value);
    }
    else if (widget == vc->pivotScale)
    {
	vc->imageWindow->setRotateSpeed(sc->value);
    }
    else
	ASSERT(0);
}
//
// Install the default resources for this class.
//
void ViewControlDialog::installDefaultResources(Widget  baseWidget)
{
    this->setDefaultResources(baseWidget, ViewControlDialog::DefaultResources);
    this->Dialog::installDefaultResources( baseWidget);
}

void ViewControlDialog::setCurrentProbeByInstance(int i)
{
    Widget w;

    if (NULL != (w = (Widget)this->probeWidgetList.getDefinition(i)))
      XtVaSetValues(this->probeOptionMenu, 
                    XmNmenuHistory, 
                    w,
                    NULL);
}

void ViewControlDialog::setCurrentPickByInstance(int i)
{
    Widget w;

    if (NULL != (w = (Widget)this->pickWidgetList.getDefinition(i)))
      XtVaSetValues(this->pickOptionMenu, 
                    XmNmenuHistory, 
                    w,
                    NULL);
}

void ViewControlDialog::sensitizeProbeOptionMenu(Boolean sensitize)
{
    if (this->probeOptionMenu)
      XtSetSensitive(this->probeOptionMenu, sensitize);
}

void ViewControlDialog::sensitizePickOptionMenu(Boolean sensitize)
{
    if (this->pickOptionMenu)
      XtSetSensitive(this->pickOptionMenu, sensitize);
}
