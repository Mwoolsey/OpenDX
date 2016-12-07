/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/* XXX - equivalence loop, prisms, efficient degenerate removal */
/* XXX - the diagonal isn't preserved with band quad->tri decomp */
/* XXX - color map component */

#include <dxconfig.h>

#if defined(HAVE_STRING_H)
#include <string.h>
#endif

/* XXX - traverse once to get values and once to set them (attributes). merge */

#ifndef _ISOSURFACE_PASS_NUMBER


/*
 * The following commands may be useful in rebuilding isosurface in
 *     the form it had before the large change (rewrite):
 *
 *     rlog -d'<93/02/06' ${file} | grep '^revis' | head -1
 *     co   -d'93/02/06'  ${file}
 *
 * for the files:
 *     isosurface.m _isosurface.h _isosurface.c _contour.c _irriso.c _regiso.c
 */

/***************************************************\
 *   Generic instantiation system:                 *
 *       for each of the base types: float, char   *
 *           create a copy of those routines that  *
 *             handle those specific types         *
\***************************************************/

extern int _dxd_isosurface_task_counter;

#define  _ISOSURFACE_PASS_NUMBER  1  /*------------------------------*/

#define ERROR_SECTION if ( DXGetError() == ERROR_NONE ) \
                  DXDebug ( "I0D", "No error set at %s:%d", __FILE__,__LINE__)

#define  VALUE_COUNT(a)   (  (a)->number )
#define  STRUCT_COUNT(a)  ( ((a)->module==BAND) ? 1      : (a)->number )
#define  MOD_NAME(a)      ( ((a)->module==BAND) ? "Band" : "Isosurface")

#define  ATTR_NAME(a) ( ((a)->module==BAND) ? "Band value" : "Isosurface value")


#endif /* ( not defined _ISOSURFACE_PASS_NUMBER ) */


#if ( _ISOSURFACE_PASS_NUMBER == 1 )


/*
** -------- Include Files -------------------------------------------------
** -------- Type Information ----------------------------------------------
** -------- Constants -----------------------------------------------------
** -------- storage areas -------------------------------------------------
** -------- Inline Macros -------------------------------------------------
** -------- Function Prototypes -------------------------------------------
*/


#include <math.h> 
#include <dx/dx.h> 
#include "_isosurface.h"
#include "_getfield.h"
#include "_helper_jea.h"

#ifdef DXD_LONG_HASHKEY
#endif
#define TST_HASH_INTS if(sizeof(PointId)!=sizeof(int32))\
                      DXErrorGoto(ERROR_UNEXPECTED,"64 bit porting error")

/* the following function resides in the file: showboundary.m */
Array _dxf_resample_array
          ( component_info comp,  int dep_ref,
            int Nin,   int Nout,
            int *push, int *pull );


static RGBColor DEFAULT_SURFACE_COLOR = { 0.5, 0.7, 1.0 }; /*bluish slate grey*/
static RGBColor DEFAULT_LINE_COLOR    = { 0.7, 0.7, 0.0 };

                        /* sizeof unsigned int is 4 bytes, 32 bits */
#define  NEWBIT(n)      ((Pointer)DXAllocateLocalZero((((n)+31)/32)*4))
#define  SETBIT(a,i,v)  (((unsigned int*)(a))[((i)/32)]|=((v)<<((i)&31)))
#define  GETBIT(a,i)    (1&(((unsigned int*)(a))[((i)/32)]>>((i)&31)))

#define  NOWHITEBOX_TEST
#define  INCLUDES_BAND


#define ABS(x) (((x)<0.0)?-(x):(x))

typedef struct
{
    Line  edge;
    int   seq;
#if 0
    float t; /* XXX - intrusive in the contour connection table ? */
#endif
}
iso_edge_hash_rec;

typedef iso_edge_hash_rec *iso_edge_hash_ptr;


#ifdef INCLUDES_BAND

typedef enum { DEP_CONNECTIONS, DEP_POSITIONS } data_dep;

#define  NEITHER_VALUE   -1

typedef  int  band_value_sel;

typedef struct
{
    Line           edge;
    band_value_sel select;
    int            seq;
}
band_edge_hash_rec;

typedef band_edge_hash_rec *band_edge_hash_ptr;
#endif



typedef struct
{
    int       count;
    HashTable table;   
    Pointer   set_vertices;
}
hash_table_rec;

typedef hash_table_rec *hash_table_ptr;



typedef struct
{
    Pointer   set_connections;
    Vector  **index;
    SegList  *list;
}
grad_table_rec;

typedef grad_table_rec *grad_table_ptr;



typedef struct
{
    int     count;
    SegList *conn;
    SegList *conn_id;
}
conn_list_rec;

typedef conn_list_rec *conn_list_ptr;



#ifdef WHITEBOX_TEST
struct
{
    int case_signatures   [ 256 ];
    int polygonalizations [ 8 ];
}
histograms;
#endif


static Field remove_degenerates ( Field in );


#endif /* ( _ISOSURFACE_PASS_NUMBER == 1 ) */



/*---------------------------------------------*\
 | Begin generic templates.
\*---------------------------------------------*/

    /* Moved to _getfield.c */

/*---------------------------------------------*\
 | End generic templates.
\*---------------------------------------------*/



#if ( _ISOSURFACE_PASS_NUMBER == 1 )



static
Pointer copy_seglist_to_memory
            ( SegList *list, Pointer mem, int itemsize, int count )
{
    SegListSegment *sublist  = NULL;
    int            items     = 0;
    char           *location = NULL;
    int            segsize   = 0;

    DXASSERTGOTO ( list != ERROR );
    DXASSERTGOTO ( mem  != ERROR );
    DXASSERTGOTO ( count == DXGetSegListItemCount ( list ) );

    if ( count == 0 ) 
        return mem;

    if ( !DXInitGetNextSegListSegment ( list ) ) goto error;

    for ( items = 0,        location = (char *) mem;
          ( NULL != ( sublist = DXGetNextSegListSegment ( list ) ) );
          items += segsize, location += ( segsize * itemsize ) )
    {
        if ( ERROR == ( segsize = DXGetSegListSegmentItemCount ( sublist ) ) )
            goto error;

        if ( !memcpy ( location,
                       (char *) DXGetSegListSegmentPointer ( sublist ),
                       ( segsize * itemsize ) ) )
            DXErrorGoto2
                ( ERROR_INTERNAL,
                  "#11800", /* C standard library call, %s, returns error */
                  "memcpy()" )
    }

    DXASSERTGOTO ( items == count );

    return mem;

    error:
        ERROR_SECTION;
        return ERROR;
}



static
array_info copy_seglist_to_array ( SegList *list, array_info array )
{
    DXASSERT ( array != ERROR );

    if ( copy_seglist_to_memory
             ( list, array->data, array->itemsize, array->items ) )
        return array;
    else
        return ERROR;
}



static
Error tally_dependers ( field_info input_info, int *posi, int *conn )
{
    component_info  comp;
    int             i;

    DXASSERTGOTO ( posi != ERROR );
    DXASSERTGOTO ( conn != ERROR );

    for ( i=0, *posi=0, *conn=0, comp=input_info->comp_list;
          i<input_info->comp_count;
          i++, comp++ )
    {
        if ( ( 0 == strcmp ( comp->name, "positions" ) ) ||
             ( ( NULL  != comp->std_attribs[(int)DEP]        ) &&
               ( ERROR != comp->std_attribs[(int)DEP]->value ) &&
               ( 0 == strcmp ( comp->std_attribs[(int)DEP]->value,
                               "positions" ) ) ) )
            (*posi)++;
        else
        if ( ( 0 == strcmp ( comp->name, "connections" ) ) ||
             ( ( NULL  != comp->std_attribs[(int)DEP]        ) &&
               ( ERROR != comp->std_attribs[(int)DEP]->value ) &&
               ( 0 == strcmp ( comp->std_attribs[(int)DEP]->value,
                               "connections" ) ) ) )
            (*conn)++;
    }

    return OK;

    /* error: */
        ERROR_SECTION;
        return ERROR;
}



static
SegList *set_c_ID ( SegList *list, int index, int ID )
{
    int  length     = DXGetSegListItemCount ( list );
    int  *list_item;

    DXASSERT ( length <= index );

    while ( length < index )
        if ( ERROR == ( list_item = (int*) DXNewSegListItem ( list ) ) )
            return ERROR;
        else
            { *list_item = ID; length++; }

    DXASSERT ( length == DXGetSegListItemCount ( list ) );

    return list;
}



static
SegList *set_cval ( SegList *list, int index, float val )
{
    int   length     = DXGetSegListItemCount ( list );
    float *list_item;

    DXASSERT ( length <= index );

    while ( length < index )
        if ( ERROR == ( list_item = (float*) DXNewSegListItem ( list ) ) )
            return ERROR;
        else
            { *list_item = val; length++; }

    DXASSERT ( length == DXGetSegListItemCount ( list ) );

    return list;
}



static
Error interp ( array_info info, Pointer scratch,
               int I, int J, double t, Pointer dest, int tot, int out_size )
{
    typedef unsigned int    unsigned_int;
    typedef unsigned char   unsigned_char;
    typedef unsigned short  unsigned_short;

    Pointer si = scratch;
    Pointer sj = (Pointer)&(((char*)scratch)[out_size]);
    int i;

    if (t == 0) {
	if (!info->get_item(I, info, dest))
	    goto error;
    } else if (t == 1) {
	if (!info->get_item(J, info, dest))
	    goto error;
    } else {
	if ( !info->get_item ( I, info, si ) || !info->get_item ( J, info, sj ) )
	    goto error;

	for(i=0; i<tot; i++)
	    ((float*)dest)[i] = (double)((float*)si)[i] + ( (double)((float*)sj)[i] - (double)((float*)si)[i] ) * (double)t;
    }

#if 0
    for(i=0; i<tot; i++)
	printf("i t si sj dest %d %16.10f %16.10f %16.10f %16.10f\n",
	       i, t, ((float*)si)[i], ((float*)sj)[i], ((float*)dest)[i]);
#endif

#if 0
    /* The following interpolation method didn't make sense because get_item  */
    /* always returns float arrays, and the destination array for all cases   */
    /* is also going to be float.  suits 9/5/96                               */

#define INTERP(CTYPE) tot=info->itemsize/sizeof(CTYPE);\
    for(i=0;i<tot;i++) ((CTYPE*)dest)[i] = ((CTYPE*)si)[i] +\
    ( ( (float)(((CTYPE*)sj)[i]) - (float)(((CTYPE*)si)[i]) ) * t );

    switch ( info->type )
    {
        case TYPE_FLOAT:  INTERP ( float          );  break;
        case TYPE_DOUBLE: INTERP ( double         );  break;
        case TYPE_INT:    INTERP ( int            );  break;
        case TYPE_BYTE:   INTERP ( char           );  break;
        case TYPE_UINT:   INTERP ( unsigned_int   );  break;
        case TYPE_UBYTE:  INTERP ( unsigned_char  );  break;
        case TYPE_USHORT: INTERP ( unsigned_char  );  break;
        case TYPE_SHORT:  INTERP ( unsigned_short );  break;
    }

#undef INTERP
#endif

    return OK;

    error:
        ERROR_SECTION;
        return ERROR;
}



static
Field create_output_arrays
          ( Field     output,  /* handle                  */
            int       nP,      /* N positions             */
            int       Pd,      /* positions dim           */
            int       nC,      /* N connections           */
            int       Cd,      /* connections dim         */
            int       nN,      /* N normals               */
            RGBColor  color,   /* default color           */
            float     Dv,      /* data value (isos. only) */
            data_dep  Dd,      /* data dependency         */
            iso_module_types module   /* isosurface/band */ )
{
    Array  array  =  NULL;

    DXASSERTGOTO ( output != ERROR );
    DXASSERTGOTO ( DXGetObjectClass ( (Object) output ) == CLASS_FIELD );

    /* 
     * Basic assumptions for isosurface output: 
     *    Colors and Data are regular (constant) dep positions.
     *    Positions and Connections are always irregular.
     *    Normals are for surfaces.
     *    Connections are used for surfaces and contours only.
     *    Connections are 1d (Lines) or 2d (Triangles) or nonexistant.
     *    int 0 is usable as float, char, ... 0.
     *          (int[2] for double), (int[3] for RBGColor)
     *
     * XXX - libdx
     *   assume that DXSetAttribute(array) and DXSetComponentAttribute 
     *    are interchangeable.
     */ 

    if ( !_dxf_MakeFieldEmpty ( output ) )
        goto error;


    if ( nP == 0 ) 

        return output;

    else
    { 
        /* POSITIONS */
        if ( ( ERROR == ( array
                   = DXNewArray ( TYPE_FLOAT, CATEGORY_REAL, 1, Pd ) ) )
             ||
             !DXAllocateArray ( array, nP )
             ||
             !DXAddArrayData ( array, 0, nP, NULL )
             ||
             !DXTrim ( array )
             ||
             !DXSetAttribute
                  ( (Object)array,
                    "dep", (Object) DXNewString ( "positions" ) )
             ||
             !DXSetComponentValue ( output, "positions", (Object) array ) )
            goto error;

        array = NULL;

        /* COLORS */
/* XXX - conditional */

        if ( ( ERROR == ( array
                   = (Array)DXNewConstantArray
                                ( ((Dd==DEP_POSITIONS)? nP: nC),
                                  (Pointer)&color, TYPE_FLOAT,
                                  CATEGORY_REAL, 1, 3 ) ) )
             ||
             !DXSetAttribute
                 ( (Object)array, "dep",
                   (Object) DXNewString
                            ( (Dd==DEP_POSITIONS)?"positions" :"connections" ) )
             ||
             !DXSetComponentValue ( output, "colors", (Object) array ) )
            goto error;

        array = NULL;

        /* DATA */
        switch ( module )
        {
            case ISOSURFACE:
                DXASSERTGOTO ( Dd == DEP_POSITIONS );
                if ( ERROR == ( array
                         = (Array)DXNewConstantArray
                                      ( nP, (Pointer)&Dv, TYPE_FLOAT,
                                        CATEGORY_REAL, 0, 0 ) ) )
                    goto error;
                break;

            case BAND:
                if ( ( ERROR == ( array
                           = DXNewArray ( TYPE_FLOAT, CATEGORY_REAL, 0, 0 ) ) )
                     ||
                     !DXAllocateArray ( array, ((Dd==DEP_POSITIONS)? nP: nC) )
                     ||
                     !DXAddArrayData
                         ( array, 0, ((Dd==DEP_POSITIONS)? nP: nC), NULL )
                     ||
                     !DXTrim ( array ) )
                    goto error;
                break;
        }

        if ( !DXSetComponentValue ( output, "data", (Object) array )
             ||
             !DXSetComponentAttribute
                  ( output, "data", "dep",
                    (Object) DXNewString
                                ( ( (Dd==DEP_POSITIONS) ?
                                       "positions" : "connections" ) ) ) )
            goto error;

        array = NULL;

        /* NORMALS */
        if ( ( nN != 0 ) && (
             ( ERROR == ( array
                           = DXNewArray ( TYPE_FLOAT, CATEGORY_REAL, 1, 3 ) ) )
             ||
             !DXAllocateArray ( array, nP )
             ||
             !DXAddArrayData ( array, 0, nP, NULL )
             ||
             !DXTrim ( array )
             ||
             !DXSetAttribute
                 ( (Object)array, "dep", (Object) DXNewString ( "positions" ) )
             ||
             !DXSetComponentValue ( output, "normals", (Object) array ) ) )
        goto error;

        array = NULL;
    } 

    if ( nC != 0 ) 
    { 
        char *element_type=NULL;

        switch ( Cd )
        {
            case 1: if ( !_dxf_SetFuzzAttribute ( output, 3 ) ) goto error;
                    element_type = "lines";     break;
            case 2: element_type = "triangles"; break;
            default:
                DXASSERTGOTO ( ( Cd == 1 ) || ( Cd == 2 ) );
        }

        /* CONNECTIONS */
        if ( ( ERROR == ( array =
                    DXNewArray ( TYPE_INT, CATEGORY_REAL, 1, ( Cd + 1 ) ) ) )
             ||
             !DXAllocateArray ( array, nC )
             ||
             !DXAddArrayData ( array, 0, nC, (Pointer) NULL )
             ||
             !DXTrim ( array )
             ||
             !DXSetAttribute
                  ( (Object)array, "ref", (Object) DXNewString ( "positions" ) )
             ||
             !DXSetAttribute
                  ( (Object)array,
                    "element type", (Object) DXNewString ( element_type ) )
             ||
             !DXSetComponentValue ( output, "connections", (Object) array ) )
        goto error;

        array = NULL;
    } 

    return ( output );

    error:
        ERROR_SECTION;

        DXDelete ( (Object) array );  array = NULL;

        return ERROR;
}



static
int iso_edge_compare_func ( Key i, Element j )
{
    /* XXX - requires pre-sorting! */
    return ( ( ( ((iso_edge_hash_ptr)i)->edge.p
                 == 
                 ((iso_edge_hash_ptr)j)->edge.p )
               &&
               ( ((iso_edge_hash_ptr)i)->edge.q
                 == 
                 ((iso_edge_hash_ptr)j)->edge.q ) ) ? 0 : 1 );
}



#ifdef INCLUDES_BAND
static
int band_edge_compare_func ( Key i, Element j )
{
    /* XXX - requires pre-sorting! */
    return ( ( ( ((band_edge_hash_ptr)i)->edge.p
                 == 
                 ((band_edge_hash_ptr)j)->edge.p )
               &&
               ( ((band_edge_hash_ptr)i)->edge.q
                 == 
                 ((band_edge_hash_ptr)j)->edge.q )
               &&
               ( ((band_edge_hash_ptr)i)->select
                 == 
                 ((band_edge_hash_ptr)j)->select ) ) ? 0 : 1 );
}
#endif



static
PseudoKey iso_edge_hash_func ( Key i )
{
#define  k1       ((int)((iso_edge_hash_ptr)i)->edge.p)
#define  k2       ((int)((iso_edge_hash_ptr)i)->edge.q)
#define  PRIME_1  10607 
#define  PRIME_2  18913   

    PseudoKey address = ( ( k1 * PRIME_1 ) + ( k2 * PRIME_2 ) );

#if 0
    DXDebug ( "I", "Address(%d,%d) = %d", k1, k2, address );
#endif

    return address;

#undef  PRIME_1
#undef  PRIME_2
#undef  k1
#undef  k2
}



#ifdef INCLUDES_BAND
static
PseudoKey band_edge_hash_func ( Key i )
{
#define  k1       ((int)((band_edge_hash_ptr)i)->edge.p)
#define  k2       ((int)((band_edge_hash_ptr)i)->edge.q)
#define  k3       ((int)((band_edge_hash_ptr)i)->select)
#define  PRIME_1  10607 
#define  PRIME_2  18913   
#define  PRIME_3  28001

    PseudoKey address = (( k1 * PRIME_1 )+ ( k2 * PRIME_2 )+ ( k3 * PRIME_3 ));

#if 0
    DXDebug ( "I", "Address(%d,%d,%d) = %d", k1, k2, k3, address );
#endif

    return address;

#undef  PRIME_1
#undef  PRIME_2
#undef  PRIME_3
#undef  k1
#undef  k2
#undef  k3
}
#endif



static
int iso_hash_enter ( Line *edge, hash_table_ptr hash, int *o )
{
    iso_edge_hash_rec  element;
    iso_edge_hash_ptr  element_ptr;

    if ( edge->p <= edge->q )
        element.edge = *edge;
    else
        { element.edge.p = edge->q; element.edge.q = edge->p; }

    if ( NULL != ( element_ptr
              = (iso_edge_hash_ptr) DXQueryHashElement
                                    ( hash->table, (Key)&element ) ) )
        *o = element_ptr->seq;
    else
    {
        *o = element.seq = (hash->count)++;

        if ( hash->set_vertices != NULL )
        {
            SETBIT ( hash->set_vertices, element.edge.p, 1 );
            SETBIT ( hash->set_vertices, element.edge.q, 1 );
        }

        if ( !DXInsertHashElement ( hash->table, (Element)&element ) )
            goto error;
    }

    return OK;

    error:
        ERROR_SECTION;
        return ERROR;
}



#ifdef INCLUDES_BAND
static
int band_hash_enter
        ( Line *edge, band_value_sel select, hash_table_ptr hash, int *o )
{
    band_edge_hash_rec  element;
    band_edge_hash_ptr  element_ptr;

   /* DXASSERTGOTO ( ( edge->p == edge->q ) == ( select == NEITHER_VALUE ) ); */

    if ( edge->p <= edge->q )
        element.edge = *edge;
    else
        { element.edge.p = edge->q; element.edge.q = edge->p; }

    element.select = select;

    if ( NULL != ( element_ptr
              = (band_edge_hash_ptr) DXQueryHashElement
                                    ( hash->table, (Key)&element ) ) )
        *o = element_ptr->seq;
    else
    {
        *o = element.seq = (hash->count)++;

        if ( !DXInsertHashElement ( hash->table, (Element)&element ) )
            goto error;
    }

    return OK;

    error:
        ERROR_SECTION;
        return ERROR;
}
#endif



static
int iso_hash_enter_indirect
      ( int *conn, Line *edge, hash_table_ptr hash, int *o, float *data_values )
{
    Line lookup;

    if ( data_values [ edge->p ] == 0.0 )
    {
        lookup.p = conn [ edge->p ];
        lookup.q = conn [ edge->p ];
    }
    else if ( data_values [ edge->q ] == 0.0 )
    {
        lookup.p = conn [ edge->q ];
        lookup.q = conn [ edge->q ];
    }
    else
    {
        lookup.p = conn [ edge->p ];
        lookup.q = conn [ edge->q ];
    }

    return iso_hash_enter ( &lookup, hash, o );
}



#ifdef INCLUDES_BAND
static
int band_hash_enter_indirect
        ( int *conn, Line *edge, band_value_sel select,
          hash_table_ptr hash, int *o, float *data_values, float *band_values )
{
    Line lookup;

    if ( select == NEITHER_VALUE )
    {
        lookup.p = conn [ edge->p ];
        lookup.q = conn [ edge->q ];
    }
    else
    {
        if ( data_values [ edge->p ] == band_values [ select ] )
        {
            lookup.p = conn [ edge->p ];
            lookup.q = conn [ edge->p ];
        }
        else if ( data_values [ edge->q ] == band_values [ select ] )
        {
            lookup.p = conn [ edge->q ];
            lookup.q = conn [ edge->q ];
        }
        else
        {
            lookup.p = conn [ edge->p ];
            lookup.q = conn [ edge->q ];
        }
    }

    return band_hash_enter ( &lookup, select, hash, o );
}
#endif




#define IVTX(E,N)    iso_hash_enter_indirect   \
                        ( conn, E,    vtab, &poly[N], data_values )
#define BVTX(E,S,N)  band_hash_enter_indirect  \
                        ( conn, E, S, vtab, &poly[N], data_values, band_values )
#define BCHK(E,S)    DXASSERT \
                        ( ( E->p == E->q ) == ( S == NEITHER_VALUE ) )

#define TRI(A,B,C) \
    if (( poly[A] != poly[B] )&&( poly[B] != poly[C] )&&( poly[C] != poly[A] )) { \
        if (ERROR == ( tp = (Triangle*)DXNewSegListItem ( tlis ))) goto error;\
        else { tp->p = poly[A];  tp->q = poly[B];  tp->r = poly[C]; } }


static
Error make_iso_line
          ( int *conn, Line *e0, Line *e1,
            hash_table_ptr vtab, hash_table_ptr ltab, float *data_values )
{
    int  poly[2];
    int  XXX;

#ifdef WHITEBOX_TEST
histograms.polygonalizations[2]++;
#endif

    if ( !IVTX ( e0, 0 ) ||
         !IVTX ( e1, 1 )   )
      goto error;

    if ( poly[0] != poly[1] )
        if ( !iso_hash_enter ( (Line*)poly, ltab, &XXX ) )
            goto error;

    return OK;

    error:
        ERROR_SECTION;
        return ERROR;
}



#ifdef INCLUDES_BAND
static
Error make_band_line
          ( int *conn,
            Line          *e0,
            Line          *e1,
            band_value_sel s0,
            band_value_sel s1,
            hash_table_ptr vtab, SegList *llis, float *data_values,
            float *band_values )
{
    int   poly[2];
    Line  *lp;

#ifdef WHITEBOX_TEST
histograms.polygonalizations[2]++;
#endif
BCHK(e0,s0);BCHK(e1,s1);

    if ( !BVTX ( e0, s0, 0 ) ||
         !BVTX ( e1, s1, 1 )   )
      goto error;

    if ( poly[0] != poly[1] ) {
        if ( ERROR == ( lp = (Line*) DXNewSegListItem ( llis ) ) )
            goto error;
        else
        {
            lp->p = poly[0];
            lp->q = poly[1];
        }
    }

    return OK;

    error:
        ERROR_SECTION;
        return ERROR;
}
#endif



static
Error make_iso_poly_3
          ( int *conn, 
            Line *e0, Line *e1, Line *e2,
            hash_table_ptr vtab, SegList *tlis, float *data_values )
{
    int      poly[3];
    Triangle *tp;

#ifdef WHITEBOX_TEST
histograms.polygonalizations[3]++;
#endif

    if ( !IVTX ( e0, 0 ) ||
         !IVTX ( e1, 1 ) ||
         !IVTX ( e2, 2 )   )
      goto error;

    TRI ( 0, 1, 2 );

    return OK;

    error:
        ERROR_SECTION;
        return ERROR;
}



#ifdef INCLUDES_BAND
static
Error make_band_poly_3
          ( int *conn, 
            Line *e0,
            Line *e1,
            Line *e2,
            band_value_sel s0,
            band_value_sel s1,
            band_value_sel s2,
            hash_table_ptr vtab, SegList *tlis, float *data_values,
            float *band_values )
{
    int      poly[3];
    Triangle *tp;

#ifdef WHITEBOX_TEST
histograms.polygonalizations[3]++;
#endif
BCHK(e0,s0);BCHK(e1,s1);BCHK(e2,s2);

    if ( !BVTX ( e0, s0, 0 ) ||
         !BVTX ( e1, s1, 1 ) ||
         !BVTX ( e2, s2, 2 )   )
      goto error;

    TRI ( 0, 1, 2 );

    return OK;

    error:
        ERROR_SECTION;
        return ERROR;
}
#endif


static
Error make_iso_poly_4
          ( int *conn, 
            Line *e0, Line *e1, Line *e2, Line *e3,
            hash_table_ptr vtab, SegList *tlis, float *data_values )
{
    Triangle *tp;
    int      poly[4];


#ifdef WHITEBOX_TEST
histograms.polygonalizations[4]++;
#endif

    if ( !IVTX ( e0, 0 ) ||
         !IVTX ( e1, 1 ) ||
         !IVTX ( e2, 2 ) ||
         !IVTX ( e3, 3 )   )
      goto error;

    TRI ( 0, 1, 2 );
    TRI ( 0, 2, 3 );

    return OK;

    error:
        ERROR_SECTION;
        return ERROR;
}



#ifdef INCLUDES_BAND
static
Error make_band_poly_4
          ( int *conn, 
            Line *e0,
            Line *e1,
            Line *e2,
            Line *e3,
            band_value_sel s0,
            band_value_sel s1,
            band_value_sel s2,
            band_value_sel s3,
            hash_table_ptr vtab, SegList *tlis, float *data_values,
            float *band_values )
{
    Triangle *tp;
    int      poly[4];


#ifdef WHITEBOX_TEST
histograms.polygonalizations[4]++;
#endif
BCHK(e0,s0);BCHK(e1,s1);BCHK(e2,s2);BCHK(e3,s3);

    if ( !BVTX ( e0, s0, 0 ) ||
         !BVTX ( e1, s1, 1 ) ||
         !BVTX ( e2, s2, 2 ) ||
         !BVTX ( e3, s3, 3 )   )
      goto error;

    TRI ( 0, 1, 2 );
    TRI ( 0, 2, 3 );

    return OK;

    error:
        ERROR_SECTION;
        return ERROR;
}
#endif



static
Error make_iso_poly_5
          ( int *conn, 
            Line *e0, Line *e1, Line *e2, Line *e3, Line *e4,
            hash_table_ptr vtab, SegList *tlis, float *data_values )
{
    Triangle *tp;
    int      poly[5];

#ifdef WHITEBOX_TEST
histograms.polygonalizations[5]++;
#endif

    if ( !IVTX ( e0, 0 ) ||
         !IVTX ( e1, 1 ) ||
         !IVTX ( e2, 2 ) ||
         !IVTX ( e3, 3 ) ||
         !IVTX ( e4, 4 )   )
      goto error;

    TRI ( 0, 1, 2 );
    TRI ( 0, 2, 3 );
    TRI ( 0, 3, 4 );

    return OK;

    error:
        ERROR_SECTION;
        return ERROR;
}



#ifdef INCLUDES_BAND
static
Error make_band_poly_5
          ( int *conn, 
            Line *e0,
            Line *e1,
            Line *e2,
            Line *e3,
            Line *e4,
            band_value_sel s0,
            band_value_sel s1,
            band_value_sel s2,
            band_value_sel s3,
            band_value_sel s4,
            hash_table_ptr vtab, SegList *tlis, float *data_values,
            float *band_values )
{
    Triangle *tp;
    int      poly[5];

#ifdef WHITEBOX_TEST
histograms.polygonalizations[5]++;
#endif
BCHK(e0,s0);BCHK(e1,s1);BCHK(e2,s2);BCHK(e3,s3);BCHK(e4,s4);

    if ( !BVTX ( e0, s0, 0 ) ||
         !BVTX ( e1, s1, 1 ) ||
         !BVTX ( e2, s2, 2 ) ||
         !BVTX ( e3, s3, 3 ) ||
         !BVTX ( e4, s4, 4 )   )
      goto error;

    TRI ( 0, 1, 2 );
    TRI ( 0, 2, 3 );
    TRI ( 0, 3, 4 );

    return OK;

    error:
        ERROR_SECTION;
        return ERROR;
}
#endif



static
Error make_iso_poly_6
          ( int *conn, 
            Line *e0, Line *e1, Line *e2, Line *e3, Line *e4, Line *e5,
            hash_table_ptr vtab, SegList *tlis, float *data_values )
{
    Triangle *tp;
    int      poly[6];

#ifdef WHITEBOX_TEST
histograms.polygonalizations[6]++;
#endif

    if ( !IVTX ( e0, 0 ) ||
         !IVTX ( e1, 1 ) ||
         !IVTX ( e2, 2 ) ||
         !IVTX ( e3, 3 ) ||
         !IVTX ( e4, 4 ) ||
         !IVTX ( e5, 5 )   )
      goto error;

    TRI ( 0, 1, 2 );
    TRI ( 0, 2, 3 );
    TRI ( 0, 3, 4 );
    TRI ( 0, 4, 5 );

    return OK;

    error:
        ERROR_SECTION;
        return ERROR;
}


