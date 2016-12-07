/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dxmods/writeimage.c,v 1.11 2006/01/03 17:02:26 davidt Exp $
 */

#include <dxconfig.h>

#if defined(HAVE_STRING_H)
#include <string.h>
#endif

/***
MODULE:
    WriteImage
SHORTDESCRIPTION:
    writes an image to a file.
CATEGORY:
    Import and Export
INPUTS:
    image;  image or image series;  NULL;        the image to write
    file;   string;                 image.rgb;   file name 
    format; string;                 .rgb;        format of file
    frame;  integer;                current + 1; frame to write
OUTPUTS:
BUGS:
    There is currently no way to overwrite an existing frame in the middle
    of a file.
AUTHOR:
    nancy s collins
END:
***/

#define I_image  in[0] 
#define I_file   in[1] 
#define I_format in[2] 
#define I_frame  in[3] 

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ctype.h>
#include <dx/dx.h>
#include "_helper_jea.h"
#include "_rw_image.h"
#include <sys/types.h>
#include <signal.h>


/* FIXME: this belongs in the arch.h file */
#if defined(ibmpvs)  || defined(os2) || defined(intelnt) || defined(WIN32)
#undef HAS_POSIX_SIGNALS 
#else
#define HAS_POSIX_SIGNALS 
#endif

#if DXD_POPEN_OK && DXD_HAS_LIBIOP
#define popen popen_host
#define pclose pclose_host
#endif


#define HANDLE_SIGPIPE 0

#if defined(intelnt) || defined(WIN32)
#define	SIGPIPE	SIGILL
#define	popen	_popen
#define	pclose	_pclose
#endif

static float GammaFromFormat(char *s);
static char* CompressionFromFormat(char *s);
static unsigned int QualityFromFormat(char *s);
static unsigned int ReductionFromFormat(char *s);

#if HANDLE_SIGPIPE
static int pipe_error_happened;
static void PipeError()
{
    pipe_error_happened = 1;
}
#endif /* HANDLE_SIGPIPE */

