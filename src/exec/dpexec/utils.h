/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dpexec/utils.h,v 1.9 2004/06/09 16:14:29 davidt Exp $
 */

#include <dxconfig.h>

#ifndef	_UTILS_H
#define	_UTILS_H

#include <string.h>
#include <stddef.h>
#include <dx/dx.h>
#include <dx/arch.h>

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#ifndef NULL
#define	NULL		0
#endif

#ifndef TRUE
#define TRUE		1
#define FALSE		0
#endif

char *_dxf_ExStrSave (char *old);

#define exJID (_dxd_exMyPID+1)

#define	KILO(n)			((n) << 10)
#define	MEGA(n)			((n) << 20)

/*
 * Covers for useful functions to make lint shut up about type problems
 */

#define	ExZero(p,n)	memset ((void *) (p), 0, (size_t) (n))
#define ExCopy(d,s,n)	memcpy ((void *) (d), (const void *) s, (size_t) (n))

/*
 * Make it easy to put function pointers into structures.
 */

typedef char			(*PFC)();
typedef short			(*PFS)();
typedef int			(*PFI)();
typedef int			(*PFIP)(void *);
typedef long			(*PFL)();
typedef float			(*PFF)();
typedef double			(*PFD)();
typedef void			(*PFV)();

typedef unsigned char		(*PFUC)();
typedef unsigned short		(*PFUS)();
typedef unsigned int		(*PFUI)();
typedef unsigned long		(*PFUL)();

typedef Error			(*PFE)();
typedef Pointer			(*PFP)();

/*
 * Useful functions.
 */

Pointer		_dxf_ExAlignBoundary		(long n, Pointer p);
char		*_dxf_ExCopyString		(char *old);
char		*_dxf_ExCopyStringN		(char *old, int len);
char		*_dxf_ExCopyStringLocal		(char *old);
int		_dxf_ExNextPowerOf2		(int n);
Array 		_dxfExNewInteger 		(int n);


#define LIST(type) struct {int nalloc; int nused; type *vals;}
#define INIT_LIST(list) ((list).nalloc = (list).nused = 0, (list).vals = NULL)
#define INIT_LIST_LOCAL(t, list) ( \
    (list).nalloc = (list).nused = 0, \
    (list).vals = (t *) DXAllocateLocal(1) \
    )
#define COPY_LIST(dstlist, srclist) ( \
    (dstlist).nalloc = (srclist).nalloc, (dstlist).nused = (srclist).nused, \
    (dstlist).vals = (srclist).vals \
    )
#define APPEND_LIST(t,list,val)					\
    do 								\
    {								\
	if ((list).nalloc == (list).nused) 			\
	{							\
	    (list).nalloc += 10;				\
	    (list).vals = (t *) DXReAllocate (			\
		(Pointer)((list).vals),				\
		(list).nalloc * sizeof ((list).vals[0]));	\
	    if ((list).vals == NULL) 				\
	    {							\
		_dxf_ExDie("Can't Reallocate list");			\
	    }							\
	}							\
	(list).vals[(list).nused++] = (val);			\
    } while (0)
#define GROW_LIST(t,list,num)					\
    do 								\
    {								\
        int _grow;						\
        _grow = (num) - ((list).nalloc - (list).nused); 	\
        if(_grow > 0) {						\
            (list).nalloc += _grow;				\
	    (list).vals = (t *) DXReAllocate ((Pointer)((list).vals),\
		      (list).nalloc * sizeof ((list).vals[0]));	\
	    if ((list).vals == NULL) 				\
	    {							\
	        _dxf_ExDie("Can't Grow list");			\
	    }							\
        }							\
    } while (0)
#define DELETE_LIST(t,list,ind)					\
    do 								\
    {								\
        int _i;      						\
        for(_i = ind; _i < (list).nused-1; _i++)                \
            (list).vals[_i] = (list).vals[_i+1];                \
	(list).nused--;                                         \
    } while (0)
#define INSERT_LIST(t,list,val,ind)				\
    do 								\
    {		                                                \
        int _i;      						\
	if ((list).nalloc == (list).nused) 			\
	{	 						\
	    (list).nalloc += 10;				\
	    (list).vals = (t *) DXReAllocate (			\
		(Pointer)((list).vals),				\
		(list).nalloc * sizeof ((list).vals[0]));	\
	    if ((list).vals == NULL) 				\
	    {							\
		_dxf_ExDie("Can't Reallocate list");		\
	    }							\
	}							\
        for(_i = (list).nused; _i > ind; _i--)                  \
            (list).vals[_i] = (list).vals[_i-1];                \
	(list).vals[ind] = (val);			        \
	(list).nused++;                                         \
    } while (0)
#define UPDATE_LIST(list,val,ind)                               \
    do                                                          \
    {                                                           \
        (list).vals[ind] = (val);                               \
    } while (0)
#define FETCH_LIST(list,ind) (((ind) < (list).nused)? (list).vals + (ind): 0)
#define INDEX_LIST(list,ind) ((list).vals + (ind))
#define SIZE_LIST(list) ((list).nused)
#define FREE_LIST(list) (DXFree((Pointer)((list).vals)), INIT_LIST(list))

#define	ExDebug			if (_dxd_exEnableDebug) DXDebug

#define	ExMarkTime(_m,_t)	if (_m & _dxd_exMarkMask) DXMarkTime (_t)
#define	ExMarkTimeLocal(_m,_t)	if (_m & _dxd_exMarkMask) DXMarkTimeLocal (_t)

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif	/* __EX_UTILS_H */
