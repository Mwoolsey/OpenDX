/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
* _IM_image.c : calls ImageMagick API to write files, especially useful for those files
*  whose formats DX does not support natively. 
* FIXED!:  does not write intermediate miff file if the miff does not exist (i.e. is not to be appended).
*FIXME: dx does not know all extensions that IM supports, can dx query IM API for the
*	supported extensions and thereby alleviate the tedious supplying of both format and extension?
*/

/*
* This was coded to preserve dx's prior behavior for other extensions/formats, entailing some complexity.
* How did we get here? 
*
* Currently from WriteImage, there are two ways to get here: 
* (1) special cases e.g. gif where an entry in ImageTable(in _rw_image.c) 
*     specifies the format and acceptable extensions and write_im is specified as the write function.
*     The special case is triggered by giving the format and/or a listed extension.
* (2) "ImageMagick supported format" format was specified and or the extension was first matched in 
*	the table entry for "ImageMagick supported format" format.
*     The extension must be included, recognized or not, in this case so ImageMagick knows what to write.
*     This scheme permits the user to specify, via an extention, an output format the dx coders did not know about
*	but that ImageMagick has added (perhaps later, perhaps as a delegate).
*     This scheme also allows the coexistence of DX methods and IM methods for writing image files.  For example,
*	a format of null and an .rgb (or missing) extension gives the DX rgb output, while a format of 
*	"ImageMagick supported format" and an .rgb extension gives the more conventional raw red/green/blue bytes in a file.
*	Another example is specifying "filename.miff" and "ImageMagick supported format", where dx writes its miff
*	and IM converts and overwrites it to its liking.
*
* DX prefers the "format" to the extension, IM goes by the extension.
* WriteImage parameters can be (optional) format and (optional) filename+extension or filename .
* Whether dot-anything is accepted as an extension depends on the listing in ImageTable (_rw_image.c).  
* dx will assume dx rgb format if the
*format is null and the filename's extension is not recognized (i.e. we won't get to this function).
*if dx recognized the extension for the format, the extension was
*removed from the basename and now the ImageArgs.extension points to the extension.
*however, dx will not (currently) have hardcoded all extensions that IM supports.
*So it is easily possible that by the time we get here, we have a format of "ImageMagick supported format" or null
* and a filename
* with or without an extension appended, with or without a null imageargs.extension.
*/
/*
* DX and IM name spaces collide on these terms (for IM 4.2.8 at least)
*/
#define ImageInfo DXImageInfo
#define ImageType DXImageType

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

#ifdef HAVE_LIBMAGICK
/* we have some namespace conflicts with ImageMagick */
#undef ImageInfo
#undef ImageType
#include <magick/api.h>
#endif /* def HAVE_LIBMAGICK */

#if (0)
#define DEBUGMESSAGE(mess) DXMessage((mess))
#else
#define DEBUGMESSAGE(mess)
#endif

#define STREQ(s1,s2) (strcmp((s1),(s2))==0)

static Error write_im(RWImageArgs *iargs);

/*
 * DXWrite out the given image in some format supported by ImageMagick
 * currently through an intermediary file in miff (Magick Image File
 * Format) though ideally this would use ImageMagick-4.2.8+'s blob support
 * for in-memory translation.  
 */
Error
_dxf_write_im(RWImageArgs *iargs) {
    /*
    *    how does one error check ?  let IM flag the error for an unsupported image format
    *    if (iargs->imgtyp != img_typ_fb)
    *	DXErrorGoto(ERROR_INTERNAL, "_dxf_write_fb: mismatched image types");
    */

    return write_im(iargs);
}

