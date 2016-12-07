/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include <defines.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include <Xm/CascadeB.h>
#include <Xm/DrawingA.h>
#include <Xm/Frame.h>
#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/List.h>
#include <Xm/RowColumn.h>
#include <Xm/ScrolledW.h>
#include <Xm/Separator.h>


#include "DXType.h"
#include "DXValue.h"
#include "DXApplication.h"
#include "ButtonInterface.h"
#include "ToggleButtonInterface.h"
#include "CommandScope.h"
#include "OpenColormapDialog.h"
#include "HelpOnContextCommand.h"
#include "CloseWindowCommand.h"
#include "WarningDialogManager.h"
#include "ColormapEditor.h"
#include "ColormapNode.h"
#include "DeferrableAction.h"
#include "ColormapEditCommand.h"
#include "ColormapAddCtlDialog.h"
#include "ColormapNBinsDialog.h"
#include "ColormapWaveDialog.h"
#include "SetColormapNameDialog.h"
#include "ColormapFileCommand.h"
#include "Network.h"
#include "../base/CascadeMenu.h"

#include "../widgets/ColorMapEditor.h"

#ifdef ABS_IN_MATH_H
# define abs __Dont_define_abs
#endif
#include <math.h>
#ifdef ABS_IN_MATH_H
# undef abs
#endif

#include "CMDefaultResources.h"

boolean ColormapEditor::ClassInitialized = FALSE;