#ifdef INCLUDES_BAND
static
Error make_band_poly_6
          ( int *conn, 
            Line *e0,
            Line *e1,
            Line *e2,
            Line *e3,
            Line *e4,
            Line *e5,
            band_value_sel s0,
            band_value_sel s1,
            band_value_sel s2,
            band_value_sel s3,
            band_value_sel s4,
            band_value_sel s5,
            hash_table_ptr vtab, SegList *tlis, float *data_values,
            float *band_values )
{
    Triangle *tp;
    int      poly[6];

#ifdef WHITEBOX_TEST
histograms.polygonalizations[6]++;
#endif
BCHK(e0,s0);BCHK(e1,s1);BCHK(e2,s2);BCHK(e3,s3);BCHK(e4,s4);BCHK(e5,s5);

    if ( !BVTX ( e0, s0, 0 ) ||
         !BVTX ( e1, s1, 1 ) ||
         !BVTX ( e2, s2, 2 ) ||
         !BVTX ( e3, s3, 3 ) ||
         !BVTX ( e4, s4, 4 ) ||
         !BVTX ( e5, s5, 5 )   )
      goto error;

    TRI ( 0, 1, 2 );
    TRI ( 0, 2, 3 );
    TRI ( 0, 3, 4 );
    TRI ( 0, 4, 5 );

    return OK;

    error:
        ERROR_SECTION;
        return ERROR;
}
#endif




static
Error make_iso_poly_7
         ( int *conn, 
           Line *e0, Line *e1, Line *e2,
           Line *e3, Line *e4, Line *e5, Line *e6,
           hash_table_ptr vtab, SegList *tlis, float *data_values )
{
    Triangle *tp;
    int      poly[7];

#ifdef WHITEBOX_TEST
histograms.polygonalizations[7]++;
#endif

    if ( !IVTX ( e0, 0 ) ||
         !IVTX ( e1, 1 ) ||
         !IVTX ( e2, 2 ) ||
         !IVTX ( e3, 3 ) ||
         !IVTX ( e4, 4 ) ||
         !IVTX ( e5, 5 ) ||
         !IVTX ( e6, 6 )   )
      goto error;

    TRI ( 0, 1, 3 );
    TRI ( 0, 3, 4 );
    TRI ( 0, 4, 6 );
    TRI ( 6, 4, 5 );
    TRI ( 1, 2, 3 );

    return OK;

    error:
        ERROR_SECTION;
        return ERROR;
}

#undef  IVTX
#undef  BVTX
#undef  POLY



#ifdef INCLUDES_BAND
static
Error band_convert_line
          ( Line            *line,
            float           data_values[2],
            float           *value_p,
            int             index,
            hash_table_ptr  vtx_hash,
            SegList         *line_list,
            int             equivalence )
{
    float value_low  = value_p [ index ];
    float value_high = value_p [ index + 1 ];

    static Line line_edge[3] = { { 0, 1 }, { 0, 0 }, { 1, 1 } };

    int  signature = 0;

    if ( equivalence == 1 );

    if ( data_values[0] > value_low  ) signature += 1;
    if ( data_values[0] > value_high ) signature += 2;
    if ( data_values[1] > value_low  ) signature += 4;
    if ( data_values[1] > value_high ) signature += 8;

#ifdef WHITEBOX_TEST
    DXDebug ( "I", "Signature = %d", signature );
    histograms.case_signatures[signature]++;
#endif

    switch ( signature )
    {
        /* 
         * This table was entered symbolically into a file called table.DB
         * and then converted to 'C' code in a file 'table.C' by a script
         * named 'table.MAK'.  
         * All files currently reside in the directory ~allain/kalvin.
         */

#define LINE(A,B,C,D)  if ( !make_band_line ( (int *)line, \
                   &line_edge[A], \
                   &line_edge[B], \
                   ((C==0)?-1:(index+C-1)), \
                   ((D==0)?-1:(index+D-1)), \
               vtx_hash, line_list, data_values, value_p ) ) goto error;

        case  0:
        case 15: break;

        case  1: LINE ( 1,0,  0,1 );  break;
        case  3: LINE ( 0,0,  2,1 );  break;
        case  4: LINE ( 0,2,  1,0 );  break;
        case  5: LINE ( 1,2,  0,0 );  break;
        case  7: LINE ( 0,2,  2,0 );  break;
        case 12: LINE ( 0,0,  1,2 );  break;
        case 13: LINE ( 1,0,  0,2 );  break;

        default:
            DXSetError ( ERROR_ASSERTION, "case signature %d", signature );
            goto error;

#undef  LINE

    }

    return OK;

    error:
        ERROR_SECTION;
        return ERROR;
}
#endif



static
Error iso_convert_triangle
          ( Triangle        *tri,
            float           data_values[3],
            hash_table_ptr  vtx_hash,
            hash_table_ptr  edge_hash,
            int             equivalence )
{
    static Line tri_edge[3] = { { 0, 1 }, { 1, 2 }, { 2, 0 } };

    int  signature = 0;

    if ( equivalence == 1 )
    {
        if ( data_values[0] > 0.0 ) signature += 1;
        if ( data_values[1] > 0.0 ) signature += 2;
        if ( data_values[2] > 0.0 ) signature += 4;
    }
    else
    {
        if ( data_values[0] == 0.0 ) signature += 1;
        if ( data_values[1] == 0.0 ) signature += 2;
        if ( data_values[2] == 0.0 ) signature += 4;
    }

#ifdef WHITEBOX_TEST
    DXDebug ( "I", "Signature = %d", signature );
    histograms.case_signatures[signature]++;
#endif

    switch ( signature )
    {
        /* 
         * This table was entered symbolically into a file called table.DB
         * and then converted to 'C' code in a file 'table.C' by a script
         * named 'table.MAK'.  
         * All files currently reside in the directory ~allain/kalvin.
         */

#define LINE(A,B)  if ( !make_iso_line ( (int *)tri, &tri_edge[A], \
                     &tri_edge[B], vtx_hash, edge_hash, data_values ) ) \
                   goto error;

        case 0: break;
        case 1: LINE ( 0, 2 );  break;
        case 2: LINE ( 0, 1 );  break;
        case 3: LINE ( 1, 2 );  break;
        case 4: LINE ( 1, 2 );  break;
        case 5: LINE ( 0, 1 );  break;
        case 6: LINE ( 0, 2 );  break;
        case 7: break;

#undef  LINE

    }

    return OK;

    error:
        ERROR_SECTION;
        return ERROR;
}


#ifdef INCLUDES_BAND
static
Error band_convert_triangle
          ( Triangle        *tri,
            float           data_values[3],
            float           *value_p,
            int             index,
            hash_table_ptr  vtx_hash,
            SegList         *tri_list,
            int             equivalence )
{
    float value_low  = value_p [ index ];
    float value_high = value_p [ index + 1 ];

    static Line tri_edge[6] = { { 0, 1 }, { 1, 2 }, { 2, 0 },
                                { 0, 0 }, { 1, 1 }, { 2, 2 } };

    int  signature = 0;

    if ( equivalence == 1 );

    if ( data_values[0] > value_low  ) signature += 1;
    if ( data_values[0] > value_high ) signature += 2;
    if ( data_values[1] > value_low  ) signature += 4;
    if ( data_values[1] > value_high ) signature += 8;
    if ( data_values[2] > value_low  ) signature += 16;
    if ( data_values[2] > value_high ) signature += 32;

#ifdef WHITEBOX_TEST
    DXDebug ( "I", "Signature = %d", signature );
    histograms.case_signatures[signature]++;
#endif

    switch ( signature )
    {
        /* 
         * This table was entered symbolically into a file called table.DB
         * and then converted to 'C' code in a file 'table.C' by a script
         * named 'table.MAK'.  
         * All files currently reside in the directory ~allain/kalvin.
         */


#define POLY3(A,B,C,D,E,F)  if(!make_band_poly_3((int*)tri, \
                      &tri_edge[A], \
                      &tri_edge[B], \
                      &tri_edge[C], \
                      ((D==0)?-1:(index+D-1)), \
                      ((E==0)?-1:(index+E-1)), \
                      ((F==0)?-1:(index+F-1)), \
                vtx_hash, tri_list, data_values, value_p ) ) goto error;

#define POLY4(A,B,C,D,E,F,G,H)  if(!make_band_poly_4((int*)tri, \
                      &tri_edge[A], \
                      &tri_edge[B], \
                      &tri_edge[C], \
                      &tri_edge[D], \
                      ((E==0)?-1:(index+E-1)), \
                      ((F==0)?-1:(index+F-1)), \
                      ((G==0)?-1:(index+G-1)), \
                      ((H==0)?-1:(index+H-1)), \
                vtx_hash, tri_list, data_values, value_p ) ) goto error;

#define POLY5(A,B,C,D,E,F,G,H,I,J)  if(!make_band_poly_5((int*)tri, \
                      &tri_edge[A], \
                      &tri_edge[B], \
                      &tri_edge[C], \
                      &tri_edge[D], \
                      &tri_edge[E], \
                      ((F==0)?-1:(index+F-1)), \
                      ((G==0)?-1:(index+G-1)), \
                      ((H==0)?-1:(index+H-1)), \
                      ((I==0)?-1:(index+I-1)), \
                      ((J==0)?-1:(index+J-1)), \
                vtx_hash, tri_list, data_values, value_p ) ) goto error;

        case  0:
        case 63: break;

        case  1: POLY3 ( 0,2,3,      1,1,0     );  break;
        case  3: POLY4 ( 0,0,2,2,    2,1,1,2   );  break;
        case  4: POLY3 ( 4,1,0,      0,1,1     );  break;
        case  5: POLY4 ( 3,4,1,2,    0,0,1,1   );  break;
        case  7: POLY5 ( 0,4,1,2,2,  2,0,1,1,2 );  break;
        case 12: POLY4 ( 0,0,1,1,    1,2,2,1   );  break;
        case 13: POLY5 ( 3,0,1,1,2,  0,2,2,1,1 );  break;
        case 15: POLY4 ( 2,1,1,2,    2,2,1,1   );  break;
        case 16: POLY3 ( 2,1,5,      1,1,0     );  break;
        case 17: POLY4 ( 3,0,1,5,    0,1,1,0   );  break;
        case 19: POLY5 ( 0,0,1,5,2,  2,1,1,0,2 );  break;
        case 20: POLY4 ( 0,4,5,2,    1,0,0,1   );  break;
        case 21: POLY3 ( 3,4,5,      0,0,0     );  break;
        case 23: POLY4 ( 0,4,5,2,    2,0,0,2   );  break;
        case 28: POLY5 ( 0,0,1,5,2,  1,2,2,0,1 );  break;
        case 29: POLY4 ( 3,0,1,5,    0,2,2,0   );  break;
        case 31: POLY3 ( 2,1,5,      2,2,0     );  break;
        case 48: POLY4 ( 2,1,1,2,    1,1,2,2   );  break;
        case 49: POLY5 ( 3,0,1,1,2,  0,1,1,2,2 );  break;
        case 51: POLY4 ( 0,0,1,1,    2,1,1,2   );  break;
        case 52: POLY5 ( 0,4,1,2,2,  1,0,2,2,1 );  break;
        case 53: POLY4 ( 3,4,1,2,    0,0,2,2   );  break;
        case 55: POLY3 ( 0,4,1,      2,0,2     );  break;
        case 60: POLY4 ( 0,0,2,2,    1,2,2,1   );  break;
        case 61: POLY3 ( 3,0,2,      0,2,2     );  break;

        default:
            DXSetError ( ERROR_ASSERTION, "case signature %d", signature );
            goto error;

#undef  POLY3
#undef  POLY4
#undef  POLY5

    }

    return OK;

    error:
        ERROR_SECTION;
        return ERROR;
}
#endif



static
Error iso_convert_quadrilateral
          ( Quadrilateral   *quad,
            float           data_values[4],
            hash_table_ptr  vtx_hash,
            hash_table_ptr  edge_hash,
            int             equivalence )
{

#if 0

    Triangle n_tri;
    float    n_data_values[3];
    int      i, j;

    static int cvt[2][3] = { { 0, 2, 3 }, { 0, 3, 1 } };

    for ( i=0; i<2; i++ )
    {
        for ( j=0; j<3; j++ )
        {
            n_data_values[j]   = data_values   [ cvt[i][j] ];
            ((int *)&n_tri)[j] = ((int *)quad) [ cvt[i][j] ];
        }

        if ( !iso_convert_triangle
                  ( &n_tri, n_data_values,
                    vtx_hash, edge_hash, equivalence ) )
            goto error;
    }

#else

    static Line quad_edge[4] = { { 0, 1 }, { 1, 3 }, { 2, 3 }, { 0, 2 } };

    int  signature = 0;

    if ( equivalence == 1 )
    {
        if ( data_values[0] > 0.0 ) signature += 1;
        if ( data_values[1] > 0.0 ) signature += 2;
        if ( data_values[2] > 0.0 ) signature += 4;
        if ( data_values[3] > 0.0 ) signature += 8;
    }
    else
    {
        if ( data_values[0] == 0.0 ) signature += 1;
        if ( data_values[1] == 0.0 ) signature += 2;
        if ( data_values[2] == 0.0 ) signature += 4;
        if ( data_values[3] == 0.0 ) signature += 8;
    }

#ifdef WHITEBOX_TEST
    DXDebug ( "I", "Signature = %d", signature );
    histograms.case_signatures[signature]++;
#endif

    switch ( signature )
    {
        /* 
         * This table was entered symbolically into a file called table.DB
         * and then converted to 'C' code in a file 'table.C' by a script
         * named 'table.MAK'.  
         * All files currently reside in the directory ~allain/kalvin.
         */

#define LINE(A,B)  if ( !make_iso_line ( (int *)quad, &quad_edge[A], \
                           &quad_edge[B], vtx_hash, edge_hash, data_values ) ) \
                   goto error;

        case  0: break;
        case  1: LINE ( 0,3 );               break;
        case  2: LINE ( 0,1 );               break;
        case  3: LINE ( 1,3 );               break;
        case  4: LINE ( 2,3 );               break;
        case  5: LINE ( 0,2 );               break;
        case  6: LINE ( 0,1 ); LINE ( 2,3 ); break;
        case  7: LINE ( 1,2 );               break;
        case  8: LINE ( 1,2 );               break;
        case  9: LINE ( 0,3 ); LINE ( 1,2 ); break;
        case 10: LINE ( 0,2 );               break;
        case 11: LINE ( 2,3 );               break;
        case 12: LINE ( 1,3 );               break;
        case 13: LINE ( 0,1 );               break;
        case 14: LINE ( 0,3 );               break;
        case 15: break;

#undef  LINE

    }

#endif

    return OK;

    error:
        ERROR_SECTION;
        return ERROR;
}



#ifdef INCLUDES_BAND
static
Error band_convert_quadrilateral
          ( Quadrilateral   *quad,
            float           data_values[4],
            float           *value_p,
            int             index,
            hash_table_ptr  vtx_hash,
            SegList         *tri_list,
            int             equivalence )
{

#if 0

    Triangle n_tri;
    float    n_data_values[3];
    int      i, j;

    static int cvt[2][3] = { { 0, 2, 3 }, { 0, 3, 1 } };

    for ( i=0; i<2; i++ )
    {
        for ( j=0; j<3; j++ )
        {
            n_data_values[j]   = data_values   [ cvt[i][j] ];
            ((int *)&n_tri)[j] = ((int *)quad) [ cvt[i][j] ];
        }

        if ( !band_convert_triangle
                  ( &n_tri, n_data_values, value_p, index,
                     vtx_hash, tri_list, equivalence ) )
            goto error;
    }

#else

    float value_low  = value_p [ index ];
    float value_high = value_p [ index + 1 ];

    static Line quad_edge[8] = { { 0, 1 }, { 1, 3 }, { 2, 3 }, { 0, 2 },
                                 { 0, 0 }, { 1, 1 }, { 2, 2 }, { 3, 3 } };

    int  signature = 0;

    if ( equivalence == 1 );

    if ( data_values[0] > value_low  ) signature += 1;
    if ( data_values[0] > value_high ) signature += 2;
    if ( data_values[1] > value_low  ) signature += 4;
    if ( data_values[1] > value_high ) signature += 8;
    if ( data_values[2] > value_low  ) signature += 16;
    if ( data_values[2] > value_high ) signature += 32;
    if ( data_values[3] > value_low  ) signature += 64;
    if ( data_values[3] > value_high ) signature += 128;

#ifdef WHITEBOX_TEST
    DXDebug ( "I", "Signature = %d", signature );
    histograms.case_signatures[signature]++;
#endif

    switch ( signature )
    {

#define POLY3(A,B,C,D,E,F)  if(!make_band_poly_3((int*)quad, \
                      &quad_edge[A], \
                      &quad_edge[B], \
                      &quad_edge[C], \
                      ((D==0)?-1:(index+D-1)), \
                      ((E==0)?-1:(index+E-1)), \
                      ((F==0)?-1:(index+F-1)), \
                vtx_hash, tri_list, data_values, value_p ) ) goto error;

#define POLY4(A,B,C,D,E,F,G,H)  if(!make_band_poly_4((int*)quad, \
                      &quad_edge[A], \
                      &quad_edge[B], \
                      &quad_edge[C], \
                      &quad_edge[D], \
                      ((E==0)?-1:(index+E-1)), \
                      ((F==0)?-1:(index+F-1)), \
                      ((G==0)?-1:(index+G-1)), \
                      ((H==0)?-1:(index+H-1)), \
                vtx_hash, tri_list, data_values, value_p ) ) goto error;

#define POLY5(A,B,C,D,E,F,G,H,I,J)  if(!make_band_poly_5((int*)quad, \
                      &quad_edge[A], \
                      &quad_edge[B], \
                      &quad_edge[C], \
                      &quad_edge[D], \
                      &quad_edge[E], \
                      ((F==0)?-1:(index+F-1)), \
                      ((G==0)?-1:(index+G-1)), \
                      ((H==0)?-1:(index+H-1)), \
                      ((I==0)?-1:(index+I-1)), \
                      ((J==0)?-1:(index+J-1)), \
                vtx_hash, tri_list, data_values, value_p ) ) goto error;

#define POLY6(A,B,C,D,E,F,G,H,I,J,K,L)  if(!make_band_poly_6((int*)quad, \
                      &quad_edge[A], \
                      &quad_edge[B], \
                      &quad_edge[C], \
                      &quad_edge[D], \
                      &quad_edge[E], \
                      &quad_edge[F], \
                      ((G==0)?-1:(index+G-1)), \
                      ((H==0)?-1:(index+H-1)), \
                      ((I==0)?-1:(index+I-1)), \
                      ((J==0)?-1:(index+J-1)), \
                      ((K==0)?-1:(index+K-1)), \
                      ((L==0)?-1:(index+L-1)), \
                vtx_hash, tri_list, data_values, value_p ) ) goto error;

        case   0:
        case 255: break;

        case   1: POLY3 ( 4,0,3,        0,1,1       );  break;
        case   3: POLY4 ( 0,0,3,3,      2,1,1,2     );  break;
        case   4: POLY3 ( 0,5,1,        1,0,1       );  break;
        case   5: POLY4 ( 4,5,1,3,      0,0,1,1     );  break;
        case   7: POLY5 ( 0,5,1,3,3,    2,0,1,1,2   );  break;
        case  12: POLY4 ( 0,0,1,1,      1,2,2,1     );  break;
        case  13: POLY5 ( 4,0,1,1,3,    0,2,2,1,1   );  break;
        case  15: POLY4 ( 3,1,1,3,      2,2,1,1     );  break;
        case  16: POLY3 ( 3,2,6,        1,1,0       );  break;
        case  17: POLY4 ( 4,0,2,6,      0,1,1,0     );  break;
        case  19: POLY5 ( 0,0,2,6,3,    2,1,1,0,2   );  break;
        case  20: POLY3 ( 0,5,1,        1,0,1       );
                  POLY3 ( 3,2,6,        1,1,0       );  break;
        case  21: POLY5 ( 4,5,1,2,6,    0,0,1,1,0   );  break;
        case  23: POLY6 ( 0,5,1,2,6,3,  2,0,1,1,0,2 );  break;
        case  28: POLY3 ( 3,2,6,        1,1,0       );
                  POLY4 ( 0,0,1,1,      1,2,2,1     );  break;
        case  29: POLY6 ( 4,0,1,1,2,6,  0,2,2,1,1,0 );  break;
        case  31: POLY5 ( 3,1,1,2,6,    2,2,1,1,0   );  break;
        case  48: POLY4 ( 3,2,2,3,      1,1,2,2     );  break;
        case  49: POLY5 ( 4,0,2,2,3,    0,1,1,2,2   );  break;
        case  51: POLY4 ( 0,0,2,2,      2,1,1,2     );  break;
        case  52: POLY3 ( 0,5,1,        1,0,1       ); 
                  POLY4 ( 3,2,2,3,      1,1,2,2     );  break;
        case  53: POLY6 ( 4,5,1,2,2,3,  0,0,1,1,2,2 );  break;
        case  55: POLY5 ( 0,5,1,2,2,    2,0,1,1,2   );  break;
        case  60: POLY4 ( 3,2,2,3,      1,1,2,2     );
                  POLY4 ( 0,0,1,1,      1,2,2,1     );  break;
        case  61: POLY3 ( 4,0,3,        0,2,2       );
                  POLY6 ( 0,1,1,2,2,3,  2,2,1,1,2,2 );  break;
        case  63: POLY4 ( 2,1,1,2,      2,2,1,1     );  break;
        case  64: POLY3 ( 1,7,2,        1,0,1       );  break;
        case  65: POLY3 ( 4,0,3,        0,1,1       );
                  POLY3 ( 1,7,2,        1,0,1       );  break;
        case  67: POLY3 ( 2,1,7,        1,1,0       ); 
                  POLY4 ( 3,0,0,3,      2,2,1,1     );  break;
        case  68: POLY4 ( 0,5,7,2,      1,0,0,1     );  break;
        case  69: POLY5 ( 4,5,7,2,3,    0,0,0,1,1   );  break;
        case  71: POLY6 ( 0,5,7,2,3,3,  2,0,0,1,1,2 );  break;
        case  76: POLY5 ( 0,0,1,7,2,    1,2,2,0,1   );  break;
        case  77: POLY6 ( 4,0,1,7,2,3,  0,2,2,0,1,1 );  break;
        case  79: POLY5 ( 3,1,7,2,3,    2,2,0,1,1   );  break;
        case  80: POLY4 ( 3,1,7,6,      1,1,0,0     );  break;
        case  81: POLY5 ( 4,0,1,7,6,    0,1,1,0,0   );  break;
        case  83: POLY6 ( 0,0,1,7,6,3,  2,1,1,0,0,2 );  break;
        case  84: POLY5 ( 0,5,7,6,3,    1,0,0,0,1   );  break;
        case  85: POLY4 ( 4,5,7,6,      0,0,0,0     );  break;
        case  87: POLY5 ( 0,5,7,6,3,    2,0,0,0,2   );  break;
        case  92: POLY6 ( 0,0,1,7,6,3,  1,2,2,0,0,1 );  break;
        case  93: POLY5 ( 4,0,1,7,6,    0,2,2,0,0   );  break;
        case  95: POLY4 ( 3,1,7,6,      2,2,0,0     );  break;
        case 112: POLY5 ( 3,1,7,2,3,    1,1,0,2,2   );  break;
        case 113: POLY6 ( 4,0,1,7,2,3,  0,1,1,0,2,2 );  break;
        case 115: POLY5 ( 0,0,1,7,2,    2,1,1,0,2   );  break;
        case 116: POLY6 ( 0,5,7,2,3,3,  1,0,0,2,2,1 );  break;
        case 117: POLY5 ( 4,5,7,2,3,    0,0,0,2,2   );  break;
        case 119: POLY4 ( 0,5,7,2,      2,0,0,2     );  break;
        case 124: POLY3 ( 1,7,2,        2,0,2       );
                  POLY6 ( 0,0,1,2,3,3,  1,2,2,2,2,1 );  break;
        case 125: POLY6 ( 4,0,1,7,2,3,  0,2,2,0,2,2 );  break;
        case 127: POLY3 ( 1,7,2,        2,0,2       );  break;
        case 192: POLY4 ( 1,1,2,2,      1,2,2,1     );  break;
        case 193: POLY3 ( 4,0,3,        0,1,1       );
                  POLY4 ( 1,1,2,2,      1,2,2,1     );  break;
        case 195: POLY4 ( 0,0,3,3,      2,1,1,2     );
                  POLY4 ( 1,1,2,2,      1,2,2,1     );  break;
        case 196: POLY5 ( 0,5,1,2,2,    1,0,2,2,1   );  break;
        case 197: POLY6 ( 4,5,1,2,2,3,  0,0,2,2,1,1 );  break;
        case 199: POLY3 ( 0,5,1,        2,0,2       ); 
                  POLY6 ( 0,1,2,2,3,3,  2,2,2,1,1,2 );  break;
        case 204: POLY4 ( 0,0,2,2,      1,2,2,1     );  break;
        case 205: POLY5 ( 4,0,2,2,3,    0,2,2,1,1   );  break;
        case 207: POLY4 ( 3,2,2,3,      2,2,1,1     );  break;
        case 208: POLY5 ( 3,1,1,2,6,    1,1,2,2,0   );  break;
        case 209: POLY6 ( 4,0,1,1,2,6,  0,1,1,2,2,0 );  break;
        case 211: POLY3 ( 3,2,6,        2,2,0       ); 
                  POLY6 ( 0,0,1,1,2,3,  2,1,1,2,2,2 );  break;
        case 212: POLY6 ( 0,5,1,2,6,3,  1,0,2,2,0,1 );  break;
        case 213: POLY5 ( 4,5,1,2,6,    0,0,2,2,0   );  break;
        case 215: POLY6 ( 3,0,5,1,2,6,  2,2,0,2,2,0 );  break;
        case 220: POLY5 ( 0,0,2,6,3,    1,2,2,0,1   );  break;
        case 221: POLY4 ( 4,0,2,6,      0,2,2,0     );  break;
        case 223: POLY3 ( 3,2,6,        2,2,0       );  break;
        case 240: POLY4 ( 3,1,1,3,      1,1,2,2     );  break;
        case 241: POLY5 ( 4,0,1,1,3,    0,1,1,2,2   );  break;
        case 243: POLY4 ( 0,0,1,1,      2,1,1,2     );  break;
        case 244: POLY5 ( 0,5,1,3,3,    1,0,2,2,1   );  break;
        case 245: POLY4 ( 4,5,1,3,      0,0,2,2     );  break;
        case 247: POLY3 ( 0,5,1,        2,0,2       );  break;
        case 252: POLY4 ( 0,0,3,3,      1,2,2,1     );  break;
        case 253: POLY3 ( 4,0,3,        0,2,2       );  break;

        default:
            DXSetError ( ERROR_ASSERTION, "case signature %d", signature );
            goto error;

#undef  POLY3
#undef  POLY4
#undef  POLY5
#undef  POLY6

    }

#endif

    return OK;

    error:
        ERROR_SECTION;
        return ERROR;
}
#endif



static
Error iso_convert_tetrahedron
          ( Tetrahedron     *tet,
            float           data_values[4],
            hash_table_ptr  vtx_hash,
            SegList         *tri_list,
            int             equivalence )
{
    static Line tetra_edge[6]
        = { { 0, 1 }, { 1, 2 }, { 0, 2 }, { 0, 3 }, { 1, 3 }, { 2, 3 } };

    int  signature = 0;

    if ( equivalence == 1 )
    {
        if ( data_values[0] > 0.0 ) signature += 1;
        if ( data_values[1] > 0.0 ) signature += 2;
        if ( data_values[2] > 0.0 ) signature += 4;
        if ( data_values[3] > 0.0 ) signature += 8;
    }
    else
    {
        if ( data_values[0] == 0.0 ) signature += 1;
        if ( data_values[1] == 0.0 ) signature += 2;
        if ( data_values[2] == 0.0 ) signature += 4;
        if ( data_values[3] == 0.0 ) signature += 8;
    }

#ifdef WHITEBOX_TEST
    DXDebug ( "I", "Signature = %d", signature );
    histograms.case_signatures[signature]++;
#endif

    switch ( signature )
    {
        /* 
         * This table was entered symbolically into a file called table.DB
         * and then converted to 'C' code in a file 'table.C' by a script
         * named 'table.MAK'.  
         * All files currently reside in the directory ~allain/kalvin.
         */

        /* coded in inversions imply that the table is reversed */

#define POLY3(A,B,C)  if(!make_iso_poly_3((int*)tet, \
                         &tetra_edge[C], &tetra_edge[B], &tetra_edge[A], \
                         vtx_hash, tri_list, data_values ) ) goto error;

#define POLY4(A,B,C,D)  if(!make_iso_poly_4((int*)tet, \
                         &tetra_edge[D], &tetra_edge[C], \
                         &tetra_edge[B], &tetra_edge[A], \
                         vtx_hash, tri_list, data_values ) ) goto error;

            case  0:  break;
            case  1:  POLY3 ( 0, 2, 3 );     break;
            case  2:  POLY3 ( 0, 4, 1 );     break;
            case  3:  POLY4 ( 1, 2, 3, 4 );  break;
            case  4:  POLY3 ( 1, 5, 2 );     break;
            case  5:  POLY4 ( 0, 1, 5, 3 );  break;
            case  6:  POLY4 ( 0, 4, 5, 2 );  break;
            case  7:  POLY3 ( 3, 4, 5 );     break;
            case  8:  POLY3 ( 3, 5, 4 );     break;
            case  9:  POLY4 ( 0, 2, 5, 4 );  break;
            case 10:  POLY4 ( 0, 3, 5, 1 );  break;
            case 11:  POLY3 ( 1, 2, 5 );     break;
            case 12:  POLY4 ( 1, 4, 3, 2 );  break;
            case 13:  POLY3 ( 0, 1, 4 );     break;
            case 14:  POLY3 ( 0, 3, 2 );     break;
            case 15:  break;

#undef  POLY3
#undef  POLY4

        default: break;
    }

    return OK;

    error:
        ERROR_SECTION;
        return ERROR;
}



static
Error iso_convert_cube
          ( Cube            *cube,
            float           data_values[8],
            hash_table_ptr  vtx_hash,
            SegList         *tri_list,
            int             equivalence )
{
    static Line cube_edge[12]
        = { { 0, 1 }, { 1, 3 }, { 0, 2 }, { 2, 3 },
            { 4, 5 }, { 5, 7 }, { 4, 6 }, { 6, 7 },
            { 0, 4 }, { 1, 5 }, { 2, 6 }, { 3, 7 } };

    int  signature = 0;

    if ( equivalence == 1 )
    {
        if ( data_values[0] > 0.0 ) signature += 1;
        if ( data_values[1] > 0.0 ) signature += 2;
        if ( data_values[2] > 0.0 ) signature += 4;
        if ( data_values[3] > 0.0 ) signature += 8;
        if ( data_values[4] > 0.0 ) signature += 16;
        if ( data_values[5] > 0.0 ) signature += 32;
        if ( data_values[6] > 0.0 ) signature += 64;
        if ( data_values[7] > 0.0 ) signature += 128;
    }
    else
    {
        if ( data_values[0] == 0.0 ) signature += 1;
        if ( data_values[1] == 0.0 ) signature += 2;
        if ( data_values[2] == 0.0 ) signature += 4;
        if ( data_values[3] == 0.0 ) signature += 8;
        if ( data_values[4] == 0.0 ) signature += 16;
        if ( data_values[5] == 0.0 ) signature += 32;
        if ( data_values[6] == 0.0 ) signature += 64;
        if ( data_values[7] == 0.0 ) signature += 128;
    }

#ifdef WHITEBOX_TEST
    DXDebug ( "I", "Signature = %d", signature );
    histograms.case_signatures[signature]++;
#endif

    switch ( signature )
    {
        /* 
         * This table was entered symbolically into a file called table.DB
         * and then converted to 'C' code in a file 'table.C' by a script
         * named 'table.MAK'.  
         * All files currently reside in the directory ~allain/kalvin.
         */

#define POLY3(A,B,C)  if(!make_iso_poly_3((int*)cube, \
                         &cube_edge[A], &cube_edge[B], &cube_edge[C], \
                         vtx_hash, tri_list, data_values ) ) goto error;

