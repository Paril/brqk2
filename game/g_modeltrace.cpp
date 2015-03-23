#include "g_local.h"

typedef struct
{
	vec3_t v;
} vertex_t;

typedef struct
{
	vertex_t *vertices;
} frame_t;

typedef unsigned short vec3s_t[3];

typedef struct
{
	vec3s_t t;
} triangle_t;

struct model_s
{
	int num_frames;
	int num_vertices;
	frame_t *frames;

	int num_triangles;
	triangle_t *triangles;

	vec3_t mins, maxs;
};

typedef struct
{
	int ident;
	int version;
	int skinwidth;
	int skinheight;
	int framesize;
	int num_skins;
	int num_xyz;
	int num_st;
	int num_tris;
	int num_glcmds;
	int num_frames;
	int ofs_skins;
	int ofs_st;
	int ofs_tris;
	int ofs_frames;
	int ofs_glcmds;
	int ofs_end;
} md2_header_t;

typedef struct
{
	vec3_t scale;
	vec3_t translate;
	char name[16];
} md2_frame_header_t;

typedef struct
{
	byte v[3];
	byte lightnormalindex;
} md2_frame_vertex_t;

typedef struct
{
	const char *name;
	const char *path;
	model_t *model;
} model_to_convert_t;

static model_to_convert_t models[] =
{
	{ "berserk", "models/monsters/berserk/tris.md2", NULL },
	{ "bitch", "models/monsters/bitch/tris.md2", NULL },
	{ "blackwidow", "models/monsters/blackwidow/tris.md2", NULL },
	{ "blackwidow2", "models/monsters/blackwidow2/tris.md2", NULL },
	{ "boss1", "models/monsters/boss1/tris.md2", NULL },
	{ "boss2", "models/monsters/boss2/tris.md2", NULL },
	{ "jorg", "models/monsters/boss3/jorg/tris.md2", NULL },
	{ "rider", "models/monsters/boss3/rider/tris.md2", NULL },
	{ "brain", "models/monsters/brain/tris.md2", NULL },
	{ "carrier", "models/monsters/carrier/tris.md2", NULL },
	{ "fixbot", "models/monsters/fixbot/tris.md2", NULL },
	{ "flipper", "models/monsters/flipper/tris.md2", NULL },
	{ "float", "models/monsters/float/tris.md2", NULL },
	{ "flyer", "models/monsters/flyer/tris.md2", NULL },
	{ "gekk", "models/monsters/gekk/tris.md2", NULL },
	{ "gladiator", "models/monsters/gladiatr/tris.md2", NULL  },
	{ "gunner", "models/monsters/gunner/tris.md2", NULL },
	{ "hover", "models/monsters/hover/tris.md2", NULL },
	{ "infantry", "models/monsters/infantry/tris.md2", NULL },
	{ "insane", "models/monsters/insane/tris.md2", NULL },
	{ "medic", "models/monsters/medic/tris.md2", NULL },
	{ "mutant", "models/monsters/mutant/tris.md2", NULL },
	{ "parasite", "models/monsters/parasite/tris.md2", NULL },
	{ "soldier", "models/monsters/soldier/tris.md2", NULL },
	{ "stalker", "models/monsters/stalker/tris.md2", NULL },
	{ "tank", "models/monsters/tank/tris.md2", NULL },
	{ "turret", "models/monsters/turret/tris.md2", NULL },
	{ "explobox", "models/objects/barrels/tris.md2", NULL },
	{ "player", "players/male/tris.md2", NULL },
};

model_t *model_get(const char *name)
{
	for (int i = 0; i < sizeof(models) / sizeof(model_to_convert_t); ++i)
	{
		model_to_convert_t *convert = &models[i];

		if (strcmp(name, convert->name) == 0)
			return convert->model;
	}

	gi.dprintf("can't load collision model %s", name);
	return NULL;
}

void models_convert()
{
	for (int i = 0; i < sizeof(models) / sizeof(model_to_convert_t); ++i)
	{
		model_to_convert_t *convert = &models[i];

		if (convert->model)
			continue;

		convert->model = model_load(convert->name);

		if (!convert->model)
		{
			static char path[512];
			cvar_t	*game = gi.cvar("game", "", 0);
			sprintf(path, "%s/collision/%s.col", game->string, convert->name);

			convert->model = model_create(convert->path);
			model_save(convert->model, path);
		}
	}
}

