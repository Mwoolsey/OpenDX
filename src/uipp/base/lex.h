/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


//////////////////////////////////////////////////////////////////////////////
// lex.h -								    //
//                                                                          //
// Contains various string lexing routines.				    //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

#ifndef _lex_h
#define _lex_h


#include "DXStrings.h"

//
// Tells whether the specified character is a white space or not.
//
inline
boolean IsWhiteSpace(const char *string, int& index)
{
    return string[index] == ' ' || string[index] == '\t';
}
inline
boolean IsWhiteSpace(const char *string)
{
    return *string == ' ' || *string == '\t';
}
//
// Increments the specified index until the corresponding characters in the
// string no longer contain space characters.
//
inline
void SkipWhiteSpace(const char* string,
	       int&  index)
{
    while (IsWhiteSpace(string, index))
	index++;
}
inline
void SkipWhiteSpace(char*& p)
{
    while (IsWhiteSpace(p))
	p++;
}

#if NOT_USED
inline 
char FindWhiteSpace(const char* string, int&  index)
{
    while (string[index] AND 
	   (string[index] != ' ' AND string[index] != '\t')) index++;
    return string[index];
}
#endif
inline
char FindWhiteSpace(char*& p)
{
    while (*p AND *p != ' ' AND *p != '\t') p++;
    return *p;
}

boolean IsAllWhiteSpace(const char *string);

//
// Returns TRUE if an integer string is found; returns FALSE, otherwise.
// The index variable is updated to the character following the lexed
// string upon successful return.
//
boolean IsEndOfString(const char* string,
		      int&        index);

//
// Returns TRUE if an integer string is found; returns FALSE, otherwise.
// The index variable is updated to the character following the lexed
// string upon successful return.
//
boolean IsInteger(const char* string,
		  int&        index);

//
// Returns TRUE if a flag string is found; returns FALSE otherwise.
// The index variable is updated to the character following the lexed
// string upon successful return.
//
boolean IsFlag(const char* string,
	       int&        index);

//
// Returns TRUE if a scalar string is found; returns FALSE otherwise.
// The index variable is updated to the character following the lexed
// string upon successful return.
//
boolean IsScalar(const char* string,
		 int&        index);
//
// Returns TRUE if a "string" string is found; returns FALSE otherwise.
// The index variable is updated to the character following the lexed
// string upon successful return.
//
boolean IsString(const char* string,
		 int&        index);

//
// Returns TRUE if an identifier string is found; returns FALSE otherwise.
// The index variable is updated to the character following the lexed
// string upon successful return.
//
boolean IsIdentifier(const char* string,
		     int&        index);
//
// Returns TRUE if the string is an identifier; returns FALSE otherwise.
//
boolean IsIdentifier(const char* string);

//
// Returns TRUE if a restricted identifier string is found; returns FALSE
// otherwise.  A restricted identifier does not contain an underscore or "@".
// The index variable is updated to the character following the lexed
// string upon successful return.
//
boolean IsRestrictedIdentifier(const char* string,
			       int&        index);

//
// Returns TRUE if the specified token is found; returns FALSE otherwise.
// The index variable is updated to the character following the lexed
// string upon successful return.
//
boolean IsToken(const char* string,
		const char* token,
		int&        index);

//
// Return TRUE if the given word is a reserved word in the DX scripting
// language.
//
boolean IsReservedScriptingWord(const char *word);


//
// Return TRUE if the string matches the dxexec's expectation for a WHERE
// parameter.  For ui windows it's X{24|12|8},$(DISPLAY),##%d.  For external windows,
// it's . And for exec windows it's .  Make the index variable point to the
// first character after the end of what we considered likable.
//
boolean IsWhere (const char* , int& );

#endif /* _lex_h */
