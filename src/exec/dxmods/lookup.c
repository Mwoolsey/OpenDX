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

#include "dx/dx.h"
#include "cat.h"

static Error traverse( Object *, Object * );
static Error doLeaf( Object *, Object * );
static Error init_lookup( lookupinfo *ls );
static Error do_lookup( lookupinfo *ls );
static int get_int( catinfo *cp, char *s );

Error
m_Lookup( Object *in, Object *out )
{
    int i;

    out[ 0 ] = NULL;

    if ( in[ 0 ] == NULL ) {
        DXSetError( ERROR_MISSING_DATA, "\"input\" must be specified" );
        return ERROR;
    }

    out[ 0 ] = DXCopy( ( Object ) in[ 0 ], COPY_STRUCTURE );

    if ( !traverse( in, out ) )
        goto error;

    return OK;

error:
    for ( i = 0; i < 1; i++ ) {
        if ( in[ i ] != out[ i ] )
            DXDelete( out[ i ] );

        out[ i ] = NULL;
    }

    return ERROR;
}


static Error
traverse( Object *in, Object *out )
{
    switch ( DXGetObjectClass( in[ 0 ] ) ) {
    case CLASS_FIELD:
    case CLASS_ARRAY:
    case CLASS_STRING:

        if ( ! doLeaf( in, out ) )
            return ERROR;

        return OK;

    case CLASS_GROUP: {
            int i;
            int memknt;
            Category c;
            Type t;
            int rank;
            int shape[ 32 ];
            Class groupClass = DXGetGroupClass( ( Group ) in[ 0 ] );

            DXGetMemberCount( ( Group ) in[ 0 ], &memknt );

            for ( i = 0; i < memknt; i++ ) {
                Object new_in[ 8 ], new_out[ 1 ];

                if ( in[ 0 ] )
                    new_in[ 0 ] = DXGetEnumeratedMember( ( Group ) in[ 0 ], i, NULL );
                else
                    new_in[ 0 ] = NULL;

                new_in[ 1 ] = in[ 1 ];

                new_in[ 2 ] = in[ 2 ];

                new_in[ 3 ] = in[ 3 ];

                new_in[ 4 ] = in[ 4 ];

                new_in[ 5 ] = in[ 5 ];

                new_in[ 6 ] = in[ 6 ];

                new_in[ 7 ] = in[ 7 ];

                if ( groupClass != CLASS_GROUP ) {
                    if ( i == 0 ) {
                        DXDelete( ( Object ) out[ 0 ] );

                        switch ( groupClass ) {
                        case CLASS_SERIES: out[ 0 ] = ( Object ) DXNewSeries(); break;
                        case CLASS_MULTIGRID: out[ 0 ] = ( Object ) DXNewMultiGrid(); break;
                        case CLASS_COMPOSITEFIELD: out[ 0 ] = ( Object ) DXNewCompositeField(); break;
                        default: return ERROR;
                        }
                    }

                    new_out[ 0 ] = DXCopy( ( Object ) new_in[ 0 ], COPY_STRUCTURE );

                    if ( ! traverse( new_in, new_out ) )
                        return ERROR;

                    if ( i == 0 ) {
                        if ( ! DXGetType( ( Object ) new_out[ 0 ], &t, &c, &rank, shape ) )
                            return ERROR;

                        if ( ! DXSetGroupTypeV( ( Group ) out[ 0 ], t, c, rank, shape ) )
                            return ERROR;
                    }
                }

                else {
                    new_out[ 0 ] = DXGetEnumeratedMember( ( Group ) out[ 0 ], i, NULL );

                    if ( ! traverse( new_in, new_out ) )
                        return ERROR;
                }


                if ( ! DXSetEnumeratedMember( ( Group ) out[ 0 ], i, new_out[ 0 ] ) )
                    return ERROR;

            }

            return OK;
        }

    case CLASS_XFORM: {
            Object new_in[ 8 ], new_out[ 1 ];

            if ( in[ 0 ] )
                DXGetXformInfo( ( Xform ) in[ 0 ], &new_in[ 0 ], NULL );
            else
                new_in[ 0 ] = NULL;

            new_in[ 1 ] = in[ 1 ];

            new_in[ 2 ] = in[ 2 ];

            new_in[ 3 ] = in[ 3 ];

            new_in[ 4 ] = in[ 4 ];

            new_in[ 5 ] = in[ 5 ];

            new_in[ 6 ] = in[ 6 ];

            new_in[ 7 ] = in[ 7 ];

            DXGetXformInfo( ( Xform ) out[ 0 ], &new_out[ 0 ], NULL );

            if ( ! traverse( new_in, new_out ) )
                return ERROR;

            DXSetXformObject( ( Xform ) out[ 0 ], new_out[ 0 ] );

            return OK;
        }

    case CLASS_CLIPPED: {
            Object new_in[ 8 ], new_out[ 1 ];


            if ( in[ 0 ] )
                DXGetClippedInfo( ( Clipped ) in[ 0 ], &new_in[ 0 ], NULL );
            else
                new_in[ 0 ] = NULL;

            new_in[ 1 ] = in[ 1 ];

            new_in[ 2 ] = in[ 2 ];

            new_in[ 3 ] = in[ 3 ];

            new_in[ 4 ] = in[ 4 ];

            new_in[ 5 ] = in[ 5 ];

            new_in[ 6 ] = in[ 6 ];

            new_in[ 7 ] = in[ 7 ];

            DXGetClippedInfo( ( Clipped ) out[ 0 ], &new_out[ 0 ], NULL );

            if ( ! traverse( new_in, new_out ) )
                return ERROR;

            DXSetClippedObjects( ( Clipped ) out[ 0 ], new_out[ 0 ], NULL );

            return OK;
        }

    default:
        break;
    }

    DXSetError( ERROR_BAD_CLASS, "input must be Field or Group" );
    return ERROR;
}

