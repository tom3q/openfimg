LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_PRELINK_MODULE := false

LOCAL_CFLAGS := -mcpu=arm1176jzf-s -Os
LOCAL_SRC_FILES := \
	compat.c \
	fragment.c \
	global.c \
	host.c \
	primitive.c \
	raster.c \
	shaders.c \
	system.c \
	texture.c
		
LOCAL_MODULE := libfimg
include $(BUILD_STATIC_LIBRARY)
