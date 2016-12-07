/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 2002                                        */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#ifndef _LBMSGS_H_
#define _LBMSGS_H_

#if defined(DX_NATIVE_WINDOWS)

#define WM_DXMESSAGE	(WM_USER+1000)

#define DX_PACKET_AVAILABLE		1
#define DX_INTERRUPT			2
#define DX_AWAKE				4
#define DX_FAILED				8
#define DX_WAIT_FOUND			16
#define DX_QUIT					32
#define DX_SIGNAL				64
#define DX_RESUME				128

int		DXWaitForMessage(int msg);
int		DXPostMessage(DWORD threadID, int msg);
int		DXPostQuitMessage(DWORD threadID);
int		DXWaitForSignal(int n, HANDLE *sems);
void	DXReleaseAndSignal(DWORD thread, HANDLE sem);
void	DXInterrupt(DWORD thread);

#endif /* DX_NATIVE_WINDOWS */

#endif /* _LBMSGS_H_ */