static int
doLeaf( Object *in, Object *out )
{
    int i, result = 0;
    Field field;
    int rank, shape[ 30 ];
    Category category;
    char *data_comp;
    char *lookup_comp;
    char *value_comp;
    char *dest_comp;
    char *table_comp;
    Array input_array = NULL;
    Array lookup_array = NULL;
    Array value_array = NULL;
    int data_knt;
    Type data_type;
    char value_str[ 200 ];
    catinfo cat_data;
    catinfo cat_lookup;
    catinfo cat_value;
    catinfo cat_dest;
    lookupinfo look;
    char *ignore;
    int free_value = 0;
    int free_input = 0;
    Class in_class;
    Class in_data_class;
    char *str;

    /* Support user-supplied return value for unfounds */
    Object nfObj = NULL;
    int delNfObj = 0;
    int nfKnt = 0;
    Category nfCat;
    Type nfType;
    int nfRank;
    int nfShape[30];
    int  nfSize = 0;

    memset( &cat_data, 0, sizeof( catinfo ) );
    memset( &cat_lookup, 0, sizeof( catinfo ) );
    memset( &cat_value, 0, sizeof( catinfo ) );
    memset( &cat_dest, 0, sizeof( catinfo ) );
    memset( &look, 0, sizeof( lookupinfo ) );
    look.data = &cat_data;
    look.lookup = &cat_lookup;
    look.value = &cat_value;
    look.dest = &cat_dest;

    in_class = DXGetObjectClass( ( Object ) in[ 0 ] );

    if ( in_class == CLASS_FIELD ) {
        field = ( Field ) in[ 0 ];

        if ( DXEmptyField( field ) )
            return OK;
    }

    if ( !DXExtractString( ( Object ) in[ 2 ], &data_comp ) )
        data_comp = STR_DATA;

    if ( !DXExtractString( ( Object ) in[ 3 ], &lookup_comp ) )
        lookup_comp = STR_POSITIONS;

    if ( !DXExtractString( ( Object ) in[ 4 ], &value_comp ) ) {
        value_comp = STR_DATA;
    }

    if ( !DXExtractString( ( Object ) in[ 5 ], &dest_comp ) )
        dest_comp = "lookedup";

    for ( i = 0; DXExtractNthString( ( Object ) in[ 6 ], i, &ignore ); i++ ) {
        if ( !strcmp( ignore, "none" ) ) {}
        else if ( !strcmp( ignore, "case" ) ) {
            look.ignore |= CAT_I_CASE;
        }

        else if ( !strcmp( ignore, "space" ) ) {
            look.ignore |= CAT_I_SPACE;
        }

        else if ( !strcmp( ignore, "lspace" ) ) {
            look.ignore |= CAT_I_LSPACE;
        }

        else if ( !strcmp( ignore, "rspace" ) ) {
            look.ignore |= CAT_I_RSPACE;
        }

        else if ( !strcmp( ignore, "lrspace" ) ) {
            look.ignore |= CAT_I_LRSPACE;
        }

        else if ( !strcmp( ignore, "punctuation" ) ) {
            look.ignore |= CAT_I_PUNCT;
        }

        else {
            DXSetError( ERROR_BAD_PARAMETER, "ignore value \"%s\" must be one of"
                        "none, case, space, lspace, rspace, lrspace, punctuation", ignore );
            goto error;
        }
    }

    nfObj = in[7];
    if( nfObj ) {
	switch(DXGetObjectClass(nfObj)){
	case CLASS_STRING:
	    /* turn string into array */
	    delNfObj = 1;
	    nfObj = (Object) DXMakeStringList(1, DXGetString((String)nfObj));
	    /* fall thru...*/
	case CLASS_ARRAY:
	    DXGetArrayInfo( (Array) nfObj,&nfKnt,&nfType,&nfCat,&nfRank,nfShape );
	    break;
	default:
	    DXSetError( ERROR_BAD_PARAMETER, "notFound must be a string or value" );
	    goto error;
	}
    }

    if ( look.ignore & CAT_I_SPACE )
        look.ignore &= ~( CAT_I_LSPACE | CAT_I_RSPACE | CAT_I_LRSPACE );

    if ( ( look.ignore & CAT_I_LSPACE ) && ( look.ignore & CAT_I_RSPACE ) )
        look.ignore |= CAT_I_LRSPACE;

    if ( look.ignore & CAT_I_LRSPACE )
        look.ignore &= ~( CAT_I_LSPACE | CAT_I_RSPACE );

    if ( in_class == CLASS_STRING ) {
        DXExtractString( ( Object ) in[ 0 ], &str );
        input_array = DXNewArray( TYPE_STRING, CATEGORY_REAL, 1, strlen( str ) + 1 );
        DXAddArrayData( input_array, 0, 1, str );
        in_class = CLASS_ARRAY;
        free_input = 1;
    }

    else if ( in_class == CLASS_ARRAY ) {
        input_array = ( Array ) in[ 0 ];
    }

    else {
        input_array = ( Array ) DXGetComponentValue( ( Field ) in[ 0 ], data_comp );
    }

    if ( ! input_array ) {
        DXSetError( ERROR_MISSING_DATA, "\"input\" has no \"%s\" component", data_comp );
        goto error;
    }

    in_data_class = DXGetObjectClass( ( Object ) input_array );

    if ( in_data_class != CLASS_ARRAY && in_data_class != CLASS_STRING ) {
        DXSetError( ERROR_BAD_CLASS, "component \"%s\" of \"input\" must be an array", data_comp );
        goto error;
    }

    if ( in[ 1 ] == NULL ) {
        if ( in_class != CLASS_FIELD ) {
            DXSetError( ERROR_BAD_PARAMETER, "table compononent must be supplied if input is an array" );
            goto error;
        }

        if ( !strcmp( lookup_comp, STR_POSITIONS ) ) {
            sprintf( value_str, "%s lookup", data_comp );
            value_comp = value_str;
            value_array = ( Array ) DXGetComponentValue( ( Field ) in[ 0 ], value_comp );

            if ( !value_array ) {
                DXSetError( ERROR_BAD_PARAMETER, "table component \"%s\" not found in input", value_str );
                goto error;
            }
        }

        else {
            sprintf( value_str, "%s lookup", data_comp );
            value_comp = value_str;
            lookup_array = ( Array ) DXGetComponentValue( ( Field ) in[ 0 ], value_comp );

            if ( !lookup_array ) {
                DXSetError( ERROR_BAD_PARAMETER, "table component \"%s\" not found in input", value_str );
                goto error;
            }
        }
    }

    else if ( DXExtractString( ( Object ) in[ 1 ], &table_comp ) ) {
        if ( in_class != CLASS_FIELD ) {
            DXSetError( ERROR_BAD_PARAMETER, "table compononent must be supplied if input is an array" );
            goto error;
        }

        if ( !strcmp( lookup_comp, STR_POSITIONS ) ) {
            value_array = ( Array ) DXGetComponentValue( ( Field ) in[ 0 ], table_comp );

            if ( !value_array ) {
                DXSetError( ERROR_BAD_PARAMETER, "table component \"%s\" not found in input", table_comp );
                goto error;
            }
        }

        else {
            lookup_array = ( Array ) DXGetComponentValue( ( Field ) in[ 0 ], table_comp );

            if ( !lookup_array ) {
                DXSetError( ERROR_BAD_PARAMETER, "table component \"%s\" not found in input", table_comp );
                goto error;
            }
        }
    }

    else if ( DXGetObjectClass( ( Object ) in[ 1 ] ) == CLASS_ARRAY ) {
        if ( !strcmp( value_comp, STR_DATA ) )
            value_array = ( Array ) in[ 1 ];
        else
            lookup_array = ( Array ) in[ 1 ];
    }

    else if ( DXGetObjectClass( ( Object ) in[ 1 ] ) == CLASS_FIELD ) {
        value_array = ( Array ) DXGetComponentValue( ( Field ) in[ 1 ], value_comp );

        if ( !value_array ) {
            DXSetError( ERROR_BAD_PARAMETER, "value component \"%s\" not found in input", value_comp );
            goto error;
        }

        lookup_array = ( Array ) DXGetComponentValue( ( Field ) in[ 1 ], lookup_comp );

        if ( !lookup_array ) {
            DXSetError( ERROR_BAD_PARAMETER, "lookup component \"%s\" not found in input", lookup_comp );
            goto error;
        }
    }

    /* if lookup_array not present, doing implicit lookup */
    if ( !lookup_array ) {
        DXGetArrayInfo( input_array, &data_knt, &data_type, &category, &rank, shape );

        if ( ( data_type != TYPE_BYTE && data_type != TYPE_UBYTE &&
                data_type != TYPE_INT && data_type != TYPE_UINT && data_type != TYPE_FLOAT )
                || category != CATEGORY_REAL || !( ( rank == 0 )
                                                   || ( ( rank == 1 ) && ( shape[ 0 ] == 1 ) ) ) ) {
            DXSetError( ERROR_DATA_INVALID, "data component \"%s\" must be scalar integer or float "
                        "if lookup is based on implicit index", data_comp );
            goto error;
        }
    }

    look.data->o = ( Object ) input_array;
    _dxf_cat_FindCompType( look.data );

    if ( lookup_array ) {
        look.lookup->o = ( Object ) lookup_array;
        _dxf_cat_FindCompType( look.lookup );
    }

    else {
        /* if no lookup_array, we already know data is an integer type */
        memcpy( look.lookup, look.data, sizeof( catinfo ) );
    }

    if ( value_array ) {
        look.value->o = ( Object ) value_array;
        _dxf_cat_FindCompType( look.value );
    }

    else {
        int i;
        int *p;
        look.value->o = ( Object ) DXNewArray( TYPE_INT, CATEGORY_REAL, 0 );
        value_array = ( Array ) look.value->o;
        look.value->num = look.lookup->num;
        DXAddArrayData( ( Array ) look.value->o, 0, look.value->num, NULL );
        p = ( int * ) DXGetArrayData( ( Array ) look.value->o );

        for ( i = 0; i < look.value->num; i++ )
            p[ i ] = i;

        _dxf_cat_FindCompType( look.value );

        free_value = 1;
    }

    if ( ( look.data->cat_type != look.lookup->cat_type )
            ||
            ( ( look.data->cat_type != CAT_STRING )
              &&
              ( ( look.data->obj_size != look.lookup->obj_size ) && ( look.data->cat_type != CAT_SCALAR ) ) ) ) {
        DXSetError( ERROR_DATA_INVALID, "data component type does not match lookup component" );
        goto error;
    }

    memcpy( look.dest, look.value, sizeof( catinfo ) );

    DXGetArrayInfo( value_array, &data_knt, &data_type, &category, &rank, shape );

    if( nfObj ){
      if(data_type == TYPE_STRING){
	/* check type, and be sure result shape is max of value
	 * and not-found value's shape.
	 */
        if( nfType != TYPE_STRING ) {
          DXSetError( ERROR_DATA_INVALID, "notFound type does not match value type" );
          goto error;
	}
	if( nfShape[0] > shape[0] )
		shape[0] = nfShape[0];
      }
      else {
        /* convert supplied not-found value to same type as value component */
        Array tmp;
        if(! (tmp = DXArrayConvertV( (Array)nfObj, data_type, category, rank, shape ))){
          DXSetError( ERROR_DATA_INVALID, "notFound type does not match value type" );
          goto error;
        }
        if( delNfObj )
	  DXDelete( nfObj );
        nfObj = (Object) tmp;
        delNfObj = 1;
      }
      look.nfValue = (Pointer) DXGetArrayData((Array)nfObj);
      look.nfLen   = DXGetItemSize((Array)nfObj);
    }

    look.dest->comp_array = ( Array ) DXNewArrayV( data_type, category, rank, shape );
    DXAddArrayData( look.dest->comp_array, 0, look.data->num, NULL );
    look.dest->comp_data = DXGetArrayData( ( Array ) ( look.dest->comp_array ) );
    DXCopyAttributes( ( Object ) look.dest->comp_array, ( Object ) input_array );

    if ( lookup_array )
        look.lut = ( table_entry * ) DXAllocate( look.value->num * sizeof( table_entry ) );

    init_lookup( &look );

    result = do_lookup( &look );

    if ( ! result ) {
        if ( DXGetError() == ERROR_NONE )
            DXSetError( ERROR_INTERNAL, "error return from user routine" );

        goto error;
    }

    if( delNfObj)
	    DXDelete( nfObj );

    if ( free_input ) {
        DXDelete( ( Object ) input_array );
        input_array = NULL;
    }

    if ( free_value ) {
        DXDelete( ( Object ) look.value->o );
        look.value->o = NULL;
    }

    if ( look.ignore && look.data->free_data && look.data->comp_data )
        DXFree( look.data->comp_data );

    look.data->comp_data = NULL;

    if ( look.ignore && look.lookup->free_data && look.lookup->comp_data )
        DXFree( look.lookup->comp_data );

    look.data->comp_data = NULL;

    if ( !out[ 0 ] ) {
        out[ 0 ] = DXCopy( in[ 0 ], COPY_STRUCTURE );
    }

    if ( DXGetObjectClass( ( Object ) in[ 0 ] ) == CLASS_FIELD ) {
        if ( DXGetComponentValue( ( Field ) out[ 0 ], dest_comp ) )
            DXDeleteComponent( ( Field ) out[ 0 ], dest_comp );

        if ( ! DXSetComponentValue( ( Field ) out[ 0 ], dest_comp, ( Object ) look.dest->comp_array ) )
            goto error;

        DXChangedComponentValues( ( Field ) out[ 0 ], dest_comp );

        result = DXEndField( ( Field ) out[ 0 ] ) != NULL;
    }

    else {
        out[ 0 ] = ( Object ) look.dest->comp_array;
        result = OK;
    }

    return result;

error:

    if ( free_input )
        DXDelete( ( Object ) input_array );

    if ( free_value )
        DXDelete( ( Object ) look.value->o );

    if( delNfObj)
	    DXDelete( nfObj );

    if ( look.dest->comp_array )
        DXDelete( ( Object ) look.dest->comp_array );

    look.dest->comp_array = NULL;

    if ( look.ignore && look.data->free_data && look.data->comp_data )
        DXFree( look.data->comp_data );

    look.data->comp_data = NULL;

    if ( look.ignore && look.lookup->free_data && look.lookup->comp_data )
        DXFree( look.lookup->comp_data );

    look.data->comp_data = NULL;

    return result;
}

