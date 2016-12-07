/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _ValueInstance_h
#define _ValueInstance_h


#include "InteractorInstance.h"


class List;
class ValueNode;
class Node;
class Network;
class ControlPanel;
class InteractorStyle;

//
// Class name definition:
//
#define ClassValueInstance	"ValueInstance"


//
// Describes an instance of an interactor in a control Panel.
//
class ValueInstance : public InteractorInstance {

  private:

  protected:

  public:
    ValueInstance(ValueNode *n);
	
    ~ValueInstance(); 

    const char *getClassName() 
	{ return ClassValueInstance; }
};

#endif // _ValueInstance_h

