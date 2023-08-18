#include <string.h>
#include "scene.h"
#include "darray.h"
#include "util.h"
#include "goat3d.h"
#include "cgmath/cgmath.h"

void scn_init(struct g3d_scene *scn)
{
	scn->meshes = darr_alloc(0, sizeof *scn->meshes);
	scn->mtls = darr_alloc(0, sizeof *scn->mtls);
	scn->nodes = darr_alloc(0, sizeof *scn->nodes);
	scn->anims = darr_alloc(0, sizeof *scn->anims);
}

void scn_destroy(struct g3d_scene *scn)
{
	scn_clear(scn);
	darr_free(scn->meshes);
	darr_free(scn->mtls);
	darr_free(scn->nodes);
	darr_free(scn->anims);
}

struct g3d_scene *scn_create(void)
{
	struct g3d_scene *scn = malloc_nf(sizeof *scn);
	scn_init(scn);
	return scn;
}

void scn_free(struct g3d_scene *scn)
{
	if(!scn) return;
	scn_destroy(scn);
	free(scn);
}

void scn_clear(struct g3d_scene *scn)
{
	int i, num;

	num = darr_size(scn->meshes);
	for(i=0; i<num; i++) {
		free_mesh(scn->meshes[i]);
	}
	darr_clear(scn->meshes);

	num = darr_size(scn->mtls);
	for(i=0; i<num; i++) {
		if(scn->mtls[i]->texmap) {
			destroy_image(scn->mtls[i]->texmap);
		}
		if(scn->mtls[i]->envmap) {
			destroy_image(scn->mtls[i]->envmap);
		}
	}
	darr_clear(scn->mtls);

	num = darr_size(scn->nodes);
	for(i=0; i<num; i++) {
		scn_free_node(scn->nodes[i]);
	}
	darr_clear(scn->nodes);

	num = darr_size(scn->anims);
	for(i=0; i<num; i++) {
		scn_free_anim(scn->anims[i]);
	}
	darr_clear(scn->anims);
}

void scn_init_node(struct g3d_node *node)
{
	memset(node, 0, sizeof *node);
	cgm_midentity(node->xform);
	cgm_vcons(&node->scale, 1, 1, 1);
	cgm_qcons(&node->rot, 0, 0, 0, 1);
}

void scn_destroy_node(struct g3d_node *node)
{
	if(!node) return;
	free(node->name);
}

void scn_free_node(struct g3d_node *node)
{
	if(!node) return;
	scn_destroy_node(node);
	free(node);
}

void scn_init_anim(struct g3d_anim *anim)
{
	anim->name = 0;
	anim->tracks = darr_alloc(0, sizeof *anim->tracks);
}

void scn_destroy_anim(struct g3d_anim *anim)
{
	int i, num;

	if(!anim) return;
	free(anim->name);

	num = darr_size(anim->tracks);
	for(i=0; i<num; i++) {
		scn_destroy_track(anim->tracks[i]);
	}
	darr_free(anim->tracks);
}

void scn_free_anim(struct g3d_anim *anim)
{
	if(!anim) return;
	scn_destroy_anim(anim);
	free(anim);
}

void scn_init_track(struct g3d_track *track)
{
	track->trk = goat3d_create_track();
}

void scn_destroy_track(struct g3d_track *track)
{
	goat3d_destroy_track(track->trk);
}

void scn_add_mesh(struct g3d_scene *scn, struct g3d_mesh *mesh)
{
	darr_push(scn->meshes, &mesh);
}

void scn_add_mtl(struct g3d_scene *scn, struct g3d_material *mtl)
{
	darr_push(scn->mtls, &mtl);
}

void scn_add_node(struct g3d_scene *scn, struct g3d_node *node)
{
	darr_push(scn->nodes, &node);
}

void scn_add_anim(struct g3d_scene *scn, struct g3d_anim *anim)
{
	darr_push(scn->anims, &anim);
}

int scn_mesh_count(struct g3d_scene *scn)
{
	return darr_size(scn->meshes);
}

