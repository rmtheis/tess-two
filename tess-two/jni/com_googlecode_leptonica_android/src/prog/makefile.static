#/*====================================================================*
# -  Copyright (C) 2001 Leptonica.  All rights reserved.
# -
# -  Redistribution and use in source and binary forms, with or without
# -  modification, are permitted provided that the following conditions
# -  are met:
# -  1. Redistributions of source code must retain the above copyright
# -     notice, this list of conditions and the following disclaimer.
# -  2. Redistributions in binary form must reproduce the above
# -     copyright notice, this list of conditions and the following
# -     disclaimer in the documentation and/or other materials
# -     provided with the distribution.
# -
# -  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# -  ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# -  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# -  A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL ANY
# -  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# -  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# -  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# -  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
# -  OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# -  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# -  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
# *====================================================================*/

#   makefile  (for linux)
#
#   For a nodebug version:             make
#   For a debug version:               make DEBUG=yes
#   For a shared library version:      make SHARED=yes
#   With nonstandard library directories:
#                    make EXTRALIBS="-L<nonstandard-lib-dir>"
#   With nonstandard header directories
#                    make EXTRAINCLUDES="-I<nonstandard-incl-dir>"
#
#   To remove all executables:  make clean
#
#   ========================================================================
#   IMPORTANT: This Makefile is set up to link with liblept.a and liblept.so
#   that were built using using src/makefile (i.e., with 'make' in src).
#   If instead you built the library with configure/make/makeinstall,
#   you must do this here:
#
#     (1) Change LIB_NODEBUG_DIR:
#            LIB_NODEBUG_DIR = /usr/local/lib  [or wherever you installed
#                                               liblept.a]
#     (2) Edit ALL_LIBS to include the imaging libraries on your system,
#         as found in config_auto.h.  For example, if you have the
#         jpeg, png, tiff and gif libraries, set
#            ALL_LIBS = $(LEPTLIB) -ltiff -ljpeg -lpng -lgif -lz -lm
#   ========================================================================
#
#   To link and run programs using shared (dynamic linked) libraries,
#   you must do one of the following two things:
#
#     (a)  make sure your LD_LIBRARY_PATH variable points to the
#          directory in which the *.so files are placed; e.g.
#            export LD_LIBRARY_PATH=../lib/shared:$LD_LIBRARY_PATH
#          or
#     (b)  have the *.so files (or links to them) in this directory
#
#   Using gnu make, this should compile properly on a bigendian
#   machine (e.g., Mac).  Otherwise, you can explicitly set the endianness,
#   using either of these
#      CPPFLAGS =    $(INCLUDES) -DL_LITTLE_ENDIAN -D_STANDARD_C_
#      CPPFLAGS =    $(INCLUDES) -DL_BIG_ENDIAN -D_STANDARD_C_
#
#   To use the non-POSIX-compliant GNU functions fmemopen() and open_memstream(),
#   remove -D_STANDARD_C_ from CPPFLAGS:
#      CPPFLAGS =    $(INCLUDES) -D$(ENDIANNESS)
#
#   On cygwin, remove -fPIC from CC.  I believe that these files --
#   convertfilestops.c, jbcorrelation.c, jbrankhaus.c, maketile.c,
#   and viewertest.c -- which use posix directory functions, will
#   now work properly.
#
#   See below for enabling gif read/write.


# Libraries are built into a binary tree with its root in the
# parent directory
ROOT_DIR = ..

LIB_NODEBUG_DIR =	$(ROOT_DIR)/lib/nodebug
LIB_DEBUG_DIR =		$(ROOT_DIR)/lib/debug
LIB_SHARED_DIR =	$(ROOT_DIR)/lib/shared

# Include files are found within the same tree
IMAGELIB_INCL =	$(ROOT_DIR)/src


CC =		gcc -ansi -DANSI -fPIC
#CC =		gcc -ansi -Werror -DANSI -fPIC
#CC =            g++ -Werror -fPIC


ifdef  SHARED
    LIB_DIR =	$(LIB_SHARED_DIR)
    OPTIMIZE =	-O2
else
    ifdef  DEBUG
	LIB_DIR =	$(LIB_DEBUG_DIR)
	OPTIMIZE =	-g
    else
	LIB_DIR =	$(LIB_NODEBUG_DIR)
	OPTIMIZE =	-O2
    endif
endif

OPTIONS =

INCLUDES =	-I$(IMAGELIB_INCL) -I/usr/X11R6/include $(EXTRAINCLUDES)
#INCLUDES =	-I$(IMAGELIB_INCL) -I/usr/X11R6/include -I/usr/local/include $(EXTRAINCLUDES)

CFLAGS =	$(OPTIMIZE) $(OPTIONS)

# Test for processor endianness (valid with gnu make only)
ENDIANNESS := $(shell $(CC) -o endiantest endiantest.c; ./endiantest; rm -f endiantest)

# Conditional compilation (depending on processor endian-ness)
# Use -D_STANDARD_C to stub out the non-posix GNU functions fmemopen()
# and open_memstream()
CPPFLAGS =      $(INCLUDES) -D$(ENDIANNESS)
#CPPFLAGS =      $(INCLUDES) -DL_LITTLE_ENDIAN
#CPPFLAGS =      $(INCLUDES) -DL_BIG_ENDIAN

LDFLAGS +=	-L$(LIB_DIR) -L/usr/X11R6/lib -L/usr/lib $(EXTRALIBS)
#LDFLAGS +=	-L$(LIB_DIR) -L/usr/X11R6/lib -L/usr/local/lib -L/usr/lib $(EXTRALIBS)

ifdef  SHARED
    LEPTLIB =		$(LIB_DIR)/liblept.so
else
    LEPTLIB =		$(LIB_DIR)/liblept.a
endif

# To enable webp read/write:
#    (1) In src/environ.h:  #define  HAVE_LIBWEBP
#    (2) Install libwebp
# To enable gif read/write:
#    (1) In src/environ.h:  #define  HAVE_LIBGIF
#    (2) Install libgif
# Be sure LD_LIBRARY_PATH includes the appropriate library directories, such
# as /usr/local/include, in which libwebp.so and/or libgif.so are installed
#    (3) Use the appropriate line below for ALL_LIBS
ALL_LIBS =	$(LEPTLIB) -ltiff -ljpeg -lpng -lz -lm
#ALL_LIBS =	$(LEPTLIB) -ltiff -ljpeg -lpng -lwebp -lz -lm
#ALL_LIBS =	$(LEPTLIB) -ltiff -ljpeg -lpng -lgif -lwebp -lz -lm

#########################################################################