static Error write_im(RWImageArgs *iargs) {
#ifdef HAVE_LIBMAGICK
    RWImageArgs tmpargs = *iargs;
    Image *image=NULL;
    Image *resize_image=NULL;
    int err = 0;
    int pixelsize;
    int linesize;
    Field field;
    Array array;
    int i;
    int dim[3], ndim;
    int nx, ny;
    Type dxcolortype;
    int imcolortype;
    char *p1, *p2;
    void* colors;
    void* copycolors=NULL;
    char *compressFlag = NULL;
    CompressionType ctype = UndefinedCompression;


    ExceptionInfo
    _dxd_exception_info;
    ImageInfo* image_info;
    ImageInfo* new_frame_info;


    int miff_exists_flag = 0;
    char* miff_filename = NULL;
    FILE* miff_fp;

    DEBUGMESSAGE("requested extension is ");
    /* if basename's .ext recognized as a format, it is removed from basename and 
     * iargs->extension points to it */
    if(!iargs->extension) {
        /* no extension or not recognized */
        if(iargs->format) {
            if(strcmp(iargs->format,"ImageMagick supported format") && strcmp(iargs->format, "ImageMagick")) {
                char* firstspace;
                /* not "ImageMagick supported format" format, use format for extension */
                iargs->extension=iargs->format;
                /* strip junk we can't deal withafter format... like "gif delayed=1" */
                firstspace=strchr(iargs->extension, ' ');
                if(firstspace != NULL)
                    *firstspace='\0';

            }
        }
    }
    /* still no recognized extension or "unique" format?  it had better be on the filename! */
    if(!iargs->extension) {
        int i;
        for(i=strlen(iargs->basename);i>0;--i) {
            if(iargs->basename[i]=='.') {
                iargs->extension=&((iargs->basename)[i+1]);
                iargs->basename[i]='\0';
                i=0;
            } else if (iargs->basename[i]=='/')
                i=0;
        }
    }
    /*
    * before proceeding,
    * find out if we can handle the conversion to the requested extension
    */

    /*
    * 8/99 some ambiguity here, since miff files are appendable.
    * My arbitrary choice: if the miff file exists already, we append and do not erase.
    * if the miff file does not exist, we erase when we are done.
    * So if you want a multi-image file in some non-natively-supported-dx-format,
    * writeimage to a miff, then on the last frame writeimage to e.g. an
    * mpeg of the same name, specifying "im" format.  Then it is up to you to erase it.
    * Maybe some other scenario will prevail.
    */

    /* Only appending to miff files with extension .miff or type MIFF ignore this for others */
    
    if(strcmp(iargs->extension, "miff") == 0) {
	tmpargs.format="miff";
	tmpargs.imgtyp=img_typ_miff;

	/* does file exist? */
	miff_filename = (char *)DXAllocateLocal(strlen(iargs->basename)+strlen(tmpargs.format)+2);
	strcpy(miff_filename,iargs->basename);
	strcat(miff_filename,".");
	strcat(miff_filename,tmpargs.format);
	if((miff_fp=fopen(miff_filename,"r"))) {
	    miff_exists_flag=1;
            fclose(miff_fp);
	} else {
	    miff_exists_flag=0;
	}
    	if(miff_filename) DXFree((Pointer)miff_filename);	
    }

    
    GetExceptionInfo(&_dxd_exception_info);
    image_info=CloneImageInfo((ImageInfo *) NULL);


    if(miff_exists_flag) {
        FILE *existFile, *newFrameFile;
        char *buf;
        int noir;

	new_frame_info=CloneImageInfo((ImageInfo *) NULL);

        DEBUGMESSAGE("starting miff appending");
        DEBUGMESSAGE(iargs->basename);
        /* _dxf_write_miff(&tmpargs); */

        /*
          Initialize the image info structure and read an image.
        */

        (void) strcpy(image_info->filename,iargs->basename);
        (void) strcat(image_info->filename,".");
        (void) strcat(image_info->filename,tmpargs.format);

        DEBUGMESSAGE("reading following file");
        DEBUGMESSAGE(image_info->filename);

        /* Now create new frame that will be added onto image. */
        
        field=iargs->image;
        array=(Array)DXGetComponentValue(field,"colors");
        DXGetArrayInfo(array,NULL,&dxcolortype,NULL,NULL,NULL);
        if (dxcolortype == TYPE_FLOAT) {
            imcolortype = FloatPixel;
            pixelsize = 3*sizeof(float);
        } else {
            imcolortype = CharPixel;
            pixelsize = 3;
        }
        
        DXGetStringAttribute((Object)field, "compression", &compressFlag);
	if(compressFlag == NULL)
	    ctype = UndefinedCompression;
	else if(strcmp(compressFlag, "BZip")==0)
            ctype = BZipCompression;
        else if(strcmp(compressFlag, "Fax")==0)
            ctype = FaxCompression;
        else if(strcmp(compressFlag, "Group4")==0)
            ctype = Group4Compression;
        else if(strcmp(compressFlag, "JPEG")==0)
            ctype = JPEGCompression;
        else if(strcmp(compressFlag, "LZW")==0)
            ctype = LZWCompression;
        else if(strcmp(compressFlag, "RLE")==0)
            ctype = RunlengthEncodedCompression;
        else if(strcmp(compressFlag, "Zip")==0)
            ctype = ZipCompression;
        
        new_frame_info->compression = ctype;

        colors=(void*)DXGetArrayData(array);
        array=(Array)DXGetComponentValue(field,"connections");
        DXQueryGridConnections(array,&ndim,dim);
        nx=dim[1];
        ny=dim[0];
        copycolors = (void*) DXAllocate(nx*ny*pixelsize);
        if(!copycolors) {
            DestroyImageInfo(new_frame_info);
            DestroyImageInfo(image_info);
            DXErrorReturn( ERROR_INTERNAL , "out of memory allocating copycolors _im_image.c");
        }
        linesize= nx*pixelsize;
        p1=(char*)colors;
        p2=(char*)copycolors;
        p1+=(ny-1)*linesize;
        for (i=0; i<ny; ++i) {
            memcpy(p2,p1,linesize);
            p1-=linesize;
            p2+=linesize;
        }
        
        image=ConstituteImage(nx,ny,"RGB",imcolortype,(void*)copycolors,&_dxd_exception_info);
	if (!image) {
            DestroyImageInfo(new_frame_info);
            DestroyImageInfo(image_info);
	    DXFree(copycolors);
            DXErrorReturn( ERROR_INTERNAL , "out of memory allocating Image _im_image.c");
	}

	image->compression = ctype;
	new_frame_info->compression = ctype;
	{
		char gam[7];
		sprintf(gam, "%2.4f", iargs->gamma);
		GammaImage(image, gam);
	}
	
        TemporaryFilename(image->filename);
	strcpy(new_frame_info->filename, image->filename);
        DEBUGMESSAGE(new_frame_info->filename);
        
		 if(iargs->reduction > 0) {
		 	int width = (int)(iargs->reduction/100.0 * nx);
		 	int height = (int)(iargs->reduction/100.0 * ny);
		 	resize_image = ResizeImage(image, width, height, LanczosFilter, 1.0, &_dxd_exception_info);
			if (!image) {
            	DestroyImageInfo(image_info);
	    		DXFree(copycolors);
            	DXErrorReturn( ERROR_INTERNAL , "out of memory allocating Image for resize in _im_image.c");
			} else {
				DestroyImage(image);
				image = resize_image;
			}
		}

	/* Write to temp file */

        err = WriteImage(new_frame_info, image);
        if(err == 0) {
            DXFree(copycolors);
            DestroyImage(image);
            DestroyImageInfo(new_frame_info);
            DestroyImageInfo(image_info);
#if MagickLibVersion > 0x0537
            DestroyConstitute();
#endif
             DXSetError(ERROR_INTERNAL, "reason = %s, description = %s",
                        image->exception.reason,
                        image->exception.description);
        }
        

        /* Now append image */

	buf = (char*) DXAllocate(4000);
	if (!buf) {
            DXFree(copycolors);
            DestroyImage(image);
            DestroyImageInfo(new_frame_info);
            DestroyImageInfo(image_info);
#if MagickLibVersion > 0x0537
            DestroyConstitute();
#endif
            DXErrorReturn( ERROR_INTERNAL , "out of memory allocating buffer _im_image.c");
        }
	
	existFile = fopen(image_info->filename, "ab");
	newFrameFile = fopen(new_frame_info->filename, "rb");
	
	while( !feof(newFrameFile) ) {
	    noir = fread(buf, 1, 4000, newFrameFile);
	    if(fwrite(buf, 1, noir, existFile) != noir) {
                DXErrorReturn( ERROR_INTERNAL, "error writing file." );
            }
	}
	
	fclose(existFile);
	fclose(newFrameFile);

	remove(new_frame_info->filename);

        DEBUGMESSAGE("back from appending the image, what did I get?");
        
	/* Now cleanup */
        DXFree(buf);
        DXFree(copycolors);
        DestroyImage(image);
        DestroyImageInfo(image_info);
        DestroyImageInfo(new_frame_info);
#if MagickLibVersion > 0x0537
        DestroyConstitute();
#endif
        
        DEBUGMESSAGE("back from DestroyImage");
        
    } else {   /* miff does not exist, do this the new way by ConstituteImage */

        field=iargs->image;
        array=(Array)DXGetComponentValue(field,"colors");
        DXGetArrayInfo(array,NULL,&dxcolortype,NULL,NULL,NULL);
        if (dxcolortype == TYPE_FLOAT) {
            imcolortype = FloatPixel;
            pixelsize = 3*sizeof(float);
        } else {
            imcolortype = CharPixel;
            pixelsize = 3;
        }

		ctype = UndefinedCompression;
		if(iargs->compression) {
			if(strcmp(iargs->compression, "BZip")==0)
				ctype = BZipCompression;
			else if(strcmp(iargs->compression, "Fax")==0)
				ctype = FaxCompression;
			else if(strcmp(iargs->compression, "Group4")==0)
				ctype = Group4Compression;
			else if(strcmp(iargs->compression, "JPEG")==0)
				ctype = JPEGCompression;
			else if(strcmp(iargs->compression, "LZW")==0)
				ctype = LZWCompression;
			else if(strcmp(iargs->compression, "RLE")==0)
				ctype = RunlengthEncodedCompression;
			else if(strcmp(iargs->compression, "Zip")==0)
				ctype = ZipCompression;
		}
        
		if(iargs->quality > 0)
			image_info->quality = iargs->quality;
        
        colors=(void*)DXGetArrayData(array);
        array=(Array)DXGetComponentValue(field,"connections");
        DXQueryGridConnections(array,&ndim,dim);
        nx=dim[1];
        ny=dim[0];
        copycolors = (void*) DXAllocate(nx*ny*pixelsize);
        if(!copycolors) {
            DestroyImageInfo(image_info);
            DXErrorReturn( ERROR_INTERNAL , "out of memory allocating copycolors _im_image.c");
        }
        linesize= nx*pixelsize;
        p1=(char*)colors;
        p2=(char*)copycolors;
        p1+=(ny-1)*linesize;
        for (i=0; i<ny; ++i) {
            memcpy(p2,p1,linesize);
            p1-=linesize;
            p2+=linesize;
        }

        image=ConstituteImage(nx,ny,"RGB",imcolortype,(void*)copycolors,&_dxd_exception_info);
	if (!image) {
            DestroyImageInfo(image_info);
	    DXFree(copycolors);
            DXErrorReturn( ERROR_INTERNAL , "out of memory allocating Image _im_image.c");
	}

        /*
          Write the image with ImageMagick
        */
        strcpy(image->filename,iargs->basename);
        if(iargs->extension) {
            strcat(image->filename,".");
            strcat(image->filename,iargs->extension);
        }

		{
			char gam[7];
			sprintf(gam, "%2.4f", iargs->gamma);
			GammaImage(image, gam);
		}
		
		/*
		 * Allow the user to use ImageMagick's resize filters to resize the image when
		 * writing. This gives us a cheap way to set up anti-aliasing.
		 */
		 
		 if(iargs->reduction > 0) {
		 	int width = (int)(iargs->reduction/100.0 * nx);
		 	int height = (int)(iargs->reduction/100.0 * ny);
		 	resize_image = ResizeImage(image, width, height, LanczosFilter, 1.0, &_dxd_exception_info);
			if (!image) {
            	DestroyImageInfo(image_info);
	    		DXFree(copycolors);
            	DXErrorReturn( ERROR_INTERNAL , "out of memory allocating Image for resize in _im_image.c");
			} else {
				DestroyImage(image);
				image = resize_image;
			}
		}

		image->compression = ctype;
		image_info->compression = ctype;

        DEBUGMESSAGE(image->filename);

        err = WriteImage(image_info,image);
        if(err == 0) {
             DXSetError(ERROR_INTERNAL, "reason = %s, description = %s",
                        image->exception.reason,
                        image->exception.description);
        }

        DXFree(copycolors);
        DestroyImage(image);
        DestroyImageInfo(image_info);
#if MagickLibVersion > 0x0537
        DestroyConstitute();
#endif
    }
    return (OK);
#else /* ndef HAVE_LIBMAGICK */

    DXErrorReturn(ERROR_NOT_IMPLEMENTED,"ImageMagick not included in build");
#endif /* def HAVE_LIBMAGICK */

}

