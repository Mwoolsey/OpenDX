/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#include "CDFChoice.h"
#include "GARApplication.h"

boolean CDFChoice::ClassInitialized = FALSE;
String CDFChoice::DefaultResources[] =
{
    NUL(char*)
};

void CDFChoice::initialize()
{
    if (!CDFChoice::ClassInitialized) {
	CDFChoice::ClassInitialized = TRUE;
	this->setDefaultResources
	    (theApplication->getRootWidget(), TypeChoice::DefaultResources);
	this->setDefaultResources
	    (theApplication->getRootWidget(), ImportableChoice::DefaultResources);
	this->setDefaultResources
	    (theApplication->getRootWidget(), CDFChoice::DefaultResources);
    }
}
