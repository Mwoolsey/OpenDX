/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _WorkSpaceGrid_h
#define _WorkSpaceGrid_h


#include "Base.h"


//
// Class name definition:
//
#define ClassWorkSpaceGrid	"WorkSpaceGrid"


//
// WorkSpaceGrid class definition:
//				
class WorkSpaceGrid : public Base
{
  //friend class GridDialog;
  friend class WorkSpaceInfo;

  private:
    //
    // Private member data:
    //
    boolean             active;         /* grid active?                 */
    short           width;              /* horizontal spacing value     */
    short           height;             /* vertical spacing value       */
    unsigned char   x_alignment;	/* horizontal alignment         */
    unsigned char   y_alignment;	/* vertical alignment           */

  protected:
    //
    // Protected member data:
    //

    void setActive(boolean set)      { this->active = set; }
    void setSpacing(int w, int h)    { this->width = w; this->height = h; }
    void setAlignment(int x, int y)  {  this->x_alignment = x; 
					this->y_alignment = y; }
    void parseAlignment(const char *align);
    char *alignmentString();

    //
    // Use the default grid configuration.
    //
    void setDefaultConfiguration();

  public:
    //
    // Constructor:
    //
    WorkSpaceGrid();

    //
    // Destructor:
    //
    ~WorkSpaceGrid(){}


    boolean isActive()                  { return this->active; }
    void    getSpacing(int& w, int& h)  { w = this->width; h = this->height; }

    char *  getCommentString();
    boolean printComments(FILE *f);
    boolean parseComment(const char *comment, 
				const char *filename, int lineno);

    //
    // Returns one of XmALIGNMENT_BEGINNING, XmALIGNMENT_CENTER,
    // XmALIGNMENT_END or XmALIGNMENT_NONE
    //
    unsigned int  getXAlignment() { return this->x_alignment; }
    unsigned int  getYAlignment() { return this->y_alignment; }

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassWorkSpaceGrid;
    }
};


#endif // _WorkSpaceGrid_h
