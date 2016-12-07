/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"

#include <ctype.h>

#include "DXValue.h"
#include "List.h"
#include "ListIterator.h"
#include "DXTensor.h"
#include "DXStrings.h"
#include "lex.h"

//
// Clip the trail zeros off of a scalar printed with %#g format that
// winds up '6.00000'.  
// Changes '6.0[0]*'  into '6.0'.
// Changes '6.0[0]* ' into '6.0 '.
//
static void clip_trailing_zeros(char *buf)
{
    char *c = strstr(buf,".0");

    if (c) {
	char a, *p = c+2;
	boolean doit = TRUE;
	boolean space = FALSE;
	while ( (a=*p) ) {
	    if (a == ' ')  {
		space = TRUE;
		break;
	    }
	    if (a != '0') {
		doit = FALSE;
		break;
	    } 
	    p++;
	}
	if (doit && p != c) {
	    // char a = *(c+6);
	    // char b = *(c+7);
	    if (space) {
		*(c+2) = ' ';
		*(c+3) = '\0';
	    } else {
		*(c+2) = '\0';
	    }
	}
    }

}

//
// Returns TRUE if a NULL string is found; returns FALSE otherwise.
// The index variable is updated to the character following the lexed
// string upon successful return.
//
static
boolean _IsNULL(const char* string,
		int&         index)
{
    int i;

    i = index;
    SkipWhiteSpace(string, i);

    if (EqualSubstring(&string[i], "NULL", 4) OR
	EqualSubstring(&string[i], "null", 4))
    {
	index = i + 4;
	return TRUE;
    }
    else
    {
	return FALSE;
    }

}    


//
// Returns TRUE if a value string is found; returns FALSE otherwise.
// If the type of value is specified (scalar, vector, or tensor), a
// comparison will be only against the specified type; otherwise, all
// three types will be checked. 
//
// The index variable is updated to the character following the lexed
// string, and the type variable to the matching type,  upon successful
// return.
//
static
boolean _IsValue(const char* string,
		 int&        index,
		 Type&       type)
{
    int tuple;

    if (string == NUL(char*))
    {
	return FALSE;
    }

    switch(type)
    {
      case DXType::ScalarType:
	return IsScalar(string, index);

      case DXType::VectorType:
	return IsVector(string, index, tuple);

      case DXType::TensorType:
	return IsTensor(string, index);

      default:
	//
	// Scalar value?
	//
	if (IsScalar(string, index))
	{
	    type = DXType::ScalarType;
	    return TRUE;
	}

	//
	// Vector value?
	//
	if (IsVector(string, index, tuple))
	{
	    type = DXType::VectorType;
	    return TRUE;
	}

	//
	// Tensor value?
	//
	if (IsTensor(string, index))
	{
	    type = DXType::TensorType;
	    return TRUE;
	}

	//
	// No match... return unsuccessfully.
	//
	return FALSE;
    }
}

//
// Returns TRUE if a list of specified type is found; returns FALSE
// otherwise.  
// The index variable is updated to the character following the lexed 
// string upon successful return.
// This does NOT check for list constructors.
//
static
boolean _IsList(const char* string,
		int&        index,
		const Type  type)
{
    Type    value_type;
    int     i;
    int     tuple;
    int     first_tuple=0;
    int     elements;
    boolean lexed;

    //
    // Parameter validity check...
    //
    if (string == NUL(char*))
    {
	return FALSE;
    }
    
    ASSERT(index >= 0 AND index <= STRLEN(string) + 1);

    elements   = 0;
    value_type = DXType::UndefinedType;
    
    //
    // Skip space.
    //
    i = index;
    SkipWhiteSpace(string, i);

    //
    // Lex '{'.
    //
    if (string[i] == '{')
    {
	i++;
	SkipWhiteSpace(string, i);
    }
    else
    {
	return FALSE;
    }

    while (string[i] != '\0' AND string[i] != '}')
    {
	if (elements > 0)
	{
	    //
	    // Lex optional comma.
	    //
	    if (string[i] == ',')
	    {
		i++;
		SkipWhiteSpace(string, i);
	    }
	}

	switch(type)
	{
	  case DXType::FlagType:
	    lexed = IsFlag(string, i);
	    break;
	  case DXType::IntegerType:
	    lexed = IsInteger(string, i);
	    break;

	  case DXType::ScalarType:
	    lexed = IsScalar(string, i);
	    break;

	  case DXType::VectorType:
	    lexed = IsVector(string, i, tuple);
	    if (elements == 0)
	    {
		first_tuple = tuple;
	    }
	    else if (tuple != first_tuple)
	    {
		return FALSE;
	    }
	    break;

	  case DXType::TensorType:
	    lexed = IsTensor(string, i);
	    break;

	  case DXType::ValueType:
	    lexed = _IsValue(string, i, value_type);
	    break;

	  case DXType::StringType:
	    lexed = IsString(string, i);
	    break;

	  default:
	    return FALSE;
	}

	if (lexed)
	{
	    elements++;
	}
	else
	{
	    return FALSE;
	}

	boolean value_ok = FALSE;
	switch (type) {
	  default:
		  break;
	  case DXType::ValueType:
		  switch (value_type) {
			default:
				break;
			case DXType::VectorType:
			case DXType::TensorType: 
				if (string[i] == '[') value_ok = TRUE;
					break; 
			}
		  // Fall through
	  case DXType::ScalarType:
		  //
		  // Make sure there is a separator
		  // If we don't do this, then '{.01.01.01}' is a scalar list.
		  //
		  if (!value_ok && !IsWhiteSpace(&string[i]) && 
			  (string[i] != ',') && (string[i] != '}'))
			  return FALSE;
		  break;
	}
	//
	// Skip space.
	//
	SkipWhiteSpace(string, i);
    }

    //
    // No end delimiter and no elements... return unsuccessfully.
    //
    if (string[i] != '}' OR elements == 0)
    {
	return FALSE;
    }

    //
    // Return successfully.
    //
    index = i + 1;
    return TRUE;

}