SRC =		adaptnorm_reg.c affine_reg.c \
		alltests_reg.c alphaops_reg.c alphaxform_reg.c \
		bilateral1_reg.c bilateral2_reg.c \
		bilinear_reg.c binarize_reg.c \
		binmorph1_reg.c binmorph2_reg.c \
		binmorph3_reg.c binmorph4_reg.c binmorph5_reg.c \
		blackwhite_reg.c \
		blend1_reg.c blend2_reg.c blend3_reg.c blend4_reg.c \
		boxa1_reg.c \
		ccthin1_reg.c ccthin2_reg.c \
		cmapquant_reg.c colorcontent_reg.c \
		coloring_reg.c colorize_reg.c \
		colormask_reg.c colorquant_reg.c \
		colorseg_reg.c colorspace_reg.c \
		compare_reg.c compfilter_reg.c \
		conncomp_reg.c conversion_reg.c convolve_reg.c \
		dewarp_reg.c distance_reg.c dna_reg.c \
		dwamorph1_reg.c dwamorph2_reg.c \
		enhance_reg.c equal_reg.c \
		expand_reg.c extrema_reg.c \
		fhmtauto_reg.c files_reg.c \
		findcorners_reg.c findpattern_reg.c \
		flipdetect_reg.c fmorphauto_reg.c \
		fpix1_reg.c fpix2_reg.c gifio_reg.c \
		grayfill_reg.c graymorph1_reg.c \
		graymorph2_reg.c  grayquant_reg.c \
		hardlight_reg.c heap_reg.c \
		insert_reg.c ioformats_reg.c \
		jp2kio_reg.c jpegio_reg.c kernel_reg.c \
		label_reg.c locminmax_reg.c \
		logicops_reg.c lowaccess_reg.c \
		maze_reg.c morphseq_reg.c multitype_reg.c \
		nearline_reg.c newspaper_reg.c \
		numa1_reg.c numa2_reg.c \
		overlap_reg.c paint_reg.c paintmask_reg.c \
		pdfseg_reg.c pixa1_reg.c pixa2_reg.c \
		pixadisp_reg.c pixalloc_reg.c \
		pixcomp_reg.c pixmem_reg.c \
		pixserial_reg.c pixtile_reg.c \
		pngio_reg.c projection_reg.c projective_reg.c \
		psio_reg.c psioseg_reg.c \
		pta_reg.c ptra1_reg.c ptra2_reg.c \
		rank_reg.c rankbin_reg.c rankhisto_reg.c \
		rasterop_reg.c rasteropip_reg.c \
		rotate1_reg.c rotate2_reg.c rotateorth_reg.c \
		scale_reg.c seedspread_reg.c selio_reg.c \
		shear1_reg.c shear2_reg.c skew_reg.c \
		smallpix_reg.c smoothedge_reg.c splitcomp_reg.c \
		string_reg.c subpixel_reg.c threshnorm_reg.c \
		translate_reg.c warper_reg.c webpio_reg.c \
		wordboxes_reg.c writetext_reg.c xformbox_reg.c \
		adaptmaptest.c adaptmap_dark.c \
		arabic_lines.c arithtest.c \
		barcodetest.c baselinetest.c \
		binarizefiles.c binarize_set.c bincompare.c \
		blendcmaptest.c buffertest.c \
		byteatest.c ccbordtest.c cctest1.c \
		colormorphtest.c \
		colorsegtest.c colorspacetest.c \
		comparepages.c comparetest.c \
		concatpdf.c contrasttest.c \
		convertfilestopdf.c convertfilestops.c \
		convertformat.c \
		convertsegfilestopdf.c convertsegfilestops.c \
		converttogray.c converttopdf.c converttops.c \
		convolvetest.c cornertest.c \
		croptest.c croptext.c \
		dewarprules.c dewarptest1.c dewarptest2.c \
		dewarptest3.c dewarptest4.c \
		digitprep1.c displayboxa.c \
		displaypix.c displaypixa.c \
		dithertest.c dwalineargen.c \
		edgetest.c falsecolortest.c \
		fcombautogen.c fhmtautogen.c \
		fileinfo.c \
		findpattern1.c findpattern2.c findpattern3.c \
		flipselgen.c fmorphautogen.c \
		fpixcontours.c gammatest.c genfonts.c \
		graphicstest.c graymorphtest.c \
		histotest.c iotest.c italictest.c \
		jbcorrelation.c jbrankhaus.c jbwords.c \
		lineremoval.c listtest.c \
		livre_adapt.c livre_hmt.c \
		livre_makefigs.c livre_orient.c \
		livre_pageseg.c livre_seedgen.c livre_tophat.c \
		maketile.c misctest1.c \
		modifyhuesat.c morphtest1.c mtifftest.c \
		numaranktest.c otsutest1.c otsutest2.c \
		pagesegtest1.c pagesegtest2.c \
		partitiontest.c pdfiotest.c \
		pixaatest.c plottest.c \
		printimage.c printsplitimage.c printtiff.c \
		quadtreetest.c ranktest.c \
		recogsort.c recogtest1.c \
		reducetest.c removecmap.c \
		renderfonts.c rotatefastalt.c \
		rotateorthtest1.c rotatetest1.c \
		runlengthtest.c scaleandtile.c \
		scaletest1.c scaletest2.c \
		seedfilltest.c sharptest.c \
		sheartest.c showedges.c \
		skewtest.c snapcolortest.c \
		sorttest.c splitimage2pdf.c \
		sudokutest.c trctest.c \
		viewertest.c warpertest.c \
		watershedtest.c wordsinorder.c \
		writemtiff.c xtractprotos.c yuvtest.c

#  Remove the .o files from these on clean
SRC2 =		dwalinear.3.c dwalinearlow.3.c

######################################################################

all:	$(SRC:%.c=%)

#    Jan 2014  (minimal set of utility programs)
#debian:	convertfilestopdf convertfilestops convertformat \
#	convertsegfilestopdf convertsegfilestops \
#	converttopdf converttops fileinfo \
#	printimage printsplitimage printtiff \
#	splitimage2pdf xtractprotos

#    Jan 2014  (could have been in Jan 2014 debian, but left off)
#	grayfill_reg histotest_reg sudokutest watershedtest 

#    Jan 2012
#debian:	binarize_reg binmorph2_reg ccthin2_reg colorquant_reg \
#	colorseg_reg convolve_reg dewarp_reg \
#	dwamorph1_reg dwamorph2_reg \
#	distance_reg enhance_reg ioformats_reg \
#	maze_reg paintmask_reg \
#	rotate1_reg rotate2_reg scale_reg \
#	seedspread_reg splitcomp_reg threshnorm_reg \
#	warper_reg convertfilestopdf convertfilestops \
#	convertjpegfilestopdf converttops dewarptest1 \
#	fcombautogen fhmtautogen fileinfo \
#	fmorphautogen pdfiotest \
#	printimage printsplitimage splitimage2pdf \
#	sudokutest watershedtest xtractprotos


#   Jan 2011
#debian:	binarize_reg cctest1 ccthin1_reg \
#	colormorphtest colorquant_reg colorspacetest \
#	comparetest convertfilestopdf convertfilestops \
#	convertformat converttops dewarp_reg \
#	distance_reg dithertest fileinfo \
#	flipdetect_reg fmorphauto_reg  gammatest \
#	grayfill_reg graymorph1_reg grayquant_reg \
#	hardlight_reg  ioformats_reg jbcorrelation \
#	kernel_reg lineremoval maze_reg \
#	pagesegtest1 paint_reg paintmask_reg \
#	printimage printsplitimage printtiff \
#	rank_reg ranktest scale_reg \
#	skewtest splitcomp_reg warper_reg \
#	watershedtest xtractprotos

######################################################################

adaptnorm_reg:	adaptnorm_reg.o $(LEPTLIB)
	$(CC) -o adaptnorm_reg adaptnorm_reg.o $(ALL_LIBS) $(EXTRALIBS)

affine_reg:	affine_reg.o $(LEPTLIB)
	$(CC) -o affine_reg affine_reg.o $(ALL_LIBS) $(EXTRALIBS)

