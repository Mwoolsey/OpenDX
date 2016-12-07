/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dxmods/_getfield.c,v 1.7 2002/03/21 02:57:26 rhh Exp $
 */

#include <dxconfig.h>




#if !defined _GETFIELD_COMPILATION_PASS
#define      _GETFIELD_COMPILATION_PASS  1    /*------------------------------*/
#endif


#if ( _GETFIELD_COMPILATION_PASS == 1 )

#include <string.h>
#include <dx/dx.h> 
#include "_getfield.h"


#define ERROR_SECTION if ( DXGetError() == ERROR_NONE ) \
                  DXDebug ( "D", "No error set at %s:%d", __FILE__,__LINE__)


/* 
 * Purpose:
 *   To access a Field's data in one step, performing necessary checks for 
 *   consistency.
 *
 * Philosophical question:
 *    If something is the (product,mesh) of all irregular, what do you call it?
 *    Takes 3N space rather than N**3 so it is compact.
 */

/*
 * XXX -
 * Enhance:
 *  SetIterator
 * Assumptions:
 *  Irregular arrays are always ArrayClass = CLASS_ARRAY.
 *  Components       are always arrays.
 *  Attributes       are always strings.
 *    (But field rendering attributes may be floats, etc.)
 *  GridConnections are always MeshArray's.
 *  GridPositions   are always ProductArray's.
 *  Therefore, stand alone PathArray or RegularArray cannot be Grid*'s.
 *  No ProductArrays or MeshArray's will have less than 1 term.
 *
 *    Connections may be:       Positions may be:
 *      Array                     Array
 *      RegularArray              PathArray
 *      GridPositions             GridConnections
 *         ProductArray              MeshArray
 *            RegularArray              PathArray
 *      ProductArray              MeshArray
 *         RegularArray              PathArray
 *         Array                     Array
 * 
 *  Constant array support.
 *  Require that:
 *    positions shape at least matches connection element type
 *    positions be rank 1
 *    positions be float
 *    connections be int
 *    connections be rank 1
 *    element type matches shape[0]
 *    data dep positions or connections 
 *    connections be a classified element type
 *    connections ref positions
 *    invalid connections are scalar byte dep connections
 */


#define  ALLOCATE    DXAllocate
#define  REALLOCATE  DXReAllocate

  /*
   * ALLO: DXAllocate structs.  INCR: Increase allocation.
   * 
   * NAME - name of the function instantiated
   * PTR  - pointer type of output and init
   * REC  - record type having superclass size
   * INIT - initial value of result
   */
#define NEW_ALLO( NAME, PTR, REC, INIT ) \
    static PTR NAME ( int count ) \
    { /*int i;*/  PTR p = (PTR)ALLOCATE((count)*sizeof(struct REC)); \
      if ( p == ERROR ) return ERROR; \
      /*for ( i=0; i<count; i++ ) p[i] = INIT;*/  return p; }


#define NEW_INCR( NAME, PTR, REC, INIT ) \
    static PTR NAME ( PTR p, int count, int start ) \
    { int i;  p = (PTR)REALLOCATE((Pointer)p,((count)*sizeof(struct REC))); \
      if ( p == ERROR ) return ERROR; \
      /*for ( i=start; i<count; i++ ) p[i] = INIT;*/  return p; }


/* ------------------------------------------------------------------------- */

NEW_ALLO ( ALLO_attribute_info, attribute_info, _attribute_info,  NULL )
NEW_ALLO ( ALLO_array_info,     array_info,     _array_allocator, NULL )
NEW_ALLO ( ALLO_component_info, component_info, _component_info,  NULL )
NEW_ALLO ( ALLO_field_info,     field_info,     _field_info,      NULL )

/* ----------------------------- "New" section ----------------------------- */


static 
array_info in_memory_array ( Array input, array_info output ) 
{
    Array  *arrays = NULL;
    int    i, grid_dim;

    DXASSERTGOTO ( ERROR != input );
    DXASSERTGOTO ( ERROR != output );
    DXASSERTGOTO ( DXGetObjectClass ( (Object)input ) == CLASS_ARRAY );

    output->array    = input;
    output->get_item = NULL;

    if ( !DXGetArrayInfo
              ( output->array, NULL, NULL, NULL, &output->rank, NULL ) )
        goto error;

    if ( output->rank == 0 ) output->shape = NULL;
    else
        if ( ERROR == ( output->shape
                 = (int *) ALLOCATE ( output->rank * sizeof(int) ) ) )
            goto error;

    if ( !DXGetArrayInfo ( output->array,
                           &output->items,
                           &output->type,
                           &output->category,
                           &output->rank,
                           &output->shape[0] ) )
        goto error;

    output->original_items = output->items;
    output->itemsize       = DXGetItemSize ( output->array );
    output->data           = NULL;
    output->handle         = NULL;

    switch ( output->class = DXGetArrayClass ( input ) )
    {
        case CLASS_ARRAY:
            if ( ERROR == ( output->data = DXGetArrayData ( output->array ) ) )
                goto error;
            break;

        case CLASS_CONSTANTARRAY:
        case CLASS_REGULARARRAY:
            {
            int count;

            if ( DXQueryConstantArray ( output->array, NULL, NULL ) )
                output->class = CLASS_CONSTANTARRAY;
            else
                if ( output->class == CLASS_CONSTANTARRAY )
                    goto error;

            if ( ( ERROR == ( ((regular_array_info)output)->origin
                                  = ALLOCATE ( output->itemsize ) ) )
                 ||
                 ( ERROR == ( ((regular_array_info)output)->delta
                                  = ALLOCATE ( output->itemsize ) ) ) )
                goto error;

            if ( !memset ( (char *)(((regular_array_info)output)->delta),
                           0, 
                           output->itemsize ) )
                DXErrorGoto2
                    ( ERROR_UNEXPECTED,
                      "#11800", /* C standard library call, %s, returns error */
                      "memset()" );

            if ( ( output->class == CLASS_CONSTANTARRAY )
                 &&
                 !DXQueryConstantArray
                      ( output->array,
                        &count,
                        ((regular_array_info)output)->origin ) )
                goto error;

            if ( ( output->class == CLASS_REGULARARRAY )
                 &&
                 !DXGetRegularArrayInfo
                      ( (RegularArray)output->array,
                         &count,
                         ((regular_array_info)output)->origin,
                         ((regular_array_info)output)->delta ) )
                goto error;

            if ( output->items != count )
            {
                DXDebug
                    ( "D",
                      "Internal error is (%s) assertion failed. (at \"%s\":%d)",
                      "( output->items == count )",
                      __FILE__, __LINE__ );
            
                DXErrorGoto
                    ( ERROR_INTERNAL, "DXGetRegularArrayInfo item count" );
            }
            }
            break;

        case CLASS_PATHARRAY:
            {
            int count;

            if ( !DXGetPathArrayInfo ( (PathArray)output->array, &count )
                 ||
                 !DXGetPathOffset ( (PathArray)output->array,
                                  &(((path_array_info)output)->offset) ) )
                goto error;

            if ( output->items != ( count - 1 ) )
            {
                DXDebug
                    ( "D",
                      "Internal error is (%s) assertion failed. (at \"%s\":%d)",
                      "( output->items == ( count - 1 ) )",
                      __FILE__, __LINE__ );
            
                DXErrorGoto
                    ( ERROR_INTERNAL, "DXGetPathArrayInfo item count" );
            }
            }
            break;

        case CLASS_PRODUCTARRAY:
            if ( !DXGetProductArrayInfo 
                      ( (ProductArray)output->array,
                        &(((product_array_info)output)->n),
                        NULL )
                 ||
                 ( ERROR == ( arrays = (Array *) ALLOCATE
                       ( ((product_array_info)output)->n * sizeof(Array) ) ) )
                 ||
                 !DXGetProductArrayInfo 
                      ( (ProductArray)output->array,
                        &(((product_array_info)output)->n),
                        &arrays[0] )
                 ||
                 ( ERROR == ( ((product_array_info)output)->terms
                                  = ALLO_array_info
                                        ( ((product_array_info)output)->n ))) )
                goto error;

            for ( i=0; i<((product_array_info)output)->n; i++ )
                if ( !in_memory_array
                          ( arrays[i], 
                            /*
                             * This messy expression should be required
                             *    in only four places.
                             */
                            (array_info)
                                &(((struct _array_allocator *)
                                    ((product_array_info)output)->terms)[i]) ) )
                    goto error;

            DXFree ( (Pointer) arrays );  arrays = NULL;

            if ( !DXQueryGridPositions
                      ( output->array, &grid_dim, NULL, NULL, NULL ) )

                ((product_array_info) output)->grid = FFALSE;
            else
            {
                ((product_array_info) output)->grid = TTRUE;

                /* 3D positions may be assembled in a 2D shape.
                   But this is not the way Bruce implemented it */
                if ( ( output->shape == ERROR )
                     ||
                     ( output->rank != 1 )
                     ||
                     ( output->shape[0] != grid_dim )
                     ||
                     ( output->itemsize != ( grid_dim * sizeof(float) ) ) )
                {
                    DXDebug
                        ( "D",
                         "Internal error assertion failed. (at \"%s\":%d)",
                          __FILE__, __LINE__ );
                
                    DXErrorGoto
                        ( ERROR_INTERNAL, "call to DXQueryGridPositions" );
                }

                /* XXX move higher */
                if ( output->type != TYPE_FLOAT )
                    DXErrorGoto
                        ( ERROR_BAD_TYPE,
                          "positions component must be floating point type" );

                if ( ( ERROR == ( ((grid_positions_info)output)->counts
                                        = (int*) ALLOCATE
                                                   ( grid_dim * sizeof(int)) ) )
                     ||
                     ( ERROR == ( ((grid_positions_info)output)->origin
                                        = ALLOCATE ( output->itemsize ) ) )
                     ||
                     ( ERROR == ( ((grid_positions_info)output)->deltas
                                        = ALLOCATE
                                              ( grid_dim * output->itemsize )) )
                     ||
                     !DXQueryGridPositions
                          ( output->array,
                            NULL,
                            ((grid_positions_info)output)->counts,
                            (float *)(((grid_positions_info)output)->origin),
                            (float *)(((grid_positions_info)output)->deltas)) )
                    goto error;
            }
            break;

        case CLASS_MESHARRAY:
            ((mesh_array_info)output)->grow_offsets = NULL;

            if ( !DXGetMeshArrayInfo
                      ( (MeshArray)output->array,
                        &(((mesh_array_info)output)->n),
                        NULL )
                 ||
                 ( ERROR == ( arrays = (Array*) ALLOCATE
                       ( ((mesh_array_info)output)->n * sizeof(Array) ) ) )
                 ||
                 !DXGetMeshArrayInfo
                      ( (MeshArray)output->array,
                        &((mesh_array_info)output)->n,
                        &arrays[0] )
                 ||
                 ( ERROR == ( ((mesh_array_info)output)->terms
                                  = ALLO_array_info
                                        ( ((mesh_array_info)output)->n ) ) ) )
                goto error;

            for ( i=0; i<((mesh_array_info)output)->n; i++ )
                if ( !in_memory_array
                          ( arrays[i], 
                            /*
                             * This messy expression should be required
                             *    in only four places.
                             */
                            (array_info)
                                &(((struct _array_allocator *)
                                    ((product_array_info)output)->terms)[i]) ) )
                    goto error;

            DXFree ( (Pointer) arrays );  arrays = NULL;

            if ( !DXQueryGridConnections ( output->array, &grid_dim, NULL ) )

                ((mesh_array_info) output)->grid = FFALSE;
            else
            {
                ((mesh_array_info) output)->grid = TTRUE;

                if ( ( output->shape == ERROR )
                     ||
                     ( output->rank != 1 )
                     ||
                     ( output->shape[0] != (1<<grid_dim) )
                     ||
                     ( output->itemsize != ( output->shape[0] * sizeof(int) )) )
                {
                    DXDebug
                        ( "D",
                          "Internal error is assertion failed. (at \"%s\":%d)",
                          __FILE__, __LINE__ );
                
                    DXErrorGoto
                        ( ERROR_INTERNAL, "call to DXQueryGridConnections" );
                }

                /* XXX move higher */
                if ( output->type != TYPE_INT )
                    DXErrorGoto
                        ( ERROR_BAD_TYPE,
                          "connections component must be integer type" );

                if ( ( ERROR == ( ((grid_connections_info)output)->counts
                                     = (int *) ALLOCATE
                                         ( output->shape[0] * sizeof(int) ) ) )
                     ||
                     ( ERROR == ( ((mesh_array_info)output)->offsets
                                      = (int *)ALLOCATE
                               ( ((mesh_array_info)output)->n * sizeof(int) )))
                     ||
                     !DXQueryGridConnections
                         ( output->array,
                           NULL,
                           ((grid_connections_info)output)->counts )
                     ||
                     /* XXX - will this be Meshes and not Grid's */
                     !DXGetMeshOffsets
                          ( (MeshArray)output->array,
                            ((mesh_array_info)output)->offsets ) )
                    goto error;
            }
            break;

        default:;
    }

    return output;

    error:
        ERROR_SECTION;
        return ERROR;
}


