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
 * converttobmp.c
 *
 *   Converts an image file to bmp if it's not already in that format.
 *
 *   Syntax: converttobmp filein [fileout]
 *
 *   Return: 0 if conversion took place; 1 if no conversion
 */


#include <string.h>
#include "allheaders.h"

main(int    argc,
     char **argv)
{
char        *filein, *fileout;
l_int32      format;
PIX         *pixs;
static char  mainName[] = "converttobmp";

    if (argc != 2 && argc != 3) {
        fprintf(stderr, "Syntax: converttobmp filein [fileout]\n");
        return 1;
    }

    filein = argv[1];
    if (argc == 2)
        fileout = (char *)"/tmp/fileout.bmp";
    else  /* argc == 3 */
        fileout = argv[2];

    pixReadHeader(filein, &format, NULL, NULL, NULL, NULL, NULL);
    if (format == IFF_BMP)
        return 1;  /* no conversion required */

    pixs = pixRead(filein);
    pixWrite(fileout, pixs, IFF_BMP);
    return 0;
}
