/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#if defined(DXD_LICENSED_VERSION)
fjdasjfasjhsjasf
#endif

#include <dxconfig.h>


#include <DXStrings.h>

// This nonsense surrounds sys/types.h because its typedef for boolean conflicts
// with one from defines.h.  Many includes are ifdef on ARCH because of the
// need for select().
#if defined(solaris) 
#define boolean bool
#endif

#include <sys/types.h>

#if defined(solaris)
#undef boolean
#endif

#include <stdio.h>
#include <time.h>

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#if defined(sgi)
#include <bstring.h>
#endif

#if defined(ibm6000)
#include <sys/select.h>
#endif

#if defined(HAVE_SYS_UTSNAME_H)
#include <sys/utsname.h>
#endif

#if defined(HAVE_SYS_TIMEB_H)
#include <sys/timeb.h>
#elif defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#endif

#if defined(_AIX41)
#include <strings.h>
#endif

#if defined(aviion)
extern "C" { void bzero(char *, int); }
#endif

#include <X11/Intrinsic.h>

#include "IBMApplication.h"

#include "License.h"

#if defined(DXD_LICENSED_VERSION) && DXD_LICENSED_VERSION!=0 
# define NEEDS_LICENSE_ROUTINES 1
#else
# define NEEDS_LICENSE_ROUTINES 0
#endif

#if NEEDS_LICENSE_ROUTINES 

extern "C" {


#if (defined(sgi) && !( __mips > 1)) || defined(aviion)
const char *crypt(const char*, const char*);
#endif

#if defined(solaris)
#include <crypt.h>
#endif

#ifdef sun4 
int gethostid();
int getdtablesize();
#endif


#ifdef sgi
unsigned sysid(unsigned char id[]);
int getdtablesize(void);
#endif


#if defined(aviion) 
int gethostid();
int getdtablesize();
int gettimeofday(struct timeval*, struct timezone*);
#endif

#ifdef alphax 
#include <crypt.h>
int getdtablesize();
#include <stdio.h>              /* standard I/O */
#include <errno.h>              /* error numbers */

#if defined(windows) && defined(HAVE_WINSOCK_H)
#include <winsock.h>
#elif defined(HAVE_CYGWIN_SOCKET_H)
#include <cygwin/socket.h>
#elif defined(HAVE_SYS_SOCKET_H)
#include <sys/socket.h>
#endif

#include <sys/ioctl.h>          /* ioctls */
#include <net/if.h>             /* generic interface structures */
#include <sys/systeminfo.h>  /* maybe someday this will be implemented...arg! */
extern "C"   int select(
          int nfds,
          fd_set *readfds,
          fd_set *writefds,
          fd_set *exceptfds,
          struct timeval *timeout) ;
#endif

}
#define CRYPT(A,B) crypt((const char*)A, (const char*)B) 

#define ANYWHERE_HOSTID "00000000"

#if defined(DXD_LICENSED_VERSION) && !defined(HAVE_CRYPT)
  error:  Can not  run licensing routines without crypt()
#endif

static int  checkexp(const char *root);

#endif // NEEDS_LICENSE_ROUTINES 

#ifndef DXD_WIN

