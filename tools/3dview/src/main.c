#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <unistd.h>
#include "miniglut.h"
#include "util.h"
#include "gfxutil.h"
#include "3dgfx/3dgfx.h"
#include "3dgfx/scene.h"
#include "ass2goat.h"
#include "imtk.h"
#include "drawtext.h"
#include "cthreads.h"

#ifndef GL_UNSIGNED_SHORT_5_6_5
#define GL_UNSIGNED_SHORT_5_6_5	0x8363
#endif

#define CON_VISLINES	5
#define CON_HEIGHT		((CON_VISLINES + 1) * 20)
#define CON_TOP			(win_height - CON_HEIGHT)

static int init(void);
static void cleanup(void);
static void display(void);
static void gui(void);
static void reshape(int x, int y);
static void keypress(unsigned char key, int x, int y);
static void keyrelease(unsigned char key, int x, int y);
static void skeypress(int key, int x, int y);
static void skeyrelease(int key, int x, int y);
static void mouse(int bn, int st, int x, int y);
static void motion(int x, int y);

static int parse_args(int argc, char **argv);

static void init_console(void);
static void scroll_console(int dir);

uint16_t *fb_pixels;
int fb_width, fb_height;

int win_width, win_height;

struct dtx_font *uifont;
int uifontsz;

static struct g3d_scene *scn;
static const char *scenefile;
static char *dirname;

static float cam_theta, cam_phi, cam_dist = 8;
static float cam_pan[3];
static unsigned int bnstate;
static int prev_mx, prev_my;
static int fbscale = 1;
static unsigned int fbtex;
static float znear = 0.5f;
static float zfar = 500.0f;

static int show_ui, show_help, show_con = 1, uibox[4], bnbox[4];
static int opt_lowres, opt_wireframe;
static int opt_lighting = 1;

static float scene_size;

#define CON_LINELEN		128
#define CON_BUFLINES	1024
static char conbuf[CON_BUFLINES][CON_LINELEN];
static int con_nlines, con_tail_idx, con_scroll;


int main(int argc, char **argv)
{
	glutInit(&argc, argv);

	if(parse_args(argc, argv) == -1) {
		return 1;
	}

	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
	glutInitWindowSize(1280, 960);
	glutCreateWindow("3D scene previewer");

	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keypress);
	glutKeyboardUpFunc(keyrelease);
	glutSpecialFunc(skeypress);
	glutSpecialUpFunc(skeyrelease);
	glutMouseFunc(mouse);
	glutMotionFunc(motion);
	glutPassiveMotionFunc(motion);

	if(init() == -1) {
		return 1;
	}
	atexit(cleanup);

	glutMainLoop();
	return 0;
}

