#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <android/window.h>
#include <unistd.h>
#include <sys/time.h>
#include "anglue.h"
#include "demo.h"
#include "gfxutil.h"
#include "util.h"
#include "logger.h"
#include "audio.h"

static void handle_command(struct android_app *app, int32_t cmd);
static int handle_input(struct android_app *app, AInputEvent *ev);
static int handle_touch_input(struct android_app *app, AInputEvent *ev);
static void reshape(int x, int y);
static int init_gl(void);
static void destroy_gl(void);
static unsigned long get_time_msec(void);
static void hide_navbar(struct android_app *state);
static const char *cmdname(uint32_t cmd);

#define MODE(w, h)	\
	{0, w, h, 16, w * 2, 5, 6, 5, 11, 5, 0, 0xf800, 0x7e0, 0x1f, 0xbadf00d, 2, 0}
static struct video_mode vmodes[] = {
	MODE(320, 240), MODE(400, 300), MODE(512, 384), MODE(640, 480),
	MODE(800, 600), MODE(1024, 768), MODE(1280, 960), MODE(1280, 1024),
	MODE(1920, 1080), MODE(1600, 1200), MODE(1920, 1200)
};
static struct video_mode *cur_vmode;


struct android_app *app;

static EGLDisplay dpy;
static EGLSurface surf;
static EGLContext ctx;
static int init_done, paused, win_valid;

static int width, height;
static float aspect;

static unsigned long start_time;
static unsigned long init_time, sys_time;

static unsigned int tex;
static int tex_xsz, tex_ysz;

static float varr[] = {-1, -1, 1, -1, -1, 1, 1, -1, 1, 1, -1, 1};
static float uvarr[] = {0, 0, 1, 0, 0, 1, 1, 0, 1, 1, 0, 1};
static unsigned int vbo_verts, vbo_uv;

static unsigned int vsdr, psdr, sdrprog;
static int attr_vertex, attr_uv;

static const char *vsdr_src =
	"attribute vec4 attr_vertex;\n"
	"attribute vec2 attr_uv;\n"
	"varying vec2 vtex;\n"
	"void main() { vtex = attr_uv; gl_Position = attr_vertex; }\n";

static const char *psdr_src =
	"precision mediump float;\n"
	"varying vec2 vtex;\n"
	"uniform sampler2D tex;\n"
	"void main() { gl_FragColor = texture2D(tex, vtex); }\n";

cgm_vec3 sball_pos = {0, 0, 0};
cgm_quat sball_rot = {0, 0, 0, 1};

cgm_vec3 sball_view_pos = {0, 0, 0};
cgm_quat sball_view_rot = {0, 0, 0, 1};



void android_main(struct android_app *app_ptr)
{
	static char *fake_argv[] = {"alephnull", 0};

	app = app_ptr;

	app->onAppCmd = handle_command;
	app->onInputEvent = handle_input;

	hide_navbar(app);

	start_logger();

	printf("Running %d bit version\n", (int)sizeof(void*) << 3);

	demo_init_cfgopt(1, fake_argv);

	if(!set_video_mode(match_video_mode(FB_WIDTH, FB_HEIGHT, FB_BPP), 1)) {
		demo_abort();
	}

	if(au_init() == -1) {
		demo_abort();
	}
	time_msec = 0;

	for(;;) {
		int num_events;
		struct android_poll_source *pollsrc;

		while(ALooper_pollAll(0, 0, &num_events, (void**)&pollsrc) >= 0) {
			if(pollsrc) {
				pollsrc->process(app, pollsrc);
			}
		}

		if(app->destroyRequested) {
			return;
		}

		sys_time = (long)get_time_msec();
		if(!init_done) {
			if(win_valid && sys_time - init_time >= 700) {
				if(init_gl() == -1) {
					demo_abort();
				}
				reshape(width, height);
				if(demo_init() == -1) {
					demo_abort();
				}
				reset_timer(0);
				init_done = 1;
			}

		} else {
			if(!paused) {
				time_msec = get_msec();
				demo_draw();
				eglSwapBuffers(dpy, surf);
			}
		}
	}
}

void demo_quit(void)
{
	exit(0);
}

void demo_abort(void)
{
	abort();
}

struct video_mode *video_modes(void)
{
	return vmodes;
}

int num_video_modes(void)
{
	return sizeof vmodes / sizeof *vmodes;
}

struct video_mode *get_video_mode(int idx)
{
	if(idx == VMODE_CURRENT) {
		return cur_vmode;
	}
	return vmodes + idx;
}

int match_video_mode(int xsz, int ysz, int bpp)
{
	struct video_mode *vm = vmodes;
	int i, count = num_video_modes();

	for(i=0; i<count; i++) {
		if(vm->xsz == xsz && vm->ysz == ysz && vm->bpp == bpp) {
			return i;
		}
		vm++;
	}
	return -1;
}

