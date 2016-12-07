/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 2002                                        */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>

#if defined(DX_NATIVE_WINDOWS)

#include <stdio.h>
#include <stdlib.h>
#include <dx/dx.h>

typedef struct _windowEntry *WindowEntry;
struct _windowEntry
{
	HWND hWnd;
	HANDLE thread;
	WindowEntry next;
};
static WindowRegistry;

static HANDLE windowRegistryLock;
static HANDLE allGone;

void
DXInitializeWindowRegistry()
{
	windowRegistryLock = CreateMutex(NULL, FALSE, NULL);
	allGone = CreateEvent(NULL, FALSE, FALSE, NULL);
	WindowRegistry = NULL;
}

void
DXFreeWindowRegistry()
{
	//CloseHandle(windowRegistryLock);
}

Error
DXRegisterWindow(HWND hWnd, HANDLE thread)
{
	WindowEntry we = (WindowEntry)DXAllocate(sizeof(struct _windowEntry));
	if (! we)
		return ERROR;
	
	we->hWnd = hWnd;
	we->thread = thread;

	if (DXWaitForSignal(1, &windowRegistryLock) == WAIT_TIMEOUT)
		return ERROR;

	we->next = WindowRegistry;
	WindowRegistry = we;

	ReleaseMutex(windowRegistryLock);

	return OK;
}

Error
DXUnregisterWindow(HWND hWnd)
{
	WindowEntry we, hWndList = NULL;
	
	if (DXWaitForSignal(1, &windowRegistryLock) == WAIT_TIMEOUT)
		return ERROR;

	for (we = WindowRegistry; we; we = we->next)
	{
		if (we->hWnd == hWnd)
			break;

		hWndList = we;
	}

	if (we)
	{
		if (hWndList)
			hWndList->next = we->next;
		else
			WindowRegistry = we->next;

		DXFree((Pointer)we);
	}

	if (WindowRegistry == NULL)
		SetEvent(allGone);

	ReleaseMutex(windowRegistryLock);

	if (we)
		return OK;
	else
		return ERROR;
}

Error
DXKillRegisteredWindows()
{	
	WindowEntry we, nxt;
	int knt = 0, i;
	HWND *hWndList;
	HANDLE *threadList;

	if (WindowRegistry == NULL)
		return OK;
	
	if (DXWaitForSignal(1, &windowRegistryLock) == WAIT_TIMEOUT)
		return ERROR;

	for (we = WindowRegistry; we; we = we->next)
		knt++;

	hWndList = (HWND *)DXAllocate(knt*sizeof(HWND));
	threadList = (HWND *)DXAllocate(knt*sizeof(HANDLE));

	for (we = WindowRegistry; we; we = nxt)
	{
		DestroyWindow(we->hWnd);
		nxt = we->next;
		DXFree((Pointer)we);
	}

	ReleaseMutex(windowRegistryLock);

	DestroyWindow(we->hWnd);

	if (DXWaitForSignal(knt, &threadList) == WAIT_TIMEOUT)
		return ERROR;

	DXFree((Pointer)hWndList);

	return OK;
}


#endif	

