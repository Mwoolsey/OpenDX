/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dxmods/vsincos.c,v 1.4 2000/08/24 20:04:55 davidt Exp $
 */


#include <dxconfig.h>
#include "vsincos.h"

/*
 * $Log: vsincos.c,v $
 * Revision 1.4  2000/08/24 20:04:55  davidt
 * Major cleanup of dxmods files. Fixed most compiler warnings. In
 * the process found a few programming errors such as | being used
 * for || in some if expressions. Most function prototypes are in
 * header files now so extern prototypes do not have to be corrected
 * across multiple files. Removed most unused variables, I see a
 * direct savings of 50K in the dxexec.
 *
 * Revision 1.3  1999/05/10 15:45:33  gda
 * Copyright message
 *
 * Revision 1.3  1999/05/10 15:45:33  gda
 * Copyright message
 *
 * Revision 1.2  1999/05/03 14:06:38  gda
 * moved to using dxconfig.h rather than command-line defines
 *
 * Revision 1.1.1.1  1999/03/24 15:18:32  gda
 * Initial CVS Version
 *
 * Revision 1.1.1.1  1999/03/19 20:59:16  gda
 * Initial CVS
 *
 * Revision 10.1  1999/02/23 21:20:14  gda
 * OpenDX Baseline
 *
 * Revision 9.1.1.1  1997/05/22 20:35:01  svs
 * wintel build AND 3.1.4 demand fixes
 *
 * Revision 9.1  1997/05/22  20:35:00  svs
 * Copy of release 3.1.4 code
 *
 * Revision 8.0  1995/10/03  19:51:22  nsc
 * Copy of release 3.1 code
 *
 * Revision 7.1  1994/01/18  18:47:45  svs
 * changes since release 2.0.1
 *
 * Revision 6.1  93/11/16  10:45:24  svs
 * ship level code, release 2.0
 * 
 * Revision 5.0  92/11/17  16:43:35  svs
 * ship level code, internal rev 1.2.2
 * 
 * Revision 4.0  92/08/18  15:36:14  svs
 * ship level code, rev 1.2
 * 
 * Revision 3.1  92/02/24  11:40:18  cpv
 * Rename vsin and vcos to qvsin and qvcos for compatability with KAI's
 * signal library.
 * 
 * Revision 3.0  91/12/03  09:19:17  nsc
 * ship level code, rev 1.0
 * 
 * Revision 1.1  91/07/09  16:50:37  cpv
 * Initial revision
 * 
 */

/*
 * Both of the following routines utilize Maclaurin series taken from
 * _Calculus and Analytic Geometry_ by Thomas, Addison-Wesley, 1968, page
 * 640.  In essence, they are the sum of the nth derivitive of v times v to 
 * the nth power over n factorial.  This is
 * sin(x) = sum (k=1..inf) [(-1**(k-1)) * (x**(2 * k - 1)) / fact (2 * k - 1)], 
 * and 
 * cos(x) = sum (k=0..inf) [(-1**k) * (x**(2*k)) / fact (2 * k)].
 * The accuracy of these formulae is greatest centered about x=0, and we 
 * desire the optimal range to be for -PI<=x<=PI, so we limit the number of 
 * iterations to 5.
 */
void qvsin(int n, float *in, float *out)
{
    float v;
    float s;
    float pow;
    float fact;
    float sign;
    int t;
    int i, j;

    for (j = 0; j < n; ++j) {
	v = in[j];
	s = 0;
	pow = v;
	fact = 1;
	sign = 1;

	for (i = 0; i < 6; ++i) {
	    s += pow * sign / fact;
	    pow *= v * v;
	    /* step from 1 to 1 * 2 * 3 */
	    t = (i + 1) << 1;	/* (i + 1) * 2 */
	    fact *= t * (t + 1);
	    sign = -sign;
	}
	out[j] = s;
    }
}

void qvcos (int n, float *in, float *out)
{
    float v;
    float s;
    float pow;
    float fact;
    float sign;
    int t;
    int i, j;

    for (j = 0; j < n; ++j) {
	v = in[j];
	s = 1;
	pow = v * v;
	fact = 2;
	sign = -1;

	for (i = 0; i < 5; ++i) {
	    s += pow * sign / fact;
	    pow *= v * v;
	    /* step fact from 2 to 2 * 3 * 4 */
	    t = (i + 2) << 1;
	    fact *= t * (t - 1);
	    sign = -sign;
	}
        out[j] = s;
    }
}
