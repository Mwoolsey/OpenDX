/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dxmods/_helper_jea.c,v 1.8 2003/07/11 05:50:33 davidt Exp $
 */

#include <dxconfig.h>


#include <string.h>
#include <math.h>
#include <dx/dx.h>
#include "_helper_jea.h"
#include <fcntl.h>

#define ERROR_SECTION if ( DXGetError() == ERROR_NONE ) \
                  DXDebug ( "I0D", "No error set at %s:%d", __FILE__,__LINE__)

/* 
* These data Must align with defined 'Component_Type' at all times.
* Never!: access by index not of type 'Component_Type'.
* Always!: access with the Component_Type_pos macro.
*
* XXX - Be prepared for shape arrays for when rank>1.
* XXX - Notion of Class/SubClass: *_CONNECTIONS_COMP "dep" POSITIONS_3D_COMP
*       is overconstrained. Perhaps procedural string matching?
*/
static struct _comp_data_rec_type
{
    Component_Type self;
    char           *name;
    Component_Type dependancy;
    Component_Type reference;
    char           *element_type;
    Type           type;
    Category       cat;
    int            rank;
    int            shape;
    int            reg_dims;
}
#if 0
  comp_data [ Component_Type_size ]
#else
  /* AIX RS6000 on odd release renumbers things */
  comp_data [ ]
#endif
    = {
        { POSITIONS_2D_COMP,
          "positions",
              NULL_COMP, NULL_COMP, NULL,
              TYPE_FLOAT, CATEGORY_REAL, 1, 2, 2 },

        { POSITIONS_3D_COMP,
          "positions",
              NULL_COMP, NULL_COMP, NULL,
              TYPE_FLOAT, CATEGORY_REAL, 1, 3, 3 },
        /*
         * Rank 1 shape 1 is more a vector, length 1 than a scalar,
         * which is rank 0.  Data is scalar and therefore of rank 0.
         */
        { POINT_DATA_COMP,
          "data",
              POSITIONS_3D_COMP, NULL_COMP, NULL,
              TYPE_FLOAT, CATEGORY_REAL, 0, 0, 1 },

        { POINT_COLORS_COMP,
          "colors",
              POSITIONS_3D_COMP, NULL_COMP, NULL,
              TYPE_FLOAT, CATEGORY_REAL, 1, 3, 3 },

        { POINT_NORMALS_COMP,
          "normals",
              POSITIONS_3D_COMP, NULL_COMP, NULL,
              TYPE_FLOAT, CATEGORY_REAL, 1, 3, 3 },

        { TETRA_NEIGHBORS_COMP,
          "neighbors",
              TETRA_CONNECTIONS_COMP, NULL_COMP, NULL,
              TYPE_INT,   CATEGORY_REAL, 1, 4, 4 },

        /* NEW */

        { CUBE_NEIGHBORS_COMP,
          "neighbors",
              CUBE_CONNECTIONS_COMP, NULL_COMP, NULL,
              TYPE_INT,   CATEGORY_REAL, 1, 6, 6 },

        /* XXX - Note that
                 "connections" "ref" 3D "positions"
                 is overly restrictive.
                 Current plan is to make an unconstrained
                 superclass to use in the ref slot.  */

        { LINE_CONNECTIONS_COMP,
          "connections",
              NULL_COMP, POSITIONS_3D_COMP, "lines",
              TYPE_INT,   CATEGORY_REAL, 1, 2, 1 },

        { QUAD_CONNECTIONS_COMP,
          "connections",
              NULL_COMP, POSITIONS_3D_COMP, "quads",
              TYPE_INT,   CATEGORY_REAL, 1, 4, 2 },

        { TRIANGLE_CONNECTIONS_COMP,
          "connections",
              NULL_COMP, POSITIONS_3D_COMP, "triangles",
              TYPE_INT,   CATEGORY_REAL, 1, 3, 3 },

        { CUBE_CONNECTIONS_COMP,
          "connections",
              NULL_COMP, POSITIONS_3D_COMP, "cubes",
              TYPE_INT,   CATEGORY_REAL, 1, 8, 3 },

        { TETRA_CONNECTIONS_COMP,
          "connections",
              NULL_COMP, POSITIONS_3D_COMP, "tetrahedra",
              TYPE_INT,   CATEGORY_REAL, 1, 4, 4 }
      };

/*------------------------------------------------*
 *                                                *
 * Guide to seeing connectivity data:             *
 *                        vertex number chart     *
 *                                                *
 * for 2D: observer is in front of the page       *
 * for 3D: lower left is closest to the observer. *
 *                                                *
 *------------------------------------------------*
 *                                                *
 * line:                                          *
 *         p --- q              0 --- 1           *
 *                                                *
 *------------------------------------------------*
 *                                                *
 * triangle:                                      *
 *            p                     0             *
 *           / \                   / \            *
 *          /   \                 /   \           *
 *         q --- r               1 --- 2          *
 *                                                *
 *------------------------------------------------*
 *                                                *
 * quad(rangle):                                  *
 *         p --- q               0 --- 1          *
 *         |     |               |     |          *
 *         |     |               |     |          *
 *         r --- s               2 --- 3          *
 *                                                *
 *------------------------------------------------*
 *                                                *
 * tetrahedron:                                   *
 *             p -s                  0 -3         *
 *            / \ |                 / \ |         *
 *           /   \|                /   \|         *
 *          q --- r               1 --- 2         *
 *                                                *
 *------------------------------------------------*
 *                                                *
 * cube:                                          *
 *           p --- q                 0 --- 1      *
 *          /|    /|                /|    /|      *
 *         r --- s |               2 --- 3 |      *
 *         | |   | |               | |   | |      *
 *         | t --| u               | 4 --| 5      *
 *         |/    |/                |/    |/       *
 *         v --- w                 6 --- 7        *
 *                                                *
 *------------------------------------------------*/

/* Decomposition of Connective Elements:
 *
 *   (Names below are Types as from <basic.h>)
 *
 *     Line          -> Two   Points
 *     Quadrilateral -> Four  Lines
 *     Triangle      -> Three Lines
 *     Cube          -> Six   Quadrilaterals
 *     Tetrahedron   -> Four  Triangles
 */
struct _connect_data_rec_type
{
    Component_Type self;
    int            dimensionality;
    Component_Type degenerates_to;
    int            ndegens;
    int            degensiz;
    int            connect[6][4];
};

#if 0
static struct _connect_data_rec_type 
 connect_data [ Connective_Component_Type_size ]
   =
     { { LINE_CONNECTIONS_COMP, 1, POSITIONS_3D_COMP, 2, 1,
             { {  0, -1, -1, -1 },
               {  1, -1, -1, -1 },
               { -1, -1, -1, -1 },
               { -1, -1, -1, -1 },
               { -1, -1, -1, -1 },
               { -1, -1, -1, -1 } } },
       { QUAD_CONNECTIONS_COMP, 2, LINE_CONNECTIONS_COMP, 4, 2,
             { {  0,  1, -1, -1 },
               {  1,  3, -1, -1 },
               {  3,  2, -1, -1 },
               {  2,  0, -1, -1 },
               { -1, -1, -1, -1 },
               { -1, -1, -1, -1 } } },
       { TRIANGLE_CONNECTIONS_COMP, 2, LINE_CONNECTIONS_COMP, 3, 2,
             { {  0,  1, -1, -1 },
               {  1,  2, -1, -1 },
               {  2,  0, -1, -1 },
               { -1, -1, -1, -1 },
               { -1, -1, -1, -1 },
               { -1, -1, -1, -1 } } },
       { CUBE_CONNECTIONS_COMP, 3, QUAD_CONNECTIONS_COMP, 6, 4,
             /* Changed */
             { {  1,  0,  3,  2 },
               {  4,  5,  6,  7 },
               {  0,  1,  4,  5 },
               {  2,  6,  3,  7 },
               {  0,  4,  2,  6 },
               {  1,  3,  5,  7 } } },
       { TETRA_CONNECTIONS_COMP, 3, TRIANGLE_CONNECTIONS_COMP, 4, 3,
             { {  1,  3,  2, -1 },
               {  0,  2,  3, -1 },
               {  0,  3,  1, -1 },
               {  0,  1,  2, -1 },
               { -1, -1, -1, -1 },
               { -1, -1, -1, -1 } } } };
