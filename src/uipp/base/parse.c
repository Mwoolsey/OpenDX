/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"

/*---------------------------------------------------------------------*
 |                                Parse.c                              |
 |                           -------------------                       |
 |                                                                     |
 *---------------------------------------------------------------------*/

#ifdef OS2
#include <stdlib.h>
#include <types.h>
#endif
#include <stdio.h>
#include <string.h>

#include <Xm/Xm.h>
#include "../widgets/MultiText.h"
#include "help.h"

#define STRCMP(a,b)  ((a) ? ((b) ? strcmp(a,b) : strcmp(a,"")) : \
			    ((b) ? strcmp("",b) : 0))

#define TAGLEN        3
#define DEFAULT_FONT  "-adobe-times-medium-r-normal--14*"
#define DEFAULT_COLOR "black"

int SendWidgetText (Widget w, FILE *infile, Bool ManPage, char *ref);
char *GetMathToken(FILE **mathfile, char *stoppat, char*buffer);


/* external routines */

extern void   Push();
extern char  *Pop(); 
extern void   DoRedrawText();
extern Stack *NewStack();
extern char  *Top();
extern void FreeSpotList(SpotList *); /* from helplist.c */
extern void InsertList(SpotList *, ListNodeType *); /* from helplist.c */



/*--------------------------------------------------------------------------*
 |                                GetMathToken                              |
 *--------------------------------------------------------------------------*/
char *GetMathToken(FILE **mathfile, char *stoppat, char*buffer)
{
 char rest[255],fmt[20];
 int  ch;
 sprintf(fmt,"%s%s%s%s%s%s","%[^",stoppat,"]","%[",stoppat,"]");
 while(((ch = fgetc(*mathfile)) != EOF) && ((ch == ' ') || ch == '\n')) ;
 if (feof(*mathfile)) return(NULL);
 ungetc(ch,*mathfile);
 if (fscanf(*mathfile,fmt,buffer,rest) == EOF)
    return(NULL);
 return(buffer);
}
 
/*--------------------------------------------------------------------------*
 |                                GetToken                                  |
 *--------------------------------------------------------------------------*/
char *GetToken (FILE **file, char *stoppat, char *buffer, int *newline, Bool ManPage)
{
  char  rest[255];
  char  fmt[20];
  int   n = 0;

 if (ManPage == FALSE) {
    sprintf(fmt, "%s%s%s%s%s%s", "%[^" ,stoppat, "]" , "%[" ,stoppat, "]");
    if (fscanf(*file, fmt, buffer, rest) == EOF) return (NULL);
    while (buffer[n] == ' ') n++;  /* skip any leading white space */
  } else {
    if (fscanf(*file, "%[^\n]%[\n]",buffer,rest) == EOF) return(NULL);
    if (strchr(buffer,'	') != NULL)
        *strchr(buffer,'\t') = '-';
    *newline = TRUE;
    }
  return(buffer + n);
}

/*--------------------------------------------------------------------------*
 |                              PushDefaults                                |
 *--------------------------------------------------------------------------*/
void PushDefaults(UserData *userdata)
{
  userdata->fontstack = NewStack();
  userdata->colorstack = NewStack();
  Push(userdata->fontstack,DEFAULT_FONT);
  Push(userdata->colorstack,DEFAULT_COLOR);
}


/*--------------------------------------------------------------------------*
 |                              SendWidgetText                              |
 *--------------------------------------------------------------------------*/
