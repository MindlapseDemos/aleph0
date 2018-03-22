//#define BASE_STR
#define BASE_LEFTJ
//#define RIGHT_STR1
#define RIGHT_STR2
//#define LEFT_STR1
//#define LEFT_STR2
//#define LEFT_STR3
//#define LEFT_STR4

#define CON_RAD(r)	((r) * 1.4)
#define CON_WIDTH	0.04

static Object *gen_base_straight();
static Object *gen_base_left_junction();
static Object *gen_base_right_junction();
static Object *gen_pipeworks();
static Object *gen_pipe(float x, float y, float rad);
static Object *gen_pipe_inwall(float x, float y, float rad);
static Object *gen_pipe_s(float x, float y0, float y1, float rad);

static Mat4 xform;

#define add_object(o) \
	do { \
		Object *tmp = o; \
		if(head) { \
			tail->next = tmp; \
			tail = tmp; \
		} else { \
			head = tail = tmp; \
		} \
		while(tail->next) tail = tail->next; \
	} while(0)

extern "C" Object *generate()
{
	Object *head = 0, *tail = 0;

#ifdef BASE_STR
	add_object(gen_base_straight());
#endif
#ifdef BASE_LEFTJ
	add_object(gen_base_left_junction());
#endif
	add_object(gen_pipeworks());
	return head;
}

static Object *gen_base_straight()
{
	Object *owalls = new Object;
	owalls->mesh = new Mesh;
	gen_box(owalls->mesh, -1, -2, -1, 1, 2);

	xform.rotation_x(deg_to_rad(90));
	owalls->mesh->apply_xform(xform);

	owalls->mesh->remove_faces(16, 23);

	xform.translation(0, 0.5, 0);
	owalls->mesh->apply_xform(xform);

	return owalls;
}

static Object *gen_base_left_junction()
{
	Mesh tmp;

	Object *obj = new Object;
	obj->mesh = new Mesh;

	gen_box(obj->mesh, -1, -2, -1, 1, 3);

	xform.rotation_x(deg_to_rad(90));
	xform.translate(0, 0.5, 0);
	obj->mesh->apply_xform(xform);

	obj->mesh->remove_faces(24, 35);
	obj->mesh->remove_faces(6, 11);

	for(int i=0; i<2; i++) {
		float sign = i == 0 ? 1.0f : -1.0f;
		gen_plane(&tmp, 0.5, 1, 1, 1);
		xform.rotation_y(deg_to_rad(90));
		xform.translate(-0.5, 0.5, sign * 0.75);
		tmp.apply_xform(xform);
		obj->mesh->append(tmp);
	}

	gen_box(&tmp, -1, -0.5, -1, 1, 1);
	xform.rotation_z(deg_to_rad(90));
	xform.translate(-0.75, 0.5, 0);
	tmp.apply_xform(xform);

	tmp.remove_faces(8, 11);

	obj->mesh->append(tmp);

	return obj;
}

static Object *gen_pipeworks()
{
	Object *head = 0, *tail = 0;

	float start_y = CON_RAD(0.08) + 0.01;

#if defined(RIGHT_STR1) || defined(RIGHT_STR2)
	add_object(gen_pipe(0.5 - 0.08, start_y, 0.08));
#endif
	start_y += (CON_RAD(0.08) + CON_RAD(0.05)) * 0.9;

	for(int i=0; i<3; i++) {
		float x = 0.5 - CON_RAD(0.05);
		float y = start_y + i * (CON_RAD(0.05) * 1.8);

#ifdef RIGHT_STR2
		if(i == 1) {
			add_object(gen_pipe_inwall(x, y, 0.05));
		} else {
			add_object(gen_pipe(x, y, 0.05));
		}
#endif
#ifdef RIGHT_STR1
		add_object(gen_pipe(x, y, 0.05));
#endif
	}

#if defined(LEFT_STR1) || defined(LEFT_STR2) || defined(LEFT_STR3) || defined(LEFT_STR4)
	//add_object(gen_pipe(-0.5 + 0.08, start_y, 0.08));
	add_object(gen_pipe(-0.5 + CON_RAD(0.05), 0.8, 0.05));
#endif
#ifdef LEFT_STR1
	add_object(gen_pipe(-0.5 + CON_RAD(0.05), 0.68, 0.05));
#endif
#ifdef LEFT_STR2
	add_object(gen_pipe(-0.5 + CON_RAD(0.05), 0.3, 0.05));
#endif
#ifdef LEFT_STR3
	add_object(gen_pipe_s(-0.5 + CON_RAD(0.05), 0.3, 0.67, 0.05));
#endif
#ifdef LEFT_STR4
	add_object(gen_pipe_s(-0.5 + CON_RAD(0.05), 0.67, 0.3, 0.05));
#endif
	return head;
}