ColormapEditor::ColormapEditor(ColormapNode*    cmnode) :
			DXWindow("colormapEditor", FALSE)
{

    //
    // Initialize member data.
    //
    this->fileMenu       = NUL(Widget);
    this->editMenu       = NUL(Widget);
    this->optionsMenu    = NUL(Widget);

    this->fileMenuPulldown       = NUL(Widget);
    this->editMenuPulldown       = NUL(Widget);
    this->optionsMenuPulldown    = NUL(Widget);
    this->colormapEditor = NUL(Widget);

    this->newOption          = NUL(CommandInterface*);
    this->openOption         = NUL(CommandInterface*);
    this->saveAsOption       = NUL(CommandInterface*);
    this->closeOption        = NUL(CommandInterface*);

    this->addControlOption = NUL(CommandInterface*);
    this->undoOption = NUL(CommandInterface*);
    this->copyOption = NUL(CommandInterface*);
    this->pasteOption = NUL(CommandInterface*);
    this->resetMapCascade	     = NULL;
    this->waveformOption = NUL(CommandInterface*);
    this->deleteSelectedOption = NUL(CommandInterface*);

    this->nBinsOption = NUL(CommandInterface*);
    this->setBackgroundOption = NUL(CommandInterface*);
    this->ticksOption = NUL(CommandInterface*);
    this->histogramOption = NUL(CommandInterface*);
    this->logHistogramOption = NUL(CommandInterface*);
    this->horizontalOption = NUL(CommandInterface*);
    this->verticalOption = NUL(CommandInterface*);
    this->onVisualProgramOption = NUL(CommandInterface*);

    this->nBinsDialog = NUL(ColormapNBinsDialog*);
    this->addCtlDialog = NUL(ColormapAddCtlDialog*);
    this->waveformDialog = NUL(ColormapWaveDialog*);
    this->setNameDialog = NULL;
    this->openColormapDialog = NUL(OpenColormapDialog*);

    this->hue_selected = 0;
    this->sat_selected = 0;
    this->val_selected = 0;
    this->op_selected = 0;
    this->selected_area = 0;

    this->constrain_vert = FALSE;
    this->constrain_hor = FALSE;
    this->background_style = STRIPES;
    this->draw_mode = CME_GRID;

    this->doingActivateCallback = FALSE;
    this->neverManaged = TRUE;

    //
    // Intialize state.
    //

    this->colormapNode = cmnode;
    const char *t = cmnode->getTitle();
    if (t)
	this->setWindowTitle(t);
 
    ASSERT(this->commandScope);

    //
    // Create the commands.
    //
    this->closeCmd = new CloseWindowCommand("closeWindow",
					this->commandScope,
					TRUE,
					this);

    this->nBinsCmd = new ColormapEditCommand("nBins",
					this->commandScope,
					TRUE,
					this,
					ColormapEditCommand::NBins);

    this->addCtlCmd = new ColormapEditCommand("addControlPoint",
					this->commandScope,
					TRUE,
					this,
					ColormapEditCommand::AddControl);

    this->undoCmd = new ColormapEditCommand("undo",
					this->commandScope,
					TRUE,
					this,
					ColormapEditCommand::Undo);

    this->copyCmd = new ColormapEditCommand("copy",
					this->commandScope,
					TRUE,
					this,
					ColormapEditCommand::Copy);

    this->pasteCmd = new ColormapEditCommand("paste",
					this->commandScope,
					TRUE,
					this,
					ColormapEditCommand::Paste);

    this->waveformCmd = new ColormapEditCommand("waveform",
					this->commandScope,
					TRUE,
					this,
					ColormapEditCommand::Waveform);

    this->delSelectedCmd = new ColormapEditCommand("deleteSelectedPoint",
					this->commandScope,
					TRUE,
					this,
					ColormapEditCommand::DeleteSelected);

    this->selectAllCmd = new ColormapEditCommand("selectAll",
					this->commandScope,
					TRUE,
					this,
					ColormapEditCommand::SelectAll);

    this->setBkgdCmd = new ColormapEditCommand("setBackground",
					this->commandScope,
					TRUE,
					this,
					ColormapEditCommand::SetBackground);

    this->ticksCmd = new ColormapEditCommand("ticks",
					this->commandScope,
					TRUE,
					this,
					ColormapEditCommand::Ticks);

    this->histogramCmd = new ColormapEditCommand("histogram",
					this->commandScope,
					TRUE,
					this,
					ColormapEditCommand::Histogram);

    this->logHistogramCmd = new ColormapEditCommand("logHistogram",
					this->commandScope,
					TRUE,
					this,
					ColormapEditCommand::LogHistogram);

    this->consVCmd = new ColormapEditCommand("constrainVertical",
					this->commandScope,
					TRUE,
					this,
					ColormapEditCommand::ConstrainV);

    this->consHCmd = new ColormapEditCommand("constrainHorizonal",
					this->commandScope,
					TRUE,
					this,
					ColormapEditCommand::ConstrainH);

    this->newCmd = new ColormapEditCommand("new",
					this->commandScope,
					TRUE,
					this,
					ColormapEditCommand::New);

    this->resetAllCmd = new ColormapEditCommand("resetALL",
					this->commandScope,
					TRUE,
					this,
					ColormapEditCommand::ResetAll);

    this->resetHSVCmd = new ColormapEditCommand("resetHSV",
					this->commandScope,
					TRUE,
					this,
					ColormapEditCommand::ResetHSV);

    this->resetOpacityCmd = new ColormapEditCommand("resetOpacity",
					this->commandScope,
					TRUE,
					this,
					ColormapEditCommand::ResetOpacity);

    this->resetMinMaxCmd = new ColormapEditCommand("resetMinMax",
					this->commandScope,
					TRUE,
					this,
					ColormapEditCommand::ResetMinMax);

    this->resetMinCmd = new ColormapEditCommand("resetMin",
					this->commandScope,
					TRUE,
					this,
					ColormapEditCommand::ResetMin);

    this->resetMaxCmd = new ColormapEditCommand("resetMax",
					this->commandScope,
					TRUE,
					this,
					ColormapEditCommand::ResetMax);

    if (theDXApplication->appAllowsCMapOpenMap())
	this->openFileCmd = new ColormapFileCommand("openFile",
					this->commandScope,
					TRUE,
					this,
					TRUE);
    else
	this->openFileCmd = NULL;
 

    if (theDXApplication->appAllowsCMapSaveMap())
	this->saveFileCmd = new ColormapFileCommand("saveFile",
					this->commandScope,
					TRUE,
					this,
					FALSE);
    else
	this->saveFileCmd = NULL; 


    this->displayCPOffCmd = new ColormapEditCommand("displayCPOff", 
					this->commandScope,
					TRUE, 
					this, 
					ColormapEditCommand::DisplayCPOff);

    this->displayCPAllCmd = new ColormapEditCommand("displayCPAll", 
					this->commandScope,
					TRUE, 
					this, 
					ColormapEditCommand::DisplayCPAll);

    this->displayCPSelectedCmd = new ColormapEditCommand("displayCPSelected", 
					this->commandScope,
					TRUE, 
					this, 
					ColormapEditCommand::DisplayCPSelected);

    // FIXME: add restriction here
    if (theDXApplication->appAllowsCMapSetName())
	this->setColormapNameCmd = new ColormapEditCommand("setColormapName", 
					this->commandScope,
					TRUE, 
					this, 
					ColormapEditCommand::SetColormapName);
    else
	this->setColormapNameCmd = NULL; 

   
    //
    // Install the default resources for THIS class (not the derived classes)
    //
    if (NOT ColormapEditor::ClassInitialized)
    {
	ASSERT(theApplication);
        ColormapEditor::ClassInitialized = TRUE;
	this->installDefaultResources(theApplication->getRootWidget());
    }
}


