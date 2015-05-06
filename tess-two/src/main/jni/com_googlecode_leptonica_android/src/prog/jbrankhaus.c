/*====================================================================*
 -  Copyright (C) 2001 Leptonica.  All rights reserved.
 -
 -  Redistribution and use in source and binary forms, with or without
 -  modification, are permitted provided that the following conditions
 -  are met:
 -  1. Redistributions of source code must retain the above copyright
 -     notice, this list of conditions and the following disclaimer.
 -  2. Redistributions in binary form must reproduce the above
 -     copyright notice, this list of conditions and the following
 -     disclaimer in the documentation and/or other materials
 -     provided with the distribution.
 -
 -  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 -  ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 -  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 -  A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL ANY
 -  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 -  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 -  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 -  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 -  OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 -  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 -  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *====================================================================*/

/*
 * jbrankhaus.c
 *
 *     jbrankhaus dirin size rank rootname [firstpage npages]
 *
 *         dirin:  directory of input pages
 *         size: size of SE used for dilation
 *         rank: min pixel fraction required in both directions in match
 *         rootname: used for naming the two output files (templates
 *                   and c.c. data)
 *
 * Notes:
 *     (1) All components larger than a default size are not saved.
 *         The default size is given in jbclass.c.
 *     (2) A set of reasonable values for cc or characters, that
 *         gives good accuracy without too manyclasses, is:
 *               size = 2  (2 x 2 structuring element)
 *               rank = 0.97
 */

#include "allheaders.h"

    /* Choose one of these */
#define  COMPONENTS  JB_CONN_COMPS
/* #define  COMPONENTS  JB_CHARACTERS */
/* #define  COMPONENTS  JB_WORDS */

#define   BUF_SIZE         512

    /* select additional debug output */
#define   DEBUG_TEST_DATA_IO        0
#define   RENDER_DEBUG              1
#define   DISPLAY_DIFFERENCE        1
#define   DISPLAY_ALL_INSTANCES     0

    /* for display output of all instances, sorted by class */
#define   X_SPACING                10
#define   Y_SPACING                15
#define   MAX_OUTPUT_WIDTH        400


int main(int    argc,
         char **argv)
{
char         filename[BUF_SIZE];
char        *dirin, *rootname, *fname;
l_int32      i, size, firstpage, npages, nfiles;
l_float32    rank;
JBDATA      *data;
JBCLASSER   *classer;
SARRAY      *safiles;
PIX         *pix, *pixt;
PIXA        *pixa, *pixadb;
static char  mainName[] = "jbrankhaus";

    if (argc != 5 && argc != 7)
        return ERROR_INT(
             " Syntax: jbrankhaus dirin size rank rootname [firstpage, npages]",
             mainName, 1);

    dirin = argv[1];
    size = atoi(argv[2]);
    rank = atof(argv[3]);
    rootname = argv[4];

    if (argc == 5) {
        firstpage = 0;
        npages = 0;
    }
    else {
        firstpage = atoi(argv[5]);
        npages = atoi(argv[6]);
    }

#if 1

    /*--------------------------------------------------------------*/

    jbRankHaus(dirin, size, rank, COMPONENTS, rootname, firstpage, npages, 1);

    /*--------------------------------------------------------------*/

#else

    /*--------------------------------------------------------------*/

    safiles = getSortedPathnamesInDirectory(dirin, NULL, firstpage, npages);
    nfiles = sarrayGetCount(safiles);

/*    sarrayWriteStream(stderr, safiles); */

        /* Classify components on requested pages */
    startTimer();
    classer = jbRankHausInit(COMPONENTS, 0, 0, size, rank);
    jbAddPages(classer, safiles);
    fprintf(stderr, "Time to classify components: %6.3f sec\n", stopTimer());

        /* Save and write out the result */
    data = jbDataSave(classer);
    jbDataWrite(rootname, data);

        /* Render the pages from the classifier data.
         * Use debugflag == FALSE to omit outlines of each component. */
    pixa = jbDataRender(data, FALSE);

        /* Write the pages out */
    npages = pixaGetCount(pixa);
    if (npages != nfiles)
        fprintf(stderr, "npages = %d, nfiles = %d, not equal!\n",
                npages, nfiles);
    for (i = 0; i < npages; i++) {
        pix = pixaGetPix(pixa, i, L_CLONE);
        snprintf(filename, BUF_SIZE, "%s.%05d", rootname, i);
        fprintf(stderr, "filename: %s\n", filename);
        pixWrite(filename, pix, IFF_PNG);
        pixDestroy(&pix);
    }

#if  DISPLAY_DIFFERENCE
    fname = sarrayGetString(safiles, 0, 0);
    pixt = pixRead(fname);
    pix = pixaGetPix(pixa, 0, L_CLONE);
    pixXor(pixt, pixt, pix);
    pixWrite("junk_output_diff", pixt, IFF_PNG);
    pixDestroy(&pix);
    pixDestroy(&pixt);
#endif  /* DISPLAY_DIFFERENCE */

#if  DEBUG_TEST_DATA_IO
{ JBDATA  *newdata;
  PIX     *newpix;
  PIXA    *newpixa;
  l_int32  same, iofail;
        /* Read the data back in and render the pages */
    newdata = jbDataRead(rootname);
    newpixa = jbDataRender(newdata, FALSE);
    iofail = FALSE;
    for (i = 0; i < npages; i++) {
        pix = pixaGetPix(pixa, i, L_CLONE);
        newpix = pixaGetPix(newpixa, i, L_CLONE);
        pixEqual(pix, newpix, &same);
        if (!same) {
            iofail = TRUE;
            fprintf(stderr, "pix on page %d are unequal!\n", i);
        }
        pixDestroy(&pix);
        pixDestroy(&newpix);

    }
    if (iofail)
        fprintf(stderr, "read/write for jbdata fails\n");
    else
        fprintf(stderr, "read/write for jbdata succeeds\n");
    jbDataDestroy(&newdata);
    pixaDestroy(&newpixa);
}
#endif  /* DEBUG_TEST_DATA_IO */

#if  RENDER_DEBUG
        /* Use debugflag == TRUE to see outlines of each component. */
    pixadb = jbDataRender(data, TRUE);
        /* Write the debug pages out */
    npages = pixaGetCount(pixadb);
    for (i = 0; i < npages; i++) {
        pix = pixaGetPix(pixadb, i, L_CLONE);
        snprintf(filename, BUF_SIZE, "%s.db.%05d", rootname, i);
        fprintf(stderr, "filename: %s\n", filename);
        pixWrite(filename, pix, IFF_PNG);
        pixDestroy(&pix);
    }
    pixaDestroy(&pixadb);
#endif  /* RENDER_DEBUG */

#if  DISPLAY_ALL_INSTANCES
        /* display all instances, organized by template */
    pix = pixaaDisplayByPixa(classer->pixaa,
                             X_SPACING, Y_SPACING, MAX_OUTPUT_WIDTH);
    pixWrite("output_instances", pix, IFF_PNG);
    pixDestroy(&pix);
#endif  /* DISPLAY_ALL_INSTANCES */

    pixaDestroy(&pixa);
    sarrayDestroy(&safiles);
    jbClasserDestroy(&classer);
    jbDataDestroy(&data);

    /*--------------------------------------------------------------*/

#endif

    return 0;
}