alltests_reg:	alltests_reg.o $(LEPTLIB)
	$(CC) -o alltests_reg alltests_reg.o $(ALL_LIBS) $(EXTRALIBS)

alphaops_reg:	alphaops_reg.o $(LEPTLIB)
	$(CC) -o alphaops_reg alphaops_reg.o $(ALL_LIBS) $(EXTRALIBS)

alphaxform_reg:	alphaxform_reg.o $(LEPTLIB)
	$(CC) -o alphaxform_reg alphaxform_reg.o $(ALL_LIBS) $(EXTRALIBS)

bilateral1_reg:	bilateral1_reg.o $(LEPTLIB)
	$(CC) -o bilateral1_reg bilateral1_reg.o $(ALL_LIBS) $(EXTRALIBS)

bilateral2_reg:	bilateral2_reg.o $(LEPTLIB)
	$(CC) -o bilateral2_reg bilateral2_reg.o $(ALL_LIBS) $(EXTRALIBS)

bilinear_reg:	bilinear_reg.o $(LEPTLIB)
	$(CC) -o bilinear_reg bilinear_reg.o $(ALL_LIBS) $(EXTRALIBS)

binarize_reg:	binarize_reg.o $(LEPTLIB)
	$(CC) -o binarize_reg binarize_reg.o $(ALL_LIBS) $(EXTRALIBS)

binmorph1_reg:	binmorph1_reg.o $(LEPTLIB)
	$(CC) -o binmorph1_reg binmorph1_reg.o $(ALL_LIBS) $(EXTRALIBS)

binmorph2_reg:	binmorph2_reg.o $(LEPTLIB)
	$(CC) -o binmorph2_reg binmorph2_reg.o $(ALL_LIBS) $(EXTRALIBS)

binmorph3_reg:	binmorph3_reg.o $(LEPTLIB)
	$(CC) -o binmorph3_reg binmorph3_reg.o $(ALL_LIBS) $(EXTRALIBS)

binmorph4_reg:	binmorph4_reg.o $(LEPTLIB)
	$(CC) -o binmorph4_reg binmorph4_reg.o $(ALL_LIBS) $(EXTRALIBS)

binmorph5_reg:	binmorph5_reg.o $(LEPTLIB)
	$(CC) -o binmorph5_reg binmorph5_reg.o $(ALL_LIBS) $(EXTRALIBS)

blackwhite_reg:	blackwhite_reg.o $(LEPTLIB)
	$(CC) -o blackwhite_reg blackwhite_reg.o $(ALL_LIBS) $(EXTRALIBS)

blend1_reg:	blend1_reg.o $(LEPTLIB)
	$(CC) -o blend1_reg blend1_reg.o $(ALL_LIBS) $(EXTRALIBS)

blend2_reg:	blend2_reg.o $(LEPTLIB)
	$(CC) -o blend2_reg blend2_reg.o $(ALL_LIBS) $(EXTRALIBS)

blend3_reg:	blend3_reg.o $(LEPTLIB)
	$(CC) -o blend3_reg blend3_reg.o $(ALL_LIBS) $(EXTRALIBS)

blend4_reg:	blend4_reg.o $(LEPTLIB)
	$(CC) -o blend4_reg blend4_reg.o $(ALL_LIBS) $(EXTRALIBS)

boxa1_reg:	boxa1_reg.o $(LEPTLIB)
	$(CC) -o boxa1_reg boxa1_reg.o $(ALL_LIBS) $(EXTRALIBS)

ccthin1_reg:	ccthin1_reg.o $(LEPTLIB)
	$(CC) -o ccthin1_reg ccthin1_reg.o $(ALL_LIBS) $(EXTRALIBS)

ccthin2_reg:	ccthin2_reg.o $(LEPTLIB)
	$(CC) -o ccthin2_reg ccthin2_reg.o $(ALL_LIBS) $(EXTRALIBS)

cmapquant_reg:	cmapquant_reg.o $(LEPTLIB)
	$(CC) -o cmapquant_reg cmapquant_reg.o $(ALL_LIBS) $(EXTRALIBS)

colorcontent_reg:	colorcontent_reg.o $(LEPTLIB)
	$(CC) -o colorcontent_reg colorcontent_reg.o $(ALL_LIBS) $(EXTRALIBS)

coloring_reg:	coloring_reg.o $(LEPTLIB)
	$(CC) -o coloring_reg coloring_reg.o $(ALL_LIBS) $(EXTRALIBS)

colorize_reg:	colorize_reg.o $(LEPTLIB)
	$(CC) -o colorize_reg colorize_reg.o $(ALL_LIBS) $(EXTRALIBS)

colormask_reg:	colormask_reg.o $(LEPTLIB)
	$(CC) -o colormask_reg colormask_reg.o $(ALL_LIBS) $(EXTRALIBS)

colorquant_reg:	colorquant_reg.o $(LEPTLIB)
	$(CC) -o colorquant_reg colorquant_reg.o $(ALL_LIBS) $(EXTRALIBS)

colorseg_reg:	colorseg_reg.o $(LEPTLIB)
	$(CC) -o colorseg_reg colorseg_reg.o $(ALL_LIBS) $(EXTRALIBS)

colorspace_reg:	colorspace_reg.o $(LEPTLIB)
	$(CC) -o colorspace_reg colorspace_reg.o $(ALL_LIBS) $(EXTRALIBS)

compare_reg: compare_reg.o $(LEPTLIB)
	$(CC) -o compare_reg compare_reg.o $(ALL_LIBS) $(EXTRALIBS)

compfilter_reg: compfilter_reg.o $(LEPTLIB)
	$(CC) -o compfilter_reg compfilter_reg.o $(ALL_LIBS) $(EXTRALIBS)

conncomp_reg: conncomp_reg.o $(LEPTLIB)
	$(CC) -o conncomp_reg conncomp_reg.o $(ALL_LIBS) $(EXTRALIBS)

conversion_reg: conversion_reg.o $(LEPTLIB)
	$(CC) -o conversion_reg conversion_reg.o $(ALL_LIBS) $(EXTRALIBS)

convolve_reg:	convolve_reg.o $(LEPTLIB)
	$(CC) -o convolve_reg convolve_reg.o $(ALL_LIBS) $(EXTRALIBS)

dewarp_reg:	dewarp_reg.o $(LEPTLIB)
	$(CC) -o dewarp_reg dewarp_reg.o $(ALL_LIBS) $(EXTRALIBS)

distance_reg:	distance_reg.o $(LEPTLIB)
	$(CC) -o distance_reg distance_reg.o $(ALL_LIBS) $(EXTRALIBS)

dna_reg:	dna_reg.o $(LEPTLIB)
	$(CC) -o dna_reg dna_reg.o $(ALL_LIBS) $(EXTRALIBS)

dwamorph1_reg:  dwamorph1_reg.o dwalinear.3.o dwalinearlow.3.o $(LEPTLIB)
	$(CC) -o dwamorph1_reg dwamorph1_reg.o dwalinear.3.o dwalinearlow.3.o $(ALL_LIBS) $(EXTRALIBS)

dwamorph2_reg:  dwamorph2_reg.o dwalinear.3.o dwalinearlow.3.o $(LEPTLIB)
	$(CC) -o dwamorph2_reg dwamorph2_reg.o dwalinear.3.o dwalinearlow.3.o $(ALL_LIBS) $(EXTRALIBS)

