#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include "miniglut.h"
#include "gfxutil.h"
#include "3dgfx/3dgfx.h"

static int init(void);
static void cleanup(void);
static void display(void);
static void reshape(int x, int y);
static void keypress(unsigned char key, int x, int y);
static void mouse(int bn, int st, int x, int y);
static void motion(int x, int y);


uint16_t *fb_pixels;
int fb_width, fb_height;

static int win_width, win_height;

static float cam_theta, cam_phi, cam_dist = 8;
static unsigned int bnstate;
static int prev_mx, prev_my;
static int fbscale = 1;


int main(int argc, char **argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
	glutInitWindowSize(1280, 960);
	glutCreateWindow("3D scene previewer");

	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keypress);
	glutMouseFunc(mouse);
	glutMotionFunc(motion);

	if(init() == -1) {
		return 1;
	}
	atexit(cleanup);

	glutMainLoop();
	return 0;
}

static int init(void)
{
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glEnable(GL_TEXTURE_2D);

	if(g3d_init() == -1) {
		return -1;
	}
	g3d_polygon_mode(G3D_GOURAUD);

	return 0;
}

static void cleanup(void)
{
	g3d_destroy();
}

static void display(void)
{
	g3d_clear(G3D_COLOR_BUFFER_BIT | G3D_DEPTH_BUFFER_BIT);

	g3d_matrix_mode(G3D_MODELVIEW);
	g3d_load_identity();
	g3d_translate(0, 0, -cam_dist);
	g3d_rotate(cam_phi, 1, 0, 0);
	g3d_rotate(cam_theta, 0, 1, 0);

	g3d_begin(G3D_TRIANGLES);
	g3d_color3f(1, 0, 0);
	g3d_vertex(-0.5, -0.5, 0);
	g3d_color3f(0, 1, 0);
	g3d_vertex(0.5, -0.5, 0);
	g3d_color3f(0, 0, 1);
	g3d_vertex(0, 0.8, 0);
	g3d_end();

	/* present */
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

	glutSwapBuffers();
	assert(glGetError() == GL_NO_ERROR);
}

static void reshape(int x, int y)
{
	win_width = x;
	win_height = y;

	glViewport(0, 0, x, y);

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

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, fb_width, fb_height, 0, GL_RGB,
			GL_UNSIGNED_BYTE, 0);

	g3d_framebuffer(fb_width, fb_height, fb_pixels);

	g3d_matrix_mode(G3D_PROJECTION);
	g3d_load_identity();
	g3d_perspective(50.0f, (float)fb_width / (float)fb_height, 0.5, 500.0);
}

static void keypress(unsigned char key, int x, int y)
{
	switch(key) {
	case 27:
		exit(0);
	}
}

static void mouse(int bn, int st, int x, int y)
{
	int bidx = bn - GLUT_LEFT_BUTTON;
	int press = st == GLUT_DOWN;

	if(press) {
		bnstate |= 1 << bidx;
	} else {
		bnstate &= ~(1 << bidx);
	}
	prev_mx = x;
	prev_my = y;
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
	if(bnstate & 4) {
		cam_dist += dy * 0.1f;
		if(cam_dist < 0) cam_dist = 0;
		glutPostRedisplay();
	}
}


/* needed by some of the demo code we're linking */
void demo_abort(void)
{
	abort();
}