int
m_WriteImage ( Object *in, Object *out )
{
    char   *exclamation=NULL, *filename, *format, basename[MAX_IMAGE_NAMELEN];
    int    deleteable = 0;
    int    transposed;
    RWImageArgs iargs;
    ImageWriteFunction f;	/* Function to write the image to disk */
    ImageInfo *imginfo;
    int r;
#if defined(HAS_POSIX_SIGNALS)
    struct sigaction action, oaction;
#else
    void (*oaction)();
#endif /* HAS_POSIX_SIGNALS */

#if ( CAN_HAVE_ARRAY_DASD == 1 )
    int Output_to_ADASD = 0;
#else
#define    Output_to_ADASD 0
#endif

    iargs.image = NULL;
    iargs.pipe = NULL;
    iargs.compression = NULL;

    if ( !I_image )
	DXErrorGoto2
            ( ERROR_MISSING_DATA, "#10000", "'image'" )
    else
    {
        switch ( iargs.imgclass = DXGetObjectClass ( I_image ) )
        {
            case CLASS_FIELD:
                iargs.image      = (Field)I_image;
		/*
		 * Check to be sure the provided image(s) are images. 
		 */
		if ( !_dxf_ValidImageField ( (Field)iargs.image) )
		    DXErrorGoto ( ERROR_DATA_INVALID, 
				"'image' Field is not an image" );
                break;

            case CLASS_GROUP:
                switch ( iargs.imgclass = DXGetGroupClass ( (Group)I_image ) )
                {
		    Object o;
		    int i;
                    case CLASS_SERIES:
                	iargs.image      = (Field)I_image;
			i=0;
			while ((o=DXGetEnumeratedMember((Group)I_image,i++,NULL)))
			    if ( !_dxf_ValidImageField ( (Field)o) )
		                    DXErrorGoto ( ERROR_DATA_INVALID, 
               		                 "series 'image' contains non-image" );
                        break;

                    case CLASS_COMPOSITEFIELD:
                        if ( ( iargs.image = _dxf_SimplifyCompositeImage
                                           ( (CompositeField)I_image ) ) )
                            deleteable = 1;
                        else
                            goto error;
                        break;

                    default:
                        DXErrorGoto ( ERROR_BAD_PARAMETER, 
				"'image' Field is not an image" );
                }
                break;
    
            default:
                DXErrorGoto
                    ( ERROR_BAD_PARAMETER, "'image' is not an image" )
        }

    }

    /*
     * Check to be sure the provided image(s) have the correct deltas
     * for an image (so that the pixels are written out correctly). 
     */
    if (!_dxf_CheckImageDeltas((Object)iargs.image, &transposed))
	goto error;
    if (transposed) 
	DXErrorGoto ( ERROR_DATA_INVALID,"'image' Field has transposed deltas" );

    /*
     * Determine the filename.  If the filename parameter is given then
     * we use it as basename.   If it is not specified then we copy "image"
     * into basename as the default name.  
     */
    if ( I_file ) {
	char *p;
        if ( !DXExtractString ( I_file, &filename ) || ( filename == NULL ) )
            DXErrorGoto2
                ( ERROR_MISSING_DATA, "#10200", "'name'" )

	p = filename;
	while (*p && (*p == ' ' || *p == '\t')) p++;
	exclamation = p;
#if !defined(DXD_OS_NON_UNIX)
        if ( (*p != '!') && strchr ( filename, ':' ) )
#if ( CAN_HAVE_ARRAY_DASD == 1 )
            Output_to_ADASD = 1;
        else 
            Output_to_ADASD = 0;
#else
            DXErrorGoto
                ( ERROR_NOT_IMPLEMENTED,
                  "cannot support a disk array 'name' on this architecture" );
#endif
#endif
	if (strlen(filename) > MAX_IMAGE_NAMELEN)
	    DXErrorGoto(ERROR_DATA_INVALID, "output file name length too large.");
    	strcpy(basename,filename);
    } else {
	filename = NULL;
        strcpy(basename,"image");
    }


    /*
     * Assemble filename from input strings: filename and format.
     */

    iargs.gamma = 2.0;

    if ( I_format )
    {
        if ( !DXExtractString ( I_format, &format ) || ( format == NULL ) )
            DXErrorGoto2
                ( ERROR_MISSING_DATA, "#10200", "'format'" )

        if (!(imginfo = _dxf_ImageInfoFromFormat(format)))
            DXErrorGoto3
                ( ERROR_BAD_PARAMETER, "#10210", format, "the file format" )

        iargs.imgtyp = imginfo->type;

	iargs.gamma = GammaFromFormat(format);
	iargs.compression = CompressionFromFormat(format);
	iargs.quality = QualityFromFormat(format);
	iargs.reduction = ReductionFromFormat(format);
	
        if (Output_to_ADASD) {
	    if (!(imginfo->flags & ADASD_OK)) 
                DXErrorGoto2 ( ERROR_NOT_IMPLEMENTED,
                  "format '%s' is not supported on the PVS disk array",format )
        } else if (imginfo->flags & ADASD_ONLY) {
                DXErrorGoto2 ( ERROR_NOT_IMPLEMENTED,
                  "format '%s' is only supported on the PVS disk array",format )
        }
    }
    else
    {
        format = NULL;

	/* 
	 * If the user specified a filename, try and determine the output
	 * image format from the given name.
	 */
	if (filename)	
     	   imginfo = _dxf_ImageInfoFromFileName(filename);
	else
	   imginfo = NULL;

        if ( Output_to_ADASD )
        {
            if ( !imginfo ) {
	    	/* Can't determine format from args, default to "fb" */
                iargs.imgtyp = img_typ_fb;
		imginfo = _dxf_ImageInfoFromType(img_typ_fb);

                if ( imginfo == NULL )
                    DXErrorGoto 
                        ( ERROR_DATA_INVALID, "image type lookup failed" );
	    } else
		iargs.imgtyp = imginfo->type;
	
            if ( (imginfo->flags & ADASD_OK) == 0)
                DXErrorGoto ( ERROR_NOT_IMPLEMENTED,
                 "The file format indicated in 'name' is not supported on the disk array." )
        }
        else
        {
            if ( !imginfo ) {	
	    	/* Can't determine format from args, default to "rgb" */
                iargs.imgtyp = img_typ_rgb;
		imginfo = _dxf_ImageInfoFromType(img_typ_rgb);

                if ( imginfo == NULL )
                    DXErrorGoto 
                        ( ERROR_DATA_INVALID, "image type lookup failed" );
	    } else
		iargs.imgtyp = imginfo->type;

            if ( imginfo->flags & ADASD_ONLY) 
                DXErrorGoto ( ERROR_NOT_IMPLEMENTED,
                 "The file format indicated in 'name' is only supported on the PVS disk array." )
        }
    }


    if ( I_frame )
    {
        if ( !DXExtractInteger ( I_frame, &iargs.startframe ) || 
		(iargs.startframe < 0))
            DXErrorGoto2 ( ERROR_DATA_INVALID, "#10030", "'frame'" )
    }
    else
       iargs.startframe = VALUE_UNSPECIFIED;

    f = imginfo->write; 
    if (!f)
	DXErrorGoto ( ERROR_UNEXPECTED, "Image write function not available" );

    if (exclamation && *exclamation == '!') {		/* Piping output */
#if DXD_POPEN_OK
	char *p = ++exclamation;
	if ((imginfo->flags & PIPEABLE_OUTPUT) == 0) {
            DXErrorGoto( ERROR_BAD_PARAMETER, 
		"Given format does not support the '!' option");
	}
	iargs.pipe = popen(p,"w");
	if (!iargs.pipe) {
            DXSetError( ERROR_BAD_PARAMETER, 
			"Error executing command '%s'",p);
	    goto error;
	}
        iargs.basename = NULL;
#else
	DXSetError(ERROR_BAD_PARAMETER, 
		"Cannot support the '!' option on this platform");
	goto error;
#endif
    } else  {		/* Putting output in a file */
	/*
	 * remove the extension in basename if it is recognized
	 * and is an acceptable extension for imgtyp.
	 */
	unsigned int original_length = 0;
	original_length= strlen(basename);
	_dxf_RemoveImageExtension(basename,iargs.imgtyp);
        iargs.extension=NULL;
	if(strlen(basename)!=original_length)
	   {
		iargs.extension=basename + strlen(basename) + 1 ; 
	   }
        iargs.basename = basename;
    }

    iargs.adasd = Output_to_ADASD;
    iargs.format = format;

   /* 
    * Handle pipe write errors which can only be caught when we do
    * a write.  We can't do a test write above since we don't know
    * what/if the ignorable characters are.
    */ 
#if DXD_POPEN_OK
    if (iargs.pipe) {
#if defined(HAS_POSIX_SIGNALS)
# if HANDLE_SIGPIPE
	action.sa_handler = PipeError;
	pipe_error_happened = 0;
# else
	action.sa_handler = SIG_IGN;
# endif	/*  HANDLE_SIGPIPE */
	sigemptyset(&action.sa_mask);
	action.sa_flags = 0;
	sigaction(SIGPIPE, &action, &oaction);	
#else
	oaction = signal(SIGPIPE, SIG_IGN);
#endif /* HAS_POSIX_SIGNALS */
    }
#endif /* DXD_POPEN_OK */

    r = (*f)(&iargs);

#if DXD_POPEN_OK
    if (iargs.pipe) {

#if defined(HAS_POSIX_SIGNALS)
	sigaction(SIGPIPE, &oaction, NULL);
#else
	signal(SIGPIPE, oaction);
#endif /* HAS_POSIX_SIGNALS */

#if HANDLE_SIGPIPE
	if (pipe_error_happened) {
	    pipe_error_happened = 0;
	    goto error;
	}
#endif /* HANDLE_SIGPIPE */
    }
#endif /* DXD_POPEN_OK */

    if (!r)
	goto error;

    if ( deleteable ) DXDelete ( (Object) iargs.image );
#if DXD_POPEN_OK
    if (iargs.pipe) pclose(iargs.pipe);
#endif
	if (iargs.compression) DXFree (iargs.compression);
    return OK;

error:
    if ( deleteable && iargs.image) DXDelete ( (Object) iargs.image );
#if DXD_POPEN_OK
    if (iargs.pipe) pclose(iargs.pipe);
#endif
    return ERROR;
}