static uint32_t *convbuf;
static int convbuf_size;

void *set_video_mode(int idx, int nbuf)
{
	struct video_mode *vm = vmodes + idx;

	if(cur_vmode == vm) {
		return vmem;
	}

	if(vm->xsz * vm->ysz > convbuf_size) {
		convbuf_size = vm->xsz * vm->ysz;
		free(convbuf);
		convbuf = malloc(convbuf_size * sizeof *convbuf);
	}

	if(demo_resizefb(vm->xsz, vm->ysz, vm->bpp) == -1) {
		fprintf(stderr, "failed to allocate virtual framebuffer\n");
		return 0;
	}
	vmem = fb_pixels;

	cur_vmode = vm;
	return vmem;
}

void wait_vsync(void)
{
}

void blit_frame(void *pixels, int vsync)
{
	unsigned int i, j;
	uint32_t *dptr = convbuf;
	uint16_t *sptr = pixels;

	demo_post_draw(pixels);

	sptr = (uint16_t*)pixels + (FB_HEIGHT - 1) * FB_WIDTH;
	for(i=0; i<FB_HEIGHT; i++) {
		for(j=0; j<FB_WIDTH; j++) {
			uint16_t pix = sptr[j];
			unsigned int r = UNPACK_R16(pix);
			unsigned int g = UNPACK_G16(pix);
			unsigned int b = UNPACK_B16(pix);
			*dptr++ = PACK_RGB32(b, g, r);
		}
		sptr -= FB_WIDTH;
	}

	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, FB_WIDTH, FB_HEIGHT, GL_RGBA,
			GL_UNSIGNED_BYTE, convbuf);

	glClear(GL_COLOR_BUFFER_BIT);

	glBindBuffer(GL_ARRAY_BUFFER, vbo_verts);
	glVertexAttribPointer(attr_vertex, 2, GL_FLOAT, 0, 0, 0);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_uv);
	glVertexAttribPointer(attr_uv, 2, GL_FLOAT, 0, 0, 0);

	glEnableVertexAttribArray(attr_vertex);
	glEnableVertexAttribArray(attr_uv);

	glDrawArrays(GL_TRIANGLES, 0, 6);

	glDisableVertexAttribArray(attr_vertex);
	glDisableVertexAttribArray(attr_uv);
}

int kb_isdown(int key)
{
	return 0;
}

/* timer */
void init_timer(int res_hz)
{
}

void reset_timer(unsigned long ms)
{
	start_time = get_time_msec() - ms;
	time_msec = get_msec();
}

unsigned long get_msec(void)
{
	return get_time_msec() - start_time;
}

void sleep_msec(unsigned long msec)
{
	usleep(msec * 1000);
}

static void handle_command(struct android_app *app, int32_t cmd)
{
	int xsz, ysz;

	printf("DBG android command: %s\n", cmdname(cmd));

	switch(cmd) {
	case APP_CMD_PAUSE:
		paused = 1;	/* TODO: handle timers */
		dseq_pause();
		break;
	case APP_CMD_RESUME:
		paused = 0;
		dseq_resume();
		break;

	case APP_CMD_INIT_WINDOW:
		ANativeActivity_setWindowFlags(app->activity, AWINDOW_FLAG_KEEP_SCREEN_ON, 0);
		/*if(init_gl() == -1) {
			exit(1);
		}*/
		init_time = (long)get_time_msec();
		win_valid = 1;
		break;

	case APP_CMD_TERM_WINDOW:
		if(init_done) {
			demo_cleanup();
		}
		init_done = 0;
		win_valid = 0;
		destroy_gl();
		break;

	case APP_CMD_WINDOW_RESIZED:
	case APP_CMD_CONFIG_CHANGED:
		xsz = ANativeWindow_getWidth(app->window);
		ysz = ANativeWindow_getHeight(app->window);
		if(xsz != width || ysz != height) {
			printf("reshape(%d, %d)\n", xsz, ysz);
			reshape(xsz, ysz);
			width = xsz;
			height = ysz;
		}
		break;

	/*
	case APP_CMD_SAVE_STATE:
	case APP_CMD_GAINED_FOCUS:
	case APP_CMD_LOST_FOCUS:
	*/
	default:
		break;
	}
}

static int handle_input(struct android_app *app, AInputEvent *ev)
{
	int evtype = AInputEvent_getType(ev);

	switch(evtype) {
	case AINPUT_EVENT_TYPE_MOTION:
		return handle_touch_input(app, ev);

	default:
		break;
	}
	return 0;
}