/*
 * Search names, in order:
 *   ".<ext>", ".0.<ext>"
 *
 *   set states saying whether:
 *     contains numeric
 */
char * _dxf_BuildIMReadFileName
( char  *buf,
  int   bufl,
  char  *basename,  /* Has path but no extension */
  char  *fullname,  /* As specified */
  char  *extension, /* File extension */
  int   framenum,
  int   *numeric )  /* Numbers postfixed onto name or not? */
{
    int  i;
    int  fd = -1;
    int  appendable_files = 0;
    char framestr[16];

    DXASSERTGOTO ( NULL != buf       );
    DXASSERTGOTO ( 0      < bufl     );
    DXASSERTGOTO ( NULL != basename  );
    DXASSERTGOTO ( NULL != fullname  );
    DXASSERTGOTO ( NULL != extension );
    DXASSERTGOTO ( 0     <= framenum );
    DXASSERTGOTO ( NULL != numeric   );

    for ( i = 0; i < 2; i++ ) {
        *numeric   = i;

        if ( *numeric && ( framenum == VALUE_UNSPECIFIED ) )
            continue;

        if (*numeric && !appendable_files)
            sprintf(framestr,".%d",framenum);
        else
            framestr[0] = '\0';

        /* Make sure buffer is big enough */
        if ((strlen(basename) + strlen(framestr) + strlen(extension) + 2) > bufl)
            DXErrorReturn(ERROR_INTERNAL,"Insufficient storage space for filename");

        sprintf( buf, "%s%s.%s", basename, framestr, extension );

        if ( 0 <= ( fd = open ( buf, O_RDONLY ) ) )
            break;
    }

    if ( 0 > fd ) {
        *numeric   = 0;
        strcpy ( buf, basename );
        fd = open ( buf, O_RDONLY );
    }

    DXDebug( "R",
             "_dxf_BuildIMReadFileName: File = %s, numeric = %d",
             buf, *numeric );

    if (fd > -1)
        close ( fd );

    return buf;

}

