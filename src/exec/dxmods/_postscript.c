/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dxmods/_postscript.c,v 1.9 2003/07/11 05:50:34 davidt Exp $
 */

#include <dxconfig.h>


/*
 * This file supports writing images to disk files in PostScript 3.0 format.
 * We also support Encapsulated PostScript format 3.0.
 * Images can be written out in either 24-bit color or 8-bit gray.
 */

#include <stdio.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <ctype.h>
#include <dx/dx.h>
#include "_helper_jea.h"
#include "_rw_image.h"
#include <sys/types.h>
#include <math.h>

struct ps_spec {
        int     filetype;       /* PS or EPSF */
        int     colormode;      /* FULLCOLOR or GRAY */
        int     width_dpi;      /* dots per inch in image height */
        int     height_dpi;     /* dots per inch in image width */
        int     image_width;    /* width of image in pixels */
        int     image_height;   /* height of image in pixels */
        float   page_width;     /* width of page in inches */
        float   page_height;    /* height of page in inches */
	float	page_margin;	/* margin width */
        int     page_orient;    /* LANDSCAPE or PORTRAIT */
        int     compress;       /* Do postscript compression */
	int	data_size;      /* For miff, size of data block */
	long	fptr;		/* File pointer for data size text */
	float   gamma;          /* gamma */
};
/*
 * This taken from 1st Edition Foley and Van Dam, page 613
 */
#define RGB_TO_BW(r,g,b)        (.30*(r) + .59*(g) + .11*(b))

#define LINEOUT(s) if (fprintf(fout, s "\n") <= 0) goto bad_write;

#define UPI             72      /* Default units per inch */
#define PS              0       /* Regular PostScript */
#define EPS             1       /* Encapsulated PostScript */

#define RLE_BINARY      1
#define RLE_ASCII       0
#define RLE_COUNT_PRE   1
#define RLE_COUNT_POST  0
#define RLE_STUPID      1
#define RLE_COMPACT     0

#define FULLCOLOR       0
#define GRAY            1

#define FLOAT_COLOR     1
#define UBYTE_COLOR     2
#define MAPPED_COLOR    3

#define NSCENES_STRING "nscenes="
#define DATASIZE_STRING "data_size="

#define BUFFSIZE 256

static Error parse_format(char *format, struct ps_spec *spec);
static Error output_ps(RWImageArgs *iargs, int colormode,int filetype);
static Error put_ps_header(FILE *fout, char *filename,
                                int frames, struct ps_spec *spec);
static Error build_rle_row(ubyte *r, int n_pixels, int bytes_per_pixel,
                        int compress, ubyte *buff, int *byte_count, int binary, int order, int compact);
static Error put_ps_prolog(FILE *fout, struct ps_spec *spec);
static Error put_ps_setup(FILE *fout, int frames, struct ps_spec *spec) ;
static Error put_colorimage_function(FILE *fout);
static Error ps_new_page(FILE *fout, int page, int pages, struct ps_spec *spec);
static Error put_ps_page_setup(FILE *fout, struct ps_spec *spec);
static Error ps_image_header(FILE *fout, struct ps_spec *spec);
static Error ps_out_flc(FILE *fout,
        Pointer pixels, RGBColor *map, int type, struct ps_spec *spec);
static Error ps_out_gry(FILE *fout,
        Pointer pixels, RGBColor *map, int type, struct ps_spec *spec);
static Error ps_end_page(FILE *fout);
static Error put_ps_trailer(FILE *fout, struct ps_spec *spec);
static Error output_miff(RWImageArgs *iargs);
static Error put_miff_header(FILE *fout, char *filename,
                                int frame, struct ps_spec *spec, int type, int nframes);
static Error read_miff_header(FILE *in, int *frame, struct ps_spec *spec, int *type, int *nframes);
static Error miff_out_flc(FILE *fout,
        Pointer pixels, RGBColor *map, int type, struct ps_spec *spec);

#define ORIENT_NOT_SET  0
#define LANDSCAPE       1
#define PORTRAIT        2
#define ORIENT_AUTO	3
static void orient_page(struct ps_spec *spec);

/*
 * Jump table target, that writes out color PostScript file.
 */
Error
_dxf_write_ps_color(RWImageArgs *iargs)
{
        Error e;
        e = output_ps(iargs,FULLCOLOR,PS);
        return e;
}
/*
 * Jump table target, that writes out gray level PostScript file.
 */
Error
_dxf_write_ps_gray(RWImageArgs *iargs)
{
        Error e;
        e = output_ps(iargs,GRAY,PS);
        return e;
}
/*
 * Jump table target, that writes out color Encapsulated PostScript file.
 */
Error
_dxf_write_eps_color(RWImageArgs *iargs)
{
        Error e;
        e = output_ps(iargs,FULLCOLOR,EPS);
        return e;
}
/*
 * Jump table target, that writes out an gray level Encapsulated PostScript
 * file.
 */
Error
_dxf_write_eps_gray(RWImageArgs *iargs)
{
        Error e;
        e = output_ps(iargs,GRAY,EPS);
        return e;
}

/*
 * Write out a "ps" (PostScript), formatted image file from the specified
 * field.  The input field should not be composite, but may be series.
 * By the time we get called, it has been asserted that we can write
 * the image to the device, be it ADASD or otherwise.
 *
 * NOTE: The semantics here differ from those for RGB images in that
 * the frame number is ignored, a new file is always created and if
 * given a series all images in the series are written out.
 */
static Error
output_ps(RWImageArgs *iargs, int colormode,int filetype)
{
    char     imagefilename[MAX_IMAGE_NAMELEN];
    int      firstframe, lastframe, i, series;
    int      frames, deleteable = 0;
    struct   ps_spec page_spec;
    Pointer  pixels;
    FILE     *fp = NULL;
    Array    colors, color_map;
    int      imageType=0;
    RGBColor *map = NULL;
    Type     type;
    int      rank, shape[32];

    SizeData  image_sizes;  /* Values as found in the image object itself */
    Field       img = NULL;

    if (iargs->adasd)
        DXErrorGoto(ERROR_NOT_IMPLEMENTED,
                "PostScript format not supported on disk array");

    series = iargs->imgclass == CLASS_SERIES;

    if (series && (filetype == EPS))
        DXErrorGoto(ERROR_DATA_INVALID,
          "Series data cannot be written in 'Encapsulated PostScript' format");

    if ( !_dxf_GetImageAttributes ( (Object)iargs->image, &image_sizes ) )
        goto error;

    /*
     * Always write all frames of a series.
     */
    frames = (image_sizes.endframe - image_sizes.startframe + 1);
    firstframe = 0;
    lastframe = frames-1;


    page_spec.width_dpi = 0;
    page_spec.height_dpi = 0;
    page_spec.page_height = 11.0;
    page_spec.page_width = 8.5;
    page_spec.page_orient = ORIENT_AUTO;
    page_spec.image_height = image_sizes.height;
    page_spec.image_width = image_sizes.width;
    page_spec.page_margin = 0.5;
    page_spec.gamma = 2.0;
    if (iargs->format) {
        if (!parse_format(iargs->format,&page_spec))
            goto error;
    }

   /*
    * Get some nice statistics
    */
    page_spec.colormode = colormode;
    page_spec.filetype = filetype;
    orient_page(&page_spec);

    /*
     * Attempt to open image file and position to start of frame(s).
     */
    if (iargs->pipe == NULL) {
        /*
         * Create an appropriate file name.
         */
        if (!_dxf_BuildImageFileName(imagefilename,MAX_IMAGE_NAMELEN,
                        iargs->basename, iargs->imgtyp,iargs->startframe,0))
                goto error;
        if ( (fp = fopen (imagefilename, "w" )) == 0 )
            ErrorGotoPlus1 ( ERROR_DATA_INVALID,
                          "Can't open image file (%s)", imagefilename );
    } else {
        strcpy(imagefilename,"stdout");
        fp = iargs->pipe;
    }

    /*
     * Write out the PostScript header, prolog and setup sections.
     */
    if (!put_ps_header(fp, imagefilename, frames, &page_spec) ||
        !put_ps_prolog(fp, &page_spec) ||
        !put_ps_setup(fp, frames, &page_spec))
        goto bad_write;


    img = iargs->image;
    for (i=firstframe ; i<=lastframe ; i++)
    {
        if (series) {
            if (deleteable) DXDelete((Object)img);
            img = _dxf_GetFlatSeriesImage((Series)iargs->image,i,
                        page_spec.image_width,page_spec.image_height,
                        &deleteable);
            if (!img)
                goto error;
        }

        colors = (Array)DXGetComponentValue(img, "colors");
        if (! colors)
        {
            DXSetError(ERROR_DATA_INVALID,
                "image does not contain colors component");
            goto error;
        }

        DXGetArrayInfo(colors, NULL, &type, NULL, &rank, shape);

        if (type == TYPE_FLOAT)
        {
            if (rank != 1 || shape[0] != 3)
            {
                DXSetError(ERROR_DATA_INVALID,
                    "floating-point colors component must be 3-vector");
                goto error;
            }

            imageType = FLOAT_COLOR;
        }
        else if (type == TYPE_UBYTE)
        {
            if (rank == 0 || (rank == 1 && shape[0] == 1))
            {
                color_map = (Array)DXGetComponentValue(img, "color map");
                if (! color_map)
                {
                    DXSetError(ERROR_DATA_INVALID,
                        "single-valued ubyte colors component requires a %s",
                        "color map");
                    goto error;
                }

                DXGetArrayInfo(color_map, NULL, &type, NULL, &rank, shape);

                if (type != TYPE_FLOAT || rank != 1 || shape[0] != 3)
                {
                    DXSetError(ERROR_DATA_INVALID,
                        "color map must be floating point 3-vectors");
                    goto error;
                }

                map = (RGBColor *)DXGetArrayData(color_map);
                imageType = MAPPED_COLOR;
            }
            else if (rank != 1 || shape[0] != 3)
            {
                DXSetError(ERROR_DATA_INVALID,
                    "unsigned byte colors component must be either single %s",
                    "valued with a color map or 3-vectors");
                goto error;
            }
            else
                imageType = UBYTE_COLOR;;
        }

        if (!(pixels = DXGetArrayData(colors)))
            goto error;

        /* Put out what ever is necessary for a new page */
        if (!ps_new_page(fp, i+1,frames, &page_spec))
           goto bad_write;

        /*
         * Write out the pixel data
         */
        if (page_spec.colormode == FULLCOLOR) {
            if (!ps_out_flc(fp, pixels, map, imageType, &page_spec))
                goto bad_write;
        }
        else if (!ps_out_gry(fp, pixels, map, imageType,  &page_spec))
            goto bad_write;

        /* Put out what ever is necessary to close a page */
        if (!ps_end_page(fp))
           goto bad_write;
    }

    /*
     * Write out anything that is needed to end the file.  This may work
     * as a compliment to some of what is done in pu_ps_header(),
     * put_ps_prolog() and/or put_ps_setup().
     */
    if (!put_ps_trailer(fp,&page_spec))
        goto bad_write;

    if (fclose(fp) != 0)
        goto bad_write;

    if (deleteable && img) DXDelete((Object)img);

    return OK;

bad_write:
    if ( iargs->pipe == NULL )
        DXErrorGoto
            ( ERROR_DATA_INVALID,
              "Can't write PostScript file." )
    else
        DXErrorGoto
            ( ERROR_BAD_PARAMETER,
              "Can't send image with specified command." );

error:
    if ( fp ) fclose(fp);
    if (deleteable && img) DXDelete((Object)img);
    return ERROR;
}

