/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _ParseMDF_h
#define _ParseMDF_h



class Dictionary;
class ParameterDefinition;

extern boolean ReadMDFFiles(const char* root, Dictionary* mdf, boolean uiOnly);
extern boolean LoadMDFFile(const char* file, const char* type, 
			Dictionary *mdf, boolean uiOnly);
extern boolean ParseMDFTypes(ParameterDefinition *param,
    char *line,
    int lineNumber = 0);
extern boolean ParseMDFOptions (ParameterDefinition* pd, char* p);

// 
// A dictionary of strings that are filenames of dynamically loadable object 
// files containing module entry points for the executive to use.  The string
// keys in the dictionary are actually the names of the loadable files.
//
extern Dictionary *theDynamicPackageDictionary;

#endif // _ParseMDF_h
