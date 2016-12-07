/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/* RPos, normals out,back faces regular, regular growth, leaks ?*/

#include <dxconfig.h>


/*
 * $Header: /src/master/dx/src/exec/dxmods/showboundary.c,v 1.8 2003/07/11 05:50:36 davidt Exp $
 */

#include <string.h>
#include <stdio.h>
#include <dx/dx.h>
#include "showboundary.h"
#include "_helper_jea.h"
#include "_isosurface.h"

#define    DEFAULT_BOUND_COLOR          DXRGB ( 0.5, 0.7, 1.0 )
#define    PROPRIETARY_COMPONENT_NAME   "overall offsets"

typedef struct bounds *bounds_ptr;

typedef struct bounds
{
    int  i_min, j_min, k_min;
    int  i_max, j_max, k_max;
    int  set;
}
bounds_rec;

struct 
{
    int  have_connects;
    int  have_nondegen;
}
global = {0, 0};


/*
 * The following functions reside herein.
 */

static Field          partition_neighbors  ( Field );
static CompositeField irregular_neighbors  ( CompositeField );
static int            is_irregular_c_field ( CompositeField );
static Object         c_field_neighbors    ( Object );
static Field          show_boundary        ( Field, char *, int );
static Object         set_aux_offsets      ( Object, bounds_ptr, Array );
static Object       traverse_flatten_cf_mg ( Object );


#define I_input  in[0]
#define I_option in[1]
#define O_output out[0]

int m_ShowBoundary ( Object *in, Object *out )
{
    Object  Xtra_output = NULL;
    int     option;

    O_output = NULL;

    if ( I_input == NULL )
        DXErrorGoto2
            ( ERROR_MISSING_DATA, 
              "#10000", /* %s must be specified */ "Object" )
#if 0
    /*
     * Perform this check here because DXCopy ( Array ) does nothing and
     * DXDelete () of that leads to deleted too often...
     */
                /* XXX - test After copy by uniqueness WRT I_input */
    else if ( ( DXGetObjectClass ( I_input ) == CLASS_ARRAY  ) ||
              ( DXGetObjectClass ( I_input ) == CLASS_STRING )   )
        DXErrorGoto2
            ( ERROR_BAD_CLASS,
              "#10190",  /* %s must be a field or a group */
              "'input'" );
#endif

    if ( I_option == NULL )
        option = 0;
    else
        if ( !DXExtractInteger ( I_option, &option )
             ||
             ( option < 0 ) || ( option > 1 ) )
            DXErrorGoto2
                ( ERROR_BAD_PARAMETER,
                  "#10070", /* %s must be either 0 or 1 */
                  "'option' parameter" );

    global.have_connects = 0;
    global.have_nondegen = 0;

    if ( ( ERROR == ( O_output = DXCopy ( I_input, COPY_STRUCTURE ) ) )

         ||
         /* 
          * Set irregular netghbors pass:
          *   c_field_neighbors
          *       c_field_neighbors (recursive)
          *       is_irregular_c_field 
          *       irregular_neighbors 
          *           partition_neighbors 
          *               partition_neighbors (recursive)
          */ 
         ( ERROR == ( O_output = c_field_neighbors ( O_output ) ) )

         ||
         /* 
          * Set regular offsets pass:
          *   set_aux_offsets 
          *       set_aux_offsets (recursive)
          */ 
         !set_aux_offsets ( O_output, NULL, NULL )

         ||
         ( ERROR == ( Xtra_output = DXProcessParts
                                        ( O_output,
                                          show_boundary,
                                          (Pointer) &option,
                                          sizeof(int),
                                          1 /* copy     */,
                                          0 /* preserve */ ) ) ) )
        goto error;

    
    DXDelete ( O_output );   O_output = Xtra_output;   Xtra_output = NULL;


    if ( ERROR == ( Xtra_output = traverse_flatten_cf_mg ( O_output ) ) )
        /* XXX - flatten CF:MG:F->MG:F to MG:CF:F */
        goto error;

    /* XXX - very wrong looking */
    if ( Xtra_output == O_output ) Xtra_output = NULL;
    else
    {
        DXDelete ( O_output );   O_output = Xtra_output;   Xtra_output = NULL;
    }

    if (ERROR == DXCopyAttributes(O_output, I_input))
	goto error;

    if ( ( global.have_connects == 1 ) && ( global.have_nondegen == 0 ) )
         DXWarning
             ( "#11836" /* all connections are degenerate */ );

    return OK;

    error:
        if ( ( Xtra_output != NULL     ) &&
             ( Xtra_output != I_input  ) &&
             ( Xtra_output != O_output )   )
            DXDelete ( Xtra_output );

        if ( ( O_output != NULL    ) &&
             ( O_output != I_input )   )
            DXDelete ( O_output );

        Xtra_output = NULL;
        O_output    = NULL;

        DXASSERT ( DXGetError() != ERROR_NONE );
        return ERROR;
}



#if 0
/* m_Slab: There were problems with colors, etc. dep connections */
static
Field call_slab ( Field field, int d, int n, int t )
{
    Object  slab_inputs  [5];
    Object  slab_outputs [2];
    int i;

    for(i=0;i<5;)  slab_inputs [i++] = NULL;
    for(i=0;i<2;)  slab_outputs[i++] = NULL;

    DXASSERTGOTO ( ERROR      != field        );
    DXASSERTGOTO ( ERROR_NONE == DXGetError() );

    if ( ( ERROR == ( slab_inputs[0] = (Object) field ) )
         ||
         ( ERROR == ( slab_inputs[1]
                          = (Object) DXNewArray
                                         ( TYPE_INT, CATEGORY_REAL, 0 ) ) ) ||
         ( ERROR == ( slab_inputs[2]
                          = (Object) DXNewArray
                                         ( TYPE_INT, CATEGORY_REAL, 0 ) ) ) ||
         ( ERROR == ( slab_inputs[3]
                          = (Object) DXNewArray
                                         ( TYPE_INT, CATEGORY_REAL, 0 ) ) )
         ||
         !DXAddArrayData ( (Array) slab_inputs[1], 0, 1, (Pointer) &d ) ||
         !DXAddArrayData ( (Array) slab_inputs[2], 0, 1, (Pointer) &n ) ||
         !DXAddArrayData ( (Array) slab_inputs[3], 0, 1, (Pointer) &t )
         ||
         !m_Slab ( slab_inputs, slab_outputs )
         ||
         ( ERROR == slab_outputs[0] ) || ( ERROR_NONE != DXGetError() ) )
        goto error;

    for (i=1;i<5;slab_inputs[i++]=NULL)  DXDelete ( slab_inputs[i] );

    return slab_outputs[0];

    error:
        for (i=1;i<5;slab_inputs [i++]=NULL)  DXDelete ( slab_inputs [i] );
        for (i=0;i<2;slab_outputs[i++]=NULL)  DXDelete ( slab_outputs[i] );

        DXASSERT ( DXGetError() != ERROR_NONE );
        return ERROR;
}
#endif



