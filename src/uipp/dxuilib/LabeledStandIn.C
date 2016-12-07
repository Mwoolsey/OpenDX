/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include <defines.h>





#include "StandIn.h"
#include "LabeledStandIn.h"
#include "Node.h"
#include "DXApplication.h"


boolean LabeledStandIn::ClassInitialized = FALSE;

String LabeledStandIn::DefaultResources[]  =  {
	NULL
};

//
// Static allocator found in theSIAllocatorDictionary
//
StandIn 
*LabeledStandIn::AllocateStandIn(WorkSpace *w, Node *node)
{
    StandIn *si = new LabeledStandIn(w,node);
    si->createStandIn();
    return si;

}

LabeledStandIn::LabeledStandIn(WorkSpace *w, Node *node):
                 StandIn(w,node)
{
}

//
// Destructor
//
LabeledStandIn::~LabeledStandIn()
{
}

void LabeledStandIn::initialize()
{
    if (!LabeledStandIn::ClassInitialized) {
        LabeledStandIn::ClassInitialized = TRUE;
        this->setDefaultResources(theApplication->getRootWidget(),
                                  LabeledStandIn::DefaultResources);
        this->setDefaultResources(theApplication->getRootWidget(),
                                  StandIn::DefaultResources);
    }

    //
    // Call the superclass initialize()
    //
    StandIn::initialize();

}

const char *LabeledStandIn::getButtonLabel()
{
   return this->node->getLabelString();
}
