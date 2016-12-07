/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dxmods/readimage.c,v 1.9 2003/07/11 05:50:36 davidt Exp $
 */

#include <dxconfig.h>
#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

/***
MODULE:
    ReadImage
INPUTS:
    name;        string;   image.rgb;       file name 
    format;      string;   data dependent;  format of file
    start;       integer;  first frame;     starting movie frame
    end;         integer;  last frame;      ending movie frame 
    delta;       integer;  1;               delta of images to read
    width;       integer;  data dependent;  width of image
    height;      Integer;  data dependent;  height of image
    delayed;     integer;  env. dep;        use delayed if present in image file
    colortype;   string;   env. dep;        "float" or "byte"
***/

#include <stdio.h>
#include <string.h>
#include <dx/dx.h>
#include "_helper_jea.h"
#include "_rw_image.h"
#include <fcntl.h>

#if defined(HAVE_ERRNO_H)
#include <errno.h>
#endif

#if DXD_HAS_LIBIOP
#include <iop/mov.h>
#include <iop/pfs.h>
#endif

#define COLORS_PIXEL 3
#define SPECIFIED(a) 	((a) != VALUE_UNSPECIFIED)

#define CONCAT_PATHS(a, b, c) \
	if (strlen(b)+strlen(c)<MAX_IMAGE_NAMELEN) \
	    sprintf(a, "%s%s", b, c); \
	else \
	    strcpy(a, c); 

static Field InputADASD     (int width, int height, int frame, char* fname, char *colortype);
static Field InputRGB       (int width, int height, int fh, char *colortype);
static Field InputR_G_B     (int width, int height, int fh[3], char *colortype);

static int   get_subpath(char *env_str, int count, char *subpath, int subpathsize);
Field DXMakeImageFormat(int width, int height, char *format);

