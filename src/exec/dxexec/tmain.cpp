/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include "../dpexec/dxThreadMain.h"
#include "../dpexec/tmainUtil.h"

#if defined(intelnt) || defined(WIN32)
#include <dx/arch.h>
#include <windows.h>
#endif

#if defined(DX_NATIVE_WINDOWS)
extern "C" int DXInitializeWindows(HINSTANCE);
#endif

#if defined(HAVE_WINSOCK_H)
#include <winsock.h>
#endif

#if defined(DX_NATIVE_WINDOWS) && defined(_WINDOWS)

/* The following creates memory that won't be freed. */
char * WideToAnsi(LPCWSTR wstr) {
	char *cvtval;
	int len = WideCharToMultiByte(CP_ACP, WC_NO_BEST_FIT_CHARS, 
		wstr, -1, NULL, 0, NULL, NULL);
	cvtval = (char *) DXAllocate (len + 1);
	len = WideCharToMultiByte(CP_ACP, WC_NO_BEST_FIT_CHARS, 
		wstr, -1, cvtval, len, NULL, NULL);
	if(!len) {
		printf("Error converting argv.\n");
	}
	return cvtval;
}

char **argv = NULL;
char **envp = NULL;
int argc = 0;
int 
WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
			LPSTR lpCmdLine, int nShowCmd)
#else

int 
main (int argc, char **argv, char **envp)
#endif
{
	DXExecThread dx;
#if defined(DX_NATIVE_WINDOWS) && defined(_WINDOWS)
	LPWSTR *wargv; int i;
#endif

	WSADATA wsadata;
	int i;

	i = WSAStartup(MAKEWORD(1,1),&wsadata);
	if (i!=0) {
		MessageBox(NULL, TEXT("Unable to initalize executive networking. Now exiting."),
			TEXT("Network Error"), MB_OK | MB_ICONERROR);
		exit (1);
	}

#if defined(DX_NATIVE_WINDOWS) && defined(_WINDOWS)
	wargv = CommandLineToArgvW( GetCommandLineW(), &argc );
	for(i=0; i < argc; i++)
		argv[i] = WideToAnsi(wargv[i]);
	if (!DXInitializeWindows(hInstance)) {
		MessageBox(NULL, TEXT("Could not initialize windows. Now exiting."),
			TEXT("DXInit Error"), MB_OK | MB_ICONERROR);
		return 0;
	}
#elif defined(DX_NATIVE_WINDOWS) && defined(_CONSOLE)
	if (!DXInitializeWindows(GetModuleHandle(NULL))) {
		MessageBox(NULL, TEXT("Could not initialize windows. Now exiting."),
			TEXT("DXInit Error"), MB_OK | MB_ICONERROR);
		return 0;
	}
#endif

	DXEnvironment dxe;
	dxe.Setup(argc, argv);

	int exargc; char **exargv = NULL;

	exargc = dxe.getExArgs(exargv);

	// Because threads can only take one variable, its easier to
	// pass stuff to the ExecThread first as setup and then actually
	// tell it to run by just passing the setup DXExecThread to the
	// thread creation function.

//	if(dxe.isExecOnly())
//		dx.Start(exargc, exargv);
//	else
		dx.StartAsThread(exargc, exargv);
	
	//return dx.Start();

	Sleep(125000);
}