/*
 * Put out anything that is considered document 'setup' (i.e. created
 * a global dictionary, scale/rotate/translate the origin, define global
 * variables...).
 *
 */
static Error
put_ps_setup(FILE *fout, int frames, struct ps_spec *spec)
{
    int bytesperpix;

    bytesperpix = (spec->colormode == FULLCOLOR) ? 3 : 1;

    if (fprintf(fout,"%%%%BeginSetup\n\n") < 0)
        goto bad_write;

    LINEOUT("DXDict begin"              );
    LINEOUT(""                          );
    LINEOUT("%% inch scaling"           );

    if (fprintf(fout,"/inch {%d mul} bind def\n\n", UPI) < 0)
        goto bad_write;

    if (fprintf(fout,"/bytesperpix %d def\n", bytesperpix) < 0)
        goto bad_write;

    if (fprintf(fout,"%%%%EndSetup\n\n") < 0)
        goto bad_write;

   return OK;

bad_write:
    DXErrorGoto(ERROR_DATA_INVALID, "Can't write PostScript file.")

error:
   return ERROR;
}
/*
 * Write out PS code to set up the page for an image of given width and height.
 */
static Error
put_ps_page_setup(FILE *fout, struct ps_spec *spec)
{
    if (fprintf(fout,"%%%%BeginPageSetup\n\n") < 0)
        goto error;


    if (fprintf(fout,"/pixperrow %d store\n",spec->image_width) < 0)
        goto error;
    if (fprintf(fout,"/nrows %d store\n",spec->image_height) < 0)
        goto error;

    LINEOUT("/outrow pixperrow bytesperpix mul string def");
    LINEOUT("/grayrow pixperrow string def"               );

    if ((fprintf(fout,"%% Number of pixels/inch in image width\n/wppi %d def\n",
                                                spec->width_dpi) <= 0) ||
       (fprintf(fout,"%% Number of pixels/inch in image height\n/hppi %d def\n",
                                                spec->height_dpi) <= 0) ||
        (fprintf(fout,"%% Size of output in inches\n/width %f def\n",
                                spec->page_width ) <= 0)                ||
        (fprintf(fout,"/height %f def\n", spec->page_height) <= 0)      ||
        (fprintf(fout,"%% Size of image in units\n"
                      "/iwidth  1 inch wppi div %d mul def\n",
                                                spec->image_width) <= 0)  ||
        (fprintf(fout,"/iheight 1 inch hppi div %d mul def\n",
                                                spec->image_height) <= 0))
        goto error;

    if (spec->page_orient == LANDSCAPE) {
        if ((fprintf(fout,"%% Rotate and translate into Landscape mode\n")
                                                        <= 0) ||
            (fprintf(fout,"90 rotate\n0 width neg inch cvi translate\n\n")
                                                        <= 0))
            goto error;
    }

    if ((fprintf(fout,"%% Locate lower left corner of image(s)\n") < 0))
        goto error;
    if (spec->page_orient == LANDSCAPE) {
        if (fprintf(fout, "height inch iwidth  sub 2 div cvi\n"
                          "width  inch iheight sub 2 div cvi\n"
                          "translate\n\n") <= 0)
            goto error;
    } else {
        if (fprintf(fout, "width  inch iwidth  sub 2 div cvi\n"
                          "height inch iheight sub 2 div cvi\n"
                          "translate\n\n") <= 0)
            goto error;
    }

    if ((fprintf(fout,"%% Image size units \niwidth iheight scale\n\n") <= 0) ||
        (fprintf(fout,"%%%%EndPageSetup\n\n") <= 0))
        goto error;


    return OK;
bad_write:
    DXErrorReturn(ERROR_DATA_INVALID, "Can't write PostScript file.");
error:
    DXErrorReturn(ERROR_DATA_INVALID, "Can't write PostScript file.");
}

/*
 * If doing full color images, put out code to define the 'colorimage'
 * function if it is not available.
 *
 */
static Error
put_ps_prolog(FILE *fout, struct ps_spec *spec)
{
    int bytesperpix=1;

        if (fprintf(fout,"%%%%BeginProlog\n") <= 0) goto error;

        if (fprintf(fout,"%%%%BeginResource: procset DXProcs 1.0 0\n") <= 0)
            goto error;

        if (fprintf(fout,"/origstate save def\n") <= 0) goto error;
        if (fprintf(fout,"/DXDict 25 dict def\n") <= 0) goto error;
        if (fprintf(fout,"DXDict begin\n\n") <= 0) goto error;

        if (spec->colormode == FULLCOLOR) {
                if (!put_colorimage_function(fout))            goto error;
                bytesperpix = 3;
        }

        LINEOUT("/buffer 1 string def"                             );
        LINEOUT("/count 0 def"                                     );

        if (fprintf(fout,"/rgbval %d string def\n\n", bytesperpix) <= 0)
                goto error;

        LINEOUT("/rlerow {"                                        );
        LINEOUT("  /count 0 store"                                 );
        LINEOUT("  {"                                              );

        if (bytesperpix == 1)
        {
            if (fprintf(fout,"    count   pixperrow   ge\n") <= 0)
                goto error;
        }
        else
        {
            if (fprintf(fout,"    count   pixperrow %d mul   ge\n", bytesperpix) <= 0)
                goto error;
        }

        LINEOUT("      {exit} if   %% end of row"                  );
        LINEOUT("    currentfile buffer readhexstring   pop"       );
        LINEOUT("    /bcount exch 0 get store"                     );
        LINEOUT("    bcount 128 ge"                                );
        LINEOUT("    {"                                            );
        LINEOUT("      %% not a run block"                         );
        LINEOUT("      bcount 128 sub {"                           );
        LINEOUT("        currentfile rgbval readhexstring pop pop" );
        LINEOUT("        outrow count rgbval putinterval"          );

        if (fprintf(fout,"        /count count %d add store\n", bytesperpix) <= 0)
                goto error;

        LINEOUT("      } repeat"                                   );
        LINEOUT("    }"                                            );
        LINEOUT("    {"                                            );
        LINEOUT("      %% run block"                               );
        LINEOUT("      currentfile rgbval readhexstring pop pop"   );
        LINEOUT("      bcount {"                                   );
        LINEOUT("        outrow count rgbval putinterval"          );

        if (fprintf(fout,"        /count count %d add store\n", bytesperpix) <= 0)
                goto error;

        LINEOUT("      } repeat"                                   );
        LINEOUT("    } ifelse"                                     );
        LINEOUT("  } loop  %% until end of row"                    );
        LINEOUT("  outrow"                                         );
        LINEOUT("} bind def"                                       );
        LINEOUT("end  %% DXDict"                                   );
        LINEOUT(""                                                 );

        if (fprintf(fout,"%%%%EndResource\n") <= 0) goto error;

        if (fprintf(fout,"%%%%EndProlog\n\n") <= 0) goto error;
        return OK;
bad_write:
    DXErrorReturn(ERROR_DATA_INVALID, "Can't write PostScript file.");
error:
        return ERROR;
}


