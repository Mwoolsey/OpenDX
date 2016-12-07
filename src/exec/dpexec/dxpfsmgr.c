/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>



#include <dx/dx.h>

#include "dxpfsmgr.h"

#ifndef DXD_HAS_LIBIOP

Error _dxf_pfsmgr(int argc, char **argv)
{
    DXSetError(ERROR_DATA_INVALID, "#8200");
    return ERROR;
}

#else  /* has libiop */

#include <sys/types.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include <iop/pfs.h>        /* partition file system */

#define DXPARTITION  "dx:"
#define pfsname(x)   strchr(x, ':')

static Error dxdf();
static Error dxrm(char *dataset);
static Error dxlist(char *partname);


/*
 * entry point for dx interface to pfs manager calls.  inputs are argc, argv
 *  command line arguments.  argv[0] is NOT "pfsmgr".
 */

Error _dxf_pfsmgr(int argc, char **argv)
{
    int rc;
    char *partname;

    /* valid commands:  df                (disk free space for all partitions)
     *                  ls part:          (ls -l for that partition)
     *                  rm part:dataset   (remove - no wildcards allowed)
     *
     * default is to list the dx: partition
     */
    if ((partname = getenv("PFSDRIVE")) == NULL)
	partname = DXPARTITION;
    
    if (argc > 0) {
	
	if (!strcmp(argv[0], "ls")) {
	    if (argc > 1)
		rc = dxlist(argv[1]);
	    else
		rc = dxlist(partname);
	    goto done;
	}

	else if (!strcmp(argv[0], "df")) {
	    rc = dxdf();
	    goto done;
	}

	else if (!strcmp(argv[0], "rm") ||
		 !strcmp(argv[0], "del") ||
		 !strcmp(argv[0], "delete")) {
	    if (argc <= 1) {
		DXSetError(ERROR_BAD_PARAMETER, "#8210", argv[0]);
		rc = ERROR;
		goto done;
	    }
	    rc = dxrm(argv[1]);
	    goto done;
	}
	
	else {
	    DXSetError(ERROR_BAD_PARAMETER, "#8220");
	    rc = ERROR;
	    goto done;
	}
    }

    else
	rc = dxlist(partname);

  done:
    if (rc == OK)
	return OK;

#if 1   /* temp - remove when exec prints error returns from here */
    DXPrintError("pfs");
#endif
    return rc;
}


/* df (disk freespace) command
 */
static Error dxdf()
{
    char dname[PFS_NAME_LEN];
    int rc, i, j;
    u_int tsize, fsize;
    
    /* is there a config file for the array disk?  with partitions? */
    i = pfs_drive_count();
    if (i <= 0) {
	DXSetError(ERROR_DATA_INVALID, "#8230");
	return ERROR;
    }
    
    DXBeginLongMessage();

    /* make sure at least one of the defined drives is valid */
    for(j = 0; j < i; j++) {
	rc = pfs_drive_list(j, dname);
	if (rc < 0)
	    continue;

	strcat(dname, ":");
	rc = pfs_size(dname, &tsize);
	if (rc < 0)
	    continue;
       
	rc = pfs_free(dname, &fsize);
	if (rc < 0)
	    continue;
       
        DXMessage("%-20s %5d total blocks, %5d blocks free\n",dname,tsize,fsize);

    }
    
    DXEndLongMessage();

    return OK;
}    

/* ls (list) a partition
 */
Error dxlist(char *partname)
{
    int i, n;
    char buf[PFS_NAME_LEN];
    char *gm_buf = NULL;
    time_t t;
    pfs_stat_t ps;

    n = pfs_count(partname);
    if (n <= 0) {
	DXSetError(ERROR_DATA_INVALID, "#8240", partname);
	return ERROR;
    }

    gm_buf = (char *)DXAllocate(n * PFS_NAME_LEN);
    if (!gm_buf)
	return ERROR;

    if (pfs_list(partname, gm_buf, 0, &n) < 0) {
	DXSetError(ERROR_DATA_INVALID, pfs_errmsg(pfs_errno));
	return ERROR;
    }

    strcpy(buf, partname);

    DXBeginLongMessage();

    for(i=0; i<n; i++) {

	strcpy(buf+strlen(partname), gm_buf + i * PFS_NAME_LEN);

	if (pfs_stat(buf, &ps) < 0) {
	    DXSetError(ERROR_DATA_INVALID, pfs_errmsg(pfs_errno));
	    return ERROR;
	}

	t = (time_t)ps.ps_dat_m;
        DXMessage("%-25s %5d 64Kblks, modified %s", 
	          gm_buf + i * PFS_NAME_LEN, ps.ps_blk, ctime(&t));
    
    }

    DXEndLongMessage();

    DXFree((Pointer) gm_buf);
    return OK;
}

/* rm (remove) command
 */
static Error dxrm(char *dataset)
{
    int rc, i;
    pfs_stat_t ps;
    
    
    /* no wildcards allowed, because there is no confirmation message - 
     *  are you really sure you want to delete *?
     */
    if (strchr(dataset, '*') || strchr(dataset, '?')) {
	DXSetError(ERROR_DATA_INVALID, "#8250");
	return ERROR;
    }

    if (pfs_stat(dataset, &ps) < 0) {
	DXSetError(ERROR_DATA_INVALID, pfs_errmsg(pfs_errno));
	return ERROR;
    }

#if 0
    DXMessage("would be calling pfs_delete(%s) here", dataset);
#else
    if (pfs_delete(dataset) < 0) {
	DXSetError(ERROR_DATA_INVALID, pfs_errmsg(pfs_errno));
	return ERROR;
    }
#endif

    return OK;
}    


#endif   /* DXD_HAS_LIBIOP */
