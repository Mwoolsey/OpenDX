/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dxmods/_rw_image.c,v 1.10 2005/02/01 00:36:23 davidt Exp $
 */

#include <dxconfig.h>


#include <stdio.h>
#include <fcntl.h>
#include <ctype.h>
#include <dx/dx.h>
#include "_helper_jea.h"
#include "_rw_image.h"
#include <sys/types.h>
#include <string.h>
#include <math.h>

static int SearchStringList(char *str, char *extension);
static int NthImageExtension(char *extensions, int n, char *buf, int bufl);

#define MAX_EXTENSION_LEN       10
/*
 * This is the table of supported images (currently, only WriteImage
 * uses this table to find the write functions).  The table will be 
 * searched sequentially for both format specifiers and extensions.
 * This means there should not be any overlap between format specifiers
 * or extensions.  If there is a clash, then the first matching format
 * or extension is the one that is used as a matched. Note the two
 * "ps" extensions for example.   When a filename extension of "ps" is
 * given it will be assumed to be a 'ps color' image.
 *
 * NOTE: although ReadImage is not currently table driven, we set the
 * 	read field to be none NULL for read supported formats.
 */
static Error nullread(RWImageArgs *dummy) 
{
   DXSetError(ERROR_UNEXPECTED,"unexpected format" );
   return ERROR;
}
static ImageInfo ImageTable[] = { 
 { img_typ_rgb,       
   	1, 
	"rgb",               
	"rgb", 	 
	APPENDABLE_FILES, 
	_dxf_write_rgb,	 
	nullread},

 { img_typ_fb, 	      
	1, 
	"fb",                
	"fb", 	 
	APPENDABLE_FILES|ADASD_OK|ADASD_ONLY, 
	_dxf_write_fb, 	 
	nullread},

 { img_typ_r_g_b,     
	3, 
	"r+g+b",             
	"r:g:b",	 
	APPENDABLE_FILES, 			
	_dxf_write_r_g_b,	 
	nullread},

 { img_typ_tiff,      
	1, 
	"tiff",              
	"tiff:tif",  /* Preferred. */
	0, 					
	_dxf_write_tiff, 	 
	NULL}, 

#if 0
 { img_typ_tiff,      
	1, 
	"tiff",              
	"tif", 	/* This is not the preferred writing format.  2nd in list.*/
	0, 					
	_dxf_write_tiff, 	 
	NULL}, 
#endif

 { img_typ_ps_color,  
	1, 
	"ps color:ps",       
	"ps",      
	PIPEABLE_OUTPUT,
	_dxf_write_ps_color,  
	NULL},

 { img_typ_eps_color, 
	1, 
	"eps color:eps",     
	"epsf",    
	PIPEABLE_OUTPUT,
	_dxf_write_eps_color, 
	NULL}, 

 { img_typ_ps_gray,   
	1, 
	"ps gray:ps grey",   
	"ps",      
	PIPEABLE_OUTPUT,
	_dxf_write_ps_gray,	 
	NULL}, 

 { img_typ_eps_gray,  
	1, 
	"eps gray:eps grey", 
	"epsf",  
	PIPEABLE_OUTPUT,
	_dxf_write_eps_gray,  
	NULL}, 


 { img_typ_yuv,       
   	1, 
	"yuv",               
	"yuv", 	 
	APPENDABLE_FILES, 
	_dxf_write_yuv,	 
	nullread},

 { img_typ_miff,  
	1, 
	"miff",       
	"miff:mif",      
	APPENDABLE_FILES,
	_dxf_write_miff,  
	NULL},
#ifdef HAVE_LIBMAGICK
 { img_typ_im,  
	1, 
	"imagemagick supported format",       /* due to pattern matching algorithm, must be lower case */
	"jpeg:jpg:gif:tif:tiff:png:pict:bmp",      /* FIXME: there is a way to get this list from IM */
	0,			/* FIXME: possibly separate into APPENDABLE_FILES and 0 */
	_dxf_write_im,  
	nullread},
 { img_typ_im,  
	1, 
	"image magick supported format",       /* due to pattern matching algorithm, must be lower case */
	"jpeg:jpg:gif:tif:tiff:png:pict:bmp",      /* FIXME: there is a way to get this list from IM */
	0,			/* FIXME: possibly separate into APPENDABLE_FILES and 0 */
	_dxf_write_im,  
	nullread},
 { img_typ_im,  
	1, 
	"imagemagick",       /* due to pattern matching algorithm, must be lower case */
	"jpeg:jpg:gif:tif:tiff:png:pict:bmp",      /* FIXME: there is a way to get this list from IM */
	0,			/* FIXME: possibly separate into APPENDABLE_FILES and 0 */
	_dxf_write_im,  
	nullread},
	
#endif /* def HAVE_LIBMAGICK */

 { img_typ_illegal } };	/* Must be last entry */


