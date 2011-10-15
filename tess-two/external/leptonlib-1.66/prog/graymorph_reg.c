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
 * graymorph_reg.c
 *
 *      (1) Tests the interpreter for grayscale morphology, as
 *          given in morphseq.c
 *
 *      (2) Tests composite operations: tophat and hdome
 *
 *      (3) Tests duality for grayscale erode/dilate, open/close,
 *          and black/white tophat
 *
 *      (4) Demonstrates closing plus white tophat.  Note that this
 *          combination of operations can be quite useful.
 *      
 *      (5) Demonstrates a method of doing contrast enhancement
 *          by taking 3 * pixs and subtracting from this the
 *          closing and opening of pixs.  Do this both with the
 *          basic pix accumulation functions and with the cleaner
 *          Pixacc wrapper.   Verify the results are equivalent.
 *
 *      (6) Playing around: extract the feynman diagrams from
 *          the stamp, using the tophat.
 *
 *   For input, use e.g., aneurisms8.jpg
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"

#define     WSIZE              7
#define     HSIZE              7
#define     BUF_SIZE           512
#define     HORIZ_SEP          0  /* set to 50 to display each image */


static void pixCompare(PIX *pix, PIX *pix2, const char *msg1, const char *msg2);


main(int    argc,
     char **argv)
{
char         dilateseq[BUF_SIZE], erodeseq[BUF_SIZE];
char         openseq[BUF_SIZE], closeseq[BUF_SIZE];
char         wtophatseq[BUF_SIZE], btophatseq[BUF_SIZE];
char        *filein;
l_int32      w, h, d;
PIX         *pixs, *pixt, *pixt2, *pixt3, *pixt3a, *pixt4;
PIX         *pixg, *pixd, *pixd1, *pixd2, *pixd3;
PIXACC      *pacc;
PIXCMAP     *cmap;
static char  mainName[] = "graymorph_reg";

    if (argc != 2)
	exit(ERROR_INT(" Syntax:  graymorph_reg filein", mainName, 1));

    filein = argv[1];
    if ((pixs = pixRead(filein)) == NULL)
	exit(ERROR_INT("pixs not made", mainName, 1));
    pixGetDimensions(pixs, &w, &h, &d);
    if (d != 8)
	exit(ERROR_INT("pixs not 8 bpp", mainName, 1));

    /* -------- Test gray morph, including interpreter ------------ */
    pixd = pixDilateGray(pixs, WSIZE, HSIZE);
    sprintf(dilateseq, "D%d.%d", WSIZE, HSIZE);
    pixg = pixGrayMorphSequence(pixs, dilateseq, HORIZ_SEP, 0);
    pixCompare(pixd, pixg, "results are the same", "results are different" );
    pixDestroy(&pixg);
    pixDestroy(&pixd);

    pixd = pixErodeGray(pixs, WSIZE, HSIZE);
    sprintf(erodeseq, "E%d.%d", WSIZE, HSIZE);
    pixg = pixGrayMorphSequence(pixs, erodeseq, HORIZ_SEP, 100);
    pixCompare(pixd, pixg, "results are the same", "results are different" );
    pixDestroy(&pixg);
    pixDestroy(&pixd);

    pixd = pixOpenGray(pixs, WSIZE, HSIZE);
    sprintf(openseq, "O%d.%d", WSIZE, HSIZE);
    pixg = pixGrayMorphSequence(pixs, openseq, HORIZ_SEP, 200);
    pixCompare(pixd, pixg, "results are the same", "results are different" );
    pixDestroy(&pixg);
    pixDestroy(&pixd);

    pixd = pixCloseGray(pixs, WSIZE, HSIZE);
    sprintf(closeseq, "C%d.%d", WSIZE, HSIZE);
    pixg = pixGrayMorphSequence(pixs, closeseq, HORIZ_SEP, 300);
    pixCompare(pixd, pixg, "results are the same", "results are different" );
    pixDestroy(&pixg);
    pixDestroy(&pixd);

    pixd = pixTophat(pixs, WSIZE, HSIZE, L_TOPHAT_WHITE);
    sprintf(wtophatseq, "Tw%d.%d", WSIZE, HSIZE);
    pixg = pixGrayMorphSequence(pixs, wtophatseq, HORIZ_SEP, 400);
    pixCompare(pixd, pixg, "results are the same", "results are different" );
    pixDestroy(&pixg);
    pixDestroy(&pixd);

    pixd = pixTophat(pixs, WSIZE, HSIZE, L_TOPHAT_BLACK);
    sprintf(btophatseq, "Tb%d.%d", WSIZE, HSIZE);
    pixg = pixGrayMorphSequence(pixs, btophatseq, HORIZ_SEP, 500);
    pixCompare(pixd, pixg, "results are the same", "results are different" );
    pixDestroy(&pixg);

    /* ------------- Test erode/dilate duality -------------- */
    pixd = pixDilateGray(pixs, WSIZE, HSIZE);
    pixInvert(pixs, pixs);
    pixd2 = pixErodeGray(pixs, WSIZE, HSIZE);
    pixInvert(pixd2, pixd2);
    pixCompare(pixd, pixd2, "results are the same", "results are different" );
    pixDestroy(&pixd);
    pixDestroy(&pixd2);

    /* ------------- Test open/close duality -------------- */
    pixd = pixOpenGray(pixs, WSIZE, HSIZE);
    pixInvert(pixs, pixs);
    pixd2 = pixCloseGray(pixs, WSIZE, HSIZE);
    pixInvert(pixd2, pixd2);
    pixCompare(pixd, pixd2, "results are the same", "results are different" );
    pixDestroy(&pixd);
    pixDestroy(&pixd2);

    /* ------------- Test tophat duality -------------- */
    pixd = pixTophat(pixs, WSIZE, HSIZE, L_TOPHAT_WHITE);
    pixInvert(pixs, pixs);
    pixd2 = pixTophat(pixs, WSIZE, HSIZE, L_TOPHAT_BLACK);
    pixCompare(pixd, pixd2, "Correct: images are duals",
	       "Error: images are not duals" );
    pixDestroy(&pixd);
    pixDestroy(&pixd2);
    pixInvert(pixs, pixs);

    pixd = pixGrayMorphSequence(pixs, "Tw9.5", HORIZ_SEP, 100);
    pixInvert(pixs, pixs);
    pixd2 = pixGrayMorphSequence(pixs, "Tb9.5", HORIZ_SEP, 300);
    pixCompare(pixd, pixd2, "Correct: images are duals",
	       "Error: images are not duals" );
    pixDestroy(&pixd);
    pixDestroy(&pixd2);

    /* ------------- Test opening/closing for large sels -------------- */
    pixd = pixGrayMorphSequence(pixs,
	    "C9.9 + C19.19 + C29.29 + C39.39 + C49.49", HORIZ_SEP, 100);
    pixDestroy(&pixd);
    pixd = pixGrayMorphSequence(pixs,
	    "O9.9 + O19.19 + O29.29 + O39.39 + O49.49", HORIZ_SEP, 400);
    pixDestroy(&pixd);

    /* ---------- Closing plus white tophat result ------------ *
     *            Parameters: wsize, hsize = 9, 29             *
     * ---------------------------------------------------------*/
    pixd = pixCloseGray(pixs, 9, 9);
    pixd1 = pixTophat(pixd, 9, 9, L_TOPHAT_WHITE);
    pixd2 = pixGrayMorphSequence(pixs, "C9.9 + TW9.9", HORIZ_SEP, 0);
    pixCompare(pixd1, pixd2, "correct: same", "wrong: different");
    pixd3 = pixMaxDynamicRange(pixd1, L_LINEAR_SCALE);
    pixDisplayWrite(pixd3, 1);
    pixDestroy(&pixd);
    pixDestroy(&pixd1);
    pixDestroy(&pixd2);
    pixDestroy(&pixd3);
    pixd = pixCloseGray(pixs, 29, 29);
    pixd1 = pixTophat(pixd, 29, 29, L_TOPHAT_WHITE);
    pixd2 = pixGrayMorphSequence(pixs, "C29.29 + Tw29.29", HORIZ_SEP, 0);
    pixCompare(pixd1, pixd2, "correct: same", "wrong: different");
    pixd3 = pixMaxDynamicRange(pixd1, L_LINEAR_SCALE);
    pixDisplayWrite(pixd3, 1);
    pixDestroy(&pixd);
    pixDestroy(&pixd1);
    pixDestroy(&pixd2);
    pixDestroy(&pixd3);

    /* --------- hdome with parameter height = 100 ------------*/
    pixd = pixHDome(pixs, 100, 4);
    pixd2 = pixMaxDynamicRange(pixd, L_LINEAR_SCALE);
    pixDisplayWrite(pixd2, 1);
    pixDestroy(&pixd2);

    /* ----- Contrast enhancement with morph parameters 9, 9 -------*/
    pixd1 = pixInitAccumulate(w, h, 0x8000);
    pixAccumulate(pixd1, pixs, L_ARITH_ADD);
    pixMultConstAccumulate(pixd1, 3., 0x8000); 
    pixd2 = pixOpenGray(pixs, 9, 9);
    pixAccumulate(pixd1, pixd2, L_ARITH_SUBTRACT);
    pixDestroy(&pixd2);
    pixd2 = pixCloseGray(pixs, 9, 9);
    pixAccumulate(pixd1, pixd2, L_ARITH_SUBTRACT);
    pixDestroy(&pixd2);
    pixd = pixFinalAccumulate(pixd1, 0x8000, 8);
    pixDisplayWrite(pixd, 1);
    pixDestroy(&pixd1);

        /* Do the same thing with the Pixacc */
    pacc = pixaccCreate(w, h, 1);
    pixaccAdd(pacc, pixs);
    pixaccMultConst(pacc, 3.);
    pixd1 = pixOpenGray(pixs, 9, 9);
    pixaccSubtract(pacc, pixd1);
    pixDestroy(&pixd1);
    pixd1 = pixCloseGray(pixs, 9, 9);
    pixaccSubtract(pacc, pixd1);
    pixDestroy(&pixd1);
    pixd2 = pixaccFinal(pacc, 8);
    pixaccDestroy(&pacc);
    pixDisplayWrite(pixd2, 1);

    pixCompare(pixd, pixd2, "Correct: same", "Wrong: different");
    pixDestroy(&pixd);
    pixDestroy(&pixd2);


    /* ----  Tophat result on feynman stamp, to extract diagrams ----- */
    pixDestroy(&pixs);
    pixs = pixRead("feynman-stamp.jpg");

        /* Make output image to hold five intermediate images */
    w = pixGetWidth(pixs);
    h = pixGetHeight(pixs);
    pixd = pixCreate(5 * w + 18, h + 6, 32);  /* composite output image */
    pixSetAllArbitrary(pixd, 0x0000ff00);  /* set to blue */

        /* Paste in the input image */
    pixt = pixRemoveColormap(pixs, REMOVE_CMAP_TO_FULL_COLOR);
    pixRasterop(pixd, 3, 3, w, h, PIX_SRC, pixt, 0, 0);  /* 1st one */
/*    pixWrite("/tmp/junkgray.jpg", pixt, IFF_JFIF_JPEG); */
    pixDestroy(&pixt);

        /* Paste in the grayscale version */
    cmap = pixGetColormap(pixs);
    if (cmap)
	pixt = pixRemoveColormap(pixs, REMOVE_CMAP_TO_GRAYSCALE);
    else
        pixt = pixConvertRGBToGray(pixs, 0.33, 0.34, 0.33);
    pixt2 = pixConvertTo32(pixt);  /* 8 --> 32 bpp */
    pixRasterop(pixd, w + 6, 3, w, h, PIX_SRC, pixt2, 0, 0);  /* 2nd one */
    pixDestroy(&pixt2);

         /* Paste in a log dynamic range scaled version of the white tophat */
    pixt2 = pixTophat(pixt, 3, 3, L_TOPHAT_WHITE);
    pixt3a = pixMaxDynamicRange(pixt2, L_LOG_SCALE);
    pixt3 = pixConvertTo32(pixt3a);
    pixRasterop(pixd, 2 * w + 9, 3, w, h, PIX_SRC, pixt3, 0, 0);  /* 3rd */
/*    pixWrite("/tmp/junktophat.jpg", pixt2, IFF_JFIF_JPEG); */
    pixDestroy(&pixt3);
    pixDestroy(&pixt3a);
    pixDestroy(&pixt);

        /* Stretch the range and threshold to binary; paste it in */
    pixt3a = pixGammaTRC(NULL, pixt2, 1.0, 0, 80);
    pixt3 = pixThresholdToBinary(pixt3a, 70);
    pixt4 = pixConvertTo32(pixt3);
    pixRasterop(pixd, 3 * w + 12, 3, w, h, PIX_SRC, pixt4, 0, 0);  /* 4th */
/*    pixWrite("/tmp/junkbin.png", pixt3, IFF_PNG); */
    pixDestroy(&pixt2);
    pixDestroy(&pixt3a);
    pixDestroy(&pixt4);

        /* Invert; this is the final result */
    pixInvert(pixt3, pixt3);
    pixt4 = pixConvertTo32(pixt3);
    pixRasterop(pixd, 4 * w + 15, 3, w, h, PIX_SRC, pixt4, 0, 0);  /* 5th */
    pixWrite("/tmp/junkbininvert.png", pixt3, IFF_PNG);
    pixDisplayWrite(pixd, 1);
/*    pixWrite("/tmp/junkall.jpg", pixd, IFF_JFIF_JPEG); */
    pixDestroy(&pixt3);
    pixDestroy(&pixt4);
    pixDestroy(&pixd);

    pixDisplayMultiple("/tmp/junk_write_display*");
    pixDestroy(&pixs);
    return 0;
}



    /* simple comparison function */
static void pixCompare(PIX         *pix1,
                       PIX         *pix2,
                       const char  *msg1,
                       const char  *msg2)
{
l_int32  same;
    pixEqual(pix1, pix2, &same);
    if (same) {
	fprintf(stderr, "%s\n", msg1);
	pixDisplayWrite(pix1, 1);
    }
    else {
	fprintf(stderr, "%s\n", msg2);
	pixDisplayWrite(pix1, 1);
	pixDisplayWrite(pix2, 1);
    }
    return;
}


