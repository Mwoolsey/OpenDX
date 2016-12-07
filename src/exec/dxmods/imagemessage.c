/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dxmods/imagemessage.c,v 1.7 2006/01/03 17:02:23 davidt Exp $
 */

#include <dxconfig.h>

#if defined(HAVE_STRING_H)
#include <string.h>
#endif

#include "dx/dx.h"
#include "interact.h"

#define BACKGROUNDCOLOR  in[1]
#define THROTTLE  in[2]
#define RECENABLE  in[3]
#define RECFILE   in[4]
#define RECFORMAT  in[5]
#define RECRES   in[6]
#define RECASPECT  in[7]
#define AAENABLED  in[8]
#define AALABELS  in[9]
#define AATICKS   in[10]
#define AACORNERS  in[11]
#define AAFRAME   in[12]
#define AAADJUST  in[13]
#define AACURSOR  in[14]
#define AAGRID   in[15]
#define AACOLORS  in[16]
#define AAANNOTATION  in[17]
#define AALABELSCALE  in[18]
#define AAFONT   in[19]
#define AAXTICKLOCS  in[20]
#define AAYTICKLOCS  in[21]
#define AAZTICKLOCS  in[22]
#define AAXTICKLABELS  in[23]
#define AAYTICKLABELS  in[24]
#define AAZTICKLABELS  in[25]
#define INTERACTIONMODE  in[26]
#define TITLE   in[27]
#define RECMODE   in[28]
#define BUAPPX   in[29]
#define BDAPPX   in[30]
#define BUDEN   in[31]
#define BDDEN   in[32]

#define NSAVED   32

#define LAST_BACKGROUNDCOLOR last[0]
#define LAST_THROTTLE  last[1]
#define LAST_RECENABLE  last[2]
#define LAST_RECFILE  last[3]
#define LAST_RECFORMAT  last[4]
#define LAST_RECRES  last[5]
#define LAST_RECASPECT  last[6]
#define LAST_AAENABLED  last[7]
#define LAST_AALABELS  last[8]
#define LAST_AATICKS  last[9]
#define LAST_AACORNERS  last[10]
#define LAST_AAFRAME  last[11]
#define LAST_AAADJUST  last[12]
#define LAST_AACURSOR  last[13]
#define LAST_AAGRID  last[14]
#define LAST_AACOLORS  last[15]
#define LAST_AAANNOTATION last[16]
#define LAST_AALABELSCALE last[17]
#define LAST_AAFONT  last[18]
#define LAST_AAXTICKLOCS last[19]
#define LAST_AAYTICKLOCS last[20]
#define LAST_AAZTICKLOCS last[21]
#define LAST_AAXTICKLABELS last[22]
#define LAST_AAYTICKLABELS last[23]
#define LAST_AAZTICKLABELS last[24]
#define LAST_INTERACTIONMODE last[25]
#define LAST_TITLE  last[26]
#define LAST_RECMODE  last[27]
#define LAST_BUAPPX  last[28]
#define LAST_BDAPPX  last[29]
#define LAST_BUDEN  last[30]
#define LAST_BDDEN  last[31]

#define TEST(a,b) \
  ((b) && (!(a) || (DXGetObjectTag(a) != DXGetObjectTag(b))))

#define REPLACE(a,b)   \
{    \
    if (a) DXDelete((a)); \
    DXReference((b));  \
    (a) = (b);   \
}

static Error delete_PendingDXLMessage( Pointer );
static Error SendPendingDXLMessage( Private );
static Error SendPendingMessage( char *, char *, char * );

#define STRCAT(dst, src)  \
{     \
    char *c = src;   \
    if (*c)    \
    {     \
 while (*c != '\0')  \
     *dst++ = *c++;  \
 *dst = '\0';   \
    }     \
}

