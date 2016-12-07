/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dxmods/_rgb_image.c,v 1.6 2003/07/11 05:50:34 davidt Exp $
 */

#include <dxconfig.h>


#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <dx/dx.h>
#include "_helper_jea.h"
#include "_rw_image.h"
#include <sys/types.h>

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#if ( CAN_HAVE_ARRAY_DASD == 1 )
#include <iop/mov.h>
#include <iop/pfs.h>
#endif

#if defined(HAVE_SYS_STAT_H)
#include <sys/stat.h>
#endif

extern Field DXOutputYUV(Field, int); /* from libdx/image.c */

/* should be in libsvs*/
static Field      OutputADASD         ( Field image, int frame, char* fname );

static Error write_rgb_or_fb(RWImageArgs *iargs);

/*
 * DXWrite out the given image in "fb" format.
 */
Error
_dxf_write_fb(RWImageArgs *iargs)
{
    if (iargs->imgtyp != img_typ_fb)
	DXErrorGoto(ERROR_INTERNAL, "_dxf_write_fb: mismatched image types");

    return write_rgb_or_fb(iargs);
error:
    return ERROR;
}
/*
 * DXWrite out the given image in "r+g+b" format.
 */
Error
_dxf_write_r_g_b (RWImageArgs *iargs)
{
    if (iargs->imgtyp != img_typ_r_g_b)
	DXErrorGoto(ERROR_INTERNAL, "_dxf_write_r_g_b: mismatched image types");

    return write_rgb_or_fb(iargs);
error:
    return ERROR;
}

/*
 * DXWrite out the given image in "rgb" format.
 */
Error
_dxf_write_rgb (RWImageArgs *iargs)
{
    if (iargs->imgtyp != img_typ_rgb)
	DXErrorGoto(ERROR_INTERNAL, "_dxf_write_rgb: mismatched image types");

    return write_rgb_or_fb(iargs);
error:
    return ERROR;
}

/*
 * DXWrite out the given image in "yuv" format.
 */
Error
_dxf_write_yuv (RWImageArgs *iargs)
{
    if (iargs->imgtyp != img_typ_yuv)
	DXErrorGoto(ERROR_INTERNAL, "_dxf_write_yuv: mismatched image types");

    return write_rgb_or_fb(iargs);
error:
    return ERROR;
}


/*
 * DXWrite out an "rgb", "r+g+b" or "fb" formatted image file from the
 * specified field.  The input field should not be composite, but may
 * be series.
 * By the time we get called, it has been asserted that we can write
 * the image to the device, be it ADASD or otherwise.
 */

Field DXOutputRGBSeparate(Field, int *);

