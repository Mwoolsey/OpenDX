/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"

#include "DXStrings.h"
#include "List.h"
#include "ListIterator.h"
#include "DXType.h"
#include "DXValue.h"


boolean DXType::DXTypeClassInitialized = FALSE;
List	*DXType::TypeList;
//
// Forces initialization so that we can call static member functions
// without creating an instance of class DXType.
//
DXType	*__force_init__ = new DXType;	

static
DXTypeName _baseType[] =
{
  {  0, "undefined",		DXType::UndefinedType  },
  {  0, "value",			DXType::ValueType  },
  {  0, "scalar",		DXType::ScalarType  },
  {  0, "integer",		DXType::IntegerType  },
  {  0, "flag",			DXType::FlagType  },
  {  0, "tensor",		DXType::TensorType  },
  {  0, "matrix",		DXType::TensorType  },
  {  0, "vector",		DXType::VectorType  },
  {  0, "value list",		DXType::ValueType | DXType::ListType  },
  {  0, "scalar list",		DXType::ScalarType | DXType::ListType  },
  {  0, "integer list",		DXType::IntegerType | DXType::ListType  },
  {  0, "flag list",		DXType::FlagType | DXType::ListType  },
  {  0, "tensor list",		DXType::TensorType | DXType::ListType  },
  {  0, "matrix list",		DXType::TensorType | DXType::ListType  },
  {  0, "vector list",		DXType::VectorType | DXType::ListType  },
  {  0, "string",		DXType::StringType  },
  {  0, "string list",		DXType::StringType | DXType::ListType  },
  {  0, "camera",		DXType::CameraType  },
  {  0, "light",			DXType::LightType  },
  {  0, "field",			DXType::FieldType  },
  {  0, "geometry field",	DXType::GeometryFieldType  },
  {  0, "color field",		DXType::ColorFieldType  },
  {  0, "scalar field",		DXType::ScalarFieldType  },
  {  0, "vector field",		DXType::VectorFieldType  },
  {  0, "data field",		DXType::DataFieldType  },
  {  0, "series",		DXType::SeriesType  },
  {  0, "field series",		DXType::FieldSeriesType  },
  {  0, "image",			DXType::ImageType  },
  {  0, "image series",		DXType::ImageType | DXType::SeriesType  },
  {  0, "vector field series",	DXType::VectorFieldType | DXType::SeriesType  },
  {  0, "group",			DXType::GroupType  },
  {  0, "value group",		DXType::ValueGroupType  },
  {  0, "value list group",	DXType::ValueListGroupType  },
  {  0, "field group",		DXType::FieldGroupType  },
  {  0, "image group",		DXType::ImageType | DXType::GroupType  },
  {  0, "list",			DXType::ListType  },
  {  0, "object",		DXType::ObjectType  },
#if !defined(DONT_DEFINE_WINDOW_TYPE)
  {  0, "window",		DXType::WhereType },
#endif
};


static
long _numBaseTypes = sizeof(_baseType) / sizeof(DXTypeName);


inline void DXType::InitializeClass()
{
    if (NOT DXType::DXTypeClassInitialized)
    {
	//
	// Create and initialize the type list with the base types.
	//
	DXType::TypeList = new List;
	for (int i = 0; i < _numBaseTypes; i++)
	{
	    DXType::TypeList->appendElement(&_baseType[i]);
	}

	DXType::DXTypeClassInitialized = TRUE;
    }
}


boolean DXType::AddUserType(const Type  type,
			    const char* name)
{
    DXTypeName* userType;

    //
    // Create a new type record and initialize it.
    //
    userType = new DXTypeName;

    userType->userDefined = TRUE;
    userType->type        = type;
    userType->name        = DuplicateString(name);

    //
    // Append the new type to the end of the list.
    //
    return DXType::TypeList->appendElement(userType);
}

