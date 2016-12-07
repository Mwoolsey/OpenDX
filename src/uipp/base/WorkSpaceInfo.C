/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#include "DXStrings.h"
#include "WorkSpace.h"
#include "WorkSpaceInfo.h"

#define MIN_WIDTH 500
#define MIN_HEIGHT 400

WorkSpaceInfo::WorkSpaceInfo()
{
    this->workSpace = NULL;
    this->setDefaultConfiguration();
}
//
// Go back to using the default settings.
//
void WorkSpaceInfo::setDefaultConfiguration()
{
    this->width  = MIN_WIDTH;
    this->height = MIN_HEIGHT;
    this->prevent_overlap = FALSE;
    this->grid.setDefaultConfiguration();

	if (this->workSpace) {
		this->workSpace->resize();
		this->workSpace->installInfo(this);
	}
}
//
// Parse a work space comment and return the information within.
//
boolean WorkSpaceInfo::printComments(FILE *f)
{
    //
    // Print workspace information
    //
    int w, h;

    if(this->workSpace)
	this->workSpace->getMaxWidthHeight(&w, &h);
    else
	this->getXYSize(&w, &h);
    if (fprintf(f,"// workspace: width = %d, height = %d\n", w,h) < 0)
	return FALSE;

    return this->grid.printComments(f);
}

//
// Parse a work space comment and return the information within.
//
boolean WorkSpaceInfo::parseComment(const char* comment, 
						const char *file, int lineno)
{
    int items_parsed;
    int w;
    int h;

    if (this->grid.parseComment(comment,file,lineno))
	return TRUE;

    if (!EqualSubstring(&comment[1],"workspace:", 10))
	return FALSE;

    items_parsed = 
	sscanf(comment, " workspace: width = %d, height = %d", &w, &h);

    if (items_parsed != 2)
        return FALSE;

    if (w > MIN_WIDTH)		// Don't allow smaller than the default
	this->width = w;

    if (h > MIN_HEIGHT)		// Don't allow smaller than the default
	this->height = h;

    return TRUE;
}
void WorkSpaceInfo::getXYSize(int *w, int *h)
{
    WorkSpace *ws = this->workSpace;

    if (ws && ws->getRootWidget()) {
	ws->getXYSize(w,h);
	this->width = *w;
	this->height = *h;
   } else {
	*w = this->width;
	*h = this->height;
   }
}
