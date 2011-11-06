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
 *  arrayaccess.c
 *
 *     Access within an array of 32-bit words
 *
 *           l_int32     l_getDataBit()
 *           void        l_setDataBit()
 *           void        l_clearDataBit()
 *           void        l_setDataBitVal()
 *           l_int32     l_getDataDibit()
 *           void        l_setDataDibit()
 *           void        l_clearDataDibit()
 *           l_int32     l_getDataQbit()
 *           void        l_setDataQbit()
 *           void        l_clearDataQbit()
 *           l_int32     l_getDataByte()
 *           void        l_setDataByte()
 *           l_int32     l_getDataTwoBytes()
 *           void        l_setDataTwoBytes()
 *           l_int32     l_getDataFourBytes()
 *           void        l_setDataFourBytes()
 *
 *     Note that these all require 32-bit alignment, and hence an input
 *     ptr to l_uint32.  However, this is not enforced by the compiler.
 *     Instead, we allow the use of a void* ptr, because the line ptrs
 *     are an efficient way to get random access (see pixGetLinePtrs()).
 *     It is then necessary to cast internally within each function
 *     because ptr arithmetic requires knowing the size of the units
 *     being referenced.
 */

#include <stdio.h>
#include "allheaders.h"


/*----------------------------------------------------------------------*
 *                 Access within an array of 32-bit words               *
 *----------------------------------------------------------------------*/
/*!
 *  l_getDataBit()
 *
 *      Input:  line  (ptr to beginning of data line)
 *              n     (pixel index)
 *      Return: val of the nth (1-bit) pixel.
 */
l_int32
l_getDataBit(void    *line,
             l_int32  n)
{
    return (*((l_uint32 *)line + (n >> 5)) >> (31 - (n & 31))) & 1;
}


/*!
 *  l_setDataBit()
 *
 *      Input:  line  (ptr to beginning of data line)
 *              n     (pixel index)
 *      Return: void
 *
 *  Action: sets the pixel to 1
 */
void
l_setDataBit(void    *line,
             l_int32  n)
{
    *((l_uint32 *)line + (n >> 5)) |= (0x80000000 >> (n & 31));
}


/*!
 *  l_clearDataBit()
 *
 *      Input:  line  (ptr to beginning of data line)
 *              n     (pixel index)
 *      Return: void
 *
 *  Action: sets the (1-bit) pixel to 0
 */
void
l_clearDataBit(void    *line,
               l_int32  n)
{
    *((l_uint32 *)line + (n >> 5)) &= ~(0x80000000 >> (n & 31));
}


/*!
 *  l_setDataBitVal()
 *
 *      Input:  line  (ptr to beginning of data line)
 *              n     (pixel index)
 *              val   (val to be inserted: 0 or 1)
 *      Return: void
 *
 *  Notes:
 *      (1) This is an accessor for a 1 bpp pix.
 *      (2) It is actually a little slower than using:
 *            if (val == 0)
 *                l_ClearDataBit(line, n);
 *            else
 *                l_SetDataBit(line, n);
 */
void
l_setDataBitVal(void    *line,
                l_int32  n,
                l_int32  val)
{
l_uint32    *pword;

    pword = (l_uint32 *)line + (n >> 5);
    *pword &= ~(0x80000000 >> (n & 31));  /* clear */
    *pword |= val << (31 - (n & 31));   /* set */
    return;
}


/*!
 *  l_getDataDibit()
 *
 *      Input:  line  (ptr to beginning of data line)
 *              n     (pixel index)
 *      Return: val of the nth (2-bit) pixel.
 */
l_int32
l_getDataDibit(void    *line,
               l_int32  n)
{
    return (*((l_uint32 *)line + (n >> 4)) >> (2 * (15 - (n & 15)))) & 3;
}


/*!
 *  l_setDataDibit()
 *
 *      Input:  line  (ptr to beginning of data line)
 *              n     (pixel index)
 *              val   (val to be inserted: 0 - 3)
 *      Return: void
 */
void
l_setDataDibit(void    *line,
               l_int32  n,
               l_int32  val)
{
l_uint32    *pword;

    pword = (l_uint32 *)line + (n >> 4);
    *pword &= ~(0xc0000000 >> (2 * (n & 15)));  /* clear */
    *pword |= (val & 3) << (30 - 2 * (n & 15));   /* set */
    return;
}


/*!
 *  l_clearDataDibit()
 *
 *      Input:  line  (ptr to beginning of data line)
 *              n     (pixel index)
 *      Return: void
 *
 *  Action: sets the (2-bit) pixel to 0
 */
void
l_clearDataDibit(void    *line,
                 l_int32  n)
{
    *((l_uint32 *)line + (n >> 4)) &= ~(0xc0000000 >> (2 * (n & 15)));
}


