/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#ifndef _DXTensor_h
#define _DXTensor_h


#include "Base.h"


boolean IsTensor(const char *s, int& index);
boolean IsVector(const char *s, int& index, int& tuple);


//
// Class name definition:
//
#define ClassDXTensor	"DXTensor"

//
// DXTensor class definition:
//				
class DXTensor : public Base
{
  private:
    //
    // Private member data:
    //
	double	 *scalars;		// List of scalar values
	DXTensor **tensors;		// List of vector values
	int	dimensions;		// Number of dimensions in the vector
	char	*dim_sizes;		// Size of each dimension.
	char 	*strval;		// String representing vector value.	
	char	*formatTensor(char *s, int& index);

  protected:
    //
    // Protected member data:
    //




  public:
    //
    // Constructor:
    //
    DXTensor(){ scalars = NUL(double*); 
		tensors = NUL(DXTensor**); 
		dimensions = 0;
		dim_sizes = NUL(char*); 
		strval = NUL(char*);  }

    //
    // Destructor:
    //
    ~DXTensor();

    //
    // Set the vector value from a null terminated string. 
    //
    void printValue();


    //
    // Set the vector value from a null terminated string. 
    //
    boolean setValue(const char* string);
    boolean setValue(const char *s, int& index);

    //
    // Put the vector value in a null terminated string. 
    //
    const char	*getValueString();

    //
    // Return the number of dimensions in this tensor. 
    //
    int		getDimensions() { return dimensions; }

    //
    // Get the number of items in the given dimension.
    // Dimensions are indexed from 1.
    //
    int getDimensionSize(int dim);

    //
    // Return the value of the i'th component of a vector. 
    // indexing is one based.
    //
    double	getVectorComponentValue(int component) 
		{
		    ASSERT(this->getDimensions() == 1);
		    ASSERT(this->dim_sizes[0] >= component);
		    return this->scalars[component-1];
		} 
    int		getVectorComponentCount()
		{
		    return this->getDimensionSize(1); 
		} 
    //
    // Set the value of the i'th component of a vector. 
    // indexing is one based.
    //
    boolean setVectorComponentValue(int component, double val);

    //
    // Make a copy of this vector. 
    //
    DXTensor	*dup();

    //
    // Returns a pointer to the class name.
    //
    virtual const char* getClassName()
    {
	return ClassDXTensor;
    }
};


#endif // _DXTensor_h
