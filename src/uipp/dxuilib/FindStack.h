/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#ifndef _FindStack_h
#define _FindStack_h


#include "Stack.h"


//
// Class name definition:
//
#define ClassFindStack	"FindStack"

//
// FindStack class definition:
//				
class FindStack : protected Stack 
{

  public:

    FindStack() : Stack() { };

    ~FindStack() { this->clear(); };

    void push(char* name, int instance, char *label);

    boolean pop(char* name, int* instance, char *label);
    
    void clear();

    int getSize() { return this->Stack::getSize(); }

    const char* getClassName()
    {
        return ClassFindStack;
    }
}; 



#endif // _FindStack_h
