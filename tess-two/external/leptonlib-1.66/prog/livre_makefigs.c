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
 * livre_makefigs.c
 *
 *   This makes all the figures in Chapter 18, "Document Image Applications",
 *   of the book "Mathematical morphology: from theory to applications",
 *   edited by Laurent Najman and hugues Talbot.  Published by Hermes
 *   Scientific Publishing, Ltd, 2010.
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"


main(int    argc,
     char **argv)
{
char         buf[256];
static char  mainName[] = "livre_makefigs";

    if (argc != 1)
	exit(ERROR_INT(" Syntax:  livre_makefigs", mainName, 1));

        /* Generate Figure 1 (page segmentation) */
    system("livre_seedgen");
    snprintf(buf, sizeof(buf), "cp /tmp/seedgen.png /tmp/dia_fig1.png");
    system(buf);

        /* Generate Figures 2-5 (page segmentation) */
    snprintf(buf, sizeof(buf), "livre_pageseg pageseg2.tif");
    system(buf);
    snprintf(buf, sizeof(buf), "cp /tmp/segout.1.png /tmp/dia_fig2.png");
    system(buf);
    snprintf(buf, sizeof(buf), "cp /tmp/segout.2.png /tmp/dia_fig3.png");
    system(buf);
    snprintf(buf, sizeof(buf), "cp /tmp/segout.3.png /tmp/dia_fig4.png");
    system(buf);
    snprintf(buf, sizeof(buf), "cp /tmp/segout.4.png /tmp/dia_fig5.png");
    system(buf);

        /* Generate Figure 6 (hmt sels for text orientation) */
    system("livre_orient");
    snprintf(buf, sizeof(buf), "cp /tmp/orient.png /tmp/dia_fig6.png");
    system(buf);

        /* Generate Figure 7 (hmt sel for fancy "Tribune") */
    system("livre_hmt 1 8");
    snprintf(buf, sizeof(buf), "cp /tmp/hmt.png /tmp/dia_fig7.png");
    system(buf);

        /* Generate Figure 8 (hmt sel for fancy "T") */
    system("livre_hmt 2 4");
    snprintf(buf, sizeof(buf), "cp /tmp/hmt.png /tmp/dia_fig8.png");
    system(buf);

        /* Generate Figure 9 (tophat background cleaning) */
    system("livre_tophat");
    snprintf(buf, sizeof(buf), "cp /tmp/tophat.jpg /tmp/dia_fig9.jpg");
    system(buf);

        /* Run livre_adapt to generate an expanded version of Figure 9 */
    system("livre_adapt");

    return 0;
}