#define POLY4(A,B,C,D)  if(!make_iso_poly_4((int*)cube, \
                         &cube_edge[A], &cube_edge[B], &cube_edge[C], \
                         &cube_edge[D], \
                         vtx_hash, tri_list, data_values ) ) goto error;

#define POLY5(A,B,C,D,E)  if(!make_iso_poly_5((int*)cube, \
                         &cube_edge[A], &cube_edge[B], &cube_edge[C], \
                         &cube_edge[D], &cube_edge[E], \
                         vtx_hash, tri_list, data_values ) ) goto error;

#define POLY6(A,B,C,D,E,F)  if(!make_iso_poly_6((int*)cube, \
                         &cube_edge[A], &cube_edge[B], &cube_edge[C], \
                         &cube_edge[D], &cube_edge[E], &cube_edge[F], \
                         vtx_hash, tri_list, data_values ) ) goto error;

#define POLY7(A,B,C,D,E,F,G)  if(!make_iso_poly_7((int*)cube, \
                         &cube_edge[A], &cube_edge[B], &cube_edge[C], \
                         &cube_edge[D], &cube_edge[E], &cube_edge[F], \
                         &cube_edge[G], \
                         vtx_hash, tri_list, data_values ) ) goto error;

        case   0:                                   break;
        case   1: POLY3 ( 0, 2, 8 );                break;
        case   2: POLY3 ( 0, 9, 1 );                break;
        case   3: POLY4 ( 1, 2, 8, 9 );             break;
        case   4: POLY3 ( 2, 3, 10 );               break;
        case   5: POLY4 ( 0, 3, 10, 8 );            break;
        case   6: POLY3 ( 0, 9, 1 );  
                  POLY3 ( 2, 3, 10 );               break;
        case   7: POLY5 ( 1, 3, 10, 8, 9 );         break;
        case   8: POLY3 ( 1, 11, 3 );               break;
        case   9: POLY3 ( 1, 11, 3 );  
                  POLY3 ( 0, 2, 8 );                break;
        case  10: POLY4 ( 0, 9, 11, 3 );            break;
        case  11: POLY5 ( 2, 8, 9, 11, 3 );         break;
        case  12: POLY4 ( 1, 11, 10, 2 );           break;
        case  13: POLY5 ( 0, 1, 11, 10, 8 );        break;
        case  14: POLY5 ( 0, 9, 11, 10, 2 );        break;
        case  15: POLY4 ( 8, 9, 11, 10 );           break;
        case  16: POLY3 ( 4, 8, 6 );                break;
        case  17: POLY4 ( 0, 2, 6, 4 );             break;
        case  18: POLY3 ( 4, 8, 6 );  
                  POLY3 ( 0, 9, 1 );                break;
        case  19: POLY5 ( 1, 2, 6, 4, 9 );          break;
        case  20: POLY3 ( 4, 8, 6 );  
                  POLY3 ( 2, 3, 10 );               break;
        case  21: POLY5 ( 0, 3, 10, 6, 4 );         break;
        case  22: POLY3 ( 4, 8, 6 );  
                  POLY3 ( 2, 3, 10 );  
                  POLY3 ( 0, 9, 1 );                break;
        case  23: POLY6 ( 1, 3, 10, 6, 4, 9 );      break;
        case  24: POLY3 ( 4, 8, 6 );  
                  POLY3 ( 1, 11, 3 );               break;
        case  25: POLY4 ( 0, 2, 6, 4 );  
                  POLY3 ( 1, 11, 3 );               break;
        case  26: POLY3 ( 4, 8, 6 );  
                  POLY4 ( 0, 9, 11, 3 );            break;
        case  27: POLY6 ( 2, 6, 4, 9, 11, 3 );      break;
        case  28: POLY3 ( 4, 8, 6 );  
                  POLY4 ( 1, 11, 10, 2 );           break;
        case  29: POLY6 ( 0, 1, 11, 10, 6, 4 );     break;
        case  30: POLY3 ( 4, 8, 6 );  
                  POLY5 ( 0, 9, 11, 10, 2 );        break;
        case  31: POLY5 ( 4, 9, 11, 10, 6 );        break;
        case  32: POLY3 ( 4, 5, 9 );                break;
        case  33: POLY3 ( 4, 5, 9 );  
                  POLY3 ( 0, 2, 8 );                break;
        case  34: POLY4 ( 0, 4, 5, 1 );             break;
        case  35: POLY5 ( 1, 2, 8, 4, 5 );          break;
        case  36: POLY3 ( 2, 3, 10 );  
                  POLY3 ( 4, 5, 9 );                break;
        case  37: POLY4 ( 0, 3, 10, 8 );  
                  POLY3 ( 4, 5, 9 );                break;
        case  38: POLY4 ( 0, 4, 5, 1 );  
                  POLY3 ( 2, 3, 10 );               break;
        case  39: POLY6 ( 1, 3, 10, 8, 4, 5 );      break;
        case  40: POLY3 ( 4, 5, 9 );  
                  POLY3 ( 1, 11, 3 );               break;
        case  41: POLY3 ( 4, 5, 9 );  
                  POLY3 ( 1, 11, 3 );  
                  POLY3 ( 0, 2, 8 );                break;
        case  42: POLY5 ( 0, 4, 5, 11, 3 );         break;
        case  43: POLY6 ( 2, 8, 4, 5, 11, 3 );      break;
        case  44: POLY3 ( 4, 5, 9 );  
                  POLY4 ( 1, 11, 10, 2 );           break;
        case  45: POLY3 ( 4, 5, 9 );  
                  POLY5 ( 0, 1, 11, 10, 8 );        break;
        case  46: POLY6 ( 0, 4, 5, 11, 10, 2 );     break;
        case  47: POLY5 ( 4, 5, 11, 10, 8 );        break;
        case  48: POLY4 ( 5, 9, 8, 6 );             break;
        case  49: POLY5 ( 0, 2, 6, 5, 9 );          break;
        case  50: POLY5 ( 0, 8, 6, 5, 1 );          break;
        case  51: POLY4 ( 1, 2, 6, 5 );             break;
        case  52: POLY4 ( 5, 9, 8, 6 );  
                  POLY3 ( 2, 3, 10 );               break;
        case  53: POLY6 ( 0, 3, 10, 6, 5, 9 );      break;
        case  54: POLY3 ( 2, 3, 10 );  
                  POLY5 ( 0, 8, 6, 5, 1 );          break;
        case  55: POLY5 ( 1, 3, 10, 6, 5 );         break;
        case  56: POLY3 ( 1, 11, 3 );  
                  POLY4 ( 5, 9, 8, 6 );             break;
        case  57: POLY3 ( 1, 11, 3 );  
                  POLY5 ( 0, 2, 6, 5, 9 );          break;
        case  58: POLY6 ( 0, 8, 6, 5, 11, 3 );      break;
        case  59: POLY5 ( 2, 6, 5, 11, 3 );         break;
        case  60: POLY4 ( 5, 9, 8, 6 );  
                  POLY4 ( 1, 11, 10, 2 );           break;
        case  61: POLY7 ( 0, 1, 11, 10, 6, 5, 9 );  break;
        case  62: POLY7 ( 0, 8, 6, 5, 11, 10, 2 );  break;
        case  63: POLY4 ( 5, 11, 10, 6 );           break;
        case  64: POLY3 ( 6, 10, 7 );               break;
        case  65: POLY3 ( 6, 10, 7 );  
                  POLY3 ( 0, 2, 8 );                break;
        case  66: POLY3 ( 6, 10, 7 );  
                  POLY3 ( 0, 9, 1 );                break;
        case  67: POLY3 ( 6, 10, 7 );  
                  POLY4 ( 1, 2, 8, 9 );             break;
        case  68: POLY4 ( 2, 3, 7, 6 );             break;
        case  69: POLY5 ( 0, 3, 7, 6, 8 );          break;
        case  70: POLY3 ( 0, 9, 1 );  
                  POLY4 ( 2, 3, 7, 6 );             break;
        case  71: POLY6 ( 1, 3, 7, 6, 8, 9 );       break;
        case  72: POLY3 ( 6, 10, 7 );  
                  POLY3 ( 1, 11, 3 );               break;
        case  73: POLY3 ( 6, 10, 7 );  
                  POLY3 ( 1, 11, 3 );  
                  POLY3 ( 0, 2, 8 );                break;
        case  74: POLY3 ( 6, 10, 7 );  
                  POLY4 ( 0, 9, 11, 3 );            break;
        case  75: POLY3 ( 6, 10, 7 );  
                  POLY5 ( 2, 8, 9, 11, 3 );         break;
        case  76: POLY5 ( 1, 11, 7, 6, 2 );         break;
        case  77: POLY6 ( 0, 1, 11, 7, 6, 8 );      break;
        case  78: POLY6 ( 0, 9, 11, 7, 6, 2 );      break;
        case  79: POLY5 ( 7, 6, 8, 9, 11 );         break;
        case  80: POLY4 ( 4, 8, 10, 7 );            break;
        case  81: POLY5 ( 0, 2, 10, 7, 4 );         break;
        case  82: POLY4 ( 4, 8, 10, 7 )
                  POLY3 ( 0, 9, 1 );                break;
        case  83: POLY6 ( 1, 2, 10, 7, 4, 9 );      break;
        case  84: POLY5 ( 2, 3, 7, 4, 8 );          break;
        case  85: POLY4 ( 0, 3, 7, 4 );             break;
        case  86: POLY5 ( 2, 3, 7, 4, 8 );  
                  POLY3 ( 0, 9, 1 );                break;
        case  87: POLY5 ( 1, 3, 7, 4, 9 );          break;
        case  88: POLY4 ( 4, 8, 10, 7 );  
                  POLY3 ( 1, 11, 3 );               break;
        case  89: POLY5 ( 0, 2, 10, 7, 4 );  
                  POLY3 ( 1, 11, 3 );               break;
        case  90: POLY4 ( 4, 8, 10, 7 );  
                  POLY4 ( 0, 9, 11, 3 );            break;
        case  91: POLY7 ( 2, 10, 7, 4, 9, 11, 3 );  break;
        case  92: POLY6 ( 1, 11, 7, 4, 8, 2 );      break;
        case  93: POLY5 ( 0, 1, 11, 7, 4 );         break;
        case  94: POLY7 ( 2, 0, 9, 11, 7, 4, 8 );   break;
        case  95: POLY4 ( 4, 9, 11, 7 );            break;
        case  96: POLY3 ( 6, 10, 7 );  
                  POLY3 ( 4, 5, 9 );                break;
        case  97: POLY3 ( 6, 10, 7 );  
                  POLY3 ( 4, 5, 9 );  
                  POLY3 ( 0, 2, 8 );                break;
        case  98: POLY3 ( 6, 10, 7 );  
                  POLY4 ( 0, 4, 5, 1 );             break;
        case  99: POLY3 ( 6, 10, 7 );  
                  POLY5 ( 1, 2, 8, 4, 5 );          break;
        case 100: POLY4 ( 2, 3, 7, 6 );  
                  POLY3 ( 4, 5, 9 );                break;
        case 101: POLY5 ( 0, 3, 7, 6, 8 );  
                  POLY3 ( 4, 5, 9 );                break;
        case 102: POLY4 ( 0, 4, 5, 1 );  
                  POLY4 ( 2, 3, 7, 6 );             break;
        case 103: POLY7 ( 8, 4, 5, 1, 3, 7, 6 );    break;
        case 104: POLY3 ( 6, 10, 7 );  
                  POLY3 ( 4, 5, 9 ); 
                  POLY3 ( 1, 11, 3 );               break;
        case 105: POLY3 ( 6, 10, 7 );  
                  POLY3 ( 4, 5, 9 );  
                  POLY3 ( 1, 11, 3 );  
                  POLY3 ( 0, 2, 8 );                break;
        case 106: POLY3 ( 6, 10, 7 );  
                  POLY5 ( 0, 4, 5, 11, 3 );         break;
        case 107: POLY3 ( 6, 10, 7 );  
                  POLY6 ( 2, 8, 4, 5, 11, 3 );      break;
        case 108: POLY3 ( 4, 5, 9 );  
                  POLY5 ( 1, 11, 7, 6, 2 );         break;
        case 109: POLY3 ( 4, 5, 9 );  
                  POLY6 ( 0, 1, 11, 7, 6, 8 );      break;
        case 110: POLY7 ( 11, 7, 6, 2, 0, 4, 5 );   break;
        case 111: POLY6 ( 11, 7, 6, 8, 4, 5 );      break;
        case 112: POLY5 ( 5, 9, 8, 10, 7 );         break;
        case 113: POLY6 ( 0, 2, 10, 7, 5, 9 );      break;
        case 114: POLY6 ( 0, 8, 10, 7, 5, 1 );      break;
        case 115: POLY5 ( 1, 2, 10, 7, 5 );         break;
        case 116: POLY6 ( 2, 3, 7, 5, 9, 8 );       break;
        case 117: POLY5 ( 0, 3, 7, 5, 9 );          break;
        case 118: POLY7 ( 8, 2, 3, 7, 5, 1, 0 );    break;
        case 119: POLY4 ( 1, 3, 7, 5 );             break;
        case 120: POLY3 ( 1, 11, 3 );  
                  POLY5 ( 5, 9, 8, 10, 7 );         break;
        case 121: POLY3 ( 1, 11, 3 );  
                  POLY6 ( 0, 2, 10, 7, 5, 9 );      break;
        case 122: POLY7 ( 5, 11, 3, 0, 8, 10, 7 );  break;
        case 123: POLY6 ( 2, 10, 7, 5, 11, 3 );     break;
        case 124: POLY7 ( 7, 5, 9, 8, 2, 1, 11 );   break;
        case 125: POLY6 ( 0, 1, 11, 7, 5, 9 );      break;
        case 126: POLY3 ( 5, 11, 7 );  
                  POLY3 ( 0, 8, 2 );                break;
        case 127: POLY3 ( 5, 11, 7 );               break;
        case 128: POLY3 ( 5, 7, 11 );               break;
        case 129: POLY3 ( 5, 7, 11 );  
                  POLY3 ( 0, 2, 8 );                break;
        case 130: POLY3 ( 5, 7, 11 );  
                  POLY3 ( 0, 9, 1 );                break;
        case 131: POLY3 ( 5, 7, 11 );  
                  POLY4 ( 1, 2, 8, 9 );             break;
        case 132: POLY3 ( 5, 7, 11 );  
                  POLY3 ( 2, 3, 10 );               break;
        case 133: POLY3 ( 5, 7, 11 );  
                  POLY4 ( 0, 3, 10, 8 );            break;
        case 134: POLY3 ( 5, 7, 11 );  
                  POLY3 ( 0, 9, 1 );  
                  POLY3 ( 2, 3, 10 );               break;
        case 135: POLY3 ( 5, 7, 11 );  
                  POLY5 ( 1, 3, 10, 8, 9 );         break;
        case 136: POLY4 ( 1, 5, 7, 3 );             break;
        case 137: POLY3 ( 0, 2, 8 );  
                  POLY4 ( 1, 5, 7, 3 );             break;
        case 138: POLY5 ( 0, 9, 5, 7, 3 );          break;
        case 139: POLY6 ( 2, 8, 9, 5, 7, 3 );       break;
        case 140: POLY5 ( 1, 5, 7, 10, 2 );         break;
        case 141: POLY6 ( 0, 1, 5, 7, 10, 8 );      break;
        case 142: POLY6 ( 0, 9, 5, 7, 10, 2 );      break;
        case 143: POLY5 ( 5, 7, 10, 8, 9 );         break;
        case 144: POLY3 ( 5, 7, 11 );  
                  POLY3 ( 4, 8, 6 );                break;
        case 145: POLY3 ( 5, 7, 11 );  
                  POLY4 ( 0, 2, 6, 4 );             break;
        case 146: POLY3 ( 5, 7, 11 );  
                  POLY3 ( 4, 8, 6 );  
                  POLY3 ( 0, 9, 1 );                break;
        case 147: POLY3 ( 5, 7, 11 );  
                  POLY5 ( 1, 2, 6, 4, 9 );          break;
        case 148: POLY3 ( 5, 7, 11 );  
                  POLY3 ( 4, 8, 6 );  
                  POLY3 ( 2, 3, 10 );               break;
        case 149: POLY3 ( 5, 7, 11 );  
                  POLY5 ( 0, 3, 10, 6, 4 );         break;
        case 150: POLY3 ( 5, 7, 11 );  
                  POLY3 ( 4, 8, 6 );  
                  POLY3 ( 2, 3, 10 );  
                  POLY3 ( 0, 9, 1 );                break;
        case 151: POLY3 ( 5, 7, 11 );  
                  POLY6 ( 1, 3, 10, 6, 4, 9 );      break;
        case 152: POLY3 ( 4, 8, 6 );  
                  POLY4 ( 1, 5, 7, 3 );             break;
        case 153: POLY4 ( 0, 2, 6, 4 );  
                  POLY4 ( 1, 5, 7, 3 );             break;
        case 154: POLY3 ( 4, 8, 6 );  
                  POLY5 ( 0, 9, 5, 7, 3 );          break;
        case 155: POLY7 ( 9, 5, 7, 3, 2, 6, 4 );    break;
        case 156: POLY3 ( 4, 8, 6 );  
                  POLY5 ( 1, 5, 7, 10, 2 );         break;
        case 157: POLY7 ( 10, 6, 4, 0, 1, 5, 7 );   break;
        case 158: POLY3 ( 4, 8, 6 );  
                  POLY6 ( 0, 9, 5, 7, 10, 2 );      break;
        case 159: POLY6 ( 9, 5, 7, 10, 6, 4 );      break;
        case 160: POLY4 ( 4, 7, 11, 9 );            break;
        case 161: POLY4 ( 4, 7, 11, 9 );  
                  POLY3 ( 0, 2, 8 );                break;
        case 162: POLY5 ( 0, 4, 7, 11, 1 );         break;
        case 163: POLY6 ( 1, 2, 8, 4, 7, 11 );      break;
        case 164: POLY4 ( 4, 7, 11, 9 );  
                  POLY3 ( 2, 3, 10 );               break;
        case 165: POLY4 ( 4, 7, 11, 9 );  
                  POLY4 ( 0, 3, 10, 8 );            break;
        case 166: POLY3 ( 2, 3, 10 );  
                  POLY5 ( 0, 4, 7, 11, 1 );         break;
        case 167: POLY7 ( 1, 3, 10, 8, 4, 7, 11 );  break;
        case 168: POLY5 ( 1, 9, 4, 7, 3 );          break;
        case 169: POLY5 ( 1, 9, 4, 7, 3 )
                  POLY3 ( 0, 2, 8 );                break;
        case 170: POLY4 ( 0, 4, 7, 3 );             break;
        case 171: POLY5 ( 2, 8, 4, 7, 3 );          break;
        case 172: POLY6 ( 1, 9, 4, 7, 10, 2 );      break;
        case 173: POLY7 ( 1, 9, 4, 7, 10, 8, 0 );   break;
        case 174: POLY5 ( 0, 4, 7, 10, 2 );         break;
        case 175: POLY4 ( 4, 7, 10, 8 );            break;
        case 176: POLY5 ( 6, 7, 11, 9, 8 );         break;
        case 177: POLY6 ( 0, 2, 6, 7, 11, 9 );      break;
        case 178: POLY6 ( 0, 8, 6, 7, 11, 1 );      break;
        case 179: POLY5 ( 1, 2, 6, 7, 11 );         break;
        case 180: POLY3 ( 2, 3, 10 );  
                  POLY5 ( 6, 7, 11, 9, 8 );         break;
        case 181: POLY7 ( 6, 7, 11, 9, 0, 3, 10 );  break;
        case 182: POLY3 ( 2, 3, 10 );  
                  POLY6 ( 0, 8, 6, 7, 11, 1 );      break;
        case 183: POLY6 ( 1, 3, 10, 6, 7, 11 );     break;
        case 184: POLY6 ( 1, 9, 8, 6, 7, 3 );       break;
        case 185: POLY7 ( 9, 0, 2, 6, 7, 3, 1 );    break;
        case 186: POLY5 ( 0, 8, 6, 7, 3 );          break;
        case 187: POLY4 ( 2, 6, 7, 3 );             break;
        case 188: POLY7 ( 7, 10, 2, 1, 9, 8, 6 );   break;
        case 189: POLY3 ( 6, 7, 10 );  
                  POLY3 ( 0, 1, 9 );                break;
        case 190: POLY6 ( 0, 8, 6, 7, 10, 2 );      break;
        case 191: POLY3 ( 6, 7, 10 );               break;
        case 192: POLY4 ( 5, 6, 10, 11 );           break;
        case 193: POLY4 ( 5, 6, 10, 11 );  
                  POLY3 ( 0, 2, 8 );                break;
        case 194: POLY4 ( 5, 6, 10, 11 );  
                  POLY3 ( 0, 9, 1 );                break;
        case 195: POLY4 ( 5, 6, 10, 11 );  
                  POLY4 ( 1, 2, 8, 9 );             break;
        case 196: POLY5 ( 2, 3, 11, 5, 6 );         break;
        case 197: POLY6 ( 0, 3, 11, 5, 6, 8 );      break;
        case 198: POLY3 ( 0, 9, 1 );  
                  POLY5 ( 2, 3, 11, 5, 6 );         break;
        case 199: POLY7 ( 3, 11, 5, 6, 8, 9, 1 );   break;
        case 200: POLY5 ( 1, 5, 6, 10, 3 );         break;
        case 201: POLY5 ( 0, 1, 5, 6, 8 );  
                  POLY3 ( 2, 10, 3 );               break;
        case 202: POLY6 ( 0, 9, 5, 6, 10, 3 );      break;
        case 203: POLY7 ( 3, 2, 8, 9, 5, 6, 10 );   break;
        case 204: POLY4 ( 1, 5, 6, 2 );             break;
        case 205: POLY5 ( 0, 1, 5, 6, 8 );          break;
        case 206: POLY5 ( 0, 9, 5, 6, 2 );          break;
        case 207: POLY4 ( 5, 6, 8, 9 );             break;
        case 208: POLY5 ( 4, 8, 10, 11, 5 );        break;
        case 209: POLY6 ( 0, 2, 10, 11, 5, 4 );     break;
        case 210: POLY3 ( 0, 9, 1 );  
                  POLY5 ( 4, 8, 10, 11, 5 );        break;
        case 211: POLY7 ( 4, 9, 1, 2, 10, 11, 5 );  break;
        case 212: POLY6 ( 2, 3, 11, 5, 4, 8 );      break;
        case 213: POLY5 ( 0, 3, 11, 5, 4 );         break;
        case 214: POLY3 ( 0, 9, 1 );  
                  POLY6 ( 2, 3, 11, 5, 4, 8 );      break;
        case 215: POLY6 ( 3, 11, 5, 4, 9, 1 );      break;
        case 216: POLY6 ( 1, 5, 4, 8, 10, 3 );      break;
        case 217: POLY7 ( 10, 3, 1, 5, 4, 0, 2 );   break;
        case 218: POLY7 ( 5, 4, 8, 10, 3, 0, 9 );   break;
        case 219: POLY3 ( 4, 9, 5 );  
                  POLY3 ( 2, 10, 3 );               break;
        case 220: POLY5 ( 1, 5, 4, 8, 2 );          break;
        case 221: POLY4 ( 0, 1, 5, 4 );             break;
        case 222: POLY6 ( 5, 4, 8, 2, 0, 9 );       break;
        case 223: POLY3 ( 4, 9, 5 );                break;
        case 224: POLY5 ( 4, 6, 10, 11, 9 );        break;
        case 225: POLY3 ( 0, 2, 8 );  
                  POLY5 ( 4, 6, 10, 11, 9 );        break;
        case 226: POLY6 ( 0, 4, 6, 10, 11, 1 );     break;
        case 227: POLY7 ( 4, 6, 10, 11, 1, 2, 8 );  break;
        case 228: POLY6 ( 2, 3, 11, 9, 4, 6 );      break;
        case 229: POLY7 ( 6, 8, 0, 3, 11, 9, 4 );   break;
        case 230: POLY7 ( 11, 1, 0, 4, 6, 2, 3 );   break;
        case 231: POLY3 ( 1, 3, 11 );  
                  POLY3 ( 4, 6, 8 );                break;
        case 232: POLY6 ( 1, 9, 4, 6, 10, 3 );      break;
        case 233: POLY3 ( 0, 2, 8 );  
                  POLY6 ( 1, 9, 4, 6, 10, 3 );      break;
        case 234: POLY5 ( 0, 4, 6, 10, 3 );         break;
        case 235: POLY6 ( 4, 6, 10, 3, 2, 8 );      break;
        case 236: POLY5 ( 1, 9, 4, 6, 2 );          break;
        case 237: POLY6 ( 1, 9, 4, 6, 8, 0 );       break;
        case 238: POLY4 ( 0, 4, 6, 2 );             break;
        case 239: POLY3 ( 4, 6, 8 );                break;
        case 240: POLY4 ( 8, 10, 11, 9 );           break;
        case 241: POLY5 ( 0, 2, 10, 11, 9 );        break;
        case 242: POLY5 ( 0, 8, 10, 11, 1 );        break;
        case 243: POLY4 ( 1, 2, 10, 11 );           break;
        case 244: POLY5 ( 2, 3, 11, 9, 8 );         break;
        case 245: POLY4 ( 0, 3, 11, 9 );            break;
        case 246: POLY6 ( 8, 2, 3, 11, 1, 0 );      break;
        case 247: POLY3 ( 1, 3, 11 );               break;
        case 248: POLY5 ( 1, 9, 8, 10, 3 );         break;
        case 249: POLY6 ( 10, 3, 1, 9, 0, 2 );      break;
        case 250: POLY4 ( 0, 8, 10, 3 );            break;
        case 251: POLY3 ( 2, 10, 3 );               break;
        case 252: POLY4 ( 1, 9, 8, 2 );             break;
        case 253: POLY3 ( 0, 1, 9 );                break;
        case 254: POLY3 ( 0, 8, 2 );                break;
        case 255:                                   break;

        default: break;

#undef  POLY3
#undef  POLY4
#undef  POLY5
#undef  POLY6
#undef  POLY7

    }

    return OK;

    error:
        ERROR_SECTION;
        return ERROR;
}



static
Error iso_convert_prism
          ( Prism           *prism,
            float           data_values[6],
            hash_table_ptr  vtx_hash,
            SegList         *tri_list,
            int             equivalence )
{
    int  signature = 0;

    DXErrorGoto2
        ( ERROR_NOT_IMPLEMENTED,
          "#11380", /* %s is an invalid connections type */
          "\"prism\"" );

    if ( equivalence == 1 )
    {
        if ( data_values[0] > 0.0 ) signature += 1;
        if ( data_values[1] > 0.0 ) signature += 2;
        if ( data_values[2] > 0.0 ) signature += 4;
        if ( data_values[3] > 0.0 ) signature += 8;
        if ( data_values[4] > 0.0 ) signature += 16;
        if ( data_values[5] > 0.0 ) signature += 32;
    }
    else
    {
        if ( data_values[0] == 0.0 ) signature += 1;
        if ( data_values[1] == 0.0 ) signature += 2;
        if ( data_values[2] == 0.0 ) signature += 4;
        if ( data_values[3] == 0.0 ) signature += 8;
        if ( data_values[4] == 0.0 ) signature += 16;
        if ( data_values[5] == 0.0 ) signature += 32;
    }

#ifdef WHITEBOX_TEST
    DXDebug ( "I", "Signature = %d", signature );
    histograms.case_signatures[signature]++;
#endif

    switch ( signature )
    {
        /* 
         * This table was entered symbolically into a file called table.DB
         * and then converted to 'C' code in a file 'table.C' by a script
         * named 'table.MAK'.  
         * All files currently reside in the directory ~allain/kalvin.
         */

        default: break;

#undef  LINE

    }

    return OK;

    error:
        ERROR_SECTION;
        return ERROR;
}





static
Field create_iso_points
          ( field_info     input_info, 
            float          *value_ptr,
            int            is_grown,
            Error (*convert_connection)
                        (void *, float *, hash_table_ptr, hash_table_ptr, int),
                     /* (void *, float *, hash_table_ptr, SegList *,    int),*/
            Field out )