static 
attribute_info in_memory_attribute (Object input, attribute_info output, int n )
{
    Object o = NULL;

    DXASSERTGOTO ( ERROR != input );
    DXASSERTGOTO ( ERROR != output );

    if ( ERROR == ( o = DXGetEnumeratedAttribute ( input, n, &output->name ) ) )
        goto error;

    if ( ( output->name == NULL ) || ( strlen ( output->name ) == 0 ) )
        DXErrorGoto3
            ( ERROR_DATA_INVALID,
              "#10210", /* `%s' is not a valid string for %s */
              "NULL", "the attribute name" );


    if ( DXGetObjectClass ( o ) == CLASS_STRING )
    {
        if ( ERROR == ( output->value = DXGetString ( (String) o ) ) )
            goto error;
    }
    else
        output->value = NULL; /* XXX */

    return output;

    error:
        ERROR_SECTION;
        return ERROR;
}



static 
component_info in_memory_component ( Field input, component_info output, int n )
{
    Object o = NULL;
    int i;

    DXASSERTGOTO ( ERROR != input );
    DXASSERTGOTO ( ERROR != output );

    if ( ERROR == ( o = DXGetEnumeratedComponentValue
                            ( input, n, &output->name ) ) )
        goto error;

    if ( output->name == NULL )
        DXErrorGoto3
            ( ERROR_DATA_INVALID,
              "#10210", /* `%s' is not a valid string for %s */
              "NULL",
              "the component name" );

    if ( DXGetObjectClass ( o ) != CLASS_ARRAY )
        DXErrorGoto2
            ( ERROR_DATA_INVALID,
              "#4414", /* `%s' component is not an array */
                       /* XXX - #4414 is a Warning class message */
              output->name );

    if ( !in_memory_array ( (Array) o, (array_info)&output->array ) )
        goto error;

    for ( output->attrib_count = 0;
          ( NULL != DXGetEnumeratedAttribute
                        ( o, output->attrib_count, NULL ) );
          output->attrib_count++ );

    if ( DXGetError() != ERROR_NONE )
        goto error;
    
    if ( ERROR == ( output->attrib_list
                        = ALLO_attribute_info ( output->attrib_count ) ) )
        goto error;

    for ( i=0; i<STD_COMPONENT_ATTRIBUTE_LIM; i++ )
        output->std_attribs [ i ] = NULL;

    output->element_type = NONE;

    for ( i=0; i<output->attrib_count; i++ )
    {
        if ( !in_memory_attribute ( o, &output->attrib_list[i], i ) )
            goto error;

#define MATCH_SUB(A,S)  ( strcmp ( output->attrib_list[i].A, S ) == 0 )

        if ( MATCH_SUB ( name, "dep" ) )
            output->std_attribs [ (int)DEP ] = &output->attrib_list[i];

        else if ( MATCH_SUB ( name, "ref" ) )
            output->std_attribs [ (int)REF ] = &output->attrib_list[i];

        else if ( MATCH_SUB ( name, "der" ) )
            output->std_attribs [ (int)DER ] = &output->attrib_list[i];

        else if ( MATCH_SUB ( name, "element type" ) )
        {
            DXASSERTGOTO ( output->element_type == NONE );
            DXASSERTGOTO ( output->attrib_list[i].value != NULL );

            output->std_attribs [ (int)DX_ELEMENT_TYPE ] = &output->attrib_list[i];

            if ( MATCH_SUB ( value, "lines" ) )
                output->element_type = LINES;

            else if ( MATCH_SUB ( value, "triangles" ) )
                output->element_type = TRIANGLES;

            else if ( MATCH_SUB ( value, "quads" ) )
                output->element_type = QUADRILATERALS;

            else if ( MATCH_SUB ( value, "tetrahedra" ) )
                output->element_type = TETRAHEDRA;

            else if ( MATCH_SUB ( value, "cubes" ) )
                output->element_type = CUBES;

            else if ( MATCH_SUB ( value, "prisms" ) )
                output->element_type = PRISMS;

            else if ( MATCH_SUB ( value, "cubes4D" ) )
                output->element_type = CUBES4D;

            else 
            {
                output->element_type = ET_UNKNOWN;

                DXErrorGoto3
                    ( ERROR_DATA_INVALID,
                      "#10210", /* `%s' is not a valid string for %s */
                      output->attrib_list[i].value,
                      "the \"element type\" attribute" );
            }
#undef  MATCH_SUB

        }
        else 
            DXDebug
                ( "D", "component attribute \"%s\" is not supported", 
                  output->attrib_list[i].name );
    }

    return output;

    error:
        ERROR_SECTION;
        return ERROR;
}