static Error
write_rgb_or_fb(RWImageArgs *iargs)
{
    char   imagefilename[MAX_IMAGE_NAMELEN];
    char   sizefilename[MAX_IMAGE_NAMELEN];
    int    i, fh[3], series, deleteable = 0;
    int    frame, num_files, frame_size_bytes;
    SizeData  filed_sizes;  /* Values from, to the size file */
    SizeData  image_sizes;  /* Values as found in the image object itself */
    Field	img=NULL;
 
    fh[0] = fh[1] = fh[2] = -1;

    if ( !_dxf_GetImageAttributes ( (Object)iargs->image, &image_sizes ) )
        goto error;

    if ( iargs->adasd )
    {
        /* Note that the sizefilename = imagefilename here */

        if ( !_dxf_BuildImageFileName(sizefilename, MAX_IMAGE_NAMELEN, 
		iargs->basename, iargs->imgtyp, iargs->startframe,0))
            goto error;

        if ( !_dxf_ReadImageSizesADASD ( sizefilename, &filed_sizes ) )
            goto error;
    }
    else
    {
        /*
         * Assemble size-file-name from input string filename and "size"
         * Open and read-in or default backwards.
         */
	if (strlen(iargs->basename) > MAX_IMAGE_NAMELEN - 5)
            DXErrorGoto ( ERROR_DATA_INVALID, "image file name too long" );
		
	strcpy(sizefilename,iargs->basename);
	strcat(sizefilename,".size");

        if ( !_dxf_ReadSizeFile ( sizefilename, &filed_sizes ) )
            goto error;
    }

    /*
     * Validate size-file inputs against those of the command line.
     */

    if ( ( filed_sizes.height != VALUE_UNSPECIFIED )
         &&
         ( ( filed_sizes.height != image_sizes.height )
           ||
           ( filed_sizes.width  != image_sizes.width  ) ) )
    {
        DXSetError
            ( ERROR_BAD_PARAMETER,
              "Image size (%dx%d) must match size file ('%s' is %dx%d).",
              image_sizes.height, image_sizes.width,
              sizefilename, 
              filed_sizes.height, filed_sizes.width );
        goto error;
    }

    filed_sizes.height = image_sizes.height;
    filed_sizes.width  = image_sizes.width;

    if ( iargs->startframe != VALUE_UNSPECIFIED )
    {
        image_sizes.startframe += iargs->startframe;
        image_sizes.endframe   += iargs->startframe;
    }
    else if ( filed_sizes.endframe != VALUE_UNSPECIFIED )
    {
        image_sizes.startframe += ( filed_sizes.endframe + 1 );
        image_sizes.endframe   += ( filed_sizes.endframe + 1 );
    }

    if ( filed_sizes.endframe == VALUE_UNSPECIFIED )
    {
        if ( image_sizes.startframe > 0 )
            DXWarning
                ( "Initial write of frames (%d..%d) (>0).",
                  image_sizes.startframe,
                  image_sizes.endframe   );
    }
    else if ( image_sizes.startframe > ( filed_sizes.endframe + 1 ) )
        DXWarning
            (
          /* TCAT can't handle broken strings. (E.G. "A" "B" implies "AB") */
          "already ('%s') have stored frames (%d..%d).  Adding frames (%d..%d)",
              sizefilename, 
              filed_sizes.startframe,
              filed_sizes.endframe,
              image_sizes.startframe,
              image_sizes.endframe   );

    filed_sizes.startframe = 0;
    if ( image_sizes.endframe > filed_sizes.endframe )
        filed_sizes.endframe = image_sizes.endframe;

    /*
     * Attempt to open image file and position to start of frame(s).
     */
    frame_size_bytes = image_sizes.width * image_sizes.height;

    switch ( iargs->imgtyp )
    {
        case img_typ_rgb:
            frame_size_bytes *= 3;
            break;

        case img_typ_r_g_b:
            frame_size_bytes *= 1;
            break;

        case img_typ_fb:
            frame_size_bytes *= 4;
            break;

        case img_typ_yuv:
            frame_size_bytes *= 2;
            break;

        default:
            DXErrorGoto ( ERROR_ASSERTION, "imgtyp" );
    }

    if (iargs->imgtyp == img_typ_r_g_b)
	num_files = 3;
    else 
	num_files = 1;

    for ( i=0; i<num_files; i++ )
    {
        if ( !_dxf_BuildImageFileName(imagefilename, MAX_IMAGE_NAMELEN, 
		iargs->basename, iargs->imgtyp, iargs->startframe,i))
            goto error;

        if ( iargs->adasd )
        {
            DXASSERTGOTO ( num_files == 1 );
        }
        else
        {
#if !defined(os2) && !defined(intelnt) && !defined(WIN32)
            if ( ( fh[i] = open
                              ( imagefilename,
                                (O_CREAT|O_WRONLY),
                                0666 ) ) < 0 )
#else
/* os2 has a 2 parameter #define for open() _open(). This is the only file
 * where 3 parameters are required.
 */
            if ( ( fh[i] = _open
                              ( imagefilename,
                                (O_CREAT | O_RDWR | O_BINARY),
                                (_S_IWRITE | _S_IREAD)) ) < 0 )
#endif
                ErrorGotoPlus1
                    ( ERROR_DATA_INVALID,
                      "Can't open image file (%s)", imagefilename );

            if ( lseek ( fh[i], (image_sizes.startframe*frame_size_bytes), 0 )
                 !=
                 ( image_sizes.startframe * frame_size_bytes ) )
                ErrorGotoPlus2
                    ( ERROR_UNEXPECTED,
                      "Cannot position to frame (%d) in file (%s)",
                      image_sizes.startframe, imagefilename );
        }
    }
            
    /*
     * DXWrite out frame, or frames, if there is a series group imput.
     */
    series = iargs->imgclass == CLASS_SERIES; 
    img = iargs->image;
    for ( i=0, frame=image_sizes.startframe;
               frame <= image_sizes.endframe;
          i++, frame++ )
    {
        if (series) {
            if (deleteable) DXDelete((Object)img);
            img = _dxf_GetFlatSeriesImage((Series)iargs->image,i,
			image_sizes.width,image_sizes.height,&deleteable);
            if (!img)
                goto error;
        }

        switch ( iargs->imgtyp )
        {
            case img_typ_rgb:
                if ( !DXOutputRGB ( img, fh[0] ) )
                    goto error;
                break;

            case img_typ_r_g_b:
                if ( !DXOutputRGBSeparate ( img, fh ) )
                    goto error;
                break;

            case img_typ_fb:
                if ( !OutputADASD ( img, frame, imagefilename ) )
                    goto error;
                break;

            case img_typ_yuv:
                if ( !DXOutputYUV ( img, fh[0] ) )
                    goto error;
                break;

            default:
                DXErrorGoto ( ERROR_ASSERTION, "image type" );

        }
    }

    if ( !iargs->adasd )
        if ( !_dxf_WriteSizeFile ( sizefilename, filed_sizes ) )
            goto error;

    if ( fh[0] >= 0 ) close  ( fh[0] );
    if ( fh[1] >= 0 ) close  ( fh[1] );
    if ( fh[2] >= 0 ) close  ( fh[2] );
    if (deleteable && img) DXDelete((Object)img);


    return OK;

error:
    if ( fh[0] >= 0 ) close  ( fh[0] );
    if ( fh[1] >= 0 ) close  ( fh[1] );
    if ( fh[2] >= 0 ) close  ( fh[2] );
    if (deleteable && img) DXDelete((Object)img);

    return ERROR;
}