{
    float           value = value_ptr [ 0 ];
    array_info      pi_info, ci_info, di_info  = NULL;
    array_info      po_info = NULL;
    hash_table_rec  vtx_hash;
    Line            line;
    float           data[2];
    double          t;
    iso_edge_hash_ptr   element_ptr = NULL;
    int             i, j;
    field_info      output_info = NULL;
    int             position_dim;
    int             connection_shape;
    int             XXX;

    array_info      a_info;
    component_info  comp;
    Array           array    = NULL;
    Pointer         scratch  = NULL;
    Pointer         out_data = NULL;

    InvalidComponentHandle  i_handle = NULL;

    DXASSERTGOTO ( ERROR != input_info );
    DXASSERTGOTO ( ERROR != value_ptr );
    DXASSERTGOTO ( ERROR != out );

    vtx_hash.count        = 0;
    vtx_hash.table        = NULL;
    vtx_hash.set_vertices = NULL;

    pi_info = (array_info) &(input_info->std_comps[(int)POSITIONS]->array);
    ci_info = (array_info) &(input_info->std_comps[(int)CONNECTIONS]->array);
    di_info = (array_info) &(input_info->std_comps[(int)DATA]->array);

    if ( ( NULL  != input_info->std_comps[(int)INVALID_CONNECTIONS] ) &&
         ( ERROR == ( i_handle = DXCreateInvalidComponentHandle
                                     ( (Object)input_info->field,
                                       NULL, "connections" ) ) ) )
        goto error;

    DXASSERTGOTO ( pi_info->get_item != ERROR );
    DXASSERTGOTO ( ci_info->get_item != ERROR );
    DXASSERTGOTO ( di_info->get_item != ERROR );

    if ( ( pi_info->shape[0] != 1 ) &&
         ( pi_info->shape[0] != 2 ) &&
         ( pi_info->shape[0] != 3 ) )
        DXErrorGoto3
            ( ERROR_DATA_INVALID,
              "#11400", /* %s has invalid dimensionality; does not match %s */
              "\"positions\" component", "\"connections\"" );

    position_dim     = pi_info->shape[0];
    connection_shape = ci_info->shape[0];

    DXASSERTGOTO ( connection_shape == 2 );
    DXASSERTGOTO ( ci_info->items >= ci_info->original_items );

    TST_HASH_INTS;

    if ( ( ERROR == ( vtx_hash.table
             = DXCreateHash
                 ( sizeof(iso_edge_hash_rec),
                   iso_edge_hash_func, iso_edge_compare_func ) ) ) )
        goto error;

    if ( ci_info->get_item == _dxf_get_conn_grid_LINES )
    {
        array_info t0;
        int        II;
        int        I0;

        t0 = (array_info)
         &(((struct _array_allocator *)((mesh_array_info)ci_info)->terms)[0]);

        DXASSERTGOTO ( ((grid_connections_info)ci_info)->counts != NULL );
        DXASSERTGOTO
          ( t0->items == ( ((grid_connections_info)ci_info)->counts[0] - 1 ) );

        if ( ((mesh_array_info)ci_info)->grow_offsets != NULL )
        {
            DXASSERTGOTO ( is_grown );

            I0 = ((mesh_array_info)ci_info)->grow_offsets[0];
        }
        else
            I0 = 0;

        if ( !ci_info->get_item ( I0, ci_info, &line )
             ||
             !di_info->get_item ( line.p, di_info, &data[0] ) )
            goto error;

        data[0] -= value;

        for ( II=I0;
              II<( I0 + t0->original_items );
              II++,
                  line.p++, line.q++, data[0] = data[1] )
            {
                if ( !di_info->get_item ( line.q, di_info, &data[1] ) )
                    goto error;

                data[1] -= value;

                /* iso_convert_line */
               if ( ( NULL != i_handle ) && DXIsElementInvalid (i_handle, II) )
                    continue;
                else if ( ( ( data[0] <= 0.0 ) && ( data[1] > 0.0 ) )
                          ||
                          ( ( data[0] > 0.0 ) && ( data[1] <= 0.0 ) ) )
                    /* XXX indirect with dummy line */
                    if ( !iso_hash_enter ( &line, &vtx_hash, &XXX ) )
                        goto error;
            }
    }
    else
    {
        for ( i=0;
              i<ci_info->original_items;
              i++ )
            if ( ( NULL != i_handle ) && DXIsElementInvalid ( i_handle, i ) )
                continue;
            else if ( !ci_info->get_item ( i, ci_info, &line ) )
                goto error;
            else
            {
                if ( !di_info->get_item ( line.p, di_info, &data[0] ) ||
                     !di_info->get_item ( line.q, di_info, &data[1] )   )
                    goto error;

                data[0] -= value;
                data[1] -= value;

                /* iso_convert_line */
                if ( ( ( data[0] <= 0.0 ) && ( data[1]  > 0.0 ) )
                     ||
                     ( ( data[0]  > 0.0 ) && ( data[1] <= 0.0 ) ) )
                    /* XXX indirect with dummy line */
                    if ( !iso_hash_enter ( &line, &vtx_hash, &XXX ) )
                        goto error;
            }
    }

    if ( !create_output_arrays
              ( out,
                vtx_hash.count,     position_dim,
                0,                  0,
                0,
                DEFAULT_LINE_COLOR, 
                value,              DEP_POSITIONS,
                ISOSURFACE ) )
        goto error;

    if ( vtx_hash.count == 0 ) 
        goto done;

    if ( ( ERROR == ( output_info = ( _dxf_InMemory ( out ) ) ) )
         ||
         !DXInitGetNextHashElement ( vtx_hash.table ) )
        goto error;

    DXASSERTGOTO ( output_info->std_comps[(int)POSITIONS] != NULL );

    po_info = (array_info) &(output_info->std_comps[(int)POSITIONS]->array);


    for ( j=0, comp=input_info->comp_list;
          j<input_info->comp_count;
          j++, comp++ )
    {

        if ( ( 0 != strcmp ( comp->name, "positions" ) ) &&
             ( ( NULL  == comp->std_attribs[(int)DEP]        ) ||
               ( ERROR == comp->std_attribs[(int)DEP]->value ) ||
               ( 0 != strcmp ( comp->std_attribs[(int)DEP]->value,
                               "positions" ) ) ) )
            continue;

        if ( ( 0 == strcmp  ( comp->name, "data"        ) ) ||
             ( 0 == strncmp ( comp->name, "original", 8 ) ) ||
             ( NULL != comp->std_attribs[(int)REF]        )   )
            continue;

        a_info = (array_info) &(comp->array);

    if ( CLASS_CONSTANTARRAY == a_info->class )
    {
        if ( ( ERROR == ( array = (Array)DXNewConstantArrayV
                                     ( vtx_hash.count,
                                       DXGetConstantArrayData ( a_info->array ),
                                       a_info->type,
                                       a_info->category,
                                       a_info->rank,
                                       a_info->shape ) ) )
             ||
             !DXSetComponentValue ( out, comp->name, (Object) array )
             ||
             !DXSetComponentAttribute
                  ( out, comp->name, "dep",
                    (Object) DXNewString ( "positions" ) ) )

            goto error;

        continue;
    }

        if ( ( ERROR == ( array
                   = DXNewArrayV
                         ( a_info->type, a_info->category,
                           a_info->rank, &a_info->shape[0] ) ) )
             ||
             !DXAllocateArray     ( array, vtx_hash.count )           ||
             !DXAddArrayData      ( array, 0, vtx_hash.count, NULL )  ||
             !DXTrim              ( array )                           ||
             !DXSetComponentValue ( out, comp->name, (Object) array )
             ||
             !DXSetComponentAttribute
                  ( out, comp->name, "dep",
                    (Object) DXNewString ( "positions" ) )
             ||
             !_dxf_SetIterator    ( comp )
             ||
             ( ERROR == ( out_data = DXGetArrayData  ( array )))             ||
             ( ERROR == ( scratch  = DXAllocateLocal ( 2 * a_info->itemsize )))
             ||
             !DXInitGetNextHashElement ( vtx_hash.table ) )
            goto error;

        array = NULL;

        for ( i = 0;
              ( NULL != ( element_ptr
                              = (iso_edge_hash_ptr) DXGetNextHashElement
                                                    ( vtx_hash.table ) ) );
              i++ )
        {
            DXASSERTGOTO ( element_ptr->seq < po_info->items );

            if ( element_ptr->edge.p == element_ptr->edge.q )
                t = 0.0;
            else
            {
                if ( !di_info->get_item
                          ( element_ptr->edge.p, di_info, &data[0] )||
                     !di_info->get_item
                          ( element_ptr->edge.q, di_info, &data[1] ) )
                    goto error;

                data[0] -= value;
                data[1] -= value;

                if      ( data[0] == 0.0 ) t = 0.0;
                else if ( data[1] == 0.0 ) t = 1.0;
                else
                    t = (double)data[0] / ( (double)data[0] - (double)data[1] );

                DXASSERTGOTO ( ( t >= 0.0 ) && ( t <= 1.0 ) );
            }

            if ( !interp ( a_info, scratch,
                           element_ptr->edge.p, element_ptr->edge.q, t, 
                           &(((char*)out_data)
                               [ a_info->itemsize * element_ptr->seq ]),
			       a_info->itemsize/sizeof(float),
			       a_info->itemsize) )
                goto error;
        }

        DXFree ( scratch );  scratch = NULL;
    }

    done:
        /* This is create_iso_points */
        if ( !DXSetIntegerAttribute ( (Object) out, "shade", 0 ) )
            goto error;

        DXFreeInvalidComponentHandle ( i_handle );

        DXDestroyHash     ( vtx_hash.table );
        DXFree            ( scratch );
        _dxf_FreeInMemory ( output_info );

        i_handle       = NULL;
        vtx_hash.table = NULL;
        vtx_hash.count = 0;
        output_info    = NULL;
        scratch        = NULL;

        return DXEndField ( out );

    error:
        ERROR_SECTION;

        DXFreeInvalidComponentHandle ( i_handle );

        DXDestroyHash     ( vtx_hash.table );
        DXFree            ( scratch );
        _dxf_FreeInMemory ( output_info );

        i_handle       = NULL;
        vtx_hash.table = NULL;
        vtx_hash.count = 0;
        output_info    = NULL;
        scratch        = NULL;

        return ERROR;
}



#ifdef INCLUDES_BAND


static
float *band_value_list
           ( Field    field,
             float    *vp,
             int      *number,
             float    *min_max,
             float    *old_min,
             float    *old_max )
{
    float *value2_ptr = NULL;
    int   i;

    *number += 2;

    if ( ( ERROR == ( value2_ptr
                         = (float *) DXAllocate ( (*number) * sizeof (float) )))
         ||
         ! memcpy ( (char *)(&value2_ptr[1]), (char *)vp,
                    ( ((*number)-2) * sizeof (float) ) ) )
        goto error;

    if ( min_max != NULL )
    {
        value2_ptr[0]           = min_max[0];
        value2_ptr[(*number)-1] = min_max[1];
    }
    else
        if ( !DXStatistics
                  ( (Object)field, "data",
                    &value2_ptr[0], &value2_ptr[(*number)-1], NULL, NULL ) )
            goto error;

    /* If values in the input list are outside min,max: prune them off */

    while ( ( value2_ptr[1] < value2_ptr[0] ) && ( (*number) > 2 ) )
    {
        for ( i=2; i<(*number); i++ ) value2_ptr[i-1] = value2_ptr[i];
        (*number)--;
    }

    while ( ( value2_ptr[(*number)-2] > value2_ptr[(*number)-1] )
            &&
            ( (*number) > 2 ) )
    {
        value2_ptr[(*number)-2] = value2_ptr[(*number)-1];
        (*number)--;
    }

    DXASSERTGOTO ( value2_ptr[0] <= value2_ptr[(*number)-1] );

    *old_min = value2_ptr [ 0 ];
    *old_max = value2_ptr [ (*number) - 1 ];

#if 0
    /* The following "fudge" can result in band values slightly outside the    */
    /* range of the input data.  For example: if you use AutoColor prior to    */
    /* band and then use the calculated colormap to color the band output, the */
    /* colormap may not encompass the data.  Thus I'm removing it now and will */
    /* check if it introduces problems.  suits 9/5/96                          */

    /* Ensure floating point inequality with min and max */

    value2_ptr[0]
        = ( value2_ptr[0] == 0 )? -DXD_MIN_FLOAT :
          ( value2_ptr[0]  > 0 )?
              value2_ptr[0] * ( 1.0 - DXD_FLOAT_EPSILON ) :
              value2_ptr[0] * ( 1.0 + DXD_FLOAT_EPSILON );

    value2_ptr[*number-1]
        = ( value2_ptr[(*number)-1] == 0 )? DXD_MIN_FLOAT :
          ( value2_ptr[(*number)-1]  > 0 )?
              value2_ptr[(*number)-1] * ( 1.0 + DXD_FLOAT_EPSILON ) :
              value2_ptr[(*number)-1] * ( 1.0 - DXD_FLOAT_EPSILON );
#endif

    return value2_ptr;

    error:
        ERROR_SECTION;

        DXFree ( (Pointer) value2_ptr );
        return ERROR;
}



static
Field create_band_lines
          ( field_info     input_info, 
            float          *value_ptr,
            int            number,
            band_remap_types remap,
            int            is_grown,
            float          *min_max,
            Error (*convert_connection)
                  (void *,float *,float *,int,hash_table_ptr,SegList *,int),
            Field out )
{
    array_info      pi_info, ci_info, di_info = NULL;
    array_info      po_info, co_info, do_info = NULL;
    hash_table_rec  vtx_hash;
    SegList         *line_list = NULL;
    SegList         *cval_list = NULL;
    Line            line;
    float           data[2];
    double          t;
    band_edge_hash_ptr  element_ptr = NULL;
    int             D;
    int             i, j;
    field_info      output_info = NULL;
    int             position_dim;
    int             connection_shape;
    int             connection_count;
    float           value_low;
    float           old_min, old_max;
    float           *value2_ptr = NULL;

    SegList         *c_ID_list = NULL;
    int             *c_ID_pull = NULL;
    int             n_c_dep, n_p_dep;
    array_info      a_info;
    component_info  comp;
    Array           array    = NULL;
    Pointer         scratch  = NULL;
    Pointer         out_data = NULL;
    int             itemsize;

    InvalidComponentHandle  i_handle = NULL;

    DXASSERTGOTO ( ERROR != convert_connection );
    DXASSERTGOTO ( ERROR != input_info );
    DXASSERTGOTO ( ERROR != value_ptr );
    DXASSERTGOTO ( ERROR != out );

    vtx_hash.count        = 0;
    vtx_hash.table        = NULL;
    vtx_hash.set_vertices = NULL;

    if ( ( NULL  != input_info->std_comps[(int)INVALID_CONNECTIONS] ) &&
         ( ERROR == ( i_handle = DXCreateInvalidComponentHandle
                                     ( (Object)input_info->field,
                                       NULL, "connections" ) ) ) )
        goto error;

    if ( ERROR == ( value2_ptr = band_value_list
                                     ( input_info->field,
                                       value_ptr, &number, min_max,
                                       &old_min, &old_max ) ) )
        goto error;

    pi_info = (array_info) &(input_info->std_comps[(int)POSITIONS]->array);
    ci_info = (array_info) &(input_info->std_comps[(int)CONNECTIONS]->array);
    di_info = (array_info) &(input_info->std_comps[(int)DATA]->array);

    DXASSERTGOTO ( pi_info->get_item != ERROR );
    DXASSERTGOTO ( ci_info->get_item != ERROR );
    DXASSERTGOTO ( di_info->get_item != ERROR );

    if ( ( pi_info->shape[0] != 1 ) &&
         ( pi_info->shape[0] != 2 ) &&
         ( pi_info->shape[0] != 3 ) )
        DXErrorGoto3
            ( ERROR_DATA_INVALID,
              "#11400", /* %s has invalid dimensionality; does not match %s */
              "\"positions\" component", "\"connections\"" );

    position_dim     = pi_info->shape[0];
    connection_shape = ci_info->shape[0];

    DXASSERTGOTO ( connection_shape == 2 );
    DXASSERTGOTO ( ci_info->items >= ci_info->original_items );

    TST_HASH_INTS;

    if ( ( ERROR == ( vtx_hash.table
             = DXCreateHash
                 ( sizeof(band_edge_hash_rec),
                   band_edge_hash_func,
                   band_edge_compare_func ) ) )
         ||
         ( ERROR == ( line_list
                          = DXNewSegList ( sizeof(Line), 1000, 1000 ) ) ) )
        goto error;

    if ( remap != ORIGINAL_DATA )
       if ( ERROR == ( cval_list = DXNewSegList ( sizeof(float), 1000, 1000 )))
            goto error;

    if ( !tally_dependers ( input_info, &n_c_dep, &n_p_dep ) )
        goto error;

    if ( ( n_c_dep > 1 )
         &&
         ( ERROR == ( c_ID_list = DXNewSegList ( sizeof(int), 1000, 1000 ) ) ) )
        goto error;

    if ( ci_info->get_item == _dxf_get_conn_grid_LINES )
    {
        array_info t0;
        int        II;
        int        I0;

        t0 = (array_info)
         &(((struct _array_allocator *)((mesh_array_info)ci_info)->terms)[0]);

        DXASSERTGOTO ( ((grid_connections_info)ci_info)->counts != NULL );
        DXASSERTGOTO
          ( t0->items == ( ((grid_connections_info)ci_info)->counts[0] - 1 ) );

        if ( ((mesh_array_info)ci_info)->grow_offsets != NULL )
        {
            DXASSERTGOTO ( is_grown );

            I0 = ((mesh_array_info)ci_info)->grow_offsets[0];
        }
        else
            I0 = 0;

        if ( !ci_info->get_item ( I0, ci_info, &line )
             ||
             !di_info->get_item ( line.p, di_info, &data[0] ) )
            goto error;

        for ( II=I0;
              II<( I0 + t0->original_items );
              II++,
                  line.p++, line.q++, data[0] = data[1] )
            {
                if ( !di_info->get_item ( line.q, di_info, &data[1] ) )
                    goto error;

               if ( ( NULL != i_handle ) && DXIsElementInvalid (i_handle, II) )
                    continue;
                else
                    for ( D=0; D<number-1; D++ )
                    {
                        if ( !convert_connection 
                                 ( &line, data, value2_ptr, D,
                                   &vtx_hash, line_list, 1 ) )
                            goto error;
                        else if ( ( NULL != c_ID_list )
                                  &&
                                  !set_c_ID
                                       ( c_ID_list,
                                         DXGetSegListItemCount ( line_list ),
                                         II ) )
                            goto error;

                        switch ( remap )
                        {
                            case ORIGINAL_DATA: break;

                            case DATA_LOW:
                                if ( !set_cval
                                         ( cval_list,
                                           DXGetSegListItemCount ( line_list ),
                                           ( ( D == 0 ) ?
                                               old_min : value2_ptr[D] ) ) )
                                    goto error;
                                break;

                            case DATA_HIGH:
                                if ( !set_cval
                                         ( cval_list,
                                           DXGetSegListItemCount ( line_list ),
                                           ( ( (D+1) == number ) ?
                                               old_max : value2_ptr[D+1] ) ) )
                                    goto error;
                                break;
                        }
                    }
            }
    }
    else
    {
        for ( i=0;
              i<ci_info->original_items;
              i++ )
            if ( ( NULL != i_handle ) && DXIsElementInvalid ( i_handle, i ) )
                continue;
            else if ( !ci_info->get_item ( i, ci_info, &line ) )
                goto error;
            else
            {
                if ( !di_info->get_item ( line.p, di_info, &data[0] ) ||
                     !di_info->get_item ( line.q, di_info, &data[1] )   )
                    goto error;

                for ( D=0; D<number-1; D++ )
                {
                    if ( !convert_connection
                             ( &line, data, value2_ptr, D,
                               &vtx_hash, line_list, 1 ) )
                        goto error;
                    else if ( ( NULL != c_ID_list )
                              &&
                              !set_c_ID
                                  ( c_ID_list,
                                    DXGetSegListItemCount ( line_list ),
                                    i ) )
                        goto error;

                    switch ( remap )
                    {
                        case ORIGINAL_DATA: break;

                        case DATA_LOW:
                            if ( !set_cval
                                     ( cval_list,
                                       DXGetSegListItemCount ( line_list ),
                                       ( ( D == 0 ) ?
                                           old_min : value2_ptr[D] ) ) )
                                goto error;
                            break;

                        case DATA_HIGH:
                            if ( !set_cval
                                     ( cval_list,
                                       DXGetSegListItemCount ( line_list ),
                                       ( ( (D+1) == number ) ?
                                           old_max : value2_ptr[D+1] ) ) )
                                goto error;
                            break;
                    }
                }
            }
    }

    connection_count = DXGetSegListItemCount ( line_list );

    if ( connection_count == 0 ) {
        if ( !_dxf_MakeFieldEmpty ( out ) )
            goto error;
        else
            goto done; 
    }

    if ( !create_output_arrays
              ( out,
                vtx_hash.count,    position_dim,
                connection_count,  1,
                0,
                DEFAULT_LINE_COLOR, 
                0.0, ( (remap==ORIGINAL_DATA)? DEP_POSITIONS: DEP_CONNECTIONS ),
                BAND ) )
        goto error;

    if ( vtx_hash.count == 0 ) 
        goto done;

    if ( ( connection_count > 0 ) && ( c_ID_list != NULL ) )
    {
        if ( ( ERROR == ( c_ID_pull = DXAllocateLocal
                                          ( connection_count * sizeof(int) ) ) )
             ||
             !copy_seglist_to_memory
                  ( c_ID_list, c_ID_pull, sizeof(int), connection_count ) )
            goto error;
    
        DXDeleteSegList ( c_ID_list );  c_ID_list = NULL;
    }

    if ( ( ERROR == ( output_info = ( _dxf_InMemory ( out ) ) ) )
         ||
         !DXInitGetNextHashElement ( vtx_hash.table ) )
        goto error;

    DXASSERTGOTO ( output_info->std_comps[(int)POSITIONS] != NULL );

    po_info = (array_info) &(output_info->std_comps[(int)POSITIONS]->array);
    co_info = (array_info) &(output_info->std_comps[(int)CONNECTIONS]->array);
    do_info = (array_info) &(output_info->std_comps[(int)DATA]->array);

    if ( !copy_seglist_to_array ( line_list, co_info ) )
        goto error;

    if ( remap != ORIGINAL_DATA )
    {
        if ( !copy_seglist_to_array ( cval_list, do_info ) )
            goto error;
    }

    DXDeleteSegList ( line_list );  line_list = NULL;
    DXDeleteSegList ( cval_list );  cval_list = NULL;

    for ( j=0, comp=input_info->comp_list;
          j<input_info->comp_count;
          j++, comp++ )
    {

        if ( ( 0 != strcmp ( comp->name, "positions" ) ) &&
             ( ( NULL  == comp->std_attribs[(int)DEP]        ) ||
               ( ERROR == comp->std_attribs[(int)DEP]->value ) ||
               ( 0 != strcmp ( comp->std_attribs[(int)DEP]->value,
                               "positions" ) ) ) )
            continue;

        if ( ( ( 0 == strcmp ( comp->name, "data" ) )
               &&
               ( remap != ORIGINAL_DATA ) )
             ||
             ( 0 == strncmp ( comp->name, "original", 8 ) ) ||
             ( NULL != comp->std_attribs[(int)REF]        )  )
            continue;

        a_info = (array_info) &(comp->array);

    if ( CLASS_CONSTANTARRAY == a_info->class )
    {
        if ( ( ERROR == ( array = (Array)DXNewConstantArrayV
                                     ( vtx_hash.count,
                                       DXGetConstantArrayData ( a_info->array ),
                                       a_info->type,
                                       a_info->category,
                                       a_info->rank,
                                       a_info->shape ) ) )
             ||
             !DXSetComponentValue ( out, comp->name, (Object) array )
             ||
             !DXSetComponentAttribute
                  ( out, comp->name, "dep",
                    (Object) DXNewString ( "positions" ) ) )

            goto error;

        continue;
    }

        if ( ( ERROR == ( array
                   = DXNewArrayV
                         ( TYPE_FLOAT, a_info->category,
                           a_info->rank, &a_info->shape[0] ) ) )
             ||
             !DXAllocateArray     ( array, vtx_hash.count )           ||
             !DXAddArrayData      ( array, 0, vtx_hash.count, NULL )  ||
             !DXTrim              ( array )                           ||
             !DXSetComponentValue ( out, comp->name, (Object) array )
             ||
             !DXSetComponentAttribute
                  ( out, comp->name, "dep",
                    (Object) DXNewString ( "positions" ) )
             ||
             !_dxf_SetIterator    ( comp )
             ||
             ( ERROR == ( out_data = DXGetArrayData  ( array )))
	     ||
	     !(itemsize = DXGetItemSize(array))
	     ||
             ( ERROR == ( scratch  = DXAllocateLocal ( 2 * itemsize)))
             ||
             !DXInitGetNextHashElement ( vtx_hash.table ) )
            goto error;

        array = NULL;

        for ( i = 0;
              ( NULL != ( element_ptr
                              = (band_edge_hash_ptr) DXGetNextHashElement
                                                    ( vtx_hash.table ) ) );
              i++ )
        {
            DXASSERTGOTO ( element_ptr->seq < po_info->items );

            if ( element_ptr->edge.p == element_ptr->edge.q )
                t = 0.0;
            else
            {
                if ( !di_info->get_item
                          ( element_ptr->edge.p, di_info, &data[0] )||
                     !di_info->get_item
                          ( element_ptr->edge.q, di_info, &data[1] ) )
                    goto error;

                /* DXASSERTGOTO ( element_ptr->select != NEITHER_VALUE ); */

                value_low = value2_ptr [ element_ptr->select ];

                if      ( data[0] == value_low ) t = 0.0;
                else if ( data[1] == value_low ) t = 1.0;
                else
                    t = ( (double)data[0] - (double)value_low ) / ( (double)data[0] - (double)data[1] );

                DXASSERTGOTO ( ( t >= 0.0 ) && ( t <= 1.0 ) );
            }

            if ( !interp ( a_info, scratch,
                           element_ptr->edge.p, element_ptr->edge.q, t, 
                           &(((char*)out_data)
                               [ a_info->itemsize * element_ptr->seq ]),
			       itemsize/sizeof(float),
			       itemsize) )
                goto error;
        }

        DXFree ( scratch );  scratch = NULL;
    }


    for ( j=0, comp=input_info->comp_list;
          j<input_info->comp_count;
          j++, comp++ )
    {

        if ( ( NULL  == comp->std_attribs[(int)DEP]        ) ||
             ( ERROR == comp->std_attribs[(int)DEP]->value ) ||
             ( 0 != strcmp ( comp->std_attribs[(int)DEP]->value,
                             "connections" ) ) )
            continue;

        if ( ( 0 == strcmp  ( comp->name, "connections" ) ) ||
             ( 0 == strncmp ( comp->name, "original", 8 ) ) ||
	     ( 0 == strcmp  ( comp->name, "invalid connections" ) ) ||
             ( NULL != comp->std_attribs[(int)REF]        )  )
            continue;

        a_info = (array_info) &(comp->array);

        if ( ( ERROR == ( array = _dxf_resample_array
                                      ( comp, 1,
                                        ((array_info) &(comp->array))->items,
                                        connection_count, NULL, c_ID_pull ) ) )
             ||
             !DXSetComponentValue ( out, comp->name, (Object)array )
             ||
             !DXSetComponentAttribute
                  ( out, comp->name, "dep",
                    (Object) DXNewString ( "connections" ) ) )
            goto error;
    }

    if ( !remove_degenerates ( out ) )
        goto error;

    done:
        /* This is create_band_lines */
        if ( DXGetAttribute ( (Object)input_info->field, "shade" ) )
            if ( !DXSetAttribute
                      ( (Object) out,
                        "shade",
                        DXGetAttribute
                            ( (Object)input_info->field, "shade" ) ) )
                goto error;

        DXDestroyHash     ( vtx_hash.table );
        DXFree            ( (Pointer) value2_ptr  );
        _dxf_FreeInMemory ( output_info );
        DXDeleteSegList   ( c_ID_list );
        DXFree            ( c_ID_pull );
        DXFree            ( scratch );

        DXFreeInvalidComponentHandle ( i_handle );

        vtx_hash.table = NULL;
        vtx_hash.count = 0;
        value2_ptr     = NULL;
        output_info    = NULL;
        i_handle       = NULL;
        c_ID_list      = NULL;
        c_ID_pull      = NULL;
        scratch        = NULL;

        return DXEndField ( out );

    error:
        ERROR_SECTION;

        DXDestroyHash     ( vtx_hash.table );
        DXFree            ( (Pointer) value2_ptr  );
        _dxf_FreeInMemory ( output_info );
        DXDeleteSegList   ( c_ID_list );
        DXFree            ( c_ID_pull );
        DXFree            ( scratch );

        DXFreeInvalidComponentHandle ( i_handle );

        vtx_hash.table = NULL;
        vtx_hash.count = 0;
        value2_ptr     = NULL;
        output_info    = NULL;
        i_handle       = NULL;
        c_ID_list      = NULL;
        c_ID_pull      = NULL;
        scratch        = NULL;

        return ERROR;
}
#endif



static
Field create_iso_contours
          ( field_info     input_info, 
            float          *value_ptr,
            int            is_grown,
            Error (*convert_connection)
                        (void *, float *, hash_table_ptr, hash_table_ptr, int),
                     /* (void *, float *, hash_table_ptr, SegList *,    int),*/
            Field out )

