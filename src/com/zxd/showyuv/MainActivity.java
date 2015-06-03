package com.zxd.showyuv;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

import android.app.Activity;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.util.DisplayMetrics;
import android.widget.LinearLayout;

public class MainActivity extends Activity {

	GLSurfaceView glSurfaceView;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		DisplayMetrics dm = getResources().getDisplayMetrics();
		LinearLayout layout = new LinearLayout(this);
		glSurfaceView = new GLSurfaceView(this);
		glSurfaceView.setRenderer(new Renderer());
		layout.addView(glSurfaceView, dm.widthPixels, dm.widthPixels * 9 / 16);
		setContentView(layout);
	}

	@Override
	protected void onPause() {
		glSurfaceView.onPause();
		super.onPause();
	}
	
	@Override
	protected void onResume() {
		super.onResume();
		glSurfaceView.onResume();
	}
	
	class Renderer implements GLSurfaceView.Renderer {

		@Override
		public void onDrawFrame(GL10 arg0) {
			NativeMethods.onNativeDrawFrame();
		}

		@Override
		public void onSurfaceChanged(GL10 arg0, int width, int height) {
			NativeMethods.onNativeSurfaceChanged(width, height);
		}

		@Override
		public void onSurfaceCreated(GL10 arg0, EGLConfig arg1) {
			NativeMethods.onNativeSurfaceCreated();
		}

	}
}
