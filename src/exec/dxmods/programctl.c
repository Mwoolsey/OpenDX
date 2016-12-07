/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
#include <dx/dx.h> 

#include <dxconfig.h>
#include <ctype.h>
#if defined(HAVE_STRING_H)
#include <string.h>
#endif
#include "plot.h"
#include "superwin.h"

typedef struct
{
  int  delete;
  char *message;
  char *messageType;
  char *major;
  char *minor;
} PendingDXLMessage;

static Error ManagePanels(Object *, char *);
static Error delete_PendingDXLMessage(Pointer);
static Error SendPendingDXLMessage(Private);
static Error SendPendingMessage(char *, char *);

extern Private DXNewPrivate(Pointer, Error (*delete)(Pointer)); /* from libdx/private.c */

Error m_ManageSequencer(Object *in)
{
  int open=0;
  
  if (in[0])
    {
      if (!DXExtractInteger(in[0], &open))
	{
	  DXSetError(ERROR_BAD_PARAMETER,"open must be 0 or 1");
	  goto error;
	}
      if ((open != 0)&&(open != 1))
	{
	  DXSetError(ERROR_BAD_PARAMETER,"open must be 0 or 1");
	  goto error;
	}
    }
  
  if (open) 
    DXUIMessage("LINK", "open sequencer");
  else
    DXUIMessage("LINK", "close sequencer");
  
  return OK;
 error:
  return ERROR;	
}

Error m_InteractionMode(Object *in)
{
  char *mode, *tmpmode;
  int i, count;
  
  if (!in[0])
    mode = "none";
  else
    {
      if (!DXExtractString(in[0],&tmpmode))
	{
	  DXSetError(ERROR_BAD_PARAMETER,"mode must be a string");
	  goto error;
	}
      
      mode = DXAllocate(strlen(tmpmode)+1);
      if (!mode)
	goto error;
      
      count = 0;
      i = 0;
      while (i<100 && tmpmode[i] != '\0')
	{
	  if (isalpha(tmpmode[i]))
	    {
	      if (isupper(tmpmode[i]))
		mode[count] = tolower(tmpmode[i]);
	      else
		mode[count] = tmpmode[i];
	      count++;
	    }
          i++;
	}
      mode[count]='\0';
    }
  
  if (!strcmp(mode,"camera"))
    DXUIMessage("LINK", "interactionMode camera");
  else if (!strcmp(mode,"cursors"))
    DXUIMessage("LINK", "interactionMode cursors");
  else if (!strcmp(mode,"pick"))
    DXUIMessage("LINK", "interactionMode pick");
  else if (!strcmp(mode,"navigate"))
    DXUIMessage("LINK", "interactionMode navigate");
  else if (!strcmp(mode,"panzoom"))
    DXUIMessage("LINK", "interactionMode panzoom");
  else if (!strcmp(mode,"roam"))
    DXUIMessage("LINK", "interactionMode roam");
  else if (!strcmp(mode,"rotate"))
    DXUIMessage("LINK", "interactionMode rotate");
  else if (!strcmp(mode,"zoom"))
    DXUIMessage("LINK", "interactionMode zoom");
  else if (!strcmp(mode,"none"))
    DXUIMessage("LINK", "interactionMode none");
  else
    {
      DXSetError(ERROR_BAD_PARAMETER,"action must be one of \"camera\", \"cursors\", \"pick\", \"navigate\", \"panzoom\", \"roam\", \"rotate\", \"zoom\", or \"none\"");
      goto error;
    }
  return OK;
 error:
  return ERROR;
  
}


Error m_Execute(Object *in)
{
  char *action, *tmpaction;
  int i, count;
  char buf[20];
  
  if (!in[0])
    action = "end";
  else
    {
      if (!DXExtractString(in[0],&tmpaction))
	{
	  DXSetError(ERROR_BAD_PARAMETER,"action must be a string");
	  goto error;
	}
      
      action = DXAllocate(strlen(tmpaction)+1);
      if (!action)
	goto error;
      
      count = 0;
      i = 0;
      while (i<100 && tmpaction[i] != '\0')
	{
	  if (isalpha(tmpaction[i]))
	    {
	      if (isupper(tmpaction[i]))
		action[count] = tolower(tmpaction[i]);
	      else
		action[count] = tmpaction[i];
	      count++;
	    }
          i++;
	}
      action[count]='\0';
    }
  
#if 0
  if (!strcmp(action,"once"))
    DXUIMessage("LINK", "execute once");
  else if (!strcmp(action,"onchange"))
    DXUIMessage("LINK", "execute onchange");
  else if (!strcmp(action,"end"))
    DXUIMessage("LINK", "execute end");
  else
    {
      DXSetError(ERROR_BAD_PARAMETER,"action must be one of \"once\", \"on change\" or \"end\"");
      goto error;
    }
#endif

  if (!strcmp(action,"once"))
    sprintf(buf, "execute once");
  else if (!strcmp(action,"onchange"))
    sprintf(buf, "execute onchange");
  else if (!strcmp(action,"end"))
    sprintf(buf, "execute end");
  else
    {
      DXSetError(ERROR_BAD_PARAMETER,"action must be one of \"once\", \"on change\" or \"end\"");
      goto error;
    }

  SendPendingMessage(buf, "default");

  return OK;
 error:
  return ERROR;
  
}