/*
 * Determine the number of images in an ImageMagick file.
 */
#if defined(HAVE_LIBMAGICK)

static int _dxf_GetNumImagesInFile( ImageInfo *image_info ) {
    /* FIXME:  Currently, ImageMagick has no way to determine the number of
     *   images in a (potentially multi-image) image file without reading 
     *   the data for all of the images into memory at once.  This was
     *   confirmed by folks on the ImageMagick mailing list.
     *   
     * Primary evidence of this is that some readers (e.g. PNG) completely 
     *   ignore the ImageInfo subimage and subrange fields, and Magick does 
     *   not support reading images (all or a subset) in ping=1 mode.
     *   So subimage specification and multi-image ping is pointless.
     *  
     * We have a choice here between:
     *   1) general, typeless image handling, where we take a performance hit 
     *      to read all images in a multi-image sequence into VM (possibly
     *      exhausting VM) to access all, one, or some subset in DX
     *   2) general, typeless image handling, where we limit access to only
     *      one sub-image
     *   3) special cases based on image type
     *
     *   Because we support separate-file multi-image read via IM, and
     *   DX's native TIFF and MIFF readers support single-file multi-image, we
     *   choose option #2.  This routine should be fixed if/when ImageMagick
     *   adds better support for single-file multi-image reads.
     */

    return 1;
}