//
// Determine if the given value matches any of the DXTypes contained
// in the typelist.
// Return the first type found to match, otherwise DXType::UndefinedType.
//
Type DXType::FindTypeMatch(const char *value,
			    List *typelist)
{
    ListIterator iterator(*typelist);
    DXType *dxtype;
    Type t; 

    ASSERT(typelist->getSize() > 0);

    for (dxtype = (DXType*)iterator.getNext();
	 dxtype != NUL(DXType*);
	 dxtype = (DXType*)iterator.getNext()) 
    {
	t = dxtype->getType();
	if (DXValue::IsValidValue(value,t))
	    return t;	
    }

    return DXType::UndefinedType;
   
}

boolean DXType::DeleteType(const Type type)
{
    ListIterator iterator(*DXType::TypeList);
    DXTypeName*  knownType;
    int          position;

    //
    // Iterate through the type list and look for a matching type.
    // If found, delete the type from the list, deallocate the
    // the name string and the type record if user-defined, and
    // return TRUE.
    //
    for (knownType  = (DXTypeName*)iterator.getNext(), position = 1;
	 knownType != NUL(DXTypeName*);
	 knownType  = (DXTypeName*)iterator.getNext(), position++)
    {
	if (type == knownType->type)
	{
	    DXType::TypeList->deleteElement(position);
	    if (knownType->userDefined)
	    {
		delete knownType->name;
		delete knownType;
	    }
	}
	return TRUE;
    }

    //
    // No match...return FALSE.
    //
    return FALSE;
}

 
boolean DXType::DeleteType(const char* name)
{
    ListIterator iterator(*DXType::TypeList);
    DXTypeName*  knownType;
    int          position;

    //
    // Iterate through the type list and look for a matching type.
    // If found, delete the type from the list, deallocate the
    // the name string and the type record if user-defined, and
    // return TRUE.
    //
    for (knownType  = (DXTypeName*)iterator.getNext(), position = 1;
	 knownType != NUL(DXTypeName*);
	 knownType  = (DXTypeName*)iterator.getNext(), position++)
    {
	if (EqualString(name, knownType->name))
	{
	    DXType::TypeList->deleteElement(position);
	    if (knownType->userDefined)
	    {
		delete knownType->name;
		delete knownType;
	    }
	}
	return TRUE;
    }

    //
    // No match...return FALSE.
    //
    return FALSE;
}


const char* DXType::TypeToString(const Type type)
{
    ListIterator iterator(*DXType::TypeList);
    DXTypeName*  knownType;

    //
    // Iterate through the type list.  If a matching type is
    // found, return the corresponding string value; otherwise,
    // return NULL.
    //
    while ((knownType=(DXTypeName*)iterator.getNext()))
    {
	if (type == knownType->type)
	{
	    return knownType->name;
	}
    }

    return NUL(const char*);
}


Type DXType::StringToType(const char* string)
{
    ListIterator iterator(*DXType::TypeList);
    DXTypeName*  knownType;

    ASSERT(string);

    //
    // Iterate through the type list.  If a matching string is
    // found, return the corresponding type value; otherwise,
    // return undefined type.
    //
    while ((knownType=(DXTypeName*)iterator.getNext()))
    {
	if (EqualString(string, knownType->name))
	{
	    return knownType->type;
	}
    }

    return DXType::UndefinedType;
}


