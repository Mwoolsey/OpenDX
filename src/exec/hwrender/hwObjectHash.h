/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


HashTable _dxf_InitObjectHash();
dxObject  _dxf_QueryObject(HashTable, dxObject);
void 	  _dxf_InsertObject(HashTable, dxObject, dxObject);
HashTable _dxf_CullObjectHash(HashTable);
void 	  _dxf_DeleteObjectHash(HashTable);
void	  _dxf_ResetObjectHash(HashTable);

HashTable _dxf_InitDisplayListHash();
dxObject  _dxf_QueryDisplayList(HashTable, xfieldP, int);
void 	  _dxf_InsertDisplayList(HashTable, xfieldP, int, dxObject);
HashTable _dxf_CullDisplayListHash(HashTable);
void 	  _dxf_DeleteDisplayListHash(HashTable);
void	  _dxf_ResetDisplayListHash(HashTable);
