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

#ifndef _DXI_LEXICAL_H_
#define _DXI_LEXICAL_H_

/*
 * Lexical vector operations
 */

#define X(x,y) x/**/y


/*
 * XYZ
 */

#define AddXYZ(A,B,C) (		\
    X(A,x) = X(B,x) + X(C,x),	\
    X(A,y) = X(B,y) + X(C,y),	\
    X(A,z) = X(B,z) + X(C,z))

#define SubXYZ(A,B,C) (		\
    X(A,x) = X(B,x) - X(C,x),	\
    X(A,y) = X(B,y) - X(C,y),	\
    X(A,z) = X(B,z) - X(C,z))

#define MulXYZ(A,B,C) (		\
    X(A,x) = X(B,x) * C,	\
    X(A,y) = X(B,y) * C,	\
    X(A,z) = X(B,z) * C)

#define DivXYZ(A,B,C) (		\
    X(A,x) = X(B,x) / C,	\
    X(A,y) = X(B,y) / C,	\
    X(A,z) = X(B,z) / C)

#define CopyXYZ(A,B)  (		\
    X(A,x) = X(B,x),		\
    X(A,y) = X(B,y),		\
    X(A,z) = X(B,z))

/*
 * XZ
 */

#define AddXZ(A,B,C) (		\
    X(A,x) = X(B,x) + X(C,x),	\
    X(A,z) = X(B,z) + X(C,z))

#define SubXZ(A,B,C) (		\
    X(A,x) = X(B,x) - X(C,x),	\
    X(A,z) = X(B,z) - X(C,z))

#define MulXZ(A,B,C) (		\
    X(A,x) = X(B,x) * C,	\
    X(A,z) = X(B,z) * C)

#define DivXZ(A,B,C) (		\
    X(A,x) = X(B,x) / C,	\
    X(A,z) = X(B,z) / C)

#define MulZ(A,B,C) (		\
    X(A,z) = X(B,z) * C)

/*
 * DXRGB
 */

#define AddRGB(A,B,C) (		\
    X(A,r) = X(B,r) + X(C,r),	\
    X(A,g) = X(B,g) + X(C,g),	\
    X(A,b) = X(B,b) + X(C,b))

#define SubRGB(A,B,C) (		\
    X(A,r) = X(B,r) - X(C,r),	\
    X(A,g) = X(B,g) - X(C,g),	\
    X(A,b) = X(B,b) - X(C,b))

#define MulRGB(A,B,C) (		\
    X(A,r) = X(B,r) * C,	\
    X(A,g) = X(B,g) * C,	\
    X(A,b) = X(B,b) * C)

#define DivRGB(A,B,C) (		\
    X(A,r) = X(B,r) / C,	\
    X(A,g) = X(B,g) / C,	\
    X(A,b) = X(B,b) / C)

#define CopyRGB(A,B)  (		\
    X(A,r) = X(B,r),		\
    X(A,g) = X(B,g),		\
    X(A,b) = X(B,b))

#define LerpRGB(A,t,B,C) (			\
    _t  =  1 - t,				\
    X(A,r)  =  X(B,r)*t  +  X(C,r)*_t,		\
    X(A,g)  =  X(B,g)*t  +  X(C,g)*_t,		\
    X(A,b)  =  X(B,b)*t  +  X(C,b)*_t)		\


/*
 * RGBO
 */

#define AddRGBO(A,B,C) (	\
    X(A,r) = X(B,r) + X(C,r),	\
    X(A,g) = X(B,g) + X(C,g),	\
    X(A,b) = X(B,b) + X(C,b),	\
    X(A,o) = X(B,o) + X(C,o))

#define SubRGBO(A,B,C) (	\
    X(A,r) = X(B,r) - X(C,r),	\
    X(A,g) = X(B,g) - X(C,g),	\
    X(A,b) = X(B,b) - X(C,b),	\
    X(A,o) = X(B,o) - X(C,o))

#define MulRGBO(A,B,C) ( 	\
    X(A,r) = X(B,r) * C,	\
    X(A,g) = X(B,g) * C,	\
    X(A,b) = X(B,b) * C,	\
    X(A,o) = X(B,o) * C)

#define DivRGBO(A,B,C) (	\
    X(A,r) = X(B,r) / C,	\
    X(A,g) = X(B,g) / C,	\
    X(A,b) = X(B,b) / C,	\
    X(A,o) = X(B,o) / C)

#define CopyRGBO(A,B)  (	\
    X(A,r) = X(B,r),		\
    X(A,g) = X(B,g),		\
    X(A,b) = X(B,b),		\
    X(A,o) = X(B,o))

#endif /* _DXI_LEXICAL_H_ */

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