#endif

#if 0
static Error _inspect ( )
{
    DXMessage
        ( "Component_Type: first %d, last %d, size %d",
          Component_Type_first,
          Component_Type_last,
          Component_Type_size );

    DXMessage
        ( "Component_Type: pos(first) %d, pos(last) %d",
          Component_Type_pos ( Component_Type_first ),
          Component_Type_pos ( Component_Type_last  ) );

    DXMessage
        ( "Component_Type: sizeof rec %d, sizeof array %d",
          sizeof ( comp_data ),
          sizeof ( struct _comp_data_rec_type ) );

    DXMessage
        ( "Connective_Component_Type: first %d, last %d, size %d",
          Connective_Component_Type_first,
          Connective_Component_Type_last,
          Connective_Component_Type_size );

    DXMessage
        ( "Connective_Component_Type: pos(first) %d, pos(last) %d",
          Connective_Component_Type_pos ( Connective_Component_Type_first ),
          Connective_Component_Type_pos ( Connective_Component_Type_last  ) );

    DXMessage
        ( "Connective_Component_Type: sizeof rec %d, sizeof array %d",
          sizeof ( connect_data ),
          sizeof ( struct _connect_data_rec_type ) );

    return OK;
}
#endif


/*----------------------------- Begin Types section -------------------------*/

char * _dxf_ClassName ( Class class )
{
   switch ( class )
   {
        case CLASS_DELETED:               return "CLASS_DELETED";
        case CLASS_MIN:                   return "CLASS_MIN";
        case CLASS_OBJECT:                return "CLASS_OBJECT";
        case CLASS_PRIVATE:               return "CLASS_PRIVATE";
        case CLASS_STRING:                return "CLASS_STRING";
        case CLASS_FIELD:                 return "CLASS_FIELD";
        case CLASS_GROUP:                 return "CLASS_GROUP";
        case CLASS_SERIES:                return "CLASS_SERIES";
        case CLASS_COMPOSITEFIELD:        return "CLASS_COMPOSITEFIELD";
        case CLASS_ARRAY:                 return "CLASS_ARRAY";
        case CLASS_REGULARARRAY:          return "CLASS_REGULARARRAY";
        case CLASS_PATHARRAY:             return "CLASS_PATHARRAY";
        case CLASS_PRODUCTARRAY:          return "CLASS_PRODUCTARRAY";
        case CLASS_MESHARRAY:             return "CLASS_MESHARRAY";
        case CLASS_INTERPOLATOR:          return "CLASS_INTERPOLATOR";
        case CLASS_FIELDINTERPOLATOR:     return "CLASS_FIELDINTERPOLATOR";
        case CLASS_GROUPINTERPOLATOR:     return "CLASS_GROUPINTERPOLATOR";
        case CLASS_LINESRR1DINTERPOLATOR: return "CLASS_LINESRR1DINTERPOLATOR";
        case CLASS_LINESRI1DINTERPOLATOR: return "CLASS_LINESRI1DINTERPOLATOR";
        case CLASS_QUADSRR2DINTERPOLATOR: return "CLASS_QUADSRR2DINTERPOLATOR";
        case CLASS_QUADSII2DINTERPOLATOR: return "CLASS_QUADSII2DINTERPOLATOR";
        case CLASS_TRISRI2DINTERPOLATOR:  return "CLASS_TRISRI2DINTERPOLATOR";
        case CLASS_CUBESRRINTERPOLATOR:   return "CLASS_CUBESRRINTERPOLATOR";
        case CLASS_CUBESIIINTERPOLATOR:   return "CLASS_CUBESIIINTERPOLATOR";
        case CLASS_TETRASINTERPOLATOR:    return "CLASS_TETRASINTERPOLATOR";
        case CLASS_GROUPITERATOR:         return "CLASS_GROUPITERATOR";
        case CLASS_ITEMITERATOR:          return "CLASS_ITEMITERATOR";
        case CLASS_XFORM:                 return "CLASS_XFORM";
        case CLASS_SCREEN:                return "CLASS_SCREEN";
        case CLASS_CLIPPED:               return "CLASS_CLIPPED";
        case CLASS_CAMERA:                return "CLASS_CAMERA";
        case CLASS_LIGHT:                 return "CLASS_LIGHT";
        case CLASS_MAX:                   return "CLASS_MAX";
        default:                          return "*** Unknown Class ***";
   }
}


/*------------------------------ End Types section --------------------------*/


/*------------------------ Begin Component Access section -------------------*/

/* Extern routine.  Consult the file header */
Array _dxf_NewComponentArray ( Component_Type comp_type,
                          int            *count,
                          Pointer        origin,
                          Pointer        delta )
{
    Array array;
    int   position = Component_Type_pos ( comp_type );

    DXASSERT ( comp_type != NULL_COMP );
    DXASSERT ( ( position >= 0 ) && ( position < Component_Type_size ) );

    if ( count && origin ) /* Regularity was requested */
        switch ( comp_type )
        {
            /*
             * Note that Category and rank are missing from DXNewRegularArray.
             * Assumptions are CATEGORY_REAL and RANK=1.
             * Special handling:
             *     Data becomes shape 1 to adapt to this restriction.
             *     (Scalar as rank 0 becomes vector length 1)
             */
            case POINT_DATA_COMP:
                DXASSERTGOTO ( delta != NULL );
                array = (Array) DXNewRegularArray ( comp_data [ position ].type,
                                                  1,
                                                  *count, origin, delta );
                break;
            case POINT_COLORS_COMP:
                DXASSERTGOTO ( delta != NULL );
                array = (Array) DXNewRegularArray ( comp_data [ position ].type,
                                                  comp_data [ position ].shape,
                                                  *count, origin, delta );
                break;
            case POSITIONS_2D_COMP:
            case POSITIONS_3D_COMP:
                DXASSERTGOTO ( delta != NULL );
                /* be aware that count, origin, delta contain 
                   a number of elements equalling shape */
                array = DXMakeGridPositionsV ( comp_data [ position ].shape,
                                             count,
                                             (float *)origin,
                                             (float *)delta  );
                break;
            case CUBE_CONNECTIONS_COMP:
            case QUAD_CONNECTIONS_COMP:
                /* be aware that count, origin contain 
                   a number of elements equalling shape */
                array = DXMakeGridConnectionsV
                            ( comp_data [ position ].reg_dims, count );
                array = (Array) DXSetMeshOffsets
                                    ( (MeshArray) array, (int *)origin );
                break;
            default:
                DXSetError
                    ( ERROR_NOT_IMPLEMENTED,
                      "Cannot allocate array as regular (%d)",
                      comp_type );
                return ERROR;
        }
    else
    {
        array = DXNewArray ( comp_data [ position ].type,
                           comp_data [ position ].cat,
                           comp_data [ position ].rank,
                           comp_data [ position ].shape );
        if ( count )
            array = DXAddArrayData ( array, 0, *count, NULL );
    }

    /* XXX - note that the attributes cannot be inserted 
             above the array in the field as
             we do not have the parent field object to set them in. */

    if ( !array )
        goto error;
    else
        return array;

    error:
        ERROR_SECTION;
        return ERROR;
}


