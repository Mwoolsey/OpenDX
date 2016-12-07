/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dxmods/impCDF.h,v 1.4 2000/08/24 20:04:36 davidt Exp $
 */

#include <dxconfig.h>

#ifndef _IMPCDF_H_
#define _IMPCDF_H_

struct infocdf
{
  CDFid  id;
  long   numDims;
  long   dimSizes[CDF_MAX_DIMS];
  long   encoding;
  long   majority;
  long   maxRec;
  long   numVars;
  long   numAttrs;
  int    rotate[CDF_MAX_DIMS];
};
typedef struct infocdf *Infocdf;


struct infovar
{
  CDFid  id;
  long   varNum;
  char   name[CDF_VAR_NAME_LEN+1];
  char   conn[CDF_VAR_NAME_LEN+1];
  long   dataType;
  long   numbytes;
  Type   dxtype;
  long   numElements;
  long   recVariance;
  long   dimVariances[CDF_MAX_DIMS];
  int    strframe;
  int    endframe;
  int    dltframe;
  int    dataDims;
  int    shape[CDF_MAX_DIMS];
  int    size;    /* number of data per time series */
  long   idx[CDF_MAX_DIMS];
  long   cnt[CDF_MAX_DIMS];
  long   intval[CDF_MAX_DIMS];
  struct infovar *next;
};
typedef struct infovar *Infovar;

#define  SERPOSCOMP     0
#define  POSITIONCOMP  1
#define  DATACOMP      2
#define  UNKNOWNCOMP   3

#endif /* _IMPCDF_H_ */
