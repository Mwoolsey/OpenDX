/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dxmods/arrange.c,v 1.6 2003/07/11 05:50:34 davidt Exp $
 */

#include <dxconfig.h>



/* The positioning Algorithm has been enhanced.
 *
 * The arrangement can be compact or not in either x or y.
 * If compact.x != 0 then the width of each column of images will be
 * equal to the maximum width of all images IN THAT COLUMN.  Similarly
 * for compact.y != 0.  If compact.x = 0, the width in each column will
 * be the maximum width of all images.
 *
 * Position specifies the location of the center of each image in its new
 * frame.  Thus [0.5 0.5] will center each, and [0 0] will put them in
 * the lower left.  The new location is adjusted toward the center if
 * there is any overhang in each frame.
 *
 * Size overrides the arrangement specification and forces the width
 * or height (if either x or y is !0) to be that specified. 
 *
 * If the array of images does not span to the edge of the frame, 
 * and either size is not zero or comact is zero, dummy 1 pixel corner
 * images will be added to provide the effect of padding around
 * the edge of the image array.
 * 
 * Handling of Composite Fields adds a certain amount of complexity:
 * Essentially, a new position is given to Fields and Composite Fields
 * only.  Members of a composite field all have their origins adjusted
 * with respect to the sum of all members of that and are not made to
 * be rearranged amongst themselves.
 */

#include <string.h>
#include <dx/dx.h>
#include "_helper_jea.h"

#define ARRANGE_INF  2147483647

static Object _arrange ( CompositeField parent,
                         Object inp,
                         int nrows,
                         int ncols,
                         int *count,
                         int *binx,
                         int *biny,
			 Vector c,
			 Vector p,
			 Vector s,
                         int transposed );

static Error _get_bins(Object I_group, int num_images, int nrows, int ncols,
    		            Vector c, Vector p, Vector s, int *binx, int *biny);

static Error _get_all_image_bounds(Object o, int n, 
					int *count, int ncols, int *binx, int *biny);

static Error _get_new_origin(Object inp, int *origin, int count, int ncols,
				 int *binx, int *biny, Vector p);

static
Error
getvec(Object o, Vector *v)
{
    int rc = DXExtractParameter(o, TYPE_FLOAT, 3, 1, (Pointer)v)
          || DXExtractParameter(o, TYPE_FLOAT, 2, 1, (Pointer)v);
    v->z = 0;
    return rc;
}