static
Object set_aux_offsets ( Object input, bounds_ptr read, Array write )
{
    int         i;
    Object      child;
    bounds_rec  bounds;
    Array       conn;
    int         dimens;
    Array       aux_array = NULL;

    /*
     * actions given arguments:
     *     ( i,,  ):  traverse until composite field
     *     ( i,r, ):  read and accumulate subordinates
     *     ( i,,w ):  write totals to subordinates
     */

    DXASSERTGOTO ( input != NULL );

    if ( ( read == NULL ) && ( write == NULL ) ) /*--------------------------*/
    {
        switch ( DXGetObjectClass ( input ) )
        {
            case CLASS_FIELD:
                break;
    
            case CLASS_XFORM:
                if ( !DXGetXformInfo  ( (Xform)input, &child, NULL ) ||
                     !set_aux_offsets ( child, NULL, NULL ) )
                    goto error;
                break;

            case CLASS_SCREEN:
                if ( !DXGetScreenInfo ( (Screen)input, &child, NULL, NULL ) ||
                     !set_aux_offsets ( child, NULL, NULL ) )
                    goto error;
                break;

            case CLASS_CLIPPED:
                if ( !DXGetClippedInfo ( (Clipped)input, &child, NULL ) ||
                     !set_aux_offsets  ( child, NULL, NULL ) )
                    goto error;
                break;

            case CLASS_GROUP:
                switch ( DXGetGroupClass ( (Group) input ) )
                {
                    case CLASS_COMPOSITEFIELD:
                        /* Read, then Write */

                        bounds.i_min = bounds.j_min = bounds.k_min = 0;
                        bounds.i_max = bounds.j_max = bounds.k_max = 0;
                        bounds.set   = 0;
                       
                        for ( i = 0; (child=DXGetEnumeratedMember
                                          ( (Group) input, i, NULL ));
                              i++ )
                            if ( !set_aux_offsets ( child, &bounds, NULL ) )
                                goto error;

                        if ( bounds.set == 1 )
                        {
                            if ( ERROR == ( aux_array = DXNewArray
                                                           ( TYPE_INT,
                                                             CATEGORY_REAL, 0 ))
                                 ||
                                 !DXAddArrayData
                                    ( aux_array, 0, 6, (Pointer) &bounds.i_min )
                                 ||
                                 /* XXX */
                                 !DXReference ( (Object) aux_array ) )
                                goto error;
    
                            for ( i = 0; (child=DXGetEnumeratedMember
                                              ( (Group) input, i, NULL ));
                                  i++ )
                                if ( !set_aux_offsets
                                          ( child, NULL, aux_array ) )
                                    goto error;
                        }
                        break;
    
                    default:
                        for ( i = 0; ( child=DXGetEnumeratedMember
                                          ( (Group) input, i, NULL ));
                              i++ )
                            if ( !set_aux_offsets ( child, NULL, NULL ) )
                                goto error;
                }
                break;
    
            default:
                DXErrorGoto2 ( ERROR_BAD_CLASS, "#10190", "'input'" );
        }
    }
    else if ( read != NULL ) /*----------------------------------------------*/
    {
        DXASSERTGOTO ( write == NULL );

        switch ( DXGetObjectClass ( input ) )
        {
            case CLASS_FIELD:
                if ( DXEmptyField ( (Field) input ) )
                   return input;
    
                if ( ERROR == ( conn = (Array) DXGetComponentValue
                                                   ( (Field) input,
                                                     "connections" ) ) )
                    DXErrorGoto2
                        ( ERROR_DATA_INVALID, "#10240", "\"connections\"" );
    
                if ( !DXQueryGridConnections ( conn, &dimens, NULL ) )
                    return input;
    
                if ( ( dimens < 1 ) || ( dimens > 3 ) )
                    DXErrorGoto
                        ( ERROR_NOT_IMPLEMENTED,
                          "regular grid connections must be 1, 2, or 3D" );
    
                if ( !DXQueryGridConnections ( conn, &dimens, &bounds.i_max ) )
                    goto error;
    
                switch ( dimens ) 
                {
                    /* the max is n-1 if the min is 0 */

                    case 3:  bounds.k_max--;
                    case 2:  bounds.j_max--;
                    case 1:  bounds.i_max--;
                }

                if ( !DXGetMeshOffsets ( (MeshArray) conn, &bounds.i_min ) )
                {
                    if ( DXGetError() != ERROR_NONE ) goto error;
                    bounds.i_min = 0;
                    bounds.j_min = 0;
                    bounds.k_min = 0;
                }
    
                switch ( dimens ) 
                {
                    case 1:  bounds.j_max = bounds.j_min = 0;
                    case 2:  bounds.k_max = bounds.k_min = 0;
                }
    
                bounds.i_max += bounds.i_min;
                bounds.j_max += bounds.j_min;
                bounds.k_max += bounds.k_min;
    
                if ( read->i_min > bounds.i_min ) read->i_min = bounds.i_min;
                if ( read->j_min > bounds.j_min ) read->j_min = bounds.j_min;
                if ( read->k_min > bounds.k_min ) read->k_min = bounds.k_min;

                if ( read->i_max < bounds.i_max ) read->i_max = bounds.i_max;
                if ( read->j_max < bounds.j_max ) read->j_max = bounds.j_max;
                if ( read->k_max < bounds.k_max ) read->k_max = bounds.k_max;
    
                read->set = 1;
    
                break;
    
            default:
                DXErrorGoto2 ( ERROR_BAD_CLASS, "#10190", "'input'" );
        }
    }
    else if ( write != NULL ) /*---------------------------------------------*/
    {
        DXASSERTGOTO ( read == NULL );

        switch ( DXGetObjectClass ( input ) )
        {
            case CLASS_FIELD:
                if ( DXEmptyField ( (Field) input ) )
                   return input;
    
                if ( ERROR == ( conn = (Array) DXGetComponentValue
                                                   ( (Field) input,
                                                     "connections" ) ) )
                    DXErrorGoto2
                        ( ERROR_DATA_INVALID, "#10240", "\"connections\"" );
    
                if ( !DXQueryGridConnections ( conn, &dimens, NULL ) )
                    return input;
    
                if ( !DXSetComponentValue
                          ( (Field)  input,
                            PROPRIETARY_COMPONENT_NAME,
                            (Object) write ) )
                    goto error;
    
                break;
    
            default:
                DXErrorGoto2 ( ERROR_BAD_CLASS, "#10190", "'input'" );
        }
    }

    DXDelete ( (Object) aux_array );

    return input;

    error:
        DXDelete ( (Object) aux_array );

        DXASSERT ( DXGetError() != ERROR_NONE );
        return ERROR;
}



static
Object c_field_neighbors ( Object object )
{
    Object child;
    int    i;

    switch ( DXGetObjectClass ( object ) )
    {
        case CLASS_FIELD:
            break;

        case CLASS_XFORM:
            if ( !DXGetXformInfo    ( (Xform)object, &child, NULL ) ||
                 !c_field_neighbors ( child ) )
                goto error;
            break;

        case CLASS_SCREEN:
            if ( !DXGetScreenInfo   ( (Screen)object, &child, NULL, NULL ) ||
                 !c_field_neighbors ( child ) )
                goto error;
            break;

        case CLASS_CLIPPED:
            if ( !DXGetClippedInfo  ( (Clipped)object, &child, NULL ) ||
                 !c_field_neighbors ( child ) )
                goto error;
            break;

        case CLASS_GROUP:
            switch ( DXGetGroupClass ( (Group)object ) )
            {
                case CLASS_COMPOSITEFIELD:
                    if ( is_irregular_c_field ( (CompositeField) object ) )
                        if ( !irregular_neighbors ( (CompositeField) object ) )
                            goto error;
                    break;

                default:
                    for ( i = 0;
                          ( NULL != ( child = DXGetEnumeratedMember
                                                  ( (Group)object, i, NULL )) );
                          i++ )
                        if ( !c_field_neighbors ( child ) )
                            goto error;
            }
            break;

        default:
            DXErrorGoto2 ( ERROR_BAD_CLASS, "#10190", "'input'" );
    }

    return object;

    error:
        DXASSERT ( DXGetError() != ERROR_NONE );
        return ERROR;
}



static
CompositeField irregular_neighbors ( CompositeField cfield )
{
    Object  child;
    int     i;

    DXASSERTGOTO ( DXGetObjectClass ( (Object) cfield ) == CLASS_GROUP );
    DXASSERTGOTO( DXGetGroupClass  ( (Group) cfield ) == CLASS_COMPOSITEFIELD );

    if ( !DXGrow ( (Object)cfield, 1, NULL, NULL ) )
        goto error;

    for ( i = 0;
          ( NULL != ( child = DXGetEnumeratedMember
                                   ( (Group)cfield, i, NULL ) ) );
          i++ )
        if ( !partition_neighbors ( (Field)child ) )
            goto error;

    if ( !DXShrink ( (Object)cfield ) )
        goto error;

    return cfield;

    error:
        DXASSERT ( DXGetError() != ERROR_NONE );
        return ERROR;
}



static
Field partition_neighbors ( Field object )
{
    Array  neighbors;
    int    i, nRefs, nNbrs, origConnections;
    int    *refs;

    DXASSERTGOTO ( DXGetObjectClass ( (Object) object ) == CLASS_FIELD );

    /* XXX - a traverser had been here */

    if ( DXEmptyField ( object ) )
        return object;

    /* 1D fail consideration. */

    if ( ( ERROR == ( neighbors = DXNeighbors ( object ) ) ) )
        return ( DXGetError == ERROR_NONE ) ? object : ERROR;

    if ( ! DXQueryOriginalSizes ( object, NULL, &origConnections ) 
         ||
         ! DXGetArrayInfo ( neighbors, NULL, NULL, NULL, NULL, &nNbrs )
         ||
         ( ERROR == ( refs = DXGetArrayData ( neighbors ) ) ) )
        goto error;

    /* XXX - who says that it is right to modify neighbors ? */

    nRefs = origConnections * nNbrs;

    for ( i = 0;  i < nRefs;  i++, refs++ )
        if ( *refs >= origConnections )
            *refs = -2;

    return object;

    error:
        DXASSERT ( DXGetError() != ERROR_NONE );
        return ERROR;
}



static
int is_irregular_c_field ( CompositeField object )
{
    Field  field;
    int    i;
    Array  connections;
    int    any_regular   = 0;
    int    any_irregular = 0;

    DXASSERT ( DXGetObjectClass ( (Object) object ) == CLASS_GROUP );
    DXASSERT ( DXGetGroupClass  ( (Group)  object ) == CLASS_COMPOSITEFIELD );

    for ( i = 0;
          ( NULL != ( field = DXGetPart ( (Object)object, i ) ) );
          i++ )
        if ( !DXEmptyField ( field )
             &&
             ( NULL != ( connections
                             = (Array)DXGetComponentValue
                                          ( field, "connections" ) ) ) )
        {
            if ( DXQueryGridConnections ( connections, NULL, NULL ) )
                any_regular   = 1;
            else
                any_irregular = 1;
        }

    /* 
     * XXX 
     * this routine should be 3-state: Empty, Regular, Irregular
     */
    if ( any_irregular || !any_regular )
        return 1;
    else
        return 0;
}



