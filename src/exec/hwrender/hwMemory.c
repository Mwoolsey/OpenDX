/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#ifndef hwMemory_h
#define hwMemory_h
/*---------------------------------------------------------------------------*\
 $Source: /src/master/dx/src/exec/hwrender/hwMemory.c,v $
  Author: Mark Hood

  This include file defines core data structures used by the direct
  interactor implementations.

 $Log: hwMemory.c,v $
 Revision 1.4  2006/05/17 14:36:44  davidt
 Remove word confidential in comments

 Revision 1.3  1999/05/10 15:45:35  gda
 Copyright message

 Revision 1.3  1999/05/10 15:45:35  gda
 Copyright message

 Revision 1.2  1999/05/03 14:06:39  gda
 moved to using dxconfig.h rather than command-line defines

 Revision 1.1.1.1  1999/03/24 15:18:33  gda
 Initial CVS Version

 Revision 1.1.1.1  1999/03/19 20:59:48  gda
 Initial CVS

 Revision 10.1  1999/02/24 20:36:40  gda
 OpenDX Baseline

 Revision 9.1  1997/05/22 20:52:00  svs
 Copy of release 3.1.4 code

 * Revision 8.0  1995/10/03  20:01:02  nsc
 * Copy of release 3.1 code
 *
 * Revision 7.2  94/03/31  11:25:20  tjm
 * added copyright notice
 * 
 * Revision 7.1  94/01/18  18:59:11  svs
 * changes since release 2.0.1
 * 
 * Revision 6.2  93/11/23  10:44:23  tjm
 * Added tdm memory tracing, purged memory leaks, fixed Deletion of Neighbors
 * component to often
 * 
 * Revision 1.2  93/07/06  15:26:30  tjm
 * *** empty log message ***
 * 
\*---------------------------------------------------------------------------*/



#include <stdio.h>
#include "hwDeclarations.h"
#include "hwMemory.h"




#ifdef TDM_MEMORY_TRACING

#define CHECKTABLEATALLOC 1

#define	BLOCKSUMMARY	0x1
#define ALLBLOCKS	0x2

#define FLAG (Flag)(0xDEADBEEF)
#define UNUSED -1
#define TABLESIZE	20000

typedef struct allocRecordS {
  int		line;
  char*		file;
  int		size;
  char*		adr;
} allocRecordT,*allocRecordP;

typedef int	FlagT;
typedef FlagT	*Flag;

static allocRecordT table[TABLESIZE];
static int next_avail = 0;

static void
_printAllocRecord(int i)
{
  printf("%s[line:%d]: 0x%x(%d bytes)\n",
	 table[i].file,
	 table[i].line,
	 table[i].adr,
	 table[i].size
	 );
}

static Pointer
_addAllocRecord(char* adr, int size, char* file, int line)
{
  Flag	ptr;
  int	i;

  if (next_avail == TABLESIZE) {
    printf("Warning: alloc/free table full.\n");
    return NULL;
  }

#ifdef CHECKTABLEATALLOC
  _tdmCheckAllocTable(0);
#endif

  for(i=0;i<next_avail;i++) {
    if (adr == table[i].adr) {
      printf("adr: 0x%x, (line %d, file %s) already allocated at:\n",
	     adr,line,file);
      _printAllocRecord(i);
      return NULL;
    }
  }

  table[next_avail].adr = adr;
  table[next_avail].line = line;
  table[next_avail].file = file;
  table[next_avail].size = size;
  next_avail++;

  ptr = (Flag)adr;
  adr += sizeof(FlagT);
  *ptr++ = FLAG;
  ptr = (Flag)((char*)ptr+size);
  *ptr = FLAG;

  return (Pointer)adr;
}

#define CHECKBLOCK(i) \
{							\
      Flag ptr = (Flag)table[i].adr;			\
      if(*ptr != FLAG) {			       	\
	printf ("Corrupted START flag for alloc:\n");	\
	_printAllocRecord(i);				\
	return NULL;					\
      }							\
      ptr++;						\
      ptr = (Flag)((char*)ptr+table[i].size);		\
      if(*ptr != FLAG) {				\
	printf ("Corrupted END flag for alloc:\n");	\
	_printAllocRecord(i);				\
	return NULL;					\
      }							\
}

static Error
_removeAllocRecord(char* adr, char* file, int line,char* function)
{
  int	i;
  Flag	ptr;

  adr -= sizeof(FlagT);

  for(i=0;i<next_avail;i++) {
    if(adr == table[i].adr) {
      CHECKBLOCK(i);
      table[i] = table[--next_avail];
      return adr;
    }
  }
  printf("alloc record not found:");
  printf("%s[line:%d]: %s 0x%x\n",
	 file,
	 line,
	 function,
	 adr
	 );
  return NULL;
}
  
Error
_tdmCheckAllocTable(int flags )
{
  int	i;
  int	*ptr;

  if(flags & BLOCKSUMMARY)
    printf("%d blocks still allocated\n",next_avail);

  if(flags & ALLBLOCKS)
    for(i=0;i<next_avail;i++)
      _printAllocRecord(i);
  
  for(i=0;i<next_avail;i++){	
    CHECKBLOCK(i);
  }
  return i == next_avail ? OK : ERROR ;
}
#define ALLOCSIZE(n) (((n+3) & ~0x3) + 2 * sizeof(FlagT))

Pointer
_tdmAllocate(unsigned int n, char * file, int LINE)
  {
  Pointer r;
  r = DXAllocate(ALLOCSIZE(n));
  return _addAllocRecord(r,n,file,LINE);
  }


Pointer
_tdmAllocateZero(unsigned int n, char * file, int LINE)
  {
  Pointer r;
  r = DXAllocateZero(ALLOCSIZE(n));
  return _addAllocRecord(r,n,file,LINE);
  }

Pointer
_tdmAllocateLocal(unsigned int n, char * file, int LINE)
  { 
  Pointer r;
  r = DXAllocateLocal(ALLOCSIZE(n));
  return _addAllocRecord(r,n,file,LINE);
  }

Pointer
_tdmAllocateLocalZero(unsigned int n, char * file, int LINE)
  {
  Pointer r;
  r = DXAllocateLocalZero(ALLOCSIZE(n));
  return _addAllocRecord(r,n,file,LINE);
  }

Pointer
_tdmReAllocate(Pointer x, unsigned int n, char * file, int LINE)
  {
  Pointer r;
  Pointer x1;

  if(!(x1 = _removeAllocRecord(x,file,LINE,"_tdmReAllocate")))
    return NULL;
  r = DXReAllocate(x1,ALLOCSIZE(n));
  return _addAllocRecord(r,n,file,LINE);
  }

Error
_tdmFree(Pointer x, char * file, int LINE)
  {
  Error r;
  Pointer x1;

  if(!x) return NULL;
  if(!(x1 = _removeAllocRecord(x,file,LINE,"_tdmFree"))) return NULL;
  return DXFree(x1);
  }

#endif

#endif /* hwMemory_h */
