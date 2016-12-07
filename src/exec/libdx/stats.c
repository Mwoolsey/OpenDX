/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>



#include <dx/dx.h>
#include <string.h>
#include <math.h>

/* internal statistics structure.  this is what the "xxx statistics"
 *  component actually contains, but users have to go through the
 *  DXStatistics() call so they never reference it directly, and it's
 *  derived (has a 'der' attribute) so it doesn't get exported, so it is
 *  reasonably safe to change it if necessary.
 */
struct stats {
    double min, max, avg, sigma;   /* the values needed to combine stats */
    double sum, sumsq, counts;     /* from different fields */
    double isvalid;                /* set to 0 if all points invalid */
};

/* to handle composite fields, where you need to know the stats of this
 * one piece by itself, but you also need to know the stats if this is
 * considered as just one part of the whole (it will be different because
 * duplicate points will be ignored in one and not in the other).
 * 
 * the has_cf flag isn't used right now because we can tell the difference
 * between a component which just has stats and one which has both types
 * of stats by the number of items in the array.
 */
struct statcomp {
    struct stats f;     /* for all valid points in the field */
    struct stats cf;    /* for composite fields, stats w/dup points ignored */
    double has_cf;      /* set if the cf stats are valid (unused) */
};

typedef struct stats *Stats;

/* !!before you use this, read the comment following the data[1] line */
struct argblock {
    int len;            /* length of this block including trailing bytes */
    int isparallel;     /* have we created a task group already? */
    Field f;            /* field to work on */
    char *compname;     /* component to compute stats for */
    char *statname;     /* stats component name */
    char data[1];       /* compname and statname point here */
    /* compname and statname point into the bytes directly following
     * this structure, which means you shouldn't try to to declare a
     * a variable of type struct argblock, only type struct argblock *.
     * (with a pointer, you can access the len and component name pointers
     * directly).  all this silliness is necessary to run in parallel, 
     * since all data for the tasks must be in global memory.
     * don't use sizeof() with this struct - use len instead.
     */
};

/* just too long for me to type, sorry */
#define ICH InvalidComponentHandle   

/* this should be in a system-wide header file (or it should be the
 * same number and i'm sure i use a different number in every file).
 */
#define MAXDIM      16

#if 0
static Field Compute_Wrapper(Field f, char *argblock, int argsize);
static CompositeField CFCompute_Wrapper(CompositeField f, 
					char *argblock, int argsize);
#endif
static Error supported(Array);
static Stats Add__Stats(Object, char *, char *, int cfmember, struct stats *sp);
static Error ComputeStats(Pointer);
static Error AccumulateStats(Object, char *, char *, Stats, int *, int);
static Error ConstantData(Array a, double *origin);
static Error RegularData(Array a);
static Error HasInvalid(Field f, char *, ICH *ih);
static Error InvalidName(Field f, char *, char **);
static Error buildblock(char *compname, char *statname, 
			struct argblock **ab, int *len);

/* temp */
static Error ProcessPartsXX(Object o, struct argblock *ab, int len);

/* undocumented entry point in component.c */
extern Object Exists2(Object o, char *name, char *name2);

/* the empty field call in libdx returns yes if there are components
 * but no positions component.  this routine should be able to return
 * the data stats from fields without a position component.
 */
static int DXReallyEmptyField(Field f)
{
    if (DXGetObjectClass((Object)f) != CLASS_FIELD)
	return 0;

    return DXGetEnumeratedComponentValue(f, 0, NULL) == NULL;
}

/* isnan doesn't exist for ibmpvs */
#if defined(ibmpvs)
#define isnan(a) 0
#elsif defined(intelnt) || defined(WIN32)
#define isnan(a) _isnan(a)
#endif

/* welcome to macro heaven. */

#define MAG2D(a) sqrt((double)(a)[0]*(a)[0] + (double)(a)[1]*(a)[1])

#define MAG3D(a) sqrt((double)(a)[0]*(a)[0] + \
		      (double)(a)[1]*(a)[1] + \
		      (double)(a)[2]*(a)[2])


#define DET2D(a) ((double)(a)[0]*(a)[3] - (double)(a)[1]*(a)[2])

#define DET3D(a) ((a)[0] * ((double)(a)[4]*(a)[8] - (double)(a)[5]*(a)[7]) - \
		  (a)[1] * ((double)(a)[3]*(a)[8] - (double)(a)[5]*(a)[6]) + \
		  (a)[2] * ((double)(a)[3]*(a)[7] - (double)(a)[4]*(a)[6]))



    
static double determinant(double *a, int rank)
{
    int i, j, k;
    int sign;
    double sum;
    double *minor, *dp;

    if (rank == 1)
	return a[0];

    /* make space for the minor of the matrix (the matrix with the
     *  i-th row and column removed).
     */
    minor = (double *)DXAllocateLocal(sizeof(double) * (rank-1) * (rank-1));
    if (!minor)
	return 0.0;

    for (i=0, sum=0.0, sign=1; i<rank; i++, sign *= -1) {
	for (j=1, dp=minor; j<rank; j++) 
	    for (k=0; k<rank; k++) {
		if (k == i)
		    continue;
		*dp++ = a[j*rank + k];
	    }

	sum += sign * a[i] * determinant(minor, rank-1);
    }

    DXFree((Pointer)minor);
    return sum;
}



#define scalar_stats(type, tmin, tmax) \
static void                                                \
type ## S_stat( type *dp, int size, struct stats *s, ICH inv) \
{                                                          \
    int i;	                                           \
    double thisval;					   \
							   \
    s->min = tmax; s->max = tmin; s->sum = s->sumsq = 0.0; \
    s->counts = s->isvalid = 0;				   \
							   \
    for(i=0; i<size; i++) {				   \
        if (inv && (DXIsElementInvalidSequential(inv, i))) \
	    continue;					   \
							   \
	thisval = (double)dp[i];			   \
							   \
	if(thisval > s->max) s->max = thisval;		   \
	if(thisval < s->min) s->min = thisval;		   \
							   \
	s->sum += thisval;				   \
	s->sumsq += ((double)thisval * thisval);	   \
	s->counts++;                                       \
    }							   \
							   \
    if (s->counts > 0) {				   \
	s->avg = s->sum / s->counts;	   		   \
        s->isvalid = 1;			   		   \
    }							   \
}							    


#define vect2D_stats(type, tmin, tmax) \
static void 						   \
type ## v2D_stat( type *dp, int size, struct stats *s, ICH inv) \
{							   \
    int i;						   \
    double thisval;					   \
							   \
    s->min = tmax; s->max = tmin; s->sum = s->sumsq = 0.0; \
    s->counts = 0;					   \
							   \
    for(i=0; i<size; i++, dp+=2) {			   \
        if (inv && (DXIsElementInvalidSequential(inv, i))) \
	    continue;					   \
							   \
	thisval = (double)MAG2D(dp);			   \
							   \
	if(thisval > s->max) s->max = thisval;		   \
	if(thisval < s->min) s->min = thisval;		   \
							   \
	s->sum += thisval;				   \
	s->sumsq += ((double)thisval * thisval);	   \
	s->counts++;                                       \
    }                                                      \
							   \
    if (s->counts > 0) {				   \
	s->avg = s->sum / s->counts;	   		   \
        s->isvalid = 1;			   		   \
    }							   \
}