/*!
 *  l_getDataQbit()
 *
 *      Input:  line  (ptr to beginning of data line)
 *              n     (pixel index)
 *      Return: val of the nth (4-bit) pixel.
 */
l_int32
l_getDataQbit(void    *line,
              l_int32  n)
{
    return (*((l_uint32 *)line + (n >> 3)) >> (4 * (7 - (n & 7)))) & 0xf;
}


/*!
 *  l_setDataQbit()
 *
 *      Input:  line  (ptr to beginning of data line)
 *              n     (pixel index)
 *              val   (val to be inserted: 0 - 0xf)
 *      Return: void
 */
void
l_setDataQbit(void    *line,
              l_int32  n,
              l_int32  val)
{
l_uint32    *pword;

    pword = (l_uint32 *)line + (n >> 3);
    *pword &= ~(0xf0000000 >> (4 * (n & 7)));  /* clear */
    *pword |= (val & 15) << (28 - 4 * (n & 7));   /* set */
    return;
}


/*!
 *  l_clearDataQbit()
 *
 *      Input:  line  (ptr to beginning of data line)
 *              n     (pixel index)
 *      Return: void
 *
 *  Action: sets the (4-bit) pixel to 0
 */
void
l_clearDataQbit(void    *line,
                l_int32  n)
{
    *((l_uint32 *)line + (n >> 3)) &= ~(0xf0000000 >> (4 * (n & 7)));
}


/*!
 *  l_getDataByte()
 *
 *      Input:  line  (ptr to beginning of data line)
 *              n     (pixel index)
 *      Return: value of the n-th (byte) pixel
 */
l_int32
l_getDataByte(void    *line,
              l_int32  n)
{
#ifdef  L_BIG_ENDIAN
    return *((l_uint8 *)line + n);
#else  /* L_LITTLE_ENDIAN */
    return *(l_uint8 *)((l_uintptr_t)((l_uint8 *)line + n) ^ 3);
#endif  /* L_BIG_ENDIAN */
}


/*!
 *  l_setDataByte()
 *
 *      Input:  line  (ptr to beginning of data line)
 *              n     (pixel index)
 *              val   (val to be inserted: 0 - 0xff)
 *      Return: void
 */
void
l_setDataByte(void    *line,
              l_int32  n,
              l_int32  val)
{
#ifdef  L_BIG_ENDIAN
    *((l_uint8 *)line + n) = val;
#else  /* L_LITTLE_ENDIAN */
    *(l_uint8 *)((l_uintptr_t)((l_uint8 *)line + n) ^ 3) = val;
#endif  /* L_BIG_ENDIAN */
}


/*!
 *  l_getDataTwoBytes()
 *
 *      Input:  line  (ptr to beginning of data line)
 *              n     (pixel index)
 *      Return: value of the n-th (2-byte) pixel
 */
l_int32
l_getDataTwoBytes(void    *line,
                  l_int32  n)
{
#ifdef  L_BIG_ENDIAN
    return *((l_uint16 *)line + n);
#else  /* L_LITTLE_ENDIAN */
    return *(l_uint16 *)((l_uintptr_t)((l_uint16 *)line + n) ^ 2);
#endif  /* L_BIG_ENDIAN */
}


/*!
 *  l_setDataTwoBytes()
 *
 *      Input:  line  (ptr to beginning of data line)
 *              n     (pixel index)
 *              val   (val to be inserted: 0 - 0xffff)
 *      Return: void
 */
void
l_setDataTwoBytes(void    *line,
                  l_int32  n,
                  l_int32  val)
{
#ifdef  L_BIG_ENDIAN
    *((l_uint16 *)line + n) = val;
#else  /* L_LITTLE_ENDIAN */
    *(l_uint16 *)((l_uintptr_t)((l_uint16 *)line + n) ^ 2) = val;
#endif  /* L_BIG_ENDIAN */
}


/*!
 *  l_getDataFourBytes()
 *
 *      Input:  line  (ptr to beginning of data line)
 *              n     (pixel index)
 *      Return: value of the n-th (4-byte) pixel
 */
l_int32
l_getDataFourBytes(void    *line,
                   l_int32  n)
{
    return *((l_uint32 *)line + n);
}


/*!
 *  l_setDataFourBytes()
 *
 *      Input:  line  (ptr to beginning of data line)
 *              n     (pixel index)
 *              val   (val to be inserted: 0 - 0xffffffff)
 *      Return: void
 */
void
l_setDataFourBytes(void    *line,
                   l_int32  n,
                   l_int32  val)
{
    *((l_uint32 *)line + n) = val;
}