static
field_info in_memory_field ( Field input, field_info output )
{
    int i;

    DXASSERTGOTO ( input  != ERROR );
    DXASSERTGOTO ( output != ERROR );
    DXASSERTGOTO ( DXGetObjectClass ( (Object) input ) == CLASS_FIELD );

    output->field = input;

    for ( output->comp_count = 0;
          (NULL != DXGetEnumeratedComponentValue
                       ( input, output->comp_count, NULL ));
          output->comp_count++ );

    if ( DXGetError() != ERROR_NONE )
        goto error;
    
    if ( ERROR == ( output->comp_list
                        = ALLO_component_info ( output->comp_count ) ) )
        goto error;

    for ( i=0; i<STD_COMPONENT_LIM; i++ )
        output->std_comps [i] = NULL;

    for ( i=0; i<output->comp_count; i++ )
    {
        if ( !in_memory_component ( input, &output->comp_list[i], i ) )
            goto error;

#define MATCH_SUB(A,S)  ( strcmp ( output->comp_list[i].A, S ) == 0 )

        if ( MATCH_SUB ( name, "positions" ) )
        {
            output->std_comps [ (int)POSITIONS ] = &output->comp_list[i];
            output->comp_list[i].std_name        = POSITIONS;
        }
        else if ( MATCH_SUB ( name, "connections" ) )
        {
            output->std_comps [ (int)CONNECTIONS ] = &output->comp_list[i];
            output->comp_list[i].std_name          = CONNECTIONS;
        }
        else if ( MATCH_SUB ( name, "data" ) )
        {
            output->std_comps [ (int)DATA ] = &output->comp_list[i];
            output->comp_list[i].std_name   = DATA;
        }
        else if ( MATCH_SUB ( name, "neighbors" ) )
        {
            output->std_comps [ (int)NEIGHBORS ] = &output->comp_list[i];
            output->comp_list[i].std_name        = NEIGHBORS;
        }
        else if ( MATCH_SUB ( name, "normals" ) )
        {
            output->std_comps [ (int)NORMALS ] = &output->comp_list[i];
            output->comp_list[i].std_name      = NORMALS;
        }
        else if ( MATCH_SUB ( name, "colors" ) )
        {
            output->std_comps [ (int)COLORS ] = &output->comp_list[i];
            output->comp_list[i].std_name     = COLORS;
        }
        else if ( MATCH_SUB ( name, "opacity" ) )
        {
            output->std_comps [ (int)OPACITY ] = &output->comp_list[i];
            output->comp_list[i].std_name      = OPACITY;
        }
        else if ( MATCH_SUB ( name, "invalid positions" ) )
        {
            output->std_comps [ (int)INVALID_POSITIONS ]
                = &output->comp_list[i];
            output->comp_list[i].std_name = INVALID_POSITIONS;
        }
        else if ( MATCH_SUB ( name, "invalid connections" ) )
        {
            output->std_comps [ (int)INVALID_CONNECTIONS ]
                = &output->comp_list[i];
            output->comp_list[i].std_name = INVALID_CONNECTIONS;
        }
        else if ( MATCH_SUB ( name, "original positions" ) )
        {
            output->std_comps [ (int)ORIGINAL_POSITIONS ]
                = &output->comp_list[i];
            output->comp_list[i].std_name = ORIGINAL_POSITIONS;
        }
        else if ( MATCH_SUB ( name, "color map" ) )
        {
            output->std_comps [ (int)COLOR_MAP ] = &output->comp_list[i];
            output->comp_list[i].std_name        = COLOR_MAP;
        }
        else if ( MATCH_SUB ( name, "front colors" ) )
        {
            output->std_comps [ (int)FRONT_COLORS ] = &output->comp_list[i];
            output->comp_list[i].std_name           = FRONT_COLORS;
        }
        else if ( MATCH_SUB ( name, "original positions" ) )
        {
            output->std_comps [ (int)BACK_COLORS ] = &output->comp_list[i];
            output->comp_list[i].std_name          = BACK_COLORS;
        }
        else 
            output->comp_list[i].std_name = NOT_A_STANDARD;

#undef  MATCH_SUB
    }

    if ( NULL != DXQueryOriginalSizes ( input, NULL, NULL ) )
    {
        /* XXX - setting the values in components dep {posit,connect}ions */

        if ( !DXQueryOriginalSizes
                  ( input,
                    &((array_info)
                         &(output->std_comps[(int)POSITIONS]->array))
                               ->original_items,
                    &((array_info)
                         &(output->std_comps[(int)CONNECTIONS]->array))
                               ->original_items ) )
            goto error;

        if ( ((array_info)&(output->std_comps[(int)CONNECTIONS]->array))->class
              == CLASS_MESHARRAY )
        {
            int *original_sizes = NULL;
            int i;

            mesh_array_info
                c_info = (mesh_array_info)
                              &(output->std_comps[(int)CONNECTIONS]->array);

            if ( ( ERROR == ( c_info->grow_offsets
                                  = (int*)ALLOCATE ( c_info->n * sizeof(int) )))
                 ||
                 ( ERROR == ( original_sizes
                                  = (int*)ALLOCATE ( c_info->n * sizeof(int) )))
                 ||
                 !DXQueryOriginalMeshExtents
                      ( input,
                        c_info->grow_offsets,
                        original_sizes ) )
                goto error;

#if 0
            /* XXX - mind the dimensionality, not necessarily 3 */
            if ( ( original_sizes[0] * original_sizes[1] * original_sizes[2] )
                 !=  
                 ((array_info)&(output->std_comps[(int)POSITIONS]->array))
                     ->original_items )
            {
                DXSetError
                    ( ERROR_DATA_INVALID, "#11450" );
              /* Object internal consistency failure.  Use the Verify module. */
                DXDebug
                    ( "D",
                      "original mesh extents for positions"
                      " must equal original sizes" );
            }

            if ( ( ( original_sizes[0] - 1 ) *
                   ( original_sizes[1] - 1 ) *
                   ( original_sizes[2] - 1 ) )
                 !=  
                 ((array_info)&(output->std_comps[(int)CONNECTIONS]->array))
                     ->original_items )
            {
                DXSetError
                    ( ERROR_DATA_INVALID, "#11450" );
              /* Object internal consistency failure.  Use the Verify module. */
                DXDebug
                    ( "D",
                      "( original mesh extents connections"
                      " == original sizes )" );
            }
#endif
            for ( i=0; i<c_info->n; i++ )
            {
                /* remember, numbers are counts of positions */
                ((array_info)
                    &(((struct _array_allocator *)c_info->terms)[i]))
                         ->original_items = ( original_sizes[i] - 1 );

                if ( ((array_info)
                        &(((struct _array_allocator *)c_info->terms)[i]))
                         ->items < ( original_sizes[i] - 1 ) )
                {
                    DXSetError
                        ( ERROR_DATA_INVALID, "#11450" );
              /* Object internal consistency failure.  Use the Verify module. */
                    DXDebug
                        ( "D",
                          "( connections count (%d) >="
                          " original mesh extents (%d) - 1 ), term [%d]",
                          ((array_info)
                             &(((struct _array_allocator *)c_info->terms)[i]))
                                 ->items,
                          original_sizes[i], i );

                    goto error;
                }
            }

            DXFree ( (Pointer)original_sizes );
        }
    }

    return output;

    error:
        ERROR_SECTION;
        return ERROR;
}



/* ----------------------------- "Free" section ----------------------------- */



#define  SUPERFREE(x)  if ( x != NULL ) { DXFree ( (Pointer) x );  x = NULL; }


static 
Error free_attribute_info_contents ( attribute_info input ) { return OK; }


static 
Error free_array_info_contents ( array_info input )
{
    int i;

    DXASSERTGOTO ( input != ERROR );

    SUPERFREE ( input->shape )

    if ( ( NULL != input->handle ) && !DXFreeArrayHandle ( input->handle ) )
        goto error;

    input->handle = NULL;

    switch ( input->class )
    {
        case CLASS_ARRAY:
            break;

        case CLASS_CONSTANTARRAY:
        case CLASS_REGULARARRAY:
            SUPERFREE ( ((regular_array_info)input)->origin )
            SUPERFREE ( ((regular_array_info)input)->delta  )
            break;

        case CLASS_PATHARRAY:
            break;

        case CLASS_PRODUCTARRAY:
            if ( ((product_array_info)input)->terms == NULL )
                ((product_array_info)input)->n = 0;

            for ( i=0; i<((product_array_info)input)->n; i++ )
                if ( !free_array_info_contents
                        /*
                         * This messy expression should be required
                         *    in only four places.
                         */
                          ( (array_info)
                                &(((struct _array_allocator *)
                                    ((product_array_info)input)->terms)[i]) ) )
                    goto error;

            if ( ((product_array_info) input)->grid == TTRUE )
            {
                SUPERFREE ( ((grid_positions_info)input)->counts )
                SUPERFREE ( ((grid_positions_info)input)->origin )
                SUPERFREE ( ((grid_positions_info)input)->deltas )
            }

            SUPERFREE ( ((product_array_info)input)->terms )
            break;

        case CLASS_MESHARRAY:
            SUPERFREE ( ((mesh_array_info)input)->offsets )
            SUPERFREE ( ((mesh_array_info)input)->grow_offsets )

            if ( ((mesh_array_info)input)->terms == NULL )
                ((mesh_array_info)input)->n = 0;

            for ( i=0; i<((mesh_array_info)input)->n; i++ )
                if ( !free_array_info_contents
                        /*
                         * This messy expression should be required
                         *    in only four places.
                         */
                          ( (array_info)
                                &(((struct _array_allocator *)
                                    ((mesh_array_info)input)->terms)[i]) ) )
                    goto error;

            if ( ((mesh_array_info) input)->grid == TTRUE )
                SUPERFREE ( ((grid_connections_info)input)->counts )

            SUPERFREE ( ((mesh_array_info)input)->terms )
            break;
        default:
	    break;
    }

    return OK;

    error:
        ERROR_SECTION;
        return ERROR;
}



static 
Error free_component_info_contents ( component_info input )
{
    int i;

    DXASSERTGOTO ( input != ERROR );

    if ( input->attrib_list == NULL )
        input->attrib_count = 0;

    for ( i=0; i<input->attrib_count; i++ )
        if ( !free_attribute_info_contents ( &input->attrib_list[i] ) )
            goto error;

    SUPERFREE ( input->attrib_list )

    if ( !free_array_info_contents ( (array_info)&(input->array) ) )
        goto error;

    return OK;

    error:
        ERROR_SECTION;
        return ERROR;
}



static 
Error free_field_info_contents ( field_info input )
{
    int i;

    DXASSERTGOTO ( input != ERROR );

    if ( input->comp_list == NULL )
        input->comp_count = 0;

    for ( i=0; i<input->comp_count; i++ )
        if ( !free_component_info_contents ( &input->comp_list[i] ) )
            goto error;

    SUPERFREE ( input->comp_list )

    return OK;

    error:
        ERROR_SECTION;
        return ERROR;
}



/* ----------------------------- extern section ----------------------------- */


