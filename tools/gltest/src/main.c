#include <stdio.h>
#include <stdlib.h>
#include <GL/glut.h>
#include "bsptree.h"
#include "mesh.h"

int init(void);
void cleanup(void);
void display(void);
void reshape(int x, int y);
void keydown(unsigned char key, int x, int y);
void mouse(int bn, int st, int x, int y);
void motion(int x, int y);

static float cam_theta, cam_phi, cam_dist = 5;
static int prev_x, prev_y;
static int bnstate[8];

/* ----------------------------------- */
static struct g3d_mesh torus;
static struct bsptree torus_bsp;

static int rebuild_bsp;
int debug_max_clip_level = 0;
/* ----------------------------------- */

int main(int argc, char **argv)
{
	glutInit(&argc, argv);
	glutInitWindowSize(800, 600);
	glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);
	glutCreateWindow("OpenGL test");

	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keydown);
	glutMouseFunc(mouse);
	glutMotionFunc(motion);

	if(init() == -1) {
		return 1;
	}
	atexit(cleanup);

	glutMainLoop();
	return 0;
}

int init(void)
{
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);

	gen_torus_mesh(&torus, 1.0, 0.25, 24, 12);

	init_bsp(&torus_bsp);
	if(bsp_add_mesh(&torus_bsp, &torus) == -1) {
		fprintf(stderr, "failed to construct torus BSP tree\n");
		return -1;
	}

	return 0;
}

void cleanup(void)
{
	free(torus.varr);
	free(torus.iarr);
	destroy_bsp(&torus_bsp);
}

static void draw_plane(struct bspnode *n)
{
	int i;
	float cx = 0, cy = 0, cz = 0;

	for(i=0; i<n->vcount; i++) {
		cx += n->verts[i].x;
		cy += n->verts[i].y;
		cz += n->verts[i].z;
	}
	cx /= n->vcount;
	cy /= n->vcount;
	cz /= n->vcount;

	glPushAttrib(GL_ENABLE_BIT);
	glDisable(GL_CULL_FACE);
	glDisable(GL_LIGHTING);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glPushMatrix();
	glTranslatef(cx, cy, cz);
	glScalef(10, 10, 10);
	glTranslatef(-cx, -cy, -cz);

	glBegin(GL_POLYGON);
	glColor4f(0.2, 0.3, 1.0, 0.5);
	for(i=0; i<n->vcount; i++) {
		glVertex3f(n->verts[i].x, n->verts[i].y, n->verts[i].z);
	}
	glEnd();

	glBegin(GL_LINE_LOOP);
	glColor4f(0.2, 0.3, 1.0, 0.85);
	for(i=0; i<n->vcount; i++) {
		glVertex3f(n->verts[i].x, n->verts[i].y, n->verts[i].z);
	}
	glEnd();

	glPopMatrix();
	glPopAttrib();
}

void display(void)
{
	float vdir[3];
	float mat[16];

	if(rebuild_bsp) {
		destroy_bsp(&torus_bsp);
		init_bsp(&torus_bsp);
		if(bsp_add_mesh(&torus_bsp, &torus) == -1) {
			fprintf(stderr, "failed to construct torus BSP tree\n");
			abort();
		}
		rebuild_bsp = 0;
	}

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0, 0, -cam_dist);
	glRotatef(cam_phi, 1, 0, 0);
	glRotatef(cam_theta, 0, 1, 0);

	glGetFloatv(GL_MODELVIEW_MATRIX, mat);
	vdir[0] = -mat[2];
	vdir[1] = -mat[6];
	vdir[2] = -mat[10];

	//g3d_draw_indexed(torus.prim, torus.varr, torus.vcount, torus.iarr, torus.icount);
	draw_bsp(&torus_bsp, vdir[0], vdir[1], vdir[2]);

	draw_plane(torus_bsp.root);

	glutSwapBuffers();
}

void reshape(int x, int y)
{
	glViewport(0, 0, x, y);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(50, (float)x / (float)y, 0.5, 500.0);
}

void keydown(unsigned char key, int x, int y)
{
	switch(key) {
	case 27:
		exit(0);

	case '=':
		debug_max_clip_level++;
		printf("max_clip_level: %d\n", debug_max_clip_level);
		rebuild_bsp = 1;
		glutPostRedisplay();
		break;

	case '-':
		if(--debug_max_clip_level < 0) {
			debug_max_clip_level = 0;
		} else {
			printf("max_clip_level: %d\n", debug_max_clip_level);
			rebuild_bsp = 1;
			glutPostRedisplay();
		}
		break;
	}
}

void mouse(int bn, int st, int x, int y)
{
	prev_x = x;
	prev_y = y;
	bnstate[bn - GLUT_LEFT] = st == GLUT_DOWN ? 1 : 0;
}

void motion(int x, int y)
{
	int dx = x - prev_x;
	int dy = y - prev_y;
	prev_x = x;
	prev_y = y;

	if(!dx && !dy) return;

	if(bnstate[0]) {
		cam_theta += dx * 0.5;
		cam_phi += dy * 0.5;

		if(cam_phi < -90) cam_phi = -90;
		if(cam_phi > 90) cam_phi = 90;
		glutPostRedisplay();
	}
	if(bnstate[2]) {
		cam_dist += dy * 0.1;

		if(cam_dist < 0.0f) cam_dist = 0.0f;
		glutPostRedisplay();
	}
}

/* dummy functions to allow linking with all the demo code */
void swap_buffers(void *buf)
{
}

unsigned int get_msec(void)
{
	return glutGet(GLUT_ELAPSED_TIME);
}

void wait_vsync(void)
{
}

void demo_quit(void)
{
}
