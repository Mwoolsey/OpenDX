/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
#ifndef _UndoRepeatableTab_h
#define _UndoRepeatableTab_h
#include "UndoNode.h"

class Node;

//
//
#define UndoRepeatableTabClassName "UndoRepeatableTab"
class UndoRepeatableTab : public UndoNode
{
    private:
    protected:
	static char OperationName[];
	boolean adding;
	boolean input;
    public:
	virtual const char* getLabel() { return UndoRepeatableTab::OperationName; }
	virtual ~UndoRepeatableTab(){}
	virtual boolean canUndo();
	UndoRepeatableTab (EditorWindow* editor, Node* n, boolean adding, boolean input);
	virtual void undo(boolean first_in_list=TRUE);
	virtual const char* getClassName() { return UndoRepeatableTabClassName; }
};

#endif // _UndoRepeatableTab_h
