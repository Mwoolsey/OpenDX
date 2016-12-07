/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#if !defined(_PLOCK_H_)
#define _PLOCK_H_

int  PLockInit();
int  PLockAllocateLock(int);
void PLockFreeLock(int);
int  PLockLock(int);
int  PLockUnlock(int);
int  PLockTryLock(int);
int  PLockAllocateWait();
void PLockFreeWait(int);
int  PLockWait(int, int);
int  PLockSignal(int);
int  PLockSnd(int);
int  PLockSndBlock(int);
int  PLockScv(int *);
int  PLockScvBlock(int *);

#endif
