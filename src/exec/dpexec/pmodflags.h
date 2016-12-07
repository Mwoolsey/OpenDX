/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dpexec/pmodflags.h,v 1.4 2000/08/11 15:28:13 davidt Exp $
 */

/*
 * these flags modify how a module is treated by the executive and the 
 * user interface.   MODULE_XXX is set in the .mdf file by putting a
 *   FLAGS XXX
 * line in the module description stanza.  the mdf2c process generates
 * user.c, which uses #defines to set bits in a single integer flag.
 * This file has to be in sync with modflags.h. This file only contains
 * the private flags that we don't want the user to see.
 */

#include <dxconfig.h>

#ifndef PMODFLAGS_H_
#define PMODFLAGS_H_

#include <dx/modflags.h>

#define __BIT(x) (1<<(x))
#if 0
/* These flags are no longer being used at all */
#define MODULE_SERIAL	   __BIT(0)  /* this module is not parallelized */
#define MODULE_SEQUENCED   __BIT(1)  /* must be executed in order */
#define MODULE_SWITCH	   __BIT(2)  /* acts as an execution switch */
#define MODULE_ASSIGN	   __BIT(4)  /* special internal executive use only! */
#define	MODULE_JOIN	   __BIT(6)  /* acts as a join point (debug only) */
#endif

/* All these flags are internal use only */
#define MODULE_OUTBOARD    __BIT(9)  /* run module as a separate executable */
#define MODULE_PASSTHRU    __BIT(15) /* internal use only */
#define MODULE_CONTAINS_STATE __BIT(17) /* internal use only */
#define MODULE_PASSBACK     __BIT(18)  /*internal use only */
#define MODULE_ASYNCNAMED   __BIT(19)  /* does this module have a rerun key*/

#endif /* PMODFLAGS_H_ */