int scn_mtl_count(struct g3d_scene *scn)
{
	return darr_size(scn->mtls);
}

int scn_node_count(struct g3d_scene *scn)
{
	return darr_size(scn->nodes);
}

int scn_anim_count(struct g3d_scene *scn)
{
	return darr_size(scn->anims);
}

struct g3d_mesh *scn_find_mesh(struct g3d_scene *scn, const char *name)
{
	int i, num = darr_size(scn->meshes);
	for(i=0; i<num; i++) {
		if(strcmp(scn->meshes[i]->name, name) == 0) {
			return scn->meshes[i];
		}
	}
	return 0;
}

struct g3d_material *scn_find_mtl(struct g3d_scene *scn, const char *name)
{
	int i, num = darr_size(scn->mtls);
	for(i=0; i<num; i++) {
		if(strcmp(scn->mtls[i]->name, name) == 0) {
			return scn->mtls[i];
		}
	}
	return 0;
}

struct g3d_node *scn_find_node(struct g3d_scene *scn, const char *name)
{
	int i, num = darr_size(scn->nodes);
	for(i=0; i<num; i++) {
		if(strcmp(scn->nodes[i]->name, name) == 0) {
			return scn->nodes[i];
		}
	}
	return 0;
}

struct g3d_anim *scn_find_anim(struct g3d_scene *scn, const char *name)
{
	int i, num = darr_size(scn->anims);
	for(i=0; i<num; i++) {
		if(strcmp(scn->anims[i]->name, name) == 0) {
			return scn->anims[i];
		}
	}
	return 0;
}


int conv_goat3d_scene(struct g3d_scene *scn, struct goat3d *g)
{
	int i, num;
	long count, nfaces;
	struct g3d_mesh *mesh;
	struct g3d_material *mtl;
	struct g3d_node *node;
	struct g3d_anim *anim;

	printf("loaded goat3d scene: %s\n", goat3d_get_name(g));

	num = goat3d_get_mtl_count(g);
	for(i=0; i<num; i++) {
		mtl = malloc_nf(sizeof *mtl);
		if(conv_goat3d_mtl(mtl, goat3d_get_mtl(g, i)) != -1) {
			scn_add_mtl(scn, mtl);
		} else {
			free(mtl);
		}
	}

	count = nfaces = 0;
	num = goat3d_get_mesh_count(g);
	for(i=0; i<num; i++) {
		mesh = malloc_nf(sizeof *mesh);
		if(conv_goat3d_mesh(scn, mesh, goat3d_get_mesh(g, i)) != -1) {
			scn_add_mesh(scn, mesh);
			nfaces += mesh->icount / 3;
			count++;
		} else {
			free(mesh);
		}
	}
	printf(" - %ld meshes with %ld total polygons\n", count, nfaces);

	num = goat3d_get_node_count(g);
	for(i=0; i<num; i++) {
		node = malloc_nf(sizeof *node);
		if(conv_goat3d_node(scn, node, goat3d_get_node(g, i)) != -1) {
			scn_add_node(scn, node);
		} else {
			free(node);
		}
	}

	for(i=0; i<num; i++) {
		link_goat3d_node(scn, scn->nodes[i], goat3d_get_node(g, i));
	}

	count = 0;
	num = goat3d_get_anim_count(g);
	for(i=0; i<num; i++) {
		anim = malloc_nf(sizeof *anim);
		if(conv_goat3d_anim(scn, anim, goat3d_get_anim(g, i)) != -1) {
			scn_add_anim(scn, anim);
			count++;
		} else {
			free(anim);
		}
	}
	printf(" - %ld animations\n", count);

	return 0;
}

