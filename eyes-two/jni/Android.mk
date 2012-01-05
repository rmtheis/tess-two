# Set this to the absolute path of the tesseract-android-tools project. Typically you should
# place the ocrservice and tesseract-android-tools project directories in the same parent
# directory, in which case this may be left as:
# TESSERACT_TOOLS_PATH := $(call my-dir)/../../tesseract-android-tools
TESSERACT_TOOLS_PATH := $(call my-dir)/../../tess-two

# Do not modify anything below this line.

PREBUILT_PATH := $(TESSERACT_TOOLS_PATH)/libs/$(TARGET_ARCH_ABI)

include $(call all-subdir-makefiles)
