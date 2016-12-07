/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#include "DXTensor.h"
#include "DXValue.h"
#include "DXStrings.h"
#include "lex.h"

//
//  This does not need to be a member function.
//  It checks to see if the string in s represents a valid Tensor.
//  index is set to point to the character following the lexed tensor.
//
boolean
IsTensor(const char *s, int& index )
{
	DXTensor t;

	return (t.setValue(s,index));
}

//
//  This does not need to be a member function.
//  It checks to see if the string in s represents a valid vector.
//  index is set to point to the character following the lexed vector.
//
boolean
IsVector(const char *s, int& index, int& tuple)
{
        DXTensor t;
	int tindex = index;

        if (t.setValue(s, tindex) && (t.getDimensions() == 1)) {
		tuple = t.getDimensionSize(1);
		index = tindex;
                return TRUE;
	}

        return FALSE;
}



DXTensor::~DXTensor()
{
	int i;

	if (this->dimensions > 1) {
	    int components=this->dim_sizes[0];
	    for (i=0 ; i<components; i++)
		if (this->tensors[i])
		    delete this->tensors[i];
	    if (this->tensors) delete this->tensors;
	} else {
	    if (this->scalars) delete this->scalars;
	}
	if (this->strval) 
	    delete this->strval;
	if (this->dim_sizes)
	    delete this->dim_sizes;
}

DXTensor*
DXTensor::dup()
{
	int i, components;
	DXTensor *t;

	t = new DXTensor;
	t->dimensions = dimensions;
	t->dim_sizes = new char[dimensions];
	ASSERT(t->dim_sizes);
	for (i=0 ; i<dimensions ; i++)
		t->dim_sizes[i] = dim_sizes[i];
	components=dim_sizes[0];

	if (dimensions > 1) {
	    t->tensors = new DXTensor*[components];
	    for (i=0 ; i<components; i++)
		t->tensors[i] = tensors[i]->dup();
	} else {
	    t->scalars = new double[components];
	    ASSERT(t->scalars);
	    for (i=0 ; i<components; i++)
		t->scalars[i] = scalars[i];
	}
	return t;
}


#if 0 // 8/18/93
#include <memory.h>
static void *
Realloc (void *ptr, int size)
{
    char *nptr = new char[size];
    if (ptr)
    {
	memcpy((char*)nptr, ptr, size);
	delete ptr;
    }
    return nptr;	
}
#endif

//
// Set the value of this according to the value parsed from s.
// Return TRUE on success, FALSE on failure.
// On successful return, index points to the character following the
// the last character in the successfully parsed value found in s.
// On failure, index is unchanged.
//
boolean
DXTensor::setValue(const char *s, int& index)
{
	int j;
	int components = 0;
	int ndim = 1, loc_index = index;
	char c;
	boolean saw_value = FALSE, saw_right_bracket = FALSE;
	boolean saw_scalar =  FALSE;
	DXTensor *subv = NUL(DXTensor*); 

	SkipWhiteSpace(s,loc_index);

	if (s[loc_index] != '[')
		return FALSE;
	loc_index++;

	while (s[loc_index]) {
		SkipWhiteSpace(s,loc_index);
		c = s[loc_index];
		if (c == '[') {
			/* Vectors are not allowed if we already saw a scalar  */
			if (saw_scalar)
				goto error;
			subv = new DXTensor;
			if (!subv->setValue(s, loc_index)) {
				delete subv;
				goto error;
			}
			this->tensors = (DXTensor**)REALLOC(this->tensors,
				++components*sizeof(DXTensor*));
			ASSERT(this->tensors);

			this->tensors[components-1] = subv;
			if (ndim == 1) {	/* First sub-component */
				/* Copy the sub-dimensions up to this vector */
				ndim += subv->dimensions;
				this->dim_sizes = (char*)REALLOC(this->dim_sizes, 
					ndim * sizeof(*dim_sizes));
				ASSERT(this->dim_sizes);
				for (j=0 ; j<subv->dimensions ; j++ )
					this->dim_sizes[j+1] = subv->dim_sizes[j];
			} else { 		
				/* Ensure that next elements have the correct dimensionality */
				if (subv->dimensions != ndim-1) {
					delete subv;
					goto error;
				}
				for (j=1 ; j<ndim ; j++) 
					if (this->dim_sizes[j] != subv->dim_sizes[j-1]) {
						this->tensors[components-1] = NULL;
						delete subv;
						goto error;
					}
			}
			saw_value = TRUE;
		} else if (c == ']') {
			if (saw_value == FALSE)
				goto error;
			saw_scalar = FALSE;
			saw_right_bracket = TRUE;
			loc_index++;
			break;
		} else if (c == ',') {
			if (!saw_value)	/* This checks for ',,' */
				goto error;
			saw_value = FALSE;
			saw_scalar = FALSE;
			loc_index++;
		} else {
			/* Scalars are not allowed if we already saw a sub-vector */
			double val;
			int matches, tmp = loc_index;
			if (ndim != 1) goto error;
			if (!IsScalar(s,tmp)) 
				goto error;

			matches = sscanf(&s[loc_index],"%lg", &val);
			if ((matches == 0) || (matches == EOF)) 
				goto error;
			loc_index = tmp;

			this->scalars =(double*)REALLOC(this->scalars,
				++components * sizeof(*scalars));
			ASSERT(this->scalars);
			this->scalars[components-1] = val;
			saw_value = TRUE;
			saw_scalar = TRUE;
		}
	}

	if ((components == 0) || !saw_right_bracket)
		goto error;

	/* Set the dimension at this level of brackets */
	if (!this->dim_sizes)
		this->dim_sizes = new char;
	this->dim_sizes[0] = components;
	this->dimensions = ndim;

	// Be sure that the string representation is cleared when the value changes.
	if (strval) {
		delete strval;
		this->strval = NUL(char*);
	}

	index = loc_index;
	return TRUE;

error:
	return FALSE;

}

