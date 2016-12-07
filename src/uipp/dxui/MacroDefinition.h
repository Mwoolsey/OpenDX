/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#ifndef _MacroDefinition_h
#define _MacroDefinition_h


#include "List.h"
#include "NodeDefinition.h"
#include "Network.h"
#include "enums.h"

//
// Class name definition:
//
#define ClassMacroDefinition	"MacroDefinition"

//
// Referenced classes
class Network;
class MacroNode;
class Command;

//
// MacroDefinition class definition:
//				
class MacroDefinition : public NodeDefinition
{
  friend class SaveMacroCommand;
  friend boolean Network::saveNetwork(const char*, boolean);

  private:
    //
    // Private member data:
    //
    Command  *saveCmd;
    boolean  systemMacro;	// Is this an internal only system macro

    //
    // Find the first available spot to place a new parameter in the given
    // list (expected to be either inputDefs or outputDefs). If there are dummy
    // parameters in the list, then the index of the first dummy is returned.
    // If no dummies, then N+1 is returned, where N is the current number of
    // items in the list.
    //
    int getFirstAvailableIOPosition(List *l);

  protected:
    //
    // Protected member data:
    //
    char *fileName;
    Network *body;

    List referencingNodes;		// a list of Nodes.

    boolean initialRead;
    boolean updatingServer;	// Are we calling DXExecCtl::updateMacros()

    void setFileName(const char*);
    
    static boolean LoadMacroFile(FILE *f,
				 const char *fileName,
				 boolean replace,
				 boolean *wasMacro = NULL,
				 boolean asSystemMacro = FALSE);
    virtual boolean removeIODef(List *l,
				ParameterDefinition *pd);
    virtual boolean addIODef(List *l,
			     ParameterDefinition *pd);
    virtual boolean replaceIODef(List *l,
				 ParameterDefinition *newPd,
				 ParameterDefinition *pd);

    //
    // Get the Nth input that is not a dummy parameter definition.
    //
    ParameterDefinition *getNonDummyIODefinition(List *l, int n);


    //
    // Allocate a new Node of the corresponding type.
    //
    virtual Node *newNode(Network *net, int instance = -1); 

  public:
    //
    // If errmsg is not NULL and an error occurs then, no error messages are
    // posted, and instead a string buffer is allocated to hold the error
    // message that would have been posted and returned.  The returned
    // string must be freed by the caller.
    //
    static boolean LoadMacro(const char *fileName, char **errmsg = NULL,
				 boolean asSystemMacro = FALSE);

    //
    // Load all .net files in the given directory that are macros.
    // If replace is TRUE, then replace any current definitions with the
    // new one, otherwise ignore the .net file.
    // If errmsg is not NULL and an error occurs then, no error messages are
    // posted, and instead a string buffer is allocated to hold the error
    // message that would have been posted and returned.  The returned
    // string must be freed by the caller.
    //
    static boolean LoadMacroDirectories(const char *path, 
					boolean replace = FALSE, 
					char **errmsg = NULL,
				 boolean asSystemMacro = FALSE);

    //
    // Constructor:
    //
    MacroDefinition(boolean systemMacro = FALSE);

    //
    // Destructor:
    //
    ~MacroDefinition();

    Command *getSaveCmd() { return this->saveCmd; }

    virtual boolean isDerivedFromMacro() { return TRUE; } 

    virtual void finishDefinition();

    virtual void reference(MacroNode *n);
    virtual void dereference(MacroNode *n);

    //
    // setNetwork should only be used when creating a new macro from a	
    // network which exists (isn't in a file or already read in), and 
    // isn't the main network.  It should really only be called by 
    // Network::makeMacro().
    //
    virtual boolean updateServer();
    virtual boolean setNetwork(Network *net);
    virtual boolean setNodeDefinitions(MacroDefinition *newDef);

    boolean removeInput(ParameterDefinition *pd)
		{ return this->removeIODef(&this->inputDefs, pd); }
    boolean removeOutput(ParameterDefinition *pd)
		{ return this->removeIODef(&this->outputDefs, pd); }
    boolean replaceInput(ParameterDefinition *newPd, ParameterDefinition *pd)
		{ return this->replaceIODef(&this->inputDefs, newPd, pd); }
    boolean replaceOutput(ParameterDefinition *newPd, ParameterDefinition *pd)
		{ return this->replaceIODef(&this->outputDefs, newPd, pd); }

    ParameterDefinition *getNonDummyInputDefinition(int n)
		{ return this->getNonDummyIODefinition(&this->inputDefs, n); }
    ParameterDefinition *getNonDummyOutputDefinition(int n)
		{ return this->getNonDummyIODefinition(&this->outputDefs, n); }
		
  
    //
    // Find the first available spot to place a new parameter in the 
    // input/ouput. If there are dummy parameters in the list, then the 
    // index of the first dummy is returned.  If no dummies, then N+1 is 
    // returned, where N is the current number of items in the list.
    //
    int getFirstAvailableInputPosition() 
		{ return this->getFirstAvailableIOPosition(&this->inputDefs); }
    int getFirstAvailableOutputPosition() 
		{ return this->getFirstAvailableIOPosition(&this->outputDefs); }

    virtual void openMacro();

    boolean isReadingNet() { return this->initialRead; }

    boolean printNetworkBody(FILE *f, PrintType ptype);
    boolean loadNetworkBody();
    Network *getNetwork() { return this->body; }

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassMacroDefinition;
    }
};


#endif // _MacroDefinition_h