int
m_ReadImage ( Object *in, Object *out )
{

#define  I_name    in[0]
#define  I_format  in[1]
#define  I_start   in[2]
#define  I_end     in[3]
#define  I_delta   in[4]
#define  I_width   in[5]
#define  I_height  in[6]
#define  I_delayed in[7]
#define  I_colortype in[8]
#define  O_image   out[0]

    char      *filename;
    char      *format;
    char      *colortype_string;
    char      *colortype;

    SizeData  param_sizes;
    SizeData  filed_sizes;
    SizeData  image_sizes;

    Field      image  = NULL;
    Series     series = NULL;
    ImageType  imgtyp = img_typ_illegal;
    ImageInfo  *imginfo;

    int        fh[3];
    int        delta, imgflags;
    int        num_files;

    char      basename[MAX_IMAGE_NAMELEN], *fb_name;
    char      originalname[MAX_IMAGE_NAMELEN];
    char      imagefilename[MAX_IMAGE_NAMELEN];
    char      sizefilename[MAX_IMAGE_NAMELEN];
    char      *envstr_DXDATA;
    int       try_count;
    int       got_file;
    int       found_subpath=0;
    char      user_path[MAX_IMAGE_PATHLEN];
    char      user_basename[MAX_IMAGE_NAMELEN];
    char      extension[40];
    int       fd=0;
    int       frame;
    long int  frame_size_bytes;
    int       i, j;
    int       delayed;
    char      *str;
    FILE      *fh_miff = NULL;
    int       channels_in_one_file;
    
#if DXD_HAS_LIBIOP
    int    Input_from_ADASD = 0;
#else
#define    Input_from_ADASD 0
#endif

    struct
    {
        char name[MAX_IMAGE_NAMELEN];
        int use_numerics;  /* numbered names. */
        int ext_sel;       /* extension: "gif" "giff" or nothing */
        int multiples;     /* many images in one file */
    }
    gifopts;
    
    struct
    {
        char name[MAX_IMAGE_NAMELEN];
        int use_numerics;  /* numbered names. */
        int ext_sel;       /* extension: "tiff" "tif" or nothing */
        int multiples;     /* many images in one file */
    }
    tiffopts;

    struct
    {
        char name[MAX_IMAGE_NAMELEN];
        int use_numerics;  /* numbered names. */
        int ext_sel;       /* extension: "miff" "mif" or nothing */
        int multiples;     /* many images in one file */
    }
    miffopts;
    
    struct
    {
        char name[MAX_IMAGE_NAMELEN];
        int use_numerics;  /* numbered names. */
        int multiples;     /* many images in one file */
    }
    imopts;
    
    /*
     * Error Check all of the inputs 
     */
    
    O_image = NULL;

    fh[0] = fh[1] = fh[2] = -1;

    if (I_delayed)
    {
	if ( !DXExtractInteger( I_delayed, &delayed ) 
			    ||
	    (delayed != DELAYED_NO && delayed != DELAYED_YES))
	    DXErrorGoto2 ( ERROR_BAD_PARAMETER, "#10070", "delayed" );
    }
    else if (NULL != (str = (char *)getenv("DXDELAYEDCOLORS")))
    {
	if (!strcmp(str, "0"))
	    delayed = DELAYED_NO;
	else if (!strcmp(str, "1"))
	    delayed = DELAYED_YES;
	else
	    DXErrorGoto2
		( ERROR_BAD_PARAMETER, "#10070",
			"DXDELAYEDCOLORS environment variable" );
    }
    else
	delayed = DELAYED_YES;

    if (I_colortype)
    {
	if ( !DXExtractString( I_colortype, &colortype_string )) 
	    DXErrorGoto2 ( ERROR_BAD_PARAMETER, "#10070", "colortype" );
	if (!strcmp(colortype_string, "float"))
	    colortype = COLORTYPE_FLOAT;
	else if (!strcmp(colortype_string, "DXFloat"))
	    colortype = COLORTYPE_FLOAT;
	else if (!strcmp(colortype_string, "byte"))
	    colortype = COLORTYPE_BYTE;
	else if (!strcmp(colortype_string, "DXByte"))
	    colortype = COLORTYPE_BYTE;
	else
	    DXErrorGoto2 ( ERROR_BAD_PARAMETER, "#10070", "colortype" );
    }
    else if (NULL != (str = (char *)getenv("DXPIXELTYPE")))
    {
	if (!strcmp(str, "float"))
	    colortype = COLORTYPE_FLOAT;
	else if (!strcmp(str, "DXFloat"))
	    colortype = COLORTYPE_FLOAT;
	else if (!strcmp(str, "byte"))
	    colortype = COLORTYPE_BYTE;
	else if (!strcmp(str, "DXByte"))
	    colortype = COLORTYPE_BYTE;
	else
	    DXErrorGoto2
		( ERROR_BAD_PARAMETER, "#10070",
			"DXPIXELTYPE environment variable" );
    }
    else
	colortype = COLORTYPE_BYTE;

    if (I_name)
    {
        if ( !DXExtractString ( I_name, &filename ) || ( filename == NULL ) )
            DXErrorGoto2 ( ERROR_MISSING_DATA, "#10200", "name" )

        else if ( strchr ( filename, ':' ) ) {
#if DXD_HAS_LIBIOP
            Input_from_ADASD = 1;
        } else { 
            Input_from_ADASD = 0;
#else
#ifndef DXD_OS_NON_UNIX
	    DXErrorGoto(ERROR_DATA_INVALID,  "#12305");
#endif
#endif
	}
	if (strlen(filename) > MAX_IMAGE_NAMELEN) {
            DXSetError( ERROR_DATA_INVALID, "#12210", "name");
	    goto error;
	}
        strcpy(originalname,filename);
    }
    else
        strcpy(originalname,"image");

    strcpy(basename,originalname);

    envstr_DXDATA = (char *)getenv("DXDATA");
    *user_path = '\0';
    
    if ( I_format )
    {
        if ( !DXExtractString ( I_format, &format ) || ( format == NULL ) )
            DXErrorGoto2 ( ERROR_MISSING_DATA, "#10200", "format" )
		
        else if (!(imginfo = _dxf_ImageInfoFromFormat(format)) )
            DXErrorGoto3
                ( ERROR_BAD_PARAMETER, "#10210", format, "the file format" )
		    
	imgflags = imginfo->flags; 
	imgtyp = imginfo->type;
        if (Input_from_ADASD) {
	    if  (!(imgflags & ADASD_OK))
                DXErrorGoto2 ( ERROR_NOT_IMPLEMENTED, "#12215",format);
	} else if (imgflags & ADASD_ONLY) { 
                DXErrorGoto2 ( ERROR_NOT_IMPLEMENTED, "#12220",format);
        }
    }
    else
    {
        format = NULL;

        imginfo = _dxf_ImageInfoFromFileName ( basename );
        if (imginfo)
		imgtyp = imginfo->type; 

        if ( Input_from_ADASD )
        {
            if (!imginfo) { 
                imgtyp = img_typ_fb;
                imginfo = _dxf_ImageInfoFromType(imgtyp);

                if ( imginfo == NULL )
                    DXErrorGoto 
                        ( ERROR_DATA_INVALID, "image type lookup failed" );
	    }
	    if ((imginfo->flags & ADASD_OK) == 0) {
                DXSetError ( ERROR_NOT_IMPLEMENTED, "#12225", "format");
		goto error;
	    }
        }
        else
        {
            if (!imginfo) { 
                imgtyp = img_typ_rgb;
                imginfo = _dxf_ImageInfoFromType(imgtyp);

                if ( imginfo == NULL )
                    DXErrorGoto 
                        ( ERROR_DATA_INVALID, "image type lookup failed" );
	    }

            if ( imginfo->flags & ADASD_ONLY )  {
                DXSetError ( ERROR_NOT_IMPLEMENTED, "#12230", "name");
		goto error;
	    }
        }
    }

    if ( ( imginfo->read ==         NULL ) &&
         ( imginfo->type != img_typ_tiff && imginfo->type != img_typ_gif && 
	   imginfo->type != img_typ_miff && imginfo->type != img_typ_im  ) )
        DXErrorGoto ( ERROR_NOT_IMPLEMENTED, "#12235");
	    
    /*
     * Extract the extension in basename, and remove it if it is recognized
     * and is an acceptable extension for imgtyp.
     */
    _dxf_ExtractImageExtension(basename,imgtyp,extension,sizeof(extension));
    _dxf_RemoveImageExtension(basename,imgtyp);

    if ( I_start )
    {
        if ( !DXExtractInteger ( I_start, &param_sizes.startframe )
             ||
             ( param_sizes.startframe < 0 ) )
            DXErrorGoto2
                ( ERROR_DATA_INVALID, "#10030",  "start" )
    }
    else
        param_sizes.startframe = VALUE_UNSPECIFIED;

    if ( I_end )
    {
        if ( !DXExtractInteger ( I_end, &param_sizes.endframe )
             ||
             ( param_sizes.endframe < 0 ) )
            DXErrorGoto2
                ( ERROR_DATA_INVALID, "#10030", "end" )
    } 
    else
        param_sizes.endframe = VALUE_UNSPECIFIED;

    if ( I_delta )
    {
        if ( !DXExtractInteger ( I_delta, &delta ) || ( delta < 1 ) )
            DXErrorGoto2
                ( ERROR_DATA_INVALID, "#10020",  "delta" )
    }
    else
        delta = 1;

    if ( I_width )
    {
        if ( !DXExtractInteger ( I_width, &param_sizes.width )
             ||
             ( param_sizes.width <= 0 ) )
            DXErrorGoto2
                ( ERROR_DATA_INVALID, "#10020", "width" )
    }
    else
        param_sizes.width = VALUE_UNSPECIFIED;

    if ( I_height )
    {
        if ( !DXExtractInteger ( I_height, &param_sizes.height )
             ||
             ( param_sizes.height <= 0 ) )
            DXErrorGoto2
                ( ERROR_DATA_INVALID, "#10020",  "height" )
    }
    else
        param_sizes.height = VALUE_UNSPECIFIED;

    /*
     * Assemble filename from input strings: filename and format.
     */
    if ( Input_from_ADASD )
    {
        DXASSERT ( imgtyp == img_typ_fb );

        /* Note that the sizefilename = imagefilename here.  
	 * Also, we can pass VALUE_UNSPECIFIED as the 'frame number' to 
	 * _dxf_BuildImageFileName, since the fb format has APPENDABLE_FILES and
	 * thus the frame number is ignored.
   	 */

	try_count = 0;
	do {
	    CONCAT_PATHS(user_basename, user_path, basename);
	    if (!_dxf_BuildImageFileName(sizefilename, MAX_IMAGE_NAMELEN, user_basename, 
				         imgtyp, VALUE_UNSPECIFIED,0))
		goto error;
	    if (!_dxf_ReadImageSizesADASD(sizefilename, &filed_sizes))
		goto error;
	} while ( !(got_file = (SPECIFIED(filed_sizes.width) && SPECIFIED(filed_sizes.height)))
	  	  &&
		  (get_subpath(envstr_DXDATA, try_count++, user_path, sizeof(user_path))) );
	found_subpath = *user_path != '\0';

	/* 
	 * Check to see that the file exists (i.e. height/width were found). 
	 * If not, then try and open the file without the extension.
	 */

	if (!got_file) {
	    int same_name;
	    if (!_dxf_BuildImageFileName(sizefilename, MAX_IMAGE_NAMELEN, basename,
					 imgtyp, VALUE_UNSPECIFIED,0))
		goto error;
	    same_name = strcmp(sizefilename,filename) == 0;
	    try_count = 0;
	    if (!same_name) do {
		CONCAT_PATHS(user_basename, user_path, filename);
		if (!_dxf_ReadImageSizesADASD(user_basename, &filed_sizes))
		    goto error;
	    } while ( !(got_file = (SPECIFIED(filed_sizes.width) && SPECIFIED(filed_sizes.height)))
		      &&
		      !found_subpath
		      &&
		      (get_subpath(envstr_DXDATA, try_count++, user_path, sizeof(user_path))) );
	    found_subpath = *user_path != '\0';
	    if (!got_file) {
		if (same_name) {
	            DXErrorGoto2(ERROR_BAD_PARAMETER, "#12240",sizefilename);
		} else {
	            DXErrorGoto3(ERROR_BAD_PARAMETER, 
				"#12245",sizefilename,filename);
		}
	    }
	}
    }
    else
    {
        /*
         * Assemble size-file-name from input string filename and "size"
         * Open and read-in or default backwards.
         */

        switch ( imgtyp /* imginfo->type */ )
        {
            case img_typ_rgb:
            case img_typ_r_g_b:
            case img_typ_fb:
            default:

	        strcpy(sizefilename,basename);
        	strcat(sizefilename,".size");

		try_count=0;
		do {
		    CONCAT_PATHS(user_basename, user_path, sizefilename);
		    if (!_dxf_ReadSizeFile(user_basename, &filed_sizes))
		        goto error;
		} while ( !(got_file = (SPECIFIED(filed_sizes.width))
					&& (SPECIFIED(filed_sizes.height)))
			  &&
			  !found_subpath
			  &&
			  get_subpath(envstr_DXDATA, try_count++, user_path, sizeof(user_path)) );
		found_subpath = *user_path != '\0';
                break;

            case img_typ_gif:

		try_count=0;
		do {
		    CONCAT_PATHS(user_basename, user_path, basename);
		    if ( !_dxf_BuildGIFReadFileName
			     ( gifopts.name,
			       MAX_IMAGE_NAMELEN,
			       user_basename,
			       originalname,
			       (param_sizes.startframe==VALUE_UNSPECIFIED)?
				   0 : param_sizes.startframe,
			       &gifopts.use_numerics,
			       &gifopts.ext_sel ) )
			goto error;
		    got_file = ((fd = open(gifopts.name, O_RDONLY)) != -1);
		    if (got_file)
			close(fd);
		} while ( !got_file
			  &&
			  !found_subpath
			  &&
			  get_subpath(envstr_DXDATA, try_count++, user_path, sizeof(user_path)) );
		if (!got_file)
		    DXErrorGoto2
		      (ERROR_BAD_PARAMETER,
		      "Could not open file %s, nor any other GIF name variation"
		      " in current path or DXDATA",
		      filename);
		if (!_dxf_ReadImageSizesGIF
				       (gifopts.name,
				       param_sizes.startframe,
				       &filed_sizes,
				       &gifopts.use_numerics,
				       gifopts.ext_sel,
				       &gifopts.multiples))
		    goto error;
		found_subpath = *user_path != '\0';
                break;


            case img_typ_tiff:

		try_count=0;
		do {
		    CONCAT_PATHS(user_basename, user_path, basename);
		    if ( !_dxf_BuildTIFFReadFileName
			     ( tiffopts.name,
			       MAX_IMAGE_NAMELEN,
			       user_basename,
			       originalname,
			       (param_sizes.startframe==VALUE_UNSPECIFIED)?
				   0 : param_sizes.startframe,
			       &tiffopts.use_numerics,
			       &tiffopts.ext_sel ) )
			goto error;
		    got_file = ((fd = open(tiffopts.name, O_RDONLY)) != -1);
		    if (got_file)
			close(fd);
		} while ( !got_file
			  &&
			  !found_subpath
			  &&
			  get_subpath(envstr_DXDATA, try_count++, user_path, sizeof(user_path)) );
		if (!got_file)
		    DXErrorGoto2
		      (ERROR_BAD_PARAMETER,
		      "Could not open file %s, nor any other TIFF name variation"
		      " in current path or DXDATA",
		      filename);
		if (!_dxf_ReadImageSizesTIFF
				       (tiffopts.name,
				       param_sizes.startframe,
				       &filed_sizes,
				       &tiffopts.use_numerics,
				       tiffopts.ext_sel,
				       &tiffopts.multiples))
		    goto error;
		found_subpath = *user_path != '\0';
                break;

            case img_typ_miff:

		try_count=0;
		do {
		    CONCAT_PATHS(user_basename, user_path, basename);
		    if (!_dxf_BuildImageFileName(miffopts.name, MAX_IMAGE_NAMELEN, user_basename, 
						imgtyp, 0, 0))
			goto error;
		    miffopts.ext_sel = 0;
		    got_file = ((fd = open(miffopts.name, O_RDONLY)) != -1);
		    if (got_file)
			close(fd);
		} while ( !got_file
			  &&
			  !found_subpath
			  &&
			  get_subpath(envstr_DXDATA, try_count++, user_path, sizeof(user_path)) );
		if (!got_file)
		    DXErrorGoto2
		      (ERROR_BAD_PARAMETER,
		      "Could not open file %s, nor any other MIFF name variation"
		      " in current path or DXDATA",
		      filename);
		if (!_dxf_ReadImageSizesMIFF
				       (miffopts.name,
				       param_sizes.startframe,
				       &filed_sizes,
				       &miffopts.use_numerics,
				       miffopts.ext_sel,
				       &miffopts.multiples))
		    goto error;
		found_subpath = *user_path != '\0';
                break;

            case img_typ_im:

		try_count=0;
		do {
		    CONCAT_PATHS(user_basename, user_path, basename);
		    if ( !_dxf_BuildIMReadFileName
			     ( imopts.name,
			       MAX_IMAGE_NAMELEN,
			       user_basename,
			       originalname,
			       extension,
			       (param_sizes.startframe==VALUE_UNSPECIFIED)?
				   0 : param_sizes.startframe,
			       &imopts.use_numerics ) )
			goto error;
		    got_file = ((fd = open(imopts.name, O_RDONLY)) != -1);
		    if (got_file)
			close(fd);
		} while ( !got_file
			  &&
			  !found_subpath
			  &&
			  get_subpath(envstr_DXDATA, try_count++, user_path, sizeof(user_path)) );
		if (!got_file)
		    DXErrorGoto2
		      (ERROR_BAD_PARAMETER,
		      "Could not open file %s, nor any other ImageMagick name variation"
		      " in current path or DXDATA",
		      filename);
		if (!_dxf_ReadImageSizesIM
				       (imopts.name,
				       param_sizes.startframe,
				       &filed_sizes,
				       &imopts.use_numerics,
				       &imopts.multiples))
		    goto error;
		found_subpath = *user_path != '\0';
                break;

        }
    }

    /*
     * Determine size-file inputs from size file and those of the command line.
     */
    /* Size file has precedence */
    image_sizes.width
        = (SPECIFIED(filed_sizes.width) ?
              filed_sizes.width : param_sizes.width);

    /* Size file has precedence */
    image_sizes.height	
        = (SPECIFIED(filed_sizes.height) ? 
              filed_sizes.height : param_sizes.height);

    /* User parameter has precedence, with a default of 0 */
    image_sizes.startframe
        = (SPECIFIED(param_sizes.startframe) ?
            param_sizes.startframe : 
            (SPECIFIED(filed_sizes.startframe) ? filed_sizes.startframe : 0));

    /* User parameter has precedence, with a default of 0 */
    image_sizes.endframe
        = (SPECIFIED(param_sizes.endframe) ?
            param_sizes.endframe : 
            (SPECIFIED(filed_sizes.endframe) ? filed_sizes.endframe : 0));


    /*
     * Warn if the user supplied 'width' and 'height' parameters  don't
     * match those found in the size file (if present).
     */
    if ( SPECIFIED(param_sizes.width) && SPECIFIED(filed_sizes.width) &&
         ( param_sizes.width != filed_sizes.width ) ) 
    {
        DXSetError(ERROR_BAD_PARAMETER, "#12250", "width");
	goto error;
    }

    if ( SPECIFIED(param_sizes.height) && SPECIFIED(filed_sizes.height) &&
         ( param_sizes.height != filed_sizes.height ) )
    {
        DXSetError(ERROR_BAD_PARAMETER, "#12250", "height");
	goto error;
    }

    /*
     * Verify the user supplied 'start' and 'end' parameters. 
     */
    if (SPECIFIED(param_sizes.startframe) && SPECIFIED(filed_sizes.endframe) && 
         ( param_sizes.startframe > filed_sizes.endframe) )
    {
        DXSetError(ERROR_BAD_PARAMETER, "#12255", "start", 
				filed_sizes.startframe, filed_sizes.endframe);
        return ERROR;
    }

    if (SPECIFIED(param_sizes.endframe) && SPECIFIED(filed_sizes.endframe) && 
         ( param_sizes.endframe > filed_sizes.endframe ) )
    {
        DXSetError(ERROR_BAD_PARAMETER, "#12255", "end",
		  filed_sizes.startframe, filed_sizes.endframe);
        return ERROR;
    }

    if ( image_sizes.startframe > image_sizes.endframe ) {
        DXSetError ( ERROR_DATA_INVALID, "#12260",
                   image_sizes.startframe, image_sizes.endframe );
        return ERROR;
    }

    if (
#if DXD_HAS_LIBIOP
   /* The sizes are inside the fb file, so it is always ok for the
    * height and width to not be specified. 
    */
    (imgtyp != img_typ_fb) &&
#endif
    (!SPECIFIED(image_sizes.width) || !SPECIFIED(image_sizes.height))) {
	/* 
	 * ARG!!! Because we allow for the .size file to be optional and
	 * we haven't opened the pixel file yet, we need to try and open the
	 * pixel file in order to give the correct error message (i.e. it
	 * could be that 1) the user gave a bad file name or 2) there is no 
	 * .size file and one of width and height was not specified).
	 */

        /* XXX - tiff name considerations */

	try_count=0;
	do {
	    CONCAT_PATHS(user_basename, user_path, basename);
	    if (!_dxf_BuildImageFileName(imagefilename, MAX_IMAGE_NAMELEN, user_basename, 
						imgtyp, image_sizes.startframe,0))
		return ERROR;
	} while ( ((i = open(imagefilename, O_RDONLY)) <0)
		  &&
		  !found_subpath
		  &&
		  get_subpath(envstr_DXDATA, try_count++, user_path, sizeof(user_path)) );
	found_subpath = *user_path != '\0';

        if (i<0)  {
	    if (!_dxf_BuildImageFileName(imagefilename, MAX_IMAGE_NAMELEN, basename, 
						imgtyp, image_sizes.startframe,0))
		return ERROR;
  	    DXSetError( ERROR_DATA_INVALID, "#12240", imagefilename); /* cannot open fname */
	} else { 
	    close(i);
            DXSetError ( ERROR_DATA_INVALID, "#12265"); /* size file is missing */
	}
        return ERROR;
    }

    /*
     * Attempt to open image file and position to start of frame(s).
     */
    frame_size_bytes = image_sizes.width * image_sizes.height;

    switch ( imgtyp )
    {
        case img_typ_rgb:    frame_size_bytes *= 3;  break;
        case img_typ_r_g_b:  frame_size_bytes *= 1;  break;
        case img_typ_fb:     frame_size_bytes *= 4;  break;
        case img_typ_tiff:                           break;
        case img_typ_gif:                            break;
        case img_typ_miff:                           break;
        case img_typ_im:                             break;

        default:
            DXErrorGoto ( ERROR_ASSERTION, "imgtyp" );
    }

    num_files = (imgtyp==img_typ_r_g_b)? 3 : 1;

    /* FIXME:  This is the logic that was here before, but it looks wrong */
    channels_in_one_file = ( imgtyp == img_typ_tiff || imgtyp == img_typ_gif || 
		             imgtyp == img_typ_miff || imgtyp == img_typ_im );

    for ( i=0; i<num_files; i++ )
    {

	do {
	    CONCAT_PATHS(user_basename, user_path, basename);
	    if ( !channels_in_one_file )
		if ( !_dxf_BuildImageFileName(imagefilename, MAX_IMAGE_NAMELEN,
					      user_basename, imgtyp,
					      image_sizes.startframe, i))
		    goto error;

	    if ( Input_from_ADASD )
	    {
		DXASSERTGOTO ( num_files == 1 );
	    }
	    else if ( !channels_in_one_file )
	    {
		fd = open(imagefilename,O_RDONLY); 
	    }
	} while ( !Input_from_ADASD && 
		  !channels_in_one_file &&
		  (fd<0) &&
		  !found_subpath &&
		  get_subpath(envstr_DXDATA, try_count++, user_path, sizeof(user_path)) );

	found_subpath = *user_path != '\0';
	if (!Input_from_ADASD && !channels_in_one_file)
	{
	    if (fd < 0) 
	    {
		if ( !_dxf_BuildImageFileName(imagefilename, MAX_IMAGE_NAMELEN,
					      basename, imgtyp,
					      image_sizes.startframe, i))
		    goto error;
		DXErrorGoto2 ( ERROR_DATA_INVALID, 
			"#12240", imagefilename );
	    }
	    if ( !channels_in_one_file )
		if ( lseek ( fd, (image_sizes.startframe*frame_size_bytes), 0 )
		     != ( image_sizes.startframe * frame_size_bytes ) )
		    DXErrorGoto3
			( ERROR_INTERNAL, "#12270",
			  image_sizes.startframe, imagefilename );
	    fh[i] = fd;
	}
    }
            
    /*
     * Read in frame(s).  For multiple images, construct a series group.
     */

    if ( image_sizes.startframe == image_sizes.endframe ) 
        series = NULL;

    else if ( ERROR == ( series = DXNewSeries() ) )
        goto error;

    fb_name = NULL;

    for ( i=0, frame = image_sizes.startframe;
	       frame <= image_sizes.endframe;
	  i++, frame+=delta ) 
    {
	if ((frame != image_sizes.startframe ) && 
	    (delta != 1) &&
            !channels_in_one_file &&
	    !Input_from_ADASD )
	{
	    for ( j=0; j<num_files; j++ )
	    {
		if ( lseek ( fh[j], ( frame * frame_size_bytes ), 0 ) 
		     !=
		     ( frame * frame_size_bytes ) ) {
		    DXSetError ( ERROR_INTERNAL, "#12270", frame, "");
		    goto error;
		}
	    }
        }

	switch ( imgtyp )
	{
	    case img_typ_rgb:
		image = InputRGB(image_sizes.width, image_sizes.height, fh[0], colortype);
		break;

	    case img_typ_r_g_b:
		image = InputR_G_B(image_sizes.width, image_sizes.height, fh, colortype);
		break;

	    case img_typ_gif:
		if ( gifopts.use_numerics && !gifopts.multiples )
		{
		    if ( gifopts.ext_sel == 2 )
		    {
			if ( !_dxf_RemoveExtension ( gifopts.name ) )
			    goto error;
		    }
		    else
			if ( !_dxf_RemoveExtension ( gifopts.name ) ||
			     !_dxf_RemoveExtension ( gifopts.name )   )
			    goto error;

		    sprintf
			( &gifopts.name [ strlen( gifopts.name ) ],
			  (gifopts.ext_sel==0)? ".%d.gif" :
			  (gifopts.ext_sel==1)? ".%d.gif"  :
						 ".%d",
			  frame );
		}
		image = _dxf_InputGIF(image_sizes.width,
		    image_sizes.height, gifopts.name,
		   (gifopts.multiples)?(frame-filed_sizes.startframe):0,
		   delayed, colortype);
		break;

	    case img_typ_tiff:
		if ( tiffopts.use_numerics && !tiffopts.multiples )
		{
		    if ( tiffopts.ext_sel == 2 )
		    {
			if ( !_dxf_RemoveExtension ( tiffopts.name ) )
			    goto error;
		    }
		    else
			if ( !_dxf_RemoveExtension ( tiffopts.name ) ||
			     !_dxf_RemoveExtension ( tiffopts.name )   )
			    goto error;

		    sprintf
			( &tiffopts.name [ strlen ( tiffopts.name ) ],
			  (tiffopts.ext_sel==0)? ".%d.tiff" :
			  (tiffopts.ext_sel==1)? ".%d.tif"  :
						 ".%d",
			  frame );
		}
		image = _dxf_InputTIFF(image_sizes.width,
		    image_sizes.height, tiffopts.name,
		   (tiffopts.multiples)?(frame-filed_sizes.startframe):0,
		   delayed, colortype);
		break;

	    case img_typ_miff:
		if ( miffopts.use_numerics && !miffopts.multiples )
		{
		    if ( miffopts.ext_sel == 2 )
		    {
			if ( !_dxf_RemoveExtension ( miffopts.name ) )
			    goto error;
		    }
		    else
			if ( !_dxf_RemoveExtension ( miffopts.name ) ||
			     !_dxf_RemoveExtension ( miffopts.name )   )
			    goto error;

		    sprintf
			( &miffopts.name [ strlen ( miffopts.name ) ],
			  (miffopts.ext_sel==0)? ".%d.miff" :
			  (miffopts.ext_sel==1)? ".%d.mif"  :
						 ".%d",
			  frame );
		}
		image = _dxf_InputMIFF(&fh_miff, image_sizes.width,
		    image_sizes.height, miffopts.name,
		   (miffopts.multiples)?(frame-filed_sizes.startframe):0,
		   delayed, colortype);
		break;

	    case img_typ_fb:
		/* For fb format we allow the file to be opened without
		 * the '.fb' extension.  Once we've found it though,
		 * stay with it.
		 */
		if (!fb_name)  {
		    image = InputADASD(image_sizes.width,
				image_sizes.height, frame, imagefilename, colortype);
		    
		    if (image)
			fb_name = imagefilename; 
		    else
		    {
			image = InputADASD(image_sizes.width,
				image_sizes.height, frame, imagefilename, colortype);

			if (image)
			    fb_name = basename; 
			else
			    goto error;
		    }
		}
		else
		{
		    image = InputADASD (image_sizes.width,
				image_sizes.height, frame, fb_name, colortype);
		    if (! image)
			goto error;
		}

		break;

	    case img_typ_im:
		if ( imopts.use_numerics && !imopts.multiples )
		{
		    if ( !_dxf_RemoveExtension ( imopts.name ) ||
			 !_dxf_RemoveExtension ( imopts.name ) )
			goto error;

		    sprintf( &imopts.name [ strlen ( imopts.name ) ],
			     ".%d.%s", frame, extension );
		}
		image = _dxf_InputIM(image_sizes.width,
			   image_sizes.height, imopts.name,
			   (imopts.multiples)?(frame-filed_sizes.startframe):0,
			   delayed, colortype);
		break;

	    default:
		DXErrorGoto ( ERROR_ASSERTION, "image type" );

	}

	if (! image)
	    goto error;

	if (series && !DXSetSeriesMember(series,i,(float)frame,(Object)image))
	    goto error;
    }

    if ( fh[0] >= 0 ) close  ( fh[0] );
    if ( fh[1] >= 0 ) close  ( fh[1] );
    if ( fh[2] >= 0 ) close  ( fh[2] );

    if (fh_miff != 0) fclose(fh_miff);

    O_image = ( ( series ) ? (Object)series : (Object) image );

    return OK;
error:
    /* Clean up. */

    if ( fh[0] >= 0 ) close  ( fh[0] );
    if ( fh[1] >= 0 ) close  ( fh[1] );
    if ( fh[2] >= 0 ) close  ( fh[2] );
    if (fh_miff != 0) fclose(fh_miff);
    if ( series           ) DXDelete ( (Object) series );
    if ( !series && image ) DXDelete ( (Object) image  );
    return ERROR;
}