/*
 * Given the type of an image, return a pointer to its ImageInfo
 * structure if it exists.
 */
ImageInfo *
_dxf_ImageInfoFromType(ImageType type)
{
    int i;

    for ( i = 0; ImageTable[i].type != img_typ_illegal ; i++ )
        if ( type == ImageTable[i].type )
		return (&ImageTable[i]);
    return(NULL);
}

/*
 * Given the format of an image, return a pointer to its ImageInfo
 * structure if it exists.
 */
ImageInfo *
_dxf_ImageInfoFromFormat(char *format)
{
    int i;
    int matches = 0;
    ImageInfo *r = NULL;

    if ( format ) {
        for ( i = 0; ImageTable[i].type != img_typ_illegal ; i++ ) {
  	    int tmp = SearchStringList(ImageTable[i].formats,format);
  	    if (tmp > matches) {
		r = &ImageTable[i];
		matches = tmp;
	    }
	}
    }

    return r; 
}


char *
_dxf_RemoveExtension ( char *extended )
{
    int i;

    for ( i = strlen ( extended ) - 1;  i >= 0;  i-- )
		if ( extended[i] == '/' )
			break;
        else if ( extended[i] == '.' )
            { extended[i] = '\0';  return extended; }

    return ERROR;
}



/*
 * Determine the image file type from the name of the file and the
 * extension there within. If there is no extension or we can't match
 * the extension, then return NULL.
 */
ImageInfo *
_dxf_ImageInfoFromFileName ( char *basename)
{
    int  i;
    char *ep;
    int matches = 0;
    ImageInfo *r = NULL;

    /*
     * Find the last '.' in the file name after the last '/', if any.
     *   ep represents the extension pointer, if any.
     */
    ep = NULL;
    for ( i = strlen ( basename ) - 1;
          i >= 0;
          i-- )
        if ( basename[i] == '.' )
        {
            ep = &basename[i+1];
            break;
        }
        else if ( basename[i] == '/' )
            break;

    /*
     * If a file extension was found, attempt to find match in table.
     */
    if ( ep ) {
        for ( i=0 ; ImageTable[i].type != img_typ_illegal ; i++ ) {
	    int tmp = SearchStringList(ImageTable[i].extensions,ep);
	    if (tmp > matches) {
		r = &ImageTable[i];
		matches = tmp;
	    }
	}
    }
    return r; 
}

/* 
 * Find the Nth field in a string of characters where fields are seperated
 * by ':'.
 *
 * Return 1 if found, 0 if not. 
 *
 */
static int
NthImageExtension(char *extensions, int n, char *buf, int bufl)
{
    char  *s = extensions;

    buf[0] = '\0';
    if (s) {
	while (*s && n--) 
	{
	    while (*s && *s != ':')
		s++;
	    if (*s == ':')
		s++;
	}
	if (*s) 	/* We found the nth extension */
	{
	    n = 0;
	    while (*s && (*s != ':') && (n < bufl)  )
	    {
		buf[n++] = *s;
		s++;
	    }
   	    buf[n] = '\0';
	    return 1;
       } 
   }
   return 0;
}

