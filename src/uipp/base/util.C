/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
#include <string.h>

#include <dxconfig.h>
#include "../base/defines.h"


int
STRLEN(char *a)
{
    if (!a) return 0;
    else return strlen(a);
}

int
STRCMP(char *a, char *b)
{
    if (!a || !b) 
        if (!a) return strcmp("", b);
	else return strcmp(a, "");
    else return strcmp(a, b);
}

int
STRNCMP(char *a, char *b, int n)
{
    if (!a || !b) 
        if (!a) return strncmp("", b, n);
	else return strncmp(a, "", n);
    else return strncmp(a, b, n);
}

#if !defined(HAVE_STRRSTR)
extern "C"
char *
strrstr(char *a, char *b)
{
    char *l, *n;
    l = strstr(a, b);
    if (l)
    {
	while ( (n = strstr(l+1, b)) )
	    l = n;
    }
    return l;
}
#endif
