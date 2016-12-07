/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include <dx/dx.h>

#include "scalar.h"

int m_ScalarList(Object *in, Object *out)
{
   if (!_dxfscalar_base(in, out,1))
      return ERROR;
   return OK;
}

