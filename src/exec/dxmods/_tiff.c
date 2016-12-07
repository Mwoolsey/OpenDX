/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dxmods/_tiff.c,v 1.13 2006/01/04 22:00:51 davidt Exp $
 */

#include <dxconfig.h>

#if defined(HAVE_STRING_H)
#include <string.h>
#endif

#include <dx/dx.h>


/*
 * This file constitutes support for writing (and some reading) TIFF (Tagged
 * Image File Format) files as specified in the TIFF 5.0 standard available
 * from 
 *	Aldus Corp.			Microsoft Corp.
 *	411 First Ave. South		16011 NE 36th Way
 *	Suite 200			Box 97017
 *	Seattle WA 98104		Redmond WA 98073-9717
 *	206-622-5500			206-882-8080
 *
 * The standard is highly recommended reading, but below is short description
 * of TIFF...
 *
 * TIFF is a binary file format that consists of on 8-byte header, and
 * a series of Image File Directories (IFD) and associated data (an image).
 * Each IFD contains a set of fields that describe the associated data.
 *
 * HEADER:		Found at offset 0 in the file.
 *	bytes 0-1 :	0x4949 (LSB byte ordering) or 0x4d4d (MSB)
 *	bytes 2-3 :	TIFF Version, 5.0 corresponds to 0x002a (Fourty-Two).
 *                        (And likely all following versions.)
 *                        (Read the Aldus publication for the philosophy.)
 *	bytes 4-7 : 	FIle offset of first IFD.	
 *
 * IFD: 		Found anywhere in a file.
 *	bytes 0-1 :	the number of fields in this IFD	
 *	bytes 2-n :	the fields for this IFD (12 bytes each).
 *	bytes n-(n+4) : the file offset of the next IFD or 0 if none.	
 *
 * FIELD:
 *	bytes 0-1 :	The tag for this field (see the spec for tag types).
 *	bytes 2-3 :	The 'type' of the value of this field. 
 *	bytes 4-7 :	the number of items in the field value.
 *	bytes 8-11:	Either 1) the file offset for the value of this field
 *			or 2) the value itself if it can fit in 4 bytes.	
 *
 * Needless to say, one of the fields in an IFD indicates the location of 
 * the data for the associated image.
 *
 * Two relatively important points follow:
 *	1) We write the initial header, then the image data followed by
 *		an IFD for each image.
 *	2) We always write an MSB byte ordered image.  I was at a loss to
 *		try an write a correct LSB image.
 */

#include <stdlib.h>

#if defined(HAVE_FCNTL_H)
#include <fcntl.h>
#endif

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#include "_helper_jea.h"
#include "_rw_image.h"

#define TIFF_ATTEMPT_STRIP_SIZE	8192 /* A number suggested in TIFF document */
#define TIFF_VERSION	42	/* 11/88 Version 5.0 of TIFF */
#define TIFF_LSB	0x4949	/* Least-significant bit first */
#define TIFF_MSB	0x4d4d	/* Most-significant bit first */

#define TIFF_NO_COMPRESS	1
#define TIFF_LZW_COMPRESS	5

/*
 * Tiff tag names and their associated numbers.
 * They are not all here, only the ones we need to write DXRGB images. 
 * Augmented by Allain 30-Aug-1993.
 */
typedef enum {
        Artist                    = 315,
        BitsPerSample             = 258,
        CellLength                = 265,
        CellWidth                 = 264,
        ColorImageType_XXX        = 318,   /* or is it WhitePoint_XXX ? */
        ColorResponseCurves       = 301,
        Compression               = 259,
        DateTime                  = 306,
        DocumentName              = 269,
        FillOrder                 = 266,
        FreeByteCounts            = 289,
        FreeOffsets               = 288,
        GrayResponseCurve         = 291,
        GrayResponseUnit          = 290,
        Group3Options             = 292,
        Group4Options             = 293,
        HostComputer              = 316,
        ImageDescription          = 270,
        ImageLength               = 257,
        ImageWidth                = 256,
        Make                      = 271,
        MaxSampleValue            = 281,
        MinSampleValue            = 280,
        Model                     = 272,
        NewSubfileType            = 254,
        Orientation               = 274,
        PageName                  = 285,
        PageNumber                = 297,
        PhotometricInterpretation = 262,
        PlanarConfiguration       = 284,
        Predictor                 = 317,
        PrimaryChromaticities     = 319,
        ResolutionUnit            = 296,
        RowsPerStrip              = 278,
        SamplesPerPixel           = 277,
        Software                  = 305,
        StripByteCounts           = 279,
        StripOffsets              = 273,
        SubfileType               = 255,
        Threshholding             = 263,
        WhitePoint_XXX            = 318,
        XPosition                 = 286,
        XResolution               = 282,
        YPosition                 = 287,
        YResolution               = 283,
        ColorMap                  = 320
} TiffTag;

typedef enum {
	tiff_BYTE	= 1,
	tiff_ASCII	= 2,
	tiff_SHORT	= 3,
	tiff_LONG	= 4,
	tiff_RATIONAL	= 5, 
  SHORT_LONG = 6      /* shorthand for tiff_SHORT || tiff_LONG  */
} TiffType;
#define TIFF_RATIONAL_SIZE	(2*4)		/* two words */

static int
tiff_type_bytecount[] = { 0, 1, 1, 2, 4, 8 };

static char
*tiff_type_name[] = { NULL, "BYTE", "ASCII", "SHORT", "LONG", "RATIONAL" };


/*
 * A tiff file header. 
 */
typedef struct tiff_header {
	uint16	byte_order;
	uint16	version;
	uint32	ifd_offset;
} TiffHeader;
#define TIFF_HEADER_SIZE 8	/* bytes */

/*
 * An field entry referenced from an IFD  
 */
typedef struct tiff_field {
	uint16	tag;		/* A TiffTag */
	uint16	type;		/* A TiffType */
	uint32 	length;         /* Corrected: short -> long.  JEA */
	union {
		uint32	offset;
		uint32	value;
	} value;
} TiffField;
#define TIFF_FIELD_SIZE	12	/* bytes */

/*
 * An image file directory (IFD).
 */
typedef struct tiff_ifd {
	uint16		nfields;
	TiffField 	*fields;
	uint32		next_ifd_offset;
} TiffIFD;

/* Determine the local byte order */
#if defined(WORDS_BIGENDIAN)
# define LOCAL_MSB_ORDER
# define  read_fwd_uint32   read_msb_uint32
# define  read_fwd_uint16   read_msb_uint16
# define  read_rev_uint32   read_lsb_uint32
# define  read_rev_uint16   read_lsb_uint16
# define  write_fwd_uint32  write_msb_uint32
# define  write_fwd_uint16  write_msb_uint16
# define  write_rev_uint32  write_lsb_uint32
# define  write_rev_uint16  write_lsb_uint16
#else
# define LOCAL_LSB_ORDER
     /* XXX - PVS testing indicated need for fixes. */
# define  read_fwd_uint32   read_lsb_uint32
# define  read_fwd_uint16   read_lsb_uint16
# define  read_rev_uint32   read_msb_uint32
# define  read_rev_uint16   read_msb_uint16
# define  write_fwd_uint32  write_lsb_uint32
# define  write_fwd_uint16  write_lsb_uint16
# define  write_rev_uint32  write_msb_uint32
# define  write_rev_uint16  write_msb_uint16
#endif


/* lseek() support.
   JEA: Jun 30 1994:  we should use SEEK_* from types.h */

#ifndef L_SET
# define L_SET	0
#endif 
#ifndef L_INCR
# define L_INCR	1
#endif

static int rewrite_word(int fd, int offset, uint32 value);
static Error put_tiff_pixels(int fd, Field image, int rows, int columns,
		int strips, int pixels_per_strip, int compress,
		uint32 *strip_offsets, uint32 *strip_byte_counts, float gamma);
static Error put_tiff_header(int fd, int byte_order, int version, 
		int ifd_offset);
static int put_rgb_ifd(int fd, int width, int length, int strips, 
		int rows_per_strip, int compress, uint32 *strip_offsets, 
		uint32 *strip_byte_counts);
static int put_field(int fd, TiffTag tag, TiffType type, int n, uint32 value);
/*
 * Define the maximum length of any dimension of the images.
 * This number is only an estimate and is not expected to be used to 
 * force an image size, but may make some TIFF editors happy.
 */
#define MAX_IMAGE_DIM	4	 /* In inches, see the RESOLUTION tag */
 


static Error
write_fwd_uint32(int fd, uint32 val)
{
    if (write(fd,(char*)&val, 4) == 4)
        return OK;
    else
    {
        DXSetError
            ( ERROR_DATA_INVALID,
              "#11800", /* C standard library call, %s, returns error */
              "write()" );

        return ERROR;
    }
}


#if !defined(WORDS_BIGENDIAN)
static Error
write_rev_uint32(int fd, uint32 val)
{
    unsigned char c[4];

    c[0] = ((unsigned char *)&val)[3];
    c[1] = ((unsigned char *)&val)[2];
    c[2] = ((unsigned char *)&val)[1];
    c[3] = ((unsigned char *)&val)[0];

    if (write(fd,c,4) == 4)
        return OK;
    else
    {
        DXSetError
            ( ERROR_DATA_INVALID,
              "#11800", /* C standard library call, %s, returns error */
              "write()" );

        return ERROR;
    }
}
#endif


static Error
write_fwd_uint16(int fd, uint16 val)
{
    if (write(fd,(char*)&val, 2) == 2)
        return OK;
    else
    {
        DXSetError
            ( ERROR_DATA_INVALID,
              "#11800", /* C standard library call, %s, returns error */
              "write()" );

        return ERROR;
    }
}

