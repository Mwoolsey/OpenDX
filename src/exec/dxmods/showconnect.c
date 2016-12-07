/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dxmods/showconnect.c,v 1.8 2005/10/19 21:23:23 davidt Exp $
 */

#include <dxconfig.h>


/***
MODULE:
    ShowConnections
SHORTDESCRIPTION:
    shows the outline of connective elements
CATEGORY:
    Realization
INPUTS:
    input; field; NULL; field to show connections of
OUTPUTS: 
    output; color field; NULL; original field with connections outlined
BUGS:
AUTHOR:
    John E. Allain
END:
***/

#include <string.h>
#include <stdio.h>
#include <dx/dx.h>
#include "_helper_jea.h"

#define DEFAULT_CONNECT_COLOR    DXRGB ( 0.7, 0.7, 0.0 )


typedef struct { HashTable table;  int count; }  hash_table_rec;
typedef struct { Line line; char invalid; }      hash_element_rec;
typedef hash_element_rec                         *hash_element_ptr;


static Error process_triangles
         ( Field field, hash_table_rec *htab, InvalidComponentHandle i_handle );
static Error process_quads
         ( Field field, hash_table_rec *htab, InvalidComponentHandle i_handle );
static Error process_cubes
         ( Field field, hash_table_rec *htab, InvalidComponentHandle i_handle );
static Error process_tetrahedra 
         ( Field field, hash_table_rec *htab, InvalidComponentHandle i_handle );
static Error process_faces_loops_edges
         ( Field field, hash_table_rec *htab, InvalidComponentHandle i_handle );

static Field field_show_connections ( Field field, char *dummy, int dummysize );
 /* Application dependant */



static
int compare_func ( Key i, Element j )
{
    return ( ( ( ((Line *)i)->p == ((Line *)j)->p ) &&
               ( ((Line *)i)->q == ((Line *)j)->q )   ) ? 0 : 1 );
}


static
PseudoKey hash_func ( Key i )
{
#define  k1       ((int)((Line *)i)->p)
#define  k2       ((int)((Line *)i)->q)
#define  PRIME_1  10607 
#define  PRIME_2  18913   

    PseudoKey address = ( ( k1 * PRIME_1 ) + ( k2 * PRIME_2 ) );

    return address;

#undef  k1
#undef  k2
}


static
Error store_hash ( hash_table_rec *hash,  PointId p,  PointId q, char invalid )
{
    hash_element_rec element;
    hash_element_ptr query;

    if ( p <= q ) { element.line.p = p; element.line.q = q; }
    else          { element.line.p = q; element.line.q = p; }

    if ( NULL == ( query = (hash_element_ptr) DXQueryHashElement
                                                ( hash->table, (Key)&element )))
    {
        element.invalid = invalid;

        if ( !DXInsertHashElement ( hash->table, (Element)&element ) )
            goto error;
        else
            hash->count++;
    }
    else
        if ( !invalid ) query->invalid = invalid;

    return OK;

    error:
        DXASSERT ( DXGetError() != ERROR_NONE );
        return ERROR;
}



int m_ShowConnections ( Object *in, Object *out )
{
    if ( ERROR == ( out[0] = DXProcessParts
                             ( in[0], field_show_connections, NULL, 0, 1, 1 ) )
         &&
         ( DXGetError() != ERROR_NONE ) )
        goto error;

    return OK;

    error:
        if ( out[0] ) DXDelete ( out[0] );

        out[0] = NULL;

        DXASSERT ( DXGetError() != ERROR_NONE );
        return ERROR;
}



