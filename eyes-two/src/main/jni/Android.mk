out_path := $(realpath $(NDK_OUT))
out_path := $(out_path:$(realpath $(out_path)/../../../..)%=%)
TESSERACT_TOOLS_PATH := $(TESSERACT_BUILD_PATH)/$(out_path)/local

PREBUILT_PATH := $(TESSERACT_TOOLS_PATH)/$(TARGET_ARCH_ABI)

include $(call all-subdir-makefiles)