#if !defined(WORDS_BIGENDIAN)
static Error
write_rev_uint16(int fd, uint16 val)
{
    unsigned char c[2];

    c[0] = ((unsigned char *)&val)[1];
    c[1] = ((unsigned char *)&val)[0];

    if (write(fd,c,2) == 2)
        return OK;
    else
    {
        DXSetError
            ( ERROR_DATA_INVALID,
              "#11800", /* C standard library call, %s, returns error */
              "write()" );

        return ERROR;
    }
}
#endif

static Error
read_fwd_uint32 ( int fd, uint32 *val )
{
    if (read(fd,(char*)val, 4) == 4)
        return OK;
    else
    {
        DXSetError
            ( ERROR_DATA_INVALID,
              "#11800", /* C standard library call, %s, returns error */
              "read()" );

        return ERROR;
    }
}


static Error
read_rev_uint32 ( int fd, uint32 *val )
{
    unsigned char c[4];

    if (read(fd,c,4) != 4)
    {
        DXSetError
            ( ERROR_DATA_INVALID,
              "#11800", /* C standard library call, %s, returns error */
              "read()" );

        return ERROR;
    }

    ((unsigned char *)val)[3] = c[0];
    ((unsigned char *)val)[2] = c[1];
    ((unsigned char *)val)[1] = c[2];
    ((unsigned char *)val)[0] = c[3];

    return OK;
}


static Error
read_fwd_uint16(int fd, uint16 *val)
{
    if (read(fd,(char*)val, 2) == 2)
        return OK;
    else
    {
        DXSetError
            ( ERROR_DATA_INVALID,
              "#11800", /* C standard library call, %s, returns error */
              "read()" );

        return ERROR;
    }
}


static Error
read_rev_uint16(int fd, uint16 *val)
{
    unsigned char c[2];

    if (read(fd,c,2) != 2)
    {
        DXSetError
            ( ERROR_DATA_INVALID,
              "#11800", /* C standard library call, %s, returns error */
              "read()" );

        return ERROR;
    }

    ((unsigned char *)val)[1] = c[0];
    ((unsigned char *)val)[0] = c[1];

    return OK;
}

/*
 * Write out a "tiff", formatted image file from the specified field.  
 * The input field should not be composite, but may be series.
 * By the time we get called, it has been asserted that we can write
 * the image to the device, be it ADASD or otherwise.
 *
 * NOTE: The semantics here differ from those for DXRGB images in that
 * the frame number is ignored, a new file is always created and if 
 * given a series all images in the series are written out.
 */
Error
_dxf_write_tiff(RWImageArgs *iargs)
{
    uint32   last_ifd_offset_location, offset; 
    char     imagefilename[MAX_IMAGE_NAMELEN];
    uint32   *strip_offsets=NULL, *strip_byte_counts;
    int      i, fd = -1, series, rows, columns, row_size, compress;
    int      firstframe, lastframe, frames, rows_per_strip; 
    int      strips, deleteable = 0;
    
    SizeData  image_sizes;  /* Values as found in the image object itself */
    Field     img = NULL;
 
    if (iargs->adasd)
	DXErrorGoto(ERROR_NOT_IMPLEMENTED, 
		"Tiff format not supported on disk array");

    if ( !_dxf_GetImageAttributes ( (Object)iargs->image, &image_sizes ) )
        goto error;

    series = iargs->imgclass == CLASS_SERIES; 
    frames = (image_sizes.endframe - image_sizes.startframe + 1); 

    /*
     * Always write all frames of a series.
     */
    firstframe = 0;
    lastframe = frames-1;

    /*
     * Create an appropriate file name. 
     */
    if (!_dxf_BuildImageFileName(imagefilename,MAX_IMAGE_NAMELEN,
                                 iargs->basename,
   				 iargs->imgtyp,iargs->startframe,0))
            goto error;

   /* 
    * Get some nice statistics
    */
    rows = image_sizes.height;
    columns = image_sizes.width;
    row_size = columns * 3;
    rows_per_strip = TIFF_ATTEMPT_STRIP_SIZE / row_size + 1;
    if (rows_per_strip > rows)
    	rows_per_strip = rows; 
    strips = (rows + rows_per_strip - 1) / rows_per_strip; 

    if (strips < 1) 
    	DXErrorGoto(ERROR_UNEXPECTED, "Tiff encountered less than 1 strip");

    /*
     * DXAllocate some space to save strip offsets and sizes. 
     */
    strip_offsets = (uint32*)DXAllocateLocal(2 * strips * sizeof(uint32));
    if (!strip_offsets)
	goto error;
    strip_byte_counts = &strip_offsets[strips];
	

    /*
     * Attempt to open image file and position to start of frame(s).
     */
    if ( (fd = creat (imagefilename, 0666 ) ) < 0 )
	ErrorGotoPlus1 ( ERROR_DATA_INVALID,
                      "Can't open image file (%s)", imagefilename );


    /*
     * Skip the header, write it later 
     * Save the location of the IFD pointer.
     */
    lseek(fd,TIFF_HEADER_SIZE, L_SET); 
    last_ifd_offset_location = TIFF_HEADER_SIZE - 4;

    compress = TIFF_NO_COMPRESS;	/* LZW compress not implemented yet */
    img = iargs->image;
    for (i=firstframe ; i<=lastframe ; i++)
    {
        if (series) {
	    if (deleteable) DXDelete((Object)img);
	    img = _dxf_GetFlatSeriesImage((Series)iargs->image,i,
			columns,rows,&deleteable);
	    if (!img) 
		goto error;
	}

	if (!put_tiff_pixels(fd, img, rows, columns, strips,
			rows_per_strip*columns, compress, 
			strip_offsets, strip_byte_counts, iargs->gamma))
	    goto error;

	offset = put_rgb_ifd(fd, columns, rows, strips, rows_per_strip, 
				compress, strip_offsets, strip_byte_counts);
	if (!offset)
	    goto bad_write;
		
	if (!rewrite_word(fd, last_ifd_offset_location, offset))
	    goto bad_write;
	
	last_ifd_offset_location = lseek(fd, 0, L_INCR) - 4;
    }
 	
    /* Set the last IFD next pointer to 0 */
    if (!rewrite_word(fd, last_ifd_offset_location, 0))
	goto bad_write;

    /*
     * Write out the header, indicating, byte ordering, version number
     * offset of 0th IFD. NOTE: we always write an MSB tiff image. 
     */
    if (!put_tiff_header(fd, (int)TIFF_MSB, TIFF_VERSION, 0))
	goto bad_write;
	

    if (fd >= 0) close(fd);
    if (deleteable && img) DXDelete((Object)img);
    if (strip_offsets) DXFree((Pointer)strip_offsets);

    return OK;

bad_write:
    DXErrorGoto( ERROR_DATA_INVALID, "Can't write TIFF file");
error:
    if ( fd >= 0 ) close(fd);
    if (deleteable && img) DXDelete((Object)img);
    if (strip_offsets) DXFree((Pointer)strip_offsets);

    return ERROR;
}

/* 
 * Write out an image file directory and its associated strip offsets and
 * byte counts, the resolution and the bits per sample for our DXRGB image. 
 * Below is a diagram of what is written.
 *
 *           . . .
 *	|    1 / 1	|	x- and y- resolutions.
 *	|	        |	Strip offsets and byte counts.
 *           . . .
 *	|	        |
 *	|   8 8 8 8	|	bits per sample (the last 8 is filler)
 *	|     IFD 	|	Beginning of IFD.
 *           . . .
 *
 * The file pointer is aligned to a word boundary before we begin any
 * writes.
 * Returns 0 on failure, and the file offset of the IFD on success.
 */