field_info _dxf_InMemory ( Field input )
{
    field_info output = NULL;

    DXASSERTGOTO ( input != ERROR );
    DXASSERTGOTO ( DXGetObjectClass ( (Object) input ) == CLASS_FIELD );

    if ( ( ERROR == ( output = ALLO_field_info ( 1 ) ) )
       ||
       !in_memory_field ( input, output ) )
        goto error;

    return output;

    error:
        ERROR_SECTION;
        _dxf_FreeInMemory ( output );

        return ERROR;
}


Error _dxf_FreeInMemory ( field_info input )
{
    if ( input != NULL ) 
        free_field_info_contents ( input );

    SUPERFREE ( input );

    return OK;
}



component_info _dxf_SetIterator ( component_info input )
{
    Error (*get_item) () = NULL;

    DXASSERTGOTO ( ERROR != input );
    DXASSERTGOTO ( ERROR != ((array_info)&input->array)->array );

    if ( NULL != ((array_info)&input->array)->get_item )
        return input;

    switch ( input->std_name )
    {
        case DATA:
            break;

        case POSITIONS:
            switch ( ((array_info)&input->array)->class )
            {
                case CLASS_PRODUCTARRAY:  
                    switch ( ((product_array_info)&input->array)->grid )
                    {
                        case TTRUE:
                            switch ( ((array_info)&input->array)->shape[0] )
                            {
                                case 1:
                                    get_item = _dxf_get_point_grid_1D;  break;
                                case 2:
                                    get_item = _dxf_get_point_grid_2D;  break;
                                case 3:
                                    get_item = _dxf_get_point_grid_3D;  break;
                                default:
                                    DXErrorGoto3
                                        ( ERROR_NOT_IMPLEMENTED,
                                          "#11390",
                                      /* %s exceeds maximum dimensionality %d */
                                          "\"positions\" component", 3 );
                            }
                            break;

                        default:;
                    }
                    break;
        
                case CLASS_ARRAY:
                    switch ( ((array_info)&input->array)->shape[0] )
                    {
                        case 1:  get_item = _dxf_get_point_irr_1D;  break;
                        case 2:  get_item = _dxf_get_point_irr_2D;  break;
                        case 3:  get_item = _dxf_get_point_irr_3D;  break;
                        default:
                            DXErrorGoto3
                                ( ERROR_NOT_IMPLEMENTED,
                                  "#11390",
                                  /* %s exceeds maximum dimensionality %d */
                                  "\"positions\" component", 3 );
                    }
                    break;

                default:;
            }
            break;

        case CONNECTIONS:
            switch ( input->element_type )
            {
                case LINES:
                    switch ( ((array_info)&input->array)->class )
                    {
                        case CLASS_MESHARRAY:
                            switch ( ((mesh_array_info)&input->array)->grid )
                            {
                                case TTRUE: get_item = _dxf_get_conn_grid_LINES;
                                    break;

                                default:;
                            }
                            break;
        
                        case CLASS_ARRAY: get_item = _dxf_get_conn_irr_LINES;
                            break;

                        default:;
                    }
                    break;
        
                case QUADRILATERALS:
                    switch ( ((array_info)&input->array)->class )
                    {
                        case CLASS_MESHARRAY:
                            switch ( ((mesh_array_info)&input->array)->grid )
                            {
                                case TTRUE: get_item = _dxf_get_conn_grid_QUADS;
                                    break;
        
                                default:;
                            }
                            break;

                        case CLASS_ARRAY: get_item = _dxf_get_conn_irr_QUADS;
                            break;

                        default:;
                    }
                    break;
        
                case TRIANGLES:
                    get_item = _dxf_get_conn_irr_TRIS;
                    break;
        
                case CUBES:
                    switch ( ((array_info)&input->array)->class )
                    {
                        case CLASS_MESHARRAY:
                            switch ( ((mesh_array_info)&input->array)->grid )
                            {
                                case TTRUE: get_item = _dxf_get_conn_grid_CUBES;
                                    break;
        
                                default:;
                            }
                            break;
        
                        case CLASS_ARRAY:
                            get_item = _dxf_get_conn_irr_CUBES;
                            break;

                        default:;
                    }
                    break;

                case TETRAHEDRA:
                    get_item = _dxf_get_conn_irr_TETS;
                    break;

                case PRISMS:
                    if ( ( NULL == ((array_info)&input->array)->data )
                         &&
                         ( ERROR == ( ((array_info)&input->array)->data
                               = DXGetArrayData
                                     ( ((array_info)&input->array)->array )) ) )
                        goto error;

                    get_item = _dxf_get_conn_irr_PRSMS;
                    break;

                case CUBES4D:
                    DXErrorGoto2
                        ( ERROR_DATA_INVALID,
                          "#11380", /* %s is an invalid connections type */
                          input->std_attribs[(int)DX_ELEMENT_TYPE]->value )
                    break;

                case NONE:
                case ET_UNKNOWN:
		    if (input->std_attribs[(int)DX_ELEMENT_TYPE]
			&&
		        input->std_attribs[(int)DX_ELEMENT_TYPE]->value)
			{
			    DXErrorGoto3
				( ERROR_DATA_INVALID,
				  "#10210", /* `%s' is not a valid string for %s */
				  input->std_attribs[(int)DX_ELEMENT_TYPE]->value,
				  "connections element type" );
			}
		      else  
			{
		            DXErrorGoto(ERROR_BAD_PARAMETER,
				    "missing element type for connections component");
			}
            }
            break;

        default:;
    }

    if ( NULL == get_item ) {
        if ( ERROR != ( ((array_info)&input->array)->handle
                 = DXCreateArrayHandle ( ((array_info)&input->array)->array )) )
            get_item = _dxf_get_item_by_handle;
        else
            goto error;
    }

    ((array_info)&input->array)->get_item = get_item;

    return input;

    error:
        ERROR_SECTION;
        return ERROR;
}



component_info _dxf_SetIterator_i ( component_info input )
{
    Error (*get_item) () = NULL;

    DXASSERTGOTO ( ERROR != input );
    DXASSERTGOTO ( ERROR != ((array_info)&input->array)->array );

    if ( NULL != ((array_info)&input->array)->get_item )
        return input;

    switch ( input->std_name )
    {
        case DATA:
            if ( ( ( CATEGORY_REAL == ((array_info)&input->array)->category ) &&
                 ( 0 == ((array_info)&input->array)->rank ) ) ||
                 ( ( ( 1 == ((array_info)&input->array)->rank     ) &&
                   ( 1 == ((array_info)&input->array)->shape[0] )   ) ) )

            switch ( ((array_info)&input->array)->class )
            {
                case CLASS_CONSTANTARRAY:
                case CLASS_REGULARARRAY:
                    switch ( ((array_info)&input->array)->type )
                    {
                        case TYPE_FLOAT:  get_item = _dxf_get_data_reg_FLOAT;
                             break;
                        case TYPE_BYTE:   get_item = _dxf_get_data_reg_BYTE;
                             break;
                        case TYPE_DOUBLE: get_item = _dxf_get_data_reg_DOUBLE;
                             break;
                        case TYPE_HYPER:  get_item = _dxf_get_data_reg_HYPER;
                             break;
                        case TYPE_INT:    get_item = _dxf_get_data_reg_INT;
                             break;
                        case TYPE_SHORT:  get_item = _dxf_get_data_reg_SHORT;
                             break;
                        case TYPE_UBYTE:  get_item = _dxf_get_data_reg_UBYTE;
                             break;
                        case TYPE_UINT:   get_item = _dxf_get_data_reg_UINT;
                             break;
                        case TYPE_USHORT: get_item = _dxf_get_data_reg_USHORT;
                             break;
		        default: break;
                    }
                    break;
        
                case CLASS_ARRAY:
                    switch ( ((array_info)&input->array)->type )
                    {
                        case TYPE_FLOAT:  get_item = _dxf_get_data_irr_FLOAT;
                             break;
                        case TYPE_BYTE:   get_item = _dxf_get_data_irr_BYTE;
                             break;
                        case TYPE_DOUBLE: get_item = _dxf_get_data_irr_DOUBLE;
                             break;
                        case TYPE_HYPER:  get_item = _dxf_get_data_irr_HYPER;
                             break;
                        case TYPE_INT:    get_item = _dxf_get_data_irr_INT;
                             break;
                        case TYPE_SHORT:  get_item = _dxf_get_data_irr_SHORT;
                             break;
                        case TYPE_UBYTE:  get_item = _dxf_get_data_irr_UBYTE;
                             break;
                        case TYPE_UINT:   get_item = _dxf_get_data_irr_UINT;
                             break;
                        case TYPE_USHORT: get_item = _dxf_get_data_irr_USHORT;
                             break;
		        default: break;
                    }
                    break;

                default:;
            }
            break;

        default:;
    }

    if ( NULL != get_item )
        ((array_info)&input->array)->get_item = get_item;
    else
        if ( !_dxf_SetIterator ( input ) )
            goto error;

    return input;

    error:
        ERROR_SECTION;
        return ERROR;
}



component_info _dxf_FindCompInfo ( field_info input, char *locate )
{
    component_info  output = NULL;
    int             i;

    DXASSERTGOTO ( ERROR != input );
    DXASSERTGOTO ( ERROR != input->comp_list );

    for ( i=0, output=input->comp_list;  i<input->comp_count;  i++, output++ )
        if ( 0 == strcmp ( output->name, locate ) )
            return output;

    return NULL;

    /* error: */
        ERROR_SECTION;
        return ERROR;
}



