/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _MacroParameterNode_h
#define _MacroParameterNode_h


#include "UniqueNameNode.h"


//
// Class name definition:
//
#define ClassMacroParameterNode	"MacroParameterNode"

//
// Referenced Classes
class Network;
class ParameterDefinition;
class Ark;

//
// MacroParameterNode class definition:
//				
class MacroParameterNode : public UniqueNameNode
{
  private:
    //
    // Private member data:
    //

    //
    // Ensure the option values for this are of a type that is safe
    // given the nodes type.  If the node has no option values, see
    // if it's possible to get some from the next node downstream.
    //
    void setTypeSafeOptions(Ark* pref=NULL);

    boolean canCoerceValue (const char* option, List* types);

  protected:
    //
    // Protected member data:
    //
    int index;		// Which (1-based) of the networks parameters are we?

    virtual boolean  netParseAuxComment(const char* comment,
						const char *file,
						int lineno);

    virtual char *netNodeString(const char *prefix);
    virtual boolean netPrintAuxComment(FILE *f);

    virtual boolean addIOArk(List *io, int index, Ark *a);
    virtual boolean removeIOArk(List *io, int index, Ark *a);

  public:
    //
    // Constructor:
    //
    MacroParameterNode(NodeDefinition *nd, Network *net, int instnc);

    //
    // Destructor:
    //
    ~MacroParameterNode();

    virtual boolean initialize();

    //
    // Match output_index of this node to input_index of n.  Returns TRUE
    // if they can connect.
    //
    virtual boolean typeMatchOutputToInput(
	int output_index,
	Node *n,
	int input_index);

    virtual const char *getParameterNameString();
    virtual ParameterDefinition *getParameterDefinition(
			boolean includeDummies = TRUE);

    virtual void setIndex(int index);
    virtual int getIndex() { return this->index; }
    virtual boolean moveIndex(int index, boolean issue_error = TRUE);

    //
    // Determine if this node is a node of the given class
    //
    virtual boolean isA(Symbol classname);

    //
    // Determine if this node an input or output
    // Should not be called until after initialization.
    //
    boolean isInput();

    //
    // Switch the node's net from 'from' to 'to'
    //
    virtual void switchNetwork(Network *from, Network *to, boolean silently=FALSE);
    virtual boolean canSwitchNetwork(Network *from, Network *to);

    //
    // Pure virtual from UniqueNameNode so that Network can watch for name
    // conflicts for us.
    //
    virtual const char* getUniqueName();

    //
    // Returns a pointer to the class name.
    //
    virtual const char* getClassName()
    {
	return ClassMacroParameterNode;
    }
};


#endif // _MacroParameterNode_h