#define BYTES_FLOAT 4
/*
 * Algorithmic considerations:
 *   Since the pixel content of an unread image is undefined,
 *   and is to be completely overwritten after this call;
 *   We therefore use the 'top fourth' of this area for
 *   our read buffer.
 *   Another useful item is that both the On-disk and in
 *   memory formats store in the order (Red-Green-Blue).
 */
static
Field InputRGB (int width, int height, int fh, char *colortype)
{
    int            oneline;
    int            oneframe;
    unsigned char  *anchor;
    unsigned char  *cptr0;
    unsigned char  *cptr1;
    int            y, x;
    Array 	   colorsArray;
    Type 	   type;
    int		   expandToFloat;
    Field	   image = NULL;
    int		   rank, shape[32];

    if ((width * height) == 0)
    {
        DXSetError ( ERROR_BAD_PARAMETER, "#12275",0);
	return ERROR;
    }

    image = DXMakeImageFormat(width, height, colortype);
    if (! image)
	goto error;
    
    colorsArray = (Array)DXGetComponentValue(image, "colors");
    if (! colorsArray)
	goto error;
    
    DXGetArrayInfo(colorsArray, NULL, &type, NULL, &rank, shape);

    if (type == TYPE_FLOAT)
    {
	expandToFloat = 1;
    }
    else 
    {
	if (rank != 1 || shape[0] != 3)
	{
	    DXSetError(ERROR_INTERNAL,
	       "readimage from .rgb files requires 3-vector colors");
	    goto error;
	}
	
	expandToFloat = 0;
    }

    anchor = (unsigned char *)DXGetArrayData(colorsArray);

    oneline  = width * COLORS_PIXEL;
    oneframe = oneline * height;

    /* Read complete image directly into memory in native disk format.
     *
     * Byte format scanlines in image memory after read-in:
     *  +--------------------------------------+
     *  |                   empty              |
     *  +--------------------------------------+
     *  |                     "                |
     *  +--------------------------------------+
     *  |                     "                |
     *  +---------+----------------------------+
     *  |    y0   |           "                |
     *  +---------+---------+---------+--------+
     *  |  ymax-1 |  ymax-2 |   ...   |   y1   |
     *  +---------+---------+---------+--------+
     *(origin)
     */

    /*
     * Note that the above figure is appropriate if the output
     * image type is float.  If its byte, the result is simply:
     *
     *  +--------------------------------------+
     *  |    y0                                |
     *  +--------------------------------------+
     *  |    y1                                |
     *  +--------------------------------------+
     *  |   ...                                |
     *  +--------------------------------------+
     *  |  ymax-2                              |
     *  +--------------------------------------+
     *  |  ymax-1			       |
     *  +--------------------------------------+
     */

    if (read(fh, anchor, oneframe) != oneframe)
    {
        DXSetError
            ( ERROR_BAD_PARAMETER, "#12280", height, width );
        return ERROR;
    }

    /*
     * Invert ordering of scan-lines
     */
    cptr0 = anchor;
    cptr1 = anchor + (height-1)*oneline;
    for (y = 0; y < (height>>1); y++, cptr0 += oneline, cptr1 -= oneline)
    {
	unsigned char *c0 = (unsigned char *)cptr0;
	unsigned char *c1 = (unsigned char *)cptr1;

	for (x = 0; x < oneline; x++, c0++, c1++)
	{
	    unsigned char tmp = *c1;
	    *c1 = *c0;
	    *c0 = tmp;
	}
    }

    if (expandToFloat)
    {
	float         *dstY = ((float *)anchor) + (height-1)*oneline;
	unsigned char *srcY = ((unsigned char *)anchor) + (height-1)*oneline;
	float	      denom = 1.0 / 255.0;

	for (y = (height-1); y >= 0; y--, dstY -= oneline, srcY -= oneline)
	{
	    float *dstX = dstY + (oneline-1);
	    unsigned char *srcX = srcY + (oneline-1);
	    for (x = 0; x < oneline; x++, dstX--, srcX--)
		*dstX = *srcX * denom;
	}
    }

    return image;

error:
    if (DXGetError() == ERROR_NONE)
	DXSetError(ERROR_INTERNAL, "");

    DXDelete((Object)image);

    return ERROR;
}


