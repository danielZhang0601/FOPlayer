package com.zxd.showyuv;

public class NativeMethods {
	public static native void onNativeDrawFrame();
	public static native void onNativeSurfaceChanged(int width,int height);
	public static native void onNativeSurfaceCreated();
	
	public static native void onNativeSetFilePath(String path);
	
	static{
		System.loadLibrary("showYUV");
	}
}
