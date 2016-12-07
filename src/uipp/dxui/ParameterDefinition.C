/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#include <stdio.h>

#include "Definition.h"
#include "Dictionary.h"
#include "ParameterDefinition.h"
#include "DXType.h"
#include "DXValue.h"
#include "ListIterator.h"

static char *defaultNULL = "NULL"; 

#define DEFAULT_VALUE_NEEDS_DELETING(pd) \
    (pd->default_value && (pd->default_value != defaultNULL)) 
			  

ParameterDefinition::ParameterDefinition(Symbol key) :
    Definition(key)
{ 
    this->description = NUL(const char*); 
    this->typeStrings = NUL(const char**);
    this->descriptive_default = FALSE;
    this->default_value = defaultNULL; 
    this->is_input = FALSE;
    this->required = FALSE;
    this->default_visibility = TRUE;
    this->viewable = TRUE;
    ASSERT(OutputFullyCached == InputDerivesOutputCacheTag);
    this->defaultCacheability = OutputFullyCached;
    this->writeableCacheability = TRUE;
    this->rerouteOutput = -1;
    this->dummy = FALSE;
    this->valueOptions = NULL;

    // IMPORTANT: if you add new data here, be sure to add it in duplicate()
    //		  as well.
}

ParameterDefinition::~ParameterDefinition()
{
    if (this->description)
	delete[] (char*)this->description; 
    if (this->typeStrings) {
#if defined(DXD_WIN) || defined(OS2)
        delete (void *)this->typeStrings;
#else
        delete[] this->typeStrings;
#endif
    }
    if (DEFAULT_VALUE_NEEDS_DELETING(this))
	delete[] this->default_value;

    if (this->valueOptions) {
	int i = 0;
	for (i=0; this->valueOptions[i] ; i++)
	    delete[] this->valueOptions[i];
	FREE(this->valueOptions);
    }

    ListIterator iterator(this->types);
    DXType *t;
    while ( (t = (DXType*)iterator.getNext()) )
	delete t;
}

//
// Make a brand new copy of this and return it.
//
ParameterDefinition *ParameterDefinition::duplicate(ParameterDefinition *newpd)
{
    if (!newpd) 
	newpd = new ParameterDefinition(this->getNameSymbol());

    newpd->description 		= NULL; 
    newpd->typeStrings 		= NULL; 
    newpd->descriptive_default 	= newpd->descriptive_default;
    newpd->default_value 	= NULL; 
    newpd->is_input 		= this->is_input;
    newpd->required 		= this->required;
    newpd->default_visibility 	= this->default_visibility;
    newpd->viewable 		= this->viewable;
    newpd->defaultCacheability 	= this->defaultCacheability;
    newpd->writeableCacheability= this->writeableCacheability;
    newpd->rerouteOutput  	= this->rerouteOutput;
    newpd->dummy 		= this->dummy; 

    //
    // Now reset everything that is owned by this parameter definition.
    //
    newpd->setDescription(this->getDescription());
    if (this->isDefaultValue())
	newpd->setDefaultValue(this->getDefaultValue());
    else
	newpd->setDescriptiveValue(this->getDefaultValue());

    //
    // Now copy the type list.
    //
    DXType *t;
    ListIterator iterator(this->types);
    while ( (t = (DXType*)iterator.getNext()) ) {
	DXType *newt = t->duplicate(); 
	newpd->addType(newt);
    }

    return newpd;
}


// 
// Append the given type t to the list of acceptable types for this
// parameter definition.  The given type is then owned by the 
// ParameterDefinition and will be deleted when it is deleted.
//
boolean
ParameterDefinition::addType(DXType *t)
{
    if (this->typeStrings)
    {
#if defined(DXD_WIN) || defined(OS2)
	delete (void*)this->typeStrings;
#else
	delete[] this->typeStrings;
#endif
	this->typeStrings = NULL;
    }

    if (!this->types.appendElement(t))
	    return FALSE;

    return TRUE;
}

//
// Build the typeStrings member, which is an array of pointers to
// strings.  Each string represents one of the types that this parameter
// can assume.  The typeStrings list is NULL terminated. 
//
void ParameterDefinition::buildTypeStrings()
{
    int i, cnt = this->types.getSize();
    
    ASSERT(cnt > 0);
    ASSERT(this->typeStrings == NUL(const char**));
    this->typeStrings = new const char*[cnt + 1];
 
    ListIterator iterator(this->types);
    for (i=0 ; i<cnt; i++) {
	DXType *t = (DXType*)iterator.getNext();
	this->typeStrings[i] = t->getName();
	ASSERT(this->typeStrings[i] != NUL(const char*));
    }
    this->typeStrings[cnt] = NUL(const char*);
		
}

boolean
ParameterDefinition::removeType(DXType *t)
{
    ListIterator iterator(this->types);
    DXType*      dxtype;

    if (this->typeStrings)
    {
#if defined(DXD_WIN) || defined(OS2)
	delete (void*)this->typeStrings;
#else
	delete[] this->typeStrings;
#endif
	this->typeStrings = NULL;
    }

    //
    // Look for type and if found, free memory associated with it 
    //
    while( (dxtype = (DXType*)iterator.getNext()) )
	if (dxtype->getType() == t->getType()) {
	    boolean r = this->types.deleteElement(
				this->types.getPosition(dxtype));
	    ASSERT(r);
	    delete dxtype;
	    return TRUE;
	}

    	
    return FALSE;
}

//
// Get the default type of this parameter.  When a parameter has more
// than 1 type, always return the first type on the type list.
//
Type ParameterDefinition::getDefaultType()
{
    DXType *dxtype;
    Type type;

    dxtype = (DXType*)this->types.getElement(1); 
    type = dxtype->getType();
    return type;
}

