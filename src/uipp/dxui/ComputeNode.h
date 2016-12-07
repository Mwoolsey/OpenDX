/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#ifndef _ComputeNode_h
#define _ComputeNode_h



#include "Node.h"


//
// Class name definition:
//
#define ClassComputeNode	"ComputeNode"

//
// Referenced Classes
class Network;

//
// ComputeNode class definition:
//				
class ComputeNode : public Node
{
  private:
    //
    // Private member data:
    //

  protected:
    //
    // Protected member data:
    //
    boolean	 exposedExpression;
    char	*expression;
    List	 nameList;

    char *resolveExpression();

    virtual boolean netPrintAuxComment(FILE *f);
    //
    // Add/remove a set of repeatable input or output parameters to the
    // this node.   An error  ocurrs if the parameter list indicated does
    // not have repeatable parameters.
    //
    virtual boolean addRepeats(boolean input);
    virtual boolean removeRepeats(boolean input);

  public:
    //
    // Constructor:
    //
    ComputeNode(NodeDefinition *nd, Network *net, int instnc);

    //
    // Destructor:
    //
    ~ComputeNode();

    virtual boolean initialize();

    void setExpression(const char *expr, boolean send = TRUE);
    const char *getExpression();
    void setName(const char *name, int i, boolean send = TRUE);
    const char *getName(int i);
    virtual boolean netParseAuxComment(const char *comment,
			 const char *file,
			 int lineno);

    //
    // A mechanism whereby StandIn::printPostScriptPage() can get a little extra
    // text to put onto the printed page.  It had always seemed unfortunate that
    // a printed-as-postscript net couldn't show the compute expression.
    //
    virtual const char* getExtraPSText() { return this->getExpression(); }

    //
    // Determine if this node is a node of the given class
    //
    virtual boolean isA(Symbol classname);

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassComputeNode;
    }
};


#endif // _ComputeNode_h
