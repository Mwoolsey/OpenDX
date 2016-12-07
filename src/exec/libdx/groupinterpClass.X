/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

//
// $Header: /src/master/dx/src/exec/libdx/groupinterpClass.X,v 1.3 2003/07/11 05:50:42 davidt Exp $
//


typedef struct groupinterpolator *GroupInterpolator;

INCLUDE  interpClass.X

SUBCLASS    GroupInterpolator     OF Interpolator
IMPLEMENTS Interpolate Delete Copy LocalizeInterpolator

GroupInterpolator NewGroupInterpolator(Group, enum interp_init, float fuzz);

struct groupinterpolator
{
    struct interpolator	interpolator;
    int			nMembers;
    Interpolator	*subInterp;
    Interpolator	hint;
};

GroupInterpolator _dxfNewGroupInterpolator(Group,
			enum interp_init, float, Matrix *);

GroupInterpolator _dxf_NewGroupInterpolator(Group, enum interp_init,
			float, Matrix *, struct groupinterpolator_class *);

GroupInterpolator _dxf_CopyGroupInterpolator(GroupInterpolator,
			GroupInterpolator, enum _dxd_copy copy);
