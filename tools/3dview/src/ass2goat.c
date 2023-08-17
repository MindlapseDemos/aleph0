#include <stdio.h>
#include <stdlib.h>
#include "goat3d.h"
#include "cgmath/cgmath.h"
#include "assimp/cimport.h"
#include "assimp/postprocess.h"
#include "assimp/scene.h"

enum {
	CONV_SCENE,
	CONV_ANIM
};

void process_material(struct goat3d_material *mtl, struct aiMaterial *aimtl);
void process_mesh(struct goat3d *goat, struct goat3d_mesh *mesh, struct aiMesh *aimesh);
void process_node(struct goat3d *goat, struct goat3d_node *parent, struct aiNode *ainode);
int process_anim(struct goat3d *goat, struct aiAnimation *aianim);
static long assimp_time(const struct aiAnimation *anim, double aitime);

#define SCE_PPFLAGS	\
	(aiProcess_Triangulate | \
	 aiProcess_GenNormals | \
	 aiProcess_JoinIdenticalVertices | \
	 aiProcess_CalcTangentSpace | \
	 aiProcess_LimitBoneWeights | \
	 aiProcess_GenUVCoords)

#define ANM_PPFLAGS \
	(aiProcess_LimitBoneWeights)

struct goat3d *ass2goat(const char *infname)
{
	int i;
	const struct aiScene *aiscn;
	struct goat3d *goat;

	if(!(aiscn = aiImportFile(infname, SCE_PPFLAGS))) {
		fprintf(stderr, "failed to import %s\n", infname);
		return 0;
	}

	goat = goat3d_create();

	for(i=0; i<(int)aiscn->mNumMaterials; i++) {
		struct aiMaterial *aimat = aiscn->mMaterials[i];
		struct goat3d_material *mat = goat3d_create_mtl();

		process_material(mat, aimat);
		goat3d_add_mtl(goat, mat);
	}

	for(i=0; i<(int)aiscn->mNumMeshes; i++) {
		struct aiMesh *aimesh = aiscn->mMeshes[i];
		struct goat3d_mesh *mesh = goat3d_create_mesh();

		process_mesh(goat, mesh, aimesh);
		goat3d_add_mesh(goat, mesh);
	}

	for(i=0; i<(int)aiscn->mRootNode->mNumChildren; i++) {
		process_node(goat, 0, aiscn->mRootNode->mChildren[i]);
	}

	for(i=0; i<aiscn->mNumAnimations; i++) {
		if(process_anim(goat, aiscn->mAnimations[i]) == -1) {
			return 0;
		}
	}

	aiReleaseImport(aiscn);
	return goat;
}

void process_material(struct goat3d_material *mtl, struct aiMaterial *aimtl)
{
	struct aiString aistr;
	struct aiColor4D color;
	float val;

	if(aiGetMaterialString(aimtl, AI_MATKEY_NAME, &aistr) == aiReturn_SUCCESS) {
		goat3d_set_mtl_name(mtl, aistr.data);
	}

	if(aiGetMaterialColor(aimtl, AI_MATKEY_COLOR_DIFFUSE, &color) == aiReturn_SUCCESS) {
		goat3d_set_mtl_attrib3f(mtl, GOAT3D_MAT_ATTR_DIFFUSE, color.r, color.g, color.b);
	}

	if(aiGetMaterialColor(aimtl, AI_MATKEY_COLOR_SPECULAR, &color) == aiReturn_SUCCESS) {
		float sstr = 1.0;
		aiGetMaterialFloatArray(aimtl, AI_MATKEY_SHININESS_STRENGTH, &sstr, 0);
		goat3d_set_mtl_attrib3f(mtl, GOAT3D_MAT_ATTR_SPECULAR, color.r * sstr, color.g * sstr, color.b * sstr);
	}

	if(aiGetMaterialFloatArray(aimtl, AI_MATKEY_BUMPSCALING, &val, 0) == aiReturn_SUCCESS) {
		goat3d_set_mtl_attrib3f(mtl, GOAT3D_MAT_ATTR_BUMP, val, val, val);
	}

	if(aiGetMaterialFloatArray(aimtl, AI_MATKEY_REFLECTIVITY, &val, 0) == aiReturn_SUCCESS) {
		goat3d_set_mtl_attrib1f(mtl, GOAT3D_MAT_ATTR_REFLECTION, val);
	}

	if(aiGetMaterialFloatArray(aimtl, AI_MATKEY_OPACITY, &val, 0) == aiReturn_SUCCESS) {
		goat3d_set_mtl_attrib1f(mtl, GOAT3D_MAT_ATTR_TRANSMISSION, 1.0 - val);
	}

	if(aiGetMaterialString(aimtl, AI_MATKEY_TEXTURE_DIFFUSE(0), &aistr) == aiReturn_SUCCESS) {
		goat3d_set_mtl_attrib_map(mtl, GOAT3D_MAT_ATTR_DIFFUSE, aistr.data);
	}
	if(aiGetMaterialString(aimtl, AI_MATKEY_TEXTURE_SPECULAR(0), &aistr) == aiReturn_SUCCESS) {
		goat3d_set_mtl_attrib_map(mtl, GOAT3D_MAT_ATTR_SPECULAR, aistr.data);
	}
	if(aiGetMaterialString(aimtl, AI_MATKEY_TEXTURE_SHININESS(0), &aistr) == aiReturn_SUCCESS) {
		goat3d_set_mtl_attrib_map(mtl, GOAT3D_MAT_ATTR_SHININESS, aistr.data);
	}
	if(aiGetMaterialString(aimtl, AI_MATKEY_TEXTURE_NORMALS(0), &aistr) == aiReturn_SUCCESS) {
		goat3d_set_mtl_attrib_map(mtl, GOAT3D_MAT_ATTR_NORMAL, aistr.data);
	}
	if(aiGetMaterialString(aimtl, AI_MATKEY_TEXTURE_REFLECTION(0), &aistr) == aiReturn_SUCCESS) {
		goat3d_set_mtl_attrib_map(mtl, GOAT3D_MAT_ATTR_REFLECTION, aistr.data);
	}
	if(aiGetMaterialString(aimtl, AI_MATKEY_TEXTURE_OPACITY(0), &aistr) == aiReturn_SUCCESS) {
		/* TODO this is semantically inverted... maybe add an alpha attribute? */
		goat3d_set_mtl_attrib_map(mtl, GOAT3D_MAT_ATTR_TRANSMISSION, aistr.data);
	}
}