int conv_goat3d_mesh(struct g3d_scene *scn, struct g3d_mesh *dstmesh, struct goat3d_mesh *srcmesh)
{
	int i, nverts, nfaces;
	struct g3d_vertex *vdst;
	float *vsrc;
	int *idxsrc;
	uint16_t *idxdst;
	struct goat3d_material *gmtl;

	nverts = goat3d_get_mesh_vertex_count(srcmesh);
	nfaces = goat3d_get_mesh_face_count(srcmesh);
	if(!(nverts | nfaces)) return -1;

	init_mesh(dstmesh, G3D_TRIANGLES, nverts, nfaces * 3);
	dstmesh->name = strdup_nf(goat3d_get_mesh_name(srcmesh));
	vdst = dstmesh->varr;
	idxdst = dstmesh->iarr;

	for(i=0; i<nverts; i++) {
		vsrc = goat3d_get_mesh_attrib(srcmesh, GOAT3D_MESH_ATTR_VERTEX, i);
		vdst->x = vsrc[0];
		vdst->y = vsrc[1];
		vdst->z = vsrc[2];
		vdst->w = 1;

		if((vsrc = goat3d_get_mesh_attrib(srcmesh, GOAT3D_MESH_ATTR_NORMAL, i))) {
			vdst->nx = vsrc[0];
			vdst->ny = vsrc[1];
			vdst->nz = vsrc[2];
		} else {
			vdst->nx = vdst->ny = vdst->nz = 0;
		}

		if((vsrc = goat3d_get_mesh_attrib(srcmesh, GOAT3D_MESH_ATTR_TEXCOORD, i))) {
			vdst->u = vsrc[0];
			vdst->v = 1.0f - vsrc[1];
		} else {
			vdst->u = vdst->v = 0;
		}

		if((vsrc = goat3d_get_mesh_attrib(srcmesh, GOAT3D_MESH_ATTR_COLOR, i))) {
			vdst->r = (int)(vsrc[0] * 255.0f);
			vdst->g = (int)(vsrc[1] * 255.0f);
			vdst->b = (int)(vsrc[2] * 255.0f);
			vdst->a = (int)(vsrc[3] * 255.0f);
		} else {
			vdst->r = vdst->g = vdst->b = vdst->a = 255;
		}
		vdst++;
	}

	for(i=0; i<nfaces; i++) {
		idxsrc = goat3d_get_mesh_face(srcmesh, i);
		idxdst[0] = idxsrc[0];
		idxdst[1] = idxsrc[1];
		idxdst[2] = idxsrc[2];
		idxdst += 3;
	}

	if((gmtl = goat3d_get_mesh_mtl(srcmesh))) {
		dstmesh->mtl = scn_find_mtl(scn, goat3d_get_mtl_name(gmtl));
	}

	return 0;
}

int conv_goat3d_mtl(struct g3d_material *dstmtl, struct goat3d_material *srcmtl)
{
	const float *mattr;
	const char *str;

	init_g3dmtl(dstmtl);
	if((str = goat3d_get_mtl_name(srcmtl))) {
		dstmtl->name = strdup_nf(str);
	}

	if((mattr = goat3d_get_mtl_attrib(srcmtl, GOAT3D_MAT_ATTR_DIFFUSE))) {
		dstmtl->r = mattr[0];
		dstmtl->g = mattr[1];
		dstmtl->b = mattr[2];
	}
	if((mattr = goat3d_get_mtl_attrib(srcmtl, GOAT3D_MAT_ATTR_ALPHA))) {
		dstmtl->a = *mattr;
	}
	if((mattr = goat3d_get_mtl_attrib(srcmtl, GOAT3D_MAT_ATTR_SPECULAR))) {
		dstmtl->sr = mattr[0];
		dstmtl->sg = mattr[1];
		dstmtl->sb = mattr[2];
	}
	if((mattr = goat3d_get_mtl_attrib(srcmtl, GOAT3D_MAT_ATTR_SHININESS))) {
		dstmtl->shin = *mattr;
	}

	if((str = goat3d_get_mtl_attrib_map(srcmtl, GOAT3D_MAT_ATTR_DIFFUSE))) {
		dstmtl->texmap = malloc_nf(sizeof *dstmtl->texmap);
		if(load_image(dstmtl->texmap, str) == -1) {
			free(dstmtl->texmap);
			dstmtl->texmap = 0;
		}
	}

	return 0;
}

