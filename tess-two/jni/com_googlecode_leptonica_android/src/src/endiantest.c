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
 *    endiantest.c
 *
 *    This test was contributed by Bill Janssen.  When used with the
 *    gnu compiler, it allows efficient computation of the endian
 *    flag as part of the normal compilation process.  As a result,
 *    it is not necessary to set this flag either manually or
 *    through the configure Makefile generator.
 */

#include <stdio.h>

int main()
{
/* fprintf(stderr, "doing the test\n"); */
    long v = 0x04030201;
    if (*((unsigned char *)(&v)) == 0x04)
        printf("L_BIG_ENDIAN\n");
    else
        printf("L_LITTLE_ENDIAN\n");
    return 0;
}


