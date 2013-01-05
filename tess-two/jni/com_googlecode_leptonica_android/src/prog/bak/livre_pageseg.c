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
 * livre_pageseg.c
 *
 *    This gives examples of the use of binary morphology for
 *    some simple and fast document segmentation operations.
 *
 *    The operations are carried out at 2x reduction.
 *    For images scanned at 300 ppi, this is typically
 *    high enough resolution for accurate results.
 *
 *    This generates several of the figures used in Chapter 18 of
 *    "Mathematical morphology: from theory to applications",
 *    edited by Laurent Najman and Hugues Talbot.  Published by
 *    Hermes Scientific Publishing, Ltd, 2010.
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"

    /* Control the display output */
#define   DFLAG        0


l_int32 DoPageSegmentation(PIX *pixs, l_int32 which);

main(int    argc,
     char **argv)
{
char        *filein;
l_int32      i;
PIX         *pixs;   /* input image sould be at least 300 ppi */
static char  mainName[] = "livre_pageseg";

    if (argc != 2)
	exit(ERROR_INT(" Syntax:  livre_pageseg filein", mainName, 1));

    filein = argv[1];
    if ((pixs = pixRead(filein)) == NULL)
	exit(ERROR_INT("pix not made", mainName, 1));

    for (i = 1; i <= 4; i++)
        DoPageSegmentation(pixs, i);
    pixDestroy(&pixs);
    return 0;
}


