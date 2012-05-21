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
 * xvdisp.c
 *
 *   xv automatically downscales if necessary, to display the entire
 *   image without scrollbars.  Because xv downscaling is suboptimal,
 *   the image looks worse than it will actually look using our
 *   area mapping downscaling.
 *
 *   With xvdisp, we do the scaling, if necessary, and launch xv on
 *   the downscaled image.
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"


main(int    argc,
     char **argv)
{
PIX         *pixs;
char        *filein;
static char  mainName[] = "xvdisp";

    if (argc != 2)
	exit(ERROR_INT(" Syntax:  xvdisp filein", mainName, 1));

    filein = argv[1];
    if ((pixs = pixRead(filein)) == NULL)
	exit(ERROR_INT("pixs not made", mainName, 1));
	    
    pixDisplay(pixs, 20, 20);
    pixDestroy(&pixs);
    return 0;
}

