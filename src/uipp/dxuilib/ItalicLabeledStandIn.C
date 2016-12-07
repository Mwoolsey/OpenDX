/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>





#include "StandIn.h"
#include "ItalicLabeledStandIn.h"
#include "Node.h"
#include "DXApplication.h"


boolean ItalicLabeledStandIn::ClassInitialized = FALSE;
String ItalicLabeledStandIn::DefaultResources[] = {
 	NULL
};

//
// Static allocator found in theSIAllocatorDictionary
//
StandIn 
*ItalicLabeledStandIn::AllocateStandIn(WorkSpace *w, Node *node)
{
    StandIn *si = new ItalicLabeledStandIn(w,node);
    si->createStandIn();
    return si;

}

ItalicLabeledStandIn::ItalicLabeledStandIn(WorkSpace *w, Node *node):
                 LabeledStandIn(w,node)
{
}

//
// Destructor
//
ItalicLabeledStandIn::~ItalicLabeledStandIn()
{
}

void ItalicLabeledStandIn::initialize()
{
    if (!ItalicLabeledStandIn::ClassInitialized) {
        ItalicLabeledStandIn::ClassInitialized = TRUE;
        this->setDefaultResources(theApplication->getRootWidget(),
                                  ItalicLabeledStandIn::DefaultResources);
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
const char *ItalicLabeledStandIn::getPostScriptLabelFont()
{
    return "Helvetica-Oblique";
}

const char *ItalicLabeledStandIn::getButtonLabelFont()
{
    return "big_oblique"; 
}