/*
 * Output everything in a PostScript file up to and including the
 * '%%EndComments' structuring comment.  When doing Encapsulated
 * PostScript, this must include the '%%BoundingBox:...' comment.
 *
 * Always returns OK.
 */
static Error
put_ps_header(FILE *fout, char *filename, int frames, struct ps_spec *spec)
{
   const time_t t = time(0);
   char *tod = ctime((const time_t *)&t);
   int major, minor, micro;

   if (spec->filetype == PS) {
      if (fprintf(fout, "%s", "%!PS-Adobe-3.0\n") <= 0) goto error;
   } else {     /* Encapsulated PostScript */
      /* Required for EPSF-3.0  */
      if (fprintf(fout, "%s", "%!PS-Adobe-3.0 EPSF-3.0\n") <= 0) goto error;
   }

   DXVersion(&major, &minor, &micro);
   if (fprintf(fout, "%%%%Creator: IBM/Data Explorer %d.%d.%d\n",
                                        major,minor,micro) <= 0) goto error;
   if (fprintf(fout, "%%%%CreationDate: %s", tod) <= 0) goto error;
   if (fprintf(fout, "%%%%Title:  %s\n",  filename) <= 0) goto error;
   if (fprintf(fout, "%%%%Pages: %d\n", frames) <= 0) goto error;

   if (spec->filetype == EPS) {
      /* Put a Bounding Box specification, required for EPSF-3.0  */
      int beginx, beginy, endx, endy;

      float wsize = UPI*spec->image_width / spec->width_dpi;
      float hsize = UPI*spec->image_height / spec->height_dpi;

      if (spec->page_orient == LANDSCAPE) {
          beginx = (spec->page_width  * UPI - hsize)/2 - .5;
          beginy = (spec->page_height * UPI - wsize)/2 - .5;
          endx = beginx + hsize + 1.0;
          endy = beginy + wsize + 1.0;
      } else {
          beginx = (spec->page_width  * UPI - wsize)/2 - .5;
          beginy = (spec->page_height * UPI - hsize)/2 - .5;
          endx = beginx + wsize + 1.0;
          endy = beginy + hsize + 1.0;
      }
      if (fprintf(fout, "%%%%BoundingBox: %d %d %d %d\n",
                                beginx,beginy,endx,endy) <= 0)
        goto error;
   }

   if (fprintf(fout, "%%%%EndComments\n\n") <= 0) goto error;

   return OK;

error:
    DXErrorReturn(ERROR_DATA_INVALID, "Can't write PostScript file.");
}


/********  RLE encoding routines  ******************************************/
/********  Modified from buffer.c ******************************************/

#define DO_COMPRESSION 1

#define CHARS_PER_LINE 80

#define HEX "0123456789abcdef"
#define HEX1(b) (HEX[(b)/16])
#define HEX2(b) (HEX[(b)%16])

#define ADD_HEX(s, b, nn, binary)         \
    if (binary == RLE_ASCII) {            \
	*s++ = HEX1(b);                   \
	*s++ = HEX2(b);                   \
	nn += 2;                          \
	if (!((nn+1)%(CHARS_PER_LINE+1))) \
	    { *s++ = '\n'; nn += 1; }     \
    } else {                              \
	*s++ = b;                         \
	nn++;                             \
    }

/***************************************************************************/
#define CMP(A,B)       (((char *)(A))[0] == ((char *)(B))[0] && \
                        ((char *)(A))[1] == ((char *)(B))[1] && \
                        ((char *)(A))[2] == ((char *)(B))[2]) 
#define ASGN(A,B)      (((char *)(A))[0] =  ((char *)(B))[0], \
                        ((char *)(A))[1] =  ((char *)(B))[1], \
                        ((char *)(A))[2] =  ((char *)(B))[2], \
                        ((char *)(A))[3] =  ((char *)(B))[3])
#define PIXSIZE         3
#define ENCODE_NAME     encode_24bit_row
/***************************************************************************/
static Error ENCODE_NAME(ubyte *pix_row, int n_pix,
                       ubyte *enc_row, int *n_out_bytes, int binary, int order, int compact)
{
    ubyte  *start_p, *p;
    int    start_i,  i, knt, size, k;
    int    out_length;
    int    x, y;
    int    image_size;
    int    totknt;
    ubyte  *segptr;
    int    max_knt = 127;

    x = n_pix;
    y = 1;
    image_size = x*y;
    segptr = enc_row;
    start_p = pix_row;
    start_i = 0;
    totknt = 0;

    if (compact == RLE_STUPID)
	max_knt = 256;

    while (start_i < image_size)
    {
        /*
         * Look for run of identical pixels
         */
        i = start_i + 1;
        p = start_p + PIXSIZE;

        while (i < image_size)
        {
            if (! CMP(start_p, p))
                break;

            i++;
            p += PIXSIZE;
        }

        knt = i - start_i;

        /*
         * if knt == 1 then two adjacent pixels differ.  so instead we
         * look for a run of differing pixels, broken when three
         * identical pixels are found.
         */
        if (knt == 1)
        {
            /*
             * terminate this loop before the comparison goes off
             * the end of the image.
             */
            while ((i < image_size - 2) &&
		    (!CMP(p+0, p+PIXSIZE) || !CMP(p+0, p+2*PIXSIZE)))
                i++, p += PIXSIZE;

            /*
             * Don't bother with rle for last couple of pixels
             */
            if (i == image_size-2)
                i = image_size;

            knt = i - start_i;

            /*
             * loop parcelling out total run in as many packets
             * as are necessary
             */
            while (knt > 0)
            {
                /*
                 * Limit run length to packet size
                 */
		int j;
                int length = (knt > max_knt) ? max_knt : knt;
                int size = length * PIXSIZE;

                /*
                 * decrement total run length by packet run length
                 */
                knt -= length;

		if (compact == RLE_COMPACT)
		{
		    if (order == RLE_COUNT_PRE) {
			ADD_HEX(segptr,(0x80 | length), (*n_out_bytes), binary);
		    }
		    for (k=0; k<size; k++)
		    {
			ADD_HEX(segptr, (*start_p), (*n_out_bytes), binary);
			start_p++;
		    }
		    if (order == RLE_COUNT_POST) {
			ADD_HEX(segptr,(0x80 | length), (*n_out_bytes), binary);
		    }
		} else for (j=0; j<length; j++) {
		    if (order == RLE_COUNT_PRE) {
			ADD_HEX(segptr, 0, (*n_out_bytes), binary);
		    }
		    for (k=0; k<PIXSIZE; k++) {
			ADD_HEX(segptr, (*start_p), (*n_out_bytes), binary);
			start_p++;
		    }
		    if (order == RLE_COUNT_POST) {
			ADD_HEX(segptr, 0, (*n_out_bytes), binary);
		    }
		}
            }
        }
        else
        {

            while (knt > 0)
            {
                /*
                 * Limit run length to packet size
                 */
                int length = (knt > max_knt) ? max_knt : knt;
                size = PIXSIZE;

                /*
                 * decrement total run length by packet run length
                 */
                knt -= length;

		out_length = length;
		if (compact == RLE_STUPID) {
		    out_length--;
		}

		if (order == RLE_COUNT_PRE) {
		    ADD_HEX(segptr, out_length, (*n_out_bytes), binary);
		}
		for (k=0; k<size; k++)
		{
		    ADD_HEX(segptr, (*start_p), (*n_out_bytes), binary);
		    start_p++;
		}
		if (order == RLE_COUNT_POST) {
		    ADD_HEX(segptr, out_length, (*n_out_bytes), binary);
		}

            }
        }

        start_i = i;
        start_p = p;
    }

    DXDebug("Y", "compaction: %d bytes in %d bytes out",
                                image_size*PIXSIZE, totknt);

    *segptr = '\0';
    return totknt;

}

/***************************************************************************/
#undef CMP
#undef ASGN
#undef PIXSIZE
#undef ENCODE_NAME