model_t *model_create(const char *fileName)
{
	static char path[512];
	cvar_t	*game = gi.cvar("game", "", 0);
	int i;

	if (!*game->string)
		sprintf(path, "%s/%s", GAMEVERSION, fileName);
	else
		sprintf(path, "%s/%s", game->string, fileName);

	FILE *fp = fopen(path, "rb");
	model_t *model;
	md2_header_t md2_header;

	if (!fp)
		sprintf(path, "%s/%s", GAMEVERSION, fileName);

	fp = fopen(path, "rb");

	if (!fp)
		return NULL;

	model = (model_t*)malloc(sizeof(model_t));

	fread(&md2_header, sizeof(md2_header), 1, fp);

	model->num_triangles = md2_header.num_tris;
	model->num_vertices = md2_header.num_xyz;
	model->num_frames = md2_header.num_frames;

	model->triangles = (triangle_t*)malloc(sizeof(triangle_t) * model->num_triangles);
	model->frames = (frame_t*)malloc(sizeof(frame_t) * model->num_frames);

	ClearBounds(model->mins, model->maxs);

	// read tris
	fseek(fp, md2_header.ofs_tris, SEEK_SET);

	for (i = 0; i < md2_header.num_tris; ++i)
	{
		triangle_t *tri = &model->triangles[i];

		fread(tri->t, sizeof(vec3s_t), 1, fp);
		fseek(fp, sizeof(vec3s_t), SEEK_CUR);
	}

	// read frames
	fseek(fp, md2_header.ofs_frames, SEEK_SET);

	for (i = 0; i < md2_header.num_frames; ++i)
	{
		frame_t *frame = &model->frames[i];
		md2_frame_header_t md2_frame_header;
		int x;

		fread(&md2_frame_header, sizeof(md2_frame_header), 1, fp);

		frame->vertices = (vertex_t*)malloc(sizeof(vertex_t)* model->num_vertices);

		for (x = 0; x < model->num_vertices; ++x)
		{
			vertex_t *vertex = &frame->vertices[x];
			md2_frame_vertex_t md2_frame_vertex;
			int y;

			fread(&md2_frame_vertex, sizeof(md2_frame_vertex), 1, fp);

			for (y = 0; y < 3; ++y)
				vertex->v[y] = md2_frame_vertex.v[y] * md2_frame_header.scale[y] + md2_frame_header.translate[y];

			AddPointToBounds(vertex->v, model->mins, model->maxs);
		}
	}

	fclose(fp);
	return model;
}

#define FloatToUShort(f) ((unsigned short)(f * USHRT_MAX))
#define UShortToFloat(us) (((float)us) / USHRT_MAX)

model_t *model_load(const char *name)
{
	static char path[512];
	cvar_t	*game = gi.cvar("game", "", 0);
	int i;
	FILE *fp;
	model_t *model;
	vec3_t bounds;

	if (!*game->string)
		sprintf(path, "%s/collision/%s.col", GAMEVERSION, name);
	else
		sprintf(path, "%s/collision/%s.col", game->string, name);

	fp = fopen(path, "rb");

	if (!fp)
		sprintf(path, "%s/collision/%s.col", GAMEVERSION, name);

	fp = fopen(path, "rb");

	if (!fp)
		return NULL;

	model = (model_t*)malloc(sizeof(model_t));

	fread(model->mins, sizeof(vec3_t), 1, fp);
	fread(model->maxs, sizeof(vec3_t), 1, fp);

	VectorSubtract(model->maxs, model->mins, bounds);

	fread(&model->num_triangles, sizeof(int), 1, fp);
	fread(&model->num_frames, sizeof(int), 1, fp);
	fread(&model->num_vertices, sizeof(int), 1, fp);

	model->triangles = (triangle_t*)malloc(sizeof(triangle_t)* model->num_triangles);
	fread(model->triangles, sizeof(triangle_t), model->num_triangles, fp);

	model->frames = (frame_t*)malloc(sizeof(frame_t)* model->num_frames);

	for (i = 0; i < model->num_frames; ++i)
	{
		int v;
		model->frames[i].vertices = (vertex_t*)malloc(sizeof(vertex_t)* model->num_vertices);

		for (v = 0; v < model->num_vertices; ++v)
		{
			vertex_t *vertice = &model->frames[i].vertices[v];
			unsigned short compressed[3];
			int x;

			fread(compressed, sizeof(unsigned short), 3, fp);

			for (x = 0; x < 3; ++x)
				vertice->v[x] = (UShortToFloat(compressed[x]) * bounds[x]) + model->mins[x];
		}
	}

	fclose(fp);
	return model;
}