ColormapEditor::~ColormapEditor()
{
    //
    // Delete the File menu commands interfaces (before deleting the commands).
    //
    if (this->newOption) delete this->newOption;
    if (this->openOption) delete this->openOption;
    if (this->saveAsOption) delete this->saveAsOption;
    if (this->closeOption) delete this->closeOption;
    //
    // Delete the Edit menu commands interfaces (before deleting the commands).
    //
    if (this->nBinsOption) delete this->nBinsOption;
    if (this->addControlOption) delete this->addControlOption;
    if (this->undoOption) delete this->undoOption;
    if (this->copyOption) delete this->copyOption;
    if (this->pasteOption) delete this->pasteOption;
    if (this->resetMapCascade) delete this->resetMapCascade;
    if (this->waveformOption) delete this->waveformOption;
    if (this->deleteSelectedOption) delete this->deleteSelectedOption;
    if (this->selectAllOption) delete this->selectAllOption;
    //
    // Delete the Option menu commands interfaces (before deleting the commands)
    //
    if (this->setBackgroundOption) delete this->setBackgroundOption;
    if (this->ticksOption) delete this->ticksOption;
    if (this->histogramOption) delete this->histogramOption;
    if (this->logHistogramOption) delete this->logHistogramOption;
    if (this->horizontalOption) delete this->horizontalOption;
    if (this->verticalOption) delete this->verticalOption;
    if (this->displayCPOffOption) delete this->displayCPOffOption;
    if (this->displayCPAllOption) delete this->displayCPAllOption;
    if (this->displayCPSelectedOption) delete this->displayCPSelectedOption;
    if (this->setColormapNameOption) delete this->setColormapNameOption;
    if (this->onVisualProgramOption) delete this->onVisualProgramOption;


    //
    // Delete the commands (after the command interfaces).
    //
    if (this->closeCmd) 		delete this->closeCmd;
    if (this->addCtlCmd) 		delete this->addCtlCmd;
    if (this->nBinsCmd) 		delete this->nBinsCmd;
    if (this->undoCmd) 			delete this->undoCmd;
    if (this->copyCmd) 			delete this->copyCmd;
    if (this->pasteCmd) 		delete this->pasteCmd;
    if (this->waveformCmd) 		delete this->waveformCmd;
    if (this->delSelectedCmd)  		delete this->delSelectedCmd;
    if (this->selectAllCmd) 		delete this->selectAllCmd;
    if (this->setBkgdCmd) 		delete this->setBkgdCmd;
    if (this->ticksCmd) 		delete this->ticksCmd;
    if (this->histogramCmd) 		delete this->histogramCmd;
    if (this->logHistogramCmd) 		delete this->logHistogramCmd;
    if (this->consHCmd) 		delete this->consHCmd;
    if (this->consVCmd) 		delete this->consVCmd;
    if (this->newCmd) 			delete this->newCmd;
    if (this->resetAllCmd) 		delete this->resetAllCmd;
    if (this->resetHSVCmd) 		delete this->resetHSVCmd;
    if (this->resetOpacityCmd) 		delete this->resetOpacityCmd;
    if (this->resetMinMaxCmd) 		delete this->resetMinMaxCmd;
    if (this->resetMinCmd) 		delete this->resetMinCmd;
    if (this->resetMaxCmd) 		delete this->resetMaxCmd;
    if (this->openFileCmd) 		delete this->openFileCmd;
    if (this->saveFileCmd) 		delete this->saveFileCmd;
    if (this->displayCPOffCmd) 		delete this->displayCPOffCmd;
    if (this->displayCPAllCmd) 		delete this->displayCPAllCmd;
    if (this->displayCPSelectedCmd) 	delete this->displayCPSelectedCmd;
    if (this->setColormapNameCmd) 	delete this->setColormapNameCmd;

 
    //
    // Delete any dialogs that may have been created.
    //
    if (this->nBinsDialog) 		delete this->nBinsDialog; 
    if (this->addCtlDialog) 		delete this->addCtlDialog;
    if (this->waveformDialog) 		delete this->waveformDialog;
    if (this->setNameDialog) 		delete this->setNameDialog;
    if (this->openColormapDialog) 	delete this->openColormapDialog;
}