boolean DXType::MatchType(const Type source,
			  const Type destination)
{
    Type intersection;
    Type baseSource;
    Type baseDestination;

    //
    // Get the intersection of source and destination types.
    //
    intersection = source & destination;

    //
    // If there is no intersection...
    //
    if (NOT intersection OR intersection == DXType::ListType)
    {
	return FALSE;
    }

    //
    // If the source/destination type is Object type...
    //
    if (source      == DXType::ObjectType OR
	destination == DXType::ObjectType)
    {
	return TRUE;
    }

    //
    // If the source type is a list while the destination type
    // is not a list, no match...
    //
    if ((source & DXType::ListType) AND (NOT (destination & DXType::ListType)))
    {
	return FALSE;
    }

    //
    // Strip off list attribute from the source and destination
    // types at this point.
    //
    baseSource      = source      & DXType::ListTypeMask;
    baseDestination = destination & DXType::ListTypeMask;

    //
    // Scalar source type cannot match flag/integer destination type.
    //
    if (baseSource == DXType::ScalarType AND
	(baseDestination == DXType::FlagType OR
	 baseDestination == DXType::IntegerType))
    {
	return FALSE;
    }
	 
#ifdef OH_YES_THEY_CAN
    //
    // Flag source type cannot match integer destination type.
    //
    if (baseSource      == DXType::FlagType AND
        baseDestination == DXType::IntegerType)
    {
	return FALSE;
    }
#endif

    //
    // Passed all the tests... return successfully.
    //
    return TRUE;
}


boolean DXType::MatchType(DXType& source,
			  DXType& destination)
{
    return DXType::MatchType(source.getType(), destination.getType());
}


boolean DXType::MatchTypeLists(List& source,
			       List& destination)
{
    ListIterator sourceIterator(source);
    ListIterator destinationIterator(destination);
    DXType*      sourceType;
    DXType*      destinationType;

    //
    // Compare each type in the source list against each type in
    // the destination list.
    //
    while ((sourceType=(DXType*)sourceIterator.getNext()))
    {
	while ((destinationType=(DXType*)destinationIterator.getNext()))
	{
	    if (MatchType(*sourceType, *destinationType))
	    {
		//
		// Match found... return successfully.
		//
		return TRUE;
	    }
	}

	//
	// Rewind the destination list...
	//
	destinationIterator.setPosition(1);
    }

    //
    // No match... return unsuccessfully.
    //
    return FALSE;
}


List* DXType::IntersectTypeLists(List& first,
				 List& second)
{
    List*        thirdList;
    ListIterator firstIterator(first);
    ListIterator secondIterator(second);
    ListIterator thirdIterator;
    ListIterator typeListIterator(*DXType::TypeList);;
    DXType*      firstType;
    DXType*      secondType;
    DXType*      thirdType;
    DXTypeName*  typeName;
    long         intersection;
    boolean      found;

    typeListIterator.setList(*DXType::TypeList);

    //
    // Create the intersection list.
    //
    thirdList = new List;
    thirdIterator.setList(*thirdList);

    //
    // Generate the new type list, which is an intersection of
    // the two type lists.
    //
    while((firstType=(DXType*)firstIterator.getNext()))
    {
	secondIterator.setPosition(1);
	while((secondType=(DXType*)secondIterator.getNext()))
	{
	    intersection = firstType->getType() & secondType->getType();

	    if (intersection AND intersection != DXType::ListType)
	    {
		//
		// Make sure that the intersection type is a valid type.
		//
		found = FALSE;
		typeListIterator.setPosition(1);
		while((typeName=(DXTypeName*)typeListIterator.getNext()))
		{
		    if (intersection == typeName->type)
		    {
			found = TRUE;
			break;
		    }
		}
		ASSERT(found);

		//
		// If type intersection found, search the new list for the
		// same type.
		//
		found = FALSE;
		thirdIterator.setPosition(1);
		while((thirdType=(DXType*)thirdIterator.getNext()))
		{
		    if (intersection == thirdType->getType())
		    {
			found = TRUE;
			break;
		    }
		}

		//
		// If the new type does not already exist in the list,
		// add to it.
		//
		if (NOT found)
		{
		    thirdList->appendElement(new DXType(intersection));
		}
	    }
	}
    }

    //
    // Return the intersection list.
    //
    return thirdList;
}



DXType::DXType()
{
    //
    // Initialize class.
    //
    DXType::InitializeClass();

    //
    // Initialize instance data.
    //
    this->type = DXType::UndefinedType;
    this->name = NUL(const char*);
}


