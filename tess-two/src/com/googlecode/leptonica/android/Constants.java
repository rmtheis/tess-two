/*
 * Copyright (C) 2010 Google Inc.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the License at
 * 
 * http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 */

package com.googlecode.leptonica.android;

/**
 * Leptonica constants.
 *
 * @author alanv@google.com (Alan Viverette)
 */
public class Constants {
    /*-------------------------------------------------------------------------*
     *                         Access and storage flags                        *
     *-------------------------------------------------------------------------*/
    /*
     * For Pix, Box, Pta and Numa, there are 3 standard methods for handling the
     * retrieval or insertion of a struct: (1) direct insertion (Don't do this
     * if there is another handle somewhere to this same struct!) (2) copy
     * (Always safe, sets up a refcount of 1 on the new object. Can be
     * undesirable if very large, such as an image or an array of images.) (3)
     * clone (Makes another handle to the same struct, and bumps the refcount up
     * by 1. Safe to do unless you're changing data through one of the handles
     * but don't want those changes to be seen by the other handle.) For Pixa
     * and Boxa, which are structs that hold an array of clonable structs, there
     * is an additional method: (4) copy-clone (Makes a new higher-level struct
     * with a refcount of 1, but clones all the structs in the array.) Unlike
     * the other structs, when retrieving a string from an Sarray, you are
     * allowed to get a handle without a copy or clone (i.e., that you don't
     * own!). You must not free or insert such a string! Specifically, for an
     * Sarray, the copyflag for retrieval is either: TRUE (or 1 or L_COPY) or
     * FALSE (or 0 or L_NOCOPY) For insertion, the copyflag is either: TRUE (or
     * 1 or L_COPY) or FALSE (or 0 or L_INSERT) Note that L_COPY is always 1,
     * and L_INSERT and L_NOCOPY are always 0.
     */

    /* Stuff it in; no copy, clone or copy-clone */
    public static final int L_INSERT = 0;

    /* Make/use a copy of the object */
    public static final int L_COPY = 1;

    /* Make/use clone (ref count) of the object */
    public static final int L_CLONE = 2;

    /*
     * Make a new object and fill with with clones of each object in the
     * array(s)
     */
    public static final int L_COPY_CLONE = 3;

    /*--------------------------------------------------------------------------*
     *                              Sort flags                                  *
     *--------------------------------------------------------------------------*/

    /* Sort in increasing order */
    public static final int L_SORT_INCREASING = 1;

    /* Sort in decreasing order */
    public static final int L_SORT_DECREASING = 2;

    /* Sort box or c.c. by horiz location */
    public static final int L_SORT_BY_X = 3;

    /* Sort box or c.c. by vert location */
    public static final int L_SORT_BY_Y = 4;

    /* Sort box or c.c. by width */
    public static final int L_SORT_BY_WIDTH = 5;

    /* Sort box or c.c. by height */
    public static final int L_SORT_BY_HEIGHT = 6;

    /* Sort box or c.c. by min dimension */
    public static final int L_SORT_BY_MIN_DIMENSION = 7;

    /* Sort box or c.c. by max dimension */
    public static final int L_SORT_BY_MAX_DIMENSION = 8;

    /* Sort box or c.c. by perimeter */
    public static final int L_SORT_BY_PERIMETER = 9;

    /* Sort box or c.c. by area */
    public static final int L_SORT_BY_AREA = 10;

    /* Sort box or c.c. by width/height ratio */
    public static final int L_SORT_BY_ASPECT_RATIO = 11;

    /* ------------------ Image file format types -------------- */
    /*
     * The IFF_DEFAULT flag is used to write the file out in the same (input)
     * file format that the pix was read from. If the pix was not read from
     * file, the input format field will be IFF_UNKNOWN and the output file
     * format will be chosen to be compressed and lossless; namely, IFF_TIFF_G4
     * for d = 1 and IFF_PNG for everything else. IFF_JP2 is for jpeg2000, which
     * is not supported in leptonica.
     */

    public static final int IFF_UNKNOWN = 0;

    public static final int IFF_BMP = 1;

    public static final int IFF_JFIF_JPEG = 2;

    public static final int IFF_PNG = 3;

    public static final int IFF_TIFF = 4;

    public static final int IFF_TIFF_PACKBITS = 5;

    public static final int IFF_TIFF_RLE = 6;

    public static final int IFF_TIFF_G3 = 7;

    public static final int IFF_TIFF_G4 = 8;

    public static final int IFF_TIFF_LZW = 9;

    public static final int IFF_TIFF_ZIP = 10;

    public static final int IFF_PNM = 11;

    public static final int IFF_PS = 12;

    public static final int IFF_GIF = 13;

    public static final int IFF_JP2 = 14;

    public static final int IFF_DEFAULT = 15;

    public static final int IFF_SPIX = 16;
}