//
// Install the default resources for this class.
//
void ColormapEditor::installDefaultResources(Widget baseWidget)
{
    this->setDefaultResources(baseWidget, ColormapEditor::DefaultResources);
    this->DXWindow::installDefaultResources(baseWidget);
}

void ColormapEditor::initialize()
{
    //
    // Now, call the superclass initialize().
    //
    this->DXWindow::initialize();

    //
    // Make sure the state of the cmap editor is up to date.
    // FIXME: do we need this,  it is done in manage().
    //
    //
    this->handleStateChange();
}

void ColormapEditor::manage()
{

    this->DXWindow::manage();

    //
    // If the .net had the name of a .cm file and this is the first time
    // we're opening this editor, then make sure the values form the .cm
    // file are fed back into the node (they were installed in 
    // createWorkArea()).
    //
    const char *name;
    if (this->neverManaged && 
	(name = this->colormapNode->getNetSavedCMFilename())) {
        struct STATSTRUCT statbuf;

        if (STATFUNC(name,&statbuf) == 0) {         // File exists
	    XmColorMapEditorRead(this->colormapEditor,(char*)name);
        } else {
           WarningMessage("Can't locate color map file '%s', "
                          "using default map",name);
        }
    } 

    this->handleStateChange();

    this->neverManaged = FALSE;
}

void ColormapEditor::setDrawMode()
{
    // Handle the case where we are not completely initialized
    if(!this->ticksOption)
    {
	// Force the widget to display a grid if we are not data driven
	if(!this->colormapNode->isDataDriven())
	{
	    this->draw_mode = CME_GRID;

	    // "Remove" the existing histogram, since it is no longer valid
	    XmColorMapEditorLoadHistogram(this->colormapEditor, NULL, 0);
	}

	// Set the sensitivity  based on whether or not it is data driven
	if(this->colormapNode->isDataDriven() &&
	   XmColorMapEditorHasHistogram(this->colormapEditor))
	{
	    this->ticksCmd->activate();
	    this->histogramCmd->activate();
	    this->logHistogramCmd->activate();
	}
	else
	{
	    if(this->ticksOption)
		((ToggleButtonInterface *)(this->ticksOption))->setState(TRUE);
	    if(this->histogramOption)
		((ToggleButtonInterface *)(this->histogramOption))->setState(FALSE);
	    if(this->logHistogramOption)
		((ToggleButtonInterface *)(this->logHistogramOption))->setState(FALSE);
	    this->ticksCmd->deactivate();
	    this->histogramCmd->deactivate();
	    this->logHistogramCmd->deactivate();
	}

	return;
    }


    if(!this->colormapNode->isDataDriven() || 
       !XmColorMapEditorHasHistogram(this->colormapEditor))
    {
	this->draw_mode = CME_GRID;

	// "Remove" the existing histogram, since it is no longer valid
	XmColorMapEditorLoadHistogram(this->colormapEditor, NULL, 0);
    }

    // Set the sensitivity  based on whether or not it is data driven
    if(this->colormapNode->isDataDriven() &&
       XmColorMapEditorHasHistogram(this->colormapEditor))
	{
	    this->ticksCmd->activate();
	    this->histogramCmd->activate();
	    this->logHistogramCmd->activate();
	}
	else
	{
	    if(this->ticksOption)
		((ToggleButtonInterface *)(this->ticksOption))->setState(TRUE);
	    if(this->histogramOption)
		((ToggleButtonInterface *)(this->histogramOption))->setState(FALSE);
	    if(this->logHistogramOption)
		((ToggleButtonInterface *)(this->logHistogramOption))->setState(FALSE);
	    this->ticksCmd->deactivate();
	    this->histogramCmd->deactivate();
	    this->logHistogramCmd->deactivate();
	}

    XtVaSetValues(this->colormapEditor, XmNdrawMode, this->draw_mode, NULL);

}
//
// Handle any state change that is associated with either the editor or
// the Node.
//
void ColormapEditor::handleStateChange()
{
    ColormapNode *node = this->colormapNode;
    int	nhist;

    if (this->doingActivateCallback)
	return;

    int *hist = node->getHistogram(&nhist);
    XmColorMapEditorLoadHistogram(this->colormapEditor,hist,nhist);
    if (hist)
	delete hist;

    if (node->isNBinsAlterable())
	 this->nBinsCmd->activate();
    else
	 this->nBinsCmd->deactivate();

    if (this->setColormapNameCmd) {
	if (node->isTitleVisuallyWriteable())
	     this->setColormapNameCmd->activate();
	else
	     this->setColormapNameCmd->deactivate();
    }


    this->setDrawMode();

    // 
    // Set the sensitivity of the editor and menu items based on the
    // data-driven state of the node.
    // 
    this->adjustSensitivities();

    //
    // Now load in the control points.
    //

    int tuple, huecnt, satcnt, valcnt, opcnt;
    double *huemap, *satmap, *valmap, *opmap;
    double min = node->getMinimumValue();
    double max = node->getMaximumValue();
    //
    // Get the hue map 
    //
    const char *mapstr = node->getHueMapList(); 
    huecnt = DXValue::GetDoublesFromList(mapstr, DXType::VectorListType,
					&huemap, &tuple);

    //
    // Get the saturation map 
    //
    mapstr = node->getSaturationMapList(); 
    satcnt = DXValue::GetDoublesFromList(mapstr, DXType::VectorListType,
					&satmap, &tuple);

    //
    // Get the value map 
    //
    mapstr = node->getValueMapList(); 
    valcnt = DXValue::GetDoublesFromList(mapstr, DXType::VectorListType,
					&valmap, &tuple);

    //
    // Get the opacity map 
    //
    mapstr  = node->getOpacityMapList(); 
    opcnt = DXValue::GetDoublesFromList(mapstr, DXType::VectorListType,
					&opmap, &tuple);


    XmColorMapEditorLoad(this->colormapEditor, min, max,
					huecnt, huemap,
					satcnt, satmap,
					valcnt, valmap,
					opcnt,  opmap);

    if (huemap) delete huemap;
    if (satmap) delete satmap;
    if (valmap) delete valmap;
    if (opmap) delete opmap;


}

