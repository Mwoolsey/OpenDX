/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _Ark_h
#define _Ark_h


#include "Base.h"



//
// Class name definition:
//
#define ClassArk	"Ark"

//
// referenced classes
//
class Node;
class ArkStandIn;

//
// Ark class definition:
//				
class Ark : public Base
{
  private:
    //
    // Private member data:
    //
    Node        *from;
    int          fromParameter;
    Node        *to;
    int          toParameter;
    ArkStandIn  *arcStandIn;

  protected:
    //
    // Protected member data:
    //


  public:
    //
    // Constructor:
    //
    Ark(Node *fromNode, int fp, Node *toNode, int tp);

    //
    // Destructor:
    //
    ~Ark();

    //
    // Access routines.
    //

    Node *getSourceNode(int& param)
    {
	param = this->fromParameter;
	return  this->from;
    }

    Node *getDestinationNode(int& param)
    {
	param = this->toParameter;
	return  this->to;
    }
    void setArkStandIn(ArkStandIn* asi)
    {
        this->arcStandIn = asi;
    }
    ArkStandIn* getArkStandIn()
    {
        return this->arcStandIn;
    }

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassArk;
    }
};


#endif // _Ark_h