void UIGetLicense(const char *root, 
			XtInputCallbackProc lostLicense,
			LicenseTypeEnum *appLic,
			LicenseTypeEnum *funcLic)
{
#if !NEEDS_LICENSE_ROUTINES 
    *appLic = FullyLicensed;
    *funcLic = FullFunctionLicense;
    return;
#else

    LicenseTypeEnum forcedFuncLic;
    int i;
    int child;
    int child_in[2],child_out[2];
    char remname[1024];
    char auth_msg[AUTH_MSG_LEN];
    char ckey[128];
    char c_buf[128],p_buf[128];	/* hold crypted msgs for comaparison */
    char envbuf[128];
    char salt[32];
    time_t ctime;
  
	
    if (checkexp (root)) { /* check for an old syle trial license */
	*appLic = NodeLockedLicense;
        *funcLic = FullFunctionLicense;
	return; 
    }


    /* didn't find a trial license so spawn the NetLS licensing process */
    *appLic = Unlicensed;
    forcedFuncLic = *funcLic;
    *funcLic = Unlicensed;
    
    /* Set up two pipes */
    if (pipe(child_in) < 0) {
        perror("pipe(child_in)");
	return;
    }

    if (pipe(child_out) < 0) {
        perror("pipe(child_out)");
	return ;
    }
    
    ctime=time(NULL);

    sprintf(envbuf,"_DX_%d=",getpid());
    sprintf(c_buf,"%x",ctime);
    strcat(envbuf,c_buf+4);
    putenv(DuplicateString(envbuf));


    child = 0xffff&fork();


    if (child == 0) {	/* Child */

      char arg1[512];
      char arg2[512];
      char arg3[512];
#ifdef hp700
    int  width = MAXFUPLIM;
#else
#ifdef solaris
    int  width = FD_SETSIZE;
#else
    int  width = getdtablesize();
#endif
#endif

      close(child_in[1]);
      close(child_out[0]);

      if (dup2(child_in[0], 0) < 0) 
	exit(1);
      
      if (dup2(child_out[1], 1) < 0) 
	exit(1);

      /* close other file descriptors here  */
#if !defined(__PURIFY__)
      // purify uses some file descriptors
      for (i=3 ; i<=width ; i++)
	   close(i);
#endif
    
      char *s;
      if (s = getenv("DXSHADOW"))
	strcpy(remname,s);
      else 
	sprintf(remname,"%s/bin_%s/dxshadow",root,DXD_ARCHNAME);

      switch (forcedFuncLic) {
            case RunTimeLicense: strcpy(arg1,"-rtonly");      break;
            case DeveloperLicense: strcpy(arg1,"-devonly");     break;
            default:    strcpy(arg1,"-dev"); break;
      }
      int maj, min,mic;
      theIBMApplication->getVersionNumbers(&maj,&min,&mic);
      sprintf(arg3,"%d.%d.%d",maj,min,mic);
	
      execlp(remname, "dxshadow", arg1, "-version", arg3, NULL);

      //
      // If we get here, we failed. 
      //
      fprintf(stderr,"License Error: could not exec license process\n");
      exit(1);	
    }

    //
    // Only the parent gets here
    //
    close (child_in[0]);
    close (child_out[1]);
      
    /* wait for response from the child */
      
#define USE_SUB_EVENT_LOOP 1
#if USE_SUB_EVENT_LOOP
    // Instead of doing a blocking read... and instead of writing a communication
    // subsystem, monitor all sockets of interest.  When something arrives, service
    // it.  As a result, X Events still get processed and we achieve a little extra
    // concurrency which should decrease startup time.  According to Quantify,
    // we were spending lots of time inside the call to read().
    // The loop waits approximately 5 seconds.  If dxshadow takes longer than that,
    // then execution continues by sitting in the read() command as it used to.
    fd_set rdfds;
    XEvent event;
    FD_ZERO(&rdfds);
    Display *d = theApplication->getDisplay();
    XtAppContext app = theApplication->getApplicationContext();
    timeval tval, starttime;
    gettimeofday (&starttime, NULL);
    while (!FD_ISSET(child_out[0], &rdfds)) {
	FD_SET (child_out[0], &rdfds);
	FD_SET (ConnectionNumber(d), &rdfds);
	tval.tv_sec = 1; tval.tv_usec = 0;
	if (select (32, (SELECT_ARG_TYPE *)&rdfds, NULL, NULL, &tval) == -1) break;
	XtInputMask mask;
	while ((mask = XtAppPending (app)) & (XtIMXEvent|XtIMTimer)) {
	    if (XtIMXEvent & mask) {
		XtAppNextEvent (app, &event);
		theIBMApplication->passEventToHandler (&event);
	    } 
	    if (XtIMTimer & mask) {
		XtAppProcessEvent (app, XtIMTimer);
	    }
	}
	if (gettimeofday (&tval, NULL) == -1) break;
	if ((tval.tv_sec - starttime.tv_sec) >= 10) break;
    }
#endif
    i = read(child_out[0],auth_msg,AUTH_MSG_LEN);

    if (!i) {
	fprintf(stderr,"License Error\n");
	goto unlicensed;
    }

    /* decipher license message here */
            
    child = (child<4096)?(child+4096):(child); /* forces to be 4 0x chars */
	
    strcpy(ckey,c_buf+4);
    sprintf(ckey+4,"%x",child);
      
    salt[0] = '7';
    salt[1] = 'q';
    salt[2] = '\0';
      	
    strcpy(p_buf,CRYPT(ckey,salt));;
	
    for(i=0;i<13;i++)
	c_buf[i] = auth_msg[(i*29)+5];
    c_buf[13] = '\0';

    if (strcmp(c_buf,p_buf)) {
	
	/* Bad message from child */

	fprintf(stderr,"License error\n");
	goto unlicensed;
    }

    /* valid message so we extract license type */

    for(i=0;i<8;i++)
	  c_buf[i] = auth_msg[(i*3)+37];

    c_buf[8] = '\0';

    sscanf(c_buf,"%x",&i);
    *appLic = (LicenseTypeEnum)(0xffff & (i^child));
    i = i >> 16;
    *funcLic = (LicenseTypeEnum)(0xffff & (i^child));
#if 000 
    fprintf(stderr,"c_buf = '%s', funcLic = 0x%x, appLic = 0x%x\n",
			    c_buf,*funcLic,*appLic);
#endif

    const char* lic_name;
    switch (*funcLic) {
	case DeveloperLicense: lic_name = "DX development"; break;
	case RunTimeLicense:   lic_name = "DX run-time"; break;
	default: 
	        fprintf(stderr,"Unrecognized license\n");
	        goto unlicensed;
		break;
    }

    switch (*appLic) {

	case NodeLockedLicense:
	  
#ifdef DEBUG
	      fprintf(stderr,"Got nodelocked %s license\n",lic_name);
#endif 
	      return; 

	case ConcurrentLicense:

#ifdef DEBUG
	      fprintf(stderr,"Got concurrent %s license\n",lic_name);
#endif
	  
	      XtAppAddInput(theIBMApplication->getApplicationContext(),
			child_out[0],
			(XtPointer)(XtInputReadMask),
			lostLicense,
			(XtPointer)NULL);

	      return ;

	  
	case Unlicensed:

#ifdef DEBUG
	      fprintf(stderr,"Could not get a license\n");
#endif 
	      break;


	default: 	/* invalid license type */
#ifdef DEBUG
	    
	      fprintf(stderr,"License Error: Invalid License Type\n");
#endif
	      goto unlicensed;
    }


unlicensed:
    *appLic = Unlicensed;
    *funcLic = Unlicensed;
    return;	

#endif // NEEDS_LICENSE_ROUTINES 
}  