Error _dxf_get_item_by_handle ( int index, array_info d, Pointer out )
{
    Pointer returned = NULL;

    DXASSERTGOTO ( d != ERROR );
    DXASSERTGOTO ( index < d->items );
    DXASSERTGOTO ( ERROR != d->handle );

    if ( ERROR == ( returned = DXGetArrayEntry ( d->handle, index, out ) ) )
        goto error;

    /* if out is returned, it was used and set.  else set it */

    if ( ( returned != out )
         &&
         !memcpy ( (char*)out, (char*)returned, d->itemsize ) )
        DXErrorGoto2
            ( ERROR_UNEXPECTED,
              "#11800", /* C standard library call, %s, returns error */
              "memset()" );

    return OK;

    error:
        ERROR_SECTION;
        return ERROR;
}



#endif /* ( _GETFIELD_COMPILATION_PASS == 1 ) */


/*---------------------------------------------*\
 | Begin generic templates.
\*---------------------------------------------*/


#if ( defined GEN_get_data_irr ) && \
    ( defined GEN_DATA_TYPE    ) && \
    ( defined GEN_DX_TYPE      )
Error GEN_get_data_irr ( int index, array_info d, float *out )
{
    DXASSERTGOTO ( d != ERROR );
    DXASSERTGOTO ( d->data != NULL );
    DXASSERTGOTO ( d->type == GEN_DX_TYPE );
    DXASSERTGOTO ( index < d->items );
    DXASSERTGOTO
        ( ( d->rank == 0 ) || (( d->rank == 1 )&&( d->shape[0] == 1 )) );

    /* Simple functions like these are the stuff of #defines */
    *out = (float) ( (GEN_DATA_TYPE *)(d->data) ) [index];
 
    return OK;

    /* error: */
        ERROR_SECTION;
        return ERROR;
}
#endif



#if ( defined GEN_get_data_reg ) && \
    ( defined GEN_DATA_TYPE    ) && \
    ( defined GEN_DX_TYPE      )
Error GEN_get_data_reg ( int index, array_info d, float *out )
{
    DXASSERTGOTO ( d != ERROR );
    DXASSERTGOTO ( d->data == NULL );
    DXASSERTGOTO ( d->type == GEN_DX_TYPE );
    DXASSERTGOTO ( index < d->items );
    DXASSERTGOTO
        ( ( d->rank == 0 ) || (( d->rank == 1 )&&( d->shape[0] == 1 )) );

    /* Simple functions like these are the stuff of #defines */
    *out = (float)
           ( *((GEN_DATA_TYPE *)((regular_array_info)d)->origin)
             +
             ( *((GEN_DATA_TYPE *)((regular_array_info)d)->delta) * index ) );

    return OK;

    /* error: */
        ERROR_SECTION;
        return ERROR;
}
#endif



#if ( defined GEN_get_point_irr ) && \
    ( defined GEN_POINT_D       ) && \
    ( defined GEN_POINT_TYPE    )
Error GEN_get_point_irr ( int index, array_info a, GEN_POINT_TYPE *out )
{
    DXASSERTGOTO ( a != ERROR );
    DXASSERTGOTO ( a->class == CLASS_ARRAY );
    DXASSERTGOTO ( ( a->rank == 1 ) && ( a->shape[0] == GEN_POINT_D ) );
    DXASSERTGOTO ( a->data != NULL );
    DXASSERTGOTO ( index < a->items );

    *((GEN_POINT_TYPE *)out) = ((GEN_POINT_TYPE *)(a->data))[index];
 
    return OK;

    /* error: */
        ERROR_SECTION;
        return ERROR;
}
#endif



#if ( defined GEN_get_point_prod ) && \
    ( defined GEN_POINT_D        ) && \
    ( defined GEN_POINT_TYPE     ) && \
    ( defined GEN_DELTA_TYPE     )
Error GEN_get_point_prod ( int index, array_info a, GEN_POINT_TYPE *out )
{
    array_info      t0, t1, t2;
    GEN_DELTA_TYPE  pt;
    int             iu, iv, iw;
    int             count_1_count_2;

    DXASSERTGOTO ( a != ERROR );
    DXASSERTGOTO ( a->data == NULL );
    DXASSERTGOTO ( a->class == CLASS_PRODUCTARRAY );
    DXASSERTGOTO ( ( a->rank == 1 ) && ( a->shape[0] == GEN_POINT_D ) );
    DXASSERTGOTO ( ((product_array_info)a)->grid == FFALSE );
    DXASSERTGOTO ( index < a->items );

#if ( GEN_POINT_D == 1 )

    t0 = (array_info)
             &(((struct _array_allocator *)((product_array_info)a)->terms)[0]);

    DXASSERTGOTO ( ( t0->rank == 1 ) && ( t0->shape[0] == GEN_POINT_D ) );

    iu     = index;

#elif ( GEN_POINT_D == 2 )

    t0 = (array_info)
             &(((struct _array_allocator *)((product_array_info)a)->terms)[0]);
    t1 = (array_info)
             &(((struct _array_allocator *)((product_array_info)a)->terms)[1]);

    DXASSERTGOTO ( ( t0->rank == 1 ) && ( t0->shape[0] == GEN_POINT_D ) );
    DXASSERTGOTO ( ( t1->rank == 1 ) && ( t1->shape[0] == GEN_POINT_D ) );

    iu     = index / t1->items;
    index  = index - ( iu * t1->items );
    iv     = index;

#elif ( GEN_POINT_D == 3 )

    t0 = (array_info)
             &(((struct _array_allocator *)((product_array_info)a)->terms)[0]);
    t1 = (array_info)
             &(((struct _array_allocator *)((product_array_info)a)->terms)[1]);
    t2 = (array_info)
             &(((struct _array_allocator *)((product_array_info)a)->terms)[2]);

    DXASSERTGOTO ( ( t0->rank == 1 ) && ( t0->shape[0] == GEN_POINT_D ) );
    DXASSERTGOTO ( ( t1->rank == 1 ) && ( t1->shape[0] == GEN_POINT_D ) );
    DXASSERTGOTO ( ( t2->rank == 1 ) && ( t2->shape[0] == GEN_POINT_D ) );

    count_1_count_2 = t1->items * t2->items;

    iu     = index / count_1_count_2;
    index  = index - ( iu * count_1_count_2 );
    iv     = index / t2->items;
    index  = index - ( iv * t2->items );
    iw     = index;

#endif


#if ( GEN_POINT_D == 1 )

    if ( t0->class == CLASS_REGULARARRAY )
    {
        pt.du   = *((GEN_POINT_TYPE *)((regular_array_info)t0)->origin);
        pt.du.x += ((GEN_POINT_TYPE *)((regular_array_info)t0)->delta)->x * iu;
    }
    else
        pt.du = ((GEN_POINT_TYPE *)(t0->data))[iu];

    ((GEN_POINT_TYPE *)out)->x  =  pt.du.x;
 
#elif ( GEN_POINT_D == 2 )

    if ( t0->class == CLASS_REGULARARRAY )
    {
        pt.du = *((GEN_POINT_TYPE *)((regular_array_info)t0)->origin);
        pt.du.x += ((GEN_POINT_TYPE *)((regular_array_info)t0)->delta)->x * iu;
        pt.du.y += ((GEN_POINT_TYPE *)((regular_array_info)t0)->delta)->y * iu;
    }
    else
        pt.du = ((GEN_POINT_TYPE *)(t0->data))[iu];

    if ( t1->class == CLASS_REGULARARRAY )
    {
        pt.dv = *((GEN_POINT_TYPE *)((regular_array_info)t1)->origin);
        pt.dv.x += ((GEN_POINT_TYPE *)((regular_array_info)t1)->delta)->x * iv;
        pt.dv.y += ((GEN_POINT_TYPE *)((regular_array_info)t1)->delta)->y * iv;
    }
    else
        pt.dv = ((GEN_POINT_TYPE *)(t1->data))[iv];

    ((GEN_POINT_TYPE *)out)->x  =  pt.du.x  +  pt.dv.x;
    ((GEN_POINT_TYPE *)out)->y  =  pt.du.y  +  pt.dv.y;
 
#elif ( GEN_POINT_D == 3 )

    if ( t0->class == CLASS_REGULARARRAY )
    {
        pt.du = *((GEN_POINT_TYPE *)((regular_array_info)t0)->origin);
        pt.du.x += ((GEN_POINT_TYPE *)((regular_array_info)t0)->delta)->x * iu;
        pt.du.y += ((GEN_POINT_TYPE *)((regular_array_info)t0)->delta)->y * iu;
        pt.du.z += ((GEN_POINT_TYPE *)((regular_array_info)t0)->delta)->z * iu;
    }
    else
        pt.du = ((GEN_POINT_TYPE *)(t0->data))[iu];

    if ( t1->class == CLASS_REGULARARRAY )
    {
        pt.dv = *((GEN_POINT_TYPE *)((regular_array_info)t1)->origin);
        pt.dv.x += ((GEN_POINT_TYPE *)((regular_array_info)t1)->delta)->x * iv;
        pt.dv.y += ((GEN_POINT_TYPE *)((regular_array_info)t1)->delta)->y * iv;
        pt.dv.z += ((GEN_POINT_TYPE *)((regular_array_info)t1)->delta)->z * iv;
    }
    else
        pt.dv = ((GEN_POINT_TYPE *)(t1->data))[iv];

    if ( t2->class == CLASS_REGULARARRAY )
    {
        pt.dw = *((GEN_POINT_TYPE *)((regular_array_info)t2)->origin);
        pt.dw.x += ((GEN_POINT_TYPE *)((regular_array_info)t2)->delta)->x * iw;
        pt.dw.y += ((GEN_POINT_TYPE *)((regular_array_info)t2)->delta)->y * iw;
        pt.dw.z += ((GEN_POINT_TYPE *)((regular_array_info)t2)->delta)->z * iw;
    }
    else
        pt.dw = ((GEN_POINT_TYPE *)(t2->data))[iw];

    ((GEN_POINT_TYPE *)out)->x  =  pt.du.x  +  pt.dv.x  +  pt.dw.x;
    ((GEN_POINT_TYPE *)out)->y  =  pt.du.y  +  pt.dv.y  +  pt.dw.y;
    ((GEN_POINT_TYPE *)out)->z  =  pt.du.z  +  pt.dv.z  +  pt.dw.z;
 
#endif

    return OK;

    error:
        ERROR_SECTION;
        return ERROR;
}
#endif




