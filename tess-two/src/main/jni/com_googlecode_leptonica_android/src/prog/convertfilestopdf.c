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
 * convertfilestopdf.c
 *
 *    Converts all image files in the given directory with matching substring
 *    to a pdf, with the specified scaling factor <= 1.0 applied to all
 *    images.
 *
 *    See below for syntax and usage.
 *
 *    The images are displayed at a resolution that depends on the
 *    input resolution (res) and the scaling factor (scalefact) that
 *    is applied to the images before conversion to pdf.  Internally
 *    we multiply these, so that the generated pdf will render at the
 *    same resolution as if it hadn't been scaled.  By downscaling, you
 *    reduce the size of the images.  For jpeg, downscaling reduces
 *    pdf size by the square of the scale factor.  The jpeg quality can
 *    be specified from 1 (very poor) to 100 (best available, but
 *    still lossy); use 0 for the default (75).
 */

#include <string.h>
#include "allheaders.h"

int main(int    argc,
         char **argv)
{
char        *dirin, *substr, *title, *fileout;
l_int32      ret, res, type, quality;
l_float32    scalefactor;
static char  mainName[] = "convertfilestopdf";

    if (argc != 9) {
        fprintf(stderr,
            " Syntax: convertfilestopdf dirin substr res"
            " scalefactor encoding_type title fileout\n"
            "         dirin:  input directory for image files\n"
            "         substr:  Use 'allfiles' to convert all files\n"
            "                  in the directory.\n"
            "         res:  Input resolution of each image;\n"
            "               assumed to all be the same\n"
            "         scalefactor:  Use to scale all images\n"
            "         encoding_type:\n"
            "              L_JPEG_ENCODE = 1\n"
            "              L_G4_ENCODE = 2\n"
            "              L_FLATE_ENCODE = 3, or 0 for per-page default)\n"
            "         quality:  used for jpeg; 0 for default (75);\n"
            "                   otherwise select from 1 to 100\n"
            "         title:  Use 'none' to omit\n"
            "         fileout:  Output pdf file\n");
        return 1;
    }

    dirin = argv[1];
    substr = argv[2];
    res = atoi(argv[3]);
    scalefactor = atof(argv[4]);
    type = atoi(argv[5]);
    quality = atoi(argv[6]);
    title = argv[7];
    fileout = argv[8];
    if (!strcmp(substr, "allfiles"))
        substr = NULL;
    if (scalefactor <= 0.0 || scalefactor > 1.0) {
        L_WARNING("invalid scalefactor: setting to 1.0\n", mainName);
        scalefactor = 1.0;
    }
    if (!strcmp(title, "none"))
        title = NULL;

    ret = convertFilesToPdf(dirin, substr, res, scalefactor, type,
                            quality, title, fileout);
    return ret;
}
