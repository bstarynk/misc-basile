#include "../include/shader.h"
#include "../include/glad/glad.h"
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>


struct rps_shader__ {
    unsigned int program;
};


rps_shader_t *
rps_shader_new(const char *vertex_src, const char *fragment_src)
{
    unsigned int vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    int success;
    char info_log[512];

    glShaderSource(vertex_shader, 1, &vertex_src, NULL);
    glCompileShader(vertex_shader);
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);

    if (!success) {
        glGetShaderInfoLog(vertex_shader, 512, NULL, info_log);
        printf("Vertex shader compilation failed: %s\n", info_log);
    }

    unsigned int fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_src, NULL);
    glCompileShader(fragment_shader);
    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);

    if (!success) {
        glGetShaderInfoLog(fragment_shader, 512, NULL, info_log);
        printf("Fragment compilation failed: %s\n", info_log);
    }

    rps_shader_t *ctx = (rps_shader_t *) malloc(sizeof *ctx);

    if (!ctx) {
        printf("Failed to allocate memory for shader!\n");
        abort();
    }

    ctx->program = glCreateProgram();
    glAttachShader(ctx->program, vertex_shader);
    glAttachShader(ctx->program, fragment_shader);
    glLinkProgram(ctx->program);
    glGetProgramiv(ctx->program, GL_LINK_STATUS, &success);

    if (!success) {
        glGetProgramInfoLog(ctx->program, 512, NULL, info_log);
        printf("Shader linking failed: %s\n", info_log);
    }

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    return ctx;
}


void
rps_shader_render(const rps_shader_t *ctx)
{
    glUseProgram(ctx->program);
}


void
rps_shader_close(rps_shader_t **ctx)
{
    rps_shader_t *s;

    if (ctx && (s = *ctx)) {
        glDeleteProgram(s->program);
        free(s);
    }

    *ctx = NULL;
}