#define vect3D_stats(type, tmin, tmax) \
static void 						   \
type ## v3D_stat( type *dp, int size, struct stats *s, ICH inv) \
{							   \
    int i;						   \
    double thisval;					   \
							   \
    s->min = tmax; s->max = tmin; s->sum = s->sumsq = 0.0; \
    s->counts = 0;					   \
							   \
    for(i=0; i<size; i++, dp+=3) {			   \
        if (inv && (DXIsElementInvalidSequential(inv, i))) \
	    continue;					   \
							   \
	thisval = (double)MAG3D(dp);			   \
							   \
	if(thisval > s->max) s->max = thisval;		   \
	if(thisval < s->min) s->min = thisval;		   \
							   \
	s->sum += thisval;				   \
	s->sumsq += ((double)thisval * thisval);	   \
	s->counts++;                                       \
    }                                                      \
							   \
    if (s->counts > 0) {				   \
	s->avg = s->sum / s->counts;	   		   \
        s->isvalid = 1;			   		   \
    }							   \
}

#define vectND_stats(type, tmin, tmax) \
static void 						   \
type ## vND_stat( type *dp, int size, int rank, struct stats *s, ICH inv) \
{							   \
    int i, j;						   \
    double thisval;					   \
							   \
    s->min = tmax; s->max = tmin; s->sum = s->sumsq = 0.0; \
    s->counts = 0;					   \
							   \
    for(i=0; i<size; i++, dp+=rank) {			   \
        if (inv && (DXIsElementInvalidSequential(inv, i))) \
	    continue;					   \
							   \
        for(j=0, thisval=0; j<rank; j++)		   \
            thisval += (double)dp[j] * dp[j];		   \
							   \
	thisval = sqrt(thisval);			   \
							   \
	if(thisval > s->max) s->max = thisval;		   \
	if(thisval < s->min) s->min = thisval;		   \
							   \
	s->sum += thisval;				   \
	s->sumsq += ((double)thisval * thisval);	   \
	s->counts++;                                       \
    }                                                      \
							   \
    if (s->counts > 0) {				   \
	s->avg = s->sum / s->counts;	   		   \
        s->isvalid = 1;			   		   \
    }							   \
}


#define matr2D_stats(type, tmin, tmax) \
static void 						   \
type ## m2D_stat( type *dp, int size, struct stats *s, ICH inv) \
{							   \
    int i;						   \
    double thisval;					   \
							   \
    s->min = tmax; s->max = tmin; s->sum = s->sumsq = 0.0; \
    s->counts = 0;					   \
							   \
    for(i=0; i<size; i++, dp+=4) {			   \
        if (inv && (DXIsElementInvalidSequential(inv, i))) \
	    continue;					   \
							   \
	thisval = (double)DET2D(dp);			   \
							   \
	if(thisval > s->max) s->max = thisval;		   \
	if(thisval < s->min) s->min = thisval;		   \
							   \
	s->sum += thisval;				   \
	s->sumsq += ((double)thisval * thisval);	   \
	s->counts++;                                       \
    }                                                      \
							   \
    if (s->counts > 0) {				   \
	s->avg = s->sum / s->counts;	   		   \
        s->isvalid = 1;			   		   \
    }							   \
}

#define matr3D_stats(type, tmin, tmax) \
static void 						   \
type ## m3D_stat( type *dp, int size, struct stats *s, ICH inv) \
{							   \
    int i;						   \
    double thisval;					   \
							   \
    s->min = tmax; s->max = tmin; s->sum = s->sumsq = 0.0; \
    s->counts = 0;					   \
							   \
    for(i=0; i<size; i++, dp+=9) {			   \
        if (inv && (DXIsElementInvalidSequential(inv, i))) \
	    continue;					   \
							   \
	thisval = (double)DET3D(dp);			   \
							   \
	if(thisval > s->max) s->max = thisval;		   \
	if(thisval < s->min) s->min = thisval;		   \
							   \
	s->sum += thisval;				   \
	s->sumsq += ((double)thisval * thisval);	   \
	s->counts++;                                       \
    }                                                      \
							   \
    if (s->counts > 0) {				   \
	s->avg = s->sum / s->counts;	   		   \
        s->isvalid = 1;			   		   \
    }							   \
}

#define matrND_stats(type, tmin, tmax) \
static void 						   \
type ## mND_stat( type *dp, int size, int rank, struct stats *s, ICH inv) \
{							   \
    int i;						   \
    double thisval;					   \
							   \
    s->min = tmax; s->max = tmin; s->sum = s->sumsq = 0.0; \
    s->counts = 0;					   \
							   \
    for(i=0; i<size; i++, dp+=rank*rank) {		   \
        if (inv && (DXIsElementInvalidSequential(inv, i))) \
	    continue;					   \
							   \
	thisval = (double) type ## _det(dp, rank);	   \
							   \
	if(thisval > s->max) s->max = thisval;		   \
	if(thisval < s->min) s->min = thisval;		   \
							   \
	s->sum += thisval;				   \
	s->sumsq += ((double)thisval * thisval);	   \
	s->counts++;                                       \
    }                                                      \
							   \
    if (s->counts > 0) {				   \
	s->avg = s->sum / s->counts;	   		   \
        s->isvalid = 1;			   		   \
    }							   \
}

#define regscalar_stats(type, tmin, tmax) \
static void                                                \
type ## RS_stat( type *dp, type *inc, int size, struct stats *s, ICH inv) \
{                                                          \
    int i;	                                           \
    double thisval, startval, incval;			   \
							   \
    s->min = tmax; s->max = tmin; s->sum = s->sumsq = 0.0; \
    s->counts = s->isvalid = 0;				   \
							   \
    startval = (double)*dp;			  	   \
    incval = (double)*inc;			  	   \
    for(i=0; i<size; i++) {				   \
        if (inv && (DXIsElementInvalidSequential(inv, i))) \
	    continue;					   \
							   \
	thisval = startval + i*incval;			   \
							   \
	if(thisval > s->max) s->max = thisval;		   \
	if(thisval < s->min) s->min = thisval;		   \
							   \
	s->sum += thisval;				   \
	s->sumsq += ((double)thisval * thisval);	   \
	s->counts++;                                       \
    }							   \
							   \
    if (s->counts > 0) {				   \
	s->avg = s->sum / s->counts;	   		   \
        s->isvalid = 1;			   		   \
    }							   \
}							    



#define scalar_conv(type)                                   \
static void                                                 \
type ## S_conv( type *dp, int size, float *fp)	            \
{							    \
    int i;						    \
    for(i=0; i<size; i++) *fp++ = (float)(*dp++);	    \
}

#define vect2D_conv(type)                                   \
static void                                                 \
type ## v2D_conv( type *dp, int size, float *fp)            \
{							    \
    int i;						    \
    for(i=0; i<size; i++, dp+=2) *fp++ = (float)MAG2D(dp);  \
}

#define vect3D_conv(type)                                   \
static void                                                 \
type ## v3D_conv( type *dp, int size, float *fp)            \
{							    \
    int i;						    \
    for(i=0; i<size; i++, dp+=3) *fp++ = (float)MAG3D(dp);  \
}

#define vectND_conv(type)                                   \
static void                                                 \
type ## vND_conv( type *dp, int size, int rank, float *fp)  \
{							    \
    int i, j;						    \
    double x;						    \
							    \
    for(i=0; i<size; i++, dp+=rank) {			    \
        for(j=0, x=0; j<rank; j++) x += (double)dp[j]*dp[j]; \
	*fp++ = (float)sqrt(x);				    \
    }							    \
}

#define matr2D_conv(type)                                   \
static void                                                 \
type ## m2D_conv( type *dp, int size, float *fp)            \
{							    \
    int i;   						    \
    for(i=0; i<size; i++, dp+=4) *fp++ = (float)DET2D(dp);  \
}

#define matr3D_conv(type)                                   \
static void                                                 \
type ## m3D_conv( type *dp, int size, float *fp)            \
{							    \
    int i;   						    \
    for(i=0; i<size; i++, dp+=9) *fp++ = (float)DET3D(dp);  \
}

#define matrND_conv(type)                                   \
static void                                                 \
type ## mND_conv( type *dp, int size, int rank, float *fp)  \
{							    \
    int i;   						    \
    for(i=0; i<size; i++, dp+=16) *fp++ = (float) type ## _det(dp, rank); \
}