/* 
 * Search a string "abcd:efgh:xyz" for a pattern that is separated by ':'
 * The input 'str' is assumed to be in lower case, the 'pattern' is 
 * copied to lower case so that a case insensitive search is done.
 *
 * The following should return > 0.
 *	SearchStringList("abcd:efgh:xyz","efgh")
 *	SearchStringList("abcd:efgh:xyz","XYZ")
 *	SearchStringList("abcd:efgh:xyz","XYZ asdf")
 * and the following 0 
 *	SearchStringList("abcd:efgh:xyz","ef")
 *	SearchStringList("abcd:efgh:xyz","abcd:ef")
 *	SearchStringList("abcd:efgh:xyz","XYZasdf")
 *
 * Return number of characters matched if found, 0 if not. 
 *
 */
static int
SearchStringList(char *str, char *pattern)
{
    int i, patlen;
    char cpy[256], buf[256], *p1, *p2;
    int matches = 0;
 
    if (str && pattern) {
	/* 
	 * Assume we we don't have any strings in the ImageTable[] that are
	 * larger than 31 characters.
	 */
	patlen = strlen(pattern);
	if (patlen > 255 || strlen(str) > 255) 
	    return(0);

	/* Copy the pattern into a lower case buffer */
	for (i=0 ; i<patlen ; i++) {
	    char b = pattern[i];
	    buf[i] = (isupper(b) ? tolower(b) : b);
	}
	buf[patlen] = '\0';

        strcpy(cpy,str);
        strcat(cpy,":");	/* A trailing colon cleans up the algorithm */
        p1 = cpy; 
	while (p1 && *p1) {
	    int len;
	    char c;
	    p2 = strchr(p1,':');
	    if (p2)
		*p2 = '\0';
	    len = strlen(p1);
	    if ((len <= patlen) && (len > matches)) {
		c = buf[len];	
		switch (c) {
		    /* Token in buf must be delimited by white space */ 
		    case ' ':
		    case '\0':
		    case '\n':
		    case '\t':
			if (!strncmp(buf,p1,len))
			    matches = len;
			break;
		    default:
			break;
		}
	    }
	    p1 = p2 ? p2+1 : p2;
    	}  
    }
    return matches;
}

static int  /* 0/1 on failure/success */
ValidImageExtension(char *ext, ImageType type)
{
    int i;

    if ( type != img_typ_im ) {
        for ( i = 0; ImageTable[i].type != img_typ_illegal; i++ ) {
            if ((ImageTable[i].type == type ) && 
                SearchStringList(ImageTable[i].extensions,ext))
                return 1; 
        }
        return 0;
    }
    else /* type == img_typ_im */ 
#ifdef HAVE_LIBMAGICK
        return _dxf_ValidImageExtensionIM( ext );
#else
        return 0;
#endif
}

/*
 * Given a file name, remove an extension (after last '.' before last 
 * '/') if it matches the given image type.
 */
void
_dxf_RemoveImageExtension(char *name, ImageType type)
{
    int i;

    /*
     * Find the last '.' in the file name before '/', if any.
     * Remove the extension in name if it is recognized.
     */
    for ( i = strlen (name) - 1; i >= 0; i-- )
        if (name[i] == '/')
            return;
        else if (name[i] == '.') 
            break;

    /*
     * Found a possible extension; see if it's an image ext we recognize
     */
    if ( ValidImageExtension(&name[i+1],type) )
        name[i] = '\0';
}

char *
_dxf_ExtractImageExtension ( char *name, ImageType type,
                             char *ext, int ext_size )
{
    int i;

    /*
     * Find the last '.' in the file name before '/', if any.
     * Extract the extension in name if it is recognized.
     */
    ext[0] = '\0';

    for ( i = strlen (name) - 1; i >= 0; i-- )
        if (name[i] == '/')
            return ext;
        else if (name[i] == '.') 
            break;

    /*
     * Found a possible extension; see if it's an image ext we recognize
     */
    if ( ValidImageExtension(&name[i+1],type) )
        strncat( ext, &name[i+1], ext_size );
    return ext;
}