static 
Field field_show_connections ( Field in_field, char *dummy, int dummysize )
{
    Line      *lines_ptr;
    Array     larray  = NULL;
    hash_table_rec htab;
    hash_element_ptr elem_ptr;
    int       np;
    Object    o_ptr;
    Object    conn_element;
    char      *conn_elem_name;
    Field     out_field  = NULL;
    int       i;
    Array     posi;

    InvalidComponentHandle i_handle = NULL;

    /* we keep connections, they are removed upon set component time */
    static char  *removables[] = { "faces", "loops", "edges", NULL };

    htab.table = NULL;

    /* 
     * Ensure that input field is Valid
     */

    DXASSERTGOTO ( in_field  != NULL );
    DXASSERTGOTO ( dummy     == NULL );
    DXASSERTGOTO ( dummysize == 0 );
    DXASSERTGOTO ( CLASS_FIELD == DXGetObjectClass ( (Object)in_field ) );

    if ( DXEmptyField ( in_field ) ) 
        return NULL;

    /* Find number of points for either regular or irregular data */

    if ( ERROR == ( posi = (Array) DXGetComponentValue
                                       ( in_field, "positions" ) ) )
        DXErrorGoto2 ( ERROR_DATA_INVALID, 
                       "#10240", /* missing %s component */ "\"positions\"" )


    else if ( DXGetObjectClass ( (Object) posi ) != CLASS_ARRAY )
        DXErrorGoto2 ( ERROR_BAD_CLASS,
                       "#4414", /* `%s' component is not an array */
                       "\"positions\"" )

    else if ( !DXGetArrayInfo ( posi, &np, NULL, NULL, NULL, NULL ) )
        goto error;


    if ( ERROR == ( out_field
                      = (Field) DXCopy ( (Object) in_field, COPY_STRUCTURE ) ) )
        goto error;

    if (NULL != (o_ptr = DXGetComponentValue(in_field, "polylines")))
    {
        /*
         * Fast exit, we already have results.
         */
	if ( !_dxf_SetDefaultColor ( out_field, DEFAULT_CONNECT_COLOR )
             ||
             !_dxf_SetFuzzAttribute ( out_field, 1 ) )
            goto error;
        return DXEndField ( out_field );
    }

   /*
    * Remove "connections", etc. from output field if present.
    */

    for ( i=0; removables[i] != NULL; i++ )

        if ( DXDeleteComponent ( out_field, removables[i] ) )
        {
            if ( !DXChangedComponentStructure ( out_field, removables[i] ) )
                goto error;
        }
        else if ( DXGetError() != ERROR_NONE )
            goto error;

   /*
    * New considerations with regards to "normals".
    */
   if ( !DXSetIntegerAttribute ( (Object)out_field, "shade", 0 ) )
       goto error;

    /*
     * Determine which type of input connectivity we have,
     *   perform the appropriate action.
     */

    if ( NULL != ( o_ptr = DXGetComponentValue ( in_field, "faces" ) ) )
        conn_elem_name = "faces";

    else if ( NULL != ( o_ptr = DXGetComponentValue
                                    ( in_field, "connections" ) ))
    {
        if ( ERROR == ( conn_element = DXGetComponentAttribute
                                           ( in_field,
                                             "connections", "element type" ) ) )
            DXErrorGoto3
                ( ERROR_MISSING_DATA,
                  "#10255", /* %s component is missing the `%s' attribute */
                  "\"connections\"",
                  "element type" )

        else if ( DXGetObjectClass ( conn_element ) != CLASS_STRING)
            DXErrorGoto2 ( ERROR_BAD_CLASS,
                          "#11080", /* name for object %d must be a string */
                          "connections component element type" )

        else if ( ERROR == ( conn_elem_name
                                 = DXGetString ( (String) conn_element ) ) )
            DXErrorGoto ( ERROR_INTERNAL,
                        "connections element type string not retrievable" )

        /*
         * Will invalidity be a factor?
         */

        /* XXX cost.  Can this be done while hashing? */
        if ( !DXInvalidateConnections           ( (Object) out_field ) ||
             !DXInvalidateUnreferencedPositions ( (Object) out_field )
             ||
             ( ERROR == ( i_handle = DXCreateInvalidComponentHandle
                                         ( (Object) out_field,
                                           NULL, "connections" ) ) ) )
             goto error;

        if ( 0 == DXGetInvalidCount ( i_handle ) )
        {
            DXFreeInvalidComponentHandle ( i_handle );
            i_handle = NULL;
        }

    }
    else
        DXErrorGoto2
            ( ERROR_MISSING_DATA,
              "#10240", /* missing %s component */
              "\"connections\" or (\"face\",\"loop\",\"edge\")" )

    htab.count = 0;
    if ( ERROR == ( htab.table = DXCreateHash
                                     ( sizeof(hash_element_rec),
                                       hash_func, compare_func ) ) )
        goto error;

    if ( strcmp ( conn_elem_name, "faces" ) == 0 )
    {
        if ( !process_faces_loops_edges ( in_field, &htab, i_handle ) )
            goto error;
    }
    else if ( strcmp ( conn_elem_name, "triangles" ) == 0 )
    {
        if ( !process_triangles ( in_field, &htab, i_handle ) )
            goto error;
    }
    else if ( strcmp ( conn_elem_name, "quads" ) == 0 )
    {
        if ( !process_quads ( in_field, &htab, i_handle ) )
            goto error;
    }
    else if ( strcmp ( conn_elem_name, "cubes" ) == 0 )
    {
        if ( !process_cubes ( in_field, &htab, i_handle ) )
            goto error;
    }
    else if ( strcmp ( conn_elem_name, "tetrahedra" ) == 0 )
    {
        if ( !process_tetrahedra ( in_field, &htab, i_handle ) )
            goto error;
    }
    else if ( strcmp ( conn_elem_name, "lines" ) == 0 )
    {
        /*
         * Fast exit, we already have results.
         *   Lines connectivity component is what ShowConnections constructs.
         *   Make sure that colors exist and leave.
         */

        DXDestroyHash ( htab.table );  htab.table = NULL;

        if ( !DXSetComponentValue
                 ( out_field, "connections", o_ptr )
             ||
             !DXSetComponentAttribute
                 ( out_field, "connections", "element type",
                   (Object) DXNewString ( "lines" ) )
             ||
             !_dxf_SetDefaultColor
                 ( out_field, DEFAULT_CONNECT_COLOR )
             ||
             !_dxf_SetFuzzAttribute ( out_field, 1 ) )
            goto error;

        return DXEndField ( out_field );
    }
    else
        DXErrorGoto2
            ( ERROR_INTERNAL,
              "#11380", /* %s is an invalid connections type */
              conn_elem_name )

    /*
     * Destroy connections dependent color, data.
     */
    if ( !DXChangedComponentStructure ( out_field, "connections" ) )
        goto error;

    if ( !DXExists ( (Object)out_field, "colors" ) )
        if ( !DXDeleteComponent ( out_field, "color map" )
             && ( DXGetError() != ERROR_NONE ) )
            goto error;


    /*
     * Convert edges in storage within hash table
     *   to component lines.
     *     Construct "lines" component of appropriate length.
     *     Extract from 'htab', place in component's array.
     *     Add colors if not already present.
     */

    if ( ( ERROR == ( larray = _dxf_NewComponentArray
                          ( LINE_CONNECTIONS_COMP, &htab.count, NULL, NULL ) ) )
         ||
         ( ERROR == ( lines_ptr = (Line *) DXGetArrayData ( larray ) ) )
         ||
         !DXSetComponentValue ( out_field, "connections", (Object)larray ) )
        goto error;

    larray = NULL;

    if ( !DXSetComponentAttribute
              ( out_field,
                "connections",
                "element type",
                (Object) DXNewString ( "lines" ) )
         ||
         !DXSetComponentValue ( out_field, "invalid connections", NULL )
         ||
         !DXInitGetNextHashElement ( htab.table ) )
        goto error;

    if ( i_handle != NULL )
    {
        if ( !DXFreeInvalidComponentHandle ( i_handle )
             ||
             ( ERROR == ( i_handle
                             = DXCreateInvalidComponentHandle
                                   ( (Object)out_field, NULL, "connections" ))))
             goto error;

        for ( i = 0;
              ( NULL != ( elem_ptr = (hash_element_ptr) DXGetNextHashElement
                                                           ( htab.table ) ) );
              i++, lines_ptr++ )
        {
            *lines_ptr = elem_ptr->line;

            if ( elem_ptr->invalid && !DXSetElementInvalid ( i_handle, i ) )
                goto error;
        }

        if ( !DXSaveInvalidComponent       ( out_field, i_handle ) ||
             !DXFreeInvalidComponentHandle ( i_handle ) )
            goto error;

        i_handle = NULL;
    }
    else
        for ( i = 0;
              ( NULL != ( elem_ptr = (hash_element_ptr) DXGetNextHashElement
                                                           ( htab.table ) ) );
              i++, lines_ptr++ )
            *lines_ptr = elem_ptr->line;

    if ( i != htab.count )
        DXErrorGoto
            ( ERROR_INTERNAL,
              "Unexpected end of table in DXGetNextHashElement call" );

    if ( ( DXGetError() != ERROR_NONE )
         ||
         !_dxf_SetDefaultColor  ( out_field, DEFAULT_CONNECT_COLOR )
         ||
         !_dxf_SetFuzzAttribute ( out_field, 1 )
         ||
         !DXDestroyHash ( htab.table ) )
        goto error;

    htab.table = NULL;

    return DXEndField ( out_field );

    error:
        if ( htab.table ) DXDestroyHash( htab.table );
        if ( larray     ) DXDelete     ( (Object) larray );
        if ( out_field  ) DXDelete     ( (Object) out_field );

        DXASSERT ( DXGetError() != ERROR_NONE );
        return ERROR;
}