void model_save(model_t *model, const char *fileName)
{
	FILE *fp = fopen(fileName, "wb");
	int i;
	vec3_t bounds;

	fwrite(model->mins, sizeof(vec3_t), 1, fp);
	fwrite(model->maxs, sizeof(vec3_t), 1, fp);

	VectorSubtract(model->maxs, model->mins, bounds);

	fwrite(&model->num_triangles, sizeof(int), 1, fp);
	fwrite(&model->num_frames, sizeof(int), 1, fp);
	fwrite(&model->num_vertices, sizeof(int), 1, fp);

	fwrite(model->triangles, sizeof(triangle_t), model->num_triangles, fp);

	for (i = 0; i < model->num_frames; ++i)
	{
		int v;

		for (v = 0; v < model->num_vertices; ++v)
		{
			vertex_t *vertice = &model->frames[i].vertices[v];
			vec3_t compressed;
			int x;

			VectorSubtract(vertice->v, model->mins, compressed);
			
			for (x = 0; x < 3; ++x)
			{
				unsigned short asShort;

				compressed[x] /= bounds[x];

				if (compressed[x] < 0 || compressed[x] > 1)
					gi.dprintf("omg");

				asShort = FloatToUShort(compressed[x]);
				fwrite(&asShort, sizeof(unsigned short), 1, fp);
			}
		}
	}

	fclose(fp);
}

void R_AliasTransformVector(vec3_t in, vec3_t out, float xf[3][4])
{
	out[0] = DotProduct(in, xf[0]) + xf[0][3];
	out[1] = DotProduct(in, xf[1]) + xf[1][3];
	out[2] = DotProduct(in, xf[2]) + xf[2][3];
}

void model_debug(model_t *model, vec3_t origin, vec3_t angles)
{
	int i;
	float	aliasworldtransform[3][4];
	vec3_t s_alias_forward, s_alias_right, s_alias_up;

	AngleVectors(angles, s_alias_forward, s_alias_right, s_alias_up);
	memset(aliasworldtransform, 0, sizeof(aliasworldtransform));

	for (i=0; i<3; i++)
	{
		aliasworldtransform[i][0] =  s_alias_forward[i];
		aliasworldtransform[i][1] = -s_alias_right[i];
		aliasworldtransform[i][2] =  s_alias_up[i];
	}

	aliasworldtransform[0][3] = origin[0];
	aliasworldtransform[1][3] = origin[1];
	aliasworldtransform[2][3] = origin[2];

	for (i = 0; i < model->num_triangles / 8; ++i)
	{
		triangle_t *triangle = &model->triangles[i];
		vec3_t ao, bo, co;

		R_AliasTransformVector(model->frames[0].vertices[triangle->t[0]].v, ao, aliasworldtransform);
		R_AliasTransformVector(model->frames[0].vertices[triangle->t[1]].v, bo, aliasworldtransform);
		R_AliasTransformVector(model->frames[0].vertices[triangle->t[2]].v, co, aliasworldtransform);

		gi.WriteByte(svc_temp_entity);
		gi.WriteByte(TE_DEBUGTRAIL);
		gi.WritePosition(ao);
		gi.WritePosition(bo);
		gi.multicast(ao, MULTICAST_ALL);
	}
}

#define EPSILON 0.000001
#define CROSS(dest,v1,v2) \
	dest[0]=v1[1]*v2[2]-v1[2]*v2[1]; \
	dest[1]=v1[2]*v2[0]-v1[0]*v2[2]; \
	dest[2]=v1[0]*v2[1]-v1[1]*v2[0];
#define DOT(v1,v2) (v1[0]*v2[0]+v1[1]*v2[1]+v1[2]*v2[2])
#define SUB(dest,v1,v2)dest[0]=v1[0]-v2[0]; \
	dest[1]=v1[1]-v2[1]; \
	dest[2]=v1[2]-v2[2];
//#define TEST_CULL

