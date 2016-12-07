/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#include <math.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <dx/dx.h>


struct colortable {
    char        colorname[80];
    RGBColor    colorvalue;
};

static  struct colortable ct[] = {
    { "aquamarine",              {0.4392157, 0.8588235, 0.5764706} },
    { "mediumaquamarine",        {0.1960784, 0.8000000, 0.6000000} },
    { "black",                   {0.0000000, 0.0000000, 0.0000000} },
    { "blue",                    {0.0000000, 0.0000000, 1.0000000} },
    { "cadetblue",               {0.3725490, 0.6235294, 0.6235294} },
    { "cornflowerblue",          {0.2588235, 0.2588235, 0.4352941} },
    { "darkslateblue",           {0.4196078, 0.1372549, 0.5568628} },
    { "lightblue",               {0.7490196, 0.8470588, 0.8470588} },
    { "lightsteelblue",          {0.5607843, 0.5607843, 0.7372549} },
    { "mediumblue",              {0.1960784, 0.1960784, 0.8000000} },
    { "mediumslateblue",         {0.4980392, 0.0000000, 1.0000000} },
    { "midnightblue",            {0.1843137, 0.1843137, 0.3098039} },
    { "navyblue",                {0.1372549, 0.1372549, 0.5568628} },
    { "navy",                    {0.1372549, 0.1372549, 0.5568628} },
    { "skyblue",                 {0.1960784, 0.6000000, 0.8000000} },
    { "slateblue",               {0.0000000, 0.4980392, 1.0000000} },
    { "steelblue",               {0.1372549, 0.4196078, 0.5568628} },
    { "coral",                   {1.0000000, 0.4980392, 0.0000000} },
    { "cyan",                    {0.0000000, 1.0000000, 1.0000000} },
    { "firebrick",               {0.5568628, 0.1372549, 0.1372549} },
    { "brown",                   {0.6470588, 0.1647059, 0.1647059} },
    { "gold",                    {0.8000000, 0.4980392, 0.1960784} },
    { "goldenrod",               {0.8588235, 0.8588235, 0.4392157} },
    { "mediumgoldenrod",         {0.9176471, 0.9176471, 0.6784314} },
    { "green",                   {0.0000000, 1.0000000, 0.0000000} },
    { "darkgreen",               {0.1843137, 0.3098039, 0.1843137} },
    { "darkolivegreen",          {0.3098039, 0.3098039, 0.1843137} },
    { "forestgreen",             {0.1372549, 0.5568628, 0.1372549} },
    { "limegreen",               {0.1960784, 0.8000000, 0.1960784} },
    { "mediumforestgreen",       {0.4196078, 0.5568628, 0.1372549} },
    { "mediumseagreen",          {0.2588235, 0.4352941, 0.2588235} },
    { "mediumspringgreen",       {0.4980392, 1.0000000, 0.0000000} },
    { "palegreen",               {0.5607843, 0.7372549, 0.5607843} },
    { "seagreen",                {0.1372549, 0.5568628, 0.4196078} },
    { "springgreen",             {0.0000000, 1.0000000, 0.4980392} },
    { "yellowgreen",             {0.6000000, 0.8000000, 0.1960784} },
    { "darkslategrey",           {0.1843137, 0.3098039, 0.3098039} },
    { "darkslategray",           {0.1843137, 0.3098039, 0.3098039} },
    { "dimgrey",                 {0.3294118, 0.3294118, 0.3294118} },
    { "dimgray",                 {0.3294118, 0.3294118, 0.3294118} },
    { "gray",                    {0.7529412, 0.7529412, 0.7529412} },
    { "grey",                    {0.7529412, 0.7529412, 0.7529412} },
    { "lightgrey",               {0.8274510, 0.8274510, 0.8274510} },
    { "lightgray",               {0.8274510, 0.8274510, 0.8274510} },
    { "khaki",                   {0.6235294, 0.6235294, 0.3725490} },
    { "magenta",                 {1.0000000, 0.0000000, 1.0000000} },
    { "maroon",                  {0.5568628, 0.1372549, 0.4196078} },
    { "orange",                  {0.8000000, 0.1960784, 0.1960784} },
    { "orchid",                  {0.8588235, 0.4392157, 0.8588235} },
    { "darkorchid",              {0.6000000, 0.1960784, 0.8000000} },
    { "mediumorchid",            {0.5764706, 0.4392157, 0.8588235} },
    { "pink",                    {0.7372549, 0.5607843, 0.5607843} },
    { "plum",                    {0.9176471, 0.6784314, 0.9176471} },
    { "red",                     {1.0000000, 0.0000000, 0.0000000} },
    { "indianred",               {0.3098039, 0.1843137, 0.1843137} },
    { "mediumvioletred",         {0.8588235, 0.4392157, 0.5764706} },
    { "orangered",               {1.0000000, 0.0000000, 0.4980392} },
    { "violetred",               {0.8000000, 0.1960784, 0.6000000} },
    { "salmon",                  {0.4352941, 0.2588235, 0.2588235} },
    { "sienna",                  {0.5568628, 0.4196078, 0.1372549} },
    { "tan",                     {0.8588235, 0.5764706, 0.4392157} },
    { "thistle",                 {0.8470588, 0.7490196, 0.8470588} },
    { "turquoise",               {0.6784314, 0.9176471, 0.9176471} },
    { "darkturquoise",           {0.4392157, 0.5764706, 0.8588235} },
    { "mediumturquoise",         {0.4392157, 0.8588235, 0.8588235} },
    { "violet",                  {0.3098039, 0.1843137, 0.3098039} },
    { "blueviolet",              {0.6235294, 0.3725490, 0.6235294} },
    { "wheat",                   {0.8470588, 0.8470588, 0.7490196} },
    { "white",                   {1.0000000, 1.0000000, 1.0000000} },
    { "yellow",                  {1.0000000, 1.0000000, 0.0000000} },
    { "greenyellow",             {0.5764706, 0.8588235, 0.4392157} },
    { "",                        {0.0,       0.0,       0.0} }, 
};