#define type_det(type)                                      \
static double 						    \
type ## _det( type *a, int rank)			    \
{							    \
    int i, j;						    \
    double *d, *dp;					    \
    double det;						    \
							    \
    d = (double *)DXAllocateLocal(sizeof(double) * rank * rank);   \
    if (!d)						    \
	return 0.0;					    \
							    \
    for (i=0, dp = d; i<rank; i++)			    \
	for (j=0; j<rank; j++)				    \
	    *dp++ = (double)(a[i*rank + j]);		    \
							    \
    det = determinant(d, rank);				    \
							    \
    DXFree((Pointer)d);					    \
    return det;						    \
}



/* 
 * actual functions for each type and shape.  to see what's going on,
 * run this file thru the preprocessor.  if you need to debug any of
 * the macros, edit the .i file and split the lines of the macros you
 * want to step thru when running under the debugger. 
 *
 * for the stats routines, the last two parms are the min and max
 * for that data type.
 */

type_det(ubyte)
type_det(byte)
type_det(ushort)
type_det(short)
type_det(uint)
type_det(int)
type_det(float)
#define double_det  determinant

scalar_stats(ubyte, 0, 255)
regscalar_stats(ubyte, 0, 255)
vect2D_stats(ubyte, 0, 255)
vect3D_stats(ubyte, 0, 255)
vectND_stats(ubyte, 0, 255)
matr2D_stats(ubyte, 0, 255)
matr3D_stats(ubyte, 0, 255)
matrND_stats(ubyte, 0, 255)

scalar_conv(ubyte)
vect2D_conv(ubyte)
vect3D_conv(ubyte)
vectND_conv(ubyte)
matr2D_conv(ubyte)
matr3D_conv(ubyte)
matrND_conv(ubyte)


scalar_stats(byte, -128, 127)
regscalar_stats(byte, -128, 127)
vect2D_stats(byte, -128, 127)
vect3D_stats(byte, -128, 127)
vectND_stats(byte, -128, 127)
matr2D_stats(byte, -128, 127)
matr3D_stats(byte, -128, 127)
matrND_stats(byte, -128, 127)

scalar_conv(byte)
vect2D_conv(byte)
vect3D_conv(byte)
vectND_conv(byte)
matr2D_conv(byte)
matr3D_conv(byte)
matrND_conv(byte)


scalar_stats(ushort, 0, (ushort)(~ 0))
regscalar_stats(ushort, 0, (ushort)(~ 0))
vect2D_stats(ushort, 0, (ushort)(~ 0))
vect3D_stats(ushort, 0, (ushort)(~ 0))
vectND_stats(ushort, 0, (ushort)(~ 0))
matr2D_stats(ushort, 0, (ushort)(~ 0))
matr3D_stats(ushort, 0, (ushort)(~ 0))
matrND_stats(ushort, 0, (ushort)(~ 0))

scalar_conv(ushort)
vect2D_conv(ushort)
vect3D_conv(ushort)
vectND_conv(ushort)
matr2D_conv(ushort)
matr3D_conv(ushort)
matrND_conv(ushort)


scalar_stats(short, -DXD_MAX_INT, DXD_MAX_INT)
regscalar_stats(short, -DXD_MAX_INT, DXD_MAX_INT)
vect2D_stats(short, -DXD_MAX_INT, DXD_MAX_INT)
vect3D_stats(short, -DXD_MAX_INT, DXD_MAX_INT)
vectND_stats(short, -DXD_MAX_INT, DXD_MAX_INT)
matr2D_stats(short, -DXD_MAX_INT, DXD_MAX_INT)
matr3D_stats(short, -DXD_MAX_INT, DXD_MAX_INT)
matrND_stats(short, -DXD_MAX_INT, DXD_MAX_INT)

scalar_conv(short)
vect2D_conv(short)
vect3D_conv(short)
vectND_conv(short)
matr2D_conv(short)
matr3D_conv(short)
matrND_conv(short)


scalar_stats(uint, 0, (uint)(~ 0))
regscalar_stats(uint, 0, (uint)(~ 0))
vect2D_stats(uint, 0, (uint)(~ 0))
vect3D_stats(uint, 0, (uint)(~ 0))
vectND_stats(uint, 0, (uint)(~ 0))
matr2D_stats(uint, 0, (uint)(~ 0))
matr3D_stats(uint, 0, (uint)(~ 0))
matrND_stats(uint, 0, (uint)(~ 0))

scalar_conv(uint)
vect2D_conv(uint)
vect3D_conv(uint)
vectND_conv(uint)
matr2D_conv(uint)
matr3D_conv(uint)
matrND_conv(uint)


scalar_stats(int, -DXD_MAX_INT, DXD_MAX_INT)
regscalar_stats(int, -DXD_MAX_INT, DXD_MAX_INT)
vect2D_stats(int, -DXD_MAX_INT, DXD_MAX_INT)
vect3D_stats(int, -DXD_MAX_INT, DXD_MAX_INT)
vectND_stats(int, -DXD_MAX_INT, DXD_MAX_INT)
matr2D_stats(int, -DXD_MAX_INT, DXD_MAX_INT)
matr3D_stats(int, -DXD_MAX_INT, DXD_MAX_INT)
matrND_stats(int, -DXD_MAX_INT, DXD_MAX_INT)

scalar_conv(int)
vect2D_conv(int)
vect3D_conv(int)
vectND_conv(int)
matr2D_conv(int)
matr3D_conv(int)
matrND_conv(int)


scalar_stats(float, -DXD_MAX_FLOAT, DXD_MAX_FLOAT)
regscalar_stats(float, -DXD_MAX_FLOAT, DXD_MAX_FLOAT)
vect2D_stats(float, -DXD_MAX_FLOAT, DXD_MAX_FLOAT)
vect3D_stats(float, -DXD_MAX_FLOAT, DXD_MAX_FLOAT)
vectND_stats(float, -DXD_MAX_FLOAT, DXD_MAX_FLOAT)
matr2D_stats(float, -DXD_MAX_FLOAT, DXD_MAX_FLOAT)
matr3D_stats(float, -DXD_MAX_FLOAT, DXD_MAX_FLOAT)
matrND_stats(float, -DXD_MAX_FLOAT, DXD_MAX_FLOAT)

scalar_conv(float)
vect2D_conv(float)
vect3D_conv(float)
vectND_conv(float)
matr2D_conv(float)
matr3D_conv(float)
matrND_conv(float)

/* these are float not double because the stats array is reported as
 * floats instead of doubles.
 */
scalar_stats(double, -DXD_MAX_FLOAT, DXD_MAX_FLOAT)
regscalar_stats(double, -DXD_MAX_FLOAT, DXD_MAX_FLOAT)
vect2D_stats(double, -DXD_MAX_FLOAT, DXD_MAX_FLOAT)
vect3D_stats(double, -DXD_MAX_FLOAT, DXD_MAX_FLOAT)
vectND_stats(double, -DXD_MAX_FLOAT, DXD_MAX_FLOAT)
matr2D_stats(double, -DXD_MAX_FLOAT, DXD_MAX_FLOAT)
matr3D_stats(double, -DXD_MAX_FLOAT, DXD_MAX_FLOAT)
matrND_stats(double, -DXD_MAX_FLOAT, DXD_MAX_FLOAT)

scalar_conv(double)
vect2D_conv(double)
vect3D_conv(double)
vectND_conv(double)
matr2D_conv(double)
matr3D_conv(double)
matrND_conv(double)


#define PP_TEST  0   /* use new processparts routine which stops at CFs */
     
