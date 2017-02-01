# Leptonica Library #

[![Build Status](https://travis-ci.org/DanBloomberg/leptonica.svg?branch=master)](https://travis-ci.org/DanBloomberg/leptonica)
[![Build status](https://ci.appveyor.com/api/projects/status/vsk607rr6n4j2tmk?svg=true)](https://ci.appveyor.com/project/DanBloomberg/leptonica)

www.leptonica.org

## The library supports many operations that are useful on ##

  * Document images
  * Natural images

## Fundamental image processing and image analysis operations ##

  * Rasterop (aka bitblt)
  * Affine transforms (scaling, translation, rotation, shear) on images of arbitrary pixel depth
  * Projective and bilinear transforms
  * Binary and grayscale morphology, rank order filters, and convolution
  * Seedfill and connected components
  * Image transformations with changes in pixel depth, both at the same scale and with scale change
  * Pixelwise masking, blending, enhancement, arithmetic ops, etc.

## Ancillary utilities ##

  * I/O for standard image formats (_jpg_, _png_, _tiff_, _webp_, _jp2_, _bmp_, _pnm_, _gif_, _ps_, _pdf_)
  * Utilities to handle arrays of image-related data types (e.g., _pixa_, _boxa_, _pta_)
  * Utilities for stacks, generic arrays, queues, heaps, lists; number and string arrays; etc.

## Examples of some applications enabled and implemented ##

  * Octcube-based color quantization (w/ and w/out dithering)
  * Modified median cut color quantization (w/ and w/out dithering)
  * Skew determination of text images
  * Adaptive normalization and binarization
  * Segmentation of page images with mixed text and images
  * Location of baselines and local skew determination
  * jbig2 unsupervised classifier
  * Border representations of 1 bpp images and raster conversion for SVG
  * Postscript generation (levels 1, 2 and 3) of images for device-independent output
  * PDF generation (G4, DCT, FLATE) of images for device-independent output
  * Connectivity-preserving thinning and thickening of 1 bpp images
  * Image warping (captcha, stereoscopic)
  * Image dewarping based on content (textlines)
  * Watershed transform
  * Greedy splitting of components into rectangles
  * Location of largest fg or bg rectangles in 1 bpp images
  * Search for least-cost paths on binary and grayscale images
  * Barcode reader for 1D barcodes (very early version as of 1.55)

## Implementation characteristics ##

  * _Efficient_: image data is packed binary (into 32-bit words); operations on 32-bit data whenever possible
  * _Simple_: small number of data structures; simplest implementations provided that are efficient
  * _Consistent_: data allocated on the heap with simple ownership rules; function names usually begin with primary data structure (e.g., _pix_); simple code patterns throughout
  * _Robust_: all ptr args checked; extensive use of accessors; exit not permitted
  * _Tested_: thorough regression tests provided for most basic functions; valgrind tested
  * _ANSI C_: automatically generated prototype header file
  * _Portable_: endian-independent; builds in linux, osx, mingw, cygwin, windows
  * _Nearly thread-safe_: ref counting on some structs
  * _Documentation_: large number of in-line comments; web pages for further background
  * _Examples_: many programs provided to test and show usage of approx. 2200 functions in the library


## Open Source Projects that use Leptonica ##
  * [php](http://en.wikipedia.org/wiki/PHP)  (scripting language for dynamic web pages)
  * [tesseract](https://github.com/tesseract-ocr/tesseract/) (optical character recognition)
  * [jbig2enc](http://www.imperialviolet.org/jbig2.html) (encodes multipage binary image documents with jbig2 compression)

## Major contributors to Leptonica ##
  * Tom Powers: Tom has supported leptonica on Windows for many years.  He has made many contributions to code quality and documentation, including the beautiful "unofficial documentation" on the web site. Without his effort, leptonica would not run today on Windows.
  * David Bryan: David has worked for years to support leptonica on multiple platforms. He designed many nice features in leptonica, such as the severity-based error messaging system, and has identified and fixed countless bugs. And he has built and tested each distribution many times on cross-compilers.
  * James Le Cuirot: James has written and supported the autotools scripts on leptonica distributions for many years, and has helped test every distribution since 1.67.
  * Jeff Breidenbach: Jeff has built every debian distribution for leptonica. He has also made many improvements to formatted image I/O, including tiff, png and pdf. He is a continuous advocate for simplification.
  * Egor Pugin: Egor is co-maintainer of leptonica on github. He ported everything, including all the old distributions, from Google Code when it shut down. He set leptonica up for appveyor and travis testing, and has implemented the cppan project, which simplifies building executables on Windows.
  * Jürgen Buchmüller: Jürgen wrote text converters to modify leptonica source code so that it generates documentation using doxygen. He also wrote tiff wrappers for memory I/O.
