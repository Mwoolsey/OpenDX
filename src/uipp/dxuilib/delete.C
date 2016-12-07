/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




void operator delete(void *p)
{
//    ASSERT(p != NULL);

#if defined(MALLOC_DEBUG) && !defined(__PURIFY__)
#ifdef sun4
    malloc_verify();
#endif
#endif

    if (p == NULL)
    {
#if defined(MALLOC_DEBUG)
	printf("void operator delete(void*): Freeing NULL pointer\n");
#endif
    }
    else
	FREE(p);
}
