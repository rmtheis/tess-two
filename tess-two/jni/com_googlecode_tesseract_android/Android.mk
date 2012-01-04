REAL_LOCAL_PATH := $(call my-dir)
LOCAL_PATH := $(call my-dir)/../..

include $(CLEAR_VARS)

LOCAL_MODULE := libtess

# scrollview_lib

LOCAL_SRC_FILES := \
  $(TESSERACT_PATH)/viewer/scrollview.cpp \
  $(TESSERACT_PATH)/viewer/svutil.cpp \
  $(TESSERACT_PATH)/viewer/svmnode.cpp \
  $(TESSERACT_PATH)/viewer/svpaint.cpp

LOCAL_C_INCLUDES := \
  $(LEPTONICA_PATH)/src

LOCAL_CFLAGS := \
  -DHAVE_LIBLEPT

# tesseract_ccutil

LOCAL_SRC_FILES += \
  $(TESSERACT_PATH)/ccutil/ambigs.cpp \
  $(TESSERACT_PATH)/ccutil/basedir.cpp \
  $(TESSERACT_PATH)/ccutil/bits16.cpp \
  $(TESSERACT_PATH)/ccutil/boxread.cpp \
  $(TESSERACT_PATH)/ccutil/clst.cpp \
  $(TESSERACT_PATH)/ccutil/elst2.cpp \
  $(TESSERACT_PATH)/ccutil/elst.cpp \
  $(TESSERACT_PATH)/ccutil/errcode.cpp \
  $(TESSERACT_PATH)/ccutil/globaloc.cpp \
  $(TESSERACT_PATH)/ccutil/hashfn.cpp \
  $(TESSERACT_PATH)/ccutil/mainblk.cpp \
  $(TESSERACT_PATH)/ccutil/memblk.cpp \
  $(TESSERACT_PATH)/ccutil/memry.cpp \
  $(TESSERACT_PATH)/ccutil/serialis.cpp \
  $(TESSERACT_PATH)/ccutil/strngs.cpp \
  $(TESSERACT_PATH)/ccutil/tessdatamanager.cpp \
  $(TESSERACT_PATH)/ccutil/tprintf.cpp \
  $(TESSERACT_PATH)/ccutil/unichar.cpp \
  $(TESSERACT_PATH)/ccutil/unicharset.cpp \
  $(TESSERACT_PATH)/ccutil/unicharmap.cpp \
  $(TESSERACT_PATH)/ccutil/ccutil.cpp \
  $(TESSERACT_PATH)/ccutil/params.cpp

LOCAL_C_INCLUDES += \
  $(TESSERACT_PATH)/ccutil

# tesseract_cutil

LOCAL_SRC_FILES += \
  $(TESSERACT_PATH)/cutil/tessarray.cpp \
  $(TESSERACT_PATH)/cutil/bitvec.cpp \
  $(TESSERACT_PATH)/cutil/callcpp.cpp \
  $(TESSERACT_PATH)/cutil/danerror.cpp \
  $(TESSERACT_PATH)/cutil/efio.cpp \
  $(TESSERACT_PATH)/cutil/emalloc.cpp \
  $(TESSERACT_PATH)/cutil/freelist.cpp \
  $(TESSERACT_PATH)/cutil/listio.cpp \
  $(TESSERACT_PATH)/cutil/oldheap.cpp \
  $(TESSERACT_PATH)/cutil/oldlist.cpp \
  $(TESSERACT_PATH)/cutil/structures.cpp \
  $(TESSERACT_PATH)/cutil/cutil.cpp \
  $(TESSERACT_PATH)/cutil/cutil_class.cpp

LOCAL_C_INCLUDES += \
  $(TESSERACT_PATH)/cutil \
  $(TESSERACT_PATH)/ccutil \

# tesseract_dict

