/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>

#include <string.h>
#include <dx/dx.h>

#include "diskio.h"

#if defined(HAVE_ERRNO_H)
#include <errno.h>
#endif

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#if defined(HAVE_IO_H)
#include <io.h>
#endif

#if DXD_HAS_LIBIOP
#include <iop/pfs.h>        /* partition file system */
#include <iop/mov.h>        /* movie library support */

static int _dxfiobuf_allocate(int nblks, Pointer *base, Pointer *mem);

static int partial_read(char *name, uint addr, uint offset, uint cnt, 
			uint bytes);
static int partial_write(char *name, uint addr, uint offset, uint cnt, 
			 uint bytes);

#endif

/* other prototypes (the rest are in diskio.h) */
static Error movie_init();
static Error file_init();
static Error file_exit();
static int name_to_fd(char *name, int *socket);
static int fd_socketsize(int fd, int *rdsize, int *wrsize);



/* if not using array disk, this data struct mimics the same calling
 *  sequence (e.g. read/write by name), using normal unix files.
 */
#include <fcntl.h>

#define NSLOTS 16     /* was 32 */
#define SOCK_BUFSIZE 32768

/* table of names to file descriptors.
 *  unused slots have -1 as the file descriptor
 *  the fname length is 256 to match the pfsmgr limit
 *  socket should be set if the fd is a socket where you aren't able
 *   to preallocate space, you can't seek back and forth, and there is
 *   a limit to the max number of bytes you can read/write at once.
 */
static struct openfiles {
    char fname[256];
    int fd;
    int socket;
    int sndbytes;
    int rcvbytes;
} *of;

static int name_to_fd(char *name, int *socket)
{
    int i;

    for (i=0; i<NSLOTS; i++) {
	if (of[i].fd < 0)
	    continue;

	if (!strcmp(of[i].fname, name)) {
	    if (socket)
		*socket = of[i].socket;

	    return of[i].fd;
	}
    }
    return -1;
}

static int fd_socketsize(int fd, int *rdsize, int *wrsize)
{
    int i;

    for (i=0; i<NSLOTS; i++) {
	if (of[i].fd != fd)
	    continue;

	if (rdsize)
	    *rdsize = of[i].rcvbytes;
	if (wrsize)
	    *wrsize = of[i].sndbytes;

	return of[i].socket;
    }
    return -1;
}

#define pfsname(x)   strchr(x, ':')


/*
 * init code for disk routines - called by init.c in libsvs or by executive
 */

Error _dxf_initdisk()
{
    int rc;

    if ((rc = file_init()) != OK)
	return rc;

    if ((rc = movie_init()) != OK)
	return rc;

    return OK;
}


/*
 * exit code for disk routines - called by executive on cleanup
 */

Error _dxf_exitdisk()
{
    return file_exit();
}



static Error file_init()
{
    int i;

#if DXD_HAS_LIBIOP
    char dname[256];  /* max drive name - look for right #define */
    int rc;
    uint size;

    /* is there a config file for the array disk?  with partitions? 
     * you get no error message if no drives are defined.
     */
    i = pfs_drive_count();
    if (i <= 0)
	goto endarray;

    /* make sure at least one of the defined drives is valid */
    while (--i >= 0) {
	rc = pfs_drive_list(i, dname);
	if (rc < 0)
	    continue;
	strcat(dname, ":");
	rc = pfs_free(dname, &size);
	if (rc < 0)
	    continue;
	
	/* valid pfs partition */
	break;
    }
   
    /* you get here if there are drives defined but none of them are valid.
     */
    if (i < 0)
	DXUIMessage("ERROR", "Array disk not available; execution continues");
    
  endarray:
#endif
    of = (struct openfiles *)DXAllocateZero(sizeof(struct openfiles) * NSLOTS);
    if (!of)
	return ERROR;
    
    for (i=0; i<NSLOTS; i++)
	of[i].fd = -1;
    
    return OK;
}

static Error file_exit()
{
    int i;

    for (i=0; i<NSLOTS; i++)
	of[i].fd = -1;

    return OK;
}

