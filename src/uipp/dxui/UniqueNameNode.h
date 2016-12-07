/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#ifndef _UniqueNameNode_h
#define _UniqueNameNode_h

#include <stdio.h> 
#include <Xm/Xm.h> 

#include "Node.h"


//
// Class name definition:
//
#define ClassUniqueNameNode	"UniqueNameNode"

//
// Referenced classes:
//

//
// Node class definition:
//				
class UniqueNameNode : public Node 
{

  private:
    //
    // Private member data:
    //

  protected:
    //
    // Protected member data:
    //

    //
    // Constructor:
    //
    UniqueNameNode(NodeDefinition *nd, Network *net, int inst) :
	Node (nd, net, inst) {} ;

  public:
    //
    // Destructor:
    //
    ~UniqueNameNode(){}; 

    //
    // Determine if this node is a node of the given class
    //
    virtual boolean isA(Symbol classname);

    virtual const char*		getUniqueName() { return this->getLabelString(); }

    virtual boolean		namesConflict
	(const char* his_label, const char* my_label, const char* his_classname);

    virtual boolean needsFastSort() { return TRUE; }

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName() { return ClassUniqueNameNode; }
};


#endif // _UniqueNameNode_h
