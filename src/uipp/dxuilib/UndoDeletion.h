/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
#ifndef _UndoDeletion_h
#define _UndoDeletion_h
#include "UndoableAction.h"

class UIComponent;
class Node;
class StandIn;
class List;
class EditorWorkSpace;

//
//
#define UndoDeletionClassName "UndoDeletion"
class UndoDeletion : public UndoableAction
{
    private:
	// will be set to the location of the bounding box of all originally 
	// selected items.  The resulting x,y will be used when placing
	// the nodes back into the network.
	int x,y;
    protected:
	// record the workSpace from which the standIns originated
	// so that we can force them back into that workSpace.
	EditorWorkSpace* workSpace;
	char* buffer;
	void selectConnectedOutputs (Node* n, int output, List& nodes_selected);
	void selectConnectedInputs (Node* n, int input, List& nodes_selected);
	void selectConnectedTo (Node* n, List& nodes_selected);

	static char OperationName[];
    public:
	virtual const char* getLabel() { return UndoDeletion::OperationName; }
	virtual ~UndoDeletion();
	virtual boolean canUndo();
	UndoDeletion (EditorWindow* editor, List* nodes, List* decorators);
	virtual void undo(boolean first_in_list=TRUE);
	virtual const char* getClassName() { return UndoDeletionClassName; }
};

#endif // _UndoDeletion_h