Error m_ManageColormapEditor(Object *in)
{
  
  if (!ManagePanels(in, "colormap"))
    goto error;
  
  return OK;
  
 error:
  return ERROR;
}

Error ManagePanels(Object *in, char *paneltype)
{
  char *name = "", *openname, *how, *buf = NULL;
  int i, j, open, nummaps, numflags, num_in_open_list;
  int *open_list=NULL, rank, shape[10];
  Type type=TYPE_STRING;
  char messageid[20];
  
  if (!strcmp(paneltype,"colormap")&& 
      !strcmp(paneltype,"image")&&
      !strcmp(paneltype,"controlpanel"))
    {
      DXSetError(ERROR_INTERNAL,"unrecognized paneltype %s", paneltype);
      goto error;
    } 
  
  sprintf(messageid,"default");
  /* can be a single string or a list of strings */  
  if (in[0])
    {
      if (!DXExtractNthString(in[0], 0, &name))
	{
	  DXSetError(ERROR_BAD_PARAMETER,"name must be a string");
	  goto error;
	}
      /* how many strings are there */
      if (!_dxfHowManyStrings(in[0], &nummaps)) 
        goto error;
    }
  
  /* it can be an integer, integer list, or string list */  
  if (in[1])
    {
      if (DXGetObjectClass(in[1])==CLASS_ARRAY)
	{
	  if (!DXGetArrayInfo((Array) in[1], &numflags, &type, NULL, &rank, shape))
	    goto error;
	  if (type == TYPE_INT)
	    {
	      if (!(rank == 0)||((rank == 1)&&(shape[0]==1)))
		{
		  DXSetError(ERROR_DATA_INVALID,
			     "open must be an integer list or string list");
		  goto error;
		}
	      open_list = (int *)DXGetArrayData((Array) in[1]);
	    }
	  else if (type!=TYPE_STRING)
	    {
	      DXSetError(ERROR_DATA_INVALID,
			 "open must be an integer list or string list");
	      goto error;
	    }
          if (numflags==1)
	    DXExtractInteger(in[1], &open);
	}
      else if (!(DXGetObjectClass(in[1])==CLASS_STRING))
	{
	  DXSetError(ERROR_DATA_INVALID,
		     "open must be an integer list or string list");
	  goto error;
        }
    }
  else
    {
      /* default */
      open=0;
      numflags=1;
      type = TYPE_INT;
    }
  
  
  if (!strcmp(paneltype,"colormap")|| !strcmp(paneltype,"image"))
    {
      if (in[2])
	{
	  if (!DXExtractString(in[2], &how))
	    {
	      DXSetError(ERROR_BAD_PARAMETER,"how must be either \"title\" or \"label\"");
	      goto error;
	    }
	  if (!strcmp(how,"title")&&(!strcmp(how,"label")))
	    {
	      DXSetError(ERROR_BAD_PARAMETER,"how must be either \"title\" or \"label\"");
	      goto error;
	    }
	}
      else 
	how="title";
    }
  
  buf = DXAllocate(strlen(name)+50);
  if (!buf)
    goto error; 
  
  
  if (type == TYPE_INT) 
    {
      if ((numflags != 1)&&(numflags != nummaps))
	{
	  DXSetError(ERROR_DATA_INVALID,
		     "open must be one integer or must be a list which matches name");
	  goto error; 
	}
      if (numflags == 1) 
	{
	  if (open)
	    {
	      if (in[0])
		{
                  for (j=0; j<nummaps; j++)
		    {
		      DXExtractNthString(in[0],j,&name);
                      if (!strcmp(paneltype,"colormap"))
		        sprintf(buf, "open colormapEditor %s=%s", how, name);
                      else if (!strcmp(paneltype,"image"))
		        sprintf(buf, "open image %s=%s", how, name);
                      else 
		        sprintf(buf, "open controlpanel %s", name);
                      sprintf(messageid,"%i", j);
                      if (!SendPendingMessage(buf, (char *) &messageid))
			goto error;
		    }
		}
	      else
                {
                  if (!strcmp(paneltype,"colormap"))
		    sprintf(buf, "open colormapEditor");
                  else if (!strcmp(paneltype,"image"))
		    sprintf(buf, "open image");
                  else
		    sprintf(buf, "open controlpanel");
		  if (!SendPendingMessage(buf, messageid))
		    goto error;
                }
	    }
	  else
	    {
	      if (in[0])
		{
                  for (j=0; j<nummaps; j++)
		    {
		      DXExtractNthString(in[0],j,&name);
                      if (!strcmp(paneltype,"colormap"))
			sprintf(buf, "close colormapEditor %s=%s", how,name);
                      else if (!strcmp(paneltype,"image"))
			sprintf(buf, "close image %s=%s", how,name);
                      else 
			sprintf(buf, "close controlpanel %s", name);
                      sprintf(messageid,"%d",j);
                      if (!SendPendingMessage(buf, messageid))
			goto error;
		    }
		}
	      else
		{
                  if (!strcmp(paneltype,"colormap"))
		    sprintf(buf, "close colormapEditor");
                  else if (!strcmp(paneltype,"image"))
		    sprintf(buf, "close image");
                  else  
		    sprintf(buf, "close controlpanel");
		  if (!SendPendingMessage(buf, messageid))
		    goto error;
		}
	    }
	}
      else
	{
	  for (j=0; j<nummaps; j++)
            {
	      open = open_list[j];
	      DXExtractNthString(in[0],j,&name);
	      if (open)
                {
		  if (!strcmp(paneltype,"colormap"))
		    sprintf(buf, "open colormapEditor %s=%s", how,name);
		  else if (!strcmp(paneltype,"image"))
		    sprintf(buf, "open image %s=%s", how,name);
		  else
		    sprintf(buf, "open controlpanel %s", name);
                }
	      else
                {
		  if (!strcmp(paneltype,"colormap"))
		    sprintf(buf, "close colormapEditor %s=%s", how,name);
		  else if (!strcmp(paneltype,"image"))
		    sprintf(buf, "close image %s=%s", how,name);
		  else
		    sprintf(buf, "close controlpanel %s", name);
                }
              sprintf(messageid,"%d",j);
	      if (!SendPendingMessage(buf, messageid))
		goto error;
            }
	}
    }
  else
    /* else "open" is a string list, of the ones which should be
       open. All others should be closed */
    {
      if (!_dxfHowManyStrings(in[1], &num_in_open_list)) 
	goto error;
      for (i=0; i<nummaps; i++) 
	{
          DXExtractNthString(in[0],i,&name);
          open=0;
          for (j=0; j<num_in_open_list; j++)
            {
	      DXExtractNthString(in[1],j,&openname);
	      if (!strcmp(name,openname)) 
		{
                  open=1;
                  break;
		}
            }
	  if (open)
	    {
	      if (!strcmp(paneltype,"colormap"))
		sprintf(buf, "open colormapEditor %s=%s", how,name);
	      else if (!strcmp(paneltype,"image"))
		sprintf(buf, "open image %s=%s", how,name);
	      else
		sprintf(buf, "open controlpanel %s", name);
	    }
	  else
	    {
	      if (!strcmp(paneltype,"colormap"))
		sprintf(buf, "close colormapEditor %s=%s", how,name);
	      else if (!strcmp(paneltype,"image"))
		sprintf(buf, "close image %s=%s", how,name);
	      else
		sprintf(buf, "close controlpanel %s", name);
	    }
	  sprintf(messageid,"%d",i);
	  if (!SendPendingMessage(buf, messageid))
	    goto error;
	}
    }
  
  
  if (buf) DXFree((Pointer)buf);
  return OK;

 error:
  if (buf) DXFree((Pointer)buf);
  return ERROR;	
}