#if ( defined GEN_get_point_grid ) && \
    ( defined GEN_POINT_D        ) && \
    ( defined GEN_POINT_TYPE     ) && \
    ( defined GEN_DELTA_TYPE     )
Error GEN_get_point_grid ( int index, array_info a, GEN_POINT_TYPE *out )
{
#if ( GEN_CONN_D == 1 ) || ( GEN_CONN_D == 2 ) || ( GEN_CONN_D == 3 )
    int    iu;
#endif
#if ( GEN_CONN_D == 2 ) || ( GEN_CONN_D == 3 )
    int    iv;
#endif
#if ( GEN_CONN_D == 3 )
    int    iw, count_1_count_2;
#endif

    DXASSERTGOTO ( a != ERROR );
    DXASSERTGOTO ( a->data == NULL );
    DXASSERTGOTO ( a->class == CLASS_PRODUCTARRAY );
    DXASSERTGOTO ( ( a->rank == 1 ) && ( a->shape[0] == GEN_POINT_D ) );
    DXASSERTGOTO ( ((product_array_info)a)->grid == TTRUE );
    DXASSERTGOTO ( index < a->items );
    DXASSERTGOTO ( ((grid_connections_info)a)->counts != NULL );

#if ( GEN_POINT_D == 1 )

    iu     = index;

#elif ( GEN_POINT_D == 2 )

    iu     = index / ((grid_positions_info)a)->counts[1];
    index  = index - ( iu * ((grid_positions_info)a)->counts[1] );
    iv     = index;

#elif ( GEN_POINT_D == 3 )

    count_1_count_2 = ((grid_positions_info)a)->counts[1]
                      * ((grid_positions_info)a)->counts[2];

    iu     = index / count_1_count_2;
    index  = index - ( iu * count_1_count_2 );
    iv     = index / ((grid_positions_info)a)->counts[2];
    index  = index - ( iv * ((grid_positions_info)a)->counts[2] );
    iw     = index;

#endif


#if ( GEN_POINT_D == 1 )

    ((GEN_POINT_TYPE *)out)->x
        = ((GEN_POINT_TYPE *)((grid_positions_info)a)->origin)->x 
          + ( ((GEN_DELTA_TYPE *)((grid_positions_info)a)->deltas)->du.x * iu );

#elif ( GEN_POINT_D == 2 )

    ((GEN_POINT_TYPE *)out)->x
        = ((GEN_POINT_TYPE *)((grid_positions_info)a)->origin)->x 
          + ( ((GEN_DELTA_TYPE *)((grid_positions_info)a)->deltas)->du.x * iu )
          + ( ((GEN_DELTA_TYPE *)((grid_positions_info)a)->deltas)->dv.x * iv );

    ((GEN_POINT_TYPE *)out)->y
        = ((GEN_POINT_TYPE *)((grid_positions_info)a)->origin)->y
          + ( ((GEN_DELTA_TYPE *)((grid_positions_info)a)->deltas)->du.y * iu )
          + ( ((GEN_DELTA_TYPE *)((grid_positions_info)a)->deltas)->dv.y * iv );

#elif ( GEN_POINT_D == 3 )

    ((GEN_POINT_TYPE *)out)->x
        = ((GEN_POINT_TYPE *)((grid_positions_info)a)->origin)->x 
          + ( ((GEN_DELTA_TYPE *)((grid_positions_info)a)->deltas)->du.x * iu )
          + ( ((GEN_DELTA_TYPE *)((grid_positions_info)a)->deltas)->dv.x * iv )
          + ( ((GEN_DELTA_TYPE *)((grid_positions_info)a)->deltas)->dw.x * iw );

    ((GEN_POINT_TYPE *)out)->y
        = ((GEN_POINT_TYPE *)((grid_positions_info)a)->origin)->y 
          + ( ((GEN_DELTA_TYPE *)((grid_positions_info)a)->deltas)->du.y * iu )
          + ( ((GEN_DELTA_TYPE *)((grid_positions_info)a)->deltas)->dv.y * iv )
          + ( ((GEN_DELTA_TYPE *)((grid_positions_info)a)->deltas)->dw.y * iw );

    ((GEN_POINT_TYPE *)out)->z
        = ((GEN_POINT_TYPE *)((grid_positions_info)a)->origin)->z
          + ( ((GEN_DELTA_TYPE *)((grid_positions_info)a)->deltas)->du.z * iu )
          + ( ((GEN_DELTA_TYPE *)((grid_positions_info)a)->deltas)->dv.z * iv )
          + ( ((GEN_DELTA_TYPE *)((grid_positions_info)a)->deltas)->dw.z * iw );

#endif

    return OK;

    /* error: */
        ERROR_SECTION;
        return ERROR;
}
#endif




#if ( defined GEN_get_conn_irr ) && \
    ( defined GEN_CONN_TYPE    ) && \
    ( defined GEN_CONN_SHAPE   )
Error GEN_get_conn_irr ( int index, array_info c, GEN_CONN_TYPE *out )
{
    DXASSERTGOTO ( c != ERROR );
    DXASSERTGOTO ( c->class == CLASS_ARRAY );
    DXASSERTGOTO ( ( c->rank == 1 ) && ( c->shape[0] == GEN_CONN_SHAPE ) );
    DXASSERTGOTO ( c->data != NULL );
    DXASSERTGOTO ( index < c->items );

    *((GEN_CONN_TYPE *)out) = ( (GEN_CONN_TYPE *) (c->data) ) [index];
 
    return OK;

    /* error: */
        ERROR_SECTION;
        return ERROR;
}
#endif



#if ( defined GEN_get_conn_grid ) && \
    ( defined GEN_CONN_D        ) && \
    ( defined GEN_CONN_TYPE     ) && \
    ( defined GEN_CONN_SHAPE    )
Error GEN_get_conn_grid ( int index, array_info c, GEN_CONN_TYPE *out )
{
#if ( GEN_CONN_D == 1 ) || ( GEN_CONN_D == 2 ) || ( GEN_CONN_D == 3 )
    int    iu;
#endif
#if ( GEN_CONN_D == 2 ) || ( GEN_CONN_D == 3 )
    int    i, iv, count_1;
#endif
#if ( GEN_CONN_D == 3 )
    int    iw, count_2, count_1_count_2;
#endif

    /* 
     * XXX - An irregular iterator using this will still
     *     have problems with grow.
     */ 

    DXASSERTGOTO ( c != ERROR );
    DXASSERTGOTO ( c->class == CLASS_MESHARRAY );
    DXASSERTGOTO ( ((mesh_array_info)c)->grid == TTRUE );
    DXASSERTGOTO ( ( c->rank == 1 ) && ( c->shape[0] == GEN_CONN_SHAPE ) );
    DXASSERTGOTO ( c->data == NULL );
    DXASSERTGOTO ( index < c->items );
    DXASSERTGOTO ( ((grid_connections_info)c)->counts != NULL );

    /* XXX - Don't forget grid connections counts is in terms of positions. */


#if ( GEN_CONN_D == 1 )

    iu = index;

#elif ( GEN_CONN_D == 2 )

    count_1 = ( ((grid_connections_info)c)->counts[1] - 1 );

    i  = index;
    iu = i / count_1;
    i  = i - ( iu * count_1 );
    iv = i;

    count_1 = count_1 + 1;

#elif ( GEN_CONN_D == 3 )

    count_1         = ( ((grid_connections_info)c)->counts[1] - 1 );
    count_2         = ( ((grid_connections_info)c)->counts[2] - 1 );
    count_1_count_2 = count_1 * count_2;

    i  = index;
    iu = i / count_1_count_2;
    i  = i - ( iu * count_1_count_2 );
    iv = i / count_2;
    i  = i - ( iv * count_2 );
    iw = i;

    count_1 = count_1 + 1;
    count_2 = count_2 + 1;
    count_1_count_2
            = count_1 * count_2;

#endif


#if ( GEN_CONN_D == 1 )

    ((GEN_CONN_TYPE *)out)->p = iu;
    ((GEN_CONN_TYPE *)out)->q = iu + 1;

#elif ( GEN_CONN_D == 2 )

    ((GEN_CONN_TYPE *)out)->p = ( iu * count_1 ) + iv;
    ((GEN_CONN_TYPE *)out)->q = ((GEN_CONN_TYPE *)out)->p + 1;
    ((GEN_CONN_TYPE *)out)->r = ((GEN_CONN_TYPE *)out)->p + count_1;
    ((GEN_CONN_TYPE *)out)->s = ((GEN_CONN_TYPE *)out)->r + 1;

#elif ( GEN_CONN_D == 3 )

    ((GEN_CONN_TYPE *)out)->p = ( iu * count_1_count_2 ) + ( iv * count_2 ) +iw;
    ((GEN_CONN_TYPE *)out)->q = ((GEN_CONN_TYPE *)out)->p + 1;
    ((GEN_CONN_TYPE *)out)->r = ((GEN_CONN_TYPE *)out)->p + count_2;
    ((GEN_CONN_TYPE *)out)->s = ((GEN_CONN_TYPE *)out)->r + 1;

    ((GEN_CONN_TYPE *)out)->t = ((GEN_CONN_TYPE *)out)->p + count_1_count_2;
    ((GEN_CONN_TYPE *)out)->u = ((GEN_CONN_TYPE *)out)->q + count_1_count_2;
    ((GEN_CONN_TYPE *)out)->v = ((GEN_CONN_TYPE *)out)->r + count_1_count_2;
    ((GEN_CONN_TYPE *)out)->w = ((GEN_CONN_TYPE *)out)->s + count_1_count_2;

#endif

    return OK;

    /* error: */
        ERROR_SECTION;
        return ERROR;
}
#endif



