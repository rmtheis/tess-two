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
 *     Low-level fast hit-miss transform with auto-generated sels
 *
 *      Dispatcher:
 *             l_int32    fhmtgen_low_1()
 *
 *      Static Low-level:
 *             void       fhmt_1_*()
 */

#include <stdio.h>
#include "allheaders.h"

static void  fhmt_1_0(l_uint32 *, l_int32, l_int32, l_int32, l_uint32 *, l_int32);
static void  fhmt_1_1(l_uint32 *, l_int32, l_int32, l_int32, l_uint32 *, l_int32);
static void  fhmt_1_2(l_uint32 *, l_int32, l_int32, l_int32, l_uint32 *, l_int32);
static void  fhmt_1_3(l_uint32 *, l_int32, l_int32, l_int32, l_uint32 *, l_int32);
static void  fhmt_1_4(l_uint32 *, l_int32, l_int32, l_int32, l_uint32 *, l_int32);
static void  fhmt_1_5(l_uint32 *, l_int32, l_int32, l_int32, l_uint32 *, l_int32);


/*---------------------------------------------------------------------*
 *                           Fast hmt dispatcher                       *
 *---------------------------------------------------------------------*/
/*
 *  fhmtgen_low_1()
 *
 *       a dispatcher to appropriate low-level code
 */
l_int32
fhmtgen_low_1(l_uint32  *datad,
              l_int32    w,
              l_int32    h,
              l_int32    wpld,
              l_uint32  *datas,
              l_int32    wpls,
              l_int32    index)
{

    switch (index)
    {
    case 0:
        fhmt_1_0(datad, w, h, wpld, datas, wpls);
        break;
    case 1:
        fhmt_1_1(datad, w, h, wpld, datas, wpls);
        break;
    case 2:
        fhmt_1_2(datad, w, h, wpld, datas, wpls);
        break;
    case 3:
        fhmt_1_3(datad, w, h, wpld, datas, wpls);
        break;
    case 4:
        fhmt_1_4(datad, w, h, wpld, datas, wpls);
        break;
    case 5:
        fhmt_1_5(datad, w, h, wpld, datas, wpls);
        break;
    }

    return 0;
}


/*--------------------------------------------------------------------------*
 *                 Low-level auto-generated static routines                 *
 *--------------------------------------------------------------------------*/
/*
 *  N.B.  In all the low-level routines, the part of the image
 *        that is accessed has been clipped by 32 pixels on
 *        all four sides.  This is done in the higher level
 *        code by redefining w and h smaller and by moving the
 *        start-of-image pointers up to the beginning of this
 *        interior rectangle.
 */
static void
fhmt_1_0(l_uint32  *datad,
         l_int32    w,
         l_int32    h,
         l_int32    wpld,
         l_uint32  *datas,
         l_int32    wpls)
{
l_int32             i;
register l_int32    j, pwpls;
register l_uint32  *sptr, *dptr;
    
    pwpls = (l_uint32)(w + 31) / 32;  /* proper wpl of src */

    for (i = 0; i < h; i++) {
        sptr = datas + i * wpls;
        dptr = datad + i * wpld;
        for (j = 0; j < pwpls; j++, sptr++, dptr++) {
            *dptr = ((~*(sptr - wpls) >> 1) | (~*(sptr - wpls - 1) << 31)) &
                    (~*(sptr - wpls)) &
                    ((~*(sptr - wpls) << 1) | (~*(sptr - wpls + 1) >> 31)) &
                    ((~*(sptr) >> 1) | (~*(sptr - 1) << 31)) &
                    (*sptr) &
                    ((~*(sptr) << 1) | (~*(sptr + 1) >> 31)) &
                    ((~*(sptr + wpls) >> 1) | (~*(sptr + wpls - 1) << 31)) &
                    (~*(sptr + wpls)) &
                    ((~*(sptr + wpls) << 1) | (~*(sptr + wpls + 1) >> 31));
        }
    }
}

static void
fhmt_1_1(l_uint32  *datad,
         l_int32    w,
         l_int32    h,
         l_int32    wpld,
         l_uint32  *datas,
         l_int32    wpls)
{
l_int32             i;
register l_int32    j, pwpls;
register l_uint32  *sptr, *dptr;
    
    pwpls = (l_uint32)(w + 31) / 32;  /* proper wpl of src */

    for (i = 0; i < h; i++) {
        sptr = datas + i * wpls;
        dptr = datad + i * wpld;
        for (j = 0; j < pwpls; j++, sptr++, dptr++) {
            *dptr = ((*(sptr) >> 1) | (*(sptr - 1) << 31)) &
                    (*sptr) &
                    ((*(sptr) << 1) | (*(sptr + 1) >> 31)) &
                    ((~*(sptr + wpls) >> 1) | (~*(sptr + wpls - 1) << 31)) &
                    (~*(sptr + wpls)) &
                    ((~*(sptr + wpls) << 1) | (~*(sptr + wpls + 1) >> 31));
        }
    }
}

