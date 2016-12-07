/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


//////////////////////////////////////////////////////////////////////////////

#ifndef _ParameterDefinition_h
#define _ParameterDefinition_h


#include "List.h"
#include "Definition.h"
#include "DXValue.h"
#include "DXType.h"

#include "Cacheability.h"


//
// Class name definition:
//
#define ClassParameterDefinition	"ParameterDefinition"


class Dictionary;

#define FOR_EACH_PARAM_TYPE(pd, dxtype, iterator) \
 for (iterator.setList(pd->types) ; (dxtype = (DXType*)iterator.getNext()) ; )

//
// Used in the .net file to flag macro parameters as dummies.
//
#define DUMMY_DESCRIPTION_STRING	"<<< Dummy parameter (do not edit) >>>"

//
// ParameterDefinition class definition:
//				
class ParameterDefinition : public Definition 
{

  private:

#if 0 // FIXME: circular includes won't let us do this.
    friend Type Parameter::setValue(const char *v);
#else
    friend class Parameter;
#endif

    //
    // Private member data:
    //
    List		types;
    const char**	typeStrings;	// If !NULL, contains type names
    const char		*description;
    char		*default_value;	
    boolean		viewable;	// Is this parameter viewable (in a cdb)
    boolean		required;
    boolean		default_visibility;
    boolean		descriptive_default;
    boolean		is_input;	// Does this define an input or output 
    boolean		dummy;          // Is this a dummy parameter
    char 		**valueOptions;

    //
    // The following is only valid for inputs and when the value is >= 0.
    // It indicates the output to which this input is rerouted when the
    // executive decides not to schedule the module owning the input.
    // Currenlty, we only keep track of it so we can send it to the 
    // executive if the input belongs to an OUTBOARD module.
    //
    int			rerouteOutput;	

    //
    // Cacheability status of parameter.  
    // Initially used for outputs only.
    //
    Cacheability	defaultCacheability;
    boolean	 	writeableCacheability;

#if 1	// FIXME
	friend class NodeDefinition;
#else
	friend boolean NodeDefinition::addInput(Symbol Name, 
					ParameterDefinition *pd);
	friend boolean NodeDefinition::addOutput(Symbol Name, 
					ParameterDefinition *pd);
#endif
    void buildTypeStrings(void);

  protected:
    //
    // Protected member data:
    //

  public:
    //
    // Constructor:
    //
    ParameterDefinition(Symbol key);

    //
    // Destructor:
    //
    ~ParameterDefinition();

    // 
    // Append the given type t to the list of acceptable types for this
    // parameter definition.  The given type is then owned by the 
    // ParameterDefinition and will be deleted when it is deleted.
    //
    boolean	addType(DXType *t);	

    //
    // Remove (and free) the type in the list that matches t 
    //
    boolean	removeType(DXType *t);

    //
    // Manipulate the default visibility of the tab for this parameter 
    //
    void 	setDefaultVisibility(boolean v = TRUE) 
					{ this->default_visibility = v; }
    boolean	getDefaultVisibility() { return default_visibility; }

    //
    // S/Get the 1 based index of the output to which this input can be
    // be rerouted. 
    //
    void setRerouteOutput(int val) 
		{ ASSERT(val > 0 && this->isInput());
		  this->rerouteOutput = val;
		}
    int getRerouteOutput() 
		{ ASSERT(this->isInput());
		  return this->rerouteOutput; 
		}
    //
    // Manipulate the viewability of the tab for this parameter 
    //
    void 	setViewability(boolean v) { this->viewable = v; 
					    if (!v) 
					       this->default_visibility = FALSE;
					  }
    boolean	isViewable() 	{ return this->viewable; }

    //
    // Retrieves a NULL terminated array of pointers to strings.
    // Each string contains the name of a type that this parameter 
    // can accept.  This returns a pointer to a constant array of
    // pointers to constant character strings.
    //
    const char* const *getTypeStrings() { 
			if (typeStrings == NUL(const char**))
					 buildTypeStrings();
				 ASSERT(this->typeStrings);
				 return (const char* const *)this->typeStrings;
				}
    List *getTypes() { return &this->types; }

    //
    // Manipulate the type of I/O this parameter definition represents. 
    //
    void	markAsInput()	{ is_input = TRUE; }
    void	markAsOutput()	{ is_input = FALSE; }
    boolean	isInput()	{ return is_input; }
    boolean	isOutput()	{ return !is_input; }
    
    //
    // Change the Description of this parameter ;
    //
    void 	setDescription(const char *d) 
			{  if (description)  delete (char*)description;
			   description = DuplicateString(d); }
    const char  *getDescription(); 

    //
    // Change need for this parameter 
    //
    boolean 	isRequired() { return required; }
    void 	setRequired() { required = TRUE; }
    void 	setNotRequired() { required = FALSE; }

    //
    // Set/Get the default value, which must be of a type found in the type list. 
    //
    boolean     isDefaultValue() { return descriptive_default == FALSE; }
    boolean	setDefaultValue(const char *value); 
    const char  *getDefaultValue()  { return default_value; }

    //
    // Get the default type of this parameter.  When a parameter has more
    // than 1 type, always return the first type on the type list.
    //
    Type getDefaultType();

    //
    // Set the default value, which must be of a type found in the type list. 
    //
    boolean	isDefaultDescriptive()   { return descriptive_default; }
    void	setDescriptiveValue(const char *d);

    void    setWriteableCacheability(boolean v)
                	{ this->writeableCacheability = v; }
    boolean hasWriteableCacheability()
                	{ return this->writeableCacheability; }
    void        setDefaultCacheability(Cacheability c)
                        { this->defaultCacheability = c; }
    Cacheability getDefaultCacheability()
                	{ return this->defaultCacheability; }


    //
    // Get the INPUT/OUTPUT entry in the MDF for this parameter.
    // The returned string must be freed by the caller.
    //
    char *getMDFString();

    //
    // Manage the dummy status of this parameter (typically a macro parameter)
    //
    void  setDummy(boolean b);
    boolean isDummy();

    //
    // Make a brand new copy of this and return it.
    // If newpd is NULL, then the new ParameterDefinition is allocated.
    //
    ParameterDefinition *duplicate(ParameterDefinition *newpd = NULL);

    //
    // Add the given value to the list of suggested values for this parameter
    //
    boolean addValueOption(const char *value);

    //
    // Get the selectable values for this parameter.
    // This returns a pointer to a constant array of pointers to
    // constant strings which is NOT to be manipulated by the caller.
    // The returned array of pointers is NULL terminated.
    //
    const char* const *getValueOptions();

    //
    // On behalf of MacroParameterNodes which ought to be able to
    // take on new types during the running of dxui, permit the
    // user to create new value options.
    //
    void removeValueOptions();

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassParameterDefinition;
    }
};


#endif // _ParameterDefinition_h