void process_mesh(struct goat3d *goat, struct goat3d_mesh *mesh, struct aiMesh *aimesh)
{
	int i, num_verts, num_faces;
	struct goat3d_material *mtl;

	if(aimesh->mName.length > 0) {
		goat3d_set_mesh_name(mesh, aimesh->mName.data);
	}

	if((mtl = goat3d_get_mtl(goat, aimesh->mMaterialIndex))) {
		goat3d_set_mesh_mtl(mesh, mtl);
	}

	num_verts = aimesh->mNumVertices;
	num_faces = aimesh->mNumFaces;

	for(i=0; i<num_verts; i++) {
		struct aiVector3D *v;
		struct aiColor4D *col;

		v = aimesh->mVertices + i;
		goat3d_add_mesh_attrib3f(mesh, GOAT3D_MESH_ATTR_VERTEX, v->x, v->y, v->z);

		if(aimesh->mNormals) {
			v = aimesh->mNormals + i;
			goat3d_add_mesh_attrib3f(mesh, GOAT3D_MESH_ATTR_NORMAL, v->x, v->y, v->z);
		}
		if(aimesh->mTangents) {
			v = aimesh->mTangents + i;
			goat3d_add_mesh_attrib3f(mesh, GOAT3D_MESH_ATTR_TANGENT, v->x, v->y, v->z);
		}
		if(aimesh->mTextureCoords[0]) {
			v = aimesh->mTextureCoords[0] + i;
			goat3d_add_mesh_attrib2f(mesh, GOAT3D_MESH_ATTR_TEXCOORD, v->x, v->y);
		}
		if(aimesh->mColors[0]) {
			col = aimesh->mColors[0] + i;
			goat3d_add_mesh_attrib4f(mesh, GOAT3D_MESH_ATTR_COLOR, col->r, col->g, col->b, col->a);
		}
		/* TODO: add bones */
	}

	for(i=0; i<num_faces; i++) {
		struct aiFace *face = aimesh->mFaces + i;

		goat3d_add_mesh_face(mesh, face->mIndices[0], face->mIndices[1], face->mIndices[2]);
	}
}