void
DXTensor::printValue()
{
    int i;

    printf("Dimensions (%d) Dim Sizes (", this->dimensions);
    for (i=0 ; i<this->dimensions ; i++)
	printf("%d ",this->dim_sizes[i]);

    printf(")\n  Value : '%s'\n",this->getValueString());

}

//
// Parse a string that represents an (N x M x...)-dimensional Tensor and set 
// the value represented into this->scalars[].
// Return TRUE on success, FALSE otherwise.
//
//
boolean
DXTensor::setValue(const char *s)
{
    int i;

    i = 0;
    return this->setValue(s, i);
}

// 
// On entrance, index points to the location to begin insterting characters 
// into s.
// On exit, index points to the NULL in the string.
//
char *
DXTensor::formatTensor(char *s, int& index)
{
	int i,components = dim_sizes[0];

	s[index++] = '[';
	s[index++] = ' ';
	s[index] = '\0';

	if (dimensions > 1) {
	    for (i=0 ; i<components; i++) {
		tensors[i]->formatTensor(s,index);
		if (i != (components-1))  
		    s[index++] = ' ';
	   	s[index] = '\0';
	    }
	} else {
	    for (i=0 ; i<components; i++) {
		char buf[128];
		DXValue::FormatDouble(scalars[i], buf);
		int cnt = strlen(buf);	
		strcpy(&s[index],buf);
		index += cnt;
		if (i != (components-1)) s[index++] = ' ';
	   	s[index] = '\0';
	    }
	}

	s[index++] = ' ';
	s[index++] = ']';
	s[index] = '\0';	
	
	return s;
}

//
// Return the a pointer to a string value of a Tensor.  
// If you intend to use the string after the Tensor is destroyed or its value
// has changed you should make a copy of the string passed back.
//
const char *
DXTensor::getValueString()
{
	int i, cnt;
	
	if (strval)
		return (strval); 

#if 0	
	values = 1;
	for (i=0 ; i<dimensions ; i++) 
		values *= dim_sizes[i]; 
	brackets = 1;
	for (i=0 ; i<dimensions-1 ; i++) 
		brackets += 2 * dim_sizes[i] * brackets;
	brackets += 2;	// For 2 outer brackets
	cnt =  9 * values + brackets + (brackets + values);
#else
	cnt = 128;	// FIXME
#endif

	strval = new char[cnt]; 

	i = 0;
	formatTensor(strval,i);

	// Let's be sure we didn't overrun the array.
	ASSERT(i < cnt);

	return (strval);
	
}
boolean DXTensor::setVectorComponentValue(int component, double val)
{
    ASSERT(this->getDimensions() == 1);
    ASSERT(this->dim_sizes[0] >= component);
    this->scalars[component-1] = val;
    if (this->strval) {
	delete this->strval;
	this->strval = NULL;
    }
    return TRUE;
}

//
// Get the number of items in the given dimension.
// Dimensions are indexed from 1.
//
int DXTensor::getDimensionSize(int dim)
{
    ASSERT(dim > 0);
    ASSERT(this->getDimensions() >= dim);
    ASSERT(this->dim_sizes);
    
    return this->dim_sizes[dim-1];
}