extern "C" void ColormapEditor_ActiveCB(Widget    widget,
                           XtPointer    clientData,
                           XtPointer    callData)
{

    ASSERT(widget);
    ColormapEditor* editor = (ColormapEditor*)clientData;
    editor->activeCallback(callData);

}
void ColormapEditor::activeCallback(XtPointer callData)
{
    XmColorMapEditorCallbackStruct* data =
                                (XmColorMapEditorCallbackStruct*)callData;


    ColormapNode* colormap = this->colormapNode;
    Widget range_widget;

    this->doingActivateCallback = TRUE;


    if(this->waveformDialog)
    {
	XtVaGetValues(this->waveformDialog->rangeoption, 
		    XmNmenuHistory, &range_widget, NULL);
	
	if(range_widget == this->waveformDialog->rangeselected)
	{
	    if( (data->hue_selected < 2) &&
		(data->sat_selected < 2) &&
		(data->val_selected < 2) &&
		(data->op_selected  < 2) )
		XtSetSensitive(this->waveformDialog->applybtn, False);
	    else
		XtSetSensitive(this->waveformDialog->applybtn, True);
	}
    }
    if (this->addCtlDialog)
    {
	if(data->selected_area != this->selected_area)
	    this->addCtlDialog->setFieldLabel(data->selected_area);
    }

    this->hue_selected   = data->hue_selected;
    this->sat_selected   = data->sat_selected;
    this->val_selected   = data->val_selected;
    this->op_selected    = data->op_selected;
    this->selected_area  = data->selected_area;

    if(data->reason == XmCR_MODIFIED)
    {
	int hcnt = data->num_hue_points;
	int scnt = data->num_sat_points;
	int vcnt = data->num_val_points;
	int opcnt = data->num_op_points;

	//
	// Since we may be updating many of the values in the the
	// attribute parameter, don't keep it up to date until after
 	// we're done here.  See undeferAction() below.
	//
	colormap->updateMinMaxAttr->deferAction();

	// 
	//  Sync the parameter values with the values set in the color map 
	//  window.
	// 
	colormap->setMaximumValue(data->max_value);
	colormap->setMinimumValue(data->min_value);
	//
	// Since we may be updating many of the values in the the
	// attribute parameter, don't keep it up to date until after
 	// we're done here, but do it before the values are sent which
  	// may cause an execution.  See deferAction() above.
	//
	colormap->updateMinMaxAttr->undeferAction();


	colormap->installNewMaps(
	    	hcnt, data->hue_values, data->hue_values + hcnt,
	    	scnt, data->sat_values, data->sat_values + scnt,
	    	vcnt, data->val_values, data->val_values + vcnt,
	    	opcnt, data->op_values, data->op_values + opcnt,
		TRUE);	

  
	//
	// Note: addCtlDialog->setStepper depends on the node's min/max
	//       having been set before hand.
	//
	if (this->addCtlDialog)
	    this->addCtlDialog->setStepper();

    }

    this->doingActivateCallback = FALSE;

}

