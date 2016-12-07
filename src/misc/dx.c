/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*  DX script - C-Windows version	*/

#include "dx.h"

#if !defined(intelnt) && !defined(WIN32)
#include <stdio.h>
int main(){fprintf(stderr, "misc/dx is only needed on Windows (non-cygwin) based systems.\n"); return 1;}
#else /* Windows system compile application */

#include "utils.h"

#define USE_REGISTRY 1

#if defined(cygwin)
void
GetShortPathName(char *src, char *dst, int max)
{
    strncpy(dst, src, strlen(src)+1);
}
#endif

#define ConvertShortPathName(s1) { \
	envstr sPath = "";	\
	GetShortPathName(s1, sPath, MAXENV);	\
    	strncpy(s1, sPath, strlen(s1));			\
}

/* Global Variables */

enum xServer whichX = UNKNOWN;

int	numparms =	0;
int	exonly =	0;
int native =	0;
int	uionly =	0;
int	startup =	0;
int	seecomline =	0;
int	echo =		0;
int	showversion =	0;
int	prompter =	0;
int	tutor =		0;
int	builder =	0;
int	wizard = 	0;
int	portset =	0;
int	javaserver = 	0;
int	needShortPath = 0;

namestr		parm[MAXPARMS];
envstr		cmd =		"";
envstr		path =		"";

envstr		dxargs =	"";
smallstr	uimode =	"";
smallstr	exmode =	"";
namestr		exhost =	"";
namestr		cdto =		"";
namestr		exmem =		"";
namestr 	uimem =		"";
namestr		exprocs =	"";
namestr 	uiflags =	"";
namestr 	errmsg =	"";
namestr 	display =	"";
smallstr 	uilog =		"";
smallstr 	exlog =		"";
smallstr 	uicache =	"";
smallstr 	excache =	"";
smallstr 	uitrace =	"";
smallstr 	extrace =	"";
smallstr 	uiread =	"";
smallstr 	exread =	"";
smallstr 	uitime =	"";
smallstr 	extime =	"";
smallstr 	uihilite =	"";
smallstr 	exhilite =	"-B"; /* Default is on */
namestr 	FileName =	"";
namestr 	dxroot =	"";
namestr 	dxexroot =	"";
namestr 	dxuiroot =	"";
namestr 	exceeddir =	"";
namestr 	exceeduserdir =	"";
namestr		starnetdir =	"";
namestr		winaxedir = 	"";
namestr		xservername =	"";
namestr		xserverversion= "";
namestr		xnlspath =	"";
namestr		xapplresdir =	"";
namestr		xkeysymdb =	"";
namestr 	dxexec =	"";
namestr 	dxmdf =		"";
namestr 	exmdf =		"";
namestr 	uimdf =		"";
namestr 	dxui =		"";
envstr 		dxdata =	"";
envstr	 	dxmacros =	"";
envstr 		dxmodules =	"";
namestr 	uirestrict =	"";
namestr	 	prompterflags =	"";
namestr		dxhwmod =	"";
smallstr	port = 		"";
smallstr	host =		"";
smallstr	dx8bitcmap =	"";
namestr		dxinclude =	"";
smallstr	dxcolors =	"";
smallstr	uiarch =	"";
smallstr	exarch = 	"";
namestr		curdir =	"";
namestr		motifbind = 	"DX*officialmascot: fred";
namestr		xparms =	"";
namestr		thishost = 	"localhost";
smallstr 	uidebug =	"";

namestr		classpath = 	"";

namestr		msgstr =	"";
namestr		errstr =	"";
envstr		argstr =	"";
envstr		magickhome =    "";


/* Function Prototypes */

int regval(enum regGet get, char *name, enum regCo co, char *value, int size, int *word);
int initrun();
int getparms(int argc, char **argv);
int fillparms(char *str, int *n);
int parseparms();
void configure();
void dxjsconfig(); /* Add more env variables for JavaDX Server */
int buildcmd();
int launchit();
int launchjs(); /* Launch the JavaDX Server */
int shorthelp();
int longhelp();

int main(int argc, char **argv)
{
    initrun();
    getparms(argc, argv);
    parseparms();
    if (javaserver) {
    	dxjsconfig();
    } else {
        configure();
	}
    buildcmd();
    if (!echo)
	    launchit();
    exit(0);
}

#if defined(USE_REGISTRY)

