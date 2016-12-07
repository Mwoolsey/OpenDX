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
#include "config.h"
#include "nodeb.h"
#include "pmodflags.h"
#include "_macro.h"
#include "distp.h"
#include "remote.h"

extern Error _dxfByteSwap(void *dest, void *src, int ndata, Type t); /* from libdx/edfdata.c */

static char rbuffer[PTBUFSIZE];
static int SwapMsg;

Error readN(int fd, Pointer Buffer, int n, Type t)
{
    static int bytesleft = 0;
    static char *ptr;
    static int nbytes;

    nbytes = n * DXTypeSize(t);

/* The 2 _dxf_ExReceiveBuffer calls here did not include SwapMsg. Prototyping
 * revealed the coding error. However, now someone that knows needs to check
 * out the calls with _dxfByteSwap.
 */

    if(nbytes > PTBUFSIZE-sizeof(int)) {
        if(_dxf_ExReceiveBuffer(fd, Buffer, n, t, SwapMsg) < 0) {
            DXUIMessage("ERROR", "Received bad parse tree");
            return(ERROR);
        }
        else 
            return(OK);
    }

    if(bytesleft == 0) {
        if(_dxf_ExReceiveBuffer(fd, rbuffer, PTBUFSIZE, TYPE_UBYTE, SwapMsg) < 0) {
            DXUIMessage("ERROR", "Received bad parse tree");
            return(ERROR);
        }
        bcopy(rbuffer, &bytesleft, sizeof(int));
        if(SwapMsg)
            _dxfByteSwap(&bytesleft, &bytesleft, 1, TYPE_INT);
        bytesleft -= sizeof(int);
        ptr = rbuffer + sizeof(int);
    }

    bcopy(ptr, Buffer, nbytes);
    if(SwapMsg)
        _dxfByteSwap(Buffer, Buffer, n, t);

    bytesleft -= nbytes;
    ptr += nbytes;
    return(OK);
} 
 
struct node *ExReadNode(int fd);

struct node *_dxf_ExReadTree(int fd, int swap)
{
    node *pt;
    SwapMsg = swap;
    pt = ExReadNode(fd);
    return(pt);
}

struct node *ExReadNodeList(int fd)
{
    int i, numattr;
    struct node *hdr = NULL;
    struct node *curr;

    readN(fd, &numattr, 1, TYPE_INT);

    if(numattr > 0) {
        hdr = ExReadNode(fd);
        LIST_CREATE(hdr);
    }
    for(i = 1; i < numattr; i++) {
        curr = ExReadNode(fd);
        LIST_APPEND(hdr, curr);
    }
    return(hdr);
}


struct node *ExReadNode(int fd)
{
    int len;
    node *n;
    int type_code;
    _ntype type;
    struct node *function;

    readN(fd, &type_code, 1, TYPE_INT);
    type = (_ntype) type_code;
    n = _dxf_ExPCreateNode( type );
    /* first print node attributes */
    n->attr = ExReadNodeList(fd);
    n->type = type;
        
