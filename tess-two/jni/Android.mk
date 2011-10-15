# NOTE: You must set these variables to their respective source paths before
# compiling. For example, set LEPTONICA_PATH to the directory containing
# the Leptonica configure file and source folders. Directories must be
# root-relative, e.g. TESSERACT_PATH := /home/username/tesseract-3.00
#
# To set the variables, you can run the following shell commands:
# export TESSERACT_PATH=<path-to-tesseract>
# export LEPTONICA_PATH=<path-to-leptonica>
# export LIBJPEG_PATH=<path-to-libjpeg>
#
# Or you can fill out and uncomment the following definitions:
# TESSERACT_PATH := <path-to-tesseract>
# LEPTONICA_PATH := <path-to-leptonica>
# LIBJPEG_PATH := <path-to-libjpeg>

TESSERACT_PATH := $(call my-dir)/../external/tesseract-3.00
LEPTONICA_PATH := $(call my-dir)/../external/leptonlib-1.66
LIBJPEG_PATH := $(call my-dir)/../external/libjpeg

ifeq "$(TESSERACT_PATH)" ""
  $(error You must set the TESSERACT_PATH variable to the Tesseract source \
          directory. See README and jni/Android.mk for details)
endif

ifeq "$(LEPTONICA_PATH)" ""
  $(error You must set the LEPTONICA_PATH variable to the Leptonica source \
          directory. See README and jni/Android.mk for details)
endif

ifeq "$(LIBJPEG_PATH)" ""
  $(error You must set the LIBJPEG_PATH variable to the Android JPEG \
          source directory. See README and jni/Android.mk for details)
endif

# Just build the Android.mk files in the subdirs
include $(call all-subdir-makefiles) $(LIBJPEG_PATH)/Android.mk