int SendWidgetText (Widget w, FILE *infile, Bool ManPage,char *ref)
{
  int              indentnum = 0;
  int              tabs[MAX_TAB_COUNT];
  int              tabCount;
  int              i,len;
  int              newline = FALSE;
  UserData         *userdata;
  ListNodeType	   *spot;
  char             buffer[255];
  char             psfile[127];
  char font[127];
  char color[127];
  char tabBuf[127];
  char link[127];
  char indent[15];
  Boolean          dpsCapable;
  char             *psBtmp;
  FILE             *ascfile;
  char             ascname[15];
  char             fascname[127];
  const char       *envptr;
  int              psBpos;

  strcpy(font, DEFAULT_FONT);
  strcpy(color, DEFAULT_COLOR);
  strcpy(tabBuf, "\0");
  strcpy(link, "\0");
  strcpy(indent, "0");
  strcpy(ascname, "\0");
  strcpy(fascname, "\0");

  XtVaGetValues(w,
		XmNuserData, &userdata,
		XmNdpsCapable, &dpsCapable,
		NULL);

  if (userdata->fontstack == NULL) {
      PushDefaults(userdata);
   }
  else { 
     if (userdata->spotlist != NULL)
       FreeSpotList(userdata->spotlist);
     len = userdata->fontstack->length;
     for (i = 0; i < len ; i++)
       Pop(userdata->fontstack);

     len = userdata->colorstack->length;
     for (i = 0; i < len ; i++)
       Pop(userdata->colorstack);
   }
 
  while (GetToken(&infile," \n",buffer,&newline,ManPage) != NULL)
    {
      if (buffer[0] == '#')
        {
          if (buffer[1] == '!')
	  {
            switch (buffer[2])
              {
              case 'R':                                
               userdata->getposition = TRUE;
               spot = (ListNodeType *) XtMalloc(sizeof(ListNodeType));
               spot->refname = XtMalloc(strlen(buffer) + 1);
               strcpy(spot->refname,buffer + TAGLEN);
               InsertList(userdata->spotlist,spot); 
               break;

              case 'F':                                 /* Font */
                font[strlen(buffer) - TAGLEN] = '\0';
                strncpy(font,buffer+TAGLEN,strlen(buffer) - TAGLEN);
                Push(userdata->fontstack,font);
                break;
              case 'C':                                 /* Color */
                color[strlen(buffer) - TAGLEN] = '\0';
                strncpy(color,buffer+TAGLEN,strlen(buffer) - TAGLEN);
                Push(userdata->colorstack,color);
                break;
              case 'L':                                 /* Link */
                link[strlen(buffer) - TAGLEN] = '\0';
                strncpy(link,buffer+TAGLEN,strlen(buffer) - TAGLEN);
                GetToken(&infile," \n",buffer,&newline,ManPage);
		switch (buffer[0])
		  {
		  case 'l':	userdata->linkType = LINK;		break;
                  case 'h':
                  case 'v':
		  case 's':	userdata->linkType = SPOTREF;		break;
		  case 't':	userdata->linkType = LINK;		break;
		  case 'f':	userdata->linkType = SPOTREF;	break;
		  case 'p':	userdata->linkType = LINK;		break;
		  default:	userdata->linkType = NOOP;		break;
		  }
                break;
               case 'P':
                psfile[strlen(buffer) - TAGLEN] = '\0';
                strncpy(psfile,buffer+TAGLEN,strlen(buffer) - TAGLEN);
 
		XtVaGetValues(w,
			      XmNdpsCapable, &dpsCapable,
			      NULL); 
 
                if (dpsCapable) { 
                  XmMultiTextAppendDPS(w, "BITMAP", Top(userdata->fontstack), 
                         Top(userdata->colorstack), indentnum, psfile, 257, 166,
                         XmMultiTextMakeLinkRecord(w, 
						 (LinkType)userdata->linkType, 
						 (LinkPosition)0, (char*)link));

                  if (userdata->getposition == TRUE) {
                     userdata->getposition = FALSE;
                     userdata->spotlist->tail->offset = XmMultiTextGetPosition(w);
                  }
                }
                else {
                 /* get ascii version of postscript files */
                   psBtmp  = strstr(psfile, ".ps.B");
                   if (psBtmp != NULL) {
                     psBpos = (abs(psBtmp - psfile));
		     /*
                     envptr = getenv("TEXTDIR");
		     */
		     envptr = GetHelpDirectory();;
                     ascname[0] = '\0';
                     strncat(ascname,psfile,psBpos);
                     fascname[0] = '\0';
                     sprintf(fascname,"%s%s%s%s",envptr,"/",ascname,".asc");
                     ascfile = fopen(fascname,"r");
                     if (ascfile == NULL)
                       printf("Error opening ascii version of DPS file %s \n",fascname);
                     else { 
                        if (strstr(ascname, "SVSPS") != NULL  ||
                            strstr(ascname, "svsps") != NULL) {
                          while (GetMathToken(&ascfile," \n",buffer) != NULL)
                          XmMultiTextAppendWord(w, buffer, Top(userdata->fontstack), 
                                    Top(userdata->colorstack), indentnum,
                                    XmMultiTextMakeLinkRecord(w, 
                                       (LinkType) userdata->linkType, 0, link));
                        }
                        else {
                          while (GetToken(&ascfile," \n",buffer,&newline,ManPage) != NULL) {
                             if (buffer[0] == '#')
                               {
                                 if (buffer[1] == '!')
                                   switch (buffer[2]) {
                                     case 'T':                                 /* Tab Stops */
                                        switch (buffer[3]) {
                                          case 'B':
                                            break;
                                          default:
                                            tabBuf[strlen(buffer) - TAGLEN] = '\0';
                                            strncpy(tabBuf,buffer+TAGLEN,strlen(buffer) - TAGLEN);
                                            tabCount = 0;
                                            for (i=0; i<strlen(tabBuf); i++)
                                              if (tabBuf[i] == ',') {
                                                sscanf(&tabBuf[i+1], "%d", &tabs[tabCount]);
                                                tabCount++;
                                              }
                                            XmMultiTextSetTabStops(w, tabs, tabCount);
                                            break;
                                        }
                                        break;
                                      case 'F':                                 /* Font */
                                        font[strlen(buffer) - TAGLEN] = '\0';
                                        strncpy(font,buffer+TAGLEN,strlen(buffer) - TAGLEN);
                                        Push(userdata->fontstack,font);
                                        break;
                                      case 'N':                                 /* New Line */
                                        indentnum = atoi(indent);
                                        XmMultiTextAppendNewLine(w, buffer, Top(userdata->fontstack), 
                                                                    indentnum);
                                        break;
                                      case 'E':                                 /* End */
                                        switch (buffer[3])
		                          {
		                            case 'F':     /* End Font */
		                            Pop(userdata->fontstack);
		                            strcpy(font, Top(userdata->fontstack));
		                            break;
                                          }
                                     default:
                                        ;
                                   }
                               } 
                               else
                                 { 
                                   if (STRCMP(buffer, "TAB") == 0)
              				XmMultiTextAppendWord(w, "	", Top(userdata->fontstack), 
                                         Top(userdata->colorstack), indentnum,
                                         XmMultiTextMakeLinkRecord(w, 
                                               (LinkType) userdata->linkType, 
					       0, link));
                                    else
                                         XmMultiTextAppendWord(w, buffer, Top(userdata->fontstack), 
                                            Top(userdata->colorstack), indentnum,
                                            XmMultiTextMakeLinkRecord(w, 
                                                (LinkType) userdata->linkType, 
						0, link));
                                  }
                            }
                        }
                        fclose(ascfile);
                     }
                   }
                }
                break;
              case 'N':                                 /* New Line */
                indentnum = atoi(indent);
                XmMultiTextAppendNewLine(w, buffer, Top(userdata->fontstack), 
                                                                    indentnum);
                break;
              case 'T':                                 /* Tab Stops */
               switch (buffer[3]) {
                 case 'B':
                    break;
                 default:
                   tabBuf[strlen(buffer) - TAGLEN] = '\0';
                   strncpy(tabBuf,buffer+TAGLEN,strlen(buffer) - TAGLEN);
                   tabCount = 0;
                   for (i=0; i<strlen(tabBuf); i++)
                     if (tabBuf[i] == ',') {
                       sscanf(&tabBuf[i+1], "%d", &tabs[tabCount]);
                       tabCount++;
                     }
                   XmMultiTextSetTabStops(w, tabs, tabCount);
                   break;
                }
                break;
              case 'I':                                 /* Indent */
                indent[strlen(buffer) - TAGLEN] = '\0';
                strncpy(indent,buffer+TAGLEN,strlen(buffer) - TAGLEN);
                break;
              case 'E':                                 /* End */
                switch (buffer[3])
		  {
		  case 'L':     /* End link */
		    userdata->linkType = NOOP;
		    link[0] = '\0';
		    break;
		  case 'F':     /* End Font */
		    Pop(userdata->fontstack);
		    strcpy(font, Top(userdata->fontstack));
		    break;
		  case 'C':     /* End Color */
		    Pop(userdata->colorstack);
		    strcpy(color, Top(userdata->colorstack)); 
		    break;
		  case 'T':     /* End Tab  */
		    break;
		  }
              default:
                ;
              }
	  }
	  else
	  {
	    XmMultiTextAppendWord(w, buffer, Top(userdata->fontstack),
				 Top(userdata->colorstack), indentnum,
				 XmMultiTextMakeLinkRecord
				    (w, (LinkType) userdata->linkType, 0, link)
				 );
	  }
        }
      else
        {
            indentnum = atoi(indent);
            if (STRCMP(buffer, "TAB") == 0)
              XmMultiTextAppendWord(w, "	", Top(userdata->fontstack), 
                                    Top(userdata->colorstack), indentnum,
                                    XmMultiTextMakeLinkRecord(w, 
                                       (LinkType) userdata->linkType, 0, link));
            else
              XmMultiTextAppendWord(w, buffer, Top(userdata->fontstack), 
                                    Top(userdata->colorstack), indentnum,
                                    XmMultiTextMakeLinkRecord(w, 
                                       (LinkType) userdata->linkType, 0, link));

            if (userdata->getposition == TRUE) {
                 userdata->getposition = FALSE;
                 userdata->spotlist->tail->offset = XmMultiTextGetPosition(w);
              }
                  
            if (ManPage == TRUE && newline == TRUE) {
                indentnum = atoi(indent);
                XmMultiTextAppendNewLine(w, "#!N", Top(userdata->fontstack), 
                                                                    indentnum);
            }

 
        }
     newline = FALSE;
    }

 return(0);
 
}