#define CMP(A,B)        (*((ubyte *)(A)) == *((ubyte *)(B)))
#define ASGN(A,B)       (*((ubyte *)(A)) = *((ubyte *)(B)))
#define PIXSIZE         1
#define ENCODE_NAME     encode_8bit_row
/***************************************************************************/
static Error ENCODE_NAME(ubyte *pix_row, int n_pix,
                       ubyte *enc_row, int *n_out_bytes, int binary, int order, int compact)
{
    ubyte *start_p, *p;
    int    start_i,  i, knt, size, k;
    int    out_length;
    int    x, y;
    int    image_size;
    int    totknt;
    ubyte  *segptr;
    int    max_knt = 127;

    x = n_pix;
    y = 1;
    image_size = x*y;
    segptr = enc_row;
    start_p = pix_row;
    start_i = 0;
    totknt = 0;

    if (compact == RLE_STUPID)
	max_knt = 256;

    while (start_i < image_size)
    {
        /*
         * Look for run of identical pixels
         */
        i = start_i + 1;
        p = start_p + PIXSIZE;

        while (i < image_size)
        {
            if (! CMP(start_p, p))
                break;

            i++;
            p += PIXSIZE;
        }

        knt = i - start_i;

        /*
         * if knt == 1 then two adjacent pixels differ.  so instead we
         * look for a run of differing pixels, broken when three
         * identical pixels are found.
         */
        if (knt == 1)
        {
            /*
             * terminate this loop before the comparison goes off
             * the end of the image.
             */
            while ((!CMP(p+0, p+PIXSIZE) ||
                    !CMP(p+0, p+2*PIXSIZE)) && i < image_size - 2)
                i++, p += PIXSIZE;

            /*
             * Don't bother with rle for last couple of pixels
             */
            if (i == image_size-2)
                i = image_size;

            knt = i - start_i;

            /*
             * loop parcelling out total run in as many packets
             * as are necessary
             */
            while (knt > 0)
            {
                /*
                 * Limit run length to packet size
                 */
                int length = (knt > max_knt) ? max_knt : knt;
                int size = length * PIXSIZE;

                /*
                 * decrement total run length by packet run length
                 */
                knt -= length;

		if (compact == RLE_COMPACT)
		{
		    if (order == RLE_COUNT_PRE) {
			ADD_HEX(segptr,(0x80 | length), (*n_out_bytes), binary);
		    }
		    for (k=0; k<size; k++)
		    {
			ADD_HEX(segptr, (*start_p), (*n_out_bytes), binary);
			start_p++;
		    }
		    if (order == RLE_COUNT_POST) {
			ADD_HEX(segptr,(0x80 | length), (*n_out_bytes), binary);
		    }
		} else {
		    for (k=0; k<length; k++)
		    {
			if (order == RLE_COUNT_PRE) {
			    ADD_HEX(segptr, 0, (*n_out_bytes), binary);
			}
			ADD_HEX(segptr, (*start_p), (*n_out_bytes), binary);
			start_p++;
			if (order == RLE_COUNT_POST) {
			    ADD_HEX(segptr, 0, (*n_out_bytes), binary);
			}
		    }
		}
            }
        }
        else
        {

            while (knt > 0)
            {
                /*
                 * Limit run length to packet size
                 */
                int length = (knt > max_knt) ? max_knt : knt;
                size = PIXSIZE;

                /*
                 * decrement total run length by packet run length
                 */
                knt -= length;

		out_length = length;
		if (compact == RLE_STUPID) {
		    out_length--;
		}

		if (order == RLE_COUNT_PRE) {
		    ADD_HEX(segptr, out_length, (*n_out_bytes), binary);
		}
                for (k=0; k<size; k++)
                {
                    ADD_HEX(segptr, (*start_p), (*n_out_bytes), binary);
                    start_p++;
                }
		if (order == RLE_COUNT_POST) {
		    ADD_HEX(segptr, out_length, (*n_out_bytes), binary);
		}

            }
        }

        start_i = i;
        start_p = p;
    }

    DXDebug("Y", "compaction: %d bytes in %d bytes out",
                                image_size*PIXSIZE, totknt);


    *segptr = '\0';
    return totknt;

}
/***************************************************************************/
#undef CMP
#undef ASGN
#undef PIXSIZE
#undef ENCODE_NAME
/***************************************************************************/


static Error build_rle_row(ubyte *r, int n_pixels, int bytes_per_pixel,
                        int compress, ubyte *buff, int *byte_count, int binary, int order, int compact)
{
    int i;

    if (compress) {
        if (bytes_per_pixel==1)
            encode_8bit_row(r, n_pixels, buff, byte_count, binary, order, compact);
          else
            encode_24bit_row(r, n_pixels, buff, byte_count, binary, order, compact);
    }
    else
        for (i=0; i<n_pixels*bytes_per_pixel; i++) {
            ADD_HEX(buff, (*r), (*byte_count), RLE_ASCII);
            r++;
        }
    return OK;
}


#define CLAMP(p) ( ( (P=(p)) < 0 ) ? 0 : (P<=255) ? P : 255 )

/*
 * This routine writes out the full color image data to the PostScript
 * file.  Each line of the image is written out in R, G and B
 * to records of length 80.
 *
 * Note, that PostScript expects its pixels from left to right and
 * top to bottom, just the way DX keeps them stored.
 *
 * Returns OK/ERROR on failure/success.
 */
static Error
ps_out_flc(FILE *fout, Pointer pixels,
        RGBColor *map, int imageType, struct ps_spec *spec)
{
   int i, j, P;
   ubyte *RGBbuff=NULL, *RGBptr;
   char *encbuff=NULL;
   int encbuff_size, RGBbuff_size;
   int byte_count;
   int row_str_length;
   ubyte gamma_table[256];

   spec->compress = DO_COMPRESSION;

   _dxf_make_gamma_table(gamma_table, spec->gamma); 

   if (!ps_image_header(fout, spec))
        goto error;

   RGBbuff_size = spec->image_width*3;
   RGBbuff = DXAllocate(RGBbuff_size);

   /* This is a worst-case row buffer size for no runs */
   encbuff_size = RGBbuff_size*128*2/127;
   /* now add room for newlines and some spare         */
   encbuff_size += encbuff_size/CHARS_PER_LINE + 100;
   encbuff = DXAllocate(encbuff_size);

   byte_count = 0;

   if (imageType == FLOAT_COLOR)
       {
           RGBColor *fpixels = (RGBColor *)pixels;
           for (i=0; i<spec->image_height; i++) {
               for (j=0, RGBptr = RGBbuff; j<spec->image_width; j++, fpixels++) {
                  float r = fpixels->r * 255;
                  float g = fpixels->g * 255;
                  float b = fpixels->b * 255;
                  *RGBptr++  = gamma_table[CLAMP(r)];
                  *RGBptr++  = gamma_table[CLAMP(g)];
                  *RGBptr++  = gamma_table[CLAMP(b)];
               }
               build_rle_row(RGBbuff, spec->image_width, 3,
                            spec->compress, (ubyte *)encbuff, &byte_count, RLE_ASCII, RLE_COUNT_PRE, RLE_COMPACT);
               row_str_length = strlen(encbuff);
               if (fprintf(fout, "%s", encbuff) != row_str_length)
                   goto error;

               /* force newlines after each row if no compression */
               if (!spec->compress)
               {
                   if ((encbuff) && (*encbuff) && (encbuff[strlen(encbuff)-1] != '\n')
                       && (fprintf(fout,"\n") != 1))
                           goto error;
                   byte_count = 0;
               }
           }
       }
       else if (imageType == UBYTE_COLOR)
       {
           RGBByteColor *upixels = (RGBByteColor *)pixels;

           for (i=0; i<spec->image_height; i++) {
               for (j=0, RGBptr = RGBbuff; j<spec->image_width; j++, upixels++) {
                  *RGBptr++  = gamma_table[upixels->r];
                  *RGBptr++  = gamma_table[upixels->g];
                  *RGBptr++  = gamma_table[upixels->b];
               }
               build_rle_row(RGBbuff, spec->image_width, 3,
                            spec->compress, (ubyte *)encbuff, &byte_count, RLE_ASCII, RLE_COUNT_PRE, RLE_COMPACT);
               row_str_length = strlen(encbuff);
               if (fprintf(fout, "%s", encbuff) != row_str_length)
                   goto error;

               /* force newlines after each row if no compression */
               if (!spec->compress)
               {
                   if ((encbuff) && (*encbuff) && (encbuff[strlen(encbuff)-1] != '\n')
                       && (fprintf(fout,"\n") != 1))
                           goto error;
                   byte_count = 0;
               }
           }
       }
       else
       {
           ubyte *upixels = (ubyte *)pixels;

           for (i=0; i<spec->image_height; i++) {
               for (j=0, RGBptr = RGBbuff; j<spec->image_width; j++, upixels++) {
                  float r = map[*upixels].r * 255;
                  float g = map[*upixels].g * 255;
                  float b = map[*upixels].b * 255;
                  *RGBptr++  = gamma_table[CLAMP(r)];
                  *RGBptr++  = gamma_table[CLAMP(g)];
                  *RGBptr++  = gamma_table[CLAMP(b)];
               }
               build_rle_row(RGBbuff, spec->image_width, 3,
                            spec->compress, (ubyte *)encbuff, &byte_count, RLE_ASCII, RLE_COUNT_PRE, RLE_COMPACT);
               row_str_length = strlen(encbuff);
               if (fprintf(fout, "%s", encbuff) != row_str_length)
                   goto error;

               /* force newlines after each row if no compression */
               if (!spec->compress)
               {
                   if ((encbuff) && (*encbuff) && (encbuff[strlen(encbuff)-1] != '\n')
                       && (fprintf(fout,"\n") != 1))
                           goto error;
                   byte_count = 0;
               }
           }
       }

   if (fprintf(fout,"\n") != 1)
       goto error;

   DXFree(RGBbuff);
   DXFree(encbuff);
   return OK;
error:
   DXFree(RGBbuff);
   DXFree(encbuff);
   DXErrorReturn(ERROR_DATA_INVALID, "Can't write PostScript file.");
}


