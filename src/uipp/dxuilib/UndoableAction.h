/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
#ifndef _UndoableAction_h
#define _UndoableAction_h
#include "../base/defines.h"

#include "Base.h"

class EditorWindow;

//
//
#define UndoableActionClassName "UndoableAction"
class UndoableAction : public Base
{
    protected:
        EditorWindow* editor;

	UndoableAction (EditorWindow* editor, boolean separator) { 
	    this->editor = editor; 
	}
    public:
	virtual ~UndoableAction(){}
	UndoableAction (EditorWindow* editor) { 
	    this->editor = editor; 
	}
	virtual void undo(boolean first_in_list=TRUE)=0;

	//
	// We rely on our creator to discard unusable UndoableActions.  However,
	// our ability to look up the objects upon which we need to act, allows
	// us to test our own ability to undo.  That way menu items be modified
	// (greyed out) as needed.
	//
	virtual boolean canUndo() = 0;

	//
	// Supply text identifying the operation to be undone.  This might
	// be used by a gui thing that wants to update a menu button so
	// that the user can see what is going to be undone next.
	//
	virtual const char* getLabel() = 0;

	//
	// Some undo operations need to set up in order for canUndo()
	// to be successful.
	//
	virtual void prepare(){}

	//
	// Some undo operations perform unmanage() in prepare().  Here
	// they can can manage()
	//
	virtual void postpare(){}

	//
	// in order to store undo objects in a stack and to have a whole
	// bunch of objects that collectively undo a single user-input,
	// we need to be able to separate groups of undo objects.
	//
	virtual boolean isSeparator() { return FALSE; }
	virtual const char* getClassName() { return UndoableActionClassName; }
};

#define UndoSeparatorClassName "UndoSeparatorClassName"
class UndoSeparator : public UndoableAction
{
    public:
	virtual ~UndoSeparator(){}
	virtual const char* getLabel() { return NUL(const char*); }
	UndoSeparator(EditorWindow* editor) : UndoableAction(editor) {}
	virtual boolean isSeparator() { return TRUE; }
	virtual void undo(boolean first_in_list=TRUE){}
	virtual boolean canUndo() { return FALSE; }
	virtual const char* getClassName() { return UndoSeparatorClassName; }
};

#endif // _UndoableAction_h
