/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>

#ifndef _QUANT_H_
#define _QUANT_H_

typedef struct color *Color;

struct color
{
    unsigned char r, g, b;
};

typedef struct treeNode *TreeNode;

struct treeNode 
{
    int      leaf;
    int      knt;
    int	     r, g, b;
    TreeNode kids[8];
    TreeNode next;
    TreeNode last;
};

typedef struct treeNodeBuf *TreeNodeBuf;

#define ALLOC 100

struct treeNodeBuf
{
    struct treeNode nodes[ALLOC];
    TreeNodeBuf     nextbuf;
    int             nextfree;
};

#define BRANCH(color, depth, index)		 \
{						 \
        int d = 7 - depth;			 \
	index = (((color->r >> d) & 0x1) << 0) + \
		(((color->g >> d) & 0x1) << 1) + \
		(((color->b >> d) & 0x1) << 2);	 \
}


TreeNode GetTreeNode();
void FreeNode( TreeNode );
void CleanupNodes();
TreeNode InsertTree( TreeNode, Color, int );
int SearchTree( TreeNode, Color, int );
int ReduceTree();
int CountLeaves( TreeNode, int * );
TreeNode CreateTree( Pointer, int, int, int );
TreeNode AddFieldToTree(TreeNode, Pointer, int, int, int );
Error CreateTable( TreeNode, Color, int, int * );
Error CreateQuantPixels(TreeNode, Pointer, unsigned char *, int, int );


#endif /* _QUANT_H_ */