static int init(void)
{
	char *last_slash;
	float bmin[3], bmax[3], dx, dy, dz;
	struct goat3d *gscn;

	init_console();

	glGenTextures(1, &fbtex);
	glBindTexture(GL_TEXTURE_2D, fbtex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glEnable(GL_TEXTURE_2D);

	if(!(uifont = dtx_open_font_glyphmap("font.glyphmap"))) {
		fprintf(stderr, "failed to open font\n");
		return -1;
	}
	uifontsz = dtx_get_glyphmap_ptsize(dtx_get_glyphmap(uifont, 0));
	dtx_use_font(uifont, uifontsz);

	imtk_set_color(IMTK_TEXT_COLOR, 1, 1, 1, 1);
	imtk_set_color(IMTK_TOP_COLOR, 0.4, 0.4, 0.4, 1);
	imtk_set_color(IMTK_BOTTOM_COLOR, 0.25, 0.25, 0.25, 1);
	imtk_set_color(IMTK_BEVEL_LIT_COLOR, 0.6, 0.6, 0.6, 1);
	imtk_set_color(IMTK_BEVEL_SHAD_COLOR, 0.2, 0.2, 0.2, 1);

	if(((last_slash = strrchr(scenefile, '/')) || (last_slash = strrchr(scenefile, '\\'))) && last_slash[1]) {
		dirname = strdup_nf(scenefile);
		dirname[last_slash - scenefile] = 0;
		scenefile = last_slash + 1;

		chdir(dirname);
	}


	if(g3d_init() == -1) {
		return -1;
	}
	g3d_polygon_mode(G3D_GOURAUD);
	g3d_enable(G3D_DEPTH_TEST);
	g3d_enable(G3D_LIGHTING);
	g3d_enable(G3D_LIGHT0);

	if(!(gscn = ass2goat(scenefile))) {
		return -1;
	}
	scn = scn_create();
	conv_goat3d_scene(scn, gscn);
	goat3d_get_bounds(gscn, bmin, bmax);
	goat3d_free(gscn);

	printf("scene bounds: %.3f %.3f %.3f -> %.3f %.3f %.3f\n", bmin[0], bmin[1], bmin[2],
			bmax[0], bmax[1], bmax[2]);
	dx = bmax[0] - bmin[0];
	dy = bmax[1] - bmin[1];
	dz = bmax[2] - bmin[2];
	cam_dist = scene_size = sqrt(dx * dx + dy * dy + dz * dz);
	printf("cam dist: %f\n", cam_dist);

	return 0;
}

static void cleanup(void)
{
	scn_free(scn);
	g3d_destroy();
}

static void display(void)
{
	g3d_clear_color(35, 35, 35);
	g3d_clear(G3D_COLOR_BUFFER_BIT | G3D_DEPTH_BUFFER_BIT);

	g3d_matrix_mode(G3D_PROJECTION);
	g3d_load_identity();
	g3d_perspective(50.0f, (float)fb_width / (float)fb_height, znear, zfar);

	g3d_matrix_mode(G3D_MODELVIEW);
	g3d_load_identity();
	g3d_translate(0, 0, -cam_dist);
	g3d_rotate(cam_phi, 1, 0, 0);
	g3d_rotate(cam_theta, 0, 1, 0);
	g3d_translate(cam_pan[0], cam_pan[1], cam_pan[2]);

	scn_draw(scn);

	/* present */
	glBindTexture(GL_TEXTURE_2D, fbtex);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, fb_width, fb_height, GL_RGB,
			GL_UNSIGNED_SHORT_5_6_5, fb_pixels);

	glBegin(GL_TRIANGLES);
	glColor3f(1, 1, 1);
	glTexCoord2f(0, 0);
	glVertex2f(0, 0);
	glTexCoord2f(2, 0);
	glVertex2f(win_width * 2, 0);
	glTexCoord2f(0, 2);
	glVertex2f(0, win_height * 2);
	glEnd();

	gui();

	glutSwapBuffers();
	assert(glGetError() == GL_NO_ERROR);
}