static
Error process_triangles
          ( Field field, hash_table_rec *htab, InvalidComponentHandle i_handle )
{
    Triangle  *tris_ptr;
    char      invalid = 0;
    int	      nt;
    int	      i;

    /* 3 edges per triangle, each of which may be shared by 2 triangles */

    if ( ERROR == _dxf_GetComponentData( (Object)field,
                                         TRIANGLE_CONNECTIONS_COMP,
                                         &nt, NULL, NULL,
                                         (Pointer *) &tris_ptr ) )
        goto error;

    for ( i=0; i<nt; i++, tris_ptr++ )
    {
        invalid = (i_handle==NULL)? 0 :
                  DXIsElementInvalidSequential ( i_handle, i );

        if ( !store_hash ( htab, tris_ptr->p, tris_ptr->q, invalid ) ||
             !store_hash ( htab, tris_ptr->q, tris_ptr->r, invalid ) ||
             !store_hash ( htab, tris_ptr->r, tris_ptr->p, invalid )   )
            goto error;
    }

    return OK;

    error:
        DXASSERT ( DXGetError() != ERROR_NONE );
        return ERROR;
}



static
Error process_quads
          ( Field field, hash_table_rec *htab, InvalidComponentHandle i_handle )
{
    Quadrilateral *quads_ptr;
    char      invalid = 0;
    int	          nq;
    int	          i;
    /*----------------*/
    /* quad ordering: */
    /*    p  q        */
    /*    r  s        */
    /*----------------*/
    /* 4 edges per quad, each of which may be shared by 2 quads */

    if ( ERROR == _dxf_GetComponentData( (Object)field,
                                         QUAD_CONNECTIONS_COMP,
                                         &nq, NULL, NULL,
                                         (Pointer *) &quads_ptr ) )
        goto error;

    for ( i=0; i<nq; i++, quads_ptr++ )
    {
        invalid = (i_handle==NULL)? 0 :
                  DXIsElementInvalidSequential ( i_handle, i );

        if ( !store_hash ( htab, quads_ptr->p, quads_ptr->q, invalid ) ||
             !store_hash ( htab, quads_ptr->q, quads_ptr->s, invalid ) ||
             !store_hash ( htab, quads_ptr->r, quads_ptr->s, invalid ) ||
             !store_hash ( htab, quads_ptr->r, quads_ptr->p, invalid )   )
            goto error;
    }

    return OK;

    error:
        DXASSERT ( DXGetError() != ERROR_NONE );
        return ERROR;
}



