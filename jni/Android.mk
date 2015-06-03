LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := showYUV
LOCAL_SRC_FILES := ShowYUV.cpp

include $(BUILD_SHARED_LIBRARY)