l_int32
DoPageSegmentation(PIX     *pixs,   /* should be at least 300 ppi */
                   l_int32  which)  /* 1, 2, 3, 4 */
{
char         buf[256];
l_int32      index, zero;
BOXA        *boxatm, *boxahm;
PIX         *pixr;   /* image reduced to 150 ppi */
PIX         *pixhs;  /* image of halftone seed, 150 ppi */
PIX         *pixm;   /* image of mask of components, 150 ppi */
PIX         *pixhm1; /* image of halftone mask, 150 ppi */
PIX         *pixhm2; /* image of halftone mask, 300 ppi */
PIX         *pixht;  /* image of halftone components, 150 ppi */
PIX         *pixnht; /* image without halftone components, 150 ppi */
PIX         *pixi;   /* inverted image, 150 ppi */
PIX         *pixvws; /* image of vertical whitespace, 150 ppi */
PIX         *pixtm1; /* image of closed textlines, 150 ppi */
PIX         *pixtm2; /* image of refined text line mask, 150 ppi */
PIX         *pixtm3; /* image of refined text line mask, 300 ppi */
PIX         *pixtb1; /* image of text block mask, 150 ppi */
PIX         *pixtb2; /* image of text block mask, 300 ppi */
PIX         *pixnon; /* image of non-text or halftone, 150 ppi */
PIX         *pixt1, *pixt2, *pixt3, *pixt4, *pixt5;
PIXA        *pixa;
PIXCMAP     *cmap;
PTAA        *ptaa;
l_int32      ht_flag = 0;
l_int32      ws_flag = 0;
l_int32      text_flag = 0;
l_int32      block_flag = 0;

    PROCNAME("DoPageSegmentation");

    if (which == 1)
        ht_flag = 1;
    else if (which == 2)
        ws_flag = 1;
    else if (which == 3)
        text_flag = 1;
    else if (which == 4)
        block_flag = 1;
    else 
        return ERROR_INT("invalid parameter: not in [1...4]", procName, 1);
    pixDisplayWrite(NULL, -1);

        /* Reduce to 150 ppi */
    pixt1 = pixScaleToGray2(pixs);
    pixDisplayWriteFormat(pixt1, L_MAX(ws_flag, L_MAX(ht_flag, block_flag)),
                          IFF_PNG);
    if (which == 1) pixWrite("/tmp/orig.gray.150.png", pixt1, IFF_PNG);
    pixDestroy(&pixt1);
    pixr = pixReduceRankBinaryCascade(pixs, 1, 0, 0, 0);

	/* Get seed for halftone parts */
    pixt1 = pixReduceRankBinaryCascade(pixr, 4, 4, 3, 0);
    pixt2 = pixOpenBrick(NULL, pixt1, 5, 5);
    pixhs = pixExpandBinaryPower2(pixt2, 8);
    pixDisplayWriteFormat(pixhs, ht_flag, IFF_PNG);
    if (which == 1) pixWrite("/tmp/htseed.150.png", pixhs, IFF_PNG);
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);

	/* Get mask for connected regions */
    pixm = pixCloseSafeBrick(NULL, pixr, 4, 4);
    pixDisplayWriteFormat(pixm, ht_flag, IFF_PNG);
    if (which == 1) pixWrite("/tmp/ccmask.150.png", pixm, IFF_PNG);

        /* Fill seed into mask to get halftone mask */
    pixhm1 = pixSeedfillBinary(NULL, pixhs, pixm, 4);
    pixDisplayWriteFormat(pixhm1, ht_flag, IFF_PNG);
    if (which == 1) pixWrite("/tmp/htmask.150.png", pixhm1, IFF_PNG);
    pixhm2 = pixExpandBinaryPower2(pixhm1, 2);
    
        /* Extract halftone stuff */
    pixht = pixAnd(NULL, pixhm1, pixr);
    if (which == 1) pixWrite("/tmp/ht.150.png", pixht, IFF_PNG);

        /* Extract non-halftone stuff */
    pixnht = pixXor(NULL, pixht, pixr);
    pixDisplayWriteFormat(pixnht, text_flag, IFF_PNG);
    if (which == 1) pixWrite("/tmp/text.150.png", pixnht, IFF_PNG);
    pixZero(pixht, &zero);
    if (zero)
	fprintf(stderr, "No halftone parts found\n");
    else
	fprintf(stderr, "Halftone parts found\n");

        /* Get bit-inverted image */
    pixi = pixInvert(NULL, pixnht);
    if (which == 1) pixWrite("/tmp/invert.150.png", pixi, IFF_PNG);
    pixDisplayWriteFormat(pixi, ws_flag, IFF_PNG);

        /* The whitespace mask will break textlines where there
         * is a large amount of white space below or above.
         * We can prevent this by identifying regions of the
         * inverted image that have large horizontal (bigger than
         * the separation between columns) and significant
         * vertical extent (bigger than the separation between
         * textlines), and subtracting this from the whitespace mask. */
    pixt1 = pixMorphCompSequence(pixi, "o80.60", 0);
    pixt2 = pixSubtract(NULL, pixi, pixt1);
    pixDisplayWriteFormat(pixt2, ws_flag, IFF_PNG);
    pixDestroy(&pixt1);

	/* Identify vertical whitespace by opening inverted image */
    pixt3 = pixOpenBrick(NULL, pixt2, 5, 1);  /* removes thin vertical lines */
    pixvws = pixOpenBrick(NULL, pixt3, 1, 200);  /* gets long vertical lines */
    pixDisplayWriteFormat(pixvws, L_MAX(text_flag, ws_flag), IFF_PNG);
    if (which == 1) pixWrite("/tmp/vertws.150.png", pixvws, IFF_PNG);
    pixDestroy(&pixt2);
    pixDestroy(&pixt3);

        /* Get proto (early processed) text line mask. */
	/* First close the characters and words in the textlines */
    pixtm1 = pixCloseSafeBrick(NULL, pixnht, 30, 1);
    pixDisplayWriteFormat(pixtm1, text_flag, IFF_PNG);
    if (which == 1) pixWrite("/tmp/textmask1.150.png", pixtm1, IFF_PNG);

	/* Next open back up the vertical whitespace corridors */
    pixtm2 = pixSubtract(NULL, pixtm1, pixvws);
    if (which == 1) pixWrite("/tmp/textmask2.150.png", pixtm2, IFF_PNG);

	/* Do a small opening to remove noise */
    pixOpenBrick(pixtm2, pixtm2, 3, 3);
    pixDisplayWriteFormat(pixtm2, text_flag, IFF_PNG);
    if (which == 1) pixWrite("/tmp/textmask3.150.png", pixtm2, IFF_PNG);
    pixtm3 = pixExpandBinaryPower2(pixtm2, 2);

        /* Join pixels vertically to make text block mask */
    pixtb1 = pixMorphSequence(pixtm2, "c1.10 + o4.1", 0);
    pixDisplayWriteFormat(pixtb1, block_flag, IFF_PNG);
    if (which == 1) pixWrite("/tmp/textblock1.150.png", pixtb1, IFF_PNG);

        /* Solidify the textblock mask and remove noise:
         *  (1) For each c.c., close the blocks and dilate slightly
	 *      to form a solid mask.
         *  (2) Small horizontal closing between components
	 *  (3) Open the white space between columns, again
         *  (4) Remove small components */
    pixt1 = pixMorphSequenceByComponent(pixtb1, "c30.30 + d3.3", 8, 0, 0, NULL);
    pixCloseSafeBrick(pixt1, pixt1, 10, 1);
    pixDisplayWriteFormat(pixt1, block_flag, IFF_PNG);
    pixt2 = pixSubtract(NULL, pixt1, pixvws);
    pixt3 = pixSelectBySize(pixt2, 25, 5, 8, L_SELECT_IF_BOTH,
                            L_SELECT_IF_GTE, NULL);
    pixDisplayWriteFormat(pixt3, block_flag, IFF_PNG);
    if (which == 1) pixWrite("/tmp/textblock2.150.png", pixt3, IFF_PNG);
    pixtb2 = pixExpandBinaryPower2(pixt3, 2);
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    pixDestroy(&pixt3);

        /* Identify the outlines of each textblock */
    ptaa = pixGetOuterBordersPtaa(pixtb2);
    pixt1 = pixRenderRandomCmapPtaa(pixtb2, ptaa, 1, 8, 1);
    cmap = pixGetColormap(pixt1);
    pixcmapResetColor(cmap, 0, 130, 130, 130);  /* set interior to gray */
    if (which == 1) pixWrite("/tmp/textblock3.300.png", pixt1, IFF_PNG);
    pixDisplayWithTitle(pixt1, 480, 360, "textblock mask with outlines", DFLAG);
    ptaaDestroy(&ptaa);
    pixDestroy(&pixt1);

        /* Fill line mask (as seed) into the original */
    pixt1 = pixSeedfillBinary(NULL, pixtm3, pixs, 8);
    pixOr(pixtm3, pixtm3, pixt1);
    pixDestroy(&pixt1);
    if (which == 1) pixWrite("/tmp/textmask.300.png", pixtm3, IFF_PNG);
    pixDisplayWithTitle(pixtm3, 480, 360, "textline mask 4", DFLAG);

        /* Fill halftone mask (as seed) into the original */
    pixt1 = pixSeedfillBinary(NULL, pixhm2, pixs, 8);
    pixOr(pixhm2, pixhm2, pixt1);
    pixDestroy(&pixt1);
    if (which == 1) pixWrite("/tmp/htmask.300.png", pixhm2, IFF_PNG);
    pixDisplayWithTitle(pixhm2, 520, 390, "halftonemask 2", DFLAG);

        /* Find objects that are neither text nor halftones */
    pixt1 = pixSubtract(NULL, pixs, pixtm3);  /* remove text pixels */
    pixnon = pixSubtract(NULL, pixt1, pixhm2);  /* remove halftone pixels */
    if (which == 1) pixWrite("/tmp/other.300.png", pixnon, IFF_PNG);
    pixDisplayWithTitle(pixnon, 540, 420, "other stuff", DFLAG);
    pixDestroy(&pixt1);

        /* Write out b.b. for text line mask and halftone mask components */
    boxatm = pixConnComp(pixtm3, NULL, 4);
    boxahm = pixConnComp(pixhm2, NULL, 8);
    if (which == 1) boxaWrite("/tmp/textmask.boxa", boxatm);
    if (which == 1) boxaWrite("/tmp/htmask.boxa", boxahm);

    pixa = pixaReadFiles("/tmp", "junk_write_display");
    pixt1 = pixaDisplayTiledAndScaled(pixa, 8, 250, 4, 0, 25, 2);
    snprintf(buf, sizeof(buf), "/tmp/segout.%d.png", which);
    pixWrite(buf, pixt1, IFF_PNG);
    pixDestroy(&pixt1);
    pixaDestroy(&pixa);

	/* clean up to test with valgrind */
    pixDestroy(&pixr);
    pixDestroy(&pixhs);
    pixDestroy(&pixm);
    pixDestroy(&pixhm1);
    pixDestroy(&pixhm2);
    pixDestroy(&pixht);
    pixDestroy(&pixnht);
    pixDestroy(&pixi);
    pixDestroy(&pixvws);
    pixDestroy(&pixtm1);
    pixDestroy(&pixtm2);
    pixDestroy(&pixtm3);
    pixDestroy(&pixtb1);
    pixDestroy(&pixtb2);
    pixDestroy(&pixnon);
    boxaDestroy(&boxatm);
    boxaDestroy(&boxahm);
    return 0;
}

