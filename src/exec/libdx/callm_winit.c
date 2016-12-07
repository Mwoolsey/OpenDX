/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 2002                                        */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>

#if defined(DX_NATIVE_WINDOWS)

#include <dx/dx.h>
#include <windows.h>

static HINSTANCE appInstance;
static DWORD containerThreadID;
static int windowsInitialized = 0;

int
DXInitializeWindows(HINSTANCE inst, DWORD ctid)
{
	appInstance = inst;
	windowsInitialized = 1;
	containerThreadID = ctid;

	return 1;
}

DWORD
_dxfGetContainerThread()
{
	return containerThreadID;
}

Error
DXGetWindowsInstance(HINSTANCE *inst)
{
	if (! windowsInitialized)
	{
		if(!(appInstance = GetModuleHandle(NULL))) {
			DXSetError(ERROR_INTERNAL, "windows has not been initialized");
			return ERROR;
		}
		containerThreadID = GetCurrentThreadId();
	}
	*inst = appInstance;
	return OK;
}

#endif /* DX_NATIVE_WINDOWS */

