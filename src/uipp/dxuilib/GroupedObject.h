/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#ifndef _GroupedObject_h
#define _GroupedObject_h

#include <stdio.h> 

#include "enums.h"

//
// Class name definition:
//
#define ClassGroupedObject	"GroupedObject"

//
// Referenced classes:
//
class Dictionary;
class GroupRecord;

//
// GroupedObject class definition:
//				
class GroupedObject
{

  private:
    //
    // Private member data:
    //


  protected:
    //
    // Protected member data:
    //

    Dictionary		*groups;

    virtual	boolean printGroupComment (FILE *);
    virtual	boolean parseGroupComment 
	(const char* comment, const char* filename, int lineno);

  public:
    //
    // Constructor:
    //
    GroupedObject() { this->groups = NUL(Dictionary*); }

    //
    // Destructor:
    //
    virtual ~GroupedObject(); 

    virtual	void	setGroupName(GroupRecord *, Symbol groupID);
    const char *	getGroupName(Symbol groupID);
    void		addToGroup(const char *group, Symbol groupID);

    virtual	Network *getNetwork() = 0;

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName() { return ClassGroupedObject; }
};


#endif // _GroupedObject_h
