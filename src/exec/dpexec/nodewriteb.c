/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#include <stdio.h>
#include <dx/dx.h>

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#include "config.h"
#include "nodeb.h"

static char wbuffer[PTBUFSIZE];

int ExWriteNode(int fd, struct node *n);

void writeBuf(int fd, Pointer Buffer, int size)
{
    static int bytesleft = PTBUFSIZE - sizeof(int);
    int avail = PTBUFSIZE - sizeof(int);
    int bytestowrite;
    static char *ptr = wbuffer + sizeof(int);
    
    /* do we need to write out buffer */
    if(size > bytesleft || Buffer == NULL) {
        /* write number of bytes actually used at beginning of buffer */
        bytestowrite = PTBUFSIZE-bytesleft;
        bcopy(&bytestowrite, wbuffer, sizeof(int));
        write(fd, wbuffer, PTBUFSIZE);
        bytesleft = avail;
        ptr = wbuffer + sizeof(int);
    }

    if(Buffer) {
        if(size > avail)
            write(fd, Buffer, size);
        else {
            bcopy(Buffer, ptr, size);
            ptr += size;
            bytesleft -= size;
        }
    }
}
        
int _dxf_ExWriteTree(struct node *pt, int fd)
{
    ExWriteNode(fd, pt);
    writeBuf(fd, NULL, 0);
    return (0);
}

int ExNodeListCheck(int fd, struct node *n)
{
    int numnodes = 0;

    for(; n; n = n->next)
        numnodes++;
    writeBuf(fd, &numnodes, sizeof(int));
    return (0);
}

int ExWriteNodeList(int fd, struct node *n)
{
    ExNodeListCheck(fd, n);
    ExWriteNode(fd, n);
    return (0);
}


int ExWriteNode(int fd, struct node *n)
{
    int len;

    for(; n; n = n->next) {
        writeBuf(fd, &(n->type), sizeof(int));
        /* first print node attributes */
        ExWriteNodeList(fd, n->attr);
        
        switch (n->type)
        {
            case NT_MACRO:                              /* macro definition */
            case NT_MODULE:
                ExWriteNodeList(fd, n->v.function.id);
                /* write next 3 ints - flags, nin, nout */
                writeBuf(fd, &(n->v.function.flags), 3*sizeof(int));
                ExWriteNodeList(fd, n->v.function.in);
                writeBuf(fd, &(n->v.function.varargs), sizeof(int));
                ExWriteNodeList(fd, n->v.function.out); 
                if(n->type == NT_MACRO)
                    ExWriteNodeList(fd, n->v.function.def.stmt);
                /* skipping function pointer, need to recreate on other side */
                /* write next 2 ints - prehidden, posthidden */
                writeBuf(fd, &(n->v.function.prehidden), 2*sizeof(int));
#if 0
                writeBuf(fd, n->v.function.led, 4*sizeof(char));
#endif
                writeBuf(fd, &(n->v.function.index), sizeof(int));
                break;
            case NT_ASSIGNMENT:
                ExWriteNodeList(fd, n->v.assign.lval);
                ExWriteNodeList(fd, n->v.assign.rval);
                break;
            case NT_CALL:
                ExWriteNodeList(fd, n->v.call.id);
                ExWriteNodeList(fd, n->v.call.arg);
                break;
            case NT_PRINT:
                writeBuf(fd, &(n->v.print.type), sizeof(_ptype));
                ExWriteNodeList(fd, n->v.print.val); 
                break;
            case NT_ATTRIBUTE:
                ExWriteNodeList(fd, n->v.attr.id);
                ExWriteNodeList(fd, n->v.attr.val);
                break;
           case NT_ARGUMENT:
                ExWriteNodeList(fd, n->v.arg.id);
                ExWriteNodeList(fd, n->v.arg.val);
                break;
            case NT_LOGICAL:
            case NT_ARITHMETIC:
                writeBuf(fd, &(n->v.expr.op), sizeof(_op));
                ExWriteNodeList(fd, n->v.expr.lhs);
                ExWriteNodeList(fd, n->v.expr.rhs);
                break;
            case NT_CONSTANT:
                /* the information in this structure is enough to  */
                /* reconstruct the array in obj. I will rebuild the */
                /* array obj on the other side. */
                writeBuf(fd, &(n->v.constant.items), sizeof(int));
                writeBuf(fd, &(n->v.constant.type), sizeof(Type));
                if(n->v.constant.type != TYPE_UBYTE) {
                    int regular = 0;
                    if(n->v.constant.origin && n->v.constant.delta) {
                        regular = 1;
                        writeBuf(fd, &regular, sizeof(int));
                        if(n->v.constant.type == TYPE_FLOAT) {
                            writeBuf(fd, n->v.constant.origin, sizeof(float));
                            writeBuf(fd, n->v.constant.delta, sizeof(float));
			}
                        if(n->v.constant.type == TYPE_INT) {
                            writeBuf(fd, n->v.constant.origin, sizeof(int));
                            writeBuf(fd, n->v.constant.delta, sizeof(int));
			}
                        break;
		    }
                    else {
                        int j;
                        writeBuf(fd, &regular, sizeof(int));
                        writeBuf(fd, &(n->v.constant.cat), sizeof(Category));
                        writeBuf(fd, &(n->v.constant.rank), sizeof(int));
                        if(n->v.constant.rank > 0)
                            writeBuf(fd, n->v.constant.shape, 
                                    sizeof(int)*n->v.constant.rank);
                        len = DXTypeSize(n->v.constant.type) * 
                                 DXCategorySize(n->v.constant.cat) *
                                 n->v.constant.items;
                        for (j=0; j<n->v.constant.rank; j++)
                            len *= n->v.constant.shape[j];
		    }
		}
                else
                    len = n->v.constant.items;
                writeBuf(fd, &len, sizeof(int));
                writeBuf(fd, n->v.constant.data, len);
                break;
            case NT_ID:
                len = strlen(n->v.id.id)+1;
                writeBuf(fd, &len, sizeof(int));
                writeBuf(fd, n->v.id.id, sizeof(char)*len);
                ExWriteNodeList(fd, n->v.id.dflt);
                break;
            case NT_EXID:
                len = strlen(n->v.exid.id)+1;
                writeBuf(fd, &len, sizeof(int));
                writeBuf(fd, n->v.exid.id, sizeof(char)*len);
                break;
            case NT_BACKGROUND:
                writeBuf(fd, &(n->v.background.type), sizeof(_bg));
                writeBuf(fd, &(n->v.background.handle), sizeof(int));
                ExWriteNodeList(fd, n->v.background.statement);
                break;
            case NT_PACKET:
                writeBuf(fd, &(n->v.packet.type), sizeof(_pack));
                writeBuf(fd, &(n->v.packet.number), sizeof(int));
                ExWriteNodeList(fd, n->v.packet.packet);
                writeBuf(fd, &(n->v.packet.size), sizeof(int));
#if 0
                /* I don't think this is used anymore */
                /* we'll have to worry about the type and size */
                /* of data on the other side */
                writeBuf(fd, n->v.packet.data, n->v.packet.size);
#endif
                break;
            case NT_DATA:
                /* where and when is this type of node used? */
                /* we could have a problem since we don't know */
                /* the type of the data. (for lsb vs. msb) */
                writeBuf(fd, &(n->v.data.len), sizeof(int));
                /* we'll have to worry about the type and size */
                /* of data on the other side */
                writeBuf(fd, n->v.data.data, n->v.data.len);
                break;
        }
    }
    return (0);
}