static
Object flatten_cf_mg ( Object in, MultiGrid out, int *position )
{
    Object child;
    int    i;

    /* XXX - Make MG:CF:F based on orientation of planes */

    DXASSERTGOTO ( in && out && position );

    switch ( DXGetObjectClass ( in ) )
    {
        case CLASS_FIELD:
            if ( !DXSetEnumeratedMember ( (Group)out, *position, in ) )
                goto error;

            (*position)++;
            break;

        case CLASS_GROUP:
            for ( i     = 0;
                  NULL != ( child = DXGetEnumeratedMember
                                        ( (Group)in, i, NULL ) );
                  i++ )
                if ( !flatten_cf_mg ( child, out, position ) )
                    goto error;
            break;

        default:
            DXErrorGoto
                ( ERROR_DATA_INVALID, "Illegal member in Composite Field" );
    }

    return (Object) out;

    error:
        DXASSERT ( DXGetError() != ERROR_NONE );
        return ERROR;
}



static
Object traverse_flatten_cf_mg ( Object object )
{
    MultiGrid  flattened = NULL;
    int        position  = 0;
    int        i;
    Object     child, new_child;

    switch ( DXGetObjectClass ( object ) )
    {
        case CLASS_XFORM:
            if ( !DXGetXformInfo ( (Xform)object, &child, NULL ) ||
                 ( ERROR == ( new_child = traverse_flatten_cf_mg ( child ) ) ) )
                goto error;
            else if ( ( new_child != child ) &&
                      !DXSetXformObject ( (Xform)object, new_child ) )
                goto error;
            break;

        case CLASS_SCREEN:
            if ( !DXGetScreenInfo   ( (Screen)object, &child, NULL, NULL )  ||
                 ( ERROR == ( new_child = traverse_flatten_cf_mg ( child ) ) ) )
                goto error;
            else if ( ( new_child != child ) &&
                      !DXSetScreenObject ( (Screen)object, new_child ) )
                goto error;
            break;

        case CLASS_CLIPPED:
            if ( !DXGetClippedInfo    ( (Clipped)object, &child, NULL )     ||
                 ( ERROR == ( new_child = traverse_flatten_cf_mg ( child ) ) ) )
                goto error;
            else if ( ( new_child != child ) &&
                      !DXSetClippedObjects ( (Clipped)object, new_child, NULL ))
                goto error;
            break;

        case CLASS_GROUP:

            switch ( DXGetGroupClass ( (Group)object ) )
            {
                /* we are looking for CF:MG:F's */
                case CLASS_COMPOSITEFIELD:
                    for ( i     = 0;
                          NULL != ( child = DXGetEnumeratedMember
                                                ( (Group)object, i, NULL ) );
                          i++ )
                        if ( ( CLASS_GROUP     == DXGetObjectClass ( child ) )&&
                             ( CLASS_MULTIGRID == DXGetGroupClass
                                                      ( (Group) child ) ) )
                        {
                            position = 0;

                            if ( ( ERROR == ( flattened = DXNewMultiGrid () ) )
                                 ||
                                !flatten_cf_mg ( object, flattened, &position ))
                                goto error;

                            /* 
                             * Delete unnecessary unless top group,
                             * for that see m_Showboundary().
                             */
                            return (Object) flattened;
                            break;
                        }
                        /* else fall through */

                default:
                    for ( i     = 0;
                          NULL != ( child = DXGetEnumeratedMember
                                                ( (Group)object, i, NULL ) );
                          i++ )
                        if ( ERROR == ( new_child = traverse_flatten_cf_mg
                                                        ( child ) ) )
                            goto error;
                        else if ( ( new_child != child ) &&
                                  !DXSetEnumeratedMember
                                       ( (Group)object, i, new_child ) )
                            goto error;

            }
            break;

        case CLASS_FIELD:
        default:
            return object;
    }
    
    return object;

    error:
#if 0
        DXDelete ( (Object) );
#endif
        DXASSERT ( DXGetError() != ERROR_NONE );
        return ERROR;
}



static
Field make_normals ( Field field, int nP, int flip )
{
    field_info  input_info;
    array_info  p_info, c_info, n_info = NULL;
    Array       array;

    Point         *pp;
    Vector        n, *np;
    Quadrilateral q, *qp;
    Triangle         *tp;
    float         length;
    int           i, j;
    PointId       t;

#if 0
DXMessage ( "    make_normals: given flip = %d", flip );
#endif

    if ( ( flip == 0 ) || DXEmptyField ( field ) )
        return field;

    if ( ( ERROR == ( array = DXNewArray ( TYPE_FLOAT, CATEGORY_REAL, 1, 3 ) ) )
         ||
         !DXAllocateArray ( array, nP )          ||
         !DXAddArrayData  ( array, 0, nP, NULL ) ||
         !DXTrim          ( array )
         ||
         !DXSetComponentValue ( field, "normals", (Object) array ) )
        goto error;

    array = NULL;

    if ( !DXSetComponentAttribute
             ( field, "normals", "dep", (Object)DXNewString ( "positions" ) )
         ||
         ( ERROR == ( input_info = _dxf_InMemory ( field ) ) )
         ||
         !_dxf_SetIterator ( input_info->std_comps[(int)CONNECTIONS] ) ||
         !_dxf_SetIterator ( input_info->std_comps[(int)POSITIONS]   )  )
        goto error;

    p_info = (array_info) &(input_info->std_comps[(int)POSITIONS]->array);
    c_info = (array_info) &(input_info->std_comps[(int)CONNECTIONS]->array);
    n_info = (array_info) &(input_info->std_comps[(int)NORMALS]->array);

    if ( !memset ( (char*)n_info->data, 0, (n_info->items*n_info->itemsize) ) )
        DXErrorGoto2
            ( ERROR_UNEXPECTED,
              "#11800", /* C standard library call, %s, returns error */
              "memset()" );

    np = (Point*)n_info->data;
    pp = (Point*)p_info->data;

    switch ( input_info->std_comps[(int)CONNECTIONS]->element_type )
    {
        case TRIANGLES:
            for ( i=0, tp=(Triangle*)c_info->data; i<c_info->items; i++, tp++ )
            {
                if ( flip == -1 )
                { t = tp->p; tp->p = tp->q; tp->q = t; } 

                n = DXCross ( DXSub ( pp[tp->p], pp[tp->q] ),
                              DXSub ( pp[tp->p], pp[tp->r] ) );

                /* potential non-cuteness! */
                for ( j=0; j<9; j++ )
                    ( (float*) & np [ ( (int*) tp ) [j/3] ] ) [j%3]
                        += ((float*)&n)[j%3];
            }
            break;

        case QUADRILATERALS:
            if ( c_info->class == CLASS_MESHARRAY ) flip = 1;

            for ( i=0; i<c_info->items; i++ )
            {
                if ( flip == -1 )
                {
                    qp = &(((Quadrilateral*)c_info->data)[i]);

                    t = qp->p; qp->p = qp->q; qp->q = t;
                    t = qp->r; qp->r = qp->s; qp->s = t;
                }
                else
                {
                    qp = &q;

                    if ( !c_info->get_item ( i, c_info, &q ) ) goto error;
                }

                n = DXCross ( DXSub ( pp[qp->p], pp[qp->r] ),
                              DXSub ( pp[qp->p], pp[qp->q] ) );

                /* potential non-cuteness! */
                for ( j=0; j<12; j++ )
                    ( (float*) & np [ ( (int*) qp ) [j/3] ] ) [j%3]
                        += ((float*)&n)[j%3];
            }
            break;
         default:
	    break;
    }


    for ( i=0, np=(Vector*)n_info->data; i<n_info->items; i++, np++ )
    {
        if ( ( length = DXLength ( *np ) ) != 0.0 )
#if 0
        if ( ( length = -DXLength ( *np ) ) != 0.0 )
#endif
        {
            np->x /= length;
            np->y /= length;
            np->z /= length;
        }
    }

    if ( !_dxf_FreeInMemory ( input_info ) )
        goto error;

    return field;

    error:
        DXASSERT ( DXGetError() != ERROR_NONE );
        return ERROR;
}


