package com.zxd.showyuv;

public class NativeMethods {
	
	static{
		System.loadLibrary("avutil-54");
		System.loadLibrary("avcodec-56");
		System.loadLibrary("swresample-1");
		System.loadLibrary("avformat-56");
		System.loadLibrary("swscale-3");
		System.loadLibrary("avfilter-5");
		System.loadLibrary("showYUV");
	}

	public static native void init(int width, int height);

	public static native void step();
	
	public static native void decodeOneFrame(String filePath);
}