{
    float           value = value_ptr [ 0 ];
    array_info      pi_info, ci_info, di_info  = NULL;
    array_info      po_info, co_info = NULL;
    hash_table_rec  vtx_hash;
    hash_table_rec  edge_hash;
    Quadrilateral   quad;
    float           data[4];
    double          t;
    iso_edge_hash_ptr   element_ptr = NULL;
    int             i, j;
    field_info      output_info = NULL;
    int             position_dim;
    int             connection_shape;

    SegList         *c_ID_list = NULL;
    int             *c_ID_pull = NULL;
    int             n_c_dep, n_p_dep;
    array_info      a_info;
    component_info  comp;
    Array           array    = NULL;
    Pointer         scratch  = NULL;
    Pointer         out_data = NULL;

    InvalidComponentHandle  i_handle = NULL;

    DXASSERTGOTO ( ERROR != convert_connection );
    DXASSERTGOTO ( ERROR != input_info );
    DXASSERTGOTO ( ERROR != value_ptr );
    DXASSERTGOTO ( ERROR != out );

    vtx_hash.count         = 0;
    vtx_hash.table         = NULL;
    vtx_hash.set_vertices  = NULL;
    edge_hash.count        = 0;
    edge_hash.table        = NULL;
    edge_hash.set_vertices = NULL;

    pi_info = (array_info) &(input_info->std_comps[(int)POSITIONS]->array);
    ci_info = (array_info) &(input_info->std_comps[(int)CONNECTIONS]->array);
    di_info = (array_info) &(input_info->std_comps[(int)DATA]->array);

    if ( ( NULL  != input_info->std_comps[(int)INVALID_CONNECTIONS] ) &&
         ( ERROR == ( i_handle = DXCreateInvalidComponentHandle
                                     ( (Object)input_info->field,
                                       NULL, "connections" ) ) ) )
        goto error;

    DXASSERTGOTO ( pi_info->get_item != ERROR );
    DXASSERTGOTO ( ci_info->get_item != ERROR );
    DXASSERTGOTO ( di_info->get_item != ERROR );

    if ( ( pi_info->shape[0] != 2 ) &&
         ( pi_info->shape[0] != 3 ) )
        DXErrorGoto3
            ( ERROR_DATA_INVALID,
              "#11400", /* %s has invalid dimensionality; does not match %s */
              "\"positions\" component", "\"connections\"" );

    position_dim     = pi_info->shape[0];
    connection_shape = ci_info->shape[0];

    DXASSERTGOTO ( ( connection_shape == 3 ) || ( connection_shape == 4 ) );
    DXASSERTGOTO ( ci_info->items >= ci_info->original_items );

    TST_HASH_INTS;

    if ( ( ERROR == ( vtx_hash.table
             = DXCreateHash
                 ( sizeof(iso_edge_hash_rec),
                   iso_edge_hash_func, iso_edge_compare_func ) ) )
         ||
         ( ERROR == ( edge_hash.table
             = DXCreateHash
                 ( sizeof(iso_edge_hash_rec),
                   iso_edge_hash_func, iso_edge_compare_func ) ) ) )
        goto error;

    if ( !tally_dependers ( input_info, &n_c_dep, &n_p_dep ) )
        goto error;

    if ( ( n_c_dep > 1 )
         &&
         ( ERROR == ( c_ID_list = DXNewSegList ( sizeof(int), 1000, 1000 ) ) ) )
        goto error;

    if ( ci_info->get_item == _dxf_get_conn_grid_QUADS )
    {
        array_info  t0, t1;
        int         II, JJ;
        int         I0, J0;

        t0 = (array_info)
         &(((struct _array_allocator *)((mesh_array_info)ci_info)->terms)[0]);

        t1 = (array_info)
         &(((struct _array_allocator *)((mesh_array_info)ci_info)->terms)[1]);

        DXASSERTGOTO ( ((grid_connections_info)ci_info)->counts != NULL );
        DXASSERTGOTO
           ( t0->items == ( ((grid_connections_info)ci_info)->counts[0] - 1 ) );
        DXASSERTGOTO
           ( t1->items == ( ((grid_connections_info)ci_info)->counts[1] - 1 ) );

        if ( ((mesh_array_info)ci_info)->grow_offsets != NULL )
        {
            DXASSERTGOTO ( is_grown );

            I0 = ((mesh_array_info)ci_info)->grow_offsets[0];
            J0 = ((mesh_array_info)ci_info)->grow_offsets[1];
        }
        else
            I0 = J0 = 0;

        for ( II=I0;
              II<( I0 + t0->original_items );
              II++ )
        {
            if ( !ci_info->get_item 
                      ( ( ( II * t1->items ) + J0 ),
                        ci_info, &quad )
                 ||
                 !di_info->get_item ( quad.p, di_info, &data[0] ) ||
                 !di_info->get_item ( quad.r, di_info, &data[2] )   )
                goto error;

            data[0] -= value;
            data[2] -= value;

            for ( JJ=J0;
                  JJ<( J0 + t1->original_items );
                  JJ++,
                        quad.p++, quad.q++, quad.r++, quad.s++,
                        data[0] = data[1],  data[2] = data[3] )
            {
                if ( !di_info->get_item ( quad.q, di_info, &data[1] ) ||
                     !di_info->get_item ( quad.s, di_info, &data[3] )   )
                    goto error;

                data[1] -= value;
                data[3] -= value;

                if ( ( NULL != i_handle )
                     &&
                     DXIsElementInvalid ( i_handle, ((II*t1->items)+JJ) ) )
                    continue;
                else if ( !convert_connection
                          ( &quad, data, &vtx_hash, &edge_hash,
                            /* equivalence */ 1 ) )
                    goto error;
                else if ( ( NULL != c_ID_list )
                          &&
                          !set_c_ID
                               ( c_ID_list, edge_hash.count,
                                 ((II*t1->items)+JJ) ) )
                    goto error;
            }
        }
    }
    else
    {
        for ( i=0;
              i<ci_info->original_items;
              i++ )
            if ( ( NULL != i_handle ) && DXIsElementInvalid ( i_handle, i ) )
                continue;
            else if ( !ci_info->get_item ( i, ci_info, &quad ) )
                goto error;
            else
            {
                for ( j=0; j<connection_shape; j++ )
                {
                    if ( !di_info->get_item
                              ( ((int *)&quad)[j], di_info, &data[j] ) )
                        goto error;
                    data[j] -= value;
                }

                if ( !convert_connection
                          ( &quad, data, &vtx_hash, &edge_hash,
                            /* equivalence */ 1 ) )
                    goto error;
                else if ( ( NULL != c_ID_list )
                          &&
                          !set_c_ID ( c_ID_list, edge_hash.count, i ) )
                    goto error;
            }
    }

    if ( edge_hash.count == 0 ) {
        if ( !_dxf_MakeFieldEmpty ( out ) )
            goto error;
        else
            goto done;
    }

    if ( !create_output_arrays
              ( out,
                vtx_hash.count,     position_dim,
                edge_hash.count,    1,
                0,
                DEFAULT_LINE_COLOR, 
                value,              DEP_POSITIONS,
                ISOSURFACE ) )
        goto error;

    if ( vtx_hash.count == 0 ) 
        goto done;

    if ( ( edge_hash.count > 0 ) && ( c_ID_list != NULL ) )
    {
        if ( ( ERROR == ( c_ID_pull = DXAllocateLocal
                                          ( edge_hash.count * sizeof(int) ) ) )
             ||
             !copy_seglist_to_memory
                  ( c_ID_list, c_ID_pull, sizeof(int), edge_hash.count ) )
            goto error;
    
        DXDeleteSegList ( c_ID_list );  c_ID_list = NULL;
    }

    if ( ( ERROR == ( output_info = ( _dxf_InMemory ( out ) ) ) )
         ||
         !DXInitGetNextHashElement ( vtx_hash.table )
         ||
         !DXInitGetNextHashElement ( edge_hash.table ) )
        goto error;

    DXASSERTGOTO ( output_info->std_comps[(int)POSITIONS]   != NULL );
    DXASSERTGOTO ( output_info->std_comps[(int)CONNECTIONS] != NULL );

    po_info = (array_info) &(output_info->std_comps[(int)POSITIONS]->array);
    co_info = (array_info) &(output_info->std_comps[(int)CONNECTIONS]->array);

    for ( i = 0;
          ( NULL != ( element_ptr
                          = (iso_edge_hash_ptr) DXGetNextHashElement
                                                ( edge_hash.table ) ) );
          i++ )
    {
        DXASSERTGOTO ( element_ptr->seq < co_info->items );

        ((Line *)co_info->data) [ element_ptr->seq ] = element_ptr->edge;
    }

    DXDestroyHash ( edge_hash.table );
        edge_hash.table = NULL;  edge_hash.count = 0;


    for ( j=0, comp=input_info->comp_list;
          j<input_info->comp_count;
          j++, comp++ )
    {

        if ( ( 0 != strcmp ( comp->name, "positions" ) ) &&
             ( ( NULL  == comp->std_attribs[(int)DEP]        ) ||
               ( ERROR == comp->std_attribs[(int)DEP]->value ) ||
               ( 0 != strcmp ( comp->std_attribs[(int)DEP]->value,
                               "positions" ) ) ) )
            continue;

        if ( ( 0 == strcmp  ( comp->name, "data"        ) ) ||
             ( 0 == strncmp ( comp->name, "original", 8 ) ) ||
             ( NULL != comp->std_attribs[(int)REF]        )   )
            continue;

        a_info = (array_info) &(comp->array);

    if ( CLASS_CONSTANTARRAY == a_info->class )
    {
        if ( ( ERROR == ( array = (Array)DXNewConstantArrayV
                                     ( vtx_hash.count,
                                       DXGetConstantArrayData ( a_info->array ),
                                       a_info->type,
                                       a_info->category,
                                       a_info->rank,
                                       a_info->shape ) ) )
             ||
             !DXSetComponentValue ( out, comp->name, (Object) array )
             ||
             !DXSetComponentAttribute
                  ( out, comp->name, "dep",
                    (Object) DXNewString ( "positions" ) ) )

            goto error;

        continue;
    }

        if ( ( ERROR == ( array
                   = DXNewArrayV
                         ( a_info->type, a_info->category,
                           a_info->rank, &a_info->shape[0] ) ) )
             ||
             !DXAllocateArray     ( array, vtx_hash.count )           ||
             !DXAddArrayData      ( array, 0, vtx_hash.count, NULL )  ||
             !DXTrim              ( array )                           ||
             !DXSetComponentValue ( out, comp->name, (Object) array )
             ||
             !DXSetComponentAttribute
                  ( out, comp->name, "dep",
                    (Object) DXNewString ( "positions" ) )
             ||
             !_dxf_SetIterator    ( comp )
             ||
             ( ERROR == ( out_data = DXGetArrayData  ( array )))             ||
             ( ERROR == ( scratch  = DXAllocateLocal ( 2 * a_info->itemsize )))
             ||
             !DXInitGetNextHashElement ( vtx_hash.table ) )
            goto error;

        array = NULL;

        for ( i = 0;
              ( NULL != ( element_ptr
                              = (iso_edge_hash_ptr) DXGetNextHashElement
                                                    ( vtx_hash.table ) ) );
              i++ )
        {
            DXASSERTGOTO ( element_ptr->seq < po_info->items );

            if ( element_ptr->edge.p == element_ptr->edge.q )
                t = 0.0;
            else
            {
                if ( !di_info->get_item
                          ( element_ptr->edge.p, di_info, &data[0] )||
                     !di_info->get_item
                          ( element_ptr->edge.q, di_info, &data[1] ) )
                    goto error;

                data[0] -= value;
                data[1] -= value;

                if      ( data[0] == 0.0 ) t = 0.0;
                else if ( data[1] == 0.0 ) t = 1.0;
                else
                    t = (double)data[0] / ( (double)data[0] - (double)data[1] );

                DXASSERTGOTO ( ( t >= 0.0 ) && ( t <= 1.0 ) );
            }

            if ( !interp ( a_info, scratch,
                           element_ptr->edge.p, element_ptr->edge.q, t,
                           &(((char*)out_data)
                               [ a_info->itemsize * element_ptr->seq ]),
			       a_info->itemsize/sizeof(float),
			       a_info->itemsize) )
                goto error;
        }

        DXFree ( scratch );  scratch = NULL;
    }


    for ( j=0, comp=input_info->comp_list;
          j<input_info->comp_count;
          j++, comp++ )
    {

        if ( ( NULL  == comp->std_attribs[(int)DEP]        ) ||
             ( ERROR == comp->std_attribs[(int)DEP]->value ) ||
             ( 0 != strcmp ( comp->std_attribs[(int)DEP]->value,
                             "connections" ) ) )
            continue;

        if ( ( 0 == strcmp  ( comp->name, "connections" ) ) ||
             ( 0 == strncmp ( comp->name, "original", 8 ) ) ||
	     ( 0 == strcmp  ( comp->name, "invalid connections" ) ) ||
             ( NULL != comp->std_attribs[(int)REF]        )  )
            continue;

        a_info = (array_info) &(comp->array);

        if ( ( ERROR == ( array = _dxf_resample_array
                                      ( comp, 1,
                                        ((array_info) &(comp->array))->items,
                                        edge_hash.count, NULL, c_ID_pull ) ) )
             ||
             !DXSetComponentValue ( out, comp->name, (Object)array )
             ||
             !DXSetComponentAttribute
                  ( out, comp->name, "dep",
                    (Object) DXNewString ( "connections" ) ) )
            goto error;
    }

    if ( !remove_degenerates ( out ) )
        goto error;

    done:
        /* This is create_iso_contours */
        if ( !DXSetIntegerAttribute ( (Object) out, "shade", 0 ) )
            goto error;

        DXDestroyHash     ( edge_hash.table );
        DXDestroyHash     ( vtx_hash.table );
        _dxf_FreeInMemory ( output_info );
        DXDeleteSegList   ( c_ID_list );
        DXFree            ( c_ID_pull );
        DXFree            ( scratch );

        DXFreeInvalidComponentHandle ( i_handle );

        edge_hash.table = NULL;
        edge_hash.count = 0;
        vtx_hash.table  = NULL;
        vtx_hash.count  = 0;
        output_info     = NULL;
        i_handle        = NULL;
        c_ID_list       = NULL;
        c_ID_pull       = NULL;
        scratch         = NULL;

        return DXEndField ( out );

    error:
        ERROR_SECTION;

        DXDestroyHash     ( edge_hash.table );
        DXDestroyHash     ( vtx_hash.table );
        _dxf_FreeInMemory ( output_info );
        DXDeleteSegList   ( c_ID_list );
        DXFree            ( c_ID_pull );
        DXFree            ( scratch );

        DXFreeInvalidComponentHandle ( i_handle );

        edge_hash.table = NULL;
        edge_hash.count = 0;
        vtx_hash.table  = NULL;
        vtx_hash.count  = 0;
        output_info     = NULL;
        i_handle        = NULL;
        c_ID_list       = NULL;
        c_ID_pull       = NULL;
        scratch         = NULL;

        return ERROR;
}



#ifdef INCLUDES_BAND
static
Field create_band_surface
          ( field_info     input_info, 
            float          *value_ptr,
            int            number,
            band_remap_types remap,
            int            is_grown,
            float          *min_max,
            Error (*convert_connection)
                  (void *,float *,float *,int,hash_table_ptr,SegList *,int),
            Field out )

{
    array_info      pi_info, ci_info, di_info;
    array_info      po_info, co_info, do_info;
    hash_table_rec  vtx_hash;
    SegList         *tri_list = NULL;
    SegList         *cval_list = NULL;
    Quadrilateral   quad;
    float           data[4];
    double          t;
    band_edge_hash_ptr   element_ptr = NULL;
    int             D;
    int             i, j;
    field_info      output_info = NULL;
    int             position_dim;
    int             connection_shape;
    int             connection_count;
    float           value_low;
    float           old_min, old_max;
    float           *value2_ptr = NULL;
    int             have_normals = 0;

    SegList         *c_ID_list = NULL;
    int             *c_ID_pull = NULL;
    int             n_c_dep, n_p_dep;
    array_info      a_info;
    component_info  comp;
    Array           array    = NULL;
    Pointer         scratch  = NULL;
    Pointer         out_data = NULL;
    int             itemsize;

    InvalidComponentHandle  i_handle = NULL;

    DXASSERTGOTO ( ERROR != input_info );
    DXASSERTGOTO ( ERROR != value_ptr );
    DXASSERTGOTO ( ERROR != convert_connection );
    DXASSERTGOTO ( ERROR != out );

    vtx_hash.count        = 0;
    vtx_hash.table        = NULL;
    vtx_hash.set_vertices = NULL;

    if ( ( NULL  != input_info->std_comps[(int)INVALID_CONNECTIONS] ) &&
         ( ERROR == ( i_handle = DXCreateInvalidComponentHandle
                                     ( (Object)input_info->field,
                                       NULL, "connections" ) ) ) )
        goto error;

    if ( ERROR == ( value2_ptr = band_value_list
                                     ( input_info->field,
                                       value_ptr, &number, min_max,
                                       &old_min, &old_max ) ) )
        goto error;

    pi_info = (array_info) &(input_info->std_comps[(int)POSITIONS]->array);
    ci_info = (array_info) &(input_info->std_comps[(int)CONNECTIONS]->array);
    di_info = (array_info) &(input_info->std_comps[(int)DATA]->array);
    /*ni_info = (input_info->std_comps[(int)NORMALS]==NULL)?
             NULL: (array_info) &(input_info->std_comps[(int)NORMALS]->array);*/

    DXASSERTGOTO ( pi_info->get_item != ERROR );
    DXASSERTGOTO ( ci_info->get_item != ERROR );
    DXASSERTGOTO ( di_info->get_item != ERROR );

    if ( ( pi_info->shape[0] != 2 ) &&
         ( pi_info->shape[0] != 3 ) )
        DXErrorGoto3
            ( ERROR_DATA_INVALID,
              "#11400", /* %s has invalid dimensionality; does not match %s */
              "\"positions\" component", "\"connections\"" );

    position_dim     = pi_info->shape[0];
    connection_shape = ci_info->shape[0];

    DXASSERTGOTO ( ( connection_shape == 3 ) || ( connection_shape == 4 ) );

    DXASSERTGOTO ( ci_info->items >= ci_info->original_items );

    TST_HASH_INTS;

    if ( ( ERROR == ( vtx_hash.table
             = DXCreateHash
                 ( sizeof(band_edge_hash_rec),
                   band_edge_hash_func,
                   band_edge_compare_func ) ) )
         ||
         ( ERROR == ( tri_list
                          = DXNewSegList ( sizeof(Triangle), 1000, 1000 ) ) ) )
        goto error;

    if ( remap != ORIGINAL_DATA )
       if ( ERROR == ( cval_list = DXNewSegList ( sizeof(float), 1000, 1000 )))
            goto error;

    if ( !tally_dependers ( input_info, &n_c_dep, &n_p_dep ) )
        goto error;

    if ( ( n_c_dep > 1 )
         &&
         ( ERROR == ( c_ID_list = DXNewSegList ( sizeof(int), 1000, 1000 ) ) ) )
        goto error;

    if ( ci_info->get_item == _dxf_get_conn_grid_QUADS )
    {
        array_info  t0, t1;
        int         II, JJ;
        int         I0, J0;

        t0 = (array_info)
         &(((struct _array_allocator *)((mesh_array_info)ci_info)->terms)[0]);

        t1 = (array_info)
         &(((struct _array_allocator *)((mesh_array_info)ci_info)->terms)[1]);

        DXASSERTGOTO ( ((grid_connections_info)ci_info)->counts != NULL );
        DXASSERTGOTO
           ( t0->items == ( ((grid_connections_info)ci_info)->counts[0] - 1 ) );
        DXASSERTGOTO
           ( t1->items == ( ((grid_connections_info)ci_info)->counts[1] - 1 ) );

        if ( ((mesh_array_info)ci_info)->grow_offsets != NULL )
        {
            DXASSERTGOTO ( is_grown );

            I0 = ((mesh_array_info)ci_info)->grow_offsets[0];
            J0 = ((mesh_array_info)ci_info)->grow_offsets[1];
        }
        else
            I0 = J0 = 0;

        for ( II=I0;
              II<( I0 + t0->original_items );
              II++ )
        {
            if ( !ci_info->get_item 
                      ( ( ( II * t1->items ) + J0 ),
                        ci_info, &quad )
                 ||
                 !di_info->get_item ( quad.p, di_info, &data[0] ) ||
                 !di_info->get_item ( quad.r, di_info, &data[2] )   )
                goto error;

            for ( JJ=J0;
                  JJ<( J0 + t1->original_items );
                  JJ++,
                        quad.p++, quad.q++, quad.r++, quad.s++,
                        data[0] = data[1],  data[2] = data[3] )
            {
                if ( !di_info->get_item ( quad.q, di_info, &data[1] ) ||
                     !di_info->get_item ( quad.s, di_info, &data[3] )   )
                    goto error;

                if ( ( NULL != i_handle )
                     &&
                     DXIsElementInvalid ( i_handle, ((II*t1->items)+JJ) ) )
                    continue;
                else
                    for ( D=0; D<number-1; D++ )
                    {
                        if ( !convert_connection
                                 ( &quad, data, value2_ptr, D,
                                   &vtx_hash, tri_list, 1 ) )
                            goto error;
                        else if ( ( NULL != c_ID_list )
                                  &&
                                  !set_c_ID
                                       ( c_ID_list,
                                         DXGetSegListItemCount ( tri_list ),
                                         II ) )
                            goto error;

                        switch ( remap )
                        {
                            case ORIGINAL_DATA: break;

                            case DATA_LOW:
                                if ( !set_cval
                                         ( cval_list,
                                           DXGetSegListItemCount ( tri_list ),
                                           ( ( D == 0 ) ?
                                               old_min : value2_ptr[D] ) ) )
                                    goto error;
                                break;

                            case DATA_HIGH:
                                if ( !set_cval
                                         ( cval_list,
                                           DXGetSegListItemCount ( tri_list ),
                                           ( ( (D+1) == number ) ?
                                               old_max : value2_ptr[D+1] ) ) )
                                    goto error;
                                break;
                        }
                    }
            }
        }
    }
    else
    {
        for ( i=0;
              i<ci_info->original_items;
              i++ )
            if ( ( NULL != i_handle ) && DXIsElementInvalid ( i_handle, i ) )
                continue;
            else if ( !ci_info->get_item ( i, ci_info, &quad ) )
                goto error;
            else
            {
                for ( j=0; j<connection_shape; j++ )
                    if ( !di_info->get_item
                              ( ((int *)&quad)[j], di_info, &data[j] ) )
                        goto error;

                for ( D=0; D<number-1; D++ )
                {
                    if ( !convert_connection
                             ( &quad, data, value2_ptr, D,
                               &vtx_hash, tri_list, 1 ) )
                        goto error;
                    else if ( ( NULL != c_ID_list )
                              &&
                              !set_c_ID
                                   ( c_ID_list,
                                     DXGetSegListItemCount ( tri_list ), i ) )
                        goto error;

                    switch ( remap )
                    {
                        case ORIGINAL_DATA: break;

                        case DATA_LOW:
                            if ( !set_cval
                                     ( cval_list,
                                       DXGetSegListItemCount ( tri_list ),
                                       ( ( D == 0 ) ?
                                           old_min : value2_ptr[D] ) ) )
                                goto error;
                            break;

                        case DATA_HIGH:
                            if ( !set_cval
                                     ( cval_list,
                                       DXGetSegListItemCount ( tri_list ),
                                       ( ( (D+1) == number ) ?
                                           old_max : value2_ptr[D+1] ) ) )
                                goto error;
                            break;
                    }
                }
            }
    }

    connection_count = DXGetSegListItemCount ( tri_list );

    if ( connection_count == 0 ) {
        if ( !_dxf_MakeFieldEmpty ( out ) )
            goto error;
        else
            goto done;
    }

    if ( !create_output_arrays
              ( out,
                vtx_hash.count,   position_dim,
                connection_count, 2,
                ( (have_normals==1)? vtx_hash.count : 0 ),
                DEFAULT_SURFACE_COLOR, 
                0.0, ( (remap==ORIGINAL_DATA)? DEP_POSITIONS: DEP_CONNECTIONS ),
                BAND ) )
        goto error;

    if ( vtx_hash.count == 0 ) 
        goto done;

    if ( ( connection_count > 0 ) && ( c_ID_list != NULL ) )
    {
        if ( ( ERROR == ( c_ID_pull = DXAllocateLocal
                                          ( connection_count * sizeof(int) ) ) )
             ||
             !copy_seglist_to_memory
                  ( c_ID_list, c_ID_pull, sizeof(int), connection_count ) )
            goto error;
    
        DXDeleteSegList ( c_ID_list );  c_ID_list = NULL;
    }

    if ( ( ERROR == ( output_info = ( _dxf_InMemory ( out ) ) ) )
         ||
         !DXInitGetNextHashElement ( vtx_hash.table ) )
        goto error;

    DXASSERTGOTO ( output_info->std_comps[(int)POSITIONS] != NULL );

    po_info = (array_info) &(output_info->std_comps[(int)POSITIONS]->array);
    co_info = (array_info) &(output_info->std_comps[(int)CONNECTIONS]->array);
    do_info = (array_info) &(output_info->std_comps[(int)DATA]->array);
    /*no_info = (output_info->std_comps[(int)NORMALS]==NULL)?
            NULL: (array_info) &(output_info->std_comps[(int)NORMALS]->array);*/

    if ( !copy_seglist_to_array ( tri_list, co_info ) )
        goto error;

    if ( remap != ORIGINAL_DATA )
        if ( !copy_seglist_to_array ( cval_list, do_info ) )
            goto error;

    DXDeleteSegList ( tri_list  );  tri_list = NULL;
    DXDeleteSegList ( cval_list );  cval_list = NULL;

    DXASSERTGOTO ( ERROR != do_info->data );


    for ( j=0, comp=input_info->comp_list;
          j<input_info->comp_count;
          j++, comp++ )
    {
	/*
	 * If its not positions and it is not dep on positions,
	 * skip it.
	 */
        if ( ( 0 != strcmp ( comp->name, "positions" ) ) &&
             ( ( NULL  == comp->std_attribs[(int)DEP]        ) ||
               ( ERROR == comp->std_attribs[(int)DEP]->value ) ||
               ( 0 != strcmp ( comp->std_attribs[(int)DEP]->value,
                               "positions" ) ) ) )
            continue;
	
	/*
	 * Skip invalid components
	 */
	if (! strcmp(comp->name, "invalid positions") ||
	    ! strcmp(comp->name, "invalid connections"))
	    continue;

        if ( ( ( 0 == strcmp ( comp->name, "data" ) )
               &&
               ( remap != ORIGINAL_DATA ) )
             ||
             ( 0 == strncmp ( comp->name, "original", 8 ) ) ||
             ( NULL != comp->std_attribs[(int)REF]        )  )

	
            continue;

        a_info = (array_info) &(comp->array);

    if ( CLASS_CONSTANTARRAY == a_info->class )
    {
        if ( ( ERROR == ( array = (Array)DXNewConstantArrayV
                                     ( vtx_hash.count,
                                       DXGetConstantArrayData ( a_info->array ),
                                       a_info->type,
                                       a_info->category,
                                       a_info->rank,
                                       a_info->shape ) ) )
             ||
             !DXSetComponentValue ( out, comp->name, (Object) array )
             ||
             !DXSetComponentAttribute
                  ( out, comp->name, "dep",
                    (Object) DXNewString ( "positions" ) ) )

            goto error;

        continue;
    }

        if ( ( ERROR == ( array
                   = DXNewArrayV
                         ( TYPE_FLOAT, a_info->category,
                           a_info->rank, &a_info->shape[0] ) ) )
             ||
             !DXAllocateArray     ( array, vtx_hash.count )           ||
             !DXAddArrayData      ( array, 0, vtx_hash.count, NULL )  ||
             !DXTrim              ( array )                           ||
             !DXSetComponentValue ( out, comp->name, (Object) array )
             ||
             !DXSetComponentAttribute
                  ( out, comp->name, "dep",
                    (Object) DXNewString ( "positions" ) )
             ||
             !_dxf_SetIterator    ( comp )
             ||
             ( ERROR == ( out_data = DXGetArrayData  ( array )))
	     ||
	     !(itemsize = DXGetItemSize(array))
	     ||
             ( ERROR == ( scratch  = DXAllocateLocal ( 2 * itemsize)))
             ||
             !DXInitGetNextHashElement ( vtx_hash.table ) )
            goto error;

        array = NULL;

        for ( i = 0;
              ( NULL != ( element_ptr
                              = (band_edge_hash_ptr) DXGetNextHashElement
                                                     ( vtx_hash.table ) ) );
              i++ )
        {
            DXASSERTGOTO ( element_ptr->seq < po_info->items );

            if ( element_ptr->edge.p == element_ptr->edge.q )
                t = 0.0;
            else
            {
                if ( !di_info->get_item
                         ( element_ptr->edge.p, di_info, &data[0] )||
                     !di_info->get_item
                         ( element_ptr->edge.q, di_info, &data[1] ) )
                    goto error;

                /* DXASSERTGOTO ( element_ptr->select != NEITHER_VALUE ); */

                value_low = value2_ptr [ element_ptr->select ];

                if      ( data[0] == value_low ) t = 0.0;
                else if ( data[1] == value_low ) t = 1.0;
                else
                    t = ( (double)data[0] - (double)value_low ) / ( (double)data[0] - (double)data[1] );

                DXASSERTGOTO ( ( t >= 0.0 ) && ( t <= 1.0 ) );
            }

            if ( !interp ( a_info, scratch,
                           element_ptr->edge.p, element_ptr->edge.q, t, 
                           &(((char*)out_data)
                               [ itemsize * element_ptr->seq ]),
			       itemsize/sizeof(float),
			       itemsize) )
                goto error;
        }

        DXFree ( scratch );  scratch = NULL;
    }


    for ( j=0, comp=input_info->comp_list;
          j<input_info->comp_count;
          j++, comp++ )
    {

        if ( ( NULL  == comp->std_attribs[(int)DEP]        ) ||
             ( ERROR == comp->std_attribs[(int)DEP]->value ) ||
             ( 0 != strcmp ( comp->std_attribs[(int)DEP]->value,
                             "connections" ) ) )
            continue;

        if ( ( 0 == strcmp  ( comp->name, "connections" ) ) ||
             ( 0 == strncmp ( comp->name, "original", 8 ) ) ||
	     ( 0 == strcmp  ( comp->name, "invalid connections" ) ) ||
             ( NULL != comp->std_attribs[(int)REF]        )  )
            continue;

        a_info = (array_info) &(comp->array);

        if ( ( ERROR == ( array = _dxf_resample_array
                                      ( comp, 1,
                                        ((array_info) &(comp->array))->items,
                                        connection_count, NULL, c_ID_pull ) ) )
             ||
             !DXSetComponentValue ( out, comp->name, (Object)array )
             ||
             !DXSetComponentAttribute
                  ( out, comp->name, "dep",
                    (Object) DXNewString ( "connections" ) ) )
            goto error;
    }

    if ( !remove_degenerates ( out ) )
        goto error;

    done:
        /* This is create_band_surface */
        if ( DXGetAttribute ( (Object)input_info->field, "shade" ) )
            if ( !DXSetAttribute
                      ( (Object) out,
                        "shade",
                        DXGetAttribute
                            ( (Object)input_info->field, "shade" ) ) )
                goto error;


        DXDestroyHash     ( vtx_hash.table );
        DXFree            ( (Pointer) value2_ptr );
        _dxf_FreeInMemory ( output_info );
        DXDeleteSegList   ( c_ID_list );
        DXFree            ( c_ID_pull );
        DXFree            ( scratch );

        DXFreeInvalidComponentHandle ( i_handle );

        vtx_hash.table = NULL;
        vtx_hash.count = 0;
        value2_ptr     = NULL;
        output_info    = NULL;
        i_handle       = NULL;
        c_ID_list      = NULL;
        c_ID_pull      = NULL;
        scratch        = NULL;

        return DXEndField ( out );

    error:
        ERROR_SECTION;

        DXDestroyHash     ( vtx_hash.table );
        DXFree            ( (Pointer) value2_ptr );
        _dxf_FreeInMemory ( output_info );
        DXDeleteSegList   ( c_ID_list );
        DXFree            ( c_ID_pull );
        DXFree            ( scratch );

        DXFreeInvalidComponentHandle ( i_handle );

        vtx_hash.table = NULL;
        vtx_hash.count = 0;
        value2_ptr     = NULL;
        output_info    = NULL;
        i_handle       = NULL;
        c_ID_list      = NULL;
        c_ID_pull      = NULL;
        scratch        = NULL;

        return ERROR;
}
#endif



#if 1
#define IsFlippedTet_METHOD _dxf_IsFlippedTet_4x4_method 
#else
#define IsFlippedTet_METHOD _dxf_IsFlippedTet_3x3_method 
#endif



#if ( IsFlippedTet_METHOD == _dxf_IsFlippedTet_4x4_method )
static
 /* required here and in showboundary.m */
int _dxf_IsFlippedTet_4x4_method ( Tetrahedron tet, Point *geom )
{
   /*
    * Calculate the geometric determinate of a tetrahedron, use it's sign.
    * The expression below is the matrix we are 'determining', with 'g'
    * representing geometry, 't' representing tetrahedral connection indices.
    *
    * +-----------------------+----------------------------------------------+
    * |   a11  a12  a13  a14  |   1.0        1.0        1.0        1.0       |
    * |   a21  a22  a23  a24  |   g[t.p].x   g[t.q].x   g[t.r].x   g[t.s].x  |
    * |   a31  a32  a33  a34  |   g[t.p].y   g[t.q].y   g[t.r].y   g[t.s].y  |
    * |   a41  a42  a43  a44  |   g[t.p].z   g[t.q].z   g[t.r].z   g[t.s].z  |
    * +-----------------------+----------------------------------------------+
    */
    int ret;

    double  a11, a12, a13, a14,   a21, a22, a23, a24, 
            a31, a32, a33, a34,   a41, a42, a43, a44;

    double  d;

    DXASSERTGOTO ( geom != NULL );

    a11=1.0;           a12=1.0;           a13=1.0;           a14=1.0;
    a21=geom[tet.p].x; a22=geom[tet.q].x; a23=geom[tet.r].x; a24=geom[tet.s].x;
    a31=geom[tet.p].y; a32=geom[tet.q].y; a33=geom[tet.r].y; a34=geom[tet.s].y;
    a41=geom[tet.p].z; a42=geom[tet.q].z; a43=geom[tet.r].z; a44=geom[tet.s].z;

    d = a11*(a22*(a33*a44-a43*a34)-a23*(a32*a44-a42*a34)+a24*(a32*a43-a42*a33))
       -a12*(a21*(a33*a44-a43*a34)-a23*(a31*a44-a41*a34)+a24*(a31*a43-a41*a33))
       +a13*(a21*(a32*a44-a42*a34)-a22*(a31*a44-a41*a34)+a24*(a31*a42-a41*a32))
      -a14*(a21*(a32*a43-a42*a33)-a22*(a31*a43-a41*a33)+a23*(a31*a42-a41*a32));

    ret = ( ABS(d) < DXD_MIN_FLOAT ) ? 0 : ( d < 0.0 )? 1 : -1;

    DXDebug ( "I", "_dxf_IsFlippedTet_4x4_method() = %g = %d", d, ret );

    return ret;

    /* error: */
        ERROR_SECTION;
        return ERROR;
}
#endif