Array _dxf_resample_array
          ( component_info comp,
            int dep_ref,   /* 1=dep,2=ref., how comp relates to push/pull */
            int Nin,       /* number of input items                */
            int Nout,      /* number of output items               */
            int *push,     /* array of (from[i:Nin] = to) indices  */
            int *pull      /* array of (to[i:Nout] = from) indices */
          )
{
    Array   array = NULL;
    Array   array2= NULL;
    Pointer data  = NULL;
    int     i, j, k;

    if ( CLASS_CONSTANTARRAY == ((array_info)&comp->array)->class )
    {
        /* XXX - if the data is ref, change the value, even if constant */

        if ( ERROR == ( array
                 = (Array)DXNewConstantArrayV
                              ( Nout,
                                DXGetConstantArrayData
                                    ( ((array_info)&comp->array)->array ),
                                ((array_info)&comp->array)->type,
                                ((array_info)&comp->array)->category,
                                ((array_info)&comp->array)->rank,
                                ((array_info)&comp->array)->shape ) ) )
            goto error;
    }
    else /* not CONSTANTARRAY */
    {
        if ( !_dxf_SetIterator ( comp ) 
             ||
             ( ERROR == ( array = DXNewArrayV
                                     ( ((array_info)&comp->array)->type,
                                       ((array_info)&comp->array)->category,
                                       ((array_info)&comp->array)->rank,
                                       ((array_info)&comp->array)->shape ) ) )
             ||
             !DXAllocateArray ( array, Nout )          ||
             !DXAddArrayData  ( array, 0, Nout, NULL ) ||
             !DXTrim          ( array )
             ||
             ( ERROR == ( data = DXGetArrayData ( array ) ) ) )
            goto error;

        if ( 1 == dep_ref ) /* DEP */
        {
            DXASSERTGOTO ( Nin == ((array_info)&comp->array)->items );

            if ( NULL != push )
            {
                /*
                 * For each input item
                 *     if it maps to output then
                 *         save in output array given push value
                 */
                for ( i=0; i<Nin; i++ )
                    if ( ( push[i] > -1 )
                         &&
                         !((array_info)&comp->array)->get_item
                             ( i, (array_info)&comp->array,
                               &((char*)data)
                                [push[i]*((array_info)&comp->array)->itemsize]))
                        goto error;
            }
            else /* NULL != pull */
            {
                DXASSERTGOTO ( NULL != pull );

                /*
                 * For each output item
                 *     retrieve from input given pull array value
                 *     save in output
                 */
                for ( i=0; i<Nout; i++ )
                    if ( !((array_info)&comp->array)->get_item
                             ( pull[i],
                               (array_info)&comp->array,
                               &((char*)data)
                                    [i*((array_info)&comp->array)->itemsize]))
                        goto error;
            }
        }
        else /* REF */
        {
            int  ref;
            int  nout = 0;

            if ( ( 0 != strcmp ( comp->name, "invalid connections" ) ) &&
                 ( 0 != strcmp ( comp->name, "invalid positions"   ) )   )
                goto error;

          DXASSERTGOTO( 2             == dep_ref );
          DXASSERTGOTO( TYPE_INT      == ((array_info)&comp->array)->type     );
          DXASSERTGOTO( CATEGORY_REAL == ((array_info)&comp->array)->category );
          DXASSERTGOTO( sizeof(int)   == ((array_info)&comp->array)->itemsize );

            for ( i=0; i<Nout; i++ ) ((int*)data)[i] = -1;

            if ( NULL != push )
            {
                /*
                 * For each input item
                 *     If it has a valid reference and is to be pushed to output
                 *         save in output
                 */
                for ( i=0; i<((array_info)&comp->array)->items; i++ )
                {
                    if ( !((array_info)&comp->array)->get_item
                             ( i, (array_info)&comp->array, ((char*)&ref) ) )
                        goto error;

                    if ( ( ref > -1 ) && ( push[ref] > -1 ) )
                        ((int*)data)[nout++] = push[ref];
                }
            }
            else /* NULL != pull */
            {
                DXASSERTGOTO ( NULL != pull );

                /*
                 * For each input item
                 *     If it has a valid reference
                 *         For each item in pull array
                 *             If the pull array item matches the reference
                 *             it can be moved, so
                 *                 If reference is not already in output
                 *                     save in output
                 */
                for ( i=0; i<((array_info)&comp->array)->items; i++ )
                {
                    if ( !((array_info)&comp->array)->get_item
                             ( i, (array_info)&comp->array, ((char*)&ref) ) )
                        goto error;

                    if ( ref > -1 )
                        for ( j=0; j<Nout; j++ )
                            if ( pull[j] == ref )
                            {
                                for(k=0; (k<nout)&&(((int*)data)[k]!=j); k++);

                                if ( k == nout )
                                    ((int*)data)[nout++] = j;
                            }
                }
            }

            if ( ( ERROR == ( array2 = DXNewArrayV
                                         ( ((array_info)&comp->array)->type,
                                           ((array_info)&comp->array)->category,
                                           ((array_info)&comp->array)->rank,
                                           ((array_info)&comp->array)->shape )))
                 ||
                 !DXAllocateArray ( array2, nout )          ||
                 !DXAddArrayData  ( array2, 0, nout, data ) ||
                 !DXTrim          ( array2 ) )
                goto error;

            DXDelete ( (Object) array );

            array  = array2;
            array2 = NULL;
        }
    }

    return array;

    error:
        DXDelete ( (Object) array );  array = NULL;
        DXDelete ( (Object) array2);  array2= NULL;

        DXASSERT ( DXGetError() != ERROR_NONE );
        return ERROR;
}



