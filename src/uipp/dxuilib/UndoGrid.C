/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
#include "UndoGrid.h"
#include "EditorWorkSpace.h"
#include "../widgets/XmDX.h"

char UndoGrid::OperationName[] = "grid";

UndoGrid::UndoGrid (EditorWindow* editor, EditorWorkSpace* workSpace) :
    UndoableAction(editor)
{
    this->workSpace = workSpace;
    Widget ww = workSpace->getRootWidget();

    //
    // At this point the new info has been stored into the 
    // info object but it hasn't been stored into the widget.
    // In order to provide undo, we fetch information from the
    // workspace object (the this).
    //
    Dimension gridWidth,gridHeight;
    unsigned char snap;
    unsigned char horizAlign,vertAlign;
    Boolean preventOverlap;
    XtVaGetValues (ww,
	XmNgridWidth, &gridWidth,
	XmNgridHeight, &gridHeight,
	XmNsnapToGrid, &snap,
	XmNverticalAlignment, &vertAlign,
	XmNhorizontalAlignment, &horizAlign,
	XmNpreventOverlap, &preventOverlap,
    NULL);
    this->info.setGridActive(snap);
    this->info.setGridSpacing(gridWidth, gridHeight);
    this->info.setGridAlignment(horizAlign, vertAlign);
    this->info.setPreventOverlap(preventOverlap);
}

UndoGrid::~UndoGrid()
{
}

//
//
void UndoGrid::undo(boolean first_in_list) 
{
    WorkSpaceInfo* wsi = this->workSpace->getInfo();
    wsi->setGridAlignment(this->info.getGridXAlignment(), this->info.getGridYAlignment());
    wsi->setGridActive(this->info.isGridActive());
    wsi->setPreventOverlap(this->info.getPreventOverlap());
    int x,y;
    this->info.getGridSpacing(x,y);
    wsi->setGridSpacing(x,y);
    // passing wsi instead of NULL has the side effect of side-stepping
    // further undo processing.
    this->workSpace->installInfo(wsi);
}

