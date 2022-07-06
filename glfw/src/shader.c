#include "../include/shader.h"
#include "../include/glad/glad.h"
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


struct rps_shader__ {
    unsigned int program;
    float *vertices;
    size_t nvertices;
    GLchar *vertex_src;
    GLchar *fragment_src;
    unsigned int vbo;
    unsigned int vao;
};


static char *copy_src(const char *src)
{
    size_t len = strlen(src);
    GLchar *bfr = (GLchar *) malloc(sizeof (GLchar) * len + 1);

    if (!bfr) {
        printf("Failed to allocate memory for shader source!\n");
        abort();
    }

    strncpy(bfr, src, len);
    bfr[len] = '\0';

    return bfr;
}


rps_shader_t *
rps_shader_new(const char *vertex_src, const char *fragment_src,
               const float vertices[], size_t nvertices)
{

    rps_shader_t *ctx = (rps_shader_t *) malloc(sizeof *ctx);

    if (!ctx) {
        printf("Failed to allocate memory for shader!\n");
        abort();
    }

    ctx->program = 0;
    ctx->nvertices = nvertices;
    ctx->vertices = (float *) malloc(sizeof *ctx->vertices * nvertices);
    ctx->vbo = 0;
    ctx->vao = 0;

    if (!ctx->vertices) {
        printf("Failed to allocate memory for vertex array!\n");
        abort();
    }

    ctx->vertex_src = copy_src(vertex_src);
    ctx->fragment_src = copy_src(fragment_src);

    return ctx;
}


void
rps_shader_init(rps_shader_t *ctx)
{
    unsigned int vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    int success;
    char info_log[512];

    glShaderSource(vertex_shader, 1, &ctx->vertex_src, NULL);
    glCompileShader(vertex_shader);
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);

    if (!success) {
        glGetShaderInfoLog(vertex_shader, 512, NULL, info_log);
        printf("Vertex shader compilation failed: %s\n", info_log);
    }

    unsigned int fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &ctx->fragment_src, NULL);
    glCompileShader(fragment_shader);
    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);

    if (!success) {
        glGetShaderInfoLog(fragment_shader, 512, NULL, info_log);
        printf("Fragment compilation failed: %s\n", info_log);
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

    glGenVertexArrays(1, &ctx->vao);
    glGenBuffers(1, &ctx->vbo);
    glBindVertexArray(ctx->vao);

    glBindBuffer(GL_ARRAY_BUFFER, ctx->vbo);
    glBufferData(GL_ARRAY_BUFFER, ctx->nvertices, ctx->vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof (float), (void *) 0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}


void
rps_shader_render(const rps_shader_t *ctx)
{
    glUseProgram(ctx->program);
    glBindVertexArray(ctx->vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
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
