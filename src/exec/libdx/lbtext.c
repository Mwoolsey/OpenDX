/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>

#include <stdio.h>
#include <dx/dx.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#if  defined(DXD_NON_UNIX_DIR_SEPARATOR)
#define DX_DIR_SEPARATOR ';'
#else
#define DX_DIR_SEPARATOR ':'
#endif


static Error verifyfont( Group font, char * );
static Error getname( char *dirlist, char *file, char *result );

/* changed.  used to read old proprietary format; now uses import
 *  to read a normal .dx format file.
 *
 * a font is a group of 256 fields, one for each character value,
 * with 2 attributes attached to the group: 
 *  "font ascent" which is a single float of max height above baseline
 *  "font descent" which is a single positive float of max depth below baseline
 * (these are for all chars combined in this font)  the combined height
 * should be 1.0
 *
 * each field must have 2d or 3d positions and either line or triangle 
 * connections, and have one attribute:
 *  "char width" which is a float for the spacing of this char
 * eventually the points or connections can have colors and/or normals.
 */

Object
DXGetFont( char *name, float *ascent, float *descent )
{
    Group font = NULL;
    char *tbuf = NULL;
    char *dir = NULL;
    float fm;

    if ( !name )
        return NULL;

    /* look in cache first */
    font = ( Group ) DXGetCacheEntry( name, 0, 0 );

    if ( font )
        goto done;

    /* where to look */
    dir = ( char * ) getenv( "DXFONTS" );

    if ( !dir )
        dir = ( char * ) getenv( "DXEXECROOT" );

    if ( !dir )
        dir = ( char * ) getenv( "DXROOT" );

    if ( !dir )
        dir = "/usr/local/dx";

#define XTRA 32  /* enough extra room for "../fonts/.. .dx" */

    tbuf = ( char * ) DXAllocateLocalZero( strlen( dir ) + strlen( name ) + XTRA );

    if ( !tbuf )
        goto error;

    /* try the places listed in the environment variable.
     *  the filename to open is returned in tbuf.
     */
    if ( !getname( dir, name, tbuf ) )
        goto error;

    /* try to open the file */
    font = ( Group ) DXImportDX( tbuf, NULL, NULL, NULL, NULL );

    if ( !font ) {
        DXAddMessage( "as font file" );
        goto error;
    }

    /* make sure the object structure is ok */
    if ( !verifyfont( font, name ) )
        goto error;

    /* put in cache */
    DXReference( ( Object ) font );

    if ( !DXSetCacheEntry( ( Object ) font, 0.0, name, 0, 0 ) )
        goto error;

done:
    /* return the info if the caller asks for it */
    if ( !DXGetFloatAttribute( ( Object ) font, "font ascent", &fm ) ) {
        DXSetError( ERROR_BAD_PARAMETER, "#10810", name );
        goto error;
    }

    if ( ascent )
        * ascent = fm;

    if ( !DXGetFloatAttribute( ( Object ) font, "font descent", &fm ) ) {
        DXSetError( ERROR_BAD_PARAMETER, "#10812", name );
        goto error;
    }

    if ( descent )
        * descent = fm;


    DXFree( ( Pointer ) tbuf );

    return ( Object ) font;

error:
    DXDelete( ( Object ) font );

    DXFree( ( Pointer ) tbuf );

    return NULL;
}

/* find the font file
 */
