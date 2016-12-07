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
#include "cat.h"

static Error HasInvalid( Field f, char *, ICH *ih );
static Field Categorize( catinfo *cp );
static Field Field_Categorize( catinfo *cp );
static Object Object_Categorize( catinfo *cp );

int
m_Categorize( Object *in, Object *out )
{
    catinfo c;
    int i;
#define MAX_NUM_COMPS 200
    char *s_list[ MAX_NUM_COMPS ];

    /* initialize catinfo struct to zero */
    memset( ( char * ) & c, '\0', sizeof( c ) );
    c.maxparallel = DXProcessors( 0 );
    c.sort = 1; /* default to sort */

    if ( c.maxparallel > 1 )
        c.maxparallel = ( c.maxparallel - 1 ) * PFACTOR;

    /* input object to categorize */
    if ( !in[ 0 ] ) {
        DXSetError( ERROR_BAD_PARAMETER, "#10000", "input" );
        return ERROR;
    } else
        c.o = in[ 0 ];

    c.comp_list = s_list;

    c.comp_list[ 0 ] = DEFAULTCOMP;

    if ( in[ 1 ] && ! DXExtractNthString( in[ 1 ], 0, &c.comp_list[ 0 ] ) ) {
        DXSetError( ERROR_BAD_PARAMETER, "#10200", "component name" );
        goto error;
    }

    for ( i = 1; i < MAX_NUM_COMPS; i++ ) {
        if ( !DXExtractNthString( in[ 1 ], i, &c.comp_list[ i ] ) )
            break;
    }

    c.comp_list[ i ] = NULL;
    
    if (in [ 2 ]) {
    	if (!DXExtractInteger(in[2], &c.sort) ||
	    	(c.sort != 0 && c.sort != 1)) {
	    	DXSetError(ERROR_BAD_PARAMETER, "#10070", "sort flag");
	    	goto error;
		}
	}


    /* do the work here */
    out[ 0 ] = Object_Categorize( &c );

    return ( out[ 0 ] ? OK : ERROR );

error:
    out[ 0 ] = out[ 1 ] = NULL;
    return ( ERROR );
}


/* general categorize routine.  not used in m_Categorize; it just packages
 *  the parms into a block and calls the internal routine to make it
 *  easier for other libDX users to call the routine.
 */
Object
_dxfCategorize( Object o, char **comp_list )
{
    catinfo c;
    Object retval = o;

    if ( !o ) {
        DXSetError( ERROR_BAD_PARAMETER, "#10000", "input" );
        return ERROR;
    }

    memset( ( char * ) & c, '\0', sizeof( c ) );
    c.maxparallel = DXProcessors( 0 );

    if ( c.maxparallel > 1 )
        c.maxparallel = ( c.maxparallel - 1 ) * PFACTOR;

    c.comp_list = comp_list;

    c.o = ( Object ) o;

    retval = Object_Categorize( &c );

    return retval;
}


/* if the object is a Series, CompositeField or Field, make one categorize
 *  from it.  if it is a general Group, make one categorize for each member.
 */
static Object
Object_Categorize( catinfo *cp )
{
    switch ( DXGetObjectClass( cp->o ) ) {
#if YYY
    case CLASS_ARRAY:
        build positions and call Field_Categorize

        return ( Object ) Array_Categorize( cp );
#endif

    case CLASS_FIELD:
        return ( Object ) Field_Categorize( cp );
#if YYY
    case CLASS_GROUP: {
            Object subo, old, cpy;

            switch ( DXGetGroupClass( ( Group ) cp->o ) ) {
            case CLASS_COMPOSITEFIELD:
                /* have to make a copy here */
                cpy = DXCopy( cp->o, COPY_STRUCTURE );

                if ( !cpy )
                    return NULL;

                old = cp->o;

                cp->o = cpy;

                if ( !DXInvalidateDupBoundary( cp->o ) ) {
                    /* delete copy and restore original object */
                    DXDelete( ( Object ) cp->o );
                    cp->o = old;
                    return NULL;
                }

                subo = ( Object ) Field_Categorize( cp );

                DXDelete( cpy );
                cp->o = old;
                return subo;

            case CLASS_MULTIGRID:
            case CLASS_SERIES:
                return ( Object ) Field_Categorize( cp );

            default:
                return ( Object ) Group_Categorize( cp );
            }
        }

    case CLASS_SCREEN:

        if ( !DXGetScreenInfo( ( Screen ) cp->o, &subo, NULL, NULL ) )
            return NULL;

        cp->o = subo;

        return Object_Categorize( cp );

    case CLASS_XFORM:
        if ( !DXGetXformInfo( ( Xform ) cp->o, &subo, NULL ) )
            return NULL;

        cp->o = subo;

        return Object_Categorize( cp );

    case CLASS_CLIPPED:
        if ( !DXGetClippedInfo( ( Clipped ) cp->o, &subo, NULL ) )
            return NULL;

        cp->o = subo;

        return Object_Categorize( cp );

#endif
    default:
        DXSetError( ERROR_BAD_TYPE, "Categorize requires field input" );
        /* DXSetError(ERROR_BAD_PARAMETER, "#10190", "input"); */
        return NULL;
    }

    /* not reached */
}

