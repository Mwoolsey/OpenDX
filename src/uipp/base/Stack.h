/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#ifndef _Stack_h
#define _Stack_h


#include "List.h"


//
// Class name definition:
//
#define ClassStack	"Stack"

//
// Stack class definition:
//				
class Stack : protected List
{

  public:

    Stack() { };

    ~Stack() { };

    void push(const void *datum);

    const void *peek();
    const void *pop();
    
    void clear();

    int getSize() { return this->List::getSize(); }

    const char* getClassName()
    {
        return ClassStack;
    }
}; 



#endif // _Stack_h
