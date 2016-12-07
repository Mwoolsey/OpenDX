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

#ifndef _DXI_TASK_H_
#define _DXI_TASK_H_

/* TeX starts here. Do not remove this comment. */

/*
\section{Parallelism}
Task objects give the programmer a mechanism to specify a collection
of things to be done so that they can be scheduled optimally on a
multi-processor.  The task model provides a simple fork/join
semantics, suitable for large-grain parallelism.  The idea is that the
programmer begins a collection of tasks to be executed in parallel
with {\tt DXCreateTaskGroup()}, then specifies each task with {\tt
DXAddTask()}, and finally ends the task group and begins the execution
of the tasks with {\tt DXExecuteTaskGroup()}.  Waiting until all tasks
have been created before beginning their execution simplifies the
model, and allows more optimal scheduling of the tasks based on their
estimated completion time.
*/

Error DXCreateTaskGroup(void);
/**
\index{DXCreateTaskGroup}
Starts a new task group. All tasks subsequently created using {\tt
DXAddTask()}, until the matching {\tt DXExecuteTaskGroup()} is executed,
will be members of this task group.  Returns {\tt OK} on success, or
returns {\tt ERROR} and sets the error code to indicate an error.
**/

Error DXAddTask(Error(*proc)(Pointer), Pointer arg, int size, double work);
/**
\index{DXAddTask}
Adds a task to the current task group started by {\tt
DXCreateTaskGroup()}.  A task represents work to be done later.  The
task is represented by a pointer to a function to do the work ({\tt
proc}), a pointer to an argument block ({\tt arg}), the size of the
argument block ({\tt size)}, and an estimate of the amount of time the
task takes ({\tt work}).  When executed, the tasks are guaranteed to
be started in decreasing order of the work estimate.  If {\tt size} is
non-zero, a copy of the argument block is made.  Executing a task
consists of calling {\tt (*proc)(arg)}.  Returns {\tt OK} on success,
or returns {\tt ERROR} and sets the error code to indicate an error.
**/

Error DXAbortTaskGroup(void);
/**
\index{DXAbortTaskGroup}
If, in the middle of creating a task group with a {\tt
DXCreateTaskGroup()} and a series of {\tt DXAddTask()}s, an error is
discovered that requires aborting the creation of the task group, this
routine must be called must be made to release the resources taken up
by the task group and the tasks.  {\tt DXExecuteTaskGroup()} must not
subsequently be called.  Returns {\tt OK} on success, or returns {\tt
ERROR} and sets the error code to indicate an error.
**/

Error DXExecuteTaskGroup(void);
/**
\index{DXExecuteTaskGroup}
Ends the group of tasks started by the matching {\tt DXCreateTaskGroup()}.
Execution of the tasks in this group can now begin, and the {\tt
DXExecuteTaskGroup()} call waits for the completion of all tasks in this
task group.  The tasks are started in decreasing order of the work
estimate given in {\tt DXAddTask}.  Returns {\tt OK} if all the tasks in the
task group completed without error, or returns {\tt ERROR} and sets
the error code to task error code if any task returns an error.
**/

int DXProcessors(int n);
/**
\index{DXProcessors}
If {\tt n>0}, this routine sets the number of processors to be used to
{\tt n}.  In any case, it returns the previous setting for number of
processors.  {\tt DXProcessors(0)} queries the number of processors that
will be used; normally, this will have been initialized by the system
to the number of physical processors that the Data Explorer uses, or
to the number of processors allocated to the user making the call.
{\tt DXProcessors(1)} can be used to turn parallelism off (by using only
one processor). These routines can help in partitioning or planning
tasks.
**/

int DXProcessorId(void);
/**
\index{DXProcessorId}
Returns a processor identifier indicating which processor the current task
is running on.  The processor identifier is a number ranging from 0 to
$n-1$, where $n$ is the number of processors in use.
**/

/* These routines are used for calling modules from other modules */
typedef struct ModuleInput {
    char *name;
    Object value;
} ModuleInput;
typedef struct ModuleOutput {
    char *name;
    Object *value;
} ModuleOutput;

#define DXSetModuleInput(p,n,v) \
    do { ModuleInput *_p = &p; _p->name=n; _p->value = v; } while(0)
#define DXSetModuleOutput(p,n,v) \
    do { ModuleOutput *_p = &p; _p->name=n; _p->value = v; } while(0)

/* These are the new routines for setting module inputs and outputs */
/* they should replace the above defines. */
Object DXModSetFloatInput(ModuleInput *in, char *name, float f);
Object DXModSetIntegerInput(ModuleInput *in, char *name, int n);
Object DXModSetStringInput(ModuleInput *in, char *name, char *s);
void   DXModSetObjectInput(ModuleInput *in, char *name, Object obj);
void   DXModSetObjectOutput(ModuleOutput *out, char *name, Object *obj);

/* initialization routine for using DXCallModule in a standalone program */
void DXInitModules(void);

/* call a module from another module, by name, with named args */
Error DXCallModule(char*, int, ModuleInput*, int, ModuleOutput*);

/* these routines are used for async modules and for modules which
 * need unique cache tags on a per-instance basis.
 */

Error DXReadyToRun(Pointer id);
Error DXReadyToRunNoExecute(Pointer id);
Pointer DXGetModuleId(void);
Pointer DXCopyModuleId(Pointer id);
Error DXCompareModuleId(Pointer id1, Pointer id2);
Error DXFreeModuleId(Pointer id);

/* arrange a callback routine if input is detected on a socket */
Error DXRegisterInputHandler (Error (*proc) (int, Pointer), int fd, Pointer arg);
/* similar, but also add a user-definable check routine to see if there is
 * (for example) any already-buffered input
 */
Error DXRegisterInputHandlerWithCheckProc (Error (*proc) (int, Pointer),
	int (*check)(int, Pointer), int fd, Pointer arg);

int DXCheckRIH(int block);

#endif /* _DXI_TASK_H_ */

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