char * _dxf_BuildImageFileName
    ( char        *buf,
      int         bufl, 
      char        *basename,	/* Does not have an extension */
      ImageType	  type,
      int	  framenum,	/* First frame in image file to write */
      int         selection)
{
    int  i;
    char extension[MAX_EXTENSION_LEN], framestr[16];
    int  found;

    /*
     * Find and append the requested extension.
     */
    found = 0;
    for ( i = 0; ImageTable[i].type != img_typ_illegal; i++) {
        if ( (type == ImageTable[i].type) &&
	     NthImageExtension(ImageTable[i].extensions,selection,
			   extension,MAX_EXTENSION_LEN) ) 
	{
		found = 1;
            	break;
	}
    }

    if ( ! found)  
    {
        DXSetError
            ( ERROR_INTERNAL,
              "Can't match type %d, selection %d in image table",
              type, selection);
        return ERROR;
    }

    if ((framenum >= 0) && (ImageTable[i].flags & APPENDABLE_FILES) == 0) {
	sprintf(framestr,".%d",framenum);
    } else {
	framestr[0] = '\0';
    }

    /* Make sure buffer is big enough, 2 for NULL and '.' */
    if ((strlen(basename) + strlen(framestr) + strlen (extension) + 2) > bufl)
        DXErrorReturn(ERROR_INTERNAL,"Insufficient storage space for filename");

    /*
     * Move the basename into the output buffer,
     *   skipping the extension if appropriate.
     */
    if ( !strcpy ( buf, basename) )
        DXErrorReturn ( ERROR_UNEXPECTED, "strncpy" );

    if ( !strcat ( buf, framestr) || !strcat(buf,".") )
        DXErrorReturn ( ERROR_UNEXPECTED, "strcat:framestr" );

    if ( !strcat ( buf, extension) )
        DXErrorReturn ( ERROR_UNEXPECTED, "strcat:extension" );
       
    return buf;
}

/*
 * DXExtract the 'member'th image of the given series, check that
 * the dimensions of the given member match those provided, and if
 * the member is a composite field, flatten the field into a 
 * non-partitioned image.
 *
 * If the member is a composite field and we return successfully,
 * then a new field has been created and '*created' is set to 1 to
 * indicate that the field was created.  If not created then, '*created'
 * is set to 0. 
 *
 * On success, returns an un-partitioned image Field. 
 * On failure, returns NULL and sets the error code.
 */
Field _dxf_GetFlatSeriesImage(Series image, int member, int width, int height, 
			int *created)
{
    Field img = NULL, m;
    SizeData  image_sizes;  /* Values as found in the image object itself */
    int was_created = 0;

    if ( NULL == ( m = (Field)DXGetSeriesMember(image, member, NULL)))
        DXErrorGoto
            ( ERROR_BAD_PARAMETER,
              "#10901" );
           /* start and end cannot be greater than numbert of series elements */

    switch (DXGetGroupClass((Group)m)) {

	default:
	    DXErrorGoto(ERROR_INTERNAL,"Encountered a non-series object");
	
  	case CLASS_COMPOSITEFIELD:
            if (!(img = _dxf_SimplifyCompositeImage((CompositeField)m)))
           	goto error;
	    was_created = 1;
            break;

        case CLASS_FIELD:
	    img = m;
            break;

    }

    /* Check to be sure the image dimensions match */
    if (!_dxf_GetImageAttributes((Object)img, &image_sizes ) )
        goto error;

    if ((image_sizes.width != width) || (image_sizes.height != height)) 
	DXErrorGoto(ERROR_BAD_PARAMETER, 
		"Series member does not have matching height and width");

    if (created) 
	*created = was_created;
    return img;

error:
   if (img && was_created) DXDelete((Object)img);
   if (created) *created = 0;
   return NULL;
}


Error 
_dxf_make_gamma_table(ubyte *gamma_table, float gamma)
{
    int i;
    float f, g;

    if (fabs(gamma)>1.0e-6)
	g = 1.0/gamma;
    else
	g = 1.0e+6;

    if (g>0) {
	gamma_table[0] = 0;
	gamma_table[255] = 255;
	for (i=1; i<255; i++) {
	    f = i/255.0;
	    gamma_table[i] = (ubyte)(255.0*pow(f, g));
	}
    } else {
	gamma_table[0] = 255;
	gamma_table[255] = 0;
	g = -g;
	for (i=1; i<255; i++) {
	    f = i/255.0;
	    gamma_table[255-i] = (ubyte)(255.0*pow(f, g));
	}
    }
    return OK;
}
