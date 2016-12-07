/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/


#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#ifndef _DXI_EXTRACT_H_
#define _DXI_EXTRACT_H_

/* TeX starts here. Do not remove this comment. */

/*
\section{Module parameters}

This section describes routines that aid in the parsing of parameters
to modules.  Inputs to modules that are simple items such as integers,
floats and character strings are packaged as array objects.  The
following routines simplify the extraction of such values.  If the
object doesn't match (even if promoted as described below), the
routines return null but don't set the error code.  Otherwise they
return the original object and fill in the pointer to the item.

If a float is expected, a short, int or long can be promoted to float.
If an integer is expected, a short can be promoted.  If a float vector is
expected, an integer vector can be promoted.  If a string is expected,
either a string object or a array of characters is accepted.
*/

Object DXExtractInteger(Object o, int *ip);
/**
\index{DXExtractInteger}
Checks that {\tt o} is an array with a type that can be converted to
an integer and returns the integer value in {\tt *ip}.  Returns {\tt o}
on success, otherwise returns null but does not set the error code.
**/

Object DXExtractFloat(Object o, float *fp);
/**
\index{DXExtractFloat}
Checks that {\tt o} is an array with a type that can be converted to
an float and returns the float value in {\tt *fp}.  Retruns {\tt o}
on success, otherwise returns null but does not set the error code.
**/

Object DXExtractString(Object o, char **cp);
/**
\index{DXExtractString}
Checks that {\tt o} is an array of characters or a string object, and
returns in {\tt **cp} a pointer to the string.  Returns {\tt o} on
success, otherwise returns null but does not set the error code.
**/

Object DXExtractNthString(Object o, int n, char **cp);
/**
\index{DXExtractNthString}
Checks that {\tt o} is an array of null separated and terminated
strings or a string object.  If it is, it returns a pointer to the
character that begins the {\tt n}th string.  Strings are indexed from
0.  Returns {\tt o} on success, otherwise returns null but does not
set the error code.
**/

Object DXQueryParameter(Object o, Type t, int dim, int *count);
/**
\index{DXQueryParameter}
Checks that {\tt o} is an array with a type that can be converted to
to type {\tt t}.  Checks that the array has dimensionality {\tt dim}.
Dimensionality of 0 or 1 both match rank 0 or rank 1 with shape 1,
i.e. scalars or vectors with one element.  Dimensionality greater than
{\tt dim} matches rank 1 with shape {\tt dim}, in vectors of dimensionality
{\tt dim}.  Returns in {\tt *n} the number of items in the array.
Returns {\tt o} on success, otherwise returns null but does not set
the error code.
**/

Object DXExtractParameter(Object o, Type t, int dim, int count, Pointer p);
/**
\index{DXExtractParameter}
Checks the type and dimensionality of {\tt o} as for {\tt
DXQueryParameter()}.  The array must have exactly {\tt n} items.
Returns the items converted to type {\tt t} in the buffer pointed to
by {\tt p}, which must be large enough to contain them.  Returns {\tt
o} on success, otherwise returns null but does not set the error code.
**/
 
/*
\paragraph{Example.}
If a routine expects either a character string or an integer, the
following code would determine the case and return the value.
\begin{program}
    Object o = input_object_to_check;
    char **cp;
    int i;

    if (DXExtractInteger(o, &i))
        x = i;
    else if (DXExtractString(o, cp))
        strcpy(buffer, *cp);
    else
        printf("bad input\n");
\end{program}
*/




Error DXQueryArrayConvert(Array a, Type t, Category c, int rank, ...);

Error DXQueryArrayConvertV(Array a, Type t, Category c, int rank, int *shape);

/** can the data in Array a be converted into the requested type, category,
rank and shape?  returns OK if yes, sets an error code and returns 
ERROR if not **/

Error DXQueryArrayCommon(Type *t, Category *c, int *rank, int *shape, 
	                 int n, Array a, ...);

Error DXQueryArrayCommonV(Type *t, Category *c, int *rank, int *shape, 
	  	         int n, Array *alist);

/** do a list of arrays have a common data format which is legal for all types,
categories, ranks and shapes to be converted into?  if so, returns the common
format.  if not, sets an error code and returns ERROR **/


Array DXArrayConvert(Array a, Type t, Category c, int rank, ... );

Array DXArrayConvertV(Array a, Type t, Category c, int rank, int *shape);

/** if the data from Array A can be converted into the requested type,
category, rank and shape, returns a new array of that form containing the
converted data.  sets an error code and returns ERROR if the data can't be
converted **/

#endif /* _DXI_EXTRACT_H_ */

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