Error
m_Arrange ( Object *in, Object *out )
{
#define  I_group       in[0]
#define  I_horizontal  in[1]
#define  I_compact     in[2]
#define  I_position    in[3]
#define  I_size        in[4]
#define  O_image       out[0]

    int limit;
    int count;
    int offset;
    int min_x;
    int min_y;
    int max_x;
    int max_y;
    int num_images;
    int transposed;
    Vector p,c,s;
    int nrows,ncols;
    int *binx, *biny;

    O_image = NULL;
    binx = NULL;
    biny = NULL;
    
    if ( !I_group )
        DXErrorGoto2 ( ERROR_MISSING_DATA, "#10000", "'group' parameter" )

    else if ( ( DXGetObjectClass ( I_group ) != CLASS_FIELD ) &&
              ( DXGetObjectClass ( I_group ) != CLASS_GROUP )   )
        DXErrorGoto2
            ( ERROR_BAD_PARAMETER, "#10190", /* %s must be a field or a group */
              "\'group\' parameter" )

    if ( I_horizontal )
    {
        if ( !DXExtractInteger ( I_horizontal, &limit )
             ||
             ( limit < 1 )
             ||
             ( limit > ARRANGE_INF ) )
            DXErrorGoto2
                ( ERROR_BAD_PARAMETER, "#10020", "'horizontal'" );
    }
    else
        limit = ARRANGE_INF;

    num_images = 0;
    if ( !_dxf_CountImages ( I_group, &num_images ) )
        goto error;

    if ( num_images == 0 )
    {
        if ( ERROR == ( O_image = (Object)DXNewCompositeField() ) )
            goto error;

        return OK;
    }

#if 0
    else if ( ( num_images == 1 )
              &&
              ( ( DXGetObjectClass ( I_group ) == CLASS_FIELD )
                ||
                ( ( DXGetObjectClass ( I_group ) == CLASS_GROUP )
                  &&
                  ( DXGetGroupClass ( (Group)I_group ) == CLASS_COMPOSITEFIELD )
                ) ) )
    {
        O_image = I_group;

        return OK;
    }
#endif

    if (limit>num_images)
        limit = ARRANGE_INF;

    c = DXPt(0,0,0);
    if (I_compact && !getvec(I_compact,&c)) {
        DXSetError(ERROR_BAD_PARAMETER, "10221");
        return ERROR;
    }

    p = DXPt(0.5,0.5,0);
    if (I_position && !getvec(I_position,&p)) {
        DXSetError(ERROR_BAD_PARAMETER, "10221");
        return ERROR;
    }

    s = DXPt(0,0,0);
    if (I_size && !getvec(I_size,&s)) {
        DXSetError(ERROR_BAD_PARAMETER, "10221");
        return ERROR;
    }

    if ( !DXGetImageBounds   ( I_group, &min_x, &min_y, &max_x, &max_y )
         ||
         !_dxf_CheckImageDeltas ( I_group, &transposed ) )
        goto error;

    DXDebug ( "A",
              "Arrange FYI: Image In  =([%d,%d], w,h=[%d,%d]),  count (%d)",
              min_x, min_y, max_x, max_y, num_images );
   
    if ( transposed == 1 )
        DXErrorGoto
            ( ERROR_NOT_IMPLEMENTED, "#11712" )
            /* transposed images not supported,... deltas are {[0,1],[1,0]} */

    offset = ( (unsigned int) num_images + (unsigned int)(limit-1) )
               /
               (unsigned int)limit;

    nrows = offset;
    ncols = limit;
    if (ncols>num_images)
        ncols=num_images;

    if (s.x && s.x < max_x) {
	s.x = max_x;
	DXWarning("#11713", "x", max_x);
    }

    if (s.y && s.y < max_y) {
	s.y = max_y;
	DXWarning("#11713", "y", max_y);
    }

    binx = DXAllocateLocalZero((ncols+1)*sizeof(int));
    biny = DXAllocateLocalZero((nrows+1)*sizeof(int));
    if (ERROR == _get_bins(I_group, num_images, nrows, ncols, c, p, s, binx, biny)) {
        DXSetError(ERROR_NO_MEMORY, "8360");
        return ERROR;
    }

    count       = 0;
    num_images  = 0;

    if ( ( ERROR == ( O_image = _arrange
                                   ( NULL, I_group,
                                     nrows,
                                     ncols,
                                     &count,
                                     binx, biny, c, p, s, transposed ) ) )
         ||
         ( ERROR == ( O_image = _dxf_FlattenHierarchy ( O_image ) ) )
         ||
         !DXGetImageBounds ( O_image, &min_x, &min_y, &max_x, &max_y )
         ||
         !_dxf_CountImages ( O_image, &num_images ) )
        goto error;

    /* Now we add dummy corner images to force padding around the border, */
    /* if the images don't span the region and they're not to be compact. */

    if ((!c.x || !c.y || s.x || s.y) && (min_x || min_y)) {
        Field LL = DXMakeImage(1,1);
	if (!LL)
	    goto error;
        if ( !_dxf_SetImageOrigin((Field)LL, 0, 0)
	    ||
            !DXSetMember((Group)O_image, NULL, (Object)LL) )
		goto error;
    }

    if ((!c.x || !c.y || s.x || s.y) && ((max_y != biny[0]) || (max_x != binx[ncols]))) {
        Field UR = DXMakeImage(1,1);
	if (!UR)
	    goto error;
        if ( !_dxf_SetImageOrigin((Field)UR, binx[ncols], biny[0])
	    ||
            !DXSetMember((Group)O_image, NULL, (Object)UR) )
		goto error;
    }

    DXDebug
        ( "A", "Arrange FYI: Image Out =([%d,%d], w,h=[%d,%d]),  count (%d)",
          min_x, min_y, max_x, max_y, count );
   
	DXFree(binx);
	DXFree(biny);
    return OK;

    error:
        DXDelete ( O_image );
	O_image = NULL;
	DXFree(binx);
	DXFree(biny);

        DXASSERT ( DXGetError() != ERROR_NONE );
        return ERROR;
}

