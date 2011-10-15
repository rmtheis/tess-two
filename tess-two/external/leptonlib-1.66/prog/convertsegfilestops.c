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
 * convertsegfilestops.c
 *
 *    Converts all image files in a 'page' directory, using optional
 *    corresponding segmentation mask files in a 'mask' directory,
 *    to a level 2 compressed PostScript file.  This is done
 *    automatically at a resolution that fits to a letter-sized
 *    (8.5 x 11) inch page.  The 'page' and 'mask' files are paired
 *    by having the same number embedded in their name.
 *    The 'numpre' and 'numpost' args specify the number of
 *    characters at the beginning and end of the filename (not
 *    counting any extension) that are NOT part of the page number.
 *
 *    The mask directory must exist, but it does not need to have
 *    any image mask files.
 *
 *    The pages are taken in lexical order of the filenames.  Therefore,
 *    the embedded numbers should be 0-padded on the left up to
 *    a fixed number of digits.
 *
 *    PostScript (and pdf) allow regions of the image to be encoded
 *    differently.  Regions can be over-written, with the last writing
 *    determining the final output.  Black "ink" can also be written
 *    through a mask that is given by a 1 bpp image.
 *
 *    The page images are typically grayscale or color.  To take advantage
 *    of this depth, one typically upscales the text by 2.0.  Likewise,
 *    the images regions, denoted by foreground in the corresponding
 *    segmentation mask, can be rendered at lower resolution, and
 *    it is often useful to downscale the image parts by 0.5.
 *
 *    If the mask does not exist, the entire page is interpreted as
 *    text; it is converted to 1 bpp and written to file with
 *    ccitt-g4 compression at the requested "textscale" relative
 *    to the page image.   If the mask exists and the foreground
 *    covers the entire page, the entire page is saved with jpeg
 *    ("dct") compression at the requested "imagescale".
 *    If the mask exists and partially covers the page image, the
 *    page is saved as a mixture of grayscale or rgb dct and 1 bpp g4.
 *
 *    This uses a single global threshold for binarizing the text
 *    (i.e., non-image) regions of every page.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "allheaders.h"


main(int    argc,
     char **argv)
{
char        *pagedir, *pagestr, *maskdir, *maskstr, *fileout;
l_int32      threshold, numpre, numpost, maxnum;
l_float32    textscale, imagescale;    
static char  mainName[] = "convertsegfilestops";

    if (argc != 12)
	exit(ERROR_INT(
         " Syntax:  convertsegfilestops pagedir pagestr maskdir maskstr"
                   " numpre numpost maxnum"
                   " textscale imagescale thresh fileout",
         mainName, 1));

    pagedir = argv[1];
    pagestr = argv[2];
    maskdir = argv[3];
    maskstr = argv[4];
    numpre = atoi(argv[5]);
    numpost = atoi(argv[6]);
    maxnum = atoi(argv[7]);
    textscale = atof(argv[8]);
    imagescale = atof(argv[9]);
    threshold = atoi(argv[10]);
    fileout = argv[11];

    return convertSegmentedPagesToPS(pagedir, pagestr, maskdir, maskstr,
                                     numpre, numpost, maxnum, textscale,
                                     imagescale, threshold, fileout);
}


