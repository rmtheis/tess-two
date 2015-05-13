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
 *  recogtest1.c
 *
 *     Tests the recog utility using the bootstrap number set,
 *     for both training and identification
 */

#include "string.h"
#include "allheaders.h"


    /* Set match method */
static const l_int32 match_method = L_USE_AVERAGE;
/* static const l_int32 match_method = L_USE_ALL; */

static const l_int32 scaledw = 20;
static const l_int32 scaledh = 32;

l_int32 main(int    argc,
             char **argv)
{
l_int32    i, j, n, index, w, h, ignore;
l_float32  score;
char      *fname, *strchar;
char       buf[256];
BOX       *box;
BOXA      *boxat;
NUMA      *naindex, *nascore, *naindext, *nascoret;
PIX       *pixs, *pixd, *pixt, *pixdb;
PIXA      *pixa, *pixat;
L_RECOG   *recog, *recog2;
L_RECOGA  *recoga;
SARRAY    *sa, *satext;

    if (argc != 1) {
        fprintf(stderr, " Syntax: recogtest1\n");
        return 1;
    }

    recog = NULL;
    recog2 = NULL;

#if 0  /* Generate a simple bootstrap pixa (bootnum1.pa) for
          number images in directory 'recog/bootnums' */
    recog = recogCreate(scaledw, scaledh, match_method, 100, 1);
    sa = getSortedPathnamesInDirectory("recog/bootnums", "png", 0, 0);
    n = sarrayGetCount(sa);
    for (i = 0; i < n; i++) {
        fname = sarrayGetString(sa, i, L_NOCOPY);
        if ((pixs = pixRead(fname)) == NULL) {
            fprintf(stderr, "Can't read %s\n", fname);
            continue;
        }
        pixGetDimensions(pixs, &w, &h, NULL);
        box = boxCreate(0, 0, w, h);
        recogTrainLabelled(recog, pixs, box, NULL, 1, 1);
        pixDestroy(&pixs);
        boxDestroy(&box);
    }
    recogTrainingFinished(recog, 1);
    recogWritePixa("/tmp/bootnum1.pa", recog);
    snprintf(buf, sizeof(buf),
             "displaypixa /tmp/bootnum1.pa 1.0 2 1 0 /tmp/bootnum1.png fonts");
    ignore = system(buf);
    sarrayDestroy(&sa);
#else
    pixa = pixaRead("recog/digits/bootnum1.pa");
    recog = recogCreateFromPixa(pixa, scaledw, scaledh, match_method, 120, 1);
    snprintf(buf, sizeof(buf),
        "displaypixa recog/digits/bootnum1.pa 1.0 2 1 0 "
        "/tmp/bootnum1.png fonts");
    ignore = system(buf);
    pixaDestroy(&pixa);
#endif

#if 0  /* roman, one per image */
    recog = recogCreate(20, 32, match_method, 100, 1);
    sa = getSortedPathnamesInDirectory("charset", "png", 0, 0);
    n = sarrayGetCount(sa);
    for (i = 0; i < n; i++) {
        fname = sarrayGetString(sa, i, L_NOCOPY);
        if ((pixs = pixRead(fname)) == NULL) {
            fprintf(stderr, "Can't read %s\n", fname);
            continue;
        }
        pixGetDimensions(pixs, &w, &h, NULL);
        box = boxCreate(0, 0, w, h);
        recogTrainLabelled(recog, pixs, box, NULL, 0, 1);
        pixDestroy(&pixs);
        boxDestroy(&box);
    }
    recogTrainingFinished(recog, 1);
    sarrayDestroy(&sa);
#endif

#if 1
    fprintf(stderr, "Print Stats\n");
    recogShowContent(stderr, recog, 1);
#endif

#if 1
    fprintf(stderr, "AverageSamples\n");
    recogAverageSamples(recog, 1);
    pixt = pixaGetPix(recog->pixadb_ave, 0, L_CLONE);
    pixWrite("/tmp/unscaled_ave.png", pixt, IFF_PNG);
    pixDestroy(&pixt);
    pixt = pixaGetPix(recog->pixadb_ave, 1, L_CLONE);
    pixWrite("/tmp/scaled_ave.png", pixt, IFF_PNG);
    pixDestroy(&pixt);
#endif


#if 1
        /* Split touching characters */
    fprintf(stderr, "Split touching\n");
    pixd = pixRead("pagenums/pagenum.29.png");  /* 25 or 29 */
    recoga = recogaCreateFromRecog(recog);
    recogaIdentifyMultiple(recoga, pixd, 3, -1, -1, &boxat, &pixat, &pixdb, 1);
    pixDisplay(pixdb, 800, 800);
    boxaWriteStream(stderr, boxat);
    pixt = pixaDisplay(pixat, 0, 0);
    pixDisplay(pixt, 1200, 800);
    pixDestroy(&pixdb);
    pixDestroy(&pixt);
    pixDestroy(&pixd);
    pixaDestroy(&pixat);
    boxaDestroy(&boxat);
#endif


#if 1
    recogDebugAverages(recog, 0);
    recogShowMatchesInRange(recog, recog->pixa_tr, 0.65, 1.0, 0);
    pixWrite("/tmp/match_ave1.png", recog->pixdb_range, IFF_PNG);
    recogShowMatchesInRange(recog, recog->pixa_tr, 0.0, 1.0, 0);
    pixWrite("/tmp/match_ave2.png", recog->pixdb_range, IFF_PNG);
#endif

#if 0
        /* Show that the pixa interface works for the entire set */
    recogIdentifyPixa(recog, recog->pixa_tr, NULL, &pixd);
    pixWrite("/tmp/pixd2", pixd, IFF_PNG);
    n = numaGetCount(naindex);
    for (i = 0; i < n; i++) {
        numaGetIValue(naindex, i, &index);
        numaGetFValue(nascore, i, &score);
        strchar = sarrayGetString(satext, i, L_NOCOPY);
        fprintf(stderr, "%d: index = %d, text = %s, score = %5.3f\n",
                i, index, strchar, score);
    }
    pixDisplay(pixd, 0, 100);
    pixDestroy(&pixd);
    pixaDestroy(&pixa);
    numaDestroy(&naindex);
    sarrayDestroy(&satext);
    numaDestroy(&nascore);
#endif

#if 1
        /* We can do about 5M correlations/sec */
    fprintf(stderr, "Remove outliers\n");
    recogRemoveOutliers(recog, 0.7, 0.5, 1);
#endif

#if 1
    fprintf(stderr, "Debug averages\n");
    recogDebugAverages(recog, 0);
    pixWrite("/tmp/averages.png", recog->pixdb_ave, IFF_PNG);
#endif

#if 1
    fprintf(stderr, "Print stats 2\n");
    recogShowContent(stderr, recog, 1);
    recogWrite("/tmp/rec1.rec", recog);
    recog2 = recogRead("/tmp/rec1.rec");
    recogResetBmf(recog2, 0);
    recogWrite("/tmp/rec2.rec", recog2);

    fprintf(stderr, "Debug averages 2\n");
    recogDebugAverages(recog2, 1);
    recogShowMatchesInRange(recog2, recog->pixa_tr, 0.0, 1.0, 1);
    pixWrite("/tmp/match_ave3.png", recog2->pixdb_range, IFF_PNG);
    recogDestroy(&recog2);
#endif

    recogaDestroy(&recoga);
    return 0;
}