//
// Returns TRUE if a list constructor is found; returns FALSE
// otherwise.  The index variable is updated to the character
// following the lexed string upon successful return.
//
static
boolean _uinIsListConstructor(const char* string,
			      int&        index)
{
    boolean parsed_dot_dot;
    boolean parsed_colon;
    int     elements;
    int     i;

    //
    // Parameter validity check...
    //
    if (string == NUL(char*))
    {
	return FALSE;
    }
    
    ASSERT(index >= 0 AND index <= STRLEN(string) + 1);

    elements       = 0;
    parsed_dot_dot = FALSE;
    parsed_colon   = FALSE;

    //
    // Skip space.
    //
    i = index;
    SkipWhiteSpace(string, i);

    //
    // Lex '{'.
    //
    if (string[i] != '{')
    {
	return FALSE;
    }

    i++;
    SkipWhiteSpace(string, i);

    while (string[i] != '\0' AND string[i] != '}')
    {
	if (IsScalar(string, i))
	{
	    elements++;
	    SkipWhiteSpace(string, i);
	}
	else
	{
	    return FALSE;
	}

	switch(elements)
	{
	  case 1:
	    if (string[i] == '.' AND string[i + 1] == '.')
	    {
		parsed_dot_dot = TRUE;

		i += 2;
		SkipWhiteSpace(string, i);
	    }
	    else
	    {
		return FALSE;
	    }
	    break;

	  case 2:
	    if (parsed_dot_dot AND string[i] == ':')
	    {
		parsed_colon = TRUE;

		i++;
		SkipWhiteSpace(string, i);
	    }
	    break;

	  case 3:
	    if (NOT parsed_colon)
	    {
		return FALSE;
	    }
	    break;
	}
    }

    //
    // No end delimiter... return unsuccessfully.
    //
    if (string[i] != '}')
    {
	return FALSE;
    }

    //
    // Semantic error?  Return unsuccessfully.
    //
    if (parsed_colon)
    {
	if (elements != 3)
	{
	    return FALSE;
	}
    }
    else if (elements != 2)
    {
	return FALSE;
    }

    //
    // Return successfully.
    //
    index = i + 1;
    return TRUE;
}

//
// Returns TRUE if a list of specified type is found; returns FALSE
// otherwise.  
// The index variable is updated to the character following the lexed 
// string upon successful return.
// This checks for both standard lists and list constructors.
//
static 
boolean IsList(const char* string,
		int&        index,
		const Type  type)
{

    boolean r = _IsList(string, index, type);
    if (!r && (type & DXType::ValueType))
	r = _uinIsListConstructor(string,index);
    return r;
}

//
// Returns TRUE if an object string is found; returns FALSE
// otherwise.  The index variable is updated to the character
// following the lexed string upon successful return.
//
static
boolean _IsObject(const char* string,
		  int&        index)
{
    Type value_type;
    int  tuple;

    if (string == NUL(char*))
    {
	return FALSE;
    }

    ASSERT(index >= 0 AND index <= STRLEN(string) + 1);

    if (IsInteger(string, index))
    {
	return TRUE;
    }

    if (IsScalar(string, index))
    {
	return TRUE;
    }

    if (IsVector(string, index, tuple))
    {
	return TRUE;
    }

    if (IsTensor(string, index))
    {
	return TRUE;
    }

    value_type = DXType::UndefinedType;
    if (_IsValue(string, index, value_type))
    {
	return TRUE;
    }

    if (IsString(string, index))
    {
	return TRUE;
    }

    if (IsList(string, index, DXType::IntegerType))
    {
	return TRUE;
    }

    if (IsList(string, index, DXType::ScalarType))
    {
	return TRUE;
    }

    if (IsList(string, index, DXType::VectorType))
    {
	return TRUE;
    }

    if (IsList(string, index, DXType::TensorType))
    {
	return TRUE;
    }

    if (IsList(string, index, DXType::StringType))
    {
	return TRUE;
    }

#if 0
    if (_uinIsListConstructor(string, index))
    {
	return TRUE;
    }
#endif

    return FALSE;
}