void ParameterDefinition::removeValueOptions()
{
    if (this->valueOptions) {
	int i = 0;
	for (i=0; this->valueOptions[i] ; i++)
	    delete[] this->valueOptions[i];
	FREE(this->valueOptions);
    }
    this->valueOptions = 0;
}


//
// Look for type that matches the type for the value given. 
// If found, set the value of the default for this instance and return 
// success, otherwise the given value was not in this parameter's type
// list and so we fail.
//
boolean
ParameterDefinition::setDefaultValue(const char *value)
{
    List	typelist;

    //
    // Make sure the value provided is recognized as one of the supported types.
    //
    if (value != NULL && DXType::ValueToType(value, typelist) == 0)
	return FALSE;		// Unrecognized value

    if (DEFAULT_VALUE_NEEDS_DELETING(this))
	delete[] this->default_value;

    this->descriptive_default = FALSE; 
    this->default_value = DuplicateString(value);
    return TRUE;

}

//
// Get the INPUT/OUTPUT entry in the MDF for this parameter.
// The returned string must be freed by the caller.
//
char *ParameterDefinition::getMDFString()
{
    int i, typecnt= this->getTypes()->getSize();
    const char* const *typeNames = this->getTypeStrings();
    char *types = new char[64 * typecnt];
     
    types[0] = '\0';
    char *p = types;
    for (i=0; i<typecnt ; i++, p+=STRLEN(p)) {
	if (i+1 == typecnt)
	    sprintf(p,"%s",typeNames[i]);
	else
	    sprintf(p,"%s or ",typeNames[i]);
    }
    const char *name = this->getNameString();
    char attributes[128];
    const char *dflt = this->getDefaultValue();
    const char *desc = this->getDescription();

    *attributes = '[';
    p = &attributes[1];
    *p = '\0';
    if (!this->isViewable()) {
        sprintf(p,"visible:2 ");
    	p += STRLEN(p);
    } else if (this->getDefaultVisibility() == FALSE) {
        sprintf(p,"visible:0 ");
    	p += STRLEN(p);
    }
    Cacheability c = this->getDefaultCacheability();
    if (this->isInput()) {
	if (c != InputDerivesOutputCacheTag) {
	    sprintf(p,"cache:%d ",c);
	    p += STRLEN(p);
	}
	int output = this->getRerouteOutput();
	if (output > 0) {
	    sprintf(p,"reroute:%d ",output-1);	// Executive uses 0 based
	    p += STRLEN(p);
	}
    } else if (c != OutputFullyCached) {
	    sprintf(p,"cache:%d ", c);
	    p += STRLEN(p);
    } 
    if (STRLEN(attributes) > 1)
	strcat(p,"]");
    else
	*attributes = '\0';

    int len = STRLEN(name) +  STRLEN(attributes)  +
			STRLEN(types) + STRLEN(dflt) + STRLEN(desc) +
			20;

     
    char *mdf = new char[len];
    if (this->isInput())
	sprintf(mdf,"INPUT %s %s ; %s ; %s ; %s\n",
		name,
		attributes,
		types,
		(dflt && !EqualString(dflt,"NULL") ? dflt : ""),
		(desc ? desc : ""));
    else 
	sprintf(mdf,"OUTPUT %s %s ; %s ; %s\n",
		name,
		attributes,
		types,
		(desc ? desc : ""));
  
    delete types;
    return mdf;
}

void ParameterDefinition::setDescriptiveValue(const char *d)
{  
    if (DEFAULT_VALUE_NEEDS_DELETING(this))
        delete[] (char*)this->default_value;

    this->default_value = DuplicateString(d);
    this->descriptive_default = TRUE;
}

//
// Get the description of this parameter. 
//
const char *ParameterDefinition::getDescription()
{
    if (this->dummy)
	return DUMMY_DESCRIPTION_STRING;
    else
	return this->description; 
}
//
// Change whether or not this is a (macro's) dummy parameter 
//
void  ParameterDefinition::setDummy(boolean b)
{
    this->dummy = b;
}

//
// Is this a (macro's) dummy parameter. 
//
boolean ParameterDefinition::isDummy()
{
    return this->dummy; 
}

//
// Get the selectable values for this parameter.
// This returns a pointer to a constant array of pointers to
// constant strings which is NOT to be manipulated by the caller.
// The returned array of pointers is NULL terminated.
//
const char* const *ParameterDefinition::getValueOptions()
{
    return (const char* const *)this->valueOptions;
}
//
// Add the given value to the list of suggested values for this parameter 
//
boolean ParameterDefinition::addValueOption(const char *value)
{
	int position;

	if (!this->valueOptions) {
		int size = 4;
		this->valueOptions = (char**)MALLOC(size * sizeof(char*));
		memset(this->valueOptions,0, size * sizeof(char*));
		position = 0;
	} else {
		position = 0;
		while (this->valueOptions[position])
			position++;	
		int nextpos = position + 1;
		switch (nextpos) {
		case 4:
		case 8:
		case 16:
		case 32:
		case 64:
			this->valueOptions = (char**)REALLOC((void*)this->valueOptions,
				2 * nextpos * sizeof(char*));
			memset(this->valueOptions + nextpos, 0, nextpos*sizeof(char*));
			break;
		default:
			if (nextpos > 64) {
				fprintf(stderr,
					"Too many value options for parameter (ignoring)");
				position = -1;
			}
			break;
		}
	}
	if (position >= 0) {
		this->valueOptions[position] = DuplicateString(value);
		return TRUE;
	} else {
		return FALSE;
	}

}

