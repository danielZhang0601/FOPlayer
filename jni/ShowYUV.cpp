#include "com_zxd_showyuv_NativeMethods.h"

#include <stdio.h>
#include <stdlib.h>
//GL
#include <GLES/gl.h>
#include <GLES2/gl2ext.h>
//log
#include <android/log.h>
//FFmpeg
#ifndef INT64_C
#define INT64_C
#define UINT64_C
#endif
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

//global
unsigned char *plane0;
unsigned char *plane1;
unsigned char *plane2;
int frameWidth = -1, frameHeight = -1;
int videoWidth = -1, videoHeight = -1;
GLuint gProgram;
GLuint g_texYId, g_texUId, g_texVId;
GLuint textureUniformY, textureUniformU, textureUniformV;

static const GLfloat vertexVertices[] = { -1.0f, -1.0f, 1.0f, -1.0f, -1.0f,
		1.0f, 1.0f, 1.0f, };
static const GLfloat textureVertices[] = { 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, };

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,"NDK_INFO",__VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,"NDK_INFO",__VA_ARGS__)

#define ATTRIB_VERTEX 3
#define ATTRIB_TEXTURE 4

static void printGLString(const char *name, GLenum s) {
	const char *v = (const char *) glGetString(s);
	LOGI("GL %s = %s\n", name, v);
}

static void checkGlError(const char* op) {
	LOGI("checkGlError %s", op);
	for (GLint error = glGetError(); error; error = glGetError()) {
		LOGE("after %s() glError (0x%x)\n", op, error);
	}
}

static const char gVertexShader[] = "attribute vec4 vertexIn;\n"
		"attribute vec2 textureIn;\n"
		"varying vec2 textureOut;\n"
		"void main(void)\n"
		"{\n"
		"gl_Position = vertexIn;\n"
		"textureOut = textureIn;\n"
		"}\n";

static const char gFragmentShader[] = "varying lowp vec2 textureOut;\n"
		"uniform sampler2D tex_y;\n"
		"uniform sampler2D tex_u;\n"
		"uniform sampler2D tex_v;\n"
		"void main(void)\n"
		"{\n"
		"mediump vec3 yuv;\n"
		"lowp vec3 rgb;\n"
		"yuv.x = texture2D(tex_y, textureOut).r;\n"
		"yuv.y = texture2D(tex_u, textureOut).r - 0.5;\n"
		"yuv.z = texture2D(tex_v, textureOut).r - 0.5;\n"
		"rgb = mat3(1,1,1,\n"
		"0,-0.39465,2.03211,\n"
		"1.13983,-0.58060,0) * yuv;\n"
		"gl_FragColor = vec4(rgb, 1);\n"
		"}\n";

