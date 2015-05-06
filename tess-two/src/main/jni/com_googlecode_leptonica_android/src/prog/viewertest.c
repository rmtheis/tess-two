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
 * viewertest.c
 *
 *    Input:  dirin:  directory of input image files
 *            dirout: directory for output files
 *            rootname: root name for output files
 *            thumbwidth: width of thumb images, in pixels; use 0 for default
 *            viewwidth: max width of view images, in pixels; use 0 for default
 *
 *    Note: This program is Unix only; it will not link to pixHtmlViewer()
 *          under cygwin.
 */

#include "allheaders.h"

int main(int    argc,
         char **argv)
{
char        *dirin, *dirout, *rootname;
l_int32      thumbwidth, viewwidth;
static char  mainName[] = "viewertest";

    if (argc != 6)
        return ERROR_INT(
            " Syntax:  viewertest dirin dirout rootname thumbwidth viewwidth",
             mainName, 1);

    dirin = argv[1];
    dirout = argv[2];
    rootname = argv[3];
    thumbwidth = atoi(argv[4]);
    viewwidth = atoi(argv[5]);
    pixHtmlViewer(dirin, dirout, rootname, thumbwidth, viewwidth, 0);
    return 0;
}

