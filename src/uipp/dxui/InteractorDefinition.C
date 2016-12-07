/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#include "InteractorDefinition.h"
//#include "InteractorNode.h"
#include "SIAllocatorDictionary.h"
#include "InteractorStandIn.h"
//#include "ParameterDefinition.h"
//#include "List.h"
//#include "ListIterator.h"


//
// Define the stand-in that is used for InteractorNode and derived classes.
//
SIAllocator InteractorDefinition::getSIAllocator()
{
    return InteractorStandIn::AllocateStandIn;
}
