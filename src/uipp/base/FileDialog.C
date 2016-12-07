/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifdef OS2
#define INCL_DOS
#include <os2.h>
#endif


#include <Xm/Xm.h>
#include <Xm/FileSB.h>
#include <Xm/DialogS.h>
#include <Xm/Text.h>
#include <Xm/SelectioB.h>
#include <Xm/Protocols.h>
#include <Xm/AtomMgr.h>

#include "FileDialog.h"
#include "Application.h"
#include "DXStrings.h"
#include "ErrorDialogManager.h"

#ifdef	DXD_WIN   /*   ajay    */
#include <direct.h>
#include <io.h>
#include "DXStrings.h"
#endif

#if 0             //SMH define local X routine
extern "C" { extern void ForceUpdate(Widget); }
#endif

#ifdef DXD_NON_UNIX_DIR_SEPARATOR
extern "C" {
char *KillRelPaths(char *p);
void NonUnixQualifySearchProc(Widget w, XtPointer *in_data, XtPointer *out_data);
void NonUnixSearchProc(Widget w, XtPointer search_data);
void NonUnixDirSearchProc(Widget w, XtPointer search_data);
const char *GetNullStr(XmString str);
}
#endif

//
// These are never installed by this class (on instances of this class)
// as this is an abstract class.  The subclasses are responsible for
// installing these resources, using setDefaultResources().
//
String FileDialog::DefaultResources[] =
{
        ".dialogStyle:     XmDIALOG_FULL_APPLICATION_MODAL",
	"*pathMode:	   XmPATH_MODE_FULL",
        NULL
};

boolean FileDialog::ClassInitialized = FALSE;

FileDialog::FileDialog(const char *name, Widget parent) :
                       Dialog(name, parent)
{
    this->hasCommentButton = FALSE;
    this->readOnlyDirectory = NULL;
}

FileDialog::~FileDialog()
{
    if (this->readOnlyDirectory)
	delete this->readOnlyDirectory;
}
    
void FileDialog::installDefaultResources(Widget  baseWidget)
{
    this->setDefaultResources(baseWidget, FileDialog::DefaultResources);
    this->Dialog::installDefaultResources( baseWidget);
}
void FileDialog::post()
{
    Widget text;

    theApplication->setBusyCursor(TRUE);
    if (this->getRootWidget() == NULL)
    {
	this->initialize();
	this->setRootWidget(this->createDialog(this->parent));

	Widget shell = XtParent(this->getRootWidget());
	XtVaSetValues(shell, XmNdeleteResponse, XmDO_NOTHING, NULL);
	Atom WM_DELETE_WINDOW = XmInternAtom(XtDisplay(shell),
					    "WM_DELETE_WINDOW", False);
	XmAddWMProtocolCallback(shell, WM_DELETE_WINDOW,
				    (XtCallbackProc)Dialog_CancelCB,
				    (void *)(this));

	XtAddCallback(this->fsb,
		      XmNokCallback,
		      (XtCallbackProc)Dialog_OkCB,
		      (XtPointer)this);

	XtAddCallback(this->fsb,
		      XmNcancelCallback,
		      (XtCallbackProc)Dialog_CancelCB,
		      (XtPointer)this);

	Widget helpButton =
	    XmFileSelectionBoxGetChild(this->fsb, XmDIALOG_HELP_BUTTON);
	XtRemoveAllCallbacks(helpButton, XmNactivateCallback);
	XtAddCallback(helpButton,
		      XmNactivateCallback,
		      (XtCallbackProc)Dialog_HelpCB,
		      (XtPointer)this);


	text  = XmFileSelectionBoxGetChild(this->fsb, XmDIALOG_TEXT);
    	this->manage();
	XmProcessTraversal(text, XmTRAVERSE_CURRENT);
	XmProcessTraversal(text, XmTRAVERSE_CURRENT);

    }
    this->manage();
    theApplication->setBusyCursor(FALSE);
}