static void gui(void)
{
	int val, x, y, bbox[4];

	imtk_begin();

	imtk_layout_start(8, 0);
	imtk_layout_spacing(8);
	imtk_layout_dir(IMTK_HORIZONTAL);

	if(imtk_button(IMUID, show_ui ? "Hide UI  (TAB)" : "Show UI (TAB)", IMTK_AUTO, IMTK_AUTO)) {
		show_ui ^= 1;
		glutPostRedisplay();
	}
	if(show_ui) {
		imtk_layout_get_bounds(bbox);
		x = bbox[0];
		y = bbox[1] + bbox[3] + 5;

		uibox[0] = 0;
		uibox[1] = y;
		uibox[2] = 130;
		uibox[3] = y + 60;

		glBegin(GL_QUADS);
		glColor3f(0.25, 0.25, 0.25);
		glVertex2f(uibox[0], uibox[1]);
		glVertex2f(uibox[0], uibox[1] + uibox[3]);
		glVertex2f(uibox[0] + uibox[2], uibox[1] + uibox[3]);
		glVertex2f(uibox[0] + uibox[2], uibox[1]);
		glEnd();

		imtk_layout_push();
		imtk_layout_start(x + 5, y + 5);
		imtk_layout_dir(IMTK_VERTICAL);

		if((val = imtk_checkbox(IMUID, "low res", IMTK_AUTO, IMTK_AUTO, opt_lowres)) != opt_lowres) {
			opt_lowres = val;
			fbscale = opt_lowres ? 3 : 1;
			reshape(win_width, win_height);
			glutPostRedisplay();
		}
		if((val = imtk_checkbox(IMUID, "lighting", IMTK_AUTO, IMTK_AUTO, opt_lighting)) != opt_lighting) {
			opt_lighting = val;
			if(opt_lighting) {
				g3d_light_ambient(0.1, 0.1, 0.1);
			} else {
				g3d_light_ambient(1, 1, 1);
			}
			glutPostRedisplay();
		}
		if((val = imtk_checkbox(IMUID, "wireframe", IMTK_AUTO, IMTK_AUTO, opt_wireframe)) != opt_wireframe) {
			opt_wireframe = val;
			if(opt_wireframe) {
				g3d_disable(G3D_LIGHTING);
				g3d_polygon_mode(G3D_WIRE);
			} else {
				g3d_enable(G3D_LIGHTING);
				g3d_polygon_mode(G3D_GOURAUD);
			}
			glutPostRedisplay();
		}

		/*
		char *list = "foo\0bar\0xyzzy\0";
		imtk_listbox(IMUID, list, -1, IMTK_AUTO, IMTK_AUTO);
		*/

		imtk_layout_pop();
	} else {
		uibox[0] = uibox[1] = uibox[2] = uibox[3] = 0;
	}

	if(imtk_button(IMUID, "Console (~)", IMTK_AUTO, IMTK_AUTO)) {
		show_con ^= 1;
		glutPostRedisplay();
	}

	if(imtk_button(IMUID, "Help (F1)", IMTK_AUTO, IMTK_AUTO)) {
		show_help ^= 1;
		glutPostRedisplay();
	}

	imtk_layout_get_bounds(bnbox);

	if(show_help) {
		static const char *helptext[] = {
			"Viewport controls",
			"------------------------------",
			" - Rotate: left mouse drag",
			" - Pan: middle mouse drag",
			" - Zoom: right mouse vertical drag or mouse wheel"
		};
		int i, j;
		imtk_set_color(IMTK_TEXT_COLOR, 0, 0, 0, 1);
		imtk_layout_spacing(15);
		imtk_layout_dir(IMTK_VERTICAL);

		for(i=0; i<2; i++) {
			imtk_layout_start(win_width / 3 - i * 2, win_height / 3 - i * 2);
			for(j=0; j<sizeof helptext / sizeof *helptext; j++) {
				imtk_label(helptext[j], IMTK_AUTO, IMTK_AUTO);
			}
			imtk_set_color(IMTK_TEXT_COLOR, 0, 1, 0, 1);
		}
		imtk_set_color(IMTK_TEXT_COLOR, 1, 1, 1, 1);
	}

	if(show_con) {
		int i, idx, y;
		static float bgcol[] = {0, 0, 0, 0.4};

		y = CON_TOP;

		imtk_draw_rect(0, y, win_width, CON_HEIGHT, bgcol, bgcol);

		glColor4f(1, 1, 1, 0.7);

		idx = (con_tail_idx + CON_BUFLINES - con_scroll - CON_VISLINES) & (CON_BUFLINES - 1);
		for(i=0; i<5; i++) {
			y += 20;
			imtk_draw_string(5, y, conbuf[idx]);
			idx = (idx + 1) & (CON_BUFLINES - 1);
		}
	}

	imtk_end();
}

static void upd_clipdist(void)
{
	float px, py, pz, max_pan;

	px = fabs(cam_pan[0]);
	py = fabs(cam_pan[1]);
	pz = fabs(cam_pan[2]);
	max_pan = px > py ? (px > pz ? px : pz) : py;

	zfar = cam_dist + scene_size + max_pan;
	znear = zfar / 1000.0f;
	if(znear < 0.25f) znear = 0.25f;
	/*printf("using z bounds [%f, %f]\n", znear, zfar);*/
}

static void reshape(int x, int y)
{
	win_width = x;
	win_height = y;

	glViewport(0, 0, x, y);
	imtk_set_viewport(x, y);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, x, y, 0, -1, 1);

	fb_width = x / fbscale;
	fb_height = y / fbscale;

	free(fb_pixels);
	fb_pixels = malloc(fb_width * fb_height * sizeof *fb_pixels);

	{
		int i, j;
		uint16_t *pptr = fb_pixels;

		for(i=0; i<fb_height; i++) {
			for(j=0; j<fb_width; j++) {
				int xor = i ^ j;
				pptr[j] = PACK_RGB16(xor, xor, xor);
			}
			pptr += fb_width;
		}
	}

	glBindTexture(GL_TEXTURE_2D, fbtex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, fb_width, fb_height, 0, GL_RGB,
			GL_UNSIGNED_BYTE, 0);

	g3d_framebuffer(fb_width, fb_height, fb_pixels);

	upd_clipdist();
}

