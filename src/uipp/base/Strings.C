/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include <defines.h>


#if defined(HAVE_SYS_ERRNO_H)
#include <sys/errno.h>
#endif

#if defined(HAVE_ERRNO_H)
#include <errno.h>
#endif

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#if defined(HAVE_PWD_H)
#include <pwd.h>
#endif

#if defined(HAVE_DIRECT_H)
#include <direct.h>
#define getcwd _getcwd
#endif

#include <stdarg.h>

#include "DXStrings.h"
#include "ctype.h"
#include "lex.h"

//
// The alpha has it but has not header for it
//
#if defined(DXD_NEEDS_MKTEMP) || defined(alphax)
extern "C" char *mktemp(char *);
#endif

#if defined(intelnt)
#include <io.h>
#include <string.h>
#include <stdio.h>

#define  mktemp   _mktemp
#define  unlink   _unlink
#endif

char* DuplicateString(const char* string)
{
    char* new_string;

    if (string == NUL(char*))
    {
	return NUL(char*);
    }

    //
    // Get a new string.
    //
    new_string = new char[STRLEN(string) + 1];

    //
    // Copy the string and return the new string.
    //
    strcpy(new_string, string); 

    return new_string;
}


boolean IsBlankString(const char* string)
{
    int i;

    //
    // If the string is NULL, then return TRUE...
    //
    if (string == NUL(char*))
    {
	return TRUE;
    }

    //
    // Otherwise, return TRUE if the string is a NULL string or
    // if the string consists solely of blanks/tabs.
    //
    for (i = 0; string[i] == ' ' OR string[i] == '\t'; i++)
	;

    return string[i] == '\0';
}

//
// Given a pathname or a filename, construct a unique filename in the given
// directory (filename implies directory of given file).  If NULL is given,
// then construct a unique filename in the current director.
// The returned string must be freed by the caller.
// If a unique filename could not be created, NULL is returned.
//
char *UniqueFilename(const char *filename)
{
    char *unique;
    char *path;

    if (filename) 
	path = GetDirname(filename);
    else
	path = NULL;

    if (!IsBlankString(path)) {
	unique = new char[strlen(path) + 12];
	strcpy(unique, path);
	strcat(unique,"/tmpXXXXXX");
	delete[] path;
    } else {
	if (path)
	    delete[] path;
	unique = DuplicateString("tmpXXXXXX");
    }

    if (!mktemp(unique)) {
	delete[] unique;	
	unique = NULL;
    }

    return unique;
}
char* GetDirname(const char* pathname)
{
    int   i;
    char* path;

    if (pathname == NUL(const char*))
    {
        return NUL(char*);
    }

    path = DuplicateString(pathname);

#ifdef DXD_NON_UNIX_DIR_SEPARATOR                               //SMH trap DOS style separators
    Dos2UnixPath(path);
    for (i = STRLEN(path) - 1; i >= 0 AND path[i] != '/' AND path[i] != ':'; i--)
        ;
    if (path[i] == ':')
	i++;
#else
    for (i = STRLEN(path) - 1; i >= 0 AND path[i] != '/'; i--)
        ;
#endif

    if (i < 0)
        i = 0;

    path[i] = NUL(char);

    return path;
}

char* GetFileBaseName(const char* pathname, const char* extension)
{
    int   i;
    int   j;
    int   length;
    char* name;
    char* temp;

    if (pathname == NUL(const char*))
    {
        return NUL(char*);
    }

    name = DuplicateString(pathname);

#ifdef DXD_NON_UNIX_DIR_SEPARATOR                               //SMH trap DOS style separators
    Dos2UnixPath(name);
#endif

    /*
     * Get rid of any trailing blanks.
     */
    for (j = STRLEN(name) - 1;
         (name[j] == '\t' OR name[j] == ' ') AND j >= 0;
         j--)
        ;

    name[++j] = NUL(char);

    /*
     * If an extension exists, lop it off.
     */
    if (extension)
    {
        length = STRLEN(extension);
        if (EqualString(&name[j - length], extension))
        {
            j -= length;
            name[j] = NUL(char);
        }
    }

    /*
     * Search for the last directory separator.
     */
#ifdef DXD_NON_UNIX_DIR_SEPARATOR
    for (i = j - 1; i>=0 AND name[i] != '/' AND name[i] != ':' ; i--)
        ;
#else
    for (i = j - 1; i>=0 AND name[i] != '/' ; i--)
        ;
#endif

    /*
     * Copy the name and delete the old string.
     */
    temp = name;
    name = DuplicateString(&name[i + 1]);
    delete[] temp;

    return name;
}