DXType::DXType(const Type type)
{
    //
    // Initialize class.
    //
    DXType::InitializeClass();

    //
    // Initialize to be undefined.
    //
    this->type = DXType::UndefinedType;
    this->name = NUL(const char*);

    //
    // Next, set the type value.
    //
    (void)this->setType(type);
}


boolean DXType::setType(const Type type)
{
    ListIterator iterator(*DXType::TypeList);
    DXTypeName*  knownType;

    //
    // Iterate through the type list and look for a matching type.
    // If found, set the instance type and name and return TRUE.
    //
    while ((knownType=(DXTypeName*)iterator.getNext()))
    {
	if (type == knownType->type)
	{
	    this->type = type;
	    this->name = (const char*)knownType->name;
	    return TRUE;
	}
    }

    //
    // If not found, return FALSE.
    //
    return FALSE;
}
#if 0
//
// Give a value contained in the value string, determine its type.
// Returns the Type of the value and DXType::UndefinedType if no
// match is found.
//
Type DXType::ValueToType(const char *value)
{

    ListIterator iterator(*DXType::TypeList);
    DXTypeName*  knownType;

    //
    // Iterate through the type list and look for a matching type.
    // If found, set the instance type and name and return TRUE.
    //
    while (knownType = (DXTypeName*)iterator.getNext())
    {
	if (DXValue::IsValidValue(value,knownType->type))
	    return knownType->type;
    }

    //
    // If not found, return FALSE.
    //
    return DXType::UndefinedType;


}
#endif // 0

//
// Give a value contained in the value string, determine its type.
// Returns the number of types found that match the value. 
//
boolean DXType::ValueToType(const char *value, List& typelist)
{

    ListIterator iterator(*DXType::TypeList);
    DXTypeName*  knownType;
    int in_size = typelist.getSize();

    //
    // Iterate through the type list and look for a matching type.
    // If found, set the instance type and name and return TRUE.
    //
    while ((knownType=(DXTypeName*)iterator.getNext()))
    {
	if (DXValue::IsValidValue(value,knownType->type))
	    typelist.appendElement(knownType);
    }

    //
    // 
    //
    return (typelist.getSize() > in_size);
}

//
// Type masks as defined in version 1.0.0 .net files. 
//
#define MT_VALUE                0x0000003d
#define MT_STRING               0x00000040
#define MT_CAMERA               0x00000080
#define MT_LIGHT                0x00000100
#define MT_FIELD                0x00000800
#define MT_GEOMETRY_FIELD       0x01001800
#define MT_IMAGE                0x00002800
#define MT_COLOR_FIELD          0x00004800
#define MT_SCALAR_FIELD         0x00008800
#define MT_VECTOR_FIELD         0x00010800
#define MT_DATA_FIELD           0x00020800
#define MT_SERIES               0x00100800
#define MT_FIELD_SERIES         0x00300800
#define MT_GROUP                0x01000800
#define MT_VALUE_GROUP          0x03000800
#define MT_VALUE_LIST_GROUP     0x05000800
#define MT_FIELD_GROUP          0x09000800
#define MT_LIST                 0x10000000
#define MT_OBJECT               0x3fffffff
#define MT_DESCRIPTION          0x40000000
//
// Convert a version 1.0 (DX/6000 version 1.2 11/92) type to new UI types.
//
Type DXType::ConvertVersionType(Type t)
{
	Type r;
 	boolean   waslist = FALSE;	

	if (t & MT_LIST) {
 	    waslist = TRUE;	
	    t &= ~MT_LIST;
	} 

	switch (t) {
	case MT_VALUE:  		r = DXType::ValueType; break;
	case MT_STRING:  		r = DXType::StringType; break;
	case MT_CAMERA:  		r = DXType::CameraType; break;
	case MT_LIGHT:  		r = DXType::LightType; break;
	case MT_GEOMETRY_FIELD: 	r = DXType::GeometryFieldType; break;
	case MT_COLOR_FIELD:  		r = DXType::ColorFieldType; break;
	case MT_SCALAR_FIELD:  		r = DXType::ScalarType; break;
	case MT_VECTOR_FIELD:  		r = DXType::VectorType; break;
	case MT_DATA_FIELD:  		r = DXType::DataFieldType; break;
	case MT_IMAGE:  		r = DXType::ImageType; break;
	case MT_SERIES:  		r = DXType::SeriesType; break;
	case MT_FIELD_SERIES:  		r = DXType::FieldType; break;
	case MT_GROUP:  		r = DXType::GroupType; break;
	case MT_VALUE_GROUP:  		r = DXType::ValueGroupType; break;
	case MT_VALUE_LIST_GROUP:  	r = DXType::ValueListGroupType; break;
	case MT_FIELD_GROUP:  		r = DXType::FieldType; break;
	case MT_OBJECT:  		r = DXType::ObjectType; break;
	case MT_DESCRIPTION:  		r = DXType::DescriptionType; break;
	default:
		r = t;
	}

	if (waslist)
	    r |= DXType::ListType;

	return r;
}