/*
 * This routine converts the RGB pixels to gray levels and writes out 8bits
 * per pixel to the PostScript.
 *
 * Note, that PostScript expects its pixels from left to right and
 * top to bottom, just the way DX keeps them stored.
 *
 * Returns OK/ERROR on failure/success.
 */
static Error
ps_out_gry(FILE *fout,
        Pointer pixels, RGBColor *map, int imageType, struct ps_spec *spec)
{
   int i, j, P;
   ubyte *BWbuff=NULL, *BWptr;
   char *encbuff=NULL;
   int encbuff_size, BWbuff_size;
   int byte_count;
   int row_str_length;
   ubyte gamma_table[256];
   float f;
   float g;

   spec->compress = DO_COMPRESSION;

   g = 1.0/spec->gamma; 
   for (i=0; i<256; i++) {
       f = i/255.0;	
       gamma_table[i] = 255*pow(f, g); 
   }

   if (!ps_image_header(fout, spec))
        goto error;

   BWbuff_size = spec->image_width;
   BWbuff = DXAllocate(BWbuff_size);

   /* This is a worst-case row buffer size for no runs */
   encbuff_size = spec->image_width*128*2/127;
   /* now add room for newlines and some spare         */
   encbuff_size += encbuff_size/CHARS_PER_LINE + 100;
   encbuff = DXAllocate(encbuff_size);

   byte_count = 0;

   if (imageType == FLOAT_COLOR)
       {
           RGBColor *fpixels = (RGBColor *)pixels;
           for (i=0; i<spec->image_height; i++) {
               for (j=0, BWptr = BWbuff; j<spec->image_width; j++, fpixels++) {
                  float r = fpixels->r;
                  float g = fpixels->g;
                  float b = fpixels->b;
                  float gray = RGB_TO_BW(r, g, b) * 255;
                  *BWptr++  = gamma_table[CLAMP(gray)];
               }
               build_rle_row(BWbuff, spec->image_width, 1,
                            spec->compress, (ubyte *)encbuff, &byte_count, RLE_ASCII, RLE_COUNT_PRE, RLE_COMPACT);
               row_str_length = strlen(encbuff);
               if (fprintf(fout, "%s", encbuff) != row_str_length)
                   goto error;

               /* force newlines after each row if no compression */
               if (!spec->compress)
               {
                   if ((encbuff) && (*encbuff) && (encbuff[strlen(encbuff)-1] != '\n')
                       && (fprintf(fout,"\n") != 1))
                           goto error;
                   byte_count = 0;
               }
           }
       }
       else if (imageType == UBYTE_COLOR)
       {
           RGBByteColor *upixels = (RGBByteColor *)pixels;

           for (i=0; i<spec->image_height; i++) {
               for (j=0, BWptr = BWbuff; j<spec->image_width; j++, upixels++) {
                  float r = upixels->r;
                  float g = upixels->g;
                  float b = upixels->b;
                  float gray = RGB_TO_BW(r, g, b);
                  *BWptr++  = gamma_table[CLAMP(gray)];
               }
               build_rle_row(BWbuff, spec->image_width, 1,
                            spec->compress, (ubyte *)encbuff, &byte_count, RLE_ASCII, RLE_COUNT_PRE, RLE_COMPACT);
               row_str_length = strlen(encbuff);
               if (fprintf(fout, "%s", encbuff) != row_str_length)
                   goto error;

               /* force newlines after each row if no compression */
               if (!spec->compress)
               {
                   if ((encbuff) && (*encbuff) && (encbuff[strlen(encbuff)-1] != '\n')
                       && (fprintf(fout,"\n") != 1))
                           goto error;
                   byte_count = 0;
               }
           }
       }
       else
       {
           ubyte *upixels = (ubyte *)pixels;

           for (i=0; i<spec->image_height; i++) {
               for (j=0, BWptr = BWbuff; j<spec->image_width; j++, upixels++) {
                  float r = map[*upixels].r;
                  float g = map[*upixels].g;
                  float b = map[*upixels].b;
                  float gray = RGB_TO_BW(r, g, b) * 255;
                  *BWptr++  = gamma_table[CLAMP(gray)];
               }
               build_rle_row(BWbuff, spec->image_width, 1,
                            spec->compress, (ubyte *)encbuff, &byte_count, RLE_ASCII, RLE_COUNT_PRE, RLE_COMPACT);
               row_str_length = strlen(encbuff);
               if (fprintf(fout, "%s", encbuff) != row_str_length)
                   goto error;

               /* force newlines after each row if no compression */
               if (!spec->compress)
               {
                   if ((encbuff) && (*encbuff) && (encbuff[strlen(encbuff)-1] != '\n')
                       && (fprintf(fout,"\n") != 1))
                           goto error;
                   byte_count = 0;
               }
           }
       }

   if (fprintf(fout,"\n") != 1)
       goto error;

   DXFree(BWbuff);
   DXFree(encbuff);
   return OK;
error:
   DXFree(BWbuff);
   DXFree(encbuff);
   DXErrorReturn(ERROR_DATA_INVALID, "Can't write PostScript file.");
}

/*
 * This routine writes out the trailer of the PostScript file
 * Undoes some of what was done in put_ps_header().
 */
static Error
put_ps_trailer(FILE *fout, struct ps_spec *spec)
{

   if (fprintf(fout, "%%%%Trailer\n") <= 0)
        goto error;
   if (fprintf(fout, "\n%% Stop using temporary dictionary\nend\n\n") <= 0)
        goto error;
   if (fprintf(fout, "%% Restore original state\norigstate restore\n\n") <= 0)
        goto error;
   if (fprintf(fout, "%%%%EOF\n") <= 0)
        goto error;
   return OK;

error:
   DXErrorReturn(ERROR_DATA_INVALID, "Can't write PostScript file.");
}

/*
 * Put the arguments on the stack and call the correct imaging
 * function (either image or colorimage).
 * It is assumed that the data for the image will be written out
 * immediately following the output here.
 *
 * Always returns OK.
 */
static Error
ps_image_header(FILE *fout, struct ps_spec *spec)
{

   if (fprintf(fout,"%% Dimensionality of data\n%d %d 8\n\n",
                                spec->image_width,spec->image_height) <= 0)
        goto error;
   if (fprintf(fout,"%% Mapping matrix\n[ %d 0 0 %d 0 0 ]\n\n",
                                spec->image_width,spec->image_height) <= 0)
        goto error;
   if (fprintf(fout, "%% Function to read pixels\n") <= 0)
        goto error;
   if (fprintf(fout, "{ rlerow }\n") <= 0)
        goto error;

   if (spec->colormode == FULLCOLOR) {
        if (fprintf(fout, "false 3\t\t\t%% Single data source, 3 colors\n")<=0)
            goto error;
        if (fprintf(fout, "colorimage\n") <= 0)
            goto error;
   } else {
        if (fprintf(fout, "image\n") <= 0)
            goto error;
   }
   return OK;

error:
   DXErrorReturn(ERROR_DATA_INVALID, "Can't write PostScript file.");
}

/*
 * Write out code to test for the colorimage function and to define it
 * if it is not defined.
 */
static Error
put_colorimage_function(FILE *fout)
{
    LINEOUT("%% Define colorimage procedure if not present"                   );
    LINEOUT("/colorimage where   %% colorimage defined?"                      );
    LINEOUT("  { pop }           %% yes: pop off dict returned"               );
    LINEOUT("  {                 %% no:  define one with grayscale conversion");
    LINEOUT("    /colorimage {"                                               );
    LINEOUT("      pop"                                                       );
    LINEOUT("      pop"                                                       );
    LINEOUT("      /hexread exch store"                                       );
    LINEOUT("      {"                                                         );
    LINEOUT("        hexread"                                                 );
    LINEOUT("        /rgbdata exch store    %% call input row 'rgbdata'"      );
    LINEOUT("        /rgbindx 0 store"                                        );
    LINEOUT("        0 1 pixperrow 2 sub {"                                   );
    LINEOUT("          grayrow exch"                                          );
    LINEOUT("          rgbdata rgbindx       get 19 mul"                      );
    LINEOUT("          rgbdata rgbindx 1 add get 38 mul"                      );
    LINEOUT("          rgbdata rgbindx 2 add get  7 mul"                      );
    LINEOUT("          add add 64 idiv"                                       );
    LINEOUT("          put"                                                   );
    LINEOUT("          /rgbindx rgbindx 3 add store"                          );
    LINEOUT("        } for"                                                   );
    LINEOUT("        grayrow  %% output row converted to grayscale"           );
    LINEOUT("      }"                                                         );
    LINEOUT("      image"                                                     );
    LINEOUT("    } bind def"                                                  );
    LINEOUT("  } ifelse"                                                      );

    return OK;

bad_write:
    DXErrorReturn(ERROR_DATA_INVALID, "Can't write PostScript file.");
    return ERROR;
}

/*
 * Called before each new image to begin a new page.
 */
