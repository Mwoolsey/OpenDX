/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
#ifndef _UndoGrid_h
#define _UndoGrid_h
#include "UndoableAction.h"
#include "WorkSpaceInfo.h"

class EditorWorkSpace;
class EditorWindow;

//
//
#define UndoGridClassName "UndoGrid"
class UndoGrid : public UndoableAction
{
    private:
	WorkSpaceInfo info;

    protected:
	// record the workSpace from which the standIns originated
	// so that we can force them back into that workSpace.
	EditorWorkSpace* workSpace;
	static char OperationName[];
    public:
	virtual const char* getLabel() { return UndoGrid::OperationName; }
	virtual ~UndoGrid();
	virtual boolean canUndo() { return TRUE; }
	UndoGrid (EditorWindow *editor, EditorWorkSpace* workSpace);
	virtual void undo(boolean first_in_list=TRUE);
	virtual const char* getClassName() { return UndoGridClassName; }
};

#endif // _UndoGrid_h
