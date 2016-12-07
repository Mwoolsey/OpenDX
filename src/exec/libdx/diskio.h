/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>

#ifndef _DISKIO_H_
#define _DISKIO_H_

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

/* disk constants */

#define ONEK	 	(1024)
#define BITS_BYTE	8

#if DXD_HAS_LIBIOP
#define ONEBLK    	(ONEK * 64)	/* array disk minimum block size */
#else
#define ONEBLK    	(512)		/* ordinary disk min block size */
#endif

#define HALFK           (512)           /* header blocksize for socket io */


/* in the following macros, "y" MUST BE A POWER OF 2.
 *
 * the macros do the following:
 *  given "x" bytes and a blocksize of "y", return the number of blocks,
 *    including any partially filled blocks.
 *  given "x" bytes and a blocksize of "y", return the number of whole blocks,
 *    ignoring any partially filled blocks.
 *  given "x" bytes and a blocksize of "y", return the number of bytes in the
 *    partial last block.
 *  given "x" bytes and a blocksize of "y", return the number of bytes when
 *    "x" is rounded up to the next multiple of "y".
 */

#define PARTIAL_BLOCKS(x, y)	(((ulong)(x) + (y)-1) / (y))
#define WHOLE_BLOCKS(x, y)	((ulong)(x) / (y))
#define LEFTOVER_BYTES(x, y)	((ulong)(x) & ((y)-1))
#define ROUNDUP_BYTES(x, y)	(((ulong)(x) + (y)-1) & ~((y)-1))


/* OBSOLETE!  this is now set in rwobject.c, and stored in the header
 *  of the file, so it can be tuned on a per-file basis.
 *
 * separation between array disk file header and body sections.
 */
#define HEADERCUTOFF	(4 * ONEK)


/* prototypes.  the routines are defined in fileio.c
 */

Error _dxf_initdisk();
Error _dxf_exitdisk();
Error _dxffile_open(char *name, int rw);
Error _dxfsock_open(char *name, int fd);
Error _dxffile_add(char *name, u_int nblocks);
Error _dxffile_read(char *name, int offset, int count, char *addr, int bytes);
Error _dxffile_write(char *name, int offset, int count, char *addr, int bytes);
Error _dxffile_close(char *name);
Error _dxfsock_close(char *name);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif /* _DISKIO_H_ */
