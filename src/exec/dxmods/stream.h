/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dxmods/stream.h,v 1.7 2001/04/17 15:39:19 gda Exp $:
 */

#ifndef _STREAM_H_
#define _STREAM_H_

#define DEFAULT_C	0.1

typedef struct instanceVars *InstanceVars;
typedef struct vectorPart   *VectorPart;
typedef struct vectorGrp    *VectorGrp;
typedef struct vectorField  *VectorField;

struct instanceVars
{
    VectorGrp	currentVectorGrp;
    Object	currentPartition;
    int		isRegular;
};

struct vectorPart
{
    Field  	  field;
    int           dependency;       /* data dependency: pos or con       */
    float	  min[3];
    float	  max[3];
    Array	  gArray;
    unsigned short *ghosts;
};

Error _dxfInitVectorPart(VectorPart, Field);
 
#define DEP_ON_POSITIONS        0x01
#define DEP_ON_CONNECTIONS      0x02

struct vectorField
{
    VectorGrp	 current;
    VectorGrp	 *members;
    int		 nmembers;
    int		 nDim;
};

#define POINT_TYPE	double
#define VECTOR_TYPE	double

struct vectorGrp
{
    InstanceVars (*NewInstanceVars)(VectorGrp);
    Error        (*FreeInstanceVars)(InstanceVars);
    int          (*FindElement)(InstanceVars, POINT_TYPE *);
    int          (*FindMultiGridContinuation)(InstanceVars, POINT_TYPE *);
    Error        (*Interpolate)(InstanceVars, POINT_TYPE *, VECTOR_TYPE *);
    Error        (*StepTime)(InstanceVars, double, VECTOR_TYPE *, double *);
    Error        (*FindBoundary)(InstanceVars, 
					POINT_TYPE *, VECTOR_TYPE *, double *);
    int          (*Neighbor)(InstanceVars, VECTOR_TYPE *);
    Error        (*CurlMap)(VectorGrp, MultiGrid);
    int          (*Weights)(InstanceVars, POINT_TYPE *);
    int          (*FaceWeights)(InstanceVars, POINT_TYPE *);
    Error        (*Delete)(VectorGrp);
    Error        (*Reset)(VectorGrp);
    Error        (*Walk)(InstanceVars, POINT_TYPE *, VECTOR_TYPE*, POINT_TYPE *);
    int          (*Ghost)(InstanceVars, POINT_TYPE *);
    int          (*ClampToBoundingBox)(InstanceVars, POINT_TYPE *);

    int          n, nDim, multigrid;
    VectorPart   *p;
};

#define WALK_NOT_FOUND	0
#define WALK_EXIT	1
#define WALK_FOUND	2
#define WALK_ERROR	3


#define STREAM_BUF_POINTS	32

struct stream
{
    float *points;
    float *vectors;
    float *time;
    Array pArray;
    Array vArray;
    Array tArray;
    int   nDim;
    int   bufKnt, arrayKnt;
};


typedef struct stream *Stream;

Error      _dxfMinMaxBox(Field, float *, float *);
int        _dxfIsInBox(VectorPart, POINT_TYPE *, int, int);

/* from _irregstream.c */
VectorGrp _dxfIrreg_InitVectorGrp(Object, char *);

/* from _regstream.c */
VectorGrp    _dxfReg_InitVectorGrp(Object, char *);


#endif /* _STREAM_H_ */