static 
Object _arrange ( CompositeField parent,
                  Object inp,
                  int nrows,
                  int ncols,
                  int *count,
                  int *binx,
                  int *biny,
		  Vector c,
                  Vector p,
 		  Vector s,
                  int transposed )
{
    CompositeField  new_parent = NULL;

    Object out = NULL;
    Object inp_member;
    int    origin[2];
    int    i;

    DXASSERTGOTO ( inp != ERROR );

    switch ( DXGetObjectClass ( inp ) )
    {
        case CLASS_GROUP:

            switch ( DXGetGroupClass ( (Group)inp ) )
            {
                case CLASS_GROUP:
                case CLASS_SERIES:

                    if ( !parent ) {
                        if ( ERROR == ( new_parent = DXNewCompositeField() ) )
                            goto error;
                        else
                            parent = new_parent;
		    }

                    out = (Object)parent;

                    for ( i=0;
                          (inp_member = DXGetEnumeratedMember
                                           ( (Group)inp, i, NULL ));
                          i++ )
                        if ( !_arrange ( parent,
                                         inp_member,
                                         nrows,
                                         ncols,
                                         count,
                                         binx,
                                         biny,
					 c,
					 p,
					 s,
                                         transposed ) )
                            goto error;

                    break;

                case CLASS_COMPOSITEFIELD:
		    _get_new_origin(inp, origin, *count, ncols, binx, biny, p);
		    (*count)++;

                    if ( ( ERROR == ( out = DXCopy ( inp, COPY_STRUCTURE ) ) )
                         ||
                         !_dxf_SetCompositeImageOrigin
                              ( (CompositeField)out, origin[0], origin[1] ) )
                        goto error;

		    if ( NULL != DXExists(out, "data"))
			if (!DXRemove(out, "data"))
			    goto error;

		    if ( !DXUnsetGroupType((Group)out))
			goto error;

                    if ( parent )
                        if ( !DXSetMember ( (Group)parent, NULL, out ) )
                            goto error;

                    break;

                default:
                    DXErrorGoto2
                        ( ERROR_BAD_CLASS, "#10190", "'image(s)'" );
            }
            break;

        case CLASS_FIELD:

            if ( ERROR == ( out = DXCopy ( inp, COPY_STRUCTURE ) ) )
                goto error;

       	    _get_new_origin(inp, origin, *count, ncols, binx, biny, p);
            (*count)++;

            /*
             * This test and set is to prevent CompositeField group complaints
             * to the tune of:
             *     "Bad parameter: Group member has bad type"
             * from DXSetMember within CLASS_COMPOSITEFIELDs
             * when the data types do not match between images.
             */
            if ( NULL != DXGetComponentValue ( (Field)out, "data" ) )
                if ( !DXSetComponentValue ( (Field)out, "data", NULL ) )
                    goto error;

            if ( !_dxf_SetImageOrigin ( (Field)out, origin[0], origin[1] ) )
                goto error;

            if ( parent )
                if ( !DXSetMember ( (Group)parent, NULL, out ) )
                    goto error;

            break;

        default:
            DXErrorGoto2
                ( ERROR_BAD_CLASS, "#10190", "'image(s)'" );

    }

    return out;

    error:
        if ( out != (Object) parent ) 
	    DXDelete ( out );

        DXDelete ( (Object) new_parent );

        new_parent = NULL;
        out        = NULL;

        DXASSERT ( DXGetError() != ERROR_NONE );
        return ERROR;
}