static int lookupcmp( const void *e1, const void *e2 )
{
    return ((table_entry *)e1)->cp->cmpcat( ((table_entry *)e1)->cp, 
    		((table_entry *)e1)->lookup, ((table_entry *)e2)->lookup );
}

static int searchcmp( const void *e1, const void *e2 )
{
    return ((table_entry *)e2)->cp->cmpcat( 
    			((table_entry*)e2)->cp, (char *)e1, ((table_entry *)e2)->lookup );
}

static int get_int( catinfo *cp, char *s )
{
    switch ( cp->comp_type ) {
    case TYPE_INT: return ( int ) ( *( ( int * ) s ) );
    case TYPE_UBYTE: return ( int ) ( *( ( ubyte * ) s ) );
    case TYPE_FLOAT: return ( int ) ( *( ( float * ) s ) + 0.5 );
    case TYPE_BYTE: return ( int ) ( *s );
    case TYPE_SHORT: return ( int ) ( *( ( short * ) s ) );
    case TYPE_USHORT: return ( int ) ( *( ( ushort * ) s ) );
    case TYPE_UINT: return ( int ) ( *( ( uint * ) s ) );
    case TYPE_DOUBLE: return ( int ) ( *( ( double * ) s ) + 0.5 );
    default: return ( int ) ( *( ( int * ) s ) );
    }
}

