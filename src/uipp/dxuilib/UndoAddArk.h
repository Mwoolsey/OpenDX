/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
#ifndef _UndoAddArk_h
#define _UndoAddArk_h
#include "UndoableAction.h"

class Node;
class Ark;
class EditorWindow;

//
//
#define UndoAddArkClassName "UndoAddArk"
class UndoAddArk : public UndoableAction
{
    private:
    protected:
	static char OperationName[];
	const char* source_name;
	const char* destination_name;
	int source_instance;
	int destination_instance;
	int input;
	int output;
	boolean adding;

	Ark* lookupArk(Node* src, Node* dst);

    public:
	virtual const char* getLabel() { return UndoAddArk::OperationName; }
	virtual ~UndoAddArk(){}
	virtual boolean canUndo();
	UndoAddArk (EditorWindow* editor, Ark* arc, boolean adding);
	virtual void undo(boolean first_in_list=TRUE);
	virtual const char* getClassName() { return UndoAddArkClassName; }
};

#endif // _UndoAddArk_h