static float GammaFromFormat(char *s)
{
    char *p, *q;
    char buf[200];
    int len;
	
    p = (char *)strstr(s, "gamma");
    if (!p)
		return 2.0;
    p = (char *)strstr(p, "=");
    if (!p)
		return 2.0;
    p++;
    for (q=p; *q && isspace(*q); q++);
    for (p=q, len=0; *q && !isspace(*q); q++, len++);
    len++;
    if (len>=200)
		return 2.0;
    strncpy(buf, p, len);
    buf[len] = '\0';
    return (float)atof(buf);
}

/* Allocates the return. Caller must DXFree the return */

static char* CompressionFromFormat(char *s)
{
    char *p, *q;
    char *buf;
    int len;
	
    p = (char *)strstr(s, "compression");
    if (!p)
		return NULL;
    p = (char *)strstr(p, "=");
    if (!p)
		return NULL;
    p++;
    for (q=p; *q && isspace(*q); q++);
    for (p=q, len=0; *q && !isspace(*q); q++, len++);
    len++;
    if (len>=200)
		return NULL;
	buf = DXAllocate(len);
    strncpy(buf, p, len-1);
    buf[len-1] = '\0';
    return buf;
}

static unsigned int QualityFromFormat(char *s)
{
    char *p, *q;
    char buf[200];
    int len;
	
    p = (char *)strstr(s, "quality");
    if (!p)
		return 0;
    p = (char *)strstr(p, "=");
    if (!p)
		return 0;
    p++;
    for (q=p; *q && isspace(*q); q++);
    for (p=q, len=0; *q && !isspace(*q); q++, len++);
    len++;
    if (len>=200)
		return 0;
    strncpy(buf, p, len);
    buf[len] = '\0';
    return (float)atoi(buf);
}

static unsigned int ReductionFromFormat(char *s)
{
    char *p, *q;
    char buf[200];
    int len;
	
    p = (char *)strstr(s, "resize");
    if (!p)
		return 0;
    p = (char *)strstr(p, "=");
    if (!p)
		return 0;
    p++;
    for (q=p; *q && isspace(*q); q++);
    for (p=q, len=0; *q && !isspace(*q); q++, len++);
    len++;
    if (len>=200)
		return 0;
    strncpy(buf, p, len);
    buf[len] = '\0';
    return (float)atoi(buf);
}
	