//
// Determine the types of the items in a list.
// If there are mixed types then the returned type is DXType::ValueType.
// If there are not mixed types than the type of each item is returned.
// If an unrecognized   list item is found, DXType::UndefinedType is returned.
//
Type DXType::DetermineListItemType(const char *val)
{
	char *s1 = NULL, *s2 = NULL;
	int count = 0, index = -1;
	char buf1[512];
	char buf2[512];
	Type ctype, type = DXType::UndefinedType;
	boolean quitting = FALSE;

	if (EqualString(val,"NULL") || EqualString(val,"null"))
		return DXType::ValueType;

	while (!quitting &&
		((s1=DXValue::NextListItem(val,&index, DXType::ValueListType,buf1,512 )) ||
		(s2=DXValue::NextListItem(val,&index, DXType::StringListType,buf2,512)))) {
			if (s2) {
				ctype = DXType::StringType;
			} else if      (DXValue::IsValidValue(s1,DXType::IntegerType)) {
				if (type == DXType::ScalarType)
					ctype = DXType::ScalarType;
				else
					ctype = DXType::IntegerType;
			} else if (DXValue::IsValidValue(s1,DXType::ScalarType)) {
				ctype = DXType::ScalarType;
				if (type == DXType::IntegerType)
					type = DXType::ScalarType;
			} else if (DXValue::IsValidValue(s1,DXType::VectorType)) {
				ctype = DXType::VectorType;
			} else if (DXValue::IsValidValue(s1,DXType::TensorType)) {
				ctype = DXType::TensorType;
			} else if (DXValue::IsValidValue(s1,DXType::ValueType)) {
				ctype = DXType::ValueType;
			} else
				ctype = DXType::UndefinedType;

			if (type == DXType::UndefinedType) 
				type = ctype;

			if (ctype == DXType::UndefinedType) { 
				type = DXType::UndefinedType;
				quitting = TRUE;
			} else if (type != ctype) {
				type = DXType::ValueType;
			}
			if (type == DXType::ValueType)
				quitting = TRUE;
			count++;	
			if ((s1)&&(s1!=buf1)) {
				delete s1;
				s1 = NULL;
			}
			if ((s2)&&(s2!=buf2)) {
				delete s2;
				s2 = NULL;
			}
	}

#if 0	// 6/1/93
	if (count == 0)
		type = DXType::ValueType;
#endif

	return type;
}
//
// Copy the instance data from this to newt.  If newt is null allocate
// a new DXType.   newt is returned.
//
DXType *DXType::duplicate(DXType *newt)
{
    if (!newt)
	newt = new DXType();
    *newt = *this;

    return newt;
}
