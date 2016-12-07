/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dxmods/_rw_image.h,v 1.8 2005/02/01 00:36:23 davidt Exp $
 */

#include <dxconfig.h>

#ifndef __RW_IMAGE_H_
#define __RW_IMAGE_H_

/*
 * This file contains basic support for reading/writing images.
 * We define enumerated image types, the tables entries that define
 * an image format, some helper routines (and much much more...).
 */
#include <stdio.h>

#define MAX_IMAGE_NAMELEN	512
#define MAX_IMAGE_PATHLEN      2048        

#define COLORTYPE_FLOAT     "DXFloat"
#define COLORTYPE_BYTE      "DXByte"
#define COLORTYPE_DELAYED   "DXDelayedColor"
#define COLORTYPE_UNSPEC    "DXInherent"

#define DELAYED_NO     0
#define DELAYED_YES    1
#define DELAYED_UNSPEC 2

/*
 * Define the various image types.
 * No entries of type 0, please.  This is the libsvs error code.
 */

typedef enum imagetype {
    img_typ_illegal = -1,
 /* img_typ_error = 0 - A place holder so 0 is not used */
    img_typ_rgb   = 1,
    img_typ_r_g_b = 2,
    img_typ_fb    = 3,
    img_typ_tiff  = 4,
    img_typ_ps_color	  = 5,
    img_typ_eps_color	  = 6,
    img_typ_ps_gray	  = 7,
    img_typ_eps_gray	  = 8,
    img_typ_gif   = 9,
    img_typ_yuv   = 10,
    img_typ_miff  = 11,
    img_typ_im  = 12
} ImageType;

/*
 * Bits that should be used in the 'flags' field of the image_info structure.
 */
/* Image format supported by DASD */
#define ADASD_OK        0x1     
/* Image format only supported on DASD */
#define ADASD_ONLY      0x2     
/* 
 * The image format supports appendable files.  That is, we don't modify
 * the file name (_dxf_BuildImageFileName()), when a frame number is specified.
 */
#define APPENDABLE_FILES 0x4	

/*
 * The format supports piping to stdout and will use the pipe if provied
 * in RWImageArgs.pipe.
 */
#define PIPEABLE_OUTPUT	 0x8

/*
 * Define the arguments that are passed to the image writing functions
 * via the following typedefs.
 * Arguments:
 * 	1) Field - the field containing the image (possibly a 
 *		series, but not composite) to write.
 *	2) char* - the filename (without extension) to use when writing
 *		the image file.
 *	3) ImageType - this is somewhat redundant but specifies the
 *		image type that is to be written out.
 *	4) int	- indicates the frame to be (over)written on the output
 *		file.  This may not be supported for each format.
 *	5) int	- a flag that if true, indicates that the output file is
 *		to be written on the disk array device. 
 *	6) FILE* - if non-zero a pipe opened by WriteImage that should be
 *		written to by the format specific methods.	
 *	
 */
/* Structure used to pass arguments to read/write routines.  */
typedef struct {
        Field 		image;
        char 		*basename;
        char 		*extension;
        ImageType 	imgtyp;
        Class		imgclass;
        int 		startframe;
        int 		adasd;
	char		*format;  /* The format specified to WriteImage */
	char		*compression; /* The compression of the format */
	unsigned int quality; /* compression level */
	unsigned int reduction; /* Amount to reduce while writing */
	float		gamma;
 	FILE		*pipe;	
} RWImageArgs;
/* Function type that does the write work */
typedef Error (*ImageWriteFunction)(RWImageArgs *iargs);

/*
 * The following structure is used to help define supported image types.
 */
typedef struct {
	/*
	 * What type of Image is this record for (tiff, rgb ...)
	 * This tag is used as a search key in ImageTable[].
	 */
        ImageType	type; 
	/*
	 * The number of files needed to support this type of file, usually
	 * just 1, but when more than 1 the 'extenstions' field should
	 * contain a list of sequentially ordered extensions.
	 */
        int     files;       
	/*
	 * A colon separated character string indicating the recognized 
	 * A (possibly colon separated) character string indicating the
	 * recognized format specifiers for this image type 
	 * (i.e. "ps:ps-color"). 
	 */
        char    *formats;
	/*
	 * A (possibly colon separated) character string indicating allowed 
	 * extensions for this image type (i.e. "r:g:b").  Also, see the 
	 * comment for the 'files' field.
	 */
        char    *extensions; 
	/*
	 * Any extra information that may be needed.
	 */
        int     flags;                  
	/*
	 * A function that can write this image format to disk.
	 */
        ImageWriteFunction write;	
	/*
	 * A function that can read this image format from disk.
	 */
        ImageWriteFunction read;	
} ImageInfo;


/* 
 * Get 'flags' field from the entry in ImageTable[] with image type 'type'
 */
ImageInfo *_dxf_ImageInfoFromType(ImageType type);

/* 
 * Determine the image type from a format specifier (i.e. "rgb", "r+g+b"...)
 * 'format' is searched for in ImageTable[] and if found the corresponding
 * 'type' field is returned. 
 */
ImageInfo *_dxf_ImageInfoFromFormat(char *format);

/* 
 * Determine the image type from a filename (i.e. "image.rgb", "image.fp"...)
 * The extension (the part after the last '.') is searched for in
 * the 'extensions' field of the ImageTable[] entries and if found the 
 * corresponding 'type' field is returned. 
 */
