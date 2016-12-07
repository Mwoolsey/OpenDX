/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dpexec/exobject.h,v 1.9 2006/01/18 00:07:16 davidt Exp $
 */

#include <dxconfig.h>


#ifndef	__EXOBJECT_H
#define	__EXOBJECT_H

#include <sys/types.h>

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#if DXD_HAS_LIBIOP
extern double SVS_double_time(); /* from some systemlib */
#endif

#include "utils.h"

/*
 * Top level objects declarations.
 */

typedef enum exo_class
{
    EXO_CLASS_DELETED = 7777,			/* Always first		*/
    EXO_CLASS_FUNCTION,
    EXO_CLASS_GVAR,
    EXO_CLASS_TASK,
    EXO_CLASS_UNKNOWN				/* Always last		*/
} exo_class;

typedef struct exo_methods
{
    int			copy;			/* has copy of methods	*/
    PFIP			*methods;		/* function pointers	*/
} exo_methods;

#if DXD_HAS_LIBIOP
typedef double		EXRefTime;
#else
typedef time_t		EXRefTime;
#endif

typedef struct exo_object
{
    char		*tag;			/* object tag		*/
    exo_class		exclass;			/* top level class	*/
    int			local;			/* object in local mem.	*/
    int			refs;			/* reference count	*/
    lock_type		lock;			/* semaphore lock	*/
    exo_methods		m;			/* method structure	*/
    EXRefTime		lastref;
#ifdef LEAK_DEBUG
    struct exo_object	*DBGprev;
    struct exo_object	*DBGnext;
#endif
} exo_object;

typedef enum   exo_class 	EXO_Class;
typedef struct exo_methods	*EXO_Methods;
typedef	struct exo_object	*EXO_Object;

#define	EXO_TAG		((char *) 0x97845031)	/* object marker	*/

/*
 * DXAdd all method declarations here
 */

#define	EXO_METH_MIN		-1		/* leave as -1		*/
#define	EXO_METH_DELETE		0
#define	EXO_METH_MAX		1

/*
 * Define a macro for each method type.
 */

/*
 * External versions, perform safety checks before invoking function
 */

#define EXO_LOCK(_obj)	 ((_obj)->local ? OK : (DXlock (&(_obj)->lock, 0)))
#define EXO_UNLOCK(_obj) ((_obj)->local ? OK : (DXunlock (&(_obj)->lock, 0)))

#if DXD_HAS_LIBIOP
#define	EXO_TIMESTAMP(_obj)	((_obj)->lastref = SVS_double_time ())
#else
#define	EXO_TIMESTAMP(_obj)	((_obj)->lastref = time (0))
#endif


#define	EXO_DELETE(_obj)\
(\
    EXO_LOCK (_obj),\
    (--(_obj)->refs <= 0)\
	? (\
	    EXO_UNLOCK (_obj),\
	    EXO_METHOD_CALL (_obj, EXO_METH_DELETE),\
	    _dxf_EXO_delete(_obj)\
	  )\
	: EXO_UNLOCK (_obj)\
)


#define	EXO_REFERENCE(_obj)\
(\
    EXO_LOCK (_obj),\
    (_obj)->refs++,\
    EXO_UNLOCK (_obj),\
    (_obj)\
)

/*
 * Invokes a particular method in an objects argument vector if it exists
 */

#define	EXO_METHOD_CALL(_obj,_meth)\
((_obj)->m.methods[_meth]\
    ? (* (_obj)->m.methods[_meth]) (_obj)\
    : (_obj) != NULL)


#define EXO_GetClass(obj)\
    ((obj) ? (((EXO_Object)(obj))->exclass) : EXO_CLASS_UNKNOWN)


#define EXO_lock(_obj) 		((_obj) ? EXO_LOCK      (_obj) : OK)
#define EXO_unlock(_obj) 	((_obj) ? EXO_UNLOCK    (_obj) : OK)
#define EXO_timestamp(_obj) 	((_obj) ? EXO_TIMESTAMP (_obj) : OK)
#define	EXO_reference(_obj)	((_obj) ? EXO_REFERENCE (_obj) : (_obj))
#define EXO_delete(_obj) 	((_obj) ? EXO_DELETE    (_obj) : OK)

extern PFIP           _dxd_EXO_default_methods[]; /* from exobject.c */
    
/*
 * The default methods
 */

int			_dxf__EXO_delete	(EXO_Object obj);

/*
 * Generally useful external routines (for other objects)
 */
Error		_dxf_EXO_init			(void);
Error		_dxf_EXO_cleanup		(void);
int		_dxf_EXO_compact		(void);
int 		_dxf_EXO_delete 		(EXO_Object obj);

EXO_Object	_dxf_EXO_create_object	(EXO_Class exclass, int size,
					 PFIP *methods);
EXO_Object	_dxf_EXO_create_object_local	(EXO_Class exclass, int size,
					 PFIP *methods);

/*
 * Nicer looking defines
 */

#define	EXObj			EXO_Object

#define	ExDelete(v)		EXO_delete ((EXObj) (v))
#define	ExReference(v)		EXO_reference ((EXObj) (v))
#define	ExLock(v)		EXO_lock ((EXObj) (v))
#define	ExUnlock(v)		EXO_unlock ((EXObj) (v))
#define	ExTimestamp(v)		EXO_timestamp ((EXObj) (v))

#define	ExNonIntraLock(v)	if (! _dxd_exIntraGraphSerialize) ExLock (v)
#define	ExNonIntraUnlock(v)	if (! _dxd_exIntraGraphSerialize) ExUnlock (v)

Error _dxf_EXOCheck (EXO_Object obj);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif	/* __EXOBJECT_H */