static Error movie_init()
{
#if DXD_HAS_LIBIOP
    int rc;
    char *movbuf;        /* allocated and held for all time by mov lib */
                         /* must execute on same processor as mov calls */

    if (!(movbuf = DXAllocate(MOV_BLK_SIZE)))
	return ERROR;

    rc = mov_init(movbuf);
    if (rc) {
	DXSetError(ERROR_DATA_INVALID, "movie library: %s", mov_errmsg(rc));
	return ERROR;
    }
#endif

    return OK;
}

Error _dxffile_open(char *name, int rw)
{
	int i, fd=-1;

	if (!name) {
		DXSetError(ERROR_BAD_PARAMETER, "no filename");
		return ERROR;
	}

#if DXD_HAS_LIBIOP
	{
		int rc;
		/* array disk, new pfs */
		if (pfsname(name)) {
			pfs_stat_t ps;

			rc = pfs_stat(name, &ps);
			if (rc < 0) {
				/* if read only, don't create */
				if (rw == 0) {
					DXSetError(ERROR_DATA_INVALID, "can't open file '%s'", name);
					return ERROR;
				}
				/* must be write or read/write, go ahead and create */
				rc = pfs_create(name, 0);
				if (rc < 0) {
					DXSetError(ERROR_DATA_INVALID, pfs_errmsg(rc));
					return ERROR;
				}
			}

			return OK;
		}
	}
#endif

	switch(rw) {
	case 0:/* read only */
		fd = open(name, O_RDONLY);
		if (fd < 0) {
			DXSetError(ERROR_DATA_INVALID, "can't open file '%s'", name);
			return ERROR;
		}
		break;
	case 1:/* write only */
	case 2:/* read/write */
		fd = open(name, O_RDWR);
		if (fd < 0) {
			fd = open(name, O_WRONLY | O_CREAT);
			if (fd < 0) {
				DXSetError(ERROR_DATA_INVALID, 
				"can't open/create file '%s'", name);
				return ERROR;
			}
		}
		break;
	}

	/* look for empty slot and put fd into table */
	for (i=0; i<NSLOTS; i++)
		if (of[i].fd == -1)
			break;

	if (i == NSLOTS) {
		DXSetError(ERROR_DATA_INVALID, "too many datasets open");
		return ERROR;
	}

	of[i].fd = fd;
	of[i].socket = 0;
	of[i].sndbytes = of[i].rcvbytes = 0;
	strcpy(of[i].fname, name);
	return OK;

}

Error _dxfsock_open(char *name, int fd)
{
    int i;
    socklen_t len = sizeof(int);

    if (!name) {
	DXSetError(ERROR_BAD_PARAMETER, "no socket name");
	return ERROR;
    }

    /* look for empty slot and put fd into table */
    for (i=0; i<NSLOTS; i++)
	if (of[i].fd == -1)
	    break;

    if (i == NSLOTS) {
	DXSetError(ERROR_DATA_INVALID, "too many datasets open");
	return ERROR;
    }

    of[i].fd = fd;
    of[i].socket = 1;
    of[i].sndbytes = SOCK_BUFSIZE;
    of[i].rcvbytes = SOCK_BUFSIZE;
    /* on the solaris getsockopt returns 0 as a bufsize. Let's pick */
    /* a reasonable size for all architectures. If the size is bigger */
    /* then the amount allow by a specific architecture getsockopt    */
    /* will return a smaller size. */
    setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (char *)&(of[i].sndbytes), len);
    setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (char *)&(of[i].rcvbytes), len);
    getsockopt(fd, SOL_SOCKET, SO_SNDBUF, (char *)&(of[i].sndbytes), &len);
    getsockopt(fd, SOL_SOCKET, SO_RCVBUF, (char *)&(of[i].rcvbytes), &len);
    strcpy(of[i].fname, name);
    return OK;

}