static float get_float( catinfo *cp, char *s )
{
    switch ( cp->comp_type ) {
    case TYPE_INT: return ( float ) ( *( ( int * ) s ) );
    case TYPE_UBYTE: return ( float ) ( *( ( ubyte * ) s ) );
    case TYPE_FLOAT: return ( float ) ( *( ( float * ) s ) );
    case TYPE_BYTE: return ( float ) ( *s );
    case TYPE_SHORT: return ( float ) ( *( ( short * ) s ) );
    case TYPE_USHORT: return ( float ) ( *( ( ushort * ) s ) );
    case TYPE_UINT: return ( float ) ( *( ( uint * ) s ) );
    case TYPE_DOUBLE: return ( float ) ( *( ( double * ) s ) );
    default: return ( float ) ( *( ( int * ) s ) );
    }
}

static Error init_lookup( lookupinfo *l )
{
    int i;
    char *s, *t;
    table_entry *lut;
    char *d;
    float *f;
    int *p;
    int size;
    int num;

    if ( !l->lut )
        return OK;

    if ( l->ignore && l->data->cat_type == CAT_STRING ) {
        d = ( char * ) l->data->comp_data;
        l->data->comp_data = ( Pointer ) DXAllocate( l->data->obj_size * l->data->num );
        l->data->free_data = 1;
        size = l->data->obj_size;
        num = l->data->num;

        for ( i = 0, s = d, t = ( char * ) l->data->comp_data; i < num; i++, s += size, t += size ) {
            memcpy( t, s, size );
            _dxf_cat_ignore( t, l->ignore );
        }

        d = ( char * ) l->lookup->comp_data;
        l->lookup->comp_data = ( Pointer ) DXAllocate( l->lookup->obj_size * l->lookup->num );
        l->lookup->free_data = 1;
        size = l->lookup->obj_size;
        num = l->lookup->num;

        for ( i = 0, s = d, t = ( char * ) l->lookup->comp_data; i < num; i++, s += size, t += size ) {
            memcpy( t, s, size );
            _dxf_cat_ignore( t, l->ignore );
        }
    }

    if ( l->data->cat_type == CAT_SCALAR && l->data->comp_type != l->lookup->comp_type ) {
        if ( l->data->comp_type == TYPE_FLOAT || l->lookup->comp_type == TYPE_FLOAT ) {
            if ( l->data->comp_type == TYPE_FLOAT ) {
                d = ( char * ) l->lookup->comp_data;
                l->lookup->comp_data = ( Pointer ) DXAllocate( sizeof( float ) * l->lookup->num );
                l->lookup->free_data = 1;
                f = ( float * ) l->lookup->comp_data;
                size = l->lookup->obj_size;
                num = l->lookup->num;

                for ( i = 0; i < num; i++, f++, d += size )
                    * f = get_float( l->lookup, d );

                l->lookup->comp_type = TYPE_FLOAT;

                l->lookup->obj_size = sizeof( float );
            }

            else {
                d = ( char * ) l->data->comp_data;
                l->data->comp_data = ( Pointer ) DXAllocate( sizeof( float ) * l->data->num );
                l->data->free_data = 1;
                f = ( float * ) l->data->comp_data;
                size = l->data->obj_size;
                num = l->data->num;

                for ( i = 0; i < num; i++, f++, d += size )
                    * f = get_float( l->data, d );

                l->data->comp_type = TYPE_FLOAT;

                l->data->obj_size = sizeof( float );
            }
        }

        else {
            if ( l->lookup->comp_type != TYPE_INT ) {
                d = ( char * ) l->lookup->comp_data;
                l->lookup->comp_data = ( Pointer ) DXAllocate( sizeof( int ) * l->lookup->num );
                l->lookup->free_data = 1;
                p = ( int * ) l->lookup->comp_data;
                size = l->lookup->obj_size;
                num = l->lookup->num;

                for ( i = 0; i < num; i++, p++, d += size )
                    * p = get_int( l->lookup, d );

                l->lookup->comp_type = TYPE_INT;

                l->lookup->obj_size = sizeof( int );
            }

            if ( l->data->comp_type != TYPE_INT ) {
                d = ( char * ) l->data->comp_data;
                l->data->comp_data = ( Pointer ) DXAllocate( sizeof( int ) * l->data->num );
                l->data->free_data = 1;
                p = ( int * ) l->data->comp_data;
                size = l->data->obj_size;
                num = l->data->num;

                for ( i = 0; i < num; i++, p++, d += size )
                    * p = get_int( l->data, d );

                l->data->comp_type = TYPE_INT;

                l->data->obj_size = sizeof( int );
            }
        }
    }

    s = ( char * ) l->lookup->comp_data;
    t = ( char * ) l->value->comp_data;

    for ( i = 0, lut = l->lut; i < l->lookup->num; i++, lut++ ) {
        lut->lookup = ( Pointer ) s;
        lut->value = ( Pointer ) t;
        lut->cp = l->lookup;
        s += l->lookup->obj_size;
        t += l->value->obj_size;
    }

    qsort( l->lut, l->value->num, sizeof( table_entry ), lookupcmp );

    return OK;
}