static Error
ps_new_page(FILE *fout,int page, int pages, struct ps_spec *spec)
{
        if (fprintf(fout,"\n%%%%Page: %d %d\n\n",page,pages) <= 0)
           goto error;
        if (!put_ps_page_setup(fout,spec))
           goto error;

        return OK;
error:
    DXErrorReturn(ERROR_DATA_INVALID, "Can't write PostScript file.");
}

/*
 * Called after every image to end a page.
 */
static Error
ps_end_page(FILE *fout)
{
   if (fprintf(fout, "\nshowpage\n") <= 0)
      goto error;
   else
      return OK;
error:
    DXErrorReturn(ERROR_DATA_INVALID, "Can't write PostScript file.");
}

/*
 * Determine the pagemode (PORTRAIT or LANDSCAPE) given the dimensions
 * of the image in spec.
 *
 */
static void
orient_page(struct ps_spec *spec)
{
    int mode = PORTRAIT;
    int pagewidth_in_pixels;
    int pageheight_in_pixels;
    int badfit = 0;
    float wx, wy;
    float aspect;
    float page_aspect;

    if (!spec->width_dpi) {
	wx = spec->page_width - 2*spec->page_margin;
	wy = spec->page_height - 2*spec->page_margin;
	page_aspect = wy/wx;
	aspect = spec->image_height*1.0/spec->image_width;
	if ((aspect>1 && page_aspect>1) || (aspect<1 && page_aspect<1)) {
	    spec->width_dpi = spec->image_width/wx;
	    spec->height_dpi = spec->image_height/wy;
	} else {
	    spec->width_dpi = spec->image_height/wx;
	    spec->height_dpi = spec->image_width/wy;
	}
    }

    pagewidth_in_pixels = spec->width_dpi * spec->page_width;
    pageheight_in_pixels = spec->height_dpi * spec->page_height;

    if (spec->image_width > pagewidth_in_pixels) {
        if (spec->image_height < pageheight_in_pixels) {
            mode = LANDSCAPE;
        } else
            badfit = 1;
    } else if (spec->image_height > pageheight_in_pixels) {
        if (spec->image_width < pagewidth_in_pixels) {
            mode = PORTRAIT;
        } else
            badfit = 1;
    }

    if (badfit)
        DXWarning("PostScript image overflows page");

    if (spec->page_orient == ORIENT_NOT_SET || spec->page_orient == ORIENT_AUTO)
        spec->page_orient = mode;

}
#define SKIP_WHITE(p) while (*p && (*p == ' ' || *p == '\t')) p++
#define FIND_EQUAL(p) while (*p && (*p != '=')) p++

static Error
parse_format(char *fmt, struct ps_spec *spec)
{
    char *format_modifier;
    float width,height;
    char *p;
    char format[1024];
    int i, len = strlen(fmt);
    int dpi_parsed = 0, width_parsed = 0;
    float g;

    if (len >= 1024) {
        DXSetError(ERROR_BAD_PARAMETER,"'format' too long.");
        return ERROR;
    } else {
        /* Copy the pattern into a lower case buffer */
        for (i=0 ; i<len ; i++) {
            char b = fmt[i];
            format[i] = (isupper(b) ? tolower(b) : b);
        }
        format[len] = '\0';
    }
    /*
     * Parse the page size.
     */
    format_modifier = "page";
    if ((p=strstr(format," page"))) {
        p += 5;
        SKIP_WHITE(p);
        if (!p) goto format_error;
        FIND_EQUAL(p);
        if (!p) goto format_error;
            p++;        /*  Skip the '=' */
            SKIP_WHITE(p);
            if (p && (sscanf(p,"%fx%f",&width,&height) == 2))  {
                spec->page_width = width;
                spec->page_height = height;
            } else if (p && (sscanf(p,"%fX%f",&width,&height) == 2))  {
                spec->page_width = width;
                spec->page_height = height;
            } else  {
                goto format_error;
            }
    }
    /*
     * Parse the dots per inch.
     */
    format_modifier = "dpi";
    if ((p=strstr(format," dpi"))) {
        int dpi;
        p += 4;
        SKIP_WHITE(p);
        if (!p) goto format_error;
        FIND_EQUAL(p);
        if (!p) goto format_error;
            p++;        /* Skip the '=' */
            SKIP_WHITE(p);
            if (!p) goto error;
            if (sscanf(p,"%d",&dpi) == 1)  {
                spec->width_dpi  = dpi;
                spec->height_dpi = dpi;
            } else  {
                goto format_error;
            }
            dpi_parsed = 1;
    }
    /*
     * Parse the gamma
     */
    format_modifier = "gamma";
    if ((p=strstr(format," gamma"))) {
        p += 4;
        SKIP_WHITE(p);
        if (!p) goto format_error;
        FIND_EQUAL(p);
        if (!p) goto format_error;
            p++;        /* Skip the '=' */
            SKIP_WHITE(p);
            if (!p) goto error;
            if (sscanf(p,"%f",&g) == 1)  {
                spec->gamma  = g;
            } else  {
                goto format_error;
            }
    }
    /*
     * Parse the page orientation.
     */
    format_modifier = "orient";
    if ((p=strstr(format," orient"))) {
        p += 7;
        SKIP_WHITE(p);
        if (!p) goto format_error;
        FIND_EQUAL(p);
        if (!p) goto format_error;
            p++;        /* Skip the '=' */
            SKIP_WHITE(p);
            if (p && !strncmp(p,"landscape",9)) {
                spec->page_orient = LANDSCAPE;
            } else if (p && !strncmp(p,"portrait",8)) {
                spec->page_orient = PORTRAIT;
            } else if (p && !strncmp(p,"auto",4)) {
                spec->page_orient = ORIENT_AUTO;
            } else  {
                goto format_error;
            }
    }
    /*
     * Parse the margin.
     */
    format_modifier = "margin";
    if ((p=strstr(format," margin"))) {
        p += 7;
        SKIP_WHITE(p);
        if (!p) goto format_error;
        FIND_EQUAL(p);
        if (!p) goto format_error;
            p++;        /* Skip the '=' */
            SKIP_WHITE(p);
            if (!p) goto error;
            if (sscanf(p,"%f",&g) == 1)  {
                spec->page_margin  = g;
            } else  {
                goto format_error;
            }
    }
    /*
     * Parse the width image dimensioning spec where width is in inches
     * and the width in this case means the dimension of the image that
     * goes across the screen.  Width indicates a resolution in both
     * height and width (to keep the aspect ratio the same).
     * This option will cause the image's width as printed to be the
     * given number of inches.
     */
    format_modifier = "width";
    if ((p=strstr(format," width"))) {
        p += 6;
        SKIP_WHITE(p);
        if (!p) goto format_error;
        FIND_EQUAL(p);
        if (!p) goto format_error;
            p++;        /* Skip the '=' */
            SKIP_WHITE(p);
            if (!p || (sscanf(p,"%f",&width) != 1))  {
                goto format_error;
            } else if (width == 0) {
                DXErrorGoto(ERROR_BAD_PARAMETER,
                    "PostScript 'width' must be non-zero");
            }
            width_parsed = 1;
            spec->height_dpi = spec->width_dpi = spec->image_width  / width;
            if (dpi_parsed)
                DXWarning(
                "'width' overrides 'dpi' PostScript format modifier");
    }
    /*
     * Parse the height image dimensioning spec where height is in inches
     * and the height in this case means the dimension of the image that
     * goes up and down the screen.  If 'width' was not already given
     * then use the same dpi in the width directoin as the height (i.e.
     * keep the same aspect ratio).
     * This option will cause the image's height as printed to be the
     * given number of inches.
     */
    format_modifier = "height";
    if ((p=strstr(format," height"))) {
        p += 7;
        SKIP_WHITE(p);
        if (!p) goto format_error;
        FIND_EQUAL(p);
        if (!p) goto format_error;
            p++;        /* Skip the '=' */
            SKIP_WHITE(p);
            if (!p || (sscanf(p,"%f",&height) != 1))  {
                goto format_error;
            } else if (height == 0) {
                DXErrorGoto(ERROR_BAD_PARAMETER,
                    "PostScript 'height' must be non-zero");
            }
            spec->height_dpi = spec->image_height / height;
            if (!width_parsed) {
                spec->width_dpi = spec->height_dpi;
                if (dpi_parsed)
                    DXWarning(
                    "'height' overrides 'dpi' PostScript format modifier");
            }
    }

    return OK;

format_error:
    DXSetError(ERROR_BAD_PARAMETER,
                "PostScript format modifier '%s' has incorrect format",
                        format_modifier);
error:
    return ERROR;
}

