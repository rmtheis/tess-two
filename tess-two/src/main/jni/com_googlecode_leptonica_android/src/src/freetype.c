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
 * freetype.c
 *      static l_int32       ftUtfToUniChar()
 *      static PIX          *ftDrawBitmap()
 *             FT_LIBRARY   *ftInitLibrary()
 *             void          ftShutdownLibrary()
 *             PIX          *pixWriteTTFText()
 */

#include <string.h>
#include <math.h>
#include "allheaders.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

#undef MAX
#define MAX(a, b) (((a)>(b))?(a):(b))

#define ROUNDUPDOWN(val, updown) (!updown) ? (val < 0 ? ((val - 63) >> 6) : val >> 6) : (val > 0 ? ((val + 63) >> 6) : val >> 6)

struct ft_library_st {
    FT_Library library;
};

static l_int32
ftUtfToUniChar(char     *str,
               l_int32  *chPtr)
{
    l_int32 byte;

        /* HTML4.0 entities in decimal form, e.g. &#197; {{{ */
    byte = *((unsigned char *) str);
    if (byte == '&') {
        l_int32 i, n = 0;

        byte = *((unsigned char *) (str+1));
        if (byte == '#') {
            for (i = 2; i < 8; i++) {
                byte = *((unsigned char *) (str+i));
                if (byte >= '0' && byte <= '9') {
                    n = (n * 10) + (byte - '0');
                } else {
                    break;
                }
            }
            if (byte == ';') {
                *chPtr = (l_int32) n;
                return ++i;
            }
        }
    }
    /* }}} */

        /* Unroll 1 to 3 byte UTF-8 sequences */

    byte = *((unsigned char *) str);
    if (byte < 0xC0) {
            /* Handles properly formed UTF-8 characters between 0x01 and 0x7F.
             * Also treats \0 and naked trail bytes 0x80 to 0xBF as valid
             * characters representing themselves. */
        *chPtr = (l_int32) byte;
        return 1;
    } else if (byte < 0xE0) {
        if ((str[1] & 0xC0) == 0x80) {
                /* Two-byte-character lead-byte followed by a trail-byte. */
            *chPtr = (l_int32) (((byte & 0x1F) << 6) | (str[1] & 0x3F));
            return 2;
        }
            /* A two-byte-character lead-byte not followed by trail-byte
             * represents itself.  */
        *chPtr = (l_int32) byte;
        return 1;
    } else if (byte < 0xF0) {
        if (((str[1] & 0xC0) == 0x80) && ((str[2] & 0xC0) == 0x80)) {
                /* Three-byte-character lead byte followed by 2 trail bytes. */
            *chPtr = (l_int32) (((byte & 0x0F) << 12) |
                                ((str[1] & 0x3F) << 6) | (str[2] & 0x3F));
            return 3;
        }

            /* A three-byte-character lead-byte not followed by two
             * trail-bytes represents itself. */
        *chPtr = (l_int32) byte;
        return 1;
    }

    *chPtr = (l_int32)byte;
    return 1;
}
/* }}} */


static PIX *
ftDrawBitmap(l_uint32  *datad,
             l_uint32   color,
             FT_Bitmap  bitmap,
             l_int32    pen_x,
             l_int32    pen_y,
             l_int32    width,
             l_int32    height)
{
l_uint32 *ppixel = NULL, pixel;
l_int32 x, y, row, col, pc, pcr, i;
l_uint8 tmp;

    PROCNAME("ftDrawBitmap");

    for (row = 0; row < bitmap.rows; row++) {
        pc = row * bitmap.pitch;
        pcr = pc;
        y = pen_y + row;

            /* Clip if out of bounds */
        if (y >= height || y < 0) {
            continue;
        }

        for (col = 0; col < bitmap.width; col++, pc++) {
            int level;
            if (bitmap.pixel_mode == ft_pixel_mode_grays) {
                level = (bitmap.buffer[pc] * 127/ (bitmap.num_grays - 1));
            } else if (bitmap.pixel_mode == ft_pixel_mode_mono) {
                level = ((bitmap.buffer[(col>>3)+pcr]) & (1<<(~col&0x07))) ?
                    127 : 0;
            } else {
                return (PIX *)ERROR_PTR("unsupported ft_pixel mode",
                                        procName, NULL);
            }

            if (color >= 0) {
                level = level * (127 - GET_DATA_BYTE(&color, L_ALPHA_CHANNEL))
                    / 127;
            }

            level = 127 - level;
            x = pen_x + col;

                /* Clip if out of bounds */
            if (x >= width || x < 0) {
                continue;
            }
            ppixel = datad + y*width + x;

                /* Mix 2 colors using level as alpha */
            if (level != 127) {
                l_uint8 new, old;

                pixel = *ppixel;
                for (i = 0; i < 3; i++) {
                    new = GET_DATA_BYTE(&color, i);
                    old = GET_DATA_BYTE(&pixel, i);
                    tmp = (double)old * ((double)level/127) +
                           (double)new * ((double)(127 - level)/127);
                    SET_DATA_BYTE(ppixel, i, tmp);
                }
           }
        }
    }
    return NULL;
}