static
Error process_faces_loops_edges
          ( Field field, hash_table_rec *htab, InvalidComponentHandle i_handle )
{
    Array     array;
    char      invalid = 0;
    int       *face_ptr, *fp, *fp_end, *fp_last;  /* base, cur, to, last */
    int       *loop_ptr, *lp, *lp_end, *lp_last;
    int       *edge_ptr, *ep, *ep_end, *ep_first;
    int	      nf;
    int	      nl;
    int	      ne;

#define ERROR_GET_DAT(I_c,O_p,O_n,EE) \
    if((ERROR==(array=(Array)DXGetComponentValue(field,I_c)))|| \
       (!DXGetArrayInfo(array,&O_n,NULL,NULL,NULL,NULL))|| \
       (ERROR==(O_p=(int*)DXGetArrayData(array)))) \
        DXErrorGoto(ERROR_DATA_INVALID,EE)

    ERROR_GET_DAT ( "faces", face_ptr, nf, "#13180" /* bad faces */ );
    ERROR_GET_DAT ( "loops", loop_ptr, nl, "#13190" /* no loops */  );
    ERROR_GET_DAT ( "edges", edge_ptr, ne, "#13200" /* no edges */  );

#undef  ERROR_GET_DAT

    fp_last = &face_ptr [ nf - 1 ];
    lp_last = &loop_ptr [ nl - 1 ];
    /*ep_last = &edge_ptr [ ne - 1 ];*/

    fp_end = fp_last;
    fp     = face_ptr;

    while ( fp <= fp_end )
    {
        lp_end = &loop_ptr [ ( ( fp != fp_last ) ? fp[1] : nl ) - 1 ];
        lp     = &loop_ptr [ fp[0] ];

        while ( lp <= lp_end )
        {
            ep_end = &edge_ptr [ ( ( lp != lp_last ) ? lp[1] : ne ) - 1 ];
            ep_first = ep = &edge_ptr [ lp[0] ];

            while ( ep < ep_end )  /* Different inequality from previous two */
            {
                if ( !store_hash ( htab, ep[0], ep[1], invalid ) )
                    goto error;

                ep++;
            }

            if ( !store_hash ( htab, ep_first[0], ep_end[0], invalid ) )
                goto error;

            lp++;
        }

        fp++;
    }

    return OK;

    error:
        DXASSERT ( DXGetError() != ERROR_NONE );
        return ERROR;
}