enhance_reg:	enhance_reg.o $(LEPTLIB)
	$(CC) -o enhance_reg enhance_reg.o $(ALL_LIBS) $(EXTRALIBS)

equal_reg:	equal_reg.o $(LEPTLIB)
	$(CC) -o equal_reg equal_reg.o $(ALL_LIBS) $(EXTRALIBS)

expand_reg:	expand_reg.o $(LEPTLIB)
	$(CC) -o expand_reg expand_reg.o $(ALL_LIBS) $(EXTRALIBS)

extrema_reg:	extrema_reg.o $(LEPTLIB)
	$(CC) -o extrema_reg extrema_reg.o $(ALL_LIBS) $(EXTRALIBS)

fhmtauto_reg:	fhmtauto_reg.o $(LEPTLIB)
	$(CC) -o fhmtauto_reg fhmtauto_reg.o $(ALL_LIBS) $(EXTRALIBS)

files_reg:	files_reg.o $(LEPTLIB)
	$(CC) -o files_reg files_reg.o $(ALL_LIBS) $(EXTRALIBS)

findcorners_reg:	findcorners_reg.o $(LEPTLIB)
	$(CC) -o findcorners_reg findcorners_reg.o $(ALL_LIBS) $(EXTRALIBS)

findpattern_reg:	findpattern_reg.o $(LEPTLIB)
	$(CC) -o findpattern_reg findpattern_reg.o $(ALL_LIBS) $(EXTRALIBS)

flipdetect_reg:	flipdetect_reg.o $(LEPTLIB)
	$(CC) -o flipdetect_reg flipdetect_reg.o $(ALL_LIBS) $(EXTRALIBS)

fmorphauto_reg:	fmorphauto_reg.o $(LEPTLIB)
	$(CC) -o fmorphauto_reg fmorphauto_reg.o $(ALL_LIBS) $(EXTRALIBS)

fpix1_reg:	fpix1_reg.o $(LEPTLIB)
	$(CC) -o fpix1_reg fpix1_reg.o $(ALL_LIBS) $(EXTRALIBS)

fpix2_reg:	fpix2_reg.o $(LEPTLIB)
	$(CC) -o fpix2_reg fpix2_reg.o $(ALL_LIBS) $(EXTRALIBS)

gifio_reg:	gifio_reg.o $(LEPTLIB)
	$(CC) -o gifio_reg gifio_reg.o $(ALL_LIBS) $(EXTRALIBS)

grayfill_reg:	grayfill_reg.o $(LEPTLIB)
	$(CC) -o grayfill_reg grayfill_reg.o $(ALL_LIBS) $(EXTRALIBS)

graymorph1_reg:	graymorph1_reg.o $(LEPTLIB)
	$(CC) -o graymorph1_reg graymorph1_reg.o $(ALL_LIBS) $(EXTRALIBS)

graymorph2_reg:	graymorph2_reg.o $(LEPTLIB)
	$(CC) -o graymorph2_reg graymorph2_reg.o $(ALL_LIBS) $(EXTRALIBS)

grayquant_reg:	grayquant_reg.o $(LEPTLIB)
	$(CC) -o grayquant_reg grayquant_reg.o $(ALL_LIBS) $(EXTRALIBS)

hardlight_reg:	hardlight_reg.o $(LEPTLIB)
	$(CC) -o hardlight_reg hardlight_reg.o $(ALL_LIBS) $(EXTRALIBS)

heap_reg:	heap_reg.o $(LEPTLIB)
	$(CC) -o heap_reg heap_reg.o $(ALL_LIBS) $(EXTRALIBS)

insert_reg:	insert_reg.o $(LEPTLIB)
	$(CC) -o insert_reg insert_reg.o $(ALL_LIBS) $(EXTRALIBS)

ioformats_reg:	ioformats_reg.o $(LEPTLIB)
	$(CC) -o ioformats_reg ioformats_reg.o $(ALL_LIBS) $(EXTRALIBS)

jp2kio_reg:	jp2kio_reg.o $(LEPTLIB)
	$(CC) -o jp2kio_reg jp2kio_reg.o $(ALL_LIBS) $(EXTRALIBS)

jpegio_reg:	jpegio_reg.o $(LEPTLIB)
	$(CC) -o jpegio_reg jpegio_reg.o $(ALL_LIBS) $(EXTRALIBS)

kernel_reg:	kernel_reg.o $(LEPTLIB)
	$(CC) -o kernel_reg kernel_reg.o $(ALL_LIBS) $(EXTRALIBS)

label_reg:	label_reg.o $(LEPTLIB)
	$(CC) -o label_reg label_reg.o $(ALL_LIBS) $(EXTRALIBS)

locminmax_reg:	locminmax_reg.o $(LEPTLIB)
	$(CC) -o locminmax_reg locminmax_reg.o $(ALL_LIBS) $(EXTRALIBS)

logicops_reg:	logicops_reg.o $(LEPTLIB)
	$(CC) -o logicops_reg logicops_reg.o $(ALL_LIBS) $(EXTRALIBS)

lowaccess_reg:	lowaccess_reg.o $(LEPTLIB)
	$(CC) -o lowaccess_reg lowaccess_reg.o $(ALL_LIBS) $(EXTRALIBS)

maze_reg:	maze_reg.o $(LEPTLIB)
	$(CC) -o maze_reg maze_reg.o $(ALL_LIBS) $(EXTRALIBS)

morphseq_reg:	morphseq_reg.o $(LEPTLIB)
	$(CC) -o morphseq_reg morphseq_reg.o $(ALL_LIBS) $(EXTRALIBS)

multitype_reg:	multitype_reg.o $(LEPTLIB)
	$(CC) -o multitype_reg multitype_reg.o $(ALL_LIBS) $(EXTRALIBS)

nearline_reg:	nearline_reg.o $(LEPTLIB)
	$(CC) -o nearline_reg nearline_reg.o $(ALL_LIBS) $(EXTRALIBS)

newspaper_reg:	newspaper_reg.o $(LEPTLIB)
	$(CC) -o newspaper_reg newspaper_reg.o $(ALL_LIBS) $(EXTRALIBS)

numa1_reg:	numa1_reg.o $(LEPTLIB)
	$(CC) -o numa1_reg numa1_reg.o $(ALL_LIBS) $(EXTRALIBS)

numa2_reg:	numa2_reg.o $(LEPTLIB)
	$(CC) -o numa2_reg numa2_reg.o $(ALL_LIBS) $(EXTRALIBS)

overlap_reg:	overlap_reg.o $(LEPTLIB)
	$(CC) -o overlap_reg overlap_reg.o $(ALL_LIBS) $(EXTRALIBS)

paint_reg:	paint_reg.o $(LEPTLIB)
	$(CC) -o paint_reg paint_reg.o $(ALL_LIBS) $(EXTRALIBS)

paintmask_reg:	paintmask_reg.o $(LEPTLIB)
	$(CC) -o paintmask_reg paintmask_reg.o $(ALL_LIBS) $(EXTRALIBS)

pdfseg_reg:	pdfseg_reg.o $(LEPTLIB)
	$(CC) -o pdfseg_reg pdfseg_reg.o $(ALL_LIBS) $(EXTRALIBS)

pixa1_reg:	pixa1_reg.o $(LEPTLIB)
	$(CC) -o pixa1_reg pixa1_reg.o $(ALL_LIBS) $(EXTRALIBS)

