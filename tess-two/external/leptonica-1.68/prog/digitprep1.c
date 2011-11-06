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
 * digitprep1.c
 *
 *   Extract barcode digits and put in a pixaa (a resource file for
 *   readnum.c).
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"

static const l_int32  HEIGHT = 32;  /* pixels */

main(int    argc,
     char **argv)
{
char         buf[8];
l_int32      i, n, h;
l_float32    scalefact;
BOXA        *boxa;
PIX         *pixs, *pix, *pixt1, *pixt2;
PIXA        *pixa, *pixas, *pixad;
PIXAA       *pixaa;
static char  mainName[] = "digitprep1";

    if (argc != 1) {
        ERROR_INT(" Syntax: digitprep1", mainName, 1);
        return 1;
    }

    if ((pixs = pixRead("barcode-digits.png")) == NULL)
        return ERROR_INT("pixs not read", mainName, 1);

        /* Extract the digits and scale to HEIGHT */
    boxa = pixConnComp(pixs, &pixa, 8);
    pixas = pixaSort(pixa, L_SORT_BY_X, L_SORT_INCREASING, NULL, L_CLONE);
    n = pixaGetCount(pixas);

        /* Move the last ("0") to the first position */
    pixt1 = pixaGetPix(pixas, n - 1, L_CLONE);
    pixaInsertPix(pixas, 0, pixt1, NULL);
    pixaRemovePix(pixas, n);

        /* Make the output scaled pixa */
    pixad = pixaCreate(n);
    for (i = 0; i < n; i++) {
        pixt1 = pixaGetPix(pixas, i, L_CLONE);
        pixGetDimensions(pixt1, NULL, &h, NULL);
        scalefact = HEIGHT / (l_float32)h;
        pixt2 = pixScale(pixt1, scalefact, scalefact);
        if (pixGetHeight(pixt2) != 32)
            return ERROR_INT("height not 32!", mainName, 1);
        sprintf(buf, "%d", i);
        pixSetText(pixt2, buf);
        pixaAddPix(pixad, pixt2, L_INSERT);
        pixDestroy(&pixt1);
    }

        /* Save in a pixaa, with 1 pix in each pixa */
    pixaa = pixaaCreateFromPixa(pixad, 1, L_CHOOSE_CONSECUTIVE, L_CLONE);
    pixaaWrite("junkdigits.pixaa", pixaa);

        /* Show result */
    pixt1 = pixaaDisplayByPixa(pixaa, 20, 20, 1000);
    pixDisplay(pixt1, 100, 100);
    pixDestroy(&pixt1);

    boxaDestroy(&boxa);
    pixaDestroy(&pixa);
    pixaDestroy(&pixas);
    pixaDestroy(&pixad);
    pixaaDestroy(&pixaa);
    return 0;
}