/*  The following queries the registry for various paths that,	*/
/*  among other things, allow dx and Xserver to start without 	*/
/*  either being in the path.  But they must be added to the	*/
/*  beginning of the path in order for them to run, and all	*/
/*  children must convey this down.				*/
int regval(enum regGet get, char *name, enum regCo co, char *value, int size, int *word)
{
    char key[500];
    char key2[500];
    int valtype;
    int sizegot = size;
    HKEY hkey_m;
    HKEY hkey_u;
    long rc, rc_m, rc_u;
    int i;
    DWORD options;
    DWORD dwDisp;
    REGSAM access = KEY_READ;
    const char **regpath;
    const char *dxpath[] = {"SOFTWARE\\OpenDX\\DX\\CurrentVersion", NULL };
    const char *snpath[] = { "SOFTWARE\\Starnet\\X-Win32 LX\\7.3",
		"SOFTWARE\\Starnet\\X-Win32 LX\\7.2",
		"SOFTWARE\\Starnet\\X-Win32 LX\\7.1",
		"SOFTWARE\\Starnet\\X-Win32 LX\\7.0",
		"SOFTWARE\\Starnet\\X-Win32 LX\\6.1",
		"SOFTWARE\\Starnet\\X-Win32 LX\\6.0",
		"SOFTWARE\\Starnet\\X-Win32\\7.1",
		"SOFTWARE\\Starnet\\X-Win32\\7.0",
		"SOFTWARE\\Starnet\\X-Win32\\6.2",
		"SOFTWARE\\Starnet\\X-Win32\\6.1",
		"SOFTWARE\\Starnet\\X-Win32\\6.0",
		"SOFTWARE\\Starnet\\X-Win32\\5.5", 
		"SOFTWARE\\Starnet\\X-Win32\\5.4",
		"SOFTWARE\\Starnet\\X-Win32\\5.3",
		"SOFTWARE\\Starnet\\X-Win32\\5.2",
		"SOFTWARE\\Starnet\\X-Win32\\5.1", NULL };
    const char *wapath[] = { "SOFTWARE\\LabF.com\\WinaXe\\7.4",
                             "SOFTWARE\\LabF.com\\WinaXe\\7.3",
                             "SOFTWARE\\LabF.com\\WinaXe\\7.2",
                             "SOFTWARE\\LabF.com\\WinaXe\\7.1",
                             "SOFTWARE\\LabF.com\\WinaXe\\7.0",
                             "SOFTWARE\\LabF.com\\WinaXe\\6.9",
                             "SOFTWARE\\LabF.com\\WinaXe\\6.8",
                             "SOFTWARE\\LabF.com\\WinaXe\\6.7",
                             "SOFTWARE\\LabF.com\\WinaXe\\6.6",
                             "SOFTWARE\\LabF.com\\WinaXe\\6.5",
			     "SOFTWARE\\LabF.com\\WinaXe\\6.4",
			     "SOFTWARE\\LabF.com\\WinaXe\\6.3",
			     "SOFTWARE\\LabF.com\\WinaXe\\6.2",
			     "SOFTWARE\\LabF.com\\WinaXe\\6.1", NULL };
    const char *hbpath[] = { "SOFTWARE\\Hummingbird\\Connectivity\\12.00\\Exceed",
		"SOFTWARE\\Hummingbird\\Connectivity\\11.10\\Exceed",
		"SOFTWARE\\Hummingbird\\Connectivity\\11.00\\Exceed",
		"SOFTWARE\\Hummingbird\\Connectivity\\10.10\\Exceed",
		"SOFTWARE\\Hummingbird\\Connectivity\\10.00\\Exceed",
		"SOFTWARE\\Hummingbird\\Connectivity\\9.10\\Exceed",
		"SOFTWARE\\Hummingbird\\Connectivity\\9.00\\Exceed",
		"SOFTWARE\\Hummingbird\\Connectivity\\8.10\\Exceed",
		"SOFTWARE\\Hummingbird\\Connectivity\\8.00\\Exceed",
		"SOFTWARE\\Hummingbird\\Connectivity\\7.10\\Exceed",
		"SOFTWARE\\Hummingbird\\Connectivity\\7.00\\Exceed", NULL };
    const char *h6path[] = { "SOFTWARE\\Hummingbird\\Exceed\\CurrentVersion", NULL };

	/* First determine which system we're looking up. */

    if(co == OPENDX_ID)
	regpath = dxpath;
    else if (co == STARNET_ID) 
	regpath = snpath;
    else if (co == LABF_ID)
	regpath = wapath;
    else if (co == HUMMBIRD_ID)
	regpath = hbpath;
    else /* Old Exceed 6 */
	regpath = h6path;
        

	/* What a pain--some systems store some info in HKEY_LOCAL_MACHINE */
	/* whereas others store it in HKEY_CURRENT_USER. It also depends   */
	/* on the type of info and how it was installed. Thus we'll have   */
	/* to always look at both to make sure we do this right and then   */
	/* return the result if in either. */

    i = 0;
    while(regpath[i]) {
#ifdef DEBUGREG
	printf("Checking: %s\n", regpath[i]);
#endif
	rc_m = RegOpenKeyEx(HKEY_LOCAL_MACHINE, __TEXT(regpath[i]), 0, access, &hkey_m);
	rc_u = RegOpenKeyEx(HKEY_CURRENT_USER, __TEXT(regpath[i]), 0, access, &hkey_u);

	if((rc_m == ERROR_SUCCESS || rc_u == ERROR_SUCCESS) && get == CHECK) {
	    RegCloseKey(hkey_m);
	    RegCloseKey(hkey_u);
	    return 1;
	}
	else if (rc_m == ERROR_SUCCESS || rc_u == ERROR_SUCCESS) {
	    break;
	}
	i++;
    }
    if(rc_m != ERROR_SUCCESS && rc_u != ERROR_SUCCESS) return 0;
    
    /* hkey now pointing in the proper reg entry area */

    if (get == GET) {
	rc_u =  RegQueryValueEx(hkey_u, __TEXT(name), (LPDWORD) 0,
		(LPDWORD) &valtype, (LPBYTE) value, &sizegot);

	if(rc_u != ERROR_SUCCESS)
	    rc_m = RegQueryValueEx(hkey_m, __TEXT(name), (LPDWORD) 0, 
		(LPDWORD) &valtype, (LPBYTE) value, &sizegot);

	if( rc_u != ERROR_SUCCESS && rc_m != ERROR_SUCCESS) {
	    rc = rc_u;
	    sprintf(errstr, "%s %s %s", 
			"Query value failed on registry value", name, "");
	    goto error;
	}

	RegCloseKey(hkey_u);
	RegCloseKey(hkey_m);

	/* Now check to see if it is a DWORD entry if so, pass it back through word
		not as a string through name. */
	switch(valtype) {
	    case REG_DWORD:
		*word = *((int *)value); 
		strcpy(value, "");
		break;
	    case REG_SZ:
		break;
	    default:
		return 0;
	}
	return 1;
    }

error:
	printf("%s: rc = %d\n", errstr, rc);
	return 0;
}
#endif


