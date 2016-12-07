/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _StringTable_h
#define _StringTable_h


#include "Base.h"

class List;

//
// Class name definition:
//
#define ClassStringTable	"StringTable"


//
// StringTable class definition:
//				
class StringTable : public Base 
{
  private:
     List *lists;
     int  size;

  public:
    //
    // Constructor:
    //
    StringTable();

    //
    // Destructor:
    //
    ~StringTable();

    //
    // Clears the symbol table of its entries.  All entries are deleted.
    //
    //
    void clear();

    //
    // Adds a string entry into the table, if it is not already a member.
    // Returns TRUE if successful, FALSE otherwise.  In either case, the
    // unique id of the string is returned.
    //
    boolean addString(const char* string, long& id);

    //
    // Searches for the specified string in the table.
    // If the string is found, its id value is returned;
    // otherwise a value of zero is returned.
    //
    int findString(const char* string);

    //
    // Retrieves the string value corresponding to the specified id 
    // value.  If the specified id is invalid, NULL is returned.
    //
    const char* getString(int id);

    //
    // Returns the current size of the table.
    //
    int getSize()
    {
	return this->size;
    }

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassStringTable;
    }
};


#endif // _StringTable_h