LOCAL_SRC_FILES += \
  $(TESSERACT_PATH)/dict/context.cpp \
  $(TESSERACT_PATH)/dict/dawg.cpp \
  $(TESSERACT_PATH)/dict/dict.cpp \
  $(TESSERACT_PATH)/dict/hyphen.cpp \
  $(TESSERACT_PATH)/dict/permdawg.cpp \
  $(TESSERACT_PATH)/dict/permute.cpp \
  $(TESSERACT_PATH)/dict/states.cpp \
  $(TESSERACT_PATH)/dict/stopper.cpp \
  $(TESSERACT_PATH)/dict/trie.cpp

LOCAL_C_INCLUDES += \
  $(TESSERACT_PATH)/cutil \
  $(TESSERACT_PATH)/ccutil \
  $(TESSERACT_PATH)/ccstruct \
  $(TESSERACT_PATH)/dict \

# tesseract_image

LOCAL_SRC_FILES += \
  $(TESSERACT_PATH)/image/image.cpp \
  $(TESSERACT_PATH)/image/imgs.cpp \
  $(TESSERACT_PATH)/image/imgtiff.cpp \
  $(TESSERACT_PATH)/image/svshowim.cpp
  
LOCAL_C_INCLUDES += \
  $(TESSERACT_PATH)/ccutil \
  $(TESSERACT_PATH)/image \
  $(TESSERACT_PATH)/viewer \
  $(LEPTONICA_PATH)/src

LOCAL_CFLAGS += \
  -DHAVE_LIBLEPT

# tesseract_cc_struct

LOCAL_SRC_FILES += \
  $(TESSERACT_PATH)/ccstruct/blobbox.cpp \
  $(TESSERACT_PATH)/ccstruct/blobs.cpp \
  $(TESSERACT_PATH)/ccstruct/blread.cpp \
  $(TESSERACT_PATH)/ccstruct/boxword.cpp \
  $(TESSERACT_PATH)/ccstruct/ccstruct.cpp \
  $(TESSERACT_PATH)/ccstruct/coutln.cpp \
  $(TESSERACT_PATH)/ccstruct/detlinefit.cpp \
  $(TESSERACT_PATH)/ccstruct/dppoint.cpp \
  $(TESSERACT_PATH)/ccstruct/genblob.cpp \
  $(TESSERACT_PATH)/ccstruct/linlsq.cpp \
  $(TESSERACT_PATH)/ccstruct/matrix.cpp \
  $(TESSERACT_PATH)/ccstruct/mod128.cpp \
  $(TESSERACT_PATH)/ccstruct/normalis.cpp \
  $(TESSERACT_PATH)/ccstruct/ocrblock.cpp \
  $(TESSERACT_PATH)/ccstruct/ocrrow.cpp \
  $(TESSERACT_PATH)/ccstruct/otsuthr.cpp \
  $(TESSERACT_PATH)/ccstruct/pageres.cpp \
  $(TESSERACT_PATH)/ccstruct/pdblock.cpp \
  $(TESSERACT_PATH)/ccstruct/points.cpp \
  $(TESSERACT_PATH)/ccstruct/polyaprx.cpp \
  $(TESSERACT_PATH)/ccstruct/polyblk.cpp \
  $(TESSERACT_PATH)/ccstruct/publictypes.cpp \
  $(TESSERACT_PATH)/ccstruct/quadlsq.cpp \
  $(TESSERACT_PATH)/ccstruct/quadratc.cpp \
  $(TESSERACT_PATH)/ccstruct/quspline.cpp \
  $(TESSERACT_PATH)/ccstruct/ratngs.cpp \
  $(TESSERACT_PATH)/ccstruct/rect.cpp \
  $(TESSERACT_PATH)/ccstruct/rejctmap.cpp \
  $(TESSERACT_PATH)/ccstruct/seam.cpp \
  $(TESSERACT_PATH)/ccstruct/split.cpp \
  $(TESSERACT_PATH)/ccstruct/statistc.cpp \
  $(TESSERACT_PATH)/ccstruct/stepblob.cpp \
  $(TESSERACT_PATH)/ccstruct/vecfuncs.cpp \
  $(TESSERACT_PATH)/ccstruct/werd.cpp

