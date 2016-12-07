/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include <defines.h>



#include <Xm/Xm.h>
#include <Xm/AtomMgr.h>

#include "TransferStyle.h"
#include "Application.h"

TransferStyle::TransferStyle (int tag, const char *name, boolean preferred)
{
    ASSERT(name);
    this->preferred = preferred;
    this->atom = XmInternAtom (theApplication->getDisplay(), (String)name, False);
    this->atomName = theSymbolManager->registerSymbol(name);
    this->name = new char[1+strlen(name)];
    this->tag = tag;
    strcpy (this->name, name);
}


TransferStyle::~TransferStyle()
{
    delete this->name;
}