static int /* offset of first byte of IFD */	
put_rgb_ifd(int fd, int width, int length, int strips, int rows_per_strip, 
	int compress, uint32 *strip_offsets, uint32 *strip_byte_counts)
{
    uint32  rationals[2], zero=0, i;	
    uint32  xres_offset, yres_offset, bps_offset, ifd_offset, offset;
    uint32  sbc_offset=0, so_offset=0;
    uint16  nentries=13;
    uint16  uint16s[4];

 
    /* 
     * Align the file pointer to a word boundary
     */
    xres_offset = lseek(fd,0,L_INCR);
    if (xres_offset & 3) {
	xres_offset = (xres_offset + 3) & ~3;
	if (lseek(fd, xres_offset, L_SET) < 0)
	    return(0);
    }

    /* 
     * Write XResolution and YResolution values  (they are equal). 
     * Create a ratio that will result in an image that has its
     * largest dimension MAX_IMAGE_DIM inches (inches, assuming the
     * RESOLUTION tag is set to 2).
     */ 
    yres_offset = xres_offset; 
    rationals[0] = MAX(length, width); 
    rationals[1] = MAX_IMAGE_DIM;
    if ( !write_msb_uint32(fd, rationals[0]) ||
         !write_msb_uint32(fd, rationals[1])   ) 
	return(0);
    offset = xres_offset + TIFF_RATIONAL_SIZE;

    /* 
     * Write the StripOffsets  and ByteCounts
     */ 
    if (strips > 1) {
	so_offset = offset; 
	for (i=0 ; i<strips ; i++)
	    if (!write_msb_uint32(fd, strip_offsets[i]))
	        return(0);
	sbc_offset = so_offset + strips*4;
	for (i=0 ; i<strips ; i++)
	    if (!write_msb_uint32(fd, strip_byte_counts[i]))
	        return(0);
	offset += 2*strips*4;
    }

    /* 
     * Write BitsPerSample (8,8,8) Value, write an extra uint16 to keep
     * file pointer, word aligned.
     */ 
    bps_offset = offset; 
    uint16s[0] = uint16s[1] = uint16s[2] = uint16s[3] = 8;
    for (i=0 ; i<4 ; i++)
        if (!write_msb_uint16(fd, uint16s[i]))
 	    return(0);
    offset += 4*2;
      
    /*
     * Begin the IFD, first uint16 indicates the number of fields. 
     */
    ifd_offset = offset; 
    if (!write_msb_uint16(fd, nentries))
	return(0);
    
         
    /*
     * Write out each of the fields, the fields must be sorted 
     * in ascending tag value order, so the order of operation is important.
     */
    if (!put_field(fd, ImageWidth, tiff_LONG, 1, width) 		||
	!put_field(fd, ImageLength, tiff_LONG, 1, length)		||
	!put_field(fd, BitsPerSample, tiff_SHORT, 3, bps_offset) 	||
	!put_field(fd, Compression, tiff_SHORT, 1, compress)		||
	!put_field(fd, PhotometricInterpretation, tiff_SHORT, 1, 2))
		return(0);

    /* If only one strip, put the offset not the offset of the offset list */
    if (((strips  == 1)  && 
	!put_field(fd, StripOffsets, tiff_LONG, 1, *strip_offsets)) ||
	!put_field(fd, StripOffsets, tiff_LONG, strips, so_offset))	
		return(0);

    if (!put_field(fd, SamplesPerPixel, tiff_SHORT, 1, 3) 		||
	!put_field(fd, RowsPerStrip, tiff_LONG, 1, rows_per_strip)) 
		return(0);

    /* If only one strip, put the byte count not the offset of the count list */
    if (((strips == 1)  && 
	!put_field(fd, StripByteCounts, tiff_LONG, 1, *strip_byte_counts)) ||
	!put_field(fd, StripByteCounts, tiff_LONG, strips, sbc_offset)) 
		return(0);

    if (!put_field(fd, XResolution, tiff_RATIONAL, 1, xres_offset)	||
	!put_field(fd, YResolution, tiff_RATIONAL, 1, yres_offset)	||
	!put_field(fd, PlanarConfiguration, tiff_SHORT, 1, 1)		||
	!put_field(fd, ResolutionUnit, tiff_SHORT, 1, 2) )
		return(0);

    /*
     * Write out the pointer to the next IFD, we don't know it now so
     * indicate there isn't one by writing 0.
     */
    if (!write_msb_uint32(fd,zero))
	return(0);
    
    return(ifd_offset);
}

/*
 * Write an individual field of a directory entry at the current file
 * offset for the given file descriptor.
 * Upon entry, the value to be written is assumed to be right justified
 * and if the size (specifed by type and n) indicates, the value(s)
 * are left justified as per the TIFF spec.
 * 
 * Returns 0/1 on failure/success.
 */
static int
put_field(int fd, TiffTag tag, TiffType type, int n, uint32 value)
{
	uint16 s;
	uint32 l;

	s = (uint16)tag;
	if (!write_msb_uint16(fd, s))
		return(0);

	s = (uint16)type;
	if (!write_msb_uint16(fd, s))
		return(0);

	l = (uint32)n;
	if (!write_msb_uint32(fd, l))
		return(0);

	/* Left justify value if indicated by the size */
	switch (type) {
	    case tiff_ASCII:
	    case tiff_BYTE: if (n < 4) value <<= (4-n)*8;
		break;
	    case tiff_SHORT: if (n == 1) value <<= 16;
		break;
	    default:
	        break;
	}

	if (!write_msb_uint32(fd, value))
		return(0);
	return(1);
}

/*
 * Overwrite the word at offset in fd with value.  
 * On return, the file pointer for fd is restored to the offset at entrance.
 * Return 0/1 on failure/success.
 */
static int
rewrite_word(int fd, int offset, uint32 value)
{
	int old_offset = lseek(fd, 0, L_INCR);

	if ((old_offset < 0) 			||
	   (lseek(fd, offset, L_SET) < 0) 	||
	   !write_msb_uint32(fd, value) 		||
	   (lseek(fd, old_offset, L_SET) < 0))
		return(0);

	return(old_offset); 
}

/*
 * Write out the DXRGB pixels of the given image which has 'rows' rows,
 * and 'columns' columns.  Pixels are to be written in 'strips' strips
 * with 'pixels_per_strip' pixels in each strip.  'compress' indicates
 * which type of compression to be done LZW or none.  On output
 * strip_offset[i] contains the offset of strip i and strip_byte_counts[i]
 * contains the number of bytes in strip i.
 *
 * Returns ERROR/OK on failure/success and sets the error code on failure. 
 */
static Error
put_tiff_pixels(int fd, 
		Field image, int rows, int columns,
		int strips, int pixels_per_strip, int compress,
		uint32 *strip_offsets, uint32 *strip_byte_counts, float gamma)
{
    uint32 count, totpixels, npixels, i, j, offset, y,x;
    Pointer *pixels;
    unsigned char	*rgb=NULL;
    Array colorsArray, colorMapArray;
    RGBColor *map = NULL;
#ifdef HAVE_LZW_COMPRESS
    unsigned char	*comp_rgb=NULL;
#endif
    Type type;
    int rank, shape[32];
    ubyte gamma_table[256];

    DXASSERTGOTO ( compress != TIFF_LZW_COMPRESS );

    _dxf_make_gamma_table(gamma_table, gamma);

    colorsArray = (Array)DXGetComponentValue(image, "colors");
    if (! colorsArray)
	goto error;
    
    DXGetArrayInfo(colorsArray, NULL, &type, NULL, &rank, shape);

    if (type == TYPE_UBYTE)
    {
	if (rank == 0 || (rank == 1 && shape[0] == 1))
	{
	    colorMapArray = (Array)DXGetComponentValue(image, "color map");
	    if (! colorMapArray)
	    {   
		DXSetError(ERROR_DATA_INVALID,
		   "single-valued byte image requires color map");
		goto error;
	    }

	    map = (RGBColor *)DXGetArrayData(colorMapArray);
	}
	else if (rank != 1 || shape[0] != 3)
	{
	    DXSetError(ERROR_DATA_INVALID, 
		"images must either contain single-valued pixels with a %s",
		"colormap or contain three-vector pixels");
	    goto error;
	}
    }
    else if (type == TYPE_FLOAT)
    {
	if (rank != 1 || shape[0] != 3)
	{
	    DXSetError(ERROR_DATA_INVALID, 
		"images must either contain single-valued pixels with a %s",
		"colormap or contain three-vector pixels");
	    goto error;
	}
    }
    else
    {
	DXSetError(ERROR_DATA_INVALID, 
		"pixel data must be either unsigned chars or floats");
	goto error;
    }

    if (!(pixels = DXGetArrayData(colorsArray)))
	goto error;

    offset = lseek(fd, 0, L_INCR);

    rgb = (unsigned char*)DXAllocateLocal(pixels_per_strip*3);
    if (!rgb)
	goto error;

#ifdef HAVE_LZW_COMPRESS
    if (compress == TIFF_LZW_COMPRESS)
    {
	comp_rgb = (unsigned char*)DXAllocateLocal(pixels_per_strip*3);
	if (!comp_rgb)
	    goto error;
    }
#endif

	
   /* 
    * This is somewhat of a mess, but seems to be required. DX images
    * are stored from left to right and bottom to top, we  need to
    * write them out from left to right and top to bottom.  Plus,
    * we need to break (set strip_offsets[] and strip_byte_counts[],
    * and write a strip) when we hit the end of strip or the end of the
    * image.
    */
#define CLAMP(a,b)				\
{						\
     int f = (a) * 255.0;			\
     f = (f < 0) ? 0 : (f > 255) ? 255 : f;	\
     (b) = (unsigned char)gamma_table[f];	\
}

    y = rows - 1;
    totpixels = rows*columns;

    if (map)
    {
	ubyte *from = ((ubyte *)pixels) + y*columns;

	for (count=i=j=npixels=0, x=1 ; (npixels < totpixels) ; )
	{
	    RGBColor *mapped = map + *from;

	    CLAMP(mapped->r, rgb[j++]);
	    CLAMP(mapped->g, rgb[j++]);
	    CLAMP(mapped->b, rgb[j++]);

	    if ((++count == pixels_per_strip) || (y==0 && x==columns)) {
		/*
		 * Time to write out a strip
		 */
#ifdef HAVE_LZW_COMPRESS
		if (compress == TIFF_LZW_COMPRESS)
		    tiff_lzw_compress(comp_rgb,rgb,j);
#endif
		if (write(fd,(char*)rgb,j) < j)
		    DXErrorGoto(ERROR_DATA_INVALID,"Can't write output file");
		strip_offsets[i] = offset;
		strip_byte_counts[i] = j;
		offset += j;
		npixels += count;
		j = count = 0;
		i++;
	    }
	    if (x == columns) {
		    y--;
		    x = 1;
		    from = ((ubyte *)pixels) + y*columns;
	    } else {
		    x++;
		    from++;
	    }
	}
    }
    else if (type == TYPE_FLOAT)
    {
	RGBColor *from = ((RGBColor *)pixels) + y*columns;

	for (count=i=j=npixels=0, x=1 ; (npixels < totpixels) ; )
	{
	    CLAMP(from->r, rgb[j++]);
	    CLAMP(from->g, rgb[j++]);
	    CLAMP(from->b, rgb[j++]);

	    if ((++count == pixels_per_strip) || (y==0 && x==columns)) {
		/*
		 * Time to write out a strip
		 */
#ifdef HAVE_LZW_COMPRESS
		if (compress == TIFF_LZW_COMPRESS)
		    tiff_lzw_compress(comp_rgb,rgb,j);
#endif
		if (write(fd,(char*)rgb,j) < j)
		    DXErrorGoto(ERROR_DATA_INVALID,"Can't write output file");
		strip_offsets[i] = offset;
		strip_byte_counts[i] = j;
		offset += j;
		npixels += count;
		j = count = 0;
		i++;
	    }
	    if (x == columns) {
		    y--;
		    x = 1;
		    from = ((RGBColor *)pixels) + y*columns;
	    } else {
		    x++;
		    from++;
	    }
	}
    }
    else
    {
	RGBByteColor *from = ((RGBByteColor *)pixels) + y*columns;

	for (count=i=j=npixels=0, x=1 ; (npixels < totpixels) ; )
	{
	    rgb[j++] = gamma_table[from->r];
	    rgb[j++] = gamma_table[from->g];
	    rgb[j++] = gamma_table[from->b];

	    if ((++count == pixels_per_strip) || (y==0 && x==columns)) {
		/*
		 * Time to write out a strip
		 */
#ifdef HAVE_LZW_COMPRESS
		if (compress == TIFF_LZW_COMPRESS)
		    tiff_lzw_compress(comp_rgb,rgb,j);
#endif
		if (write(fd,(char*)rgb,j) < j)
		    DXErrorGoto(ERROR_DATA_INVALID,"Can't write output file");
		strip_offsets[i] = offset;
		strip_byte_counts[i] = j;
		offset += j;
		npixels += count;
		j = count = 0;
		i++;
	    }
	    if (x == columns) {
		    y--;
		    x = 1;
		    from = ((RGBByteColor *)pixels) + y*columns;
	    } else {
		    x++;
		    from ++;
	    }
	}
    }


    DXFree((Pointer)rgb);
#ifdef HAVE_LZW_COMPRESS
    DXFree((Pointer)comp_rgb);
#endif
    return OK;
error:
    if (rgb) DXFree((Pointer)rgb);
#ifdef HAVE_LZW_COMPRESS
    if (comp_rgb) DXFree((Pointer)comp_rgb);
#endif
    return ERROR;
}
/*
 * Write the TIFF header to offset 0 of file descriptor fd.
 * Returns 0/1 on failure/success.
 */