LOCAL_C_INCLUDES += \
  $(TESSERACT_PATH)/api \
  $(TESSERACT_PATH)/ccstruct \
  $(TESSERACT_PATH)/ccutil \
  $(TESSERACT_PATH)/cutil \
  $(TESSERACT_PATH)/image \
  $(TESSERACT_PATH)/viewer \
  $(LEPTONICA_PATH)/src

# tesseract_classify

LOCAL_SRC_FILES += \
  $(TESSERACT_PATH)/classify/adaptive.cpp \
  $(TESSERACT_PATH)/classify/adaptmatch.cpp \
  $(TESSERACT_PATH)/classify/blobclass.cpp \
  $(TESSERACT_PATH)/classify/chartoname.cpp \
  $(TESSERACT_PATH)/classify/classify.cpp \
  $(TESSERACT_PATH)/classify/cluster.cpp \
  $(TESSERACT_PATH)/classify/clusttool.cpp \
  $(TESSERACT_PATH)/classify/cutoffs.cpp \
  $(TESSERACT_PATH)/classify/extract.cpp \
  $(TESSERACT_PATH)/classify/featdefs.cpp \
  $(TESSERACT_PATH)/classify/flexfx.cpp \
  $(TESSERACT_PATH)/classify/float2int.cpp \
  $(TESSERACT_PATH)/classify/fpoint.cpp \
  $(TESSERACT_PATH)/classify/fxdefs.cpp \
  $(TESSERACT_PATH)/classify/intfx.cpp \
  $(TESSERACT_PATH)/classify/intmatcher.cpp \
  $(TESSERACT_PATH)/classify/intproto.cpp \
  $(TESSERACT_PATH)/classify/kdtree.cpp \
  $(TESSERACT_PATH)/classify/mf.cpp \
  $(TESSERACT_PATH)/classify/mfdefs.cpp \
  $(TESSERACT_PATH)/classify/mfoutline.cpp \
  $(TESSERACT_PATH)/classify/mfx.cpp \
  $(TESSERACT_PATH)/classify/normfeat.cpp \
  $(TESSERACT_PATH)/classify/normmatch.cpp \
  $(TESSERACT_PATH)/classify/ocrfeatures.cpp \
  $(TESSERACT_PATH)/classify/outfeat.cpp \
  $(TESSERACT_PATH)/classify/picofeat.cpp \
  $(TESSERACT_PATH)/classify/protos.cpp \
  $(TESSERACT_PATH)/classify/speckle.cpp \
  $(TESSERACT_PATH)/classify/xform2d.cpp

LOCAL_C_INCLUDES += \
  $(TESSERACT_PATH)/cutil \
  $(TESSERACT_PATH)/classify \
  $(TESSERACT_PATH)/ccutil \
  $(TESSERACT_PATH)/ccstruct \
  $(TESSERACT_PATH)/dict

# tesseract_textord

