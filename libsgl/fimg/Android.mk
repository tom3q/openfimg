LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_PRELINK_MODULE := false

LOCAL_CFLAGS += -march=armv6zk -mtune=arm1176jzf-s -g
LOCAL_CFLAGS += -DLOG_TAG=\"libfimg\"

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
