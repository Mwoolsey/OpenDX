/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dxmods/importtable.c,v 1.5 2000/08/24 20:04:38 davidt Exp $
 */

#include <dxconfig.h>


#include <dx/dx.h>
#include "import.h"
#include "genimp.h"

/* 
 * table of entry points for each format.
 *
 *  for each type of file to be imported, three things must be supplied:
 *   a function to determine if this is a valid file of this type
 *   a function to actually import the data and return it
 *   a list of format strings which indicate this type of file
 * 
 *  if the autotype function is null, the extension must be given for import
 *   to process this type of file.
 *
 */
struct import_table _dxdImportTable[] = {

 /* netCDF */	{ _dxftry_ncdf,	_dxfget_ncdf,   {"cdf","ncdf", "nc",
							"netcdf","netCDF",0}},
 /* HDF */	{ _dxftry_hdf,	_dxfget_hdf,    {"hdf", 0}},
 /* bin */	{ _dxftry_bin,	_dxfget_bin,    {"bin", 0}},
 /* CDF */	{ _dxftry_cdf,	_dxfget_cdf,    {"CDF", 0}},
 /* general */	{ NULL,	        _dxf_gi_get_gen,{"general", "gai", 0}},
 /* DX */	{ _dxftry_dx,	_dxfget_dx,	{"dx", 0}},
 /* wv90 */	{ _dxftry_wv,	_dxfget_wv,	{"wv90", "fieldhdr", 0}},
 /* cm */	{ NULL,		_dxfget_cm,	{"cm", 0}},
 /* ADD HERE */
 /* must be last */  { 0,              0,         {0}},

};

