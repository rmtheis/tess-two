AM_CFLAGS = $(DEBUG_FLAGS)
AM_CPPFLAGS = -I$(top_srcdir)/src
LDADD = $(top_builddir)/src/liblept.la $(LIBM)

bin_PROGRAMS = adaptnorm_reg affine_reg \
	alltests_reg alphaops_reg \
	alphaxform_reg bilinear_reg binarize_reg \
	binmorph1_reg binmorph2_reg \
	binmorph3_reg binmorph4_reg binmorph5_reg \
	blend_reg blend2_reg \
	ccthin1_reg ccthin2_reg \
	cmapquant_reg coloring_reg \
	colormask_reg colorquant_reg \
	colorseg_reg compare_reg compfilter_reg \
	conncomp_reg conversion_reg convolve_reg \
	dewarp_reg distance_reg dna_reg \
	dwamorph1_reg dwamorph2_reg \
	enhance_reg equal_reg \
	expand_reg extrema_reg \
	fhmtauto_reg findpattern_reg \
	flipdetect_reg fmorphauto_reg \
	fpix_reg gifio_reg \
	grayfill_reg graymorph1_reg \
	graymorph2_reg grayquant_reg \
	hardlight_reg heap_reg ioformats_reg \
	kernel_reg locminmax_reg \
	logicops_reg lowaccess_reg \
	maze_reg morphseq_reg numa_reg \
	overlap_reg paint_reg paintmask_reg \
	pdfseg_reg pixa1_reg pixa2_reg \
	pixadisp_reg pixalloc_reg \
	pixcomp_reg pixmem_reg \
	pixserial_reg pixtile_reg \
	pngio_reg \
	projection_reg projective_reg \
	psio_reg psioseg_reg \
	pta_reg ptra1_reg ptra2_reg \
	rank_reg rankbin_reg rankhisto_reg \
	rasterop_reg rasteropip_reg \
	rotate1_reg rotate2_reg rotateorth_reg \
	scale_reg seedspread_reg selio_reg \
	shear_reg shear2_reg skew_reg \
	smallpix_reg smoothedge_reg splitcomp_reg \
	string_reg subpixel_reg \
	threshnorm_reg translate_reg \
	warper_reg writetext_reg xformbox_reg \
	adaptmaptest arithtest \
	barcodetest baselinetest \
	bincompare blendcmaptest \
	blendtest1 buffertest byteatest \
	ccbordtest cctest1 \
	colormorphtest colorsegtest \
	colorspacetest comparepages \
	comparetest contrasttest \
	convertfilestopdf convertfilestops \
	convertformat \
	convertsegfilestopdf convertsegfilestops \
	converttogray converttops \
	convolvetest cornertest \
	croptext dewarptest1 dewarptest2 dewarptest3 \
	digitprep1 dithertest \
	dwalineargen edgetest falsecolortest \
	fcombautogen fhmtautogen fileinfo \
	findpattern1 findpattern2 findpattern3 \
	flipselgen fmorphautogen \
	fpixcontours gammatest \
	genfonts graphicstest graymorphtest \
	histotest iotest inserttest \
	jbcorrelation jbrankhaus jbwords \
	lineremoval listtest \
	livre_adapt livre_hmt \
	livre_makefigs livre_orient \
	livre_pageseg livre_seedgen livre_tophat \
	maketile misctest1 \
	modifyhuesat morphtest1 mtifftest \
	numaranktest otsutest1 otsutest2 \
	pagesegtest1 pagesegtest2 \
	partitiontest pdfiotest \
	pixaatest plottest \
	printimage printsplitimage printtiff \
	quadtreetest \
	ranktest reducetest \
	removecmap renderfonts \
	rotatefastalt rotateorthtest1 \
	rotatetest1 runlengthtest \
	scaleandtile scaletest1 scaletest2 \
	seedfilltest sharptest \
	sheartest showedges \
	skewtest snapcolortest \
	sorttest splitimage2pdf \
	sudokutest textlinemask trctest	\
	viewertest warpertest watershedtest \
	wordsinorder writemtiff \
	xtractprotos xvdisp yuvtest

dwamorph1_reg_SOURCES = dwamorph1_reg.c dwalinear.3.c dwalinearlow.3.c
dwamorph2_reg_SOURCES = dwamorph2_reg.c dwalinear.3.c dwalinearlow.3.c