static
Error 
_get_bins(Object o, int num_images, int nrows, int ncols, 
          Vector c, Vector p, Vector s, int *binx, int *biny)
{
	int i;
	int count;
	int max_w, max_h;
	int new_w, new_h;

	count = 0;
	_get_all_image_bounds(o, num_images, &count, ncols, binx, biny); 

	max_w = 0;
	max_h = 0;
	for (i=0;i<ncols;i++)
	    if (max_w<binx[i+1])
		max_w = binx[i+1];
	for (i=0;i<nrows;i++)
    	    if (max_h<biny[i])
		max_h = biny[i];

	new_w = max_w;
	new_h = max_h;
	if (s.x>0)
    	    new_w = s.x;
	if (s.y>0)
	    new_h = s.y;

	binx[0]=0;
	for (i=0;i<ncols;i++) 
    	    if ((s.x == 0) && (c.x != 0))
	        binx[i+1] += binx[i];
	      else
		binx[i+1] = binx[i]+new_w;

	biny[nrows]=0;
	for (i=0;i<nrows;i++) 
	    if ((s.y == 0) && (c.y != 0))
		biny[nrows-i-1] += biny[nrows-i];
	      else
		biny[nrows-i-1] =  biny[nrows-i]+new_h;

    return OK;
}    

static
Error
_get_all_image_bounds(Object o, int n, int *count, int ncols, int *binx, int *biny)
{
	Object oo;
	int width, height;
	int min_x, min_y;
	int i,j;

	if (*count==n)
    	    return OK;

    switch ( DXGetObjectClass ( o ) )
    {
        case CLASS_GROUP:

            switch ( DXGetGroupClass ( (Group)o ) )
            {
                case CLASS_GROUP:
                case CLASS_SERIES:
			for (i=0;(oo=DXGetEnumeratedMember((Group)o,i,NULL));i++)
			    if (!_get_all_image_bounds(oo, n, count, ncols, binx, biny))
				return ERROR;
		break;

                case CLASS_COMPOSITEFIELD:
			DXGetImageBounds(o, &min_x, &min_y, &width, &height);
			i = *count%ncols+1;
			j = *count/ncols;
			if (binx[i]<width)
		    	    binx[i]=width;
			if (biny[j]<height)
		    	    biny[j]=height;
			(*count)++;
			return OK;

                default:
			return OK;
            }
            break;

        case CLASS_FIELD:
		DXGetImageBounds(o, &min_x, &min_y, &width, &height);
		i = *count%ncols+1;
		j = *count/ncols;
		if (binx[i]<width)
		    binx[i]=width;
		if (biny[j]<height)
		    biny[j]=height;
		(*count)++;
		return OK;
	default:
		return OK;
	}
	return OK;
}

static
Error
_get_new_origin(Object inp, int *origin, int count, int ncols,
				 int *binx, int *biny, Vector p)
{
	int isizex, isizey;
	int bsizex, bsizey;
	int xoffset, yoffset;
	int row, col;
	int min_x, min_y;
	
	row = count / ncols +1;
	col = count % ncols;
	bsizex = binx[col+1]-binx[col];
	bsizey = biny[row-1]-biny[row];
	DXGetImageBounds(inp, &min_x, &min_y, &isizex, &isizey);
	xoffset = bsizex*p.x-(isizex/2);
	if (xoffset<0)
	    xoffset=0;
	if (xoffset+isizex>bsizex)
	    xoffset=bsizex-isizex;
	yoffset = bsizey*p.y-(isizey/2);
	if (yoffset<0)
	    yoffset=0;
	if (yoffset+isizey>bsizey)
	    yoffset=bsizey-isizey;
	origin[0] = binx[col]+xoffset;
	origin[1] = biny[row]+yoffset;

	return OK;
}
