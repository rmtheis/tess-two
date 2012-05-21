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

/*!
 *      Top-level fast hit-miss transform with auto-generated sels
 *
 *             PIX     *pixHMTDwa_1()
 *             PIX     *pixFHMTGen_1()
 */

#include <string.h>
#include "allheaders.h"

PIX *pixHMTDwa_1(PIX *pixd, PIX *pixs, char *selname);
PIX *pixFHMTGen_1(PIX *pixd, PIX *pixs, char *selname);
l_int32 fhmtgen_low_1(l_uint32 *datad, l_int32 w,
                      l_int32 h, l_int32 wpld,
                      l_uint32 *datas, l_int32 wpls,
                      l_int32 index);

static l_int32   NUM_SELS_GENERATED = 6;
static char  SEL_NAMES[][80] = {
                             "sel_3hm",
                             "sel_3de",
                             "sel_3ue",
                             "sel_3re",
                             "sel_3le",
                             "sel_sl1"};

/*!
 *  pixHMTDwa_1()
 *
 *      Input:  pixd (usual 3 choices: null, == pixs, != pixs)
 *              pixs (1 bpp)
 *              sel name
 *      Return: pixd
 *
 *  Notes:
 *      (1) This simply adds a 32 pixel border, calls the appropriate
 *          pixFHMTGen_*(), and removes the border.
 *          See notes below for that function.
 */
PIX *
pixHMTDwa_1(PIX   *pixd,
            PIX   *pixs,
            char  *selname)
{
PIX  *pixt1, *pixt2, *pixt3;

    PROCNAME("pixHMTDwa_1");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, pixd);
    if (pixGetDepth(pixs) != 1)
        return (PIX *)ERROR_PTR("pixs must be 1 bpp", procName, pixd);

    pixt1 = pixAddBorder(pixs, 32, 0);
    pixt2 = pixFHMTGen_1(NULL, pixt1, selname);
    pixt3 = pixRemoveBorder(pixt2, 32);
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);

    if (!pixd)
        return pixt3;

    pixCopy(pixd, pixt3);
    pixDestroy(&pixt3);
    return pixd;
}


/*!
 *  pixFHMTGen_1()
 *
 *      Input:  pixd (usual 3 choices: null, == pixs, != pixs)
 *              pixs (1 bpp)
 *              sel name
 *      Return: pixd
 *
 *  Notes:
 *      (1) This is a dwa implementation of the hit-miss transform
 *          on pixs by the sel.
 *      (2) The sel must be limited in size to not more than 31 pixels
 *          about the origin.  It must have at least one hit, and it
 *          can have any number of misses.
 *      (3) This handles all required setting of the border pixels
 *          before erosion and dilation.
 */
PIX *
pixFHMTGen_1(PIX   *pixd,
             PIX   *pixs,
             char  *selname)
{
l_int32    i, index, found, w, h, wpls, wpld;
l_uint32  *datad, *datas, *datat;
PIX       *pixt;

    PROCNAME("pixFHMTGen_1");

    if (!pixs)
        return (PIX *)ERROR_PTR("pixs not defined", procName, pixd);
    if (pixGetDepth(pixs) != 1)
        return (PIX *)ERROR_PTR("pixs must be 1 bpp", procName, pixd);

    found = FALSE;
    for (i = 0; i < NUM_SELS_GENERATED; i++) {
        if (strcmp(selname, SEL_NAMES[i]) == 0) {
            found = TRUE;
            index = i;
            break;
        }
    }
    if (found == FALSE)
        return (PIX *)ERROR_PTR("sel index not found", procName, pixd);

    if (!pixd) {
        if ((pixd = pixCreateTemplate(pixs)) == NULL)
            return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    }
    else  /* for in-place or pre-allocated */
        pixResizeImageData(pixd, pixs);
    wpls = pixGetWpl(pixs);
    wpld = pixGetWpl(pixd);

        /*  The images must be surrounded with 32 additional border
         *  pixels, that we'll read from.  We fabricate a "proper"
         *  image as the subimage within the border, having the 
         *  following parameters:  */
    w = pixGetWidth(pixs) - 64;
    h = pixGetHeight(pixs) - 64;
    datas = pixGetData(pixs) + 32 * wpls + 1;
    datad = pixGetData(pixd) + 32 * wpld + 1;

    if (pixd == pixs) {  /* need temp image if in-place */
        if ((pixt = pixCopy(NULL, pixs)) == NULL)
            return (PIX *)ERROR_PTR("pixt not made", procName, pixd);
        datat = pixGetData(pixt) + 32 * wpls + 1;
        fhmtgen_low_1(datad, w, h, wpld, datat, wpls, index);
        pixDestroy(&pixt);
    }
    else {  /* not in-place */
        fhmtgen_low_1(datad, w, h, wpld, datas, wpls, index);
    }

    return pixd;
}

