/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



// ToggleNode.h -						    //
//                                                                          //
// Definition for the ToggleNode class.			    //
//
/*
 This node implements an interactor that outputs one of two values.  The
 output is either considered 'set' (on) or 'reset' (off).  Methods are
 provided to change the values that are output for set and reset, to
 retrieve the set/reset values and to change the output set/reset state.
 Because we need to keep track of the set/reset state, one should NOT
 call this->setOutputValue() without EXTREME caution.  Instead use set() 
 and reset(), which keep track of the set/reset state,  to change the 
 output values of this node. 
*/

#ifndef _ToggleNode_h
#define _ToggleNode_h


#include "InteractorNode.h"


//
// Class name definition:
//
#define ClassToggleNode	"ToggleNode"


//
// ToggleNode class definition:
//				
class ToggleNode : public InteractorNode
{
  private:
    //
    // Private member data:
    //

    //
    // Set the output and send it if requested.
    // If setit is TRUE, then set the output to the first of the toggle values
    // otherwise the 2nd of toggle values.
    // If how is 1, then send the value.
    // If how is 0,  then don't send the value.
    // If how is -1, then send the value quietly.
    //
    boolean setTheToggle(boolean setit, int how);

    //
    // Set the two potential output values.  The 1st corresponds to the
    // set (toggle down) state and the second to the reset state (toggle up).
    // If how is 1, then send the values.
    // If how is 0,  then don't send the values.
    // If how is -1, then send the values quietly.
    //
    boolean changeToggleValues(const char *set, const char *reset, 
							int how);
  protected:
    //
    // Protected member data:
    //
    Type	outputType;
    boolean	is_set;

    //
    // 
    //
    int  handleInteractorMsgInfo(const char *line);


    //
    // Set the output to and send it.
    // If setit is TRUE, then set the output to the first of the toggle values
    // otherwise the 2nd of toggle values.
    // If send is TRUE, then send it.
    //
    boolean setToggle(boolean setit, boolean send);
    boolean setToggleQuietly(boolean setit);

    //
    // Get the values that are output for the set and reset states.
    // The returned string must be freed by the caller.
    //
    char *getToggleValue(boolean setval);

    //
    // Initialize the two potential output values.  The 1st corresponds to the
    // set (toggle down) state and the second to the reset state (toggle up).
    // This (init) is called at creation, where as setToggleValues() is called
    // after initialization.
    //
    boolean initToggleValues(const char *set, const char *reset);

    //
    // Create a new interactor instance for this class.
    //
    virtual InteractorInstance *newInteractorInstance();

    //
    // Print/Parse standard comments and add/parse a line with 
    // the 'toggle: %d, set = %s, reset = %s' comment.
    // Only print/parse the ', set =....' part if includeValues is set.
    //
    boolean printToggleComment(FILE *f, const char *indent, 
					boolean includeValues);
    boolean parseToggleComment(const char* comment, const char* filename, 
					int lineno, boolean includeValues);

    virtual boolean netPrintAuxComment(FILE *f);
    virtual boolean netParseAuxComment(const char* comment,
					 const char* filename, int lineno);
    virtual boolean cfgPrintInteractorComment(FILE *f);
    virtual boolean cfgParseInteractorComment(const char* comment,
					const char* filename, int lineno);


    virtual int getShadowingInput(int output_index);

#if SYSTEM_MACROS // 3/13/95 - begin work for nodes to use system macros

    virtual char *netNodeString(const char *prefix);
    virtual char *netBeginningOfMacroNodeString(const char *prefix);
#endif


  public:
    //
    // Constructor:
    //
    ToggleNode(NodeDefinition *nd, Network *net, int instnc);

    //
    // Destructor:
    //
    ~ToggleNode();

    //
    // Called after allocation is complete.
    // The work done here is to assigned default values to the InteractorNode 
    // inputs so that we can use them later when setting the attributes for the
    // Interactor.
    //
    virtual boolean initialize();

#if SYSTEM_MACROS // 3/13/95 - begin work for nodes to use system macros
    virtual boolean     sendValues(boolean     ignoreDirty = TRUE);
    virtual boolean     printValues(FILE *f, const char *prefix, PrintType dest);
#endif


    //
    // Determine if this node is a node of the given class
    //
    virtual boolean isA(Symbol classname);

    //
    // Set the two potential output values.  The 1st corresponds to the
    // set (toggle down) state and the second to the reset state (toggle up). 
    // The output is changed (without being sent to the executive) to match 
    // the new set of set/reset values.
    //
    boolean setToggleValues(const char *set, const char *reset);

    //
    // Change the output value of the toggle.
    //
    boolean set(boolean send = TRUE) 	{ return this->setToggle(TRUE, send); }
    boolean reset(boolean send = TRUE) 	{ return this->setToggle(FALSE, send); }

    //
    // Get the values that are output for the set and reset states.
    // The returned string must be freed by the caller.
    //
    char *getSetValue() 	{ return this->getToggleValue(TRUE); }
    char *getResetValue() 	{ return this->getToggleValue(FALSE); }
 
    //
    // Determine if the toggle is currently set.
    //
    boolean isSet()	{ return this->is_set; }

    boolean isSetAttributeVisuallyWriteable();
    boolean isResetAttributeVisuallyWriteable();

    virtual const char* getJavaNodeName() { return "ToggleNode"; } 
    virtual boolean printJavaType (FILE*, const char*, const char*);
    virtual boolean printJavaValue(FILE*);


    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassToggleNode;
    }
};

#endif // _ToggleNode_h