void process_node(struct goat3d *goat, struct goat3d_node *parent, struct aiNode *ainode)
{
	int i, j;
	struct goat3d_node *node;
	struct goat3d_mesh *mesh;
	cgm_vec3 vec;
	cgm_quat quat;
	float xform[16];
	ai_real *aimat;

	node = goat3d_create_node();
	goat3d_set_node_name(node, ainode->mName.data);

	if(parent) {
		goat3d_add_node_child(parent, node);
	}

	aimat = &ainode->mTransformation.a1;
	for(i=0; i<4; i++) {
		for(j=0; j<4; j++) {
			xform[j*4+i] = aimat[i*4+j];
		}
	}
	cgm_mget_translation(xform, &vec);
	goat3d_set_node_position(node, vec.x, vec.y, vec.z);
	cgm_mget_rotation(xform, &quat);
	cgm_qnormalize(&quat);
	goat3d_set_node_rotation(node, quat.x, quat.y, quat.z, quat.w);
	cgm_mget_scaling(xform, &vec);
	goat3d_set_node_scaling(node, vec.x, vec.y, vec.z);

	if(ainode->mNumMeshes) {

		if(!(mesh = goat3d_get_mesh(goat, ainode->mMeshes[0]))) {
			fprintf(stderr, "process_node: %s: invalid reference to mesh %d\n",
					goat3d_get_node_name(node), ainode->mMeshes[0]);
			goat3d_destroy_node(node);
			return;
		}
		goat3d_set_node_object(node, GOAT3D_NODE_MESH, mesh);

		if(ainode->mNumMeshes > 1) {
			char *name = alloca(strlen(ainode->mName.data) + 32);
			struct goat3d_node *cnode;

			fprintf(stderr, "process_node: %s has multiple meshes (%d), creating dummy child nodes\n",
					goat3d_get_node_name(node), ainode->mNumMeshes);

			for(i=1; i<ainode->mNumMeshes; i++) {
				sprintf(name, "%s-dummy%03d", ainode->mName.data, i);

				if(!(mesh = goat3d_get_mesh(goat, ainode->mMeshes[i]))) {
					fprintf(stderr, "process_node: %s(%d): skipping dummy for invalid mesh %d\n",
							goat3d_get_node_name(node), i, ainode->mMeshes[i]);
					continue;
				}

				cnode = goat3d_create_node();
				goat3d_add_node(goat, cnode);
				goat3d_add_node_child(node, cnode);

				goat3d_set_node_name(cnode, name);
				goat3d_set_node_object(cnode, GOAT3D_NODE_MESH, mesh);
			}
		}
	}

	for(i=0; i<ainode->mNumChildren; i++) {
		process_node(goat, node, ainode->mChildren[i]);
	}

	goat3d_add_node(goat, node);
}

int process_anim(struct goat3d *goat, struct aiAnimation *aianim)
{
	int i, j;
	struct goat3d_anim *anim;
	struct goat3d_node *node;
	struct goat3d_track *trk;
	const char *node_name;

	if(!(anim = goat3d_create_anim())) {
		fprintf(stderr, "failed to create animation\n");
		return -1;
	}
	goat3d_set_anim_name(anim, aianim->mName.length <= 0 ? "unnamed" : aianim->mName.data);

	/* for each animation "channel" ... */
	for(i=0; i<(int)aianim->mNumChannels; i++) {
		struct aiNodeAnim *ainodeanim = aianim->mChannels[i];

		/* find the node it refers to */
		node_name = ainodeanim->mNodeName.data;
		if(!(node = goat3d_get_node_by_name(goat, node_name))) {
			fprintf(stderr, "failed to process animation for unknown node: %s\n", node_name);
			goto err;
		}

		/* add all the keys ... */
		if(ainodeanim->mNumPositionKeys > 0) {
			if(!(trk = goat3d_create_track())) {
				fprintf(stderr, "failed to allocate position track\n");
				goto err;
			}
			goat3d_set_track_type(trk, GOAT3D_TRACK_POS);
			goat3d_set_track_node(trk, node);

			for(j=0; j<(int)ainodeanim->mNumPositionKeys; j++) {
				struct aiVectorKey *key = ainodeanim->mPositionKeys + j;
				long tm = assimp_time(aianim, key->mTime);
				goat3d_set_track_vec3(trk, tm, key->mValue.x, key->mValue.y, key->mValue.z);
			}
			goat3d_add_anim_track(anim, trk);
		}

		if(ainodeanim->mNumRotationKeys > 0) {
			if(!(trk = goat3d_create_track())) {
				fprintf(stderr, "failed to allocate rotation track\n");
				goto err;
			}
			goat3d_set_track_type(trk, GOAT3D_TRACK_ROT);
			goat3d_set_track_node(trk, node);

			for(j=0; j<(int)ainodeanim->mNumRotationKeys; j++) {
				struct aiQuatKey *key = ainodeanim->mRotationKeys + j;
				long tm = assimp_time(aianim, key->mTime);
				goat3d_set_track_quat(trk, tm, key->mValue.x, key->mValue.y, key->mValue.z, key->mValue.w);
			}
			goat3d_add_anim_track(anim, trk);
		}

		if(ainodeanim->mNumScalingKeys > 0) {
			if(!(trk = goat3d_create_track())) {
				fprintf(stderr, "failed to allocate scaling track\n");
				goto err;
			}
			goat3d_set_track_type(trk, GOAT3D_TRACK_SCALE);
			goat3d_set_track_node(trk, node);

			for(j=0; j<(int)ainodeanim->mNumScalingKeys; j++) {
				struct aiVectorKey *key = ainodeanim->mScalingKeys + j;
				long tm = assimp_time(aianim, key->mTime);
				goat3d_set_track_vec3(trk, tm, key->mValue.x, key->mValue.y, key->mValue.z);
			}
			goat3d_add_anim_track(anim, trk);
		}
	}

	goat3d_add_anim(goat, anim);
	return 0;

err:
	goat3d_destroy_anim(anim);
	return -1;
}

static long assimp_time(const struct aiAnimation *anim, double aitime)
{
	double sec;
	if(anim->mTicksPerSecond < 1e-6) {
		/* assume time in frames? */
		sec = aitime / 30.0;
	} else {
		sec = aitime / anim->mTicksPerSecond;
	}
	return (long)(sec * 1000.0);
}
