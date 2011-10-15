/*====================================================================*
 -  Copyright (C) 2001 Leptonica.  All rights reserved.
 -  This software is distributed in the hope that it will be
 -  useful, but with NO WARRANTY OF ANY KIND.
 -  No author or distributor accepts responsibility to anyone for the
 -  consequences of using this software, or for whether it serves any
 -  particular purpose or works at all, unless he or she says so in
 -  writing.  Everyone is granted permission to copy, modify and
 -  redistribute this source code, for commercial or non-commercial
 -  purposes, with the following restrictions: (1) the origin of this
 -  source code must not be misrepresented; (2) modified versions must
 -  be plainly marked as such; and (3) this notice may not be removed
 -  or altered from any source or modified source distribution.
 *====================================================================*/

/*
 * dwalineargen.c
 *
 *   This generates the C code for the full set of linear Sels,
 *   for dilation, erosion, opening and closing, and for both
 *   horizontal and vertical operations, from length 2 to 63.
 *
 *   These are put in files:
 *        dwalinear.3.c
 *        dwalinearlow.3.c
 *
 *   Q. Why is this C code generated here in prog, and not placed
 *      in the library where it can be linked in with all programs?
 *   A. Because the two files it generates have 17K lines of code!
 *      We also make this code available here ("out of the box") so that you
 *      can build and run dwamorph1_reg and dwamorph2_reg, without
 *      first building and running dwalineargen.c
 *   
 *   Q. Why do we build code for operations up to 63 in width and height?
 *   A. Atomic DWA operations work on Sels that have hits and misses
 *      that are not larger than 31 pixel positions from the origin.
 *      Thus, they can implement a horizontal closing up to 63 pixels
 *      wide if the origin is in the center.
 *
 *      Note the word "atomic".  DWA operations can be done on arbitrarily
 *      large Sels using the *ExtendDwa() functions.  See morphdwa.c
 *      for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"

main(int    argc,
     char **argv)
{
SELA        *sela;
static char  mainName[] = "dwalineargen";

    if (argc != 1)
	exit(ERROR_INT(" Syntax:  dwalineargen", mainName, 1));

        /* Generate the linear sel dwa code */
    sela = selaAddDwaLinear(NULL);
    if (fmorphautogen(sela, 3, "dwalinear"))
        return 1;
    selaDestroy(&sela);
    return 0;
}