static void keypress(unsigned char key, int x, int y)
{
	switch(key) {
	case 27:
		exit(0);

	case '\t':
		show_ui ^= 1;
		glutPostRedisplay();
		break;

	case '`':
		show_con ^= 1;
		glutPostRedisplay();
		break;

	default:
		imtk_inp_key(key, IMTK_DOWN);
	}
}

static void keyrelease(unsigned char key, int x, int y)
{
	imtk_inp_key(key, IMTK_UP);
}

static void skeypress(int key, int x, int y)
{
	switch(key) {
	case GLUT_KEY_F1:
		show_help ^= 1;
		glutPostRedisplay();
		break;

	case GLUT_KEY_UP:
		scroll_console(1);
		break;
	case GLUT_KEY_DOWN:
		scroll_console(-1);
		break;

	case GLUT_KEY_PAGE_UP:
		scroll_console(CON_VISLINES);
		break;
	case GLUT_KEY_PAGE_DOWN:
		scroll_console(-CON_VISLINES);
		break;

	case GLUT_KEY_HOME:
		scroll_console(CON_BUFLINES);
		break;
	case GLUT_KEY_END:
		scroll_console(-CON_BUFLINES);
		break;

	default:
		imtk_inp_key(key, IMTK_DOWN);
	}
}

static void skeyrelease(int key, int x, int y)
{
	imtk_inp_key(key, IMTK_UP);
}

static void zoom(float dz)
{
	cam_dist += dz * sqrt(cam_dist);
	if(cam_dist < 1) cam_dist = 1;

	upd_clipdist();
	glutPostRedisplay();
}

static int drag_in_gui;

static void mouse(int bn, int st, int x, int y)
{
	int bidx = bn - GLUT_LEFT_BUTTON;
	int press = st == GLUT_DOWN;
	int over_con = 0;

	imtk_inp_mouse(bidx, press ? IMTK_DOWN : IMTK_UP);

	drag_in_gui = 0;
	if(show_con && y >= CON_TOP) {
		over_con = 1;
		drag_in_gui = 1;
	} else if(x >= uibox[0] && y >= uibox[1] && x < uibox[0] + uibox[2] && y < uibox[1] + uibox[3]) {
		drag_in_gui = 1;
	} else if(x >= bnbox[0] && y >= bnbox[1] && x < bnbox[0] + bnbox[2] && y < bnbox[1] + bnbox[3]) {
		drag_in_gui = 1;
	}

	prev_mx = x;
	prev_my = y;

	if(press) {
		bnstate |= 1 << bidx;

		if(!drag_in_gui) {
			switch(bidx) {
			case 3:
				zoom(-2);
				break;
			case 4:
				zoom(2);
				break;
			}
		} else if(over_con) {
			switch(bidx) {
			case 3:
				scroll_console(1);
				break;
			case 4:
				scroll_console(-1);
				break;
			}
		}

	} else {
		bnstate &= ~(1 << bidx);
	}
}

static void motion(int x, int y)
{
	int dx = x - prev_mx;
	int dy = y - prev_my;
	prev_mx = x;
	prev_my = y;

	imtk_inp_motion(x, y);

	if(drag_in_gui) return;
	if(!(dx | dy)) return;

	if(bnstate & 1) {
		cam_theta += dx * 0.5f;
		cam_phi += dy * 0.5f;

		if(cam_phi < -90) cam_phi = -90;
		if(cam_phi > 90) cam_phi = 90;
		glutPostRedisplay();
	}
	if(bnstate & 2) {
		float up[3], right[3];
		float theta = cam_theta * M_PI / 180.0f;
		float phi = cam_phi * M_PI / 180.0f;
		float speed = 0.01 * sqrt(cam_dist);

		up[0] = -sin(theta) * sin(phi);
		up[1] = -cos(phi);
		up[2] = cos(theta) * sin(phi);
		right[0] = cos(theta);
		right[1] = 0;
		right[2] = sin(theta);

		cam_pan[0] += (right[0] * dx + up[0] * dy) * speed;
		cam_pan[1] += up[1] * dy * speed;
		cam_pan[2] += (right[2] * dx + up[2] * dy) * speed;
		glutPostRedisplay();
	}
	if(bnstate & 4) {
		zoom(dy * 0.1f);
	}
}

