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
 *   bootnumgen.c
 *
 *   Function for generating prog/recog/digits/bootnum1.pa from an
 *   encoded, gzipped and serialized string.
 *
 *   This was generated using the stringcode utility, slightly edited,
 *   and then merged into a single file.
 *
 *   The code and encoded strings were made using the stringcode utility:
 *
 *       L_STRCODE  *strc;
 *       strc = strcodeCreate(103);   // arbitrary integer
 *       strcodeGenerate(strc, "recog/digits/bootnum1.pa", "PIXA");
 *       strcodeFinalize(&strc, ".");
 *
 *   The two output files, autogen.103.c and autogen.103.h, were
 *   then slightly edited and merged into this file.
 *
 *   Call this way:
 *       Pixa  *pixa = (PIXA *)l_bootnum_gen();
 */

#include <string.h>
#include "allheaders.h"

/*---------------------------------------------------------------------*/
/*                         Serialized string                           */
/*---------------------------------------------------------------------*/
static const char *l_bootnum =
    "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"
    "kAFKXhg435zw+t8w/wcqdPzO";


/*---------------------------------------------------------------------*/
/*                      Auto-generated deserializer                    */
/*---------------------------------------------------------------------*/
/*!
 *  l_bootnum_gen()
 *
 *      Return: the bootnum pixa
 *
 *  Call this way:
 *      PIXA  *pixa = (PIXA *)l_bootnum_gen();   (C)
 *      Pixa  *pixa = (Pixa *)l_bootnum_gen();   (C++)
 */
void *
l_bootnum_gen()
{
l_uint8  *data1, *data2;
l_int32   size1;
size_t    size2;
void     *result;

        /* Unencode selected string, write to file, and read it */
    data1 = decodeBase64(l_bootnum, strlen(l_bootnum), &size1);
    data2 = zlibUncompress(data1, size1, &size2);
    l_binaryWrite("/tmp/data.bin", "w", data2, size2);
    result = (void *)pixaRead("/tmp/data.bin");
    FREE(data1);
    FREE(data2);
    return result;
}


