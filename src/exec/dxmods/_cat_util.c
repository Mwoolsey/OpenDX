/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>

#if defined(HAVE_STRING_H)
#include <string.h>
#endif

#include <dx/dx.h>
#include <math.h>

#if defined(HAVE_CTYPE_H)
#include <ctype.h>
#endif

#include "cat.h"

#define HASH_KEY_AVOID ((PseudoKey) -1)
#define HASH_KEY_INSTEAD ((PseudoKey) 0xefefefef)

static Error GetObjectSize( Object o, uint *nitems, int *len, Type *typ,
                            int *rank, int *shape, int *count, int *cat_type );

#define NPRIMES 11
static int prime1[ NPRIMES ] = {2003, 9973, 19997, 29989, 39989, 49999,
                                59999, 70001, 79999, 90001, 99991};
static int prime2[ NPRIMES ] = {4999, 9001, 24989, 34981, 44987, 55001,
                                64997, 74959, 84991, 94999, 104999};

static int _dxf_cat_lstrip( char *s )
{
    char * p;
    int i, n;

    if ( !s )
        return 0;

    n = strlen( s );

    if ( !n )
        return 0;

    for ( p = s, i = 0; *p && isspace( *p ); p++, i++ )

        ;
    if ( !i )
        return 0;

    memmove( s, p, ( n - i + 1 ) );

    return 1;
}

static int _dxf_cat_rstrip( char *s )
{
    char * p;
    int i, n;

    if ( !s )
        return 0;

    n = strlen( s );

    if ( !n )
        return 0;

    for ( p = &s[ n - 1 ], i = n; i && isspace( *p ); p--, i-- )

        ;
    p[ 1 ] = '\0';

    return 1;
}

static int _dxf_cat_strip( char *s )
{
    char * p, *q;
    int n;

    if ( !s )
        return 0;

    n = strlen( s );

    if ( !n )
        return 0;

    for ( p = s, q = s; *q; q++ )
        if ( !isspace( *q ) )
            * p++ = *q;

    *p = '\0';

    return 1;
}

static int _dxf_cat_punct( char *s )
{
    char * p, *q;
    int n;

    if ( !s )
        return 0;

    n = strlen( s );

    if ( !n )
        return 0;

    for ( p = s, q = s; *q; q++ )
        if ( !ispunct( *q ) )
            * p++ = *q;

    *p = '\0';

    return 1;
}

static int _dxf_cat_lower( char *s )
{
    char * p;

    if ( !s )
        return 0;

    for ( p = s; *p; p++ )
        *p = tolower( *p );

    return 1;
}

int _dxf_cat_ignore( char *s, int ignore )
{
    if ( ignore && CAT_I_SPACE ) {
        _dxf_cat_strip( s );
    }

    else if ( ignore && CAT_I_LSPACE ) {
        _dxf_cat_lstrip( s );
    }

    else if ( ignore && CAT_I_RSPACE ) {
        _dxf_cat_rstrip( s );
    }

    else if ( ignore && CAT_I_LRSPACE ) {
        _dxf_cat_rstrip( s );
        _dxf_cat_lstrip( s );
    }

    if ( ignore && CAT_I_PUNCT ) {
        _dxf_cat_punct( s );
    }

    if ( ignore && CAT_I_CASE ) {
        _dxf_cat_lower( s );
    }

    return 1;
}

int _dxf_cat_cmp_str( catinfo *cp, Pointer s, Pointer t )
{
    int c;

    c = strncmp( ( char * ) s, ( char * ) t, cp->obj_size );
#ifdef CAT_DEBUG
    printf( "%d-%s-%s-\n", c, ( char * ) s, ( char * ) t );
#endif
    return ( c > 0 ? 1 : c < 0 ? -1 : 0 );
}

#define CMP(TYPE)       \
int _dxf_cat_cmp_ ## TYPE(catinfo *cp, Pointer s, Pointer t)  \
{ int i;        \
  TYPE *a = (TYPE *)s;       \
  TYPE *b = (TYPE *)t;       \
  int count = cp->count;      \
  for (i=0; i<count && *a == *b; a++, b++, i++);   \
  if (i == count) return 0;      \
  return (*a < *b ? -1 : 1); }