pixa2_reg:	pixa2_reg.o $(LEPTLIB)
	$(CC) -o pixa2_reg pixa2_reg.o $(ALL_LIBS) $(EXTRALIBS)

pixadisp_reg:	pixadisp_reg.o $(LEPTLIB)
	$(CC) -o pixadisp_reg pixadisp_reg.o $(ALL_LIBS) $(EXTRALIBS)

pixalloc_reg:	pixalloc_reg.o $(LEPTLIB)
	$(CC) -o pixalloc_reg pixalloc_reg.o $(ALL_LIBS) $(EXTRALIBS)

pixcomp_reg:	pixcomp_reg.o $(LEPTLIB)
	$(CC) -o pixcomp_reg pixcomp_reg.o $(ALL_LIBS) $(EXTRALIBS)

pixmem_reg:	pixmem_reg.o $(LEPTLIB)
	$(CC) -o pixmem_reg pixmem_reg.o $(ALL_LIBS) $(EXTRALIBS)

pixserial_reg:	pixserial_reg.o $(LEPTLIB)
	$(CC) -o pixserial_reg pixserial_reg.o $(ALL_LIBS) $(EXTRALIBS)

pixtile_reg:	pixtile_reg.o $(LEPTLIB)
	$(CC) -o pixtile_reg pixtile_reg.o $(ALL_LIBS) $(EXTRALIBS)

pngio_reg:	pngio_reg.o $(LEPTLIB)
	$(CC) -o pngio_reg pngio_reg.o $(ALL_LIBS) $(EXTRALIBS)

projection_reg:	projection_reg.o $(LEPTLIB)
	$(CC) -o projection_reg projection_reg.o $(ALL_LIBS) $(EXTRALIBS)

projective_reg:	projective_reg.o $(LEPTLIB)
	$(CC) -o projective_reg projective_reg.o $(ALL_LIBS) $(EXTRALIBS)

psio_reg:	psio_reg.o $(LEPTLIB)
	$(CC) -o psio_reg psio_reg.o $(ALL_LIBS) $(EXTRALIBS)

psioseg_reg:	psioseg_reg.o $(LEPTLIB)
	$(CC) -o psioseg_reg psioseg_reg.o $(ALL_LIBS) $(EXTRALIBS)

pta_reg:	pta_reg.o $(LEPTLIB)
	$(CC) -o pta_reg pta_reg.o $(ALL_LIBS) $(EXTRALIBS)

ptra1_reg:	ptra1_reg.o $(LEPTLIB)
	$(CC) -o ptra1_reg ptra1_reg.o $(ALL_LIBS) $(EXTRALIBS)

ptra2_reg:	ptra2_reg.o $(LEPTLIB)
	$(CC) -o ptra2_reg ptra2_reg.o $(ALL_LIBS) $(EXTRALIBS)

rank_reg:	rank_reg.o $(LEPTLIB)
	$(CC) -o rank_reg rank_reg.o $(ALL_LIBS) $(EXTRALIBS)

rankbin_reg:	rankbin_reg.o $(LEPTLIB)
	$(CC) -o rankbin_reg rankbin_reg.o $(ALL_LIBS) $(EXTRALIBS)

rankhisto_reg:	rankhisto_reg.o $(LEPTLIB)
	$(CC) -o rankhisto_reg rankhisto_reg.o $(ALL_LIBS) $(EXTRALIBS)

rasterop_reg:	rasterop_reg.o $(LEPTLIB)
	$(CC) -o rasterop_reg rasterop_reg.o $(ALL_LIBS) $(EXTRALIBS)

rasteropip_reg:	rasteropip_reg.o $(LEPTLIB)
	$(CC) -o rasteropip_reg rasteropip_reg.o $(ALL_LIBS) $(EXTRALIBS)

rotate1_reg:	rotate1_reg.o $(LEPTLIB)
	$(CC) -o rotate1_reg rotate1_reg.o $(ALL_LIBS) $(EXTRALIBS)

rotate2_reg:	rotate2_reg.o $(LEPTLIB)
	$(CC) -o rotate2_reg rotate2_reg.o $(ALL_LIBS) $(EXTRALIBS)

rotateorth_reg:	rotateorth_reg.o $(LEPTLIB)
	$(CC) -o rotateorth_reg rotateorth_reg.o $(ALL_LIBS) $(EXTRALIBS)

scale_reg:	scale_reg.o $(LEPTLIB)
	$(CC) -o scale_reg scale_reg.o $(ALL_LIBS) $(EXTRALIBS)

seedspread_reg:	seedspread_reg.o $(LEPTLIB)
	$(CC) -o seedspread_reg seedspread_reg.o $(ALL_LIBS) $(EXTRALIBS)

selio_reg:	selio_reg.o $(LEPTLIB)
	$(CC) -o selio_reg selio_reg.o $(ALL_LIBS) $(EXTRALIBS)

shear1_reg:	shear1_reg.o $(LEPTLIB)
	$(CC) -o shear1_reg shear1_reg.o $(ALL_LIBS) $(EXTRALIBS)

shear2_reg:	shear2_reg.o $(LEPTLIB)
	$(CC) -o shear2_reg shear2_reg.o $(ALL_LIBS) $(EXTRALIBS)

skew_reg:	skew_reg.o $(LEPTLIB)
	$(CC) -o skew_reg skew_reg.o $(ALL_LIBS) $(EXTRALIBS)

smallpix_reg:	smallpix_reg.o $(LEPTLIB)
	$(CC) -o smallpix_reg smallpix_reg.o $(ALL_LIBS) $(EXTRALIBS)

smoothedge_reg:	smoothedge_reg.o $(LEPTLIB)
	$(CC) -o smoothedge_reg smoothedge_reg.o $(ALL_LIBS) $(EXTRALIBS)

splitcomp_reg:	splitcomp_reg.o $(LEPTLIB)
	$(CC) -o splitcomp_reg splitcomp_reg.o $(ALL_LIBS) $(EXTRALIBS)

string_reg:	string_reg.o $(LEPTLIB)
	$(CC) -o string_reg string_reg.o $(ALL_LIBS) $(EXTRALIBS)

subpixel_reg:	subpixel_reg.o $(LEPTLIB)
	$(CC) -o subpixel_reg subpixel_reg.o $(ALL_LIBS) $(EXTRALIBS)

threshnorm_reg:	threshnorm_reg.o $(LEPTLIB)
	$(CC) -o threshnorm_reg threshnorm_reg.o $(ALL_LIBS) $(EXTRALIBS)

translate_reg:	translate_reg.o $(LEPTLIB)
	$(CC) -o translate_reg translate_reg.o $(ALL_LIBS) $(EXTRALIBS)

warper_reg:	warper_reg.o $(LEPTLIB)
	$(CC) -o warper_reg warper_reg.o $(ALL_LIBS) $(EXTRALIBS)

webpio_reg:	webpio_reg.o $(LEPTLIB)
	$(CC) -o webpio_reg webpio_reg.o $(ALL_LIBS) $(EXTRALIBS)

wordboxes_reg:	wordboxes_reg.o $(LEPTLIB)
	$(CC) -o wordboxes_reg wordboxes_reg.o $(ALL_LIBS) $(EXTRALIBS)

writetext_reg:	writetext_reg.o $(LEPTLIB)
	$(CC) -o writetext_reg writetext_reg.o $(ALL_LIBS) $(EXTRALIBS)

