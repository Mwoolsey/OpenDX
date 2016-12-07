/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
#ifndef _TreeNode_h
#define _TreeNode_h

#include "Base.h"
#include "SymbolManager.h"
#include "List.h"
#include "ListIterator.h"

#define ClassTreeNode "TreeNode"

class List;

class TreeNode : public Base {

  private:

  protected:
    Symbol definition;
    TreeNode* parent;
    TreeNode(Symbol s, TreeNode* parent) {
	this->definition = s;
	this->expanded = FALSE;
	this->parent = parent;
    }

    boolean expanded;

  public:
    virtual ~TreeNode() { }

    virtual boolean hasChildren()=0;
    virtual List* getChildren()=0;
    virtual boolean isExpanded() { return this->expanded; }
    virtual const char* getString() {
	return theSymbolManager->getSymbolString(this->definition);
    }
    virtual boolean isRoot()=0;
    virtual boolean isLeaf()=0;
    virtual Symbol getDefinition() { return this->definition; }
    virtual void setExpanded(boolean e=TRUE) {
	this->expanded = e;
    }
    virtual boolean isSorted() { return FALSE; }

    TreeNode* getParent() { return this->parent; }
 
    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassTreeNode;
    }
};

class LeafNode : public TreeNode {
    private:
    protected:
    public:
	LeafNode(Symbol s, TreeNode* parent) : TreeNode(s, parent) {
	}
	boolean isRoot() { return FALSE; }
	boolean isLeaf() { return TRUE; }
	List* getChildren() { return NUL(List*); }
	boolean hasChildren() { return FALSE ; }
};

class CategoryNode: public TreeNode {
    private:
    protected:
	List kids;
    public:
	CategoryNode(Symbol s, TreeNode* parent) : TreeNode(s, parent) { }
	virtual ~CategoryNode() {
	    ListIterator iter(this->kids);
	    TreeNode* node;
	    while ((node=(TreeNode*)iter.getNext())) {
		delete node;
	    }
	}
	boolean isRoot() { return FALSE; }
	boolean isLeaf() { return FALSE; }
	boolean hasChildren() { return TRUE; }
	void addChild(TreeNode* t) { this->kids.appendElement(t); }
	List* getChildren() { return &this->kids; }
};

class RootNode: public CategoryNode {
    private:
    protected:
    public:
	RootNode() : CategoryNode((Symbol)0, (TreeNode*)0) { }
	boolean isRoot() { return TRUE; }
	const char* getString() { return ""; }
	boolean isExpanded() { return TRUE; }
};

#endif // _TreeNode_h


