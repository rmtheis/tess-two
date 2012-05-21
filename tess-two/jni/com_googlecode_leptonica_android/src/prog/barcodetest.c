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
 * barcodetest.c
 *
 *      barcodetest filein
 *
 *  For each barcode in the image, if the barcode format is supported,
 *  this deskews and crops it, and then decodes it twice:
 *      (1) as is (deskewed)
 *      (2) after 180 degree rotation
 */

#include <stdio.h>
#include <stdlib.h>
#include "allheaders.h"
#include "readbarcode.h"


main(int    argc,
     char **argv)
{
char        *filein;
PIX         *pixs;
SARRAY      *saw1, *saw2, *saw3, *sad1, *sad2, *sad3;
static char  mainName[] = "barcodetest";

    if (argc != 2)
	exit(ERROR_INT(" Syntax:  barcodetest filein", mainName, 1));

    filein = argv[1];
    if ((pixs = pixRead(filein)) == NULL)
	exit(ERROR_INT("pixs not made", mainName, 1));
	    
    sad1 = pixProcessBarcodes(pixs, L_BF_ANY, L_USE_WIDTHS, &saw1, 0);
    sarrayWrite("/tmp/junksaw1", saw1);
    sarrayWrite("/tmp/junksad1", sad1);
    sarrayDestroy(&saw1);
    sarrayDestroy(&sad1);

    pixRotate180(pixs, pixs);
    sad2 = pixProcessBarcodes(pixs, L_BF_ANY, L_USE_WIDTHS, &saw2, 0);
    sarrayWrite("/tmp/junksaw2", saw2);
    sarrayWrite("/tmp/junksad2", sad2);
    sarrayDestroy(&saw2);
    sarrayDestroy(&sad2);

/*    sad3 = pixProcessBarcodes(pixs, L_BF_ANY, L_USE_WINDOW, &saw3, 1);
    sarrayWrite("/tmp/junksaw3", saw3);
    sarrayWrite("/tmp/junksad3", sad3);
    sarrayDestroy(&saw3);
    sarrayDestroy(&sad3); */

    pixDestroy(&pixs);
    exit(0);
}

