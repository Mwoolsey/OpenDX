/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
#ifndef _UndoMove_h
#define _UndoMove_h
#include "UndoableAction.h"

class UIComponent;
class StandIn;
class VPEAnnotator;
class Node;
class EditorWindow;


//
// In order to track movements in the canvas we'll store
// things in this object and keep them in a list.  It would be
// better to make this a generic Undo object and then make UndoMove
// a subclass of Undo, but I'm not planning on really implementing Undo/Redo.
//
#define UndoMoveClassName "UndoMove"
class UndoMove : public UndoableAction
{
    private:
    protected:
	int x,y;
	UndoMove (EditorWindow* editor, UIComponent* uic);
	static char OperationName[];
    public:
	virtual const char* getLabel() { return UndoMove::OperationName; }
	virtual ~UndoMove(){}
	virtual const char* getClassName() { return UndoMoveClassName; }
};

#define UndoStandInMoveClassName "UndoStandInMove"
class UndoStandInMove : public UndoMove
{
    private:
    protected:
	const char* className;
	int instance_number;
	//Node* lookupNode();

    public:
	//
	// Some undo operations need to set up in order for canUndo()
	// to be successful.
	//
	virtual void prepare();

	//
	// Some undo operations perform unmanage() in prepare().  Here
	// they can can manage()
	//
	virtual void postpare();

	virtual boolean canUndo();
	UndoStandInMove (EditorWindow* editor, StandIn* si, 
		const char* className, int instance);
	virtual void undo(boolean first_in_list=TRUE);
	virtual const char* getClassName() { return UndoStandInMoveClassName; }
};

#define UndoDecoratorMoveClassName "UndoDecoratorMove"
class UndoDecoratorMove : public UndoMove
{
    private:
	int instance;
    protected:
	VPEAnnotator* lookupDecorator();
    public:
	//
	// Some undo operations need to set up in order for canUndo()
	// to be successful.
	//
	virtual void prepare();

	//
	// Some undo operations perform unmanage() in prepare().  Here
	// they can can manage()
	//
	virtual void postpare();

	virtual boolean canUndo();
	virtual ~UndoDecoratorMove();
	UndoDecoratorMove (EditorWindow* editor, VPEAnnotator* dec);
	virtual void undo(boolean first_in_list=TRUE);
	virtual const char* getClassName() { return UndoDecoratorMoveClassName; }
};
#endif // _UndoMove_h
