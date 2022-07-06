#ifndef __MISC_BASILE_GLFW_SHADER_H__
#define __MISC_BASILE_GLFW_SHADER_H__

#include <stddef.h>


#ifdef __cplusplus
extern "C" {
#endif


typedef struct rps_shader__ rps_shader_t;

extern rps_shader_t *rps_shader_new(const char *, const char *, const float[],
                                    size_t);
void rps_shader_init(rps_shader_t *);
extern void rps_shader_render(const rps_shader_t *);
extern void rps_shader_free(rps_shader_t **);


#ifdef __cplusplus
}
#endif

#endif // __MISC_BASILE_GLFW_SHADER_H__
