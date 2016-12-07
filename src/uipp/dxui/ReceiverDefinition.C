/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#include "ReceiverDefinition.h"
#include "ReceiverNode.h"
#include "ItalicLabeledStandIn.h"
#include "ListIterator.h"
#include "Cacheability.h"
#include "ParameterDefinition.h"

NodeDefinition *ReceiverDefinition::AllocateDefinition()
{
    return new ReceiverDefinition;
}


ReceiverDefinition::ReceiverDefinition() : 
    NodeDefinition()
{
}


Node *ReceiverDefinition::newNode(Network *net, int instance)
{
    ReceiverNode *d = new ReceiverNode(this, net, instance);
    return d;
}

SIAllocator ReceiverDefinition::getSIAllocator()
{
   return ItalicLabeledStandIn::AllocateStandIn;
}

//
// Mark all outputs as cache none with read-only cache attributes
// Receivers are not backed up by a node.  They are represented in the generated
// code by only an assignment statement:
//   main_Receiver_1_out_1 = wireless_%d;
//   .
//   .
//   .
//   whatever1 = main_Receiver_1_out_1;
//   whatever2 = main_Receiver_1_out_1;
//
// Even if the generated code looks like:
//   whatever1[cache:1] = main_Receiver_1_out_1;
// The executive would not actually perform caching.
//
// Note:  Transmitter has this problem, also.  (It appears not to have an
// output but it really does.  You just can't see the tab for it.)  Transmitter
// gets a special setting in ui.mdf which sets it cache value to OutputNotCached.
//
void ReceiverDefinition::finishDefinition()
{
    this->NodeDefinition::finishDefinition();
    ParameterDefinition *pd;

    ListIterator iterator(this->outputDefs);
    while ( (pd = (ParameterDefinition*)iterator.getNext()) ) {
	pd->setWriteableCacheability(FALSE);
	pd->setDefaultCacheability(OutputNotCached);
    }
}

