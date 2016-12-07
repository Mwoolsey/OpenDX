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

#ifndef _DXI_PICK_H_
#define _DXI_PICK_H_

/*
 * Given a field containing pick information, this routine returns
 * the number of pokes.  Probably never returns an error.
 */

Error
DXQueryPokeCount(Field picks, int *pokeCount);

/*
 * Given a field containing pick information and a poke number,
 * this routine returns the number of picks resulting from that
 * poke.  Errors include requesting a pick count for a non-existent
 * poke.
 */

Error
DXQueryPickCount(Field picks, int poke, int *pickCount);

/*
 * Given a field containing pick information, a poke number and
 * a pick number, return the pick point in world coordinates. 
 */
  
Error
DXGetPickPoint(Field picks, int poke, int pick, Point *point);

/*
 * Given a field containing pick information, a poke number and
 * a pick number within that poke, this routine returns the length
 * of the pick path, a pointer to the pick path itself, the index
 * of the picked element in the field and the index of the closest
 * vertex of the picked element (in screen space) to the poke. 
 * If the field contains no connections component, the element 
 * and vertex indices will be the index of the closest point to the 
 * poke point.  Errors would include non-existent poke number and
 * non-existent pick number.
 */

Error
DXQueryPickPath(Field picks, int poke, int pick,
		int *pathLength, int **path, int *eid, int *vid);

/*
 * The following routine would be used to actually traverse the 
 * data object to reach the picked field, element and vertex.
 * Given a current object (initially the root of the data object),
 * a pointer to a matrix (initially identity), and an index from
 * the pick path, it returns the sub-object of the current object
 * selected by index.  When Xform objects are encountered, the 
 * matrix associated with the Xform is concatenated onto that 
 * pointed to by the matrix parameter (if one was passed in). When
 * the end of the path is found (either by recognizing that the
 * returned object is a field or that the returned object is the
 * same as the current object) the caller is left with the picked
 * field and a transform carrying the coordinate system of that
 * field to the eye coordinate system.  If there are two remaining
 * pick path elements, then they are the picked element and vertex
 * numbers; if only one remains, it is a vertex number.  Returns
 * NULL on error, including invalid path index (eg. child 27 for
 * a Xform object).
 */

Object
DXTraversePickPath(Object current, int index, Matrix *matrix);

/*
 * Example: find every picked vertex.
 * 
 *     DXQueryPokeCount(pickField, &nPokes);
 * 
 *     for (poke = 0; poke < nPokes; poke++)
 *     {
 *         DXQueryPickCount(pickField, poke, &nPicks);
 * 
 *         for (pick = 0; pick < nPicks; pick++)
 *         {
 *             DXQueryPickPath(pickField, poke, pick,
 * 				&pathLen, &path, &eid, &vid);
 * 
 *             current = dataObject;
 *             matrix  = Identity;
 * 
 *             for (i = 0; i < pathLen; i++)
 *             {
 *                 current = DXTraversePickPath(current, path[i], &matrix);
 *                 if (current == NULL)
 *                     goto error;
 *             }
 *             
 *            
 *             now futz with vertex #vid in field current.
 *             
 *         }
 *     }
 */

#endif /* _DXI_PICK_H_ */

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