Error DXStatistics (Object o, char *compname, 
		    float *min, float *max, float *avg, float *stdev)
{
    char *statname = NULL;
    struct stats sp, *ap;
    struct argblock *ab = NULL;
    int len;
    int isvalid = 0;


    /* 
     * set these to the extreme values in case we find an error and exit early 
     */
    if (min) *min = DXD_MAX_FLOAT;
    if (max) *max = -DXD_MAX_FLOAT;
    if (avg) *avg = 0.0;
    if (stdev) *stdev = 0.0;
    
    /* the object and the component name are required */
    if (!o || !compname) {
	DXSetError (ERROR_BAD_PARAMETER, "#11870");
	goto error;
    }
    

    memset (&sp, '\0', sizeof(struct stats));
    sp.min = DXD_MAX_FLOAT;
    sp.max = -DXD_MAX_FLOAT;
    
    /* the input to this routine can be a single array, a field or
     * an entire hierarchical group structure.  if the input is a
     * single array, compute the stats and return them.  if the input
     * is anything else, as the stats are being computed on each field
     * add a stats component so that the next time stats are requested
     * they won't have to be recomputed.  this does modify the input object
     * (i think it's the only exception to this rule in the system), but
     * so far it hasn't caused any problems.  the only alternative we can
     * think of is to always compute stats when the field is changed (in 
     * DXEndField(), for example), but then it may do a lot of unnecessary
     * work if no one downstream wants the numbers.  we could reconsider this.
     */

    if (DXGetObjectClass(o) == CLASS_ARRAY) {
	
        struct stats s;
	ap = Add__Stats(o, NULL, NULL, 0, &s);
	if (!ap || !ap->isvalid)
	    goto error;
	
	sp.min = ap->min;
	sp.max = ap->max;
	sp.sum += ap->sum;
	sp.sumsq += ap->sumsq;
	sp.counts += ap->counts;

    } else {

	/* 
	 * if the component doesn't exist in any field, return error 
	 */
	if (!DXExists (o, compname)) {
	    DXSetError (ERROR_MISSING_DATA, "#10250", "input", compname);
	    goto error;
	}
	
#define STATLEN 12  /* strlen(" statistics"); */
	statname = (char *)DXAllocateLocal (strlen(compname) + STATLEN);
	if (!statname)
	    goto error;
	strcpy (statname, compname);
	strcat (statname, " statistics");
	
	/* if the statistics don't already exist, compute them in parallel 
	 */

	/* Exists2 is an undocumented call which requires that all fields
	 *  which contain the first component also contain the second.
         *  when this section is done, all fields are guarenteed to have
	 *  a stats component.
         */
	  
	if (!Exists2 (o, compname, statname)) {

	    /* 
	     * if we have to compute the stats, build a parm block
	     * in global memory so we can use it with DXAddTask.
	     * on a workstation we still need a place to store the
	     * component name and the "compname statistics" strings,
	     * so it isn't bad to do this even if this isn't an mp system.
	     */
	    if (!buildblock(compname, statname, &ab, &len))
		goto error;

#if PP_TEST
	    if ((_dxfProcessPartsG (o, CFCompute_Wrapper, 
				    ab, len, 0, 1)) == NULL) {
		if (DXGetError() != ERROR_NONE)
		    goto error;
		/* else it's a valid empty field */
	    }
#else
	    /* if this is a simple field, compute the stats and return.
             * if this is a complicated object, schedule the stats to be
	     * computed in parallel, and execute all the stats computations
	     * with ExecuteTaskGroup.
	     */
	    if ((ProcessPartsXX (o, ab, len) == ERROR))
		goto error;
	    
	    if (ab->isparallel) {
		if (!DXExecuteTaskGroup())
		    goto error;
	    }
#endif

#if 0
	    if ((DXProcessParts (o, Compute_Wrapper, ab, len, 0, 1)) == NULL) {
		if (DXGetError() != ERROR_NONE)
		    goto error;
		/* else it's a valid empty field */
	    }
#endif
	
	}

	/* when you get here, you know that all the fields which contain
	 * a "compname" component also contain a "compname statistics"
	 * component with valid stats.  all that this routine needs to do
	 * is combine all the stats components together to get the combined
	 * min, max, avg, etc 
	 */
	if (!AccumulateStats (o, compname, statname, &sp, &isvalid, 0))
	    goto error;
	
	/* if all the items in the input object are invalid, set this error
	 *  and return.  it's unfortunate that it has to be an error, but
	 *  there is no way other than changing the interface to this routine
	 *  to indicate that the numbers coming back have nothing to do with
	 *  what the stats really are.
	 */
	if (!isvalid) {
	    DXSetError (ERROR_DATA_INVALID, "#11905");
	    goto error;
	}
    }

    /* check for illegal values.  since all values contribute to the sum
     * check it to see if it's NaN.  if so, change all others to be also.
     */
    if (isnan(sp.sum))
	sp.min = sp.max = sp.sumsq = sp.avg = sp.sum;

    /* now that we know the values, fill in the ones the user asked for.
     * the temp variable is because on the pvs at one point we were having
     * round-off errors with very small numbers and the temp value was 
     * getting slightly negative.  asking for sqrt in that case segfaulted.
     */
    if (min) *min = sp.min;
    if (max) *max = sp.max;
    if (avg && sp.counts) *avg = sp.sum / sp.counts;
    if (stdev && sp.counts > 1 && (sp.max != sp.min))  {
	double temp;
	temp = (sp.sumsq - (sp.sum * sp.sum / sp.counts)) / (sp.counts - 1);
	if (temp > 0.0)
	    *stdev = sqrt(temp);
    }


    DXFree((Pointer)ab);
    DXFree((Pointer)statname);
    return OK;
    
  error:
    DXFree((Pointer)ab);
    DXFree((Pointer)statname);
    return ERROR;
}

/* 
 * the library process parts routine doesn't stop at composite fields.
 * this is a replacement which does.
 */

static Error
ProcessPartsXX(Object o, struct argblock *ab, int len)
{
    Object subo, subo2;
    Matrix m;
    int fixed, z;
    int i;

    switch (DXGetObjectClass(o)) {

      case CLASS_GROUP:

	if (ab->isparallel) {
	    if (DXGetGroupClass((Group)o) == CLASS_COMPOSITEFIELD) {
		ab->f = (Field)o;
		if (!DXAddTask(ComputeStats, (Pointer)ab, len, 0.0))
		    goto error;
	    } else {
		for (i=0; (subo=DXGetEnumeratedMember((Group)o, i, NULL)); i++)
		    if (!ProcessPartsXX (subo, ab, len))
			return ERROR;
	    }
	} else {
	    if (DXGetGroupClass((Group)o) == CLASS_COMPOSITEFIELD) {
		ab->f = (Field)o;
		return (ComputeStats((Pointer)ab));
		
	    } else {
		if (!DXCreateTaskGroup())
		    return ERROR;
		ab->isparallel = 1;
		for (i=0; (subo=DXGetEnumeratedMember((Group)o, i, NULL)); i++)
		    if (!ProcessPartsXX (subo, ab, len))
			return ERROR;
	    }
	}
	
	break;
	    
	
      case CLASS_FIELD:
	ab->f = (Field)o;
	if (ab->isparallel) {
	    if (!DXAddTask(ComputeStats, (Pointer)ab, len, 0.0))
		goto error;
	} else {
	    if (!ComputeStats(ab))
		return ERROR;
	}
	break;

      case CLASS_SCREEN:
	if (!DXGetScreenInfo((Screen)o, &subo, &fixed, &z))
	    return ERROR;

	if (!ProcessPartsXX(subo, ab, len))
	    return ERROR;
	
	break;

      case CLASS_XFORM:
	if (!DXGetXformInfo((Xform)o, &subo, &m))
	    return ERROR;
	
	if (!ProcessPartsXX(subo, ab, len))
	    return ERROR;
	
	break;
	
      case CLASS_CLIPPED:
	if (!DXGetClippedInfo((Clipped)o, &subo, &subo2))
	    return ERROR;
	
	if (!ProcessPartsXX(subo, ab, len))
	    return ERROR;
	
	break;

      default:
	/* any other types of objects can't contain fields.
	 */
	break;
    }

    return OK;

  error:
    if (ab->isparallel)
	DXAbortTaskGroup();
    return ERROR;
}


