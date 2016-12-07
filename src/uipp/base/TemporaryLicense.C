/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#if defined(DXD_LICENSED_VERSION)

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/time.h>
#include <time.h>
#ifndef DXD_DO_NOT_REQ_UNISTD_H
#include <unistd.h>
#endif


#if defined(ibm6000) 
extern "C" void gettimer (int, struct timestruc_t* );
#endif

#include "DXStrings.h"

#if defined(aviion) || defined(solaris) || defined(sgi)
#include <crypt.h>
#endif

#if defined(ibm6000) || defined(hp700)
#include <sys/utsname.h>
#if defined(ibm6000) && !defined(_AIX41)
extern "C" const char *crypt(const char* , const char*);
#else
extern "C" char *crypt(const char* , const char*);
#endif
#endif

#if defined(aviion) || defined(solaris) || defined(sgi)
#include <sys/systeminfo.h>
#endif
#if defined(sgi) && !( __mips > 1)
extern "C" unsigned sysid(unsigned char *id);
#endif
#if defined(aviion)
extern "C" long sysinfo (int , char *, long );
#endif
#if defined(aviion)
extern "C" int gettimeofday(struct timeval *);
#endif

#if defined(sun4)
extern "C" long gethostid();
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
extern "C"   int select(int , fd_set *, fd_set *, fd_set *, struct timeval *) ;
#endif

#include "TemporaryLicense.h"


const char *
TemporaryLicense::MakeKey
    (enum Arkh arch, char *hostId, int months, int days, 
	time_t *newTime, enum DxType type)
{
    char               host[9];
#if defined(ibm6000)
    struct timestruc_t sysTime;
#else
    timeval sysTime;
#endif
    struct tm          timeStruct;
    char	       cryptHost[301];
    char	       cryptTime[301];
    int                i;
    static char	      key[100];
    

    if (strlen(hostId) > 8 ||
	arch == IBM6000 ||
	arch == HP700 ||
	arch == SGI)
	strncpy(host, hostId+2, 8);
    else
	strncpy(host, hostId, 8);
    host[8] = '\0';

#if defined(aviion)
    gettimeofday(&sysTime);
#else
#if defined(ibm6000)
    gettimer(TIMEOFDAY, &sysTime);
#else
    gettimeofday(&sysTime, NULL);
#endif
#endif
#if defined(ibm6000) || defined(hp700)
    time_t tvs = sysTime.tv_sec;
    timeStruct = *localtime(&tvs);
    sysTime.tv_sec = (unsigned long)tvs;
#else
    timeStruct = *localtime(&sysTime.tv_sec);
#endif
    timeStruct.tm_mday += days;
    timeStruct.tm_mon  += timeStruct.tm_mday / 30;
    timeStruct.tm_mday  = timeStruct.tm_mday % 30;
    timeStruct.tm_mon  += months;
    timeStruct.tm_year += timeStruct.tm_mon / 12;
    timeStruct.tm_mon   = timeStruct.tm_mon % 12;

#if defined(sun4)
    *newTime = timelocal(&timeStruct);
#else
    *newTime = mktime(&timeStruct);
#endif

    *newTime ^= XOR_KEY; 
    
    if (type == UP)
      sprintf(cryptTime, "As%08x95D", *newTime);
    else if (type == SMP)
      sprintf(cryptTime, "Am%08x9lD", *newTime);
    
    *newTime ^= XOR_KEY; 

    strcpy(cryptHost, crypt(host, KEY1));

    for (i = 0; i < strlen(cryptHost); ++i) {
	key[2*i] = cryptHost[i];
	key[2*i+1] = cryptTime[i];
    }
    key[2*i] = '\0';

    return key;
}



//
// Warn the user that we're about to go away.  Set a timer.  When the timer
// goes off, bye bye.
//
static void LostLicense(XtPointer , int* fid, XtInputId*)
{
    //
    // As of 3.1.4, we're going to try to read from the file descriptor.
    // If there is any byte at all we're going to just keep
    // on running.  If we see error, or eof, then we'll assume that
    // dxshadow died and that we should die also.
    //
    char buff[256];
    int bread;
    bread = read(*fid, buff, 256);
    if (bread > 0) {
        while (bread == 256)
            bread = read(*fid, buff, 256);
    } else {
	exit(1);
    }
}

void 
TemporaryLicense::determineUILicense(LicenseTypeEnum *app, LicenseTypeEnum *func)
{
    *app = Unlicensed;
    UIGetLicense(this->getRootDir(), LostLicense, app,func);
}


