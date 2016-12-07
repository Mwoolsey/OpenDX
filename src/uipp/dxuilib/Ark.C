/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"





#include "Ark.h"
#include "ArkStandIn.h"
#include "Node.h"
#include "StandIn.h"



Ark::Ark(Node *fromNode, int fp, Node *toNode, int tp)
{
        this->arcStandIn = NULL;

        this->from = fromNode;
        this->fromParameter = fp;
        this->to = toNode;
        this->toParameter = tp;
        this->from->addOutputArk(this,fp);
        this->to->addInputArk(this,tp);
}

Ark::~Ark()
{
     Node*      sourceNode;
     Node*      destNode;
     StandIn*   srcStandIn;
     StandIn*   destStandIn;
     int        sourceIndex, destIndex;

     sourceNode = this->getSourceNode(sourceIndex);
     destNode   = this->getDestinationNode(destIndex);

     // 
     // remove the arc from the approiate lists.
     //
     this->to->deleteArk(this);

     //
     // If a standIn exists update the UI.
     //
     if (this->arcStandIn != NULL) {
         delete this->arcStandIn;
         srcStandIn = sourceNode->getStandIn();
	 if (srcStandIn)
             srcStandIn->drawTab(sourceIndex, True);
         destStandIn = destNode->getStandIn();
	 if (destStandIn)
             destStandIn->drawTab(destIndex, False);
     }

    
}


