THIS_PATH := $(call my-dir)

# LOCAL_PATH := $(THIS_PATH)
# include $(CLEAR_VARS) 

include $(call all-named-subdir-makefiles, libfimg)

#
# Build the hardware OpenGL ES library
#

LOCAL_PATH := $(THIS_PATH)
include $(CLEAR_VARS) 

LOCAL_PRELINK_MODULE := false

LOCAL_ARM_MODE := arm

LOCAL_SRC_FILES:= \
	eglBase.cpp \
	eglAndroid.cpp \
	glesBase.cpp \
	glesFramebuffer.cpp \
	glesGet.cpp \
	glesMatrix.cpp \
	glesPixel.cpp \
	glesTex.cpp \
	fglmatrix.cpp \
	fglframebuffer.cpp \
	fglsurface.cpp

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../include

LOCAL_CFLAGS += -DLOG_TAG=\"libsgl\"
LOCAL_CFLAGS += -DGL_GLEXT_PROTOTYPES -DEGL_EGLEXT_PROTOTYPES
LOCAL_CFLAGS += -O2 -mcpu=arm1176jzf-s -mfpu=vfp -Wall -Wno-unused-parameter

LOCAL_SHARED_LIBRARIES := libcutils libhardware libutils libpixelflinger libETC1
LOCAL_STATIC_LIBRARIES := libfimg
LOCAL_LDLIBS := -lpthread -ldl

ifeq ($(TARGET_ARCH),arm)
	LOCAL_CFLAGS += -fstrict-aliasing
endif

ifeq ($(ARCH_ARM_HAVE_TLS_REGISTER),true)
    LOCAL_CFLAGS += -DHAVE_ARM_TLS_REGISTER
endif

ifneq ($(TARGET_SIMULATOR),true)
    # we need to access the private Bionic header <bionic_tls.h>
    # on ARM platforms, we need to mirror the ARCH_ARM_HAVE_TLS_REGISTER
    # behavior from the bionic Android.mk file
    ifeq ($(TARGET_ARCH)-$(ARCH_ARM_HAVE_TLS_REGISTER),arm-true)
        LOCAL_CFLAGS += -DHAVE_ARM_TLS_REGISTER
    endif
    LOCAL_C_INCLUDES += bionic/libc/private
endif

# Build EGL for Android
LOCAL_CFLAGS += -DFGL_PLATFORM_ANDROID

# Use TLS slot of Bionic dedicated for OpenGL
LOCAL_CFLAGS += -DANDROID_FAST_TLS

# Debug GL errors
LOCAL_CFLAGS += -DGLES_ERR_DEBUG

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/egl
LOCAL_MODULE:= libGLES_fimg

include $(BUILD_SHARED_LIBRARY)
