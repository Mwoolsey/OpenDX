/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _ParameterCDB_h
#define _ParameterCDB_h


#include "ConfigurationDialog.h"
#include "List.h"


//
// Class name definition:
//
#define ClassParameterCDB	"ParameterCDB"

//
// Referenced classes
class Node;
class List;

//
// ParameterCDB class definition:
//				
class ParameterCDB : public ConfigurationDialog
{
  private:
    //
    // Private member data:
    //
    static boolean ClassInitialized;
    static String  DefaultResources[];

  protected:
    //
    // Protected member data:
    //
    // The Parameter's parameters.
    Widget position;
    Widget name;
    Widget type;
    Widget value;
    Widget description;
    Widget optionValues;
    Widget required;
    Widget descriptive;
    Widget hidden;

    int     initialPosition;
    char   *initialName;
    char   *initialValue;
    char   *initialDescription;
    char   *initialOptionValues;
    boolean initialRequired;
    boolean initialDescriptive;
    boolean initialHidden;

    //
    // Protected functions

    virtual Widget createParam(Widget parent, Widget top);

    // See the ConfigurationDialog's description of these.  createInputs
    // replaces the standard inputs section with one that consists of
    // a text widget for each of the expression's parameters (and a source
    // field), and the expression itself.
    virtual Widget createInputs(Widget parent, Widget top);
    virtual Widget createOutputs(Widget parent, Widget top);
    virtual boolean applyCallback(Dialog *d);
    virtual void restoreCallback(Dialog *d);

    virtual void saveInitialValues();
    virtual void restoreInitialValues();
    virtual boolean applyValues();


    //
    // Install the default resources for this class and then call the
    // same super class method to get the default resources from the
    // super classes.
    //
    virtual void installDefaultResources(Widget baseWidget);

    //
    // Form the %s ; %s ; %s ... string which represents the option
    // values for the input.  Returns allocated memory that the
    // caller must delete.  The returned string is the information
    // that is currently stored in the node.
    //
    char* getOptionValuesString();

  public:
    static ConfigurationDialog *AllocateConfigurationDialog(Widget parent,
							   Node *node);
    //
    // Constructor:
    //
    ParameterCDB(Widget parent, Node *node);

    //
    // Destructor:
    //
    ~ParameterCDB();

    virtual void changeInput(int i);
    virtual void changeOutput(int i);

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassParameterCDB;
    }
};


#endif // _ParameterCDB_h
