/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dpexec/attribute.h,v 1.4 2000/08/11 15:28:09 davidt Exp $
 */

#include <dxconfig.h>


#ifndef	__ATTRIBUTE_H
#define	__ATTRIBUTE_H

#include <dx/dx.h>
#include "parse.h"

Object	_dxf_ExGetAttribute	   (node *n, char *attr);
char *  _dxf_ExGetStringAttribute  (node *n, char *attrname);
Error   _dxf_ExHasStringAttribute  (node *n, char *attrname, char **value);
int     _dxf_ExGetIntegerAttribute (node *n, char *attrname);
Error   _dxf_ExHasIntegerAttribute (node *n, char *attrname, int *i);


#define ATTR_CACHE	"cache"
#define ATTR_REMOTE	"remote"
#define ATTR_INSTANCE	"instance"
#define ATTR_PGRP	"group"
#define ATTR_DIREROUTE	"reroute"
#define ATTR_ONESHOT  	"oneshot"
#define ATTR_RERUNKEY   "rerun_key"  

/* maximum length of attribute strings define above */
#define MAX_ATTRLEN	15

#endif	/* __ATTRIBUTE_H */