#endif


/*
 * Set a SizeData struct with values.
 *   set state saying whether:
 *     has multiple images in file
 *   Careful:
 *     if the images are stored internally,
 *       then reset the use_numerics flag.
 */
SizeData * _dxf_ReadImageSizesIM
( char     *name,
  int      startframe,
  SizeData *data,
  int      *use_numerics,
  int      *multiples ) {
#if !defined(HAVE_LIBMAGICK)
    DXErrorReturn(ERROR_NOT_IMPLEMENTED,"ImageMagick 5 not included in build");
#else

    Image         *image;
    ImageInfo     *image_info = NULL;
    ExceptionInfo  _dxd_exception_info;
    char           user_basename [ MAX_IMAGE_NAMELEN ];
    char           newname[ MAX_IMAGE_NAMELEN ];
    char           extension[40];
    int            fh;

    DXASSERTGOTO ( ERROR != name         );
    DXASSERTGOTO ( ERROR != data         );
    DXASSERTGOTO ( ERROR != use_numerics );
    DXASSERTGOTO ( ERROR != multiples    );

    /*  Magick init  */
    GetExceptionInfo( &_dxd_exception_info );
    image_info = CloneImageInfo( NULL );

    /*  PingImage returns info on the first image (possibly in a sequence)  */
    image_info->filename[0] = '\0';
    strncat( image_info->filename, name, sizeof( image_info->filename ) - 1 );

    if ( (image = PingImage( image_info, &_dxd_exception_info )) == NULL )
        DXErrorGoto2( ERROR_BAD_PARAMETER, "#13950",
                      /* failed to read image in file '%s' */ name );

    data->height = image->rows;
    data->width  = image->columns;

    data->startframe = ( startframe == VALUE_UNSPECIFIED ) ? 0 : startframe;
    data->endframe   = ( data->startframe +
                         _dxf_GetNumImagesInFile( image_info ) - 1 );
    *multiples = data->startframe != data->endframe;
    if ( *multiples )
        *use_numerics = 0;

    /*
    * Intent: If use_numerics (implies there aren't multiple images
    *         in this file), see if there are multiple numbered files on disk.
    */
    if ( !(*use_numerics) )
        *multiples = 0;
    else {
        /*  Magick filenames "always" have a filetype extension  */
        user_basename[0] = extension[0] = '\0';
        _dxf_ExtractImageExtension(name,img_typ_im,
                                   extension,sizeof(extension));
        strncat( user_basename, name, sizeof(user_basename)-1 );
        _dxf_RemoveImageExtension( user_basename, img_typ_im );
        _dxf_RemoveExtension( user_basename );

        do {
            sprintf( newname, "%s.%d.%s",
                     user_basename, data->endframe, extension );
            if ( (fh = open ( newname, O_RDONLY )) < 0 )
                break;

            close ( fh );
            data->endframe++;
            DXDebug ( "R", "_dxf_ReadImageSizesIM: opened: %s", newname );
        } while ( 1 );

        data->endframe--;
    }

    DXDebug ( "R",
              "_dxf_ReadImageSizesIM: h,w, s,e = %d,%d, %d,%d;  "
              "numer = %d, multi = %d",
              data->height, data->width, data->startframe, data->endframe,
              *use_numerics, *multiples );

    if ( image_info )
        DestroyImageInfo( image_info );
    if ( image )
        DestroyImage( image );
    return data;

error:
    if ( image_info )
        DestroyImageInfo( image_info );
    if ( image )
        DestroyImage( image );
    return NULL;

#endif /* HAVE_LIBMAGICK  */
}