/* This function creates the message which will tell the exec if it
 * is OK to run without a license. inkey must be at least char[14]
 * and should contain the key returned from the $getkey.  type
 * should be either ConcurrentLicense or NodeLockedLicense. On return
 * inkey holds the string to send to the exec with $license.    
 * The returned string must be freed by the caller.
 */	 

char *GenerateExecKey(const char *inkey, LicenseTypeEnum licenseType)
{

#if NEEDS_LICENSE_ROUTINES 

    int i;
    char keybuf[64];
    char cryptbuf[64];
    char salt[8];

    for(i=0;i<4;i++)
      keybuf[i*2]=inkey[i];

    keybuf[1] = 'g';
    keybuf[3] = '3';
    keybuf[5] = '$';
    keybuf[7] = 'Q';
    keybuf[8] = '\0';
	    
    salt[0] = '4';
    salt[1] = '.';
    salt[2] = '\0';

    strcpy(cryptbuf,CRYPT(keybuf,salt));

    char *outkey = new char[64]; 
    sprintf(outkey,"%s%hx",cryptbuf,
			(unsigned short)licenseType ^
			(*((unsigned char *)&cryptbuf[4])<<8)+(*((unsigned char *)&cryptbuf[5])));

    return outkey;
#else
    return NULL;
#endif // NEEDS_LICENSE_ROUTINES 

}




#if NEEDS_LICENSE_ROUTINES 

#define KEY1 "a9"
#define KEY2 "Pp"

#if defined(aviion) || defined(solaris) 
#include <sys/systeminfo.h>
#if defined(aviion) 
extern "C" {
 long sysinfo (int command, char *buf, long count);
}
#endif
#endif

#ifdef alphax
extern "C" int gethostid(void);
#endif

