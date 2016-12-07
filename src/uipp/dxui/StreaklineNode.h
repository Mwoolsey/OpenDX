/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#ifndef _StreaklineNode_h
#define _StreaklineNode_h


#include "Node.h"


//
// Class name definition:
//
#define ClassStreaklineNode	"StreaklineNode"

//
// Referenced Classes
//
class NodeDefinition;
class Network;

//
// StreaklineNode class definition:
//				
class StreaklineNode : public Node
{
  private:
    //
    // Private member data:
    //

  protected:
    //
    // Protected member data:
    //

  public:
    //
    // Let the superclass do the real instance number work but
    // this class uses a name string which
    // is unique for each instance.  So when swapping networks, we'll update this
    // param when the instance number changes.
    virtual int assignNewInstanceNumber();

    //
    // Constructor:
    //
    StreaklineNode(NodeDefinition *nd, Network *net, int instnc);

    //
    // Destructor:
    //
    ~StreaklineNode();

    //
    // Determine if this node is a node of the given class
    //
    virtual boolean isA(Symbol classname);

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassStreaklineNode;
    }
};


#endif // _StreaklineNode_h