static
Field InputR_G_B (int width, int height, int fh[3], char *colortype)
{
    int            oneframe;
    Pointer        *pixels;
    ubyte   	   *buffer = NULL;
    ubyte   	   *c_ptr;
    int            c;
    int            i, j;
    Field	   image = NULL;
    Array	   colorsArray = NULL;
    Type	   type;
    int		   rank, shape[32];

    if ((width * height) == 0) 
        DXErrorReturn(ERROR_BAD_PARAMETER, "Attempt to read 0 sized image");

    image = DXMakeImageFormat(width, height, colortype);
    if (! image)
	goto error;
    
    colorsArray = (Array)DXGetComponentValue(image, "colors");
    if (! colorsArray)
	goto error;
    
    pixels = DXGetArrayData(colorsArray);

    oneframe = width * height;
    if (!( buffer = DXAllocateLocal(oneframe)))
        goto error;

    DXGetArrayInfo(colorsArray, NULL, &type, NULL, &rank, shape);

    if (type == TYPE_UBYTE)
    {
	ubyte *p_ptr;

	if (rank != 1 || shape[0] != 3)
	{
	    DXSetError(ERROR_INTERNAL,
	       "readimage from r+g+b files requires 3-vector colors");
	    goto error;
	}
	
	for (c=0; c<3; c++)
	{
	    if ( read ( fh[c], buffer, oneframe ) != oneframe )
	    {
		DXSetError
		    ( ERROR_BAD_PARAMETER, "#12290",
		      height, width, (c==0)?'r' : (c==1)?'g' : 'b' );
		return ERROR;
	    }

	    for ( i=0; i<height; i++ )
	    {
		c_ptr = buffer + width*( height-i-1);
		p_ptr = ((ubyte *)pixels) + 3*width*i + c;

		for ( j=0; j<width; j++, c_ptr++, p_ptr+=3)
		    *p_ptr = *c_ptr;
	    }
	}
    }
    else if (type == TYPE_FLOAT)
    {
	float *p_ptr;

	for (c=0; c<3; c++)
	{
	    if ( read ( fh[c], buffer, oneframe ) != oneframe )
	    {
		DXSetError
		    ( ERROR_BAD_PARAMETER, "#12290",
		      height, width, (c==0)?'r' : (c==1)?'g' : 'b' );
		return ERROR;
	    }

	    for ( i=0; i<height; i++ )
	    {
		c_ptr = buffer + width*(height-i-1);
		p_ptr = ((float *)pixels) + 3*width*i + c;

		for ( j=0; j<width; j++, c_ptr++, p_ptr+=3 )
		    *p_ptr = *c_ptr / 255.0;
	    }
	}
    }
    else
    {
	DXSetError(ERROR_INTERNAL,
	   "readimage from .rgb files requires 3-vector byte or float colors");
	goto error;
    }
	

    DXFree((Pointer)buffer);

    return image;

error:
    if(DXGetError() == ERROR_NONE)
	DXSetError(ERROR_INTERNAL, "");

    DXFree((Pointer)buffer);
    DXDelete((Object)image);
    return ERROR;
}