static int  checkexp(const char *root)
{
#if !DXD_HAS_CRYPT
  return (1);
#else
  int	host_match;
  char   key[32];
  char   cryptHost[1024];
  char   cryptTime[1024];
  char   host[512];
  time_t timeOut;
  int    i;
  char  *myCryptHost;
  struct timeval sysTime;
#if defined(ibm6000) || defined(hp700)
  struct utsname name;
#endif
#if defined(sgi) || defined(sun4)   || defined (alphax)
  long   name;
#endif
  time_t time;
  char   fileName[1024];
  FILE   *f;
  
  for (i = 0; i < sizeof(key); ++i)
    key[i] = '\0';
  
#ifdef ibm6000
#define FOUND_ID 1
  uname(&name);
  name.machine[10] = '\0';
  strcpy(host, name.machine+2);
#endif
#if hp700
#define FOUND_ID 1
  uname(&name);
  name.idnumber[10] = '\0';
  strcpy(host, name.idnumber+2);
#endif
#if sgi              /* sgi does not like #if...#elif..#endif construct */
#define FOUND_ID 1
  name = sysid(NULL);
  sprintf(host, "%d", name);
  strcpy(host, host+2);
#endif
#if sun4 
#define FOUND_ID 1
  name = gethostid();
  sprintf(host, "%x", name);
#endif
#if aviion
#define FOUND_ID 1
  sysinfo(SI_HW_SERIAL,host,301);
#endif
#if solaris
#define FOUND_ID 1
    sysinfo(SI_HW_SERIAL,host,301);
    sprintf(host, "%x", atol(host));
#endif
#if defined(alphax)
#ifdef SYSINFO_WORKS 
// The man page for OSF/1 V2 says that SI_HW_SERIAL does not work.  We'll use it
// for now even though it doesn't work.  So far it looks like the only mechanism
// to get unique ids.
    sysinfo(SI_HW_SERIAL,host,301);
    sprintf(host, "%x", atol(host));
#else
{
    char *device;
    char *dflt_devices[] = {"tu0","ln0", NULL };
    int s,i;                             /* On Alpha OSF/1 we use ethernet */;

    /* Get a socket */
    strcpy(host,"");
    s = socket(AF_INET,SOCK_DGRAM,0);
    if (s < 0) {
        perror("socket");
    } else {
	for (i=0, device=(char*)getenv("DXKEYDEVICE"); dflt_devices[i]; i++) {
	    static   struct  ifdevea  devea; /* MAC address from and ioctl() */
	    char *dev, buf[32];
	    if (!device) 
		dev = dflt_devices[i];
	    else
		dev = device;
	    strcpy(devea.ifr_name,dev);
	    if (ioctl(s,SIOCRPHYSADDR,&devea) >= 0)  {
		strcpy(host,"");
		for (i = 2; i < 6; i++){
		    sprintf(buf,"%x", devea.default_pa[i] );
		    strcat(host,buf);
		}
		break;
	    } 
	    if (device) break;
	}
	close(s);
    }
}

#endif
#define FOUND_ID 1
#endif
#if !defined(FOUND_ID)
  Trial version not supported on this architecture.
#else
# undef FOUND_ID
#endif
    
  gettimeofday(&sysTime, NULL);
  time = sysTime.tv_sec;
  
  if (getenv("DXTRIALKEY")) {
	char *k = getenv("DXTRIALKEY");
	fprintf(stderr, "Data Explorer trial password found in"
		    " DXTRIALKEY environment variable\n");
	strncpy(key,k,27);
        key[27] = '\0';	// Make sure it is terminated
  } else {
      char *fname;
      fname = getenv("DXTRIALKEYFILE");
      if (!fname) {
	  sprintf(fileName, "%s/expiration", root);
	  fname = fileName;
      }
      f = fopen(fname, "r");
      if (f)  {
	fprintf(stderr, "Data Explorer trial password found in file %s\n",
							fname);
	fgets(key, 27, f);
	fclose(f);
      } else {
	return 0;
      }
  } 
  
  
  if (strlen(key) != 26) {
    fprintf(stderr, "You are running an expired Trial version of Data Explorer\n");
    return(0);
  }
  
  for (i = 0; i < 13; ++i) {
    cryptHost[i] = key[2 * i];
    cryptTime[i] = key[2 * i + 1];
  }
  cryptHost[i] = key[2 * i] = '\0';
  cryptTime[i] = key[2 * i + 1] = '\0';
  
  if (cryptTime[0] != 'A' || 
      cryptTime[10] != '9' || 
      cryptTime[12] != 'D') {
    fprintf(stderr, "You are running an Expired trial version of Data Explorer\n");
    return(0);
  }
  

  myCryptHost = (char*)CRYPT(host,KEY1); 
  host_match = strcmp(cryptHost, myCryptHost) == 0;
  if (!host_match) {
	myCryptHost = (char*)CRYPT(ANYWHERE_HOSTID,KEY1);
        host_match = strcmp(cryptHost, myCryptHost) == 0;
  } 
  if (!host_match) {
    fprintf(stderr, 
	"You are running a trial version of Data Explorer on an"
	" unlicensed host\n");
    return (0);
  }
  
  if(cryptTime[1]=='s')
     sscanf(cryptTime, "As%08x95D", &timeOut);
  else if (cryptTime[1]=='m')
     sscanf(cryptTime, "Am%08x9lD", &timeOut);
  

timeOut ^= 0x12345678;
  
  if (time > timeOut) {
    fprintf(stderr, "You are running an expired trial version of Data Explorer\n");
    fprintf(stderr,"This trial key expired on %s", ctime(&timeOut));  
    return (0);
  }
  fprintf(stderr,"This Data Explorer trial key will expire on %s",
				ctime(&timeOut));  
  return (1);
#endif /* DXD_HAS_CRYPT */
}