int intersect_triangle(float orig[3], float dir[3], float vert0[3], float vert1[3], float vert2[3], float *t, float *u, float *v)
{
	double edge1[3], edge2[3], tvec[3], pvec[3], qvec[3];
	double det, inv_det;

	/* find vectors for two edges sharing vert0 */
	SUB(edge1, vert1, vert0);
	SUB(edge2, vert2, vert0);

	/* begin calculating determinant - also used to calculate U parameter */
	CROSS(pvec, dir, edge2);

	/* if determinant is near zero, ray lies in plane of triangle */
	det = DOT(edge1, pvec);

#ifdef TEST_CULL
	/* define TEST_CULL if culling is desired */
	if (det < EPSILON)
		return 0;

	/* calculate distance from vert0 to ray origin */
	SUB(tvec, orig, vert0);

	/* calculate U parameter and test bounds */
	*u = DOT(tvec, pvec);

	if (*u < 0.0 || *u > det)
		return 0;

	/* prepare to test V parameter */
	CROSS(qvec, tvec, edge1);

	/* calculate V parameter and test bounds */
	*v = DOT(dir, qvec);

	if (*v < 0.0 || *u + *v > det)
		return 0;

	/* calculate t, scale parameters, ray intersects triangle */
	*t = DOT(edge2, qvec);

	inv_det = 1.0 / det;
	*t *= inv_det;
	*u *= inv_det;
	*v *= inv_det;
#else
	/* the non-culling branch */
	if (det > -EPSILON && det < EPSILON)
		return 0;

	inv_det = 1.0 / det;

	/* calculate distance from vert0 to ray origin */
	SUB(tvec, orig, vert0);

	/* calculate U parameter and test bounds */
	*u = DOT(tvec, pvec) * inv_det;
	if (*u < 0.0 || *u > 1.0)
		return 0;

	/* prepare to test V parameter */
	CROSS(qvec, tvec, edge1);

	/* calculate V parameter and test bounds */
	*v = DOT(dir, qvec) * inv_det;

	if (*v < 0.0 || *u + *v > 1.0)
		return 0;

	/* calculate t, ray intersects triangle */
	*t = DOT(edge2, qvec) * inv_det;
#endif

	return 1;
}

#include <float.h>

bool model_trace(edict_t *ent, vec3_t start, vec3_t end, float *o, float *n)
{
	vec3_t dir;
	int i;
	float	aliasworldtransform[3][4];
	vec3_t s_alias_forward, s_alias_right, s_alias_up;
	float len;

	VectorSubtract(end, start, dir);
	len = VectorNormalize(dir);

	AngleVectors(ent->s.angles, s_alias_forward, s_alias_right, s_alias_up);
	memset(aliasworldtransform, 0, sizeof(aliasworldtransform));

	for (i=0; i<3; i++)
	{
		aliasworldtransform[i][0] =  s_alias_forward[i];
		aliasworldtransform[i][1] = -s_alias_right[i];
		aliasworldtransform[i][2] =  s_alias_up[i];
	}

	aliasworldtransform[0][3] = ent->s.origin[0];
	aliasworldtransform[1][3] = ent->s.origin[1];
	aliasworldtransform[2][3] = ent->s.origin[2];

	float t, lastT = FLT_MAX;
	bool succeeded = false;
	int numIntersects = 0;

	for (i = 0; i < ent->collision_model->num_triangles; ++i)
	{
		float u, v;
		int r;

		triangle_t *triangle = &ent->collision_model->triangles[i];
		vec3_t ao, bo, co;
		vertex_t *vertices = ent->collision_model->frames[ent->s.frame].vertices;

		R_AliasTransformVector(vertices[triangle->t[0]].v, ao, aliasworldtransform);
		R_AliasTransformVector(vertices[triangle->t[1]].v, bo, aliasworldtransform);
		R_AliasTransformVector(vertices[triangle->t[2]].v, co, aliasworldtransform);

		r = intersect_triangle(start, dir, ao, bo, co, &t, &u, &v);

		if (r)
		{
			if (t < lastT && t > 0 && t < len)
			{
				lastT = t;

				if (o)
					*o = t;

				if (n)
				{
					vec3_t s1, s2;
					VectorSubtract(vertices[triangle->t[1]].v, vertices[triangle->t[0]].v, s1);
					VectorSubtract(vertices[triangle->t[2]].v, vertices[triangle->t[0]].v, s2);
					CrossProduct(s1, s2, n);
					VectorNormalize(n);
					VectorInverse(n);
				}

				succeeded = true;
				++numIntersects;
			}
		}
	}

	return succeeded;
}