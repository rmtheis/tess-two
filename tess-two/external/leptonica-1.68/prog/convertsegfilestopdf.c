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
 * convertsegfilestopdf.c
 *
 *    Converts all image files in the given directory with matching substring
 *    to a pdf, with the specified scaling factor <= 1.0.  To convert
 *    all files in the directory, use 'allfiles' for the substring.
 *
 *    See below for syntax and usage.
 *
 *    The image regions are displayed at a resolution that depends on the
 *    input resolution (res) and the scaling factor (scalefact) that
 *    is applied to the images before conversion to pdf.  Internally
 *    we multiply these, so that the generated pdf will render at the
 *    same resolution as if it hadn't been scaled.  By downscaling, this:
 *       (1) reduces the size of the images.  For jpeg, downscaling
 *           reduces by square of the scale factor the 'image' segmented part.
 *       (2) regenerates the jpeg with quality = 75 after downscaling.
 *
 *    If boxaafile doesn't exist, the images are converted without
 *    scaling and with the best compression for each image.
 *
 *    To see how this works:
 *       (1) run pdfseg_reg
 *           This generates image and boxaa files in /tmp/segtest/
 *       (2) run convertsegfilestopdf:
 *           convertsegfilestopdf /tmp/segtest ".jpg" 100 2 140
 *              /tmp/segtest/seg.baa 1.0 segtest /tmp/segtest.pdf
 */

#include <string.h>
#include "allheaders.h"

main(int    argc,
     char **argv)
{
char        *dirin, *substr, *title, *fileout, *boxaafile, *boxaapath;
l_int32      ret, res, type, thresh;
l_float32    scalefactor;
BOXAA       *baa;
static char  mainName[] = "convertsegfilestopdf";

    if (argc != 10) {
        fprintf(stderr,
	    " Syntax: convertsegfilestopdf dirin substr res type thresh \\ \n"
            "                       boxaafile scalefactor title fileout\n"
            "     where\n"
            "         dirin:  input directory for image files\n"
            "         substr:  Use 'allfiles' to convert all files\n"
            "                  in the directory\n"
            "         res:  Input resolution of each image;\n"
            "               assumed to all be the same\n"
            "         type: compression used for non-image regions:\n"
            "               0: default (G4 encoding)\n"
            "               1: JPEG encoding\n"
            "               2: G4 encoding\n"
            "               3: PNG encoding\n"
            "         thresh:  threshold for binarization; use 0 for default\n"
            "         boxaafile: File of 'image' regions within each page\n"
            "                    This contains a boxa for each page,\n"
            "                    consisting of a set of regions\n"
            "         scalefactor:  Use to scale down the image regions\n"
            "         title:  Use 'none' to omit\n"
            "         fileout:  Output pdf file\n");
        return 1;
    }

    dirin = argv[1];
    substr = argv[2];
    res = atoi(argv[3]);
    type = atoi(argv[4]);
    thresh = atoi(argv[5]);
    boxaafile = argv[6];
    scalefactor = atof(argv[7]);
    title = argv[8];
    fileout = argv[9];

    if (!strcmp(substr, "allfiles"))
        substr = NULL;
    if (scalefactor <= 0.0 || scalefactor > 1.0) {
        L_WARNING("invalid scalefactor: setting to 1.0", mainName);
        scalefactor = 1.0;
    }
    if (type != 1 && type != 2 && type != 3)
        type = L_G4_ENCODE;
    if (thresh <= 0)
        thresh = 150;
    if (!strcmp(title, "none"))
        title = NULL;

    boxaapath = genPathname(boxaafile, NULL);
    if ((baa = boxaaRead(boxaapath)) == NULL) {
        L_WARNING(
            "boxaa file not found; converting unsegmented and unscaled",
            mainName);
        ret = convertFilesToPdf(dirin, substr, res, 1.0, 75, title,
                                fileout);
        FREE(boxaapath);
        return ret;
    }

    ret = convertSegmentedFilesToPdf(dirin, substr, res, type, thresh, baa,
                                     75, scalefactor, title, fileout);
    FREE(boxaapath);
    boxaaDestroy(&baa);
    return ret;
}