char* GetFullFilePath(const char* oldPath)
{
    char path[2048];

    ASSERT(oldPath);

    if (oldPath[0] == '\0')
    {
        (void)getcwd(path, 1024);
    }
    else if (oldPath[0] == '.')
    {
        (void)getcwd(path, 1024);
        (void)strcat(path, "/");
        (void)strcat(path, oldPath);
    }
    else if (oldPath[0] == '~')
    {
#ifdef DXD_OS_NON_UNIX     //SMH home directory not really defined in NT
        printf("Can't use ~ when running ui on NT or Windows\n");
#else
        char* home;

        if (oldPath[1] == '/' || oldPath[1] == '\0')
        {
            home = getenv("HOME");
            if (home == NULL)
            {
                strcpy(path, oldPath);
            }
            else
            {
                strcpy(path, home);
                strcat(path, oldPath + 1);
            }
        }
        else
        {
            char user[1024];
            struct passwd *ent;
            int i;

            /*
             * Get username.
             */
            for (i = 1; oldPath[i] && oldPath[i] != '/'; ++i)
            {
                user[i - 1] = oldPath[i];
            }
            user[i -1] = '\0';
            ent = getpwnam(user);

            if (ent == NULL)
            {
                strcpy(path, oldPath);
            }
            else
            {
                strcpy(path, ent->pw_dir);
                strcat(path, oldPath + i);
            }
        }
#endif    
    }
#ifndef DXD_NON_UNIX_DIR_SEPARATOR
    else if (oldPath[0] != '/')
#else
    //SMH recognize NT drive letters as root pat
    else if (!( (oldPath[0] == '/') || (oldPath[0] == '\\') ||
		((oldPath[1] == ':')&&(isalpha(oldPath[0]))) ))
#endif
    {
        getcwd(path, 1024);
        strcat(path, "/");
        strcat(path, oldPath);
    }
    else
    {
        strcpy(path, oldPath);
    }

    return FilterDottedPath(path);
}

char* FilterDottedPath(const char* oldPath)
{
    char* path;
    int   i;
    int   j;

    ASSERT(oldPath);

    path = DuplicateString(oldPath);

    DOS2UNIXPATH(path);

    i = j = 0;
    while(TRUE)
    {
        if (path[j] == '.')
        {
            j++;
            if (path[j] == '.')
            {
                j++;
                if (path[j] == '/')
                {
                    for(i--; path[i] != '/'; i--)
                        ;

                    if (i > 0)
                    {
                        for(i--; path[i] != '/'; i--)
                            ;
                    }
                }
                else if (path[j] == '\0')
                {
                    for(i--; path[i] != '/'; i--)
                        ;

                    if (i > 0)
                    {
                        for(i--; path[i] != '/'; i--)
                            ;
                    }

                    if (i == 0)
                    {
                        i++;
                    }
                }
                else
                {
                    j -= 2;
                }
            }
            else if (path[j] == '/')
            {
                j++;
            }
            else if (path[j] == '\0')
            {
                if (i - 1 > 0)
                {
                    i--;
                }
            }
            else
            {
                j--;
            }
        }

        path[i] = path[j];

        if (NOT path[i])
            break;

        i++;
        j++;
    }

    return path;
}

#if 0
//
//  duplicate a string, but put the characters in reverse order.
//
static char *strrev(const char *s)
{
   int len = STRLEN (s);
   char *t = new char [len +1];
  
   for (i=0, j=len-1 ; i<len ; i++, j--)
	t[i] = s[j];
   t[len + 1] = '\0';

   return t;
}
#endif

#ifdef NEEDS_STRRSTR
//
// Find the last occurence of s2 in s1.
// Returns a pointer to last occurence or NULL.
//
const char *strrstr(const char *s1, const char *s2)
{
   ASSERT(s1 && s2);

   int len1 = STRLEN(s1); 
   int len2 = STRLEN(s2); 
   int i;

   if (len2 <= len1) {
       for (i=len1-len2 ; i>=0 ; i--) {
	    if (!strncmp(&s1[i],s2,len2))
		return &s1[i];
       }
   }

   return NULL;
}
#endif

#ifdef NON_ANSI_SPRINTF
int
_sprintf(char *s, const char* format, ...)
{
    va_list ap;

    va_start(ap, format);
    vsprintf(s,format,ap);	
    va_end(ap);

    return STRLEN(s);
}
#endif 	// NON_ANSI_SPRINTF