Error
m_ImageMessage( Object *in, Object *out )
{
    Array a;
    Object *last;
    char buffer0[ MAX_MSGLEN ], buffer1[ MAX_MSGLEN ];
    char *b0ptr = buffer0;
    char *mod_id = ( char * ) DXGetModuleId();
    int doit = 0;
    char *sendtype;

    if ( ! in[ 0 ] ) {
        DXSetError( ERROR_MISSING_DATA, "id" );
        return ERROR;
    }

    sendtype = DXGetString( ( String ) in[ 0 ] );

    a = ( Array ) DXGetCacheEntry( mod_id, 0, 0 );

    if ( ! a ) {
        a = DXNewArray( TYPE_BYTE, CATEGORY_REAL, 0 );

        if ( ! a )
            goto error;

        if ( ! DXAddArrayData( a, 0, NSAVED * sizeof( Object ), NULL ) )
            goto error;

        memset( ( char * ) DXGetArrayData( a ), 0, NSAVED * sizeof( Object ) );

        if ( ! DXSetCacheEntry( ( Object ) a, CACHE_PERMANENT, mod_id, 0, 0 ) )
            goto error;
    }

    DXFreeModuleId( ( Pointer ) mod_id );

    last = ( Object * ) DXGetArrayData( a );

    buffer0[ 0 ] = '\0';

    if ( TEST( LAST_BACKGROUNDCOLOR, BACKGROUNDCOLOR ) ) {
        if ( BACKGROUNDCOLOR ) {
            char * color, cbuf[ 128 ];
            float vec[ 3 ];

            if ( ! DXExtractString( BACKGROUNDCOLOR, &color ) ) {
                if ( !DXExtractParameter( BACKGROUNDCOLOR, TYPE_FLOAT,
                                          3, 1, ( Pointer ) vec ) ) {
                    DXSetError( ERROR_BAD_PARAMETER,
                                "image background color must be a string or 3-vector" );
                    goto error;
                }

                sprintf( cbuf, "[%g %g %g]", vec[ 0 ], vec[ 1 ], vec[ 2 ] );
                color = cbuf;
            }

            STRCAT( b0ptr, " backgroundColor=" );
            STRCAT( b0ptr, color );
            STRCAT( b0ptr, ";" );
            doit = 1;
        }

        REPLACE( LAST_BACKGROUNDCOLOR, BACKGROUNDCOLOR );
    }

    if ( TEST( LAST_THROTTLE, THROTTLE ) ) {
        if ( THROTTLE ) {
            float throttle;

            if ( ! DXExtractFloat( THROTTLE, &throttle ) ) {
                DXSetError( ERROR_BAD_PARAMETER,
                            "throttle must be a scalar" );
                goto error;
            }

            sprintf( buffer1, " throttle=%g", throttle );
            STRCAT( b0ptr, buffer1 );
            STRCAT( b0ptr, ";" );
            doit = 1;
        }

        REPLACE( LAST_THROTTLE, THROTTLE );
    }

    if ( TEST( LAST_RECENABLE, RECENABLE ) ) {
        if ( RECENABLE ) {
            int recenable;

            if ( ! DXExtractInteger( RECENABLE, &recenable ) ||
                    ( recenable != 0 && recenable != 1 ) ) {
                DXSetError( ERROR_BAD_PARAMETER,
                            "recenable must be a integer either 0 or 1" );
                goto error;
            }

            sprintf( buffer1, " recenable=%d", recenable );
            STRCAT( b0ptr, buffer1 );
            STRCAT( b0ptr, ";" );
            doit = 1;
        }

        REPLACE( LAST_RECENABLE, RECENABLE );
    }

    if ( TEST( LAST_RECFILE, RECFILE ) ) {
        if ( RECFILE ) {
            char * file;

            if ( ! DXExtractString( RECFILE, &file ) ) {
                DXSetError( ERROR_BAD_PARAMETER,
                            "recorded image filename must be a string" );
                goto error;
            }

            STRCAT( b0ptr, " recfile=" );
            STRCAT( b0ptr, file );
            STRCAT( b0ptr, ";" );
            doit = 1;
        }

        REPLACE( LAST_RECFILE, RECFILE );
    }

    if ( TEST( LAST_RECFORMAT, RECFORMAT ) ) {
        if ( RECFORMAT ) {
            char * format;

            if ( ! DXExtractString( RECFORMAT, &format ) ) {
                DXSetError( ERROR_BAD_PARAMETER,
                            "recorded image format must be a string" );
                goto error;
            }

            STRCAT( b0ptr, " recformat=" );
            STRCAT( b0ptr, format );
            STRCAT( b0ptr, ";" );
            doit = 1;
        }

        REPLACE( LAST_RECFORMAT, RECFORMAT );
    }

    if ( TEST( LAST_RECRES, RECRES ) ) {
        if ( RECRES ) {
            int res;

            if ( ! DXExtractInteger( RECRES, &res ) ) {
                DXSetError( ERROR_BAD_PARAMETER,
                            "recorded image resolution must be an integer" );
                goto error;
            }

            sprintf( buffer1, " recresolution=%d;", res );
            STRCAT( b0ptr, buffer1 );
            doit = 1;
        }

        REPLACE( LAST_RECRES, RECRES );
    }

    if ( TEST( LAST_RECASPECT, RECASPECT ) ) {
        if ( RECASPECT ) {
            float aspect;

            if ( ! DXExtractFloat( RECASPECT, &aspect ) ) {
                DXSetError( ERROR_BAD_PARAMETER,
                            "recorded image aspect must be a float" );
                goto error;
            }

            sprintf( buffer1, " recaspect=%g;", aspect );
            STRCAT( b0ptr, buffer1 );
            doit = 1;
        }

        REPLACE( LAST_RECASPECT, RECASPECT );
    }

    if ( TEST( LAST_AAENABLED, AAENABLED ) ) {
        if ( AAENABLED ) {
            int aaenable;

            if ( ! DXExtractInteger( AAENABLED, &aaenable ) ||
                    ( aaenable != 0 && aaenable != 1 ) ) {
                DXSetError( ERROR_BAD_PARAMETER,
                            "aaenable must be a integer either 0 or 1" );
                goto error;
            }

            sprintf( buffer1, " aaenabled=%d", aaenable );
            STRCAT( b0ptr, buffer1 );
            STRCAT( b0ptr, ";" );
            doit = 1;
        }

        REPLACE( LAST_AAENABLED, AAENABLED );
    }

    if ( TEST( LAST_AALABELS, AALABELS ) ) {
        if ( AALABELS ) {
        	int len;
            char ** labels = NULL;
            int i, j;
	        DXGetArrayInfo ((Array) AALABELS, &len, NULL, NULL, NULL, NULL);
	        labels = (char **) DXAllocate((len + 1) * sizeof(char **));

            for ( i = 0; DXExtractNthString( AALABELS, i, labels + i ); i++ );

            if ( i == 1 ) {
                STRCAT( b0ptr, "aalabel=" );
                STRCAT( b0ptr, labels[ 0 ] );
                STRCAT( b0ptr, ";" );
            }

            else if ( i > 1 ) {
                STRCAT( b0ptr, "aalabellist={\"" );

                for ( j = 0; j < i; j++ ) {
                    if ( j != 0 )
                        STRCAT( b0ptr, "\", \"" );

                    STRCAT( b0ptr, labels[ j ] );
                }

                STRCAT( b0ptr, "\"};" );
            }

			if(labels)
				DXFree(labels);
				
            doit = 1;
        }

        REPLACE( LAST_AALABELS, AALABELS );
    }

    if ( TEST( LAST_AATICKS, AATICKS ) ) {
        if ( AATICKS ) {
            int tics[ 3 ];

            if ( DXExtractParameter( AATICKS, TYPE_INT, 1, 3, ( Pointer ) tics ) )
                sprintf( buffer1, " aaticklist={ %d %d %d }",
                         tics[ 0 ], tics[ 1 ], tics[ 2 ] );
            else if ( DXExtractInteger( AATICKS, tics ) )
                sprintf( buffer1, " aatick=%d", tics[ 0 ] );
            else {
                DXSetError( ERROR_BAD_PARAMETER,
                            "aaticks must be an integer or an integer list of 3 elements" );
                goto error;
            }

            STRCAT( b0ptr, buffer1 );
            STRCAT( b0ptr, ";" );
            doit = 1;
        }

        REPLACE( LAST_AATICKS, AATICKS );
    }

    if ( TEST( LAST_AACORNERS, AACORNERS ) ) {
        if ( AACORNERS ) {
            float corners[ 6 ];
            Point box[ 8 ];

            if ( DXExtractParameter( AACORNERS, TYPE_FLOAT,
                                 3, 2, ( Pointer ) corners ) ) {}
            else if ( DXExtractParameter( AACORNERS, TYPE_FLOAT,
                                          2, 2, ( Pointer ) corners ) ) {
                corners[ 5 ] = 0;
                corners[ 4 ] = corners[ 3 ];
                corners[ 3 ] = corners[ 2 ];
                corners[ 2 ] = 0;
            }

            else if ( DXExtractParameter( AACORNERS, TYPE_FLOAT,
                                          1, 2, ( Pointer ) corners ) ) {
                corners[ 5 ] = 0;
                corners[ 4 ] = 0;
                corners[ 3 ] = corners[ 1 ];
                corners[ 2 ] = 0;
                corners[ 1 ] = 0;
            }

            else {
                if ( ! DXBoundingBox( AACORNERS, box ) ) {
                    DXSetError( ERROR_BAD_PARAMETER, "AA corners" );
                    goto error;
                }

                corners[ 0 ] = box[ 0 ].x;
                corners[ 1 ] = box[ 0 ].y;
                corners[ 2 ] = box[ 0 ].z;
                corners[ 3 ] = box[ 7 ].x;
                corners[ 4 ] = box[ 7 ].y;
                corners[ 5 ] = box[ 7 ].z;
            }

            sprintf( buffer1, " aacorners={[%g %g %g] [%g %g %g]};",
                     corners[ 0 ], corners[ 1 ], corners[ 2 ],
                     corners[ 3 ], corners[ 4 ], corners[ 5 ] );

            STRCAT( b0ptr, buffer1 );
            doit = 1;
        }

        REPLACE( LAST_AACORNERS, AACORNERS );
    }

    if ( TEST( LAST_AAFRAME, AAFRAME ) ) {
        if ( AAFRAME ) {
            int aaframe;

            if ( ! DXExtractInteger( AAFRAME, &aaframe ) ||
                    ( aaframe != 0 && aaframe != 1 ) ) {
                DXSetError( ERROR_BAD_PARAMETER,
                            "aaframe must be a integer either 0 or 1" );
                goto error;
            }

            sprintf( buffer1, " aaframe=%d", aaframe );
            STRCAT( b0ptr, buffer1 );
            STRCAT( b0ptr, ";" );
            doit = 1;
        }

        REPLACE( LAST_AAFRAME, AAFRAME );
    }

    if ( TEST( LAST_AAADJUST, AAADJUST ) ) {
        if ( AAADJUST ) {
            int aaadjust;

            if ( ! DXExtractInteger( AAADJUST, &aaadjust ) ||
                    ( aaadjust != 0 && aaadjust != 1 ) ) {
                DXSetError( ERROR_BAD_PARAMETER,
                            "aaadjust must be a integer either 0 or 1" );
                goto error;
            }

            sprintf( buffer1, " aaadjust=%d", aaadjust );
            STRCAT( b0ptr, buffer1 );
            STRCAT( b0ptr, ";" );
            doit = 1;
        }

        REPLACE( LAST_AAADJUST, AAADJUST );
    }

    if ( TEST( LAST_AACURSOR, AACURSOR ) ) {
        if ( AACURSOR ) {
            float cursor[ 3 ];

            if ( ! DXExtractParameter( AACURSOR, TYPE_FLOAT,
                                       3, 1, ( Pointer ) cursor ) ) {
                DXSetError( ERROR_BAD_PARAMETER,
                            "aacursor must be a float 3-vector" );
                goto error;
            }

            sprintf( buffer1, " aacursor=%g,%g,%g",
                     cursor[ 0 ], cursor[ 1 ], cursor[ 2 ] );

            STRCAT( b0ptr, buffer1 );
            STRCAT( b0ptr, ";" );
            doit = 1;
        }

        REPLACE( LAST_AACURSOR, AACURSOR );
    }

    if ( TEST( LAST_AAGRID, AAGRID ) ) {
        if ( AAGRID ) {
            int aagrid;

            if ( ! DXExtractInteger( AAGRID, &aagrid ) ||
                    ( aagrid != 0 && aagrid != 1 ) ) {
                DXSetError( ERROR_BAD_PARAMETER,
                            "aagrid must be a integer either 0 or 1" );
                goto error;
            }

            sprintf( buffer1, " aagrid=%d", aagrid );
            STRCAT( b0ptr, buffer1 );
            STRCAT( b0ptr, ";" );
            doit = 1;
        }

        REPLACE( LAST_AAGRID, AAGRID );
    }

    if ( TEST( LAST_AACOLORS, AACOLORS ) ) {
        if ( AACOLORS ) {
            char * color;

            if ( DXExtractNthString( AACOLORS, 0, &color ) ) {
                char * colors[ 32 ];
                int j, i = 0;

                for ( i = 0; DXExtractNthString( AACOLORS, i, colors + i ); i++ );

                if ( i == 1 ) {
                    STRCAT( b0ptr, "aacolor=" );
                    STRCAT( b0ptr, colors[ 0 ] );
                    STRCAT( b0ptr, ";" )
                }

                else if ( i > 1 ) {
                    STRCAT( b0ptr, "aacolorlist={\"" );

                    for ( j = 0; j < i; j++ ) {
                        if ( j != 0 )
                            STRCAT( b0ptr, "\", \"" );

                        STRCAT( b0ptr, colors[ j ] );
                    }

                    STRCAT( b0ptr, "\"};" );
                }
            }

            else {
                int n;
                char *p;

                if ( DXGetObjectClass( AACOLORS ) != CLASS_ARRAY ) {
                    DXSetError( ERROR_BAD_PARAMETER,
                                "aacolors must be list of strings or 3-vectors" );
                    goto error;
                }

                DXGetArrayInfo( ( Array ) AACOLORS, &n, NULL, NULL, NULL, NULL );

                if ( n == 1 )
                    STRCAT( b0ptr, "aacolor=" )
                    else
                        STRCAT( b0ptr, "aacolorlist={" );

                p = buffer0 + strlen( buffer0 );

                if ( DXTypeCheck( ( Array ) AACOLORS, TYPE_FLOAT, CATEGORY_REAL,
                                  1, 3 ) ) {
                    int i;
                    float *foo = ( float * ) DXGetArrayData( ( Array ) AACOLORS );

                    for ( i = 0; i < n; i++, foo += 3 )
                        sprintf( p, " [ %g %g %g ]", foo[ 0 ], foo[ 1 ], foo[ 2 ] );

                    while ( *p++ != ']' );
                }

                else if ( DXTypeCheck( ( Array ) AACOLORS, TYPE_INT,
                                       CATEGORY_REAL, 1, 3 ) ) {
                    int i;
                    int *foo = ( int * ) DXGetArrayData( ( Array ) AACOLORS );

                    for ( i = 0; i < n; i++, foo += 3 )
                        sprintf( p, " [ %d %d %d ]", foo[ 0 ], foo[ 1 ], foo[ 2 ] );

                    while ( *p++ != ']' );
                }

                else {
                    DXSetError( ERROR_BAD_PARAMETER,
                                "aacolors must be list of strings or 3-vectors" );
                    goto error;
                }

                if ( n == 1 )
                    STRCAT( b0ptr, ";" )
                    else
                        STRCAT( b0ptr, "};" );

            }

            doit = 1;
        }

        REPLACE( LAST_AACOLORS, AACOLORS );
    }

    if ( TEST( LAST_AAANNOTATION, AAANNOTATION ) ) {
        if ( AAANNOTATION ) {
            char * annot[ 32 ];
            int i, j;

            for ( i = 0; DXExtractNthString( AAANNOTATION, i, annot + i ); i++ );

            if ( i == 1 ) {
                STRCAT( b0ptr, "aaannotation=\"" );
                STRCAT( b0ptr, annot[ 0 ] );
                STRCAT( b0ptr, "\";" );
            }

            else if ( i > 1 ) {
                STRCAT( b0ptr, "aaannotationlist={\"" );

                for ( j = 0; j < i; j++ ) {
                    if ( j != 0 )
                        STRCAT( b0ptr, "\", \"" );

                    STRCAT( b0ptr, annot[ j ] );
                }

                STRCAT( b0ptr, "\"};" );
            }

            doit = 1;
            doit = 1;
        }

        REPLACE( LAST_AAANNOTATION, AAANNOTATION );
    }

    if ( TEST( LAST_AALABELSCALE, AALABELSCALE ) ) {
        if ( AALABELSCALE ) {
            float labelscale;

            if ( ! DXExtractFloat( AALABELSCALE, &labelscale ) ) {
                DXSetError( ERROR_BAD_PARAMETER,
                            "labelscale must be a scalar" );
                goto error;
            }

            sprintf( buffer1, " aalabelscale=%g", labelscale );
            STRCAT( b0ptr, buffer1 );
            STRCAT( b0ptr, ";" );
            doit = 1;
        }

        REPLACE( LAST_AALABELSCALE, AALABELSCALE );
    }

    if ( TEST( LAST_AAFONT, AAFONT ) ) {
        if ( AAFONT ) {
            char * aafont;

            if ( ! DXExtractString( AAFONT, &aafont ) ) {
                DXSetError( ERROR_BAD_PARAMETER,
                            "aafont must be a string" );
                goto error;
            }

            STRCAT( b0ptr, " aafont=" );
            STRCAT( b0ptr, aafont );
            STRCAT( b0ptr, ";" );
            doit = 1;
        }

        REPLACE( LAST_AAFONT, AAFONT );
    }

    if ( TEST( LAST_AAXTICKLOCS, AAXTICKLOCS ) ) {
        if ( AAXTICKLOCS ) {
            Array array = ( Array ) AAXTICKLOCS;
            Type type;
            Category cat;
            int rank, shape;
            int i, n;
            float *buf;
            char buf2[ 128 ];

            if ( DXGetObjectClass( ( Object ) array ) != CLASS_ARRAY ) {
                DXWarning( "ImageMessage: invalid parameter - possible version mismatch between net and DX executive" );
                goto returnOK;
            }

            DXGetArrayInfo( array, &n, &type, &cat, &rank, &shape );

            if ( ( type != TYPE_FLOAT && type != TYPE_INT ) ||
                    cat != CATEGORY_REAL ||
                    ( rank != 0 && ( rank != 1 || shape != 1 ) ) ) {
                DXSetError( ERROR_BAD_PARAMETER,
                            "aaxticklocs must scalar or 1-vector values" );
                goto error;
            }

            buf = ( float * ) DXAllocate( n * sizeof( float ) );

            if ( ! buf )
                goto error;

            if ( ! DXExtractParameter( ( Object ) array, TYPE_FLOAT, 0, n, ( Pointer ) buf ) ) {
                DXSetError( ERROR_BAD_PARAMETER,
                            "could not extract aaxticklocs" );
                DXDelete( ( Pointer ) buf );
                goto error;
            }

            if ( doit ) {
                SendPendingMessage( sendtype, buffer0, "prexticks" );
                doit = 0;
                b0ptr = buffer0;
                buffer0[ 0 ] = '\0';
            }

            sprintf( buf2, " aaxticklocs=%g", buf[ 0 ] );
            STRCAT( b0ptr, buf2 );

            for ( i = 1; i < n; i++ ) {
                sprintf( buf2, ", %g", buf[ i ] );
                STRCAT( b0ptr, buf2 );
            }

            STRCAT( b0ptr, ";" );
            DXFree( buf );
            SendPendingMessage( sendtype, buffer0, "postxticks" );
            doit = 0;
            b0ptr = buffer0;
            buffer0[ 0 ] = '\0';
        }

        REPLACE( LAST_AAXTICKLOCS, AAXTICKLOCS );
    }

    if ( TEST( LAST_AAYTICKLOCS, AAYTICKLOCS ) ) {
        if ( AAYTICKLOCS ) {
            Array array = ( Array ) AAYTICKLOCS;
            Type type;
            Category cat;
            int rank, shape;
            int i, n;
            float *buf;
            char buf2[ 128 ];

            if ( DXGetObjectClass( ( Object ) array ) != CLASS_ARRAY ) {
                DXSetError( ERROR_BAD_PARAMETER,
                            "aayticklocs must be an array" );
                goto error;
            }

            DXGetArrayInfo( array, &n, &type, &cat, &rank, &shape );

            if ( ( type != TYPE_FLOAT && type != TYPE_INT ) ||
                    cat != CATEGORY_REAL ||
                    ( rank != 0 && ( rank != 1 || shape != 1 ) ) ) {
                DXSetError( ERROR_BAD_PARAMETER,
                            "aayticklocs must scalar or 1-vector values" );
                goto error;
            }

            buf = ( float * ) DXAllocate( n * sizeof( float ) );

            if ( ! buf )
                goto error;

            if ( ! DXExtractParameter( ( Object ) array, TYPE_FLOAT, 0, n, ( Pointer ) buf ) ) {
                DXSetError( ERROR_BAD_PARAMETER,
                            "could not extract aayticklocs" );
                DXDelete( ( Pointer ) buf );
                goto error;
            }

            if ( doit ) {
                SendPendingMessage( sendtype, buffer0, "preyticks" );
                doit = 0;
                b0ptr = buffer0;
                buffer0[ 0 ] = '\0';
            }

            sprintf( buf2, " aayticklocs=%g", buf[ 0 ] );
            STRCAT( b0ptr, buf2 );

            for ( i = 1; i < n; i++ ) {
                sprintf( buf2, ", %g", buf[ i ] );
                STRCAT( b0ptr, buf2 );
            }

            STRCAT( b0ptr, ";" );
            DXFree( buf );
            SendPendingMessage( sendtype, buffer0, "postyticks" );
            doit = 0;
            b0ptr = buffer0;
            buffer0[ 0 ] = '\0';
        }

        REPLACE( LAST_AAYTICKLOCS, AAYTICKLOCS );
    }

    if ( TEST( LAST_AAZTICKLOCS, AAZTICKLOCS ) ) {
        if ( AAZTICKLOCS ) {
            Array array = ( Array ) AAZTICKLOCS;
            Type type;
            Category cat;
            int rank, shape;
            int i, n;
            float *buf;
            char buf2[ 128 ];

            if ( DXGetObjectClass( ( Object ) array ) != CLASS_ARRAY ) {
                DXSetError( ERROR_BAD_PARAMETER,
                            "aazticklocs must be an array" );
                goto error;
            }

            DXGetArrayInfo( array, &n, &type, &cat, &rank, &shape );

            if ( ( type != TYPE_FLOAT && type != TYPE_INT ) ||
                    cat != CATEGORY_REAL ||
                    ( rank != 0 && ( rank != 1 || shape != 1 ) ) ) {
                DXSetError( ERROR_BAD_PARAMETER,
                            "aazticklocs must scalar or 1-vector values" );
                goto error;
            }

            buf = ( float * ) DXAllocate( n * sizeof( float ) );

            if ( ! buf )
                goto error;

            if ( ! DXExtractParameter( ( Object ) array, TYPE_FLOAT, 0, n, ( Pointer ) buf ) ) {
                DXSetError( ERROR_BAD_PARAMETER,
                            "could not extract aazticklocs" );
                DXDelete( ( Pointer ) buf );
                goto error;
            }

            if ( doit ) {
                SendPendingMessage( sendtype, buffer0, "prezticks" );
                doit = 0;
                b0ptr = buffer0;
                buffer0[ 0 ] = '\0';
            }

            sprintf( buf2, " aazticklocs=%g", buf[ 0 ] );
            STRCAT( b0ptr, buf2 );

            for ( i = 1; i < n; i++ ) {
                sprintf( buf2, ", %g", buf[ i ] );
                STRCAT( b0ptr, buf2 );
            }

            STRCAT( b0ptr, ";" );
            DXFree( buf );
            SendPendingMessage( sendtype, buffer0, "postzticks" );
            doit = 0;
            b0ptr = buffer0;
            buffer0[ 0 ] = '\0';
        }

        REPLACE( LAST_AAZTICKLOCS, AAZTICKLOCS );
    }

    if ( TEST( LAST_AAXTICKLABELS, AAXTICKLABELS ) ) {
        if ( AALABELS ) {
        	int len;
            char ** labels = NULL;
            int i, j;
	        DXGetArrayInfo ((Array) AAXTICKLABELS, &len, NULL, NULL, NULL, NULL);
	        labels = (char **) DXAllocate((len + 1) * sizeof(char **));

            for ( i = 0; DXExtractNthString( AAXTICKLABELS, i, labels + i ); i++ );

            if ( i == 1 ) {
                STRCAT( b0ptr, "aaxticklabels=" );
                STRCAT( b0ptr, labels[ 0 ] );
                STRCAT( b0ptr, ";" );
            }

            else if ( i > 1 ) {
                STRCAT( b0ptr, "aaxticklabels={\"" );

                for ( j = 0; j < i; j++ ) {
                    if ( j != 0 )
                        STRCAT( b0ptr, "\", \"" );

                    STRCAT( b0ptr, labels[ j ] );
                }

                STRCAT( b0ptr, "\"};" );
            }
            if(labels)
            	DXFree(labels);

            doit = 1;
        }

        REPLACE( LAST_AAXTICKLABELS, AAXTICKLABELS );
    }

    if ( TEST( LAST_AAYTICKLABELS, AAYTICKLABELS ) ) {
        if ( AALABELS ) {
        	int len;
            char ** labels = NULL;
            int i, j;
	        DXGetArrayInfo ((Array) AAYTICKLABELS, &len, NULL, NULL, NULL, NULL);
	        labels = (char **) DXAllocate((len + 1) * sizeof(char **));

            for ( i = 0; DXExtractNthString( AAYTICKLABELS, i, labels + i ); i++ );

            if ( i == 1 ) {
                STRCAT( b0ptr, "aayticklabels=" );
                STRCAT( b0ptr, labels[ 0 ] );
                STRCAT( b0ptr, ";" );
            }

            else if ( i > 1 ) {
                STRCAT( b0ptr, "aayticklabels={\"" );

                for ( j = 0; j < i; j++ ) {
                    if ( j != 0 )
                        STRCAT( b0ptr, "\", \"" );

                    STRCAT( b0ptr, labels[ j ] );
                }

                STRCAT( b0ptr, "\"};" );
            }

			if(labels)
				DXFree(labels);
				
            doit = 1;
        }

        REPLACE( LAST_AAYTICKLABELS, AAYTICKLABELS );
    }

    if ( TEST( LAST_AAZTICKLABELS, AAZTICKLABELS ) ) {
        if ( AALABELS ) {
        	int len;
            char ** labels = NULL;
            int i, j;
	        DXGetArrayInfo ((Array) AAZTICKLABELS, &len, NULL, NULL, NULL, NULL);
	        labels = (char **) DXAllocate((len + 1) * sizeof(char **));

            for ( i = 0; DXExtractNthString( AAZTICKLABELS, i, labels + i ); i++ );

            if ( i == 1 ) {
                STRCAT( b0ptr, "aazticklabels=" );
                STRCAT( b0ptr, labels[ 0 ] );
                STRCAT( b0ptr, ";" );
            }

            else if ( i > 1 ) {
                STRCAT( b0ptr, "aazticklabels={\"" );

                for ( j = 0; j < i; j++ ) {
                    if ( j != 0 )
                        STRCAT( b0ptr, "\", \"" );

                    STRCAT( b0ptr, labels[ j ] );
                }

                STRCAT( b0ptr, "\"};" );
            }

			if(labels)
				DXFree(labels);
				
            doit = 1;
        }

        REPLACE( LAST_AAZTICKLABELS, AAZTICKLABELS );
    }

    if ( TEST( LAST_RECMODE, RECMODE ) ) {
        if ( RECMODE ) {
            int recmode;

            if ( ! DXExtractInteger( RECMODE, &recmode ) ||
                    ( recmode != 0 && recmode != 1 ) ) {
                DXSetError( ERROR_BAD_PARAMETER,
                            "recmode must be a integer either 0 or 1" );
                goto error;
            }

            sprintf( buffer1, " recmode=%d", recmode );
            STRCAT( b0ptr, buffer1 );
            STRCAT( b0ptr, ";" );
            doit = 1;
        }

        REPLACE( LAST_RECMODE, RECMODE );
    }

    if ( TEST( LAST_BUAPPX, BUAPPX ) ) {
        if ( BUAPPX ) {
            char * buappx;

            if ( ! DXExtractString( BUAPPX, &buappx ) ) {
                DXSetError( ERROR_BAD_PARAMETER,
                            "button up approximation must be a string" );
                goto error;
            }

            STRCAT( b0ptr, " buappx=" );
            STRCAT( b0ptr, buappx );
            STRCAT( b0ptr, ";" );
            doit = 1;
        }

        REPLACE( LAST_BUAPPX, BUAPPX );
    }

    if ( TEST( LAST_BDAPPX, BDAPPX ) ) {
        if ( BDAPPX ) {
            char * bdappx;

            if ( ! DXExtractString( BDAPPX, &bdappx ) ) {
                DXSetError( ERROR_BAD_PARAMETER,
                            "button down approximation must be a string" );
                goto error;
            }

            STRCAT( b0ptr, " bdappx=" );
            STRCAT( b0ptr, bdappx );
            STRCAT( b0ptr, ";" );
            doit = 1;
        }

        REPLACE( LAST_BDAPPX, BDAPPX );
    }

    if ( TEST( LAST_BUDEN, BUDEN ) ) {
        if ( BUDEN ) {
            int buden;

            if ( ! DXExtractInteger( BUDEN, &buden ) ) {
                DXSetError( ERROR_BAD_PARAMETER,
                            "button up density must be a integer" );
                goto error;
            }

            sprintf( buffer1, " buden=%d", buden );
            STRCAT( b0ptr, buffer1 );
            STRCAT( b0ptr, ";" );
            doit = 1;
        }

        REPLACE( LAST_BUDEN, BUDEN );
    }

    if ( TEST( LAST_BDDEN, BDDEN ) ) {
        if ( BDDEN ) {
            int bdden;

            if ( ! DXExtractInteger( BDDEN, &bdden ) ) {
                DXSetError( ERROR_BAD_PARAMETER,
                            "button down density must be a integer" );
                goto error;
            }

            sprintf( buffer1, " bdden=%d", bdden );
            STRCAT( b0ptr, buffer1 );
            STRCAT( b0ptr, ";" );
            doit = 1;
        }

        REPLACE( LAST_BDDEN, BDDEN );
    }

    if ( TEST( LAST_INTERACTIONMODE, INTERACTIONMODE ) ) {
        if ( INTERACTIONMODE ) {
            char * mode;

            if ( ! DXExtractString( INTERACTIONMODE, &mode ) ) {
                DXSetError( ERROR_BAD_PARAMETER,
                            "interaction mode must be a string" );
                goto error;
            }

            if ( strncmp( mode, "navigate", 8 ) &&
                    strncmp( mode, "panzoom", 7 ) &&
                    strncmp( mode, "cursors", 7 ) &&
                    strncmp( mode, "camera", 6 ) &&
                    strncmp( mode, "rotate", 6 ) &&
                    strncmp( mode, "pick", 4 ) &&
                    strncmp( mode, "roam", 4 ) &&
                    strncmp( mode, "zoom", 4 ) &&
                    strncmp( mode, "none", 4 ) ) {
                DXSetError( ERROR_BAD_PARAMETER,
                            "interaction mode must one of navigate, panzoom, "
                            "cursors, rotate, pick, roam, zoom, camera or none" );
                goto error;
            }

            STRCAT( b0ptr, " interactionmode=" );
            STRCAT( b0ptr, mode );
            STRCAT( b0ptr, ";" );
            doit = 1;
        }

        REPLACE( LAST_INTERACTIONMODE, INTERACTIONMODE );
    }

    if ( TEST( LAST_TITLE, TITLE ) ) {
        if ( TITLE ) {
            char * title;

            if ( ! DXExtractString( TITLE, &title ) ) {
                DXSetError( ERROR_BAD_PARAMETER,
                            "image title must be a string" );
                goto error;
            }

            STRCAT( b0ptr, " title=" );
            STRCAT( b0ptr, title );
            STRCAT( b0ptr, ";" );
            doit = 1;
        }

        REPLACE( LAST_TITLE, TITLE );
    }

    if ( doit )
        SendPendingMessage( sendtype, buffer0, "last" );

returnOK:
          return OK;

error:
    return ERROR;
}