static
Field jea_slab
          ( field_info input_info,
            int  d,
            int  n )
{
    int  nC_out, nP_in, nP_out;

    Field           out_field;
    Array           array     = NULL;
    array_info      c_info    = NULL;
    component_info  comp;
    Pointer         data      = NULL;
    int             *pull     = NULL;

    int flip = 0;

    array_info  t0, t1, t2=NULL;
    int         I,  II, OO,  i, j, k;
    int         n1;

    int    rev = (( d == 0 ) && ( n == 0 )) ||
                 (( d == 1 ) && ( n != 0 )) ||
                 (( d == 2 ) && ( n == 0 ));

 /*
  * Note:  if irregular data or invalidity culling, this call not made.
  */

#if 0
DXMessage ( "jea_slab d=%d, n=%d", d, n );
DXPrint   ( (Object)input_info->field, "rd", "connections", 0);
#endif


    c_info = (array_info) &(input_info->std_comps[(int)CONNECTIONS]->array);
    nP_in  = ((array_info) &(input_info->std_comps[(int)POSITIONS]->array))
                 ->items;

    if ( ERROR == ( out_field
                 = (Field) DXCopy ( (Object)input_info->field, COPY_HEADER ) ) )
        goto error;

    switch ( input_info->std_comps[(int)CONNECTIONS]->element_type )
    {
        case QUADRILATERALS:
            t0 = (array_info) &(((struct _array_allocator *)
                                    ((mesh_array_info)c_info)->terms)[0]);
            t1 = (array_info) &(((struct _array_allocator *)
                                    ((mesh_array_info)c_info)->terms)[1]);

            if ( ERROR == ( array = DXMakeGridConnections
                                        ( 1,
                                          ( (d!=0) ? t0->items+1 :
                                                     t1->items+1 ) ) ) )
                goto error;

            nC_out = ((d!=0)? t0->items: t1->items );
            nP_out = nC_out + 1;

            break;

        case CUBES:
            t0 = (array_info) &(((struct _array_allocator *)
                                    ((mesh_array_info)c_info)->terms)[0]);
            t1 = (array_info) &(((struct _array_allocator *)
                                    ((mesh_array_info)c_info)->terms)[1]);
            t2 = (array_info) &(((struct _array_allocator *)
                                    ((mesh_array_info)c_info)->terms)[2]);

            if ( ERROR == ( array = DXMakeGridConnections
                                        ( 2,
                                          ( (d!=0)? t0->items+1: t1->items+1 ), 
                                          ( (d!=2)? t2->items+1: t1->items+1 ) )
                          ) )
                goto error;

            nC_out = ((d!=0)? t0->items: t1->items ) *
                     ((d!=2)? t2->items: t1->items );

            nP_out = ( ((d!=0)? t0->items: t1->items ) + 1 ) *
                     ( ((d!=2)? t2->items: t1->items ) + 1 );

            if ( ERROR == ( flip = _dxf_get_flip
                                       ( input_info,
                                         &global.have_connects,
                                         &global.have_nondegen ) ) )
                goto error;

#if 0
            DXMessage ( "jea_slab: get flip = %d", flip );
#endif
            break;

        default:
            DXErrorGoto2
                ( ERROR_DATA_INVALID,
                  "#11380", /* %s is an invalid connections type */
                  input_info->std_comps[(int)CONNECTIONS]->name );
    }

    if ( !DXSetComponentValue ( out_field, "connections", (Object) array ) )
        goto error;

    array = NULL;

    if ( !DXSetComponentAttribute
             ( out_field, "connections", "element type",
               (Object)DXNewString
                           ( 
              ( CUBES == input_info->std_comps[(int)CONNECTIONS]->element_type )
                        ? "quads" : "lines" ) ) )
        goto error;


    /*
     * For each component in input:
     *     Determine if copied unchanged, already handled,
     *     discarded, or remapped.
     */
    for ( I=0, comp=input_info->comp_list;
          I<input_info->comp_count;
          I++, comp++ )
    {
        array = NULL;

#if 0
DXMessage ( "---------------------------------------------------------------" );
DXMessage ( "The name of the dog is %s", comp->name );
DXPrint   ( (Object)((array_info)&comp->array)->array, "rd", 0);
#endif

        if ( 0 == strcmp ( comp->name, "color map" ) || 
	     0 == strcmp ( comp->name, "opacity map" ) )
        {
            array = ((array_info)&comp->array)->array;
        }
        else if ( ( 0 == strncmp ( comp->name, "original", 8 ) ) ||
                  ( 0 == strcmp  ( comp->name, "normals"     ) )   )
        {
            array = NULL;
        }
        else if ( ( NULL  != comp->std_attribs[(int)DEP]        ) &&
                  ( ERROR != comp->std_attribs[(int)DEP]->value )   )
        {
            if (0 == strcmp ( comp->std_attribs[(int)DEP]->value, "positions" ))
            {
                if ( CLASS_CONSTANTARRAY == ((array_info)&comp->array)->class )
                {
                    if ( ERROR == ( array
                             = (Array)DXNewConstantArrayV
                                   ( nP_out,
                                     DXGetConstantArrayData
                                         ( ((array_info)&comp->array)->array ),
                                     ((array_info)&comp->array)->type,
                                     ((array_info)&comp->array)->category,
                                     ((array_info)&comp->array)->rank,
                                     ((array_info)&comp->array)->shape ) ) )
                        goto error;
                }
                else
                {
                    if ( !_dxf_SetIterator ( comp ) 
                         ||
                         ( ERROR == ( array
                               = DXNewArrayV
                                     ( ((array_info)&comp->array)->type,
                                       ((array_info)&comp->array)->category,
                                       ((array_info)&comp->array)->rank,
                                       ((array_info)&comp->array)->shape ) ) )
                         ||
                         !DXAllocateArray ( array, nP_out )          ||
                         !DXAddArrayData  ( array, 0, nP_out, NULL ) ||
                         !DXTrim          ( array )
                         ||
                         ( ERROR == ( data = DXGetArrayData ( array ) ) ) )
                        goto error;

                    switch ( input_info->std_comps[(int)CONNECTIONS]
                              ->element_type )
                    {
                        case QUADRILATERALS:
                            for ( i = (d==0)? n : 0;
                                  (d==0)? (i==n) : (i<(t0->items+1));
                                  i++ )
                            for ( j = (d==1)? n : 0;
                                  (d==1)? (j==n) : (j<(t1->items+1));
                                  j++ )
                            {
                                /* 2D ( rev ) */
                                II = ( i * (t1->items+1) ) + j;
                                OO = (d==0)? j : i;

                                if ( !((array_info)&comp->array)->get_item
                                          ( II, (array_info)&comp->array,
                                            &((char*)data)
                                     [OO*((array_info)&comp->array)->itemsize]))
                                    goto error;
                            }
                            break;

                        case CUBES:
                            for ( i = (d==0)? n : 0;
                                  (d==0)? (i==n) : (i<(t0->items+1));
                                  i++ )
                            for ( j = (d==1)? n : 0;
                                  (d==1)? (j==n) : (j<(t1->items+1));
                                  j++ )
                            for ( k = (d==2)? n : 0;
                                  (d==2)? (k==n) : (k<(t2->items+1));
                                  k++ )
                            {
                                if ( rev )
                                {
                                    if ( d == 2 )
                                        II = ((((t0->items-i)*(t1->items+1))+j)
                                             *(t2->items+1))+k;
                                    else
                                        II = (((i*(t1->items+1))+j)
                                             *(t2->items+1)) + t2->items-k;
                                }
                                else
                                II = (((i*(t1->items+1))+j)*(t2->items+1))+k;

                                OO = (d==0)? ( ( j * (t2->items+1) ) + k ) :
                                     (d==1)? ( ( i * (t2->items+1) ) + k ) :
                                             ( ( i * (t1->items+1) ) + j );

                                if ( !((array_info)&comp->array)->get_item
                                          ( II, (array_info)&comp->array,
                                            &((char*)data)
                                     [OO*((array_info)&comp->array)->itemsize]))
                                    goto error;
                            }
                            break;
		         default:
			    break;
                    }
                }
            }
            else if ( ( 0 == strcmp ( comp->std_attribs[(int)DEP]->value,
                                      "connections" ) ) &&
                      ( 0 != strcmp ( comp->name, "connections" ) ) )
            {
                if ( CLASS_CONSTANTARRAY == ((array_info)&comp->array)->class )
                {
                    if ( ERROR == ( array
                             = (Array)DXNewConstantArrayV
                                   ( nC_out,
                                     DXGetConstantArrayData
                                         ( ((array_info)&comp->array)->array ),
                                     ((array_info)&comp->array)->type,
                                     ((array_info)&comp->array)->category,
                                     ((array_info)&comp->array)->rank,
                                     ((array_info)&comp->array)->shape ) ) )
                        goto error;
                }
                else
                {
                    if ( !_dxf_SetIterator ( comp ) 
                         ||
                         ( ERROR == ( array
                               = DXNewArrayV
                                     ( ((array_info)&comp->array)->type,
                                       ((array_info)&comp->array)->category,
                                       ((array_info)&comp->array)->rank,
                                       ((array_info)&comp->array)->shape ) ) )
                         ||
                         !DXAllocateArray ( array, nC_out )          ||
                         !DXAddArrayData  ( array, 0, nC_out, NULL ) ||
                         !DXTrim          ( array )
                         ||
                         ( ERROR == ( data = DXGetArrayData ( array ) ) ) )
                        goto error;

                    n1 = (n==0)? 0 : n-1;  /* max connections = max p - 1 */

                    switch ( input_info->std_comps[(int)CONNECTIONS]
                             ->element_type )
                    {
                        case QUADRILATERALS:
                            for ( i = (d==0)? n1 : 0;
                                  (d==0)? (i==n1) : (i<t0->items);
                                  i++ )
                            for ( j = (d==1)? n1 : 0;
                                  (d==1)? (j==n1) : (j<t1->items);
                                  j++ )
                            {
                                /* 2D ( rev ) */
                                II = ( i * t1->items ) + j;
                                OO = (d==0)? j : i;

                                if ( !((array_info)&comp->array)->get_item
                                          ( II, (array_info)&comp->array,
                                            &((char*)data)
                                     [OO*((array_info)&comp->array)->itemsize]))
                                    goto error;
                            }
                            break;

                        case CUBES:
                            for ( i = (d==0)? n1 : 0;
                                  (d==0)? (i==n1) : (i<t0->items);
                                  i++ )
                            for ( j = (d==1)? n1 : 0;
                                  (d==1)? (j==n1) : (j<t1->items);
                                  j++ )
                            for ( k = (d==2)? n1 : 0;
                                  (d==2)? (k==n1) : (k<t2->items);
                                  k++ )
                            {
                                if ( rev )
                                {
                                    if ( d == 2 )
                                        II = (((((t0->items-1)-i)*t1->items)+j)
                                             *t2->items) + k;
                                    else
                                        II = (((i*t1->items)+j)
                                             *t2->items) + (t2->items-1)-k;
                                }
                                else
                               II = ((( i * t1->items ) + j ) * t2->items ) + k;

                                OO = (d==0)? ( ( j * t2->items ) + k ) :
                                     (d==1)? ( ( i * t2->items ) + k ) :
                                             ( ( i * t1->items ) + j );

                                if ( !((array_info)&comp->array)->get_item
                                          ( II, (array_info)&comp->array,
                                            &((char*)data)
                                     [OO*((array_info)&comp->array)->itemsize]))
                                    goto error;
                            }
                            break;
		         default:
			    break;
                    }
                }
            }
        }
        else if ( ( NULL  != comp->std_attribs[(int)REF]        ) &&
                  ( ERROR != comp->std_attribs[(int)REF]->value ) &&
                  ( ( 0 == strcmp ( comp->name, "invalid connections" ) ) ||
                    ( 0 == strcmp ( comp->name, "invalid positions"   ) ) ))
        {
            /*
             * Time/space tradeoff:
             *   push (see _dxf_resample_array) is simpler, pull is smaller
             */
            if ( 0 == strcmp ( comp->std_attribs[(int)REF]->value, "positions"))
            {
                if ( ERROR == ( pull = (int*) DXAllocate (nP_out*sizeof(int))) )
                    goto error;

                switch ( input_info->std_comps[(int)CONNECTIONS]->element_type )
                {
                    case QUADRILATERALS:
                        for ( i = (d==0)? n : 0;
                              (d==0)? (i==n) : (i<(t0->items+1));
                              i++ )
                        for ( j = (d==1)? n : 0;
                              (d==1)? (j==n) : (j<(t1->items+1));
                              j++ )
                        {
                            /* 2D ( rev ) */
                            II = ( i * (t1->items+1) ) + j;
                            OO = (d==0)? j : i;

                            pull[OO] = II;
                        }
                        break;

                    case CUBES:
                        for ( i = (d==0)? n : 0;
                              (d==0)? (i==n) : (i<(t0->items+1));
                              i++ )
                        for ( j = (d==1)? n : 0;
                              (d==1)? (j==n) : (j<(t1->items+1));
                              j++ )
                        for ( k = (d==2)? n : 0;
                              (d==2)? (k==n) : (k<(t2->items+1));
                              k++ )
                        {
                            if ( rev )
                            {
                                if ( d == 2 )
                                    II = ((((t0->items-i)*(t1->items+1))+j)
                                         *(t2->items+1)) + k;
                                else
                                    II = (((i*(t1->items+1))+j)
                                         *(t2->items+1)) + t2->items-k;
                            }
                            else
                            II = (((i*(t1->items+1))+j)*(t2->items+1))+k;

                            OO = (d==0)? ( ( j * (t2->items+1) ) + k ) :
                                 (d==1)? ( ( i * (t2->items+1) ) + k ) :
                                         ( ( i * (t1->items+1) ) + j );

                            pull[OO] = II;
                        }
                        break;
		    default:
		        break;
                }

                if ( ERROR == ( array = _dxf_resample_array
                                            ( comp, 2 /* REF */,
                                              nP_in, nP_out,
                                              NULL, pull ) ) )
                    goto error;

                DXFree ( (Pointer) pull );  pull = NULL;

            }
            else if ( 0 == strcmp ( comp->std_attribs[(int)REF]->value,
                                    "connections" ) )
            {
                if ( ERROR == ( pull = (int*) DXAllocate (nC_out*sizeof(int))) )
                    goto error;

                n1 = (n==0)? 0 : n-1;  /* max connections = max p - 1 */

                switch ( input_info->std_comps[(int)CONNECTIONS]->element_type )
                {
                    case QUADRILATERALS:
                        for ( i = (d==0)? n1 : 0;
                              (d==0)? (i==n1) : (i<t0->items);
                              i++ )
                        for ( j = (d==1)? n1 : 0;
                              (d==1)? (j==n1) : (j<t1->items);
                              j++ )
                        {
                            /* 2D ( rev ) */
                            II = ( i * t1->items ) + j;
                            OO = (d==0)? j : i;

                            pull[OO] = II;
                        }
                        break;

                    case CUBES:
                        for ( i = (d==0)? n1 : 0;
                              (d==0)? (i==n1) : (i<t0->items);
                              i++ )
                        for ( j = (d==1)? n1 : 0;
                              (d==1)? (j==n1) : (j<t1->items);
                              j++ )
                        for ( k = (d==2)? n1 : 0;
                              (d==2)? (k==n1) : (k<t2->items);
                              k++ )
                        {
                            if ( rev )
                            {
                                if ( d == 2 )
                                    II = (((((t0->items-1)-i)*t1->items)+j)
                                         * t2->items ) + k;
                                else
                                    II = (((i*t1->items)+j)
                                         * t2->items ) + (t2->items-1)-k;
                            }
                            else
                           II = ((( i * t1->items ) + j ) * t2->items ) + k;

                            OO = (d==0)? ( ( j * t2->items ) + k ) :
                                 (d==1)? ( ( i * t2->items ) + k ) :
                                         ( ( i * t1->items ) + j );

                            pull[OO] = II;
                        }
                        break;
		     default:
		        break;
                }

                if ( ERROR == ( array = _dxf_resample_array
                                            ( comp, 2 /* REF */,
                                              c_info->items, nC_out,
                                              NULL, pull ) ) )
                    goto error;

                DXFree ( (Pointer) pull );  pull = NULL;
            }
        }

        if ( 0 != strcmp ( comp->name, "connections" ) )
            if ( !DXSetComponentValue ( out_field, comp->name, (Object)array ) )
                goto error;
        
    }

    /* XXX - remove "der"'s ! (This gets the follow-on der's as well) */
    if ( !DXChangedComponentValues ( out_field, "positions"   ) ||
         !DXChangedComponentValues ( out_field, "connections" )  )
        goto error;

    if ( input_info->std_comps[(int)CONNECTIONS]->element_type == CUBES )
        if ( !make_normals ( out_field, nP_out, flip ) )
            goto error;

    if ( !_dxf_SetDefaultColor ( out_field, DEFAULT_BOUND_COLOR ) )
        goto error;

    return out_field;

    error:
        DXASSERT ( DXGetError() != ERROR_NONE );
        return ERROR;
}