//
// Determines whether the specified string is a valid string value
// of the specified type (of DXType constant).
// 
// FIXME: this needs to know about user types (also see DXType::ValueToType()).
//
boolean DXValue::IsValidValue(const char* string,
		       const Type  type)
{
    int em_size=4096;
    char extra_memory[4096];
    char*   value;
    boolean result;
    Type    value_type;
    Type    list_type;
    int     tuple;
    int     i;
    boolean to_be_freed;

    //
    // No string... return unsuccessfully.
    //
    if (string == NUL(char*))
    {
	return FALSE;
    }

    //
    // Copy the string.
    //
    if (strlen(string) >= em_size) {
	value = DuplicateString(string);
        to_be_freed = TRUE;
    } else {
	value = extra_memory;
	strcpy (value, string);
	to_be_freed = FALSE;
    }
    for (i = 0; value[i] != '\0'; i++)
    {
        //
        // FIXME: tolower returns junk if the input is negative.
        // This was that cute bug found by gresh just hours before the
	// final build.  Cheers.
        //
#if defined(aviion)
	value[i] = (value[i]>=0 ? tolower(value[i]): value[i]);
#else
	value[i] = tolower(value[i]);
#endif
    }

    //
    // Remove the trailing blanks.
    //
    for (i = (int)STRLEN(value) - 1; value[i] == ' ' OR value[i] == '\t'; i--)
	;
    value[i + 1] = '\0';

    //
    // Find the first non-blank letter.
    //
    i = 0;
    SkipWhiteSpace(value, i);

    //
    // Initialize the result.
    //
    result = FALSE;

    //
    // Special-case NULL.
    //
    if (_IsNULL(value, i))
    {
	result = TRUE;
    }

    //
    // Flag type:
    //
    else if (type == DXType::FlagType)
    {
	result = IsFlag(value, i);
    }
    //
    // Integer type:
    //
    else if (type == DXType::IntegerType) 
    {
	result = IsInteger(value, i);
    }

    //
    // Scalar type:
    //
    else if (type == DXType::ScalarType)
    {
	result = IsScalar(value, i);
    }

    //
    // Vector type:
    //
    else if (type == DXType::VectorType)
    {
	result = IsVector(value, i, tuple);
    }

    //
    // Tensor type:
    //
    else if (type == DXType::TensorType)
    {
	result = IsTensor(value, i);
    }

    //
    // Value type:
    //
    else if (type == DXType::ValueType)
    {
	value_type = DXType::UndefinedType;
	result = _IsValue(value, i, value_type);
    }

    //
    // String type:
    //
    else if (type == DXType::StringType)
    {
	result = IsString(value, i);
    }

    //
    // Object type:
    //
    else if (type == DXType::ObjectType)
    {
	result = _IsObject(value, i);
    }

    //
    // List type:
    //
    else if (type & DXType::ListType)
    {
	list_type = type & DXType::ListTypeMask;

	result = IsList(value, i, list_type);
    }

    //
    // Where paramter type
    //
    else if (type & DXType::WhereType)
    {
	result = IsWhere(value, i);
    }

    else if (type & DXType::UserType1 OR
	     type & DXType::UserType2 OR
	     type & DXType::UserType4 OR
	     type & DXType::UserType5 OR
	     type & DXType::UserType6 OR
	     type & DXType::DescriptionType)
    {
	result = TRUE;
    }

    //
    // Check for end of string.
    //
    if (result)
    {
	result = IsEndOfString(value, i);
    }

    //
    // Free the value string.
    //
    if (to_be_freed)
	delete value;

    //
    // Return the result.
    //
    return result;
}


DXValue::DXValue()
{
    //
    // Initialize instance data.
    //
    this->type.setType(DXType::UndefinedType);
    this->defined = FALSE;
    this->string  = NUL(char*);
    this->integer = 0;
    this->scalar  = 0.0;

    this->tensor = NUL(DXTensor*);
}


DXValue::~DXValue()
{
    //
    // Clear this object.
    //
    this->clear();
}

void DXValue::clear()
{
    //
    // Clear all values.
    //

    if (this->string)
    {
	delete this->string;
        this->string = NUL(char*);
    }

    this->integer = 0;
    this->scalar  = 0.0;

    if (this->tensor)
    {
	delete this->tensor;
        this->tensor= NUL(DXTensor*);
    }

    //
    // Set this value object to be undefined.
    //
    this->defined = FALSE;
    (void)this->type.setType(DXType::UndefinedType);
}

//
// Assigns a string value of the specified type (DXType constant).
// Returns TRUE if the new value has been assigned;
// returns FALSE otherwise.
//
boolean DXValue::setValue(const char* string,
						  const Type  type)
{
	int i;
	boolean r;
	ASSERT(this);

	if (DXValue::IsValidValue(string, type))
	{
		//
		// Avoid the case where string and this->string are the same.
		//
		char *s = DuplicateString(string);

		//
		// Clear the value and reset the type.
		//
		this->clear();
		this->type.setType(type);

		//
		// Save the string representation of the value (ALWAYS!!!).
		//
		this->string = s;	
		string = (const char*) s;	// in case string == this->string

		//
		// Special case NULL value.
		//
		i = 0;
		if (_IsNULL(string, i))
		{
			// Allow NULL to match any type.
			this->type.setType(type);
		}
		else
		{
			//
			// Convert the string to internal, native form according to type.
			//
			char *newval, *p, buf[64];
			int index;
			switch(type)
			{
			case DXType::FlagType:
				// 
				//  The executive does not recognize 'true' or 'false', so
				//  we convert them to integer values.
				// 
				if (strstr(string,"false"))  {
					this->integer = 0; this->scalar = 0;
					delete this->string;
					this->string = DuplicateString("0");
				} else if (strstr(string,"true")) {
					this->integer = 1; this->scalar = 1;
					delete this->string;
					this->string = DuplicateString("1");
				} else {
					sscanf(string, "%d",  &this->integer);
					sscanf(string, "%lg", &this->scalar);
				}
				break;

			case DXType::IntegerType:
				sscanf(string, "%d", &this->integer);
				sscanf(string, "%lg", &this->scalar);
				break;

			case DXType::ScalarType:
				sscanf(string, "%lg", &this->scalar);
				if (!strchr(string,'.')) {
					//
					// Need to make sure that the string representation, really
					// represents a floating point number and not an integer.
					//
					delete this->string;
					this->string = DXValue::FormatDouble(this->scalar);
				}

				break;

			case DXType::VectorType:
				this->tensor = new DXTensor;
				r = this->tensor->setValue(string);
				ASSERT(r);
				break;

			case DXType::TensorType:
			case DXType::ValueType:
			case DXType::StringType:
			case DXType::ObjectType:
			case DXType::WhereType:
			case DXType::DescriptionType:
				break;

			default:
				if (type & DXType::ListType)
				{
					switch(type & DXType::ListTypeMask)
					{
					case DXType::ScalarType:
						//
						// Convert '{ 1 2.0000 3.3 }' into '{ 1.0 2.0 3.3 }'
						// Ignore lists specified with the elipsis '{ 1..2: 3}'
						//
						if (strchr(this->string,':'))
							break;
						newval = new char[STRLEN(this->string) + 1024];
						index = -1;
						strcpy(newval,"{ ");
						p = newval + STRLEN(newval);
						while (DXValue::NextListItem(this->string,
							&index,type, buf, 64)) {
								if (strchr(buf,'.')) {
									// Already has a decimal, just copy.
									sprintf(p,"%s ",buf);
								} else {
									double d = (double)atof(buf);
									this->FormatDouble(d,p);
									strcat(p," ");
								}
								clip_trailing_zeros(p);
								p += STRLEN(p);
						}	
						strcat(p,"}");
						delete this->string;
						// Copy this so we don't waste space (1024 above).
						this->string = DuplicateString(newval); 
						delete newval;
						break;
					case DXType::IntegerType:
					case DXType::FlagType:
					case DXType::VectorType:
					case DXType::TensorType:
					case DXType::ValueType:
					case DXType::StringType:
						break;
					}
				}
				else if (type & DXType::UserType1 OR
					type & DXType::UserType2 OR
					type & DXType::UserType4 OR
					type & DXType::UserType5 OR
					type & DXType::UserType6)
				{
				}
				else
				{
					ASSERT(FALSE);
				}
				break;
			}
		}

		return TRUE;
	}
	else
	{
		return FALSE;
	}
}


