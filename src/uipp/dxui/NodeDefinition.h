/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#ifndef _NodeDefinition_h
#define _NodeDefinition_h


#include <Xm/Xm.h>

#include "DXStrings.h"
#include "Dictionary.h"
#include "Definition.h"
#include "SIAllocatorDictionary.h" 	// For SIAllocator typedef 
#include "CDBAllocatorDictionary.h" 	// For CDBAllocator typedef 
#include "Cacheability.h"

//
// Class name definition:
//
// 
#define ClassNodeDefinition	"NodeDefinition"

//
// Forward definitions 
//
class Node;
class Definition;
class Network;
class StandIn;
class ConfigurationDialog;
class Dictionary;
class ParameterDefinition;
class Parameter;

extern Dictionary *theNodeDefinitionDictionary;

//
// NodeDefinition class definition:
//				
class NodeDefinition : public Definition 
{
  private:
    //
    // Private member data:
    //

    boolean uiLoadedOnly;	// Was this definition only loaded by the ui 

  protected:
    //
    // Protected member data:
    //
    List	inputDefs;	// List of input parameter definitions.
    List	outputDefs; 	// List of output parameter definitions.
    Symbol	category;
    long	mdf_flags;	// Bits representing FLAGS statement in mdf.
    char	*description;	// A short description
    int		input_repeats;	// The last n inputs are repeatable
    int		output_repeats;	// the last n outputs are repeatable
    char	*loadFile;	// Name of file containing loadable object
    char	*outboardCommand;	// Name of executable file.	
    char	*outboardHost;	

    boolean	userTool;  	// TRUE if defined by a user .mdf file and 
			   	// defaults to TRUE 

    boolean	writeableCacheability;   // Is the cacheability of this type of
					// node modifyable
    Cacheability defaultCacheability;    // Is this module cached by default

    virtual boolean addIODef(List *l, ParameterDefinition *pd) 
	    	{ return l->appendElement((void*)pd); }

    //
    // Increment this number to generate numbers for instances
    // of the type that is being defined.
    //
    int		nodeInstanceNumbers;	

    //
    // Get the dictionary for this class.  
    // Must be defined by the derived classes
    //
      
    //
    // Returns the StandIn allocator for this node. 
    //
    virtual SIAllocator getSIAllocator();

    //
    // Returns the Configuration dialog box allocator for this node. 
    //
    virtual CDBAllocator getCDBAllocator();

    //
    // Allocate a new Node of the corresponding type.
    //
    virtual Node *newNode(Network *net, int instance = -1); 

    //
    // Get the MDF header (stuff before the INPUT/OUTPUT statements) for this
    // NodeDefinition.
    // The returned string must be delete by the caller.
    //
    char *getMDFHeaderString();
    //
    // Get the list of INPUT/OUTPUT lines in the MDF and the REPEAT if any
    // The returned string must be delete by the caller.
    //
    char *getMDFParametersString(boolean inputs);

    //
    // Set/clear one of the this->mdf_flag bits.
    //
    void setMDFFlag(boolean val, long flag);


  public:
    //
    // Reset all the instance numbers in a dictionary of definitions. 
    //
    static void ResetInstanceNumbers(Dictionary *d);

    //
    // Allocate a new NodeDefinition.
    // This function is intended to be used as an allocator in
    // theNDAllocatorDictionary.
    //
    static NodeDefinition *AllocateDefinition();

    //
    // Constructor:
    //
    NodeDefinition(); 

    //
    // Destructor:
    //
    ~NodeDefinition();

    virtual boolean isDerivedFromMacro();
    
    //
    // Instance number manipulations 
    //
    int 	newInstanceNumber() { return ++nodeInstanceNumbers; }
    void	setNextInstance(int inst) { nodeInstanceNumbers = inst - 1; } 
    int		getCurrentInstance() { return this->nodeInstanceNumbers; }

    //
    // Add an input or output to the list of i/o definitions 
    // (makes a copy of *pd).
    //
    boolean addInput(ParameterDefinition *pd) 
		{ return addIODef(&inputDefs, pd); };
    boolean addOutput(ParameterDefinition *pd) 
		{ return addIODef(&outputDefs, pd); };

    //
    // Get the number of base (unrepeated) inputs/outputs for this node. 
    //
    int	getInputCount() { return this->inputDefs.getSize(); }
    int	getOutputCount() { return this->outputDefs.getSize(); }


    //
    // Get the Nth Input or Output ParameterDefinition.
    // n is indexed from 1.  If n is greater than the number of input/output 
    // defintions then it is a repeatable parameter, in which case we 
    // account for this to find the correct definition.
    //
    ParameterDefinition *getInputDefinition(int n);
    ParameterDefinition *getOutputDefinition(int n);

