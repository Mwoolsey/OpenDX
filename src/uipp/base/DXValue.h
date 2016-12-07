/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#ifndef _DXValue_h
#define _DXValue_h


#include "DXStrings.h"
#include "Base.h"
#include "DXType.h"
#include "DXTensor.h"


//
// DXValue class definition:
//				
class DXValue : public Base
{
  protected:
    //
    // Protected member data:
    //
    DXType	type;		// value type
    boolean	defined;	// is the value defined?

    // FIX ME: Are the following should be in a union?

    char*	string;		// string value/representation
    int		integer;	// integer/flag value
    double	scalar;		// scalar value
    DXTensor*	tensor;		// vector value

  public:
    //
    // Determines whether the specified string is a valid string value
    // of the specified type (of DXType constant).
    //
    static boolean IsValidValue(const char* string,
				const Type  type);

    static boolean IsValidValue(const char* string,
				DXType&     type)
    {
	ASSERT(string);
	return IsValidValue(string, type.getType());
    }
    //
    // For the given type, try and augment the given string value with
    // syntactic sugar to coerce the given value to the given type.
    // The types we can coerce are StringType, ListType, VectorType and 
    // TensorType.
    // If the value is coerceable return a new string that does not need to
    // be coerced to the given value, otherwise return NULL.
    // On success, the returned string must be freed by the caller.
    //
    static char *CoerceValue(const char *value, Type type);


    //
    // Get the next item from a list of the given type.
    // Currently, only ScalarListType, IntegerListType, VectorListType and
    // StringListType are supported.
    // Search for the item is started a s[*index], and upon a successfull,
    // return *index is updated to point to the character not parsed as a list .
    // item. If *index is less than zero then we parse the initial list 
    // delimiter '{'.  If buf is provided (!NULL), then the list item is copied 
    // into it otherwise a new string is allocated to hold the list item.  In 
    // either case a pointer to the string holding the list item is returned .
    // upon success.
    //
    static char *NextListItem(const char *s, int *index, 
					Type listtype, char *buf = NULL, int bufsz=0);

    //
    //  Append a list item to a list value. 
    //  The returned string must be deleted by the caller.
    //  NULL is returned on failure.
    //
    static char *AppendListItem(const char *list, const char *item);

#if 00	// Not needed yet
    //
    //
    // Insert a list item into a list value.
    // The returned string must be deleted by the caller.
    // NULL is returned on failure.  Positions begin at 1, but if position
    // is less than or equal to 0, then an Append is implied.  This works
    // on the same list types that NextListItem() does.
    //
    static char *InsertListItem(const char *list, const char *item,
				    Type listtype, int position);
#endif
    //
    //
    // Replace a list item in a list value.
    // The returned string must be deleted by the caller.
    // NULL is returned on failure.  Positions begin at 1.
    // This works on the same list types that NextListItem() does.
    //
    static char *ReplaceListItem(const char *list, const char *item,
				    Type listtype, int position);


    //
    // Delete a list item from a list value.
    // The returned string must be deleted by the caller.
    // NULL is returned on failure.  Positions begin at 1, but if position
    // is less than or equal to 0, then the last item is removed.  This works
    // on the same list types that NextListItem() does.
    //
    static char *DeleteListItem(const char *list, Type listtype, int position);

    //
    // Get the index'th list item from the given string which
    // is expected to be a list. 
    // The return list item must be deleted by the caller.
    // NULL is returned if the item was not found or if the listtype is wrong.
    //
    static char *GetListItem(const char *list, int index, Type listtype);

    //
    // Get the number of items in the list.
    //
    static int GetListItemCount(const char *list, Type listtype);

    //
    // Change the dimensionality of the given vector to the given dimensions.
    // If added components, use the dflt value.
    // Returns NULL on failure.
    // Otherwise, the returned string must be deleted by the caller.
    //
    static char *AdjustVectorDimensions(const char *vec, int new_dim,
                                                double dflt, boolean is_int);

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
    static boolean ClampVSIValue(const char *val, Type valtype, 
			double *mins, double *maxs,
                        char **clampedval);

    //
    // Parse the floating point numbers out of either a ScalarList or a
    // VectorList.  Numbers are parsed into an array created from within.
    // The values are placed into the array in the order they are encountered
    // in the list.  In the case of VectorLists, all vectors in the list must
    // be of the same dimensionality.
    // The return value is the number of list items parsed out of the list.  
    // *data is the allocated array of values, and must be freed by the caller.
    // *tuple is the dimensionality of the VectorList items (1 for ScalarList).
    // '*tuple * return-val' is the number of items in the *data array.
    //
    static int GetDoublesFromList(const char *list, Type listtype, 
					    double **data, int *tuple);

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
    // 9/12 - added another defaulting param for telling DXValue to strip
    // off all trailing 0's including the .0 left behind when decimals==0.
    // I added this right before shipping 3.1 due to a problem starting in
    // ScalarInstance:140 related to the vector interactor - c1tignor33?
    //
    static char *FormatDouble(double value, char *buf = NULL, int decimals = -1,
	boolean strip_all_zeros = FALSE);

    //
    // Constructor:
    //
    DXValue();

    //
    // Destructor:
    //
    ~DXValue();

    //
    // Is the value defined?
    //
    boolean isDefined()
    {
	return this->defined;
    }

    //
    // Clears the value and sets it undefined.
    //
    void clear();

    //
    // Assigns a string value of the specified type (DXType constant).
    // Returns TRUE if the new value has been assigned;
    // returns FALSE otherwise.
    //
    boolean setValue(const char* string,
		     const Type  type);

    boolean setValue(const char* string,
		     DXType&     type)
    {
	ASSERT(string);
	return this->setValue(string, type.getType());
    }

    //
    // Assigns a string value.
    // Returns TRUE if the string is correct; returns FALSE, otherwise.
    //
    boolean setString(const char* string);

    //
    // Assigns an integer value.
    // Returns TRUE if successful, FALSE otherwise.
    //
    boolean setInteger(const int integer);

    //
    // Assigns a scalar value.
    // Returns TRUE if successful, FALSE otherwise.
    //
    boolean setScalar(const double scalar);

    //
    // Assigns a vector value.
    // Returns TRUE if successful, FALSE otherwise.
    //
    boolean setVector(DXTensor& vector);

    //
    // Returns the string representation of the current value.
    //
    const char* getValueString();

    //
    // Access routines:
    //
    Type getType();
    const char* getTypeName();
    int getInteger();
    double getScalar();
    double getVectorComponentValue(int component);

    //
    // The number of components in a vector or the 1st vector in a vector list
    // Must have a value and the type must be Vector or VectorList.
    //
    int getVectorComponentCount();


    boolean setVectorComponentValue(int component, double val);


    //
    // Does the given string represent the given type?
    //
    boolean Valid(const char* string, const Type type);

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return "DXValue";
    }
};


#endif /* _DXValue_h */