const char* DXValue::getValueString()
{
    //return DuplicateString(this->string ? this->string : "NULL");
    return this->string ? this->string : "NULL";
}


//
// Assigns a value in string representation.
// Returns TRUE if the string is correct; returns FALSE, otherwise.
//
boolean DXValue::setString(const char* string)
{
    ASSERT(this);

    if (DXValue::IsValidValue(string, (Type)DXType::StringType))
    {
	this->clear();
	this->type.setType(DXType::StringType);
	this->string = DuplicateString(string);

	return TRUE;
    }
    else
    {
	return FALSE;
    }
}


//
// Assigns an integer value.
// Returns TRUE if successful, FALSE otherwise.
//
boolean DXValue::setInteger(const int integer)
{
    ASSERT(this);

    //
    // Clear the old values and reset the type.
    //
    this->clear();
    this->type.setType(DXType::IntegerType);

    //
    // Set the new integer value.
    //
    this->integer = integer;

    //
    // Generate a string equivalent of this new value.
    //
    this->string = new char[32];
    sprintf(this->string, "%d", integer);

    //
    // Return successfully.
    //
    return TRUE;
}


//
// Assigns a scalar value.
// Returns TRUE if successful, FALSE otherwise.
//
boolean DXValue::setScalar(const double scalar)
{
    ASSERT(this);

    //
    // Clear the old values and reset the type.
    //
    this->clear();
    this->type.setType(DXType::ScalarType);

    //
    // Set the new scalar value.
    //
    this->scalar = scalar;

    //
    // Generate a string equivalent of this new value.
    //
    this->string = this->FormatDouble(scalar);

    //
    // Return successfully.
    //
    return TRUE;
}

//
// Format a double into a string to preserve the double precision.
// The values is formatted into buf if provided otherwise into a
// newly allocated string.
// The value is formatted with a number of decimal places as indicated
// by 'decimals'. If decimals is greater than 0, then that number
// of decimal places is always printed (upto that number supported by
// sprintf()).  If decimals is less than or equal to 0, then we use 8 
// decimals, except that we clip any trailing zeros down to ".0".  
// Further, if decimals is 0 then we clip off the ".0" too. 
//
// The strip_all_zeros argument was added 9/12/95 because vector interactors
// want to get converted to ints whenever possible but it's wrong to do
// a strstr for ".0".  The default value of strip_all_zeros is FALSE and it's
// specified as TRUE by ScalarInstance (as of 9/12/95 anyway).
//
char *DXValue::FormatDouble(double value, char *buf, int decimals, 
boolean strip_all_zeros)
{
    if (!buf)
        buf = new char[64];

    int precision;
    if (decimals <= 0)
	precision = 8;
    else
	precision = decimals;
    //
    // Use a format that will preserver the precision.  %#g prints 
    // 407.0000006 as 407.0000 and drops the 6.
    //
    sprintf(buf, "%.*g", precision, value);

    //
    // If the number is not in exponential form and does not
    // have a decimal point, then add one (%.8g may leave off the .000
    // if the value is an integer value). 
    //
    if (!strchr(buf,'.') && !strchr(buf,'e')) {
	strcat(buf,".0");
	if (decimals > 1) {
	    int i;
	    for (i=1 ; i<decimals ; i++)
		strcat(buf,"0");
	}
    } 
 
    if (strip_all_zeros) {
	clip_trailing_zeros(buf);
	int len = strlen(buf);
	if ((len>=2) && (buf[len-1] == '0') && (buf[len-2] == '.')) {
	    buf[len-2] = '\0';
	}
    } else if (decimals <= 0) {
	clip_trailing_zeros(buf);
	if (decimals == 0) {
	    char *p = (char*)strrstr(buf,".0");
	    if (p) 
		*p = '\0';
	}
    }

    return buf;
}