/* Extern routine.  Consult the file header */
Error _dxf_GetComponentData ( Object         in_object,
                              Component_Type comp_type,
                              int            *count, /* length */
                              Pointer        origin,
                              Pointer        delta,
                              Pointer       *pointer )
{
    Array     array;
    Object    attribute;
    Pointer   ptr     = NULL;

    int       rank;
    int       position = Component_Type_pos ( comp_type );

    /*  NULL pointer -> regular; else irregular */
    if ( pointer ) *pointer = NULL;

    DXASSERT ( comp_type != NULL_COMP );
    DXASSERT ( ( position >= 0 ) && ( position < Component_Type_size ) );
    DXASSERT ( count != NULL );

    if ( !in_object ) DXErrorReturn ( ERROR_MISSING_DATA, "Null Object" );

    *count = 0;

    if ( ( array = (Array) DXGetComponentValue
                               ( (Field)in_object,
                                 comp_data [ position ].name ) ) == NULL )
        DXErrorGoto2
            ( ERROR_MISSING_DATA,
              "\"%s\" component absent.", comp_data [ position ].name )

    if ( comp_data [ position ].element_type != NULL )
    {
        if ( ( attribute = DXGetComponentAttribute
                             ( (Field)in_object,
                               comp_data [ position ].name,
                               "element type"                ) ) == NULL )
            ErrorGotoPlus1
                ( ERROR_DATA_INVALID,
                  "\"%s\" component is missing the \"element type\" attribute",
                  comp_data [ position ].name )

        else if ( DXGetObjectClass ( attribute ) != CLASS_STRING )

            ErrorGotoPlus1 
                ( ERROR_DATA_INVALID,
                  "\"%s\" component \"element type\" attribute is not a string",
                  comp_data [ position ].name )

        else if ( strcmp
                      ( DXGetString
                            ( (String)attribute ), 
                        comp_data [ position ].element_type ) != 0 )
            ErrorGotoPlus3
                ( ERROR_DATA_INVALID,
                  "the \"%s\" component \"element type\" is \"%s\", not \"%s\"",
                  comp_data [ position ].name,
                  DXGetString ( (String)attribute ),
                  comp_data[ position ].element_type )
    }

    /*
     * Elaborate check just to say: is the request scalar?
     */
    if ( ( comp_data[ position ].rank == 0 )
         || 
         ( ( comp_data[ position ].rank == 1 )
           &&
           ( comp_data[ position ].shape == 1 ) ) )
    {
        if ( !DXTypeCheck ( array,
                          comp_data [ position ].type,
                          comp_data [ position ].cat,
                          0 ) )
        {
            DXResetError();
            if ( !DXTypeCheck ( array,
                              comp_data [ position ].type,
                              comp_data [ position ].cat,
                              1, 1 ) )
                ErrorGotoPlus1
                    ( ERROR_DATA_INVALID,
                      "\"%s\" component failed scalar type check",
                      comp_data [ position ].name )
        }
    }
    else
        if ( !DXTypeCheck ( array,
                          comp_data [ position ].type,
                          comp_data [ position ].cat,
                          comp_data [ position ].rank,
                          comp_data [ position ].shape ) )
        ErrorGotoPlus1
            ( ERROR_DATA_INVALID,
              "\"%s\" component failed type check",
              comp_data [ position ].name )

    if ( !DXGetArrayInfo ( array, count, NULL, NULL, NULL, NULL ) )
        goto error;

    if ( origin ) /* Regularity was requested */
        switch ( comp_type )
        {
            /* Recall that ranks and shapes were already TypeChecked */
            case POINT_DATA_COMP:
            case POINT_COLORS_COMP:
                DXASSERTGOTO ( delta != NULL );
                if ( ( DXGetArrayClass ( array ) == CLASS_REGULARARRAY ) 
                     &&
                     DXGetRegularArrayInfo
                         ( (RegularArray)array, count, origin, delta ) ) {
                    if ( pointer ) *pointer = NULL;
                    return OK;
                }
                break;
            case POSITIONS_2D_COMP:
            case POSITIONS_3D_COMP:
                DXASSERTGOTO ( delta != NULL );
                if ( DXQueryGridPositions ( array,
                                          &rank, count,
                                          (float *)origin, (float *)delta ) ) {
                    if ( pointer ) *pointer = NULL;
                    return OK;
                }
                break;
            case QUAD_CONNECTIONS_COMP:
            case CUBE_CONNECTIONS_COMP:
                if ( ( DXGetArrayClass ( array ) == CLASS_MESHARRAY ) 
                     &&
                     DXQueryGridConnections ( array, &rank, count )
                     &&
                     DXGetMeshOffsets ( (MeshArray) array,
                                      (int *) origin ) ) {
                    if ( pointer ) *pointer = NULL;
                    return OK;
                }
                break;
            default:
                break;
                /* Expand in any case */
        }

#if 0

    else /* Regularity was NOT requested */

#define WARN_EXPAND \
        DXDebug ( "D", "About to expand regular data using DXGetArrayData" )
        switch ( comp_type )
        {
            case POINT_DATA_COMP:
            case POINT_COLORS_COMP:
                if ( ( DXGetArrayClass ( array ) == CLASS_REGULARARRAY ) 
                     &&
                     DXGetRegularArrayInfo
                         ( (RegularArray)array, NULL, NULL, NULL ) )
                    WARN_EXPAND;
                break;
            case POSITIONS_2D_COMP:
            case POSITIONS_3D_COMP:
                if ( ( DXGetArrayClass ( array ) == CLASS_PRODUCTARRAY ) 
                     &&
                     ( DXGetProductArrayInfo ( (ProductArray)array, NULL, NULL )
                       ||
                       DXQueryGridPositions ( array, NULL, NULL, NULL, NULL ) ) )
                    WARN_EXPAND;
                break;
            case QUAD_CONNECTIONS_COMP:
            case CUBE_CONNECTIONS_COMP:
                if ( ( DXGetArrayClass ( array ) == CLASS_MESHARRAY ) 
                     &&
                     ( DXGetMeshArrayInfo ( (MeshArray)array, NULL, NULL )
                       ||
                       DXQueryGridConnections ( array, NULL, NULL ) ) )
                    WARN_EXPAND;
                break;
            default:
                break;
        }
#endif

    if ( ERROR == ( ptr = DXGetArrayData ( array ) ) ) {
        if ( DXGetError() != ERROR_NONE ) 
            goto error;
        else
            ErrorGotoPlus1
                ( ERROR_MISSING_DATA,
                  "\"%s\" component is null", comp_data [ position ].name )
    }

    if ( pointer ) *pointer = ptr;
    return OK;

    error:
        ERROR_SECTION;
        return ERROR;
}

/*------------------------- End Component Access section --------------------*/


/*----------------------------- Begin Array section -------------------------*/


Array _dxf_CopyArray_jea ( Array in, enum _dxd_copy copy )
{
    Array    out = NULL;
    Type     type;
    Category category;
    int      rank;
    int      *shape = NULL;

    DXASSERTGOTO ( in   != NULL );
    DXASSERTGOTO ( copy == COPY_HEADER );

   /*
    * De Facto rank limits:
    *   Nancy: Import: 15 -> 50;  Donna: Construct: 32;  Bruce: libdx: 100
    */

    if ( !DXGetArrayInfo ( in, NULL, NULL, NULL, &rank, NULL ) )
        goto error;

    if ( rank == 0 )
    {
        if ( !DXGetArrayInfo ( in, NULL, &type, &category, &rank, NULL ) )
            goto error;
    }
    else
    {
        if ( ( ERROR == ( shape = (int *) DXAllocateLocal
                                              ( rank * sizeof(int) ) ) )
             ||
             !DXGetArrayInfo ( in, NULL, &type, &category, &rank, shape ) )
            goto error;
    }

    if ( ERROR == ( out = DXNewArrayV ( type, category, rank, shape ) ) ) 
        goto error;

    DXFree ( (Pointer) shape );  shape = NULL;

    return out;

    error:
        ERROR_SECTION;

        DXDelete ( (Object)  out   );  out   = NULL;
        DXFree   ( (Pointer) shape );  shape = NULL;

        return ERROR;
}