ImageInfo *_dxf_ImageInfoFromFileName ( char *basename);

/*
 * Remove the extension (last part of filename after '.') if it is an
 * extension for the given image type.
 */
void _dxf_RemoveImageExtension(char *name, ImageType type);

/*
 * Extract the extension (last part of filename after '.') if it is an
 * extension for the given image type.
 */
char *
_dxf_ExtractImageExtension ( char *name, ImageType type, 
                             char *ext, int ext_size );

/*
 * Get the 'member'th image of series.  Assert that dimensions match
 * a un-partition the image if it is composite.
 */
Field _dxf_GetFlatSeriesImage(Series image, int member,
                                int width, int height, int *created);



/*
 * Remove the last dotted part from a filename.
 */
char * _dxf_RemoveExtension ( char *extended );

/*-------------------- Begin _rgb_image.c ----------------------------------*/

SizeData *_dxf_ReadImageSizesADASD ( char *name, SizeData *sd );
char *_dxf_BuildImageFileName ( char *, int, char *, ImageType, int, int);
char *_dxf_ReplaceImageFileExtension ( char*, int, char*, ImageType, char*);

/*
 * The following are routines to write specific image formats to disk.
 */
/* Write an image in "fb" format, see _rgb_image.c */
Error _dxf_write_fb(RWImageArgs *iargs);

/* Write an image in "rgb" format, see _rgb_image.c */
Error _dxf_write_rgb(RWImageArgs *iargs);

/* Write an image in "r+g+b" format, see _rgb_image.c */
Error _dxf_write_r_g_b(RWImageArgs *iargs);

Error _dxf_write_yuv(RWImageArgs *iargs);

/*
 * Read/write the 'size' files associated with "rgb", "r+g+b" format 
 * Until we make ReadImage run off of ImageTable[] like WriteImage
 * does, these need to be declared external (readimage.m references them).
 */
SizeData *_dxf_ReadSizeFile ( char *name, SizeData *sd );
Error _dxf_WriteSizeFile ( char *name, SizeData sd );

/*-------------------- End _rgb_image.c ----------------------------------*/

/*-------------------- Begin _tiff.c ----------------------------------*/

char * _dxf_BuildTIFFReadFileName                                
           ( char *buf, int bufl, char *basename, char *fullname, int framenum,
             int *numeric, int *selection );

SizeData * _dxf_ReadImageSizesTIFF
            ( char *name, int startframe, SizeData *data, int *use_numerics,
              int ext_sel, int *multiples );

Field _dxf_InputTIFF
	    (int width, int height, char *name, int relframe, int delayed, char * colortype);

/* Write an image in "tiff" format, see _tiff.c */
Error _dxf_write_tiff(RWImageArgs *iargs);

/*-------------------- End _tiff.c ----------------------------------*/

/*-------------------- Begin _gif.c ----------------------------------*/

char * _dxf_BuildGIFReadFileName                                
           ( char *buf, int bufl, char *basename, char *fullname, int framenum,
             int *numeric, int *selection );

SizeData * _dxf_ReadImageSizesGIF
            ( char *name, int startframe, SizeData *data, int *use_numerics,
              int ext_sel, int *multiples );

Field _dxf_InputGIF
	    (int width, int height, char *name, int relframe, int delayed, char *colortype);

/* Write an image in "gif" format, see _gif.c */
Error _dxf_write_gif(RWImageArgs *iargs);

/*-------------------- End _gif.c ----------------------------------*/

/*-------------------- Begin _im_image.c ----------------------------------*/
Error _dxf_write_im(RWImageArgs *iargs);

char * _dxf_BuildIMReadFileName                                
           ( char *buf, int bufl, char *basename, char *fullname, 
             char *extension, int framenum, int *numeric );

SizeData * _dxf_ReadImageSizesIM
            ( char *name, int startframe, SizeData *data, int *use_numerics,
              int *multiples );

Field _dxf_InputIM( int width, int height, char *name, int relframe, 
                    int delayed, char *colortype );

int _dxf_ValidImageExtensionIM( char *ext );

/*-------------------- End  _im_image.c ----------------------------------*/
/*-------------------- Begin _ps.c ----------------------------------*/

/* Write a color image in PostScript format */
Error _dxf_write_ps_color(RWImageArgs *iargs);

/* Write a gray image in PostScript format */
Error _dxf_write_ps_gray(RWImageArgs *iargs);

/* Write a color image in Encapsulated PostScript format */
Error _dxf_write_eps_color(RWImageArgs *iargs);

/* Write a gray image in Encapsulated PostScript format */
Error _dxf_write_eps_gray(RWImageArgs *iargs);

Error _dxf_write_miff(RWImageArgs *iargs);
SizeData * _dxf_ReadImageSizesMIFF
            ( char *name, int startframe, SizeData *data, int *use_numerics,
              int ext_sel, int *multiples );

Field _dxf_InputMIFF
	    (FILE **fh, int width, int height, char *name, int relframe, int delayed, char *colortype);
/*-------------------- End _ps.c ----------------------------------*/

Error _dxf_make_gamma_table(ubyte *gamma_table, float gamma);

#endif /* __RW_IMAGE_H_ */