boolean DXValue::setVector(DXTensor& vector)
{
    ASSERT(this); 
    ASSERT(this->tensor);
    ASSERT(this->tensor->getDimensions() == 1);
   

    //
    // Clear the old values and reset the type.
    //
    this->clear();
    this->type.setType(DXType::VectorType);

    //
    // Make a copy of the vector and generate a string image.
    //
    this->tensor = vector.dup();
    this->string = DuplicateString(vector.getValueString());

    return TRUE;
}

//
// Get the next item from a list of the given type.
// Currently, only ScalarListType, IntegerListType, VectorListType and
// StringListType are supported.
// Search for the item is started a s[*index], and upon a successfull return, 
// *index is updated to point to the character not parsed as a list item.
// If *index is less than zero then we parse the initial list delimiter '}'.
// If buf is provided (!NULL), then the list item is copied into it otherwise
// a new string is allocated to hold the list item.  In either case a pointer
// the string holding the list item is returned upon success.
//
char *DXValue::NextListItem(const char *s, int *index, 
				Type listtype, char *buf, int bufsz)
{
    int start, i = *index;
    boolean r = true;
    char *p ;

    if (!s)
	return NULL;

    ASSERT(listtype & DXType::ListType);


    if (i < 0) {	// First item in list, handle the '{'
	i = 0;
	SkipWhiteSpace(s,i);
	if (s[i] != '{')
	    return NULL;
	i++;
    	SkipWhiteSpace(s,i);
    } else {		// Not the first item, handle the optional ','
    	SkipWhiteSpace(s,i);
	if (s[i] == ',') {
	    i++;
	    SkipWhiteSpace(s,i);
	}
    }

    start = i;
    int tuple;
    switch (listtype & DXType::ListTypeMask) {
	case DXType::ScalarType:
	    r = IsScalar(s, i);
	    break;
	case DXType::IntegerType:
	    r = IsInteger(s, i);
	    break;
	case DXType::VectorType:
	    r = IsVector(s, i, tuple);
	    break;
	case DXType::TensorType:
	    r = IsTensor(s, i);
	    break;
	case DXType::StringType:
	    r = IsString(s, i);
	    break;
	case DXType::ValueType:
	    r = IsInteger(s, i) || IsScalar(s,i) || IsVector(s,i,tuple) ||
						    IsTensor(s,i);
	    break;
	default:
	    printf("Can't get list items from a list of type 0x%lx\n",listtype);
	    ASSERT(0);
    }

    p = NULL;
    if (r) {
	p = buf;
	int len = i - start;
	if (!p) 
	    p = new char[len + 1];
	else if ((bufsz > 0) && (bufsz < (len+1))) {
	    printf ("%s[%d]Warning: buffer overrun\n", __FILE__,__LINE__);
	}
	strncpy(p,&s[start], len);
	p[len] = '\0';
	*index = i;
    }

    return p;
} 
#if 0
//
// Create a string representation of the given number with the give decimal
// places. 
//
char *DXValue::FormatNumber(double val, int decimals)
{
    char format[64], tmp[8];

    if ((val > -0.001 && val < 0.0) || (val > 0.0 && val < .001)) {
        sprintf(format, "%%.%de", decimals);
    } else {
    	sprintf(format, "%%.%df", decimals);
    }
    char *s = new char [64];

    sprintf(s, format,val);

    return s;
}
#endif
//
// For the given type, try and augment the given string value with
// syntactic sugar to coerce the given value to the given type.
// The types we can coerce are StringType, ListType, VectorType and TensorType.
// If the value is coerceable return a new string that does not need to
// be coerced to the given value, otherwise return NULL.
//
char *DXValue::CoerceValue(const char *value, Type type)
{
	char *s;

	if (type == DXType::ValueType) {
		s = DXValue::CoerceValue(value,DXType::VectorType);
	} else {
		char *p = (char*)value;
		int cnt, end;
		boolean failed;

		SkipWhiteSpace(p);
		cnt = STRLEN(p);
		s = new char[cnt+3];
		if (cnt) {
			s[0] = ' ';
			strcpy(&s[1],p);
			cnt = STRLEN(s);

			// Skip trailing white space.
			while (s[--cnt] == ' ' || s[cnt] == '\t') continue;

			end = cnt+1;
			s[end+1] = '\0';
		} else {
			end = 1;
			s[2]='\0';
		}


		//
		// window must be surrounded by quotes just as string is.
		//
		if ((type == DXType::StringType) || (type == DXType::WhereType)) {
			s[0]   = '"';
			s[end] = '"';
		} else if (type & DXType::ListType) {
			s[0]   = '{';
			s[end] = '}';
		} else if ((type == DXType::TensorType) ||
			(type == DXType::VectorType)) {
				s[0]   = '[';
				s[end] = ']';
		} else {	// Allow anything that does not need syntax help
			strcpy(s,p);
		}

		if (!DXValue::IsValidValue(s,type)) 
			failed = TRUE;
		else
			failed = FALSE;

		//
		// Try and coerce a list with a single element (i.e. coerce both the
		// element and the overall value).
		//
		if (failed && (type & DXType::ListType)) {
			Type basetype = type & DXType::ListTypeMask;
			if ((p = DXValue::CoerceValue(value,basetype))) {
				char *p2  = DXValue::CoerceValue(p,type);
				if (p2) {
					delete s;
					s = p2;
					failed = FALSE;
				}
				delete p;
			}
		}
		if (failed) {
			delete s;
			s = NULL;
		}
	}
	return s;
}
//
//  Append a list item to a list value. 
//  The returned string must be deleted by the caller. 
//  NULL is returned on failure. 
//
char *DXValue::AppendListItem(const char *list, const char *item)
{
    int itemlen = STRLEN(item);
    char *nlist;

    if (EqualString(list,"NULL")) {

	nlist = new char[itemlen + 5];
	sprintf(nlist,"{ %s }",item);

    } else {

        nlist = new char [STRLEN(list) + itemlen + 8 ];
	strcpy(nlist,list);
	char *c = strrchr(nlist,'}');

	if (!c) {
	    delete nlist;
	    nlist = NULL; 
	} else {
	    *c = '\0';	
	    strcat(c-1, item);
	    strcat(c, " }");
	}
    }

    return nlist;
	

}
//
// Get the index'th list item from the given string which
// is expected to be a list.
// The return list item must be deleted by the caller.
// NULL is returned if the item was not found or if the listtype is wrong.
//
char *DXValue::GetListItem(const char *list, int index, Type listtype)
{
   int size = STRLEN(list);
   char *value = NULL, *buf = new char [size];
   int i, idx = -1;


   for (i=0 ; i<index; i++) {
        value = DXValue::NextListItem(list, &idx, listtype, buf,size);
        if (!value)
            break;
   }

   if (!value) 
        delete buf;
 
   return value;
}

