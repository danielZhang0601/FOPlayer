#include "com_zxd_showyuv_NativeMethods.h"

#include <stdio.h>
#include <stdlib.h>
//GL
#include <GLES/gl.h>
#include <GLES2/gl2ext.h>
//log
#include <android/log.h>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,"NDK_INFO",__VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,"NDK_INFO",__VA_ARGS__)

#define ATTRIB_VERTEX 3
#define ATTRIB_TEXTURE 4

static void printGLString(const char *name, GLenum s) {
	const char *v = (const char *) glGetString(s);
	LOGI("GL %s = %s\n", name, v);
}

static void checkGlError(const char* op) {
	for (GLint error = glGetError(); error; error = glGetError()) {
		LOGI("after %s() glError (0x%x)\n", op, error);
	}
}

static const char gVertexShader[] = "attribute vec4 vertexIn;\n"
		"attribute vec2 textureIn;\n"
		"varying vec2 textureOut;\n"
		"void main(void){\n"
		"gl_Position = vertexIn;\n"
		"textureOut = textureIn;}\n";

static const char gFragmentShader[] = "precision mediump float;\n"
		"varying vec2 textureOut;\n"
		"uniform sampler2D tex_y;\n"
		"uniform sampler2D tex_u;\n"
		"uniform sampler2D tex_v;\n"
		"void main(void){\n"
		"vec3 yuv;\n"
		"vec3 rgb;\n"
		"yuv.x = texture2D(tex_y, textureOut).r;\n"
		"yuv.y = texture2D(tex_u, textureOut).r - 0.5;\n"
		"yuv.z = texture2D(tex_v, textureOut).r - 0.5;\n"
		"rgb = mat3( 1,       1,         1,\n"
		"0,       -0.39465,  2.03211,\n"
		"1.13983, -0.58060,  0) * yuv;\n"
		"gl_FragColor = vec4(rgb, 1);}\n";

static const GLfloat vertexVertices[] = { -1.0f, -1.0f, 1.0f, -1.0f, -1.0f,
		1.0f, 1.0f, 1.0f, };
static const GLfloat textureVertices[] = { 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, };

GLuint loadShader(GLenum shaderType, const char* pSource) {
	GLuint shader = glCreateShader(shaderType);
	LOGI("shaderType:%d,code:%s.", shaderType, pSource);
	if (shader) {
		glShaderSource(shader, 1, &pSource, NULL);
		glCompileShader(shader);
		GLint compiled = 0;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
		if (!compiled) {
			GLint infoLen = 0;
			glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
			if (infoLen) {
				char* buf = (char*) malloc(infoLen);
				if (buf) {
					glGetShaderInfoLog(shader, infoLen, NULL, buf);
					LOGE("Could not compile shader %d:\n%s\n", shaderType, buf);
					free(buf);
				}
				glDeleteShader(shader);
				shader = 0;
			}
		}
	}
	return shader;
}

GLuint createProgram(const char* pVertexSource, const char* pFragmentSource) {
	GLuint vertexShader = loadShader(GL_VERTEX_SHADER, pVertexSource);
	if (!vertexShader) {
		return 0;
	}

	GLuint pixelShader = loadShader(GL_FRAGMENT_SHADER, pFragmentSource);
	if (!pixelShader) {
		return 0;
	}

	GLuint program = glCreateProgram();
	if (program) {
		glAttachShader(program, vertexShader);
		checkGlError("glAttachShader");
		glAttachShader(program, pixelShader);
		checkGlError("glAttachShader");
		glLinkProgram(program);
		GLint linkStatus = GL_FALSE;
		glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
		if (linkStatus != GL_TRUE) {
			GLint bufLength = 0;
			glGetProgramiv(program, GL_INFO_LOG_LENGTH, &bufLength);
			if (bufLength) {
				char* buf = (char*) malloc(bufLength);
				if (buf) {
					glGetProgramInfoLog(program, bufLength, NULL, buf);
					LOGE("Could not link program:\n%s\n", buf);
					free(buf);
				}
			}
			glDeleteProgram(program);
			program = 0;
		}
	}
	return program;
}

GLuint gProgram;
GLuint id_y, id_u, id_v; // Texture id
GLuint textureUniformY, textureUniformU, textureUniformV;

JNIEXPORT void JNICALL Java_com_zxd_showyuv_NativeMethods_onNativeDrawFrame(
		JNIEnv *env, jclass clazz) {

}

JNIEXPORT void JNICALL Java_com_zxd_showyuv_NativeMethods_onNativeSurfaceChanged(
		JNIEnv *env, jclass clazz, jint width, jint height) {
	printGLString("Version", GL_VERSION);
	printGLString("Vendor", GL_VENDOR);
	printGLString("Renderer", GL_RENDERER);
	printGLString("Extensions", GL_EXTENSIONS);

	LOGI("setupGraphics(%d, %d)", width, height);

	gProgram = createProgram(gVertexShader, gFragmentShader);
	if (!gProgram) {
		LOGE("Could not create program.");
		return;
	}
	LOGI("createProgram success:%d", gProgram);
	GLint linked;
	glGetProgramiv(gProgram, GL_LINK_STATUS, &linked);
	glUseProgram(gProgram);

	//Get Uniform Variables Location
	textureUniformY = glGetUniformLocation(gProgram, "tex_y");
	textureUniformU = glGetUniformLocation(gProgram, "tex_u");
	textureUniformV = glGetUniformLocation(gProgram, "tex_v");

	//Set Arrays
	glVertexAttribPointer(ATTRIB_VERTEX, 2, GL_FLOAT, 0, 0, vertexVertices);
	//Enable it
	glEnableVertexAttribArray (ATTRIB_VERTEX);
	glVertexAttribPointer(ATTRIB_TEXTURE, 2, GL_FLOAT, 0, 0, textureVertices);
	glEnableVertexAttribArray (ATTRIB_TEXTURE);

	//Init Texture
	glGenTextures(1, &id_y);
	glBindTexture(GL_TEXTURE_2D, id_y);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glGenTextures(1, &id_u);
	glBindTexture(GL_TEXTURE_2D, id_u);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glGenTextures(1, &id_v);
	glBindTexture(GL_TEXTURE_2D, id_v);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glViewport(0, 0, width, height);
	checkGlError("glViewport");
}

JNIEXPORT void JNICALL Java_com_zxd_showyuv_NativeMethods_onNativeSurfaceCreated(
		JNIEnv *env, jclass clazz) {

}