static int handle_touch_input(struct android_app *app, AInputEvent *ev)
{
	/*
	int i, pcount, x, y, idx;
	unsigned int action;
	static int prev_pos[2];

	action = AMotionEvent_getAction(ev);

	idx = (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >>
		AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;

	x = AMotionEvent_getX(ev, idx);
	y = AMotionEvent_getY(ev, idx);

	switch(action & AMOTION_EVENT_ACTION_MASK) {
	case AMOTION_EVENT_ACTION_DOWN:
	case AMOTION_EVENT_ACTION_POINTER_DOWN:
		demo_mouse(0, 1, x, y);

		prev_pos[0] = x;
		prev_pos[1] = y;
		break;

	case AMOTION_EVENT_ACTION_UP:
	case AMOTION_EVENT_ACTION_POINTER_UP:
		demo_mouse(0, 0, x, y);

		prev_pos[0] = x;
		prev_pos[1] = y;
		break;

	case AMOTION_EVENT_ACTION_MOVE:
		pcount = AMotionEvent_getPointerCount(ev);
		for(i=0; i<pcount; i++) {
			int id = AMotionEvent_getPointerId(ev, i);
			if(id == 0) {
				demo_motion(x, y);
				prev_pos[0] = x;
				prev_pos[1] = y;
				break;
			}
		}
		break;

	default:
		break;
	}
	*/

	return 1;
}

static void reshape(int x, int y)
{
	width = x;
	height = y;
	aspect = (float)x / (float)y;
	glViewport(0, 0, x, y);
}

static int init_gl(void)
{
	static const int eglattr[] = {
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
		EGL_RED_SIZE, 5,
		EGL_GREEN_SIZE, 5,
		EGL_BLUE_SIZE, 5,
		EGL_DEPTH_SIZE, 16,
		EGL_NONE
	};
	static const int ctxattr[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };

	EGLConfig eglcfg;
	int i, count, vis;
	int status, loglen;
	float s;
	char *buf;

	dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	if(!dpy || !eglInitialize(dpy, 0, 0)) {
		fprintf(stderr, "failed to initialize EGL\n");
		destroy_gl();
		return -1;
	}

	if(!eglChooseConfig(dpy, eglattr, &eglcfg, 1, &count)) {
		fprintf(stderr, "no matching EGL config found\n");
		destroy_gl();
		return -1;
	}

	/* configure the native window visual according to the chosen EGL config */
	eglGetConfigAttrib(dpy, &eglcfg, EGL_NATIVE_VISUAL_ID, &vis);
	ANativeWindow_setBuffersGeometry(app->window, 0, 0, vis);

	if(!(surf = eglCreateWindowSurface(dpy, eglcfg, app->window, 0))) {
		fprintf(stderr, "failed to create window\n");
		destroy_gl();
		return -1;
	}

	if(!(ctx = eglCreateContext(dpy, eglcfg, EGL_NO_CONTEXT, ctxattr))) {
		fprintf(stderr, "failed to create OpenGL ES context\n");
		destroy_gl();
		return -1;
	}
	eglMakeCurrent(dpy, surf, surf, ctx);

	eglQuerySurface(dpy, surf, EGL_WIDTH, &width);
	eglQuerySurface(dpy, surf, EGL_HEIGHT, &height);
	aspect = (float)width / (float)height;

	printf("egl framebuffer: %dx%d\n", width, height);

	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);

	tex_xsz = next_pow2(cur_vmode->xsz);
	tex_ysz = next_pow2(cur_vmode->ysz);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex_xsz, tex_ysz, 0, GL_RGBA,
			GL_UNSIGNED_BYTE, 0);
	if(opt.scaler == SCALER_LINEAR) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	} else {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}

	if(aspect >= FB_ASPECT) {
		s = FB_ASPECT / aspect;
	} else {
		s = aspect / FB_ASPECT;
	}
	for(i=0; i<6; i++) {
		varr[i * 2] *= s;
		uvarr[i * 2] *= (float)cur_vmode->xsz / (float)tex_xsz;
		uvarr[i * 2 + 1] *= (float)cur_vmode->ysz / (float)tex_ysz;
	}

	glGenBuffers(1, &vbo_verts);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_verts);
	glBufferData(GL_ARRAY_BUFFER, sizeof varr, varr, GL_STATIC_DRAW);
	glGenBuffers(1, &vbo_uv);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_uv);
	glBufferData(GL_ARRAY_BUFFER, sizeof uvarr, uvarr, GL_STATIC_DRAW);

	vsdr = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vsdr, 1, &vsdr_src, 0);
	glCompileShader(vsdr);
	glGetShaderiv(vsdr, GL_COMPILE_STATUS, &status);
	if(!status) {
		fprintf(stderr, "failed to compile vertex shader\n");
		return -1;
	}

	psdr = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(psdr, 1, &psdr_src, 0);
	glCompileShader(psdr);
	glGetShaderiv(psdr, GL_COMPILE_STATUS, &status);
	glGetShaderiv(psdr, GL_INFO_LOG_LENGTH, &loglen);
	if(loglen > 0 && (buf = malloc(loglen + 1))) {
		glGetShaderInfoLog(psdr, loglen, 0, buf);
		fprintf(stderr, "Pixel shader: %s\n", buf);
		free(buf);
	}
	if(!status) {
		fprintf(stderr, "failed to compile pixel shader\n");
		return -1;
	}

	sdrprog = glCreateProgram();
	glAttachShader(sdrprog, vsdr);
	glAttachShader(sdrprog, psdr);
	glLinkProgram(sdrprog);
	glGetProgramiv(sdrprog, GL_LINK_STATUS, &status);
	if(!status) {
		fprintf(stderr, "failed to link shader program\n");
		return -1;
	}
	glUseProgram(sdrprog);

	attr_vertex = glGetAttribLocation(sdrprog, "attr_vertex");
	attr_uv = glGetAttribLocation(sdrprog, "attr_uv");

	return 0;
}