extern "C" void ColormapEditor_EditMenuMapCB(Widget    widget,
                           XtPointer    clientData,
                           XtPointer    callData)
{
    ColormapEditor* editor = (ColormapEditor*)clientData;
    ASSERT(widget);
    ASSERT(callData);
    editor->editMenuMapCallback();
}
void ColormapEditor::editMenuMapCallback()
{
    (this->sat_selected == 0 AND this->val_selected == 0 AND
     this->hue_selected == 0 AND this->op_selected == 0) 
			   ? this->delSelectedCmd->deactivate() 
			   : this->delSelectedCmd->activate(); 

    (this->sat_selected == 0 AND this->val_selected == 0 AND
     this->hue_selected == 0 AND this->op_selected == 0) 
			   ? this->copyCmd->deactivate() 
			   : this->copyCmd->activate(); 
    if(XmColorMapPastable())
	this->pasteCmd->activate();
    else
	this->pasteCmd->deactivate();

    if(XmColorMapUndoable(this->colormapEditor))
	this->undoCmd->activate();
    else
	this->undoCmd->deactivate();

}

Widget ColormapEditor::createWorkArea(Widget parent)
{
    Arg    arg[8];
    int    n;

    ASSERT(parent);

    //
    // Create the outer and inner frame.
    //
    n = 0;
    XtSetArg(arg[n], XmNresizePolicy,    XmRESIZE_NONE); n++;
    this->colormapEditor = XtCreateWidget("editor", xmColorMapEditorWidgetClass,
				parent, arg, n);

    //
    // Make sure context sensitive help works.
    //
    this->installComponentHelpCallback(this->colormapEditor);


    XtAddCallback(this->colormapEditor,
                  XmNactivateCallback,
                  (XtCallbackProc)ColormapEditor_ActiveCB,
                  (XtPointer)this);

    //
    // Return the topmost widget of the work area.
    //
    return this->colormapEditor;
}


void ColormapEditor::createFileMenu(Widget parent)
{
    ASSERT(parent);

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

    this->newOption =
       new ButtonInterface(pulldown, "cmeNewOption",this->newCmd);


    boolean buttons = FALSE;
    if (this->openFileCmd) {
	this->openOption =
	   new ButtonInterface(pulldown, "cmeOpenOption",this->openFileCmd);
	buttons = TRUE;
    }

    if (this->saveFileCmd) {
	this->saveAsOption =
	    new ButtonInterface(pulldown, "cmeSaveAsOption", this->saveFileCmd);
	buttons = TRUE;
    }

    if (buttons)
	XtVaCreateManagedWidget
	    ("optionSeparator", xmSeparatorWidgetClass, pulldown, NULL);

    this->closeOption =
	new ButtonInterface(pulldown, "cmeCloseOption", this->closeCmd);


}


