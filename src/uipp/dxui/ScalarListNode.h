/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#ifndef _ScalarListNode
#define _ScalarListNode


#include "ScalarNode.h"
#include "StepperInteractor.h"
#include "DXValue.h"

class Network;
class ComponentAttributes;

//
// Class name definition:
//
#define ClassScalarListNode	"ScalarListNode"

//
// ScalarListNode class definition:
//				
class ScalarListNode : public ScalarNode 
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
    // Adjusts the dimensionality of all outputs for
    // this->changeDimensionality().
    //
    virtual boolean adjustOutputDimensions(int old_dim, int new_dim);

    //
    // Get a ScalarListInstance instead of an ScalarInstance.
    //
    virtual InteractorInstance	*newInteractorInstance();

    // The messages are any that a ScalarNode can handle plus 
    //
    // 	'list={%g %g ...}' 
    //
    // or 
    //
    // 	'list={ [ %g %g ... ], ...}'
    //
    // Returns the number of message blocks we found.
    //
    virtual int  handleInteractorMsgInfo(const char *line);
    //
    // Handle
    //
    //              'list={%g, %g...}'
    //
    // Return the number of tokens handled.
    //
    int handleScalarListMsgInfo(const char *line);
    //
    // Handle the vector list message
    //
    //              'list={ [ %g %g ... ], ...}'
    //
    // Return the number of tokens handled.
    //
    int handleVectorListMsgInfo(const char *line);

    //
    // Do what ever is necessary when the given component of the output
    // value is out of range as indicated by the limits.  Typically, this just
    // means resetting the current output value that is associated with the
    // node.  ScalarListNodes however, have the current (non-output) value
    // associated with the interactor instance. Therefore, ScalarListNodes,
    // re-implement this to reset the component values of all their instances.
    // If 'component' is less than 0, then min/max are ignored and all
    // components are checked with the current min/max values.
    //
    virtual void doRangeCheckComponentValue(int component, 
						double min, double max);

  public:
    //
    // Constructor:
    //
    ScalarListNode(NodeDefinition *nd, Network *net, int instnc, 
					boolean isvector, int dimensions); 

    //
    // Destructor:
    //
    ~ScalarListNode(){}

    //
    // Called once for each instance 
    //
    virtual boolean initialize();

    //
    // Determine if this node is a node of the given class
    //
    virtual boolean isA(Symbol classname);

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassScalarListNode;
    }
};


#endif // _ScalarListNode