/*------------------------------ End Array section --------------------------*/

/*----------------------------- Begin Group section -------------------------*/


static
/* The initial value of position is 0 */
/* XXX Move up */
Object _flatten_hierarchy ( Object in, CompositeField out, int *position )
{
    int    i;
    Object member;

    DXASSERTGOTO ( in && out && position );

    switch ( DXGetObjectClass ( in ) )
    {
        case CLASS_FIELD:
            if ( !DXSetEnumeratedMember ( (Group)out, *position, in ) )
                goto error;

            (*position)++;
            break;

        case CLASS_GROUP:

            if ( DXGetGroupClass ( (Group) in ) != CLASS_COMPOSITEFIELD )
                DXErrorGoto
                    ( ERROR_DATA_INVALID, "Illegal member in Composite Field" );

            for ( i      = 0;
                  (member=DXGetEnumeratedMember ( (Group)in, i, NULL ));
                  i++ )
                if ( !_flatten_hierarchy ( member, out, position ) )
                    goto error;
            break;

        default:
            DXErrorGoto ( ERROR_DATA_INVALID, 
                        "Illegal member in Composite Field" );
    }

    return (Object) out;

    error:
        ERROR_SECTION;
        return ERROR;
}


Object _dxf_FlattenHierarchy ( Object in )
{
    CompositeField  out = NULL;

    int  position = 0;


    if ( DXGetObjectClass ( in ) == CLASS_FIELD )
        return in;
    else
    {
        if ( ( NULL == ( out = DXNewCompositeField () ) )
             ||
             !_flatten_hierarchy ( in, out, &position ) )
            goto error;

        DXDelete ( (Object) in ); in = NULL;

        return (Object) out;
    }

    error:
        ERROR_SECTION;

        DXDelete ( (Object) out );
        DXDelete ( (Object) in  );

        return ERROR;
}


#if 0
  /* XXX - Next two are unused */

/* 
 * What we have to work with:
 *
 * Object DXGetMember           (Group g, char *name);
 * Object DXGetEnumeratedMember (Group g, int n, char **name);
 * Object DXGetSeriesMember     (Series s, int n, float *position);
 *
 * Series DXSetSeriesMember     (Series s, int n, double position, Object o);
 * Group  DXSetMember           (Group g, char *name, Object value);
 * Group  DXSetEnumeratedMember (Group g, int n, Object value);
 * 
 *  member attribute             when a Group    when a Series
 *  ----------------             ------------    -------------
 *    member name                    Use           Ignore
 *    position number                Ignore        Use
 *    position floating point value  Ignore        Use
 */

Group _dxf_SetMember_G_S ( Group g, int n, char *name, float position, Object o )
{
    DXASSERT ( DXGetObjectClass ( (Object)g ) == CLASS_GROUP );
    DXASSERT ( o );

    if ( DXGetGroupClass(g) == CLASS_SERIES )
    {
        if ( name && DXQueryDebug ( "P" ) )
            DXWarning ( "Series group member name (%s) ignored.", name );

        return (Group) DXSetSeriesMember ( (Series)g, n, position, o );
    }
    else
        return DXSetMember ( g, name, o );
}


Object _dxf_GetEnumeratedMember_G_S ( Group g, int n, char **name, float *position )
{
    DXASSERT ( DXGetObjectClass((Object)g)==CLASS_GROUP );
    DXASSERT ( name );
    DXASSERT ( position );


    if ( DXGetGroupClass(g) == CLASS_SERIES )
    {
        *name = NULL;  /* should not be used downstream */

        return DXGetSeriesMember ( (Series)g, n, position );
    }
    else
    {
        *position = 0.0;  /* should not be used downstream */

        return DXGetEnumeratedMember ( g, n, name );
    }
}
#endif

/*--------------------------- End Group Access section ----------------------*/


/*----------------------------- Begin Memory section ------------------------*/


/*------------------------------ End Memory section -------------------------*/


/*----------------------------- Begin Generic section -----------------------*/

/* Extern routine.  Consult the file header */
int _dxf_greater_prime ( int in )
{
    int      i, j, k;
    int  prime;

    i     = in;
    prime = FALSE;

    while ( !prime ) {
        i++;
        prime = TRUE;
        k     = sqrt ( (float) i ) + 1;

        for ( j=2; j<=k; j++ )
            if ( ( i % j ) == 0 ) {
                prime = FALSE;
                break;
            }
    }
    return ( i );
}

/*------------------------------ End Generic section ------------------------*/

/*---------------------- Begin filename construction section ----------------*/


/*-------------------*/


/*----------------------- Begin Field manipulation section ------------------*/

Field _dxf_MakeFieldEmpty ( Field field )
{
    char *name;
    int  protect = 0;

    DXASSERT ( field != NULL );
    DXASSERT ( DXGetObjectClass ( (Object) field ) == CLASS_FIELD );

    while ( DXGetEnumeratedComponentValue ( field, 0, &name ) )
    {
        if ( protect++ > 512 )
            DXErrorGoto
                ( ERROR_UNEXPECTED,
                  "unusually large number of components, _dxf_MakeFieldEmpty" );

        if ( !DXDeleteComponent ( field, name )
             ||
             ( DXGetError() != ERROR_NONE ) )
                goto error;
    }

    if ( DXGetError() != ERROR_NONE ) goto error;

    return field;

    error:
        ERROR_SECTION;
        return ERROR;
}


/*
 * for 'infield', calculate and set a "fuzz" component.
 * 'type' values progress as follows:
 *      0 for surfaces and volumes
 *      1 for line connections
 *      2 for positions
 */
/* Extern routine.  Consult the file header */
Field _dxf_SetFuzzAttribute ( Field infield, int type )
{
    Array  positions;
    int    ndim;
    int    npoints;
    double fuzz;

    if ( !DXEmptyField ( infield )
         && DXExists ( (Object) infield, "positions" )
         && ( positions = (Array) DXGetComponentValue ( infield, "positions" ) )
         && DXGetArrayInfo ( positions, &npoints, NULL, NULL, NULL, &ndim )
         && ( npoints > 0 ) )
    {

#if 0

        if ( ( ndim != 2 ) && ( ndim != 3 ) )
        {
            DXSetError
                ( ERROR_NOT_IMPLEMENTED,
                  "\"positions\" dimensionality %d not in (2..3)",
                  ndim );
            goto error;
        }

        if ( !DXBoundingBox ( (Object) infield, bbox ) )
            goto error;

        for ( i = 1, min = bbox[0], max = bbox[0]; i < 8; i++ )
        {
            if ( bbox[i].x < min.x ) min.x = bbox[i].x;
            if ( bbox[i].y < min.y ) min.y = bbox[i].y;
            if ( bbox[i].z < min.z ) min.z = bbox[i].z;
            if ( bbox[i].x > max.x ) max.x = bbox[i].x;
            if ( bbox[i].y > max.y ) max.y = bbox[i].y;
            if ( bbox[i].z > max.z ) max.z = bbox[i].z;
        }
        /*
         * Temporarily use vector 'min' to hold 'max' - 'min'
         */
        min.x = ( max.x - min.x );
        min.y = ( max.y - min.y );
        min.z = ( max.z - min.z );

        fuzz  = sqrt ( ( min.x * min.x ) +
                       ( min.y * min.y ) +
                       ( min.z * min.z ) );

        if ( ( type < 1 ) || ( type > 2 ) )
        {
            DXWarning
                ( "_dxf_SetFuzzAttribute ( type = %d ) mis-specified, using type=2",
                  type );
            type = 2;
        }

        /*
         * Fuzz value had to be chosen by trial and error
         *  image resolution ~= 500; object size ~= 10.0
         */

        fuzz = fuzz / 200.0;
        fuzz = fuzz * ( (double) type );

#else

        fuzz = 2.0 * type;

#endif

        if ( !DXSetFloatAttribute ( (Object) infield, "fuzz", fuzz ) )
            goto error;

        return infield;
    }

    else if ( DXGetError() == ERROR_NONE ) 
        return infield;

    else
        return ERROR;

    error:
        ERROR_SECTION;
        return ERROR;
}


