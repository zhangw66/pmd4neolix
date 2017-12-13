LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := neolixCubingAlgrthm
ifeq ($(TARGET_ARCH_ABI),arm64-v8a)
LOCAL_SRC_FILES := ./neolixAlgorithm/lib/arm64-v8a/libneolixCubingAlgrthm.so
else
LOCAL_SRC_FILES := ./neolixAlgorithm/lib/armeabi-v7a/libneolixCubingAlgrthm.so
endif
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := usb1.0
ifeq ($(TARGET_ARCH_ABI),arm64-v8a)
LOCAL_SRC_FILES := ./pmd_sdk/arm64-v8a/libusb1.0.so
else
LOCAL_SRC_FILES := ./pmd_sdk/armeabi-v7a/libusb1.0.so
endif
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := Tof
ifeq ($(TARGET_ARCH_ABI),arm64-v8a)
LOCAL_SRC_FILES := ./pmd_sdk/arm64-v8a/libTof.so
#NDK_APP_DST_DIR := ./pmd_sdk/arm64-v8a/
else
LOCAL_SRC_FILES := ./pmd_sdk/armeabi-v7a/libTof.so
#NDK_APP_DST_DIR := ./pmd_sdk/armeabi-v7a/
endif
include $(PREBUILT_SHARED_LIBRARY)
include $(CLEAR_VARS)
LOCAL_MODULE := mars04_sdk_wrapper 
ifeq ($(TARGET_ARCH_ABI),arm64-v8a)
LOCAL_SRC_FILES := ./pmd_sdk/arm64-v8a/libmars04_sdk_wrapper.so
#NDK_APP_DST_DIR := ./pmd_sdk/arm64-v8a/
else
LOCAL_SRC_FILES := ./pmd_sdk/armeabi-v7a/libmars04_sdk_wrapper.so
#NDK_APP_DST_DIR := ./pmd_sdk/armeabi-v7a/
endif
include $(PREBUILT_SHARED_LIBRARY)
include $(CLEAR_VARS)

include $(CLEAR_VARS)
#OPENCV_LIB_TYPE=STATIC 
#include ./opencv/native/jni/OpenCV.mk 
LOCAL_MODULE           := NeolixCubing
#CODE_PATH              := $(LOCAL_PATH)
#LOCAL_CFLAGS    :=

LOCAL_C_INCLUDES := ../../libs \
	./neolixAlgorithm/include \
	./opencv/native/jni/include

LOCAL_SRC_FILES := \
    NeolixCubing.cpp

LOCAL_CFLAGS := -DTARGET_PLATFORM_ANDROID

LOCAL_SHARED_LIBRARIES := \
	usb1.0 \
	Tof    \
	mars04_sdk_wrapper \
	neolixCubingAlgrthm
LOCAL_LDLIBS :=  -llog -ldl -ljnigraphics

include $(BUILD_SHARED_LIBRARY)