static Error
getname( char *dirlist, char *file, char *result )
{
    int fd = -1;
    char *dirbuf = NULL;
    char *dp, *cp;

    /* if absolute pathname, don't fool with directories.
     */
#if !defined(DXD_NON_UNIX_DIR_SEPARATOR)

    if ( file[ 0 ] == '/' ) {
#else

    if ( file[ 0 ] == '/' || file[ 0 ] == '\\' || file[ 1 ] == ':' ) {
#endif
        strcpy( result, file );
        return OK;
    }

    /* allocate space to construct variations of the given filename.
     *  get enough space the first time so we can construct any variation
     *  without having to reallocate (xtra defined up above).
     */
    dirbuf = ( char * ) DXAllocateLocalZero( strlen( dirlist ) + strlen( file ) + XTRA );

    if ( !dirbuf )
        return ERROR;


    /* allow dirlist to be a colon separated list of directory names.
     *  try dir/name, dir/name.dx, dir/fonts/name, dir/fonts/name.dx
     */
    dp = dirlist;

    while ( dp ) {

        strcpy( dirbuf, dp );

        if ( ( cp = strchr( dirbuf, DX_DIR_SEPARATOR ) ) != NULL )
            * cp = '\0';

        strcat( dirbuf, "/" );

        strcat( dirbuf, file );

        if ( ( fd = open( dirbuf, O_RDONLY ) ) >= 0 )
            goto done;

        strcat( dirbuf, ".dx" );

        if ( ( fd = open( dirbuf, O_RDONLY ) ) >= 0 )
            goto done;

        strcpy( dirbuf, dp );

        if ( ( cp = strchr( dirbuf, DX_DIR_SEPARATOR ) ) != NULL )
            * cp = '\0';

        strcat( dirbuf, "/fonts/" );

        strcat( dirbuf, file );

        if ( ( fd = open( dirbuf, O_RDONLY ) ) >= 0 )
            goto done;

        strcat( dirbuf, ".dx" );

        if ( ( fd = open( dirbuf, O_RDONLY ) ) >= 0 )
            goto done;

        dp = strchr( dp, DX_DIR_SEPARATOR );

        if ( dp )
            dp++;
    }

    /*  error: */
    /* only gets here on error */
    DXSetError( ERROR_DATA_INVALID, "#10800", file );

    result[ 0 ] = '\0';

    DXFree( ( Pointer ) dirbuf );

    return ERROR;


done:
    if ( fd >= 0 )
        close( fd );

    strcpy( result, dirbuf );

    DXFree( ( Pointer ) dirbuf );

    return OK;
}

static Error
verifyfont( Group font, char * name ) {
    Object subo;
    float fm;
    int items;


    /* do sanity checks now on things we will assume are ok later
     */

    if ( DXGetObjectClass( ( Object ) font ) != CLASS_GROUP ) {
        DXSetError( ERROR_BAD_PARAMETER, "#10802", name );
        goto error;
    }

    if ( !DXGetMemberCount( font, &items ) || items != 256 ) {
        DXSetError( ERROR_BAD_PARAMETER, "#10802", name );
        goto error;
    }

    for ( items = 0; ( subo = DXGetEnumeratedMember( font, items, NULL ) ); items++ ) {
        if ( DXGetObjectClass( subo ) != CLASS_FIELD ) {
            DXSetError( ERROR_BAD_PARAMETER, "#10804", name );
            goto error;
        }

        if ( DXEmptyField( ( Field ) subo ) )
            continue;

        if ( !DXGetFloatAttribute( ( Object ) subo, "char width", &fm ) ) {
            DXSetError( ERROR_BAD_PARAMETER, "#10806", name );
            goto error;
        }
    }

    if ( !DXGetFloatAttribute( ( Object ) font, "font ascent", &fm ) ) {
        DXSetError( ERROR_BAD_PARAMETER, "#10810", name );
        goto error;
    }

    if ( !DXGetFloatAttribute( ( Object ) font, "font descent", &fm ) ) {
        DXSetError( ERROR_BAD_PARAMETER, "#10812", name );
        goto error;
    }

    return OK;

error:
    return ERROR;
}

Object
DXGeometricText( char * s, Object font, float * stringwidth ) {
    unsigned char * p;
    double offsetx, offsety, maxlinewidth;
    Array a, points_array = NULL, conn_array = NULL;
    Field f, newf = NULL;
    float charwidth, lineheight, fascent, fdescent, lead=.133;
    Pointer conn, newconn;
    float *newpos;
    Point *newpoints;
    Line *lines, *newlines;
    Triangle *tris, *newtris;
    int pointdim, connsize = 0;
    int npoints, nconnect = 0, np, nc, i;
    int p1, q1, r1;
    char *conntype;
    static RGBColor white = { 1, 1, 1 };

    /* output field: newpoints.  do connections after you know whether
     * they are lines or triangles.
     */
    newf = DXNewField();

    if ( !newf )
        goto error;

    points_array = DXNewArray( TYPE_FLOAT, CATEGORY_REAL, 1, 3 );

    if ( !points_array )
        goto error;

    if ( !DXSetComponentValue( newf, "positions", ( Object ) points_array ) )
        goto error;

    npoints = 0;
    offsetx = 0.0;
    maxlinewidth = 0.0;
    offsety = 0.0;
    
    if ( !DXGetFloatAttribute( ( Object ) font, "font ascent", &fascent ) )
        fascent = 0.0;
    if ( !DXGetFloatAttribute( ( Object ) font, "font descent", &fdescent ) )
        fdescent = 0.0;
    lineheight = fascent + fdescent + lead;

    /* for each character in the string...
     */
    for ( p = ( unsigned char * ) s; *p; p++ ) {

        f = ( Field ) DXGetEnumeratedMember( ( Group ) font, *p, NULL );

        if ( !f )
            continue;

    	if (*p == '\n') {
    		offsetx = 0.0;
    		offsety -= lineheight;
    		continue;
    	}

        if ( !DXGetFloatAttribute( ( Object ) f, "char width", &charwidth ) )
            charwidth = 0.0;

        /* get the points and lines/triangles associated with this character.
         * allow empty fields to have a width.
         */
        a = ( Array ) DXGetComponentValue( f, "connections" );

        DXGetArrayInfo( a, &nc, NULL, NULL, NULL, NULL );

        if ( !a || nc <= 0 ) {
            offsetx += charwidth;
            continue;
        }

        newconn = DXGetArrayData( a );

        /* make the new connections component only after you've seen the
         * first connections component so you can make the right type.
         */

        if ( !conn_array ) {
            if ( !DXGetStringAttribute( ( Object ) a, "element type", &conntype ) ||
                    ( strcmp( conntype, "lines" ) && strcmp( conntype, "triangles" ) ) ) {
                DXSetError( ERROR_DATA_INVALID, "#10814", "object" );
                goto error;
            }

            connsize = strcmp( conntype, "lines" ) ? 3 : 2;
            conn_array = DXNewArray( TYPE_INT, CATEGORY_REAL, 1, connsize );

            if ( !conn_array )
                goto error;

            DXSetConnections( newf, connsize == 2 ? "lines" : "triangles",
                              conn_array );

            nconnect = 0;
        }

        a = ( Array ) DXGetComponentValue( f, "positions" );

        if ( !a ) {
            offsetx += charwidth;
            continue;
        }

        newpos = ( float * ) DXGetArrayData( a );
        DXGetArrayInfo( a, NULL, NULL, NULL, NULL, &pointdim );

        if ( pointdim != 2 && pointdim != 3 ) {
            DXSetError( ERROR_DATA_INVALID, "#10816", "object" );
            goto error;
        }

        np = nc * connsize;

        /* add new points, connections */

        if ( !DXAddArrayData( points_array, npoints, np, NULL ) )
            goto error;

        newpoints = ( ( Point * ) DXGetArrayData( points_array ) ) + npoints;

        if ( !DXAddArrayData( conn_array, nconnect, nc, NULL ) )
            goto error;

        conn = ( Pointer ) ( ( int * ) DXGetArrayData( conn_array ) +
                             nconnect * connsize );

        if ( connsize == 2 ) {
            lines = ( Line * ) conn;
            newlines = ( Line * ) newconn;

            for ( i = 0; i < nc; i++ ) {
                p1 = newlines[ i ].p;
                q1 = newlines[ i ].q;
                newpoints[ 2 * i ] = DXPt( newpos[ p1 * pointdim + 0 ] + offsetx,
                                           newpos[ p1 * pointdim + 1 ] + offsety,
                                           pointdim == 2 ? 0.0 : newpos[ p1 * pointdim + 2 ] );
                newpoints[ 2 * i + 1 ] = DXPt( newpos[ q1 * pointdim + 0 ] + offsetx,
                                               newpos[ q1 * pointdim + 1 ] + offsety,
                                               pointdim == 2 ? 0.0 : newpos[ q1 * pointdim + 2 ] );
                lines[ i ] = DXLn( npoints + 2 * i, npoints + 2 * i + 1 );
            }
        }
        else {
            tris = ( Triangle * ) conn;
            newtris = ( Triangle * ) newconn;

            for ( i = 0; i < nc; i++ ) {
                p1 = newtris[ i ].p;
                q1 = newtris[ i ].q;
                r1 = newtris[ i ].r;
                newpoints[ 3 * i ] = DXPt( newpos[ p1 * pointdim + 0 ] + offsetx,
                                           newpos[ p1 * pointdim + 1 ] + offsety,
                                           pointdim == 2 ? 0.0 : newpos[ p1 * pointdim + 2 ] );
                newpoints[ 3 * i + 1 ] = DXPt( newpos[ q1 * pointdim + 0 ] + offsetx,
                                               newpos[ q1 * pointdim + 1 ] + offsety,
                                               pointdim == 2 ? 0.0 : newpos[ q1 * pointdim + 2 ] );
                newpoints[ 3 * i + 2 ] = DXPt( newpos[ r1 * pointdim + 0 ] + offsetx,
                                               newpos[ r1 * pointdim + 1 ] + offsety,
                                               pointdim == 2 ? 0.0 : newpos[ r1 * pointdim + 2 ] );
                tris[ i ] = DXTri( npoints + 3 * i, npoints + 3 * i + 1, npoints + 3 * i + 2 );
            }
        }

        offsetx += charwidth;
        if ( maxlinewidth < offsetx ) maxlinewidth = offsetx;
        npoints += np;
        nconnect += nc;
    }

    /* colors */
    a = ( Array ) DXNewConstantArray( npoints, ( Pointer ) & white,
                                      TYPE_FLOAT, CATEGORY_REAL, 1, 3 );

    if ( !a )
        goto error;

    DXSetComponentValue( newf, "colors", ( Object ) a );


    if ( stringwidth )
        * stringwidth = maxlinewidth;

    DXSetStringAttribute( ( Object ) newf, "Text string", s );

    return ( Object ) DXEndField( newf );

error:
    DXDelete( ( Object ) newf );

    return NULL;
}