int conv_goat3d_node(struct g3d_scene *scn, struct g3d_node *dstnode, struct goat3d_node *srcnode)
{
	struct goat3d_mesh *gmesh;

	scn_init_node(dstnode);

	dstnode->name = strdup_nf(goat3d_get_node_name(srcnode));
	if(goat3d_get_node_type(srcnode) == GOAT3D_NODE_MESH) {
		if((gmesh = goat3d_get_node_object(srcnode))) {
			dstnode->mesh = scn_find_mesh(scn, goat3d_get_mesh_name(gmesh));
		}
	}

	goat3d_get_node_pivot(srcnode, &dstnode->pivot.x, &dstnode->pivot.y, &dstnode->pivot.z);
	goat3d_get_node_position(srcnode, &dstnode->pos.x, &dstnode->pos.y, &dstnode->pos.z);
	goat3d_get_node_rotation(srcnode, &dstnode->rot.x, &dstnode->rot.y, &dstnode->rot.z, &dstnode->rot.w);
	goat3d_get_node_scaling(srcnode, &dstnode->scale.x, &dstnode->scale.y, &dstnode->scale.z);
	goat3d_get_node_matrix(srcnode, dstnode->xform);
	dstnode->xform_valid = 1;
	return 0;
}

int link_goat3d_node(struct g3d_scene *scn, struct g3d_node *dstnode, struct goat3d_node *srcnode)
{
	int i, num;
	struct goat3d_node *gnode;
	struct g3d_node *node, *tail;

	if((gnode = goat3d_get_node_parent(srcnode))) {
		dstnode->parent = scn_find_node(scn, goat3d_get_node_name(gnode));
	}

	num = goat3d_get_node_child_count(srcnode);
	for(i=0; i<num; i++) {
		gnode = goat3d_get_node_child(srcnode, i);
		if((node = scn_find_node(scn, goat3d_get_node_name(gnode)))) {
			node->next = 0;
			if(dstnode->child) {
				tail->next = node;
				tail = node;
			} else {
				dstnode->child = tail = node;
			}
		}
	}

	return 0;
}

int conv_goat3d_anim(struct g3d_scene *scn, struct g3d_anim *dstanim, struct goat3d_anim *srcanim)
{
	int i, num;
	const char *str;
	struct g3d_track *trk;
	struct goat3d_track *gtrk;

	scn_init_anim(dstanim);

	if((str = goat3d_get_anim_name(srcanim))) {
		dstanim->name = strdup_nf(str);
	}

	dstanim->dur = goat3d_get_anim_timeline(srcanim, &dstanim->start, &dstanim->end);

	num = goat3d_get_anim_track_count(srcanim);
	for(i=0; i<num; i++) {
		gtrk = goat3d_get_anim_track(srcanim, i);

		trk = malloc_nf(sizeof *trk);
		if(conv_goat3d_track(scn, trk, gtrk) != -1) {
			darr_push(dstanim->tracks, &trk);
		}
	}

	return 0;
}

int conv_goat3d_track(struct g3d_scene *scn, struct g3d_track *dsttrk, struct goat3d_track *srctrk)
{
	int i, num;
	struct goat3d_key key;
	struct goat3d_node *gnode;
	const char *name;

	scn_init_track(dsttrk);

	if(!(gnode = goat3d_get_track_node(srctrk)) || !(name = goat3d_get_node_name(gnode))) {
		return -1;
	}
	if(!(dsttrk->node = scn_find_node(scn, name))) {
		fprintf(stderr, "conv_goat3d_track: failed to find node: %s\n", name);
		return -1;
	}

	if((name = goat3d_get_track_name(srctrk))) {
		goat3d_set_track_name(dsttrk->trk, name);
	}
	goat3d_set_track_type(dsttrk->trk, goat3d_get_track_type(srctrk));
	goat3d_set_track_interp(dsttrk->trk, goat3d_get_track_interp(srctrk));
	goat3d_set_track_extrap(dsttrk->trk, goat3d_get_track_extrap(srctrk));

	num = goat3d_get_track_key_count(srctrk);
	for(i=0; i<num; i++) {
		goat3d_get_track_key(srctrk, i, &key);
		goat3d_set_track_key(dsttrk->trk, &key);
	}
	return 0;
}