static void destroy_gl(void)
{
	if(!dpy) return;

	eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

	if(ctx) {
		eglDestroyContext(dpy, ctx);
		ctx = 0;
	}
	if(surf) {
		eglDestroySurface(dpy, surf);
		surf = 0;
	}

	eglTerminate(dpy);
	dpy = 0;
}

static unsigned long get_time_msec(void)
{
	struct timespec ts;
	static struct timespec ts0;

	clock_gettime(CLOCK_MONOTONIC, &ts);
	if(ts0.tv_sec == 0 && ts0.tv_nsec == 0) {
		ts0 = ts;
		return 0;
	}
	return (ts.tv_sec - ts0.tv_sec) * 1000 + (ts.tv_nsec - ts0.tv_nsec) / 1000000;
}

static void hide_navbar(struct android_app *state)
{
	JNIEnv *env;
	jclass cactivity, cwin, cview;
	jobject win, view;
	jmethodID get_window, get_decor_view, set_system_ui_visibility;
	jfieldID field_flag_fs, field_flag_hidenav, field_flag_immersive;
	int flag_fs, flag_hidenav, flag_immersive;

	(*state->activity->vm)->AttachCurrentThread(state->activity->vm, &env, 0);

	cactivity = (*env)->FindClass(env, "android/app/NativeActivity");
	get_window = (*env)->GetMethodID(env, cactivity, "getWindow", "()Landroid/view/Window;");

	cwin = (*env)->FindClass(env, "android/view/Window");
	get_decor_view = (*env)->GetMethodID(env, cwin, "getDecorView", "()Landroid/view/View;");

	cview = (*env)->FindClass(env, "android/view/View");
	set_system_ui_visibility = (*env)->GetMethodID(env, cview, "setSystemUiVisibility", "(I)V");

	win = (*env)->CallObjectMethod(env, state->activity->clazz, get_window);
	view = (*env)->CallObjectMethod(env, win, get_decor_view);

	field_flag_fs = (*env)->GetStaticFieldID(env, cview, "SYSTEM_UI_FLAG_FULLSCREEN", "I");
	field_flag_hidenav = (*env)->GetStaticFieldID(env, cview, "SYSTEM_UI_FLAG_HIDE_NAVIGATION", "I");
	field_flag_immersive = (*env)->GetStaticFieldID(env, cview, "SYSTEM_UI_FLAG_IMMERSIVE_STICKY", "I");

	flag_fs = (*env)->GetStaticIntField(env, cview, field_flag_fs);
	flag_hidenav = (*env)->GetStaticIntField(env, cview, field_flag_hidenav);
	flag_immersive = (*env)->GetStaticIntField(env, cview, field_flag_immersive);

	(*env)->CallVoidMethod(env, view, set_system_ui_visibility, flag_fs | flag_hidenav | flag_immersive);

	(*state->activity->vm)->DetachCurrentThread(state->activity->vm);
}

static const char *cmdname(uint32_t cmd)
{
	static const char *names[] = {
		"APP_CMD_INPUT_CHANGED",
		"APP_CMD_INIT_WINDOW",
		"APP_CMD_TERM_WINDOW",
		"APP_CMD_WINDOW_RESIZED",
		"APP_CMD_WINDOW_REDRAW_NEEDED",
		"APP_CMD_CONTENT_RECT_CHANGED",
		"APP_CMD_GAINED_FOCUS",
		"APP_CMD_LOST_FOCUS",
		"APP_CMD_CONFIG_CHANGED",
		"APP_CMD_LOW_MEMORY",
		"APP_CMD_START",
		"APP_CMD_RESUME",
		"APP_CMD_SAVE_STATE",
		"APP_CMD_PAUSE",
		"APP_CMD_STOP",
		"APP_CMD_DESTROY"
	};
	if(cmd >= sizeof names / sizeof *names) {
		return "unknown";
	}
	return names[cmd];
}