/*
 *
 */
Field
_dxf_SetDefaultColor ( Field input, RGBColor color )
{
    int      np;
    Array    p;
    Array    carray = NULL;
    int      set_default = 1;
    int      i;

    static char *color_names[] = { "colors", "front colors", NULL };

    for ( i=0; color_names[i] != NULL; i++ )
        if ( DXGetComponentValue ( input, color_names[i] ) )
            set_default = 0;

    if ( set_default && !DXEmptyField ( input ) )
    {
        if ( ERROR == ( p = (Array) DXGetComponentValue
                                        ( input, "positions" ) ) )
            DXErrorGoto
                ( ERROR_ASSERTION,
                  "A non-EmptyField has no positions" );

        if ( !DXGetArrayInfo ( p, &np, NULL, NULL, NULL, NULL )
            ||
            ( ERROR == ( carray = (Array) DXNewConstantArray
                                             ( np,
                                               (Pointer) &color,
                                               TYPE_FLOAT, CATEGORY_REAL,
                                               1, 3 ) ) )
            ||
            !DXSetComponentValue ( input, "colors", (Object) carray ) )
            goto error;

        carray = NULL;

        if ( !DXSetComponentAttribute
                  ( input, "colors", "dep",
                    (Object) DXNewString ( "positions" ) ) )
            goto error;
    }

    return input;

    error:
        ERROR_SECTION;

        DXDelete ( (Object) carray );  carray = NULL;

        return ERROR;
}

/*------------------------ End Field manipulation section -------------------*/

/*------------------------------ Begin Image section ------------------------*/

#define   SMALL 0.4
#define  NONZERO(a)  ( ( (a) <  -SMALL ) || ( (a) >  SMALL ) )
#define     ZERO(a)  ( ( (a) >= -SMALL ) && ( (a) <= SMALL ) )

/*-------------------*/


/*-------------------*/

Field
_dxf_GetEnumeratedImage ( Object o, int n )
{
    int   i;
    Field image = NULL;
    Class class;
    float position;

    if ( ( class = DXGetObjectClass ( o ) ) == CLASS_FIELD )
    {
        if ( n == 0 ) 
            return ( (Field) o );
        else
            DXErrorGoto ( ERROR_ASSERTION, "_dxf_GetEnumeratedImage(Field,n!=0)" )
    }
    else if ( ( class == CLASS_GROUP )
              &&
              ( ( class = DXGetGroupClass ( (Group) o ) ) == CLASS_SERIES ) )
    {
        for ( i=0;
              ( image = (Field) DXGetSeriesMember ( (Series) o, i, &position ) );
              i++ )
           if ( ( (int)position ) == n ) {
               if ( _dxf_ValidImageField ( image ) )
                   return image;
               else
                   goto error;
	   }
    }
    else if ( DXGetError() != ERROR_NONE )
        goto error;
    else
        DXErrorGoto ( ERROR_INTERNAL, "Illegal object type" )

    return NULL;

    error:
        ERROR_SECTION;
        return ERROR;
}


/*-------------------*/


SizeData *
_dxf_GetImageAttributes ( Object o, SizeData *sd )
{
    int   i;
    Field image = NULL;
    Class class;
    float position;
    int   w, h;

    sd->height     = VALUE_UNSPECIFIED;
    sd->width      = VALUE_UNSPECIFIED;
    sd->startframe = VALUE_UNSPECIFIED;
    sd->endframe   = VALUE_UNSPECIFIED;

    if ( ( class = DXGetObjectClass ( o ) ) == CLASS_FIELD )
    {
        if ( !DXGetImageSize ( (Field) o, &sd->width, &sd->height ) )
            goto error;

        sd->startframe = 0;
        sd->endframe   = 0;
    }
    else if ( ( class == CLASS_GROUP )
              &&
              ( ( class = DXGetGroupClass ( (Group) o ) ) == CLASS_SERIES ) )

        for ( i=0;
              ( image = (Field) DXGetSeriesMember ( (Series) o, i, &position ) );
              i++ )
        {
            /*
             * XXX - position values are ignored.
             */
            if ( !DXGetImageSize ( image, &w, &h ) ) goto error;

            sd->endframe = i;

            if ( i == 0 )
            {
                sd->height     = h;
                sd->width      = w;
                sd->startframe = 0;
            }
            else if ( ( sd->height != h ) || ( sd->width != w ) )
                DXErrorGoto
                    ( ERROR_DATA_INVALID,
                      "Image Series changes sizes internally" );
        }

    else if ( DXGetError() == ERROR_NONE )
        DXErrorGoto ( ERROR_INTERNAL, "Illegal object type" )
    else
        goto error;

    return sd;

    error:
        ERROR_SECTION;
        return ERROR;
}


/*-------------------*/

/* Extern routine.  Consult the file header */
Error _dxf_ValidImageField ( Field image )
{
    int  h, w;

    if ( !DXGetImageSize ( image, &w, &h ) )
    {
        DXResetError();
        return ERROR;
    }
    else
        return OK;
}


/* Extern routine.  Consult the file header */
Field _dxf_CheckImage ( Field image )
{
    int   width, height;
    int   test_height, test_width;
    int   count[2];
    float Origin[2];
    float Delta[4];

    /* Do We Really know what is assumed?  Find out. */

    if ( !DXGetImageSize ( image, &width, &height ) )
        goto error;

    if ( !_dxf_GetComponentData ( (Object)image,
                                  POSITIONS_2D_COMP,
                                  count, (Pointer)Origin, (Pointer)Delta, 
                                  NULL ) )
        goto error;

    if ( NONZERO (  Delta[0] ) ||    ZERO (  Delta[1] ) ||
            ZERO (  Delta[2] ) || NONZERO (  Delta[3] ) ||
             ( count[0] == 0 ) ||     ( count[1] == 0 )   )
        DXWarning ( "Image has unsupportable size attributes." );

    test_width  = ( Delta[0] * count[0] ) + ( Delta[2] * count[1] );
    test_height = ( Delta[1] * count[0] ) + ( Delta[3] * count[1] );

    if ( ( test_height != height ) || ( test_width != width ) )
        DXWarning ( "DXGetImageSize data does not match DXQueryGridPositions" );

    return image;

    error:
        ERROR_SECTION;
        return ERROR;
}


Error _dxf_CountImages ( Object image, int *count )
{
    int     i;
    Object  member;

    DXASSERTGOTO ( image && count );

    switch ( DXGetObjectClass ( image ) )
    {
        case CLASS_FIELD:
            (*count)++;
            break;

        case CLASS_GROUP:
            if ( DXGetGroupClass ( (Group) image ) == CLASS_COMPOSITEFIELD )
                (*count)++;
            else
            {
                for ( i      = 0;
                      (member=DXGetEnumeratedMember ( (Group)image, i, NULL ));
                      i++ )
                    if ( !_dxf_CountImages ( member, count ) )
                        goto error;
            }
            break;

        default:
            DXErrorGoto ( ERROR_DATA_INVALID, "Image is not a group or field" );
    }

    return OK;

    error:
        ERROR_SECTION;
        return ERROR;
}



