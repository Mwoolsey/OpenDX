/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/


#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#ifndef _DXI_VERSION_H_
#define _DXI_VERSION_H_

/* routines to return info about the product version numbers */
Error DXVersion(int *version, int *release, int *modification);
Error DXVersionString(char **name);


/* the following routines are only used when dealing with outboard 
 * modules or dynamically loaded modules.
 */

#define DXD_OUTBOARD_INTERFACE_VERSION  1
#define DXD_SYMBOL_INTERFACE_VERSION    1
#define DXD_HWDD_INTERFACE_VERSION	1

Error DXOutboardInterface(int *version);
Error DXSymbolInterface(int *version);
Error DXHwddInterface(int *version);

#endif /* _DXI_VERSION_H_ */

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
