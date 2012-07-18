LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional
LOCAL_PRELINK_MODULE := false

LOCAL_ARM_MODE := arm
LOCAL_CFLAGS += -Wall -Wno-unused-parameter -O2 -mcpu=arm1176jzf-s -mfloat-abi=softfp -mfpu=vfp
LOCAL_CFLAGS += -DLOG_TAG=\"libfimg\"
LOCAL_CFLAGS += -DFGL_PLATFORM_ANDROID

LOCAL_SRC_FILES := \
	compat.c \
	fragment.c \
	global.c \
	host.c \
	primitive.c \
	raster.c \
	system.c \
	texture.c \
	dump.c

LOCAL_MODULE := libfimg
include $(BUILD_STATIC_LIBRARY)
