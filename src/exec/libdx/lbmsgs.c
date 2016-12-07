/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>

#if defined(DX_NATIVE_WINDOWS)

#include <windows.h>
#include "lbmsgs.h"

void
DXReleaseAndSignal(DWORD thread, HANDLE sem)
{
	ReleaseMutex(sem);
	DXPostMessage(thread, DX_SIGNAL);
}

int
DXWaitForSignal(int n, HANDLE *sems)
{		
	MSG msg;
	int found = (WaitForMultipleObjects(n, sems, FALSE, 0) != WAIT_TIMEOUT);
	if (found)
		return found;

	while(found == WAIT_TIMEOUT && GetMessage(&msg, NULL, 0, 0))
	{
		if (msg.message == WM_DXMESSAGE)
		{
			if (msg.wParam == DX_SIGNAL)
				found = WaitForMultipleObjects(n, sems, FALSE, 0);
			if (msg.wParam == DX_INTERRUPT)
				return WAIT_TIMEOUT;
		}
		else
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);

			found = WaitForMultipleObjects(n, sems, FALSE, 0);
		}
	}
}

int
DXWaitForMessage(int dxmsg)
{
	MSG msg;
	int r;

	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (msg.message == WM_DXMESSAGE)
		{
			if (msg.wParam & dxmsg)
			{
				r = msg.wParam;
				break;
			}
			else if (msg.wParam == DX_INTERRUPT)
			{
				r = 0;
				break;
			}
			else if (msg.wParam == DX_SIGNAL)
			{
				r = 0;
				break;
			}
			else if (msg.wParam == DX_QUIT)
			{
				r = 0;
				break;
			}
		}
		else
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return r;
}

int 
DXPostQuitMessage(DWORD threadID)
{
	return DXPostMessage(threadID, DX_QUIT);

	return 1;
}

int 
DXPostMessage(DWORD threadID, int msg)
{
	PostThreadMessage(threadID, WM_DXMESSAGE, msg, 0);

	return 1;
}

void	
DXInterrupt(DWORD threadID)
{
	DXPostMessage(threadID, DX_INTERRUPT);
}

#endif
