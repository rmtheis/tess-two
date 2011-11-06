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
 *  spixio.c
 *
 *    This does fast serialization of a pix in memory to file,
 *    copying the raw data for maximum speed.  The underlying
 *    function serializes it to memory, and it is wrapped to be
 *    callable from standard pixRead and pixWrite functions.
 *
 *      Reading spix from file
 *           PIX        *pixReadStreamSpix()
 *           l_int32     readHeaderSpix()
 *           l_int32     freadHeaderSpix()
 *           l_int32     sreadHeaderSpix()
 *
 *      Writing spix to file
 *           l_int32     pixWriteStreamSpix()
 *
 *      Low-level serialization of pix to/from memory (uncompressed)
 *           PIX        *pixReadMemSpix()
 *           l_int32     pixWriteMemSpix()
 *           l_int32     pixSerializeToMemory()
 *           PIX        *pixDeserializeFromMemory()
 *
 */

#include <string.h>
#include "allheaders.h"


/*-----------------------------------------------------------------------*
 *                          Reading spix from file                       *
 *-----------------------------------------------------------------------*/
/*!
 *  pixReadStreamSpix()
 *
 *      Input:  stream
 *      Return: pix, or null on error.
 *
 *  Notes:
 *      (1) If called from pixReadStream(), the stream is positioned
 *          at the beginning of the file.
 */
PIX *
pixReadStreamSpix(FILE  *fp)
{
size_t    nbytes;
l_uint8  *data;
PIX      *pix;

    PROCNAME("pixReadStreamSpix");

    if (!fp)
        return (PIX *)ERROR_PTR("stream not defined", procName, NULL);

    if ((data = l_binaryReadStream(fp, &nbytes)) == NULL)
        return (PIX *)ERROR_PTR("data not read", procName, NULL);
    if ((pix = pixReadMemSpix(data, nbytes)) == NULL) {
        FREE(data);
        return (PIX *)ERROR_PTR("pix not made", procName, NULL);
    }

    FREE(data);
    return pix;
}


/*!
 *  readHeaderSpix()
 *
 *      Input:  filename
 *              &width (<return>)
 *              &height (<return>)
 *              &bps (<return>, bits/sample)
 *              &spp (<return>, samples/pixel)
 *              &iscmap (<optional return>; input NULL to ignore)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) If there is a colormap, iscmap is returned as 1; else 0.
 */
l_int32
readHeaderSpix(const char *filename,
               l_int32    *pwidth,
               l_int32    *pheight,
               l_int32    *pbps,
               l_int32    *pspp,
               l_int32    *piscmap)
{
l_int32  ret;
FILE    *fp;

    PROCNAME("readHeaderSpix");

    if (!filename)
        return ERROR_INT("filename not defined", procName, 1);
    if (!pwidth || !pheight || !pbps || !pspp)
        return ERROR_INT("input ptr(s) not defined", procName, 1);
    if ((fp = fopenReadStream(filename)) == NULL)
        return ERROR_INT("image file not found", procName, 1);
    ret = freadHeaderSpix(fp, pwidth, pheight, pbps, pspp, piscmap);
    fclose(fp);
    return ret;
}


/*!
 *  freadHeaderSpix()
 *
 *      Input:  stream
 *              &width (<return>)
 *              &height (<return>)
 *              &bps (<return>, bits/sample)
 *              &spp (<return>, samples/pixel)
 *              &iscmap (<optional return>; input NULL to ignore)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) If there is a colormap, iscmap is returned as 1; else 0.
 */
l_int32
freadHeaderSpix(FILE     *fp,
                l_int32  *pwidth,
                l_int32  *pheight,
                l_int32  *pbps,
                l_int32  *pspp,
                l_int32  *piscmap)
{
l_int32    nbytes, ret;
l_uint32  *data;

    PROCNAME("freadHeaderSpix");

    if (!fp)
        return ERROR_INT("stream not defined", procName, 1);
    if (!pwidth || !pheight || !pbps || !pspp)
        return ERROR_INT("input ptr(s) not defined", procName, 1);

    nbytes = fnbytesInFile(fp);
    if (nbytes < 32)
        return ERROR_INT("file too small to be spix", procName, 1);
    if ((data = (l_uint32 *)CALLOC(6, sizeof(l_uint32))) == NULL)
        return ERROR_INT("CALLOC fail for data", procName, 1);
    if (fread(data, 4, 6, fp) != 6)
        return ERROR_INT("error reading data", procName, 1);
    ret = sreadHeaderSpix(data, pwidth, pheight, pbps, pspp, piscmap);
    FREE(data);
    return ret;
}