CMP( char )
CMP( ubyte )
CMP( short )
CMP( ushort )
CMP( float )
CMP( int )
CMP( uint )
CMP( double )
#undef CMP

int _dxf_cat_hashcmp( Key searchkey, Element element )
{
    int i;
    int *a;
    int *b;
    char *c;
    char *d;
    hashelement *h1 = ( hashelement * ) element;
    hashelement *h2 = ( hashelement * ) searchkey;
    int size = h1->cp->intsize;
    int remainder = h1->cp->remainder;
    a = h1->p;
    b = h2->p;

    for ( i = 0; ( i < size ) && ( *a == *b ); a++, b++, i++ )
    	;

    if ( i != size ) {
#ifdef CAT_DEBUG
        printf( "hashcmp0 returns 1, i=%d, size=%d, remainder=%d\n", i, size, remainder );
#endif
        return 1;
    }

    for ( i = 0, c = ( char * ) a, d = ( char * ) b; 
    		( i < remainder ) && ( *c == *d ); c++, d++, i++ )
    	;

#ifdef CAT_DEBUG
    printf( "hashcmp1 returns %d, i=%d, size=%d, remainder=%d\n", 
    		( i == remainder ? 0 : 1 ), i, size, remainder );

#endif
    return ( i == remainder ? 0 : 1 );
}

int _dxf_cat_hashstrcmp( Key searchkey, Element element )
{
    char * a;
    char *b;
    hashelement *h1 = ( hashelement * ) element;
    hashelement *h2 = ( hashelement * ) searchkey;
    int size = h1->cp->obj_size;
    a = ( char * ) h1->p;
    b = ( char * ) h2->p;
#ifdef CAT_DEBUG
    printf( "cathashstrcmp %s %s %d\n", a, b, strncmp( a, b, size ) );
#endif
    return ( !strncmp( a, b, size ) ? 0 : 1 );
}

#define NUM_BITS (sizeof(long) * 8)
#define THREE_FOURTHS (NUM_BITS - 8)
#define ONE_EIGHTH (4)
#define HIGH_BITS ((long)(~((long) (~0) >> ONE_EIGHTH)))

PseudoKey _dxf_cat_hashstrkey( Key key )
{
    long hash, tmp;
    char *s;
    hashelement *h = ( hashelement * ) key;
    s = ( char * ) h->p;
    /*t = (char *)h->p;*/

    for ( hash = 0; *s; s++ ) {
        hash = ( hash << ONE_EIGHTH ) + ( *s ) * prime2[ hash % NPRIMES ] + 
        			prime1[ ( *s ) % NPRIMES ];

        if ( ( tmp = hash & HIGH_BITS ) ) {
            hash = ( hash ^ ( tmp >> THREE_FOURTHS ) );
            hash = hash ^ tmp;
        }
    }

    if ( hash == HASH_KEY_AVOID )
        hash = HASH_KEY_INSTEAD;

#ifdef CAT_DEBUG
    printf( "hashstrkey of -%s- = %lx\n", t, hash );

#endif
    return ( PseudoKey ) ( hash );

}

PseudoKey _dxf_cat_hashkey( Key key )
{
    long fullhash, hash, tmp;
    int i, j;
    ubyte *s;
    hashelement *h = key;
    int size = h->cp->obj_size;
    int count = h->cp->count;
    int indsize = size / count;

    /* If a vector, then create hash for each portion and combine */
	for ( fullhash = 0, j = 0; j < count; j++) {
		 s = (( ubyte * ) h->p) + (j * indsize);
	
        for ( hash = 0, i = 0; i < indsize; i++, s++ ) {
            hash = ( hash << ONE_EIGHTH ) + prime2[ hash % NPRIMES ] * 
                        ( *s ? *s : prime1[ i % NPRIMES ] );
            if ( ( tmp = hash & HIGH_BITS ) ) {
                hash = ( hash ^ ( tmp >> THREE_FOURTHS ) );
                hash = hash ^ tmp;
            }
            
        fullhash = (fullhash << ONE_EIGHTH) + hash;
        }

    }

#ifdef CAT_DEBUG
    printf( "hash key: %lx\n", fullhash );
#endif
    if ( fullhash == HASH_KEY_AVOID )
        fullhash = HASH_KEY_INSTEAD;

    return ( PseudoKey ) ( fullhash );
}

