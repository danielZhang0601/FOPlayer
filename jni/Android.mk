LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)  
LOCAL_MODULE :=avcodec
LOCAL_SRC_FILES :=FFmpeg/lib/libavcodec-56.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)  
LOCAL_MODULE :=avfilter  
LOCAL_SRC_FILES :=FFmpeg/lib/libavfilter-5.so  
include $(PREBUILT_SHARED_LIBRARY)  
   
include $(CLEAR_VARS)  
LOCAL_MODULE :=avformat
LOCAL_SRC_FILES :=FFmpeg/lib/libavformat-56.so  
include $(PREBUILT_SHARED_LIBRARY)  
   
include $(CLEAR_VARS)  
LOCAL_MODULE :=  avutil
LOCAL_SRC_FILES :=FFmpeg/lib/libavutil-54.so  
include $(PREBUILT_SHARED_LIBRARY)  
   
include $(CLEAR_VARS)  
LOCAL_MODULE :=  swresample
LOCAL_SRC_FILES :=FFmpeg/lib/libswresample-1.so  
include $(PREBUILT_SHARED_LIBRARY)  
   
include $(CLEAR_VARS)  
LOCAL_MODULE :=  swscale
LOCAL_SRC_FILES :=FFmpeg/lib/libswscale-3.so  
include $(PREBUILT_SHARED_LIBRARY) 

include $(CLEAR_VARS)
LOCAL_MODULE    := showYUV
LOCAL_SRC_FILES := ShowYUV.cpp
LOCAL_LDLIBS    := -llog -lGLESv2
LOCAL_C_INCLUDES := $(LOCAL_PATH)/FFmpeg/include
LOCAL_SHARED_LIBRARIES := avcodec avfilter avformat avutil swresample swscale
include $(BUILD_SHARED_LIBRARY)