//
// Get the number of items in the list. 
//
int DXValue::GetListItemCount(const char *list, Type listtype)
{
   char *buf;
   int count =0,  idx = -1;

   if (EqualString(list,"NULL") || EqualString(list,"null"))
	return 0;

   if (listtype == DXType::UndefinedType) {
	listtype = DXType::DetermineListItemType(list);
 	if (listtype == DXType::UndefinedType)
	    return 0;
	listtype |= DXType::ListType;
   }

   char stack[4096];
   boolean to_be_freed;
   int size = 4096;
   if (STRLEN(list) < size) {
       buf = stack;
       to_be_freed = FALSE;
   } else {
       size = STRLEN(list);
       buf = new char [size];
       to_be_freed = TRUE;
   }
   while (DXValue::NextListItem(list, &idx, listtype, buf, size))
	count++;

   if (to_be_freed)
       delete buf;
 
   return count;
}

boolean DXValue::setVectorComponentValue(int component, double val)
{
    ASSERT(this->type.getType() == DXType::VectorType);
    ASSERT(this->tensor);
    boolean r = this->tensor->setVectorComponentValue(component, val);
    if (r) {
	if (this->string) delete this->string;
	this->string = DuplicateString(this->tensor->getValueString());
    }
    return r;
}
//
// Change the dimensionality of the given vector to the given dimensions.
// If added components, use the dflt value.
// Returns NULL on failure.
// Otherwise, the returned string must be deleted by the caller.
//
char *DXValue::AdjustVectorDimensions(const char *vec, int new_dim,
                                                double dflt, boolean is_int)
{
    DXTensor vector;
    int i;

    if (!vector.setValue(vec))
        return NULL;

    int old_dim = vector.getVectorComponentCount();
    char *value = new char[32 * new_dim];
    int min_dim = (old_dim > new_dim ? new_dim : old_dim);

    //
    // Adjust the value
    //
    strcpy(value,"[");
    char *p  = value;
    for (i = 1, p += STRLEN(value); i<=min_dim; i++, p+= STRLEN(p)) {
        double v = vector.getVectorComponentValue(i);
        if (is_int)
            sprintf(p, "%d ", (int)v);
        else {
	    DXValue::FormatDouble(v,p);
	    strcat(p," ");
	}
    }
    for (i = min_dim+1 ; i<=new_dim; i++) {
        p += STRLEN(p);
        if (is_int)
            sprintf(p, "%d ", (int)dflt);
        else {
	    DXValue::FormatDouble(dflt,p);
	    strcat(p," ");
	}
    }
    strcat(p,"]");
    return value;

}
//
// Make sure all values (Scalar[List], Integer[List] and Vector[List] 
// components) are within the given ranges for each component.  
// 'val' is the given string value representing the Scalar, Integer or 
// Vector and 'valtype' is expected to be its type.  
// mins/maxs are arrays of minimum and maximum values for the 
// corresponding components (scalar and integer only have a single 
// component).  
// TRUE is returned if the value needs to be clamped to be within the 
// given ranges, FALSE otherwise.  If clampedval is provided, then a 
// string is passed back which represents the clamped value.
//
boolean DXValue::ClampVSIValue(const char *val, Type valtype,
		    double *mins, double *maxs,
                        char **clampedval)
{
    boolean reset = FALSE;
    char valbuf[64], itembuf[1024];
    Type itemtype;
    DXValue dxval;

    ASSERT(val);
    if (!dxval.setValue(val,valtype)) {
	if (clampedval) *clampedval = NULL;
	return FALSE;
    }
    itemtype = valtype & DXType::ListTypeMask;
    ASSERT((itemtype == DXType::IntegerType) ||
           (itemtype == DXType::ScalarType)  ||
           (itemtype == DXType::VectorType));

    if (valtype & DXType::ListType) {
	int index = -1;
	char *clamped = NULL;
	char *p=NULL;
	int maxlen = 0, clampedlen = 0;
	if (clampedval) {
	    maxlen = STRLEN(val) + 16;	
	    clamped = new char[maxlen];
	    strcpy(clamped,"{");
	    clampedlen = 1;
	    p = clamped;
	}
	while (DXValue::NextListItem(val,&index,valtype,itembuf,1024)) {
	    char *newval, *s;
	    if (ClampVSIValue(itembuf,itemtype,mins,maxs,&newval)) {
	        ASSERT(newval);
	        s = newval;
		reset = TRUE;
	    } else {
	       s = itembuf;
	    }
	    if (clamped) {
		int newchars = STRLEN(s) + 2;	
		if (clampedlen + newchars >= maxlen )  {
		    if (newchars < clampedlen)
			maxlen = 2 * clampedlen; 
		    else
			maxlen = clampedlen + newchars;  
		    clamped = (char*)REALLOC(clamped, maxlen);
		    p = clamped;
		    clampedlen = 0;	// Set correctly below.
		}
		strcat(p, " ");
		strcat(p, s);
		int l = STRLEN(p);
		p += l; 
		clampedlen += l;
	    }
	    if (newval)
		delete newval;
	}
	if (reset) {
	    if (clamped) {
		if (maxlen - clampedlen <= 1)
		    p = clamped = (char*)REALLOC(clamped, maxlen + 2);
		strcat(p,"}");
	    }
	} else {
	    if (clamped)
		delete clamped;
	    clamped = NULL;
	}
	if (clampedval)
	    *clampedval = clamped;
    } else {

	int i, ncomp;
	boolean isvector =  (itemtype == DXType::VectorType);
	boolean isinteger = (itemtype == DXType::IntegerType);

	if (isvector)
	    ncomp = dxval.getVectorComponentCount();
	else
	    ncomp = 1;

	itembuf[0] = '\0';
	if (isvector)
	   strcat(itembuf,"[");

	for (i=0 ; i<ncomp ; i++) {
	    double val;
	    double min = mins[i];
	    double max = maxs[i];
	    if (isvector) {
		val = (double)dxval.getVectorComponentValue(i+1);
	    } else if (isinteger) {
		val = (double)dxval.getInteger();
	    } else {
		val = (double)dxval.getScalar();
	    }
	    if (val > max) {
		val = max;
		reset = TRUE;
	    } else if (val < min) {
		val = min;
		reset = TRUE;
	    }
	    if (isinteger) {
		sprintf(valbuf,"%d ", (int)(val));
	    } else {
		DXValue::FormatDouble(val,valbuf);
		strcat(valbuf," ");
	    }
	    strcat(itembuf, valbuf);
	}
	if (clampedval) {
	    if (reset) {
		if (isvector)
               	    strcat(itembuf,"]");
		*clampedval = DuplicateString(itembuf);
	    } else 
		*clampedval = NULL;
        }
    }
    return reset;
}


