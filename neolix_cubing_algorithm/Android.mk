# OpenNI Android makefile.
# libLetvdepthmodule.so
#

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS) 
OPENCV_LIB_TYPE=STATIC 
include ./opencv/native/jni/OpenCV.mk 
# set path to source
MY_FILES_PATH := $(LOCAL_PATH)/
#$(warning "MY_FILES_PATH are =================$(MY_FILES_PATH)")
#====================================================================
# 配置自己的源文件目录和源文件后缀名

MY_FILES_SUFFIX := %.cpp %.c
# 递归遍历目录下的所有的文件
rwildcard=$(wildcard $1$2) $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2))

# 获取相应的源文件
MY_ALL_FILES := $(foreach src_path,$(MY_FILES_PATH), $(call rwildcard,$(src_path),*.*) ) 
MY_ALL_FILES := $(MY_ALL_FILES:$(MY_CPP_PATH)/./%=$(MY_CPP_PATH)%)
MY_SRC_LIST  := $(filter $(MY_FILES_SUFFIX),$(MY_ALL_FILES)) 
MY_SRC_LIST  := $(MY_SRC_LIST:$(LOCAL_PATH)/%=%)

#$(warning "MY_ALL_FILES are =================$(MY_ALL_FILES)")
#$(warning "MY_SRC_LIST are =================$(MY_SRC_LIST)")

# 去除字串的重复单词
define uniq =
	$(eval seen :=)
	$(foreach _,$1,$(if $(filter $_,${seen}),,$(eval seen += $_)))
	${seen}
endef

# 递归遍历获取所有目录
MY_ALL_DIRS := $(dir $(foreach src_path,$(MY_FILES_PATH), $(call rwildcard,$(src_path),*/) ) )
MY_ALL_DIRS := $(call uniq,$(MY_ALL_DIRS))

#$(warning "MY_ALL_DIRS are ================= $(MY_ALL_DIRS)")

# 赋值给NDK编译系统
LOCAL_SRC_FILES  := $(MY_SRC_LIST)
LOCAL_C_INCLUDES := $(MY_ALL_DIRS)
#LOCAL_C_INCLUDES += $(MY_ALL_DIRS) 
					

$(warning "LOCAL_SRC_FILES are =================$(LOCAL_SRC_FILES)")
$(warning "LOCAL_C_INCLUDES are =================$(LOCAL_C_INCLUDES)")
#====================================================================
LOCAL_CFLAGS += -fPIC -Wno-multichar -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -w
#LOCAL_CFLAGS += -fvisibility=default
LOCAL_CFLAGS += -std=c++11
# for ADI SDK
#LOCAL_CFLAGS += -DLinux -DADI_DEPTH_CALC -DADI_SDK -DADDI9030_MACROS_FULL
# support try-throw-catch which ADI SDK use 
LOCAL_CPPFLAGS += -fexceptions

APP_STL := gnustl_static
LOCAL_LDFLAGS := -fPIE -pie
#LOCAL_SHARED_LIBRARIES := neolixCubingAlgrthm
#LOCAL_SHARED_LIBRARIES := opencv_core opencv_imgproc opencv_highgui

LOCAL_MODULE := neolixCubingAlgrthm

include $(BUILD_SHARED_LIBRARY)