#ifdef ibm6000
// Some very strange declarations that allow AIX 3.2.4 systems to link.
// These symbols are statics inside of "crypt.c" in /usr/lib/libc.a, but
// are referenced in the mapping defined for the shared library.  Therefore,
// the system won't link without them.
extern "C" char __setkey[1024];
extern "C" char __crypt[1024];
extern "C" char __encrypt[1024];
char __setkey[1024];
char __crypt[1024];
char __encrypt[1024];
#endif

#endif // NEEDS_LICENSE_ROUTINES 

#else  // DXD_WIN
#if 0

#include <windows.h>
#include <math.h>

static int getregval(char *name, char *value)
{
    char key[500];
    int valtype;
    unsigned long sizegot = 500;
    int word;
    char errstr[200];
    HKEY hkey[10];
    long rc;
    int i, k=0;

#define iferror(s)	\
    if (rc != ERROR_SUCCESS) {	\
	strcpy(errstr, s);	\
	goto error;		\
    }

    strcpy(value, "");
    word = 0;
    strcpy(key, "SOFTWARE");

    rc = RegOpenKeyEx(HKEY_LOCAL_MACHINE, (LPCTSTR) key, 0,
	KEY_QUERY_VALUE, &hkey[k++]);

    strcat(key, "\\OpenDX");
    rc = RegOpenKeyEx(HKEY_LOCAL_MACHINE, (LPCTSTR) key, 0,
	KEY_QUERY_VALUE, &hkey[k++]);
    iferror("Error opening key");

    strcat(key, "\\Open Visualization Data Explorer");
    rc = RegOpenKeyEx(HKEY_LOCAL_MACHINE, (LPCTSTR) key, 0,
	KEY_QUERY_VALUE, &hkey[k++]);
    iferror("Error opening key");

    strcat(key, "\\CurrentVersion");
    rc = RegOpenKeyEx(HKEY_LOCAL_MACHINE, (LPCTSTR) key, 0,
	KEY_QUERY_VALUE, &hkey[k++]);
    iferror("Error opening key");

    rc = RegQueryValueEx(hkey[k-1], (LPTSTR) name, (LPDWORD) 0, 
	(LPDWORD) &valtype, (LPBYTE) value, &sizegot);
    iferror("Query value failed");

    for (i=k; i > 0; i--) {
	rc = RegCloseKey(hkey[i-1]);
	iferror("CloseKey failed");
    }

    switch(valtype) {
	case REG_DWORD:
	    word = *((int *)value); 
	    value = "";
	    break;
	case REG_SZ:
	    break;
	default:
	    return 0;
    }

    return 1;

error:
    fprintf(stderr, "%s: rc = %d\n", errstr, rc);
    return 0;
}

static int keyformat(char *k)
{
    int i;
    char buf[25];
    char *p;

    for (i=0, p=k; *p && i<20; p++) {
	if (isalpha(*p))
	    buf[i++] = tolower(*p);
	if (isdigit(*p))
	    buf[i++] = *p;
	if (i==5 || i==10 || i==15)
	    buf[i++] = ' ';
    }
    buf[i] = '\0';
    if (strlen(buf) != 20)
	return 0;
    strcpy(k, buf);
    return 1;
}

int getuserinforeg(char *username, char *userco, char *keystr)
{
    *username = '\0';
    *userco = '\0';
    *keystr = '\0';
    getregval("UserName", username);
    getregval("CompanyName", userco);
    getregval("LicenseKey", keystr);
    keyformat(keystr);
    return 1;
}

static int gettimenow(int *m, int *d, int *y)
{
    SYSTEMTIME t;
    GetSystemTime(&t);
    *m = t.wMonth;
    *d = t.wDay;
    *y = t.wYear;
    return 1;
}