#if ( defined GEN_get_neighb_irr ) && \
    ( defined GEN_NEIGHB_TYPE    ) && \
    ( defined GEN_NEIGHB_SHAPE   )
Error GEN_get_neighb_irr ( int index, array_info n, GEN_NEIGHB_TYPE *out )
{
    DXASSERTGOTO ( n != ERROR );
    DXASSERTGOTO ( n->class == CLASS_ARRAY );
    DXASSERTGOTO ( ( n->rank == 1 ) && ( n->shape[0] == GEN_NEIGHB_SHAPE ) );
    DXASSERTGOTO ( n->data != NULL );
    DXASSERTGOTO ( index < n->items );

    *((GEN_NEIGHB_TYPE *)out) = ( (GEN_NEIGHB_TYPE *) (n->data) ) [index];
 
    return OK;

    /* error: */
        ERROR_SECTION;
        return ERROR;
}
#endif



#if ( defined GEN_get_neighb_grid ) && \
    ( defined GEN_NEIGHB_TYPE     ) && \
    ( defined GEN_CONN_D          ) && \
    ( defined GEN_CONN_SHAPE      )
Error GEN_get_neighb_grid
          ( int index, array_info c, mesh_bounds b, GEN_NEIGHB_TYPE *out )
{
#if ( GEN_CONN_D == 1 ) || ( GEN_CONN_D == 2 ) || ( GEN_CONN_D == 3 )
    int    iu, count_0;
#endif
#if ( GEN_CONN_D == 2 ) || ( GEN_CONN_D == 3 )
    int    i, iv, count_1;
#endif
#if ( GEN_CONN_D == 3 )
    int    iw, count_2, count_1_count_2;
#endif

    DXASSERTGOTO ( c != ERROR );
    DXASSERTGOTO ( c->class == CLASS_MESHARRAY );
    DXASSERTGOTO ( ( c->rank == 1 ) && ( c->shape[0] == GEN_CONN_SHAPE ) );
    DXASSERTGOTO ( c->data == NULL );
    DXASSERTGOTO ( index < c->items );
    DXASSERTGOTO ( ((mesh_array_info)c)->grid == TTRUE );
    DXASSERTGOTO ( ((grid_connections_info)c)->counts != NULL );

    /* XXX - Don't forget grid connections counts is in terms of positions. */

#if ( GEN_CONN_D == 1 )

    count_0 = ( ((grid_connections_info)c)->counts[0] - 1 );

    iu = index;

#elif ( GEN_CONN_D == 2 )

    count_0 = ( ((grid_connections_info)c)->counts[0] - 1 );
    count_1 = ( ((grid_connections_info)c)->counts[1] - 1 );

    i  = index;
    iu = i / count_1;
    i  = i - ( iu * count_1 );
    iv = i;

#elif ( GEN_CONN_D == 3 )

    count_0         = ( ((grid_connections_info)c)->counts[0] - 1 );
    count_1         = ( ((grid_connections_info)c)->counts[1] - 1 );
    count_2         = ( ((grid_connections_info)c)->counts[2] - 1 );
    count_1_count_2 = count_1 * count_2;

    i  = index;
    iu = i / count_1_count_2;
    i  = i - ( iu * count_1_count_2 );
    iv = i / count_2;
    i  = i - ( iv * count_2 );
    iw = i;

#endif


#if ( GEN_CONN_D == 1 )

    ((GEN_NEIGHB_TYPE *)out)->i[0]
        = ( iu == 0 ) ? 
          ( ((b==NULL)||(b->i_min==((mesh_array_info)c)->offsets[0]))? -1 : -2 )
          : ( index - 1 );

    ((GEN_NEIGHB_TYPE *)out)->i[1]
        = ( iu >= ( count_0 - 1 ) ) ? 
          ( ((b==NULL)||(b->i_max==(((mesh_array_info)c)->offsets[0])+count_0))?
              -1 : -2 )
          : ( index + 1 );

#elif ( GEN_CONN_D == 2 )

    ((GEN_NEIGHB_TYPE *)out)->i[0]
        = ( iu == 0 ) ? 
          ( ((b==NULL)||(b->i_min==((mesh_array_info)c)->offsets[0]))? -1 : -2 )
          : ( index - count_1 );

    ((GEN_NEIGHB_TYPE *)out)->i[2]
        = ( iv == 0 ) ? 
          ( ((b==NULL)||(b->j_min==((mesh_array_info)c)->offsets[1]))? -1 : -2 )
          : ( index - 1 );

    ((GEN_NEIGHB_TYPE *)out)->i[1]
        = ( iu >= ( count_0 - 1 ) ) ? 
          ( ((b==NULL)||(b->i_max==(((mesh_array_info)c)->offsets[0])+count_0))?
              -1 : -2 )
          : ( index + count_1 );

    ((GEN_NEIGHB_TYPE *)out)->i[3]
        = ( iv >= ( count_1 - 1 ) ) ? 
          ( ((b==NULL)||(b->j_max==(((mesh_array_info)c)->offsets[1])+count_1))?
              -1 : -2 )
          : ( index + 1 );

#elif ( GEN_CONN_D == 3 )

    ((GEN_NEIGHB_TYPE *)out)->i[0]
        = ( iu == 0 ) ? 
          ( ((b==NULL)||(b->i_min==((mesh_array_info)c)->offsets[0]))? -1 : -2 )
          : ( index - count_1_count_2 );

    ((GEN_NEIGHB_TYPE *)out)->i[2]
        = ( iv == 0 ) ? 
          ( ((b==NULL)||(b->j_min==((mesh_array_info)c)->offsets[1]))? -1 : -2 )
          : ( index - count_2 );

    ((GEN_NEIGHB_TYPE *)out)->i[4]
        = ( iw == 0 ) ? 
          ( ((b==NULL)||(b->k_min==((mesh_array_info)c)->offsets[2]))? -1 : -2 )
          : ( index - 1 );

    ((GEN_NEIGHB_TYPE *)out)->i[1]
        = ( iu >= ( count_0 - 1 ) ) ? 
          ( ((b==NULL)||(b->i_max==(((mesh_array_info)c)->offsets[0])+count_0))?
              -1 : -2 )
          : ( index + count_1_count_2 );

    ((GEN_NEIGHB_TYPE *)out)->i[3]
        = ( iv >= ( count_1 - 1 ) ) ? 
          ( ((b==NULL)||(b->j_max==(((mesh_array_info)c)->offsets[1])+count_1))?
              -1 : -2 )
          : ( index + count_2 );

    ((GEN_NEIGHB_TYPE *)out)->i[5]
        = ( iw >= ( count_2 - 1 ) ) ? 
          ( ((b==NULL)||(b->k_max==(((mesh_array_info)c)->offsets[2])+count_2))?
              -1 : -2 )
          : ( index + 1 );

#if 0
DXMessage ( "neighbor[%d] = (%d,%d,%d,%d,%d,%d), limits = (%d,%d,%d,%d,%d,%d), offs = (%d,%d,%d), coun = (%d,%d,%d)",
    i,
    ((GEN_NEIGHB_TYPE *)out)->i[0],
    ((GEN_NEIGHB_TYPE *)out)->i[1],
    ((GEN_NEIGHB_TYPE *)out)->i[2],
    ((GEN_NEIGHB_TYPE *)out)->i[3],
    ((GEN_NEIGHB_TYPE *)out)->i[4],
    ((GEN_NEIGHB_TYPE *)out)->i[5],
    b->i_max, b->j_max, b->k_max,
    b->i_max, b->j_max, b->k_max,
    ((mesh_array_info)c)->offsets[0],
    ((mesh_array_info)c)->offsets[1],
    ((mesh_array_info)c)->offsets[2],
    count_0, count_1, count_2 );
#endif

#endif

    return OK;

    /* error: */
        ERROR_SECTION;
        return ERROR;
}
#endif


/*---------------------------------------------*\
 | End generic templates.
\*---------------------------------------------*/


#if ( _GETFIELD_COMPILATION_PASS == 1 )

/*---------------------------------------------*\
 | Begin generic instantiation system:
 |    for each of the base types: float, char
 |       create a copy of those routines that
 |          handle those specific types 
\*---------------------------------------------*/

#undef   _GETFIELD_COMPILATION_PASS
#define  _GETFIELD_COMPILATION_PASS  2

#define  GEN_get_data_irr    _dxf_get_data_irr_FLOAT
#define  GEN_get_data_reg    _dxf_get_data_reg_FLOAT
#define  GEN_DX_TYPE         TYPE_FLOAT
#define  GEN_DATA_TYPE       float

#include __FILE__

#undef   GEN_get_data_irr
#undef   GEN_get_data_reg
#undef   GEN_DX_TYPE
#undef   GEN_DATA_TYPE

#define  GEN_get_data_irr    _dxf_get_data_irr_SHORT
#define  GEN_get_data_reg    _dxf_get_data_reg_SHORT
#define  GEN_DX_TYPE         TYPE_SHORT
#define  GEN_DATA_TYPE       signed short

#include __FILE__

#undef   GEN_get_data_irr
#undef   GEN_get_data_reg
#undef   GEN_DX_TYPE
#undef   GEN_DATA_TYPE