/* the input is a generic group;  call Categorize for each member.
 */
#if YYY
static Group
Group_Categorize( catinfo *cp )
{
    Group g;
    Group newg = NULL;
    Object subo;
    Object h;
    char *name;
    int i, members;
    parinfo *pinfo = NULL;
    /* handle empty groups, and groups of only 1 member
     */
    g = ( Group ) cp->o;

    for ( members = 0; subo = DXGetEnumeratedMember( g, members, &name ); members++ )
        ;

    if ( members == 0 )
        return ( Group ) cp->o;

    newg = ( Group ) DXCopy( cp->o, COPY_HEADER );

    if ( !newg )
        return NULL;

    if ( members == 1 ) {
        subo = DXGetEnumeratedMember( g, 0, &name );

        if ( !subo )
            goto error;

        cp->o = subo;

        h = Object_Categorize( cp );

        if ( !h )
            goto error;

        /* if the member is named, add it by name, else add by number */
        if ( ! ( name ? DXSetMember( newg, name, h )
                 : DXSetEnumeratedMember( newg, 0, h ) ) )
            goto error;

        return newg;
    }


    /* if we have groups nested in groups, make sure we don't start
     *  too many parallel tasks.
     */
    if ( cp->goneparallel > cp->maxparallel ) {
        for ( i = 0; subo = DXGetEnumeratedMember( g, i, &name ); i++ ) {

            cp->o = subo;
            h = Object_Categorize( cp );

            if ( !h )
                goto error;

            /* if the member is named, add it by name, else add by number */
            if ( ! ( name ? DXSetMember( newg, name, h )
                     : DXSetEnumeratedMember( newg, i, h ) ) )
                goto error;

        }

        return newg;
    }


    /* for each member, construct a categorize in parallel, and
     *  add them to the group later.
     */

    pinfo = ( parinfo * ) DXAllocateZero( members * sizeof( parinfo ) );

    if ( !pinfo )
        goto error;

    if ( !DXCreateTaskGroup() )
        goto error;

    for ( i = 0; subo = DXGetEnumeratedMember( g, i, &name ); i++ ) {
        cp->o = subo;
        cp->p = &pinfo[ i ];
        cp->goneparallel++;

        if ( !DXAddTask( Categorize_Wrapper,
                         ( Pointer ) cp, sizeof( catinfo ), 0.0 ) )
            goto error;

    }

    if ( !DXExecuteTaskGroup() )
        goto error;

    cp->goneparallel -= members;

    /* now add accumulated categorizes to the parent group */
    for ( i = 0; i < members; i++ ) {
        /* if the member is named, add it by name, else add by number */
        if ( ! ( pinfo[ i ].name ? DXSetMember( newg, pinfo[ i ].name, pinfo[ i ].h )
                 : DXSetEnumeratedMember( newg, i, pinfo[ i ].h ) ) )
            goto error;

        cp->didwork += pinfo[ i ].didwork;
    }

    DXFree( ( Pointer ) pinfo );
    return newg;

error:
    DXDelete( ( Object ) newg );
    DXFree( ( Pointer ) pinfo );
    return NULL;
}

#endif

/* for going parallel.  the wrapper routine which calls Object_Categorize
 *  and puts the results in the right place.
 */
#if YYY
static Error
Categorize_Wrapper( Pointer a )
{
    catinfo * cp = ( catinfo * ) a;
    catinfo h2;

    if ( !CopyCatinfo( cp, &h2 ) )
        return ERROR;

    cp->p->h = Object_Categorize( &h2 );

    if ( !cp->p->h )
        goto error;

    cp->p->didwork += h2.didwork;

    /* DXFreeInvalidComponentHandle(cp->invalid); */  /* not needed here? */
    FreeCatinfo( &h2 );

    return OK;

error:
    FreeCatinfo( &h2 );
    return ERROR;
}

