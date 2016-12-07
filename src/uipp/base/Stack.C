/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#include <string.h> 
#include "Stack.h"


void Stack::push(const void *datum)
{
    this->insertElement(datum,1);  
}

const void *Stack::peek()
{
    return this->getElement(1);
}
const void *Stack::pop()
{
    const void *datum = this->peek();

    if (this->deleteElement(1))
	return datum;
    else
	return NULL;
}
    
void Stack::clear()
{
    this->List::clear();
}




