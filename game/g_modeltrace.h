typedef struct model_s model_t;

model_t *model_create(const char *filename);
model_t *model_load(const char *name);
void model_debug(model_t *model, vec3_t origin, vec3_t angles);
bool model_trace(edict_t *ent, vec3_t start, vec3_t end, float *o, float *n);
void model_save(model_t *model, const char *fileName);
void models_convert();
model_t *model_get(const char *name);