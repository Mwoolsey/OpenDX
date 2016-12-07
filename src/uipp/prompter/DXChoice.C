/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#include "DXChoice.h"
#include "GARApplication.h"


boolean DXChoice::ClassInitialized = FALSE;
String DXChoice::DefaultResources[] =
{
    NUL(char*)
};


void DXChoice::initialize() 
{
    if (!DXChoice::ClassInitialized) {
	DXChoice::ClassInitialized = TRUE;
	this->setDefaultResources
	    (theApplication->getRootWidget(), TypeChoice::DefaultResources);
	this->setDefaultResources
	    (theApplication->getRootWidget(), ImportableChoice::DefaultResources);
	this->setDefaultResources
	    (theApplication->getRootWidget(), DXChoice::DefaultResources);
    }
}

