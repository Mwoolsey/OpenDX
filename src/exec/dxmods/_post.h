/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

/*
 * $Header: /src/master/dx/src/exec/dxmods/_post.h,v 1.1 2000/08/24 20:04:17 davidt Exp $:
 */

#include <dxconfig.h>

#ifndef __POST_H
#define __POST_H

Object _dxfPost( Object, char *, Array );
Array _dxfPostArray( Field, char *, char * );

#define COPYOUT(TYPE, KNT)						\
		    {							\
			float *fPtr = (float *)accumulator;		\
			TYPE  *dPtr = (TYPE *)DXGetArrayData(nArray);   \
			int j, k;					\
									\
			if (! dPtr)					\
			    goto error;					\
									\
			kPtr = (byte *)knts;				\
			for (j = 0; j < KNT; j++)			\
			{						\
			    if (*kPtr > 1)				\
			    {						\
				float d;				\
									\
				d = 1.0 / (float)(*kPtr);		\
			       						\
				for (k = 0; k < vPerI; k++)		\
				{					\
				    *dPtr = *fPtr * d;			\
				    fPtr ++;				\
				    dPtr ++;				\
				}					\
			    }						\
			    else					\
			    {						\
				for (k = 0; k < vPerI; k++)		\
				{					\
				    *dPtr = *fPtr;			\
				    fPtr ++;				\
				    dPtr ++;				\
				}					\
			    }						\
									\
			    kPtr ++;					\
			}						\
		    }

#define TO_POSITIONS_REGULAR(TYPE)					\
		    {							\
			byte  *kPtr;					\
									\
			while(1)					\
			{						\
			    TYPE  *cPtr, *c;				\
			    float *pPtr, *p;				\
			    int   k, l;					\
			    Loop  *lptr;				\
									\
			    lptr = loop;				\
									\
			    kPtr = lptr->kPtr;				\
			    cPtr = (TYPE *)lptr->cPtr;			\
			    pPtr = (float *)lptr->pPtr;			\
									\
			    for (j = 0; j < (lptr->limit-1); j++)	\
			    {						\
				for (k = 0; k < vPerE; k++)		\
				{					\
				    p = pPtr + adj[k]*vPerI;		\
				    c = cPtr;				\
									\
				    for (l = 0; l < vPerI; l++)		\
					*p++ += *c++;			\
									\
				    kPtr[adj[k]] ++;			\
				}					\
									\
				cPtr += vPerI;				\
				pPtr += vPerI;				\
				kPtr ++;				\
			    }						\
									\
			    lptr++;					\
									\
			    for (j = 1; j < cDim; j++, lptr++)		\
			    {						\
				lptr->inc++;				\
				lptr->cPtr += lptr->cSkip;		\
				lptr->pPtr += lptr->pSkip;		\
				lptr->kPtr += lptr->kSkip;		\
									\
				if (lptr->inc < (lptr->limit - 1))	\
				    break;				\
									\
			    }						\
									\
			    if (j == cDim)				\
				break;					\
									\
			    for (j--; j >= 0; j--)			\
			    {						\
				loop[j].inc   = 0;			\
				loop[j].cPtr  = lptr->cPtr;		\
				loop[j].pPtr  = lptr->pPtr;		\
				loop[j].kPtr  = lptr->kPtr;		\
			    }						\
			}						\
									\
			COPYOUT(TYPE, nPositions);			\
		    }


#define TO_POSITIONS_IRREGULAR(TYPE)					\
		    {							\
			int   k;					\
			byte  *kPtr;					\
			TYPE  *cPtr;					\
			float *fPtr;					\
			int   *ePtr;					\
		     							\
			cPtr = (TYPE *)srcData;				\
			fPtr = (float *)accumulator;			\
									\
			for (j = 0; j < nConnections; j++)		\
			{						\
			    ePtr = (int *)DXCalculateArrayEntry		\
						    (cHandle, j, ebuf);	\
									\
			    for (k = 0; k < vPerE; k++)			\
			    {						\
				float *p;				\
				TYPE *c;				\
									\
				p = fPtr + (*ePtr * vPerI);		\
				c = (TYPE *)cPtr;			\
									\
				for (l = 0; l < vPerI; l++)		\
				    *p++ += *c++;			\
									\
				knts[*ePtr] ++;				\
									\
				ePtr ++;				\
			    }						\
									\
			    cPtr += vPerI;				\
			}						\
									\
			COPYOUT(TYPE, nPositions);			\
		    }		

#define TO_CONNECTIONS_REGULAR(TYPE)					\
		    {							\
			byte *kPtr;					\
									\
			while(1)					\
			{						\
			    float *cPtr, *c;				\
			    TYPE  *pPtr, *p;				\
			    int   k, l;					\
			    Loop  *lptr;				\
									\
			    lptr = loop;				\
									\
			    cPtr = (float *)lptr->cPtr;			\
			    kPtr = (byte  *)lptr->kPtr;			\
			    pPtr = (TYPE  *)lptr->pPtr;			\
									\
			    for (j = 0; j < (lptr->limit-1); j++)	\
			    {						\
				for (k = 0; k < vPerE; k++)		\
				{					\
				    p = pPtr + adj[k]*vPerI;		\
				    c = cPtr;				\
									\
				    for (l = 0; l < vPerI; l++)		\
					*c++ += *p++;			\
				}					\
									\
				*kPtr += vPerE;				\
				 					\
				cPtr += vPerI;				\
				pPtr += vPerI;				\
				kPtr ++;				\
			    }						\
				 					\
			    lptr++;					\
				 					\
			    for (j = 1; j < cDim; j++, lptr++)		\
			    {						\
				lptr->inc++;				\
				lptr->cPtr += lptr->cSkip;		\
				lptr->pPtr += lptr->pSkip;		\
				lptr->kPtr += lptr->kSkip;		\
									\
				if (lptr->inc < (lptr->limit - 1))	\
				    break;				\
									\
			    }						\
									\
			    if (j == cDim)				\
				break;					\
									\
			    for (j--; j >= 0; j--)			\
			    {						\
				loop[j].inc   = 0;			\
				loop[j].cPtr  = lptr->cPtr;		\
				loop[j].pPtr  = lptr->pPtr;		\
				loop[j].kPtr  = lptr->kPtr;		\
			    }						\
			}						\
									\
			COPYOUT(TYPE, nConnections);			\
		    }							\

#define TO_CONNECTIONS_IRREGULAR(TYPE)					\
		    {							\
			int   k;					\
			TYPE  *pPtr;					\
			byte  *kPtr;					\
			float *cPtr;					\
			int   *ePtr;					\
		     							\
			cPtr = (float *)accumulator;			\
			pPtr = (TYPE *)srcData;				\
									\
			for (j = 0; j < nConnections; j++)		\
			{						\
			    ePtr = (int *)DXCalculateArrayEntry		\
						    (cHandle, j, ebuf);	\
									\
			    for (k = 0; k < vPerE; k++)			\
			    {						\
				float *c;				\
				TYPE  *p;				\
									\
				p = pPtr + (*ePtr * vPerI);		\
				c = cPtr;				\
									\
				for (l = 0; l < vPerI; l++)		\
				    *c++ += *p++;			\
									\
				ePtr ++;				\
			    }						\
									\
			    knts[j] += vPerE;				\
			    cPtr    += vPerI;				\
			}						\
									\
			COPYOUT(TYPE, nConnections);			\
		   }

#endif /* __POST_H_ */