/* everything below this line should be in libsvs somewhere */



SizeData *
_dxf_ReadImageSizesADASD ( char *name, SizeData *sd )
{

#if ( CAN_HAVE_ARRAY_DASD == 0 )

    DXErrorReturn
        ( ERROR_ASSERTION,
          "_dxf_ReadImageSizesADASD() call on illegal architecture" );

#else

    int         mov_errcode; /* 'mov_errno' is a global */
    mov_stat_t  ms;

    DXASSERT ( name );
    DXASSERT ( sd   );

    if ( ( mov_errcode = mov_stat ( name, &ms ) ) < 0 )
    {
        /* error if not 'file does not exist' */

        if ( ( mov_errcode != MOV_ERROR_PFS )
             ||
             ( mov_pfserrno != PFS_ERROR_NOENT ) )
            ErrorGotoPlus1
                ( ERROR_INTERNAL,
                  "ADASD error: %s (mov_stat)",
                  mov_errmsg ( mov_errcode ) )

        /*
         * A nonexistant file is considered logically equivalent
         * to one containing all unspecified values
         */
        sd->height     = VALUE_UNSPECIFIED;
        sd->width      = VALUE_UNSPECIFIED;
        sd->startframe = VALUE_UNSPECIFIED;
        sd->endframe   = VALUE_UNSPECIFIED;
    }
    else
    {
        sd->height     = ms.height;
        sd->width      = ms.width;
        sd->startframe = 0;
        sd->endframe   = ms.frames - 1;
    }

    return sd;

    error:
        if ( DXGetError() == ERROR_NONE )
            DXSetError ( ERROR_INTERNAL, "_dxf_ReadImageSizesADASD()" ); 

        return ERROR;

#endif
}


/* Output an image to the Array-Direct-Access-Storage-Device */
static Field 
OutputADASD ( Field image, int frame, char* fname )
{

#if ( CAN_HAVE_ARRAY_DASD == 0 )

    DXErrorReturn
        ( ERROR_ASSERTION,
          "OutputADASD() call on illegal architecture" );

#else

    typedef struct { unsigned char b, g, r, a } BGRA;

    char    *buffer = NULL;
    int     mov_errcode; /* 'mov_errno' is a global */
    u_int   *buffer_aligned;
    mov_id  mid = 0;
    RGBColor *ipixels;
    RGBColor *ip;
    BGRA    *opixels;
    BGRA    *op;
    int     width;
    int     height;
    int     i, j;
int P;

    if ( !DXGetImageSize ( image, &width, &height )
         ||
         !( ipixels = DXGetPixels ( image ) )
         ||
         !( buffer = (char *)DXAllocate ( ( width * height * sizeof ( BGRA ) )
                                        + 1023 ) ) )
        goto error;

    buffer_aligned = ( (int)buffer % 1024 )
                         ? (u_int *)( (int)buffer + 1024 - ( (int)buffer % 1024 ) )
                         : (u_int *)buffer;

    if ( ( mov_errcode = mov_create
                             ( fname,
                               MOV_FORMAT_ARGB,
                               0,
                               (u_int)width,
                               (u_int)height,
                               1 ) ) < 0 )
        /*
         * This is the check for error not 'file exists'.
         */
        if ( ( mov_errcode != MOV_ERROR_PFS )
             ||
             ( mov_pfserrno != PFS_ERROR_EXIST ) )
            ErrorGotoPlus1
                ( ERROR_INTERNAL,
                  "ADASD error: %s, (mov_create)",
                  mov_errmsg ( mov_errcode))

    /* File now exists */
    if ( !( mid = mov_open ( fname ) ) )
        ErrorGotoPlus1
            ( ERROR_INTERNAL,
              "ADASD error: %s, (mov_open)",
              mov_errmsg ( mov_errno ) )

#define CLAMP1(p) ( ( (P=(p)) < 0 ) ? 0 : (P<=255) ? P : 255 )

    opixels = (BGRA *) buffer_aligned;

    for ( i=0; i<height; i++ )
    {
        ip = &ipixels [ i * width ];
        op = &opixels [ (height-i-1) * width ];

        for ( j=0; j<width; j++, ip++, op++ )
        {
            op->a = (unsigned char) 0xff;
                                    /* XXX ? must be -1 for the hardware card */
            op->r = (unsigned char) CLAMP1 ( 255.0 * ip->r );
            op->g = (unsigned char) CLAMP1 ( 255.0 * ip->g );
            op->b = (unsigned char) CLAMP1 ( 255.0 * ip->b );
        }
    }
                          
    if ( ( mov_errcode = mov_put
                             ( mid,
                               ( frame + 1 ),
                               ( frame + 1 ),
                               (u_int *)buffer_aligned ) ) < 0 )
        ErrorGotoPlus1
            ( ERROR_INTERNAL,
              "ADASD error: %s, (mov_put)",
              mov_errmsg ( mov_errcode ) )
        
    if ( ( mov_errcode = mov_close ( mid ) ) < 0 )
        ErrorGotoPlus1
            ( ERROR_INTERNAL,
              "ADASD error: %s, (mov_close)",
              mov_errmsg ( mov_errcode ) )

    mid = 0;
        
    DXFree ( (Pointer) buffer );

    return image;

    error:
        if ( DXGetError() == ERROR_NONE )
            DXSetError ( ERROR_INTERNAL, "OutputADASD()" );

        DXFree ( (Pointer) buffer );

        return ERROR;

#endif
}

