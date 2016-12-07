/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include <dx/arch.h>
#include "utils.h"
#include "dx.h"
#include <stdio.h>

#if defined(intelnt) || defined(cygwin) || defined(WIN32)

#if defined(HAVE_WINDOWS_H)
#include <windows.h>
#endif

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#if defined(HAVE_ERRNO_H)
#include <errno.h>
#endif

#if defined(HAVE_PROCESS_H)
#include <process.h>
#endif

void d2u(char *s)
{
#if defined(intelnt) || defined(WIN32)
    int i;
    for (i=0; s && *s && (i<strlen(s)); i++)
	if (s[i] == '\\')
	    s[i] = '/';
#endif
}

void u2d(char *s)
{
#if defined(intelnt) || defined(WIN32)
    int i;
    for (i=0; s && *s && (i<strlen(s)); i++)
	if (s[i] == '/')
	    s[i] = '\\';
#endif
}

#if 0
void p2des(char *s) /* path to dos with extra seperator */
{
	namestr temp;
    int i, j, length;
    length=strlen(s);
    for(i=0, j=0; i<length; i++) {
    	if(s[i] == '/' || s[i] == '\\') {
    		temp[j++] = '\\'; temp[j++] = '\\';
    	} else
    		temp[j++] = s[i];
	}
    temp[j] = '\0';
    strcpy(s, temp);
}
#endif

void removeQuotes(char *s)
{
    char *p, *p2; p = s; p2 = s;
    while(p && *p) {
	while(*p && p && *p == '"') p++;
	*p2 = *p; p2++; p++;
    }
    *p2 = '\0';
}

void addQuotes(char *s)
{
    int i, length;
    length=strlen(s);
    for(i=length; i>0; i--)
	s[i]=s[i-1];
    s[length+1] = '"';
    s[length+2] = '\0';
    s[0] = '"';
}

int getenvstr(char *name, char *value)
{
    char *s=NULL;

    s = getenv(name);
    if (s)
	strcpy(value, s);

    return ((s && *s) ? 1 : 0);
}

/*  In Windows, the name/value pair must be separated by = 	*/
/*  with no blanks on either side of =.  Null values would have */
/*  name=\0 to unset them in the environment.			*/
int putenvstr(char *name, char *value, int echo)
{
    char s[MAXENV];
    char *p, *q;
    int rc;
    int len;
    int newlen;

    if (!*name)
	return 0;

    if (!*value)
        return 0;

    for(p = name; *p == ' '; p++);
    for(q = &name[strlen(name)-1]; *q == ' ' && q != p; q--);

    len = (int)(q-p)+1;
    strncpy(s, p, len);
    s[len] = '\0';
    strcat(s, "=");

    /* All env params except path and MAGICK_HOME should be Unix style */
    if (strcasecmp(s, "path=") && strcasecmp(s, "magick_home="))
	d2u(value);

    for(p = value; *p == ' '; p++);
    if(strlen(p)) {
	for(q = &value[strlen(value)-1]; *q == ' ' && q != p; q--);
	if (*p != ' ') {
	    newlen = strlen(s);
	    len = (int)(q-p)+1;
	    strncat(s, p, len);
	    s[len+newlen] = '\0';
	}
    }

    p = malloc(strlen(s) + 1);
    strcpy(p, s);
    
    rc = _putenv(p);
    if (echo)
		printf("%s\n", s);
    return (!rc ? 1 :0);
}

#endif