static Error
put_tiff_header(int fd, int byte_order, int version, int ifd_offset)
{
	uint16 s[2];
	uint32 l;

	lseek(fd, 0, L_SET);

	s[0] = (uint16)(byte_order&0xffff);
	s[1] = (uint16)(version&0xffff);
	if (!write_msb_uint16(fd,s[0]) || !write_msb_uint16(fd,s[1]))
	    return(0);

	l = ifd_offset;
	if (l > 0) 
	    if (!write_msb_uint32(fd,l))
		return(0);

	return(1);
}


/*----------------------------------------------------------------------------*/


    typedef struct 
    {
        TiffTag  tag;
        char     *tag_name;
        TiffType tiff_type;
        uint32   size;
    }
    tag_table_entry;


#define    FUNCTION  -1
#define        NONE  -1

    /* 
     * The entries (plural!) 318 are not to be trusted since there is no
     *   determining which interpretation to use.
     */ 
    tag_table_entry
    tag_entries[]
    =
    { { NewSubfileType,      "NewSubfileType",     tiff_LONG,     1 },
      {  SubfileType,         "SubfileType",        tiff_SHORT,    1 },
      {  ImageWidth,          "ImageWidth",         SHORT_LONG,    1 },
      {  ImageLength,         "ImageLength",        SHORT_LONG,    1 },
      {  BitsPerSample,       "BitsPerSample",      tiff_SHORT,    FUNCTION },
      {  Compression,         "Compression",        tiff_SHORT,    1 },
      {  PhotometricInterpretation,
                      "PhotometricInterpretation", tiff_SHORT,    1 },
      {  Threshholding,       "Threshholding",      tiff_SHORT,    1 },
      {  CellWidth,           "CellWidth",          tiff_SHORT,    1 },
      {  CellLength,          "CellLength",         tiff_SHORT,    1 },
      {  FillOrder,           "FillOrder",          tiff_SHORT,    1 },
      {  DocumentName,        "DocumentName",       tiff_ASCII,    NONE },
      {  ImageDescription,    "ImageDescription",   tiff_ASCII,    NONE },
      {  Make,                "Make",               tiff_ASCII,    NONE },
      {  Model,               "Model",              tiff_ASCII,    NONE },
      {  StripOffsets,        "StripOffsets",       SHORT_LONG,    FUNCTION },
      {  Orientation,         "Orientation",        tiff_SHORT,    1 },
      {  SamplesPerPixel,     "SamplesPerPixel",    tiff_SHORT,    1 },
      {  RowsPerStrip,        "RowsPerStrip",       SHORT_LONG,    1 },
      {  StripByteCounts,     "StripByteCounts",    SHORT_LONG,    FUNCTION },
      {  MinSampleValue,      "MinSampleValue",     tiff_SHORT,    FUNCTION },
      {  MaxSampleValue,      "MaxSampleValue",     tiff_SHORT,    FUNCTION },
      {  XResolution,         "XResolution",        tiff_RATIONAL, 1 },
      {  YResolution,         "YResolution",        tiff_RATIONAL, 1 },
      {  PlanarConfiguration, "PlanarConfiguration",tiff_SHORT,    1 },
      {  PageName,            "PageName",           tiff_ASCII,    NONE },
      {  XPosition,           "XPosition",          tiff_RATIONAL, NONE },
      {  YPosition,           "YPosition",          tiff_RATIONAL, NONE },
      {  FreeOffsets,         "FreeOffsets",        tiff_LONG,     NONE },
      {  FreeByteCounts,      "FreeByteCounts",     tiff_LONG,     NONE },
      {  GrayResponseUnit,    "GrayResponseUnit",   tiff_SHORT,    1 },
      {  GrayResponseCurve,   "GrayResponseCurve",  tiff_SHORT,    FUNCTION },
      {  Group3Options,       "Group3Options",      tiff_LONG,     1 },
      {  Group4Options,       "Group4Options",      tiff_LONG,     1 },
      {  ResolutionUnit,      "ResolutionUnit",     tiff_SHORT,    1 },
      {  PageNumber,          "PageNumber",         tiff_SHORT,    2 },
      {  ColorResponseCurves, "ColorResponseCurves",tiff_SHORT,    FUNCTION },
      {  Software,            "Software",           tiff_ASCII,    NONE },
      {  DateTime,            "DateTime",           tiff_ASCII,    20 },
      {  Artist,              "Artist",             tiff_ASCII,    NONE },
      {  HostComputer,        "HostComputer",       tiff_ASCII,    NONE },
      {  Predictor,           "Predictor",          tiff_SHORT,    1 },
      {  WhitePoint_XXX,      "WhitePoint",         tiff_RATIONAL, 2 },
      {  ColorImageType_XXX,  "ColorImageType",     tiff_SHORT,    1 },
      {  PrimaryChromaticities,
                          "PrimaryChromaticities", tiff_RATIONAL, 6 },
      {  ColorMap,            "ColorMap",           tiff_SHORT,    FUNCTION },
      {  ColorMap,            NULL,                 tiff_SHORT,    FUNCTION }
        };



static
tag_table_entry * tiff_lookup ( TiffTag tag )
{
    int    i;
    static tag_table_entry error
           = { (TiffTag) -99, "** Error **", (TiffType) -99, (uint32) -99 };

    for ( i=0; tag_entries[i].tag_name != NULL; i++ )
        if ( tag_entries[i].tag == tag )
            return &tag_entries[i];
    
    return &error;
}



  /*
   * Search names, in order:
   *   ".tiff", ".tif", ".0.tiff", ".0.tif"
   *
   *   set states saying whether:
   *     contains numeric
   *     has which extension.
   */
char * _dxf_BuildTIFFReadFileName 
    ( char  *buf,
      int   bufl,
      char  *basename,  /* Does not have an extension */
      char  *fullname,  /* As specified */
      int   framenum,
      int   *numeric,   /* Numbers postfixed onto name or not? */
      int   *selection  /* file name extension ? (enumeral) */ )
{
    int  i;
    int  fd = -1;

    DXASSERTGOTO ( ERROR != buf      );
    DXASSERTGOTO ( 0      < bufl     );
    DXASSERTGOTO ( ERROR != basename );
    DXASSERTGOTO ( ERROR != fullname );
    DXASSERTGOTO ( 0     <= framenum );
    DXASSERTGOTO ( ERROR != numeric   );
    DXASSERTGOTO ( ERROR != selection );

    for ( i = 0;
          i < 4;
          i++ )
    {
        *numeric   = i / 2;
        *selection = i % 2;
 
        if ( ( *numeric == 1 ) && ( framenum == VALUE_UNSPECIFIED ) ) continue;

        if ( !_dxf_BuildImageFileName 
                  ( buf,
                    bufl,
                    basename,
                    img_typ_tiff,
                    (*numeric==1)? framenum : -1,
                    *selection ) )
            goto error;

        if ( 0 <= ( fd = open ( buf, O_RDONLY ) ) ) 
            break;
    }

    if ( 0 > fd )
    {
        *numeric   = 0;
        *selection = 2;

        strcpy ( buf, basename );

        fd = open ( buf, O_RDONLY ); 
    }

    DXDebug( "R",
             "_dxf_BuildTIFFReadFileName: Filen = %s, numer = %d, ext_sel = %d",
             buf,
             *numeric,
             *selection );

    if (fd > -1)
	close ( fd );

    return buf;

    error:
        return ERROR;
}



