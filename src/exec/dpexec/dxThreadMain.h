/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 2004                                        */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#ifndef _DXTHREADMAIN_H_
#define _DXTHREADMAIN_H_

#pragma unmanaged 

/* This file is a version of dxmain specifically written to be a thread
 * of a main process such as the User Interface. It is needed because
 * Windows does not share the functionality that X-Windows has of being
 * able to share "Windows" between processes. Thus we must create the 
 * Executive as a thread of the master process (usually the User Interface).

 * This could be extended for use as a thread on UN*X as well, but for
 * now this is written to get the native Window's port going.
 */

/* Due to the number of global variables in the original dxmain, I've
 * decided to wrap the ThreadMain into a class that can handle all of
 * this with much better grace. 
 */

/* To use the threaded exec, create a new instance of DXThreadMain and
 * call Start().
 */
//#include <dx/dx.h>
#include <stdio.h>
#include <windows.h>
#include "sfile.h"

/* Types that would normally come from <dx/dx.h>
 * problem is, that some of the other definitions from the
 * dx headers cause conflicts - such as array, String.
 */

typedef volatile int lock_type;
typedef int Error;
typedef void *Pointer;

#define MDF_MAX  5

class __declspec(dllexport) DXExecThread {
private:
	class childThreadTable {
	public:
		HANDLE threadHandle;
		HANDLE running;
		DWORD threadID;
		int RQwrite_fd;
		int shutdown;
		HANDLE parentReady;
	};

	int numThreads;
	int numProcs;
	int largc;
	char **largv;
	char **lenvp;
	int extestport;
	int nmdfs;
	char *mdffiles[MDF_MAX];
	char extesthost[256];
	int showTrace;
	int logcache;
	int markModules;
	int showTiming;
	int savedParseAhead;
	childThreadTable *child;
	int exParent;
	int exParent_RQread_fd;
	int exParent_RQwrite_fd;
	int exChParent_RQread_fd;
	SFILE *_pIfd;
	int logerr;
	char *_pIstr;
	lock_type *childtbl_lock;
	int *exReady;
	HANDLE mainthread;

public:
	/* Methods */
	int SetupArgs(int argc, char **argv);

	// Start, starts the exec and hangs until the exec ends.
	int Start();
	int Start(int, char **);

	// StartAsThread, starts the exec and returns a handle to the thread.
	HANDLE StartAsThread();
	HANDLE StartAsThread(int, char **);
	// May want a StartAsThread with some security attributes also.

	HANDLE GetThreadHandle() { return mainthread; }

	void Terminate();
	HWND GetImageWindowHandle(const char *name);

	char **getEnvp(void);
	DXExecThread() : numThreads(1), numProcs(0), largc(0), largv(NULL),
		lenvp(NULL), extestport(-1), nmdfs(0), showTrace(FALSE), logcache(FALSE),
		markModules(FALSE), showTiming(FALSE), savedParseAhead(FALSE), child(NULL),
		exParent(FALSE), exParent_RQread_fd(0), exParent_RQwrite_fd(0), 
		exChParent_RQread_fd(0), _pIfd(NULL), logerr(0), _pIstr("stdin"),
		childtbl_lock(NULL), exReady(NULL) {};

	Error FromMasterInputHndlr(int, Pointer);
	void Cleanup(void);
	Error ChildRQMessage(int *);
	Error ParentRQMessage(void);
	void InitPromptVars();

	~DXExecThread();


private:
	void ProcessArgs(void);
	void Usage(void);
	void Initialize(void);
	void MainLoopMaster(void);
	void Settings(void);
	void Version(void);
	void Copyright(int p = 1);
	int  GetNumThreads(void);
	void SetNumThreads(int t=1);
	void ConnectInput(void);
	void MainLoopSlave(void);
	void CheckTerminate(void);
	void RegisterRQ_fds(void);
	void ParallelMaster(void);
	void LockChildpidtbl(void);
	void UpdateChildpid(int, int, int);
	void SetRQReadFromChild1(int);
	void SetRQReader(int);
	void SetRQWriter(int);
	void SigDanger(int);
	void ChildThread();
	void MainLoop();
	void KillChildren();
	void InitFailed(char *);
	int  CheckRunqueue(int);
	int  CheckGraphQueue(int);
	int  InputAvailable(SFILE *);
	void SigPipe(int);
	void SigQuit(int);
	int  OKToRead(SFILE*);
	void PromptSet(char *, char *);
	int  MyChildProc();
	int  DXWinFork();
	int  CheckInput();
	int  FindPID(int);

};

#pragma managed

#endif /* _DXTHREADMAIN_H_ */