/*
 *  Read an image from a file into a DX image field using ImageMagick.
 */
Field _dxf_InputIM( int width, int height, char *name, int relframe,
                    int delayed, char *colortype ) {
#if !defined(HAVE_LIBMAGICK)
    DXErrorReturn(ERROR_NOT_IMPLEMENTED,"ImageMagick 5 not included in build");
#else

    Field          dx_image = NULL;
    Image         *image    = NULL,
                              *image_tmp;
    ImageInfo     *image_info = NULL;
    ImageType      image_type;
    ExceptionInfo  _dxd_exception_info;
    Type           type;
    int            rank, shape[32];
    Array          colorsArray = NULL;
    Array          opacitiesArray = NULL;
    Pointer        pixels;
    Pointer        opacities = NULL;
    StorageType    storage_type;
    int            i;

    DXDebug ( "R", "_dxf_InputIM: name = %s, relframe = %d", name, relframe );

    if ((width * height) == 0)
        DXErrorGoto2( ERROR_BAD_PARAMETER, "#12275", 0 );

    /*  Magick init  */
    GetExceptionInfo( &_dxd_exception_info );
    image_info = CloneImageInfo( NULL );

    image_info->filename[0] = '\0';
    strncat( image_info->filename, name, sizeof( image_info->filename ) - 1 );

    image_info->subimage=relframe;
    image_info->subrange=1;

    if ( (image = ReadImage( image_info, &_dxd_exception_info )) == NULL )
        DXErrorGoto2( ERROR_BAD_PARAMETER, "#13950",
                      /* failed to read image in file '%s' */ name );

    if ( width  != image->columns || height != image->rows )
        DXErrorGoto(ERROR_INTERNAL,
                    "Image dimensions don't match requested dimensions" );

    /*  If it's CMYK, convert to RGB.  */
    if ( image->colorspace == CMYKColorspace )
        if ( !TransformRGBImage( image, RGBColorspace ) )
            DXErrorGoto(ERROR_INTERNAL, "CMYK->RGB transform failed" );

    /*
    * Set the colors (and possibly color map) components
    */
#if MagickLibVersion < 0x0540

    image_type = GetImageType( image );
#else

    image_type = GetImageType( image, &_dxd_exception_info );
#endif

    switch ( image_type ) {
        default :

            /*  Since we handled CMYK above, we should never get here  */
            DXErrorGoto2( ERROR_DATA_INVALID,  "#13960", image_type );

            /**********************************************************************/
            /*  OPTION #1:  Source is RGB direct (dest is float or byte direct)   */
        case TrueColorType      :
        case TrueColorMatteType :

            /*  Create the image field  */
            dx_image = DXMakeImageFormat(width, height, colortype);
            if (! dx_image)
                goto error;

            colorsArray = (Array)DXGetComponentValue(dx_image, "colors");
            DXGetArrayInfo(colorsArray, NULL, &type, NULL, &rank, shape);

            /*  Colors are 3-vectors (direct)  */
            if (rank != 1 || shape[0] != 3 ||
                    (type != TYPE_UBYTE && type != TYPE_FLOAT))
                DXErrorGoto(ERROR_INTERNAL,
                            "Invalid image field direct color format" );

            storage_type = ( type == TYPE_UBYTE ) ? CharPixel : FloatPixel;
            pixels       = DXGetArrayData(colorsArray);

            /*  Transfer the pixels to the field                       */
            /*    (NOTE: DX stores rows bottom-to-top, so flip first)  */
            image_tmp = FlipImage( image, &_dxd_exception_info );
            if ( !image_tmp )
                DXErrorGoto(ERROR_INTERNAL, "Failed to flip image" );
            DestroyImage( image );
            image = image_tmp;
            
#if MagickLibVersion < 0x0540
            DispatchImage( image, 0, 0, width, height, "RGB", storage_type,
                           pixels );
#else
    DispatchImage( image, 0, 0, width, height, "RGB", storage_type,
                   pixels, &_dxd_exception_info);
#endif
            /*  Handle transparency (opacities are floats) */
            if ( image->matte ) {
                float *optr;

                if ( !(opacitiesArray = DXNewArray( TYPE_FLOAT,
                                                    CATEGORY_REAL, 0 )) ||
                        !DXAddArrayData( opacitiesArray, 0, width*height, NULL) ||
                        !DXSetComponentValue( (Field)dx_image, "opacities",
                                              (Object)opacitiesArray ) ||
                        !(opacities = DXGetArrayData(opacitiesArray)) ||
                        !DXEndField( (Field)dx_image ) )
                    DXErrorGoto(ERROR_INTERNAL, "Failed to create opacities" );

#if MagickLibVersion < 0x0540
                DispatchImage( image, 0, 0, width, height, "A", FloatPixel,
                               opacities );
#else
    DispatchImage( image, 0, 0, width, height, "A", FloatPixel,
                   opacities, &_dxd_exception_info );
#endif

                for ( i = width * height, optr = (float *)opacities;
                        i > 0;  i--, optr++ )
                    *optr = 1.0 - *optr;
            }
            break;

            /**********************************************************************/
            /*  OPTION #2:  Source is palettized (includes gray scale or B&W)      */
            /*              (dest is float or byte direct, or byte delayed with a  */
            /*              256-element float colormap)                            */
        case PaletteType        :
        case PaletteMatteType   :
        case GrayscaleType      :
        case GrayscaleMatteType :
        case BilevelType   :

            /*  ReadImage(delayed=<y/n>) only applies to delayed color images  */
            if (delayed == DELAYED_YES)
                colortype = COLORTYPE_DELAYED;

            /*  Create the image field  */
            dx_image = DXMakeImageFormat(width, height, colortype);
            if (! dx_image)
                goto error;

            colorsArray = (Array)DXGetComponentValue(dx_image, "colors");
            DXGetArrayInfo(colorsArray, NULL, &type, NULL, &rank, shape);
            pixels = DXGetArrayData(colorsArray);

            /*  Colors are 3-vectors (direct)  */
            if (rank == 1 && shape[0] == 3) {
                if (type != TYPE_UBYTE && type != TYPE_FLOAT)
                    DXErrorGoto(ERROR_INTERNAL,
                                "Invalid image field direct color format" );

                storage_type = ( type == TYPE_UBYTE ) ? CharPixel : FloatPixel;

                /*  Transfer the pixels to the field                       */
                /*    (NOTE: DX stores rows bottom-to-top, so flip first)  */
                image_tmp = FlipImage( image, &_dxd_exception_info );
                if ( !image_tmp )
                    DXErrorGoto(ERROR_INTERNAL,
                                "Failed to flip image" );
                DestroyImage( image );
                image = image_tmp;
#if MagickLibVersion < 0x0540
                DispatchImage( image, 0, 0, width, height, "RGB", storage_type,
                               pixels );
#else
    DispatchImage( image, 0, 0, width, height, "RGB", storage_type,
                   pixels, &_dxd_exception_info );
#endif
                /*  Handle transparency (opacities are floats) */
                if ( image->matte ) {
                    float *optr;

                    if ( !(opacitiesArray = DXNewArray( TYPE_FLOAT,
                                                        CATEGORY_REAL, 0 )) ||
                            !DXAddArrayData( opacitiesArray, 0, width*height, NULL) ||
                            !DXSetComponentValue( (Field)dx_image, "opacities",
                                                  (Object)opacitiesArray ) ||
                            !(opacities = DXGetArrayData(opacitiesArray)) ||
                            !DXEndField( (Field)dx_image ) )
                        DXErrorGoto(ERROR_INTERNAL, "Failed to create opacities" );

#if MagickLibVersion < 0x0540
                    DispatchImage( image, 0, 0, width, height, "A", FloatPixel,
                                   opacities );
#else
    DispatchImage( image, 0, 0, width, height, "A", FloatPixel,
                   opacities, &_dxd_exception_info );
#endif

                    for ( i = width * height, optr = (float *)opacities;
                            i > 0;  i--, optr++ )
                        *optr = 1.0 - *optr;
                }
            }

            /*  Colors are scalars (delayed)  */
            else if (rank == 0 || (rank == 1 && shape[0] == 1)) {
                Type mType;
                int  mRank, mShape[32], mLen;
                int  i,x,y;
                float (*cmap)[3], *omap = NULL;
                Array colorMap, opacityMap;

                colorMap = (Array)DXGetComponentValue(dx_image, "color map");
                if (! colorMap)
                    DXErrorGoto(ERROR_INTERNAL,
                                "single-valued image requires a color map" );

                DXGetArrayInfo(colorMap, &mLen, &mType, NULL, &mRank, mShape);

                if (mLen != 256 || mType != TYPE_FLOAT ||
                        mRank != 1 || mShape[0] != 3)
                    DXErrorGoto(ERROR_INTERNAL,
                                "DXMakeImageFormat returned invalid delayed-color image");

                cmap = (float (*)[3]) DXGetArrayData(colorMap);

                /*  If transparency, opacities is byte & opacity map is float  */
                if ( image->matte ) {
                    if ( !(opacitiesArray = DXNewArray( TYPE_UBYTE,
                                                        CATEGORY_REAL, 0 )) ||
                            !DXAddArrayData( opacitiesArray, 0, width*height, NULL) ||
                            !DXSetComponentValue( (Field)dx_image, "opacities",
                                                  (Object)opacitiesArray ) ||
                            !(opacities = DXGetArrayData(opacitiesArray)) )
                        DXErrorGoto(ERROR_INTERNAL, "Failed to create opacities" );

                    if ( !(opacityMap = DXNewArray( TYPE_FLOAT,
                                                    CATEGORY_REAL, 0 )) ||
                            !DXAddArrayData( opacityMap, 0, 256, NULL) ||
                            !DXSetComponentValue( (Field)dx_image, "opacity map",
                                                  (Object)opacityMap ) ||
                            !(omap = DXGetArrayData(opacityMap)) )
                        DXErrorGoto(ERROR_INTERNAL, "Failed to create opacity map");

                    if ( !DXEndField( (Field)dx_image ) )
                        DXErrorGoto(ERROR_INTERNAL, "Failed to close field");
                }

                /*  Read the colormap  */
                for ( i = 0; i < image->colors; i++ ) {
                    cmap[i][0]   = ((float) image->colormap[i].red   ) / MaxRGB;
                    cmap[i][1]   = ((float) image->colormap[i].green ) / MaxRGB;
                    cmap[i][2]   = ((float) image->colormap[i].blue  ) / MaxRGB;
                    if ( image->matte )
                        omap[i] = (1.0 -
                                   ((float) image->colormap[i].opacity ) / MaxRGB);
                }
                for ( ; i < 256; i++ ) {
                    cmap[i][0] = cmap[i][1] = cmap[i][2] = 0.0;
                    if ( image->matte )
                        omap[i] = 0.0;
                }

                /*  Now copy the pixels (i.e. color/opacity map indicies)  */
                for ( y = 0; y < height; y++ ) {
                    ubyte       *pptr = ((ubyte *)pixels   ) + (height-1-y)*width;
                    ubyte       *optr = NULL;
                    PixelPacket *pixies;
                    IndexPacket *indexes, *indexes2;

                    if ( image->matte )
                        optr = ((ubyte *)opacities) + (height-1-y)*width;

                    pixies = GetImagePixels( image, 0, y, width, 1 );
                    indexes = indexes2 = GetIndexes( image );

                    if ( sizeof(indexes[0]) == 1 )
                        memcpy( pptr, indexes, width );
                    else
                        for ( x = 0; x < width; x++ )
                            if( image->matte )
                                *(pptr++) = *(optr++) = *(indexes++);
                            else
                                *(pptr++) = *(indexes++);

                    /* Opacities in colormap is wrong; use direct color map */
                    if ( image->matte )
                        for ( x = 0; x < width; x++ )
                            omap[*(indexes2++)] = ( 1.0 -
                                                    ((float) (pixies++)->opacity) / MaxRGB );
                }
            } else
                DXErrorGoto( ERROR_INTERNAL,  "unexpected image field format" );

            break;
    }

    if ( image_info )
        DestroyImageInfo( image_info );
    if ( image )
        DestroyImage( image );
    return dx_image;

error:
    if ( image_info )
        DestroyImageInfo( image_info );
    if ( image )
        DestroyImage( image );
    return NULL;

#endif /* HAVE_LIBMAGICK */
}

int  /* 0/1 on failure/success */
_dxf_ValidImageExtensionIM(char *ext) {
#if !defined(HAVE_LIBMAGICK)
    DXErrorReturn(ERROR_NOT_IMPLEMENTED,"ImageMagick 5 not included in build");
#else

    ExceptionInfo  _dxd_exception_info;
    const MagickInfo    *minfo;

    GetExceptionInfo( &_dxd_exception_info );
    minfo = GetMagickInfo( ext, &_dxd_exception_info );
    return (minfo != NULL);
#endif
}