int _dxf_cat_cmp_any( catinfo *cp, Pointer s, Pointer t )
{
    int i;
    char *a = ( char * ) s;
    char *b = ( char * ) t;
    int size = cp->obj_size;

    for ( i = 0; i < size && *a == *b; a++, b++, i++ )
    	;

    if ( i == size ) return 0;

    return ( *a < *b ? -1 : 1 );
}

Error _dxf_cat_FindCompType( catinfo *cp )
{
    Object o;

    if ( DXGetObjectClass( ( Object ) cp->o ) == CLASS_ARRAY ) {
        o = cp->o;
    }

    else {
        if ( !( o = DXGetComponentValue( ( Field ) cp->o, cp->comp_name ) ) )
            return ERROR;
    }

    cp->comp_array = ( Array ) o;
    cp->comp_data = DXGetArrayData( cp->comp_array );

    if ( !GetObjectSize( o, &cp->num, &cp->obj_size, &cp->comp_type,
                         &cp->comp_rank, cp->comp_shape, &cp->count, &cp->cat_type ) )
        return ERROR;

    cp->intsize = cp->obj_size / sizeof( int );

    cp->remainder = cp->obj_size % sizeof( int );

    switch ( cp->comp_type ) {
    case TYPE_STRING: cp->cmpcat = _dxf_cat_cmp_str; break;
    case TYPE_BYTE: cp->cmpcat = _dxf_cat_cmp_char; break;
    case TYPE_UBYTE: cp->cmpcat = _dxf_cat_cmp_ubyte; break;
    case TYPE_SHORT: cp->cmpcat = _dxf_cat_cmp_short; break;
    case TYPE_USHORT: cp->cmpcat = _dxf_cat_cmp_ushort; break;
    case TYPE_INT: cp->cmpcat = _dxf_cat_cmp_int; break;
    case TYPE_UINT: cp->cmpcat = _dxf_cat_cmp_uint; break;
    case TYPE_FLOAT: cp->cmpcat = _dxf_cat_cmp_float; break;
    case TYPE_DOUBLE: cp->cmpcat = _dxf_cat_cmp_double; break;
    default: cp->cmpcat = _dxf_cat_cmp_any;
    }

    switch ( cp->comp_type ) {
    case TYPE_STRING: cp->hashcmp = _dxf_cat_hashstrcmp; break;
    default: cp->hashcmp = _dxf_cat_hashcmp;
    }

    switch ( cp->comp_type ) {
    case TYPE_STRING: cp->catkey = _dxf_cat_hashstrkey; break;
    default: cp->catkey = _dxf_cat_hashkey;
    }


    return OK;
}

static Error GetObjectSize( Object o, uint *nitems, int *len, Type *typ,
                            int *rank, int *shape, int *count, int *cat_type )
{
    Category c;
    *cat_type = CAT_ARBITRARY;
    *count = 1;

    if ( DXGetObjectClass( o ) != CLASS_ARRAY )
        return ERROR;

    if ( !DXGetArrayInfo( ( Array ) o, ( int * ) nitems, typ, &c, rank, shape ) )
        return ERROR;

    if ( !( *len = DXGetItemSize( ( Array ) o ) ) )
        return ERROR;

    if ( c != CATEGORY_REAL )
        return OK;

    if ( *typ == TYPE_STRING ) {
        *cat_type = CAT_STRING;
        return OK;
    }

    if ( !rank || *rank == 0 ) {
        *cat_type = CAT_SCALAR;
        return OK;
    }

    if ( *rank > 0 ) {
        int i;

        for ( i = 0; i < *rank; i++ )
            ( *count ) *= shape[ i ];

        if ( *count == 1 )
            * cat_type = CAT_SCALAR;

        return OK;
    }
    return ERROR;
}