    //
    // Set/Get the repeat count for inputs/outputs 
    // FIXME: should the set*() be protected/friends.
    //
    void setInputRepeatCount(int n)	{ input_repeats = n; }    
    int  getInputRepeatCount()		{ return input_repeats ; }    
    void setOutputRepeatCount(int n)	{ output_repeats = n; }    
    int  getOutputRepeatCount()		{ return output_repeats ; }    
    boolean isOutputRepeatable() 	{ return output_repeats != 0; }    
    boolean isInputRepeatable()	 	{ return input_repeats != 0; }    

    //
    // Manipulate the cacheability of the node 
    //
    void        setWriteableCacheability(boolean v) 
				{ this->writeableCacheability= v; }
    boolean     hasWriteableCacheability() 
				{ return this->writeableCacheability;}
    void        setDefaultCacheability(Cacheability c) 
				{ this->defaultCacheability = c; }
    Cacheability getDefaultCacheability() 
				{ return this->defaultCacheability; }

    //
    // Set/Get the category for this node 
    //
    void   setCategory(Symbol c) { category = c ; }
    Symbol getCategorySymbol() { return this->category; }
    const char *getCategoryString() 
		{ return theSymbolManager->getSymbolString(this->category); }

    //
    // Returns the name of executive module that is called to do the work
    // for this type of node.
    // Be default, that name is just the name of the Node.
    //
    virtual const char *getExecModuleNameString();

    //
    // Set/Get short description for this node. 
    //
    void setDescription(const char* d);
    const char *getDescription() 
		{ return (description ? (const char*)description : "") ;  }

    //
    // Create a new Node (not NodeDefinition) of this type. 
    // Node initialize() is done here and any other boiler plate as required.
    //
    virtual Node *createNewNode(Network *net, int instance = -1); 

    //
    // Called after the NodeDefinition has been parsed out of the mdf file
    //
    virtual void finishDefinition() {}

    virtual boolean isAllowedInMacro() { return TRUE; }

    //
    // Returns a pointer to the class name.
    // This is an abstract class, enforce it by making this pure virtual.
    //
    virtual const char* getClassName() { return ClassNodeDefinition; }

    //
    // Get a new parameter for the node n that corresponds to this node 
    // definition. index is the one based index of the input/output parameter.
    //
    virtual Parameter *newParameter(ParameterDefinition *pd, 
						Node *n, int index);

    void setMDFFlagERR_CONT(boolean val = TRUE);
    boolean isMDFFlagERR_CONT();
    void setMDFFlagSWITCH(boolean val = TRUE);
    boolean isMDFFlagSWITCH();
    void setMDFFlagPIN(boolean val = TRUE);
    boolean isMDFFlagPIN();
    void setMDFFlagSIDE_EFFECT(boolean val = TRUE);
    boolean isMDFFlagSIDE_EFFECT();
    void setMDFFlagASYNCHRONOUS(boolean val = TRUE);
    boolean isMDFFlagASYNCHRONOUS();
    void setMDFFlagPERSISTENT(boolean val = TRUE);
    boolean isMDFFlagPERSISTENT();
    void setMDFFlagREACH(boolean val = TRUE); 
    boolean isMDFFlagREACH();
    void setMDFFlagREROUTABLE(boolean val = TRUE);
    boolean isMDFFlagREROUTABLE();
    void setMDFFlagLOOP(boolean val = TRUE);
    boolean isMDFFlagLOOP();

				
    boolean isOutboard(); 
    void setDefaultOutboardHost(const char *host);
    const char *getDefaultOutboardHost() { return this->outboardHost;}
    void setOutboardCommand(const char *comand);
    const char *getOutboardCommand() { return this->outboardCommand;}

    boolean isDynamicallyLoaded(); 
    void setDynamicLoadFile(const char *file);
    const char *getDynamicLoadFile();

    //
    // Get the MDF record for this Node definition.
    // The returned string must be delete by the caller.
    //
    char *getMDFString();

    boolean isUserTool() { return this->userTool; }
    void setUserTool(boolean setting = TRUE) { this->userTool = setting; }

    //
    // Called after reading in a mdf record for this node definition.
    // Calls finishDefinition() after installing the CDB and SI allocators
    // for this node.
    //
    void	completeDefinition();

    boolean isUILoadedOnly(); 
    void setUILoadedOnly(); 

    //
    // Add a selectable value for the n'th input.
    //
    boolean addInputValueOption(int n, const char *value);


};


#endif // _NodeDefinition_h
