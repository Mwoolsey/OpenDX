/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include <defines.h>

#include "history.h"
#include "helplist.h"
#include "helpstack.h"

#define SPOTFILE "spots"
#define README_PREFIX "README"
#define README_PLATFORM "README_PLATFORM"
#define README_GENERIC "README_GENERIC"

typedef struct {
   Widget toplevel;
   Widget goback;
   Widget menushell;
   Widget hbar;
   Widget vbar;
   Widget swin;
   Stack  *fontstack;
   Stack  *colorstack;
   HistoryList  *historylist;
   int    linkType;
   char   filename[MAXPATHLEN];
   char   label[MAXPATHLEN];
   Bool   getposition;
   SpotList   *spotlist;
   Bool   mapped;

} UserData;


typedef struct {
 char *String;
 char *FileName;
 char *Font;
 char *Color;

} HelpIndex;


HelpIndex *MakeIndexEntry (char *File, char *str);

char *BuildHelpIndex(Widget RefWidget, HelpIndex **IndexTable, int Entries);

extern const char *GetHelpDirectory();