static
TiffHeader * read_tiff_header
                 ( int fh,
                   TiffHeader *hdr,
                   Error (**rs)(int,uint16*),
                   Error (**rl)(int,uint32*) )
{
    int32 file_last;

    DXASSERTGOTO ( fh  >  0 );
    DXASSERTGOTO ( hdr != ERROR );
    DXASSERTGOTO ( rs  != ERROR );
    DXASSERTGOTO ( rl  != ERROR );

    if ( 2 != read ( fh, &hdr->byte_order, 2 ) ) goto error;

    switch ( hdr->byte_order )
    {
        default:
            DXErrorGoto
                ( ERROR_DATA_INVALID,
                  "TIFF file header must start with 0x4949 or 0x4D4D" );
            break;

        case TIFF_LSB:  *rs = read_lsb_uint16;  *rl = read_lsb_uint32;  break;
        case TIFF_MSB:  *rs = read_msb_uint16;  *rl = read_msb_uint32;  break;
    }

    if ( !(*(*rs)) ( fh, &hdr->version    ) ||
         !(*(*rl)) ( fh, &hdr->ifd_offset )   )
        goto error;

    if ( hdr->version != 42 )
        DXWarning ( "The TIFF version number in the file is not 42." );

    DXDebug ( "R",
              "ordering,version,offset = %4x,%d,%d",
              hdr->byte_order,
              hdr->version,
              hdr->ifd_offset );

    if ( 0 > ( file_last = lseek ( fh, 0, 2 ) ) )
        DXErrorGoto2
            ( ERROR_DATA_INVALID,
              "#11800", /* C standard library call, %s, returns error */
              "lseek()" )

    if ( hdr->ifd_offset > file_last )
    {
        DXDebug("R", 
               "The TIFF initial offset (%d) is greater than file length (%d).",
                hdr->ifd_offset, file_last );

        DXErrorGoto
            ( ERROR_DATA_INVALID,
              "The TIFF initial offset is greater than file length." );
    }
 
    if ( 0 > lseek ( fh, hdr->ifd_offset, 0 ) )
        DXErrorGoto2
            ( ERROR_DATA_INVALID,
              "#10911", /* Seeking to offset %d in binary file failed */
              hdr->ifd_offset )

    return hdr;

    error:
        return ERROR;
}



static 
TiffField * read_tiff_field
                ( int        fh,
                  TiffField  *fld, 
                  Error      (*rs)(int,uint16*),
                  Error      (*rl)(int,uint32*) )
{
    int i;
    int save;

    DXASSERTGOTO ( fh  >  0 );
    DXASSERTGOTO ( fld != ERROR );
    DXASSERTGOTO ( rs  != ERROR );
    DXASSERTGOTO ( rl  != ERROR );

    if ( !rs ( fh, &fld->tag    ) ||
         !rs ( fh, &fld->type   ) ||
         !rl ( fh, &fld->length )   )
        goto error;

    if ( 0 > ( save = lseek ( fh, 0, 1 ) ) )
        DXErrorGoto2
            ( ERROR_DATA_INVALID,
              "#11800", /* C standard library call, %s, returns error */
              "lseek()" )
    else
        save += 4;

    DXDebug ( "R",
              "tag,type,length = %d %s, %d %s, %d",
              fld->tag,  ( tiff_lookup ( (TiffTag) fld->tag ) ) -> tag_name,
              fld->type, ((fld->type>=1)&&(fld->type<=5)) ?
                             tiff_type_name [ fld->type ] :
                             "** Error **",
              fld->length );

    if ( ( tiff_type_bytecount [ fld->type ] * fld->length ) > 4 )
        { if ( !rl ( fh, &fld->value.offset ) ) goto error; }
    else
        switch ( fld->type )
        {
            case tiff_BYTE:  
            case tiff_ASCII:
                if (read ( fh, &fld->value.value, fld->length ) != fld->length )
                    DXErrorGoto2
                        ( ERROR_DATA_INVALID,
                          "#11800",
                          /* C standard library call, %s, returns error */
                          "read()" );
                break;

            case tiff_SHORT:
                for ( i=0; i<fld->length; i++ )
                    if ( !rs ( fh, &((uint16*)&fld->value.value)[i] ) )
                        goto error;
                break;

            case tiff_LONG:
                if ( !rl ( fh, &fld->value.value ) )
                    goto error;
                break;
        }

    if ( 0 > lseek ( fh, save, 0 ) )
        DXErrorGoto2
            ( ERROR_DATA_INVALID,
              "#10911", /* Seeking to offset %d in binary file failed */
              save );

    return fld;

    error:
        return ERROR;
}



static
  /*
   * Read sizes from an opened TIFF file.
   *   return read pointers for checking next entry in file IFD.
   *
   * Problem:
   *   Image sizes may change internal to a TIFF file, yet we want only one.
   * Solution:
   *   Read and use only the first image H,W.
   *   (Alternative, not used: have restrictor parameters EG:strt,end)
   */
SizeData * read_image_sizes_tiff
               ( SizeData *sd,
                 int      fh,
                 Error    (*rs)(int,uint16*),
                 Error    (*rl)(int,uint32*) )
{
    TiffField  field;
    uint16     tiff_entity_count;
    int        i;

    DXASSERTGOTO ( sd != ERROR );
    DXASSERTGOTO ( fh != ERROR );
    DXASSERTGOTO ( rs != ERROR );
    DXASSERTGOTO ( rl != ERROR );

    sd->width      = -1;
    sd->height     = -1;
    sd->startframe = -1;
    sd->endframe   = -1;

    if ( !rs ( fh, &tiff_entity_count ) )
        goto error;

    DXDebug ( "R",
              "read_image_sizes_tiff: tiff_entity_count = %d",
              tiff_entity_count );

    for ( i=0;
          i < tiff_entity_count;
          i++ )
        if ( !read_tiff_field ( fh, &field, rs, rl ) )
            goto error;
        else
            switch ( field.tag )
            {
                case ImageWidth:
                    if ( field.length != 1 )
                        DXErrorGoto2
                            ( ERROR_DATA_INVALID,
                              "TIFF field `%s' has bad size setting.",
                              "ImageWidth" );

                    if ( field.type == tiff_SHORT )
                        sd->width = ((uint16*)&field.value.value)[0];
                    else if ( field.type == tiff_LONG )
                        sd->width = field.value.value;
                    else
                        DXErrorGoto2
                            ( ERROR_DATA_INVALID,
                              "TIFF field `%s' has bad type.",
                              "ImageWidth" );
                    break;

                case ImageLength:
                    if ( field.length != 1 )
                        DXErrorGoto2
                            ( ERROR_DATA_INVALID,
                              "TIFF field `%s' has bad size setting.",
                              "ImageLength" );

                    if ( field.type == tiff_SHORT )
                        sd->height = ((uint16*)&field.value.value)[0];
                    else if ( field.type == tiff_LONG )
                        sd->height = field.value.value;
                    else
                        DXErrorGoto2
                            ( ERROR_DATA_INVALID,
                              "TIFF field `%s' has bad type.",
                              "ImageLength" );
                    break;

                default:
                    ;
            }

    if ( sd->width == -1 )
        DXErrorGoto
        ( ERROR_DATA_INVALID, "Required TIFF field `ImageWidth' is missing" );

    if ( sd->height == -1 )
        DXErrorGoto
        ( ERROR_DATA_INVALID, "Required TIFF field `ImageLength' is missing" );

    DXDebug ( "R",
              "read_image_sizes_tiff: width,height = %d,%d",
              sd->width,
              sd->height );

    return sd;

    error:
        return ERROR;
}



  /*
   * Set a SizeData struct with values.
   *   set state saying whether:
   *     has multiple images in file
   *   Careful:
   *     if the images are stored internally,
   *       then reset the use_numerics flag.
   */
