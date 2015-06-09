package com.zxd.showyuv;

import java.io.File;
import android.app.Activity;
import android.os.Bundle;
import android.os.Environment;
import android.util.DisplayMetrics;
import android.widget.LinearLayout;

public class MainActivity extends Activity {

	private PlayerGLSurfaceView glSurfaceView;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		DisplayMetrics dm = getResources().getDisplayMetrics();
		LinearLayout layout = new LinearLayout(this);
		glSurfaceView = new PlayerGLSurfaceView(this);
		layout.addView(glSurfaceView, dm.widthPixels, dm.widthPixels * 9 / 16);
		setContentView(layout);
	}

	@Override
	protected void onStart() {
		super.onStart();
		NativeMethods.decodeOneFrame(Environment.getExternalStorageDirectory()
				.toString() + File.separator + "1080.mp4");
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
}