void scn_merge_anims(struct g3d_scene *scn)
{
	int i, j;
	struct g3d_anim *dest, *anim;
	long t0, t1;

	dest = scn->anims[0];
	for(i=1; i<darr_size(scn->anims); i++) {
		anim = scn->anims[i];
		for(j=0; j<darr_size(anim->tracks); j++) {
			struct g3d_track *trk = anim->tracks[j];

			darr_push(dest->tracks, &trk);

			goat3d_get_track_timeline(trk->trk, &t0, &t1);
			if(t0 < dest->start) dest->start = t0;
			if(t1 > dest->end) dest->end = t1;
		}

		free(anim->name);
		free(anim);
	}

	darr_resize(scn->anims, 1);
	dest->dur = dest->end - dest->start;
}

void scn_eval_anim(struct g3d_anim *anim, long tm)
{
	int i, num;
	struct g3d_track *trk;
	struct g3d_node *node;

	num = darr_size(anim->tracks);
	for(i=0; i<num; i++) {
		trk = anim->tracks[i];
		if(!(node = trk->node)) {
			printf("skip track without node\n");
			continue;
		}

		switch(goat3d_get_track_type(trk->trk)) {
		case GOAT3D_TRACK_POS:
			goat3d_get_track_vec3(trk->trk, tm, &node->pos.x, &node->pos.y, &node->pos.z);
			node->xform_valid = 0;
			break;
		case GOAT3D_TRACK_ROT:
			goat3d_get_track_quat(trk->trk, tm, &node->rot.x, &node->rot.y, &node->rot.z, &node->rot.w);
			cgm_qnormalize(&node->rot);
			node->xform_valid = 0;
			break;
		case GOAT3D_TRACK_SCALE:
			goat3d_get_track_vec3(trk->trk, tm, &node->scale.x, &node->scale.y, &node->scale.z);
			node->xform_valid = 0;
			break;
		default:
			break;
		}
	}
}

void scn_calc_node_matrix(struct g3d_node *node)
{
	int i;
	float rmat[16];
	float *mat = node->xform;

	cgm_mtranslation(mat, node->pivot.x, node->pivot.y, node->pivot.z);
	cgm_mrotation_quat(rmat, &node->rot);

	for(i=0; i<3; i++) {
		mat[i] = rmat[i];
		mat[4 + i] = rmat[4 + i];
		mat[8 + i] = rmat[8 + i];
	}

	mat[0] *= node->scale.x; mat[4] *= node->scale.y; mat[8] *= node->scale.z; mat[12] += node->pos.x;
	mat[1] *= node->scale.x; mat[5] *= node->scale.y; mat[9] *= node->scale.z; mat[13] += node->pos.y;
	mat[2] *= node->scale.x; mat[6] *= node->scale.y; mat[10] *= node->scale.z; mat[14] += node->pos.z;

	cgm_mpretranslate(mat, -node->pivot.x, -node->pivot.y, -node->pivot.z);

	/* that's basically: pivot * rotation * translation * scaling * -pivot */

	node->xform_valid = 1;
}

void scn_draw(struct g3d_scene *scn)
{
	int i, num;
	struct g3d_node *node;

	if(darr_empty(scn->nodes)) {
		num = darr_size(scn->meshes);
		for(i=0; i<num; i++) {
			draw_mesh(scn->meshes[i]);
		}
	} else {
		num = darr_size(scn->nodes);
		for(i=0; i<num; i++) {
			node = scn->nodes[i];
			if(node->parent) continue;

			scn_draw_node(node);
		}
	}
}

void scn_draw_node(struct g3d_node *node)
{
	struct g3d_node *n;

	if(!node->xform_valid) {
		scn_calc_node_matrix(node);
	}

	g3d_push_matrix();
	g3d_mult_matrix(node->xform);

	if(node->mesh) {
		draw_mesh(node->mesh);
	}

	n = node->child;
	while(n) {
		scn_draw_node(n);
		n = n->next;
	}

	g3d_pop_matrix();
}