// 
// Parse the floating point numbers out of either a ScalarList or a
// VectorList.  Numbers are parsed into an array created from within.
// The values are placed into the array in the order they are encountered
// in the list.  In the case of VectorLists, all vectors in the list must
// be of the same dimensionality.
// The return value is the number of list items (and not numbers) parsed out 
// of the list.  
// *data is the allocated array of values, and must be freed by the caller.
// *tuple is the dimensionality of the VectorList items (1 for ScalarList).
// '*tuple * return-val' is the number of items in the *data array.
//
int DXValue::GetDoublesFromList(const char *list, Type listtype, 
					double **data, int *tuple)
{
    int count = DXValue::GetListItemCount(list, listtype);
    double *values = NULL;
    int vtuple = 0, nitems = 0;
 
    if (count) {
        int     i, index = -1, offset = 0;
	DXTensor t;
        //
        // Get the floating point values out of the string list.
        //
        for (i=0 ; i<count ; i++) {
            char        stringval[64];
            if (DXValue::NextListItem(list,&index,listtype,stringval)) {
		boolean r;
		int this_tuple, j;
		switch (listtype) {
		    case DXType::IntegerListType:
		    case DXType::ScalarListType:
			if (i == 0)  {
			    vtuple = 1;
			    values = new double[count];	
			}
			sscanf(stringval,"%lg",&values[i]); 
			break;
		    case DXType::VectorListType:
			r = t.setValue(stringval);
			ASSERT(r);
			this_tuple = t.getVectorComponentCount();
			if (i == 0)  {
			    vtuple = this_tuple;
			    values = new double[count * this_tuple];
			}
			ASSERT(vtuple == this_tuple);
			for (j=0 ; j<this_tuple ; j++) 
			    values[offset + j] = 
				t.getVectorComponentValue(j+1);
			offset += vtuple;
			break;
		    default:
			ASSERT(FALSE);
		}
		nitems++;
            } else {
                ASSERT(FALSE);
	    }	
        }
    }

    if (tuple) 
	*tuple = vtuple;
    *data = values;
    return nitems;
}