//
// Find the first string in s that is delimeted by the 'begin' and 'end'
// characters. 
// Return NULL if none found otherwise a string that contains the begin and
// end characters that must be freed by the caller.
// If buf is NULL a new strings is allocated and must be deleted by the caller.
// if buf is not NULL it must be large enough to hold the resulting string and
// on success contains the delimited string.
// If nest_chars is given as a string of nesting characters (i.e. '"'), then
// if the end delimiter is found within the nesting characters then don't 
// accept it as a terminating delimiter. This is useful for finding 
// '"{a} {b}"' within '{"{a} {b}"}'.
//
char *FindDelimitedString(const char *s, char begin, char end, 
		char *buf, const char *nest_chars)
{
   char *ret=NULL;
 
   ASSERT(s);

   while (*s && (*s != begin))
	s++;

   if (*s == begin) {
#if 00
	if (buf) {
	    char *b = buf;
	    p = (char*)s;
	    do {
	    	*b = *p;
		b++; p++;
	    } while (*p && (*p != end));
	    if (*p == end) {
		*b = end;
		*(++b) = '\0';
	        ret = buf;
	    } else
		ret = NULL;
	} else 
#endif
	{
	    char nesting_char = '\0';
	    const char *src = s; 
	    char *dest;
	    
	    if (buf) {
		ret = dest = buf; 
	    } else {
		ret = dest = new char [STRLEN(s) + 1];
	    }
	    *dest = *src;
	    dest++; src++;	// Skip first delimiter (in case begin == end)

	    while (*src && ((*src != end) || nesting_char))  {
		//
		// Look for a nested string that can include the last delimiter
		//
		if (nest_chars) {
		    if (nesting_char)  {	// Check for end nesting char
			if (nesting_char == *src)
			    nesting_char = '\0';
		    } else {			// Check for begin nesting char
			if (strchr(nest_chars, *src))
			    nesting_char = *src;
		    }
		}
	        *dest = *src;
	        src++; 
		dest++;
	    }

	    if (*src == end) {
		*dest = end;
		*(++dest) = '\0';
	    } else {
		ret = NULL;
	    }
	}
   }

   return ret; 
}

//
// Convert all escaped characters (i.e. \n, \t, \r, \f, \b and \123)
// into a new string with the real characters.
// The returned string must be freed by the caller.
//
char *DeEscapeString(const char *str)
{
    ASSERT(str);
 
    char *newstr = new char[STRLEN(str) + 1]; 
    char c, *pnew = newstr;
    const char *pstr = str;


    while ( (c = *pstr) ) {
	switch (c) {
	    case '\\':
		pstr++;
		switch (*pstr) {
		    case 'n': *pnew = '\n'; break;             /* '\n'  */
		    case 't': *pnew = '\t'; break;             /* '\t'  */
		    case 'r': *pnew = '\r'; break;             /* '\r'  */
		    case 'f': *pnew = '\f'; break;             /* '\r'  */
		    case 'b': *pnew = '\b'; break;             /* '\b'  */
		    default:
			if (isdigit(*pstr)) {
			    /* DXExtract an 1, 2 or 3 digit octal number */
			    int num = 0, i = 0; 
			    pstr--;    /* Prime the loop */
			    do {
				pstr++;
				int digit = *pstr - '0';
				if (digit >= 8)   
				    break;
				num = num*8 + *pstr - '0';
			    } while ((++i < 3) && isdigit(*(pstr+1)));
			    if (num > 0xff) 
				    break;
			    *pnew = (char)num;
			} else {
			    *pnew = *pstr;
		    	}
		    }
		break;
	    default:
		*pnew = *pstr;
		break;
	}
	pnew++;
	pstr++;
    }
    *pnew = '\0';
    return newstr;
}

//
// Strip all the white space characters out of the given string.
// If the string contains all white space, then a zero-length string
// is returned.  If the given string is NULL, NULL is returned.
// The returned string must be deleted by the caller.
//
char* StripWhiteSpace(const char* string)
{
    char *p, *new_string;
    int n;

    //
    // Count non-space characters
    //
    for (n=0, p=(char*)string; *p ; p++)
	if (!IsWhiteSpace(p))
	    n++; 

    //
    // Allocate and fill string 
    //
    new_string = new char[n + 1];
    new_string[0] = '\0';

    for (n=0, p=(char*)string; *p ; p++) {
	if (!IsWhiteSpace(p)) {
	    new_string[n] = *p;
	    n++; 
	} 
    }
    new_string[n] = '\0';

    return new_string;
}

void Dos2UnixPath(char *path)
{
    unsigned int i;

    for (i=0; i<STRLEN(path); i++)
      if (path[i]=='\\')
        path[i]='/';
}
void Unix2DosPath(char *path)
{
    unsigned int i;

    for (i=0; i<STRLEN(path); i++)
      if (path[i]=='/')
        path[i]='\\';
}

boolean unRename(const char *SrcFile, const char *DestFile)
{
    FILE *fpSrc, *fpDest;

    if(!SrcFile || !DestFile)
	return FALSE;

    fpSrc = fopen(SrcFile, "rb");
    if (fpSrc == NULL) return FALSE;

    fpDest = fopen(DestFile, "wb");
    if (fpDest == NULL) {
	fclose(fpSrc);
	return FALSE;
    }

    while(!feof(fpSrc)) {
	fputc(fgetc(fpSrc), fpDest);
    }
    fclose(fpSrc);
    fclose(fpDest);
    unlink(SrcFile);

    return TRUE;


}