Error m_ManageImageWindow(Object *in)
{
  char *where;
  int map;

  if (! DXExtractString(in[0], &where) ||
      ! DXExtractInteger(in[1], &map) ||
      ! _dxf_mapSupervisedWindow(where, map))
    if (!ManagePanels(in, "image"))
      goto error;

  return OK;

error:
  return ERROR;
}


Error m_OpenVisualProgram(Object *in)
{
  /* this one sets pending to 1 */
  char *name, *buf;
  int flush=1;
  char messageid[20];
  
  sprintf(messageid,"default");
  
  
  if (in[0])
    {
      if (!DXExtractString(in[0], &name))
	{
	  DXSetError(ERROR_BAD_PARAMETER,"name must be a string");
	  goto error;
	}
    }
  else
    {
      DXSetError(ERROR_MISSING_DATA,"name must be specified");
      goto error;
    }

  if (in[1])
    {
      if (!DXExtractInteger(in[1], &flush))
	{
	  DXSetError(ERROR_BAD_PARAMETER,"flush must be 0 or 1");
	  goto error;
	}
      if ((flush != 0)&&(flush != 1))
	{
	  DXSetError(ERROR_BAD_PARAMETER,"flush must be 0 or 1");
	  goto error;
	}
    }

  buf = DXAllocate((strlen(name)+50));
  if (!buf)
     goto error;

  if (flush)
    sprintf(buf, "open network %s", name);
  else
    sprintf(buf, "open networkNoReset %s", name);

  if (!SendPendingMessage(buf, messageid))
    goto error;
  
  return OK;
 error:
  return ERROR;	
}

