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
 *   viewfiles.c
 *
 *     Generate smaller images for viewing and write html
 *        l_int32    pixHtmlViewer()
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "allheaders.h"

    /* MS VC++ can't handle array initialization with static consts ! */
#define L_BUF_SIZE      512

static const l_int32  DEFAULT_THUMB_WIDTH = 120;
static const l_int32  DEFAULT_VIEW_WIDTH = 800;
static const l_int32  MIN_THUMB_WIDTH = 50;
static const l_int32  MIN_VIEW_WIDTH = 300;


/*---------------------------------------------------------------------*
 *            Generate smaller images for viewing and write html       *
 *---------------------------------------------------------------------*/
/*!
 *  pixHtmlViewer()
 *
 *      Input:  dirin:  directory of input image files
 *              dirout: directory for output files
 *              rootname: root name for output files
 *              thumbwidth:  width of thumb images
 *                           (in pixels; use 0 for default)
 *              viewwidth:  maximum width of view images (no up-scaling)
 *                           (in pixels; use 0 for default)
 *              copyorig:  1 to copy originals to dirout; 0 otherwise
 *      Return: 0 if OK; 1 on error
 *
 *  Notes:
 *      (1) The thumb and view reduced images are generated,
 *          along with two html files:
 *             <rootname>.html and <rootname>-links.html
 *      (2) The thumb and view files are named
 *             <rootname>_thumb_xxx.jpg
 *             <rootname>_view_xxx.jpg
 *          With this naming scheme, any number of input directories
 *          of images can be processed into views and thumbs
 *          and placed in the same output directory.
 */
