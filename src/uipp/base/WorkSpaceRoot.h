/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _WorkSpaceRoot_h
#define _WorkSpaceRoot_h

#include <Xm/Xm.h>

#include "WorkSpace.h"
#include "List.h"
#include "SymbolManager.h"


//
// Class name definition:
//
#define ClassWorkSpaceRoot "WorkSpaceRoot"


//
// How does it work?:
// Root defines the behaviors of a workspace which holds other workspaces.  The
// most important parts are 1) a List of of pages, 2) routines to select and
// show members from the list.  WorkSpace classes which are directly instantiated
// will multiply inherit using WorkSpace and WorkSpace{Root,Page}.  That way there
// are 2 distinct types of WorkSpaces and window will work with, but both support
// exactly the same set of methods for doing real work.  They differ only in
// how they handle paging.  So in all pre-existing situations it isn't necessary
// for the window class to query the workspace to determine its type.
//

#define UNDETERMINED_PAGE -1

//
// WorkSpace class definition:
//				
class WorkSpaceRoot : public Base
{
  private:

    int current_page;

    //
    // A list of pages belonging to this WorkSpace
    //
    List page_list;

  protected:

    WorkSpaceRoot() { 
	this->current_page = UNDETERMINED_PAGE;
    }

    //
    // Destructor:
    //
    ~WorkSpaceRoot(); 
  
    //
    // Command for creating a new page.
    //
    virtual WorkSpace *newPage (int width, int height) = 0;

    //
    // Should translate to WorkSpace::getXYSize(int*, int*)
    //
    virtual void getObjectXYSize (int*, int*) = 0;

    //
    // Should access WorkSpace::activeManyPlacements in 
    // order to keep pages and root in sync
    //
    virtual int  getPlacementCount() = 0;
    virtual void setPlacementCount(int) = 0;

    //
    // Access WorkSpace::{wasAllowOverlap,wasLineDrawingEnabled}
    //
    virtual boolean getLineDrawingEnabled() = 0;
    virtual boolean getOverlapEnabled() = 0;

    //
    // Access the Workspace widget of WorkSpace
    //
    virtual Widget getWorkSpaceWidget() = 0;

    //
    // Set width/height of the WorkSpace
    //
    virtual void setDimensions(int, int) = 0;

    //
    // Share WorkSpaceInfo with pages
    //
    virtual WorkSpaceInfo* getWorkSpaceInfo() = 0;

    //
    // Keep cursors in sync among the pages.
    //
    virtual void setPageCursor(int cursorType);
    virtual void resetPageCursor();

  public:
    //
    // public method for adding a page... should not be overridden
    //
    WorkSpace* addPage();

    //
    // page list manipulation
    //
    int 	getPageCount() 		{ return this->page_list.getSize(); }
    int 	getCurrentPage() 	{ return this->current_page; }
    WorkSpace* 	getElement(int page)
	{ return (WorkSpace*)this->page_list.getElement(page); }

    virtual void showWorkSpace(int page);
    virtual void showWorkSpace(WorkSpace* page);
    void showRoot();

    virtual void resizeWorkSpace();

    //
    // Install the new grid in the WorkSpace
    //
    virtual void installPageInfo(WorkSpaceInfo *);

    //
    // Handle the page aspects of quieting down
    //
    virtual void beginManyPagePlacements();
    virtual void   endManyPagePlacements();

    //
    // This happens after a page is deleted
    //
    void removePage(const void* ws);

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassWorkSpaceRoot;
    }

    virtual boolean isA (Symbol classname);
    boolean isA (const char* classname);
};


#endif // _WorkSpaceRoot_h
