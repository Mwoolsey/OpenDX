/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>



#ifndef _NodeList_h
#define _NodeList_h


#include "List.h"
#include "Dictionary.h"


//
// Class name definition:
//
#define ClassNodeList	"NodeList"


//
// Referenced classes:
//
class Node;


class NodeList : public List
{
  private:
    //
    // Private member data:
    //
    Dictionary nodeDict;

  protected:
    List* getSubList(Node*);

  public:
    //
    // Constructor:
    //
    NodeList();

    //
    // Destructor:
    //
    ~NodeList();

    //
    // We always do what the superclass does plus a little extra work for
    // keeping certain node types in sorted order.  This provides a faster
    // lookup.  
    //
    virtual void clear();
    virtual boolean insertElement(const void* , const int);
    virtual boolean replaceElement(const void* , const int);
    virtual boolean deleteElement(const int position);
    virtual List *dup();

    List* makeClassifiedNodeList( const char* );

    const char* getClassName() { return ClassNodeList; }
};


#endif // _NodeList_h 