LOCAL_SRC_FILES += \
  $(TESSERACT_PATH)/textord/alignedblob.cpp \
  $(TESSERACT_PATH)/textord/blkocc.cpp \
  $(TESSERACT_PATH)/textord/bbgrid.cpp \
  $(TESSERACT_PATH)/textord/colfind.cpp \
  $(TESSERACT_PATH)/textord/colpartition.cpp \
  $(TESSERACT_PATH)/textord/colpartitionset.cpp \
  $(TESSERACT_PATH)/textord/colpartitiongrid.cpp \
  $(TESSERACT_PATH)/textord/devanagari_processing.cpp \
  $(TESSERACT_PATH)/textord/drawedg.cpp \
  $(TESSERACT_PATH)/textord/drawtord.cpp \
  $(TESSERACT_PATH)/textord/edgblob.cpp \
  $(TESSERACT_PATH)/textord/edgloop.cpp \
  $(TESSERACT_PATH)/textord/fpchop.cpp \
  $(TESSERACT_PATH)/textord/gap_map.cpp \
  $(TESSERACT_PATH)/textord/imagefind.cpp \
  $(TESSERACT_PATH)/textord/linefind.cpp \
  $(TESSERACT_PATH)/textord/makerow.cpp \
  $(TESSERACT_PATH)/textord/oldbasel.cpp \
  $(TESSERACT_PATH)/textord/pithsync.cpp \
  $(TESSERACT_PATH)/textord/pitsync1.cpp \
  $(TESSERACT_PATH)/textord/scanedg.cpp \
  $(TESSERACT_PATH)/textord/sortflts.cpp \
  $(TESSERACT_PATH)/textord/strokewidth.cpp \
  $(TESSERACT_PATH)/textord/tabfind.cpp \
  $(TESSERACT_PATH)/textord/tablefind.cpp \
  $(TESSERACT_PATH)/textord/tabvector.cpp \
  $(TESSERACT_PATH)/textord/tablerecog.cpp \
  $(TESSERACT_PATH)/textord/textord.cpp \
  $(TESSERACT_PATH)/textord/topitch.cpp \
  $(TESSERACT_PATH)/textord/tordmain.cpp \
  $(TESSERACT_PATH)/textord/tospace.cpp \
  $(TESSERACT_PATH)/textord/tovars.cpp \
  $(TESSERACT_PATH)/textord/underlin.cpp \
  $(TESSERACT_PATH)/textord/wordseg.cpp \
  $(TESSERACT_PATH)/textord/workingpartset.cpp

LOCAL_C_INCLUDES += \
  $(TESSERACT_PATH)/ccstruct \
  $(TESSERACT_PATH)/ccutil \
  $(TESSERACT_PATH)/image \
  $(TESSERACT_PATH)/viewer \
  $(TESSERACT_PATH)/textord

LOCAL_CFLAGS += \
  -DHAVE_LIBLEPT

# tesseract_wordrec

LOCAL_SRC_FILES += \
  $(TESSERACT_PATH)/wordrec/associate.cpp \
  $(TESSERACT_PATH)/wordrec/bestfirst.cpp \
  $(TESSERACT_PATH)/wordrec/chop.cpp \
  $(TESSERACT_PATH)/wordrec/chopper.cpp \
  $(TESSERACT_PATH)/wordrec/closed.cpp \
  $(TESSERACT_PATH)/wordrec/drawfx.cpp \
  $(TESSERACT_PATH)/wordrec/findseam.cpp \
  $(TESSERACT_PATH)/wordrec/gradechop.cpp \
  $(TESSERACT_PATH)/wordrec/heuristic.cpp \
  $(TESSERACT_PATH)/wordrec/language_model.cpp \
  $(TESSERACT_PATH)/wordrec/makechop.cpp \
  $(TESSERACT_PATH)/wordrec/matchtab.cpp \
  $(TESSERACT_PATH)/wordrec/olutil.cpp \
  $(TESSERACT_PATH)/wordrec/outlines.cpp \
  $(TESSERACT_PATH)/wordrec/pieces.cpp \
  $(TESSERACT_PATH)/wordrec/plotedges.cpp \
  $(TESSERACT_PATH)/wordrec/plotseg.cpp \
  $(TESSERACT_PATH)/wordrec/render.cpp \
  $(TESSERACT_PATH)/wordrec/segsearch.cpp \
  $(TESSERACT_PATH)/wordrec/tally.cpp \
  $(TESSERACT_PATH)/wordrec/tface.cpp \
  $(TESSERACT_PATH)/wordrec/wordclass.cpp \
  $(TESSERACT_PATH)/wordrec/wordrec.cpp

LOCAL_C_INCLUDES += \
  $(TESSERACT_PATH)/ccstruct \
  $(TESSERACT_PATH)/ccutil \
  $(TESSERACT_PATH)/cutil \
  $(TESSERACT_PATH)/cstruct \
  $(TESSERACT_PATH)/classify \
  $(TESSERACT_PATH)/image \
  $(TESSERACT_PATH)/dict \
  $(TESSERACT_PATH)/viewer

# tesseract_neural_networks_runtime