int initrun()
{
#if defined(intelnt) || defined(WIN32)
    OSVERSIONINFO osvi;
#endif
#if defined(USE_REGISTRY)
    int keydata;

    namestr	xenvvar = "";
    namestr 	dxrootreg =	"";
    namestr	dxdatareg =	"";
    namestr	dxmacroreg = 	"";
    namestr	magickhomereg = "";

    if(!(regval(CHECK, "Default", OPENDX_ID, dxrootreg, sizeof(dxrootreg), &keydata) &&
       (regval(GET, "DXROOT", OPENDX_ID, dxrootreg, sizeof(dxrootreg), &keydata) +
    	regval(GET, "DXDATA", OPENDX_ID, dxdatareg, sizeof(dxdatareg), &keydata) +
    	regval(GET, "DXMACROS", OPENDX_ID, dxmacroreg, sizeof(dxmacroreg), &keydata) +
	regval(GET, "IMHOME", OPENDX_ID, magickhomereg, sizeof(magickhomereg), &keydata))  > 3) )
    	    	printf("This version of OpenDX does not appear to be correctly installed on this\n"
    	       "machine. Execution will be attempted anyway, and if it fails, please try\n"
    	       "reinstalling the software.\n");

	getenvstr("XSRVR", xenvvar);
	if(strlen(xenvvar)==0 || strcmp(xenvvar, "exceed")==0) {
            if(regval(CHECK, "Default", HUMMBIRD_ID, exceeddir, sizeof(exceeddir), &keydata)) {
    	        strcpy(xservername, "Exceed 7"); whichX = EXCEED7;
                if(!(regval(GET, "HomeDir", HUMMBIRD_ID, exceeddir, sizeof(exceeddir), &keydata) &&
		     regval(GET, "UserDir", HUMMBIRD_ID, exceeduserdir, sizeof(exceeduserdir), &keydata))) {
			printf("If Exceed is installed on this machine, please make sure it is available\n"
	       		"to you as a user.  Otherwise, make sure another X server is installed and running.\n");
	       		whichX = UNKNOWN;
	       	}    
            } 
	    if(regval(CHECK, "Default", HUMMBIRD_ID2, exceeddir, sizeof(exceeddir), &keydata)) {
    	        strcpy(xservername, "Exceed 6"); whichX = EXCEED6;
                if(!(regval(GET, "PathName", HUMMBIRD_ID2, exceeddir, sizeof(exceeddir), &keydata) &&
		     regval(GET, "UserDir", HUMMBIRD_ID2, exceeduserdir, sizeof(exceeduserdir), &keydata))) {
			printf("If Exceed is installed on this machine, please make sure it is available\n"
	       		"to you as a user.  Otherwise, make sure another X server is installed and running.\n");
	       		whichX = UNKNOWN;
	       	}    
            } 

	}
	if((strlen(xenvvar)==0 || strcmp(xenvvar, "xwin32")==0) && whichX == UNKNOWN) {
	    if (regval(CHECK, "Default", STARNET_ID, starnetdir, sizeof(starnetdir), &keydata)) {
		strcpy(xservername, "X-Win32"); whichX = XWIN32;
		if(!regval(GET, "Pathname", STARNET_ID, starnetdir, sizeof(starnetdir), &keydata)) {
		    printf("If X-Win32 is installed on this machine, please make sure it is available\n"
			"to you as a user.  Otherwise, make sure another X server is installed and running.\n");
		    whichX = UNKNOWN;
		}
	    }
	}
	if((strlen(xenvvar)==0 || strcmp(xenvvar, "winaxe")==0) && whichX == UNKNOWN) {
	    if(regval(CHECK, "Default", LABF_ID, winaxedir, sizeof(winaxedir), &keydata)) {
		strcpy(xservername, "WinaXe"); whichX = WINAXE;
		if(!regval(GET, "App Path", LABF_ID, winaxedir, sizeof(winaxedir), &keydata))
		    printf("If WinaXe is installed on this machine, please make sure it is available\n"
		    "to you as a user.  Otherwise, make sure another X server is installed and running.\n");
	    }
	}
#endif /*(USE_REGISTRY)*/

#if defined(intelnt) || defined(WIN32)
    osvi.dwOSVersionInfoSize = sizeof(osvi);
    GetVersionEx(&osvi);
    
    if(osvi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
    	needShortPath = 1;
    else
#endif
        needShortPath = 0;

    strcpy(exhost, thishost);
    strcpy(exarch, DXD_ARCHNAME);
    strcpy(uiarch, DXD_ARCHNAME);
    putenvstr("ARCH", DXD_ARCHNAME, echo);
    startup = 1;
    
    /* Currently everything is correct, except that on NT/2000 we need to
       not convert to short pathnames. Only do this on Windows ME and below.
       Thus we need to query the system get the revision number and then
       make the decision based on this fact. */
    
    getenvstr("DXARGS", dxargs);
    getcwd(curdir, sizeof(curdir));
    getenvstr("DXROOT", dxroot);
    getenvstr("DXDATA", dxdata);
    getenvstr("DXMACROS", dxmacros);
    getenvstr("DXMODULES", dxmodules);
    getenvstr("DXMDF", dxmdf);
    getenvstr("DXINCLUDE", dxinclude);
    getenvstr("DXCOLORS", dxcolors);
    getenvstr("DXEXEC", dxexec);
    getenvstr("DXUI", dxui);
    getenvstr("DISPLAY", display);
    getenvstr("MAGICK_HOME", magickhome);

#if defined(USE_REGISTRY)
    /* If env vars not set then try using the Registry vars */
    if (!*dxroot)
	strcpy(dxroot, dxrootreg);
    if(!*dxdata)
    	strcpy(dxdata, dxdatareg);
    if(!*dxmacros)
    	strcpy(dxmacros, dxmacroreg);
    if(!*magickhome)
	strcpy(magickhome, magickhomereg);
#endif

    /* If all else fails, set some defaults for OpenSource Unix layout */
    if (!*dxroot) {
#if defined(cygwin)
		strcpy(dxroot, "/usr/local/dx/");
#else
    	strcpy(dxroot, "\\usr\\local\\dx\\");
#endif
    }

    /* Now strip off any garbage that may have been set on dxroot */
    removeQuotes(dxroot);
    u2d(dxroot);
    if(needShortPath)
    	ConvertShortPathName(dxroot);

#if defined(cygwin)
    if (dxdata && *dxdata)
	strcat(dxdata, ":");
    strcat(dxdata, dxroot);
    if(dxroot[strlen(dxroot)-1] !='/')
	strcat(dxdata, "/");
    strcat(dxdata,"samples/data");
#else
    if (dxdata && *dxdata)
	strcat(dxdata, ";");
    strcat(dxdata, dxroot);
    if(dxroot[strlen(dxroot)-1] !='\\')
	strcat(dxdata, "\\");
    strcat(dxdata,"samples\\data");
#endif

/* Append the default dxroot/samples/macros to current macros if it doesn't exist */

#if defined(cygwin)
    if(dxmacros && *dxmacros)
	strcat(dxmacros, ":");
    strcat(dxmacros, dxroot);
    if(dxroot[strlen(dxroot)-1] !='/')
	strcat(dxmacros, "/");
    strcat(dxmacros,"samples/macros");
#else
   {
        namestr buf;
        strcpy(buf, dxroot);
        if(dxroot[strlen(dxroot)-1] !='\\')
	    strcat(buf, "\\");
        strcat(buf,"samples\\macros");
        if(strstr(dxmacros, buf) == 0) {
	    if(dxmacros && *dxmacros)
                strcat(dxmacros, ";");
            strcat(dxmacros, buf);
        }
    }
#endif

    /* fill envargs */

    return 1;
}

#define notset(s) (s && !*s)

#define setifnot(s, v)		\
    if notset(s) {		\
	strcpy(s, v);		\
    }


void configure()
{
	int result=0;
	namestr temp, xs;

	if(dxroot[strlen(dxroot)-1] == '\\') dxroot[strlen(dxroot)-1] = '\0';

	getenvstr("Path", path);

	getenvstr("XSERVER_LAUNCHED", xs);

#if defined(USE_REGISTRY)
	/* If using registry, then installer installs this file
	   appropriately.
	 */

	sprintf(xkeysymdb, "%s\\lib\\keysyms.dx", dxroot);
	if(needShortPath)
		ConvertShortPathName(xkeysymdb);
	setenvpair("XKEYSYMDB", xkeysymdb);
#endif

	if(strcmp(xs, "yes") != 0) {
		if (whichX == EXCEED6) {
			/* Set Exceed 6 env variables */
			if(needShortPath)
				ConvertShortPathName(exceeddir);
			if(exceeddir[strlen(exceeddir)-1] == '\\') 
				exceeddir[strlen(exceeddir)-1] = '\0';
			strcat(path, ";"); 
			strcat(path, exceeddir);
			setenvpair("Path", path);
			result = _spawnlp(_P_NOWAIT, "Exceed", "Exceed", NULL);
			if(result == -1)
				printf( "Error spawning Exceed: %s\n", strerror( errno ) );
			setenvpair("XSERVER_LAUNCHED", "yes");
		}

		if (whichX == XWIN32) {
			/* Need to define X-Win32 env variables */
			/* set DISPLAY to COMPUTERNAME:0 */
			/* Start XWIN32 */
			if(needShortPath)
				ConvertShortPathName(starnetdir);
			if(starnetdir[strlen(starnetdir)-1] == '\\') 
				starnetdir[strlen(starnetdir)-1] = '\0';
			strcat(path,";"); 
			strcat(path, starnetdir);
			setenvpair("Path", path);
			result = _spawnlp(_P_NOWAIT, "xwin32", "xwin32", NULL);
			if(result == -1)
				printf( "Error spawning xwin32: %s\n", strerror( errno ) );

			setenvpair("XSERVER_LAUNCHED", "yes");
		}
		if (whichX == WINAXE) {
			if(needShortPath)
				ConvertShortPathName(winaxedir);
			if(winaxedir[strlen(winaxedir)-1] == '\\') 
				winaxedir[strlen(winaxedir)-1] = '\0';
			strcat(path,";"); 
			strcat(path, winaxedir);
			setenvpair("Path", path);
			result = _spawnlp(_P_NOWAIT, "xserver", "xserver", NULL);
			if(result == -1)
				printf( "Error spawning winaxe: %s\n", strerror( errno ) );

			setenvpair("XSERVER_LAUNCHED", "yes");
		}	
	}

	if (whichX == EXCEED7) {
		/* Need to define Exceed Env Variables */
		if(needShortPath)
			ConvertShortPathName(exceeddir);
		if(exceeddir[strlen(exceeddir)-1] == '\\') 
			exceeddir[strlen(exceeddir)-1] = '\0';
		strcat(path, ";"); 
		strcat(path, exceeddir);
	}

	sprintf(temp, "%s\\bin_%s", dxroot, DXD_ARCHNAME);
	if(needShortPath)
		ConvertShortPathName(temp);
	strcat(path, ";"); 
	strcat(path, temp);

	u2d(magickhome);
	if(needShortPath)
		ConvertShortPathName(magickhome);
	strcat(path, ";"); 
	strcat(path, magickhome);

	setenvpair("Path", path);

	/*  The following logic is specific to the PC, where the	*/
	/*  the sample data and macro paths are automatically set	*/
	/*  if neither is specified.  This allows the samples not	*/
	/*  to have hardcoded paths in the pc environment (they	*/
	/*  are sed'd out).  Unix needs different logic.		*/


	setenvpair("DXDATA", dxdata);
	setenvpair("DXMACROS", dxmacros);
	setenvpair("DXMODULES", dxmodules);
	setenvpair("DXINCLUDE", dxinclude);
	setenvpair("DXMDF", dxmdf);
	setenvpair("DXCOLORS", dxcolors);
	setenvpair("DX8BITCMAP", dx8bitcmap);
	setenvpair("MAGICK_HOME", magickhome);


	if (!*display || !strcasecmp(display, "localpc:0"))
	{
		if(whichX == EXCEED7)
			strcpy(display, "localpc:0");
		else
			strcpy(display, "localhost:0");
	}
	setenvpair("DISPLAY", display);
	setenvpair("DXNO_BACKING_STORE", "1");
	setenvpair("DXFLING", "1");
	//Solve problem with queuing in DXLink within Windows.
		setenvpair("DX_STALL", "1");

	/* Due to the way XrmParseCommand works, it cannot accept
           spaces when launching other processes from dxui, etc. 
	   Therefore, must store root in the short version without
           spaces. */
	if(strstr(dxroot, " "))
		ConvertShortPathName(dxroot);
	
	setenvpair("DXROOT", dxroot);
	if (strcasecmp(dxroot, dxexroot))
		setenvpair("DXEXECROOT", dxexroot);
	if (strcasecmp(dxroot, dxuiroot))
		setenvpair("DXUIROOT", dxuiroot);
	if (strcmp(exhost, thishost))
		setenvpair("DXHOST", exhost);
}


void dxjsconfig() {
    namestr jdxsrvPath, temp;

    /* Don't need X Server for Java Explorer server */
    setenvpair("XSERVER_LAUNCHED", "yes");

    if(dxroot[strlen(dxroot)-1] == '\\') dxroot[strlen(dxroot)-1] = '\0';
    sprintf(jdxsrvPath, "%s\\java\\server", dxroot);
    ConvertShortPathName(jdxsrvPath);

    sprintf(classpath, "%s\\class", jdxsrvPath);
    ConvertShortPathName(classpath);

    getenvstr("Path", path);
    
    /* Add dxroot\bin */
    sprintf(temp, "%s\\bin", dxroot);
    ConvertShortPathName(temp);
    strcat(path, ";"); strcat(path, temp);

    /* Add dxroot\bin_arch */
    sprintf(temp, "%s\\bin_%s", dxroot, DXD_ARCHNAME);
    ConvertShortPathName(temp);
    strcat(path, ";"); strcat(path, temp);

    /* Add dxroot\lib_arch */
    sprintf(temp, "%s\\lib_%s", dxroot, DXD_ARCHNAME);
    ConvertShortPathName(temp);
    strcat(path, ";"); strcat(path, temp);

    /* Add jdxsrvPath\lib_arch */
    sprintf(temp, "%s\\lib_%s", jdxsrvPath, DXD_ARCHNAME);
    ConvertShortPathName(temp);
    strcat(path, ";"); strcat(path, temp);

    u2d(magickhome);
    ConvertShortPathName(magickhome);
    strcat(path, ";"); strcat(path, magickhome);

    setenvpair("Path", path);

    /*  The following logic is specific to the PC, where the	*/
    /*  the sample data and macro paths are automatically set	*/
    /*  if neither is specified.  This allows the samples not	*/
    /*  to have hardcoded paths in the pc environment (they	*/
    /*  are sed'd out).  Unix needs different logic.		*/

    sprintf(temp, "%s\\dxmacros", jdxsrvPath);
    ConvertShortPathName(temp);
    if(*dxmacros)
    	strcat(dxmacros, ";"); strcat(dxmacros, temp);
    sprintf(temp, "%s\\usermacros", jdxsrvPath);
    ConvertShortPathName(temp);
    strcat(dxmacros, ";"); strcat(dxmacros, temp);
    setenvpair("DXMACROS",	dxmacros);

    strcpy(dxinclude, dxmacros);
    setenvpair("DXINCLUDE",	dxinclude);

    sprintf(temp, "%s\\userdata", jdxsrvPath);
    ConvertShortPathName(temp);
    if(*dxdata)
    	strcat(dxdata, ";"); strcat(dxdata, temp);
    setenvpair("DXDATA",	dxdata);

    setenvpair("DXMODULES",	dxmodules);
    setenvpair("DXMDF", 	dxmdf);
    setenvpair("DXCOLORS",	dxcolors);
    setenvpair("DX8BITCMAP",	dx8bitcmap);
    setenvpair("MAGICK_HOME",	magickhome);

    setenvpair("DXROOT",	dxroot);
    if (strcasecmp(dxroot, dxexroot))
	setenvpair("DXEXECROOT", dxexroot);
    if (strcasecmp(dxroot, dxuiroot))
	setenvpair("DXUIROOT", dxuiroot);
    if (strcmp(exhost, thishost))
	setenvpair("DXHOST", exhost);
	
	setenvpair("DXARGS", "-execonly -highlight off -optimize memory");
}

int buildcmd()
{
	envstr tmp;
	namestr outdir;
	namestr	dxexecdef = "";
	envstr	exflags = "";


	if(javaserver) {
		sprintf(outdir, "%s\\java\\output", dxroot);
		ConvertShortPathName(outdir);
			
		u2d(classpath);
		d2u(outdir);

		sprintf(cmd, "java -classpath %s\\server.jar -DDXServer.pathsFile=%s\\dxserver.paths -DDXServer.hostsFile=%s\\dxserver.hosts -DDXServer.outUrl=output -DDXServer.outDir=%s DXServer",
		classpath, classpath, classpath, outdir);
	}
	else
		{

		setifnot(dxexroot, dxroot);
		u2d(dxexroot);
		setifnot(dxuiroot, dxroot);

		if(native)
			sprintf(dxexecdef, "%s\\bin_%s\\dxexec-native%s", dxexroot, exarch, EXE_EXT);
		else
			sprintf(dxexecdef, "%s\\bin_%s\\dxexec%s", dxexroot, exarch, EXE_EXT);
		setifnot(dxexec, dxexecdef);
		setifnot(exmode, "-r");

		setifnot(dxuiroot, dxroot);
		if (notset(dxui))
			sprintf(dxui, "%s\\bin_%s\\dxui%s", dxuiroot, uiarch, EXE_EXT);

		setifnot(uimode, "-edit");
		setifnot(cdto, curdir);

		u2d(dxroot);
		u2d(dxexec);
		u2d(dxui);
		u2d(cdto);
		d2u(prompterflags);
		d2u(argstr);
		d2u(uimdf);
		d2u(uiflags);
		d2u(exmdf);
		if (*FileName) {
			d2u(FileName);
#if defined(intelnt) || defined(WIN32)
			addQuotes(FileName);
#endif
		}

		if (uionly && exonly)
			ErrorGoto("-uionly and -execonly are mutually exclusive");

		if (!strcmp(uimode, "-java") && !*FileName) 
			ErrorGoto("-program name required with -java flag");

		if (showversion) {
			if (*xservername) {
				printf("X server found: %s\n", xservername);
			}
			printf("Open Visualization Data Explorer, version %s (%s, %s)\n", SCRIPTVERSION, __TIME__, __DATE__);
			sprintf(cmd, "%s -v", dxexec);
			launchit();
			sprintf(cmd, "%s -version", dxui);
			launchit();
			exit(0);
		}

		if (tutor) {
			sprintf(cmd, "%s%sbin_%s%stutor%s", dxexroot, DIRSEP, uiarch, DIRSEP, EXE_EXT);
			u2d(cmd);
		}
		else if (prompter) {
			if (*FileName) {
				strcat(prompterflags, " -file ");
				strcat(prompterflags, FileName);
			}
			sprintf(cmd, "%s%sbin_%s%sprompter%s %s", dxuiroot, DIRSEP, uiarch, DIRSEP, EXE_EXT, prompterflags);
			u2d(cmd);
		}
		else if (startup) {
			sprintf(cmd, "%s%sbin_%s%sstartupui%s %s", dxuiroot, DIRSEP, uiarch, DIRSEP, EXE_EXT, argstr);
		u2d(cmd);
		}
		else if (builder) {
			sprintf(cmd, "%s%sbin_%s%sbuilder%s %s", dxuiroot, DIRSEP, uiarch, DIRSEP, EXE_EXT, FileName);
			/* sprintf(cmd, "%s%sbin_%s%sbuilder -xrm %s %s", dxroot, DIRSEP, uiarch, DIRSEP, motifbind, FileName); */
			u2d(cmd);
		}
		else if (exonly) {
			printf("Starting DX executive\n");
			sprintf(exflags, "%s %s %s %s %s %s %s %s %s %s",
			exmode, excache, exlog, exread, exmem, exprocs,
			extrace, exhilite, extime,
			exmdf);
			sprintf(cmd, "%s %s ", dxexec, exflags);
			if (*FileName) {
				u2d(FileName);
			}

		}
		else {
			printf("Starting DX user interface\n");
			sprintf(tmp, " %s %s %s %s %s %s %s %s %s %s %s %s", 
			uimode, uidebug, uimem, uilog, uicache,
			uiread, uitrace, uitime, uihilite, uimdf, xparms,
			uirestrict);
			strcat(uiflags, tmp);
			if (portset) {
				strcat(uiflags, " -port ");
				strcat(uiflags, port);
			}
			if (*FileName) {
				strcat(uiflags, " -program ");
				strcat(uiflags, FileName);
			}
			if (uionly)
				strcat(uiflags, " -uionly");
			if (wizard)
				strcat(uiflags, " -wizard");
#ifndef DXD_WIN 
			if (*cdto) {
				strcat(uiflags, " -directory ");
				strcat(uiflags, cdto);
			}
#endif
			if (strcmp(dxexec, dxexecdef)) {
				strcat(uiflags, " -exec ");
				d2u(dxexec);
				strcat(uiflags, dxexec);
			}

			sprintf(cmd, "%s %s", dxui, uiflags);
			/* sprintf(cmd, "%s %s -xrm %s", dxui, uiflags, motifbind); */

		}
	}

	if (seecomline || echo)
		printf("%s\n", cmd);


	return 1;

error:
	printf("%s\n", errstr);
	return 1;
}


int launchit()
{
#define BUFFSIZE 2048
    int rc;
    char *args[100];
    char *s, *lastSep;
    int i;
    FILE *f, *p;

    char buf[BUFFSIZE];

    u2d(cdto);
    chdir(cdto);
    if (exonly && *FileName) {
	removeQuotes(FileName);
	f = fopen(FileName, "r");
	if (!f)
	    ErrorGoto2("File", FileName, "not found");
	p = popen(cmd, "wt");
	if (!p)
	    ErrorGoto2("Error spawning exec using", cmd, "");
	while (fgets(buf, BUFFSIZE, f))
	    fputs(buf, p);
	pclose(p);
	fclose(f);
    }
    else
    { 
	lastSep = strstr(cmd, " -") -1;
	
	for (i=0, s=cmd; *s; s++) {
	    for ( ; *s && *s == ' '; s++)
		;
	    if (!*s)
		break;
	    for (args[i++]=s; *s && (*s != ' ' || s < lastSep); s++)
		;
	    if (!*s)
		break;
	    *s = '\0';
	}
	args[i] = NULL;
	
#if defined(DEBUG)
	printf("DEBUG: %d\n", DEBUG);
	for (i=0; args[i] != NULL; i++)
		printf("arg[%d]: %s\n", i, args[i]);
#endif

#if defined(HAVE_SPAWNVP)
	if (strcmp(exmode, "-r") || showversion)
	    rc = spawnvp(_P_WAIT, cmd, &args[0]);
	else
	    rc = spawnvp(_P_OVERLAY, cmd, &args[0]);
	if (rc < 0)
	{
	    rc = errno;
	    perror("error");
	}
#else
	rc = execvp(args[0], args);
#endif
    }
    return 1;
error:
    printf("%s\n", errstr);
    return 0;
}



int getparms(int argc, char **argv)
{
    int i;
    int n = 0;
    int readpipe = 0;
    char c;
    envstr pipestr = "";


    /* fill parms array, first with DXARGS values	*/
    /* then with command line options			*/
    /* then with pipe args				*/
    /* so they will have priority given to last ones in	*/

    /* -pipeargs is necessary evil since I couldn't get	*/
    /* Windows to do a non-blocking peek at stdin to	*/
    /* see if anything is there.  -pipeargs makes it	*/
    /* explicit, anyway.				*/

    fillparms(dxargs, &n);

    for (i=1; i<argc; i++) {
	strcpy(parm[n], argv[i]);
	if (!strcmp(parm[n], "-pipeargs"))
	    readpipe = 1;
	else
	{
	    strcat(argstr, parm[n]);
	    strcat(argstr, " ");
	    n++;
	}
    }

    if (readpipe) {
	for (i=0; (c=getchar()) != EOF; i++)
	    pipestr[i] = c;
	pipestr[i] = '\0';
	fillparms(pipestr, &n);
    }

    numparms = n;

    return 1;
}

/*  This fills the parm array with tokens that have their	*/
/*  leading and trailing blanks removed.  In Windows, filepaths	*/
/*  may contain spaces if they were passed in as quoted strings	*/
/*  and the shell may have removed the quotes, leaving interior	*/
/*  blanks.							*/
int fillparms(char *s, int *n)
{
    char *p, *q, *dest;

    for (p = s; *p; p++) 
	if (*p == '\t' || *p == '\n' || *p == '\f')
		*p = ' ';

    for (p = s; *p; p = q) {			/* start next */
	dest = parm[*n];
	for ( ; *p == ' '; p++)			/* skip to nonblank */
	    ;
	for (q = p; *q && *q != ' '; q++)	/* find token end */
	    ;
	if (p == q)				/* no more tokens */
	    return 1;
	strncpy(dest, p, (int)(q-p));		/* load it */
	dest[(int)(q-p)] = '\0';
	(*n)++;
    }

    return 1;
}





/*  Below is a slew of macros.  Note that some would be very	*/
/*  unhappy if a c-style semicolon were used after them, while	*/
/*  others wouldn't mind.					*/

/*  Note also that  a few automatically advance the pointer, 	*/
/*  while others do not.					*/

/*  The leading - is implicit in the conditionals, but must be	*/
/*  explicit in the startswith() query.				*/

#define advance 		\
    p++;			\
    s = parm[p];

#define is(str)			\
    if (!strcmp(s, "-" #str)) {

#define startswith(str)		\
    if (!strncmp(s, #str, strlen(#str))) {

#define isregexpr		\
    (s[strlen(s)-1] == '*')

#define isor(str)		\
    if (!strcmp(s, "-" #str) ||

#define or(str)			\
    !strcmp(s, "-" #str) ||

#define rosi(str)		\
    !strcmp(s, "-" #str)) {

#define next			\
    } else

#define islast			\
    (p >= numparms)

#define last(str)		\
    if islast {			\
	strcpy(errmsg, str);	\
	goto error;		\
    }				\
    advance;

#define lastoff(str)		\
    last("-" #str ": missing parameter, must be on or off");

#define check(str)					\
    if ((p >= numparms-1) || (*parm[p+1] == '-')) {	\
	strcpy(errmsg, str);				\
	goto error;					\
    }							\
    advance;

#define set(val)		\
    strcpy(val, s);

#define add(val)		\
    strcat(val, " ");		\
    strcat(val, s);

#define add2(val1, val2)	\
    add(val1)			\
    add(val2)

#define argadd(what, val)		\
    is(what)				\
	strcat(val, " -" #what);	\
    next

#define eq(val)			\
    if (!strcmp(s, #val)) {

#define eqon			\
    if (!strcmp(s, "on")) {

#define eqoff			\
    if (!strcmp(s, "off")) {

#define neither(str)		\
    } else {			\
	strcpy(errmsg, str);	\
	goto error;		\
    }

#define neitheroff(str)		\
    neither("-" #str ": bad parameter, must be on or off")

#define nextisswitch		\
    (*parm[p+1] == '-')

#define skipwarn(val)							\
    is(val)								\
	strcpy(msgstr, "ignoring option -" #val  " and its value, ");	\
	advance;							\
	strcat(msgstr, s);						\
	printf("%s\n", msgstr);						\
    next

#define skipwarn0(val)					\
    is(val)						\
	strcpy(msgstr, "ignoring option -" #val);	\
	printf("%s\n", msgstr); 			\
    next

#define skip0(val)					\
    is(val)						\
    next

#define skipnowarn(val)							\
    is(val)								\
	advance;							\
    next

/*  Here's the script, which amounts to a long chain of if-else's	*/
int parseparms()
{
    char *s;
    int p;
    
    for (p=0; p<numparms; ) {

	s = parm[p];

	is(whereami)
		d2u(dxroot);
		printf("installed in %s\n", dxroot);
		exit(0);
	next
	
	is(whicharch)
		printf("intelnt\n");
		exit(0);
	next
	
	is(host)
	    check("-host: missing host name");
	    set(exhost);
	next

	is(local)
	    strcpy(exhost, thishost);
	next

	skip0(queue)

	skipwarn(connect)

	is(directory)
	    check("-directory: missing directory name");
	    set(cdto);
	next

	is(memory)
	    check("-memory: missing parameter, must give number of Megabytes");
	    strcpy(exmem, " -M");
	    strcat(exmem, s);
	    strcpy(uimem, "-memory");
	    add(uimem);
	next

	is(processors)
	    check("-processors: missing parameter, must give number of processors");
	    strcpy(exprocs, " -p");
	    add(exprocs);
	next

	/*  -port is handled slightly differently from on Unix		*/
	/*  The Unix script doesn't handle it explicitly, but the ui	*/
	/*  understands it.  I couldn't find an equivalent because	*/
	/*  -uionly prevented the ui from trying to connect, while	*/
	/*  !-uionly forced the ui to spawn the server, which has been	*/
	/*  started manually.						*/
	is(port)
	    check("-port: missing port number");
	    portset = 1;
	    set(port);
	next

	isor(image) or(edit) or(java) rosi(kiosk)
	    strcpy(uimode, s);
	    exonly = 0;
	    strcpy(exmode, "-r");
	    startup = 0;
	next

	is(wizard)
	    wizard = 1;
	next

	is(menubar)
	    strcpy(uimode, "-kiosk");
	    exonly = 0;
	    strcpy(exmode, "-r");
	    startup = 0;
	next

	argadd(execute, uiflags)

	argadd(execute_on_change, uiflags)

	argadd(suppress, uiflags)

	is(xrm)
	    check("-xrm: missing resource value");
	    strcpy(xparms, "-xrm");
	    add(xparms);
	next

	argadd(synchronous, uiflags)

	is(display)
	    check("-display: missing X name");
	    set(display);
	next

	is(log)
	    lastoff(log);
	    eqon
		strcpy(exlog, "-l");
		strcpy(uilog, "-log on");
	    next eqoff
		strcpy(exlog, "");
		strcpy(uilog, "-log off");
	    neitheroff(log);
	next

	is(cache)
	    lastoff(cache);
	    eqon
		strcpy(excache, "");
		strcpy(uicache, "-cache on");
	    next eqoff
		strcpy(excache, "-c");
		strcpy(uicache, "-cache off");
	    neitheroff(cache);
	next

	is(trace)
	    lastoff(trace);
	    eqon
		strcpy(extrace, "-T");
		strcpy(uitrace, "-trace on");
	    next eqoff
		strcpy(extrace, "");
		strcpy(uitrace, "-trace off");
	    neitheroff(trace);
	next

	is(readahead)
	    lastoff(readahead);
	    eqon
		strcpy(exread, "");
		strcpy(uiread, "-readahead on");
	    next eqoff
		strcpy(exread, "-u");
		strcpy(uiread, "-readahead off");
	    neitheroff(readahead);
	next

	is(timing)
	    lastoff(timing)
	    eqon
		strcpy(extime, "-m");
		strcpy(uitime, "-timing on");
	    next eqoff
		strcpy(extime, "");
		strcpy(uitime, "-timing off");
	    neitheroff(timing);
	next


	is(highlight)
	    lastoff(highlight)
	    eqon
		strcpy(exhilite, "-B");
		strcpy(uihilite, "-highlight on");
	    next eqoff
		strcpy(exhilite, "");
		strcpy(uihilite, "-highlight off");
	    neitheroff(highlight);
	next

	skipnowarn(processors)

	is(optimize)
	    check("-optimize: missing parameter");
	    eq(memory)
		putenvstr("DXPIXELTYPE", "DXByte", echo);
		putenvstr("DXDELAYEDCOLORS", "1", echo);
	    next eq(precision)
		putenvstr("DXPIXELTYPE", "DXFloat", echo);
		putenvstr("DXDELAYEDCOLORS", "", echo);
	    next
		{
		    sprintf(errmsg, "-optimize: parameter \'%s\' not recognized", s);
		    goto error;
		}
	next

	is(script)
	    exonly = 1;
	    strcpy(exmode, "-R");
	    startup = 0;
	    if (islast)
		goto done;
	    if (!nextisswitch) {
		advance;
		set(FileName);
	    }
	next
	
	is(native)
		native = 1;
	next

	is(program)
	    check("-program: missing program name");
	    set(FileName);
	    startup = 0;
	next

	is(cfg)
	    check("-cfg: missing configuration file");
	    strcat(uiflags, "-cfg");
	    add(uiflags);
	next

	is(uionly)
	    uionly = 1;
	    startup = 0;
	next

	isor(exonly) rosi(execonly)
	    exonly = 1;
	    startup = 0;
	next

	is(dxroot)
	    check("-dxroot: missing directory name");
	    set(dxroot);
	next

	isor(execroot) rosi(exroot)
	    check("-execroot: missing directory name");
	    set(dxexroot);
	next

	is(uiroot)
	    check("-uiroot: missing directory name");
	    set(dxuiroot);
	next

	is(exec)
	    check("-exec: missing filename");
	    set(dxexec);
	next

	is(mdf)
	    check("-mdf: missing filename");
	    set(dxmdf);
	    strcat(exmdf, " -F");
	    add(exmdf);
	    strcat(uimdf, " -mdf");
	    add(uimdf);
	next

	is(dxmdf)
	    add(uimdf);
	    check("-dxmdf: missing filename");
	    add(uimdf);
	next

	is(uimdf)
	    check("-uimdf: missing filename");
	    strcat(uimdf, " -uimdf");
	    add(uimdf);
	next

	is(ui)
	    check("-ui: missing filename");
	    set(dxui);
	next

	is(data)
	    check("-data: missing directory list");
	    set(dxdata);
	next

	is(macros)
	    check("-macros: missing directory list");
	    set(dxmacros);
	next

	is(modules)
	    check("-modules: missing directory list");
	    set(dxmodules);
	next

	is(include)
	    check("-include: missing directory list");
	    set(dxinclude);
	next

	is(colors)
	    check("-colors: missing file name");
	    set(dxcolors);
	next

	is(8bitcmap)
	    set(dx8bitcmap);
	    if (!strcmp(dx8bitcmap, "private"))
		strcpy(dx8bitcmap, "-1.0");
	    else if (!strcmp(dx8bitcmap, "shared"))
		strcpy(dx8bitcmap, "1.0");
	next

	is(hwrender)
	    check("-hwrender: missing parameter, must be gl or opengl");
	    eq(opengl)
		putenvstr("DXHWMOD", "DXHwddOGL.o", echo);
	    next eq(gl)
		putenvstr("DXHWMOD", "DXHwdd.o", echo);
	    next
		{
		    sprintf(errmsg, "-hwrender: parameter \'%s\' not recognized", s);
		    goto error;
		}
	next
	
	is(jdxserver)
		javaserver = 1;
	next

	is(verbose)
	    seecomline = 1;
	next

	is(uidebug)
	    set(uidebug);
	next

	skipwarn0(outboarddebug)

	is(echo)
	    echo = 1;
	next

	skipwarn0(remoteecho)

	isor(help) or(shorthelp) rosi(h)
	    shorthelp();
	next

	isor(morehelp) rosi(longhelp)
	    longhelp();
	next

	is(version)
	    showversion = 1;
	next

	is(prompter)
	    prompter = 1;
	next

	is(startup)
	    startup = 1;
	next

	isor(tutor) rosi(tutorial)
	    tutor = 1;
	    startup = 0;
	next

	is(builder)
	    builder = 1;
	    startup = 0;
	next

	startswith(-no)
	    add(uirestrict);
	next

	argadd(limitImageOptions, uirestrict)

	argadd(metric, uiflags)

	is(restrictionLevel)
	    add(uirestrict);
	    check("-restrictionLevel: missing level");
	    add(uirestrict);
	next

	is(appHost)
	    add(uiflags);
	    check("-appHost: missing host");
	    add(uiflags);
	next

	is(appPort)
	    add(uiflags);
	    check("-appPort: missing port");
	    add(uiflags);
	next

	is(file)
	    check("-file: missing input filename");
	    set(FileName);
	next

	is(full)
	    add(prompterflags);
	next

	is(view)
	    add(uiflags);
	    check("-view: missing input filename");
	    add(uiflags);
	next

	startswith(&)
	    sprintf(msgstr, "ignoring option: %s --- & used only on Unix systems\n", s);
	    printf(msgstr);
	next

	startswith(-)
	    strcpy(errmsg, "Unrecognized parameter: ");
	    strcat(errmsg, s);
	    goto error;
	next

	{
	    if (*FileName) {
		sprintf(errmsg, "input filename already set to \'%s\'; \'%s\' unrecognized", FileName, s);
		goto error;
	    }
	    set(FileName);
	}
	advance;
    }

done:
    return 1;

error:
    printf("%s\n", errmsg);
    exit(1);
}


int shorthelp()
{
    printf(


"command line parameters:\n"
" -program filename    start UI with this network\n"
" -image               start DX with an image window as the anchor window \n"
" -edit                start DX with an editor window as the anchor window \n"
" -menubar             start DX with a small menubar as the anchor window \n"
" -startup             start DX with an initial startup panel (default) \n"
"\n"
" -uionly              start the UI only (no exec)\n"
" -script filename     start the exec only, in script mode, running this script\n"
" -script              start the exec only, in script mode\n"
"\n"
" -host hostname       start executive on this machine\n"
" -memory #Mbytes      set the amount of memory the exec uses\n"
"\n"
" -macros pathlist     directory list to search for UI macros\n"
" -data pathlist       directory list to search for data files\n"
"\n"
" -prompter            start the DX Data Prompter\n"
" -builder             start the DX Module Builder\n"
" -tutor               start the DX Tutorial\n"
"\n"
" -morehelp            print longer help with information about other options\n"
 

    );

    exit(0);
}

int longhelp()
{
    printf(


"command line parameters:\n"
" -host hostname       start executive on this machine               \n"
" -local               start executive on the current machine (default)\n"
"\n"
" -program filename    start UI with this network\n"
" -script filename     run exec only, in script mode, with this script\n"
" -script              run exec only, in script mode\n"
"\n"
" -image               start DX with an image window as the anchor window \n"
" -edit                start DX with the VP Editor as the anchor \n"
" -menubar             start DX with a small menubar as the anchor window \n"
" -startup             start DX with an initial dialog (default) \n"
"\n"
" -uionly              start the UI only (no exec)\n"
" -execonly            start the exec only (no UI) in remote mode\n"
" -connect host:port   start a distributed exec only (no UI)\n"
"\n"
" -prompter            start the DX Data Prompter\n"
" -jdxserver           start the JavaDX Server\n"
" -full                start the Full Data Prompter\n"
" -file filename       start the Data Prompter with this header file\n"
"\n"
" -builder             start the DX Module Builder\n"
" -tutor               start the DX Tutorial\n"
"\n"
" -suppress            do not open control panels at startup in image mode\n"
" -execute             execute the program automatically at startup\n"
" -execute_on_change   go into execute_on_change mode at startup\n"
"\n"
" -optimize [memory|precision]\n"
"                      select whether to minimize memory usage or to produce \n"
"                      more color-accurate images.  When memory is optimized, \n"
"                      image objects are generated with 24 bits/pixel instead \n"
"                      of 96, and ReadImage will produce delayed color images \n"
"                      if supported by the format. (default = precision)\n"
"\n"
" -memory #Mbytes      set the amount of memory the exec uses\n"
" -processors #proc    set the number of processors the exec uses \n"
"                      (SMP versions only)\n"
" -log [on|off]        executive and ui logging: (default = off)\n"
" -cache [on|off]      executive cache: (default = on)\n"
" -trace [on|off]      executive trace: (default = off)\n"
" -readahead [on|off]  executive readahead: (default = on)\n"
" -timing [on|off]     module timing: (default = off)\n"
" -highlight [on|off]  node execution highlighting: (default = on)\n"
" -directory dirname   cd to this directory before starting exec\n"
" -display hostname:0  set X display destination\n"
" -metric              have the UI use metric units when possible\n"
"\n"
" -exec filename       execute this user executive\n"
" -mdf filename        use this user .mdf file\n"
"\n"
" -key <64bit hex>     16 character hexidecimal (64 bit) number that is used\n"
"		      to encode and decode .net files.\n"
" -encode              Encode a .net file into a binary format with a key \n"
"                      that must be specified with the -key option.   \n"
"                      For example, \n"
"                        dx -encode -key 193495946952abed foo.net \n"
"                      The resulting file can only be decoded by the DX\n"
"                      user interface when using the same -key option.\n"
"                      For example, \n"
"                        dx -key 193495946952abed bar.net \n"
"\n"
" -dxroot dirname      dx root directory; defaults to /usr/lpp/dx\n"
"\n"
" -macros pathlist     directory list to search for UI macros\n"
" -data pathlist       directory list to search for data files\n"
" -include pathlist    directory list to search for script files\n"
" -modules pathlist    directory list to search for outboard modules\n"
"\n"
" -colors filename     replace default color names/RGB values file\n"
" -8bitcmap [private|shared|0-1|-1]\n"
"                      private/shared colormap error threshold (default=0.1)\n"
"                      -1 is equivalent to private.\n"
" -hwrender [gl|opengl]  \n"
"                      override default hardware rendering library on platforms\n"
"                      where both are supported.  (default = opengl).\n"
"\n"
" -verbose             echo command lines before executing\n"
" -echo                echo the command lines without executing them\n"
" -outboarddebug       let user start outboard modules by hand\n"
" -version             show version numbers of dxexec, dxui, dx.exe, and X server\n"
" -synchronous         force synchronization of X events between dx and the x server\n"
"\n"
" -help                print a short help message\n"
" -morehelp            print this help message\n"
"\n"
"environment variables:\n"
" DXHOST               sets hostname for -host\n"
"\n"
" DXPROCESSORS         sets number of processors for multiprocessor DX\n"
" DXMEMORY             sets memory limit in megabytes\n"
" DXAXESMAXWIDTH       sets the maximum number of digits in axes tick labels\n"
"                      before a switch to scientific notation is made\n"
" DXDEFAULTIMAGEFORMAT Sets the image type to either 24-bit color or floating\n"
"                      point color (96-bit) images depending on the value\n"
"                      DXByte (24-bit) or DXFloat (96-bit)\n"
" DXDELAYEDCOLORS      If set to anything other than 0, enables ReadImage to\n"
"                      created delayed color images if the image is stored in\n"
"                      a tiff byte-with-colormap format or a gif image\n"
" DX_NESTED_LOOPS      For faces, loops, and edges data, if set, allows loops\n"
"                      other than the enclosing loop for a face to be listed\n"
"                      first, with a consequent decrease in performance\n"
" DXPIXELTYPE          Sets the image type to either 24-bit color or floating\n"
"                      point (96-bit) color.  This affects the behavior of\n"
"                      Render and ReadImage.  This can be set to either DXByte\n"
"                      or DXFloat (default).\n"
" DX_USER_INTERACTOR_FILE Specifies a file containing user interactors for use by\n"
"                      the SuperviseState and SuperviseWindow modules\n"
"\n"
" DXEXEC               sets filename for -exec\n"
" DXMDF                sets filename for -mdf\n"
"\n"
" DXMACROS             sets pathlist for -macros\n"
" DXDATA               sets pathlist for -data\n"
" DXINCLUDE            sets pathlist for -include\n"
" DXMODULES            sets pathlist for -modules\n"
"\n"
" DXCOLORS             sets filename for -colors\n"
" DX8BITCMAP           sets threshold for -8bitcmap\n"
"\n"
" DXGAMMA              sets gamma correction for displayed images. Default is 2.\n"
" DXGAMMA_8BIT\n"
" DXGAMMA_12BIT\n"
" DXGAMMA_24BIT        sets the gamma correction factor for the windows with \n"
"                      the indicated window depth.  Overrides the value set \n"
"                      by DXGAMMA.\n"
"\n"
" DXHWMOD              specifies the name of the hardware rendering library \n"
"                      to use when more than one is supported. Should be\n"
"                      either DXhwdd.o or DXhwddOGL.o\n"
"\n"
" DXROOT               sets directory for -dxroot\n"
"\n"
" DXARGS               prepends these args to the command line\n"
"\n"
"command line parameters override environment variables.\n"
"If conflicting parameters are given, the last one has precedence.\n"
"Also, there are some other less frequently used command line options\n"
"that are not documented here.  See the User's Guide for a complete\n"
"list of options.\n"
"\n"


    );

    exit(0);
}
#endif