void ColormapEditor::createEditMenu(Widget parent)
{
    ASSERT(parent);

    Widget            pulldown;

    //
    // Create "Execute" menu and options.
    //
    pulldown =
	this->editMenuPulldown =
	    XmCreatePulldownMenu
		(parent, "editMenuPulldown", NUL(ArgList), 0);
    this->editMenu =
	XtVaCreateManagedWidget
	    ("editMenu",
	     xmCascadeButtonWidgetClass,
	     parent,
	     XmNsubMenuId, pulldown,
	     NULL);

    XtAddCallback(pulldown,
                  XmNmapCallback,
                  (XtCallbackProc)ColormapEditor_EditMenuMapCB,
                  (XtPointer)this);

    //
    // Begin edit menu options 
    //
    this->undoOption =
	new ButtonInterface(pulldown, "cmeUndoOption", this->undoCmd);

    this->copyOption =
	new ButtonInterface(pulldown, "cmeCopyOption", this->copyCmd);

    this->pasteOption =
	new ButtonInterface(pulldown, "cmePasteOption", this->pasteCmd);

    //
    // Reset maps cascade menu 
    //
    CascadeMenu *cascade_menu = this->resetMapCascade = 
			new CascadeMenu("cmeResetCascade",pulldown); 
    Widget menu_parent = this->resetMapCascade->getMenuItemParent();

    CommandInterface *ci = new ButtonInterface(menu_parent,
				"cmeResetHSVOption",this->resetHSVCmd);
    cascade_menu->appendComponent(ci);
    ci = new ButtonInterface(menu_parent,
				"cmeResetOpacityOption",this->resetOpacityCmd);
    cascade_menu->appendComponent(ci);

    ci = new ButtonInterface(menu_parent,
				"cmeResetMinOption",this->resetMinCmd);
    cascade_menu->appendComponent(ci);
    ci = new ButtonInterface(menu_parent,
				"cmeResetMaxOption",this->resetMaxCmd);
    cascade_menu->appendComponent(ci);
    
    XtVaCreateManagedWidget
	    ("resetSeparator", xmSeparatorWidgetClass, menu_parent, NULL);

    ci = new ButtonInterface(menu_parent,
				"cmeResetAllOption",this->resetAllCmd);
    cascade_menu->appendComponent(ci);
    ci = new ButtonInterface(menu_parent,
				"cmeResetMinMaxOption",this->resetMinMaxCmd);
    cascade_menu->appendComponent(ci);

    //
    // More options
    //
    this->addControlOption =
	new ButtonInterface(pulldown, "cmeAddControlOption", this->addCtlCmd);

    this->horizontalOption =
	new ToggleButtonInterface(pulldown, "cmeHorizontalOption", 
				  this->consHCmd, False);

    this->verticalOption =
	new ToggleButtonInterface(pulldown, "cmeVerticalOption", 
				  this->consVCmd, False);

    this->waveformOption =
	new ButtonInterface(pulldown, "cmeWaveformOption", this->waveformCmd);

    XtVaCreateManagedWidget
	    ("optionSeparator", xmSeparatorWidgetClass, pulldown, NULL);

    this->deleteSelectedOption =
	new ButtonInterface(pulldown, "cmeDeleteSelectedOption", 
	    this->delSelectedCmd);

    this->selectAllOption =
	new ButtonInterface(pulldown, "cmeSelectAllOption", 
	    this->selectAllCmd);

}


void ColormapEditor::createOptionsMenu(Widget parent)
{
    ASSERT(parent);

    Widget      pulldown;
    int		n;
    Arg		wargs[20];

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


    this->setBackgroundOption =
	new ButtonInterface(pulldown, "cmeSetBackgroundOption", this->setBkgdCmd);

    n = 0;
    XtSetArg(wargs[n], XmNradioBehavior, True); n++;
    this->drawModePulldown =
            XmCreatePulldownMenu
                (pulldown, "drawModePulldown", wargs, n);

    this->drawModeButton =
        XtVaCreateManagedWidget
            ("cmeSetDrawModeOption",
             xmCascadeButtonWidgetClass,
             pulldown,
             XmNsubMenuId, this->drawModePulldown,
             XmNsensitive, TRUE,
             NULL);

    this->ticksOption = 
	new ToggleButtonInterface(this->drawModePulldown,
	    "cmeTicksOption", this->ticksCmd, TRUE);

    this->histogramOption = 
	new ToggleButtonInterface(this->drawModePulldown,
	    "cmeHistogramOption", this->histogramCmd, FALSE);

    this->logHistogramOption = 
	new ToggleButtonInterface(this->drawModePulldown,
	    "cmeLogHistogramOption", this->logHistogramCmd, FALSE);

    this->nBinsOption =
	new ButtonInterface(pulldown, "cmeNBinsOption", this->nBinsCmd);

    XtVaCreateManagedWidget
	("optionSeparator", xmSeparatorWidgetClass, pulldown, NULL);

    n = 0;
    XtSetArg(wargs[n], XmNradioBehavior, True); n++;
    this->displayCPPulldown =
            XmCreatePulldownMenu
                (pulldown, "displayCPPulldown", wargs, n);

    this->displayCPButton =
        XtVaCreateManagedWidget
            ("displayCPButton",
             xmCascadeButtonWidgetClass,
             pulldown,
             XmNsubMenuId, this->displayCPPulldown,
             XmNsensitive, TRUE,
             NULL);

    this->displayCPOffOption = 
	new ToggleButtonInterface(this->displayCPPulldown,
	    "displayCPOffOption", this->displayCPOffCmd, FALSE);

    this->displayCPAllOption = 
	new ToggleButtonInterface(this->displayCPPulldown,
	    "displayCPAllOption", this->displayCPAllCmd, TRUE);

    this->displayCPSelectedOption = 
	new ToggleButtonInterface(this->displayCPPulldown,
	    "displayCPSelectedOption", this->displayCPSelectedCmd, FALSE);

    if (this->setColormapNameCmd)  {
	XtVaCreateManagedWidget
	    ("optionSeparator", xmSeparatorWidgetClass, pulldown, NULL);
	this->setColormapNameOption =
	    new ButtonInterface(pulldown, "setColormapNameOption", 
			this->setColormapNameCmd);
    } else
	this->setColormapNameOption = NULL;
	
}