extern Error DXColorNameToRGB(char *, RGBColor *);

static Error FreeColorTable(Pointer);
static int   GetNextStringRedGreenBlue(FILE *, char *, 
				       float *, float *, float *);


Error DXColorNameToRGB(char *colorstr, RGBColor *colorvec)
{
    struct colortable *ctread=NULL, *cp;
    int count, i, numread, matchedcolor;
    float r, g, b;
    char newstring[30], laststring[80];
    char compactstring[80];
    char *rootstring, colorfile[100];
    Private cachedtable;
    FILE *in;
  
  
    /*  get color name  */
    /*  convert color name to all lower case and remove spaces */
    count = 0;
    i = 0;
    while ( i<29 && colorstr[i] != '\0') {
	if (isalpha(colorstr[i])) {
	    if (isupper(colorstr[i]))
		newstring[count] = tolower(colorstr[i]);
	    else
		newstring[count] = colorstr[i];
	    count++;
	} 
	else {
	    if (colorstr[i] != ' ') {
		newstring[count] = colorstr[i];
		count++;
	    }
	}
	i++;
    }
    newstring[count]='\0';
    matchedcolor = 0;
    
    cachedtable = (Private)DXGetCacheEntry("rgbcolortable",  0, 0);
    if (!cachedtable) {
	/* better make the table */
	/* first check the environment variable DXCOLORS */
	rootstring = (char *)getenv("DXCOLORS");
	if (rootstring) {
	    sprintf(colorfile,"%s",rootstring);
	    in = fopen(colorfile, "r");
	    if (in) goto got_colorfile;
	}
	/* if we haven't succeeded yet, try DXEXECROOT */
	rootstring = (char *)getenv("DXEXECROOT");
	if (rootstring) {
	    sprintf(colorfile,"%s/lib/colors.txt",rootstring);
	    in = fopen(colorfile, "r");
	    if (in) goto got_colorfile;
	}
	/* if we haven't succeeded yet, try DXROOT */
	rootstring = (char *)getenv("DXROOT");
	if (rootstring) {
	    sprintf(colorfile,"%s/lib/colors.txt",rootstring);
	    in = fopen(colorfile, "r");
	    if (in) goto got_colorfile;
	}
	/* if we still haven't succeeded, try the system version */
	in = fopen("/usr/local/dx/lib/colors.txt","r");
	if (in) goto got_colorfile;
	
	/* all has failed; use our old table */
	ctread = ct;
	
      got_colorfile:
	if (in) {
	    /* read the system table */
	    numread = 0;
	    strcpy(laststring,"");
	    while (GetNextStringRedGreenBlue(in, compactstring, &r, &g, &b)) {
		if (strcmp(compactstring,laststring)) {
		    numread++;
		    strcpy(laststring, compactstring);
		}
	    }
	    /* the table MUST BE IN GLOBAL MEMORY FOR THE CACHE TO WORK */
	    ctread = (struct colortable *)DXAllocate
		((numread+1) * sizeof(struct colortable));
	    if (!ctread) 
		goto error;
	    
	    cp = ctread;
	    
	    /* rewind the file */
	    fseek(in,0,0);
	    
	    strcpy(laststring,"");
	    while (GetNextStringRedGreenBlue(in, compactstring, &r, &g, &b)) {
		if (strcmp(compactstring,laststring)) {
		    strcpy(cp->colorname, compactstring);
		    cp->colorvalue = DXRGB(r,g,b);
		    strcpy(laststring, compactstring);
		    cp++;
		}
	    }
            /* need to set the last "colorname" to a null string */
            strcpy(cp->colorname,"");
            cp->colorvalue = DXRGB(0.0, 0.0, 0.0);

	    fclose(in);
	}
    }
    else {
	/* we found the table in the cache*/
	ctread = (struct colortable *)DXGetPrivateData(cachedtable);
	if (!ctread) 
	    goto error; 
    }
    
    /*  run through the color table to find the rgb value */
    for (cp = ctread; strcmp(cp->colorname,""); cp++) {
	if (strcmp(cp->colorname,newstring)) 
	    continue;
	else {
	    *colorvec = cp->colorvalue ;
	    matchedcolor = 1;
	    continue;
	}
    } 
    
    /* put the thing in the cache */
    if (!cachedtable) {
	cachedtable = DXNewPrivate((Pointer)ctread, FreeColorTable); 
	DXReference((Object)cachedtable);
	if (!DXSetCacheEntry((Object)cachedtable, CACHE_PERMANENT, "rgbcolortable", 0, 0))
	    goto error;
	DXDelete((Object)cachedtable);
    } 
    else {
	/* delete the table since it has an extra reference count XXX */
	DXDelete((Object)cachedtable); 
    }
    
    if (!matchedcolor) {
	DXSetError(ERROR_BAD_PARAMETER, "#11760", newstring);
	return ERROR;
    }
    
    return OK;

  error: 
    
    /* delete the table since it has an extra reference count XXX */
    DXDelete((Object)cachedtable); 
    return ERROR;
} 