SizeData * _dxf_ReadImageSizesTIFF
               ( char     *name,
                 int      startframe,
                 SizeData *data,
                 int      *use_numerics,
                 int      ext_sel,
                 int      *multiples )
{
    TiffHeader hdr;
    uint16     tiff_entity_count;
    char       copyname [ MAX_IMAGE_NAMELEN ];
    SizeData   frame_data;
    int        fh;
    Error      (*rs)(int,uint16*);
    Error      (*rl)(int,uint32*);

    DXASSERTGOTO ( ERROR != name         );
    DXASSERTGOTO ( ERROR != data         );
    DXASSERTGOTO ( ERROR != use_numerics );
    DXASSERTGOTO ( ( ext_sel >= 0 ) && ( ext_sel <= 2 ) );
    DXASSERTGOTO ( ERROR != multiples    );

   /*
    * 
    */

    if ( 0 > ( fh = open ( name, O_RDONLY ) ) ) 
        DXErrorGoto2
           ( ERROR_BAD_PARAMETER, "#12240", /* can not open file '%s' */ name );

    if ( !read_tiff_header ( fh, &hdr, &rs, &rl ) )
        goto error;

    /*
     * We are now relying on the fact that the file pointer
     * is at the next header.
     */
    if ( !read_image_sizes_tiff ( &frame_data, fh, rs, rl ) )
        goto error;

    data->height     = frame_data.height;
    data->width      = frame_data.width;
    data->startframe = ( startframe == VALUE_UNSPECIFIED )? 0 : startframe;
    data->endframe   = data->startframe;
    *multiples       = 0; /* For now. */

    /*
     * Intent: See if there are multiple images in this file.
     */
    if ( !rl ( fh, &hdr.ifd_offset ) )
        goto error;

    while ( hdr.ifd_offset != 0 )
    {
        data->endframe++;

        if ( 0 > lseek ( fh, hdr.ifd_offset, 0 ) )
            DXErrorGoto2
                ( ERROR_DATA_INVALID,
                  "#10911", /* Seeking to offset %d in binary file failed */
                  hdr.ifd_offset );

        if ( !rs ( fh, &tiff_entity_count ) )
            goto error;

        DXDebug ( "R",
                  "_dxf_ReadImageSizesTIFF: tiff_entity_count = %d",
                  tiff_entity_count );

        if ( 0 > lseek ( fh, (tiff_entity_count*TIFF_FIELD_SIZE), 1 ) )
            DXErrorGoto2
                ( ERROR_DATA_INVALID,
                  "#10911", /* Seeking to offset %d in binary file failed */
                  (tiff_entity_count*TIFF_FIELD_SIZE) );

        if ( !rl ( fh, &hdr.ifd_offset ) )
            goto error;
    }

    close ( fh );

    if ( data->startframe != data->endframe )
    {
        *multiples    = 1;
        *use_numerics = 0;

        goto ok;
    }


    /*
     * Intent: See if there are multiple numbered files on disk.
     */
    if ( *use_numerics != 1 )
    {
        *multiples = 0;
        goto ok;
    }

    if ( ERROR == strcpy ( copyname, name ) )
        DXSetError
            ( ERROR_UNEXPECTED,
              "#11800", /* C standard library call, %s, returns error */
              "strcpy()" );

    while ( 0 <= ( fh = open ( copyname, O_RDONLY ) ) )
    {
        close ( fh );

        DXDebug ( "R", "_dxf_ReadImageSizesTIFF: opened: %s", copyname );

        if ( ext_sel == 2 ) 
        {
            if ( !_dxf_RemoveExtension ( copyname ) )
                goto error;
        }
        else
            if ( !_dxf_RemoveExtension ( copyname ) ||
                 !_dxf_RemoveExtension ( copyname )   )
                goto error;

        data->endframe++;

        sprintf
            ( &copyname [ strlen ( copyname ) ],
              (ext_sel==0)? ".%d.tiff" :
              (ext_sel==1)? ".%d.tif"  :
                            ".%d",
              data->endframe );
    }

    data->endframe--;

    ok:
        DXDebug ( "R",
                  "_dxf_ReadImageSizesTIFF: h,w, s,e = %d,%d, %d,%d;  "
                  "numer = %d, multi = %d",
                  data->height,
                  data->width,
                  data->startframe,
                  data->endframe,
                  *use_numerics,
                  *multiples );

        return data;

    error:
        return ERROR;
}

#define SEEK_TO_STRIP							\
{ 									\
    uint16  ss; 							\
    uint32  ll;								\
 									\
    if ( 0 > lseek ( fh, seek_offset, 0 ) ) 				\
	DXErrorGoto2(ERROR_DATA_INVALID, "#10911", seek_offset); 	\
 									\
    switch (file_data.StripOffsets.type) 				\
    { 									\
	case tiff_SHORT: 						\
	    if ( !rs(fh,&ss) ) goto error; 				\
	    seek_offset += 2; 						\
	    ll = ss; 							\
	    break; 							\
 									\
	case tiff_LONG: 						\
	    if ( !rl(fh,&ll) ) goto error; 				\
	    seek_offset += 4; 						\
	    break; 							\
    } 									\
 									\
    if (0 > lseek(fh, ll, 0)) 						\
	DXErrorGoto2(ERROR_DATA_INVALID, "#10911", ll); 		\
 									\
    striplines = file_data.rowsperstrip.val; 				\
} 

/*
 */
