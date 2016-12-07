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

#ifndef _DXI_PENDING_H_
#define _DXI_PENDING_H_

Error
DXSetPendingCmd(char *, char *, int (*)(Private), Private);

#endif /* _DXI_PENDING_H_ */

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