#endif

/* for going parallel.  the wrapper routine which calls Array_Categorize
 *  and puts the results in the right place.
 */
#if YYY
static Error
Categorize_Wrapper2( Pointer a )
{
    catinfo * cp = ( catinfo * ) a;
    catinfo h2;

    if ( !CopyCatinfo( cp, &h2 ) )
        return ERROR;

    cp->p->h = ( Object ) Array_Categorize( &h2 );

    if ( !cp->p->h )
        goto error;

    cp->p->didwork += h2.didwork;

    DXFreeInvalidComponentHandle( cp->invalid );

    FreeCatinfo( &h2 );

    return OK;

error:
    FreeCatinfo( &h2 );
    return ERROR;
}

#endif


/* this routine takes case of simple fields, and composite fields.  in
 *  both cases, the output is 1 categorize in a simple field.
 *
 *  the code goes parallel on partitioned data.
 */
static Field
Field_Categorize( catinfo *cp )
{
    Field subo = NULL;
    Array ao;
#if YYY
    Array ao1;
    Type type;
    Group g;
    parinfo *pinfo = NULL;
    Field *f = NULL;
    int isparallel = 0;
    int items;
    int multidim;
    int members, i, j;
    int totalcnt;
    float *accum, *add ;
    float binsize, tmin, tmax;
    float gather, median;

#endif

    /* simple fields - categorize the data component */
    if ( DXGetObjectClass( cp->o ) == CLASS_FIELD ) {
        /* ignore empty fields/partitions */
        if ( DXEmptyField( ( Field ) cp->o ) )
            return ( Field ) cp->o;

        cp->o = DXCopy( ( Object ) cp->o, COPY_STRUCTURE );

        while ( ( cp->comp_name = *( cp->comp_list++ ) ) ) {
            cp->ncats = 0;
            ao = ( Array ) DXGetComponentValue( ( Field ) cp->o, cp->comp_name );
            if ( !ao ) {
                /* no data component */
                DXSetError( ERROR_MISSING_DATA, "#10240", cp->comp_name );
                return NULL;
            }

            /* this has to happen here while o is still the field */
            if ( !HasInvalid( ( Field ) cp->o, cp->comp_name, &cp->invalid ) )
                return NULL;

            if ( !_dxf_cat_FindCompType( cp ) )
                goto error;

            cp->new_comp = DXNewArray( TYPE_UINT, CATEGORY_REAL, 0, 1 );
            DXAddArrayData( cp->new_comp, 0, cp->num, NULL );
            cp->cind = DXGetArrayData( cp->new_comp );

            if ( !cp->cind )
                goto error;

            subo = ( Field ) Categorize( cp );
            cp->o = ( Object ) subo;
            DXFreeInvalidComponentHandle( cp->invalid );
        }
        return ( Field ) subo;
    }

#if YYY
    /* not a simple field - a composite field or series.  process each
     *  member in parallel and sum them at the end.
     */

    /* count the members, and return if none */
    if ( !DXGetMemberCount( ( Group ) cp->o, &members ) || ( members == 0 ) )
        return ( Field ) cp->o;


    /* the default min and max bin values must be for the entire group.
     *  set them here if they aren't already set.
     */
    if ( !cp->minset || !cp->maxset ) {
        if ( !cp->minset && !cp->maxset ) {
            if ( !ObjectVectorStats( cp->o, &multidim, &cp->md_min, &cp->md_max ) )
                return NULL;

            cp->minset = BYOBJECT;
            cp->maxset = BYOBJECT;
        }

        if ( !cp->minset ) {
            if ( !ObjectVectorStats( cp->o, &multidim, &cp->md_min, NULL ) )
                return NULL;

            cp->minset = BYOBJECT;
        }

        if ( !cp->maxset ) {
            if ( !ObjectVectorStats( cp->o, &multidim, NULL, &cp->md_max ) )
                return NULL;

            cp->maxset = BYOBJECT;
        }

        if ( ( cp->dim != multidim ) && !FullAllocCatinfo( cp, multidim, 0 ) )
            return NULL;
    }

    /* if the binsize hasn't been set yet, base it on the data type.
     *  byte data default is data range (max 256); everything else is 100 bins.
     */
    if ( !cp->binset ) {
        if ( DXGetType( cp->o, &type, NULL, NULL, NULL ) == NULL )
            return NULL;

        for ( i = 0; i < cp->dim; i++ )
            cp->md_bins[ i ] = ( type != TYPE_UBYTE ) ? DEFAULTBINS :
                               cp->md_max[ i ] - cp->md_min[ i ] + 1;

        cp->binset = BYOBJECT;
    }

    /* allocate space to hold the array of categorize fields.
     * we are going to use the first field to return the data, and
     *  extract the data from the others and add it in and then
     *  discard the rest of the fields.
     */
    pinfo = ( parinfo * ) DXAllocateZero( members * sizeof( parinfo ) );

    if ( !pinfo )
        return NULL;

    /* below here, goto error instead of just returning */

    f = ( Field * ) DXAllocateZero( members * sizeof( Field ) );
    if ( !f )
        goto error;

    /* iterate thru the members - this can happen in parallel.
     */
    if ( cp->goneparallel < cp->maxparallel ) {
        if ( !DXCreateTaskGroup() )
            goto error;
        else
            isparallel++;
    }

    g = ( Group ) cp->o;

    for ( i = 0; subo = DXGetEnumeratedMember( g, i, NULL ); i++ ) {
        cp->o = subo;
        cp->p = &pinfo[ i ];

        /* if not a field, categorize it with the higher level routine.
         *  what if it's a group????
         */

        if ( DXGetObjectClass( subo ) != CLASS_FIELD ) {
            if ( isparallel ) {
                if ( !DXAddTask( Categorize_Wrapper,
                                 ( Pointer ) cp, sizeof( catinfo ), 0.0 ) )
                    goto error;

                cp->goneparallel++;
            } else {
                f[ i ] = ( Field ) Object_Categorize( cp );
                if ( DXGetError() != ERROR_NONE )
                    goto error;
            }
            continue;
        }

        /* ignore empty fields */
        if ( DXEmptyField( ( Field ) subo ) )
            continue;

        ao = ( Array ) DXGetComponentValue( ( Field ) subo, cp->comp_name );

        if ( !ao ) {
            DXSetError( ERROR_MISSING_DATA, "#10240", cp->comp_name );
            /* no data component */
            goto error;
        }

        if ( !HasInvalid( ( Field ) subo, cp->comp_name, &cp->invalid ) )
            goto error;

        cp->o = ( Object ) ao;

        if ( isparallel ) {
            if ( !DXAddTask( Categorize_Wrapper2,
                             ( Pointer ) cp, sizeof( catinfo ), 0.0 ) )
                goto error;

            cp->goneparallel++;
        }

        else {
            f[ i ] = Array_Categorize( cp );
            if ( DXGetError() != ERROR_NONE )
                goto error;
            
            DXFreeInvalidComponentHandle( cp->invalid );
        }
    }

    if ( isparallel ) {
        if ( !DXExecuteTaskGroup() )
            goto error;

        cp->goneparallel -= members;

        /* put categorizes where they are expected to be */
        for ( i = 0; i < members; i++ ) {
            f[ i ] = ( Field ) pinfo[ i ].h;
            cp->didwork += pinfo[ i ].didwork;
        }
    }


    /* coalesce the bins into one categorize
     */
    accum = NULL;
    totalcnt = 0;
    for ( i = 0; i < members; i++ ) {
        if ( !f[ i ] || DXEmptyField( f[ i ] ) )
            continue;

        ao1 = ( Array ) DXGetComponentValue( f[ i ], cp->comp_name );

        if ( !ao1 ) {
            INTERNALERROR;
            goto error;
        }

        /* use the data array from the first field to accumulate into */
        if ( !accum ) {
            DXGetArrayInfo( ao1, &items, NULL, NULL, NULL, NULL );
            accum = ( float * ) DXGetArrayData( ao1 );

            if ( !accum ) {
                INTERNALERROR;
                goto error;
            }

            /* we will accumulate the total categorize into this field.
             *  continue here so we don't accumulate this field twice.
             */
            h = f[ i ];
            for ( j = 0; j < items; j++ )
                totalcnt += accum[ j ];

            /* XXX md - can't recompute totalcnt like this.  need slicecnt
                    * for Nd median.
             */

            continue;
        }
        add = ( float * ) DXGetArrayData( ao1 );
        if ( !add ) {
            INTERNALERROR;
            goto error;
        }
        for ( j = 0; j < items; j++ ) {
            accum[ j ] += add [ j ];
            totalcnt += add [ j ];
            /* XXX md - ditto totalcnt comment.  accum is ok */
        }

    }

    /* delete all but the accumulator field, and recompute the median */
    for ( i = 0; i < members; i++ ) {
        if ( f[ i ] && f[ i ] != h ) {
            DXDelete( ( Object ) f[ i ] );
            f[ i ] = NULL;
        }
    }

    if ( !h ) {
        DXSetError( ERROR_MISSING_DATA, "no categorize data accumulated" );
        goto error;
    }

    /* if no counts in any bin, no median */
    if ( totalcnt == 0 )
        goto done;


    /* else, set it */
    if ( cp->bins == 0 ) {
        INTERNALERROR;
        goto error;
    }


    /* XXX start median recalc */

    /* the code in this area is trying to set the binsize, and not
     *  dividing by zero during the process 
     */
    /* XXX md */
    if ( fabs( cp->max - cp->min ) < FUZZ )
        binsize = 1.0;
    else
        binsize = ( cp->max - cp->min ) / cp->bins;

    if ( binsize == 0 ) {
        INTERNALERROR;
        goto error;
    }

    /* XXX md - have to do this per axis */
    gather = 0;
    for ( i = 0; i < cp->bins; i++ ) {
        if ( ( gather + accum[ i ] ) >= ( totalcnt / 2. ) )
            break;

        gather += accum[ i ];
    }

    if ( accum[ i ] == 0 ) {
        INTERNALERROR;
        goto error;
    } else
        median = ( i * binsize ) + cp->min +
                 ( ( ( ( totalcnt / 2. ) - gather ) / accum[ i ] ) * binsize );

    cp->median = median;

    /* XXX end median calc */

#endif

#if YYY
done:
#endif
    return ( Field ) cp->o;

error:

    return NULL;
}

