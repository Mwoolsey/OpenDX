/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>

#ifndef __COMPUTE_H_
#define __COMPUTE_H_

#define NT_ERROR	(-1)
#define NT_INPUT	0
#define NT_CONSTANT	1
#define NT_CONSTRUCT	2
#define NT_COND		4
#define NT_TOP		5

#define OPER_PLUS	6
#define OPER_MINUS	7
#define OPER_MUL	8
#define OPER_DIV	9
#define OPER_MOD	10
#define OPER_CROSS	11
#define OPER_DOT	12
#define OPER_LT		13
#define OPER_LE		14
#define OPER_GT		15
#define OPER_GE		16
#define OPER_EQ		17
#define OPER_NE		18
#define OPER_AND	19
#define OPER_OR		20
#define OPER_EXP	21
#define OPER_PERIOD	22
#define OPER_NOT	23
#define OPER_ASSIGNMENT	24
#define OPER_VARIABLE	25

#define LAST_OPER	25
/* _compoper.c defines other operators starting at LAST_OPER+1 */


typedef struct {
    int  	items;
    Type 	type;
    Category 	category;
    int 	rank;
    int 	shape[32];
    int		id;
} MetaType;

typedef struct PTreeNode PTreeNode;
typedef struct ObjStruct ObjStruct;
typedef int (*CompFuncV)(PTreeNode*, ObjStruct*, 
			 int, Pointer*, Pointer,
			 InvalidComponentHandle*, InvalidComponentHandle);

struct PTreeNode {
    struct PTreeNode 	*next;		/* Parent's List */
    int  		oper;		/* Type of this node */
    char 		*operName;	/* Name of this function */
    CompFuncV 		impl;		/* Routine to implement this node */
    struct PTreeNode 	*args;		/* My arguments (NULL if none) */
    MetaType		metaType;
    union {
	float	f;
	int	i;
	double	d;
#define MAX_CA_STRING 512
	char	s[MAX_CA_STRING];
    } 			u;		/* Constant data */
};

#define MAX_INPUTS	21

typedef struct {
    int used;
    Class class;
    Object object;
} CompInput;


/* ObjStruct holds the definition of the input structures.
 * If the class is CLASS_GROUP, members contains a list of 
 * group members (linked by next).  Objects contains a list of
 * elements paired up for processing.  If GetObjectClass(output) == 
 * CLASS_ARRAY, then output isn't a copy, and when we know the type 
 * (shape, ...) then we should create a new one of the appropriate type.
 */
struct ObjStruct {
    Class	 class;
    Class	 subClass;
    Object 	 output;
    Object 	 inputs[MAX_INPUTS];
    PTreeNode	 *parseTree;
    MetaType 	 metaType;
    struct ObjStruct *members;
    struct ObjStruct *next;
};


#define OPRL_INIT(node, v)   ((node)->args = (v))
#define OPRL_INSERT(node, v) ((v)->next = (node)->args, (node)->args = (v))

extern PTreeNode   *_dxdcomputeTree;
extern CompInput  _dxdcomputeInput[];


/* from _compinput.c */
void	  _dxfComputeInitInputs(CompInput *);
ObjStruct *_dxfComputeGetInputs(Object *, CompInput *);
void	  _dxfComputeDumpInputs(CompInput *, ObjStruct *);
void      _dxfComputeFreeObjStruct (ObjStruct *);


/* from _compexec.c */
int	_dxfComputeExecute (PTreeNode *, ObjStruct *);

/* from compute.c */
int m_Compute(Object *, Object *);



/* #define COMP_DEBUG 1 */

#endif /* __COMPUTE_H_ */