static Error do_lookup( lookupinfo *l )
{
    int i;
    char *s, *v;
    table_entry *found;
    int num = l->data->num;
    int size = l->value->obj_size;
    int datasize = l->data->obj_size;
    int n;

    s = ( char * ) l->data->comp_data;
    v = ( char * ) l->dest->comp_data;

    if ( l->lut ) {
        for ( i = 0; i < num; i++, s += datasize, v += size ) {
            found = ( table_entry * ) bsearch( s, l->lut, l->lookup->num,
                                               sizeof( table_entry ), searchcmp );

            if ( found ) {
                memcpy( v, ( char * ) found->value, size );
            }

	    else if (l->nfValue) {
                memcpy( v, ( char * ) l->nfValue, l->nfLen );
	    }

            else {
                memset( v, 0, size );
            }
        }
    }

    else {
        for ( i = 0; i < num; i++, s += datasize, v += size ) {
            n = get_int( l->data, s );

            if ( n >= 0 && n < l->value->num ) {
                memcpy( v, ( char * ) ( &( ( ( char * ) ( l->value->comp_data ) ) [ n * size ] ) ), size );
            }

	    else if (l->nfValue) {
                memcpy( v, ( char * ) l->nfValue, l->nfLen );
	    }

            else {
                memset( v, 0, size );
            }
        }
    }

    return OK;
}
