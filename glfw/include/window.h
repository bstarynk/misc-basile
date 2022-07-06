#ifndef __MISC_BASILE_GLFW_WINDOW_H__
#define __MISC_BASILE_GLFW_WINDOW_H__

#include "shader.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef struct rps_window__ rps_window_t;


extern rps_window_t *rps_window_new(rps_shader_t *);
extern void rps_window_free(rps_window_t **);
extern void rps_window_run(rps_window_t *);


#ifdef __cplusplus
}
#endif

#endif /* !__MISC_BASILE_GLFW_WINDOW_H__ */
