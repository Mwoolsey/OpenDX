/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

INCLUDE  objectClass.X

SUBCLASS Private OF Object
IMPLEMENTS Delete

struct private {
    struct object object;		/* object preamble */
    Pointer data;			/* pointer to user's data */
    Error (*delete)(Pointer);		/* deletion routine */
};