static
Field InputADASD (int width, int height, int frame, char* fname, char *colortype)
{

#if !defined(DXD_HAS_LIBIOP)
    DXErrorReturn(ERROR_DATA_INVALID, "#12295");
#else
    typedef struct { unsigned char b, g, r, a } BGRA;
    static float   lut [ 256 ];
    static int     lut_loaded = 0;
    char    *ibuffer = NULL;
    int     mov_errcode; /* 'mov_errno' is a global */
    struct  mov_stat_s ms;
    u_int   *buffer_aligned;
    mov_id  mid = 0;
    Pointer *opixels;
    BGRA    *ipixels;
    BGRA    *ip;
    int     i, j, bufsize;
    Field   image = NULL;
    Array   colorsArray;
    Type    type;


    image = DXMakeImageFormat(width, height, colortype);
    if (! image)
	goto error;
    
    colorsArray = (Array *)DXGetComponentValue(image, "colors");

    opixels = (Pointer)DXGetArrayData(colorsArray);

    DXGetArrayInfo(colorsArray, NULL, &type, NULL, NULL, NULL);


    if (mov_stat(fname, &ms) < 0) {
        DXErrorGoto2 ( ERROR_INTERNAL, 
			"%s, (mov_open)", mov_errmsg ( mov_errno ) );
    }

    if (frame+1 > ms.frames)
        DXErrorGoto2 ( ERROR_DATA_INVALID,  "#12300", frame);

    bufsize = ms.frame_blocks * MOV_BLK_SIZE + 1023;

    ibuffer = (char *)DXAllocate(bufsize);
    if (! ibuffer)
        goto error;

    buffer_aligned = ( (int)ibuffer % 1024 )
                     ? (u_int *)( (int)ibuffer + 1024 - ( (int)ibuffer % 1024))
                     : (u_int *)ibuffer;


    if ((mid = mov_open(fname)) == 0)
        DXErrorGoto2
            ( ERROR_INTERNAL, "%s, (mov_open)", mov_errmsg ( mov_errno ) );

    if ( ( mov_errcode = mov_get
                             ( mid,
                               ( frame + 1 ),
                               ( frame + 1 ),
                               (u_int *)buffer_aligned ) ) < 0 )
        DXErrorGoto2
            ( ERROR_INTERNAL, "%s, (mov_get)", mov_errmsg ( mov_errcode ));
        
    if ( ( mov_errcode = mov_close ( mid ) ) < 0 )
        DXErrorGoto2
            ( ERROR_INTERNAL, "%s, (mov_close)", mov_errmsg ( mov_errcode ));

    mid = 0;

    ipixels = (BGRA *) buffer_aligned;

    if (type == TYPE_FLOAT)
    {
	/* 
	 * Pre-Calculate the divisions, so they aren't recalculated on a 
	 * per-pixel basis.
	 */
	if ( !lut_loaded )
	{
	    for (i=0; i<256; i++)
		lut [i] = ((float)i / 255.0);

	    lut_loaded = 1;
	}

	for ( i=0; i<height; i++ )
	{
	    RGBColor *op = ((RGBColor *)opixels) + i*width;
	    ip = &ipixels [ (height-i-1) * width ];

	    for ( j=0; j<width; j++, ip++, op++ )
	    {
		op->r = lut[ip->r];
		op->g = lut[ip->g];
		op->b = lut[ip->b];
	    }
	}
    }
    else if (type == TYPE_UBYTE)
    {
	for ( i=0; i<height; i++ )
	{
	    ubyte *op = ((ubyte *)opixels) + i*width;
	    ip = ipixels + (height-i-1)*width;

	    memcpy(op, ip, 3*width);
	}
    }
    else
    {
	DXSetError(ERROR_INTERNAL,
	   "readimage from adasd files requires 3-vector byte or float colors");
	goto error;
    }
			      
    if (ibuffer) DXFree ( (Pointer) ibuffer );

    return image;

error:
    if (ibuffer)
        DXFree((Pointer)ibuffer);

    if (DXGetError() == ERROR_NONE)
	DXSetError(ERROR_INTERNAL, "");

    DXDelete((Object)image);

    return ERROR;

#endif
}

static int get_subpath(char *env_str, int count, char *subpath, int subpathsize)
{
    int i;
    char *s;
#ifndef DXD_OS_NON_UNIX
#   define PATH_SEP_CHAR ':'
#else
#   define PATH_SEP_CHAR ';'
#endif

    *subpath = '\0';
    for (s=env_str,i=0;i<count && s && *s;i++)
    {
	s = strchr(s, PATH_SEP_CHAR);
	if (s && *s)
	    s++;
    }
    if (s && *s)
	strncpy(subpath, s, subpathsize);
    s = strchr(subpath, PATH_SEP_CHAR);
    if (s && *s)
	*s = '\0';
    if (*subpath && (subpath[strlen(subpath)-1] != '/'))
	strcat(subpath,"/");
    
    return (int) *subpath;
}