static Error
put_miff_header(FILE *fout, char *filename, int frame, struct ps_spec *spec, int imageType, int nframes)
{
   const time_t t = time(0);
   char *tod = ctime((const time_t *)&t);
   int major, minor, micro;

   if (fprintf(fout, "id=ImageMagick\n") <= 0) goto error;
   if (imageType != MAPPED_COLOR) {
       if (fprintf(fout, "class=DirectClass\n") <= 0) goto error;
   } else {
       if (fprintf(fout, "class=PseudoClass colors=256\n") <= 0) goto error;
   }
   /*  Change gamma in the pixels themselves */
#if 0
   if (spec->gamma != 1) {
       if (fprintf(fout, "gamma=%08.4f\n", spec->gamma) <= 0) goto error;
   }
#endif
   if (fprintf(fout, "compression=RunlengthEncoded\n") <= 0) goto error;
   if (fprintf(fout, "columns=%d  rows=%d depth=8\n",
			spec->image_width, spec->image_height) <= 0) goto error;
   if (fprintf(fout, "scene=%d\n", frame) <= 0) goto error;
   DXVersion(&major, &minor, &micro);
   if (fprintf(fout, "{\nCreated by IBM Data Explorer %d.%d.%d",
                                        major,minor,micro) <= 0) goto error;
   if (fprintf(fout, "  Date: %s", tod) <= 0) goto error;
   if (frame == 0) {
       if (fprintf(fout, "DO NOT ALTER NEXT LINE\n") <= 0) goto error;
       if (fprintf(fout, NSCENES_STRING) <= 0) goto error;
       if (fprintf(fout, "%06d\n", nframes) <= 0) goto error;
   } 
   if (fprintf(fout, DATASIZE_STRING) <= 0) goto error;
   spec->fptr = ftell(fout);
   if (fprintf(fout, "%08d\n",0) <= 0) goto error;
   if (fprintf(fout, "}\n:\n") <= 0) goto error;

   return OK;

error:
   DXErrorReturn(ERROR_DATA_INVALID, "Can't write miff file.");
}

#define STAT_OK           0
#define STAT_DONE         1
#define STAT_COMPLETE     2
#define STAT_ERROR        4

static int
get_token(char *b, char *t, char *v)
{
    int l;
    int p, q, r;

    *t = *v = '\0';
    l = strlen(b);
    if (!l)
	return STAT_DONE;
    for (p = 0; p<l && isspace(b[p]); p++)
	;
    if (b[p] == ':')
	return STAT_COMPLETE;
    if (p == l)
	return STAT_DONE;
    for (q = p+1; q<l && !isspace(b[q]) && b[q] != '='; q++)
	;
    if (b[q] != '=')
	return STAT_ERROR;
    strncpy(t, &b[p], (q-p));
    t[q-p] = '\0';
    for (q++; q<l && isspace(b[q]); q++)
	;
    if (q == l)
	return STAT_ERROR;
    for (r = q; r<l && !isspace(b[r]); r++)
	;
    strncpy(v, &b[q], (r-q));
    v[r-q] = '\0';
    memmove(b, &b[r], l-r+1);

    return STAT_OK;
}


static Error
read_miff_header(FILE *fin, int *frame, struct ps_spec *spec, int *imageType, int *nframes)
{

    char buff[BUFFSIZE]; 
    char token[128], value[128];
    int  found_frames = 0;
    int  found_data = 0;
    int  status = STAT_OK;
    int  tmp;

    for (;;) {
	if (!fgets(buff, BUFFSIZE, fin)) goto error;
	if (strstr(buff, "{")) {
	    for (;;) {
		if (!fgets(buff, BUFFSIZE, fin)) goto error;
		if (strstr(buff, "}"))
		    break;
		if (!found_frames && (1 == sscanf(buff, "nscenes=%d", &tmp))) {
		    found_frames = 1;
		    *nframes = tmp;
		}
		if (!found_data && (1 == sscanf(buff, "data_size=%d", &tmp))) {
		    found_data = 1;
		    spec->data_size = tmp;
		}
	    }
	    continue;
	}
	for (status = get_token(buff, token, value); status == STAT_OK;
			status = get_token(buff, token, value)) {
	    if (!strcmp(token, "class")) {
		if (strcmp(value, "DirectClass")) {
		    DXSetError(ERROR_DATA_INVALID, "MIFF: class=%s is not supported", value);
		    goto error;
		}
	    } else if (!strcmp(token, "columns")) {
		if (1 != sscanf(value, "%d", &spec->image_width)) {
		    DXSetError(ERROR_DATA_INVALID, "MIFF: bad columns value, %s", value);
		    goto error;
		}
	    } else if (!strcmp(token, "compression")) {
		if (strcmp(value, "RunlengthEncoded")) {
		    DXSetError(ERROR_DATA_INVALID, "MIFF: compression=%s is not supported", value);
		    goto error;
		}
	    } else if (!strcmp(token, "rows")) {
		if (1 != sscanf(value, "%d", &spec->image_height)) {
		    DXSetError(ERROR_DATA_INVALID, "MIFF: bad rows value, %s", value);
		    goto error;
		}
	    } else if (!strcmp(token, "scene")) {
		if (1 != sscanf(value, "%d", frame)) {
		    DXSetError(ERROR_DATA_INVALID, "MIFF: bad scene value, %s", value);
		    goto error;
		}
	    }
	}
	if (status == STAT_COMPLETE)
	    break;
	if (status == STAT_ERROR) {
	    DXSetError(ERROR_DATA_INVALID, "MIFF: error parsing line %s", buff);
	    goto error;
	}
    }

    return OK;

error:
    return ERROR;
}

SizeData * _dxf_ReadImageSizesMIFF(char *name,
				    int startframe,
				    SizeData *data,
				    int *use_numerics,
				    int ext_sel,
				    int *multiples)
{
    FILE *fin;
    struct ps_spec spec;
    int imgtype;
    int nframes;

    memset(&spec, 0, sizeof(spec));
    if (NULL == (fin = fopen(name, "r")))
	goto error;
    if (ERROR == read_miff_header(fin, &startframe, &spec, &imgtype, &nframes))
	goto error;
    fclose(fin);
    data->height = spec.image_height;
    data->width = spec.image_width;
    *multiples = ((nframes>0)?1:0);
    data->startframe = 0;
    data->endframe = nframes-1;
    *use_numerics = 0;

    return data;

error:
    return ERROR;
}

static Error
miff_out_flc(FILE *fout, Pointer pixels,
        RGBColor *map, int imageType, struct ps_spec *spec)
{
   int i, j, P;
   ubyte *encbuff, *RGBbuff, *RGBptr;
   int encbuff_size, RGBbuff_size;
   int byte_count;
   ubyte gamma_table[256];

   spec->compress = DO_COMPRESSION;

   _dxf_make_gamma_table(gamma_table, spec->gamma); 

   RGBbuff_size = spec->image_width*3;
   RGBbuff = DXAllocate(RGBbuff_size);

   /* This is a worst-case row buffer size for no runs */
   encbuff_size = RGBbuff_size*2;
   /* now add some spare         */
   encbuff_size += 100;
   encbuff = DXAllocate(encbuff_size);

   if (imageType == FLOAT_COLOR)
       {
           RGBColor *fpixels = (RGBColor *)pixels;
	   fpixels += spec->image_width*(spec->image_height-1);
           for (i=0; i<spec->image_height; i++) {
               for (j=0, RGBptr = RGBbuff; j<spec->image_width; j++, fpixels++) {
                  float r = fpixels->r * 255;
                  float g = fpixels->g * 255;
                  float b = fpixels->b * 255;
                  *RGBptr++  = gamma_table[CLAMP(r)];
                  *RGBptr++  = gamma_table[CLAMP(g)];
                  *RGBptr++  = gamma_table[CLAMP(b)];
               }
	       byte_count = 0;
               build_rle_row(RGBbuff, spec->image_width, 3,
                            spec->compress, encbuff, &byte_count, RLE_BINARY, RLE_COUNT_POST, RLE_STUPID);
               if (fwrite(encbuff, sizeof(ubyte), byte_count, fout) != byte_count)
                   goto error;
	       spec->data_size += byte_count;		
	       fpixels -= 2*spec->image_width;	

           }
       }
       else if (imageType == UBYTE_COLOR)
       {
           RGBByteColor *upixels = (RGBByteColor *)pixels;
	   upixels += spec->image_width*(spec->image_height-1);

           for (i=0; i<spec->image_height; i++) {
               for (j=0, RGBptr = RGBbuff; j<spec->image_width; j++, upixels++) {
                  *RGBptr++  = gamma_table[upixels->r];
                  *RGBptr++  = gamma_table[upixels->g];
                  *RGBptr++  = gamma_table[upixels->b];
               }
	       byte_count = 0;
               build_rle_row(RGBbuff, spec->image_width, 3,
                            spec->compress, encbuff, &byte_count, RLE_BINARY, RLE_COUNT_POST, RLE_STUPID);
               if (fwrite(encbuff, sizeof(ubyte), byte_count, fout) != byte_count)
                   goto error;
	       spec->data_size += byte_count;		
	       upixels -= 2*spec->image_width;	
           }
       }
       else
       {
           ubyte *upixels = (ubyte *)pixels;
	   ubyte cmap[256*3];
	   for (i=0, RGBptr = cmap; i<256; i++) {
                  float r = map[i].r * 255;
                  float g = map[i].g * 255;
                  float b = map[i].b * 255;
                  *RGBptr++  = gamma_table[CLAMP(r)];
                  *RGBptr++  = gamma_table[CLAMP(g)];
                  *RGBptr++  = gamma_table[CLAMP(b)];
	   }

	   if (fwrite(cmap, sizeof(ubyte), 256*3, fout) != 256*3)
		goto error;

	   spec->data_size += 256*3;		
	   upixels += spec->image_width*(spec->image_height-1); 
           for (i=0; i<spec->image_height; i++) {
	       byte_count = 0;
               build_rle_row(upixels, spec->image_width, 1,
                            spec->compress, encbuff, &byte_count, RLE_BINARY, RLE_COUNT_POST, RLE_STUPID);
               if (fwrite(encbuff, sizeof(ubyte), byte_count, fout) != byte_count)
                   goto error;
	       spec->data_size += byte_count;		
	       upixels -= spec->image_width;
           }
       }

   DXFree(RGBbuff);
   DXFree(encbuff);
   return OK;
error:
   DXFree(RGBbuff);
   DXFree(encbuff);
   DXErrorReturn(ERROR_DATA_INVALID, "Can't write miff pixel data.");
}