FT_LIBRARY *
ftInitLibrary(void)
{
FT_Error err;
FT_LIBRARY *lib_ptr;

    lib_ptr = CALLOC(1, sizeof(FT_LIBRARY));

    err = FT_Init_FreeType(&lib_ptr->library);
    if (err) {
        FREE(lib_ptr);
        return NULL;
    }
    return lib_ptr;
}


void
ftShutdownLibrary(FT_LIBRARY  *lib_ptr)
{
    if (lib_ptr) {
        FT_Done_FreeType(lib_ptr->library);
        FREE(lib_ptr);
    }
}


PIX *
pixWriteTTFText(FT_LIBRARY  *lib_ptr,
                PIX         *pixs,
                l_float32    size,
                l_float32    angle,
                l_int32      x,
                l_int32      y,
                l_int32      letter_space,
                l_uint32     color,
                l_uint8     *fontfile,
                l_uint8     *text,
                l_int32      text_len,
                l_int32     *brect)
{
PIX *pixd, *pixt = NULL;
FT_Error err;
FT_Face face;
FT_Glyph image;
FT_BitmapGlyph bitmap;
FT_CharMap charmap;
FT_Matrix matrix;
FT_Vector pen, penf;
FT_UInt glyph_index, previous;
FT_BBox char_bbox, bbox;
l_uint32 *datad, letter_space_x, letter_space_y;
l_int32 i, found, len, ch, x1 = 0, y1 = 0, width, height;
l_uint16 platform, encoding;
char *next;
l_float32 cos_a, sin_a;

    PROCNAME("pixWriteTTFText");

    if (pixGetDepth(pixs) != 32) {
        pixt = pixConvertTo32(pixs);
        if (!pixt) {
            return (PIX *)ERROR_PTR("failed to convert pixs to 32bpp image",
                                    procName, NULL);
        }
        pixd = pixCopy(NULL, pixt);
    } else {
        pixd = pixCopy(NULL, pixs);
    }
    if (!pixd) {
        pixDestroy(&pixt);
        return (PIX *)ERROR_PTR("pixd not made", procName, NULL);
    }

    datad = pixGetData(pixd);
    pixGetDimensions(pixd, &width, &height, NULL);
    err = FT_New_Face (lib_ptr->library, (char *)fontfile, 0, &face);
    if (err) {
        pixDestroy(&pixt);
        pixDestroy(&pixd);
        return (PIX *)ERROR_PTR("failed to load font file", procName, NULL);
    }

    err = FT_Set_Char_Size(face, 0, (FT_F26Dot6) (size * 64),
                           LEPTONICA_FT_RESOLUTION, LEPTONICA_FT_RESOLUTION);
    if (err) {
        pixDestroy(&pixt);
        pixDestroy(&pixd);
        FT_Done_Face(face);
        return (PIX *)ERROR_PTR("failed to set font size", procName, NULL);
    }

    found = 0;
    for (i = 0; i < face->num_charmaps; i++) {
        charmap = face->charmaps[i];
        platform = charmap->platform_id;
        encoding = charmap->encoding_id;
        if ((platform == 3 && encoding == 1)    /* Windows Unicode */
            || (platform == 3 && encoding == 0) /* Windows Symbol */
            || (platform == 2 && encoding == 1) /* ISO Unicode */
            || (platform == 0)) {               /* Apple Unicode */
            found = 1;
            break;
        }
    }

    if (!found) {
        pixDestroy(&pixt);
        pixDestroy(&pixd);
        FT_Done_Face(face);
        return (PIX *)ERROR_PTR("could not find Unicode charmap",
                                procName, NULL);
    }

        /* Degrees to radians */
    angle = angle * (M_PI/180);
    sin_a = sin(angle);
    cos_a = cos(angle);
    matrix.xx = (FT_Fixed) (cos_a * (1 << 16));
    matrix.yx = (FT_Fixed) (sin_a * (1 << 16));
    matrix.xy = -matrix.yx;
    matrix.yy = matrix.xx;
    FT_Set_Transform(face, &matrix, NULL);
    penf.x = penf.y = 0;    /* running position of non-rotated string */
    pen.x = pen.y = 0;      /* running position of rotated string */

    previous = 0;
    next = (char *)text;
    i = 0;
    while (*next) {
        if (i == 0) { /* use char spacing for 1+ characters */
            letter_space_x = 0;
            letter_space_y = 0;
        } else {
            letter_space_x = cos_a * letter_space * i;
            letter_space_y = -sin_a * letter_space * i;
        }

        len = ftUtfToUniChar(next, &ch);
//        ch |= 0xf000;
        next += len;

        glyph_index = FT_Get_Char_Index(face, ch);

        err = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);
        if (err) {
            pixDestroy(&pixt);
            pixDestroy(&pixd);
            FT_Done_Face(face);
            return (PIX *)ERROR_PTR("could not load glyph into the slot",
                                    procName, NULL);
        }

        err = FT_Get_Glyph(face->glyph, &image);
        if (err) {
            pixDestroy(&pixt);
            pixDestroy(&pixd);
            FT_Done_Face(face);
            return (PIX *)ERROR_PTR("could not extract glyph from a slot",
                                    procName, NULL);
        }

        if (brect) {
            FT_Glyph_Get_CBox(image, ft_glyph_bbox_gridfit, &char_bbox);
            char_bbox.xMin += penf.x;
            char_bbox.yMin += penf.y;
            char_bbox.xMax += penf.x;
            char_bbox.yMax += penf.y;

            if (i == 0) {
                bbox.xMin = char_bbox.xMin;
                bbox.yMin = char_bbox.yMin;
                bbox.xMax = char_bbox.xMax;
                bbox.yMax = char_bbox.yMax;
            } else {
                if (bbox.xMin > char_bbox.xMin) {
                    bbox.xMin = char_bbox.xMin;
                }
                if (bbox.yMin > char_bbox.yMin) {
                    bbox.yMin = char_bbox.yMin;
                }
                if (bbox.xMax < char_bbox.xMax) {
                     bbox.xMax = char_bbox.xMax;
                }
                if (bbox.yMax < char_bbox.yMax) {
                     bbox.yMax = char_bbox.yMax;
                }
            }
        }

        if (image->format != ft_glyph_format_bitmap &&
            FT_Glyph_To_Bitmap(&image, ft_render_mode_normal, 0, 1)) {
            pixDestroy(&pixt);
            pixDestroy(&pixd);
            FT_Done_Face(face);
            return (PIX *)ERROR_PTR("could not convert glyph to bitmap",
                                    procName, NULL);
        }

            /* Now, draw to our target surface */
        bitmap = (FT_BitmapGlyph) image;
        ftDrawBitmap(datad, color, bitmap->bitmap,
                  letter_space_x + x + x1 + ((pen.x + 31) >> 6) + bitmap->left,
                  letter_space_y + y - y1 + ((pen.y + 31) >> 6) - bitmap->top,
                  width, height);

            /* Record current glyph index for kerning */
        previous = glyph_index;

            /* Increment pen position */
        pen.x += image->advance.x >> 10;
        pen.y -= image->advance.y >> 10;
        penf.x += face->glyph->metrics.horiAdvance;

        FT_Done_Glyph(image);
        i++;
    }

    if (brect) {
        double d1 = sin (angle + 0.78539816339744830962);
        double d2 = sin (angle - 0.78539816339744830962);

            /* Rotate bounding rectangle */
        brect[0] = (int) (bbox.xMin * cos_a - bbox.yMin * sin_a);
        brect[1] = (int) (bbox.xMin * sin_a + bbox.yMin * cos_a);
        brect[2] = (int) (bbox.xMax * cos_a - bbox.yMin * sin_a);
        brect[3] = (int) (bbox.xMax * sin_a + bbox.yMin * cos_a);
        brect[4] = (int) (bbox.xMax * cos_a - bbox.yMax * sin_a);
        brect[5] = (int) (bbox.xMax * sin_a + bbox.yMax * cos_a);
        brect[6] = (int) (bbox.xMin * cos_a - bbox.yMax * sin_a);
        brect[7] = (int) (bbox.xMin * sin_a + bbox.yMax * cos_a);

            /* Scale, round and offset brect */
        brect[0] = x + ROUNDUPDOWN(brect[0], d2 > 0);
        brect[1] = y - ROUNDUPDOWN(brect[1], d1 < 0);
        brect[2] = x + ROUNDUPDOWN(brect[2], d1 > 0);
        brect[3] = y - ROUNDUPDOWN(brect[3], d2 > 0);
        brect[4] = x + ROUNDUPDOWN(brect[4], d2 < 0);
        brect[5] = y - ROUNDUPDOWN(brect[5], d1 > 0);
        brect[6] = x + ROUNDUPDOWN(brect[6], d1 < 0);
        brect[7] = y - ROUNDUPDOWN(brect[7], d2 < 0);
    }

    pixDestroy(&pixt);
    return pixd;
}