static
Error process_tetrahedra
          ( Field field, hash_table_rec *htab, InvalidComponentHandle i_handle )
{
    Tetrahedron *tetras_ptr;
    char      invalid = 0;
    int	      nt;
    int	      i;
    /* 6 edges per tetrahedra, each of which may be shared by 6 tetrahedras */

    if ( ERROR == _dxf_GetComponentData( (Object)field,
                                         TETRA_CONNECTIONS_COMP,
                                         &nt, NULL, NULL,
                                         (Pointer *) &tetras_ptr ) )
        goto error;

    for ( i=0; i<nt; i++, tetras_ptr++ )
    {
        invalid = (i_handle==NULL)? 0 :
                  DXIsElementInvalidSequential ( i_handle, i );

        if ( !store_hash ( htab, tetras_ptr->p, tetras_ptr->q, invalid ) ||
             !store_hash ( htab, tetras_ptr->p, tetras_ptr->r, invalid ) ||
             !store_hash ( htab, tetras_ptr->p, tetras_ptr->s, invalid ) ||
             !store_hash ( htab, tetras_ptr->q, tetras_ptr->r, invalid ) ||
             !store_hash ( htab, tetras_ptr->r, tetras_ptr->s, invalid ) ||
             !store_hash ( htab, tetras_ptr->s, tetras_ptr->q, invalid )   )
            goto error;
    }

    return OK;

    error:
        DXASSERT ( DXGetError() != ERROR_NONE );
        return ERROR;
}



static
Error process_cubes
          ( Field field, hash_table_rec *htab, InvalidComponentHandle i_handle )
{
    /*----------------*/
    /* cube ordering: */
    /*     p   q      */
    /*    r   s       */
    /*     t   u      */
    /*    v   w       */
    /*----------------*/
    Cube      *cubes_ptr;
    char      invalid = 0;
    int	      nc;
    int	      i;
    /* 12 edges per cube, each of which may be shared by 4 cubes */

    if ( ERROR == _dxf_GetComponentData( (Object)field,
                                         CUBE_CONNECTIONS_COMP,
                                         &nc, NULL, NULL,
                                         (Pointer *) &cubes_ptr ) )
        goto error;

    for ( i=0; i<nc; i++, cubes_ptr++ )
    {
        invalid = (i_handle==NULL)? 0 :
                  DXIsElementInvalidSequential ( i_handle, i );

        if ( !store_hash ( htab, cubes_ptr->p, cubes_ptr->q, invalid ) ||
             !store_hash ( htab, cubes_ptr->q, cubes_ptr->s, invalid ) ||
             !store_hash ( htab, cubes_ptr->s, cubes_ptr->r, invalid ) ||
             !store_hash ( htab, cubes_ptr->r, cubes_ptr->p, invalid ) ||
             !store_hash ( htab, cubes_ptr->p, cubes_ptr->t, invalid ) ||
             !store_hash ( htab, cubes_ptr->q, cubes_ptr->u, invalid ) ||
             !store_hash ( htab, cubes_ptr->s, cubes_ptr->w, invalid ) ||
             !store_hash ( htab, cubes_ptr->r, cubes_ptr->v, invalid ) ||
             !store_hash ( htab, cubes_ptr->t, cubes_ptr->u, invalid ) ||
             !store_hash ( htab, cubes_ptr->u, cubes_ptr->w, invalid ) ||
             !store_hash ( htab, cubes_ptr->v, cubes_ptr->w, invalid ) ||
             !store_hash ( htab, cubes_ptr->v, cubes_ptr->t, invalid )    )
            goto error;
    }

    return OK;

    error:
        DXASSERT ( DXGetError() != ERROR_NONE );
        return ERROR;
}
