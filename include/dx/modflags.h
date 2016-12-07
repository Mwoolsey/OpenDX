/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * these flags modify how a module is treated by the executive and the 
 * user interface.   MODULE_XXX is set in the .mdf file by putting a
 *   FLAGS XXX
 * line in the module description stanza.  the mdf2c process generates
 * user.c, which uses #defines to set bits in a single integer flag.
 * Check in pmodflags.h before adding another flag here. They use
 * the same bits. pmodflags.h is a private internal use only file.
 */

#ifndef _DXI_MODFLAGS_H_
#define _DXI_MODFLAGS_H_

#define __BIT(x) (1<<(x))
#define MODULE_PIN	   __BIT(3)  /* pin module to a single processor */
#define MODULE_SIDE_EFFECT __BIT(5)  /* module has a side effect */
#define	MODULE_ERR_CONT    __BIT(7)  /* run even if an input has an error */
#define MODULE_PERSISTENT  __BIT(10) /* outboard doesn't exit after each run*/
#define MODULE_ASYNC       __BIT(11) /* can generate outputs asynchronously */

/* the following are internal use only */
#define	MODULE_REROUTABLE  __BIT(8)  /* has reroutable inputs (e.g. DDI's) */
#define MODULE_REACH       __BIT(12) /* run if would be run cache last */
#define MODULE_LOOP        __BIT(13) /* special module for looping */
#define MODULE_ASYNCLOCAL  __BIT(14) /* can generate local outputs asynchronously */
#define MODULE_CHANGES_STATE __BIT(16) /* internal use only */

#endif /* _DXI_MODFLAGS_H_ */
