/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
#include <dx/dx.h>

#include <dxconfig.h>


static Error TraverseToScreen(Object, float);
static Error ScaleScreen(Object, Object, float);
static Error  GetFactor(int, Camera, float *);
static Camera MakeNewCamera(Camera, float);


Error m_ScaleScreen (Object *in, Object *out)
{
  float factor;
  int final_res;
  Object incopy=NULL;
  Camera new_camera;
  
  if (!in[0])
    {
      DXSetError(ERROR_BAD_PARAMETER, "object must be specified");
      goto error;
    }

  if ((DXGetObjectClass(in[0]) != CLASS_STRING) &&
      (DXGetObjectClass(in[0]) != CLASS_ARRAY))
  {
     incopy = DXCopy(in[0], COPY_STRUCTURE);
     if (!incopy)
        goto error;
  }
  else
  {
     DXSetError(ERROR_MISSING_DATA,"object must be specified");
     goto error;
  }
  
  if (!incopy && !in[2])
    {
      out[0] = incopy;
      out[1] = in[3];
      return OK;
    }

  if (in[1])
   if (!DXExtractFloat(in[1], &factor)) 
    {
      DXSetError(ERROR_BAD_PARAMETER, "scale_factor must be a scalar value");
      goto error;
    }
  
  if (in[2])
    if (!DXExtractInteger(in[2], &final_res)) 
    {
      DXSetError(ERROR_BAD_PARAMETER, "final_res must be an integer");
      goto error;
    }
  
  if (in[1] && in[2])
    {
      DXWarning("both scale_factor and final_res are specified. Only scale_factor will be used");
    }
  
  if (in[3])
    {
      if ((DXGetObjectClass(in[3]) != CLASS_CAMERA))
	{
	  DXSetError(ERROR_BAD_PARAMETER,"current_camera must be a camera");
	  goto error;
	}
    }
  
  if (!in[1] && in[2] && !in[3])
    {
      DXSetError(ERROR_DATA_INVALID,"if final_res is specified, current_camera must be specified");
      goto error;
    }
 
  /* figure out the appropriate scale factor */
  if (!in[1]){
     if (in[3]) {
         if (!GetFactor(final_res, (Camera)in[3], &factor))
            goto error;
     }
     else {
       factor = 1;
     } 
  } 
  
  if (!TraverseToScreen(incopy, factor))
    goto error;
  
  out[0] = incopy; 
  
  /* make the output camera, if appropriate */
  if (in[3])
    {
      new_camera = MakeNewCamera((Camera)in[3], factor);
      if (!new_camera)
      {
        goto error; 
      }
      out[1] = (Object)new_camera;
    }
  else
    out[1] = NULL;
  
  
  return OK;
 error:
  DXDelete((Object)incopy);
  return ERROR;
}


static Error TraverseToScreen(Object o, float factor)
{
  /* should traverse down to view-port relative coordinates */
  
  Object subo;
  int i, position;
  
  switch (DXGetObjectClass(o)) {
  case (CLASS_GROUP):
    for (i=0; (subo = DXGetEnumeratedMember((Group)o, i, NULL)); i++) {
      if (!TraverseToScreen(subo, factor))
	goto error;
    }
    break;
  case (CLASS_FIELD):
    return OK;
  case (CLASS_CLIPPED):
    if (!DXGetClippedInfo((Clipped)o, &subo, NULL))
      goto error;
    if (!TraverseToScreen(subo, factor))
      goto error;
    break;
  case (CLASS_XFORM):
    if (!DXGetXformInfo((Xform)o, &subo, NULL))
      goto error;
    if (!TraverseToScreen(subo, factor))
      goto error;
    break;
  case (CLASS_SCREEN):
    
    if (!DXGetScreenInfo((Screen)o, &subo, &position, NULL))
      goto error;

    if ((position == SCREEN_VIEWPORT)||(position == SCREEN_WORLD))
      {
	if (!ScaleScreen(o, subo, factor))
	  goto error;
      }
    else
      if (!TraverseToScreen(subo, factor))
	goto error;
    break;
  default:
    break;
  }
  
  return OK;
 error:
  return ERROR;
  
}

static Error ScaleScreen(Object parent, Object child, float factor)
{
  Object new;
  
  /* now scale */
  new = (Object)DXNewXform(child, DXScale(factor, factor, 1.0));
  
  if (!DXSetScreenObject((Screen)parent, new))
    goto error;
  
  return OK; 
 error:
  return ERROR;
}


static Error GetFactor(int res, Camera cam, float *factor)
{
   int xres;

   /* extract the current x resolution of the camera */
   if (!DXGetCameraResolution(cam, &xres, NULL))
     goto error;

   if (xres == 0)
   {
      DXSetError(ERROR_DATA_INVALID,"x resolution of current_camera = 0");
      goto error;
   }

   *factor = (float)res/(float)xres;
   return OK;

error:
   return ERROR;

}
static Camera MakeNewCamera(Camera cam, float factor)
{
   int xres, newres;
   Camera newcam;

   if (!DXGetCameraResolution(cam, &xres, NULL))
     return NULL;

   newres = factor*xres;

   newcam = (Camera)DXCopy((Object)cam, COPY_STRUCTURE);
   if (!newcam) 
      return NULL;

   if (!DXSetResolution(newcam, newres, 1))
      return NULL;

   return newcam;

}