#if 0
#if ( IsFlippedTet_METHOD == _dxf_IsFlippedTet_3x3_method )
static
 /* required here and in showboundary.m */
int _dxf_IsFlippedTet_3x3_method ( Tetrahedron tet, Point *geom )
{
   /*
    * Calculate the geometric determinate of a tetrahedron, use it's sign.
    * Three delta vectors method.
    *
    *+-------------+-----------------------------------------------------------+
    *| a11 a12 a13 | (g[t.p].x-g[t.q].x) (g[t.p].x-g[t.r].x) (g[t.p].x-g[t.s].x)
    *| a21 a22 a23 | (g[t.p].y-g[t.q].y) (g[t.p].y-g[t.r].y) (g[t.p].y-g[t.s].y)
    *| a31 a32 a33 | (g[t.p].z-g[t.q].z) (g[t.p].z-g[t.r].z) (g[t.p].z-g[t.s].z)
    *+-------------+-----------------------------------------------------------+
    */
    int    ret;

    double  a11, a12, a13,   a21, a22, a23,   a31, a32, a33;

    double  d;

    DXASSERTGOTO ( geom != NULL );

    a11 = geom[tet.p].x - geom[tet.q].x;
    a21 = geom[tet.p].y - geom[tet.q].y;
    a31 = geom[tet.p].z - geom[tet.q].z;

    a12 = geom[tet.p].x - geom[tet.r].x;
    a22 = geom[tet.p].y - geom[tet.r].y;
    a32 = geom[tet.p].z - geom[tet.r].z;

    a13 = geom[tet.p].x - geom[tet.s].x;
    a23 = geom[tet.p].y - geom[tet.s].y;
    a33 = geom[tet.p].z - geom[tet.s].z;

    d = a11*(a22*a33-a32*a23) - a12*(a21*a33-a31*a23) + a13*(a21*a32-a31*a22);

    ret = ( ABS(d) < DXD_MIN_FLOAT ) ? 0 : ( d < 0.0 )? 1 : -1;

    DXDebug ( "I", "_dxf_IsFlippedTet_3x3_method() = %g = %d", d, ret );

    return ret;

    /* error: */
        ERROR_SECTION;
        return ERROR;
}
#endif
#endif

int _dxf_get_flip
        ( field_info input_info,
          int        *have_connections,
          int        *have_nondegeneracy )
{
    array_info  p_info, c_info = NULL;
    Point       pseudo_pt[4];

    static Tetrahedron pseudo_tet = { 0, 1, 2, 3 };

    int flipq;
    int i;

    /* XXX - DXEmptyField call belongs here */

    if ( NULL == input_info->std_comps[(int)CONNECTIONS] )
        DXErrorGoto2
            ( ERROR_DATA_INVALID,
              "#10240", /* missing %s component */ "connections" );

    if ( (input_info->std_comps[(int)CONNECTIONS]->element_type != TETRAHEDRA)
         &&
         (input_info->std_comps[(int)CONNECTIONS]->element_type != CUBES ) )
        DXErrorGoto3 ( ERROR_DATA_INVALID, "#10203", /* %s must be one of: %s */
                       "connections element type", "tetrahedra, cubes" );

    p_info = (array_info) &(input_info->std_comps[(int)POSITIONS]->array);
    c_info = (array_info) &(input_info->std_comps[(int)CONNECTIONS]->array);

    if ( p_info->get_item == NULL )
        if ( !_dxf_SetIterator ( input_info->std_comps[(int)POSITIONS] ) )
            goto error;

    if ( c_info->get_item == NULL )
        if ( !_dxf_SetIterator ( input_info->std_comps[(int)CONNECTIONS] ) )
            goto error;

    if ( c_info->items != 0 )
        *have_connections = 1;

    for ( i=0, flipq=0; ( ( flipq == 0 ) && ( i < c_info->items ) ); i++ )
    {
        switch ( input_info->std_comps[(int)CONNECTIONS]->element_type )
        {
            case TETRAHEDRA:
                {
                    Tetrahedron tet;
                    if ( !c_info->get_item ( i, c_info, &tet ) ||
                         !p_info->get_item ( tet.p, p_info, &pseudo_pt[0] ) ||
                         !p_info->get_item ( tet.q, p_info, &pseudo_pt[1] ) ||
                         !p_info->get_item ( tet.r, p_info, &pseudo_pt[2] ) ||
                         !p_info->get_item ( tet.s, p_info, &pseudo_pt[3] ) )
                        goto error;
                }
                break;
    
            case CUBES:
                {
                    Cube cube;
                    /* XXX - 0132 */
                    if ( !c_info->get_item ( i, c_info, &cube ) ||
                         !p_info->get_item ( cube.p, p_info, &pseudo_pt[0] ) ||
                         !p_info->get_item ( cube.s, p_info, &pseudo_pt[1] ) ||
                         !p_info->get_item ( cube.u, p_info, &pseudo_pt[3] ) ||
                         !p_info->get_item ( cube.v, p_info, &pseudo_pt[2] ) )
                        goto error;
                }
                break;

            default:
                goto error;
        }
        flipq = IsFlippedTet_METHOD ( pseudo_tet, pseudo_pt );
    }


    if ( ( flipq == 1 ) || ( flipq == -1 ) )
        *have_nondegeneracy = 1;
    else
        /* arbitrary */
        flipq = 1;
        
    return flipq;

    error:
        ERROR_SECTION;
        return ERROR;
}



static
Vector *tetra_grad
           ( Point *p0,  Point *p1,  Point *p2,  Point *p3, 
             float  d0,  float  d1,  float  d2,  float  d3, 
             Vector *gradient )
{
    float  A, B, C, D;
    float  invD;

    float  px, py, pz, pw;
    float  a0, a1, a2, a3;
    float  b0, b1, b2, b3;
    float  c0, c1, c2, c3;
    float  ab01, ac01, bc01, ab23, ac23, bc23;

    /* 
     * (Somewhat mixed symbology, EG: a0,a1,a2 represents x,y,z coordinates).
     *
     * Algorithm as from _gradient.c/worker_3D_tetrahedra_manhattan
     *   which is this:
     *
     * We now form 4-vectors whose first 3 coordinates are the x, y, and z
     * coordinates of a vertex of the tetrahedron, and whose 4th coordinate
     * w is the scalar data value associated with that vertex.  There are
     * 4 such 4-vectors.  We then find the equation of the 3D hyperplane
     * in 4D space which is determined by these 4 4-vectors.  We seek
     * an equation of the form: w = K1 * x + K2 * y + K3 * z + K4.
     * In this equation, K1 == df/dx, K2 == df/dy, and K3 == df/dz.
     *
     *
     * To find the equation, we represent the first point 
     * and its data value p3 as a 4-vector <px, py, pz, pw>.  We form 
     * difference 4-vectors a, b, and c whose first three coordinates
     * are the x, y, and z differences between the remaining three 
     * points of the tetrahedron and the first point, and whose last
     * coordinate is the difference between the data values of the
     * points of the tetrahedron and the data value of the first point.
     * The equation is given by the determinant:
     *
     *  | x - px   y - py   z - pz   w - pw |
     *  |   a0       a1       a2       a3   | == 0
     *  |   b0       b1       b2       b3   |
     *  |   c0       c1       c2       c3   |
     *
     * The trick here is to solve this determinant with as few
     * multiplications as possible.
     */

    px = p0->x;
    py = p0->y;
    pz = p0->z;
    pw = d0;

    a0 = p1->x - px;
    a1 = p1->y - py;
    a2 = p1->z - pz;
    a3 = d1    - pw;

    b0 = p2->x - px;
    b1 = p2->y - py;
    b2 = p2->z - pz;
    b3 = d2    - pw;

    c0 = p3->x - px;
    c1 = p3->y - py;
    c2 = p3->z - pz;
    c3 = d3    - pw;

    ab01 = ( a0 * b1 ) - ( b0 * a1 );
    ac01 = ( a0 * c1 ) - ( c0 * a1 );
    bc01 = ( b0 * c1 ) - ( c0 * b1 );
    ab23 = ( a2 * b3 ) - ( b2 * a3 );
    ac23 = ( a2 * c3 ) - ( c2 * a3 );
    bc23 = ( b2 * c3 ) - ( c2 * b3 );

    A = ( a1 * bc23 ) - ( b1 * ac23 ) + ( c1 * ab23 );
    B = ( a0 * bc23 ) - ( b0 * ac23 ) + ( c0 * ab23 );
    C = ( a3 * bc01 ) - ( b3 * ac01 ) + ( c3 * ab01 );
    D = ( a2 * bc01 ) - ( b2 * ac01 ) + ( c2 * ab01 );

    if ( ( D >= -DXD_MIN_FLOAT ) && ( D <= DXD_MIN_FLOAT ) )
    {
        /* Degenerate tetrahedron. */
        /* Note: still will get 0 magnitude gradient when all dvalues equal */

        DXDebug ( "I", "Degenerate tetrahedron." );
        gradient->x = gradient->y = gradient->z = 0.0;
    }
    else
    {
        /* Normal tetrahedron. */

        invD        = 1.0 / D;
        gradient->x =  A * invD;
        gradient->y = -B * invD;
        gradient->z =  C * invD;
    }

    return gradient;
}



static
Error flip_normals_and_triangles
          ( field_info  input_info,
            field_info  output_info,
            int         normal_direction )
{
    array_info  pi_info, ci_info = NULL;
    array_info  co_info, no_info = NULL;

    int flipq;

    Vector   *np;
    Triangle *tp;
    float     length;
    int       i, save;

    int have_connects = 0;
    int have_nondegen = 0;

    DXASSERTGOTO ( ( normal_direction == 1 ) || ( normal_direction == -1 ) );

    pi_info = (array_info) &(input_info->std_comps[(int)POSITIONS]->array);
    ci_info = (array_info) &(input_info->std_comps[(int)CONNECTIONS]->array);
    co_info = (array_info) &(output_info->std_comps[(int)CONNECTIONS]->array);
    no_info = (array_info) &(output_info->std_comps[(int)NORMALS]->array);

    DXASSERTGOTO ( pi_info->get_item != ERROR );
    DXASSERTGOTO ( ci_info->get_item != ERROR );


    if ( ERROR == ( flipq = _dxf_get_flip
                                ( input_info,
                                  &have_connects, &have_nondegen ) ) )
        goto error;

    if ( ( have_connects == 1 ) && ( have_nondegen == 0 ) )
         DXErrorGoto
             ( ERROR_DATA_INVALID,
               "#11836" /* all connections are degenerate */ );


    switch ( input_info->std_comps[(int)CONNECTIONS]->element_type )
    {
        case CUBES: flipq *= -1;  break;
        default: ;
    }

#if 0
    DXWarning ( "normal_direction = %d, flipq = %d", normal_direction, flipq );
#endif

    if ( normal_direction == flipq )
        for ( i=0, tp=(Triangle*)co_info->data;
              i<co_info->items;
              i++, tp++ )
        {
            save  = tp->p;
            tp->p = tp->q;
            tp->q = save;
        }

    if ( normal_direction == 1 )
    {
        for ( i=0, np=(Vector*) no_info->data;
              i<no_info->items;
              i++, np++ )
            if ( ( length = DXLength ( *np ) ) != 0.0 )  /* CONSTANT NORMALS */
            {
                np->x /= length;
                np->y /= length;
                np->z /= length;
            }
    }
    else
        for ( i=0, np=(Vector*) no_info->data;
              i< no_info->items;
              i++, np++ )
            if ( ( length = -DXLength ( *np ) ) != 0.0 )  /* CONSTANT NORMALS */
            {
                np->x /= length;
                np->y /= length;
                np->z /= length;
            }

    return OK;

    error:
        ERROR_SECTION;
        return ERROR;
}



static
grad_table_ptr create_special_gradient
             ( field_info      input_info,
               hash_table_ptr  vtx_hash,
               grad_table_ptr  grad )
{
    array_info    pi_info, ci_info, di_info = NULL;
    Cube          connection;
    Point         centroid = {0,0,0};
    Point         point[8];
    float         data[8];
    Vector        ccg;
    Point         d;
    Vector        *g[8];
    int           i, j, k;
    int           connection_shape;
    float         dist, invdist;

    InvalidComponentHandle  i_handle = NULL;

    pi_info = (array_info) &(input_info->std_comps[(int)POSITIONS]->array);
    ci_info = (array_info) &(input_info->std_comps[(int)CONNECTIONS]->array);
    di_info = (array_info) &(input_info->std_comps[(int)DATA]->array);

    if ( ( NULL  != input_info->std_comps[(int)INVALID_CONNECTIONS] ) &&
         ( ERROR == ( i_handle = DXCreateInvalidComponentHandle
                                     ( (Object)input_info->field,
                                       NULL, "connections" ) ) ) )
        goto error;

    DXASSERTGOTO ( pi_info->get_item != ERROR );
    DXASSERTGOTO ( ci_info->get_item != ERROR );
    DXASSERTGOTO ( di_info->get_item != ERROR );

    connection_shape = ci_info->shape[0];

    DXASSERTGOTO ( ( connection_shape == 4 ) || ( connection_shape == 8 ) );

    for ( i=0; i<ci_info->items; i++ )
        if ( ( NULL != i_handle ) && DXIsElementInvalid ( i_handle, i ) )
            continue;
        else if ( !ci_info->get_item ( i, ci_info, &connection ) )
            goto error;
        else
            for ( j=0; j<connection_shape; j++ )
#if 1
                if ( GETBIT ( vtx_hash->set_vertices, ((int*)&connection)[j] ) )
#else
                if ( 1 )
#endif
                {
                    for ( k=0; k<connection_shape; k++ )
                    {
                        if ( !di_info->get_item 
                                 ( ((int*)&connection)[k], di_info, &data[k] )
                             ||
                             !pi_info->get_item
                                 ( ((int*)&connection)[k], pi_info, &point[k]))
                            goto error;

                        if ( k == 0 )
                            centroid = point[0];
                        else
                        {
                            centroid.x += point[k].x;
                            centroid.y += point[k].y;
                            centroid.z += point[k].z;
                        }

                        if ( NULL ==
                                 ( g[k] = grad->index[((int*)&connection)[k]]) )
                        {
                            if ( ERROR ==
                                    ( g[k] = grad->index[((int*)&connection)[k]]
                                        = (Vector*) DXNewSegListItem 
                                                        ( grad->list ) ) )
                                goto error;
                            else
                                g[k]->x = g[k]->y = g[k]->z = 0.0;
                        }
                    }

/* XXX - dist was not used in this calculation? */

#define WEIGHTED_CONTRIBUTION(N) \
    d.x = centroid.x-point[N].x; d.y = centroid.y-point[N].y;\
    d.z = centroid.z-point[N].z; \
    dist = ABS(d.x) + ABS(d.y) + ABS(d.z);    \
    if (dist > DXD_MIN_FLOAT) { invdist = 1.0 / dist;   \
        g[N]->x += ccg.x * invdist;   \
        g[N]->y += ccg.y * invdist;   \
        g[N]->z += ccg.z * invdist; }
#if 0
    if (dist > 0.0) { \
        g[N]->x += ccg.x;   \
        g[N]->y += ccg.y;   \
        g[N]->z += ccg.z; }
    if (dist > _small_) { invdist = 1.0 / ( dist * dist );   \
        g[N]->x += ccg.x * invdist;   \
        g[N]->y += ccg.y * invdist;   \
        g[N]->z += ccg.z * invdist; }
#endif

                    if ( connection_shape == 4 )
                    {
                        centroid.x *= 0.25;
                        centroid.y *= 0.25;
                        centroid.z *= 0.25;

                        if ( !tetra_grad ( &point[0], &point[1],
                                           &point[2], &point[3],
                                           data[0], data[1],
                                           data[2], data[3], &ccg ) )
                            goto error;

                        WEIGHTED_CONTRIBUTION ( 0 ); 
                        WEIGHTED_CONTRIBUTION ( 1 ); 
                        WEIGHTED_CONTRIBUTION ( 2 ); 
                        WEIGHTED_CONTRIBUTION ( 3 ); 
                    }
                    else if ( connection_shape == 8 )
                    {
                        centroid.x *= 0.125;
                        centroid.y *= 0.125;
                        centroid.z *= 0.125;

                        /* PSUV=0356 */

                        if ( !tetra_grad ( &point[0], &point[3],
                                           &point[5], &point[6],
                                           data[0], data[3],
                                           data[5], data[6], &ccg ) )
                            goto error;

                        WEIGHTED_CONTRIBUTION ( 0 ); 
                        WEIGHTED_CONTRIBUTION ( 1 ); 
                        WEIGHTED_CONTRIBUTION ( 2 ); 
                        WEIGHTED_CONTRIBUTION ( 3 ); 
                        WEIGHTED_CONTRIBUTION ( 4 ); 
                        WEIGHTED_CONTRIBUTION ( 5 ); 
                        WEIGHTED_CONTRIBUTION ( 6 ); 
                        WEIGHTED_CONTRIBUTION ( 7 ); 
                    }
                    break /*j*/;
                }
#undef  WEIGHTED_CONTRIBUTION

    DXFreeInvalidComponentHandle ( i_handle );

    i_handle = NULL;

    return grad;

    error:
        ERROR_SECTION;

        DXFreeInvalidComponentHandle ( i_handle );

        i_handle = NULL;

        return ERROR;
}



/*
 * Special scan to remove geometrically degenerate connectives.
 *   Zero area triangles.
 *   (Zero length lines we'll ignore until problems are revealed)
 * 
 *   Comment: since this field was constructed by isosurface,
 *     we skip some integrity checks, about connection dependency and so on.
 *   XXX:
 *     since edge degeneration is coalesced, point compare by ID should be OK.
 *     (below its by geometric value)
 */
static
Field remove_degenerates ( Field in )
{
#if 1

    /* XXX - should not remove follow-ons, EG "color map" ? */

    /*
     * Remove unused points.
     */

    if ( DXEmptyField ( in ) )
        return in;

    if ( !DXInvalidateUnreferencedPositions ( (Object)in ) ||
         !DXCull                            ( (Object)in )   )
        return ERROR;

    return in;

#else

    array_info p_info;
    array_info c_info;

    component_info comp;

    Array array;

    int      *new_pts        = NULL;
    int      *new_tris       = NULL;
    Triangle *tri_base, *t_a = NULL;
    Point    *pts_base, *p_a = NULL;
    int      i, j;
    int      nto, npo;

    DXDebug ( "I", "start:RemoveDegenerates()" );

    /*
     * Initialize bad_pts to 0, bad_tris to 0, new_pts[i] to i.
     * Foreach tri
     *    if any two of the three points coincide then
     *        set higher of 2 points' new_pts to the other matching
     *        mark bad_pts of higher
     *        mark bad_tri
     * Foreach pt
     *    if not a bad_pt then
     *        Copy the coordinates up in memory
     *        Assign a unique ID for the point
     *    else
     *        Assign the ID of the coincident point
     * Set the component array lengths based on the new point count.
     * Foreach tri
     *    if not a bad_tri then
     *        set the three ID's based on new_pts
     * Set the connections component lengths based on the new triangle count.
     * Clean up.
     */

    /* XXX - QueryConnectivity ( in, 0 ) == HAS_TRIANGLES */

    if ( !DXEmptyField ( in->field ) )
    {
        p_info = (array_info) &(in->std_comps[(int)POSITIONS]->array);
        c_info = (array_info) &(in->std_comps[(int)CONNECTIONS]->array);

        tri_base = (Triangle *) c_info->data;
        pts_base = (Point *)    p_info->data;


#define SETUP_AREAS \
{if ((ERROR==(new_pts=(int*)DXAllocateLocal(p_info->items*sizeof(int))))\
 ||(ERROR==(new_tris=(int*)DXAllocateLocal(c_info->items*sizeof(int)))))\
    goto error; \
    DXDebug ( "I", "RemoveDegenerates: Doin' it at i=%d.", i ); \
    for ( j=0; j<c_info->items; j++ ) new_tris [ j ] = j; \
    for ( j=0; j<p_info->items; j++ ) new_pts  [ j ] = j; }


        for ( i=0, nto=c_info->items, t_a=tri_base;
              i<c_info->items;
              i++, t_a++ )
        {
            if ( 0 == memcmp ( (char *) &pts_base [ t_a->p ],
                               (char *) &pts_base [ t_a->q ], 
                               sizeof ( Point ) ) )
            {
                if ( new_tris == NULL ) SETUP_AREAS;

                new_tris [ i ] = -1;   nto--;

                if ( t_a->p < t_a->q ) new_pts [ t_a->q ] = t_a->p;
                else                   new_pts [ t_a->p ] = t_a->q;
            }

            if ( 0 == memcmp ( (char *) &pts_base [ t_a->q ],
                               (char *) &pts_base [ t_a->r ], 
                               sizeof ( Point ) ) )
            {
                if ( new_tris == NULL ) SETUP_AREAS;

                new_tris [ i ] = -1;   nto--;

                if ( t_a->q < t_a->r ) new_pts [ t_a->r ] = t_a->q;
                else                   new_pts [ t_a->q ] = t_a->r;
            }

            if ( 0 == memcmp ( (char *) &pts_base [ t_a->r ],
                               (char *) &pts_base [ t_a->p ], 
                               sizeof ( Point ) ) )
            {
                if ( new_tris == NULL ) SETUP_AREAS;

                new_tris [ i ] = -1;   nto--;

                if ( t_a->r < t_a->p ) new_pts [ t_a->p ] = t_a->r;
                else                   new_pts [ t_a->r ] = t_a->p;
            }
        }
#undef SETUP_AREAS

        if ( new_tris == NULL )
            return in;

#if 0
        Warning ( "Areas = %s", ( new_tris == NULL )? "NULL" : "Stuffed" );

        for ( i=0; i<c_info->items; i++ )
            if ( new_tris[i] != i ) 
                DXWarning
                   ( "bad[%d] = [%d,%d,%d], {[%g,%g,%g],[%g,%g,%g],[%g,%g,%g]}",
                     i,
                           tri_base[i].p, tri_base[i].q, tri_base[i].r,
                           pts_base [ tri_base[i].p ].x,
                           pts_base [ tri_base[i].p ].y,
                           pts_base [ tri_base[i].p ].z,
                           pts_base [ tri_base[i].q ].x,
                           pts_base [ tri_base[i].q ].y,
                           pts_base [ tri_base[i].q ].z,
                           pts_base [ tri_base[i].r ].x,
                           pts_base [ tri_base[i].r ].y,
                           pts_base [ tri_base[i].r ].z );
#endif

        for ( i=0, npo=0; i<p_info->items; i++ )
            if ( new_pts [ i ] == i )
                new_pts [ i ] = npo++;
            else
                new_pts [ i ] = new_pts [ new_pts [ i ] ];

        for ( i=0, nto=0, t_a=tri_base; i<c_info->items; i++, t_a++ )
            if ( new_tris [ i ] != -1 )
            {
                new_tris [ i ] = nto++;

                t_a->p = new_pts [ t_a->p ];
                t_a->q = new_pts [ t_a->q ];
                t_a->r = new_pts [ t_a->r ];
            }

        for ( j=0, comp=in->comp_list;   j<in->comp_count;   j++, comp++ )
        {
            if ( ( 0 == strcmp ( comp->name, "positions" ) ) ||
                 ( ( NULL  != comp->std_attribs[(int)DEP]        ) &&
                   ( ERROR != comp->std_attribs[(int)DEP]->value ) &&
                   ( 0 == strcmp ( comp->std_attribs[(int)DEP]->value,
                                   "positions" ) ) ) )
            {
                if ( ( ERROR == ( array = _dxf_resample_array
                                            ( comp, 1,
                                              p_info->items,
                                              npo, new_pts, NULL ) ) )
                     ||
                     !DXSetComponentValue
                         ( in->field, comp->name, (Object)array ) )
                    goto error;
            }
            else
            if ( ( 0 == strcmp ( comp->name, "connections" ) ) ||
                 ( ( NULL  != comp->std_attribs[(int)DEP]        ) &&
                   ( ERROR != comp->std_attribs[(int)DEP]->value ) &&
                   ( 0 == strcmp ( comp->std_attribs[(int)DEP]->value,
                                   "connections" ) ) ) )
                if ( ( ERROR == ( array = _dxf_resample_array
                                            ( comp, 1,
                                              c_info->items,
                                              nto, new_tris, NULL ) ) )
                     ||
                     !DXSetComponentValue
                         ( in->field, comp->name, (Object)array ) )
                    goto error;
        }

        DXFree ( (Pointer) new_tris );  new_tris = NULL;
        DXFree ( (Pointer) new_pts  );  new_pts  = NULL;
    }

    return in;

    error:
        ERROR_SECTION;

        DXFree ( (Pointer) new_tris );  new_tris = NULL;
        DXFree ( (Pointer) new_pts  );  new_pts  = NULL;

        return ERROR;
#endif
}