    switch (type)
    {
        case NT_MACRO:                              /* macro definition */
        case NT_MODULE:
            n->v.function.id = ExReadNodeList(fd);
            /* read next 3 ints - flags, nin, nout */
            readN(fd, &(n->v.function.flags), 3, TYPE_INT);
            n->v.function.in = ExReadNodeList(fd);
            readN(fd, &(n->v.function.varargs), 1, TYPE_INT);
            n->v.function.out = ExReadNodeList(fd); 
            if(type == NT_MACRO)
                n->v.function.def.stmt = ExReadNodeList(fd);
            else {
                if(n->v.function.flags & MODULE_OUTBOARD)
                    n->v.function.def.func = DXOutboard;
                else {
                    function =(node *)_dxf_ExMacroSearch(n->v.function.id->v.id.id);
                    if(function)
                        n->v.function.def.func = function->v.function.def.func;
                    else
                        n->v.function.def.func = NULL;
                }
            }
            /* read next 2 ints - prehidden, posthidden */
            readN(fd, &(n->v.function.prehidden), 2, TYPE_INT);
#if 0
            readN(fd, n->v.function.led, 4, TYPE_UBYTE);
#endif
            readN(fd, &(n->v.function.index), 1, TYPE_INT);
            break;
        case NT_ASSIGNMENT:
            n->v.assign.lval = ExReadNodeList(fd);
            n->v.assign.rval = ExReadNodeList(fd);
            break;
        case NT_CALL:
            n->v.call.id = ExReadNodeList(fd);
            n->v.call.arg = ExReadNodeList(fd);
            break;
        case NT_PRINT:
            readN(fd, &(n->v.print.type), 1, TYPE_INT);
            n->v.print.val = ExReadNodeList(fd); 
            break;
        case NT_ATTRIBUTE:
            n->v.attr.id = ExReadNodeList(fd);
            n->v.attr.val = ExReadNodeList(fd);
            break;
       case NT_ARGUMENT:
            n->v.arg.id = ExReadNodeList(fd);
            n->v.arg.val = ExReadNodeList(fd);
            break;
        case NT_LOGICAL:
        case NT_ARITHMETIC:
            readN(fd, &(n->v.expr.op), 1, TYPE_INT);
            n->v.expr.lhs = ExReadNodeList(fd);
            n->v.expr.rhs = ExReadNodeList(fd);
            break;
        case NT_CONSTANT:
            /* the information in this structure is enough to  */
            /* reconstruct the array in obj. I will rebuild the */
            /* array obj on the other side. */
            readN(fd, &(n->v.constant.items), 1, TYPE_INT);
            readN(fd, &(n->v.constant.type), 1, TYPE_INT);
            n->v.constant.origin = n->v.constant.delta = NULL;
            n->v.constant.obj = NULL;
            if(n->v.constant.type != TYPE_UBYTE) {
                int regular = 0;
                readN(fd, &regular, 1, TYPE_INT);
                if(regular) { 
                    if(n->v.constant.type == TYPE_FLOAT) {
                        n->v.constant.origin = 
                           (float *)DXAllocateLocal(sizeof(float));
                        if(n->v.constant.origin == NULL)
                            _dxf_ExDie("Could not allocate memory for data");
                        n->v.constant.delta = 
                           (float *)DXAllocateLocal(sizeof(float));
                        if(n->v.constant.delta == NULL)
                            _dxf_ExDie("Could not allocate memory for data");
                        readN(fd, n->v.constant.origin, 1, TYPE_FLOAT);
                        readN(fd, n->v.constant.delta, 1, TYPE_FLOAT);
                    }	
                    if(n->v.constant.type == TYPE_INT) {
                        n->v.constant.origin = 
                           (int *)DXAllocateLocal(sizeof(int));
                        if(n->v.constant.origin == NULL)
                            _dxf_ExDie("Could not allocate memory for data");
                        n->v.constant.delta = 
                           (int *)DXAllocateLocal(sizeof(int));
                        if(n->v.constant.delta == NULL)
                            _dxf_ExDie("Could not allocate memory for data");
                        readN(fd, n->v.constant.origin, 1, TYPE_INT);
                        readN(fd, n->v.constant.delta, 1, TYPE_INT);
	            }
	        }
                else {
                    readN(fd, &(n->v.constant.cat), 1, TYPE_INT);
                    readN(fd, &(n->v.constant.rank), 1, TYPE_INT);
                    if(n->v.constant.rank > 0) {
                        if(n->v.constant.rank <= LOCAL_SHAPE) {
                            n->v.constant.shape = n->v.constant.lshape;
                            n->v.constant.salloc = LOCAL_SHAPE;
                        }
                        else {
                            n->v.constant.shape = 
                                  (int *)DXAllocateLocal(
                                         n->v.constant.rank*sizeof(int));
                            if(n->v.constant.shape == NULL)
                                _dxf_ExDie("Could not allocate memory");
                            n->v.constant.salloc = n->v.constant.rank;
                        }
		     }
                        readN(fd, n->v.constant.shape, n->v.constant.rank, 
                              TYPE_INT);
		}
	    }
            if(!n->v.constant.origin) { /* not regular */
                readN(fd, &len, 1, TYPE_INT);
                n->v.constant.data = (Pointer *)DXAllocateLocal(len);
                if(n->v.constant.data == NULL)
                    _dxf_ExDie("Could not allocate memory for data");
                len /= DXTypeSize(n->v.constant.type);
                readN(fd, n->v.constant.data, len, n->v.constant.type);
            }
            /* make an array object for this node */
            _dxf_ExEvalConstant(n);
            break;
        case NT_ID:
            readN(fd, &len, 1, TYPE_INT);
            n->v.id.id = (char *)DXAllocateLocal(len);
            if(n->v.id.id == NULL)
                _dxf_ExDie("Could not allocate memory for name");
            readN(fd, n->v.id.id, len, TYPE_UBYTE);
            n->v.id.dflt = ExReadNodeList(fd);
            break;
        case NT_EXID:
            readN(fd, &len, 1, TYPE_INT);
            n->v.exid.id = (char *)DXAllocateLocal(len);
            if(n->v.exid.id == NULL)
                _dxf_ExDie("Could not allocate memory for name");
            readN(fd, n->v.exid.id, len, TYPE_UBYTE);
            break;
        case NT_BACKGROUND:
            readN(fd, &(n->v.background.type), 1, TYPE_INT);
            readN(fd, &(n->v.background.handle), 1, TYPE_INT);
            n->v.background.statement = ExReadNodeList(fd);
            break;
        case NT_PACKET:
            readN(fd, &(n->v.packet.type), 1, TYPE_INT);
            readN(fd, &(n->v.packet.number), 1, TYPE_INT);
            n->v.packet.packet = ExReadNodeList(fd);
            readN(fd, &(n->v.packet.size), 1, TYPE_INT);
#if 0
            /* I don't think this is used anymore */
            /* we'll have to worry about the type and size */
            /* of data on the other side */
            n->v.packet.data = (Pointer *)DXAllocateLocal(n->v.packet.size);
            if(n->v.packet.data == NULL)
                _dxf_ExDie("Could not allocate memory for data");
            readN(fd, n->v.packet.data, n->v.packet.size, TYPE_UBYTE);
#endif
            break;
        case NT_DATA:
            /* where and when is this type of node used? */
            /* we could have a problem since we don't know */
            /* the type of the data. (for lsb vs. msb) */
            readN(fd, &(n->v.data.len), 1, TYPE_INT);
            /* we'll have to worry about the type and size */
            /* of data on the other side */
            n->v.data.data = (Pointer *)DXAllocateLocal(n->v.data.len);
            if(n->v.data.data == NULL)
                _dxf_ExDie("Could not allocate memory for data");
            readN(fd, n->v.data.data, n->v.data.len, TYPE_UBYTE);
            break;
        default:
            DXSetError(ERROR_INTERNAL, "bad node type\n");
            DXFree(n);
            return(NULL);
    }
    return(n);
}