LOCAL_SRC_FILES += \
  $(TESSERACT_PATH)/neural_networks/runtime/input_file_buffer.cpp \
  $(TESSERACT_PATH)/neural_networks/runtime/neural_net.cpp \
  $(TESSERACT_PATH)/neural_networks/runtime/neuron.cpp \
  $(TESSERACT_PATH)/neural_networks/runtime/sigmoid_table.cpp

LOCAL_C_INCLUDES += \
  $(TESSERACT_PATH)/ccstruct \
  $(TESSERACT_PATH)/ccutil \
  $(TESSERACT_PATH)/cutil \
  $(TESSERACT_PATH)/image \
  $(TESSERACT_PATH)/dict \
  $(TESSERACT_PATH)/viewer

LOCAL_CFLAGS += \
  -DUSE_STD_NAMESPACE

# tesseract_cube

LOCAL_SRC_FILES += \
  $(TESSERACT_PATH)/cube/altlist.cpp \
  $(TESSERACT_PATH)/cube/beam_search.cpp \
  $(TESSERACT_PATH)/cube/bmp_8.cpp \
  $(TESSERACT_PATH)/cube/cached_file.cpp \
  $(TESSERACT_PATH)/cube/char_altlist.cpp \
  $(TESSERACT_PATH)/cube/char_bigrams.cpp \
  $(TESSERACT_PATH)/cube/char_samp.cpp \
  $(TESSERACT_PATH)/cube/char_samp_enum.cpp \
  $(TESSERACT_PATH)/cube/char_samp_set.cpp \
  $(TESSERACT_PATH)/cube/char_set.cpp \
  $(TESSERACT_PATH)/cube/classifier_factory.cpp \
  $(TESSERACT_PATH)/cube/con_comp.cpp \
  $(TESSERACT_PATH)/cube/conv_net_classifier.cpp \
  $(TESSERACT_PATH)/cube/cube_line_object.cpp \
  $(TESSERACT_PATH)/cube/cube_line_segmenter.cpp \
  $(TESSERACT_PATH)/cube/cube_object.cpp \
  $(TESSERACT_PATH)/cube/cube_search_object.cpp \
  $(TESSERACT_PATH)/cube/cube_tuning_params.cpp \
  $(TESSERACT_PATH)/cube/cube_utils.cpp \
  $(TESSERACT_PATH)/cube/feature_bmp.cpp \
  $(TESSERACT_PATH)/cube/feature_chebyshev.cpp \
  $(TESSERACT_PATH)/cube/feature_hybrid.cpp \
  $(TESSERACT_PATH)/cube/hybrid_neural_net_classifier.cpp \
  $(TESSERACT_PATH)/cube/search_column.cpp \
  $(TESSERACT_PATH)/cube/search_node.cpp \
  $(TESSERACT_PATH)/cube/tess_lang_mod_edge.cpp \
  $(TESSERACT_PATH)/cube/tess_lang_model.cpp \
  $(TESSERACT_PATH)/cube/word_altlist.cpp \
  $(TESSERACT_PATH)/cube/word_list_lang_model.cpp \
  $(TESSERACT_PATH)/cube/word_size_model.cpp \
  $(TESSERACT_PATH)/cube/word_unigrams.cpp

LOCAL_C_INCLUDES += \
  $(TESSERACT_PATH)/cutil \
  $(TESSERACT_PATH)/ccutil \
  $(TESSERACT_PATH)/ccstruct \
  $(TESSERACT_PATH)/dict \
  $(TESSERACT_PATH)/ccmain \
  $(TESSERACT_PATH)/classify \
  $(TESSERACT_PATH)/textord \
  $(TESSERACT_PATH)/wordrec \
  $(TESSERACT_PATH)/neural_networks/runtime \
  $(TESSERACT_PATH)/image \
  $(TESSERACT_PATH)/viewer

# tesseract_main