static
Field create_iso_surface
          ( field_info     input_info, 
            float          *value_ptr,
            int            is_grown,
            iso_norm_types normal_type,
            int            normal_direction,
            Interpolator   gradient_interpolator,
            Error (*convert_connection)
                         (void *, float *, hash_table_ptr, hash_table_ptr, int),
                      /* (void *, float *, hash_table_ptr, SegList *,    int),*/
            Field out )
{
    float           value = value_ptr [ 0 ];
    array_info      pi_info, ci_info, di_info  = NULL;
    array_info      po_info, co_info, no_info  = NULL;
    hash_table_rec  vtx_hash;
    grad_table_rec  grad_table;
    SegList         *tri_list = NULL;
    Cube            cube;
    Point           point[8];
    float           data[8];
    double          t;
    iso_edge_hash_ptr   element_ptr = NULL;
    int             i, j;
    field_info      output_info = NULL;
    int             position_dim;
    int             connection_count;
    int             connection_shape;
    Object          copy_g  = NULL;

    SegList         *c_ID_list = NULL;
    int             *c_ID_pull = NULL;
    int             n_c_dep, n_p_dep;
    array_info      a_info;
    component_info  comp;
    Array           array    = NULL;
    Pointer         scratch  = NULL;
    Pointer         out_data = NULL;

    InvalidComponentHandle  i_handle = NULL;

    DXASSERTGOTO ( ERROR != input_info );
    DXASSERTGOTO ( ERROR != value_ptr );
    DXASSERTGOTO ( ERROR != convert_connection );
    DXASSERTGOTO ( ERROR != out );

    vtx_hash.count        = 0;
    vtx_hash.table        = NULL;
    vtx_hash.set_vertices = NULL;
    grad_table.index      = NULL;
    grad_table.list       = NULL;

    pi_info = (array_info) &(input_info->std_comps[(int)POSITIONS]->array);
    ci_info = (array_info) &(input_info->std_comps[(int)CONNECTIONS]->array);
    di_info = (array_info) &(input_info->std_comps[(int)DATA]->array);

    if ( ( NULL  != input_info->std_comps[(int)INVALID_CONNECTIONS] ) &&
         ( ERROR == ( i_handle = DXCreateInvalidComponentHandle
                                     ( (Object)input_info->field,
                                       NULL, "connections" ) ) ) )
        goto error;

    DXASSERTGOTO ( pi_info->get_item != ERROR );
    DXASSERTGOTO ( ci_info->get_item != ERROR );
    DXASSERTGOTO ( di_info->get_item != ERROR );

    if ( ( pi_info->shape[0] != 3 ) )
        DXErrorGoto3
            ( ERROR_DATA_INVALID,
              "#11400", /* %s has invalid dimensionality; does not match %s */
              "\"positions\" component", "\"connections\"" );

    position_dim     = pi_info->shape[0];
    connection_shape = ci_info->shape[0];

    DXASSERTGOTO ( ( connection_shape == 4 ) || ( connection_shape == 8 ) );

    TST_HASH_INTS;

    if ( ( ERROR == ( vtx_hash.table
                          = DXCreateHash
                                ( sizeof(iso_edge_hash_rec),
                                  iso_edge_hash_func, iso_edge_compare_func )) )
         ||
         ( ERROR == ( tri_list
                          = DXNewSegList ( sizeof(Triangle), 1000, 1000 ) ) ) )
        goto error;

    if ( !tally_dependers ( input_info, &n_c_dep, &n_p_dep ) )
        goto error;

    if ( ( n_c_dep > 1 )
         &&
         ( ERROR == ( c_ID_list = DXNewSegList ( sizeof(int), 1000, 1000 ) ) ) )
        goto error;

    if ( normal_type == NORMALS_COMPUTED )
        if ( ( ERROR == ( grad_table.list
                              = DXNewSegList ( sizeof(Vector), 1000, 1000 ) ) ) 
             ||
             ( ERROR == ( grad_table.index = (Vector**)
                              DXAllocateLocalZero
                                  ( pi_info->items * sizeof(Vector*) ) ) )
             ||
             ( ERROR == ( vtx_hash.set_vertices
                              = NEWBIT ( pi_info->items ) ) ) )
            goto error;

    DXASSERTGOTO ( ci_info->items >= ci_info->original_items );

    if ( ci_info->get_item == _dxf_get_conn_grid_CUBES )
    {
        array_info  t0, t1, t2;
        int         II, JJ, KK;
        int         I0, J0, K0;

        t0 = (array_info)
         &(((struct _array_allocator *)((mesh_array_info)ci_info)->terms)[0]);

        t1 = (array_info)
         &(((struct _array_allocator *)((mesh_array_info)ci_info)->terms)[1]);

        t2 = (array_info)
         &(((struct _array_allocator *)((mesh_array_info)ci_info)->terms)[2]);

        DXASSERTGOTO ( ((mesh_array_info)ci_info)->grid == TTRUE );
        DXASSERTGOTO ( ((grid_connections_info)ci_info)->counts != NULL );
        DXASSERTGOTO
           ( t0->items == ( ((grid_connections_info)ci_info)->counts[0] - 1 ) );
        DXASSERTGOTO
           ( t1->items == ( ((grid_connections_info)ci_info)->counts[1] - 1 ) );
        DXASSERTGOTO
           ( t2->items == ( ((grid_connections_info)ci_info)->counts[2] - 1 ) );

        if ( ((mesh_array_info)ci_info)->grow_offsets != NULL )
        {
            DXASSERTGOTO ( is_grown );

            I0 = ((mesh_array_info)ci_info)->grow_offsets[0];
            J0 = ((mesh_array_info)ci_info)->grow_offsets[1];
            K0 = ((mesh_array_info)ci_info)->grow_offsets[2];
        }
        else
            I0 = J0 = K0 = 0;

        for ( II=I0;
              II<( I0 + t0->original_items );
              II++ )
            for ( JJ=J0;
                  JJ<( J0 + t1->original_items );
                  JJ++ )
            {
                if ( !ci_info->get_item 
                          ( (((( II * t1->items ) + JJ ) * t2->items ) + K0 ),
                            ci_info, &cube )
                     ||
                     !di_info->get_item ( cube.p, di_info, &data[0] ) ||
                     !di_info->get_item ( cube.r, di_info, &data[2] ) ||
                     !di_info->get_item ( cube.t, di_info, &data[4] ) ||
                     !di_info->get_item ( cube.v, di_info, &data[6] )   )
                    goto error;

                data[0] -= value;
                data[2] -= value;
                data[4] -= value;
                data[6] -= value;

                for ( KK=K0;
                      KK<( K0 + t2->original_items );
                      KK++,
                            cube.p++, cube.q++, cube.r++, cube.s++,
                            cube.t++, cube.u++, cube.v++, cube.w++,
                            data[0] = data[1],  data[2] = data[3],
                            data[4] = data[5],  data[6] = data[7] )
                {
                    if ( !di_info->get_item ( cube.q, di_info, &data[1] ) ||
                         !di_info->get_item ( cube.s, di_info, &data[3] ) ||
                         !di_info->get_item ( cube.u, di_info, &data[5] ) ||
                         !di_info->get_item ( cube.w, di_info, &data[7] )   )
                        goto error;

                    data[1] -= value;
                    data[3] -= value;
                    data[5] -= value;
                    data[7] -= value;

                    if ( ( NULL != i_handle )
                         &&
                         DXIsElementInvalid
                            ( i_handle, ((((II*t1->items)+JJ)*t2->items)+KK) ) )
                        continue;
                    else if ( !convert_connection
                                  ( &cube, data, &vtx_hash, 
                                    (hash_table_ptr) tri_list,
                                    /* XXX parameter 4 */
                                    /* equivalence */ 1 ) )
                        goto error;
                    else if ( ( NULL != c_ID_list )
                              &&
                              !set_c_ID
                                   ( c_ID_list,
                                     DXGetSegListItemCount ( tri_list ),
                                     ((((II*t1->items)+JJ)*t2->items)+KK) ) )
                        goto error;
                }
            }
    }
    else
    {
        for ( i=0; i<ci_info->original_items; i++ )
            if ( ( NULL != i_handle ) && DXIsElementInvalid ( i_handle, i ) )
                continue;
            else if ( !ci_info->get_item ( i, ci_info, &cube ) )
                goto error;
            else
            {
                for ( j=0; j<connection_shape; j++ )
                {
                    if ( !di_info->get_item
                              ( ((int *)&cube)[j], di_info, &data[j] ) )
                        goto error;
                    data[j] -= value;
                }

                if ( !convert_connection
                          ( &cube, data, &vtx_hash,
                            (hash_table_ptr) tri_list, /* XXX parameter 4 */
                            /* equivalence */ 1 ) )
                    goto error;
                else if ( ( NULL != c_ID_list )
                          &&
                          !set_c_ID
                               ( c_ID_list,
                                 DXGetSegListItemCount ( tri_list ), i ) )
                    goto error;
            }
    }

    connection_count = DXGetSegListItemCount ( tri_list );

    if ( connection_count == 0 ) {
        if ( !_dxf_MakeFieldEmpty ( out ) )
            goto error;
        else
            goto done;
    }

    if ( normal_type == NORMALS_COMPUTED )
    {
        if ( !create_output_arrays
                  ( out,
                    vtx_hash.count,        position_dim,
                    connection_count,      2,
                    vtx_hash.count,
                    DEFAULT_SURFACE_COLOR,
                    value,                 DEP_POSITIONS,
                    ISOSURFACE ) )
            goto error;

        if ( vtx_hash.count == 0 ) 
            goto done;

        if ( !create_special_gradient ( input_info, &vtx_hash, &grad_table ) )
            goto error;
    }
    else
    {
        if ( !create_output_arrays
                  ( out,
                    vtx_hash.count,        position_dim,
                    connection_count,      2,
                    0,
                    DEFAULT_SURFACE_COLOR,
                    value,                 DEP_POSITIONS,
                    ISOSURFACE ) )
            goto error;

        if ( vtx_hash.count == 0 ) 
            goto done;
    }

    if ( ( connection_count > 0 ) && ( c_ID_list != NULL ) )
    {
        if ( ( ERROR == ( c_ID_pull = DXAllocateLocal
                                          ( connection_count * sizeof(int) ) ) )
             ||
             !copy_seglist_to_memory
                  ( c_ID_list, c_ID_pull, sizeof(int), connection_count ) )
            goto error;
    
        DXDeleteSegList ( c_ID_list );  c_ID_list = NULL;
    }

    if ( ERROR == ( output_info = ( _dxf_InMemory ( out ) ) ) )
        goto error;

    DXASSERTGOTO ( output_info->std_comps[(int)POSITIONS]   != NULL );
    DXASSERTGOTO ( output_info->std_comps[(int)CONNECTIONS] != NULL );

    po_info = (array_info) &(output_info->std_comps[(int)POSITIONS]->array);
    co_info = (array_info) &(output_info->std_comps[(int)CONNECTIONS]->array);
    no_info = ( normal_type == NORMALS_COMPUTED )?
              (array_info) &(output_info->std_comps[(int)NORMALS]->array)
              : NULL;

    if ( !copy_seglist_to_array ( tri_list, co_info ) )
        goto error;

    DXDeleteSegList ( tri_list ); tri_list = NULL;

    for ( j=0, comp=input_info->comp_list;
          j<input_info->comp_count;
          j++, comp++ )
    {

        if ( ( 0 != strcmp ( comp->name, "positions" ) ) &&
             ( ( NULL  == comp->std_attribs[(int)DEP]        ) ||
               ( ERROR == comp->std_attribs[(int)DEP]->value ) ||
               ( 0 != strcmp ( comp->std_attribs[(int)DEP]->value,
                               "positions" ) ) ) )
            continue;

        if ( ( 0 == strcmp  ( comp->name, "data"        ) ) ||
             ( 0 == strncmp ( comp->name, "original", 8 ) ) ||
             ( 0 == strcmp  ( comp->name, "invalid positions" ) ) ||
             ( 0 == strcmp  ( comp->name, "invalid connections" ) ) ||
             ( NULL != comp->std_attribs[(int)REF]        )  )
            continue;

        if ( ( 0 == strcmp ( comp->name, "normals" ) ) &&
             ( normal_type != NO_NORMALS )               )
            continue;

        a_info = (array_info) &(comp->array);

    if ( CLASS_CONSTANTARRAY == a_info->class )
    {
        if ( ( ERROR == ( array = (Array)DXNewConstantArrayV
                                     ( vtx_hash.count,
                                       DXGetConstantArrayData ( a_info->array ),
                                       a_info->type,
                                       a_info->category,
                                       a_info->rank,
                                       a_info->shape ) ) )
             ||
             !DXSetComponentValue ( out, comp->name, (Object) array )
             ||
             !DXSetComponentAttribute
                  ( out, comp->name, "dep",
                    (Object) DXNewString ( "positions" ) ) )

            goto error;

        continue;
    }

        if ( ( ERROR == ( array
                   = DXNewArrayV
                         ( a_info->type, a_info->category,
                           a_info->rank, &a_info->shape[0] ) ) )
             ||
             !DXAllocateArray     ( array, vtx_hash.count )           ||
             !DXAddArrayData      ( array, 0, vtx_hash.count, NULL )  ||
             !DXTrim              ( array )                           ||
             !DXSetComponentValue ( out, comp->name, (Object) array )
             ||
             !DXSetComponentAttribute
                  ( out, comp->name, "dep",
                    (Object) DXNewString ( "positions" ) )
             ||
             !_dxf_SetIterator    ( comp )
             ||
             ( ERROR == ( out_data = DXGetArrayData  ( array )))             ||
             ( ERROR == ( scratch  = DXAllocateLocal ( 2 * a_info->itemsize )))
             ||
             !DXInitGetNextHashElement ( vtx_hash.table ) )
            goto error;

        array = NULL;

        for ( i = 0;
              ( NULL != ( element_ptr
                              = (iso_edge_hash_ptr) DXGetNextHashElement
                                                    ( vtx_hash.table ) ) );
              i++ )
        {
            DXASSERTGOTO ( element_ptr->seq < po_info->items );

            if ( element_ptr->edge.p == element_ptr->edge.q )
                t = 0.0;
            else
            {
                if ( !di_info->get_item
                         ( element_ptr->edge.p, di_info, &data[0] )||
                     !di_info->get_item
                         ( element_ptr->edge.q, di_info, &data[1] ) )
                    goto error;

                data[0] -= value;
                data[1] -= value;

                if      ( data[0] == 0.0 ) t = 0.0;
                else if ( data[1] == 0.0 ) t = 1.0;
                else {
                    t = (double)data[0] / ((double)data[0] - (double)data[1]);
		}

                DXASSERTGOTO ( ( t >= 0.0 ) && ( t <= 1.0 ) );
            }

            if ( !interp ( a_info, scratch,
                           element_ptr->edge.p, element_ptr->edge.q, t, 
                           &(((char*)out_data)
                               [ a_info->itemsize * element_ptr->seq ]),
			       a_info->itemsize/sizeof(float),
			       a_info->itemsize) )
                goto error;

            if ( ( a_info == pi_info ) && ( normal_type == NORMALS_COMPUTED ) )
            {
                DXASSERTGOTO ( grad_table.index [ element_ptr->edge.p ] );
                DXASSERTGOTO ( grad_table.index [ element_ptr->edge.q ] );

                point[0] = *grad_table.index [ element_ptr->edge.p ];
                point[1] = *grad_table.index [ element_ptr->edge.q ];

                point[0].x += ( point[1].x - point[0].x ) * t;
                point[0].y += ( point[1].y - point[0].y ) * t;
                point[0].z += ( point[1].z - point[0].z ) * t;

                ((Vector *)no_info->data) [ element_ptr->seq ] = point[0];
            }
        }

        DXFree ( scratch );  scratch = NULL;
    }


    for ( j=0, comp=input_info->comp_list;
          j<input_info->comp_count;
          j++, comp++ )
    {
        if ( ( NULL  == comp->std_attribs[(int)DEP]        ) ||
             ( ERROR == comp->std_attribs[(int)DEP]->value ) ||
             ( 0 != strcmp ( comp->std_attribs[(int)DEP]->value,
                             "connections" ) ) )
            continue;

        if ( ( 0 == strcmp  ( comp->name, "connections" ) ) ||
             ( 0 == strncmp ( comp->name, "original", 8 ) ) ||
             ( NULL != comp->std_attribs[(int)REF]        )  )
            continue;

        a_info = (array_info) &(comp->array);

        if ( ( ERROR == ( array = _dxf_resample_array
                                      ( comp, 1,
                                        ((array_info) &(comp->array))->items,
                                        connection_count, NULL, c_ID_pull ) ) )
             ||
             !DXSetComponentValue ( out, comp->name, (Object)array )
             ||
             !DXSetComponentAttribute
                  ( out, comp->name, "dep",
                    (Object) DXNewString ( "connections" ) ) )
            goto error;
    }

    _dxf_FreeInMemory ( output_info );

    if ( ERROR == ( output_info = _dxf_InMemory ( out ) ) )
        goto error;

    switch ( normal_type )
    {
        case GRADIENT_NORMALS:
        {
            Object inv_bef = DXExists ( (Object) out, "invalid positions" );

            if ( ( ERROR == ( copy_g
                       = DXCopy ( (Object) gradient_interpolator,
                                  COPY_STRUCTURE ) ) )
                 ||
                 !DXMap ( (Object) out, copy_g, "positions", "normals" ) )
                goto error;

            if ( !inv_bef && DXExists ( (Object) out, "invalid positions" ) )
            {
            DXWarning
            ( "Isosurface: Mapping external gradient reveals invalid points." );
            DXWarning
               ( "Isosurface: Check that external gradient encompasses data." );
            }

            _dxf_FreeInMemory ( output_info );
            DXDelete          ( copy_g );
            copy_g            = NULL;

            if ( ERROR == ( output_info = ( _dxf_InMemory ( out ) ) ) )
                goto error;
        }

        case NORMALS_COMPUTED:
            if ( !flip_normals_and_triangles
                      ( input_info,
                        output_info,
                        normal_direction ) )
                goto error;
            break;

        case NO_NORMALS:
            break;
    }

    /* Do this last since it invalidates (( output_info ))  */
    if ( !remove_degenerates ( out ) )
        goto error;

    done:
        /* This is create_iso_surface */
        if ( DXGetAttribute ( (Object)input_info->field, "shade" ) )
            if ( !DXSetAttribute
                      ( (Object) out,
                        "shade",
                        DXGetAttribute
                            ( (Object)input_info->field, "shade" ) ) )
                goto error;

        DXDestroyHash     ( vtx_hash.table        );
        DXFree            ( vtx_hash.set_vertices );
        DXFree            ( (Pointer) grad_table.index );
        DXDeleteSegList   ( grad_table.list       ); 
        DXDeleteSegList   ( tri_list              );
        DXDeleteSegList   ( c_ID_list );
        DXFree            ( c_ID_pull );
        DXFree            ( scratch );

        _dxf_FreeInMemory ( output_info );

        DXFreeInvalidComponentHandle ( i_handle );

        i_handle              = NULL;
        vtx_hash.table        = NULL;
        vtx_hash.set_vertices = NULL;
        grad_table.index      = NULL;
        grad_table.list       = NULL;
        output_info           = NULL;
        tri_list              = NULL;
        c_ID_list             = NULL;
        c_ID_pull             = NULL;
        scratch               = NULL;

        return DXEndField ( out );

    error:
        ERROR_SECTION;

        DXDestroyHash     ( vtx_hash.table        );
        DXFree            ( vtx_hash.set_vertices );
        DXFree            ( (Pointer) grad_table.index );
        DXDeleteSegList   ( grad_table.list       ); 
        DXDeleteSegList   ( tri_list              );
        DXDeleteSegList   ( c_ID_list );
        DXFree            ( c_ID_pull );
        DXFree            ( scratch );

        _dxf_FreeInMemory ( output_info );

        DXFreeInvalidComponentHandle ( i_handle );

        i_handle              = NULL;
        vtx_hash.table        = NULL;
        vtx_hash.set_vertices = NULL;
        grad_table.index      = NULL;
        grad_table.list       = NULL;
        output_info           = NULL;
        tri_list              = NULL;
        c_ID_list             = NULL;
        c_ID_pull             = NULL;
        scratch               = NULL;

        return ERROR;
}



static
Error isosurface_field
          ( Field          input,
            int            number,
            float          *values,
            float          *min_max,
            iso_module_types module,
            band_remap_types remap,
            Field          *out_h,
            int            *skipvec,
            iso_norm_types normal_type,
            Interpolator   gradient_interpolator,
            int            is_grown,
            int            normal_direction )
{
    int           i;
    field_info    input_info = NULL;
    array_info    d_info = NULL;

    Field (*create_output)      () = NULL;
    Error (*convert_connection) () = NULL;

#ifdef WHITEBOX_TEST
    for ( i=0; i<8;   i++ ) histograms.polygonalizations[i] = 0;
    for ( i=0; i<256; i++ ) histograms.case_signatures  [i] = 0;
#endif

    DXASSERTGOTO ( input != ERROR );
    DXASSERTGOTO ( !DXEmptyField ( input ) );
    DXASSERTGOTO ( DXGetObjectClass ( (Object)input ) == CLASS_FIELD );

    if ( !DXInvalidateConnections ( (Object) input )
         ||
         ( ERROR == ( input_info = _dxf_InMemory ( input ) ) ) )
        goto error;

    if ( input_info->std_comps[(int)POSITIONS] == NULL )
        DXErrorGoto3
            ( ERROR_DATA_INVALID,
              "#10250", /*%s is missing %s component*/
              "'data' parameter",
              "\"positions\"" );

    if ( input_info->std_comps[(int)CONNECTIONS] == NULL )
        DXErrorGoto3
            ( ERROR_DATA_INVALID,
              "#10250", /*%s is missing %s component*/
              "'data' parameter",
              "\"connections\"" );

    if ( input_info->std_comps[(int)DATA] == NULL )
        DXErrorGoto3
            ( ERROR_DATA_INVALID,
              "#10250", /*%s is missing %s component*/
              "'data' parameter",
              "\"data\"" );

    if ( ( input_info->std_comps[(int)DATA]->std_attribs[(int)DEP] == NULL )
         ||
         ( strcmp 
              ( input_info->std_comps[(int)DATA]->std_attribs[(int)DEP]->value,
                "positions" ) != 0 ) )
        DXErrorGoto2
            ( ERROR_DATA_INVALID,
              "#11251", /* %s must be dependent on positions */
              "the \"data\" component" );

    /*p_info = (array_info) &(input_info->std_comps[(int)POSITIONS]->array);*/
    /*c_info = (array_info) &(input_info->std_comps[(int)CONNECTIONS]->array);*/
    d_info = (array_info) &(input_info->std_comps[(int)DATA]->array);

    if ( ( d_info->rank != 0 )
         &&
         ( ( d_info->rank != 1 ) || ( d_info->shape[0] != 1 ) ) )
        DXErrorGoto2
            ( ERROR_DATA_INVALID,
              "#10081", /* %s must be scalar or 1-vector */
              "\"data\" component" );

    if ( !_dxf_SetIterator   ( input_info->std_comps[(int)POSITIONS]   ) ||
         !_dxf_SetIterator   ( input_info->std_comps[(int)CONNECTIONS] ) ||
         !_dxf_SetIterator_i ( input_info->std_comps[(int)DATA]        )   )
        goto error;

    switch ( module )
    {
        case ISOSURFACE:
            switch ( input_info->std_comps[(int)CONNECTIONS]->element_type )
            {
                case LINES:
                    create_output      = create_iso_points;
                    convert_connection = NULL;
                    break;

                case QUADRILATERALS:
                    create_output      = create_iso_contours;
                    convert_connection = iso_convert_quadrilateral;
                    break;

                case TRIANGLES:
                    create_output      = create_iso_contours;
                    convert_connection = iso_convert_triangle;
                    break;

                case CUBES:
                    create_output      = create_iso_surface;
                    convert_connection = iso_convert_cube;
                    break;

                case TETRAHEDRA:
                    create_output      = create_iso_surface;
                    convert_connection = iso_convert_tetrahedron;
                    break;

                case PRISMS:
                    create_output      = create_iso_surface;
                    convert_connection = iso_convert_prism;
                    break;

                case CUBES4D:
                case NONE:
                case ET_UNKNOWN:
                default:
                    DXErrorGoto2
                        ( ERROR_DATA_INVALID,
                          "#11380", /* %s is an invalid connections type */
                          input_info->std_comps[(int)CONNECTIONS]->name );
            }
            break;

#ifdef INCLUDES_BAND
        case BAND:
            switch ( input_info->std_comps[(int)CONNECTIONS]->element_type )
            {
                case LINES:
                    create_output      = create_band_lines;
                    convert_connection = band_convert_line;
                    break;

                case QUADRILATERALS:
                    create_output      = create_band_surface;
                    convert_connection = band_convert_quadrilateral;
                    break;

                case TRIANGLES:
                    create_output      = create_band_surface;
                    convert_connection = band_convert_triangle;
                    break;

                case CUBES:
                case TETRAHEDRA:
                case PRISMS:
                case CUBES4D:
                case NONE:
                case ET_UNKNOWN:
                default:
                    DXErrorGoto2
                        ( ERROR_DATA_INVALID,
                          "#11380", /* %s is an invalid connections type */
                          input_info->std_comps[(int)CONNECTIONS]->
                              std_attribs[DX_ELEMENT_TYPE]->value );
            }
#endif
    }

    if ( ( create_output == create_iso_points   ) ||
         ( create_output == create_iso_contours )   )
    {
        for ( i=0; i<number; i++ )
            if ( !skipvec[i] )
            {
                out_h[i] = create_output
                               ( input_info,
                                 &values[i],
                                 is_grown,
                                 convert_connection,
                                 out_h[i] );

                if ( DXGetError() != ERROR_NONE )
                    goto error;

		if (! DXCopyAttributes((Object)out_h[i], (Object)input_info->field))
		    goto error;
            }
    }
    else if ( create_output == create_iso_surface )
    {
        /* XXX - create and preserve gradient between accesses */

        for ( i=0; i<number; i++ )
            if ( !skipvec[i] )
            {
                out_h[i] = create_iso_surface
                               ( input_info,
                                 &values[i],
                                 is_grown,
                                 normal_type,
                                 normal_direction,
                                 gradient_interpolator,
                                 convert_connection,
                                 out_h[i] );

                if ( DXGetError() != ERROR_NONE )
                    goto error;

		if (! DXCopyAttributes((Object)out_h[i], (Object)input_info->field))
		    goto error;
            }
    }
    else if ( ( create_output == create_band_lines   ) ||
              ( create_output == create_band_surface )  )
#if 0
              ( create_output == create_band_volume  ) 
#endif
    {
        out_h[0] = create_output
                       ( input_info,
                         values, number,
                         remap,
                         is_grown,
                         min_max,
                         convert_connection,
                         out_h[0] );

        if ( DXGetError() != ERROR_NONE )
            goto error;

	if (! DXCopyAttributes((Object)out_h[0], (Object)input_info->field))
	    goto error;
    }
    else
        goto error;

#ifdef WHITEBOX_TEST
    if ( DXQueryDebug ( "I" ) )
    { 
        int hit;

    DXDebug ( "I",
         "Poly frequency: [2]:%6d, [3]:%6d, [4]:%6d, [5]:%6d, [6]:%6d, [7]:%6d",
              histograms.polygonalizations[2],
              histograms.polygonalizations[3],
              histograms.polygonalizations[4],
              histograms.polygonalizations[5],
              histograms.polygonalizations[6],
              histograms.polygonalizations[7] );

    for ( i=0, hit=0;
          i<256;
          hit += ( histograms.case_signatures [ i++ ] == 0 )? 0: 1 );

    DXDebug ( "I", "Case hits: %d, misses %d", hit, (256-hit) );

#if 0
    DXDebug ( "I", "Case frequencies:" );

    for ( i=0; i<256; i+=8 )
        DXDebug ( "i",
                  "   [%03d]: %7d %7d %7d %7d %7d %7d %7d %7d",
                  i,
                  histograms.case_signatures[i],
                  histograms.case_signatures[i+1],
                  histograms.case_signatures[i+2],
                  histograms.case_signatures[i+3],
                  histograms.case_signatures[i+4],
                  histograms.case_signatures[i+5],
                  histograms.case_signatures[i+6],
                  histograms.case_signatures[i+7] );
#endif

    } 
#endif

    if ( !_dxf_FreeInMemory ( input_info ) ) goto error;   input_info = NULL;

    return OK;

    error:
        ERROR_SECTION;

        _dxf_FreeInMemory ( input_info );   input_info = NULL;

        return ERROR;
}


static
int is_volume_topology ( CompositeField cf )
{
    Field child;
    char  *type;
    int   i;
    enum { dont_know, is_volume, isnt_volume } topo = dont_know;

    DXASSERTGOTO ( DXGetObjectClass ( (Object) cf ) == CLASS_GROUP          );
    DXASSERTGOTO ( DXGetGroupClass  ( (Group)  cf ) == CLASS_COMPOSITEFIELD );

    i = 0;
    while ( topo == dont_know )
    {
        if ( ERROR == ( child = (Field)DXGetEnumeratedMember
                                           ( (Group)cf, i++, NULL ) ) )
            DXErrorGoto
                ( ERROR_DATA_INVALID,
                  "CompositeField has no members containing \"connections\"" );

        if ( DXGetObjectClass ( (Object) child ) != CLASS_FIELD )
            DXErrorGoto
                ( ERROR_DATA_INVALID,
                  "CompositeField contains a member that is not a Field" );

        if ( NULL != DXGetComponentValue ( child, "connections" ) )
        {
            type = DXGetString
                       ( (String) DXGetComponentAttribute
                                    ( child, "connections", "element type" ) );

            if ( ( ERROR == type ) || ( DXGetError() != ERROR_NONE ) )
                goto error;

            if ( ( 0 == strcmp ( type, "cubes"      ) ) ||
                 ( 0 == strcmp ( type, "pyramids"   ) ) ||
                 ( 0 == strcmp ( type, "tetrahedra" ) )   )

                topo = is_volume;
            else
                topo = isnt_volume;
        }
    }

    switch ( topo )
    {
        case is_volume:    return 1;
        case isnt_volume:  return 0;
        case dont_know:
            DXSetError
                ( ERROR_DATA_INVALID,
                  "#11380", /* %s is an invalid connections type */
                  "composite field member" );
            goto error;
    }

    error:
        ERROR_SECTION;
        return ERROR;
}


/*
 * If any ERROR, all ERROR
 * else If any OK, all OK
 * else EMPTY
 */
typedef enum { OBJ_ERROR, OBJ_OK, OBJ_EMPTY } type_check;

static
type_check iso_object_type_check ( Object o )
{
    Type     type;
    Category cat;
    int      rank;
    int      shape;
    Object   child;
    int      i;

    DXASSERT ( o != NULL );

    if ( ( DXGetObjectClass ( o ) != CLASS_XFORM   ) &&
         ( DXGetObjectClass ( o ) != CLASS_SCREEN  ) &&
         ( DXGetObjectClass ( o ) != CLASS_CLIPPED )
         &&
         DXGetType ( o, &type, &cat, &rank, NULL  ) )
    {
        if ( ( cat == CATEGORY_REAL )
             &&
             ( ( rank == 0 )
               ||
               ( ( rank == 1 )
                 &&
                 DXGetType ( o, NULL, NULL, NULL, &shape )
                 &&
                 ( shape == 1 ) ) ) )

            return OBJ_OK;
        else
            DXErrorGoto2
                ( ERROR_DATA_INVALID, 
                  "#10081", /* %s must be scalar or 1-vector */
                  "\"data\" component" );
    }
    else if ( DXGetError() != ERROR_NONE )
        goto error;

    else
        switch ( DXGetObjectClass ( o ) )
        {
            case CLASS_FIELD:
                if ( DXEmptyField ( (Field) o ) )
                    return OBJ_EMPTY;
                else
                    DXErrorGoto3
                        ( ERROR_DATA_INVALID,
                          "#10250", /* %s is missing %s component */
                          "Field (non-empty)", "\"data\"" );

            case CLASS_XFORM:
                if ( !DXGetXformInfo ( (Xform)o, &child, NULL ) )
                    goto error;

                switch ( iso_object_type_check ( child ) )
                {
                    case OBJ_OK:    return OBJ_OK;
                    case OBJ_EMPTY: return OBJ_EMPTY;
                    case OBJ_ERROR: goto error;
                }
                break;

            case CLASS_SCREEN:
                if ( !DXGetScreenInfo ( (Screen)o, &child, NULL, NULL ) )
                    goto error;

                switch ( iso_object_type_check ( child ) )
                {
                    case OBJ_OK:    return OBJ_OK;
                    case OBJ_EMPTY: return OBJ_EMPTY;
                    case OBJ_ERROR: goto error;
                }
                break;

            case CLASS_CLIPPED:
                if ( !DXGetClippedInfo ( (Clipped)o, &child, NULL ) )
                    goto error;

                switch ( iso_object_type_check ( child ) )
                {
                    case OBJ_OK:    return OBJ_OK;
                    case OBJ_EMPTY: return OBJ_EMPTY;
                    case OBJ_ERROR: goto error;
                }
                break;

            case CLASS_GROUP:
            {
                int Nok = 0;

                for ( i=0;
                      NULL != ( child = DXGetEnumeratedMember
                                            ( (Group) o, i, NULL ) );
                      i++ )
                    switch ( iso_object_type_check ( (Object) child ) )
                    {
                        case OBJ_OK:    Nok++; break;
                        case OBJ_EMPTY:        break;
                        case OBJ_ERROR:   goto error;
                    }

                return ( Nok > 0 ) ? OBJ_OK : OBJ_EMPTY;
            }
                break;

            default:
                DXErrorGoto2
                    ( ERROR_DATA_INVALID,
                      "#10190", /* %s must be a field or a group */
                      "'data' parameter" );
        }

    error:
        ERROR_SECTION;
        return OBJ_ERROR;
}


static
/*
 * Special return codes:  Pointer and NULL are both acceptable as
 * non-error conditions.  when DXGetError() signifies that there is error,
 * then there is one.
 * 
 * XXX - Important:
 *    Statistics will give an error if called with an DXEmptyField
 *    or a group containing no fields with "data" components
 *      (as with EmptyFields)
 */
iso_arg_type * stats ( iso_arg_type *iso_arg, float **deletable )
{
    float  min, mean, max;
    int    i;

    DXASSERTGOTO ( deletable != NULL );

    *deletable = NULL;

    if ( ( !iso_arg->have_minmax )
         &&
         ( DXGetObjectClass ( iso_arg->self ) == CLASS_GROUP )
         &&
         ( DXGetGroupClass ( (Group) iso_arg->self ) == CLASS_COMPOSITEFIELD ) )
    {
        switch ( iso_object_type_check ( iso_arg->self ) )
        {
            case OBJ_EMPTY: return iso_arg; /* XXX - This is a hack */
            case OBJ_OK:    break;
            case OBJ_ERROR: goto error;
        }

        if ( !DXStatistics
                  ( iso_arg->self,
                    "data",
                    &iso_arg->band_min_cf,
                    &iso_arg->band_max_cf,
                    NULL, NULL ) )
            goto error;

        iso_arg->have_minmax = 1;
    }

    if ( iso_arg->isovals )
        return iso_arg;

    if ( _dxd_user_def_values )
    {
        iso_arg->isovals = _dxd_user_def_values;
        return iso_arg;
    }

    if ( ( DXGetObjectClass ( iso_arg->self ) == CLASS_GROUP )
         &&
         ( DXGetGroupClass ( (Group) iso_arg->self ) == CLASS_GROUP ) )
        return iso_arg;
        /* No error:  Isovalues are not evaluated over generic groups */

    switch ( iso_object_type_check ( iso_arg->self ) )
    {
        case OBJ_EMPTY: return iso_arg; /* XXX - This is a hack */
        case OBJ_OK:    break;
        case OBJ_ERROR: goto error;
    }

    if ( !DXStatistics ( iso_arg->self, "data", &min, &max, &mean, NULL )
         ||
         ( ERROR == ( iso_arg->isovals 
                = (float*) DXAllocate
                              ( sizeof(float) * VALUE_COUNT ( iso_arg ) ) ) ) )
        goto error;

    *deletable = iso_arg->isovals;

    if ( iso_arg->use_mean )
    {
        iso_arg->isovals[0] = mean;

        DXMessage ( "%s: 'value' defaults to mean: %g .",
                    MOD_NAME ( iso_arg ), iso_arg->isovals[0] );
    }
    else if ( VALUE_COUNT(iso_arg) > 0 )
    {
        for ( i=0; i<VALUE_COUNT(iso_arg); i++ )
            iso_arg->isovals[i] = min + ( ( max - min )
                               * ( (float) ( i + 1 ) ) /
                                 ( (float) ( VALUE_COUNT(iso_arg) + 1 ) ) );

        if ( VALUE_COUNT ( iso_arg ) == 1 )
            DXMessage
                ( "%s: 'value' is %g .",
                  MOD_NAME ( iso_arg ),
                  iso_arg->isovals[0] );
        else
            DXMessage
                ( "%s: 'value' is %d values ranging from %g to %g .",
                  MOD_NAME ( iso_arg ),
                  VALUE_COUNT ( iso_arg ),
                  iso_arg->isovals [ 0 ],
                  iso_arg->isovals [ VALUE_COUNT ( iso_arg ) - 1 ] );
    }

    return iso_arg;

    error:
        ERROR_SECTION;

        DXFree ( (Pointer) *deletable );

        *deletable = NULL;

        return ERROR;
}