Error _dxffile_add(char *name, uint nblocks)
{
    int fd;
    int issocket = 0;

    if (!name) {
	DXSetError(ERROR_BAD_PARAMETER, "no filename");
	return ERROR;
    }

#if DXD_HAS_LIBIOP
    if (pfsname(name)) {
	rc = pfs_resize(name, nblocks);
	if (rc < 0) {
	    DXSetError(ERROR_DATA_INVALID, pfs_errmsg(rc));
	    pfs_delete(name);
	    return ERROR;
	}
	return OK;
    }
#endif

    /* find name->fd from table */
    fd = name_to_fd(name, &issocket);
    if (fd < 0) {
	DXSetError(ERROR_BAD_PARAMETER, "can't open dataset '%s'", name);
	return ERROR;
    }

    if (!issocket && nblocks > 0) {
	lseek(fd, nblocks*ONEBLK - sizeof(int), 0);
	if(write(fd, &fd, sizeof(int)) == -1) {
            DXSetError(ERROR_INTERNAL, "can't write to dataset '%s'", name);
            return ERROR;
        }
	lseek(fd, 0, 0);
    }

    return OK;

}


Error _dxffile_read(char *name, int offset, int count, char *addr, int bytes)
{
    int cnt, fd, rc, left;
    int issocket = 0;
    int maxio;
    int oneblock;
    
    if (!name) {
	DXSetError(ERROR_BAD_PARAMETER, "no filename");
	return ERROR;
    }
    
    if (!addr) {
	DXSetError(ERROR_BAD_PARAMETER, "bad memory address 0");
	return ERROR;
    }
    
    
#if DXD_HAS_LIBIOP
    if (pfsname(name)) {
	/* check alignment of addr */
	if (LEFTOVER_BYTES(addr, ONEK)) {
	    DXSetError(ERROR_DATA_INVALID, "buffer must be 1K alligned");
	    return ERROR;
	}
	/* check for partial blocks */
	if (bytes != 0)
	    /* copy the partial last block into a complete block and read */
	    rc = partial_read(name, (uint)addr, (uint)offset, (uint)count, 
								(uint)bytes);
	else
	    rc = pfs_read(name, (void *)addr, (uint)offset, (uint)count);

	if (rc < 0) {
	    DXSetError(ERROR_DATA_INVALID, pfs_errmsg(rc));
	    return ERROR;
	}
	    
	return OK;
    }
#endif

    /* find name->fd from table */
    fd = name_to_fd(name, &issocket);
    if (fd < 0) {
	DXSetError(ERROR_BAD_PARAMETER, "can't open dataset '%s'", name);
	return ERROR;
    }

    if(issocket)
        oneblock = HALFK;
    else
        oneblock = ONEBLK;

    cnt = count*oneblock;
    if (bytes != 0)
	cnt += bytes - oneblock;

    if (!issocket) {
	lseek(fd, offset*oneblock, 0);
	maxio = cnt;
    }
    else {
	if (fd_socketsize(fd, &maxio, NULL) != 1)
	    return ERROR;
    }

    left = cnt;
    while (left > 0) {
	cnt = left > maxio ? maxio : left;
	if ((rc = read(fd, addr, cnt)) <= 0) {
	    DXSetError(ERROR_DATA_INVALID, "can't read dataset '%s'", name);
	    return ERROR;
	}
	left -= rc;
	addr += rc;
    }

    return OK;

}