//
// Delete a list item from a list value.
// The returned string must be deleted by the caller.
// NULL is returned on failure.  Positions begin at 1, but if position
// is less than or equal to 0, then the last item is removed.  This works
// on the same list types that NextListItem() does.
//
char *DXValue::DeleteListItem(const char *list, Type listtype, int position)
{
   char *p, *value = NULL, *newlist, *buf;
   int i, idx;

   if (position <= 0)  
	position = DXValue::GetListItemCount(list,listtype);

   if ((position <= 0) || EqualString(list,"NULL"))
	return DuplicateString("NULL");

   int len = STRLEN(list);
   char buf_space[1024];
   int space_size = 1024;
   boolean to_be_freed;
   newlist= new char[len];

   if (len > space_size) {
       buf = new char[len];
       to_be_freed = TRUE;
   } else {
       buf = buf_space;
       to_be_freed = FALSE;
   }

   //
   // Skip over the first list delimiter.
   //
   for (idx=0 ; list[idx] != '{' ; idx++);
   idx++;

   //
   // Skip over the first 'position-1' items.
   //
   strcpy(newlist,"{"); 
   p = newlist;
   for (i=1, p+=STRLEN(p); i<position ; i++, p += STRLEN(p)) {
        value = DXValue::NextListItem(list, &idx, listtype, buf,space_size);
        if (!value) {
            break;
	}
	strcat(p," ");	
	strcat(p,value);	
   }
   //
   // Skip over the 'position'th item.
   //
   value = DXValue::NextListItem(list, &idx, listtype, buf,space_size);

   //
   // Add the rest of the items. 
   //
   if (!value)
       strcat(p,"}");
   else
       strcat(p,&list[idx]);

   if (to_be_freed)
       delete buf;

   // FIXME: can return "{ }"
   return newlist;
}
//
// Replace a list item in a list value.
// The returned string must be deleted by the caller.
// NULL is returned on failure.  Positions begin at 1.
// This works on the same list types that NextListItem() does.
//
char *DXValue::ReplaceListItem(const char *list, const char *item,
				Type listtype, int position)
{
   char *p, *value = NULL, *newlist, *buf;
   int i, idx;
   boolean premature_end;
   int listlen = STRLEN(list);
   int itemlen = STRLEN(item);

   ASSERT(position > 0);

   if (EqualString(list,"NULL")) 
	return DuplicateString("NULL");
   
   char buf_space[1024];
   int space_size = 1024;
   boolean to_be_freed;
   if ((listlen+1) > space_size) {
       buf = new char[listlen + 1];
       to_be_freed = TRUE;
   } else {
       buf = buf_space;
       to_be_freed = FALSE;
   }
   newlist= new char [listlen + itemlen + position + 4];

   //
   // Skip over the first list delimiter.
   //
   for (idx=0 ; list[idx] != '{' ; idx++);
   idx++;

   //
   // Skip over the first 'position-1' items.
   //
   strcpy(newlist,"{"); 
   p = newlist;
   premature_end = FALSE;
   for (i=1, p+=STRLEN(p); i<position ; i++, p += STRLEN(p)) {
        value = DXValue::NextListItem(list, &idx, listtype, buf, space_size);
        if (!value) {
   	    premature_end = FALSE;
            break;
	}
	strcat(p," ");	
	strcat(p,buf);	
   }

   //
   // Insert the new item. 
   //
   strcat(p," ");
   strcat(p,item);


    //
    // Tack on the rest of the list. 
    //
    if (premature_end) {
   	strcat(p,"}");
    } else {
	//
	// Skip over the 'position'th item.
	//
	value = DXValue::NextListItem(list, &idx, listtype, buf,space_size);
	strcat(p,&list[idx]);
    }

    if (to_be_freed)
	delete buf;
    return newlist;
}
//
// The number of components in a vector or the 1st vector in a vector list
// Must have a value and the type must be Vector or VectorList. 
//
int DXValue::getVectorComponentCount()
{
    DXTensor vlist_item, *t = NULL;
    char *listitem;
    boolean r;

    switch (this->type.getType()) {
	case DXType::VectorType:
	    ASSERT(this->tensor);
	    t = this->tensor;
	    break;
	case DXType::VectorListType:
	    ASSERT(this->string);
            listitem = DXValue::GetListItem(this->string,1,
						DXType::VectorListType);
	    ASSERT(listitem);
	    r = vlist_item.setValue(listitem);
	    delete listitem;
	    ASSERT(r);
	    t = &vlist_item;
	    break;
	default:
	    ASSERT(0);
    }
    return t->getVectorComponentCount();
}

Type DXValue::getType()
{
    return this->type.getType();
}

const char* DXValue::getTypeName()
{
    return this->type.getName();
}

int DXValue::getInteger()
{
    ASSERT(this->type.getType() == DXType::IntegerType || 
           this->type.getType() == DXType::FlagType);
    return this->integer;
}

double DXValue::getScalar()
{
    ASSERT(this->type.getType() == DXType::ScalarType);
    return this->scalar;
}

double DXValue::getVectorComponentValue(int component)
{
    ASSERT(this->type.getType() == DXType::VectorType);
    ASSERT(this->tensor);
    return this->tensor->getVectorComponentValue(component);
}

#if 00	// Not needed yet
//
// Insert a list item into a list value.
// The returned string must be deleted by the caller.
// NULL is returned on failure.  Positions begin at 1, but if position
// is less than or equal to 0, then an Append is implied.  This works
// on the same list types that NextListItem() does.
//
char *DXValue::InsertListItem(const char *list, const char *item,
				Type listtype, int position)
{
   char *p, *value = NULL, *newlist, *buf;
   int i, idx;
   boolean premature_end;
   int listlen = STRLEN(list);
   int itemlen = STRLEN(item);

   if (position <= 0) {
	return DXValue::AppendListItem(list,item);
   } else if (EqualString(list,"NULL")) {
	newlist = new char[itemlen + 5];
	sprintf(newlist,"{ %s }",item);
	return newlist;
   }
   
   buf = new char[(listlen > itemlen ? listlen : itemlen)];
   newlist= new char [listlen + itemlen + 32];

   //
   // Skip over the first list delimiter.
   //
   for (idx=0 ; list[idx] != '{' ; idx++);
   idx++;

   //
   // Skip over the first 'position-1' items.
   //
   strcpy(newlist,"{"); 
   p = newlist;
   premature_end = FALSE;
   for (i=1, p+=STRLEN(p); i<position ; i++, p += STRLEN(p)) {
        value = DXValue::NextListItem(list, &idx, listtype, buf);    
        if (!value) {
   	    premature_end = FALSE;
            break;
	}
	strcat(p," ");	
	strcat(p,buf);	
   }
   //
   // Insert the new item. 
   //
   strcat(p," ");
   strcat(p,item);

   //
   // Tack on the rest of the list. 
   //
   if (premature_end)
   	strcat(p,"}");
   else
	strcat(p,&list[idx]);

   delete buf;
   return newlist;
}
#endif // 00	Not needed yet
