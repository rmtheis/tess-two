LOCAL_PATH:= $(call my-dir)

common_SRC_FILES := \
	png.c \
	pngerror.c \
	pngget.c \
	pngmem.c \
	pngpread.c \
	pngread.c \
	pngrio.c \
	pngrtran.c \
	pngrutil.c \
	pngset.c \
	pngtrans.c \
	pngwio.c \
	pngwrite.c \
	pngwtran.c \
	pngwutil.c \

ifeq ($(ARCH_ARM_HAVE_NEON),true)
my_cflags_arm := -DPNG_ARM_NEON_OPT=2
endif

my_cflags_arm64 := -DPNG_ARM_NEON_OPT=2

my_src_files_arm := \
			arm/arm_init.c \
			arm/filter_neon.S \
			arm/filter_neon_intrinsics.c


common_CFLAGS := -std=gnu89 #-fvisibility=hidden ## -fomit-frame-pointer

include $(CLEAR_VARS)
LOCAL_CLANG := true
LOCAL_SRC_FILES := $(common_SRC_FILES)
LOCAL_CFLAGS += $(common_CFLAGS) -ftrapv
LOCAL_CFLAGS_arm := $(my_cflags_arm)
LOCAL_ASFLAGS += $(common_ASFLAGS)
LOCAL_SRC_FILES_arm := $(my_src_files_arm)
LOCAL_CFLAGS_arm64 := $(my_cflags_arm64)
LOCAL_SRC_FILES_arm64 := $(my_src_files_arm)
LOCAL_SANITIZE := never
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)
LOCAL_LDLIBS := -lz
LOCAL_MODULE:= libpngt
include $(BUILD_SHARED_LIBRARY)