Error _dxffile_write(char *name, int offset, int count, char *addr, int bytes)
{
    int cnt, fd, rc, left;
    int issocket = 0;
    int maxio;
    int oneblock;

    if (!name) {
	DXSetError(ERROR_BAD_PARAMETER, "no filename");
	return ERROR;
    }

    if (!addr) {
	DXSetError(ERROR_BAD_PARAMETER, "bad memory address 0");
	return ERROR;
    }


#if DXD_HAS_LIBIOP
    if (pfsname(name)) {
	char *base, *align;
	int i, size;
	/* check alignment of addr */
	if (LEFTOVER_BYTES(addr, ONEK)) {
	    size = count * 64 * 1024 + bytes;
	    base = (char *)DXAllocate(size + 1024);
	    align = (char *)(((int)base + 1023) & ~1023); 
	    bcopy(addr, align, size);
	    
	    if (bytes != 0) 
	        rc = partial_write(name, (uint)align, (uint)offset, 
					(uint)count, (uint)bytes);
	    else
	        rc = pfs_write(name, (void *)align, (uint)offset, (uint)count);
	    DXFree((Pointer)base);
	    if (rc < 0) {
	        DXSetError(ERROR_DATA_INVALID, pfs_errmsg(rc));
	        return ERROR;
	    }
	    return OK;
	}
	/* check for partial last block */
	if (bytes != 0) 
	    rc = partial_write(name, (uint)addr, (uint)offset, (uint)count,
								(uint)bytes);
	else
	    rc = pfs_write(name, (void *)addr, (uint)offset, (uint)count);
	
	if (rc < 0) {
	    DXSetError(ERROR_DATA_INVALID, pfs_errmsg(rc));
	    return ERROR;
	}
	
	return OK;
    }
#endif

    /* find name->fd from table */
    fd = name_to_fd(name, &issocket);
    if (fd < 0) {
	DXSetError(ERROR_BAD_PARAMETER, "can't open dataset '%s'", name);
	return ERROR;
    }

    if(issocket)
        oneblock = HALFK;
    else 
        oneblock = ONEBLK;

    cnt = count*oneblock;
    if (bytes != 0)
	cnt += bytes - oneblock;

    if (!issocket) {
	lseek(fd, offset*oneblock, 0);
	maxio = cnt;
    }
    else {
	if (fd_socketsize(fd, NULL, &maxio) != 1)
	    return ERROR;
    }

    left = cnt;
    while (left > 0) {
	cnt = left > maxio ? maxio : left;
	if ((rc = write(fd, addr, cnt)) <= 0) {
	    DXSetError(ERROR_BAD_PARAMETER, "can't write dataset '%s'", name);
	    return ERROR;
	}
	left -= rc;
	addr += rc;
    }

    return OK;

}

Error _dxffile_close(char *name)
{
    int i;

    if (!name) {
	DXSetError(ERROR_BAD_PARAMETER, "no filename");
	return ERROR;
    }


#if DXD_HAS_LIBIOP
    if (pfsname(name))
	return OK;
#endif

    /* find name->fd in table */
    for (i=0; i<NSLOTS; i++) {
	if (of[i].fd < 0)
	    continue;

	if (!strcmp(of[i].fname, name)) {
	    close(of[i].fd);
	    of[i].fd = -1;
	    return OK;
	}

    }

    DXSetError(ERROR_BAD_PARAMETER, "can't close dataset '%s'", name);
    return ERROR;

}

/* in spite of the name, it doesn't close the socket; it just releases 
 *  the slot in the table.
 */
Error _dxfsock_close(char *name)
{
    int i;

    if (!name) {
	DXSetError(ERROR_BAD_PARAMETER, "no socket name");
	return ERROR;
    }

    /* find name->fd in table */
    for (i=0; i<NSLOTS; i++) {
	if (of[i].fd < 0)
	    continue;

	if (!strcmp(of[i].fname, name)) {
	    of[i].fd = -1;
	    return OK;
	}

    }

    DXSetError(ERROR_BAD_PARAMETER, "can't close dataset '%s'", name);
    return ERROR;

}



#if DXD_HAS_LIBIOP
/* handle reads & writes with partial last blocks */
static int 
partial_read(char *name, uint addr, uint offset, uint cnt, uint bytes)
{
    int rc;
    Pointer frag_base = NULL, frag_mem = NULL;
	
    if (bytes == 0)
	return pfs_read(name, (void *)addr, offset, cnt);

    if (_dxfiobuf_allocate(1, &frag_base, &frag_mem) == 0)
	return -1;

    cnt--;
    if (cnt > 0) {
	rc = pfs_read(name, (void *)addr, offset, cnt);
	if (rc < 0)
	    return -1;

	addr += cnt * ONEBLK;
	offset += cnt;
    }

    rc = pfs_read(name, (void *)frag_mem, offset, 1);
    if (rc < 0) {
	DXFree(frag_base);
	return rc;
    }

    memcpy(addr, frag_mem, bytes);
    DXFree(frag_base);
    return 0;

}