static const char *usage_fmt = "Usage: %s [options] <scene file>\n"
	"Options:\n"
	" -h,-help: print usage and exit\n\n";

static int parse_args(int argc, char **argv)
{
	int i;

	for(i=1; i<argc; i++) {
		if(argv[i][0] == '-') {
			if(strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "-help") == 0) {
				printf(usage_fmt, argv[0]);
				exit(0);
			} else {
				fprintf(stderr, "invalid option: %s\n", argv[i]);
				return -1;
			}
		} else {
			if(scenefile) {
				fprintf(stderr, "unexpected argument: %s\n", argv[i]);
				return -1;
			}
			scenefile = argv[i];
		}
	}

	if(!scenefile) {
#ifdef __WIN32
		MessageBox(0, "You need to pass a scene file to open", "No scene specified",
				MB_ICONERROR | MB_OK);
#else
		fprintf(stderr, "You need to pass a scene file to open\n");
#endif
		return -1;
	}
	return 0;
}

static thrd_t con_thread;
static mtx_t con_mutex;
static int orig_fd1;

static int con_thread_func(void *cls)
{
	static char buf[1024];
	char *endl;
	int len, buflen;

#ifdef __WIN32
	DWORD rdsize;
	HANDLE prd = cls;
#else
	ssize_t rdsize;
	int prd = (intptr_t)cls;
#endif

	buflen = 0;
	for(;;) {
#ifdef __WIN32
		if(ReadFile(prd, buf + buflen, sizeof buf - buflen - 1, &rdsize, 0)) {
#else
		if((rdsize = read(prd, buf + buflen, sizeof buf - buflen - 1)) > 0) {
#endif
			endl = buf + buflen;
			buflen += rdsize;

			while(rdsize > 0) {
				if(*endl++ == '\n') {
					mtx_lock(&con_mutex);
					len = endl - buf;
					rdsize -= len;

					if(len > CON_LINELEN - 1) len = CON_LINELEN;
					memcpy(conbuf[con_tail_idx], buf, len);
					conbuf[con_tail_idx][len] = 0;
					con_tail_idx = (con_tail_idx + 1) & (CON_BUFLINES - 1);
					con_nlines++;
					mtx_unlock(&con_mutex);

					write(orig_fd1, buf, len);

					if(rdsize > 0) {
						memmove(buf, endl, rdsize);
						buflen = 0;
						endl = buf;
					}
				}
			}

			buflen = 0;
		}
	}

	return 0;
}

static void init_console(void)
{
#ifdef __WIN32
	HANDLE prd, pwr;

	if(!CreatePipe(&prd, &pwr, 0, 0)) {
		return;
	}
	SetStdHandle(STD_OUTPUT_HANDLE, pwr);
	SetStdHandle(STD_ERROR_HANDLE, pwr);
#else
	int pfd[2], prd;

	if(pipe(pfd) == -1) {
		return;
	}
	orig_fd1 = dup(1);
	close(1);
	close(2);
	dup(pfd[1]);
	dup(pfd[1]);
	prd = pfd[0];
#endif
	setvbuf(stdout, 0, _IOLBF, 0);
	setvbuf(stderr, 0, _IONBF, 0);

	mtx_init(&con_mutex, mtx_plain);

	thrd_create(&con_thread, con_thread_func, (void*)prd);
	thrd_detach(con_thread);
}

static void scroll_console(int dir)
{
	if(!show_con) return;

	if(dir > 0) {
		int max_scroll = (con_nlines >= CON_BUFLINES ? CON_BUFLINES : con_nlines) - CON_VISLINES;
		con_scroll = con_scroll + dir;
		if(con_scroll > max_scroll) {
			con_scroll = max_scroll;
		}
	} else {
		con_scroll = con_scroll + dir;
		if(con_scroll < 0) {
			con_scroll = 0;
		}
	}
	glutPostRedisplay();
}

/* needed by some of the demo code we're linking */
void demo_abort(void)
{
	abort();
}