l_int32
pixHtmlViewer(const char  *dirin,
              const char  *dirout,
              const char  *rootname,
              l_int32      thumbwidth,
              l_int32      viewwidth,
              l_int32      copyorig)
{
char      *fname, *fullname, *outname;
char      *mainname, *linkname, *linknameshort;
char      *viewfile, *thumbfile;
char      *shtml, *slink;
char       charbuf[L_BUF_SIZE];
char       htmlstring[] = "<html>";
char       framestring[] = "</frameset></html>";
l_int32    i, nfiles, index, w, nimages;
l_float32  factor;
PIX       *pix, *pixthumb, *pixview;
SARRAY    *safiles, *sathumbs, *saviews, *sahtml, *salink;

    PROCNAME("pixHtmlViewer");

    if (!dirin)
        return ERROR_INT("dirin not defined", procName, 1);
    if (!dirout)
        return ERROR_INT("dirout not defined", procName, 1);
    if (!rootname)
        return ERROR_INT("rootname not defined", procName, 1);

    if (thumbwidth == 0)
        thumbwidth = DEFAULT_THUMB_WIDTH;
    if (thumbwidth < MIN_THUMB_WIDTH) {
        L_WARNING("thumbwidth too small; using min value", procName);
        thumbwidth = MIN_THUMB_WIDTH;
    }
    if (viewwidth == 0)
        viewwidth = DEFAULT_VIEW_WIDTH;
    if (viewwidth < MIN_VIEW_WIDTH) {
        L_WARNING("viewwidth too small; using min value", procName);
        viewwidth = MIN_VIEW_WIDTH;
    }

        /* Make the output directory if it doesn't already exist */
    sprintf(charbuf, "mkdir -p %s", dirout);
    system(charbuf);

        /* Capture the filenames in the input directory */
    if ((safiles = getFilenamesInDirectory(dirin)) == NULL)
        return ERROR_INT("safiles not made", procName, 1);

        /* Generate output text file names */
    sprintf(charbuf, "%s/%s.html", dirout, rootname);
    mainname = stringNew(charbuf);
    sprintf(charbuf, "%s/%s-links.html", dirout, rootname);
    linkname = stringNew(charbuf);
    linknameshort = stringJoin(rootname, "-links.html");

    if ((sathumbs = sarrayCreate(0)) == NULL)
        return ERROR_INT("sathumbs not made", procName, 1);
    if ((saviews = sarrayCreate(0)) == NULL)
        return ERROR_INT("saviews not made", procName, 1);

        /* Generate the thumbs and views */
    nfiles = sarrayGetCount(safiles);
    index = 0;
    for (i = 0; i < nfiles; i++) {
        fname = sarrayGetString(safiles, i, L_NOCOPY);
        fullname = genPathname(dirin, fname);
        fprintf(stderr, "name: %s\n", fullname);
        if ((pix = pixRead(fullname)) == NULL) {
            fprintf(stderr, "file %s not a readable image\n", fullname);
            FREE(fullname);
            continue;
        }
        FREE(fullname);
        if (copyorig) {
            outname = genPathname(dirout, fname);
            pixWrite(outname, pix, IFF_JFIF_JPEG);
            FREE(outname);
        }

            /* Make and store the thumb */
        w = pixGetWidth(pix);
        factor = (l_float32)thumbwidth / (l_float32)w;
        if ((pixthumb = pixScale(pix, factor, factor)) == NULL)
            return ERROR_INT("pixthumb not made", procName, 1);
        sprintf(charbuf, "%s_thumb_%03d.jpg", rootname, index);
        sarrayAddString(sathumbs, charbuf, L_COPY);
        outname = genPathname(dirout, charbuf);
        pixWrite(outname, pixthumb, IFF_JFIF_JPEG);
        FREE(outname);
        pixDestroy(&pixthumb);

            /* Make and store the view */
        factor = (l_float32)viewwidth / (l_float32)w;
        if (factor >= 1.0)
            pixview = pixClone(pix);   /* no upscaling */
        else {
            if ((pixview = pixScale(pix, factor, factor)) == NULL)
                return ERROR_INT("pixview not made", procName, 1);
        }
        sprintf(charbuf, "%s_view_%03d.jpg", rootname, index);
        sarrayAddString(saviews, charbuf, L_COPY);
        outname = genPathname(dirout, charbuf);
        pixWrite(outname, pixview, IFF_JFIF_JPEG);
        FREE(outname);
        pixDestroy(&pixview);

        pixDestroy(&pix);
        index++;
    }

        /* Generate the main html file */
    if ((sahtml = sarrayCreate(0)) == NULL)
        return ERROR_INT("sahtml not made", procName, 1);
    sarrayAddString(sahtml, htmlstring, L_COPY);
    sprintf(charbuf, "<frameset cols=\"%d, *\">", thumbwidth + 30);
    sarrayAddString(sahtml, charbuf, L_COPY);
    sprintf(charbuf, "<frame name=\"thumbs\" src=\"%s\">", linknameshort); 
    sarrayAddString(sahtml, charbuf, L_COPY);
    sprintf(charbuf, "<frame name=\"views\" src=\"%s\">",
            sarrayGetString(saviews, 0, L_NOCOPY)); 
    sarrayAddString(sahtml, charbuf, L_COPY);
    sarrayAddString(sahtml, framestring, L_COPY);
    shtml = sarrayToString(sahtml, 1);
    arrayWrite(mainname, "w", shtml, strlen(shtml));
    FREE(shtml);
    FREE(mainname);

        /* Generate the link html file */
    nimages = sarrayGetCount(saviews);
    fprintf(stderr, "num. images = %d\n", nimages);
    if ((salink = sarrayCreate(0)) == NULL)
        return ERROR_INT("salink not made", procName, 1);
    for (i = 0; i < nimages; i++) {
        viewfile = sarrayGetString(saviews, i, L_NOCOPY);
        thumbfile = sarrayGetString(sathumbs, i, L_NOCOPY);
        sprintf(charbuf, "<a href=\"%s\" TARGET=views><img src=\"%s\"></a>",
            viewfile, thumbfile);
        sarrayAddString(salink, charbuf, L_COPY);
    }
    slink = sarrayToString(salink, 1);
    arrayWrite(linkname, "w", slink, strlen(slink));
    FREE(slink);
    FREE(linkname);
    FREE(linknameshort);

    sarrayDestroy(&safiles);
    sarrayDestroy(&sathumbs);
    sarrayDestroy(&saviews);
    sarrayDestroy(&sahtml);
    sarrayDestroy(&salink);

    return 0;
}

