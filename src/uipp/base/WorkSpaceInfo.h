/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _WorkSpaceInfo_h
#define _WorkSpaceInfo_h


#include "Base.h"
#include "WorkSpaceGrid.h"

//
// Class name definition:
//
#define ClassWorkSpaceInfo	"WorkSpaceInfo"

class WorkSpaceRoot;
class WorkSpace;
class UndoGrid;

//
// WorkSpaceInfo class definition:
//				
class WorkSpaceInfo : public Base
{
     friend class WorkSpace;
     friend class WorkSpaceRoot;
     friend class ControlPanel;
     friend class EditorWindow;
     friend class Network;
     friend class GridDialog;

  private:
    //
    // Private member data:
    //
    WorkSpaceGrid grid;
    int 	width; 
    int		height;
    WorkSpace	*workSpace;	// Set when referenced by a WorkSpace
    boolean	prevent_overlap;

  protected:
    //
    // Protected member data:
    //

    void setWidth(int val)    { this->width = val; }
    void setHeight(int val)   { this->height = val; }
    void setGridActive(boolean val = TRUE) { this->grid.setActive(val); }
    void setGridSpacing(int w, int h) { this->grid.setSpacing(w, h); }
    void setGridAlignment(int x, int y) { this->grid.setAlignment(x,y); }
    void setPreventOverlap(boolean val=TRUE) { this->prevent_overlap = val; }
    void associateWorkSpace(WorkSpace *ws) { this->workSpace = ws; }
    void disassociateWorkSpace() { this->workSpace = NULL; }

    void setDefaultConfiguration();

    // ouch, what a hack
    friend class UndoGrid;

  public:
    //
    // Constructor:
    //
    WorkSpaceInfo();

    //
    // Destructor:
    //
    ~WorkSpaceInfo(){}

    boolean printComments(FILE *f);
    boolean parseComment(const char *comment, const char *filename,int lineno);
    
    int getWidth()    { return this->width; }
    int getHeight()   { return this->height; }

    boolean isGridActive() { return this->grid.isActive(); }
    void    getGridSpacing(int& w, int& h) { this->grid.getSpacing(w,h); }
    boolean getPreventOverlap() { return this->prevent_overlap; }

    //
    // Returns one of XmALIGNMENT_BEGINNING, XmALIGNMENT_CENTER or
    // XmALIGNMENT_END
    //
    unsigned int  getGridXAlignment() { return this->grid.getXAlignment(); }
    unsigned int  getGridYAlignment() { return this->grid.getYAlignment(); }
   
    void getXYSize(int *w, int *h);

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassWorkSpaceInfo;
    }
};


#endif // _WorkSpaceInfo_h
