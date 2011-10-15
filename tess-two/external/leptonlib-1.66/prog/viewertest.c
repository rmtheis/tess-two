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
 * viewertest.c
 *
 *    Input:  dirin:  directory of input image files
 *            dirout: directory for output files
 *            rootname: root name for output files
 *            thumbwidth: width of thumb images, in pixels; use 0 for default
 *            viewwidth: max width of view images, in pixels; use 0 for default
 *
 *    Note: This program is Unix only; it will not link to pixHtmlViewer()
 *          under cygwin.
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"

main(int    argc,
     char **argv)
{
char        *dirin, *dirout, *rootname;
l_int32      thumbwidth, viewwidth;
static char  mainName[] = "viewertest";

    if (argc != 6)
	exit(ERROR_INT(
	    " Syntax:  viewertest dirin dirout rootname thumbwidth viewwidth",
	     mainName, 1));

    dirin = argv[1];
    dirout = argv[2];
    rootname = argv[3];
    thumbwidth = atoi(argv[4]);
    viewwidth = atoi(argv[5]);

    pixHtmlViewer(dirin, dirout, rootname, thumbwidth, viewwidth, 0);
    return 0;
}

