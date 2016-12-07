/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include <defines.h>


#include <Xm/Xm.h>
#include <Xm/Text.h>
#include <Xm/SelectioB.h>

#include "NoUndoCommand.h"
#include "ColormapEditCommand.h"
#include "ColormapEditor.h"
#include "ColormapNode.h"
#include "ToggleButtonInterface.h"
#include "ButtonInterface.h"

#include "../widgets/ColorMapEditor.h"

String ColormapEditCommand::DefaultResources[] =
{
        NULL
};

ColormapEditCommand::ColormapEditCommand(const char*   name,
                         CommandScope* scope,
                         boolean       active,
                         ColormapEditor* editor,
			 int option):NoUndoCommand(name,scope,active)
{
	this->editor = editor;	
	this->option = option;	
}

boolean ColormapEditCommand::doIt(CommandInterface *ci)
{
    Arg      	   wargs[8];
    ColormapEditor *editor = this->editor;
    ColormapNode   *cmnode = editor->colormapNode;
    Widget         colormap = this->editor->workArea;
    ToggleButtonInterface *tbi;
    Boolean	   set;

    switch (this->option)
    {
	case ColormapEditCommand::New:

	     XmColorMapReset(colormap);

	     break;

	case ColormapEditCommand::AddControl:

	     editor->openAddCtlDialog();

	     break;

	case ColormapEditCommand::NBins:

	     editor->openNBinsDialog();

	     break;

	case ColormapEditCommand::Waveform:

	     editor->openWaveformDialog();

	     break;

	case ColormapEditCommand::Undo:

	     XmColorMapUndo(colormap);

	     break;

	case ColormapEditCommand::Copy:

	     XmColorMapCopy(colormap);

	     break;

	case ColormapEditCommand::Paste:

	     XmColorMapPaste(colormap);

	     break;

	case ColormapEditCommand::DeleteSelected:

	     XmColorMapEditorDeleteCP(colormap);

	     break;

	case ColormapEditCommand::SelectAll:

	     XmColorMapSelectAllCP(colormap);

	     break;

	case ColormapEditCommand::SetBackground:

             editor->background_style = 1 - editor->background_style;
             XtSetArg(wargs[0],XmNbackgroundStyle, editor->background_style);
             XtSetValues(colormap, wargs, 1);

	     if(editor->background_style)
             	((ButtonInterface*)this->editor->setBackgroundOption)->setLabel(
					"Set Background Style to Stripes");
	     else
             	((ButtonInterface*)this->editor->setBackgroundOption)->setLabel(
					"Set Background Style to Checkboard");

	     break;

	case ColormapEditCommand::Ticks:

             editor->draw_mode = CME_GRID;
	     editor->setDrawMode();
	     break;

	case ColormapEditCommand::Histogram:

             editor->draw_mode = CME_HISTOGRAM;
	     editor->setDrawMode();
	     break;

	case ColormapEditCommand::LogHistogram:

             editor->draw_mode = CME_LOGHISTOGRAM;
	     editor->setDrawMode();
	     break;

	case ColormapEditCommand::ConstrainH:

             editor->constrain_hor = !editor->constrain_hor;
             XtSetArg(wargs[0],XmNconstrainHorizontal, editor->constrain_hor);
             XtSetValues(colormap, wargs, 1);

             if (editor->constrain_hor && editor->constrain_vert)
             {
            	editor->constrain_vert = False;
            	XtSetArg(wargs[0], XmNconstrainVertical, False);
            	XtSetValues(colormap, wargs, 1);
            	((ToggleButtonInterface*)this->editor->verticalOption)->setState(False);
             }
	     break;

	case ColormapEditCommand::ConstrainV:

             editor->constrain_vert = !editor->constrain_vert;
             XtSetArg(wargs[0],XmNconstrainVertical, editor->constrain_vert);
             XtSetValues(colormap, wargs, 1);

             if (editor->constrain_hor && editor->constrain_vert)
             {
            	editor->constrain_hor = False;
            	XtSetArg(wargs[0], XmNconstrainHorizontal, False);
            	XtSetValues(colormap, wargs, 1);
            	((ToggleButtonInterface*)this->editor->horizontalOption)->setState(False);
             }
	     break;

	case ColormapEditCommand::DisplayCPOff:
	    tbi = (ToggleButtonInterface *)this->editor->displayCPOffOption;
	    XtVaGetValues(tbi->getRootWidget(), XmNset, &set, NULL);
	    if (set) XtVaSetValues(colormap, XmNprintCP, XmPRINT_NONE, NULL);
	    break;

	case ColormapEditCommand::DisplayCPAll:
	    tbi = (ToggleButtonInterface *)this->editor->displayCPAllOption;
	    XtVaGetValues(tbi->getRootWidget(), XmNset, &set, NULL);
	    if (set) XtVaSetValues(colormap, XmNprintCP, XmPRINT_ALL, NULL);
	    break;

	case ColormapEditCommand::DisplayCPSelected:
	    tbi = (ToggleButtonInterface*)this->editor->displayCPSelectedOption;
	    XtVaGetValues(tbi->getRootWidget(), XmNset, &set, NULL);
	    if (set) XtVaSetValues(colormap, XmNprintCP, XmPRINT_SELECTED, NULL);
	    break;

	case ColormapEditCommand::SetColormapName:
	    editor->editColormapName();
	    break;

	case ColormapEditCommand::ResetAll:
	    if (cmnode->hasDefaultHSVMap())
		cmnode->installCurrentDefaultHSVMap(FALSE);
	    if (cmnode->hasDefaultOpacityMap())
		cmnode->installCurrentDefaultOpacityMap(FALSE);
	    if (cmnode->hasDefaultMin())
		cmnode->installCurrentDefaultMin(FALSE);
	    if (cmnode->hasDefaultMax())
		cmnode->installCurrentDefaultMax(FALSE);
	    cmnode->sendValues(FALSE);
	    break;
	case ColormapEditCommand::ResetMinMax:
	    cmnode->installCurrentDefaultMinAndMax(TRUE);
	    break;
	case ColormapEditCommand::ResetMin:
	    cmnode->installCurrentDefaultMin(TRUE);
	    break;
	case ColormapEditCommand::ResetMax:
	    cmnode->installCurrentDefaultMax(TRUE);
	    break;
	case ColormapEditCommand::ResetHSV:
	    cmnode->installCurrentDefaultHSVMap(TRUE);
	    break;
	case ColormapEditCommand::ResetOpacity:
	    cmnode->installCurrentDefaultOpacityMap(TRUE);
	    break;

	default:

	     ASSERT(FALSE);
    }

    return TRUE;
}