#define  GEN_get_data_irr    _dxf_get_data_irr_INT
#define  GEN_get_data_reg    _dxf_get_data_reg_INT
#define  GEN_DX_TYPE         TYPE_INT
#define  GEN_DATA_TYPE       signed int

#include __FILE__

#undef   GEN_get_data_irr
#undef   GEN_get_data_reg
#undef   GEN_DX_TYPE
#undef   GEN_DATA_TYPE


#define  GEN_get_data_irr    _dxf_get_data_irr_DOUBLE
#define  GEN_get_data_reg    _dxf_get_data_reg_DOUBLE
#define  GEN_DX_TYPE         TYPE_DOUBLE
#define  GEN_DATA_TYPE       double

#include __FILE__

#undef   GEN_get_data_irr
#undef   GEN_get_data_reg
#undef   GEN_DX_TYPE
#undef   GEN_DATA_TYPE


#define  GEN_get_data_irr    _dxf_get_data_irr_BYTE
#define  GEN_get_data_reg    _dxf_get_data_reg_BYTE
#define  GEN_DX_TYPE         TYPE_BYTE
#define  GEN_DATA_TYPE       signed char

#include __FILE__

#undef   GEN_get_data_irr
#undef   GEN_get_data_reg
#undef   GEN_DX_TYPE
#undef   GEN_DATA_TYPE


#define  GEN_get_data_irr    _dxf_get_data_irr_UBYTE
#define  GEN_get_data_reg    _dxf_get_data_reg_UBYTE
#define  GEN_DX_TYPE         TYPE_UBYTE
#define  GEN_DATA_TYPE       unsigned char

#include __FILE__

#undef   GEN_get_data_irr
#undef   GEN_get_data_reg
#undef   GEN_DX_TYPE
#undef   GEN_DATA_TYPE


#define  GEN_get_data_irr    _dxf_get_data_irr_USHORT
#define  GEN_get_data_reg    _dxf_get_data_reg_USHORT
#define  GEN_DX_TYPE         TYPE_USHORT
#define  GEN_DATA_TYPE       unsigned short

#include __FILE__

#undef   GEN_get_data_irr
#undef   GEN_get_data_reg
#undef   GEN_DX_TYPE
#undef   GEN_DATA_TYPE


#define  GEN_get_data_irr    _dxf_get_data_irr_UINT
#define  GEN_get_data_reg    _dxf_get_data_reg_UINT
#define  GEN_DX_TYPE         TYPE_UINT
#define  GEN_DATA_TYPE       unsigned int

#include __FILE__

#undef   GEN_get_data_irr
#undef   GEN_get_data_reg
#undef   GEN_DX_TYPE
#undef   GEN_DATA_TYPE


#define  GEN_get_data_irr    _dxf_get_data_irr_HYPER
#define  GEN_get_data_reg    _dxf_get_data_reg_HYPER
#define  GEN_DX_TYPE         TYPE_HYPER
#define  GEN_DATA_TYPE       signed long

#include __FILE__

#undef   GEN_get_data_irr
#undef   GEN_get_data_reg
#undef   GEN_DX_TYPE
#undef   GEN_DATA_TYPE


#define  GEN_get_conn_irr    _dxf_get_conn_irr_LINES
#define  GEN_get_conn_grid   _dxf_get_conn_grid_LINES
#define  GEN_get_neighb_irr  _dxf_get_neighb_irr_LINES
#define  GEN_get_neighb_grid _dxf_get_neighb_grid_LINES
#define  GEN_get_point_irr   _dxf_get_point_irr_1D
#define  GEN_get_point_grid  _dxf_get_point_grid_1D
#define  GEN_POINT_D         1
#define  GEN_CONN_D          1
#define  GEN_CONN_SHAPE      2
#define  GEN_NEIGHB_SHAPE    2
#define  GEN_POINT_TYPE      Point1D
#define  GEN_CONN_TYPE       Line
#define  GEN_NEIGHB_TYPE     neighbor2
#define  GEN_DELTA_TYPE      Deltas1D

#include __FILE__

#undef   GEN_get_conn_irr
#undef   GEN_get_conn_grid
#undef   GEN_get_neighb_irr
#undef   GEN_get_neighb_grid
#undef   GEN_get_point_irr
#undef   GEN_get_point_grid
#undef   GEN_POINT_D
#undef   GEN_CONN_D
#undef   GEN_CONN_SHAPE
#undef   GEN_NEIGHB_SHAPE
#undef   GEN_POINT_TYPE
#undef   GEN_CONN_TYPE
#undef   GEN_NEIGHB_TYPE
#undef   GEN_DELTA_TYPE


#define  GEN_get_conn_irr    _dxf_get_conn_irr_QUADS
#define  GEN_get_conn_grid   _dxf_get_conn_grid_QUADS
#define  GEN_get_neighb_irr  _dxf_get_neighb_irr_QUADS
#define  GEN_get_neighb_grid _dxf_get_neighb_grid_QUADS
#define  GEN_get_point_irr   _dxf_get_point_irr_2D 
#define  GEN_get_point_grid  _dxf_get_point_grid_2D 
#define  GEN_POINT_D         2 
#define  GEN_CONN_D          2 
#define  GEN_CONN_SHAPE      4 
#define  GEN_NEIGHB_SHAPE    4 
#define  GEN_POINT_TYPE      Point2D 
#define  GEN_CONN_TYPE       Quadrilateral
#define  GEN_NEIGHB_TYPE     neighbor4
#define  GEN_DELTA_TYPE      Deltas2D

#include __FILE__

#undef   GEN_get_conn_irr
#undef   GEN_get_conn_grid
#undef   GEN_get_neighb_irr
#undef   GEN_get_neighb_grid
#undef   GEN_get_point_irr
#undef   GEN_get_point_grid
#undef   GEN_POINT_D
#undef   GEN_CONN_D
#undef   GEN_CONN_SHAPE
#undef   GEN_NEIGHB_SHAPE
#undef   GEN_POINT_TYPE
#undef   GEN_CONN_TYPE
#undef   GEN_NEIGHB_TYPE
#undef   GEN_DELTA_TYPE


#define  GEN_get_conn_irr    _dxf_get_conn_irr_CUBES
#define  GEN_get_conn_grid   _dxf_get_conn_grid_CUBES
#define  GEN_get_neighb_irr  _dxf_get_neighb_irr_CUBES
#define  GEN_get_neighb_grid _dxf_get_neighb_grid_CUBES
#define  GEN_get_point_irr   _dxf_get_point_irr_3D
#define  GEN_get_point_grid  _dxf_get_point_grid_3D
#define  GEN_POINT_D         3
#define  GEN_CONN_D          3
#define  GEN_CONN_SHAPE      8
#define  GEN_NEIGHB_SHAPE    6
#define  GEN_POINT_TYPE      Point
#define  GEN_CONN_TYPE       Cube
#define  GEN_NEIGHB_TYPE     neighbor6
#define  GEN_DELTA_TYPE      Deltas3D

#include __FILE__

#undef   GEN_get_conn_irr
#undef   GEN_get_conn_grid
#undef   GEN_get_neighb_irr
#undef   GEN_get_neighb_grid
#undef   GEN_get_point_irr
#undef   GEN_get_point_grid
#undef   GEN_POINT_D
#undef   GEN_CONN_D
#undef   GEN_CONN_SHAPE
#undef   GEN_NEIGHB_SHAPE
#undef   GEN_POINT_TYPE
#undef   GEN_CONN_TYPE
#undef   GEN_NEIGHB_TYPE
#undef   GEN_DELTA_TYPE


#define  GEN_get_conn_irr    _dxf_get_conn_irr_TRIS
#define  GEN_get_neighb_irr  _dxf_get_neighb_irr_TRIS
#define  GEN_CONN_SHAPE      3
#define  GEN_NEIGHB_SHAPE    3
#define  GEN_CONN_TYPE       Triangle
#define  GEN_NEIGHB_TYPE     neighbor3

#include __FILE__

#undef   GEN_get_conn_irr
#undef   GEN_get_neighb_irr
#undef   GEN_CONN_SHAPE
#undef   GEN_NEIGHB_SHAPE
#undef   GEN_CONN_TYPE
#undef   GEN_NEIGHB_TYPE


#define  GEN_get_conn_irr    _dxf_get_conn_irr_TETS
#define  GEN_get_neighb_irr  _dxf_get_neighb_irr_TETS
#define  GEN_CONN_SHAPE      4
#define  GEN_NEIGHB_SHAPE    4
#define  GEN_CONN_TYPE       Tetrahedron
#define  GEN_NEIGHB_TYPE     neighbor4

#include __FILE__

#undef   GEN_get_conn_irr
#undef   GEN_get_neighb_irr
#undef   GEN_CONN_SHAPE
#undef   GEN_NEIGHB_SHAPE
#undef   GEN_CONN_TYPE
#undef   GEN_NEIGHB_TYPE


#define  GEN_get_conn_irr    _dxf_get_conn_irr_PRSMS
#define  GEN_get_neighb_irr  _dxf_get_neighb_irr_PRSMS
#define  GEN_CONN_SHAPE      6
#define  GEN_NEIGHB_SHAPE    5
#define  GEN_CONN_TYPE       Prism
#define  GEN_NEIGHB_TYPE     neighbor5

#include __FILE__

#undef   GEN_get_conn_irr
#undef   GEN_get_neighb_irr
#undef   GEN_CONN_SHAPE
#undef   GEN_NEIGHB_SHAPE
#undef   GEN_CONN_TYPE
#undef   GEN_NEIGHB_TYPE

#endif /* ( _GETFIELD_COMPILATION_PASS == 1 ) */