static Error FreeColorTable(Pointer ptr)
{
    struct colortable *ctp = ptr;
    
    /* do not free the pointer if we used the static color table ct[] */
    if (ctp != ct)
	DXFree((Pointer)ctp);

    return OK;
}


static int GetNextStringRedGreenBlue(FILE *in, char *compactstring, float *r,
				     float *g, float *b)
{
    char string[80];
    int i, j;
    
    /* read the colorname (at least the first word of it) */
    if (fscanf(in,"%s", string) != EOF) {
        j=0;
 more_string:
	i=0;
	while (i < 80 && string[i] != '\0') {
	   if (string[i] != ' ') {
	       compactstring[j] = tolower(string[i]);
	       j++;
	   }
	   i++;
	}
	/* now read the next token. Might be more color name, or 
	   might be red color */
	if(fscanf(in,"%s", string) != EOF) {
	    if ((isdigit(string[0]))||(string[0] == '.')) {
	        /* it's red */
	        *r = atof(string);
	        if(fscanf(in,"%f %f \n", g, b) != 2) {
                    return 0;
                }
	    }
	    else {
	       /* it's more color name */
	       goto more_string;
	    }
        } 
	/* null terminate */
	compactstring[j] = '\0';
	return 1;
    }
    /* EOF */
    return 0;
}
