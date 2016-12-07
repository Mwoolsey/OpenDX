/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#include "instrument.h"
#include <dx/dx.h>

#ifdef INSTRUMENT
Instrumentation	*exInstrument;
int 		exDoInstrumentation = 1;
static int	instNprocs = 0;

void
ExResetInstrument(void)
{
    int i;

    for (i = 0; i < instNprocs; ++i) {
	exInstrument[i].tasks		= 0; 
	exInstrument[i].modules		= 0;
	exInstrument[i].numSlaveTry	= 0;
	exInstrument[i].numMasterTry	= 0;
    }
}

void
ExAllocateInstrument(int nprocs)
{
    instNprocs = nprocs;
    exInstrument = (Instrumentation*) DXAllocate
	(sizeof (Instrumentation) * instNprocs);
    ExResetInstrument();
}

void
ExFreeInstrument(void)
{
    DXFree ((Pointer)exInstrument);
}

void
ExPrintInstrument(void)
{
    int i;
    float taskSum;
    float moduleSum;
    int minTasks;
    int maxTasks;
    int minMods;
    int maxMods;

    if (!exDoInstrumentation)
	return;

    taskSum = 0;
    moduleSum = 0;
    maxTasks = minTasks = exInstrument[0].tasks;
    maxMods = minMods = exInstrument[0].modules;
    for (i = 0; i < instNprocs; ++i) {
	DXMessage ("#1240", i, exInstrument[i].tasks, exInstrument[i].modules);
	maxTasks = MAX(maxTasks, exInstrument[i].tasks);
	minTasks = MIN(minTasks, exInstrument[i].tasks);
	maxMods = MAX(maxMods, exInstrument[i].modules);
	minMods = MIN(minMods, exInstrument[i].modules);
	taskSum += exInstrument[i].tasks;
	moduleSum += exInstrument[i].modules;
    }
    DXMessage ("#1250", taskSum / instNprocs, minTasks, maxTasks);
    DXMessage ("#1260", moduleSum / instNprocs, minMods, maxMods);

    for (i = 0; i < instNprocs; ++i) {
	DXMessage ("#1270", i, exInstrument[i].numSlaveTry, 
	    exInstrument[i].numMasterTry);
    }
}
#endif
