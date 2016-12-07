/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _GlobalLocalNode_h
#define _GlobalLocalNode_h

//
// This class of Node is implemented to address a design problem with the
// Get and Set modules that went out in versions 3.1.0 - 3.1.2.  The fix
// is to replace them with GetLocal, GetGlobal, and SetLocal, SetGlobal.
// To make things easier for your users, we will try and automatically
// convert Get and Set nodes (which are assumed to have the same inputs
// as the new nodes), by setting a flag (per the user's desires) and then
// printing the name of the node accordingly.  So, all this node really
// does is change the name to include the Local or Global suffix depending
// on the setting of a flag.
//


#include "Node.h"


//
// Class name definition:
//
#define ClassGlobalLocalNode	"GlobalLocalNode"

//
// Referenced Classes
//
class NodeDefinition;
class Network;
class EditorWindow;

//
// GlobalLocalNode class definition:
//				
class GlobalLocalNode : public Node
{
     friend class EditorWindow; // Actually, I only want convertGetAndSetNodes() to
				// be able to call setAsLocal/GlobalNode().
  private:
    //
    // Private member data:
    //
    boolean isGlobal;
    Symbol  myNodeNameSymbol;

    void clearMyNodeName();
    void setMyNodeNameIfNecessary();


  protected:
    //
    // Protected member data:
    //

    void setAsLocalNode();
    void setAsGlobalNode();
    void markForResend();

    boolean isLocalNode(); 
    boolean isGlobalNode();

  public:
    //
    // Constructor:
    //
    GlobalLocalNode(NodeDefinition *nd, Network *net, int instnc);

    //
    // Destructor:
    //
    ~GlobalLocalNode();
  
    //
    // Mangle the name if this is a local or global version of this node.
    //
    virtual Symbol getNameSymbol();

    //
    // The name string is normally returned by NodeDefinition however nodes
    // of this class will not all return the same string.
    //
    virtual const char *getExecModuleNameString();

    //
    // Determine if this node is a node of the given class
    //
    virtual boolean isA(Symbol classname);

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassGlobalLocalNode;
    }
};


#endif // _GlobalLocalNode_h
