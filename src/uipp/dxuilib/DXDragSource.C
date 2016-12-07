/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"

#include <stdio.h>

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#if defined(HAVE_SYS_STAT_H)
#include <sys/stat.h>
#endif

#include "DXStrings.h"
#include "DragSource.h"
#include "DXApplication.h"
#include "DXDragSource.h"
#include "WarningDialogManager.h"
#include "Network.h"

#if defined(HAVE_NETDB_H)
#include <netdb.h>
#endif

#if defined(NEEDS_GETHOSTNAME_DECL)
extern "C" int gethostname(char *address, int address_len);
#endif

boolean DXDragSource::DXDragSourceClassInitialized = FALSE;

static char *header_fmt = "%s:%d, net length = %d, cfg length = %d\n";

//
// superclass constructor used a list of Atom names which we have
// to pass through.
//
DXDragSource::DXDragSource (PrintType printType): DragSource() 
{ 
    this->printType = printType;
}

//
// No data allocated in Constructor... nothing to free
//
DXDragSource::~DXDragSource() { }

//
void
DXDragSource::initialize()
{
    if (!DXDragSourceClassInitialized) {
	DXDragSourceClassInitialized = TRUE;
    }
}

//
// Called almost like a callback, by the OSF/Motif dnd mechanism.  This class
// doesn't allow subclasses to replace this method.  The reason for living is
// to supply .net,.cfg files as dnd data communication mechanism.  Invoke a 
// virtual function which subclasses are required to provide.  That supplies
// 2 file names.  Take those files, create a header string, concat all the
// data and supply it to Motif to pass to the X server.  The header string
// is a special format which DXDropSite understands.  It tells hostname,pid
// do that we can figure out if this is intra-executable dnd.  It also tells
// lengths of .net and .cfg chunks so that DXDropSite can unravel it and
// put the .net and .cfg portions into files.
//
boolean DXDragSource::convert 
	(Network *netw, char *, XtPointer *value, unsigned long *length, long)
{
FILE            *netf = 0;
FILE            *cfgf = 0;
char            netfilename[256];
char            cfgfilename[256];
char            header[256];
struct STATSTRUCT statb;
char            *buf;
unsigned long   header_len;
unsigned long   net_len;
unsigned long   cfg_len;
char		hostname[MAXHOSTNAMELEN];
const char 	*tmpdir;
int		tmpdirlen;

    //
    // build the tmp filenames
    //
    tmpdir = theDXApplication->getTmpDirectory();
    tmpdirlen = STRLEN(tmpdir);
    if (!tmpdirlen) return FALSE;
    if (tmpdir[tmpdirlen-1] == '/') {
	sprintf(netfilename, "%sdx%d.net", tmpdir, getpid());
	sprintf(cfgfilename, "%sdx%d.cfg", tmpdir, getpid());
    } else {
	sprintf(netfilename, "%s/dx%d.net", tmpdir, getpid());
	sprintf(cfgfilename, "%s/dx%d.cfg", tmpdir, getpid());
    }
    unlink (netfilename);
    unlink (cfgfilename);

    netf = fopen(netfilename, "a+");
    if(netf == NULL)
    {
        WarningMessage("ControlPanel:fopen Drag conversion failed");
        return FALSE;
    }

    // callee should not write into filenames
    if (!this->createNetFiles (netw, netf, cfgfilename)) return FALSE; // no warning;
    fclose(netf);

    // FIXME: I would rather not close the file, then reopen it immediately
    // but you must use "r+" to open for reading and writing, and you 
    // can't use "r+" on a file that doesn't exist yet.

#ifdef DXD_OS_NON_UNIX
    if ((netf = fopen(netfilename, "rb")) == NULL)
#else
    if ((netf = fopen(netfilename, "r")) == NULL)
#endif
    {
	WarningMessage("DXDragSource:Drag conversion failed");
	return FALSE;
    }

#ifdef DXD_OS_NON_UNIX
    cfgf = fopen(cfgfilename, "rb");
#else
    cfgf = fopen(cfgfilename, "r");
#endif

    if(cfgf == NULL)
    {
        cfg_len = 0;
    }
    else
    {
	//
	// Get the length of the cfg file
	//
        if(STATFUNC(cfgfilename, &statb) == -1)
        {
            WarningMessage("DXDragSource:stat Drag conversion failed");
	    fclose (netf);
            return FALSE;
        }
        cfg_len = statb.st_size;
    }

    //
    // Get the length of the net file
    //
    if(STATFUNC(netfilename, &statb) == -1)
    {
        WarningMessage("DXDragSource:stat Drag conversion failed");
	if (netf) fclose(netf);
        return FALSE;
    }
    net_len = statb.st_size;

    if (gethostname (hostname, sizeof(hostname)) == -1)
    {
        WarningMessage("DXDragSource:could not find name of current host");
	if (netf) fclose(netf);
	if (cfgf) fclose(cfgf);
        return FALSE;
    }
    
    sprintf (header, header_fmt, hostname, getpid(), net_len, cfg_len);
    header_len = STRLEN(header);

    *length = header_len + net_len + cfg_len;
    buf = new char[1 + *length];

    //
    // Construct the buffer [header + net + cfg]
    //
    strcpy(buf, header);
    if(fread(&(buf[header_len]), sizeof(char),
	     (unsigned int)net_len, netf) != net_len)
    {
        WarningMessage("DXDragSource:fread Drag conversion failed");
	if (netf) fclose(netf);
	if (cfgf) fclose(cfgf);
        return FALSE;
    }
    if(cfg_len > 0)
    {
        if(fread(&(buf[header_len+net_len]), sizeof(char),
		 (unsigned int)cfg_len, cfgf) != cfg_len)
        {
            WarningMessage("DXDragSource:fread Drag conversion failed");
	    if (netf) fclose(netf);
	    if (cfgf) fclose(cfgf);
            return FALSE;
        }
        fclose(cfgf);
	unlink (cfgfilename);
    }

    fclose(netf);
    *value = buf;

    // Clean up the tmp files
    unlink (netfilename);

    return TRUE;
}

boolean DXDragSource::createNetFiles (Network *net, FILE *netf, char *cfgfile)
{
    net->printNetwork (netf, this->printType);
    net->cfgPrintNetwork (cfgfile, this->printType);
    return TRUE;
}