static void
fhmt_1_2(l_uint32  *datad,
         l_int32    w,
         l_int32    h,
         l_int32    wpld,
         l_uint32  *datas,
         l_int32    wpls)
{
l_int32             i;
register l_int32    j, pwpls;
register l_uint32  *sptr, *dptr;
    
    pwpls = (l_uint32)(w + 31) / 32;  /* proper wpl of src */

    for (i = 0; i < h; i++) {
        sptr = datas + i * wpls;
        dptr = datad + i * wpld;
        for (j = 0; j < pwpls; j++, sptr++, dptr++) {
            *dptr = ((~*(sptr - wpls) >> 1) | (~*(sptr - wpls - 1) << 31)) &
                    (~*(sptr - wpls)) &
                    ((~*(sptr - wpls) << 1) | (~*(sptr - wpls + 1) >> 31)) &
                    ((*(sptr) >> 1) | (*(sptr - 1) << 31)) &
                    (*sptr) &
                    ((*(sptr) << 1) | (*(sptr + 1) >> 31));
        }
    }
}

static void
fhmt_1_3(l_uint32  *datad,
         l_int32    w,
         l_int32    h,
         l_int32    wpld,
         l_uint32  *datas,
         l_int32    wpls)
{
l_int32             i;
register l_int32    j, pwpls;
register l_uint32  *sptr, *dptr;
    
    pwpls = (l_uint32)(w + 31) / 32;  /* proper wpl of src */

    for (i = 0; i < h; i++) {
        sptr = datas + i * wpls;
        dptr = datad + i * wpld;
        for (j = 0; j < pwpls; j++, sptr++, dptr++) {
            *dptr = (*(sptr - wpls)) &
                    ((~*(sptr - wpls) << 1) | (~*(sptr - wpls + 1) >> 31)) &
                    (*sptr) &
                    ((~*(sptr) << 1) | (~*(sptr + 1) >> 31)) &
                    (*(sptr + wpls)) &
                    ((~*(sptr + wpls) << 1) | (~*(sptr + wpls + 1) >> 31));
        }
    }
}

static void
fhmt_1_4(l_uint32  *datad,
         l_int32    w,
         l_int32    h,
         l_int32    wpld,
         l_uint32  *datas,
         l_int32    wpls)
{
l_int32             i;
register l_int32    j, pwpls;
register l_uint32  *sptr, *dptr;
    
    pwpls = (l_uint32)(w + 31) / 32;  /* proper wpl of src */

    for (i = 0; i < h; i++) {
        sptr = datas + i * wpls;
        dptr = datad + i * wpld;
        for (j = 0; j < pwpls; j++, sptr++, dptr++) {
            *dptr = ((~*(sptr - wpls) >> 1) | (~*(sptr - wpls - 1) << 31)) &
                    (*(sptr - wpls)) &
                    ((~*(sptr) >> 1) | (~*(sptr - 1) << 31)) &
                    (*sptr) &
                    ((~*(sptr + wpls) >> 1) | (~*(sptr + wpls - 1) << 31)) &
                    (*(sptr + wpls));
        }
    }
}

static void
fhmt_1_5(l_uint32  *datad,
         l_int32    w,
         l_int32    h,
         l_int32    wpld,
         l_uint32  *datas,
         l_int32    wpls)
{
l_int32             i;
register l_int32    j, pwpls;
register l_uint32  *sptr, *dptr;
l_int32             wpls2, wpls3, wpls4;
l_int32             wpls5, wpls6;
    
    wpls2 = 2 * wpls;
    wpls3 = 3 * wpls;
    wpls4 = 4 * wpls;
    wpls5 = 5 * wpls;
    wpls6 = 6 * wpls;
    pwpls = (l_uint32)(w + 31) / 32;  /* proper wpl of src */

    for (i = 0; i < h; i++) {
        sptr = datas + i * wpls;
        dptr = datad + i * wpld;
        for (j = 0; j < pwpls; j++, sptr++, dptr++) {
            *dptr = ((~*(sptr - wpls6) << 1) | (~*(sptr - wpls6 + 1) >> 31)) &
                    ((*(sptr - wpls6) << 3) | (*(sptr - wpls6 + 1) >> 29)) &
                    (~*(sptr - wpls2)) &
                    ((*(sptr - wpls2) << 2) | (*(sptr - wpls2 + 1) >> 30)) &
                    ((~*(sptr + wpls2) >> 1) | (~*(sptr + wpls2 - 1) << 31)) &
                    ((*(sptr + wpls2) << 1) | (*(sptr + wpls2 + 1) >> 31)) &
                    ((~*(sptr + wpls6) >> 2) | (~*(sptr + wpls6 - 1) << 30)) &
                    (*(sptr + wpls6));
        }
    }
}