#define SavedCopiesMaxDEF 1024

static struct
{
    CompositeField cf [ SavedCopiesMaxDEF ];
    int            used;
}
SavedCopies;

#define init_saved_copies_DEF \
 { SavedCopies.used = -1; }

#define free_saved_copies_DEF \
    {while(SavedCopies.used>-1) \
         DXDelete((Object)SavedCopies.cf[SavedCopies.used--]);}


static
CompositeField copy_grow_save ( CompositeField input )
{
    char    *growl[100];
    char    *dep;
    int     ll=0;
    int     i, j;
    Field   f;
    Object  a;

    CompositeField  copy  = NULL;

    for ( i=0;
          ( NULL != ( f = (Field)DXGetEnumeratedMember 
                                     ( (Group)input, i, NULL ) ) );
          i++ )

        if ( !DXEmptyField ( f ) )
        {
            for ( j=0, ll=0;
                  ( NULL != ( a = DXGetEnumeratedComponentValue
                                      ( f, j, &growl[ll] ) ) );
                  j++ )
            {
                if ( ll >= 100 )
                    DXErrorGoto
                        ( ERROR_UNEXPECTED,
                         "more than 100 components to grow in copy_grow_save" );

                if ( NULL != growl[ll] ) {

                    if ( ( 0 == strcmp ( growl[ll], "positions"   ) ) ||
                         ( 0 == strcmp ( growl[ll], "connections" ) ) ||
                         ( 0 == strcmp ( growl[ll], "data"        ) )  )

                        ll++;

                    else if ( ( NULL != ( a = DXGetAttribute ( a, "dep" ) ) )
                              &&
                              ( CLASS_STRING == DXGetObjectClass ( a ) )
                              &&
                              ( NULL != ( dep = DXGetString ( (String)a ) ) )
                              &&
                              ( ( 0 == strcmp ( dep, "positions"   ) ) ||
                                ( 0 == strcmp ( dep, "connections" ) )  ) )

                        ll++;
                   }
            }

            break;
        }

    growl [ ll ] = NULL;

    if ( DXQueryDebug ( "I" ) )
    {
        DXDebug ( "I", "Grow component list length = %d", ll );

        for ( i=0; i<ll; i+=4 )
            DXDebug ( "I", (ll>=(i+4)) ? "    = `%s',`%s',`%s',`%s'" :
                           (ll==(i+3)) ? "    = `%s',`%s',`%s'" :
                           (ll==(i+2)) ? "    = `%s',`%s'" :
                                         "    = `%s'",
                      growl[i], growl[i+1], growl[i+2], growl[i+3] );
    }

    DXASSERTGOTO ( DXGetGroupClass ( (Group) input ) == CLASS_COMPOSITEFIELD );

    if ( SavedCopies.used >= ( SavedCopiesMaxDEF - 1 ) )
        DXErrorGoto
            ( ERROR_UNEXPECTED,
              "more compositefields to grow in copy_grow_save" );

    /* XXX - COPY_STRUCTURE vs COPY_ATTRIBUTES */

    if ( ERROR == ( copy = (CompositeField) DXCopy ( (Object) input,
                                                  COPY_STRUCTURE ) ) )
         if ( DXGetError() != ERROR_NONE ) 
             /* proper error */
             goto error;
         else
             DXErrorGoto
                 ( ERROR_INTERNAL,
                   "DXCopy: returned error but didn't set the ErrorCode" )
     else
         if ( DXGetError() != ERROR_NONE ) 
             DXMessageGoto
                 ( "DXCopy: set the ErrorCode but didn't return error" )
         /* else: properly succeeded */

    if ( !DXGrowV ( (Object) copy, 1, GROW_NONE, growl ) )
         if ( DXGetError() != ERROR_NONE ) 
             /* proper error */
             goto error;
         else
             DXErrorGoto
                 ( ERROR_INTERNAL,
                   "DXGrow: returned error but didn't set the ErrorCode" )
     else
         if ( DXGetError() != ERROR_NONE ) 
             DXMessageGoto
                 ( "DXGrow: set the ErrorCode but didn't return error" )
         /* else: properly succeeded */

    SavedCopies.cf [ ++SavedCopies.used ] = copy;

    return copy;

    error:
        ERROR_SECTION;

        DXDelete ( (Object) copy );
        copy = NULL;

        return ERROR;
}



/*
 * Traversal section:
 *   See parts.c/DXProcessParts for desired bahavior
 *      for a traverser such as this.
 *   Who calls who:
 *       m_Isosurface
 *           _dxf_IsosurfaceObject
 *               stats
 *               iso_part
 *                   stats
 *                   iso_part{recursive}
 *                   DXAddTask with iso_added_task 
 *               - or -
 *               iso_added_task 
 *                   isosurface_field
 */

/*-----------------------------------------*\
\*-----------------------------------------*/
static
Error add_attribute ( Object object, iso_arg_type *iso_arg, int i )
{
    Array   array = NULL;
    Pointer valp  = NULL;
    int     valc;

    DXASSERTGOTO ( ERROR != iso_arg->isovals );

    /* if i<0 and module==ISOSURFACE, add entire array of isovals */

    if (iso_arg->module==BAND || (iso_arg->module==ISOSURFACE && i<0))
    {
	  valp = iso_arg->isovals;
	  valc = iso_arg->number;
    }
    else
    {
	  valp = &iso_arg->isovals[i];
	  valc = 1;
    }

    if ( object != NULL )
    {
        if ( ( ERROR == ( array
                   = DXNewArray ( TYPE_FLOAT, CATEGORY_REAL, 0, 0 ) ) )
             ||
             !DXAllocateArray ( array, valc )          ||
             !DXAddArrayData  ( array, 0, valc, valp ) ||
             !DXTrim          ( array )
             ||
             !DXSetAttribute ( object, ATTR_NAME(iso_arg), (Object)array ) )
            goto error;

        if ( ( iso_arg->module == ISOSURFACE && i > -1 )
             &&
             ( CLASS_GROUP == DXGetObjectClass ( iso_arg->parent[0] ) )
             &&
             ( CLASS_SERIES == DXGetGroupClass  ( (Group) iso_arg->parent[0] ) )
             &&
             ( NULL == DXGetAttribute ( (Object)object, "series position" ) ) )
            if ( !DXSetAttribute ( object, "series position", (Object)array ) )
                goto error;
    }

    return OK;

    error:
        ERROR_SECTION;
        return ERROR;
}


/*-----------------------------------------*\
   set by DXAddTask
   called by DXExecuteTaskGroup
\*-----------------------------------------*/
static
Error iso_added_task ( Pointer args )
{
    iso_arg_type *argcast = (iso_arg_type *) args;
    Field        *outputs = NULL;
    int          *skipvec = NULL;
    float        min, max;
    int          i;
    int          have_hit;
    int          can_cull_fields;
    int          can_setmember;

    if ( !args )
        DXErrorGoto ( ERROR_MISSING_DATA, "during DXAdd(ed)Task" );

    if ( OBJ_ERROR == iso_object_type_check ( argcast->self ) )
        goto error;

    if ( !DXStatistics ( argcast->self, "data", &min, &max, NULL, NULL ) )
        DXMessageGoto
            ( "Statistics on field \"data\" component" )

    switch ( DXGetObjectClass ( argcast->parent[0] ) )
    {
        case CLASS_XFORM: case CLASS_SCREEN: case CLASS_CLIPPED:
            can_cull_fields = 0; break;

        case CLASS_GROUP:
            can_cull_fields
                = ( DXGetGroupClass ( (Group) argcast->parent[0] )
                    == CLASS_COMPOSITEFIELD );
            break;

        default: 
            DXErrorGoto ( ERROR_UNEXPECTED, "parent not a group" );
            break;
    }

    have_hit = 0;
    if ( argcast->module == BAND )
        have_hit = 1;
    else
        for ( i=0; ( ( i < VALUE_COUNT ( argcast ) ) && !have_hit ); i++ )
            if ( ( argcast->isovals[i] >= min ) &&
                 ( argcast->isovals[i] <= max )   )
                have_hit = 1;

    /* this trivial case shadda been done call -1 */
    if ( !have_hit && can_cull_fields )
        /* XXX */
        goto ok;

    if ( ( ERROR == ( outputs
               = (Field *) DXAllocateZero
                             ( sizeof(Field) * STRUCT_COUNT ( argcast ) ) ) )
         ||
         ( ERROR == ( skipvec
               = (int *) DXAllocate
                             ( sizeof(int) * STRUCT_COUNT ( argcast ) ) ) ) )
        goto error;

    for ( i=0; i<STRUCT_COUNT ( argcast ); i++ )
        if ( ( argcast->isovals[i] >= min ) &&
             ( argcast->isovals[i] <= max )   )

            skipvec[i] = 0;
        else
            /* skip test not valid for a 2-valued system */
            skipvec[i] = ( argcast->module == ISOSURFACE ) ? 1 : 0;

    if ( can_cull_fields )
    {
        for ( i=0; i<STRUCT_COUNT(argcast); i++ )
            if ( skipvec[i] )
                outputs[i] = NULL;
            else if ( ERROR == ( outputs[i] = DXNewField () ) )
                goto error;
    }
    else
        for ( i=0; i<STRUCT_COUNT(argcast); i++ )
            if ( ERROR == ( outputs[i] = DXNewField () ) )
                goto error;

    if ( !isosurface_field ( (Field) argcast->self,
                             argcast->number,
                             argcast->isovals,
                             (argcast->have_minmax)?
                                 &argcast->band_min_cf : NULL,
                             argcast->module,
                             argcast->remap,
                             outputs,
                             skipvec,
                             argcast->normal_type,
                             argcast->gradient_interpolator,
                             argcast->is_grown,
                             argcast->normal_direction )
         ||
         ( DXGetError() != ERROR_NONE ) )
        goto error;

    /* if(argcast->is_grown)DXShrink(); not required: grown area is Deleted */

    for ( i=0; i<STRUCT_COUNT(argcast); i++ )
    { 
        can_setmember = 0;

        if ( !outputs[i] )
        { 
            if ( !can_cull_fields )
            { 
                if ( ERROR == ( outputs[i] = DXEndField ( DXNewField() ) ) )
                    goto error;

                can_setmember = 1;
            } 
        } 
        else
        { 
            if ( ERROR == ( outputs[i] = DXEndField ( outputs[i] ) ) )
                goto error;

            if ( !DXEmptyField ( outputs[i] ) )
            { 
                can_setmember = 1;
            } 
            else
            { 
                if ( !can_cull_fields )
                    can_setmember = 1;
                else
                { 
                    if ( !DXDelete ( (Object) outputs[i] ) )
                        goto error;
                    else
                        outputs[i] = NULL;
                } 
            } 
        } 

        if ( !add_attribute ( (Object)outputs[i], argcast, i ) )
            goto error;

        /* XXX - DXEndField ? */
        if ( can_setmember )
            switch ( DXGetObjectClass ( argcast->parent[i] ) )
            {
                case CLASS_GROUP:
                    if ( ( CLASS_SERIES ==
                              DXGetGroupClass ( (Group) argcast->parent[i] ) )
                         &&
                         ( -1 != argcast->series_ordinal ) )
                    {
                        if ( !DXSetSeriesMember
                                  ( (Series) argcast->parent[i],
                                    argcast->series_ordinal,
                                    (double) argcast->series_FP_val,
                                    (Object) outputs[i] ) )
                             goto error;
                    }
                    else

                    if ( !DXSetMember ( (Group) argcast->parent[i],
                                        argcast->member_name,
                                        (Object) outputs[i] ) )
                        goto error;


                    break;

                case CLASS_XFORM:
                    if ( !DXSetXformObject
                              ( (Xform) argcast->parent[i],
                                (Object) outputs[i] ) )
                        goto error;
                    break;

                case CLASS_SCREEN:
                    if ( !DXSetScreenObject
                              ( (Screen) argcast->parent[i],
                                (Object) outputs[i] ) )
                        goto error;
                    break;

                case CLASS_CLIPPED:
                    if ( !DXSetClippedObjects
                              ( (Clipped) argcast->parent[i],
                                (Object) outputs[i], NULL ) )
                        goto error;
                    break;
	        default:
		    break;
            }
    }

    if (argcast && argcast->parent  && argcast->module == ISOSURFACE && argcast->task_counter == 0)
    {
	add_attribute((Object)argcast->parent[0], argcast, -1);
    }

    ok:

    DXFree ( (Pointer) outputs );
    DXFree ( (Pointer) skipvec );
    DXFree ( (Pointer) argcast->isovals );
    DXFree ( (Pointer) argcast->parent  );

    outputs          = NULL;
    skipvec          = NULL;
    argcast->isovals = NULL;
    argcast->parent  = NULL;

    return OK;

    error:
        ERROR_SECTION;

        if ( outputs )
        {
            for ( i=0; i<STRUCT_COUNT(argcast); i++ )
                if ( outputs[i] )
                    DXDelete ( (Object) outputs[i] );

            DXFree ( (Pointer) outputs );
            outputs = NULL;
        }

        DXFree ( (Pointer) skipvec );
        DXFree ( (Pointer) argcast->isovals );
        DXFree ( (Pointer) argcast->parent  );

        skipvec          = NULL;
        argcast->isovals = NULL;
        argcast->parent  = NULL;

        return ERROR;
}


/*--------------------------------*\
  recursive
\*--------------------------------*/
static
Error iso_part ( iso_arg_type iso_arg )
{
    Class          class;
    int            i;
    iso_arg_type   call_arg;
    float          *deletable_isovals = NULL;
    CompositeField grown;

    call_arg         = iso_arg;
    call_arg.isovals = NULL;
    call_arg.parent  = NULL;
        /* Significantly: copy over the invariant attributes */

    /*
     * Traverse down from:
     *   iso_arg.parent
     *   iso_arg.self  to  call_arg.parent
     *                     call_arg.self
     *
     * Or act on iso_arg.self if a leaf.
     */

    switch ( class = DXGetObjectClass ( iso_arg.self ) )
    {
        case CLASS_FIELD:

            /* XXX - output culling is 'kosher'? */

            /* 
             * DXCopy Arrayed data (make unique) and submit task.
             */

            if ( !DXEmptyField ( (Field) iso_arg.self ) )
            {
                /*
                 * The stats>DXStatistics call will fail on EmptyFields.
                 *   hence it had to be moved here.
                 */

                if ( !stats ( &iso_arg, &deletable_isovals ) )
                     goto error;

                DXASSERTGOTO ( iso_arg.isovals != NULL );
                /*
                 * arguments are copied so that they can be deleted 
                 * without contention by each added task 
                 */
                if ( ( ERROR == ( call_arg.parent
                           = (Object *) DXAllocate
                                  ( STRUCT_COUNT(&iso_arg) * sizeof (Object) )))
                     ||
                     ( ERROR == ( call_arg.isovals
                           = (float *) DXAllocate
                                  ( VALUE_COUNT(&iso_arg) * sizeof (float) ) ) )
                     ||
                     ! memcpy ( (char *)call_arg.parent,
                                (char *)iso_arg.parent,
                                ( STRUCT_COUNT(&iso_arg) * sizeof (Object) ) )
                     ||
                     ! memcpy ( (char *)call_arg.isovals,
                                (char *)iso_arg.isovals,
                                ( VALUE_COUNT(&iso_arg) * sizeof (float) ) ) )
                    goto error;
                /*
                 * Recall that DXAddTask will copy its input when
                 * it is passed in as non-zero length.
                 */

		call_arg.task_counter = _dxd_isosurface_task_counter++;
                if ( !DXAddTask ( iso_added_task,
                                  (Pointer)&call_arg,
                                  sizeof( iso_arg_type ),
                                  (double)1.0 ) )
                    goto error;
            }
            break;

        case CLASS_XFORM:
        case CLASS_SCREEN:
        case CLASS_CLIPPED:

            call_arg.isovals = iso_arg.isovals;

            if ( ERROR == ( call_arg.parent
                     = (Object *) DXAllocate
                                      ( STRUCT_COUNT(&iso_arg) *
                                        sizeof (Object) ) ) )
                goto error;

            for ( i=0; i<STRUCT_COUNT(&iso_arg); i++ )
                if ( ERROR == ( call_arg.parent[i]
                         = DXCopy ( iso_arg.self, COPY_ATTRIBUTES ) ) )
                    goto error;

            switch ( DXGetObjectClass ( iso_arg.parent[0] ) )
            {
                case CLASS_GROUP:
                    if ( ( CLASS_SERIES ==
                               DXGetGroupClass ( (Group) iso_arg.parent[0] ) )
                         &&
                         ( -1 != iso_arg.series_ordinal ) )
                        for ( i=0; i<STRUCT_COUNT(&iso_arg); i++ )
                        {
                            if ( !DXSetSeriesMember
                                     ( (Series) iso_arg.parent[i],
                                       iso_arg.series_ordinal,
                                       (double) iso_arg.series_FP_val,
                                       (Object) call_arg.parent[i] ) )
                                goto error;
                        }
                    else

                    for ( i=0; i<STRUCT_COUNT(&iso_arg); i++ )
                        if ( !DXSetMember ( (Group) iso_arg.parent[i],
                                            iso_arg.member_name,
                                            call_arg.parent[i] ) )
                            goto error;

                    break;

                case CLASS_XFORM:
                    for ( i=0; i<STRUCT_COUNT(&iso_arg); i++ )
                        if ( !DXSetXformObject
                                  ( (Xform)iso_arg.parent[i],
                                    call_arg.parent[i] ) )
                            goto error;
                    break;

                case CLASS_SCREEN:
                    for ( i=0; i<STRUCT_COUNT(&iso_arg); i++ )
                        if ( !DXSetScreenObject
                                  ( (Screen)iso_arg.parent[i],
                                    call_arg.parent[i] ) )
                            goto error;
                    break;

                case CLASS_CLIPPED:
                    for ( i=0; i<STRUCT_COUNT(&iso_arg); i++ )
                        if ( !DXSetClippedObjects
                                  ( (Clipped)iso_arg.parent[i],
                                    call_arg.parent[i],
                                    NULL ) )
                            goto error;
                    break;
	        default:
		    break;
            }

            switch ( class )
            {
                case CLASS_XFORM:
                    if ( !DXGetXformInfo
                              ( (Xform)iso_arg.self, &call_arg.self, NULL )
                         ||
                         !iso_part ( call_arg ) )
                        goto error;

                    break;

                case CLASS_SCREEN:
                    if ( !DXGetScreenInfo
                              ( (Screen)iso_arg.self, &call_arg.self,
                                NULL, NULL )
                         ||
                         !iso_part ( call_arg ) )
                        goto error;

                    break;

                case CLASS_CLIPPED:
                    if ( !DXGetClippedInfo
                              ( (Clipped)iso_arg.self, &call_arg.self, NULL )
                         ||
                         !iso_part ( call_arg ) )
                        goto error;

                    break;
	        default:
		    break;
            }

            DXFree ( (Pointer) call_arg.parent );
            call_arg.parent = NULL;

            break;

        case CLASS_GROUP:

             /* Two checks before calling is_volume_topology */
            switch ( iso_object_type_check ( iso_arg.self ) )
            {
                /* is_volume_topology doesn't like empty CF's */
                case OBJ_ERROR: goto error;

                /* see CLASS_FIELD above for example of early exit */
                case OBJ_EMPTY: return OK;
	        default: break;
            }

            /*
             * The stats>DXStatistics call would fail on EmptyFields.
             *   hence it had to be moved away from the top of this routine.
             */

            if ( !stats ( &iso_arg, &deletable_isovals ) )
                 goto error;

            /* 
             * DXCopy and DXGrow if a composite field and forming local gradient
             * Form contents of variable call_arg with each group member.
             *  - miscellaneous settings are copied.
             *  - isovalues are referenced
             *  - parents are created from attribute copy of the group
             *  - parents are setmembered up one level within 'grandparent'
             * Call IsoPart with call_arg across the membership of the group.
             */

            if ( ( DXGetGroupClass ( (Group)iso_arg.self )
                       == CLASS_COMPOSITEFIELD )
                 && 
                 /* Valid complaint: only useful for volume connections! */
                 is_volume_topology ( (CompositeField)iso_arg.self )
                 && 
                 ( iso_arg.normal_type == NORMALS_COMPUTED ) )
            {
                 if ( ERROR == ( grown = copy_grow_save
                                            ( (CompositeField) iso_arg.self )) )
                     goto error;

                 iso_arg.self      = (Object)grown;
                 call_arg.is_grown = 1;
            }

            if ( DXGetError() != ERROR_NONE ) /* is_volume_topology */
                 goto error;

            call_arg.isovals = iso_arg.isovals;

            if ( ERROR == ( call_arg.parent
                     = (Object *) DXAllocate
                                      ( STRUCT_COUNT(&iso_arg) *
                                        sizeof (Object) ) ) )
                goto error;

            for ( i=0; i<STRUCT_COUNT(&iso_arg); i++ )
                if ( ERROR == ( call_arg.parent[i]
                                  = DXCopy ( iso_arg.self, COPY_ATTRIBUTES ) ) )
                    goto error;

            if ( DXGetGroupClass((Group)iso_arg.self) == CLASS_COMPOSITEFIELD )
                for ( i=0; i<STRUCT_COUNT(&iso_arg); i++ )
                    if ( !add_attribute ( call_arg.parent[i], &iso_arg, i ) )
                        goto error;

            switch ( DXGetObjectClass ( iso_arg.parent[0] ) )
            {
                case CLASS_GROUP:
                    if ( ( CLASS_SERIES ==
                                DXGetGroupClass ( (Group) iso_arg.parent[0] ) )
                         &&
                         ( -1 != iso_arg.series_ordinal ) )
                        for ( i=0; i<STRUCT_COUNT(&iso_arg); i++ )
                        {
                            if ( !DXSetSeriesMember
                                      ( (Series) iso_arg.parent[i],
                                        iso_arg.series_ordinal,
                                        (double) iso_arg.series_FP_val,
                                        call_arg.parent[i] ) )
                                goto error;
                        }
                    else
                        for ( i=0; i<STRUCT_COUNT(&iso_arg); i++ )
                            if ( !DXSetMember ( (Group) iso_arg.parent[i],
                                                iso_arg.member_name,
                                                call_arg.parent[i] ) )
                                goto error;

                    break;

                case CLASS_XFORM:
                    for ( i=0; i<STRUCT_COUNT(&iso_arg); i++ )
                        if ( !DXSetXformObject
                                  ( (Xform)iso_arg.parent[i],
                                    call_arg.parent[i] ) )
                            goto error;
                    break;

                case CLASS_SCREEN:
                    for ( i=0; i<STRUCT_COUNT(&iso_arg); i++ )
                        if ( !DXSetScreenObject
                                  ( (Screen)iso_arg.parent[i],
                                    call_arg.parent[i] ) )
                            goto error;
                    break;

                case CLASS_CLIPPED:
                    for ( i=0; i<STRUCT_COUNT(&iso_arg); i++ )
                        if ( !DXSetClippedObjects
                                  ( (Clipped)iso_arg.parent[i],
                                    call_arg.parent[i],
                                    NULL ) )
                            goto error;
                    break;
		default:
		    break;
            }

            /* XXX - errorcheck DXGetEnumeratedMember */
            for ( i             = 0;
                  (call_arg.self=DXGetEnumeratedMember
                                      ( (Group)iso_arg.self,
                                        i,
                                        &call_arg.member_name ));
                  i++ )
            {
                call_arg.series_ordinal = -1;
                call_arg.series_FP_val  = -1.0;

                if ( CLASS_SERIES == DXGetGroupClass ( (Group)iso_arg.self ) )
                {
                    Object check = NULL;

                    call_arg.series_ordinal = i;

                    if ( ERROR == ( check = DXGetSeriesMember
                                              ( (Series)iso_arg.self,
                                                call_arg.series_ordinal,
                                                &call_arg.series_FP_val ) ) )
                        goto error;

                    else if ( call_arg.self != check )
                        DXWarning
                            ( "DXGetSeriesMember != DXGetEnumeratedMember" );
                }

                if ( !iso_part ( call_arg ) )
                    goto error;
            }

            /* XXX: Also: error recovery deletion */
            DXFree ( (Pointer) call_arg.parent );
            call_arg.parent = NULL;

            break;

        default:
            /* Luckily we know that this is the 'data' parameter" */
            DXSetError
                ( ERROR_DATA_INVALID,
                  "#10190", /* %s must be a field or a group */
                  "'data' parameter" );
            DXDebug
                ( "I", "Unknown Object type (%s)", _dxf_ClassName ( class ) );
            goto error;
            break;
    }

    DXFree ( (Pointer) deletable_isovals );
    deletable_isovals = NULL;

    return OK;

    error:
        ERROR_SECTION;

        if ( call_arg.isovals != _dxd_user_def_values )
        {
            DXFree ( (Pointer) call_arg.isovals );
            call_arg.isovals = NULL;
        }

        DXFree ( (Pointer) call_arg.parent  );

        call_arg.parent  = NULL;

        return ERROR;
}



/*----------------------------------------------------------*\
\*----------------------------------------------------------*/
Object _dxf_IsosurfaceObject ( iso_arg_type iso_arg )
{
    Series  base   = NULL;
    Object  member = NULL;
    Field   tempty_f = NULL;
    Class   class;
    int     i;
    float   *copy_isovals      = NULL;
    float   *deletable_isovals = NULL;

    init_saved_copies_DEF;
    _dxd_isosurface_task_counter = 0;

    if ( DXGetError() != ERROR_NONE ) goto error;

    if ( ( ERROR == ( base = DXNewSeries() ) )
         ||
         ( ERROR == ( iso_arg.parent
               = (Object *) DXAllocate ( STRUCT_COUNT(&iso_arg) *
                                         sizeof (Object) ) ) ) )
        goto error;

    for ( i=0; i<STRUCT_COUNT(&iso_arg); i++ )
        iso_arg.parent[i] = (Object) base;

    if ( _dxd_user_def_values )
    {
        float bubble;
        int   j;

        for ( i=0; i<(VALUE_COUNT(&iso_arg)-1); i++ )
            for ( j=(i+1); j<VALUE_COUNT(&iso_arg); j++ )
                if ( _dxd_user_def_values[j] < _dxd_user_def_values[i] )
                {
                    bubble                  = _dxd_user_def_values[j];
                    _dxd_user_def_values[j] = _dxd_user_def_values[i];
                    _dxd_user_def_values[i] = bubble;
                }
    }

    switch ( class = DXGetObjectClass ( iso_arg.self ) )
    {
        case CLASS_FIELD:
            if ( DXEmptyField ( (Field) iso_arg.self ) )
            {
                if ( ( ERROR == ( tempty_f = DXNewField() ) )
                     ||
                     !DXSetMember ( (Group) base, NULL, (Object) tempty_f ) )
                    goto error;

                tempty_f = NULL;
            }
            else
            {
                if ( !stats ( &iso_arg, &deletable_isovals ) )
                    goto error;

                /* most noticeable when isovals = _dxd_user_def_values */
                if ( ( ERROR == ( copy_isovals
                            = (float *) DXAllocate
                                  ( VALUE_COUNT(&iso_arg) * sizeof (float) ) ) )
                     ||
                     ! memcpy ( (char *)copy_isovals,
                                (char *)iso_arg.isovals,
                                ( VALUE_COUNT(&iso_arg) * sizeof (float) ) ) )
                    goto error;

                iso_arg.isovals = copy_isovals;

                /* recall that this function deallo's arrays passed in to it */
                if ( !iso_added_task ( (Pointer) &iso_arg ) )
                    goto error;
            }
            break;

        case CLASS_XFORM: case CLASS_SCREEN: case CLASS_CLIPPED:
        case CLASS_GROUP:

            if ( !stats ( &iso_arg, &deletable_isovals ) )
                goto error;

            if ( !DXCreateTaskGroup  ()          ||
                 !iso_part           ( iso_arg ) ||
                 !DXExecuteTaskGroup ()            )
                goto error;
            break;

        default:
            /* Luckily we know that this is the 'data' parameter" */
            DXSetError
                ( ERROR_DATA_INVALID,
                  "#10190", /* %s must be a field or a group */
                  "'data' parameter" );
            DXDebug
                ( "I", "Unknown Object type (%s)", _dxf_ClassName ( class ) );
            goto error;
    }

    if ( DXGetError() != ERROR_NONE )
        DXMessageGoto
            ( "'process_part' set the error code, but didn't return error" )

#if 0
    if ( !DXProcessParts ( (Object) base,
                         iso_normals_field,
                         (Pointer)&iso_arg.normal_direction,
                         sizeof(int), 0, 1 ) )
        goto error;
#endif

    DXFree ( (Pointer) iso_arg.parent    );
    DXFree ( (Pointer) deletable_isovals );
    free_saved_copies_DEF;

    iso_arg.parent    = NULL;
    deletable_isovals = NULL;

    if ( DXGetEnumeratedMember ( (Group) base, 1, NULL ) )
        return ( (Object) base );

    else
    {
        if ( NULL != ( member = DXGetEnumeratedMember
                                    ( (Group) base, 0, NULL ) ) )
        {
            if ( ( ERROR == ( member = DXCopy ( member, COPY_STRUCTURE ) ) )
                 ||
                 !DXDelete ( (Object) base ) )
                goto error;

            base = NULL;
        }
        else
            if ( ERROR == ( member = (Object) DXNewField () ) )
                goto error;

        return ( member );
    }

    error:
        ERROR_SECTION;

        DXDelete ( (Object) base );
        DXDelete ( (Object) tempty_f );
        DXFree   ( (Pointer) iso_arg.parent    );
        DXFree   ( (Pointer) deletable_isovals );

        base              = NULL;
        tempty_f          = NULL;
        iso_arg.parent    = NULL;
        deletable_isovals = NULL;

        free_saved_copies_DEF;

        return ERROR;
}

#endif /* ( _ISOSURFACE_PASS_NUMBER == 1 ) */