static
MultiGrid regular_boundary ( field_info input_info )
{
    component_info  proprietary = NULL;
    bounds_ptr      bptr        = NULL;
    MultiGrid       output      = NULL;
    Field           field       = NULL;
    array_info      c_info      = NULL;
    int             nfields     = 0;
    array_info      t0, t1, t2;


    if ( ( ERROR == ( output = DXNewMultiGrid() ) ) )
        goto error;

    if ( ( ERROR == ( proprietary = _dxf_FindCompInfo
                                        ( input_info,
                                          PROPRIETARY_COMPONENT_NAME ) ) )
         &&
         ( DXGetError() != ERROR_NONE ) )
        goto error;

    bptr = ( NULL == proprietary )?
               NULL : ((array_info)&(proprietary->array))->data;

    c_info = (array_info) &(input_info->std_comps[(int)CONNECTIONS]->array);

    /* XXX - growth */

#if 0
    if ( bptr == NULL ) 
        DXMessage ( "offsets: NULL" );
    else
        DXMessage ( "offsets: %d, %d, %d, %d, %d, %d",
                    bptr->i_min, bptr->j_min, bptr->k_min,
                    bptr->i_max, bptr->j_max, bptr->k_max );
#endif

#define PUT_FACE(DN,IJK_MINMAX,VALUE) \
    if((bptr==NULL)||\
       (((VALUE)+((mesh_array_info)c_info)->offsets[DN])==bptr->IJK_MINMAX)) \
      if((ERROR==(field=jea_slab(input_info,DN,(VALUE))))|| \
        !DXSetEnumeratedMember((Group)output,nfields++,(Object)field))goto error

    switch ( input_info->std_comps[(int)CONNECTIONS]->element_type )
    {
        case QUADRILATERALS:
            DXDATAASSERTGOTO ( ((mesh_array_info)c_info)->n == 2 );

            t0 = (array_info) &(((struct _array_allocator *)
                                    ((mesh_array_info)c_info)->terms)[0]);
            t1 = (array_info) &(((struct _array_allocator *)
                                    ((mesh_array_info)c_info)->terms)[1]);

            PUT_FACE ( 0, i_min, 0 );
            PUT_FACE ( 1, j_min, 0 );
            PUT_FACE ( 0, i_max, t0->items );
            PUT_FACE ( 1, j_max, t1->items );
            break;

        case CUBES:
            DXDATAASSERTGOTO ( ((mesh_array_info)c_info)->n == 3 );

            t0 = (array_info) &(((struct _array_allocator *)
                                    ((mesh_array_info)c_info)->terms)[0]);
            t1 = (array_info) &(((struct _array_allocator *)
                                    ((mesh_array_info)c_info)->terms)[1]);
            t2 = (array_info) &(((struct _array_allocator *)
                                    ((mesh_array_info)c_info)->terms)[2]);

            PUT_FACE ( 0, i_min, 0 );
            PUT_FACE ( 1, j_min, 0 );
            PUT_FACE ( 2, k_min, 0 );
            PUT_FACE ( 0, i_max, t0->items );
            PUT_FACE ( 1, j_max, t1->items );
            PUT_FACE ( 2, k_max, t2->items );
            break;

        default:
            DXErrorGoto
                ( ERROR_DATA_INVALID,
                  "Bad \"connections\" element type" );
    }
#undef PUT_FACE

    DXDelete ( (Object) input_info->field );
    /* gets PROPRIETARY_COMPONENT_NAME */

    if ( nfields == 0 ) { DXDelete ( (Object) output );  output = NULL; }
    
    return output;

    error:
        DXASSERT ( DXGetError() != ERROR_NONE );
        return ERROR;
}