/*!
 *  sreadHeaderSpix()
 *
 *      Input:  data
 *              &width (<return>)
 *              &height (<return>)
 *              &bps (<return>, bits/sample)
 *              &spp (<return>, samples/pixel)
 *              &iscmap (<optional return>; input NULL to ignore)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) If there is a colormap, iscmap is returned as 1; else 0.
 */
l_int32
sreadHeaderSpix(const l_uint32  *data,
                l_int32         *pwidth,
                l_int32         *pheight,
                l_int32         *pbps,
                l_int32         *pspp,
                l_int32         *piscmap)
{
char    *id;
l_int32  d, ncolors;

    PROCNAME("sreadHeaderSpix");

    if (!data)
        return ERROR_INT("data not defined", procName, 1);
    if (!pwidth || !pheight || !pbps || !pspp)
        return ERROR_INT("input ptr(s) not defined", procName, 1);
    *pwidth = *pheight = *pbps = *pspp = 0;
    if (piscmap)
      *piscmap = 0;

        /* Check file id */
    id = (char *)data;
    if (id[0] != 's' || id[1] != 'p' || id[2] != 'i' || id[3] != 'x')
        return ERROR_INT("not a valid spix file", procName, 1);

    *pwidth = data[1];
    *pheight = data[2];
    d = data[3];
    if (d <= 16) {
      *pbps = d;
      *pspp = 1;
    }
    else {
      *pbps = 8;
      *pspp = d / 8;  /* if the pix is 32 bpp, call it 4 samples */
    }
    ncolors = data[5];
    if (piscmap)
        *piscmap = (ncolors == 0) ? 0 : 1;

    return 0;
}


/*-----------------------------------------------------------------------*
 *                            Writing spix to file                       *
 *-----------------------------------------------------------------------*/
/*!
 *  pixWriteStreamSpix()
 *
 *      Input:  stream
 *              pix
 *      Return: 0 if OK; 1 on error
 */
l_int32
pixWriteStreamSpix(FILE  *fp,
                   PIX   *pix)
{
l_uint8  *data;
size_t    size;

    PROCNAME("pixWriteStreamSpix");

    if (!fp)
        return ERROR_INT("stream not defined", procName, 1);
    if (!pix)
        return ERROR_INT("pix not defined", procName, 1);

    if (pixWriteMemSpix(&data, &size, pix))
        return ERROR_INT("failure to write pix to memory", procName, 1);
    fwrite(data, 1, size, fp);
    FREE(data);
    return 0;
}


/*-----------------------------------------------------------------------*
 *       Low-level serialization of pix to/from memory (uncompressed)    *
 *-----------------------------------------------------------------------*/
/*!
 *  pixReadMemSpix()
 *
 *      Input:  data (const; uncompressed)
 *              size (of data)
 *      Return: pix, or null on error
 */
PIX *
pixReadMemSpix(const l_uint8  *data,
               size_t          size)
{
    return pixDeserializeFromMemory((l_uint32 *)data, size);
}


/*!
 *  pixWriteMemSpix()
 *
 *      Input:  &data (<return> data of serialized, uncompressed pix)
 *              &size (<return> size of returned data)
 *              pix (all depths; colormap OK)
 *      Return: 0 if OK, 1 on error
 */
l_int32
pixWriteMemSpix(l_uint8  **pdata,
                size_t    *psize,
                PIX       *pix)
{
    return pixSerializeToMemory(pix, (l_uint32 **)pdata, psize);
}


/*!
 *  pixSerializeToMemory()
 *
 *      Input:  pixs (all depths, colormap OK)
 *              &data (<return> serialized data in memory)
 *              &nbytes (<return> number of bytes in data string)
 *      Return: 0 if OK, 1 on error
 *
 *  Notes:
 *      (1) This does a fast serialization of the principal elements
 *          of the pix, as follows:
 *            "spix"    (4 bytes) -- ID for file type
 *            w         (4 bytes)
 *            h         (4 bytes)
 *            d         (4 bytes)
 *            wpl       (4 bytes)
 *            ncolors   (4 bytes) -- in colormap; 0 if there is no colormap
 *            cdatasize (4 bytes) -- size of serialized colormap
 *                                   = 4 * (num colors)
 *            cdata     (cdatasize)
 *            rdatasize (4 bytes) -- size of serialized raster data
 *                                   = 4 * wpl * h
 *            rdata     (rdatasize)
 */