xformbox_reg:	xformbox_reg.o $(LEPTLIB)
	$(CC) -o xformbox_reg xformbox_reg.o $(ALL_LIBS) $(EXTRALIBS)

adaptmaptest:	adaptmaptest.o $(LEPTLIB)
	$(CC) -o adaptmaptest adaptmaptest.o $(ALL_LIBS) $(EXTRALIBS)

adaptmap_dark:	adaptmap_dark.o $(LEPTLIB)
	$(CC) -o adaptmap_dark adaptmap_dark.o $(ALL_LIBS) $(EXTRALIBS)

arabic_lines:	arabic_lines.o $(LEPTLIB)
	$(CC) -o arabic_lines arabic_lines.o $(ALL_LIBS) $(EXTRALIBS)

arithtest:	arithtest.o $(LEPTLIB)
	$(CC) -o arithtest arithtest.o $(ALL_LIBS) $(EXTRALIBS)

barcodetest:	barcodetest.o $(LEPTLIB)
	$(CC) -o barcodetest barcodetest.o $(ALL_LIBS) $(EXTRALIBS)

baselinetest:	baselinetest.o $(LEPTLIB)
	$(CC) -o baselinetest baselinetest.o $(ALL_LIBS) $(EXTRALIBS)

binarizefiles:	binarizefiles.o $(LEPTLIB)
	$(CC) -o binarizefiles binarizefiles.o $(ALL_LIBS) $(EXTRALIBS)

binarize_set:	binarize_set.o $(LEPTLIB)
	$(CC) -o binarize_set binarize_set.o $(ALL_LIBS) $(EXTRALIBS)

bincompare:	bincompare.o $(LEPTLIB)
	$(CC) -o bincompare bincompare.o $(ALL_LIBS) $(EXTRALIBS)

blendcmaptest:	blendcmaptest.o $(LEPTLIB)
	$(CC) -o blendcmaptest blendcmaptest.o $(ALL_LIBS) $(EXTRALIBS)

buffertest:	buffertest.o $(LEPTLIB)
	$(CC) -o buffertest buffertest.o $(ALL_LIBS) $(EXTRALIBS)

byteatest:	byteatest.o $(LEPTLIB)
	$(CC) -o byteatest byteatest.o $(ALL_LIBS) $(EXTRALIBS)

ccbordtest:	ccbordtest.o $(LEPTLIB)
	$(CC) -o ccbordtest ccbordtest.o $(ALL_LIBS) $(EXTRALIBS)

cctest1:	cctest1.o $(LEPTLIB)
	$(CC) -o cctest1 cctest1.o $(ALL_LIBS) $(EXTRALIBS)

colormasktest:	colormasktest.o $(LEPTLIB)
	$(CC) -o colormasktest colormasktest.o $(ALL_LIBS) $(EXTRALIBS)

colormorphtest:	colormorphtest.o $(LEPTLIB)
	$(CC) -o colormorphtest colormorphtest.o $(ALL_LIBS) $(EXTRALIBS)

colorsegtest:	colorsegtest.o $(LEPTLIB)
	$(CC) -o colorsegtest colorsegtest.o $(ALL_LIBS) $(EXTRALIBS)

colorspacetest:	colorspacetest.o $(LEPTLIB)
	$(CC) -o colorspacetest colorspacetest.o $(ALL_LIBS) $(EXTRALIBS)

comparepages:	comparepages.o $(LEPTLIB)
	$(CC) -o comparepages comparepages.o $(ALL_LIBS) $(EXTRALIBS)

comparetest:	comparetest.o $(LEPTLIB)
	$(CC) -o comparetest comparetest.o $(ALL_LIBS) $(EXTRALIBS)

concatpdf:	concatpdf.o $(LEPTLIB)
	$(CC) -o concatpdf concatpdf.o $(ALL_LIBS) $(EXTRALIBS)

contrasttest:	contrasttest.o $(LEPTLIB)
	$(CC) -o contrasttest contrasttest.o $(ALL_LIBS) $(EXTRALIBS)

convertfilestopdf:	convertfilestopdf.o $(LEPTLIB)
	$(CC) -o convertfilestopdf convertfilestopdf.o $(ALL_LIBS) $(EXTRALIBS)

convertfilestops:	convertfilestops.o $(LEPTLIB)
	$(CC) -o convertfilestops convertfilestops.o $(ALL_LIBS) $(EXTRALIBS)

convertformat:	convertformat.o $(LEPTLIB)
	$(CC) -o convertformat convertformat.o $(ALL_LIBS) $(EXTRALIBS)

convertsegfilestopdf:	convertsegfilestopdf.o $(LEPTLIB)
	$(CC) -o convertsegfilestopdf convertsegfilestopdf.o $(ALL_LIBS) $(EXTRALIBS)

convertsegfilestops:	convertsegfilestops.o $(LEPTLIB)
	$(CC) -o convertsegfilestops convertsegfilestops.o $(ALL_LIBS) $(EXTRALIBS)

converttogray:	converttogray.o $(LEPTLIB)
	$(CC) -o converttogray converttogray.o $(ALL_LIBS) $(EXTRALIBS)

converttopdf:	converttopdf.o $(LEPTLIB)
	$(CC) -o converttopdf converttopdf.o $(ALL_LIBS) $(EXTRALIBS)

converttops:	converttops.o $(LEPTLIB)
	$(CC) -o converttops converttops.o $(ALL_LIBS) $(EXTRALIBS)

convolvetest:	convolvetest.o $(LEPTLIB)
	$(CC) -o convolvetest convolvetest.o $(ALL_LIBS) $(EXTRALIBS)

cornertest:	cornertest.o $(LEPTLIB)
	$(CC) -o cornertest cornertest.o $(ALL_LIBS) $(EXTRALIBS)

croptest:	croptest.o $(LEPTLIB)
	$(CC) -o croptest croptest.o $(ALL_LIBS) $(EXTRALIBS)

croptext:	croptext.o $(LEPTLIB)
	$(CC) -o croptext croptext.o $(ALL_LIBS) $(EXTRALIBS)

dewarprules:	dewarprules.o $(LEPTLIB)
	$(CC) -o dewarprules dewarprules.o $(ALL_LIBS) $(EXTRALIBS)

dewarptest1:	dewarptest1.o $(LEPTLIB)
	$(CC) -o dewarptest1 dewarptest1.o $(ALL_LIBS) $(EXTRALIBS)

dewarptest2:	dewarptest2.o $(LEPTLIB)
	$(CC) -o dewarptest2 dewarptest2.o $(ALL_LIBS) $(EXTRALIBS)

dewarptest3:	dewarptest3.o $(LEPTLIB)
	$(CC) -o dewarptest3 dewarptest3.o $(ALL_LIBS) $(EXTRALIBS)

dewarptest4:	dewarptest4.o $(LEPTLIB)
	$(CC) -o dewarptest4 dewarptest4.o $(ALL_LIBS) $(EXTRALIBS)

digitprep1:	digitprep1.o $(LEPTLIB)
	$(CC) -o digitprep1 digitprep1.o $(ALL_LIBS) $(EXTRALIBS)

displayboxa:	displayboxa.o $(LEPTLIB)
	$(CC) -o displayboxa displayboxa.o $(ALL_LIBS) $(EXTRALIBS)

displaypix:	displaypix.o $(LEPTLIB)
	$(CC) -o displaypix displaypix.o $(ALL_LIBS) $(EXTRALIBS)