static
Error _get_image_deltas ( Object image, float *deltas, int *set )
{

    DXASSERTGOTO ( image && deltas && set );

    switch ( DXGetObjectClass ( image ) )
    {
        case CLASS_FIELD:
          {
            float   Origin[2];
            float   Delta[4];
            int     count[2];
            Pointer ptr;

            if ( !_dxf_GetComponentData ( (Object)image,
                                     POSITIONS_2D_COMP,
                                     count, (Pointer)Origin, (Pointer)Delta,
                                     &ptr ) )
                goto error;

            if ( ptr )
                DXErrorGoto2
                    ( ERROR_DATA_INVALID,
                      "#10612", /* only regular positions for %s supported */
                      "images" )

            /* Direction is a consideration Only.  Not the magnitude. */

            Delta[0] = ( Delta[0] == 0.0 ) ? 0 : ( Delta[0] > 0.0 ) ? 1 : -1;
            Delta[1] = ( Delta[1] == 0.0 ) ? 0 : ( Delta[1] > 0.0 ) ? 1 : -1;
            Delta[2] = ( Delta[2] == 0.0 ) ? 0 : ( Delta[2] > 0.0 ) ? 1 : -1;
            Delta[3] = ( Delta[3] == 0.0 ) ? 0 : ( Delta[3] > 0.0 ) ? 1 : -1;

            if ( *set )    
            {
                if ( 0 != memcmp
                             ( (char *)deltas, (char *)Delta, sizeof(Delta) ) )
                    DXErrorGoto
                        ( ERROR_DATA_INVALID,
                          "images must have like positional deltas" );
            }
            else
            {
                if ( !memcpy ( (char *)deltas, (char *)Delta, sizeof(Delta) ) )
                    DXErrorGoto
                        ( ERROR_INTERNAL, "memcpy()" );

                *set = 1;
            }
          }
            break;

        case CLASS_GROUP:
          {
            int     i;
            Object  member;
#if 0
            if ( DXGetGroupClass ( (Group) in ) != CLASS_COMPOSITEFIELD )
                DXErrorGoto
                    ( ERROR_DATA_INVALID, "Illegal member in Composite Field" );
#endif
            for ( i      = 0;
                  (member=DXGetEnumeratedMember ( (Group)image, i, NULL ));
                  i++ )
                if ( !_get_image_deltas ( member, deltas, set ) )
                    goto error;
          }
            break;

        default:
            DXErrorGoto ( ERROR_DATA_INVALID, 
                        "Illegal member in Composite Field" );
    }

    return OK;

    error:
        ERROR_SECTION;
        return ERROR;
}


Error _dxf_GetImageDeltas ( Object image, float *deltas )
{
    int set = 0;

    DXASSERTGOTO ( image && deltas );

    /* Get AND Verify they match */

    if ( !_get_image_deltas ( image, deltas, &set ) )
        goto error;

    if ( set == 0 )
        DXErrorGoto ( ERROR_INTERNAL, "_dxf_GetImageDeltas.  set == 0" );

    return OK;

    error:
        ERROR_SECTION;
        return ERROR;
}


Error _dxf_CheckImageDeltas ( Object image, int *transposed )
{
    float deltas[4];
    int   dsignature = 0;

    DXASSERTGOTO ( transposed != NULL );

    if ( !_dxf_GetImageDeltas ( image, deltas ) ) 
        goto error;

    /*
     * delta characterization:
     *   for each term:        d[0]  d[1]  d[2]  d[3]
     *   assign bits for +,-:  0 1 | 2 3 | 4 5 | 6 7
     *   with the values:      1             ..  128
     */
    dsignature |= (deltas[0]==0)? 0 : (deltas[0]>0)?   1 :   2;
    dsignature |= (deltas[1]==0)? 0 : (deltas[1]>0)?   4 :   8;
    dsignature |= (deltas[2]==0)? 0 : (deltas[2]>0)?  16 :  32;
    dsignature |= (deltas[3]==0)? 0 : (deltas[3]>0)?  64 : 128;

    if ( dsignature == ( 16 | 4 ) ) /* y:first, x:fastest */
        *transposed = 0;

    else if ( dsignature == ( 1 | 64 ) ) /* x:first, y:fastest */
        *transposed = 1;

    else
        DXErrorGoto
            ( ERROR_DATA_INVALID, "image has unsupported deltas" );

    return OK;

    error:
        ERROR_SECTION;
        return ERROR;
}


#if 0
#endif
/* Extern routine.  Consult the file header */
Error _dxf_GetImageOrigin ( Field image, int *xorigin, int *yorigin )
{
    int     count[2];
    int     offsets[2];
    Pointer ptr;

    if ( !_dxf_GetComponentData ( (Object)image,
                                  QUAD_CONNECTIONS_COMP,
                                  count, (Pointer)offsets, NULL, &ptr ) )
        goto error;
#if 0
    _dxf_GetComponentData ( (Object)image,
                            POSITIONS_2D_COMP,
                            count, (Pointer)Origin, (Pointer)Delta, &ptr );
#endif

    if ( ptr == NULL )
    {
	if ( yorigin ) *yorigin = offsets[0];
	if ( xorigin ) *xorigin = offsets[1];
#if 0
	if ( NULL == ( array = (Array) DXGetComponentValue
					   ( image, "connections" ) ) )
	    ErrorGotoPlus2
		( ERROR_MISSING_DATA, "#10250", "image", "\"connections\"" )

	if ( DXGetMeshOffsets ( (MeshArray) array, count ) )
	{
	    if ( xorigin ) *xorigin = count[1];
	    if ( yorigin ) *yorigin = count[0];
	}
	else
	{
	    if ( DXGetError() != ERROR_NONE ) goto error;

	    if ( xorigin ) *xorigin = 0;
	    if ( yorigin ) *yorigin = 0;
	}
	if ( !CheckImagePosition ( image ) ) goto error;
#endif
    }
    else
	DXErrorGoto2
	    ( ERROR_DATA_INVALID,
	      "#10610", /* only regular connections for %s supported */
	      "images" )
    
    DXDebug ( "A", 
         "_dxf_GetImageOrigin ( [%x], (=%d), (=%d) )", image, *xorigin, *yorigin );

    return OK;

    error:
        ERROR_SECTION;
        return ERROR;
}


/* Extern routine.  Consult the file header */
Field _dxf_SetImageOrigin ( Field image, int xorigin, int yorigin )
{
    int     count[2];
    int     offsets[2];
    float   Origin[2];
    float   Delta[4];
    Pointer ptr;
    Array   array;

    if ( !_dxf_GetComponentData ( (Object)image,
				  POSITIONS_2D_COMP,
				  count, (Pointer)Origin, (Pointer)Delta, 
				  &ptr ) )
        goto error;

    if ( ptr )
        DXErrorGoto2
            ( ERROR_DATA_INVALID,
              "#10612", /* only regular positions for %s supported */
              "images" )

    if ( !_dxf_GetComponentData ( (Object)image,
				  QUAD_CONNECTIONS_COMP,
				  count, (Pointer)offsets, NULL, &ptr ) )
        goto error;

    if ( ptr == NULL )
    {
	if ( Delta[1] /*U.y*/ != 0.0 )
	{
	    /* x varying fastest (y slowest) */

	    offsets[0] = yorigin;
	    offsets[1] = xorigin;

	    Origin[0] = xorigin * Delta[2]; /* V.x */
	    Origin[1] = yorigin * Delta[1]; /* U.y */
	}
	else
	{
	    /* y varying fastest */

	    offsets[0] = xorigin;
	    offsets[1] = yorigin;

	    Origin[0] = xorigin * Delta[0]; /* U.x */
	    Origin[1] = yorigin * Delta[3]; /* V.y */
	}

	DXDebug ( "A", 
		  "_dxf_SetImageOrigin ( [%x], %d, %d )",
		  image, xorigin, yorigin );

	if ( ( NULL == ( array = _dxf_NewComponentArray
				     ( QUAD_CONNECTIONS_COMP,
				       count, (Pointer)offsets, NULL ) ) )
	     ||
	     !DXSetComponentValue ( image, "connections", (Object)array )
	     ||
	     ( NULL == ( array = _dxf_NewComponentArray
				     ( POSITIONS_2D_COMP,
				       count,
				       (Pointer)Origin, (Pointer)Delta ) ) )
	     ||
	     !DXSetComponentValue ( image, "positions", (Object)array ) 
	     ||
	     !DXChangedComponentValues ( image, "positions" ) 
	     ||
	     !DXEndField ( image ) )
	    goto error;
#if 0
	if ( !CheckImagePosition ( image ) ) goto error;
#endif
	return image;
    }
    else
        DXErrorGoto2
            ( ERROR_DATA_INVALID,
              "#10610", /* only regular connections for %s supported */
              "images" )
    error:
        ERROR_SECTION;
        return ERROR;
}