static
Field irregular_boundary
         ( field_info input_info, InvalidComponentHandle i_handle )
{
    array_info      c_info, n_info;
    component_info  proprietary = NULL;
    component_info  comp;

    int  i, j, k,   n, t;
    int  nP_in,  nC_out, nP_out;

    Array    array         = NULL;
    Pointer  data          = NULL;
    int      *o_conn_index = NULL;
    int      *i_posi_index = NULL;
    int      outshape;
    
    bounds_ptr bptr        = NULL;
    neighbor6  neighb;
    Cube       conn;

    int flip=0;

    Error (*get_neighbor)() = NULL;

    /* duplicated out of _helper_jea.h */

                       /* XXX - we don't have or know line neighb's */
    int  quad_to_line  [4][2] = { { 0, 1 }, { 3, 2 }, { 2, 0 }, { 1, 3 } };
    int  tri_to_line   [3][2] = { { 1, 2 }, { 2, 0 }, { 0, 1 } };
    int  cube_to_quad  [6][4]
             = { { 1, 0, 3, 2 }, { 4, 5, 6, 7 }, { 0, 1, 4, 5 },
                 { 2, 6, 3, 7 }, { 0, 4, 2, 6 }, { 1, 3, 5, 7 } };
    int  tetra_to_tri  [4][3]
             = { { 1, 3, 2 }, { 0, 2, 3 }, { 0, 3, 1 }, { 0, 1, 2 } };

    c_info = (array_info) &(input_info->std_comps[(int)CONNECTIONS]->array);
    nP_in  = ((array_info) &(input_info->std_comps[(int)POSITIONS]->array))
                 ->items;

    switch ( input_info->std_comps[(int)CONNECTIONS]->element_type )
    {
        case TETRAHEDRA:
        case CUBES:
            if ( ERROR == ( flip = _dxf_get_flip
                                       ( input_info,
                                         &global.have_connects,
                                         &global.have_nondegen ) ) )
                goto error;
#if 0
            DXMessage ( "irregular_boundary: get flip = %d", flip );
#endif
            break;
	 default:
	    break;
    }

    if ( input_info->std_comps[(int)NEIGHBORS] != NULL )
        n_info = (array_info) &(input_info->std_comps[(int)NEIGHBORS]->array);
    else if ( ( CLASS_MESHARRAY == c_info->class )
              &&
              ( TTRUE == ((mesh_array_info)c_info)->grid ) )
    {
        n_info = NULL;

        if ( ( ERROR == ( proprietary = _dxf_FindCompInfo
                                            ( input_info,
                                              PROPRIETARY_COMPONENT_NAME ) ) )
             &&
             ( DXGetError() != ERROR_NONE ) )
            goto error;

        bptr = (proprietary==NULL)?
                   NULL : ((array_info)&(proprietary->array))->data;

        switch ( input_info->std_comps[(int)CONNECTIONS]->element_type )
        {
          case QUADRILATERALS: get_neighbor = _dxf_get_neighb_grid_QUADS; break;
          case CUBES:          get_neighbor = _dxf_get_neighb_grid_CUBES; break;
	  default: break;
        }
    }
    else
        DXErrorGoto3
            ( ERROR_UNEXPECTED,
              "#10250", /*%s is missing %s component*/
              "'input' parameter",
              "\"neighbors\"" );


    /*
     * COUNT:  find total output connections, using variable in nC_out.
     *     N_NEIGHB - Number of neighbors in a connection.
     */
#define COUNT( N_NEIGHB ) \
    if ( i_handle == NULL ) \
        for ( i=0; i<c_info->items; i++ ) \
            for ( j=0; j<N_NEIGHB; j++ ) \
            { \
                if ( -1 == ( (int*) n_info->data ) [i*N_NEIGHB+j] ) \
                    nC_out ++; \
            } \
    else if ( n_info != NULL ) \
        for ( i=0; i<c_info->items; i++ ) \
        { \
            if ( !DXIsElementInvalid ( i_handle, i ) ) \
                for ( j=0; j<N_NEIGHB; j++) \
                    if ( ( -1 == ( n = \
                                 ((int *) n_info->data) [i*N_NEIGHB+j] ) ) \
                         || ( ( n > -1 ) && \
                         DXIsElementInvalid ( i_handle, n ) ) ) \
                        nC_out ++; \
        } \
    else \
        for ( i=0; i<c_info->items; i++ ) \
            if ( !DXIsElementInvalid ( i_handle, i ) ) { \
                if ( !get_neighbor ( i, c_info, bptr, &neighb ) ) \
                    goto error;\
                else \
                    for ( j=0; j<N_NEIGHB; j++ ) \
                        if ( ( -1 == ( n = ((int*)&neighb)[j] ) ) \
                             || ( ( n > -1 ) && \
                             DXIsElementInvalid ( i_handle, n ) ) ) \
                            nC_out ++; \
	    }



    nC_out = 0;
    switch ( input_info->std_comps[(int)CONNECTIONS]->element_type )
    {
        case TRIANGLES:      outshape = 2;  COUNT ( 3 ); break;
        case QUADRILATERALS: outshape = 2;  COUNT ( 4 ); break;
        case TETRAHEDRA:     outshape = 3;  COUNT ( 4 ); break;
        case CUBES:          outshape = 4;  COUNT ( 6 ); break;
        default:
            DXErrorGoto2
                ( ERROR_DATA_INVALID,
                  "#11380", /* %s is an invalid connections type */
                  input_info->std_comps[(int)CONNECTIONS]->name );
    }
#undef  COUNT

    if ( nC_out == 0 )
        return _dxf_MakeFieldEmpty ( input_info->field );

    else
        if ( ( ERROR == ( i_posi_index = (int *) DXAllocate
                                                     ( nP_in * sizeof(int) )) )
             ||
             ( ERROR == ( o_conn_index = (int *) DXAllocate
                                                     ( nC_out * sizeof(int) )) )
             ||
             ( ERROR == ( array = DXNewArray ( TYPE_INT, CATEGORY_REAL,
                                               1, outshape ) ) )
             ||
             !DXAllocateArray ( array, nC_out )          ||
             !DXAddArrayData  ( array, 0, nC_out, NULL ) ||
             !DXTrim          ( array )
             ||
             ( ERROR == ( data = DXGetArrayData ( array ) ) )
             ||
             !DXSetComponentValue
                  ( input_info->field, "connections", (Object) array ) )
            goto error;

    array = NULL;

    if ( !DXSetComponentAttribute
             ( input_info->field, "connections", "element type",
               (Object)DXNewString
                           ( ( outshape == 1 ) ? "points"    :
                             ( outshape == 2 ) ? "lines"     :
                             ( outshape == 3 ) ? "triangles" :
                             ( outshape == 4 ) ? "quads"     :
                                                 "ERROR" ) ) )
        goto error;

    for ( i=0; i<nP_in; i++ ) i_posi_index [ i ] = -1;

#if 0
DXMessage( "ci[%d][%d] = %d  -->>  co[%d][%d] = %d",
i,IN_TO_OUT[j][k],((int *) c_info->data)[i*N_CONN+IN_TO_OUT[j][k]],
nC_out,k,((int *) data) [nC_out*N_CONN_OUT+k] );
#endif

    /*
     * CONVERT:
     *     N_NEIGHB   - Number of neighbors in a connection.
     *     N_CONN     - Number of indices in an input connection.
     *     N_CONN_OUT - Number of indices in an output connection.
     *     IN_TO_OUT  - mapping of a neighbor to a conn. spanning that face.
     */
#define CONVERT(N_NEIGHB,N_CONN,N_CONN_OUT,IN_TO_OUT) \
    if ( i_handle == NULL ) { \
        for ( i=0; i<c_info->items; i++ ) \
            for ( j=0; j<N_NEIGHB; j++ ) \
            { \
                if ( -1 == ((int *) n_info->data) [i*N_NEIGHB+j] ) \
                { \
                    for ( k=0; k<N_CONN_OUT; k++ ) \
                    { \
                        t = ( (int*) c_info->data ) \
                                [i*N_CONN+IN_TO_OUT[j][k]]; \
                        \
                        if ( i_posi_index[t] == -1 ) \
                            i_posi_index[t] = nP_out++; \
                        \
                        ((int *) data) [nC_out*N_CONN_OUT+k] \
                            = i_posi_index[t]; \
                    } \
                    o_conn_index [ nC_out++ ] = i; \
                } \
            } \
    } else if ( n_info != NULL ) { \
        for ( i=0; i<c_info->items; i++ ) \
        { \
            if ( !DXIsElementInvalid ( i_handle, i ) ) \
                for ( j=0; j<N_NEIGHB; j++ ) \
                { \
                    if ( ( -1 == ( n = \
                                 ( (int*) n_info->data) [i*N_NEIGHB+j] ) ) \
                         || ( ( n > -1 ) && \
                         DXIsElementInvalid ( i_handle, n ) ) ) \
                    { \
                        for ( k=0; k<N_CONN_OUT; k++ ) \
                        { \
                            t = ( (int*) c_info->data ) \
                                    [i*N_CONN+IN_TO_OUT[j][k]]; \
                            \
                            if ( i_posi_index[t] == -1 ) \
                                i_posi_index[t] = nP_out++; \
                            \
                            ( (int*) data ) [nC_out*N_CONN_OUT+k] \
                                = i_posi_index[t]; \
                        } \
                        o_conn_index [ nC_out++ ] = i; \
                    } \
                } \
        } \
    } else { \
        for ( i=0; i<c_info->items; i++ ) \
            if ( !DXIsElementInvalid ( i_handle, i ) ) { \
                if ( !get_neighbor ( i, c_info, bptr, &neighb ) ) \
                    goto error; \
                else \
                    for ( j=0; j<N_NEIGHB; j++ ) \
                        if ( ( -1 == ( n = ((int*)&neighb) [j] ) ) \
                             || ( ( n > -1 ) && \
                             DXIsElementInvalid ( i_handle, n ) ) ) \
                        { \
                            if ( !c_info->get_item ( i, c_info, &conn ) ) \
                                goto error; \
                            else \
                                for ( k=0; k<N_CONN_OUT; k++ ) \
                                { \
                                    t = ((int*)&conn) [IN_TO_OUT[j][k]]; \
                                    \
                                    if ( i_posi_index[t] == -1 ) \
                                        i_posi_index[t] = nP_out++; \
                                    \
                                    ( (int*) data ) [nC_out*N_CONN_OUT+k] \
                                        = i_posi_index[t]; \
                                } \
                                o_conn_index [ nC_out++ ] = i; \
                        } \
	    } \
     }


    nC_out = 0;
    nP_out = 0;
    switch ( input_info->std_comps[(int)CONNECTIONS]->element_type )
    {
        case TRIANGLES:      CONVERT ( 3, 3, 2,   tri_to_line  );  break;
        case QUADRILATERALS: CONVERT ( 4, 4, 2,  quad_to_line  );  break;
        case TETRAHEDRA:     CONVERT ( 4, 4, 3, tetra_to_tri   );  break;
        case CUBES:          CONVERT ( 6, 8, 4,  cube_to_quad  );  break;
        default: break;
    }
#undef  CONVERT


    /*
     * For each component in input:
     *     Determine if copied unchanged, already handled,
     *     discarded, or remapped.
     */
    for ( i=0, comp=input_info->comp_list;
          i<input_info->comp_count;
          i++, comp++ )
    {
        array = NULL;

#if 0
DXMessage ( "---------------------------------------------------------------" );
DXMessage ( "The name of the dog is %s", comp->name );
#endif

        if ( 0 == strcmp ( comp->name, "color map" ) || 
	     0 == strcmp ( comp->name, "opacity map" ) )
        {
            array = ((array_info)&comp->array)->array;
        }
        else if ( ( 0 == strncmp ( comp->name, "original", 8 ) ) ||
                  ( 0 == strcmp  ( comp->name, "normals"     ) )   )
        {
            array = NULL;
        }
        else if ( ( NULL  != comp->std_attribs[(int)DEP]        ) &&
                  ( ERROR != comp->std_attribs[(int)DEP]->value )   )
        {
            if (0 == strcmp ( comp->std_attribs[(int)DEP]->value, "positions" ))
            {
                if ( ERROR == ( array = _dxf_resample_array
                                            ( comp, 1 /* DEP */, nP_in, nP_out,
                                              i_posi_index, NULL ) ) )
                    goto error;
            }
            else if ( ( 0 == strcmp ( comp->std_attribs[(int)DEP]->value,
                                      "connections" ) ) &&
                      ( 0 != strcmp ( comp->name, "connections" ) ) )
            {
                if ( ERROR == ( array = _dxf_resample_array
                                            ( comp, 1 /* DEP */,
                                              c_info->items, nC_out,
                                              NULL, o_conn_index ) ) )
                    goto error;
            }
        }
        else if ( ( NULL  != comp->std_attribs[(int)REF]        ) &&
                  ( ERROR != comp->std_attribs[(int)REF]->value ) &&
                  ( NULL  == i_handle ) &&
                  ( ( 0 == strcmp ( comp->name, "invalid connections" ) ) ||
                    ( 0 == strcmp ( comp->name, "invalid positions"   ) )   ) )
        {
            if (0 == strcmp ( comp->std_attribs[(int)REF]->value, "positions" ))
            {
                if ( ERROR == ( array = _dxf_resample_array
                                            ( comp, 2 /* REF */, nP_in, nP_out,
                                              i_posi_index, NULL ) ) )
                    goto error;
            }
            else if (0 == strcmp ( comp->std_attribs[(int)REF]->value,
                                   "connections" ) )
            {
                if ( ERROR == ( array = _dxf_resample_array
                                            ( comp, 2 /* REF */,
                                              c_info->items, nC_out,
                                              NULL, o_conn_index ) ) )
                    goto error;
            }
        }

        if ( 0 != strcmp ( comp->name, "connections" ) )
            if ( !DXSetComponentValue
                      ( input_info->field, comp->name, (Object)array ) )
                goto error;
    }

    DXFree ( (Pointer) i_posi_index );  i_posi_index = NULL;
    DXFree ( (Pointer) o_conn_index );  o_conn_index = NULL;

    /* XXX - remove "der"'s ! (This gets the follow-on der's as well) */
    if ( !DXChangedComponentValues ( input_info->field, "positions"   ) ||
         !DXChangedComponentValues ( input_info->field, "connections" )  )
        goto error;

    switch ( input_info->std_comps[(int)CONNECTIONS]->element_type )
    {
        case TETRAHEDRA:
        case CUBES:
            if ( !make_normals ( input_info->field, nP_out, flip ) )
                goto error;
            break;
    	default:
	    break;
    }

    if ( !_dxf_SetDefaultColor ( input_info->field, DEFAULT_BOUND_COLOR ) )
        goto error;

    return input_info->field;

    error:
        DXFree ( (Pointer) i_posi_index );  i_posi_index = NULL;
        DXFree ( (Pointer) o_conn_index );  o_conn_index = NULL;

        DXASSERT ( DXGetError() != ERROR_NONE );
        return ERROR;
}