static int datacmp( const void *e1, const void *e2 )
{
    sortelement * s1 = ( sortelement * ) e1;
    sortelement *s2 = ( sortelement * ) e2;
    return s1->cp->cmpcat( s1->cp, s1->ph->p, s2->ph->p );
}

static int hashindexcmp( const void *e1, const void *e2 )
{
    sortelement * s1 = ( sortelement * ) e1;
    sortelement *s2 = ( sortelement * ) e2;
    return ( s1->ph->index - s2->ph->index );
}

static Field
Categorize( catinfo *cp )
{
    uint i;
#define LUT_POSTFIX " lookup"
    char buff[ 1024 ];
    char *sorted;
    uint size;
    HashTable hashtable = NULL;
    hashelement h, *ph;
    sortelement *sortlist = NULL;

    size = ( uint ) cp->obj_size;
    hashtable = DXCreateHash( ( int ) sizeof( hashelement ), NULL, cp->hashcmp );

    if ( !hashtable ) {
        DXSetError( ERROR_INTERNAL, "Unable to create hash table" );
        goto error;
    }

    for ( i = 0; i < cp->num; i++ ) {
        if ( cp->invalid && DXIsElementInvalidSequential( cp->invalid, i ) )
            break;

        h.p = ( Pointer ) &(((char *) ( cp->comp_data )) [ i * size ]);
        h.cp = cp;
        h.key = cp->catkey( &h );

        if (( ph = ( hashelement * ) DXQueryHashElement( hashtable, ( Key ) & h ))) {
#ifdef CAT_DEBUG
            printf( "%d found at %d\n", i, ph->index );
#endif
            cp->cind[ i ] = ph->index;
        } else {
            h.index = cp->ncats++;
#ifdef CAT_DEBUG
            printf( "-%s- %d not found -> ncats is %d\n", ( char * ) h.p, i, h.index );
#endif
            cp->cind[ i ] = h.index;

            if ( ERROR == DXInsertHashElement( hashtable, ( Element ) & h ) ) {
            	if(DXGetError() == ERROR_NONE)
                	DXSetError( ERROR_INTERNAL, "Hash table internal error" );
                goto error;
            }
        }
    }

    sortlist = DXAllocate( cp->ncats * sizeof( sortelement ) );
    DXInitGetNextHashElement( hashtable );

    for ( i = 0; i < cp->ncats; i++ ) {
        sortlist[ i ].ph = DXGetNextHashElement( hashtable );
        sortlist[ i ].cp = cp;
    }

    /* sort based on hash data and set sortindex */
	if(cp->sort)
        qsort( sortlist, cp->ncats, sizeof( sortelement ), datacmp );

    for ( i = 0; i < cp->ncats; i++ )
        sortlist[ i ].sortindex = i;

    /* sort based on hash index */
    qsort( sortlist, cp->ncats, sizeof( sortelement ), hashindexcmp );

    /* do lookup replacement */
    for ( i = 0; i < cp->num; i++ ) {
        if ( cp->invalid && DXIsElementInvalidSequential( cp->invalid, i ) ) {
            cp->cind[ i ] = 0;
        } else {
            cp->cind[ i ] = sortlist[ cp->cind[ i ] ].sortindex;
        }
    }

    /* convert to ubytes if few enough categories */
    if ( cp->ncats < 257 ) {
        Array a;
        ubyte *d;
        a = DXNewArray( TYPE_UBYTE, CATEGORY_REAL, 0, 1 );
        DXAddArrayData( a, 0, cp->num, NULL );

        d = DXGetArrayData( a );

        if ( !d )
            goto error;

        for ( i = 0; i < cp->num; i++ )
            d[ i ] = ( ubyte ) cp->cind[ i ];

        DXDelete( ( Object ) cp->new_comp );

        cp->new_comp = a;
    }

    DXChangedComponentStructure( ( Field ) cp->o, cp->comp_name );
    cp->unique_array = DXNewArrayV( cp->comp_type, CATEGORY_REAL, cp->comp_rank, cp->comp_shape );

    if ( !cp->unique_array )
        goto error;

    DXAddArrayData( cp->unique_array, 0, cp->ncats, NULL );
    DXTrim( cp->unique_array );
    sorted = ( char * ) DXGetArrayData( cp->unique_array );

    for ( i = 0; i < cp->ncats; i++ )
        memcpy( &sorted[ sortlist[ i ].sortindex * size ], sortlist[ i ].ph->p, size );

    DXFree( sortlist );
    sortlist = NULL;
    DXDestroyHash( hashtable );
    hashtable = NULL;
    strcpy( buff, cp->comp_name );
    strcat( buff, LUT_POSTFIX );
    DXSetComponentValue( ( Field ) cp->o, buff, ( Object ) cp->unique_array );

    /*  Don't think der should be set */
    /* DXSetComponentAttribute((Field)cp->o, buff, "der", (Object)DXNewString(cp->comp_name)); */

    DXSetComponentValue( ( Field ) cp->o, cp->comp_name, ( Object ) cp->new_comp );

    if ( cp->invalid )
        DXSaveInvalidComponent( ( Field ) cp->o, cp->invalid );

    /* Can't set it ref lookup since it might be ubyte and the exec refs to be int */
    /* DXSetComponentAttribute((Field)cp->o, cp->comp_name, "ref", (Object)DXNewString(buff)); */

    if ( !DXEndField( ( Field ) cp->o ) )
        goto error;

    return ( Field ) cp->o;

error:
    if ( sortlist )
        DXFree( sortlist );

    if ( hashtable )
        DXDestroyHash( hashtable );

    if ( cp->unique_array )
        DXDelete( ( Object ) cp->unique_array );

    if ( cp->new_comp )
        DXDelete( ( Object ) cp->new_comp );

    return NULL;

}


static Error HasInvalid( Field f, char *component, ICH *ih )
{
    Error rc = OK;
    char *dep = NULL;
    char *invalid = NULL;

    *ih = NULL;

    if ( !DXGetStringAttribute( DXGetComponentValue( f, component ), "dep", &dep ) )
        return OK;

#define INVLEN 10     /* strlen("invalid "); */

    if ( !( invalid = ( char * ) DXAllocate( strlen( dep ) + INVLEN ) ) )
        return ERROR;

    strcpy( invalid, "invalid " );
    strcat( invalid, dep );

    /* if no component, return NULL */
    if ( !DXGetComponentValue( f, invalid ) )
        goto done;

    *ih = DXCreateInvalidComponentHandle( ( Object ) f, NULL, dep );

    if ( !*ih ) {
        rc = ERROR;
        goto done;
    }

done:
    DXFree( ( Pointer ) invalid );
    return rc;
}

