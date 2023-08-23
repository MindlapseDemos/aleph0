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

#ifndef GL_UNSIGNED_SHORT_5_6_5
#define GL_UNSIGNED_SHORT_5_6_5	0x8363
#endif

static int init(void);
static void cleanup(void);
static void display(void);
static void gui(void);
static void reshape(int x, int y);
static void keypress(unsigned char key, int x, int y);
static void keyrelease(unsigned char key, int x, int y);
static void mouse(int bn, int st, int x, int y);
static void motion(int x, int y);

static int parse_args(int argc, char **argv);

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

static int opt_lowres, opt_wireframe;
static int opt_lighting = 1;

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
	cam_dist = sqrt(dx * dx + dy * dy + dz * dz);
	printf("cam dist: %f\n", cam_dist);

	zfar = cam_dist * 2.0f;
	znear = zfar / 1000.0f;
	if(znear < 0.25f) znear = 0.25f;
	printf("using z bounds [%f, %f]\n", znear, zfar);

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
	int val;

	imtk_begin();

	glBegin(GL_QUADS);
	glColor3f(0.7, 0.7, 0.7);
	glVertex2f(0, 0);
	glVertex2f(0, 80);
	glVertex2f(128, 80);
	glVertex2f(128, 0);
	glEnd();

	imtk_layout_start(10, 0);
	imtk_layout_spacing(10);
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

	imtk_end();
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

	g3d_matrix_mode(G3D_PROJECTION);
	g3d_load_identity();
	g3d_perspective(50.0f, (float)fb_width / (float)fb_height, 1.0, 1000.0);
}

static void keypress(unsigned char key, int x, int y)
{
	switch(key) {
	case 27:
		exit(0);

	case '\t':
		opt_lowres ^= 1;
		fbscale = opt_lowres ? 3 : 1;
		reshape(win_width, win_height);
		glutPostRedisplay();
		break;

	case 'l':
		opt_lighting ^= 1;
		if(opt_lighting) {
			g3d_light_ambient(0.1, 0.1, 0.1);
		} else {
			g3d_light_ambient(1, 1, 1);
		}
		glutPostRedisplay();
		break;

	case 'w':
		opt_wireframe ^= 1;
		if(opt_wireframe) {
			g3d_disable(G3D_LIGHTING);
			g3d_polygon_mode(G3D_WIRE);
		} else {
			g3d_enable(G3D_LIGHTING);
			g3d_polygon_mode(G3D_GOURAUD);
		}
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

static void zoom(float dz)
{
	cam_dist += dz * sqrt(cam_dist);
	if(cam_dist < 1) cam_dist = 1;
	glutPostRedisplay();
}

static void mouse(int bn, int st, int x, int y)
{
	int bidx = bn - GLUT_LEFT_BUTTON;
	int press = st == GLUT_DOWN;

	if(press) {
		bnstate |= 1 << bidx;

		switch(bidx) {
		case 4:
			zoom(2);
			break;
		case 3:
			zoom(-2);
			break;
		}
	} else {
		bnstate &= ~(1 << bidx);
	}
	prev_mx = x;
	prev_my = y;

	imtk_inp_mouse(bidx, press ? IMTK_DOWN : IMTK_UP);
}

static void motion(int x, int y)
{
	int dx = x - prev_mx;
	int dy = y - prev_my;
	prev_mx = x;
	prev_my = y;

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

	imtk_inp_motion(x, y);
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

/* needed by some of the demo code we're linking */
void demo_abort(void)
{
	abort();
}