void TemporaryLicense::initialize()
{

    //
    // Find out what we've got for a license.
    //
    LicenseTypeEnum app_lic, func_lic;
    app_lic = TimedLicense;
    func_lic = Unlicensed;
    if (getenv("DXTRIALKEY")) {
    } else {
	this->determineUILicense(&app_lic, &func_lic);

	//
	// Problems: 
	// - A UP license on an SMP machine gets full smp ability because 
	// the trial key I'm generating is using the SMP flag.
	// - If the user replaces the dxui executable with something of his own
	// which simply prints out the environment, then he'll get to see his
	// trial key which will give him full developer, multi-user, smp
	// capability.
	// - If the user edits the dx.workerscript, he could thwart my attempt
	// to limit the functioning of dxui since I'm just passing cmd line args
	// to the script when I use DXLStartDX().
	//

	//
	// If we got a floating developer license, then we'll go ahead an unleash
	// full use of the product.  If the license is runtime or nodelocked, then
	// the product has to be restricted:
	// 1) to prevent the user from reconnecting to a remote exec which 
	// would effectively allow unlimited use since you can run any number 
	// of dxui's on one machine if the dxexec's are remote.
	// 2) to prevent editor access which turns a runtime into a developer license.
	//
	if ((func_lic == DeveloperLicense) && (getenv("DXTEST_RUNTIME") == NUL(char*)) &&
	    ((app_lic == NodeLockedLicense) || (app_lic == ConcurrentLicense)) ) {
	    if (app_lic == NodeLockedLicense) {
		//
		// If you want to add something special for the case of nodelocked
		// developer license, this is the place.  I originally used
		// -noConnectionMenus so that the user would be unable to send
		// the trial key to another machine which would effectively allow
		// unlimited use of the produce.  Now, that trial key won't work
		// on another host, so -noConn isn't needed.
		//
		//this->setDxuiArgs(" -noConnectionMenus ");
		//this->limited_use = TRUE;
	    } 
	} else {
	    this->setDxuiArgs(this->getLimitedArgs());
	    this->limited_use = TRUE;
	}

	if (func_lic != Unlicensed) {

	    //
	    // If we got a good license, then put a trial key into the environment
	    // I think a reasonable alternative to the trial key would be encrypting
	    // the time.  Then the child app could do the same.  (Excluding seconds)
	    // Then if we gave the child app our encrypted time, she could compare
	    // and know (authenticate) that the parent is in fact startup.  Then 
	    // no licensing would be required.
	    //
	    // Security problems: If someone replaces the dxui executable, it would
	    // be simple to print out the environment and obtain the trial key we're
	    // using. 
	    //
	    char host[64];

	    // S G I     S G I     S G I     S G I     S G I     S G I     S G I     
#	if defined(sgi)
#	if ( __mips > 1)
	    sysinfo (SI_HW_SERIAL, host, sizeof(host));
#	else
            // this should really be an unsigned int because that's what sysid
	    // returns, but it HAS to be long to be compatible with the UI and exec
	    long name = (long)sysid(NULL);
	    sprintf (host, "%d", name);
	    strcpy (host, host+2);
#	endif
#	endif

	    // H P 7 0 0     H P 7 0 0     H P 7 0 0     H P 7 0 0     H P 7 0 0 
#	if defined(hp700)
	    struct utsname name;
	    uname (&name);
	    name.idnumber[10] = '\0';
	    strcpy (host, name.idnumber+2);
#	endif

	    // S O L A R I S     S O L A R I S     S O L A R I S     S O L A R I S 
#	if defined(solaris)
	    sysinfo(SI_HW_SERIAL,host,301);
	    sprintf(host, "%x", atol(host));
#	endif

	    // A V I I O N     A V I I O N     A V I I O N     A V I I O N 
#	if defined(aviion)
	    sysinfo(SI_HW_SERIAL,host,301);
#	endif

	    // I B M 6 0 0 0     I B M 6 0 0 0     I B M 6 0 0 0     I B M 6 0 0 0 
#	if defined(ibm6000)
	    struct utsname name;
	    uname (&name);
	    name.machine[10] = '\0';
	    strcpy (host, name.machine+2);
#	endif

	    // S U N 4     S U N 4     S U N 4     S U N 4     S U N 4     S U N 4 
#	if defined(sun4)
	    sprintf (host, "%x", gethostid());
#	endif

	    // A L P H A X     A L P H A X     A L P H A X     A L P H A X 
#	if defined(alphax)
	    {
		char *dflt_devices[] = {"tu0","ln0", NULL };
		char *device;
		int found, s,i        /* On Alpha OSF/1 we use ethernet */;

		/* Get a socket */
		s = socket(AF_INET,SOCK_DGRAM,0);
		if (s < 0) {
		    perror("socket");
		    exit(1);
		}
		for (found=i=0, device=(char*)getenv("DXKEYDEVICE"); dflt_devices[i]; i++) {
		    static struct ifdevea devea; /* MAC address from and ioctl()   */
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
			found = 1;
			break;
		    }
		    if (device) break;
		}
		if (!found)  {
		    fprintf(stderr,"Could not find ethernet device.\n");
		    strcpy(host,"UNKNOWN");
		}
		close(s);
	    }

#	endif

	    time_t tim;
	    //
	    // The use of ANY right here doesn't mean anything.  It isn't an
	    // "any" key, it's still nodelocked.  We're just passing ANY because
	    // the hostid has already been munged and MakeKey does not need to
	    // munge it further as it would in the expire program.
	    //
	    const char* key = TemporaryLicense::MakeKey(ANY, host, 0, 1, &tim, SMP);

	    char* cp = "DXTRIALKEY=";
	    char *envsetting = new char[strlen(cp) + strlen(key) + 1];
	    sprintf (envsetting, "%s%s", cp,key);
	    putenv (envsetting);
	    cp = "DXSTARTUP=1";
	    envsetting = new char[strlen(cp) + 1];
	    strcpy (envsetting, cp);
	    putenv (envsetting);
	}
    } 

}

void TemporaryLicense::setDxuiArgs(const char* a) 
{
    if (dxui_args) delete dxui_args;
    if (a) dxui_args = DuplicateString(a);
    else dxui_args = NUL(char*);
}

const char* TemporaryLicense::getLimitedArgs()
{ 
    return " -noConnectionMenus -noEditorAccess "; 
}

#endif // DO NOT ADD ANYTHING AFTER THIS ENDIF
