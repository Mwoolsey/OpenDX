/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dxmods/import.h,v 1.5 2000/08/24 20:04:37 davidt Exp $
 */

#include <dxconfig.h>

#ifndef _IMPORT_H_
#define _IMPORT_H_

/* 
 * input parameter list which each function must accept:
 *
 *  the parms are:
 *    pointer to the filename
 *    null terminated string list of fields to import
 *    pointer to the format string
 *    optionally, start, end and deltas for series objects
 */

struct parmlist {
    char *filename;
    char **fieldlist;
    char *format;
    int *startframe;
    int *endframe;
    int *deltaframe;
};


/* data-format file search returns */
typedef enum
{
    IMPORT_STAT_FOUND,
    IMPORT_STAT_NOT_FOUND,
    IMPORT_STAT_ERROR
} ImportStatReturn;

/* 
 * table of entry points for each format.
 *
 *  for each type of file to be imported, three things must be supplied:
 *   a function to determine if this is a valid file of this type
 *   a function to actually import the data and return it
 *   a list of format strings which indicate this type of file
 * 
 *  if the autotype function is null, the extension must be given for import
 *   to process this type of file.
 *
 */
struct import_table {
    ImportStatReturn (*autotype)(struct parmlist *);
    Object (*readin)(struct parmlist *);
    char *formatlist[10];
};

/* the actual table */
extern struct import_table _dxdImportTable[];


/*
 * useful utility routines for dealing with binary data and machines
 * with conflicting byte orders.
 */

/* these use the (destination, source, ... ) parm order */
#define SWAB(x,y,z)  _dxfByteSwap((void *)x, (void*)y, (int)(z)/2, TYPE_SHORT)
#define SWAW(x,y,z)  _dxfByteSwap((void *)x, (void*)y, (int)(z)/4, TYPE_INT)
#define SWAD(x,y,z)  _dxfByteSwap((void *)x, (void*)y, (int)(z)/8, TYPE_DOUBLE)

/* Byte order */
typedef enum {
    BO_UNKNOWN = 1,
    BO_MSB = 2,
    BO_LSB = 3
} ByteOrder;

Error     _dxfByteSwap(void *, void*, int, Type);
ByteOrder _dxfLocalByteOrder(void);

ImportStatReturn  _dxftry_ncdf(struct parmlist *p);
ImportStatReturn  _dxftry_hdf(struct parmlist *p);
ImportStatReturn  _dxftry_dx(struct parmlist *p);
ImportStatReturn  _dxftry_cdf(struct parmlist *p);
ImportStatReturn  _dxftry_bin(struct parmlist *p);
ImportStatReturn  _dxftry_wv(struct parmlist *p);

Object _dxfget_ncdf(struct parmlist *p);
Object _dxfget_hdf(struct parmlist *p);
Object _dxfget_dx(struct parmlist *p);
Object _dxfget_cdf(struct parmlist *p);
Object _dxfget_bin(struct parmlist *p);
Object _dxfget_wv(struct parmlist *p);
Object _dxfget_cm(struct parmlist *p);

/* from import_ncdf.c */
ImportStatReturn _dxfstat_netcdf_file(char *);

/* from import_hdf.c */
ImportStatReturn _dxfstat_hdf(char *);
int   _dxfget_hdfcount(char *);
int   _dxfwhich_hdf(char *, char *);

/* from import_cdf.c */
ImportStatReturn _dxfstat_cdf(char *);


#define LOCAL_BYTEORDER   _dxfLocalByteOrder()

#endif /* _IMPORT_H_ */