Error
_dxf_write_miff(RWImageArgs *iargs)
{
        Error e;
        e = output_miff(iargs);
        return e;
}

static Error
output_miff(RWImageArgs *iargs)
{
    char     imagefilename[MAX_IMAGE_NAMELEN];
    int      firstframe, lastframe, i, series;
    int      frames, deleteable = 0;
    struct   ps_spec page_spec;
    Pointer  pixels;
    FILE     *fp = NULL;
    Array    colors, color_map;
    RGBColor *map = NULL;
    int      imageType=0;
    Type     type;
    int      rank, shape[32];
    char     buff[BUFFSIZE];
    char     *seqptr;
    long     fptr = 0;
    int      init_nframes = 0;

    SizeData  image_sizes;  /* Values as found in the image object itself */
    Field       img = NULL;

    if (iargs->adasd)
        DXErrorGoto(ERROR_NOT_IMPLEMENTED,
                "MIFF format not supported on disk array");

    series = iargs->imgclass == CLASS_SERIES;

    if ( !_dxf_GetImageAttributes ( (Object)iargs->image, &image_sizes ) )
        goto error;

    /*
     * Always write all frames of a series.
     */
    frames = (image_sizes.endframe - image_sizes.startframe + 1);
    firstframe = image_sizes.startframe;
    lastframe = image_sizes.endframe;
    page_spec.image_height = image_sizes.height;
    page_spec.image_width = image_sizes.width;
    page_spec.gamma = 2.0;
    if (iargs->format) {
        if (!parse_format(iargs->format,&page_spec))
            goto error;
    }

    /*
     * Attempt to open image file and position to start of frame(s).
     */
    if (iargs->pipe == NULL) {
        /*
         * Create an appropriate file name.
         */
        if (!_dxf_BuildImageFileName(imagefilename,MAX_IMAGE_NAMELEN,
                        iargs->basename, iargs->imgtyp,iargs->startframe,0))
                goto error;
	/*
	 * Need to open the file if it exists, and find number of scenes present
	 * as specified in the first header.
	 * Then save pointer to this number string for later update.
	*/
	if ((fp=fopen(imagefilename, "r+"))) {
	   for ( ; ; ) { 
	       fptr = ftell(fp);
	       if (!fgets(buff, BUFFSIZE, fp)) goto error;  
	       if ((seqptr=strstr(buff, NSCENES_STRING))) break;
	   } 
	   seqptr += strlen(NSCENES_STRING);
	   if (!sscanf(seqptr,"%d", &init_nframes)) goto error;
	   if (fseek(fp, fptr, SEEK_SET)) goto error;
	   if (fprintf(fp, NSCENES_STRING) < 0) goto error;
	   if (fprintf(fp, "%06d\n", init_nframes+frames) < 0) goto error;
	   if (fseek(fp, 0, SEEK_END)) goto error;
	   fptr = ftell(fp);
	   fclose(fp);
	}

	if (!fptr) 
	{
	    if ( (fp = fopen (imagefilename, "w+" )) == 0 )
		ErrorGotoPlus1 ( ERROR_DATA_INVALID,
			      "Can't open image file (%s)", imagefilename );
	} else {
	    if ( (fp = fopen (imagefilename, "r+" )) == 0 )
		ErrorGotoPlus1 ( ERROR_DATA_INVALID,
			      "Can't open image file (%s)", imagefilename );
	    fseek(fp, fptr, SEEK_SET);
	}

    } else {
        strcpy(imagefilename,"stdout");
        fp = iargs->pipe;
    }

    img = iargs->image;
    for (i=firstframe ; i<=lastframe ; i++)
    {
        if (series) {
            if (deleteable) DXDelete((Object)img);
            img = _dxf_GetFlatSeriesImage((Series)iargs->image,i,
                        page_spec.image_width,page_spec.image_height,
                        &deleteable);
            if (!img)
                goto error;
        }

        colors = (Array)DXGetComponentValue(img, "colors");
        if (! colors)
        {
            DXSetError(ERROR_DATA_INVALID,
                "image does not contain colors component");
            goto error;
        }

        DXGetArrayInfo(colors, NULL, &type, NULL, &rank, shape);

        if (type == TYPE_FLOAT)
        {
            if (rank != 1 || shape[0] != 3)
            {
                DXSetError(ERROR_DATA_INVALID,
                    "floating-point colors component must be 3-vector");
                goto error;
            }

            imageType = FLOAT_COLOR;
        }
        else if (type == TYPE_UBYTE)
        {
            if (rank == 0 || (rank == 1 && shape[0] == 1))
            {
                color_map = (Array)DXGetComponentValue(img, "color map");
                if (! color_map)
                {
                    DXSetError(ERROR_DATA_INVALID,
                        "single-valued ubyte colors component requires a %s",
                        "color map");
                    goto error;
                }

                DXGetArrayInfo(color_map, NULL, &type, NULL, &rank, shape);

                if (type != TYPE_FLOAT || rank != 1 || shape[0] != 3)
                {
                    DXSetError(ERROR_DATA_INVALID,
                        "color map must be floating point 3-vectors");
                    goto error;
                }

                map = (RGBColor *)DXGetArrayData(color_map);
                imageType = MAPPED_COLOR;
            }
            else if (rank != 1 || shape[0] != 3)
            {
                DXSetError(ERROR_DATA_INVALID,
                    "unsigned byte colors component must be either single %s",
                    "valued with a color map or 3-vectors");
                goto error;
            }
            else
                imageType = UBYTE_COLOR;;
        }

        if (!(pixels = DXGetArrayData(colors)))
            goto error;

	/*
	 * Write out the miff header
	 */
	if (!put_miff_header(fp, imagefilename, i+init_nframes, &page_spec, imageType, frames))
	    goto bad_write;

        /*
         * Write out the pixel data
         */
	page_spec.data_size = 0;
        if (!miff_out_flc(fp, pixels, map, imageType, &page_spec))
            goto bad_write;
	fptr = ftell(fp);
	if (fseek(fp, page_spec.fptr, SEEK_SET))
	    goto error;
	if (fprintf(fp, "%08d", page_spec.data_size) < 0)
	    goto error;
	if (fseek(fp, fptr, SEEK_SET))
	    goto error;
    }

    if (fclose(fp) != 0)
        goto bad_write;

    if (deleteable && img) DXDelete((Object)img);

    return OK;

bad_write:
    if ( iargs->pipe == NULL )
        DXErrorGoto
            ( ERROR_DATA_INVALID,
              "Can't write miff file." )
    else
        DXErrorGoto
            ( ERROR_BAD_PARAMETER,
              "Can't send image with specified command." );

error:
    if ( fp ) fclose(fp);
    if (deleteable && img) DXDelete((Object)img);
    return ERROR;
}

Field _dxf_InputMIFF (FILE **fh, int width, int height, char *name, 
                      int relframe, int delayed, char *colortype)
{
    int      i;
    int      frame, nframes;
    struct   ps_spec page_spec;
    Pointer  pixels;
    int      imageType;
    Type     type;
    int      rank, shape[32];
    Field    image = NULL;
    Array    colorsArray;
    int      npix = 0;
    int      tot_pix;
    ubyte    *pixptr;

    struct {
	ubyte rgb[3];
	ubyte count;
    }rle_entry;

    if (!*fh)
	if (NULL == (*fh = fopen(name, "r")))
	    goto error;

    read_miff_header(*fh, &frame, &page_spec, &imageType, &nframes);
    image = DXMakeImageFormat(width, height, "BYTE");
    if (!image)
	goto error;

    tot_pix = width*height;
    colorsArray = (Array)DXGetComponentValue(image, "colors");
    DXGetArrayInfo(colorsArray, NULL, &type, NULL, &rank, shape);
    pixels = DXGetArrayData(colorsArray);
    pixptr = (ubyte *)pixels;
    pixptr += width*(height-1)*3*sizeof(ubyte);
    do {
	if (fread(&rle_entry, sizeof(rle_entry), 1, *fh) < 1)
	    goto error;
	for (i=0; i<rle_entry.count+1; i++) {
	    memcpy(pixptr, rle_entry.rgb, sizeof(rle_entry.rgb));
	    pixptr += 3;
	}
	npix += rle_entry.count+1;
	if (npix % width == 0)
	    pixptr -= 2*width*3*sizeof(ubyte);
    } while (npix < tot_pix);

    return image;


error:
    if (NULL != *fh)
	fclose(*fh);
    return ERROR;
}