LOCAL_SRC_FILES += \
  $(TESSERACT_PATH)/ccmain/tessedit.cpp \
  $(TESSERACT_PATH)/ccmain/cube_control.cpp \
  $(TESSERACT_PATH)/ccmain/adaptions.cpp \
  $(TESSERACT_PATH)/ccmain/applybox.cpp \
  $(TESSERACT_PATH)/ccmain/control.cpp \
  $(TESSERACT_PATH)/ccmain/cube_reco_context.cpp \
  $(TESSERACT_PATH)/ccmain/docqual.cpp \
  $(TESSERACT_PATH)/ccmain/fixspace.cpp \
  $(TESSERACT_PATH)/ccmain/fixxht.cpp \
  $(TESSERACT_PATH)/ccmain/imgscale.cpp \
  $(TESSERACT_PATH)/ccmain/osdetect.cpp \
  $(TESSERACT_PATH)/ccmain/output.cpp \
  $(TESSERACT_PATH)/ccmain/pagesegmain.cpp \
  $(TESSERACT_PATH)/ccmain/pagewalk.cpp \
  $(TESSERACT_PATH)/ccmain/paramsd.cpp \
  $(TESSERACT_PATH)/ccmain/pgedit.cpp \
  $(TESSERACT_PATH)/ccmain/reject.cpp \
  $(TESSERACT_PATH)/ccmain/scaleimg.cpp \
  $(TESSERACT_PATH)/ccmain/recogtraining.cpp \
  $(TESSERACT_PATH)/ccmain/tesseract_cube_combiner.cpp \
  $(TESSERACT_PATH)/ccmain/tessbox.cpp \
  $(TESSERACT_PATH)/ccmain/tesseractclass.cpp \
  $(TESSERACT_PATH)/ccmain/tessvars.cpp \
  $(TESSERACT_PATH)/ccmain/tfacepp.cpp \
  $(TESSERACT_PATH)/ccmain/thresholder.cpp \
  $(TESSERACT_PATH)/ccmain/werdit.cpp

LOCAL_C_INCLUDES += \
  $(TESSERACT_PATH)/ccutil \
  $(TESSERACT_PATH)/ccstruct \
  $(TESSERACT_PATH)/image \
  $(TESSERACT_PATH)/viewer \
  $(TESSERACT_PATH)/dict \
  $(TESSERACT_PATH)/classify \
  $(TESSERACT_PATH)/wordrec \
  $(TESSERACT_PATH)/cutil \
  $(TESSERACT_PATH)/textord \
  $(TESSERACT_PATH)/ccmain \
  $(TESSERACT_PATH)/cube \
  $(LEPTONICA_PATH)/src

LOCAL_CFLAGS += \
  -DHAVE_LIBLEPT

# tesseract_api

LOCAL_SRC_FILES += \
  $(TESSERACT_PATH)/api/baseapi.cpp \
  $(TESSERACT_PATH)/api/pageiterator.cpp \
  $(TESSERACT_PATH)/api/resultiterator.cpp

LOCAL_C_INCLUDES += \
  $(TESSERACT_PATH)/api \
  $(TESSERACT_PATH)/ccutil \
  $(TESSERACT_PATH)/ccstruct \
  $(TESSERACT_PATH)/image \
  $(TESSERACT_PATH)/viewer \
  $(TESSERACT_PATH)/dict \
  $(TESSERACT_PATH)/classify \
  $(TESSERACT_PATH)/wordrec \
  $(TESSERACT_PATH)/cutil \
  $(TESSERACT_PATH)/textord \
  $(TESSERACT_PATH)/neural_networks/runtime \
  $(TESSERACT_PATH)/cube \
  $(TESSERACT_PATH)/ccmain \
  $(LEPTONICA_PATH)/src

LOCAL_CFLAGS += \
  -DHAVE_LIBLEPT

# jni

LOCAL_SRC_FILES += \
  $(REAL_LOCAL_PATH)/tessbaseapi.cpp

LOCAL_C_INCLUDES += \
  $(REAL_LOCAL_PATH) \
  $(TESSERACT_PATH)/api \
  $(LEPTONICA_PATH)/src
 

LOCAL_LDLIBS += \
  -ljnigraphics \
  -llog \

# common

LOCAL_PRELINK_MODULE := false
LOCAL_SHARED_LIBRARIES := liblept

include $(BUILD_SHARED_LIBRARY)