displaypixa:	displaypixa.o $(LEPTLIB)
	$(CC) -o displaypixa displaypixa.o $(ALL_LIBS) $(EXTRALIBS)

dithertest:	dithertest.o $(LEPTLIB)
	$(CC) -o dithertest dithertest.o $(ALL_LIBS) $(EXTRALIBS)

dwalineargen:	dwalineargen.o $(LEPTLIB)
	$(CC) -o dwalineargen dwalineargen.o $(ALL_LIBS) $(EXTRALIBS)

edgetest:	edgetest.o $(LEPTLIB)
	$(CC) -o edgetest edgetest.o $(ALL_LIBS) $(EXTRALIBS)

falsecolortest:	falsecolortest.o $(LEPTLIB)
	$(CC) -o falsecolortest falsecolortest.o $(ALL_LIBS) $(EXTRALIBS)

fhmtautogen:	fhmtautogen.o $(LEPTLIB)
	$(CC) -o fhmtautogen fhmtautogen.o $(ALL_LIBS) $(EXTRALIBS)

fileinfo:	fileinfo.o $(LEPTLIB)
	$(CC) -o fileinfo fileinfo.o $(ALL_LIBS) $(EXTRALIBS)

findpattern1:	findpattern1.o $(LEPTLIB)
	$(CC) -o findpattern1 findpattern1.o $(ALL_LIBS) $(EXTRALIBS)

findpattern2:	findpattern2.o $(LEPTLIB)
	$(CC) -o findpattern2 findpattern2.o $(ALL_LIBS) $(EXTRALIBS)

findpattern3:	findpattern3.o $(LEPTLIB)
	$(CC) -o findpattern3 findpattern3.o $(ALL_LIBS) $(EXTRALIBS)

flipselgen:	flipselgen.o $(LEPTLIB)
	$(CC) -o flipselgen flipselgen.o $(ALL_LIBS) $(EXTRALIBS)

fcombautogen:	fcombautogen.o $(LEPTLIB)
	$(CC) -o fcombautogen fcombautogen.o $(ALL_LIBS) $(EXTRALIBS)

fmorphautogen:	fmorphautogen.o $(LEPTLIB)
	$(CC) -o fmorphautogen fmorphautogen.o $(ALL_LIBS) $(EXTRALIBS)

fpixcontours:	fpixcontours.o $(LEPTLIB)
	$(CC) -o fpixcontours fpixcontours.o $(ALL_LIBS) $(EXTRALIBS)

gammatest:	gammatest.o $(LEPTLIB)
	$(CC) -o gammatest gammatest.o $(ALL_LIBS) $(EXTRALIBS)

genfonts:	genfonts.o $(LEPTLIB)
	$(CC) -o genfonts genfonts.o $(ALL_LIBS) $(EXTRALIBS)

graphicstest:	graphicstest.o $(LEPTLIB)
	$(CC) -o graphicstest graphicstest.o $(ALL_LIBS) $(EXTRALIBS)

graymorphtest:	graymorphtest.o $(LEPTLIB)
	$(CC) -o graymorphtest graymorphtest.o $(ALL_LIBS) $(EXTRALIBS)

histotest:	histotest.o $(LEPTLIB)
	$(CC) -o histotest histotest.o $(ALL_LIBS) $(EXTRALIBS)

iotest:		iotest.o $(LEPTLIB)
	$(CC) -o iotest iotest.o $(ALL_LIBS) $(EXTRALIBS)

italictest:	italictest.o $(LEPTLIB)
	$(CC) -o italictest italictest.o $(ALL_LIBS) $(EXTRALIBS)

jbcorrelation:	jbcorrelation.o $(LEPTLIB)
	$(CC) -o jbcorrelation jbcorrelation.o $(ALL_LIBS) $(EXTRALIBS)

jbrankhaus:	jbrankhaus.o $(LEPTLIB)
	$(CC) -o jbrankhaus jbrankhaus.o $(ALL_LIBS) $(EXTRALIBS)

jbwords:	jbwords.o $(LEPTLIB)
	$(CC) -o jbwords jbwords.o $(ALL_LIBS) $(EXTRALIBS)

lineremoval:	lineremoval.o $(LEPTLIB)
	$(CC) -o lineremoval lineremoval.o $(ALL_LIBS) $(EXTRALIBS)

listtest:	listtest.o $(LEPTLIB)
	$(CC) -o listtest listtest.o $(ALL_LIBS) $(EXTRALIBS)

livre_adapt:	livre_adapt.o $(LEPTLIB)
	$(CC) -o livre_adapt livre_adapt.o $(ALL_LIBS) $(EXTRALIBS)

livre_hmt:	livre_hmt.o $(LEPTLIB)
	$(CC) -o livre_hmt livre_hmt.o $(ALL_LIBS) $(EXTRALIBS)

livre_makefigs:	livre_makefigs.o $(LEPTLIB)
	$(CC) -o livre_makefigs livre_makefigs.o $(ALL_LIBS) $(EXTRALIBS)

livre_orient:	livre_orient.o $(LEPTLIB)
	$(CC) -o livre_orient livre_orient.o $(ALL_LIBS) $(EXTRALIBS)

livre_pageseg:	livre_pageseg.o $(LEPTLIB)
	$(CC) -o livre_pageseg livre_pageseg.o $(ALL_LIBS) $(EXTRALIBS)

livre_seedgen:	livre_seedgen.o $(LEPTLIB)
	$(CC) -o livre_seedgen livre_seedgen.o $(ALL_LIBS) $(EXTRALIBS)

livre_tophat:	livre_tophat.o $(LEPTLIB)
	$(CC) -o livre_tophat livre_tophat.o $(ALL_LIBS) $(EXTRALIBS)

maketile:	maketile.o $(LEPTLIB)
	$(CC) -o maketile maketile.o $(ALL_LIBS) $(EXTRALIBS)

misctest1:	misctest1.o $(LEPTLIB)
	$(CC) -o misctest1 misctest1.o $(ALL_LIBS) $(EXTRALIBS)

modifyhuesat:	modifyhuesat.o $(LEPTLIB)
	$(CC) -o modifyhuesat modifyhuesat.o $(ALL_LIBS) $(EXTRALIBS)

morphtest1:	morphtest1.o $(LEPTLIB)
	$(CC) -o morphtest1 morphtest1.o $(ALL_LIBS) $(EXTRALIBS)

mtifftest:	mtifftest.o $(LEPTLIB)
	$(CC) -o mtifftest mtifftest.o $(ALL_LIBS) $(EXTRALIBS)

numaranktest:	numaranktest.o $(LEPTLIB)
	$(CC) -o numaranktest numaranktest.o $(ALL_LIBS) $(EXTRALIBS)

otsutest1:	otsutest1.o $(LEPTLIB)
	$(CC) -o otsutest1 otsutest1.o $(ALL_LIBS) $(EXTRALIBS)

otsutest2:	otsutest2.o $(LEPTLIB)
	$(CC) -o otsutest2 otsutest2.o $(ALL_LIBS) $(EXTRALIBS)

pagesegtest1:	pagesegtest1.o $(LEPTLIB)
	$(CC) -o pagesegtest1 pagesegtest1.o $(ALL_LIBS) $(EXTRALIBS)

pagesegtest2:	pagesegtest2.o $(LEPTLIB)
	$(CC) -o pagesegtest2 pagesegtest2.o $(ALL_LIBS) $(EXTRALIBS)