//
// Set the name of the current file.
//
void FileDialog::setFileName(const char *file)
{
    ASSERT(this->fsb);
    XmString xmdir = NULL;
    char *path = NULL, *dir = NULL;

#ifdef DXD_NON_UNIX_DIR_SEPARATOR
    if (!(file[0] == '\\' || file[0] == '/' || (STRLEN(file) > 1 && file[1] == ':'))) {
    /* if (!strrchr(file, '\\') && !strrchr(file, ':') && !strrchr(file, '/')) { */
#else
    if (file[0] != '/') {
#endif
	XtVaGetValues(this->fsb,XmNdirectory, &xmdir, NULL);
	XmStringGetLtoR(xmdir,XmSTRING_DEFAULT_CHARSET, &dir); 

	path = new char [STRLEN(dir) + 2 + STRLEN(file)];
 
	sprintf(path,"%s%s",dir,file);
	file = path;
    } 

    Widget dialog_text  = XmSelectionBoxGetChild(this->fsb, XmDIALOG_TEXT); 


    //	DOS2UNIXPATH((char *) file);
    XmTextSetString(dialog_text, (char*)file);

    //
    // Right justify the text in the text window.
    //
    int len = STRLEN(file);
    XmTextShowPosition(dialog_text,len);
    XmTextSetInsertionPosition(dialog_text,len);

    

    if (path) delete path;
    if (dir)  XtFree(dir);
    if (xmdir) XmStringFree(xmdir);
    
}
//
// Get the name of the currently selected file.
// The returned string must be freed by the caller.
// If a filename is not currently selected, then return NULL.
//
char *FileDialog::getSelectedFileName()
{
    ASSERT(this->fsb);
    char *p, *s = XmTextGetString(XmSelectionBoxGetChild(this->fsb, 
					XmDIALOG_TEXT));
#ifdef DXD_NON_UNIX_DIR_SEPARATOR
    char *q = strrchr(s,'\\');
    p = strrchr(s,'/');
    if (q && q>p)
	p = q;
#else
    p = strrchr(s,'/');
#endif
    if (p) {
	p++;
	if (!*p) {
	    XtFree(s);
	    s = NULL;
	}
    }
    return s;
}

//
// Get the name of the current directory (without a trailing '/').
// The returned string must be freed by the caller.
//
char *FileDialog::getCurrentDirectory()
{
    ASSERT(this->fsb);
    char *dir = NULL;
    XmString xmdir;

    XtVaGetValues(this->fsb,XmNdirectory, &xmdir, NULL);
    if (xmdir) {
	XmStringGetLtoR(xmdir,XmSTRING_DEFAULT_CHARSET, &dir);
	char* cp = DuplicateString(dir);
	XtFree(dir);
	dir = cp;
	XmStringFree(xmdir);
	int last = STRLEN(dir) - 1;
#ifdef DXD_NON_UNIX_DIR_SEPARATOR
	if ((last >= 0) && ((dir[last] == '\\') || (dir[last] == '/')))
#else
	if ((last >= 0) && (dir[last] == '/'))
#endif
	    dir[last] = '\0';
    } else {
	dir = DuplicateString(".");
    }
    
    //	DOS2UNIXPATH(dir);
    return dir;
}

boolean FileDialog::okCallback(Dialog *d)
{
    char *string;

    string = this->getSelectedFileName(); 

    if (!string) {
	ModalErrorMessage(this->getRootWidget(),"A filename must be given");
	return FALSE;
    }

    this->unmanage();
    theApplication->setBusyCursor(TRUE);
    this->okFileWork(string);
    theApplication->setBusyCursor(FALSE);
    XtFree(string);
    return FALSE;	// Already unmanaged above
}

Widget FileDialog::createDialog(Widget parent)
{
    Widget shell;
    Arg    wargs[10];
    int    n;
    char shellname[512];

    n = 0;
    XtSetArg(wargs[n], XmNallowShellResize, True); n++;
    sprintf(shellname,"%sShell",this->name);
    shell = XmCreateDialogShell(parent, shellname, wargs, n);

    //
    // Motif FileSelection dialogs don't tolerate being sized to tiny
    // dimensions.  They're not able to lay themselves out following
    // such a resize.  So impose min sizes.  It might be better to 
    // make a virtual method in Dialog.
    //
    XtVaSetValues (shell, XmNminWidth, 130, XmNminHeight, 320, NULL);

    this->fsb =  this->createFileSelectionBox(shell, this->name);

    return this->fsb;
}

//
// Create the file selection box (i.e. the part with the filter, directory
// and file list boxes, and the 4 buttons, ok, comment, cancel, apply).
// The four buttons are set the have a width of 120
//

Widget FileDialog::createFileSelectionBox(Widget parent, const char *name)
{
    Widget button;
    Dimension width;

    Widget fsb = XtVaCreateWidget(name,
                                  xmFileSelectionBoxWidgetClass, parent,
#if 0
#ifdef DXD_NON_UNIX_DIR_SEPARATOR
                                  XmNfileSearchProc,        NonUnixSearchProc,
                                  XmNdirSearchProc,         NonUnixDirSearchProc,
                                  XmNqualifySearchDataProc, NonUnixQualifySearchProc,
#endif
#endif
                                  NULL);

    if (!this->hasCommentButton) {
        button = XmFileSelectionBoxGetChild(fsb, XmDIALOG_HELP_BUTTON);
        XtUnmanageChild(button);
    }

    //
    // Ensure the buttons are at least 120 wide
    //
    button = XmFileSelectionBoxGetChild(fsb, XmDIALOG_OK_BUTTON);
    XtVaGetValues(button, XmNwidth, &width, NULL);
    if (width < 120)
    {
	XtVaSetValues(button, XmNwidth, 120, XmNrecomputeSize, False, NULL);

	button = XmFileSelectionBoxGetChild(fsb, XmDIALOG_CANCEL_BUTTON);
	XtVaSetValues(button, XmNwidth, 120, XmNrecomputeSize, False, NULL);

	button = XmFileSelectionBoxGetChild(fsb, XmDIALOG_APPLY_BUTTON);
	XtVaSetValues(button, XmNwidth, 120, XmNrecomputeSize, False, NULL);

	button = XmFileSelectionBoxGetChild(fsb, XmDIALOG_HELP_BUTTON);
	XtVaSetValues(button, XmNwidth, 120, XmNrecomputeSize, False, NULL);
    }
    
    return fsb;
}

void FileDialog::manage()
{
    XmFileSelectionDoSearch(this->getFileSelectionBox(), NULL);
#if 0             // SMH hack the loop away
    ForceUpdate(this->fsb);
#endif
    this->displayLimitedMode(this->readOnlyDirectory != NULL);
    this->Dialog::manage();
    char *file = this->getDefaultFileName();
    if (file) {
	this->setFileName(file);
	delete file;
    }


}

//
// If dirname is not NULL, then set the dialog so that the directory is
// fixed.  If dirname is NULL, then the fixed directory is turned off.
//
void FileDialog::setReadOnlyDirectory(const char *dirname)
{
    if (this->readOnlyDirectory)
	delete this->readOnlyDirectory;
    
    if (dirname) {
	this->readOnlyDirectory = DuplicateString(dirname);	
    } else {
	this->readOnlyDirectory = NULL; 
    }


    if (this->isManaged())  {
	XmString string = XmStringCreate((char*)dirname,
						XmSTRING_DEFAULT_CHARSET);
	XtVaSetValues(this->fsb, XmNdirectory,string, NULL);
	XmStringFree(string);
	this->displayLimitedMode(TRUE);
    }
	
}

//
// If 'limit' is true, change the dialog box so that it operates in 
// limited mode, that is, the user can only select file from the 
// current directory specified by XmNdirectory. 
//
void FileDialog::displayLimitedMode(boolean limit)
{
    Widget w[10]; 
    int	n = 0;

    ASSERT(this->fsb);
    w[n++] = XmFileSelectionBoxGetChild(this->fsb,XmDIALOG_APPLY_BUTTON);
    w[n++] = XmFileSelectionBoxGetChild(this->fsb,XmDIALOG_FILTER_LABEL); 
    w[n++] = XmFileSelectionBoxGetChild(this->fsb,XmDIALOG_FILTER_TEXT); 
    w[n++] = XmFileSelectionBoxGetChild(this->fsb,XmDIALOG_DIR_LIST_LABEL); 
    w[n++] = XmFileSelectionBoxGetChild(this->fsb,XmDIALOG_DIR_LIST); 
    w[n] = XtParent(w[n-1]); n++;
    w[n++] = XmFileSelectionBoxGetChild(this->fsb,XmDIALOG_TEXT); 
    w[n++] = XmFileSelectionBoxGetChild(this->fsb,XmDIALOG_SELECTION_LABEL); 
    w[n++] = XmFileSelectionBoxGetChild(this->fsb,XmDIALOG_LIST_LABEL); 

    if (limit) {
	while (n--) 
	    XtUnmanageChild(w[n]);	
    } else {
	while (n--) 
	    XtManageChild(w[n]);	
    }
}

// 
// Get the default file name that is to be placed in the filename text
// each time this is FileDialog::manage()'d.  By default this returns
// NULL and nothing happens.  Subclasses can implement this to acquire
// the described behavior. 
//
char *FileDialog::getDefaultFileName()
{
    return NULL;
}


#ifdef DXD_NON_UNIX_DIR_SEPARATOR

extern "C" char *KillRelPaths(char *p)
{
  char *s,*p1;

  if (p)
    {
    while (!strncmp(p,"/../",4))
      memmove(p, &p[3], strlen(&p[3])+1);
    while ((p1=s=strstr(p,"/../")) && s!=p)
      {
        s--;
        while (s!=p && *s != '/')
          s--;
        if (*s == '/')
          memmove(s, &p1[3], strlen(&p1[3])+1);
      }
    while (s=strstr(p,"/./"))
      memmove(s, &s[2], strlen(&s[2])+1);
   }
  return p;
}

extern "C" void NonUnixQualifySearchProc(Widget w, XtPointer *in_data, XtPointer *out_data)
{
  Widget u;
  char *s1,*s2,*s3,*s4;
  char dir[_MAX_PATH];
  char mask[_MAX_PATH];
  char value[_MAX_PATH];
  char buff[_MAX_PATH];
  char filter[_MAX_PATH];
  ULONG PathLength;
  char *ptr;
  XmString	ext;
  int i;


#ifndef	DXD_WIN
  APIRET rc;
#endif

  XmFileSelectionBoxCallbackStruct *cbs =
        (XmFileSelectionBoxCallbackStruct *) in_data;
  XmFileSelectionBoxCallbackStruct *cbs2 =
        (XmFileSelectionBoxCallbackStruct *) out_data;

  u = XmFileSelectionBoxGetChild(w, XmDIALOG_FILTER_TEXT);
  s1 = XmTextGetString(u);

  if (s1 && *s1)
    i = ((*s1 == '/') || (*s1 == '\\') || (strlen(s1)>1 && s1[1] == ':'));
  else
    i = 0;

  if (!i)
    {
	PathLength=sizeof(mask);
#ifdef  os2 //  ajay
	rc = DosQueryCurrentDir(0, mask, &PathLength);
#endif
#ifdef  DXD_WIN
	_getdcwd(0, mask, PathLength - 1);
#endif
	if (mask[strlen(mask)-1] != '\\')
	    strcat(mask, "\\");
	strcpy(filter, GetNullStr(cbs->mask));
	strcat(mask, filter);
    }
   else 
	strcpy(mask, s1);

  XtFree(s1);
  DOS2UNIXPATH(mask);
  if (*mask != '/' && !strchr(mask,':'))   /* put leading /       */
    {
      PathLength=sizeof(buff);
      strcat(buff,mask);
      strcpy(mask,buff);
    }
  strcpy(dir,mask);                        /* get dir from mask    */

  ptr=strrchr(dir,'/');
  if(!ptr) ptr = strrchr(dir, ':');

  if (ptr)
    {
      strcpy(filter,&ptr[1]);              /* get filter from filter text */
      ptr[1]='\0';
    }
   else
    {
      strcpy(filter,"*");
      strcat(dir,"/");
    }

  if (!cbs->event && cbs->dir)       /* clicked on dir selection */
    {
      XmStringGetLtoR(cbs->dir, XmFONTLIST_DEFAULT_TAG, &s1);
      strcpy(mask,s1);
      XtFree(s1);
      DOS2UNIXPATH(mask);
      strcat(mask,"/");
      if (*mask != '/' && !strchr(mask,':'))
        {
          strcpy(buff,"/");
          strcat(buff,mask);
          strcpy(mask,buff);
        }
      strcpy(dir,mask);
      strcat(mask, filter);
    }

  u = XmFileSelectionBoxGetChild(w, XmDIALOG_TEXT);
  s2 = XmTextGetString(u);
  if (!s2 || !strlen(s2))
    strcpy(value,mask);
   else
    strcpy(value,s2);
  XtFree(s2);

  KillRelPaths(mask);
  KillRelPaths(dir);
  KillRelPaths(value);

  cbs2->reason = cbs->reason;
  cbs2->event = cbs->event;
  cbs2->value = XmStringCreateLocalized(value);
  cbs2->length = XmStringLength(cbs2->value);
  cbs2->mask = XmStringCreateLocalized(mask);
  cbs2->mask_length = XmStringLength(cbs2->mask);
  cbs2->dir = XmStringCreateLocalized(dir);
  cbs2->dir_length = XmStringLength(cbs2->dir);
  cbs2->pattern = XmStringCreateLocalized(filter);
  cbs2->pattern_length = XmStringLength(cbs2->pattern);

}

#ifdef DXD_WIN

#define MAX_DIR_LIST 256

extern "C" static int LocalFileCmp(const void *a, const void *b)
{
    return strcmp(*(char **)a, *(char **)b);
}

extern "C" static int GetSortedList(char *mask, char *dir, char **list, int max, int dirs)
{
    struct _finddata_t  DirHandle;
    long HFile;
    long rc;
    char *p;
    int k = 0;

    HFile = _findfirst(mask, &DirHandle );
    if (HFile == -1L)
	rc = -1;
    else
	rc = 0;

    while (!rc) {
	if ((dirs == 0) != ((DirHandle.attrib & _A_SUBDIR) == 0)){
	    rc = _findnext(HFile, &DirHandle);
	    continue;
	}
	list[k] = (char *)malloc(_MAX_PATH * sizeof(char));
	p = list[k++];

	strcpy(p, dir);
	strcat(p, DirHandle.name);

	DOS2UNIXPATH(p);

	rc = _findnext(HFile, &DirHandle);
    }
    if (k>1)
	qsort(list, k, sizeof(char *), LocalFileCmp);
    
    return k;
}
#endif  // DXD_WIN

extern "C" void NonUnixSearchProc(Widget w, XtPointer search_data)
{
  char          *mask;
  char          fullname[300];
  char          *dir;
  XmString      names[MAX_DIR_LIST];
  int           i=0;
#ifndef	DXD_WIN   /*   ajay   */
  HDIR          DirHandle=1;
  FILEFINDBUF3  FindBuffer;
  APIRET        rc;
#endif
#ifdef	DXD_WIN	/*   ajay   */
  long	rc;
  char *list[MAX_DIR_LIST];
  int knt;
#endif
  int         FindCount = 0;
  XmFileSelectionBoxCallbackStruct *cbs =
        (XmFileSelectionBoxCallbackStruct *) search_data;

  malloc(28);

  if (!XmStringGetLtoR(cbs->mask, XmFONTLIST_DEFAULT_TAG, &mask))
    return;
  if (!XmStringGetLtoR(cbs->dir, XmFONTLIST_DEFAULT_TAG, &dir))
    return;

  UNIX2DOSPATH(mask);

#ifdef  os2 //  ajay
  rc = DosFindFirst(mask, &DirHandle, FILE_READONLY, (PVOID) &FindBuffer,
         sizeof(FindBuffer), &FindCount, FIL_STANDARD);

  XtFree(mask);
  while(!rc && i<MAX_DIR_LIST){
    /* DosQueryPathInfo(Findbuffer.achName, FIL_QUERYFULLNAME,
      fullname, sizeof(fullname)); */
    strcpy(fullname,dir);
    strcat(fullname,FindBuffer.achName);

    DOS2UNIXPATH(fullname);
    names[i++] = XmStringCreateLocalized(fullname);
    FindCount = 1;

    rc = DosFindNext(DirHandle, &FindBuffer, sizeof(FindBuffer), &FindCount);
#endif

#ifdef  DXD_WIN //  ajay
  FindCount = GetSortedList(mask, dir, list, MAX_DIR_LIST, 0);
  XtFree(mask);
  for (i=0; i<FindCount; i++) {
    names[i] = XmStringCreateLocalized(list[i]);
    free(list[i]);
  }

#endif

  if (!FindCount)
      names[FindCount++] = XmStringCreateLocalized("       [      ] ");

  XtVaSetValues(w,
    XmNfileListItems,         names,
    XmNfileListItemCount,     FindCount,
    XmNdirSpec,               names[0],
    XmNlistUpdated,           True,
    NULL);

  while (FindCount > 0)
    XmStringFree(names[--FindCount]);

  XtFree(dir);
}

extern "C" void NonUnixDirSearchProc(Widget w, XtPointer search_data)
{
  char          dirmask[_MAX_PATH];
  char		*list[MAX_DIR_LIST];
  char          fullname[_MAX_PATH];
  char          *dir;
  XmString      names[MAX_DIR_LIST];
  int           i=0;
#ifdef  os2 
  HDIR          DirHandle=1;
  FILEFINDBUF3  FindBuffer;
  APIRET        rc;
#endif
#ifdef  DXD_WIN
    long        rc;
#endif
  int FindCount = 0; 


  XmFileSelectionBoxCallbackStruct *cbs =
        (XmFileSelectionBoxCallbackStruct *) search_data;

  if (!XmStringGetLtoR(cbs->dir, XmFONTLIST_DEFAULT_TAG, &dir))
    return;

  UNIX2DOSPATH(dir);
  strcpy(dirmask, dir);
  strcat(dirmask, "*");

#ifdef  os2
  rc = DosFindFirst(dirmask, &DirHandle, MUST_HAVE_DIRECTORY | FILE_READONLY, (PVOID) &FindBuffer,
         sizeof(FindBuffer), &FindCount, FIL_STANDARD);

  while(!rc && i<MAX_DIR_LIST){
    strcpy(fullname,dir);
    strcat(fullname,FindBuffer.achName);

    DOS2UNIXPATH(fullname);
    names[i++] = XmStringCreateLocalized(fullname);
    FindCount = 1;

    rc = DosFindNext(DirHandle, &FindBuffer, sizeof(FindBuffer), &FindCount);
#endif

#ifdef  DXD_WIN //  ajay
  FindCount = GetSortedList(dirmask, dir, list, MAX_DIR_LIST, 1);
  XtFree(dir);
  for (i=0; i<FindCount; i++) {
    names[i] = XmStringCreateLocalized(list[i]);
    free(list[i]);
  }

#endif

  if (!FindCount) {
      names[FindCount++] = XmStringCreateLocalized("/.");
      names[FindCount++] = XmStringCreateLocalized("/..");
  }

  XtVaSetValues(w,
    XmNdirListItems,         names,
    XmNdirListItemCount,     FindCount,
    XmNlistUpdated,          True,
    XmNdirectoryValid,       True,
    NULL);

  while (FindCount > 0)
    XmStringFree(names[--FindCount]);

  XtFree(dir);
}



extern "C" const char *GetNullStr(XmString str)
{
    XmStringContext cxt;
    char *text, *tag;
    static char buf[1024];
    XmStringDirection dir;
    Boolean sep;
    int os;

    if (!str) return "";
    if (!XmStringInitContext (&cxt, str)) {
	XtWarning ("Can't convert compound string.");
	return "";
    }
    os = 0;
    while (XmStringGetNextSegment (cxt, &text, &tag, &dir, &sep)) {
	strcpy (&buf[os], text);
	os+= strlen(text);
	if (sep) {
	    buf[os++] = '\n';
	}
	// unsure about this line. The motif manual doesn't do this
	// but purify complains about memory loss
	if (tag) XtFree(tag);
	XtFree(text);
    }

    buf[os] = '\0';
    ASSERT(os<sizeof(buf));
    XmStringFreeContext(cxt);
    return buf;
}

#endif




