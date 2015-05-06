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
 *   viewfiles.c
 *
 *     Generate smaller images for viewing and write html
 *        l_int32    pixHtmlViewer()
 */

#include <string.h>
#include "allheaders.h"

#ifdef _WIN32
#include <windows.h>   /* for CreateDirectory() */
#endif

static const l_int32  L_BUF_SIZE = 512;
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
l_int32    i, nfiles, index, w, nimages, ret;
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
        L_WARNING("thumbwidth too small; using min value\n", procName);
        thumbwidth = MIN_THUMB_WIDTH;
    }
    if (viewwidth == 0)
        viewwidth = DEFAULT_VIEW_WIDTH;
    if (viewwidth < MIN_VIEW_WIDTH) {
        L_WARNING("viewwidth too small; using min value\n", procName);
        viewwidth = MIN_VIEW_WIDTH;
    }

        /* Make the output directory if it doesn't already exist */
#ifndef _WIN32
    sprintf(charbuf, "mkdir -p %s", dirout);
    ret = system(charbuf);
#else
    ret = CreateDirectory(dirout, NULL) ? 0 : 1;
#endif  /* !_WIN32 */
    if (ret) {
        L_ERROR("output directory %s not made\n", procName, dirout);
        return 1;
    }

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
        if (factor >= 1.0) {
            pixview = pixClone(pix);   /* no upscaling */
        } else {
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
    l_binaryWrite(mainname, "w", shtml, strlen(shtml));
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
    l_binaryWrite(linkname, "w", slink, strlen(slink));
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
