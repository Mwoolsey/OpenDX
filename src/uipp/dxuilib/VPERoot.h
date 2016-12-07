/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _VPERoot_h
#define _VPERoot_h

#include "EditorWorkSpace.h"
#include "WorkSpaceRoot.h"

extern "C" Boolean VPERoot_MoveWindowWP (XtPointer);
extern "C" Boolean VPERoot_ShowWindowWP (XtPointer);

//
// Class name definition:
//
#define ClassVPERoot	"VPERoot"

class PageSelector;
class UndoGrid;

//
// EditorWorkSpace class definition:
//				
class VPERoot : public EditorWorkSpace, public WorkSpaceRoot
{
  private:

    PageSelector* selector;
    XtWorkProcId  show_wpid;
    XtWorkProcId  move_wpid;
    EditorWorkSpace* to_be_shown;


  protected:

    virtual WorkSpace *newPage (int width, int height);

    //
    // D E F I N E    R O O T    P A G E    B E H A V I O R
    // D E F I N E    R O O T    P A G E    B E H A V I O R
    // (pure virtual from WorkSpaceRoot)
    //
    virtual void getObjectXYSize (int* w, int* h) {
	this->EditorWorkSpace::getXYSize(w,h);
    }

    virtual int getPlacementCount() { 
	return this->EditorWorkSpace::getPlacementCount();
    }

    virtual void setPlacementCount(int count) {
	this->EditorWorkSpace::setPlacementCount(count);
    }

    virtual boolean getLineDrawingEnabled() {
	return this->EditorWorkSpace::getLineDrawing();
    }

    virtual boolean getOverlapEnabled() {
	return this->EditorWorkSpace::getOverlap();
    }

    virtual Widget getWorkSpaceWidget() {
	return this->EditorWorkSpace::getRootWidget();
    }

    virtual void setDimensions(int width, int height) {
	this->EditorWorkSpace::setWidthHeight (width, height);
    }

    virtual WorkSpaceInfo* getWorkSpaceInfo() {
	return this->EditorWorkSpace::getInfo();
    }

    friend Boolean VPERoot_MoveWindowWP (XtPointer);
    friend Boolean VPERoot_ShowWindowWP (XtPointer);
    friend class UndoGrid;

  public:

    //
    // Constructor:
    //
    VPERoot(const char *name, Widget parent, 
	WorkSpaceInfo *info, EditorWindow *editor, PageSelector* selector) : 
	EditorWorkSpace(name, parent, info, editor), WorkSpaceRoot() {
	this->selector = selector;
	this->move_wpid = NUL(XtWorkProcId);
	this->show_wpid = NUL(XtWorkProcId);
	this->to_be_shown = NUL(EditorWorkSpace*);
    }
 
    //
    // Destructor:
    //
    ~VPERoot();

    //
    // D E F I N E    R O O T    P A G E    B E H A V I O R
    // D E F I N E    R O O T    P A G E    B E H A V I O R
    //
    virtual void setCursor(int cursorType) {
	this->EditorWorkSpace::setCursor(cursorType);
	this->WorkSpaceRoot::setPageCursor(cursorType);
    }

    virtual void resetCursor() {
	this->EditorWorkSpace::resetCursor();
	this->WorkSpaceRoot::resetPageCursor();
    }

    virtual void resize() {
	this->WorkSpaceRoot::resizeWorkSpace();
    }

    virtual void showWorkSpace (int);
    virtual void showWorkSpace (WorkSpace* ws) { 
	this->WorkSpaceRoot::showWorkSpace(ws); 
    }

    virtual void installInfo (WorkSpaceInfo* info);

    virtual void beginManyPlacements () {
	this->EditorWorkSpace::beginManyPlacements();
	this->WorkSpaceRoot::beginManyPagePlacements();
    }

    virtual void endManyPlacements () {
	this->EditorWorkSpace::endManyPlacements();
	this->WorkSpaceRoot::endManyPagePlacements();
    }

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName() { return ClassVPERoot; }
    virtual boolean isA (Symbol classname);
    boolean isA (const char* classname);
};


#endif // _VPERoot_h