static Object *gen_pipe(float x, float y, float rad)
{
	Object *opipe = new Object;
	opipe->mesh = new Mesh;


	for(int i=0; i<2; i++) {
		Mesh tmp;

		float pipelen = 1 - CON_WIDTH * 2;
		gen_cylinder(&tmp, rad, pipelen, 6, 1);
		xform.translation(0, i - pipelen / 2 - CON_WIDTH, 0);
		tmp.apply_xform(xform);
		opipe->mesh->append(tmp);

		gen_cylinder(&tmp, CON_RAD(rad), CON_WIDTH, 7, 1, 1);
		xform.translation(0, 1 - i - CON_WIDTH / 2, 0);
		tmp.apply_xform(xform);
		opipe->mesh->append(tmp);

		gen_cylinder(&tmp, CON_RAD(rad), CON_WIDTH, 7, 1, 1);
		xform.translation(0, 1 - i - CON_WIDTH * 1.5 - pipelen, 0);
		tmp.apply_xform(xform);
		opipe->mesh->append(tmp);
	}

	xform.rotation_x(deg_to_rad(90));
	opipe->mesh->apply_xform(xform);
	opipe->xform.translation(x, y, 0);

	return opipe;
}

static Object *gen_pipe_inwall(float x, float y, float rad)
{
	Object *opipe = new Object;
	opipe->mesh = new Mesh;

	const float pipelen = 0.2;
	float zoffs = 1.0 - pipelen - CON_WIDTH;

	for(int i=0; i<2; i++) {
		Mesh tmp;
		float sign = i == 0 ? 1.0f : -1.0f;

		gen_torus(&tmp, rad * 2.0, rad, 4, 7, 0.25);
		xform = Mat4::identity;
		if(i > 0) xform.rotate_y(deg_to_rad(90));
		xform.translate(rad * 2.0, 0, sign * zoffs);
		tmp.apply_xform(xform);
		opipe->mesh->append(tmp);

		gen_cylinder(&tmp, rad, pipelen, 7, 1);
		xform.rotation_x(deg_to_rad(90));
		xform.rotate_z(deg_to_rad(90));
		xform.translate(0, 0, sign * (zoffs + pipelen / 2.0));
		tmp.apply_xform(xform);
		opipe->mesh->append(tmp);

		gen_cylinder(&tmp, CON_RAD(rad), CON_WIDTH, 7, 1, 1);
		xform.rotation_x(deg_to_rad(90));
		xform.translate(0, 0, sign * (1 - CON_WIDTH / 2.0));
		tmp.apply_xform(xform);
		opipe->mesh->append(tmp);
	}

	opipe->xform.translation(x, y, 0);

	return opipe;
}

static Object *gen_pipe_s(float x, float y0, float y1, float rad)
{
	Mesh tmp;
	float dist = fabs(y0 - y1);
	float pipelen = 1.0f - rad * 2.0 - CON_WIDTH / 2.0;

	Object *obj = new Object;
	obj->mesh = new Mesh;

	for(int i=0; i<2; i++) {
		float zsign = i == 0 ? 1.0f : -1.0f;
		float ysign = zsign * (y0 > y1 ? 1.0f : -1.0f);

		gen_torus(&tmp, rad * 2.0, rad, 4, 6, 0.25);
		xform.rotation_z(deg_to_rad(90) * ysign);
		if(i != 0) xform.rotate_x(deg_to_rad(y0 < y1 ? -90 : 90));
		xform.translate(0, ysign * (-dist / 2.0 + rad * 2.0), zsign * rad * 2.0);
		tmp.apply_xform(xform);
		obj->mesh->append(tmp);

		gen_cylinder(&tmp, rad, pipelen, 6, 1);
		xform.rotation_x(deg_to_rad(90));
		xform.translate(0, ysign * -dist / 2.0, zsign * (pipelen / 2.0 + rad * 2.0));
		tmp.apply_xform(xform);
		obj->mesh->append(tmp);

		gen_cylinder(&tmp, CON_RAD(rad), CON_WIDTH, 7, 1, 1);
		xform.rotation_x(deg_to_rad(90));
		xform.translate(0, ysign * -dist / 2.0, zsign * (1 - CON_WIDTH / 2.0));
		tmp.apply_xform(xform);
		obj->mesh->append(tmp);

		gen_cylinder(&tmp, CON_RAD(rad), CON_WIDTH, 7, 1, 1);
		xform.rotation_x(deg_to_rad(90));
		xform.translate(0, ysign * -dist / 2.0, zsign * (rad * 2.0 + CON_WIDTH / 2.0));
		tmp.apply_xform(xform);
		obj->mesh->append(tmp);
	}

	gen_cylinder(&tmp, rad, dist - rad * 4.0, 6, 1);
	obj->mesh->append(tmp);

	obj->xform.translation(x, (y0 + y1) / 2.0, 0);
	return obj;
}