/* Extern routine.  Consult the file header */
CompositeField _dxf_SetCompositeImageOrigin ( CompositeField image,
                                         int xorigin, int yorigin )
{
    Object member;
    int    cfield_origin[2];
    int    member_origin[2];
    int    i;

    if ( !DXGetImageBounds ( (Object)image, &cfield_origin[0], &cfield_origin[1],
                           NULL, NULL ) )
        goto error;

    if ( ( cfield_origin[0] == xorigin )
         &&
         ( cfield_origin[1] == yorigin ) )

        return image; /* We are in luck */

    for ( i = 0;
          (member= DXGetEnumeratedMember ( (Group)image, i, NULL ));
          i++ )

        switch ( DXGetObjectClass ( member ) )
        {
            case CLASS_FIELD:
                if ( !DXGetImageBounds
                          ( (Object)member,
                            &member_origin[0], &member_origin[1], NULL, NULL ) )
                    goto error;

                member_origin[0] = ( member_origin[0] - cfield_origin[0] )
                                   + xorigin;
                member_origin[1] = ( member_origin[1] - cfield_origin[1] )
                                   + yorigin;

                if ( !_dxf_SetImageOrigin
                          ( (Field)member,
                            member_origin[0],
                            member_origin[1] ) )
                    goto error;

                break;

            case CLASS_GROUP:

                switch ( DXGetGroupClass ( (Group)member ) )
                {
                    case CLASS_COMPOSITEFIELD:

                        if ( !DXGetImageBounds
                                  ( (Object)member,
                                    &member_origin[0], &member_origin[1],
                                    NULL, NULL ) )
                            goto error;

                        member_origin[0]
                            = ( member_origin[0] - cfield_origin[0] ) + xorigin;

                        member_origin[1]
                            = ( member_origin[1] - cfield_origin[1] ) + yorigin;

                        if ( !_dxf_SetCompositeImageOrigin
                                  ( (CompositeField)member,
                                    member_origin[0],
                                    member_origin[1] ) )
                            goto error;

                        break;

                    default:
                        DXErrorGoto
                            ( ERROR_DATA_INVALID, 
                              "Illegal member in Arranged Group" );
                }
                break;

            default:
                DXErrorGoto ( ERROR_DATA_INVALID, 
                            "Illegal member in Arranged Group" );
        }

    return image;
    error:
        ERROR_SECTION;
        return ERROR;

}


#if 0
/* Extents are in x and y, and are normalized min_(x,y) = 0 in special cases.
 * For Members in a 'Plain' Group, normalize first, then accumulate.
 * For Members in a Composite Field, do not normalize, just accumulate.
 * This reinforces the rule that Composite Field members are Non-Overlapping.
 */
/* Extern routine.  Consult the file header */
Error _dxf_QueryImageExtents ( Object inp, int *min_x, int *min_y,
                                      int *max_x, int *max_y )
{
    Class  class;
    Object inp_member;
    int    loc_min_x;
    int    loc_min_y;
    int    loc_max_x;
    int    loc_max_y;
    int    origin[2];
    int    size[2];
    int    i;

    switch ( class = DXGetObjectClass ( inp ) )
    {
        case CLASS_GROUP:
            switch ( class = DXGetGroupClass ( (Group)inp ) )
            {

                /* GROUP: extents = sum of normalized extents*/
                case CLASS_SERIES:
                case CLASS_GROUP:

                    *min_x = 0;
                    *min_y = 0;
                    *max_x = 0;
                    *max_y = 0;
                    for ( i=0;
                          inp_member = DXGetEnumeratedMember
                                           ( (Group)inp, i, NULL );
                          i++ )
                        if ( !_dxf_QueryImageExtents
                                  ( inp_member, &loc_min_x, &loc_min_y,
                                                &loc_max_x, &loc_max_y ) )
                            goto error;
                        else
                        {
                            loc_max_x = loc_max_x - loc_min_x;
                            loc_max_y = loc_max_y - loc_min_y;
                            if ( i == 0 )
                            {
                                *max_x = loc_max_x;
                                *max_y = loc_max_y;
                            }
                            else
                            {
                                *max_x = MAX ( *max_x, loc_max_x );
                                *max_y = MAX ( *max_y, loc_max_y );
                            }
                        }
                    return OK;

                /* COMPOSITE FIELD: extents = sum of extents (unnormalized)*/
                case CLASS_COMPOSITEFIELD:

                    /* XXX - Possible to have valid empty composite field */
                    *min_x = 0;
                    *min_y = 0;
                    *max_x = 0;
                    *max_y = 0;

                    for ( i=0;
                          inp_member = DXGetEnumeratedMember
                                           ( (Group)inp, i, NULL );
                          i++ )
                        if ( !_dxf_QueryImageExtents
                                  ( inp_member, &loc_min_x, &loc_min_y,
                                                &loc_max_x, &loc_max_y ) )
                            goto error;
                        else
                            if ( i == 0 )
                            {
                                /* Not empty so override defaults */
                                *min_x = loc_min_x;
                                *min_y = loc_min_y;
                                *max_x = loc_max_x;
                                *max_y = loc_max_y;
                            }
                            else
                            {
                                *min_x = MIN ( *min_x, loc_min_x );
                                *min_y = MIN ( *min_y, loc_min_y );
                                *max_x = MAX ( *max_x, loc_max_x );
                                *max_y = MAX ( *max_y, loc_max_y );
                            }
                    return OK;
                    break;

                default:
                    ErrorGotoPlus1 ( ERROR_DATA_INVALID, "#10190", "image" );
            }
            break;

        case CLASS_FIELD:

            if ( !_dxf_GetImageOrigin ( (Field)inp, &origin[0], &origin[1] )
                 ||
                 !DXGetImageSize ( (Field)inp, &size[0], &size[1] ) )
                goto error;

            *min_x = origin[0];
            *min_y = origin[1];
            *max_x = origin[0] + size[0];
            *max_y = origin[1] + size[1];
            return OK;

        default:
            ErrorGotoPlus1 ( ERROR_DATA_INVALID, "#10190", "image" );

    }
    error:
        ERROR_SECTION;
        return ERROR;
}
#endif

#define CLAMP(s, d) \
{ \
    float t = s; \
    d = (t <= 0.0) ? 0 : (t >= 1.0) ? 255 : 255*t; \
}

#define COPY_FLOAT_INTO_UBYTE \
{ \
    float *src = (float *)srcPtr; \
    ubyte *dst = (ubyte *)dstPtr; \
 \
    for ( y=0, is=0, id=(((sy0*wd)+sx0)*3); y<hs; y++, id+=(wd-ws)*3) \
        for ( x=0; x<3*ws; x++) \
	    CLAMP(src[is++], dst[id++]); \
}

#define COPY_UBYTE_INTO_FLOAT \
{ \
    ubyte *src = (ubyte *)srcPtr; \
    float *dst = (float   *)dstPtr; \
    float d = 1.0 / 255.0; \
 \
    for ( y=0, is=0, id=(((sy0*wd)+sx0)*3); y<hs; y++, id+=(wd-ws)*3) \
        for ( x=0; x<3*ws; x++) \
	    dst[id++] = d*src[is++]; \
}

#define COPY_SAME_INTO_SAME(type) \
{ \
    type *src = (type *)srcPtr; \
    type *dst = (type   *)dstPtr; \
 \
    for ( y=0, is=0, id=(((sy0*wd)+sx0)*3); y<hs; y++, id+=(wd-ws)*3) \
        for ( x=0; x<3*ws; x++) \
	    dst[id++] = src[is++]; \
}

