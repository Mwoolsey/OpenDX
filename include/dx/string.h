/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/


#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#ifndef _DXI_STRING_H_
#define _DXI_STRING_H_

/* TeX starts here.  Do not remove this comment. */

String DXNewString(char *s);
/**
\index{DXNewString}
Creates a new string object and initializes it with a copy of the specified
null-terminated string.  Returns the object or null to indicate an error.
**/

char *DXGetString(String s);
/**
\index{DXGetString}
Returns a pointer to the string value of an object, or null to indicate
an error.
**/

String DXMakeString(char *s);

#endif /* _DXI_STRING_H_ */

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
