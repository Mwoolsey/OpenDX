/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dxmods/unpart.h,v 1.4 2000/08/24 20:04:54 davidt Exp $
 */

#include <dxconfig.h>

#ifndef	_UNPART_H
#define	_UNPART_H

#include <dx/dx.h>

#ifndef MAXDIMS
#define	MAXDIMS		32
#endif

#ifndef TRUE
#define	TRUE		1
#define	FALSE		0
#endif

typedef enum
{
    DEP_OTHER,
    DEP_CONNECTIONS,
    DEP_POSITIONS
} DataDep;

typedef struct
{
    Object		obj;			/* the object itself	*/
    Class		class;			/* FIELD/COMPOSITEFIELD	*/
    int			members;		/* if CF # of members	*/
    Type		type;			/* Data type		*/
    Category		cat;			/* Data category	*/
    int			epi;			/* elements per item	*/
    Array		data;			/* components		*/
    Array		con;
    Array		pos;
    int			ndims;			/* dims in conn space	*/
    DataDep		dep;			/* data dep on		*/
    int			origin[MAXDIMS];	/* offset from origin	*/
    int			counts[MAXDIMS];	/* count in each dim	*/
} FieldInfo;

Error _dxfGetFieldInformation (FieldInfo *info);
Error _dxfCoalesceFieldElement (FieldInfo *src, FieldInfo *dst,
			    int element, int me, int total);
Error _dxfExtractFieldElement (FieldInfo *src, FieldInfo *dst,
			   int element, int me, int total);

#endif	/* _UNPART_H */