static
Field show_boundary ( Field input, char *arg, int args )
{
    field_info  input_info     = NULL;
    array_info  c_info = NULL;
    int         option;
    Object      output;

    InvalidComponentHandle  i_handle  = NULL;

    DXASSERTGOTO ( input != ERROR );
    DXASSERTGOTO ( DXGetObjectClass ( (Object)input ) == CLASS_FIELD );
    DXASSERTGOTO ( ( arg != ERROR ) && ( args == sizeof(int) ) );

    option = *((int *)arg);

    if ( ( ERROR == ( output = DXCopy ( (Object)input, COPY_HEADER ) ) )
         ||
         !DXInvalidateConnections ( (Object)output ) )
        goto error;

    if ( DXEmptyField ( (Field) output ) )
       return (Field) output;

    if ( !DXExists    ( output, "neighbors" ) &&
         !DXNeighbors ( (Field)output       ) &&
         ( DXGetError() != ERROR_NONE ) )
        goto error;

    if ( ( ERROR == ( input_info = _dxf_InMemory ( (Field)output ) ) ) )
        goto error;

    if ( input_info->std_comps[(int)POSITIONS] == NULL )
        DXErrorGoto3
            ( ERROR_DATA_INVALID,
              "#10250", /*%s is missing %s component*/
              "'input' parameter",
              "\"positions\"" );

    if ( input_info->std_comps[(int)CONNECTIONS] == NULL )
        DXErrorGoto3
            ( ERROR_DATA_INVALID,
              "#10250", /*%s is missing %s component*/
              "'input' parameter",
              "\"connections\"" );

    if ( LINES == input_info->std_comps[(int)CONNECTIONS]->element_type )
        DXErrorGoto2
            ( ERROR_DATA_INVALID,
              "#10340", /* %s must be 2D or 3D */
              "\"connections\"" ); 

    /*p_info = (array_info) &(input_info->std_comps[(int)POSITIONS]->array);*/
    c_info = (array_info) &(input_info->std_comps[(int)CONNECTIONS]->array);

    if ( !_dxf_SetIterator ( input_info->std_comps[(int)CONNECTIONS] ) )
        goto error;

    if ( ( option == 1 )
         &&
         ( NULL == input_info->std_comps[(int)INVALID_CONNECTIONS] ) )
        option = 0;

    if ( ( option == 1 )
         &&
         ( ERROR == ( i_handle = DXCreateInvalidComponentHandle
                                     ( (Object)input_info->field,
                                       NULL, "connections" ) ) ) )
        goto error;

    if ( ( CLASS_MESHARRAY == c_info->class )
         &&
         ( TTRUE == ((mesh_array_info)c_info)->grid )
         &&
         ( 0 == option ) )
    {
        if ( ( ERROR == ( output = (Object) regular_boundary ( input_info ) ) )
             &&
             ( DXGetError() != ERROR_NONE ) )
            goto error;
    }
    else
    {
        if ( ( ERROR == ( output = (Object) irregular_boundary
                                              ( input_info, i_handle ) ) )
             &&
             ( DXGetError() != ERROR_NONE ) )
            goto error;
    }

    DXFreeInvalidComponentHandle ( i_handle );  i_handle = NULL;

    if ( !_dxf_FreeInMemory ( input_info ) ) goto error;   input_info = NULL;

    return (Field) output;

    error:
        DXDelete          ( output );       output     = NULL;
        _dxf_FreeInMemory ( input_info );   input_info = NULL;

        DXASSERT ( DXGetError() != ERROR_NONE );
        return ERROR;
}