typedef struct
{
    int delete;
    char *message;
    char *messageType;
    char *major;
    char *minor;
}

PendingDXLMessage;

static Error
delete_PendingDXLMessage( Pointer d )
{
    PendingDXLMessage * p = ( PendingDXLMessage * ) d;
    DXFree( p->message );
    DXFree( p->messageType );
    DXFree( p->major );
    DXFree( p->minor );
    DXFree( p );
    return OK;
}

static Error
SendPendingDXLMessage( Private P )
{
    PendingDXLMessage * p = ( PendingDXLMessage * ) DXGetPrivateData( P );

    DXUIMessage( p->messageType, p->message );

    if ( p->delete )
        if ( ! DXSetPendingCmd( p->major, p->minor, NULL, NULL ) ) {
            DXSetError( ERROR_INTERNAL, "error return from DXSetPendingCmd" );
            return ERROR;
        }

    return OK;
}

static Error
SendPendingMessage( char *type, char *buf, char *minor )
{
    char * major = NULL;
    PendingDXLMessage *plr = NULL;
    Private p = NULL;

    plr = ( PendingDXLMessage * ) DXAllocateZero( sizeof( PendingDXLMessage ) );

    if ( !plr )
        goto error;

    plr->message = ( char * ) DXAllocate( strlen( buf ) + 1 );

    if ( !plr->message )
        goto error;

    plr->messageType = ( char * ) DXAllocate( strlen( type ) + 1 );

    if ( !plr->messageType )
        goto error;

    major = DXGetModuleId();

    if ( !major )
        goto error;

    plr->major = ( char * ) DXAllocate( strlen( major ) + 1 );

    if ( !plr->major )
        goto error;

    plr->minor = ( char * ) DXAllocate( strlen( minor ) + 1 );

    if ( !plr->minor )
        goto error;

    p = DXNewPrivate( ( Pointer ) plr, delete_PendingDXLMessage );

    if ( !p )
        goto error;

    strncpy( plr->message, buf, strlen( buf ) + 1 );

    strcpy( plr->messageType, type );

    strncpy( plr->major, major, strlen( major ) + 1 );

    strncpy( plr->minor, minor, strlen( minor ) + 1 );

    DXFreeModuleId( major );

    major = NULL;

    plr->delete = 1;

    if ( !DXSetPendingCmd( plr->major, plr->minor, SendPendingDXLMessage, p ) )
        goto error;

    return OK;

error:

    if ( p )
        DXDelete( ( Object ) p );
    else if ( plr ) {
        DXFree( plr->major );
        DXFree( plr->minor );
        DXFree( plr->message );
        DXFree( plr->messageType );
        DXFree( plr );
    }

    if ( major )
        DXFreeModuleId( major );

    return ERROR;

}
