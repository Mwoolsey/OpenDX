/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>



#ifndef _DecoratorInfo_h
#define _DecoratorInfo_h

#include "Base.h"

class Decorator;
class Network;

#define ClassDecoratorInfo "DecoratorInfo"

typedef void (*DragInfoFuncPtr)(void*);

//
// Used by members of WorkSpaceComponent hierarchy so that they can live in
// ControlPanel or EditorWindow without having carnal knowledge of either.
// This obj is created and maintained by anyone with a decorator list.  Decorator
// has a pointer to one of these and uses fields in it to perform some actions.
//
class DecoratorInfo: public Base
{
  friend class Decorator;

  private:
    void*            ownerObj;
    Network*	     net;
    DragInfoFuncPtr  dragPrep;
    DragInfoFuncPtr  dlete;
    DragInfoFuncPtr  select;

  public:

    DecoratorInfo (Network *net, void* owner, 
	DragInfoFuncPtr dragPrep, 
	DragInfoFuncPtr dlete,
	DragInfoFuncPtr select = NULL) 
    {
	this->net      = net;
	this->ownerObj = owner;
	this->dragPrep = dragPrep;
	this->dlete    = dlete;
	this->select   = select;
    }

    ~DecoratorInfo() {};

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
        return ClassDecoratorInfo;
    }
};

#endif // _DecoratorInfo_h