static int addtotime(int *m, int *d, int *y)
{
    int mm, dd, yy;

    gettimenow(&mm, &dd, &yy);
    *y += yy;
    *m += mm;
    if (*m>12) {
	*y += (*m-1)/12;
	*m = *m%12 + 1;
    }
    *d += dd;
    if (*d > 28) {
	(*m)++;
	*d = *d%31;
    }
    if (*m>12) {
	*y += (*m-1)/12;
	*m = *m%12 + 1;
    }
    return 1;
}

static int genkey(char *keystr, char *username, char *userco,
		    int m, int d, int y, char lictype)
{
    char *p, *q;
    unsigned int key, key1, key2, key3, prime;
    int i;
    char data[500];

    data[0] = (char)(m + (int)'a');
    if (d > 20)
	data[0] = (char)((int)data[0] + 13);
    data[1] = (char)(d%20 + (int)'b');
    data[2] = (char)((y - 1994)/10 + (int)'g');
    data[3] = (char)((y - 1994)%10 + (int)'m');
    if (!lictype)
	lictype = 't';
    data[4] = lictype;

    q = &data[5];
    for (p=username; *p; p++)
	if (!isspace(*p) && !ispunct(*p))
	    *(q++) = tolower(*p);
    for (p=userco; *p; p++)
	if (!isspace(*p) && !ispunct(*p))
	    *(q++) = tolower(*p);
    *q = '\0';

    key = 99;
    key = key * 100 + 99;
    key = key * 100 + 83;

    prime = 4999;
    prime = prime * 1000 + 999;

    for (i = 0, p=data; *p; p++, i++)
	key += (*p * (i + key)) * prime;

    key %=   10000000000;
    key1 = key/100000000;
    key2 = key%100000000;
    key2 /= 10000;
    key3 = key%10000;
    sprintf(keystr, "%c%c%c%c%c %04d %04d %04d", data[0],
		data[1], data[2], data[3], data[4], key1, key2, key3);
    return 1;
}

int getdatefromkey(char *keystr, int *m, int *d, int *y)
{
    *m = (int)keystr[0] - (int)'a';
    *d = (int)keystr[1] - (int)'b';
    if (*m > 12) {
	*m -= 13;
	*d += 20;
    }
    if (*d < 1)
	*d = 1;
    if (*d > 31)
	*d = 31;
    *y = ((int)keystr[2] - (int)'g') * 10 +
		(int)keystr[3] - (int)'m' + 1994;
    return 1;
}

static int isexpired(char *keystr)
{
    int m, d, y;
    int ms, ds, ys;

    gettimenow(&ms, &ds, &ys);
    getdatefromkey(keystr, &m, &d, &y);
    if (ys > y)
	return 1;
    if (ys == y && ms > m)
	return 1;
    if (ys == y && ms == m && ds > d)
	return 1;
    return 0;
}

void UIGetLicense(const char *root, 
			XtInputCallbackProc lostLicense,
			LicenseTypeEnum *appLic,
			LicenseTypeEnum *funcLic)
{
    char username[200];
    char userco[200];
    char keystrreg[100];
    char keystr[100];
    char lictype;
    int m;
    int d;
    int y;
    int expired;

    *appLic = Unlicensed;
    *funcLic = Unlicensed;
    getuserinforeg(username, userco, keystrreg);
    if (strlen(username) + strlen(userco) < 6) {
	fprintf(stderr, "Improper registration: short username and company\n");
	return;
    }
    if (!keyformat(keystrreg)) {
	fprintf(stderr, "Improper registration: registration key is not in proper format: %s\n",
			    keystrreg);
	return;
    }
    fprintf(stderr, "Registered to %s of %s\n", username, userco);
    getdatefromkey(keystrreg, &m, &d, &y);
    lictype = keystrreg[4];
    genkey(keystr, username, userco, m, d, y, lictype);
    if (strcmp(keystr, keystrreg)) {
	fprintf(stderr, "Improper registration: key does not match user setup\n");
	return;
    }
    expired = isexpired(keystrreg);
    if (expired) {
	fprintf(stderr, "Registration expired %d/%d/%d\n", m, d, y);
	return;
    }
    // Don't show expire date if license isn't beta or trial
    if (lictype == 'b' || lictype == 't')
	fprintf(stderr, "Registration expires %d/%d/%d\n", m, d, y);
    if (lictype == 'r') {
	*appLic = NodeLockedLicense;
	*funcLic = RunTimeLicense;
    } else {
	*appLic = FullyLicensed;
	*funcLic = DeveloperLicense;
    }
    return;
}

char *GenerateExecKey(const char *inkey, LicenseTypeEnum licenseType)
{
    return "No License Generated";
}
#endif
#endif  // DXD_WIN
