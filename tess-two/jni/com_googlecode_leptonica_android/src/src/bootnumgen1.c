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

/*!
 * \file  bootnumgen1.c
 * <pre>
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
 *       strc = strcodeCreate(101);   // arbitrary integer
 *       strcodeGenerate(strc, "recog/digits/bootnum1.pa", "PIXA");
 *       strcodeFinalize(\&strc, ".");
 *
 *   The two output files, autogen.101.c and autogen.101.h, were
 *   then slightly edited and merged into this file.
 *
 *   Call this way:
 *       PIXA  *pixa = l_bootnum_gen1();   (C)
 *       Pixa  *pixa = l_bootnum_gen1();   (C++)
 * </pre>
 */

#include <string.h>
#include "allheaders.h"

/*---------------------------------------------------------------------*/
/*                         Serialized string                           */
/*---------------------------------------------------------------------*/
static const char *l_bootnum1 =
    "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"
    "U7aI7hwmZkQ4a/+vm1L/A0W3YQM=";


/*---------------------------------------------------------------------*/
/*                      Auto-generated deserializer                    */
/*---------------------------------------------------------------------*/
/*!
 * \brief   l_bootnum_gen1()
 *
 * \return   pixa  of labelled digits
 *
 * <pre>
 * Call this way:
 *      PIXA  *pixa = l_bootnum_gen1();   (C)
 *      Pixa  *pixa = l_bootnum_gen1();   (C++)
 * </pre>
 */
PIXA *
l_bootnum_gen1(void)
{
l_uint8  *data1, *data2;
l_int32   size1;
size_t    size2;
PIXA     *pixa;

        /* Unencode selected string, write to file, and read it */
    data1 = decodeBase64(l_bootnum1, strlen(l_bootnum1), &size1);
    data2 = zlibUncompress(data1, size1, &size2);
    pixa = pixaReadMem(data2, size2);
    lept_free(data1);
    lept_free(data2);
    return pixa;
}