partitiontest:	partitiontest.o $(LEPTLIB)
	$(CC) -o partitiontest partitiontest.o $(ALL_LIBS) $(EXTRALIBS)

pdfiotest:	pdfiotest.o $(LEPTLIB)
	$(CC) -o pdfiotest pdfiotest.o $(ALL_LIBS) $(EXTRALIBS)

pixaatest:	pixaatest.o $(LEPTLIB)
	$(CC) -o pixaatest pixaatest.o $(ALL_LIBS) $(EXTRALIBS)

plottest:	plottest.o $(LEPTLIB)
	$(CC) -o plottest plottest.o $(ALL_LIBS) $(EXTRALIBS)

printimage:	printimage.o $(LEPTLIB)
	$(CC) -o printimage printimage.o $(ALL_LIBS) $(EXTRALIBS)

printsplitimage:	printsplitimage.o $(LEPTLIB)
	$(CC) -o printsplitimage printsplitimage.o $(ALL_LIBS) $(EXTRALIBS)

printtiff:	printtiff.o $(LEPTLIB)
	$(CC) -o printtiff printtiff.o $(ALL_LIBS) $(EXTRALIBS)

quadtreetest:	quadtreetest.o $(LEPTLIB)
	$(CC) -o quadtreetest quadtreetest.o $(ALL_LIBS) $(EXTRALIBS)

ranktest:	ranktest.o $(LEPTLIB)
	$(CC) -o ranktest ranktest.o $(ALL_LIBS) $(EXTRALIBS)

recogsort:	recogsort.o $(LEPTLIB)
	$(CC) -o recogsort recogsort.o $(ALL_LIBS) $(EXTRALIBS)

recogtest1:	recogtest1.o $(LEPTLIB)
	$(CC) -o recogtest1 recogtest1.o $(ALL_LIBS) $(EXTRALIBS)

reducetest:	reducetest.o $(LEPTLIB)
	$(CC) -o reducetest reducetest.o $(ALL_LIBS) $(EXTRALIBS)

removecmap:	removecmap.o $(LEPTLIB)
	$(CC) -o removecmap removecmap.o $(ALL_LIBS) $(EXTRALIBS)

renderfonts:	renderfonts.o $(LEPTLIB)
	$(CC) -o renderfonts renderfonts.o $(ALL_LIBS) $(EXTRALIBS)

rotatefastalt:	rotatefastalt.o $(LEPTLIB)
	$(CC) -o rotatefastalt rotatefastalt.o $(ALL_LIBS) $(EXTRALIBS)

rotateorthtest1:	rotateorthtest1.o $(LEPTLIB)
	$(CC) -o rotateorthtest1 rotateorthtest1.o $(ALL_LIBS) $(EXTRALIBS)

rotatetest1:	rotatetest1.o $(LEPTLIB)
	$(CC) -o rotatetest1 rotatetest1.o $(ALL_LIBS) $(EXTRALIBS)

runlengthtest:	runlengthtest.o $(LEPTLIB)
	$(CC) -o runlengthtest runlengthtest.o $(ALL_LIBS) $(EXTRALIBS)

scaleandtile:	scaleandtile.o $(LEPTLIB)
	$(CC) -o scaleandtile scaleandtile.o $(ALL_LIBS) $(EXTRALIBS)

scaletest1:	scaletest1.o $(LEPTLIB)
	$(CC) -o scaletest1 scaletest1.o $(ALL_LIBS) $(EXTRALIBS)

scaletest2:	scaletest2.o $(LEPTLIB)
	$(CC) -o scaletest2 scaletest2.o $(ALL_LIBS) $(EXTRALIBS)

seedfilltest:	seedfilltest.o $(LEPTLIB)
	$(CC) -o seedfilltest seedfilltest.o $(ALL_LIBS) $(EXTRALIBS)

sharptest:	sharptest.o $(LEPTLIB)
	$(CC) -o sharptest sharptest.o $(ALL_LIBS) $(EXTRALIBS)

sheartest:	sheartest.o $(LEPTLIB)
	$(CC) -o sheartest sheartest.o $(ALL_LIBS) $(EXTRALIBS)

showedges:	showedges.o $(LEPTLIB)
	$(CC) -o showedges showedges.o $(ALL_LIBS) $(EXTRALIBS)

skewtest:	skewtest.o $(LEPTLIB)
	$(CC) -o skewtest skewtest.o $(ALL_LIBS) $(EXTRALIBS)

snapcolortest:	snapcolortest.o $(LEPTLIB)
	$(CC) -o snapcolortest snapcolortest.o $(ALL_LIBS) $(EXTRALIBS)

sorttest:	sorttest.o $(LEPTLIB)
	$(CC) -o sorttest sorttest.o $(ALL_LIBS) $(EXTRALIBS)

splitimage2pdf:	splitimage2pdf.o $(LEPTLIB)
	$(CC) -o splitimage2pdf splitimage2pdf.o $(ALL_LIBS) $(EXTRALIBS)

sudokutest:	sudokutest.o $(LEPTLIB)
	$(CC) -o sudokutest sudokutest.o $(ALL_LIBS) $(EXTRALIBS)

trctest:	trctest.o $(LEPTLIB)
	$(CC) -o trctest trctest.o $(ALL_LIBS) $(EXTRALIBS)

warpertest:	warpertest.o $(LEPTLIB)
	$(CC) -o warpertest warpertest.o $(ALL_LIBS) $(EXTRALIBS)

viewertest:	viewertest.o $(LEPTLIB)
	$(CC) -o viewertest viewertest.o $(ALL_LIBS) $(EXTRALIBS)

watershedtest:	watershedtest.o $(LEPTLIB)
	$(CC) -o watershedtest watershedtest.o $(ALL_LIBS) $(EXTRALIBS)

wordsinorder:	wordsinorder.o $(LEPTLIB)
	$(CC) -o wordsinorder wordsinorder.o $(ALL_LIBS) $(EXTRALIBS)

writemtiff:	writemtiff.o $(LEPTLIB)
	$(CC) -o writemtiff writemtiff.o $(ALL_LIBS) $(EXTRALIBS)

xtractprotos:   xtractprotos.o $(LEPTLIB)
	$(CC) -o xtractprotos xtractprotos.o $(ALL_LIBS)
	cp xtractprotos ../src

yuvtest:	yuvtest.o $(LEPTLIB)
	$(CC) -o yuvtest yuvtest.o $(ALL_LIBS) $(EXTRALIBS)

###########################################################

clean:
	-@ for file in $(SRC:%.c=%) ; do \
		rm -f $$file; \
	done ;
	-@ for file in $(SRC:%.c=%.o) ; do \
		rm -f $$file; \
	done ;
	-@ for file in $(SRC2:%.c=%.o) ; do \
		rm -f $$file; \
	done ;

###########################################################
 
install:
	mkdir -p $(DESTDIR)/usr/bin/leptonica
	-@ for file in $(SRC:%.c=%) ; do \
		install $$file $(DESTDIR)/usr/bin/leptonica; \
	done ;

###########################################################

depend:
	$(BIN)/makedepend -DNO_PROTOS $(CPPFLAGS) $(SRC)

dependprotos:
	$(BIN)/makedepend $(CPPFLAGS) $(SRC)

# DO NOT DELETE THIS LINE -- make depend depends on it.
