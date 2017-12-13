LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := usb1.0
ifeq ($(TARGET_ARCH_ABI),arm64-v8a)
LOCAL_SRC_FILES := ../libs/arm64-v8a/libusb1.0.so
else
LOCAL_SRC_FILES := ../libs/armeabi-v7a/libusb1.0.so
endif
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := Tof
ifeq ($(TARGET_ARCH_ABI),arm64-v8a)
LOCAL_SRC_FILES := ../libs/arm64-v8a/libTof.so
NDK_APP_DST_DIR := ../libs/arm64-v8a/
else
LOCAL_SRC_FILES := ../libs/armeabi-v7a/libTof.so
NDK_APP_DST_DIR := ../libs/armeabi-v7a/
endif
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
    SampleCode.cpp
	
LOCAL_MODULE_PATH := $(LOCAL_PATH)
	
LOCAL_SHARED_LIBRARIES := \
	usb1.0 \
	Tof
	
#LOCAL_STATIC_LIBRARIES := \
#	libjpeg_static
	
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../include
		
LOCAL_CFLAGS += -fPIC -Wno-multichar -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -w

#LOCAL_LDFLAGS := -w
#LOCAL_LDFLAGS := -Wl,-v,--fatal-warnings
LOCAL_LDFLAGS := -fPIE -pie
#LOCAL_LDFLAGS += -L$(LOCAL_PATH) -lusb1.0 -lTof


APP_STL := gnustl_static
LOCAL_LDLIBS     := -llog
LOCAL_LDLIBS     += -lz

LOCAL_MODULE:= mars_test
LOCAL_MODULE_TAGS := debug

include $(BUILD_EXECUTABLE)