static Error buildblock(char *compname, char *statname, 
		       struct argblock **ab, int *len)
{
    char *cp;
    
    *len = sizeof(struct argblock) + strlen(compname) + strlen(statname) + 2;
    *ab = (struct argblock *)DXAllocate(*len);
    if (!*ab)
	return ERROR;

    (*ab)->len = *len;
    (*ab)->isparallel = 0;
    (*ab)->f = NULL;
    cp = (*ab)->data;
    (*ab)->compname = cp;
    strcpy(cp, compname);
    
    cp += strlen(compname) + 1;
    (*ab)->statname = cp;
    strcpy(cp, statname);
    
    return OK;
}



/*
 * for ProcessParts - not used now.
 */ 
#if 0
static Field Compute_Wrapper(Field f, char *argblock, int argsize)
{
    return ((Field)ComputeStats((Object)f, 
				(struct argblock *)argblock, 
				argsize));
}

static CompositeField CFCompute_Wrapper(CompositeField f, 
					char *argblock, int argsize)
{
    return ((CompositeField)ComputeStats((Object)f, 
					 (struct argblock *)argblock, 
					 argsize));
}
#endif


/* 
 * called only on fields or compositefields
 */
static Error ComputeStats(Pointer ptr)
{
    Object o, subo;
    struct argblock *ab = (struct argblock *)ptr;
    int i;
    struct stats dummy_stats;

    /* should be only a field or composite field */
    o = (Object)ab->f;
    if (!o) {
	DXSetError(ERROR_BAD_PARAMETER, "#11870");
	return ERROR;
    }
    
    
    switch (DXGetObjectClass(o)) {
      case CLASS_GROUP:
	if (DXGetGroupClass((Group)o) != CLASS_COMPOSITEFIELD)
	    goto error;
	
	/* partitioned data shares points, so compute it twice.  once
	 * ignoring duplicated points and once independently per field.
	 * do the normal type of stats first, then do cf type.  the order
	 * is important here - there is some code at the end of Add__Stats
	 * which cares (it wants the component to exist the next time around.
	 */
#if PP_TEST
	if ((DXProcessParts (o, Compute_Wrapper, ab, len, 0, 1)) == NULL)
		return ERROR;
#else
	for (i = 0; (subo=DXGetEnumeratedMember((Group)o, i, NULL)); i++) {
	    if (DXReallyEmptyField((Field)subo))
		continue;
	
	    if (!Add__Stats(subo, ab->compname, ab->statname, 0,&dummy_stats))
		return ERROR;
	}
#endif

	/* this must be called on the entire composite field so it can
         *  add the duplicated points to the invalid list.
	 */ 
	if (! DXInvalidateDupBoundary(o))
	    return ERROR;
	
#if PP_TEST
	if ((DXProcessParts (o, Compute_Wrapper, ab, len, 0, 1)) == NULL)
		return ERROR;
#else
	for (i = 0; (subo=DXGetEnumeratedMember((Group)o, i, NULL)); i++) {
	    if (DXReallyEmptyField((Field)subo))
		continue;
	
	    if (!Add__Stats(subo, ab->compname, ab->statname, 1, &dummy_stats))
		return ERROR;
	}
#endif

	if (! DXRestoreDupBoundary(o))
	    return ERROR;
	
	break;
	
      case CLASS_FIELD:
	if (DXReallyEmptyField((Field)o))
	    break;
	
	if (!Add__Stats(o, ab->compname, ab->statname, 0, &dummy_stats))
	    return ERROR;
	
	break;
	
      default:
      error:
	DXSetError(ERROR_INTERNAL, 
		   "ComputeStats called with wrong object type");
	return ERROR;
    }
    
    return OK;
}


static Error
AccumulateStats(Object o, char *compname, char *statname, Stats sp, int *iv,
		int cfmember)
{
    Object subo, subo2;
    Matrix m;
    Array a;
    Stats ap;
    int fixed, z;
    int i;

    switch (DXGetObjectClass(o)) {

      case CLASS_GROUP:

	if (DXGetGroupClass((Group)o) == CLASS_COMPOSITEFIELD) {
	    /* use the cf stats here */
	    for (i=0; (subo=DXGetEnumeratedMember((Group)o, i, NULL)); i++)
		if (!AccumulateStats(subo, compname, statname, sp, iv, 1))
		    return ERROR;
	    
	} else {   /* any other type of group uses normal stats */
	    
	    for (i=0; (subo=DXGetEnumeratedMember((Group)o, i, NULL)); i++)
		if (!AccumulateStats(subo, compname, statname, sp, iv, 0))
		    return ERROR;
	}
	    
	break;
	    
      case CLASS_FIELD:

	/* ignore fields with no components. */
	if (DXReallyEmptyField((Field)o))
	    break;
	
	a = (Array)DXGetComponentValue((Field)o, statname);
	if (!a) {
	    a = (Array)DXGetComponentValue((Field)o, compname);
	    if (!a)
		return OK;
	    
	    DXSetError(ERROR_MISSING_DATA, "#11880", compname);
	    return ERROR;
	}

	ap = (struct stats *)DXGetArrayData(a);
	if (!ap)
	    return ERROR;
	
	if (cfmember)
	    ap++;

	if (ap->isvalid) {
	    if (sp->min > ap->min)
		sp->min = ap->min;
	    
	    if (sp->max < ap->max)
		sp->max = ap->max;
	    
	    sp->sum += ap->sum;
	    sp->sumsq += ap->sumsq;
	    sp->counts += ap->counts;
	    (*iv)++;
	}
	
	break;
	
      case CLASS_SCREEN:
	if (!DXGetScreenInfo((Screen)o, &subo, &fixed, &z))
	    return ERROR;

	if (!AccumulateStats(subo, compname, statname, sp, iv, 0))
	    return ERROR;
	
	break;

      case CLASS_XFORM:
	if (!DXGetXformInfo((Xform)o, &subo, &m))
	    return ERROR;
	
	if (!AccumulateStats(subo, compname, statname, sp, iv, 0))
	    return ERROR;
	
	break;
	
      case CLASS_CLIPPED:
	if (!DXGetClippedInfo((Clipped)o, &subo, &subo2))
	    return ERROR;
	
	if (!AccumulateStats(subo, compname, statname, sp, iv, 0))
	    return ERROR;
	
	break;

      default:
	/* anything else can't have field objects below it, so it can
	 *  safely be passed through unchanged 
	 */
	break;
    }

    return OK;
}


/*
 * On successful completion, fill in the Stats structure pointed to by sp.
 */