l_int32
pixSerializeToMemory(PIX        *pixs,
                     l_uint32  **pdata,
                     size_t    *pnbytes)
{
char      *id;
l_int32    w, h, d, wpl, rdatasize, cdatasize, ncolors, nbytes, index;
l_uint8   *cdata;  /* data in colormap (4 bytes/color table entry) */
l_uint32  *data;
l_uint32  *rdata;  /* data in pix raster */
PIXCMAP   *cmap;

    PROCNAME("pixSerializeToMemory");

    if (!pdata || !pnbytes)
        return ERROR_INT("&data and &nbytes not both defined", procName, 1);
    *pdata = NULL;
    *pnbytes = 0;
    if (!pixs)
        return ERROR_INT("pixs not defined", procName, 1);

    pixGetDimensions(pixs, &w, &h, &d);
    wpl = pixGetWpl(pixs);
    rdata = pixGetData(pixs);
    rdatasize = 4 * wpl * h;
    cdatasize = 0;
    ncolors = 0;
    cdata = NULL;
    if ((cmap = pixGetColormap(pixs)) != NULL)
        pixcmapSerializeToMemory(cmap, 4, &ncolors, &cdata, &cdatasize);

    nbytes = 32 + cdatasize + rdatasize;
    if ((data = (l_uint32 *)CALLOC(nbytes / 4, sizeof(l_uint32))) == NULL)
        return ERROR_INT("data not made", procName, 1);
    *pdata = data;
    *pnbytes = nbytes;
    id = (char *)data;
    id[0] = 's';
    id[1] = 'p';
    id[2] = 'i';
    id[3] = 'x';
    data[1] = w;
    data[2] = h;
    data[3] = d;
    data[4] = wpl;
    data[5] = ncolors;
    data[6] = cdatasize;
    if (cdatasize > 0)
        memcpy((char *)(data + 7), (char *)cdata, cdatasize);
    index = 7 + cdatasize / 4;
    data[index] = rdatasize;
    memcpy((char *)(data + index + 1), (char *)rdata, rdatasize);

#if  DEBUG_SERIALIZE
    fprintf(stderr, "Serialize:   "
            "raster size = %d, ncolors in cmap = %d, total bytes = %d\n",
            rdatasize, ncolors, nbytes);
#endif  /* DEBUG_SERIALIZE */

    FREE(cdata);
    return 0;
}


/*!
 *  pixDeserializeFromMemory()
 *
 *      Input:  data (serialized data in memory)
 *              nbytes (number of bytes in data string)
 *      Return: pix, or NULL on error
 *
 *  Notes:
 *      (1) See pixSerializeToMemory() for the binary format.
 */
PIX *
pixDeserializeFromMemory(const l_uint32  *data,
                         size_t           nbytes)
{
char      *id;
l_int32    w, h, d, wpl, imdatasize, cdatasize, ncolors;
l_uint32  *imdata;  /* data in pix raster */
PIX       *pixd;
PIXCMAP   *cmap;

    PROCNAME("pixDeserializeFromMemory");

    if (!data)
        return (PIX *)ERROR_PTR("data not defined", procName, NULL);
    if (nbytes < 32)
        return (PIX *)ERROR_PTR("invalid data", procName, NULL);

    id = (char *)data;
    if (id[0] != 's' || id[1] != 'p' || id[2] != 'i' || id[3] != 'x')
        return (PIX *)ERROR_PTR("invalid id string", procName, NULL);
    w = data[1];
    h = data[2];
    d = data[3];
    if ((pixd = pixCreate(w, h, d)) == NULL)
        return (PIX *)ERROR_PTR("pix not made", procName, NULL);

    wpl = data[4];
    ncolors = data[5];
    if ((cdatasize = data[6]) > 0) {
        cmap = pixcmapDeserializeFromMemory((l_uint8 *)(&data[7]), ncolors,
                                            cdatasize);
        if (!cmap)
            return (PIX *)ERROR_PTR("cmap not made", procName, NULL);
        pixSetColormap(pixd, cmap);
    }

    imdata = pixGetData(pixd);
    imdatasize = nbytes - 32 - cdatasize;
    memcpy((char *)imdata, (char *)(data + 8 + cdatasize / 4), imdatasize);

#if  DEBUG_SERIALIZE
    fprintf(stderr, "Deserialize: "
            "raster size = %d, ncolors in cmap = %d, total bytes = %d\n",
            imdatasize, ncolors, nbytes);
#endif  /* DEBUG_SERIALIZE */

    return pixd;
}