SizeData *
_dxf_ReadSizeFile ( char *name, SizeData *sd )
{
    char record[128];
    int  rl;
    int  fh = -1;

    DXASSERT ( sd );

    if ( ( fh = open ( name, O_RDONLY ) ) < 0 ) 
    {
        /*
         * A nonexistant size file is considered logically equivalent
         * to one containing all unspecified values
         */
        sd->height     = VALUE_UNSPECIFIED;
        sd->width      = VALUE_UNSPECIFIED;
        sd->startframe = VALUE_UNSPECIFIED;
        sd->endframe   = VALUE_UNSPECIFIED;
    }
    else
    {
        if ( ( rl = read ( fh, record, sizeof ( record ) ) ) <= 0 )
        {
            DXSetError ( ERROR_DATA_INVALID,
                         "Can't read from size file (%s).", name );
            goto error;
        }
        else if ( rl >= sizeof ( record ) )  /* == is more a worry than > */
        {
            DXSetError ( ERROR_DATA_INVALID,
                         "Unexpectedly long size file (%s).", name );
            goto error;
        }

        record [ rl ] = '\0';

        if ( sscanf ( record, "%dx%dx%d",
                      &sd->width, &sd->height, &sd->endframe ) == 3 )
        {
            sd->startframe = 0;
            sd->endframe   = sd->endframe - 1;
        }
        else if ( sscanf ( record, "%dx%d", &sd->width, &sd->height ) == 2 )
        {
            sd->startframe = 0;
            sd->endframe   = 0;
        }
        else 
        {
            DXSetError ( ERROR_DATA_INVALID,
                       "Bad format in size file (%s)", name );
            goto error;
        }
    }

    if ( fh >= 0 ) close ( fh );

    return sd;

    error:
        if ( fh >= 0 ) close ( fh );
        return ERROR;
}


/*-------------------*/

Error
_dxf_WriteSizeFile ( char *name, SizeData sd )
{
    char record[128];
    int  fh = -1;

    /*
     * Insist on a frame count specification.
     */
    DXASSERTGOTO ( sd.startframe != VALUE_UNSPECIFIED );
    DXASSERTGOTO ( sd.endframe   != VALUE_UNSPECIFIED );

#if !defined(os2) && !defined(intelnt) && !defined(WIN32)
    if ( ( fh = open ( name, ( O_CREAT | O_RDWR ), 0666 ) ) < 0 )
#else
/* os2 has a 2 parameter #define for open() _open(). This is the only file
 * where 3 parameters are required.
 */
    if ( ( fh = _open ( name, ( O_CREAT | O_RDWR | O_BINARY), (_S_IREAD | _S_IWRITE) ) ) < 0 )
#endif
    {
	DXSetError ( ERROR_DATA_INVALID, "can't open file %s", name );
	goto error;
    }

    if ( sprintf ( record, "%dx%dx%d\n",
                   sd.width, sd.height, ( sd.endframe + 1 ) ) <= 0 )
        DXErrorGoto ( ERROR_INTERNAL, "sprintf()" );

    if ( strlen ( record ) != write ( fh, (char*)record, strlen ( record ) ) )
    {
	DXSetError ( ERROR_UNEXPECTED, "can't write to size file %s", name );
	goto error;
    }

    if ( fh >= 0 ) close ( fh );

    return OK;

    error:
        if ( fh >= 0 ) close ( fh );
        return ERROR;
}