static Stats Add__Stats(Object o, char *compname, 
			char *statname, int cfmember, struct stats *sptr)
{
    Array a, na = NULL;
    Type type;
    Category category;
    int rank;
    Pointer dp, dp2;
    int isarray = 0;
    int size, shape[MAXDIM];
    double origin[MAXDIM];
    struct stats *s = NULL;
    ICH hasinvalid = NULL;

    /* this code either does a simple field, or a single array */
    if (DXGetObjectClass(o) == CLASS_FIELD) {
	a = (Array)DXGetComponentValue((Field)o, compname);
	if (!a) {
	    DXSetError(ERROR_MISSING_DATA, "#10250", "input", compname);
	    return NULL;
	}
	/* are they already there?  they must be if cfmember is set, they
         *  may or may not be otherwise.
	 */
	na = (Array)DXGetComponentValue((Field)o, statname);
	if (cfmember && !na) {
	    DXSetError(ERROR_INTERNAL, 
		       "problems computing partitioned statistics");
	    return NULL;
	}
	if (!cfmember && na)
	    return (Stats)DXGetArrayData(na);

	na = NULL;
	if (!HasInvalid((Field)o, compname, &hasinvalid))
	    return NULL;
    } else {
	a = (Array)o;
	isarray++;
    }
    if (DXGetObjectClass((Object)a) != CLASS_ARRAY) {
	DXSetError(ERROR_BAD_CLASS, "#11890");
	goto done1;
    }

    /* get size and shape */
    DXGetArrayInfo(a, &size, &type, &category, &rank, shape);
    if (rank == 0)
	shape[0] = 1;

    /* check type, category, rank & shape */
    if (supported(a) != OK)
	goto done1;

    s = sptr;
    memset(s, '\0', sizeof(struct stats));
    

    /* after the stats array is set to 0, return if there is no data */
    if (size <= 0)
	goto nodata;

    s->counts = size;
	

    /* if the data is constant, this converts the origins to double, and
     *  does NOT call DXGetArrayData so the constant array doesn't get
     *  expanded.
     */
    if (ConstantData(a, origin) && !hasinvalid) {
	
	switch(shape[0]) {
	  case 0:    /* scalar == 1-vector == 1-matrix, etc */
	  case 1:
	    doubleS_stat(origin, 1, s, NULL);
	    break;
	    
	  case 2:  /* 2-vector or 2x2 matrix */
	    switch(rank) {
	      case 1:
		doublev2D_stat(origin, 1, s, NULL);
		break;
	      case 2:
		doublem2D_stat(origin, 1, s, NULL);
		break;
	    }
	    break;

	  case 3:   /* 3-vector or 3x3 matrix */
	    switch(rank) {
	      case 1:
		doublev3D_stat(origin, 1, s, NULL);
		break;
	      case 2:
		doublem3D_stat(origin, 1, s, NULL);
		break;
	    }
	    break;
	    
	  default:  /* N-vector or NxN matrix */
	    switch(rank) {
	      case 1:
		doublevND_stat(origin, 1, shape[0], s, NULL);
		break;
	      case 2:
		doublemND_stat(origin, 1, shape[0], s, NULL);
		break;
	    }
	    break;
	}

	/* fix sum and sum squared because we've only actually worked
         * on 1 data item instead of the real count.  also have to
	 * fix counts as well.
	 */
	s->sum = s->avg * size;
	s->sumsq = s->avg * s->avg * size;
	s->counts = size;  

	if (s->counts <= 0)
	    goto nodata;

	s->isvalid = 1;
	goto done;
    }

    /* define the macros used in the case statements below.
     */

#define EACHTYPE(ss) \
    switch(type) { \
      case TYPE_UBYTE: ubyte##ss##_stat((ubyte *)dp, size, s, hasinvalid); \
	break; \
      case TYPE_BYTE: byte##ss##_stat((byte *)dp, size, s, hasinvalid); \
	break; \
      case TYPE_USHORT: ushort##ss##_stat((ushort *)dp, size, s, hasinvalid); \
	break; \
      case TYPE_SHORT: short##ss##_stat((short *)dp, size, s, hasinvalid); \
	break; \
      case TYPE_UINT: uint##ss##_stat((uint *)dp, size, s, hasinvalid); \
	break; \
      case TYPE_INT: int##ss##_stat((int *)dp, size, s, hasinvalid); \
	break; \
      case TYPE_FLOAT: float##ss##_stat((float *)dp, size, s, hasinvalid); \
	break; \
      case TYPE_DOUBLE: double##ss##_stat((double *)dp, size, s, hasinvalid); \
	break; \
    }
    
#define EACHTYPE_R(ss) \
    switch(type) { \
      case TYPE_UBYTE: \
	ubyte##ss##_stat((ubyte *)dp, (ubyte *)dp2, size, s, hasinvalid); \
	break; \
      case TYPE_BYTE: \
        byte##ss##_stat((byte *)dp, (byte *)dp2, size, s, hasinvalid); \
	break; \
      case TYPE_USHORT: \
        ushort##ss##_stat((ushort *)dp, (ushort *)dp2, size, s, hasinvalid); \
	break; \
      case TYPE_SHORT: \
        short##ss##_stat((short *)dp, (short *)dp2, size, s, hasinvalid); \
	break; \
      case TYPE_UINT: \
        uint##ss##_stat((uint *)dp, (uint *)dp2, size, s, hasinvalid); \
	break; \
      case TYPE_INT: \
        int##ss##_stat((int *)dp, (int *)dp2, size, s, hasinvalid); \
	break; \
      case TYPE_FLOAT: \
        float##ss##_stat((float *)dp, (float *)dp2, size, s, hasinvalid); \
	break; \
      case TYPE_DOUBLE: \
        double##ss##_stat((double *)dp, (double *)dp2, size, s, hasinvalid); \
	break; \
    }
    
#define EACHTYPE_N(ss, len) \
    switch(type) { \
      case TYPE_UBYTE: \
        ubyte##ss##_stat((ubyte *)dp, size, len, s, hasinvalid); \
	break; \
      case TYPE_BYTE: \
        byte##ss##_stat((byte *)dp, size, len, s, hasinvalid); \
	break; \
      case TYPE_USHORT: \
        ushort##ss##_stat((ushort *)dp, size, len, s, hasinvalid); \
	break; \
      case TYPE_SHORT: \
        short##ss##_stat((short *)dp, size, len, s, hasinvalid); \
	break; \
      case TYPE_UINT: \
        uint##ss##_stat((uint *)dp, size, len, s, hasinvalid); \
	break; \
      case TYPE_INT: \
        int##ss##_stat((int *)dp, size, len, s, hasinvalid); \
	break; \
      case TYPE_FLOAT: \
        float##ss##_stat((float *)dp, size, len, s, hasinvalid); \
	break; \
      case TYPE_DOUBLE: \
        double##ss##_stat((double *)dp, size, len, s, hasinvalid); \
	break; \
    }
    

    /* if the data is scalar and regular, compute the stats without
     *  calling DXGetArrayData so the data doesn't get expanded.
     */
    if (RegularData(a)) {
	
	switch(shape[0]) {
	  case 0:    /* scalar == 1-vector == 1-matrix, etc */
	  case 1:
	    dp = (Pointer)origin;
	    dp2 = (Pointer)(origin+1);
	    if (!DXGetRegularArrayInfo((RegularArray)a, NULL, dp, dp2))
		goto error;

	    EACHTYPE_R(RS);
	    
	    if (s->counts <= 0)
		goto nodata;
	    
	    s->isvalid = 1;
	    goto done;
	    
	  default: /* for now, don't handle vectors or matricies */
	    break;
	}
    }


    /* must be irregular and/or have invalid parts */
    
    dp = DXGetArrayData(a);
    if (!dp)
	goto error;
    
    switch(shape[0]) {
      case 0:    /* scalar == 1-vector == 1-matrix, etc */
      case 1:
	EACHTYPE(S);
	break;
	
      case 2:  /* 2-vector or 2x2 matrix */
	switch(rank) {
	  case 1:
	    EACHTYPE(v2D);
	    break;
	  case 2:
	    EACHTYPE(m2D);
	    break;
	}
	break;

      case 3:   /* 3-vector or 3x3 matrix */
	switch(rank) {
	  case 1:
	    EACHTYPE(v3D);
	    break;
	  case 2:
	    EACHTYPE(m3D);
	    break;
	}
	break;
	
      default:  /* N-vector or NxN matrix */
	switch(rank) {
	  case 1:
	    EACHTYPE_N(vND, shape[0]);
	    break;
	  case 2:
	    EACHTYPE_N(mND, shape[0]);
	    break;
	}
	break;
    }

    if (s->counts <= 0)
	goto nodata;

#define STATLEN 12  /* strlen(" statistics"); */

  nodata:
    /* fall thru */

  done:
    if (!isarray) {
	int n = sizeof(struct stats) / sizeof(double);
	double one = 1.0;

	/* make new stats component if not composite field */
	if (!cfmember) {
	    na = DXNewArray(TYPE_DOUBLE, CATEGORY_REAL, 0);
	    if(!na)
		goto error;
	    
	    if (!DXAddArrayData(na, 0, n, (Pointer)sptr))
		goto error;

	    /* if there was an invalid component, make the stats both der
	     * on the data and on the invalid component.  else just der data.
	     */
	    if (hasinvalid) {
		char *cp;
		Array sl;

		if (!InvalidName((Field)o, compname, &cp))
		    goto error;
		sl = DXMakeStringList(2, compname, cp);
		DXFree(cp);
		if (!sl)
		    goto error;
		if (!DXSetAttribute((Object)na, "der", (Object)sl)) {
		    DXDelete((Object)sl);
		    goto error;
		}
	    } else if (!DXSetStringAttribute((Object)na, "der", compname))
		goto error;

	    if (!DXSetComponentValue((Field)o, statname, (Object)na))
		goto error;
	    
	} else {
	    na = (Array)DXGetComponentValue((Field)o, statname);
	    if (!na)
		goto error;

	    if (!DXAddArrayData(na, n, n, (Pointer)sptr))
		goto error;
	    
	    if (!DXAddArrayData(na, n*2, 1, &one))
		goto error;
	}
    }

  done1:    
    DXFreeInvalidComponentHandle(hasinvalid);
    return s;

  error:
    DXFreeInvalidComponentHandle(hasinvalid);
    DXDelete((Object)na);
    return NULL;
}


