/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#include <Xm/Xm.h>
 
#include "XmDX.h"
#include "DXStrings.h"
#include "WorkSpaceGrid.h"
#include "ErrorDialogManager.h"

WorkSpaceGrid::WorkSpaceGrid()
{
    this->setDefaultConfiguration();
}
//
// Use the default grid configuration.
//
void WorkSpaceGrid::setDefaultConfiguration()
{
    this->active = FALSE;
    this->width = this->height = 50;
    this->x_alignment =  this->y_alignment = XmALIGNMENT_NONE;
}

char *WorkSpaceGrid::alignmentString()
{
    char *align = new char[4];

    switch (this->y_alignment) 
    {
      case XmALIGNMENT_BEGINNING:
	align[0] = 'U';
        break;
      case XmALIGNMENT_CENTER:
	align[0] = 'C';
        break;
      case XmALIGNMENT_END:
	align[0] = 'L';
        break;
      case XmALIGNMENT_NONE:
	align[0] = 'N';
        break;
    }
    switch (this->x_alignment) 
    {
      case XmALIGNMENT_BEGINNING:
	align[1] = 'L';
        break;
      case XmALIGNMENT_CENTER:
	align[1] = 'C';
        break;
      case XmALIGNMENT_END:
	align[1] = 'R';
        break;
      case XmALIGNMENT_NONE:
	align[1] = 'N';
        break;
    }
    align[2] = '\0';

    return align;
}
void WorkSpaceGrid::parseAlignment(const char *align)
{

    switch(align[0])
    {
      case 'U':
        this->y_alignment = XmALIGNMENT_BEGINNING;
        break;
      case 'C':
        this->y_alignment = XmALIGNMENT_CENTER;
        break;
      case 'L':
        this->y_alignment = XmALIGNMENT_END;
        break;
      case 'N':
        this->y_alignment = XmALIGNMENT_NONE;
        break;
      case '\0':
        this->y_alignment = XmALIGNMENT_CENTER;
        this->x_alignment = XmALIGNMENT_CENTER;
	return;
    }
    switch(align[1])
    {
      case 'L':
        this->x_alignment = XmALIGNMENT_BEGINNING;
        break;
      case 'C':
      case '\0':
        this->x_alignment = XmALIGNMENT_CENTER;
        break;
      case 'R':
        this->x_alignment = XmALIGNMENT_END;
        break;
      case 'N':
        this->x_alignment = XmALIGNMENT_NONE;
        break;
    }
}

//
// Parse a control panel's 'layout' comment.
//
boolean WorkSpaceGrid::printComments(FILE *f)
{
    char *s = this->getCommentString();
    boolean r;
 
    if (!s)
	return TRUE;

    if (fputs(s,f) < 0) 
	r = FALSE;
    else 
	r = TRUE;

    if (s) delete s;

    return r;
}
//
// Parse a control panel's 'layout' comment.
// The returned string must be deleted by the caller
//
char *WorkSpaceGrid::getCommentString()
{
    int w, h;

    char *r = new char[96];
    char *s = this->alignmentString();
    this->getSpacing(w,h);
    sprintf(r, "// layout: snap = %d, width = %d, height = %d, align = %s\n",
               (this->isActive() ? 1 : 0),
               w,
               h,
               s);

    if (s) delete s;

    return r;

}
//
// Parse a control panel's 'layout' comment.
//
boolean WorkSpaceGrid::parseComment(const char *comment,
                                const char *filename, int lineno)
{
    int      items_parsed;
    int      snap;
    int      width;
    int      height;
    char     alignment[8];

    if (!EqualSubstring(comment+1,"layout",6))
        return FALSE;

    items_parsed =
        sscanf(comment,
               " layout: snap = %d, width = %d, height = %d, align = %[^\n]",
               &snap,
               &width,
               &height,
               alignment);
    /*
     * If not parsed correctly, error.
     */
    if (items_parsed != 4)
    {
#if 0
        (_parse_widget, "#10001", "grid", _parse_file, yylineno);
#else
        ErrorMessage("Bad 'layout' panel comment (file %s, line %d)",
                                filename, lineno);
#endif
        return TRUE;
    }

    /*
     * Save the grid information for later use.
     */
    this->setActive(snap > 0);
    this->setSpacing(width, height);
    this->parseAlignment(alignment);
    
    return TRUE;
}