static Error
  SendPendingDXLMessage(Private P)
{
  PendingDXLMessage *p = (PendingDXLMessage *)DXGetPrivateData(P);
  
  DXUIMessage(p->messageType, p->message);
  
  if (p->delete)
    if (! DXSetPendingCmd(p->major, p->minor, NULL, NULL))
      {
	DXSetError(ERROR_INTERNAL, "error return from DXSetPendingCmd");
	return ERROR;
      }
  
  return OK;
}


static Error delete_PendingDXLMessage(Pointer d)
{
  PendingDXLMessage *p = (PendingDXLMessage *)d;
  DXFree(p->message);
  DXFree(p->messageType);
  DXFree(p->major);
  DXFree(p->minor);
  DXFree(p);
  return OK;
}



Error m_ManageControlPanel(Object *in)
{

  if (!ManagePanels(in, "controlpanel"))
     goto error;

  return OK;

error:
  return ERROR;
}


#if 0
static Error GetFlag(Object obj, int *flag)
{

  if (!DXExtractInteger(obj,flag))
    {
      DXSetError(ERROR_BAD_PARAMETER,"flag must be 0 or 1");
      goto error;
    }
  if ((*flag != 0) && (*flag != 1))
    {
      DXSetError(ERROR_BAD_PARAMETER,"flag must be 0 or 1");
      goto error;
    }
  return OK;
 error:
  return ERROR;
}
#endif
 

static Error SendPendingMessage(char *buf, char *messageid)
{
  char *major = NULL;
  PendingDXLMessage *plr = NULL;
  Private p = NULL;

  plr = (PendingDXLMessage *)DXAllocateZero(sizeof(PendingDXLMessage));
  if (!plr)
     goto error;
      
  plr->message = (char *)DXAllocate(strlen(buf)+1);
  if (!plr->message)
     goto error;
      
  plr->messageType = (char *)DXAllocate(5);
  if (!plr->messageType)
     goto error;

  major = DXGetModuleId();
  if (!major)
     goto error;

  plr->major = (char *)DXAllocate(strlen(major)+1);
  if (!plr->major)
     goto error;
      
  plr->minor = (char *)DXAllocate(strlen(messageid)+1);
  if (!plr->minor)
     goto error;

  p = DXNewPrivate((Pointer)plr, delete_PendingDXLMessage);
  if (!p)
     goto error;
      
  strncpy(plr->message, buf, strlen(buf)+1);
  strncpy(plr->messageType, "LINK", 5);
  strncpy(plr->major, major, strlen(major)+1);
  strncpy(plr->minor, messageid, strlen(messageid)+1);
      
  DXFreeModuleId(major);
  major = NULL;
      
  plr->delete = 1;
      
  if (!DXSetPendingCmd(plr->major, messageid, SendPendingDXLMessage, p))
     goto error;
  return OK;
  
error:    
  if (p)
    DXDelete((Object)p);
  else if (plr)
    {
      DXFree(plr->major);
      DXFree(plr->minor);
      DXFree(plr->message);
      DXFree(plr->messageType);
      DXFree(plr);
    }
  
  if (major)
    DXFreeModuleId(major);
  return ERROR;
  
}
