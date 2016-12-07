/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
#ifndef _UndoNode_h
#define _UndoNode_h
#include "UndoableAction.h"

class UIComponent;
class StandIn;
class VPEAnnotator;
class Node;
class EditorWindow;
class EditorWorkSpace;


//
// Support class for undo operations that operate on a single node
//
#define UndoNodeClassName "UndoNodeName"
class UndoNode : public UndoableAction
{
    private:
    protected:
	const char* className;
	int instance_number;
	Node* lookupNode();
	UndoNode (EditorWindow* editor, Node* n);
	EditorWorkSpace* workSpace;

    public:
	static Node* LookupNode (EditorWindow* edidtor, const char* className, int instance);
	virtual const char* getClassName() { return UndoNodeClassName; }
};

#endif // _UndoNode_h