static Error supported(Array a)
{
    Type type;
    Category category;
    int rank, shape[MAXDIM];

    /* get size and shape */
    DXGetArrayInfo(a, NULL, &type, &category, &rank, shape);

    /* recognized types */
    switch(type) {
      case TYPE_UBYTE:
      case TYPE_BYTE:
      case TYPE_USHORT:
      case TYPE_SHORT:
      case TYPE_UINT:
      case TYPE_INT:
      case TYPE_FLOAT:
      case TYPE_DOUBLE:
	break;
      default:
	DXSetError(ERROR_NOT_IMPLEMENTED, "#11900", "type");
	return ERROR;
    }

    /* scalar, all vectors, and square matricies
     */
    if(rank > 2 || (rank == 2 && (shape[0] != shape[1]))) {
	DXSetError(ERROR_NOT_IMPLEMENTED, "#11900", "shape");
	return ERROR;
    }
    
    if(category != CATEGORY_REAL) {
	DXSetError(ERROR_NOT_IMPLEMENTED, "#11900", "category");
	return ERROR;
    }
    
    return OK;
}


Array DXScalarConvert(Array a)
{
    int i, nitems;
    int rank, shape[MAXDIM];
    double origins[MAXDIM];
    float norigin, ndelta;
    Type type;
    Pointer dp, dp2;
    float *fp;
    Array na = NULL;

    /* type check: first object then datatype */
    if (DXGetObjectClass((Object)a) != CLASS_ARRAY) {
	DXSetError(ERROR_DATA_INVALID, "#11920");
	return NULL;
    }

    if (supported(a) != OK)
	return NULL;

    /* get the number of items in the input array */
    if (!DXGetArrayInfo(a, &nitems, &type, NULL, &rank, shape))
	return NULL;

    if (rank == 0)
	shape[0] = 1;

    if (nitems <= 0) {
	DXSetError(ERROR_DATA_INVALID, "#11910");
	return NULL;
    }
	
    /* if the data is constant, create a rank 0, type float array 
     *  with the same number of items.   the ConstantData() call
     *  returns the data value as a double in the "origins" array,
     *  so all the subsequent calls convert the double value to a float
     *  with the XXX_conv() routines, and then finally norigin is used
     *  when creating the scalar actual array.
     */
    if (ConstantData(a, origins)) {

	switch(shape[0]) {
	  case 0:    /* scalars, 1-vectors and 1-matricies are same */
	  case 1:
	    doubleS_conv(origins, 1, &norigin);
	    break;
	    
	  case 2:  /* 2-vectors or 2x2 matricies */
	    switch(rank) {
	      case 1:
		doublev2D_conv(origins, 1, &norigin);
		break;
	      case 2:
		doublem2D_conv(origins, 1, &norigin);
		break;
	    }
	    break;
	
	  case 3:   /* 3-vectors or 3x3 matricies */
	    switch(rank) {
	      case 1:
		doublev3D_conv(origins, 1, &norigin);
		break;
	      case 2:
		doublem3D_conv(origins, 1, &norigin);
		break;
	    }
	    break;
	    
	  default:   /* N-vectors or NxN matricies */
	    switch(rank) {
	      case 1:
		doublevND_conv(origins, 1, shape[0], &norigin);
		break;
	      case 2:
		doublemND_conv(origins, 1, shape[0], &norigin);
		break;
	    }
	    break;
	}
	

	/* a constant array of scalars */
	na = (Array)DXNewConstantArray(nitems, (Pointer)&norigin,
				       TYPE_FLOAT, CATEGORY_REAL, 0);

	return na;

    }

#undef EACHTYPE
#define EACHTYPE(ss) \
    switch(type) { \
      case TYPE_UBYTE: ubyte##ss##_conv((ubyte *)dp, nitems, fp); \
	break; \
      case TYPE_BYTE: byte##ss##_conv((byte *)dp, nitems, fp); \
	break; \
      case TYPE_USHORT: ushort##ss##_conv((ushort *)dp, nitems, fp); \
	break; \
      case TYPE_SHORT: short##ss##_conv((short *)dp, nitems, fp); \
	break; \
      case TYPE_UINT: uint##ss##_conv((uint *)dp, nitems, fp); \
	break; \
      case TYPE_INT: int##ss##_conv((int *)dp, nitems, fp); \
	break; \
      case TYPE_FLOAT: float##ss##_conv((float *)dp, nitems, fp); \
	break; \
      case TYPE_DOUBLE: double##ss##_conv((double *)dp, nitems, fp); \
	break; \
    }

    
#undef EACHTYPE_N
#define EACHTYPE_N(ss, len) \
    switch(type) { \
      case TYPE_UBYTE: ubyte##ss##_conv((ubyte *)dp, nitems, len, fp); \
	break; \
      case TYPE_BYTE: byte##ss##_conv((byte *)dp, nitems, len, fp); \
	break; \
      case TYPE_USHORT: ushort##ss##_conv((ushort *)dp, nitems, len, fp); \
	break; \
      case TYPE_SHORT: short##ss##_conv((short *)dp, nitems, len, fp); \
	break; \
      case TYPE_UINT: uint##ss##_conv((uint *)dp, nitems, len, fp); \
	break; \
      case TYPE_INT: int##ss##_conv((int *)dp, nitems, len, fp); \
	break; \
      case TYPE_FLOAT: float##ss##_conv((float *)dp, nitems, len, fp); \
	break; \
      case TYPE_DOUBLE: double##ss##_conv((double *)dp, nitems, len, fp); \
	break; \
    }
    


    /* if the data is scalar and regular, compute the new float Regular array
     *  without calling DXGetArrayData so the data doesn't get expanded.
     */
    if (RegularData(a)) {
	
	switch(shape[0]) {
	  case 0:    /* scalar == 1-vector == 1-matrix, etc */
	  case 1:
	    /* convert origin & deltas to float & create new reg array 
             */

	    dp = (Pointer)origins;
	    dp2 = (Pointer)(origins+1);
	    if (!DXGetRegularArrayInfo((RegularArray)a, NULL, dp, dp2))
		goto error;
	    
	    /* temporarily lie about the number of items 
	     */
	    i = nitems;
	    nitems = 1;

	    fp = (Pointer)&norigin;
	    EACHTYPE(S);

	    dp = (Pointer)(origins+1);
	    fp = (Pointer)&ndelta;
	    EACHTYPE(S);

	    /* now restore them 
	     */
	    nitems = i;

	    /* a regular array of scalars */
	    na = (Array)DXNewRegularArray(TYPE_FLOAT, 1, nitems,
					  (Pointer)&norigin, (Pointer)&ndelta);
	    
	    return na;
	    
	  default: /* for now, don't handle vectors or matricies */
	    break;
	}
    }


    /* data must be irregular */
    
    if (!(na = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 0)))
	return NULL;

    if (!DXAddArrayData(na, nitems, 0, NULL))
	goto error;
    
    dp = DXGetArrayData(a);
    if(!dp)
	goto error;
    
    fp = (float *)DXGetArrayData(na);
    if(!fp)
	goto error;
    

    switch(shape[0]) {
      case 0:    /* scalars, 1-vectors and 1-matricies are same */
      case 1:
	EACHTYPE(S);
	break;
	
      case 2:  /* 2-vectors or 2x2 matricies */
	switch(rank) {
	  case 1:
	    EACHTYPE(v2D);
	    break;
	  case 2:
	    EACHTYPE(m2D);
	    break;
	}
	break;
	
      case 3:   /* 3-vectors or 3x3 matricies */
	switch(rank) {
	  case 1:
	    EACHTYPE(v3D);
	    break;
	  case 2:
	    EACHTYPE(m3D);
	    break;
	}
	break;

      default:   /* N-vectors or NxN matricies */
	switch(rank) {
	  case 1:
	    EACHTYPE_N(vND, shape[0]);
	    break;
	  case 2:
	    EACHTYPE_N(mND, shape[0]);
	    break;
	}
	break;
    }
    
    /* return the new array */
    return na;

  error:
    if (na) DXDelete((Object)na);
    return NULL;
}


