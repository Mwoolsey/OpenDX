/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/


#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#ifndef _DXI_IMPORT_H_
#define _DXI_IMPORT_H_

/* TeX starts here. Do not remove this comment. */

/*
\section{Data Import}

This section describes routines for importing data into the Data Explorer.
This includes support for Data Explorer (.dx) files, as well as
industry-standard netCDF files.

\paragraph{Data Explorer Files}
Files in the Data Explorer format, as described in ---Data Model chapter,
User's manual--- can be imported by the {\tt DXImportDX()} routine.
*/

Object DXImportDX(char *filename, char **variable,
		int *start, int *end, int *delta);
/**
\index{DXImportDX}
Imports data from a Data Explorer file specified by the {\tt filename}
parameter.  The {\tt variable} parameter specifies a null-terminated
list of strings that identify which variables to import.  This
parameter identifies objects in the file that have the names specified
by {\tt variable}.

For series data, {\tt *start}, {\tt *end}, and {\tt *delta} are used
to control which series members are read in.  If {\tt start} is null,
it defaults to the beginning of the series; if {\tt end} is null, it
defaults to the end of the series; if {\tt delta} is null, it defaults
to one.

Returns a pointer to the object, or returns null and sets the error
code to indicate on error.
**/

Object DXExportDX(Object o, char *filename, char *format);
/**
\incde{DXExportDX}
Exports data in Data Explorer file format.
**/

/*
\paragraph{netCDF Data}
The Data Explorer is capable of accepting industry-standard netCDF
files.  These files are capable only of a subset of the Data Explorer
data model capabilities.  For example, netCDF files are limited to
regular data.  This routine is provided for compatibility with other
systems that use netCDF.
*/

Object DXImportNetCDF(char *filename, char **variable,
		    int *start, int *end, int *delta);
/**
\index{DXImportNetCDF}
Creates a new group or field object.  The {\tt filename} parameter is
the name of a data file in netCDF format.  The {\tt variable}
parameter specifies a null-terminated list of strings that identify
which variables to import.

For series data, {\tt *start}, {\tt *end}, and {\tt *delta} are used
to control which series members are read in.  If {\tt start} is null,
it defaults to the beginning of the series; if {\tt end} is null, it
defaults to the end of the series; if {\tt delta} is null, it defaults
to one.

Returns a pointer to the object, or returns null and sets the error
code to indicate on error.
**/


/* DO NOT INCLUDE IN DOCUMENTATION */
Field DXImportHDF(char *filename, char *variable);

Object DXImportCDF(char *filename, char **fieldlist, int *start,
		int *end, int *delta);

Object DXImportCM(char *filename,char **fieldlist);

#endif /* _DXI_IMPORT_H_ */

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
