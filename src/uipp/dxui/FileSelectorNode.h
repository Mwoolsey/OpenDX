/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _FileSelectorNode_h
#define _FileSelectorNode_h


#include "ValueNode.h"

class InteractorInstance;
class NodeDefinition;
class Network;

//
// Class name definition:
//
#define ClassFileSelectorNode	"FileSelectorNode"

//
// FileSelectorNode class definition:
//				
class FileSelectorNode : public ValueNode
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
    // Get a new interactor instance for this class.
    // Derived classes can override this to allocated subclasses of 
    // InteractorInstance which may be specific to the derived class.
    //
    virtual	InteractorInstance *newInteractorInstance();
    //
    // Print the file filter comment 
    //
    virtual boolean cfgPrintInstanceAuxInfo(FILE *f, InteractorInstance *ii);
    //
    // Parse the file filter and all the super class's comments.
    //
    virtual boolean cfgParseComment(const char *comment,
			    const char *filename, int lineno);
  public:
    //
    // Constructor:
    //
    FileSelectorNode(NodeDefinition *nd, Network *net, int instnc);

    //
    // Destructor:
    //
    ~FileSelectorNode();

    //
    // We implement this for FileSelector node so that we can keep the
    // 2nd output (the basename of the full file name) up to date.
    // One should never use this method to set the 2nd output, only the 1st.
    //
    Type setOutputValue(int index, const char *value, 
				Type t = DXType::UndefinedType, 
				boolean send = TRUE);

    //
    // Determine if this node is a node of the given class
    //
    virtual boolean isA(Symbol classname);

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassFileSelectorNode;
    }
};


#endif // _FileSelectorNode_h
