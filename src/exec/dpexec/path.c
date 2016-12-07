/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#include <dx/dx.h>
#include "path.h"
#include "utils.h"


#define	EX_MAXPATH	4096
#define	EX_I_SEP	':'		/* instance #   separator	*/
#define	EX_M_SEP	'/'		/* macro/module separator	*/
#define	EX_END		'\000'		/* NULL				*/
#define	EX_NUM_LEN	11		/* max printable length of int	*/
#define	EX_PATHLEN(l)	((l)+EX_NUM_LEN+3)	/* chars to add path	*/


char
*_dxf_ExPathStrPrepend (char *name, int instance, char *path)
{
    static char localPath[EX_MAXPATH];
    char *tail = localPath;
    int l = strlen (name);
    int np;

    *(tail++) = EX_M_SEP;
    strcpy (tail, name);
    tail += l;
    *(tail++) = EX_I_SEP;
    if (instance < 0 || instance > 999)
    {
		/* sprintf automatically appends \0 to the end of a 
		   string so why include it twice? */
        /* was -- sprintf(tail, "%d\0", instance); */
        sprintf(tail, "%d", instance);
        l = strlen(tail);
        tail += l;
    } 
    else if (instance > 99)
    {
	np = instance / 100;
	*(tail++) = np + '0';
	instance -= np * 100;
	np = instance / 10;
	*(tail++) = np + '0';
	instance -= np * 10;
	*(tail++) = instance + '0';
    }
    else if (instance > 9)
    {
	np = instance / 10;
	*(tail++) = np + '0';
	instance -= np * 10;
	*(tail++) = instance + '0';
    }
    else
	*(tail++) = instance + '0';
    *(tail) = '\0';

    if (path != NULL)
	strcat(tail, path);

    return localPath;
}