static int 
partial_write(char *name, uint addr, uint offset, uint cnt, uint bytes)
{
    int rc;
    Pointer frag_base = NULL, frag_mem = NULL;
	
    if (bytes == 0)
	return pfs_write(name, (void *)addr, offset, cnt);

    if (_dxfiobuf_allocate(1, &frag_base, &frag_mem) == 0)
	return -1;

    cnt--;
    if (cnt > 0) {
	rc = pfs_write(name, (void *)addr, offset, cnt);
	if (rc < 0)
	    return -1;

	addr += cnt * ONEBLK;
	offset += cnt;
    }

    memcpy(frag_mem, addr, bytes);
    rc = pfs_write(name, (void *)frag_mem, offset, 1);

    DXFree(frag_base);
    return rc;

}


/* allocates the requested number of 64K blocks.  base is the value
 *  to be DXFree'd when finished; mem is the pointer aligned on an even
 *  1K memory boundary.
 */
static int 
_dxfiobuf_allocate(int blocks, Pointer *base, Pointer *mem)
{
    *base = DXAllocate(blocks * ONEBLK + ONEK);
    *mem = (Pointer)ROUNDUP_BYTES(*base, ONEK);
    
    return (*base ? OK : ERROR);
}


#endif  /*DXD_HAS_LIBIOP*/


/* from here down, the routines are unused, as far as i know */

#if 0

/* unused, as far as i know 
 */
static Error 
file_info(char *name, struct dinfo *dip)
{
    if (!name) {
	DXSetError(ERROR_BAD_PARAMETER, "no filename");
	return ERROR;
    }

    if (!dip) {
	DXSetError(ERROR_BAD_PARAMETER, "bad info address 0");
	return ERROR;
    }

#if DXD_HAS_LIBIOP
    if (pfsname(name)) {
	pfs_stat_t ps;

	return ((pfs_stat(name, &ps) == 0) ? OK : ERROR);
    }
#endif

    /* do a stat on the filename, and fill in the dip struct */
    return OK;

}

/* unused, as far as i know 
 */
static Error 
file_list(int i, char **name, struct dinfo *dip)
{

    if (!name) {
	DXSetError(ERROR_BAD_PARAMETER, "no filename");
	return ERROR;
    }

    if (!dip) {
	DXSetError(ERROR_BAD_PARAMETER, "bad info address 0");
	return ERROR;
    }

    if (i < 0) {
	DXSetError(ERROR_BAD_PARAMETER, "bad info number");
	return ERROR;
    }


#if DXD_HAS_LIBIOP
    if (pfsname(*name)) {
	pfs_stat_t ps;
	return ((pfs_stat(*name, &ps) == 0) ? OK : ERROR);
    }
#endif

    /* well?  what here?  stat *.bin and take the Nth one? */
    return OK;

}

/* debug */
static
void file_dirprint()
{
    int i, n;
    char buf[256];
    char *gm_buf = NULL;

#if DXD_HAS_LIBIOP
    n = pfs_count(NULL);
    if (n <= 0)
	return;

    gm_buf = (char *)DXAllocate(n * PFS_NAME_LEN);
    if (!gm_buf)
	return;

    pfs_list(NULL, gm_buf, 0, &n);

    for(i=0; i<n; i++)
	printf("slot %3d: fn %s\n", i, gm_buf + i * PFS_NAME_LEN);

    DXFree((Pointer) gm_buf);
    return;
#endif

    for (i=0; i<NSLOTS; i++) {
	if (of[i].fd < 0)
	    continue;
	
	printf("slot %3d: fd %2d, name '%s'\n", i, of[i].fd, of[i].fname);
    }

    return;
}


#endif    /* if 0 */
