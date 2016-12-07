/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"

#ifndef _DXStrings_h
#define _DXStrings_h

#include <string.h>

#ifdef DXD_NEEDS_CTYPE_H
#include <ctype.h>                  //SMH to get isalpha prototype
#endif

#if defined(DXD_NON_UNIX_DIR_SEPARATOR) || defined(DXD_OS_NON_UNIX)
#define UNIX2DOSPATH(a) (Unix2DosPath(a))
#define DOS2UNIXPATH(a) (Dos2UnixPath(a))
#else
#define UNIX2DOSPATH(a)
#define DOS2UNIXPATH(a)
#endif


#ifdef NEEDS_STRRSTR
//
// Find the last occurence of s2 in s1.
// Returns a pointer to last occurence or NULL.
//
const char *strrstr(const char *s1, const char *s2);
#endif

//
// Compares two strings and returns TRUE if they are equal, FALSE otherwise.
//
#define EqualString(first, second) \
	(!STRCMP((first), (second)))


//
// Compares two substrings; returns TRUE if first n characters are equal,
//  FALSE otherwise.
//
#define EqualSubstring(first, second, n)	\
	 (!STRNCMP((first), (second), (n)))

#ifdef NON_ANSI_SPRINTF
  int _sprintf(char *s, const char* format, ...);
# define SPRINTF	_sprintf
#else
# define SPRINTF	sprintf
#endif

#define STRLEN(a)	((a != NULL) ? strlen(a) : 0)

# define STRCMP(a,b) 	((a) ? ((b) ? strcmp(a,b)  \
                                    : strcmp(a,"")) \
                             : ((b) ? strcmp("",b) \
                                    : 0))

# define STRNCMP(a,b,n)    ((a) ? ((b) ? strncmp(a,b,n)  \
                                    : strncmp(a,"",n)) \
                             : ((b) ? strncmp("",b,n) \
                                    : 0))    


#ifdef NEEDS_STRERROR
extern
char *strerror(int errnum);
#endif

//
// Duplicates a given string.
//
extern
char* DuplicateString(const char* string);

//
// Returns TRUE if the entire string consists of blanks/tabs; FALSE, otherwise.
//
extern
boolean IsBlankString(const char* string);

extern
char* GetDirname(const char*);

extern
char* GetFileBaseName(const char* path, const char* extension);

extern
char* GetFullFilePath(const char*);

extern
char* FilterDottedPath(const char*);

//
// Find the first string in s that is delimited by the 'begin' and 'end'
// characters.
// Return NULL if none found otherwise a string that contains the begin and
// end characters. 
// If buf is NULL a new strings is allocated and must be deleted by the caller.
// if buf is not NULL it must be large enough to hold the resulting string and
// on success contains the delimited string.
// If nest_chars is given as a string of nesting characters (i.e. '"'), then
// if the end delimiter is found within the nesting characters then don't 
// accept it as a terminating delimiter. This is useful for finding 
// '"{a} {b}"' within '{"{a} {b}"}'.
//
char *FindDelimitedString(const char *s, char begin, char end, 
				char *buf=NULL, const char *nest_chars = NULL);

//
// Convert all escaped characters (i.e. \n, \t, \r, \f, \b and \123)
// into a new string with the real characters.
// The returned string must be freed by the caller.
//
char *DeEscapeString(const char *str);

//
// Given a pathname or a filename, construct a unique filename in the given
// directory (filename implies directory of given file).  If NULL is given,
// then construct a unique filename in the current director.
// The returned string must be freed by the caller.
// If a unique filename could not be created, NULL is returned.
//
char *UniqueFilename(const char *filename);

//
// Strip all the white space characters out of the given string.
// If the string contains all white space, then a zero-length string
// is returned.  If the given string is NULL, NULL is returned.
// The returned string must be deleted by the caller.
//
char* StripWhiteSpace(const char* string);
void Dos2UnixPath(char *path);
void Unix2DosPath(char* path);
boolean unRename(const char *SrcFile, const char *DestFile);

#endif /* _DXStrings.h */
