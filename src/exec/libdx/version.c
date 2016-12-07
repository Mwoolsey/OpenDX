/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#include <dx/dx.h>

/* routines to deal with the product level version numbers of dx 
 */
Error
DXVersion(int *version, int *release, int *modification)
{
    if (version)
	*version = DXD_VERSION;
    
    if (release)
	*release = DXD_RELEASE;
    
    if (modification)
	*modification = DXD_MODIFICATION;

    return OK;
}


Error
DXVersionString(char **name)
{
    if (*name)
	*name = DXD_VERSION_STRING;

    return OK;
}


/* these routines are only needed by specific parts of the system which
 *  have to worry about interface changes.
 */

Error
DXSymbolInterface(int *version)
{
    if (version)
	*version = DXD_SYMBOL_INTERFACE_VERSION;

    return OK;
}

Error
DXOutboardInterface(int *version)
{
    if (version)
	*version = DXD_OUTBOARD_INTERFACE_VERSION;

    return OK;
}

Error
DXHwddInterface(int *version)
{
    if (version)
	*version = DXD_HWDD_INTERFACE_VERSION;

    return OK;
}