GLuint loadShader(GLenum shaderType, const char* pSource) {
	GLuint shader = glCreateShader(shaderType);
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
		glBindAttribLocation(program, ATTRIB_VERTEX, "vertexIn");
		checkGlError("glBindAttribLocation vertexIn");
		glBindAttribLocation(program, ATTRIB_TEXTURE, "textureIn");
		checkGlError("glBindAttribLocation textureIn");
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

bool setupGraphics(int w, int h) {
	printGLString("Version", GL_VERSION);
	printGLString("Vendor", GL_VENDOR);
	printGLString("Renderer", GL_RENDERER);
	printGLString("Extensions", GL_EXTENSIONS);

	LOGI("setupGraphics(%d, %d)", w, h);
	videoWidth = w;
	videoHeight = h;
	gProgram = createProgram(gVertexShader, gFragmentShader);
	if (!gProgram) {
		LOGE("Could not create program.");
		return false;
	}
	LOGI("create program:%d", gProgram);
	glUseProgram(gProgram);

	textureUniformY = glGetUniformLocation(gProgram, "tex_y");
	checkGlError("glGetUniformLocation tex_y");
	textureUniformU = glGetUniformLocation(gProgram, "tex_u");
	checkGlError("glGetUniformLocation tex_u");
	textureUniformV = glGetUniformLocation(gProgram, "tex_v");
	checkGlError("glGetUniformLocation tex_v");

	glVertexAttribPointer(ATTRIB_VERTEX, 2, GL_FLOAT, 0, 0, vertexVertices);
	checkGlError("glVertexAttribPointer vertexVertices");
	glEnableVertexAttribArray(ATTRIB_VERTEX);
	checkGlError("glEnableVertexAttribArray ATTRIB_VERTEX");
	glVertexAttribPointer(ATTRIB_TEXTURE, 2, GL_FLOAT, 0, 0, textureVertices);
	checkGlError("glVertexAttribPointer textureVertices");
	glEnableVertexAttribArray(ATTRIB_TEXTURE);
	checkGlError("glEnableVertexAttribArray ATTRIB_TEXTURE");

	glGenTextures(1, &g_texYId);
	checkGlError("glGenTextures g_texYId");
	glBindTexture(GL_TEXTURE_2D, g_texYId);
	checkGlError("glBindTexture g_texYId");
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	checkGlError("glTexParameteri g_texYId GL_TEXTURE_MAG_FILTER");
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	checkGlError("glTexParameteri g_texYId GL_TEXTURE_MIN_FILTER");
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	checkGlError("glTexParameteri g_texYId GL_TEXTURE_WRAP_S");
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	checkGlError("glTexParameteri g_texYId GL_TEXTURE_WRAP_T");

	glGenTextures(1, &g_texUId);
	checkGlError("glGenTextures g_texUId");
	glBindTexture(GL_TEXTURE_2D, g_texUId);
	checkGlError("glBindTexture g_texUId");
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	checkGlError("glTexParameteri g_texUId GL_TEXTURE_MAG_FILTER");
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	checkGlError("glTexParameteri g_texUId GL_TEXTURE_MIN_FILTER");
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	checkGlError("glTexParameteri g_texUId GL_TEXTURE_WRAP_S");
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	checkGlError("glTexParameteri g_texUId GL_TEXTURE_WRAP_T");

	glGenTextures(1, &g_texVId);
	checkGlError("glGenTextures g_texVId");
	glBindTexture(GL_TEXTURE_2D, g_texUId);
	checkGlError("glBindTexture g_texVId");
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	checkGlError("glTexParameteri g_texVId GL_TEXTURE_MAG_FILTER");
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	checkGlError("glTexParameteri g_texVId GL_TEXTURE_MIN_FILTER");
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	checkGlError("glTexParameteri g_texVId GL_TEXTURE_WRAP_S");
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	checkGlError("glTexParameteri g_texVId GL_TEXTURE_WRAP_T");

	glViewport(0, 0, w, h);
	checkGlError("glViewport");
	return true;
}

//const GLfloat gTriangleVertices[] = { 0.0f, 0.5f, -0.5f, -0.5f, 0.5f, -0.5f };

void renderFrame() {
	glClearColor(0.0, 255, 0.0, 0.0);
	checkGlError("glClearColor");
	glClear( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	checkGlError("glClear");
	glActiveTexture(GL_TEXTURE0);
	checkGlError("glActiveTexture GL_TEXTURE0");
	LOGI("g_texYId:%d", g_texYId);
	glBindTexture(GL_TEXTURE_2D, g_texYId);
	checkGlError("glBindTexture GL_TEXTURE0");
	LOGI("plane0 is NULL? %d", plane0 == NULL);
	if (plane0 == NULL)
		return;
	glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, frameWidth, frameHeight, 0,
	GL_LUMINANCE, GL_UNSIGNED_BYTE, plane0);
	checkGlError("glTexImage2D GL_TEXTURE0");
	glUniform1i(textureUniformY, 0);
	checkGlError("glUniform1i GL_TEXTURE0");
	glActiveTexture(GL_TEXTURE1);
	checkGlError("glActiveTexture GL_TEXTURE1");
	glBindTexture(GL_TEXTURE_2D, g_texUId);
	checkGlError("glBindTexture GL_TEXTURE1");
	glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, frameWidth / 2,
			frameHeight / 2, 0,
			GL_LUMINANCE, GL_UNSIGNED_BYTE, plane1);
	checkGlError("glTexImage2D GL_TEXTURE1");
	glUniform1i(textureUniformU, 1);
	checkGlError("glUniform1i GL_TEXTURE1");
	glActiveTexture(GL_TEXTURE2);
	checkGlError("glActiveTexture GL_TEXTURE2");
	glBindTexture(GL_TEXTURE_2D, g_texVId);
	checkGlError("glBindTexture GL_TEXTURE2");
	glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, frameWidth / 2,
			frameHeight / 2, 0,
			GL_LUMINANCE, GL_UNSIGNED_BYTE, plane2);
	checkGlError("glTexImage2D GL_TEXTURE2");
	glUniform1i(textureUniformV, 2);
	checkGlError("glUniform1i GL_TEXTURE2");
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	checkGlError("glDrawArrays");
	glFlush();
	checkGlError("glFlush");
}