Field _dxf_InputTIFF
	(int width, int height, char *name, int relframe, int delayed, char *colortype)
{
    Field	    image = NULL;
    int             seek_offset;
    int             striplines;
    int             i, x, y;
    TiffHeader      hdr;
    TiffField       field;
    uint16          tiff_entity_count;
    Error           (*rs)(int,uint16*);
    Error           (*rl)(int,uint32*);
    tag_table_entry *lookup;

    RGBColor        map[256];
    RGBByteColor    imap[256];
    ubyte	    *buf = NULL;

    int             fh     = -1;
    int             rframe;

    struct
    {
        int       height, width;
        RGBColor  *anchor;
    }
    image_data;

    typedef struct 
    {
        uint32 val;
        int    set;
    }
    setval;

    struct
    {
        setval     height, width, rowsperstrip, samplesperpixel;
        setval     planarconfiguration, photometricinterpretation, compression;
        TiffField  StripOffsets, BitsPerSample;
        TiffField  ColorMap, ColorResponseCurves;
    }
    file_data;

#define  UNSET_TAG  65535

    DXDebug ( "R", "_dxf_InputTIFF: name = %s, relframe = %d", name, relframe );

    file_data.height.set                    = 0;
    file_data.width.set                     = 0;
    file_data.rowsperstrip.set              = 0;
    file_data.samplesperpixel.set           = 0;
    file_data.planarconfiguration.set       = 0;
    file_data.compression.set               = 0;
    file_data.photometricinterpretation.set = 0;

    file_data.StripOffsets.tag              = UNSET_TAG;
    file_data.BitsPerSample.tag             = UNSET_TAG;
    file_data.ColorMap.tag                  = UNSET_TAG;
    file_data.ColorResponseCurves.tag       = UNSET_TAG;


    image_data.width  = width;
    image_data.height = height;
    image_data.anchor = NULL;

    if ((image_data.width * image_data.height) == 0)
        DXErrorGoto2
            ( ERROR_BAD_PARAMETER, "#12275", 0 );

    if ( 0 > ( fh = open ( name, O_RDONLY ) ) ) 
        DXErrorGoto2
            ( ERROR_BAD_PARAMETER,
              "#12240", /* cannot open file '%s' */
              name );

    if ( !read_tiff_header ( fh, &hdr, &rs, &rl ) )
        goto error;

    for ( rframe = 0; rframe <= relframe; rframe++ )
    {
        if ( !rs ( fh, &tiff_entity_count ) )
            goto error;

        DXDebug ( "R",
                  "frame:[%d] tiff entity count = %d",
                  rframe,
                  tiff_entity_count );
        /*
         * Seek past end of entity list and to beginning of next.
         * For each frame except the last. 
         */
        if ( rframe < relframe )
        {
            if ( 0 > lseek ( fh, (tiff_entity_count*TIFF_FIELD_SIZE), 1 ) )
                DXErrorGoto2
                    ( ERROR_DATA_INVALID,
                      "#10911", /* Seeking to offset %d in binary file failed */
                      (tiff_entity_count*TIFF_FIELD_SIZE) );

            if ( !rl ( fh, &hdr.ifd_offset ) )
                goto error;

            if ( hdr.ifd_offset == 0 )
                DXErrorGoto3
                    ( ERROR_INTERNAL,
                      "Ifd_offset of zero at rframe %d of %d",
                      rframe, relframe );

            if ( 0 > lseek ( fh, hdr.ifd_offset, 0 ) )
                DXErrorGoto2
                    ( ERROR_DATA_INVALID,
                      "#10911", /* Seeking to offset %d in binary file failed */
                      hdr.ifd_offset );
        }
    }

    /*
     * Read in all significant field data.
     */
    for ( i=0; i<tiff_entity_count; i++ )
        if ( !read_tiff_field ( fh, &field, rs, rl ) )
            goto error;
        else
        {
            setval tmp_integer = {0, 0};
            int    remote_data = -1;

            /*
             * Pre-screening for fields that we have to look at.
             */
            switch ( field.tag )
            {
                case ImageWidth:
                case ImageLength:
                case BitsPerSample:
                case StripOffsets:
                case SamplesPerPixel:
                case RowsPerStrip:
                case PlanarConfiguration:
                case PhotometricInterpretation:
                case Compression:
                case ColorResponseCurves:
                case ColorMap:

                    if ( ERROR == ( lookup = 
                                    tiff_lookup ( (TiffTag)field.tag ) ) )
                        goto error;

                    if ( ( ( field.type < tiff_BYTE     ) ||
                           ( field.type > tiff_RATIONAL )   )
                         ||
                         ( ( lookup->tiff_type == SHORT_LONG ) &&
                           ( ( field.type != tiff_SHORT ) &&
                             ( field.type != tiff_LONG  )   ) )
                         ||
                         ( ( lookup->tiff_type != SHORT_LONG ) &&
                           ( lookup->tiff_type != field.type )   ) )
                        DXErrorGoto2
                            ( ERROR_DATA_INVALID,
                              "TIFF field `%s' has bad type.",
                              lookup->tag_name );
   
                    if ( ( lookup->size != FUNCTION ) &&
                         ( field.length != lookup->size ) )
                        DXErrorGoto2
                            ( ERROR_DATA_INVALID,
                              "TIFF field `%s' has bad size setting.",
                              lookup->tag_name );

                    if ( field.length != 1 )
                        tmp_integer.set = 0;
                    else
                        switch ( field.type )
                        {
                            case tiff_SHORT:
                                tmp_integer.val
                                    = ((uint16*)&field.value.value)[0];
                                tmp_integer.set = 1;
                                break;

                            case tiff_LONG:
                                tmp_integer.val = field.value.value;
                                tmp_integer.set = 1;
                                break;

                            default:
                                tmp_integer.set = 0;
                        }

                    remote_data = 
                        (4 < ( tiff_type_bytecount[field.type] * field.length));

                    break;

                default:
                    remote_data = -1;
            }

            /*
             * Store the setting.
             */
            switch ( field.tag )
            {
                case ImageWidth:
                    if ( !tmp_integer.set || ( remote_data != 0 ) )
                        DXErrorGoto
                            ( ERROR_UNEXPECTED,
                              "Getting TIFF integer, field `ImageWidth'" );

                    file_data.width = tmp_integer;
                    break;

                case ImageLength:
                    if ( !tmp_integer.set || ( remote_data != 0 ) )
                        DXErrorGoto
                            ( ERROR_UNEXPECTED,
                              "Getting TIFF integer, field `ImageLength'" ); 

                    file_data.height = tmp_integer;
                    break;

                case SamplesPerPixel:
                    if ( !tmp_integer.set || ( remote_data != 0 ) )
                        DXErrorGoto
                            ( ERROR_UNEXPECTED,
                              "Getting TIFF integer, field `SamplesPerPixel'" );

                    file_data.samplesperpixel = tmp_integer;
                    break;

                case RowsPerStrip:
                    if ( !tmp_integer.set || ( remote_data != 0 ) )
                        DXErrorGoto
                            ( ERROR_UNEXPECTED,
                              "Getting TIFF integer, field `RowsPerStrip'" );

                    file_data.rowsperstrip = tmp_integer;
                    break;

                case PhotometricInterpretation:
                    if ( !tmp_integer.set || ( remote_data != 0 ) )
                        DXErrorGoto
                            ( ERROR_UNEXPECTED,
                              "Getting TIFF integer,"
                              " field `PhotometricInterpretation'" );

                    file_data.photometricinterpretation = tmp_integer;
                    break;

                case PlanarConfiguration:
                    if ( !tmp_integer.set || ( remote_data != 0 ) )
                        DXErrorGoto
                            ( ERROR_UNEXPECTED,
                          "Getting TIFF integer, field `PlanarConfiguration'" );

                    file_data.planarconfiguration = tmp_integer;
                    break;

                case Compression:
                    if ( !tmp_integer.set || ( remote_data != 0 ) )
                        DXErrorGoto
                            ( ERROR_UNEXPECTED,
                              "Getting TIFF integer, field `Compression'" );

                    file_data.compression = tmp_integer;
                    break;

                case BitsPerSample:
                    file_data.BitsPerSample = field;
                    break;

                case StripOffsets:
                    file_data.StripOffsets = field;
                    break;

                case ColorResponseCurves:
                    file_data.ColorResponseCurves = field;
                    break;

                case ColorMap:
                    file_data.ColorMap = field;
                    break;

                default:
                    ;
            }
        }


    /*
     * Check what was found, default what can be, Error on what can't.
     * Mandatory settings
     */
    /* 256 ImageWidth */
    if ( !file_data.width.set )
        DXErrorGoto
            ( ERROR_DATA_INVALID,
              "Required TIFF field `ImageWidth' is missing" )
    else
        if ( file_data.width.val != image_data.width )
            DXErrorGoto
                ( ERROR_INTERNAL, "TIFF Image width disagreement" );

    /* 257 ImageLength */
    if ( !file_data.height.set )
        DXErrorGoto
            ( ERROR_DATA_INVALID,
              "Required TIFF field `ImageLength' is missing" )
    else
        if ( file_data.height.val != image_data.height )
            DXErrorGoto
                ( ERROR_INTERNAL, "TIFF Image length disagreement" );
   
    /* 273 StripOffsets */
    if ( file_data.StripOffsets.tag == UNSET_TAG )
        DXErrorGoto
            ( ERROR_DATA_INVALID,
              "Required TIFF field `StripOffsets' is missing" );


    /*
     * Defaultable settings.
     * Since TIFF settings are supposed to be in numeric sorted order, 
     * construct defaulting (using backward cross references) this way.
     * exceptions:
     *     SamplesPerPixel is defined after BitsPerSample (size depends on)
     */

    /* 258 BitsPerSample */
    if ( file_data.BitsPerSample.tag == UNSET_TAG )
    {
        file_data.BitsPerSample.tag         = BitsPerSample;
        file_data.BitsPerSample.value.value = 8; /* XXX 3 */ /* XXX uint16 */
        DXMessage
            ( "Defaulting TIFF `BitsPerSample' to 8,8,8" );
    }
    else
    {
        if ( 1 == file_data.BitsPerSample.length )
        {
            if ( ((uint16*)&file_data.BitsPerSample.value.value)[0] != 8 )
                DXErrorGoto
                    ( ERROR_DATA_INVALID,
                      "TIFF field `BitsPerSample' must be 8" );
        }
        else if ( 3 == file_data.BitsPerSample.length )
        {
            uint16 bps[3];

            if ( 0 > lseek ( fh, file_data.BitsPerSample.value.offset, 0 ) )
                DXErrorGoto2
                    ( ERROR_DATA_INVALID,
                      "#10911", /* Seeking to offset %d in binary file failed */
                      file_data.BitsPerSample.value.offset );

            if ( !rs(fh, &bps[0]) || !rs(fh, &bps[1]) || !rs(fh, &bps[2]) )
                goto error;

            if ( ( bps[0] != 8 ) || ( bps[1] != 8 ) || ( bps[2] != 8 ) )
                DXErrorGoto
                    ( ERROR_DATA_INVALID,
                      "TIFF field `BitsPerSample' must be 8,8,8" );
        }
    }


    /* 259 Compression */
    if ( !file_data.compression.set )
    {
        file_data.compression.val = 1;
        file_data.compression.set = 1;
        DXMessage
            ( "Defaulting TIFF `Compression' to 1 (None)" );
    }
    else if ( file_data.compression.val != 1 )
        DXErrorGoto
            ( ERROR_DATA_INVALID,
              "TIFF field `Compression' must be 1 (None)" );

    /* 262 PhotometricInterpretation */
    if ( !file_data.photometricinterpretation.set )
    {
        file_data.photometricinterpretation.val = 2;
        file_data.photometricinterpretation.set = 1;
        DXMessage
            ( "Defaulting TIFF `PhotometricInterpretation' to 2 (RGB)" );
    }
    else if (
             ( file_data.photometricinterpretation.val != 0 ) &&
             ( file_data.photometricinterpretation.val != 1 ) &&
             ( file_data.photometricinterpretation.val != 2 ) &&
             ( file_data.photometricinterpretation.val != 3 )   )
        DXErrorGoto
            ( ERROR_DATA_INVALID,
              "TIFF field `PhotometricInterpretation'"
              " must be 0 (Min-Is-White) 1 (Min-Is-Black) 2 (RGB) or 3 (Palette)" );

    DXDebug ( "R", "PhotometricInterpretation is %s",
                   (file_data.photometricinterpretation.val == 0)? "Min_Is_White" :
                   (file_data.photometricinterpretation.val == 1)? "Min_Is_Black" :
                   (file_data.photometricinterpretation.val == 2)? "RGB" :
                   (file_data.photometricinterpretation.val == 3)? "Palette" :
                                                                   "Bad!" );


    /* 277 SamplesPerPixel */
    if ( !file_data.samplesperpixel.set )
    {
        if ( file_data.photometricinterpretation.val == 2 )
            file_data.samplesperpixel.val = 3;

        else if ( file_data.photometricinterpretation.val == 0 || 
	    file_data.photometricinterpretation.val == 1 || 
	    file_data.photometricinterpretation.val == 3 )  
            file_data.samplesperpixel.val = 1;

        file_data.samplesperpixel.set = 1;
        DXMessage
            ( "Defaulting TIFF `SamplesPerPixel' to %d",
              file_data.samplesperpixel.val );
    }
    else if ( 
              ( file_data.samplesperpixel.val != 1 ) &&
              ( file_data.samplesperpixel.val != 3 )   )
        DXErrorGoto
            ( ERROR_DATA_INVALID,
              "TIFF field `SamplesPerPixel' must be 1 or 3" );

    if ( file_data.samplesperpixel.val != file_data.BitsPerSample.length )
    {
        DXWarning
            ( "TIFF fields `BitsPerSample'(length) and `SamplesPerPixel'"
              " conflict.  (L%d != %d)", 
              file_data.BitsPerSample.length,
              file_data.samplesperpixel.val );

        DXWarning
            ( "Proceeding, though errors may result..." );
    }

    if ( ( ( file_data.photometricinterpretation.val == 2 ) &&
           ( file_data.samplesperpixel.val           != 3 )   )
         ||
         ( (( file_data.photometricinterpretation.val == 0 ) || 
            ( file_data.photometricinterpretation.val == 1 ) ||
            ( file_data.photometricinterpretation.val == 3 )) &&
           ( file_data.samplesperpixel.val           != 1 )   ) )
    {
        DXWarning
            ( "TIFF fields `PhotometricInterpretation' and `SamplesPerPixel'"
              " conflict.  (pmi=%d, spp=%d)",
              file_data.photometricinterpretation.val,
              file_data.samplesperpixel.val );

        DXWarning
            ( "Proceeding, though errors may result..." );
    }


    /* 278 RowsPerStrip */
    if ( !file_data.rowsperstrip.set )
    {
        file_data.rowsperstrip.val = 2000000000; /* Signed integer infinity */
        file_data.rowsperstrip.set = 1;
    }
    /* When rowsperstrip not arbitrarily large... */
    else if ( ( file_data.rowsperstrip.val
                < ( file_data.height.val * file_data.width.val ) )
              &&
              /* perform number of strips consistency check */
              ( file_data.StripOffsets.length
                < ( ( file_data.height.val + file_data.rowsperstrip.val - 1 )
                     / file_data.rowsperstrip.val ) ) )
    {
        DXWarning
            ( "TIFF fields `StripOffsets(length),ImageLength,RowsPerStrip'"
              " conflict.  (L(so)=%d,il=%d,rps=%d)",
              file_data.StripOffsets.length,
              file_data.height.val,
              file_data.rowsperstrip.val );

        DXWarning
            ( "Proceeding, though errors may result..." );
    }


    /* 284 PlanarConfiguration */
    if ( !file_data.planarconfiguration.set )
    {
        file_data.planarconfiguration.val = 1;
        file_data.planarconfiguration.set = 1;
        DXMessage
          ( "Defaulting TIFF `PlanarConfiguration' to 1 (contiguous pixels)" );
    }
    else if ( file_data.planarconfiguration.val != 1 )
        DXErrorGoto
            ( ERROR_DATA_INVALID,
              "TIFF field `PlanarConfiguration' must be 1 (contiguous)" );


    /* 301 ColorResponseCurves */
    if ( file_data.ColorResponseCurves.tag != UNSET_TAG )
        DXWarning
            ( "TIFF Field `ColorResponseCurves' ignored." );


    /* 320 ColorMap */
    if ( file_data.ColorMap.tag != UNSET_TAG )
    {
        if ( file_data.photometricinterpretation.val == 0  ||
             file_data.photometricinterpretation.val == 1  ||
             file_data.photometricinterpretation.val == 2 )
            DXWarning
                ( "This is not a PaletteColor image."
                  "  TIFF Field `ColorMap' ignored." );
        else
        {
            int    ii;
            uint16 ss;

            if ( file_data.ColorMap.length != 768 )
            {
                DXWarning
                    ( "TIFF field `ColorMap' has bad size setting.  (%d)",
                      file_data.ColorMap.length );

                DXWarning
                    ( "Proceeding, though errors may result..." );
            }

            if ( 0 > lseek ( fh, file_data.ColorMap.value.offset, 0 ) )
                DXErrorGoto2
                    ( ERROR_DATA_INVALID,
                      "#10911", /* Seeking to offset %d in binary file failed */
                      file_data.ColorMap.value.offset );

            /* XXX speed concerns */
            for ( ii=0; ii<256; ii++ )
                if ( rs ( fh, (uint16 *)&ss ) ) map[ii].r = ss / 65535.0;
                else goto error;

            for ( ii=0; ii<256; ii++ )
                if ( rs ( fh, (uint16 *)&ss ) ) map[ii].g = ss / 65535.0;
                else goto error;

            for ( ii=0; ii<256; ii++ )
                if ( rs ( fh, (uint16 *)&ss ) ) map[ii].b = ss / 65535.0;
                else goto error;
        }
    }
    else
    {
        if ( file_data.photometricinterpretation.val == 0 )
        {
            int  ii;

            for ( ii=0; ii<256; ii++ )
                map[ii].r = map[ii].g = map[ii].b = (255-ii) / 256.0;

        }
        else if ( file_data.photometricinterpretation.val == 1 )
        {
            int  ii;

            for ( ii=0; ii<256; ii++ )
                map[ii].r = map[ii].g = map[ii].b = ii / 256.0;

        }
        else if ( file_data.photometricinterpretation.val == 3 )
        {
            int  ii;

            DXWarning
                ( "TIFF field `ColorMap' is not present."
                  "  Installing a simple linear one." );

            for ( ii=0; ii<256; ii++ )
                map[ii].r = map[ii].g = map[ii].b = ii / 256.0;

            DXWarning
                ( "Proceeding, though errors may result..." );
        }
    }


    /*
     * End TIFF internal file parameter analysis.
     */
    if ( file_data.StripOffsets.length == 1 )
    {
        if (0 > lseek(fh, file_data.StripOffsets.value.offset, 0))
            DXErrorGoto2(ERROR_DATA_INVALID, "#10911", 
		      file_data.StripOffsets.value.offset );

        seek_offset = 0;
        striplines  = file_data.rowsperstrip.val;
    }
    else
    {
        seek_offset = file_data.StripOffsets.value.offset;
        striplines  = 0;
    }

    /*
     * Set the image pixels.  Finally!
     */
    if ( ( file_data.samplesperpixel.val           == 3 ) &&
         ( file_data.photometricinterpretation.val == 2 )   )
    {
	Type  type;
	int   rank, shape[32];
	Array colorsArray;
	Pointer pixels;

	image = DXMakeImageFormat(width, height, colortype);
	if (! image)
	    goto error;
	
	colorsArray = (Array)DXGetComponentValue(image, "colors");

	DXGetArrayInfo(colorsArray, NULL, &type, NULL, &rank, shape);

	if (rank != 1 || shape[0] != 3 ||
	    (type != TYPE_UBYTE && type != TYPE_FLOAT))
	{
	    DXSetError(ERROR_INTERNAL,
	       "3-vector float or byte image templates required %s",
	       "to import non-mapped tiff");
	    goto error;
	}

	/*
	 * If the output is type float then we'll be needing an input
	 * buffer in any case
	 */
	if (type == TYPE_FLOAT)
	{
	    buf = DXAllocate(width * file_data.samplesperpixel.val);
	    if (! buf)
	        goto error;
	}

	pixels = DXGetArrayData(colorsArray);

	for (y=(image_data.height-1); y>=0; y--, striplines--)
	{

	    if (striplines == 0)
		SEEK_TO_STRIP;
	    
	    if (type == TYPE_FLOAT)
	    {
	        int n = read(fh, buf, width*file_data.samplesperpixel.val);
		float *fptr = ((float *)pixels) + 3*y*width;
		ubyte *cptr = (ubyte *)buf;

		if (n != width*file_data.samplesperpixel.val)
		{
		    DXSetError(ERROR_INTERNAL,
			    "error reading in a scan-line from tiff file");
		    goto error;
		}
		
		for (x = 0; x < image_data.width*3; x++)
		    *fptr++ = *cptr++ / 255.0;
	    }
	    else
	    {
		ubyte *fptr = ((ubyte *)pixels) + 3*y*width;
		
		if (read(fh, fptr, width*file_data.samplesperpixel.val)
			!= width*file_data.samplesperpixel.val)
		{
		    DXSetError(ERROR_INTERNAL,
			"error reading in a scan-line from tiff file");
		    goto error;
		}
	    }
	}
    }
    else if ( ( file_data.samplesperpixel.val           == 1 ) &&
              ((file_data.photometricinterpretation.val == 0 ) ||
	       (file_data.photometricinterpretation.val == 1 ) ||
	       (file_data.photometricinterpretation.val == 3 )) )
    {
	Type  type;
	int   rank, shape[32];
	Array colorsArray = NULL;
	Array colorMap = NULL;
	Pointer pixels;

	if (delayed == DELAYED_YES)
	    colortype = COLORTYPE_DELAYED;
	image = DXMakeImageFormat(width, height, colortype);
	if (! image)
	    goto error;
	
	colorsArray = (Array)DXGetComponentValue(image, "colors");

	DXGetArrayInfo(colorsArray, NULL, &type, NULL, &rank, shape);

	pixels = DXGetArrayData(colorsArray);

	/*
	 * we'll be needing an input buffer 
	 */
	buf = DXAllocate(width * file_data.samplesperpixel.val);
	if (! buf)
	    goto error;

	if (rank == 0 || (rank == 1 && shape[0] == 1))
	{
	    Type mType;
	    int  mRank, mShape[32], mLen;

	    colorMap = (Array)DXGetComponentValue(image, "color map");
	    if (! colorMap)
	    {
		DXSetError(ERROR_INTERNAL, 
		    "single-valued image requires a color map");
		goto error;
	    }

	    DXGetArrayInfo(colorMap, &mLen, &mType, NULL, &mRank, mShape);

	    if (mLen != 256 || mType != TYPE_FLOAT ||
		mRank != 1 || mShape[0] != 3)
	    {
		DXSetError(ERROR_INTERNAL, 
		    "DXMakeImage returned invalid delayed-color image");
		goto error;
	    }

	    memcpy(DXGetArrayData(colorMap), map, 256*3*sizeof(float));

	    for (y=(image_data.height-1); y>=0; y--, striplines--)
	    {
		ubyte *fptr = ((ubyte *)pixels) + y*width;

		if (striplines == 0)
		    SEEK_TO_STRIP;
		
		if (read(fh, fptr, width) != width)
		{
		    DXSetError(ERROR_INTERNAL,
			"error reading in a scan-line from tiff file");
		    goto error;
		}
	    }
	}
	else if (rank == 1 && shape[0] == 3)
	{
	    if (type == TYPE_UBYTE)
	    {
		int i;
		for (i = 0; i < 256; i++)
		{
		    imap[i].r = 255*map[i].r;
		    imap[i].g = 255*map[i].g;
		    imap[i].b = 255*map[i].b;
		}
	    }
		    
	    for (y=(image_data.height-1); y>=0; y--, striplines--)
	    {
		if (striplines == 0)
		    SEEK_TO_STRIP;
	    
		if (read(fh, buf, width*file_data.samplesperpixel.val)
			    != width*file_data.samplesperpixel.val)
		{
		    DXSetError(ERROR_INTERNAL,
			    "error reading in a scan-line from tiff file");
		    goto error;
		}

		if (type == TYPE_FLOAT)
		{
		    float *fptr = ((float *)pixels) + 3*y*width;
		    ubyte *cptr = (ubyte *)buf;

		    for (x = 0; x < image_data.width; x++)
		    {
			*fptr++ = map[*cptr].r;
			*fptr++ = map[*cptr].g;
			*fptr++ = map[*cptr].b;
			cptr ++;
		    }
		}
		else if (type == TYPE_UBYTE)
		{
		    ubyte *fptr = ((ubyte *)pixels) + 3*y*width;
		    ubyte *cptr = (ubyte *)buf;

		    for (x = 0; x < image_data.width; x++)
		    {
			*fptr++ = imap[*cptr].r;
			*fptr++ = imap[*cptr].g;
			*fptr++ = imap[*cptr].b;
			cptr ++;
		    }
		}
	    }
	}
    }
    else
        DXErrorGoto ( ERROR_INTERNAL, "TIFF incompatible modes" );

    close ( fh );

    DXFree((Pointer)buf);
    return image;

error:
    if ( fh >= 0 ) close ( fh );
    DXFree((Pointer)buf);
    return ERROR;
}