void ColormapEditor::createMenus(Widget parent)
{
    ASSERT(parent);

    //
    // Create the menus.
    //
    this->createFileMenu(parent);
    this->createEditMenu(parent);
    this->createExecuteMenu(parent);
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

void ColormapEditor::createHelpMenu(Widget parent)
{
    ASSERT(parent);

    this->DXWindow::createHelpMenu(parent);

    XtVaCreateManagedWidget("separator", xmSeparatorWidgetClass,
                                        this->helpMenuPulldown,
                                        NULL);
    this->onVisualProgramOption =
        new ButtonInterface(this->helpMenuPulldown, "cmapOnVisualProgramOption",
 		this->colormapNode->getNetwork()->getHelpOnNetworkCommand());
}



void ColormapEditor::openAddCtlDialog()
{
    if (!this->addCtlDialog)
	this->addCtlDialog = new ColormapAddCtlDialog(this->getRootWidget(),
						      this);
    this->addCtlDialog->post();
    this->addCtlDialog->setFieldLabel(this->selected_area);

}
void ColormapEditor::openNBinsDialog()
{
    if (!this->nBinsDialog)
	this->nBinsDialog = new ColormapNBinsDialog(this->getRootWidget(),
						     this);
    this->nBinsDialog->post();

}
void ColormapEditor::openWaveformDialog()
{
    if (!this->waveformDialog)
	this->waveformDialog = new ColormapWaveDialog(
					    this->getRootWidget(),
					    this);
    this->waveformDialog->post();
}

boolean ColormapEditor::cmOpenFile(const char* string)
{
    return this->colormapNode->cmOpenFile(string);
}
boolean ColormapEditor::cmSaveFile(const char* string)
{
   return this->colormapNode->cmSaveFile(string);
}


void ColormapEditor::postOpenColormapDialog(boolean opening)
{
    if (this->openColormapDialog == NULL) {
        this->openColormapDialog = new OpenColormapDialog( 
                                            this->getRootWidget(), 
					    this,
                                            opening);
    }
    this->openColormapDialog->setMode(opening);
    this->openColormapDialog->post();
}
//
// Same as the super class, but also sets the icon name.
//
void ColormapEditor::setWindowTitle(const char *name, boolean check_geometry)
{
    this->DXWindow::setWindowTitle(name, check_geometry);
    this->DXWindow::setIconName(name);
}

//
// Edit the name of this Colormap editor.
//
void ColormapEditor::editColormapName()
{
     if (!this->setNameDialog)
        this->setNameDialog = new SetColormapNameDialog(
                                this->getRootWidget(), this->colormapNode);
     this->setNameDialog->post();
}

//
// Set the widget sensitivity based on the whether or not the node thinks
// the HSV, opacity and min and/or max are writeable.
// This includes sensitizing the editMenu, min/max fields, and hsv and
// opacity control point spaces. 
//
void ColormapEditor::adjustSensitivities()
{
    ColormapNode *node = this->colormapNode;
    boolean has_hsv = node->hasDefaultHSVMap();
    boolean has_op = node->hasDefaultOpacityMap();
    boolean has_min = node->hasDefaultMin();
    boolean has_max = node->hasDefaultMax();
    
    if (has_hsv)
	this->resetHSVCmd->activate();
    else
	this->resetHSVCmd->deactivate();

    if (has_op)
	this->resetOpacityCmd->activate();
    else
	this->resetOpacityCmd->deactivate();

    if (has_min)
	this->resetMinCmd->activate();
    else
	this->resetMinCmd->deactivate();

    if (has_max)
	this->resetMaxCmd->activate();
    else
	this->resetMaxCmd->deactivate();

    if (has_min && has_max)
	this->resetMinMaxCmd->activate();
    else
	this->resetMinMaxCmd->deactivate();

    if (has_hsv || has_op || has_min || has_max)
	this->resetAllCmd->activate();
    else
	this->resetAllCmd->deactivate();
    
    this->resetMapCascade->setActivationFromChildren();

}


void ColormapEditor::getGeometryAlternateNames(String* names, int* count, int max)
{
    int cnt = *count;
    if (cnt < (max-1)) {
	char* name = new char[32];
	int inum = this->colormapNode->getInstanceNumber(); 
	sprintf (name, "%s%d", this->name,inum);
	names[cnt++] = name;
	*count = cnt;
    }
    this->DXWindow::getGeometryAlternateNames(names, count, max);
}

