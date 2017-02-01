AUTOMAKE_OPTIONS = parallel-tests
AM_CFLAGS = $(DEBUG_FLAGS)
AM_CPPFLAGS = -I$(top_srcdir)/src -I$(top_builddir)/src
LDADD = $(top_builddir)/src/liblept.la $(LIBM)
 
INSTALL_PROGS = convertfilestopdf convertfilestops \
	convertformat \
	convertsegfilestopdf convertsegfilestops \
	converttopdf converttops fileinfo \
	printimage printsplitimage printtiff \
	splitimage2pdf xtractprotos

AUTO_REG_PROGS = adaptmap_reg affine_reg alphaops_reg alphaxform_reg \
	bilateral2_reg bilinear_reg binarize_reg blackwhite_reg \
	blend1_reg blend2_reg blend3_reg blend4_reg \
	ccthin1_reg ccthin2_reg cmapquant_reg \
	colorcontent_reg coloring_reg \
	colorize_reg colormask_reg \
	colormorph_reg colorquant_reg \
	colorseg_reg colorspace_reg compare_reg compfilter_reg \
	conncomp_reg convolve_reg dewarp_reg distance_reg \
	dither_reg dna_reg dwamorph1_reg edge_reg enhance_reg \
	expand_reg findcorners_reg findpattern_reg \
	fpix1_reg fpix2_reg genfonts_reg \
	graymorph1_reg graymorph2_reg \
	grayquant_reg hardlight_reg \
	insert_reg ioformats_reg \
	jbclass_reg jpegio_reg \
	kernel_reg label_reg lineremoval_reg \
	logicops_reg maze_reg multitype_reg \
	nearline_reg newspaper_reg \
	overlap_reg pageseg_reg paint_reg paintmask_reg \
	pdfseg_reg pixa2_reg pixadisp_reg \
	pixserial_reg pngio_reg pnmio_reg \
	projection_reg projective_reg \
	psio_reg psioseg_reg \
	pta_reg rankbin_reg rankhisto_reg \
	rank_reg rasteropip_reg \
	rotate1_reg rotate2_reg rotateorth_reg \
	scale_reg seedspread_reg \
	selio_reg shear1_reg shear2_reg \
	skew_reg splitcomp_reg subpixel_reg \
	texturefill_reg threshnorm_reg translate_reg \
	warper_reg writetext_reg xformbox_reg

if HAVE_LIBGIF
AUTO_REG_PROGS += gifio_reg
endif

if HAVE_LIBWEBP
AUTO_REG_PROGS += webpio_reg
endif

if HAVE_LIBJP2K
AUTO_REG_PROGS += jp2kio_reg
endif

MANUAL_REG_PROGS = alltests_reg adaptnorm_reg bilateral1_reg \
	binmorph1_reg binmorph2_reg binmorph3_reg \
	binmorph4_reg binmorph5_reg \
	boxa1_reg boxa2_reg \
	colorize_reg conversion_reg \
	dwamorph2_reg equal_reg extrema_reg \
	fhmtauto_reg files_reg flipdetect_reg fmorphauto_reg \
	grayfill_reg heap_reg locminmax_reg \
	lowaccess_reg morphseq_reg \
	numa1_reg numa2_reg pixa1_reg \
	pixalloc_reg pixcomp_reg \
	pixmem_reg pixtile_reg \
	ptra1_reg ptra2_reg \
	rasterop_reg smallpix_reg \
	smoothedge_reg string_reg \
	wordboxes_reg

OTHER_PROGS = adaptmap_dark \
	arabic_lines arithtest \
	autogentest1 autogentest2 \
	barcodetest baselinetest \
	binarizefiles binarize_set bincompare \
	blendcmaptest buffertest \
	byteatest ccbordtest cctest1 cleanpdf \
	colorsegtest comparepages comparepixa \
	comparetest concatpdf \
	contrasttest converttogray \
	cornertest croptest croptext \
	dewarprules dewarptest1 dewarptest2 \
	dewarptest3 dewarptest4 dewarptest5 \
	digitprep1 displayboxa displayboxes_on_pixa \
	displaypix displaypixa \
	dwalineargen falsecolortest \
	fcombautogen fhmtautogen findbinding \
	findpattern1 findpattern2 findpattern3 \
	flipselgen fmorphautogen \
	fpixcontours gammatest \
	gifio_leaktest graphicstest graymorphtest \
	hashtest histotest htmlviewer \
	iotest italictest \
	jbcorrelation jbrankhaus jbwords \
	listtest livre_adapt livre_hmt \
	livre_makefigs livre_orient \
	livre_pageseg livre_seedgen livre_tophat \
	maketile maptest misctest1 \
	modifyhuesat morphtest1 mtifftest \
	numaranktest otsutest1 otsutest2 \
	pagesegtest1 pagesegtest2 \
	partitiontest pdfiotest percolatetest \
	pixaatest pixafileinfo plottest \
	quadtreetest rbtreetest \
	recog_bootnum recogsort recogtest1 \
	reducetest removecmap \
	renderfonts rotatefastalt \
	rotateorthtest1 rotatetest1 \
	runlengthtest scaleandtile \
	scaletest1 scaletest2 \
	seedfilltest settest sharptest \
	sheartest showedges \
	skewtest snapcolortest \
	sorttest sudokutest trctest \
	warpertest watershedtest wordsinorder \
	writemtiff yuvtest

if ENABLE_PROGRAMS
bin_PROGRAMS = $(INSTALL_PROGS)
noinst_PROGRAMS = $(AUTO_REG_PROGS) $(MANUAL_REG_PROGS) $(OTHER_PROGS)
endif

check_PROGRAMS = $(AUTO_REG_PROGS)
TESTS = $(AUTO_REG_PROGS)
TESTS_ENVIRONMENT = $(srcdir)/reg_wrapper.sh
EXTRA_DIST = reg_wrapper.sh

dwamorph1_reg_SOURCES = dwamorph1_reg.c dwalinear.3.c dwalinearlow.3.c
dwamorph2_reg_SOURCES = dwamorph2_reg.c dwalinear.3.c dwalinearlow.3.c

autogentest2_SOURCES = autogentest2.c autogen.137.c