JNIEXPORT void JNICALL Java_com_zxd_showyuv_NativeMethods_init(JNIEnv *env,
		jclass clazz, jint width, jint height) {
	setupGraphics(width, height);
}

JNIEXPORT void JNICALL Java_com_zxd_showyuv_NativeMethods_step(JNIEnv *env,
		jclass clazz) {
	renderFrame();
}

JNIEXPORT void JNICALL Java_com_zxd_showyuv_NativeMethods_decodeOneFrame(
		JNIEnv *env, jclass clazz, jstring path) {
	AVFormatContext *pFormatCtx;
	int videoindex;
	AVCodecContext *pCodecCtx;
	AVCodec *pCodec;
	AVFrame *pFrame, *pFrameYUV;
	uint8_t *out_buffer;
	AVPacket *packet;
	int y_size;
	int ret, got_picture;
	struct SwsContext *img_convert_ctx;
	const char *file_path = env->GetStringUTFChars(path, NULL);
	av_register_all();
	avformat_network_init();
	pFormatCtx = avformat_alloc_context();

	if (avformat_open_input(&pFormatCtx, file_path, NULL, NULL) != 0) {
		LOGE("Couldn't open input stream:%s.", file_path);
		return;
	}
	LOGI("Open file success:%s.", file_path);
	if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
		LOGE("Couldn't find stream information.");
		return;
	}
	LOGI("File stream information.");
	videoindex = -1;
	for (int i = 0; i < pFormatCtx->nb_streams; i++)
		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
			videoindex = i;
			break;
		}
	if (videoindex == -1) {
		LOGE("Didn't find a video stream.");
		return;
	}
	LOGI("Find video stream:%d.", videoindex);
	pCodecCtx = pFormatCtx->streams[videoindex]->codec;
	if (pCodecCtx == NULL) {
		LOGE("Codec context not found.");
		return;
	}
	LOGI("Find codec context:%d", pCodecCtx->codec_id);
	pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
	if (pCodec == NULL) {
		LOGE("Codec not found.");
		return;
	}
	LOGI("Find codec:%d.", pCodec->id);
	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
		LOGE("Could not open codec.");
		return;
	}
	LOGI("Open codec success.");
	pFrame = av_frame_alloc();
	pFrameYUV = av_frame_alloc();
	out_buffer = (uint8_t *) av_malloc(
			avpicture_get_size(PIX_FMT_YUV420P, pCodecCtx->width,
					pCodecCtx->height));
	avpicture_fill((AVPicture *) pFrameYUV, out_buffer, PIX_FMT_YUV420P,
			pCodecCtx->width, pCodecCtx->height);
	packet = (AVPacket *) av_malloc(sizeof(AVPacket));
//Output Info-----------------------------
	LOGI("--------------- File Information ----------------\n");
	av_dump_format(pFormatCtx, 0, file_path, 0);
	LOGI("-------------------------------------------------\n");
	img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height,
			pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height,
			PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);
	frameWidth = pCodecCtx->width;
	frameHeight = pCodecCtx->height;
	LOGI("frameWidth:%d\nframeHieght:%d", frameWidth, frameHeight);
	while (av_read_frame(pFormatCtx, packet) >= 0) {
		if (packet->stream_index == videoindex) {
			ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture,
					packet);
			if (ret < 0) {
				LOGE("Decode Error.\n");
				return;
			}
			if (got_picture) {
				sws_scale(img_convert_ctx,
						(const uint8_t* const *) pFrame->data, pFrame->linesize,
						0, pCodecCtx->height, pFrameYUV->data,
						pFrameYUV->linesize);
				plane0 = new unsigned char[frameWidth * frameHeight];
				plane1 = new unsigned char[frameWidth * frameHeight / 4];
				plane2 = new unsigned char[frameWidth * frameHeight / 4];
				memcpy(plane0, pFrameYUV->data[0], frameWidth * frameHeight);
				memcpy(plane1, pFrameYUV->data[1],
						frameWidth * frameHeight / 4);
				memcpy(plane2, pFrameYUV->data[2],
						frameWidth * frameHeight / 4);
				break;
			}
		}
		av_free_packet(packet);
	}

	sws_freeContext(img_convert_ctx);

	av_frame_free(&pFrameYUV);
	av_frame_free(&pFrame);
	avcodec_close(pCodecCtx);
	avformat_close_input(&pFormatCtx);
}