#define COPY_MAPPED_INTO_FLOAT \
{ \
    ubyte *src = (ubyte *)srcPtr; \
    float *dst = (float *)dstPtr; \
 \
    for ( y=0, is=0, id=((sy0*wd)+sx0)*3; y<hs; y++, id+=(wd-ws)*3) \
        for ( x=0; x<ws; x++) \
	{ \
	    float *m = srcMap + 3*src[is++]; \
	    dst[id++] = *m++; \
	    dst[id++] = *m++; \
	    dst[id++] = *m++; \
	} \
}

#define COPY_MAPPED_INTO_UBYTE \
{ \
    ubyte *src = (ubyte *)srcPtr; \
    ubyte *dst = (ubyte *)dstPtr; \
 \
    for ( y=0, is=0, id=((sy0*wd)+sx0)*3; y<hs; y++, id+=(wd-ws)*3) \
        for ( x=0; x<ws; x++) \
	{ \
	    float *m = srcMap + src[is++]; \
	    CLAMP(*m++, dst[id++]); \
	    CLAMP(*m++, dst[id++]); \
	    CLAMP(*m++, dst[id++]); \
	} \
}

static
/* 
 * Place 'source' at location ('xoff','yoff') within 'destination'.
 */
Field PlaceCompositeMember ( Object source,
                             int xoff, int yoff, Field destination )
{
    Object    member;
    int       i, x, y;
    int       sx0, sy0, sx1, sy1,  is,  hs, ws;
    int       dx0, dy0, dx1, dy1,  id,  hd, wd;
    Array     array;
    Pointer   srcPtr, dstPtr;
    Type      src_type, dst_type, map_type;
    int       src_vector;
    int       rank, shape[32];
    float     *srcMap = NULL;

    switch ( DXGetObjectClass ( source ) )
    {
        case CLASS_GROUP:
            switch ( DXGetGroupClass ( (Group)source ) )
            {
                case CLASS_COMPOSITEFIELD:
                    for ( i=0;
                          (member=DXGetEnumeratedMember
                                       ( (Group)source, i, NULL ));
                          i++ )
                        if ( !PlaceCompositeMember
                                  ( member, xoff, yoff, destination ) )
                            goto error;

                    return destination;
                    break;

                default:
                    DXErrorGoto ( ERROR_DATA_INVALID, "Illegal object type" );
            }
            break;

        case CLASS_FIELD:

            if ( !DXGetImageBounds ( source, &sx0, &sy0, &sx1, &sy1 ) )
                goto error;

            sx1 += sx0;
            sy1 += sy0;

            sx0 = sx0 + xoff;
            sy0 = sy0 + yoff;
            sx1 = sx1 + xoff;
            sy1 = sy1 + yoff;

            if ( !DXGetImageBounds
                      ( (Object)destination, &dx0, &dy0, &dx1, &dy1 ) )
                goto error;

            dx1 += dx0;
            dy1 += dy0;

            /* width and height of source and destination */
            ws = sx1 - sx0;
            hs = sy1 - sy0;
            wd = dx1 - dx0;
            hd = dy1 - dy0;

            if ( ( sx0 < 0 ) || ( sx0 > wd ) || ( sx1 < 0 ) || ( sx1 > wd )
                 ||
                 ( sy0 < 0 ) || ( sy0 > hd ) || ( sy1 < 0 ) || ( sy1 > hd )
                 ||
                 ( ws > wd ) || ( hs > hd ) )
                DXErrorGoto ( ERROR_DATA_INVALID, "Image sizes clash" );

            /* XXX - Valid only for X varying fastest */

	    array  = (Array)DXGetComponentValue((Field)source, "colors");
	    srcPtr = (Pointer)DXGetArrayData(array);

	    DXGetArrayInfo(array, NULL, &src_type, NULL, &rank, shape);

	    if (rank == 0 || (rank == 1 && shape[0] == 1))
		src_vector = 0;
	    else if (rank == 1 && shape[0] == 3)
		src_vector = 1;
	    else
		DXErrorGoto(ERROR_DATA_INVALID,
		    "images must be either scalar, 1-vector or 3-vector");

	    if (! src_vector)
	    {
		if (src_type != TYPE_UBYTE)
		{
		    DXErrorGoto(ERROR_DATA_INVALID,
			"Delayed-color image pixels must be ubytes");
		}
			
		array = (Array)DXGetComponentValue((Field)source, "color map");
		if (! array)
		    DXErrorGoto(ERROR_DATA_INVALID, 
			"Delayed-color image has no color map");
		
		DXGetArrayInfo(array, NULL, &map_type, NULL, &rank, shape);

		if (map_type != TYPE_FLOAT || rank != 1 || shape[0] != 3)
		    DXErrorGoto(ERROR_DATA_INVALID,
			"Delayed-color color map must be float 3-vector");
		
		srcMap = (float *)DXGetArrayData(array);
	    }
	    else 
		if (src_type != TYPE_UBYTE && src_type != TYPE_FLOAT)
		    DXErrorGoto(ERROR_DATA_INVALID,
			"color image pixels must be ubytes or floats");
		
	    array  = (Array)DXGetComponentValue(destination, "colors");
	    dstPtr = (Pointer)DXGetArrayData(array);

	    DXGetArrayInfo(array, NULL, &dst_type, NULL, &rank, shape);

	    if (rank != 1 || shape[0] != 3)
		DXErrorGoto(ERROR_DATA_INVALID,
		    "destination image must be 3-vector");
	    
	    if (src_type != TYPE_UBYTE && src_type != TYPE_FLOAT)
		DXErrorGoto(ERROR_DATA_INVALID,
		    "destination image pixels must be ubytes or floats");
	    
	    if (src_vector)
	    {
		if (src_type == TYPE_UBYTE && dst_type == TYPE_UBYTE)
		    COPY_SAME_INTO_SAME(ubyte)
		else if (src_type == TYPE_UBYTE && dst_type == TYPE_FLOAT)
		    COPY_UBYTE_INTO_FLOAT
		else if (src_type == TYPE_FLOAT && dst_type == TYPE_UBYTE)
		    COPY_FLOAT_INTO_UBYTE
		else
		    COPY_SAME_INTO_SAME(float);
	    }
	    else
	    {
		if (dst_type == TYPE_FLOAT)
		    COPY_MAPPED_INTO_FLOAT
		else
		    COPY_MAPPED_INTO_UBYTE;
	    }

            return destination;

            break;

        default:
            DXErrorGoto ( ERROR_DATA_INVALID, "Illegal object type" );
    }

    error:
        ERROR_SECTION;
        return ERROR;
}

/* Extern routine.  Consult the file header */
Field _dxf_SimplifyCompositeImage ( CompositeField image )
{
    int       min_x;
    int       min_y;
    int       max_x;
    int       max_y;
    Field     fld_image = NULL;
    Pointer   *pixels;
    Array     array;
    
    if ( !DXGetImageBounds ( (Object) image, &min_x, &min_y, &max_x, &max_y ) )
        goto error;

    max_x += min_x;  /* look below. */
    max_y += min_y;

    max_x = max_x - min_x;
    max_y = max_y - min_y;

    fld_image = DXMakeImageFormat(max_x, max_y, "BYTE");
    if (! fld_image)
        goto error;
    
    array = (Array)DXGetComponentValue(fld_image, "colors");
    if (! array)
	goto error;
    
    pixels = DXGetArrayData(array);

    memset((char *)pixels, 0, (max_x * max_y * DXGetItemSize(array)));

    if ( !PlaceCompositeMember((Object)image, -min_x, -min_y, fld_image))
        goto error;

    return fld_image;

    error:

        ERROR_SECTION;

        DXDelete ( (Object)fld_image);

        return ERROR;
}

/*------------------------------- End Image section -------------------------*/