/* returns OK if data is constant, and converts origins to double.
 * returns ERROR (but doesn't set an error code) if not constant data value.
 */
static Error
ConstantData(Array a, double *origin)
{
    Type type;
    Category cat;
    int rank;
    int i, shape[MAXDIM], count;
    ubyte *op;
    ubyte generic_origin[MAXDIM*sizeof(double)];
    ubyte generic_delta[MAXDIM*sizeof(double)];


    /* if scalar or vector, don't expand.  for rank > 2, i'll need
     *  to add more code.
     */
    if (DXGetArrayClass(a) == CLASS_CONSTANTARRAY) {
	DXGetArrayInfo(a, NULL, &type, &cat, &rank, shape);
	if (cat != CATEGORY_REAL || rank >= 2)
	    return ERROR;
	
	count = DXGetItemSize(a) / DXTypeSize(type);
	op = (ubyte *)DXGetConstantArrayData(a);
	
#define MAKEDOUBLE(type) \
	for (i=0; i<count; i++) \
	    origin[i] = (double)((type *)op)[i];

    
	/* convert origins from whatever type they are to double
	 */
	switch(type) {
	  case TYPE_UBYTE:	MAKEDOUBLE(ubyte);	break;
	  case TYPE_BYTE:	MAKEDOUBLE(byte);	break;
	  case TYPE_USHORT:	MAKEDOUBLE(ushort);	break;
	  case TYPE_SHORT:	MAKEDOUBLE(short);	break;
	  case TYPE_UINT:	MAKEDOUBLE(uint);	break;
	  case TYPE_INT:	MAKEDOUBLE(int);	break;
	  case TYPE_FLOAT:	MAKEDOUBLE(float);	break;
	  case TYPE_DOUBLE:	MAKEDOUBLE(double);	break;
	}
	
	return OK;
	
    }
	
    /* if regular array with zero deltas */
    if (DXGetArrayClass(a) != CLASS_REGULARARRAY)
	return ERROR;

    /* get the things you can't get from RegArrayInfo */
    DXGetArrayInfo(a, NULL, &type, NULL, &rank, shape);

    DXGetRegularArrayInfo((RegularArray)a, NULL,
			  (Pointer)generic_origin, (Pointer)generic_delta);


#define CHECKDELTAS(type) \
\
    for (i=0; i<shape[0]; i++) { \
	origin[i] = (double)((type *)generic_origin)[i]; \
	if (((type *)generic_delta)[i] == 0) \
	    continue; \
\
	return ERROR; \
    }

    
    /* if deltas are non-zero, treat as irregular.
     *  convert origins from whatever type they are to double
     */
    switch(type) {
      case TYPE_UBYTE:	CHECKDELTAS(ubyte);	break;
      case TYPE_BYTE:	CHECKDELTAS(byte);	break;
      case TYPE_USHORT:	CHECKDELTAS(ushort);	break;
      case TYPE_SHORT:	CHECKDELTAS(short);	break;
      case TYPE_UINT:	CHECKDELTAS(uint);	break;
      case TYPE_INT:	CHECKDELTAS(int);	break;
      case TYPE_FLOAT:	CHECKDELTAS(float);	break;
      case TYPE_DOUBLE:	CHECKDELTAS(double);	break;
    }

    return OK;
}
    
/* returns OK if data is scalar and regular.
 * returns ERROR (but doesn't set an error code) if not.
 */
static Error
RegularData(Array a)
{
    Type type;
    Category cat;
    int rank;
    int shape[MAXDIM];


    if (DXGetArrayClass(a) != CLASS_REGULARARRAY)
	return ERROR;

    if (!DXGetArrayInfo(a, NULL, &type, &cat, &rank, shape))
	return ERROR;

    if (cat != CATEGORY_REAL || rank >= 2)
	return ERROR;

    if (rank == 1 && shape[0] != 1)
	return ERROR;

    return OK;
}
    
static Error HasInvalid(Field f, char *component, ICH *ih)
{
    Error rc = OK;
    char *dep = NULL;
    char *invalid = NULL;

    *ih = NULL;

    if (!DXGetStringAttribute(DXGetComponentValue(f, component), "dep", &dep))
	return OK;
    
#define INVLEN 10     /* strlen("invalid "); */    
    
    if (!(invalid = (char *)DXAllocateLocal(strlen(dep) + INVLEN)))
	return ERROR;

    strcpy(invalid, "invalid ");
    strcat(invalid, dep);

    /* if no component, return NULL */
    if (!DXGetComponentValue(f, invalid))
	goto done;

    *ih = DXCreateInvalidComponentHandle((Object)f, NULL, dep);
    if (!*ih) {
	rc = ERROR;
	goto done;
    }
    
  done:
    DXFree((Pointer)invalid);
    return rc;
}
    
/* set the name of the invalid component which corresponds to the 
 * given component.
 */
static Error InvalidName(Field f, char *component, char **invalid)
{
    char *dep = NULL;

    *invalid = NULL;
    
    if (!DXGetStringAttribute(DXGetComponentValue(f, component), "dep", &dep))
	return OK;
    
#define INVLEN 10     /* strlen("invalid "); */    
    
    if (!(*invalid = (char *)DXAllocateLocal(strlen(dep) + INVLEN)))
	return ERROR;
    
    strcpy(*invalid, "invalid ");
    strcat(*invalid, dep);
    
    /* if no component, return ERROR - at the point we call this 
     *  routine, we know there are invalids.
     */
    if (!DXGetComponentValue(f, *invalid)) {
	DXSetError(ERROR_INTERNAL, 
		   "looking for %s component, which is not found", *invalid);
	DXFree((Pointer)*invalid);
	*invalid = NULL;
	return ERROR;
    }
    
    return OK;
}
    

