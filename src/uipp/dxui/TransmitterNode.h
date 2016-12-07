/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#ifndef _TransmitterNode_h
#define _TransmitterNode_h


#include "UniqueNameNode.h"


//
// Class name definition:
//
#define ClassTransmitterNode	"TransmitterNode"

//
// Referenced Classes

//
// TransmitterNode class definition:
//				
class TransmitterNode : public UniqueNameNode
{
  private:
    //
    // Private member data:
    //

  protected:
    //
    // Protected member data:
    //
    virtual char *netNodeString(const char *prefix);


  public:
    //
    // Constructor:
    //
    TransmitterNode(NodeDefinition *nd, Network *net, int instnc);

    //
    // Destructor:
    //
    ~TransmitterNode();

    virtual boolean initialize();

    boolean setLabelString(const char *label);

    //
    // Determine if this node is a node of the given class
    //
    virtual boolean isA(Symbol classname);

    //
    // Switch the node from one net to another.  Resolve any name 
    // space collisions.
    //
    void switchNetwork(Network *from, Network *to, boolean silently=FALSE);

    //
    // Check for name conflicts.  Usually you disallow 2 nodes with the same
    // labelString, but it certain cases it's desirable.  Capture that logic
    // locally.
    //
    virtual boolean namesConflict (const char* his_label, const char* my_label,
	const char* his_classname);


    //
    // Returns a pointer to the class name.
    //
    virtual const char* getClassName()
    {
	return ClassTransmitterNode;
    }
};


#endif // _TransmitterNode_h
